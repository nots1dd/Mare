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
#define COLOR_DIM    "\033[2m"

#define COLOR_RED         "\033[1;31m"
#define COLOR_GREEN       "\033[1;32m"
#define COLOR_YELLOW      "\033[1;33m"
#define COLOR_BLUE        "\033[1;34m"
#define COLOR_MAGENTA     "\033[1;35m"
#define COLOR_CYAN        "\033[1;36m"
#define COLOR_WHITE       "\033[1;37m"
#define COLOR_GRAY        "\033[1;90m"
#define COLOR_BOLD_YELLOW "\033[1;33m"

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
