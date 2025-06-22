#include "Compiler.hpp"
#include "Globals.hpp"
#include "PrimitiveTypes.hpp"
#include <string>

namespace Mare::Util
{

// Generic getter for min value
template <typename T> constexpr auto dtype_min() -> T
{
  static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");
  return std::numeric_limits<T>::lowest(); // use `lowest()` for floats
}

// Generic getter for max value
template <typename T> constexpr auto dtype_max() -> T
{
  static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");
  return std::numeric_limits<T>::max();
}

template <typename T> constexpr bool always_false = false;

inline auto GetConstantFromValue(const Global::ValueVariant& val, llvm::Type* valType,
                                 llvm::LLVMContext& ctx) -> llvm::Constant*
{
  return std::visit(
    [&](auto&& v) -> llvm::Constant*
    {
      using T = std::decay_t<decltype(v)>;

      if constexpr (std::is_integral_v<T>)
      {
        return llvm::ConstantInt::get(valType, v, /*isSigned=*/true);
      }
      else if constexpr (std::is_floating_point_v<T>)
      {
        return llvm::ConstantFP::get(ctx, llvm::APFloat(v));
      }
      else
      {
        static_assert(always_false<T>, "Unsupported type in ValVariant");
      }
    },
    val);
}

auto StringCheckForEscapeSequences(int idx, std::string& ProcessedStr) -> void
{
  switch (Global::StringVal[idx + 1])
  {
    case 'n':
      ProcessedStr += ESCAPE_SEQUENCE_NEWLINE;
      break;
    case 'r':
      ProcessedStr += ESCAPE_SEQUENCE_CARRIAGE_RET;
      break;
    case 't':
      ProcessedStr += ESCAPE_SEQUENCE_TAB;
      break;
    case 'b':
      ProcessedStr += ESCAPE_SEQUENCE_BACKSPACE;
      break; // Backspace
    case 'f':
      ProcessedStr += ESCAPE_SEQUENCE_FORMFEED;
      break; // Form feed
    case 'v':
      ProcessedStr += ESCAPE_SEQUENCE_VERTICAL_TAB;
      break; // Vertical tab
    case '0':
      ProcessedStr += ESCAPE_SEQUENCE_NULL;
      break; // Null character
    case ESCAPE_SEQUENCE_BACKSLASH:
      ProcessedStr += ESCAPE_SEQUENCE_BACKSLASH;
      break; // Backslash
    case '"':
      ProcessedStr += '"';
      break; // Double quote

    default:
      ProcessedStr += Global::StringVal[idx];     // Keep original '\'
      ProcessedStr += Global::StringVal[idx + 1]; // Keep next char as is
  }
}

static auto ProcessString() -> std::string
{
  std::string processedStr;

  for (size_t i = 0; i < Global::StringVal.size(); ++i)
  {
    if (Global::StringVal[i] == ESCAPE_SEQUENCE_BACKSLASH &&
        i + 1 < Global::StringVal.size()) // Check for escape sequences
    {
      Util::StringCheckForEscapeSequences(i, processedStr);
      ++i; // Skip next char as it’s part of escape sequence
    }
    else
    {
      processedStr += Global::StringVal[i]; // Normal character
    }
  }

  return processedStr;
}

// Utility to parse return type
static auto ParseReturnTypeProto(const Token__ CurTok) -> llvm::Type*
{
  switch (CurTok)
  {
    case tok_void:
      return MARE_VOID_TYPE;
    case tok_double:
      return MARE_DOUBLE_TYPE;
    case tok_float:
      return MARE_FLOAT_TYPE;
    case tok_string:
      return MARE_STRPTR_TYPE;
    case tok_int8:
      return MARE_INT8_TYPE;
    case tok_int16:
      return MARE_INT16_TYPE;
    case tok_int32:
      return MARE_INT32_TYPE;
    case tok_int64:
      return MARE_INT64_TYPE;
    default:
      return nullptr;
  }
}

} // namespace Mare::Util
