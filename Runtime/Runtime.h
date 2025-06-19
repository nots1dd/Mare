#pragma once

#include <cstdint>

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __attribute__((visibility("default")))
#endif

#define MARE_COMPILER_RT_API DLLEXPORT
#define MARE_COMPILER_ABI    extern

// ----------------------------
// Type Aliases for Readability
// ----------------------------

using F64    = double;
using F32    = float;
using F64Fn  = F64 (*)(F64);
using F64Fn2 = F64 (*)(F64, F64);
using F32Fn  = F32 (*)(F32);
using F32Fn2 = F32 (*)(F32, F32);

// ------------------------------------------
// Mare Runtime ABI - Header
// ------------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif

  // ------------------------------
  // Printing Helpers (stderr)
  // ------------------------------

  MARE_COMPILER_RT_API void __mare_printc(char x);
  MARE_COMPILER_RT_API void __mare_printstr(char* x);
  MARE_COMPILER_RT_API void __mare_printf(F32 x);
  MARE_COMPILER_RT_API void __mare_printd(F64 x);
  MARE_COMPILER_RT_API void __mare_printi8(int8_t x);
  MARE_COMPILER_RT_API void __mare_printi16(int16_t x);
  MARE_COMPILER_RT_API void __mare_printi32(int32_t x);
  MARE_COMPILER_RT_API void __mare_printi64(int64_t x);
  MARE_COMPILER_RT_API auto __mare_putchard(F64 x) -> F64;

  // ------------------------------
  // Unary Float/Double Math (f/d)
  // ------------------------------

  MARE_COMPILER_RT_API auto __mare_sqrtd(F64 x) -> F64;
  MARE_COMPILER_RT_API auto __mare_sqrtf(F32 x) -> F32;

  MARE_COMPILER_RT_API auto __mare_sind(F64 x) -> F64;
  MARE_COMPILER_RT_API auto __mare_sinf(F32 x) -> F32;

  MARE_COMPILER_RT_API auto __mare_cosd(F64 x) -> F64;
  MARE_COMPILER_RT_API auto __mare_cosf(F32 x) -> F32;

  MARE_COMPILER_RT_API auto __mare_tand(F64 x) -> F64;
  MARE_COMPILER_RT_API auto __mare_tanf(F32 x) -> F32;

  MARE_COMPILER_RT_API auto __mare_logd(F64 x) -> F64;
  MARE_COMPILER_RT_API auto __mare_logf(F32 x) -> F32;

  MARE_COMPILER_RT_API auto __mare_expd(F64 x) -> F64;
  MARE_COMPILER_RT_API auto __mare_expf(F32 x) -> F32;

  MARE_COMPILER_RT_API auto __mare_roundd(F64 x) -> F64;
  MARE_COMPILER_RT_API auto __mare_roundf(F32 x) -> F32;

  MARE_COMPILER_RT_API auto __mare_floord(F64 x) -> F64;
  MARE_COMPILER_RT_API auto __mare_floorf(F32 x) -> F32;

  MARE_COMPILER_RT_API auto __mare_ceild(F64 x) -> F64;
  MARE_COMPILER_RT_API auto __mare_ceilf(F32 x) -> F32;

  // ------------------------------
  // Binary Float/Double Math (f/d)
  // ------------------------------

  MARE_COMPILER_RT_API auto __mare_powd(F64 x, F64 y) -> F64;
  MARE_COMPILER_RT_API auto __mare_powf(F32 x, F32 y) -> F32;

  MARE_COMPILER_RT_API auto __mare_hypotd(F64 x, F64 y) -> F64;
  MARE_COMPILER_RT_API auto __mare_hypotf(F32 x, F32 y) -> F32;

  MARE_COMPILER_RT_API auto __mare_fmodd(F64 x, F64 y) -> F64;
  MARE_COMPILER_RT_API auto __mare_fmodf(F32 x, F32 y) -> F32;

#ifdef __cplusplus
}
#endif
