#pragma once

#include "CmdLineParser.hpp"
#include "Compiler.hpp"
#include "ErrorHandling.hpp"
#include "PrimitiveTypes.hpp"
#include "Tokenizer.hpp"
#include <llvm/IR/DerivedTypes.h>

using namespace Mare::Err;

namespace Mare::Parser
{

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
static std::map<char, int> BinopPrecedence;

static auto ParseExpression() -> std::unique_ptr<Expr>;
static auto ParseBlock() -> std::unique_ptr<Expr>;
static auto ParseReturnExpr() -> std::unique_ptr<Expr>;

inline auto extractPrecedence() -> std::optional<unsigned>
{
  return std::visit(
    [](auto&& val) -> std::optional<unsigned>
    {
      using T = std::decay_t<decltype(val)>;

      if constexpr (std::is_integral_v<T>)
      {
        if (val >= 1 && val <= 100)
        {
          return static_cast<unsigned>(val);
        }
      }
      else if constexpr (std::is_floating_point_v<T>)
      {
        if (val >= 1.0 && val <= 100.0)
        {
          return static_cast<unsigned>(val);
        }
      }
      return std::nullopt;
    },
    Global::NumVal);
}

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static auto GetTokPrecedence() -> int
{
  if (!Tokenizer::IsCurTokAscii())
    return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[Tokenizer::CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}

/// numberexpr ::= number (must ensure that the CurTok is tok_number !!)
static auto ParseNumberExpr() -> std::unique_ptr<Expr>
{
  llvm::Type* numType = Tokenizer::assignDTypeToNumExpr();

  if (numType == nullptr)
    return LogError("Unknown numeric token type");

  auto Result = std::make_unique<NumberExpr>(Global::NumVal, numType);
  Tokenizer::getNextToken(); // consume the number

  return Result;
}

/// parenexpr ::= '(' expression ')'
static auto ParseParenExpr() -> std::unique_ptr<Expr>
{
  Tokenizer::getNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V)
    return nullptr;

  if (Tokenizer::CurTok != RIGHT_PAREN)
    return LogError("expected ')'");
  Tokenizer::getNextToken(); // eat ).
  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static auto ParseIdentifierExpr() -> std::unique_ptr<Expr>
{
  std::string IdName = Global::IdentifierStr;

  Tokenizer::getNextToken(); // eat identifier.

  if (Tokenizer::CurTok != LEFT_PAREN) // Simple variable ref.
    return std::make_unique<VariableExpr>(IdName);

  // Call.
  Tokenizer::getNextToken(); // eat (
  std::vector<std::unique_ptr<Expr>> Args;
  if (Tokenizer::CurTok != RIGHT_PAREN)
  {
    while (true)
    {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (Tokenizer::CurTok == RIGHT_PAREN)
        break;

      if (Tokenizer::CurTok != ARG_DELIM_PROTO)
        return LogError("Expected ')' or ',' in argument list.");
      Tokenizer::getNextToken();
    }
  }

  // Eat the ')'.
  Tokenizer::getNextToken();

  return std::make_unique<CallExpr>(IdName, std::move(Args));
}

/// ifexpr ::= 'if' expression 'then' expression 'else' expression
static auto ParseIfExpr() -> std::unique_ptr<Expr>
{
  Tokenizer::getNextToken(); // eat the if.

  // condition.
  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;

  if (Tokenizer::CurTok != tok_then)
    return LogError("Expected the keyword \"then\".");
  Tokenizer::getNextToken(); // eat the then

  auto Then = ParseExpression();
  if (!Then)
    return nullptr;

  if (Tokenizer::CurTok != tok_else)
    return LogError("Expected the keyword \"else\".");

  Tokenizer::getNextToken();

  auto Else = ParseExpression();
  if (!Else)
    return nullptr;

  return std::make_unique<IfExpr>(std::move(Cond), std::move(Then), std::move(Else));
}

/// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
static auto ParseForExpr() -> std::unique_ptr<Expr>
{
  Tokenizer::getNextToken(); // eat the for.

  if (Tokenizer::CurTok != tok_identifier)
    return LogError("Expected identifier after 'for'.");

  std::string IdName = Global::IdentifierStr;
  Tokenizer::getNextToken(); // eat identifier.

  if (Tokenizer::CurTok != '=')
    return LogError("Expected '=' after 'for'.");
  Tokenizer::getNextToken(); // eat '='.

  auto Start = ParseExpression();
  if (!Start)
    return nullptr;
  if (Tokenizer::CurTok != ',')
    return LogError("Expected ',' after for start value.");
  Tokenizer::getNextToken();

  auto End = ParseExpression();
  if (!End)
    return nullptr;

  // The step value is optional.
  std::unique_ptr<Expr> Step;
  if (Tokenizer::CurTok == ',')
  {
    Tokenizer::getNextToken();
    Step = ParseExpression();
    if (!Step)
      return nullptr;
  }

  if (Tokenizer::CurTok != tok_in)
    return LogError("Expected 'in' after for");
  Tokenizer::getNextToken(); // eat 'in'.

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

  return std::make_unique<ForExpr>(IdName, std::move(Start), std::move(End), std::move(Step),
                                   std::move(Body));
}

/// varexpr ::= 'var' identifier ('=' expression)?
//                    (',' identifier ('=' expression)?)* 'in' expression
static auto ParseVarExpr() -> std::unique_ptr<Expr>
{
  Tokenizer::getNextToken(); // eat the var.

  // At least one variable name is required.
  if (Tokenizer::CurTok != tok_identifier)
    return LogError("Expected identifier after 'var'.");

  std::string VarName = Global::IdentifierStr;
  Tokenizer::getNextToken();

  if (Tokenizer::CurTok != '=')
    return LogError("Expected '=' after variable name");

  Tokenizer::getNextToken(); // eat '='

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

  return std::make_unique<VarExpr>(VarName, std::move(Body));
}

static auto ParseStringExpr() -> std::unique_ptr<Expr>
{
  const std::string ProcessedStr = Util::ProcessString(); // check for escape sequences

  auto Result = std::make_unique<StringExpr>(ProcessedStr);
  Tokenizer::getNextToken(); // Consume the string token
  return Result;
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
///   ::= ifexpr
///   ::= forexpr
///   ::= varexpr
static auto ParsePrimary() -> std::unique_ptr<Expr>
{
  switch (Tokenizer::CurTok)
  {
    default:
      return LogError("Unknown token when expecting an expression");
    case tok_identifier:
      return ParseIdentifierExpr();
    case tok_number:
      return ParseNumberExpr();
    case '(':
      return ParseParenExpr();
    case tok_if:
      return ParseIfExpr();
    case tok_for:
      return ParseForExpr();
    case tok_string:
      return ParseStringExpr();
    case tok_var:
      return ParseVarExpr();
  }
}

/// unary
///   ::= primary
///   ::= '!' unary
static auto ParseUnary() -> std::unique_ptr<Expr>
{
  // If the current token is not an operator, it must be a primary expr.
  if (Tokenizer::IsCurTokPrimaryExpr())
    return ParsePrimary();

  // If this is a unary operator, read it.
  int Opc = Tokenizer::CurTok;
  Tokenizer::getNextToken();
  if (auto Operand = ParseUnary())
    return std::make_unique<UnaryExpr>(Opc, std::move(Operand));
  return nullptr;
}

/// binoprhs
///   ::= ('+' unary)*
static auto ParseBinOpRHS(int ExprPrec, std::unique_ptr<Expr> LHS) -> std::unique_ptr<Expr>
{
  // If this is a binop, find its precedence.
  while (true)
  {
    int TokPrec = GetTokPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec)
      return LHS;

    // Okay, we know this is a binop.
    int BinOp = Tokenizer::CurTok;
    Tokenizer::getNextToken(); // eat binop

    // Parse the unary expression after the binary operator.
    auto RHS = ParseUnary();
    if (!RHS)
      return nullptr;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec)
    {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }

    // Merge LHS/RHS.
    LHS = std::make_unique<BinaryExpr>(BinOp, std::move(LHS), std::move(RHS));
  }
}

/// expression
///   ::= unary binoprhs
///
static auto ParseExpression() -> std::unique_ptr<Expr>
{
  if (Tokenizer::CurTok == tok_ret)
    return ParseReturnExpr();

  auto LHS = ParseUnary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}

static auto ParseBlock() -> std::unique_ptr<Expr>
{
  std::vector<std::unique_ptr<Expr>> Exprs;

  while (true)
  {
    // End of file or new function, break
    if (Tokenizer::IsCurTokOverBlock())
    {
      Tokenizer::getNextToken();
      break;
    }

    auto Expr = ParseExpression();
    if (!Expr)
      return nullptr;

    Exprs.push_back(std::move(Expr));

    // Optional semicolon (skip if you don't require it)
    if (Tokenizer::CurTok == STATEMENT_DELIM)
      Tokenizer::getNextToken();
  }

  return std::make_unique<BlockExpr>(std::move(Exprs));
}

static auto ParseTypedArgument() -> std::optional<std::pair<std::string, llvm::Type*>>
{
  llvm::Type* ArgType = MARE_DOUBLE_TYPE;

  switch (Tokenizer::CurTok)
  {
    case tok_double:
      break;
    case tok_float:
      ArgType = MARE_FLOAT_TYPE;
      break;
    case tok_int64:
      ArgType = MARE_INT64_TYPE;
      break;
    case tok_int32:
      ArgType = MARE_INT32_TYPE;
      break;
    case tok_int16:
      ArgType = MARE_INT16_TYPE;
      break;
    case tok_int8:
      ArgType = MARE_INT8_TYPE;
      break;
    case tok_string:
      ArgType = MARE_STRPTR_TYPE;
      break;
    case tok_identifier:
      break;
    default:
      return LogErrorP("Unexpected token in argument list"), std::nullopt;
  }

  if (Tokenizer::CurTok != tok_identifier)
    Tokenizer::getNextToken(); // Eat the type token

  if (Tokenizer::CurTok != tok_identifier)
    return LogErrorP("Expected argument name after type"), std::nullopt;

  std::string name = Global::IdentifierStr;
  Tokenizer::getNextToken(); // Eat identifier
  return std::make_pair(name, ArgType);
}

/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER number? (id, id)
///   ::= unary LETTER (id)
static auto ParsePrototype() -> std::unique_ptr<Prototype>
{
  llvm::Type* RetType = MARE_VOID_TYPE;
  std::string FnName;
  unsigned    Kind = 0, BinaryPrecedence = 30;

  switch (Tokenizer::CurTok)
  {
    case tok_identifier:
      FnName = Global::IdentifierStr;
      Kind   = 0;
      Tokenizer::getNextToken();
      break;

    case tok_unary:
      Tokenizer::getNextToken();
      if (!Tokenizer::IsCurTokAscii())
        return LogErrorP("Expected unary operator");
      FnName = __MARE_UNARY_FUNC_DECL__ + std::string(1, Tokenizer::CurTokChar());
      Kind   = 1;
      Tokenizer::getNextToken();
      break;

    case tok_binary:
      Tokenizer::getNextToken();
      if (!Tokenizer::IsCurTokAscii())
        return LogErrorP("Expected binary operator");
      FnName = __MARE_BINARY_FUNC_DECL__ + std::string(1, Tokenizer::CurTokChar());
      Kind   = 2;
      Tokenizer::getNextToken();
      if (Tokenizer::CurTok == tok_number)
      {
        auto maybePrec = extractPrecedence();
        if (!maybePrec)
          return LogErrorP("Invalid precedence: must be 1..100");
        BinaryPrecedence = *maybePrec;
        Tokenizer::getNextToken();
      }
      break;

    default:
      return LogErrorP("Expected function name in prototype");
  }

  if (Tokenizer::CurTok != LEFT_PAREN)
    return LogErrorP("Expected '(' in prototype");

  std::vector<std::string> ArgNames;
  std::vector<llvm::Type*> ArgTypes;
  Tokenizer::getNextToken(); // eat '('

  while (Tokenizer::TokenIsValidArg())
  {
    auto maybeArg = ParseTypedArgument();
    if (!maybeArg)
      return nullptr;

    ArgNames.push_back(maybeArg->first);
    ArgTypes.push_back(maybeArg->second);

    if (Tokenizer::CurTok == ',')
      Tokenizer::getNextToken(); // eat ','
  }

  if (Tokenizer::CurTok != RIGHT_PAREN)
    return LogErrorP("Expected ')' in argument decl");

  Tokenizer::getNextToken(); // eat ')'

  if (Tokenizer::CurTok == tok_arrow)
  {
    Tokenizer::getNextToken(); // consume the arrow
    RetType = Util::ParseReturnTypeProto(Tokenizer::CurTok);
    if (RetType == nullptr)
    {
      LogErrorP("Expected return type after '->'");
      return nullptr;
    }
    Tokenizer::getNextToken(); // eat return type
  }

  if (Kind && ArgNames.size() != Kind)
    return LogErrorP("Invalid number of operands for operator");

  return std::make_unique<Prototype>(FnName, ArgNames, ArgTypes, RetType, Kind != 0,
                                     BinaryPrecedence);
}

/// definition ::= 'fn' prototype expression
static auto ParseDefinition() -> std::unique_ptr<FunctionalAST>
{
  Tokenizer::getNextToken(); // eat def
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  if (Tokenizer::CurTok != '{')
  {
    LogError("Expected '{' to start function body");
    return nullptr;
  }

  Tokenizer::getNextToken(); // consume '{'

  if (auto E = ParseBlock())
    return std::make_unique<FunctionalAST>(std::move(Proto), std::move(E));
  return nullptr;
}

/// toplevelexpr ::= expression
static auto ParseTopLevelExpr() -> std::unique_ptr<FunctionalAST>
{
  if (auto E = ParseExpression())
  {
    // Determine the return type based on the expression.
    llvm::Type* RetType = MARE_VOID_TYPE; // Default to void

    // Make an anonymous prototype with the return type.
    auto Proto = std::make_unique<Prototype>("__anon_expr", std::vector<std::string>(),
                                             std::vector<llvm::Type*>(), RetType);
    return std::make_unique<FunctionalAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

static auto ParseReturnExpr() -> std::unique_ptr<Expr>
{
  Tokenizer::getNextToken(); // consume 'return'

  // Support optional return expression (e.g., `return;`)
  if (Tokenizer::CurTok == ';' || Tokenizer::CurTok == tok_eof)
    return std::make_unique<ReturnExpr>(nullptr);

  // Parse the return value expression directly without going through ParseExpression
  auto LHS = ParseUnary();
  if (!LHS)
    return nullptr;
  auto RetExpr = ParseBinOpRHS(0, std::move(LHS));
  if (!RetExpr)
    return nullptr;
  return std::make_unique<ReturnExpr>(std::move(RetExpr));
}

/// external ::= 'extern' prototype
static auto ParseExtern() -> std::unique_ptr<Prototype>
{
  Tokenizer::getNextToken(); // Consume 'extern'

  if (Tokenizer::CurTok != tok_identifier)
    return LogErrorP("Expected function name after 'extern'");

  return ParsePrototype();
}

} // namespace Mare::Parser
