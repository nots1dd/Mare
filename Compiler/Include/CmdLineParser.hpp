#pragma once

#include "Colors.h"
#include "Compiler.hpp"
#include "Config.hpp"
#include "Utils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

// --- Print Utilities ---
void printError(const std::string& msg)
{
  std::cerr << ADD_COLOR(COLOR_RED, "error: ") << msg << "\n";
}

void printHint(const std::string& msg)
{
  std::cerr << ADD_COLOR(COLOR_CYAN, "hint:  ") << msg << "\n";
}

void printInfo(const std::string& msg)
{
  std::cout << ADD_COLOR(COLOR_GREEN, "info:  ") << msg << "\n";
}

// --- Argument Parser ---
struct ArgParser
{
  FileContent__ inputFile;
  StdFilePath__ inputPath       = std::filesystem::current_path();
  FilePath__    linkerPath      = "/usr/bin/clang++";
  FilePath__    outputFile      = "a.out";
  bool          showCPUFeatures = false;
  std::ifstream inputFileStream;

  void printUsage()
  {
    using Entry = std::pair<std::string, std::string>;

    const std::vector<Entry> options = {
      {"-o <file>", "Set output binary filename (default: a.out)"},
      {"--output=<file>", "Same as -o"},
      {"--linker=<path>", "Path to linker (default: /usr/bin/clang++)"},
      {"--show-cpu-features", "Show the current target's CPU features (LLVM API)"},
      {"-h, --help", "Show this help message"}};

    // Header
    std::cout << COLOR_BOLD << COLOR_YELLOW
              << "\n╭─────────────── Mare Compiler Help ───────────────╮\n"
              << "│  Version  : " << COLOR_RESET << __MARE_VERSION__ << COLOR_BOLD << COLOR_YELLOW
              << "\n"
              << "│  Commit   : " << COLOR_RESET << __MARE_COMMIT_HASH__ << COLOR_BOLD
              << COLOR_YELLOW << "\n"
              << "│  Target   : " << COLOR_RESET << __MARE_TARGET_ARCH__ << COLOR_BOLD
              << COLOR_YELLOW << "\n"
              << "│  Triple   : " << COLOR_RESET << __MARE_LLVM_TRIPLE__ << COLOR_BOLD
              << COLOR_YELLOW << "\n"
              << "│  Build    : " << COLOR_RESET << __MARE_BUILD_TYPE__ << COLOR_BOLD
              << COLOR_YELLOW << "\n"
              << "╰──────────────────────────────────────────────────╯\n"
              << COLOR_RESET;

    // Usage
    std::cout << COLOR_BOLD << "\nUsage:\n"
              << COLOR_RESET << "  " << ADD_COLOR(COLOR_CYAN, "mare") << " [options] <file"
              << __MARE_FILE_EXTENSION_STEM__ << ">\n";

    // Options
    std::cout << ADD_COLOR(COLOR_BOLD, "\nOptions:\n");
    for (const auto& [flag, desc] : options)
    {
      std::cout << "  " << COLOR_GREEN << flag << COLOR_RESET;
      if (flag.size() < 20)
        std::cout << std::string(20 - flag.size(), ' '); // align descriptions
      std::cout << desc << "\n";
    }

    // Example
    std::cout << ADD_COLOR(COLOR_BOLD, "\nExample:\n") << "  mare -o myprog main"
              << __MARE_FILE_EXTENSION_STEM__ << "\n"
              << std::endl;
  }

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
      CmdLineArgs__ arg = argv[i];

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
      else if (arg.starts_with("--show-cpu-features"))
      {
        showCPUFeatures = true;
      }
      else if (!arg.starts_with("-") && inputFile.empty())
      {
        inputFile = arg; // Tentatively accept as source file
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
      printError("no input `.mare` source file provided.");
      return false;
    }

    StdFilePath__ fullPath = inputFile;

    if (fullPath.extension() != __MARE_FILE_EXTENSION_STEM__)
    {
      printError("invalid source file extension: " + fullPath.filename().string());
      printHint("Expected a file ending with: " + std::string(__MARE_FILE_EXTENSION_STEM__));
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
};

static ArgParser mareArgs;
