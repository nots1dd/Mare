#pragma once

#include "Compiler.hpp"
#include <llvm/IR/Type.h>

#define MARE_DOUBLE_TYPE llvm::Type::getDoubleTy(*TheContext)
#define MARE_FLOAT_TYPE  llvm::Type::getFloatTy(*TheContext)
#define MARE_INT1_TYPE   llvm::Type::getInt1Ty(*TheContext)
#define MARE_INT8_TYPE   llvm::Type::getInt8Ty(*TheContext)
#define MARE_INT16_TYPE  llvm::Type::getInt16Ty(*TheContext)
#define MARE_INT32_TYPE  llvm::Type::getInt32Ty(*TheContext)
#define MARE_INT64_TYPE  llvm::Type::getInt64Ty(*TheContext)
#define MARE_VOID_TYPE   llvm::Type::getVoidTy(*TheContext)
#define MARE_STRPTR_TYPE llvm::PointerType::getInt8Ty(*TheContext) // i8*

// For generic integer types of N bits
#define MARE_INTN_TYPE(N) llvm::Type::getIntNTy(*TheContext, N)

// Vector type example: <4 x float>
#define MARE_VECTOR_TYPE(elemType, count) llvm::VectorType::get(elemType, count)

// Array type example: [10 x i32]
#define MARE_ARRAY_TYPE(elemType, count) llvm::ArrayType::get(elemType, count)
