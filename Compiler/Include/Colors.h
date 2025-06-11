#pragma once

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

// ────────────────────────────────────────────────
// ANSI Escape Codes for Formatting
// ────────────────────────────────────────────────
#define COLOR_RESET  "\033[0m"
#define COLOR_BOLD   "\033[1m"
#define COLOR_UNDERL "\033[4m"

#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_WHITE   "\033[1;37m"
#define COLOR_GRAY    "\033[1;90m"

// ────────────────────────────────────────────────
// Labels with Stylized Framing
// ────────────────────────────────────────────────
#define ERROR_LABEL    COLOR_BOLD COLOR_RED "ERROR:" COLOR_RESET
#define WARNING_LABEL  COLOR_BOLD COLOR_YELLOW "WARNING:" COLOR_RESET
#define INFO_LABEL     COLOR_BOLD COLOR_BLUE "INFO:" COLOR_RESET
#define HINT_LABEL     COLOR_BOLD COLOR_CYAN "[Hint]" COLOR_RESET
#define COMPILED_LABEL COLOR_BOLD COLOR_GREEN "[Compile]" COLOR_RESET
#define DEBUG_LABEL    COLOR_BOLD COLOR_MAGENTA "[DEBUG]" COLOR_RESET
#define INTERNAL_LABEL COLOR_BOLD COLOR_GRAY "[Internal]" COLOR_RESET

// ────────────────────────────────────────────────
// Pretty ASCII Header Divider
// ────────────────────────────────────────────────
#define LINE_DIVIDER "────────────────────────────────────────────────────"

// ────────────────────────────────────────────────
// Formatted Print Macros (Clean Framed Output)
// ────────────────────────────────────────────────
#define PRINT_ERROR(msg)                                              \
  do                                                                  \
  {                                                                   \
    fprintf(stderr, "\n%s %s\n%s\n", ERROR_LABEL, msg, LINE_DIVIDER); \
  } while (0)

#define PRINT_WARNING(msg)                                              \
  do                                                                    \
  {                                                                     \
    fprintf(stderr, "\n%s %s\n%s\n", WARNING_LABEL, msg, LINE_DIVIDER); \
  } while (0)

#define PRINT_HINT(msg)                          \
  do                                             \
  {                                              \
    fprintf(stderr, "%s %s\n", HINT_LABEL, msg); \
  } while (0)

#define PRINT_SUCCESS(msg)                                               \
  do                                                                     \
  {                                                                      \
    fprintf(stderr, "\n%s %s\n%s\n", COMPILED_LABEL, msg, LINE_DIVIDER); \
  } while (0)

#define PRINT_INFO(msg)                                                \
  do                                                                   \
  {                                                                    \
    fprintf(stderr, "\n%s %s\n%s\n\n", INFO_LABEL, msg, LINE_DIVIDER); \
  } while (0)

#define PRINT_DEBUG(msg)                                              \
  do                                                                  \
  {                                                                   \
    fprintf(stderr, "\n%s %s\n%s\n", DEBUG_LABEL, msg, LINE_DIVIDER); \
  } while (0)

#define PRINT_INTERNAL_ERROR(msg)                                        \
  do                                                                     \
  {                                                                      \
    fprintf(stderr, "\n%s %s\n%s\n", INTERNAL_LABEL, msg, LINE_DIVIDER); \
  } while (0)

inline auto getLineFromFile(const std::string& filename, int targetLine) -> std::string
{
  std::ifstream file(filename);
  if (!file.is_open())
    return "";

  std::string line;
  for (int i = 1; i <= targetLine; ++i)
  {
    if (!std::getline(file, line))
      return "";
  }
  return line;
}

inline void printDiagnostic(const std::string& typeLabel, const std::string& message,
                            const std::string& filename, int line, int column,
                            const std::string& hint = "")
{
  std::string sourceLine = getLineFromFile(filename, line);

  std::cerr << "\n" << typeLabel << " " << message << "\n";
  std::cerr << " --> " << filename << ":" << line << ":" << column << "\n";
  std::cerr << "  |\n";

  if (!sourceLine.empty())
  {
    std::cerr << std::setw(3) << line << " | " << sourceLine << "\n";
    std::cerr << "    | " << std::string(column - 1, ' ') << COLOR_RED "^" << COLOR_RESET << "\n";
  }

  if (!hint.empty())
    std::cerr << HINT_LABEL << " " << hint << "\n";

  std::cerr << LINE_DIVIDER << "\n\n";
}
