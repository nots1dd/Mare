#pragma once

// ANSI Escape Codes for Formatting
#define COLOR_RESET  "\033[0m"
#define COLOR_BOLD   "\033[1m"
#define COLOR_UNDERL "\033[4m"

// Foreground Colors
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_WHITE   "\033[1;37m"

// Formatting Macros for Readability
#define ERROR_LABEL    COLOR_BOLD COLOR_RED "[Error]" COLOR_RESET
#define WARNING_LABEL  COLOR_BOLD COLOR_YELLOW "[Warning]" COLOR_RESET
#define INFO_LABEL     COLOR_BOLD COLOR_BLUE "[Info]" COLOR_RESET
#define HINT_LABEL     COLOR_BOLD COLOR_CYAN "[Hint]" COLOR_RESET
#define COMPILED_LABEL COLOR_BOLD COLOR_GREEN "[Compile]" COLOR_RESET

// Function Macros for Error Messages
#define PRINT_ERROR(msg) fprintf(stderr, "%s " COLOR_BOLD "%s" COLOR_RESET "\n", ERROR_LABEL, msg)

#define PRINT_WARNING(msg) \
  fprintf(stderr, "%s " COLOR_BOLD "%s" COLOR_RESET "\n", WARNING_LABEL, msg)

#define PRINT_HINT(msg) fprintf(stderr, "%s " COLOR_BOLD "%s" COLOR_RESET "\n", HINT_LABEL, msg)

#define PRINT_SUCCESS(msg) \
  fprintf(stderr, "%s " COLOR_BOLD "%s" COLOR_RESET "\n", COMPILED_LABEL, msg)
