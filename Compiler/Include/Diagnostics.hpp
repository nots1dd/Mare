#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "CmdLineParser.hpp"
#include "Colors.h" // Define ANSI codes like COLOR_RED, COLOR_YELLOW, etc.

enum class DiagnosticLevel
{
  Error,
  Warning,
  Note,
  Info
};

inline auto levelToString(DiagnosticLevel level) -> const char*
{
  switch (level)
  {
    case DiagnosticLevel::Error:
      return "error";
    case DiagnosticLevel::Warning:
      return "warning";
    case DiagnosticLevel::Note:
      return "note";
    case DiagnosticLevel::Info:
      return "info";
  }
  return "";
}

inline auto levelColor(DiagnosticLevel level) -> const char*
{
  switch (level)
  {
    case DiagnosticLevel::Error:
      return COLOR_RED;
    case DiagnosticLevel::Warning:
      return COLOR_YELLOW;
    case DiagnosticLevel::Note:
      return COLOR_CYAN;
    case DiagnosticLevel::Info:
      return COLOR_BLUE;
  }
  return "";
}

inline auto getLineFromFile(const std::string& filename, int targetLine) -> FileContent__
{
  std::ifstream file(filename);
  if (!file.is_open())
    return ""; // Could also log this error

  std::string line;
  for (int currentLine = 1; std::getline(file, line); ++currentLine)
  {
    if (currentLine == targetLine)
      return line;
  }

  return ""; // Line not found
}

inline void printDiagnostic(DiagnosticLevel level, const std::string& message,
                            const std::string& filename, int line, int column,
                            const std::string& hint = "", int length = 1)
{
  FileContent__ sourceLine = getLineFromFile(filename, line);
  const char*   color      = levelColor(level);
  const char*   label      = levelToString(level);

  // Basic UTF-8 sanitization: replace bad characters
  for (char& c : sourceLine)
  {
    if (static_cast<unsigned char>(c) < 0x20 && c != '\t' && c != '\n')
    {
      c = '?';
    }
  }

  std::cerr << "\n"
            << color << label << COLOR_RESET << ": " << message << "\n"
            << "  " << COLOR_DIM << "--> " << filename << ":" << line << ":" << column
            << COLOR_RESET << "\n"
            << "   " << COLOR_DIM << "│" << COLOR_RESET << "\n";

  if (!sourceLine.empty())
  {
    std::cerr << std::setw(3) << line << " " << COLOR_DIM << "│ " << COLOR_RESET << sourceLine
              << "\n";

    std::cerr << "    " << COLOR_DIM << "│ " << COLOR_RESET << std::string(column - 1, ' ') << color
              << "^";
    if (length > 1)
      std::cerr << std::string(length - 1, '~');
    std::cerr << COLOR_RESET << "\n";
  }

  if (!hint.empty())
  {
    // Enhanced hint: bold yellow with arrow and spacing
    std::cerr << "    " << COLOR_DIM << "│" << COLOR_RESET << "\n"
              << "    " << COLOR_DIM << "╰── " << COLOR_BOLD_YELLOW << hint << COLOR_RESET << "\n";
  }

  std::cerr << std::endl;
}
