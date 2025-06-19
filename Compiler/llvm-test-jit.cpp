#include "Include/AST.hpp"
#include "Include/Compiler.hpp"
#include "Include/Gen.hpp"
#include "Include/Parser.hpp"
#include "JIT/JIT.hpp" // Include JIT header
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

using namespace GooAST;
using namespace llvm;
using namespace llvm::orc;

std::unique_ptr<GooJIT> TheJIT; // JIT Engine

//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

static void InitializeModuleAndManagers()
{
  // Open a new context and module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule  = std::make_unique<Module>("GooJIT", *TheContext);
  TheModule->setDataLayout(TheJIT->getDataLayout());

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  // Create new pass and analysis managers.
  TheFPM  = std::make_unique<FunctionPassManager>();
  TheLAM  = std::make_unique<LoopAnalysisManager>();
  TheFAM  = std::make_unique<FunctionAnalysisManager>();
  TheCGAM = std::make_unique<CGSCCAnalysisManager>();
  TheMAM  = std::make_unique<ModuleAnalysisManager>();
  ThePIC  = std::make_unique<PassInstrumentationCallbacks>();
  TheSI   = std::make_unique<StandardInstrumentations>(*TheContext,
                                                       /*DebugLogging*/ true);
  TheSI->registerCallbacks(*ThePIC, TheMAM.get());

  // Add transform passes.
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->addPass(InstCombinePass());
  // Reassociate expressions.
  TheFPM->addPass(ReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->addPass(GVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->addPass(SimplifyCFGPass());

  // Register analysis passes used in these transform passes.
  PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM);
  PB.registerFunctionAnalyses(*TheFAM);
  PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);
}

static void HandleDefinition()
{
  if (auto FnAST = ParseDefinition())
  {
    if (auto* FnIR = FnAST->codegen())
    {
      fprintf(stderr, "Read function definition:");
      FnIR->print(errs());
      fprintf(stderr, "\n");
      ExitOnErr(TheJIT->addModule(ThreadSafeModule(std::move(TheModule), std::move(TheContext))));
      InitializeModuleAndManagers();
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
    if (FnAST->codegen())
    {
      // Create a ResourceTracker to track JIT'd memory allocated to our
      // anonymous expression -- that way we can free it after executing.
      auto RT = TheJIT->getMainJITDylib().createResourceTracker();

      auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
      ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
      InitializeModuleAndManagers();

      // Search the JIT for the __anon_expr symbol.
      auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));

      // Get the symbol's address and cast it to the right type (takes no
      // arguments, returns a double) so we can call it as a native function.
      auto ExprAddr  = ExprSymbol.getAddress();
      double (*FP)() = (double (*)())ExprAddr.toPtr<void*>();
      fprintf(stderr, "Evaluated to %f\n", FP());

      // Delete the anonymous expression module from the JIT.
      ExitOnErr(RT->remove());
    }
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
  fprintf(stderr, "ready> ");
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
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; // highest.

  // Prime the first token.
  fprintf(stderr, "ready> ");
  getNextToken();

  TheJIT = ExitOnErr(GooJIT::Create());

  InitializeModuleAndManagers();

  // Run the main "interpreter loop" now.
  MainLoop();

  return 0;
}
