#pragma once

#include "AST.hpp"
#include "Colors.h"
#include "Diagnostics.hpp"

struct FileCoords
{
  int line = 1;
  int col  = 0;

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

namespace Mare::Err
{

[[noreturn]] void FatalError(const char* message)
{
  fprintf(stderr, "-- %s Cursor stopped at line %d, column %d\n", HINT_LABEL, fileCoords.line,
          fileCoords.col);
  std::exit(EXIT_FAILURE);
}

/// LogError* - These are little helper functions for error handling.
auto LogError(const char* msg) -> std::unique_ptr<Expr>
{
  // If you have current location tracking:
  printDiagnostic(DiagnosticLevel::Error, msg, mareArgs.inputFile, fileCoords.line, fileCoords.col,
                  "Check syntax near the cursor!");

  FatalError("Exiting compilation.");
  return nullptr;
}

auto LogErrorP(const char* Str) -> std::unique_ptr<Prototype>
{
  printDiagnostic(
    DiagnosticLevel::Error, Str,
    mareArgs.inputFile, // Optionally provide current filename
    fileCoords.line,    // Line number, if known
    fileCoords.col,     // Column number, if known
    "Ensure function prototypes are declared as: fn name(type name, ...) -> return_type");

  FatalError("Exiting compilation due to prototyping errors.");

  return nullptr;
}

} // namespace Mare::Err
