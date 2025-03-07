#pragma once

#include "AST.hpp"
#include "Colors.h"
#include "Compiler.hpp"

using namespace GooAST;

static std::string IdentifierStr; // Filled in if tok_identifier
static double      NumVal;        // Filled in if tok_number
static std::string StringVal;
static bool        isExtern = false;

/// gettok - Return the next token from standard input.
static auto gettok() -> int
{
  static int LastChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar))
    LastChar = getchar();

  // Handle string literals
  if (LastChar == '"')
  {
    StringVal = "";
    while ((LastChar = getchar()) != '"' && LastChar != EOF)
    {
      StringVal += LastChar;
    }

    if (LastChar == EOF)
      return tok_eof;

    LastChar = getchar(); // Consume closing quote
    return tok_string;
  }

  // Handle identifiers and keywords
  if (isalpha(LastChar))
  { // identifier: [a-zA-Z][a-zA-Z0-9]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getchar())))
      IdentifierStr += LastChar;

    if (IdentifierStr == "fn")
      return tok_def;
    if (IdentifierStr == "extern")
      return tok_extern;
    if (IdentifierStr == "if")
      return tok_if;
    if (IdentifierStr == "then")
      return tok_then;
    if (IdentifierStr == "else")
      return tok_else;
    if (IdentifierStr == "for")
      return tok_for;
    if (IdentifierStr == "in")
      return tok_in;
    if (IdentifierStr == "binary")
      return tok_binary;
    if (IdentifierStr == "unary")
      return tok_unary;
    if (IdentifierStr == "var")
      return tok_var;
    if (IdentifierStr == "void")
      return tok_void;
    if (IdentifierStr == "double")
      return tok_double;
    return tok_identifier;
  }

  // Handle numbers (integers and floating points)
  if (isdigit(LastChar) || LastChar == '.')
  {
    std::string NumStr;
    do
    {
      NumStr += LastChar;
      LastChar = getchar();
    } while (isdigit(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), nullptr);
    return tok_number;
  }

  // Handle comments (skip until end of line)
  if (LastChar == '#')
  {
    do
      LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
  }

  // Handle the arrow token '->'
  if (LastChar == '-')
  {
    LastChar = getchar();
    if (LastChar == '>')
    {
      LastChar = getchar(); // Consume '>'
      return tok_arrow;     // Return the token for '->'
    }
    return '-'; // Otherwise, return just '-'
  }

  // Check for end of file. Don't eat the EOF.
  if (LastChar == EOF)
    return tok_eof;

  // Otherwise, just return the character as its ASCII value.
  int ThisChar = LastChar;
  LastChar     = getchar();
  return ThisChar;
}

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int  CurTok;
static auto getNextToken() -> int { return CurTok = gettok(); }

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
static std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static auto GetTokPrecedence() -> int
{
  if (!isascii(CurTok))
    return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}

/// LogError* - These are little helper functions for error handling.
auto LogError(const char* Str) -> std::unique_ptr<ExprAST>
{
  PRINT_ERROR(Str);

  // Provide additional debugging info
  fprintf(stderr, "-- %s Check syntax near '%c'.\n", HINT_LABEL, CurTok);
  fprintf(stderr, "-- %s Review the expected syntax.\n", HINT_LABEL);

  return nullptr;
}

auto LogErrorP(const char* Str) -> std::unique_ptr<PrototypeAST>
{
  PRINT_ERROR(Str);
  fprintf(stderr, "-- %s Ensure function prototypes are correctly declared.\n", HINT_LABEL);
  fprintf(stderr, "-- %s Expected Format: <return_type> <function_name>(parameters...)\n",
          HINT_LABEL);
  return nullptr;
}

static auto ParseExpression() -> std::unique_ptr<ExprAST>;

/// numberexpr ::= number
static auto ParseNumberExpr() -> std::unique_ptr<ExprAST>
{
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  getNextToken(); // consume the number
  return Result;
}

