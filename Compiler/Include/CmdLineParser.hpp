#pragma once

#include "Colors.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

// --- Print Utilities ---
void printError(const std::string& msg)
{
  std::cerr << COLOR_RED << "error: " << COLOR_RESET << msg << "\n";
}

void printHint(const std::string& msg)
{
  std::cerr << COLOR_CYAN << "hint:  " << COLOR_RESET << msg << "\n";
}

void printInfo(const std::string& msg)
{
  std::cout << COLOR_GREEN << "info:  " << COLOR_RESET << msg << "\n";
}

// --- Usage Help ---
void printUsage()
{
  std::cout << COLOR_BOLD << "Usage: " << COLOR_RESET << "goo [options] <file.goo>\n\n"
            << "Options:\n"
            << "  -o <file>          Set output binary filename (default: a.out)\n"
            << "  --output=<file>    Same as -o\n"
            << "  --linker=<path>    Path to linker executable (default: /usr/bin/clang++)\n"
            << "  -h, --help         Show this help message\n";
}

// --- Argument Parser ---
struct ArgParser
{
  std::string   inputFile;
  std::string   inputPath  = std::filesystem::current_path();
  std::string   linkerPath = "/usr/bin/clang++";
  std::string   outputFile = "a.out";
  std::ifstream inputFileStream;

  auto parse(int argc, char* argv[]) -> bool
  {
    if (argc < 2)
    {
      printError("no input file provided.");
      printHint("Use `--help` for usage information.");
      return false;
    }

    for (int i = 1; i < argc; ++i)
    {
      std::string arg = argv[i];

      if (arg == "-h" || arg == "--help")
      {
        printUsage();
        return false;
      }
      else if (arg.starts_with("--linker="))
      {
        linkerPath = arg.substr(9);
      }
      else if (arg == "-o" && i + 1 < argc)
      {
        outputFile = argv[++i];
      }
      else if (arg.starts_with("--output="))
      {
        outputFile = arg.substr(9);
      }
      else if (arg.ends_with(".goo"))
      {
        inputFile = arg;
      }
      else
      {
        printError("unknown argument: '" + arg + "'");
        printHint("Use `--help` to see supported options.");
        return false;
      }
    }

    if (inputFile.empty())
    {
      printError("no `.goo` source file specified.");
      return false;
    }

    std::filesystem::path fullPath = inputFile;
    if (fullPath.extension() != ".goo")
    {
      printError("expected a `.goo` file, got: " + fullPath.filename().string());
      return false;
    }

    inputFileStream.open(fullPath);
    if (!inputFileStream.is_open())
    {
      printError("failed to open source file: " + fullPath.string());
      return false;
    }

    return true;
  }

  void printSummary()
  {
    std::cout << COLOR_BOLD << "Compiler Configuration:\n"
              << COLOR_RESET << "  Source File : " << inputFile << "\n"
              << "  Output File : " << outputFile << "\n"
              << "  Linker Path : " << linkerPath << "\n"
              << std::endl;
  }
};

static ArgParser gooArgs;
