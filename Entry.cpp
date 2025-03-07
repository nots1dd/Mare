#include <cstdio>
#include <iostream>
#include <string>

#define DECLARE_EXTERN_FUNC(name, ...) extern "C" auto name(__VA_ARGS__)->double;

DECLARE_EXTERN_FUNC(printstar, double)
DECLARE_EXTERN_FUNC(PrintBetterStar, double, double)
DECLARE_EXTERN_FUNC(fail, double)
DECLARE_EXTERN_FUNC(printString)

auto main(int argc, char* argv[]) -> int
{
  if (argc < 1)
  {
    std::cout << "Improper use of FFI entry!" << std::endl;
    return 1;
  }
  printstar(std::stoi(argv[1]));
  PrintBetterStar(10, 5);
  std::cout << fail(std::stoi(argv[2])) << std::endl;
  printString();

  return 0;
}
