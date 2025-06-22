#pragma once

#include "CmdLineParser.hpp"
#include "Compiler.hpp"
#include "ErrorHandling.hpp"
#include "PrimitiveTypes.hpp"

namespace Mare::Tokenizer
{

using namespace Mare::Global;

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static Token__ CurTok;

inline Coords LastLine = 1;
inline Coords LastCol  = 0;

static auto setNumVal(const std::string& numStr, bool isFloatLike, bool hasFSuffix) -> int
{
  try
  {
    if (isFloatLike && hasFSuffix)
    {
      float val = std::stof(numStr);
      NumVal    = val;
      return tok_float;
    }
    else if (isFloatLike)
    {
      double val = std::stod(numStr);
      NumVal     = val;
      return tok_double;
    }
    else
    {
      i64 val = std::stoll(numStr);

      NumVal = val;
      if (val >= Util::dtype_min<i8>() && val <= Util::dtype_max<i8>())
        return tok_int8;
      else if (val >= Util::dtype_min<i16>() && val <= Util::dtype_max<i16>())
        return tok_int16;
      else if (val >= Util::dtype_min<i32>() && val <= Util::dtype_min<i32>())
        return tok_int32;
      else
        return tok_int64;
    }
  }
  catch (const std::invalid_argument&)
  {
    Err::LogError("Invalid number literal!");
    return tok_double; // define this as needed
  }
  catch (const std::out_of_range&)
  {
    Err::LogError("Number out of range!");
    return tok_double; // define this as needed
  }
}

//===----------------------------------------------------------------------===//
// Tokenizer
//===----------------------------------------------------------------------===//

static auto getNextChar() -> int
{
  int ch = mareArgs.inputFileStream.get();

  if (ch == '\n')
  {
    fileCoords.line++;
    fileCoords.resetCol();
  }
  else
  {
    fileCoords.col++;
  }

  return ch;
}

/// gettok - Return the next token from standard input.
static auto gettok() -> Token__
{
  static Token__ LastChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar))
    LastChar = getNextChar();

  // Handle string literals
  if (LastChar == '"')
  {
    StringVal = "";
    while ((LastChar = getNextChar()) != '"' && LastChar != EOF)
    {
      StringVal += LastChar;
    }

    if (LastChar == EOF)
      return tok_eof;

    LastChar = getNextChar(); // Consume closing quote
    return tok_string;
  }

  // Handle identifiers and keywords
  if (isalpha(LastChar) || LastChar == '_')
  { // identifier: [a-zA-Z][a-zA-Z0-9]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getNextChar())) || LastChar == '_')
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
    if (IdentifierStr == "grab")
      return tok_grab;
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
    if (IdentifierStr == "float" || IdentifierStr == "flt")
      return tok_float;
    if (IdentifierStr == "int" || IdentifierStr == "i64")
      return tok_int64;
    if (IdentifierStr == "i32")
      return tok_int32;
    if (IdentifierStr == "i16")
      return tok_int16;
    if (IdentifierStr == "i8")
      return tok_int8;
    if (IdentifierStr == "string")
      return tok_string;
    if (IdentifierStr == "ret")
      return tok_ret;
    return tok_identifier;
  }

  // Handle numbers (integers and floating points)
  if (isdigit(LastChar) || LastChar == '.')
  {
    std::string NumStr;
    bool        isFloatLike = false;

    do
    {
      if (LastChar == '.')
        isFloatLike = true;

      NumStr += LastChar;
      LastChar = getNextChar();
    } while (isdigit(LastChar) || LastChar == '.');

    bool hasFSuffix = false;
    if (LastChar == 'f' || LastChar == 'F')
    {
      hasFSuffix = true;
      LastChar   = getNextChar();
    }

    NumTok = setNumVal(NumStr, isFloatLike, hasFSuffix);

    return tok_number;
  }

  // Handle comments (skip until end of line)
  if (LastChar == '#')
  {
    do
      LastChar = getNextChar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
  }

  // Handle the arrow token '->'
  if (LastChar == '-')
  {
    LastChar = getNextChar();
    if (LastChar == '>')
    {
      LastChar = getNextChar(); // Consume '>'
      return tok_arrow;         // Return the token for '->'
    }
    return '-'; // Otherwise, return just '-'
  }

  // Check for end of file. Don't eat the EOF.
  if (LastChar == EOF)
    return tok_eof;

  // Otherwise, just return the character as its ASCII value.
  Token__ ThisChar = LastChar;
  LastChar         = getNextChar();
  return ThisChar;
}

inline auto IsCurTokOverBlock() -> bool { return (CurTok == '}' || CurTok == tok_eof); }

inline auto TokenIsValidInt() -> bool
{
  return CurTok == tok_int64 || CurTok == tok_int32 || CurTok == tok_int16 || CurTok == tok_int8;
}

inline auto TokenIsValidArg() -> bool
{
  return (CurTok == tok_identifier || CurTok == tok_double || CurTok == tok_float ||
          CurTok == tok_string || TokenIsValidInt());
}

inline auto CurTokChar() -> char { return (char)CurTok; }

inline auto IsCurTokAscii() -> bool { return isascii(CurTok); }

inline auto IsCurTokPrimaryExpr() -> bool
{
  return (!IsCurTokAscii() || CurTok == '(' || CurTok == ',');
}

static auto getNextToken() -> Token__ { return CurTok = gettok(); }

inline auto assignDTypeToNumExpr() -> llvm::Type*
{
  llvm::Type* llvmType = nullptr;

  switch (Global::NumTok)
  {
    case tok_int8:
      llvmType = MARE_INT8_TYPE;
      break;
    case tok_int16:
      llvmType = MARE_INT16_TYPE;
      break;
    case tok_int32:
      llvmType = MARE_INT32_TYPE;
      break;
    case tok_int64:
      llvmType = MARE_INT64_TYPE;
      break;
    case tok_double:
      llvmType = MARE_DOUBLE_TYPE;
      break;
    case tok_float:
      llvmType = MARE_FLOAT_TYPE;
  }

  return llvmType;
}
} // namespace Mare::Tokenizer