/// parenexpr ::= '(' expression ')'
static auto ParseParenExpr() -> std::unique_ptr<ExprAST>
{
  getNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V)
    return nullptr;

  if (CurTok != ')')
    return LogError("expected ')'");
  getNextToken(); // eat ).
  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static auto ParseIdentifierExpr() -> std::unique_ptr<ExprAST>
{
  std::string IdName = IdentifierStr;

  getNextToken(); // eat identifier.

  if (CurTok != '(') // Simple variable ref.
    return std::make_unique<VariableExprAST>(IdName);

  // Call.
  getNextToken(); // eat (
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (CurTok != ')')
  {
    while (true)
    {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')')
        break;

      if (CurTok != ',')
        return LogError("Expected ')' or ',' in argument list.");
      getNextToken();
    }
  }

  // Eat the ')'.
  getNextToken();

  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// ifexpr ::= 'if' expression 'then' expression 'else' expression
static auto ParseIfExpr() -> std::unique_ptr<ExprAST>
{
  getNextToken(); // eat the if.

  // condition.
  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;

  if (CurTok != tok_then)
    return LogError("Expected the keyword \"then\".");
  getNextToken(); // eat the then

  auto Then = ParseExpression();
  if (!Then)
    return nullptr;

  if (CurTok != tok_else)
    return LogError("Expected the keyword \"else\".");

  getNextToken();

  auto Else = ParseExpression();
  if (!Else)
    return nullptr;

  return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then), std::move(Else));
}

/// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
static auto ParseForExpr() -> std::unique_ptr<ExprAST>
{
  getNextToken(); // eat the for.

  if (CurTok != tok_identifier)
    return LogError("Expected identifier after 'for'.");

  std::string IdName = IdentifierStr;
  getNextToken(); // eat identifier.

  if (CurTok != '=')
    return LogError("Expected '=' after 'for'.");
  getNextToken(); // eat '='.

  auto Start = ParseExpression();
  if (!Start)
    return nullptr;
  if (CurTok != ',')
    return LogError("Expected ',' after for start value.");
  getNextToken();

  auto End = ParseExpression();
  if (!End)
    return nullptr;

  // The step value is optional.
  std::unique_ptr<ExprAST> Step;
  if (CurTok == ',')
  {
    getNextToken();
    Step = ParseExpression();
    if (!Step)
      return nullptr;
  }

  if (CurTok != tok_in)
    return LogError("Expected 'in' after for");
  getNextToken(); // eat 'in'.

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

  return std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End), std::move(Step),
                                      std::move(Body));
}

/// varexpr ::= 'var' identifier ('=' expression)?
//                    (',' identifier ('=' expression)?)* 'in' expression
static auto ParseVarExpr() -> std::unique_ptr<ExprAST>
{
  getNextToken(); // eat the var.

  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;

  // At least one variable name is required.
  if (CurTok != tok_identifier)
    return LogError("Expected identifier after 'var'.");

  while (true)
  {
    std::string Name = IdentifierStr;
    getNextToken(); // eat identifier.

    // Read the optional initializer.
    std::unique_ptr<ExprAST> Init = nullptr;
    if (CurTok == '=')
    {
      getNextToken(); // eat the '='.

      Init = ParseExpression();
      if (!Init)
        return nullptr;
    }

    VarNames.emplace_back(Name, std::move(Init));

    // End of var list, exit loop.
    if (CurTok != ',')
      break;
    getNextToken(); // eat the ','.

    if (CurTok != tok_identifier)
      return LogError("Expected identifier list after 'var'");
  }

  // At this point, we have to have 'in'.
  if (CurTok != tok_in)
    return LogError("Expected 'in' keyword after 'var'");
  getNextToken(); // eat 'in'.

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

  return std::make_unique<VarExprAST>(std::move(VarNames), std::move(Body));
}

