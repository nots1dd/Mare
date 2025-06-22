#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>

// Custom typedefs in Mare follow <Name>__ format
//
// <Name> is meant to be in PascalCase

using Directory__   = std::string;
using FilePath__    = std::string;
using FileContent__ = std::string;
using StdFilePath__ = std::filesystem::path;
using Token__       = int;

using CmdLineArgs__ = std::string;

using i8     = int8_t;
using i16    = int16_t;
using i32    = int32_t;
using i64    = int64_t;
using Coords = int;

namespace Mare::Global
{

struct CodegenCoords
{
  int line = 1;
  int col  = 0;
};

struct FileCoords
{
  int line = 1;
  int col  = 0;

  CodegenCoords codegenCoords;

public:
  void resetLine() { line = 0; }
  void resetCol() { col = 0; }
  void resetAll()
  {
    resetLine();
    resetCol();
  }
};

static FileCoords fileCoords;

void UpdateCodegenCoords()
{
  fileCoords.codegenCoords.line = fileCoords.line;
  fileCoords.codegenCoords.col  = fileCoords.col;
}

using ValueVariant = std::variant<int8_t, int16_t, int32_t, int64_t, float, double>;

static std::string  IdentifierStr; // Filled in if tok_identifier
static Token__      NumTok;
static ValueVariant NumVal;
static std::string  StringVal;
static bool         isExtern = false;
} // namespace Mare::Global
