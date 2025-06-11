#include "Runtime.h"
#include <cmath>
#include <cstdio>

// ----------------------------
// C ABI IO Functions
// ----------------------------

namespace goo_rt
{

template <typename T> inline void print(const char* fmt, T value)
{
  std::fprintf(stderr, fmt, value);
}

} // namespace goo_rt

// ----------------------------
// Templated Math Wrappers
// ----------------------------

namespace goo_rt
{

template <double (*Func)(double)> inline auto unary(double x) -> double { return Func(x); }

template <double (*Func)(double, double)> inline auto binary(double x, double y) -> double
{
  return Func(x, y);
}

} // namespace goo_rt

// ----------------------------
// Exported C ABI Functions
// ----------------------------

#define DEFINE_UNARY_ABI(func)                                      \
  extern "C" GOO_COMPILER_RT_API double func##d(double x)           \
  {                                                                 \
    return goo_rt::unary<static_cast<double (*)(double)>(func)>(x); \
  }

#define DEFINE_BINARY_ABI(func)                                                 \
  extern "C" GOO_COMPILER_RT_API double func##d(double x, double y)             \
  {                                                                             \
    return goo_rt::binary<static_cast<double (*)(double, double)>(func)>(x, y); \
  }

extern "C"
{

  GOO_COMPILER_RT_API void __printc(char x) { goo_rt::print("%c\n", x); }
  GOO_COMPILER_RT_API void __printstr(char* x) { goo_rt::print("%s\n", x); }
  GOO_COMPILER_RT_API void __printd(double x) { goo_rt::print("%f\n", x); }

  GOO_COMPILER_RT_API auto putchard(double x) -> double
  {
    std::fputc(static_cast<unsigned char>(static_cast<int>(x)), stderr);
    return 0.0;
  }

  // Unary math functions

  // ------------------
  // MISC MATH FUNCS
  // ------------------
  DEFINE_UNARY_ABI(sqrt)
  DEFINE_UNARY_ABI(exp)
  DEFINE_UNARY_ABI(log10)
  DEFINE_UNARY_ABI(log)
  DEFINE_UNARY_ABI(fabs)
  DEFINE_UNARY_ABI(floor)
  DEFINE_UNARY_ABI(ceil)
  DEFINE_UNARY_ABI(round)

  // ------------------
  // TRIG FUNCS
  // ------------------

  DEFINE_UNARY_ABI(sin)
  DEFINE_UNARY_ABI(cos)
  DEFINE_UNARY_ABI(tan)
  DEFINE_UNARY_ABI(asin)
  DEFINE_UNARY_ABI(acos)
  DEFINE_UNARY_ABI(atan)

  // ----------------------
  // HYPERBOLIC TRIG FUNCS
  // ----------------------

  DEFINE_UNARY_ABI(sinh)
  DEFINE_UNARY_ABI(cosh)
  DEFINE_UNARY_ABI(tanh)
  DEFINE_UNARY_ABI(asinh)
  DEFINE_UNARY_ABI(acosh)
  DEFINE_UNARY_ABI(atanh)

  // Binary math functions
  DEFINE_BINARY_ABI(pow)
  DEFINE_BINARY_ABI(hypot)
  DEFINE_BINARY_ABI(fmod)
}