static auto ProcessString(std::string& ProcessedStr) -> void
{
  for (size_t i = 0; i < StringVal.size(); ++i)
  {
    if (StringVal[i] == '\\' && i + 1 < StringVal.size()) // Check for escape sequences
    {
      switch (StringVal[i + 1])
      {
        case 'n':
          ProcessedStr += '\n';
          break;
        case 'r':
          ProcessedStr += '\r';
          break;
        case 't':
          ProcessedStr += '\t';
          break;
        case 'b':
          ProcessedStr += '\b';
          break; // Backspace
        case 'f':
          ProcessedStr += '\f';
          break; // Form feed
        case 'v':
          ProcessedStr += '\v';
          break; // Vertical tab
        case '0':
          ProcessedStr += '\0';
          break; // Null character
        case '\\':
          ProcessedStr += '\\';
          break; // Backslash
        case '"':
          ProcessedStr += '"';
          break; // Double quote

        // Hexadecimal escape sequence: \xHH
        case 'x':
          if (i + 3 < StringVal.size() && isxdigit(StringVal[i + 2]) && isxdigit(StringVal[i + 3]))
          {
            std::string hexStr = StringVal.substr(i + 2, 2);
            ProcessedStr += static_cast<char>(std::stoi(hexStr, nullptr, 16));
            i += 3; // Skip \xHH
          }
          else
          {
            ProcessedStr += "\\x"; // Invalid sequence, keep as is
          }
          break;

        // Unicode escape sequence: \uHHHH (Basic Multilingual Plane)
        case 'u':
          if (i + 5 < StringVal.size() && isxdigit(StringVal[i + 2]) &&
              isxdigit(StringVal[i + 3]) && isxdigit(StringVal[i + 4]) &&
              isxdigit(StringVal[i + 5]))
          {
            std::string unicodeStr   = StringVal.substr(i + 2, 4);
            int         unicodeValue = std::stoi(unicodeStr, nullptr, 16);
            ProcessedStr += static_cast<char>(unicodeValue); // Basic Unicode handling
            i += 5;                                          // Skip \uHHHH
          }
          else
          {
            ProcessedStr += "\\u"; // Invalid sequence, keep as is
          }
          break;

        default:
          ProcessedStr += StringVal[i];     // Keep original '\'
          ProcessedStr += StringVal[i + 1]; // Keep next char as is
      }
      ++i; // Skip next char as it’s part of escape sequence
    }
    else
    {
      ProcessedStr += StringVal[i]; // Normal character
    }
  }
}

static auto ParseStringExpr() -> std::unique_ptr<ExprAST>
{
  std::string ProcessedStr;

  ProcessString(ProcessedStr); // check for escape sequences

  auto Result = std::make_unique<StringExprAST>(ProcessedStr);
  getNextToken(); // Consume the string token
  return Result;
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
///   ::= ifexpr
///   ::= forexpr
///   ::= varexpr
static auto ParsePrimary() -> std::unique_ptr<ExprAST>
{
  switch (CurTok)
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
static auto ParseUnary() -> std::unique_ptr<ExprAST>
{
  // If the current token is not an operator, it must be a primary expr.
  if (!isascii(CurTok) || CurTok == '(' || CurTok == ',')
    return ParsePrimary();

  // If this is a unary operator, read it.
  int Opc = CurTok;
  getNextToken();
  if (auto Operand = ParseUnary())
    return std::make_unique<UnaryExprAST>(Opc, std::move(Operand));
  return nullptr;
}

/// binoprhs
///   ::= ('+' unary)*
static auto ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) -> std::unique_ptr<ExprAST>
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
    int BinOp = CurTok;
    getNextToken(); // eat binop

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
    LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  }
}

