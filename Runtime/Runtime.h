#pragma once

// Goo exposes some basic math functions from C at runtime for Goo files

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#define DECL_MATH_FUNC(name)  extern "C" DLLEXPORT auto name(double X) -> double;
#define DECL_MATH_FUNC2(name) extern "C" DLLEXPORT auto name(double X, double Y) -> double;

// Basic functions
extern "C" DLLEXPORT auto putchard(double X) -> double;
extern "C" DLLEXPORT auto printd(double X) -> double;

// Trigonometric and logarithmic functions
DECL_MATH_FUNC(sqrtd)
DECL_MATH_FUNC(sind)
DECL_MATH_FUNC(cosd)
DECL_MATH_FUNC(tand)
DECL_MATH_FUNC(asind)
DECL_MATH_FUNC(acosd)
DECL_MATH_FUNC(atand)
DECL_MATH_FUNC(expd)
DECL_MATH_FUNC(logd)
DECL_MATH_FUNC(log10d)

// Binary math functions
DECL_MATH_FUNC2(powd)
DECL_MATH_FUNC2(hypotd)
DECL_MATH_FUNC2(fmodd)

// Rounding functions
DECL_MATH_FUNC(fabsd)
DECL_MATH_FUNC(floord)
DECL_MATH_FUNC(ceild)
DECL_MATH_FUNC(roundd)

// Hyperbolic functions
DECL_MATH_FUNC(sinhd)
DECL_MATH_FUNC(coshd)
DECL_MATH_FUNC(tanhd)

// Inverse hyperbolic functions
DECL_MATH_FUNC(asinhd)
DECL_MATH_FUNC(acoshd)
DECL_MATH_FUNC(atanhd)
