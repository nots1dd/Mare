#include "Include/Colors.h"
#include "Include/Compiler.hpp"
#include "Include/Gen.hpp"
#include "Include/Parser.hpp"
#include "Include/PrimitiveTypes.hpp"
#include <iostream>
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
      case ';': // ignore top-level semicolons.
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
  // Initialize all targets (safe to call multiple times)
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  // Get host target triple
  std::string targetTriple = llvm::sys::getDefaultTargetTriple();
  TheModule->setTargetTriple(targetTriple);

  std::cout << "[*] Detected target triple: " << targetTriple << "\n";

  // Lookup the target
  std::string         error;
  const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
  if (!target)
  {
    llvm::errs() << "[!] Failed to lookup target: " << error << "\n";
    return nullptr;
  }

  // Get host CPU and feature set
  std::string cpu = llvm::sys::getHostCPUName().str();

  if (cpu.empty())
    cpu = __MARE_CPU_STANDARD__; // fallback to generic

  std::cout << "[*] Host CPU: " << cpu << "\n";
  llvm::TargetOptions  opt;
  llvm::TargetMachine* targetMachine =
    target->createTargetMachine(targetTriple, cpu, "", opt, Reloc::PIC_);

  if (!targetMachine)
  {
    llvm::errs() << "[!] Failed to create TargetMachine for triple: " << targetTriple << "\n";
    return nullptr;
  }

  TheModule->setDataLayout(targetMachine->createDataLayout());

  return targetMachine;
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

  auto TargetMachine = CreateHostTargetMachine();

  std::error_code EC;
  raw_fd_ostream  dest(__MARE_OBJECT_FILE_NAME__, EC, sys::fs::OF_None);

  if (EC)
  {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }

  legacy::PassManager pass;
  auto                FileType = CodeGenFileType::ObjectFile;

  if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType))
  {
    errs() << "TheTargetMachine can't emit a file of this type";
    return 1;
  }

  pass.run(*TheModule);
  dest.flush();

  outs() << COLOR_UNDERL << COLOR_BOLD << COLOR_GREEN
         << "-- Compiled to Object File: " << __MARE_OBJECT_FILE_NAME__ << "\n"
         << COLOR_RESET;

  return 0;
}
