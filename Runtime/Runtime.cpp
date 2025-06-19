#include "Runtime.h"
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <type_traits>

// ----------------------------
// Generic Templated Print
// ----------------------------

namespace Mare::RT
{

template <typename T> inline void print(const char* fmt, T value)
{
  std::fprintf(stderr, fmt, value);
}

} // namespace Mare::RT

// ----------------------------
// Templated Math Wrappers
// ----------------------------

namespace Mare::RT::Ops
{

template <typename T, typename Fn> inline auto unary(Fn func, T x) -> T { return func(x); }

template <typename T, typename Fn> inline auto binary(Fn func, T x, T y) -> T { return func(x, y); }

} // namespace Mare::RT::Ops

// ----------------------------
// Macro Helpers
// ----------------------------

#define DEFINE_UNARY_OP_ABI(NAME, TYPE, SUFFIX, FUNC)                           \
  MARE_COMPILER_ABI "C" MARE_COMPILER_RT_API TYPE __mare_##NAME##SUFFIX(TYPE x) \
  {                                                                             \
    return Mare::RT::Ops::unary<TYPE>(FUNC, x);                                 \
  }

#define DEFINE_BINARY_OP_ABI(NAME, TYPE, SUFFIX, FUNC)                                  \
  MARE_COMPILER_ABI "C" MARE_COMPILER_RT_API TYPE __mare_##NAME##SUFFIX(TYPE x, TYPE y) \
  {                                                                                     \
    return Mare::RT::Ops::binary<TYPE>(FUNC, x, y);                                     \
  }

// ----------------------------
// C ABI Functions
// ----------------------------

extern "C"
{

  // Print helpers
  MARE_COMPILER_RT_API void __mare_printc(char x) { Mare::RT::print("%c\n", x); }
  MARE_COMPILER_RT_API void __mare_printstr(char* x) { Mare::RT::print("%s", x); }
  MARE_COMPILER_RT_API void __mare_printf(F32 x) { Mare::RT::print("%f\n", x); }
  MARE_COMPILER_RT_API void __mare_printd(F64 x) { Mare::RT::print("%f\n", x); }
  MARE_COMPILER_RT_API void __mare_printi8(int8_t x) { Mare::RT::print("%" PRId8 "\n", x); }
  MARE_COMPILER_RT_API void __mare_printi16(int16_t x) { Mare::RT::print("%" PRId16 "\n", x); }
  MARE_COMPILER_RT_API void __mare_printi32(int32_t x) { Mare::RT::print("%" PRId32 "\n", x); }
  MARE_COMPILER_RT_API void __mare_printi64(int64_t x) { Mare::RT::print("%" PRId64 "\n", x); }

  MARE_COMPILER_RT_API auto putchard(F64 x) -> F64
  {
    std::fputc(static_cast<unsigned char>(static_cast<int>(x)), stderr);
    return 0.0;
  }

  // Unary float (f) and double (d) math
  DEFINE_UNARY_OP_ABI(sqrt, F64, d, static_cast<F64Fn>(std::sqrt))
  DEFINE_UNARY_OP_ABI(sqrt, F32, f, static_cast<F32Fn>(std::sqrtf))

  DEFINE_UNARY_OP_ABI(sin, F64, d, static_cast<F64Fn>(std::sin))
  DEFINE_UNARY_OP_ABI(sin, F32, f, static_cast<F32Fn>(std::sinf))

  DEFINE_UNARY_OP_ABI(cos, F64, d, static_cast<F64Fn>(std::cos))
  DEFINE_UNARY_OP_ABI(cos, F32, f, static_cast<F32Fn>(std::cosf))

  DEFINE_UNARY_OP_ABI(tan, F64, d, static_cast<F64Fn>(std::tan))
  DEFINE_UNARY_OP_ABI(tan, F32, f, static_cast<F32Fn>(std::tanf))

  DEFINE_UNARY_OP_ABI(log, F64, d, static_cast<F64Fn>(std::log))
  DEFINE_UNARY_OP_ABI(log, F32, f, static_cast<F32Fn>(std::logf))

  DEFINE_UNARY_OP_ABI(exp, F64, d, static_cast<F64Fn>(std::exp))
  DEFINE_UNARY_OP_ABI(exp, F32, f, static_cast<F32Fn>(std::expf))

  DEFINE_UNARY_OP_ABI(round, F64, d, static_cast<F64Fn>(std::round))
  DEFINE_UNARY_OP_ABI(round, F32, f, static_cast<F32Fn>(std::roundf))

  DEFINE_UNARY_OP_ABI(floor, F64, d, static_cast<F64Fn>(std::floor))
  DEFINE_UNARY_OP_ABI(floor, F32, f, static_cast<F32Fn>(std::floorf))

  DEFINE_UNARY_OP_ABI(ceil, F64, d, static_cast<F64Fn>(std::ceil))
  DEFINE_UNARY_OP_ABI(ceil, F32, f, static_cast<F32Fn>(std::ceilf))

  // Binary float and double math
  DEFINE_BINARY_OP_ABI(pow, F64, d, static_cast<F64Fn2>(std::pow))
  DEFINE_BINARY_OP_ABI(pow, F32, f, static_cast<F32Fn2>(std::powf))

  DEFINE_BINARY_OP_ABI(hypot, F64, d, static_cast<F64Fn2>(std::hypot))
  DEFINE_BINARY_OP_ABI(hypot, F32, f, static_cast<F32Fn2>(std::hypotf))

  DEFINE_BINARY_OP_ABI(fmod, F64, d, static_cast<F64Fn2>(std::fmod))
  DEFINE_BINARY_OP_ABI(fmod, F32, f, static_cast<F32Fn2>(std::fmodf))

} // extern "C"
