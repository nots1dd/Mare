#include "Include/Colors.h"
#include "Include/Compiler.hpp"
#include "Include/Gen.hpp"
#include "Include/Parser.hpp"

using namespace GooAST;

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
    if (auto* FnIR = FnAST->codegen())
    {
      fprintf(stderr, "%s%s -- Read function definition: %s\n", COLOR_UNDERL, COLOR_BLUE,
              COLOR_RESET);
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

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

auto main() -> int
{
  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; // highest.

  // Prime the first token.
  getNextToken();

  InitializeModuleAndPassManager();

  // Run the main "interpreter loop" now.
  MainLoop();

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
