#include "Include/CmdLineParser.hpp"
#include "Include/Colors.h"
#include "Include/Compiler.hpp"
#include "Include/Gen.hpp"
#include "Include/Parser.hpp"
#include <iostream>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Program.h> // for ExecuteAndWait

using namespace GooAST;

static bool foundMain = false;

//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

static void InitializeModuleAndPassManager()
{
  // Open a new module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule  = std::make_unique<Module>("Goo", *TheContext);

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);
}

static void HandleDefinition()
{
  if (auto FnAST = ParseDefinition())
  {
    if (FnAST->getName() == "main" && FnAST->getReturnType() == llvm::Type::getVoidTy(*TheContext))
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
    getNextToken();
  }
}

static void HandleExtern()
{
  if (auto ProtoAST = ParseExtern())
  {
    if (auto* FnIR = ProtoAST->codegen())
    {
      fprintf(stderr, "Read extern: ");
      FnIR->print(errs());
      fprintf(stderr, "\n");
      FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
    }
  }
  else
  {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleTopLevelExpression()
{
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = ParseTopLevelExpr())
  {
    FnAST->codegen();
  }
  else
  {
    // Skip token for error recovery.
    getNextToken();
  }
}

/// top ::= definition | external | expression | ';'
static void MainLoop()
{
  while (true)
  {
    switch (CurTok)
    {
      case tok_eof:
        return;
      case ';': // ignore top-level semicolons.
        getNextToken();
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
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; // highest.
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

auto main(int argc, char* argv[]) -> int
{

  if (!gooArgs.parse(argc, argv))
  {
    return 1;
  }

  SetPrecedence();

  // Prime the first token.
  getNextToken();

  InitializeModuleAndPassManager();

  // Run the main "interpreter loop" now.
  MainLoop();

  fprintf(stderr, "%s%s%s: \n", COLOR_BOLD, gooArgs.inputFile.c_str(), COLOR_RESET);

  if (!foundMain)
  {
    PRINT_ERROR("Missing required 'main' function entry point.");
    PRINT_HINT("Define a top-level function: fn main() -> void");
    return 1;
  }

  // Initialize the target registry etc.
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();

  auto TargetTriple = sys::getDefaultTargetTriple();
  TheModule->setTargetTriple(TargetTriple);

  std::string Error;
  auto        Target = TargetRegistry::lookupTarget(TargetTriple, Error);

  // Print an error and exit if we couldn't find the requested target.
  // This generally occurs if we've forgotten to initialise the
  // TargetRegistry or we have a bogus target triple.
  if (!Target)
  {
    errs() << Error;
    return 1;
  }

  auto Features = "";

  TargetOptions opt;
  auto          TheTargetMachine =
    Target->createTargetMachine(TargetTriple, __GOO_CPU_STANDARD__, Features, opt, Reloc::PIC_);

  TheModule->setDataLayout(TheTargetMachine->createDataLayout());

  std::error_code EC;
  raw_fd_ostream  dest(__GOO_OBJECT_FILE_NAME__, EC, sys::fs::OF_None);

  if (EC)
  {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }

  legacy::PassManager pass;
  auto                FileType = CodeGenFileType::ObjectFile;

  if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType))
  {
    errs() << "TheTargetMachine can't emit a file of this type";
    return 1;
  }

  pass.run(*TheModule);
  dest.flush();

  outs() << COLOR_UNDERL << COLOR_BOLD << COLOR_GREEN
         << "-- Compiled to Object File: " << __GOO_OBJECT_FILE_NAME__ << "\n"
         << COLOR_RESET;

  return 0;
}
