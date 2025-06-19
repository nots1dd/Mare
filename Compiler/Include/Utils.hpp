#include <filesystem>
#include <string>

// Custom typedefs in Mare follow <Name>__ format
//
// <Name> is meant to be in PascalCase

using Directory__   = std::string;
using FilePath__    = std::string;
using FileContent__ = std::string;
using StdFilePath__ = std::filesystem::path;

using CmdLineArgs__ = std::string;
