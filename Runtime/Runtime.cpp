#include "Runtime.h"
#include <cmath>
#include <cstdio>

extern "C" DLLEXPORT auto putchard(double X) -> double
{
  fputc((char)X, stderr);
  return 0;
}

extern "C" DLLEXPORT auto printd(double X) -> double
{
  fprintf(stderr, "%f\n", X);
  return 0;
}

#define DEFINE_MATH_FUNC(name, func) \
  extern "C" DLLEXPORT auto name##d(double X)->double { return func(X); }

#define DEFINE_MATH_FUNC2(name, func) \
  extern "C" DLLEXPORT auto name##d(double X, double Y)->double { return func(X, Y); }

// Single-argument functions
DEFINE_MATH_FUNC(sqrt, sqrt)
DEFINE_MATH_FUNC(sin, sin)
DEFINE_MATH_FUNC(cos, cos)
DEFINE_MATH_FUNC(tan, tan)
DEFINE_MATH_FUNC(asin, asin)
DEFINE_MATH_FUNC(acos, acos)
DEFINE_MATH_FUNC(atan, atan)
DEFINE_MATH_FUNC(exp, exp)
DEFINE_MATH_FUNC(log, log)
DEFINE_MATH_FUNC(log10, log10)
DEFINE_MATH_FUNC(fabs, fabs)
DEFINE_MATH_FUNC(floor, floor)
DEFINE_MATH_FUNC(ceil, ceil)
DEFINE_MATH_FUNC(round, round)
DEFINE_MATH_FUNC(sinh, sinh)
DEFINE_MATH_FUNC(cosh, cosh)
DEFINE_MATH_FUNC(tanh, tanh)
DEFINE_MATH_FUNC(asinh, asinh)
DEFINE_MATH_FUNC(acosh, acosh)
DEFINE_MATH_FUNC(atanh, atanh)

// Two-argument functions
DEFINE_MATH_FUNC2(pow, pow)
DEFINE_MATH_FUNC2(hypot, hypot)
DEFINE_MATH_FUNC2(fmod, fmod)
