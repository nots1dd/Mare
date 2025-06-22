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
#include <cassert>
#include <map>
#include <memory>
#include <string>

#define __MARE_OBJECT_FILE_NAME__    "MareCompilerOutput.o"
#define __MARE_ASM_FILE_NAME__       "MareCompilerOutput.asm"
#define __MARE_CPU_STANDARD__        "generic"
#define __MARE_FILE_EXTENSION_STEM__ ".mare"

//===----------------------------------------------------------------------===//
// Escape Sequences (useful for parser)
//===----------------------------------------------------------------------===//

#define ESCAPE_SEQUENCE_NEWLINE       '\n' // Line Feed (LF)
#define ESCAPE_SEQUENCE_CARRIAGE_RET  '\r' // Carriage Return (CR)
#define ESCAPE_SEQUENCE_TAB           '\t' // Horizontal Tab
#define ESCAPE_SEQUENCE_VERTICAL_TAB  '\v' // Vertical Tab
#define ESCAPE_SEQUENCE_BACKSPACE     '\b' // Backspace
#define ESCAPE_SEQUENCE_FORMFEED      '\f' // Form feed (new page)
#define ESCAPE_SEQUENCE_ALERT         '\a' // Bell / Alert
#define ESCAPE_SEQUENCE_BACKSLASH     '\\' // Backslash
#define ESCAPE_SEQUENCE_SINGLE_QUOTE  '\''
#define ESCAPE_SEQUENCE_DOUBLE_QUOTE  '\"'
#define ESCAPE_SEQUENCE_QUESTION_MARK '\?' // Literal question mark (avoids trigraphs)

// Optional: macro for null terminator
#define ESCAPE_SEQUENCE_NULL '\0'

#define LEFT_PAREN        '('
#define RIGHT_PAREN       ')'
#define STATEMENT_DELIM   ';'
#define BLOCK_SCOPE_BEGIN '{'
#define BLOCK_SCOPE_END   '}'
#define ARG_DELIM_PROTO   ',' // (a, b) --> comma is the argument delimiter in the prototype

//===----------------------------------------------------------------------===//
// Unary and Binary Function Decl
//===----------------------------------------------------------------------===//

#define __MARE_UNARY_FUNC_DECL__  "_mare_std_unary"
#define __MARE_BINARY_FUNC_DECL__ "_mare_std_binary"

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
  tok_error = 0,
  tok_eof   = -1,

  // commands
  tok_def    = -2,
  tok_extern = -3,
  tok_grab   = -24,

  // primary
  tok_identifier = -4,
  tok_number     = -5,

  // control
  tok_if   = -6,
  tok_then = -7,
  tok_else = -8,
  tok_for  = -9,
  tok_in   = -10,
  tok_ret  = -11,

  // operators
  tok_binary = -12,
  tok_unary  = -13,

  // var definition
  tok_var    = -14,
  tok_string = -15,
  tok_void   = -16,
  tok_double = -17,
  tok_float  = -18,
  tok_int8   = -19,
  tok_int16  = -20,
  tok_int32  = -21,
  tok_int64  = -22,
  tok_arrow  = -23
};
