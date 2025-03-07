#pragma once

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#define __GOO_OBJECT_FILE_NAME__ "Output.o"
#define __GOO_CPU_STANDARD__     "generic"

#define NEWLINE_CHAR '\n'

using namespace llvm;
using namespace llvm::sys;

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static std::unique_ptr<LLVMContext>                  TheContext;
static std::unique_ptr<Module>                       TheModule;
static std::unique_ptr<IRBuilder<>>                  Builder;
static std::map<std::string, AllocaInst*>            NamedValues;
static std::unique_ptr<FunctionPassManager>          TheFPM;
static std::unique_ptr<LoopAnalysisManager>          TheLAM;
static std::unique_ptr<FunctionAnalysisManager>      TheFAM;
static std::unique_ptr<CGSCCAnalysisManager>         TheCGAM;
static std::unique_ptr<ModuleAnalysisManager>        TheMAM;
static std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
static std::unique_ptr<StandardInstrumentations>     TheSI;
static ExitOnError                                   ExitOnErr;

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token
{
  tok_eof = -1,

  // commands
  tok_def    = -2,
  tok_extern = -3,

  // primary
  tok_identifier = -4,
  tok_number     = -5,

  // control
  tok_if   = -6,
  tok_then = -7,
  tok_else = -8,
  tok_for  = -9,
  tok_in   = -10,

  // operators
  tok_binary = -11,
  tok_unary  = -12,

  // var definition
  tok_var    = -13,
  tok_string = -14,
  tok_void   = -15,
  tok_double = -16,
  tok_arrow  = -17
};
