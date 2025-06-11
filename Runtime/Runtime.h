#pragma once

// Goo exposes some basic math functions from C at runtime for Goo files

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __attribute__((visibility("default")))
#endif

#define GOO_COMPILER_RT_API DLLEXPORT