/// expression
///   ::= unary binoprhs
///
static auto ParseExpression() -> std::unique_ptr<ExprAST>
{
  auto LHS = ParseUnary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER number? (id, id)
///   ::= unary LETTER (id)
static auto ParsePrototype() -> std::unique_ptr<PrototypeAST>
{
  llvm::Type* RetType = llvm::Type::getDoubleTy(*TheContext); // Default to double

  std::string FnName;
  unsigned    Kind             = 0; // 0 = identifier, 1 = unary, 2 = binary.
  unsigned    BinaryPrecedence = 30;

  switch (CurTok)
  {
    default:
      return LogErrorP("Expected function name in prototype");
    case tok_identifier:
      FnName = IdentifierStr;
      Kind   = 0;
      getNextToken();
      break;
    case tok_unary:
      getNextToken();
      if (!isascii(CurTok))
        return LogErrorP("Expected unary operator");
      FnName = "unary";
      FnName += (char)CurTok;
      Kind = 1;
      getNextToken();
      break;
    case tok_binary:
      getNextToken();
      if (!isascii(CurTok))
        return LogErrorP("Expected binary operator");
      FnName = "binary";
      FnName += (char)CurTok;
      Kind = 2;
      getNextToken();

      // Read the precedence if present.
      if (CurTok == tok_number)
      {
        if (NumVal < 1 || NumVal > 100)
          return LogErrorP("Invalid precedence: must be 1..100");
        BinaryPrecedence = (unsigned)NumVal;
        getNextToken();
      }
      break;
  }

  if (CurTok != '(')
    return LogErrorP("Expected '(' in prototype");

  std::vector<std::pair<std::string, llvm::Type*>> Args;
  getNextToken(); // Eat '('

  while (CurTok == tok_identifier || CurTok == tok_double)
  {
    llvm::Type* ArgType = llvm::Type::getDoubleTy(*TheContext); // Default type

    if (CurTok == tok_identifier)
    {
      Args.emplace_back(IdentifierStr, ArgType);
      getNextToken();
    }
    else if (CurTok == tok_double)
    {
      getNextToken(); // Eat 'double'
      if (CurTok != tok_identifier)
        return LogErrorP("Expected argument name after 'double'");

      Args.emplace_back(IdentifierStr, ArgType);
      getNextToken();
    }

    if (CurTok == ',')
      getNextToken(); // Eat ','
  }

  if (CurTok != ')')
    return LogErrorP("Expected ')' in prototype");

  getNextToken(); // Eat ')'

  // Check if return type is explicitly specified
  if (CurTok == tok_arrow)
  {
    getNextToken(); // Consume `->`
    if (CurTok == tok_void)
    {
      RetType = llvm::Type::getVoidTy(*TheContext);
      getNextToken(); // Consume 'void'
    }
    else if (CurTok == tok_double)
    {
      getNextToken(); // Consume 'double' (default return type remains)
    }
    else
    {
      return LogErrorP("Expected return type after '->'");
    }
  }

  // Verify correct operand count for operators.
  if (Kind && Args.size() != Kind)
    return LogErrorP("Invalid number of operands for operator");

  std::vector<std::string> ArgNames;
  for (const auto& Arg : Args)
    ArgNames.push_back(Arg.first);

  return std::make_unique<PrototypeAST>(FnName, ArgNames, RetType, Kind != 0, BinaryPrecedence);
}

/// definition ::= 'def' prototype expression
static auto ParseDefinition() -> std::unique_ptr<FunctionAST>
{
  getNextToken(); // eat def.
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  if (auto E = ParseExpression())
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  return nullptr;
}

/// toplevelexpr ::= expression
static auto ParseTopLevelExpr() -> std::unique_ptr<FunctionAST>
{
  if (auto E = ParseExpression())
  {
    // Determine the return type based on the expression.
    llvm::Type* RetType = llvm::Type::getDoubleTy(*TheContext); // Default to double

    // Make an anonymous prototype with the return type.
    auto Proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>(), RetType);
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

/// external ::= 'extern' prototype
static auto ParseExtern() -> std::unique_ptr<PrototypeAST>
{
  getNextToken(); // Consume 'extern'
  return ParsePrototype();
}
