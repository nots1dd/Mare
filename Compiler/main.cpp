#include "Include/Colors.h"
#include "Include/Compiler.hpp"
#include "Include/Gen.hpp"
// #include "Include/Grab.hpp"
#include "Include/Parser.hpp"
#include "Include/PrimitiveTypes.hpp"
#include <iostream>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Program.h> // for ExecuteAndWait
#include <llvm/TargetParser/Host.h>

using namespace Mare;

static bool foundMain = false;

//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

static void InitializeModuleAndPassManager()
{
  // Open a new module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule  = std::make_unique<Module>("Mare", *TheContext);

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);
}

static void HandleDefinition()
{
  if (auto FnAST = Parser::ParseDefinition())
  {
    if (FnAST->getName() == "main" && FnAST->getReturnType() == MARE_VOID_TYPE)
    {
      foundMain = true;
    }
    if (auto* FnIR = FnAST->codegen())
    {
      std::cout << COLOR_UNDERL << COLOR_BLUE << "-- Function decl:" << COLOR_RESET << std::endl;
      FnIR->print(errs());
      fprintf(stderr, "\n");
    }
  }
  else
  {
    // Skip token for error recovery.
    Tokenizer::getNextToken();
  }
}

static void HandleExtern()
{
  if (auto ProtoAST = Parser::ParseExtern())
  {
    if (auto* FnIR = ProtoAST->codegen())
    {
      fprintf(stderr, "Read extern: ");
      FnIR->print(errs());
      FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
    }
  }
  else
  {
    // Skip token for error recovery.
    Tokenizer::getNextToken();
  }
}

static void HandleTopLevelExpression()
{
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = Parser::ParseTopLevelExpr())
  {
    FnAST->codegen();
  }
  else
  {
    // Skip token for error recovery.
    Tokenizer::getNextToken();
  }
}

/// top ::= definition | external | expression | ';'
static void MainLoop()
{
  while (true)
  {
    switch (Tokenizer::CurTok)
    {
      case tok_eof:
        return;
      case STATEMENT_DELIM: // ignore top-level semicolons.
        Tokenizer::getNextToken();
        break;
      case tok_def:
        HandleDefinition();
        break;
      case tok_extern:
        HandleExtern();
        break;
      default:
        HandleTopLevelExpression();
        break;
    }
  }
}

void SetPrecedence()
{
  // Install standard binary operators.
  // 1 is lowest precedence.
  Parser::BinopPrecedence['<'] = 10;
  Parser::BinopPrecedence['>'] = 10;
  Parser::BinopPrecedence['+'] = 20;
  Parser::BinopPrecedence['-'] = 20;
  Parser::BinopPrecedence['*'] = 40; // highest.
  Parser::BinopPrecedence['/'] = 50;
}

inline auto CreateHostTargetMachine() -> llvm::TargetMachine*
{
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  std::string targetTriple = llvm::sys::getDefaultTargetTriple();
  TheModule->setTargetTriple(targetTriple);
  std::cout << "[*] Detected target triple: " << targetTriple << "\n";

  std::string         error;
  const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
  if (!target)
  {
    llvm::errs() << "[!] Failed to lookup target: " << error << "\n";
    return nullptr;
  }

  std::string cpu = llvm::sys::getHostCPUName().str();
  if (cpu.empty())
    cpu = __MARE_CPU_STANDARD__;

  std::cout << "[*] Host CPU: " << cpu << "\n";

  if (mareArgs.showCPUFeatures)
  {
    // Feature detection
    llvm::StringMap<bool> hostFeatures = llvm::sys::getHostCPUFeatures();
    std::string           features;
    for (const auto& f : hostFeatures)
    {
      if (f.second)
        features += f.first().str() + ",";
    }
    if (!features.empty())
      features.pop_back(); // Remove trailing comma

    std::cout << "[*] CPU features: " << features << "\n";
  }

  llvm::TargetOptions opt;
  opt.AllowFPOpFusion      = llvm::FPOpFusion::Fast;
  opt.UnsafeFPMath         = true;
  opt.NoInfsFPMath         = true;
  opt.NoNaNsFPMath         = true;
  opt.MCOptions.AsmVerbose = true;
  opt.EnableFastISel       = true;

  llvm::TargetMachine* targetMachine =
    target->createTargetMachine(targetTriple, cpu, "", opt, Reloc::PIC_);

  if (!targetMachine)
  {
    llvm::errs() << "[!] Failed to create TargetMachine\n";
    return nullptr;
  }

  TheModule->setDataLayout(targetMachine->createDataLayout());
  std::cout << "[*] DataLayout: " << TheModule->getDataLayout().getStringRepresentation() << "\n";

  return targetMachine;
}

static auto AddOptimizationsAndEmitObjectFile() -> bool
{
  auto TargetMachine = CreateHostTargetMachine();

  std::error_code EC;
  raw_fd_ostream  dest(__MARE_OBJECT_FILE_NAME__, EC, sys::fs::OF_None);

  if (EC)
  {
    llvm::errs() << "[!] Could not open output file: " << EC.message() << "\n";
    return false;
  }

  // --- Set up the new pass manager ---
  llvm::LoopAnalysisManager     loopAM;
  llvm::FunctionAnalysisManager functionAM;
  llvm::CGSCCAnalysisManager    cgsccAM;
  llvm::ModuleAnalysisManager   moduleAM;

  llvm::PassBuilder PB(TargetMachine);

  // Register required analyses
  PB.registerModuleAnalyses(moduleAM);
  PB.registerCGSCCAnalyses(cgsccAM);
  PB.registerFunctionAnalyses(functionAM);
  PB.registerLoopAnalyses(loopAM);
  PB.crossRegisterProxies(loopAM, functionAM, cgsccAM, moduleAM);

  // Create the optimization pipeline at -O3
  llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);

  // Run the pass pipeline
  MPM.run(*TheModule, moduleAM);

  // Emit object file
  llvm::legacy::PassManager codeGenPass;
  if (TargetMachine->addPassesToEmitFile(codeGenPass, dest, nullptr, CodeGenFileType::ObjectFile))
  {
    llvm::errs() << "[!] TargetMachine can't emit file of this type\n";
    return false;
  }

  codeGenPass.run(*TheModule);
  dest.flush();

  return true;
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

auto main(int argc, char* argv[]) -> int
{

  if (!mareArgs.parse(argc, argv))
  {
    return 1;
  }

  SetPrecedence();

  // Prime the first token.
  Tokenizer::getNextToken();

  InitializeModuleAndPassManager();

  // Run the main "interpreter loop" now.
  MainLoop();

  fprintf(stderr, "%s%s%s%s: \n", COLOR_BOLD, COLOR_UNDERL, mareArgs.inputFile.c_str(),
          COLOR_RESET);

  if (!foundMain)
  {
    PRINT_ERROR("Missing required 'main' function entry point.");
    PRINT_HINT("Define a top-level function: fn main() -> void");
    return 1;
  }

  AddOptimizationsAndEmitObjectFile();

  outs() << COLOR_UNDERL << COLOR_BOLD << COLOR_GREEN
         << "-- Compiled to Object File: " << __MARE_OBJECT_FILE_NAME__ << "\n"
         << COLOR_RESET;

  return 0;
}
