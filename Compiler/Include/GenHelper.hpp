#pragma once

#include "AST.hpp"
#include "Parser.hpp"

inline auto LogErrorV(const char* Str) -> Value*
{
  Mare::Err::LogError(Str);
  return nullptr;
}

auto getCommonType(Type* T1, Type* T2) -> Type*
{
  if (T1 == T2)
    return T1;

  // Validate input types first
  if (!T1 || !T2)
    return nullptr;

  // Type promotion hierarchy for ValueVariant types
  // int8_t < int16_t < int32_t < int64_t < float < double

  auto getTypeRank = [](Type* T) -> int
  {
    if (!T)
      return 0;
    if (T->isIntegerTy())
    {
      unsigned BitWidth = T->getIntegerBitWidth();
      switch (BitWidth)
      {
        case 8:
          return 1; // int8_t
        case 16:
          return 2; // int16_t
        case 32:
          return 3; // int32_t
        case 64:
          return 4; // int64_t
        default:
          return 0; // Unknown integer type
      }
    }
    else if (T->isFloatTy())
    {
      return 5; // float
    }
    else if (T->isDoubleTy())
    {
      return 6; // double
    }
    return 0; // Unknown type
  };

  int Rank1 = getTypeRank(T1);
  int Rank2 = getTypeRank(T2);

  if (Rank1 == 0 || Rank2 == 0)
    return nullptr; // Unknown type

  // Return the higher-ranked type
  return (Rank1 >= Rank2) ? T1 : T2;
}

// Helper function to promote a value to a target type
auto promoteValue(Value* Val, Type* FromType, Type* ToType) -> Value*
{
  if (!Val || !FromType || !ToType)
    return nullptr;
  if (FromType == ToType)
    return Val;

  // Ensure we're working with supported types
  bool FromSupported = FromType->isIntegerTy() || FromType->isFloatTy() || FromType->isDoubleTy();
  bool ToSupported   = ToType->isIntegerTy() || ToType->isFloatTy() || ToType->isDoubleTy();

  if (!FromSupported || !ToSupported)
  {
    LogErrorV("Unsupported type in value promotion");
    return nullptr;
  }

  // Integer to integer promotion/demotion
  if (FromType->isIntegerTy() && ToType->isIntegerTy())
  {
    unsigned FromBits = FromType->getIntegerBitWidth();
    unsigned ToBits   = ToType->getIntegerBitWidth();

    if (FromBits < ToBits)
    {
      return Builder->CreateSExt(Val, ToType, "sext");
    }
    else if (FromBits > ToBits)
    {
      return Builder->CreateTrunc(Val, ToType, "trunc");
    }
    return Val; // Same bit width
  }

  // Integer to float promotion
  if (FromType->isIntegerTy() && ToType->isFloatTy())
  {
    return Builder->CreateSIToFP(Val, ToType, "sitofp");
  }

  // Integer to double promotion
  if (FromType->isIntegerTy() && ToType->isDoubleTy())
  {
    return Builder->CreateSIToFP(Val, ToType, "sitofp");
  }

  // Float to double promotion
  if (FromType->isFloatTy() && ToType->isDoubleTy())
  {
    return Builder->CreateFPExt(Val, ToType, "fpext");
  }

  // Double to float demotion (potentially lossy)
  if (FromType->isDoubleTy() && ToType->isFloatTy())
  {
    return Builder->CreateFPTrunc(Val, ToType, "fptrunc");
  }

  // Float/double to integer conversion (potentially lossy)
  if (FromType->isFloatingPointTy() && ToType->isIntegerTy())
  {
    return Builder->CreateFPToSI(Val, ToType, "fptosi");
  }

  LogErrorV("Unsupported type conversion in value promotion");
  return nullptr;
}
