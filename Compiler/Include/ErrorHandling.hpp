#pragma once

#include "AST.hpp"
#include "Colors.h"
#include "Diagnostics.hpp"
#include "Globals.hpp"

namespace Mare::Err
{

//===----------------------------------------------------------------------===//
// FatalError - Will print the current codegen coords and parsing coords
// This function will always exit in failure!!
//===----------------------------------------------------------------------===//
[[noreturn]] void FatalError(const char* message)
{
  fprintf(stderr,
          "-- %s Reading Cursor stopped at line %d, column %d\n-- %s Codegen cursor stopped at "
          "line %d, column %d\n",
          HINT_LABEL, Global::fileCoords.line, Global::fileCoords.col, HINT_LABEL,
          Global::fileCoords.codegenCoords.line, Global::fileCoords.codegenCoords.col);
  std::exit(EXIT_FAILURE);
}

/// LogError* - These are little helper functions for error handling.
auto LogError(const char* msg) -> std::unique_ptr<Expr>
{
  // If you have current location tracking:
  printDiagnostic(DiagnosticLevel::Error, msg, mareArgs.inputFile,
                  Global::fileCoords.codegenCoords.line, Global::fileCoords.codegenCoords.col,
                  "Check syntax near the cursor!");

  FatalError("Exiting compilation.");
  return nullptr;
}

auto LogErrorP(const char* Str) -> std::unique_ptr<Prototype>
{
  printDiagnostic(
    DiagnosticLevel::Error, Str,
    mareArgs.inputFile,                    // Optionally provide current filename
    Global::fileCoords.codegenCoords.line, // Line number, if known
    Global::fileCoords.codegenCoords.col,  // Column number, if known
    "Ensure function prototypes are declared as: fn name(type name, ...) -> return_type");

  FatalError("Exiting compilation due to prototyping errors.");

  return nullptr;
}

} // namespace Mare::Err
