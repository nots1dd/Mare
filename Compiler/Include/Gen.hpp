#pragma once

#include "GenHelper.hpp"
#include "PrimitiveTypes.hpp"

namespace Mare
{

static std::map<std::string, std::unique_ptr<Prototype>> FunctionProtos;

inline auto getFunction(std::string Name) -> llvm::Function*
{
  // First, see if the function has already been added to the current module.
  if (auto* F = TheModule->getFunction(Name))
    return F;

  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end())
    return FI->second->codegen();

  // If no existing prototype exists, return null.
  return nullptr;
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static auto CreateEntryBlockAlloca(llvm::Function* TheFunction, llvm::Type* AllocType,
                                   llvm::StringRef VarName) -> llvm::AllocaInst*
{
  llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(AllocType, nullptr, VarName);
}

inline auto NumberExpr::codegen() -> llvm::Value*
{
  if (std::holds_alternative<int64_t>(Val))
  {
    return llvm::ConstantInt::get(ValType, std::get<int64_t>(Val), /*isSigned=*/true);
  }
  else if (std::holds_alternative<int32_t>(Val))
  {
    return llvm::ConstantInt::get(ValType, std::get<int32_t>(Val), /*isSigned=*/true);
  }
  else if (std::holds_alternative<int16_t>(Val))
  {
    return llvm::ConstantInt::get(ValType, std::get<int16_t>(Val), /*isSigned=*/true);
  }
  else if (std::holds_alternative<int8_t>(Val))
  {
    return llvm::ConstantInt::get(ValType, std::get<int8_t>(Val), /*isSigned=*/true);
  }
  else if (std::holds_alternative<double>(Val))
  {
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(std::get<double>(Val)));
  }
  else if (std::holds_alternative<float>(Val))
  {
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(std::get<float>(Val)));
  }

  return nullptr; // Should never hit this if parsing is correct
}

inline auto VariableExpr::codegen() -> Value*
{
  // Look this variable up in the function.
  AllocaInst* V = NamedValues[Name];
  if (!V)
    return LogErrorV("(Var) Unknown variable name");

  // Use stored type or infer from alloca
  Type* loadType = VarType ? VarType : V->getAllocatedType();
  return Builder->CreateLoad(loadType, V, Name.c_str());
}

inline auto UnaryExpr::codegen() -> Value*
{
  Value* OperandV = Operand->codegen();
  if (!OperandV)
    return nullptr;

  llvm::Function* F = getFunction(std::string("unary") + Opcode);
  if (!F)
    return LogErrorV("Unknown unary operator");

  return Builder->CreateCall(F, OperandV, "unop");
}

inline auto BinaryExpr::codegen() -> llvm::Value*
{
  // Handle assignment
  if (Op == '=')
  {
    auto* LHSE = dynamic_cast<VariableExpr*>(LHS.get());
    if (!LHSE)
      return LogErrorV("destination of '=' must be a variable");

    llvm::Value* Val = RHS->codegen();
    if (!Val)
      return nullptr;

    llvm::Value* Variable = NamedValues[LHSE->getName()];
    if (!Variable)
      return LogErrorV("Unknown variable name");

    Builder->CreateStore(Val, Variable);
    return Val;
  }

  llvm::Value* L = LHS->codegen();
  llvm::Value* R = RHS->codegen();
  if (!L || !R)
    return nullptr;

  llvm::Type* LT = L->getType();
  llvm::Type* RT = R->getType();

  // Promote operands to compatible types
  if (LT != RT)
  {
    if (LT->isFloatingPointTy() && RT->isIntegerTy())
    {
      R  = Builder->CreateSIToFP(R, LT, "cast_rhs");
      RT = LT;
    }
    else if (RT->isFloatingPointTy() && LT->isIntegerTy())
    {
      L  = Builder->CreateSIToFP(L, RT, "cast_lhs");
      LT = RT;
    }
    else if (LT->isIntegerTy() && RT->isIntegerTy())
    {
      unsigned LBits = LT->getIntegerBitWidth();
      unsigned RBits = RT->getIntegerBitWidth();
      if (LBits > RBits)
        R = Builder->CreateSExt(R, LT, "cast_rhs");
      else if (RBits > LBits)
        L = Builder->CreateSExt(L, RT, "cast_lhs");
      // else same bits: nothing needed
    }
    else
    {
      llvm::errs() << "[codegen] Cannot reconcile types: ";
      LT->print(llvm::errs());
      llvm::errs() << " vs ";
      RT->print(llvm::errs());
      llvm::errs() << "\n";
      return LogErrorV("Type mismatch in binary expression");
    }
  }

  // Refresh common type
  LT = L->getType();

  switch (Op)
  {
    case '+':
      return LT->isFloatingPointTy() ? Builder->CreateFAdd(L, R, "addtmp")
                                     : Builder->CreateAdd(L, R, "addtmp");
    case '-':
      return LT->isFloatingPointTy() ? Builder->CreateFSub(L, R, "subtmp")
                                     : Builder->CreateSub(L, R, "subtmp");
    case '*':
      return LT->isFloatingPointTy() ? Builder->CreateFMul(L, R, "multmp")
                                     : Builder->CreateMul(L, R, "multmp");
    case '/':
      return LT->isFloatingPointTy() ? Builder->CreateFDiv(L, R, "divtmp")
                                     : Builder->CreateSDiv(L, R, "divtmp");
    case '<':
      return LT->isFloatingPointTy() ? Builder->CreateFCmpULT(L, R, "lt")
                                     : Builder->CreateICmpSLT(L, R, "lt");
    case '>':
      return LT->isFloatingPointTy() ? Builder->CreateFCmpUGT(L, R, "gt")
                                     : Builder->CreateICmpSGT(L, R, "gt");
    default:
      break;
  }

  // User-defined operator fallback
  std::string FnName = "binary";
  FnName += Op;
  if (llvm::Function* F = getFunction(FnName))
    return Builder->CreateCall(F, {L, R}, "binop");

  llvm::errs() << "[codegen] Unknown binary operator '" << Op << "'\n";
  return LogErrorV("Unknown binary operator");
}

inline auto CallExpr::codegen() -> Value*
{
  // Look up the name in the global module table.
  llvm::Function*   CalleeF = getFunction(Callee);
  const std::string errMsg  = "Unknown function referenced: " + Callee;
  if (!CalleeF)
    return LogErrorV(errMsg.c_str());

  // If argument mismatch error.
  if (CalleeF->arg_size() != Args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value*> ArgsV;
  for (const auto& Arg : Args)
  {
    ArgsV.push_back(Arg->codegen());
    if (!ArgsV.back())
      return nullptr;
  }

  // If the function returns void, don't create a named call.
  if (CalleeF->getReturnType()->isVoidTy())
  {
    return Builder->CreateCall(CalleeF, ArgsV);
  }

  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

inline auto StringExpr::codegen() -> llvm::Value*
{
  // Create a global string constant
  llvm::Value* Str = llvm::ConstantDataArray::getString(*TheContext, Val, true);
  auto*        GV =
    new llvm::GlobalVariable(*TheModule, Str->getType(), true, llvm::GlobalValue::PrivateLinkage,
                             llvm::cast<llvm::Constant>(Str), ".str");

  // Generate GEP to get pointer to the string data
  llvm::Value* Zero      = llvm::ConstantInt::get(MARE_INT32_TYPE, 0);
  llvm::Value* StringPtr = Builder->CreateGEP(GV->getValueType(), GV, {Zero, Zero}, "strptr");

  // Return the string pointer directly
  return StringPtr;
}

inline auto IfExpr::codegen() -> Value*
{
  Value* CondV = Cond->codegen();
  if (!CondV)
    return nullptr;

  // Convert condition to a bool by comparing to zero
  // Handle different types for the condition
  Value* ZeroValue = nullptr;
  Type*  CondType  = CondV->getType();

  if (CondType->isIntegerTy())
  {
    ZeroValue = ConstantInt::get(CondType, 0);
    CondV     = Builder->CreateICmpNE(CondV, ZeroValue, "ifcond");
  }
  else if (CondType->isFloatTy())
  {
    ZeroValue = ConstantFP::get(CondType, 0.0f);
    CondV     = Builder->CreateFCmpONE(CondV, ZeroValue, "ifcond");
  }
  else if (CondType->isDoubleTy())
  {
    ZeroValue = ConstantFP::get(CondType, 0.0);
    CondV     = Builder->CreateFCmpONE(CondV, ZeroValue, "ifcond");
  }
  else
  {
    LogErrorV("Unsupported condition type in 'if' expression");
    return nullptr;
  }

  llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();

  // Create blocks for the then and else cases.  Insert the 'then' block at the
  // end of the function.
  BasicBlock* ThenBB  = BasicBlock::Create(*TheContext, "then", TheFunction);
  BasicBlock* ElseBB  = BasicBlock::Create(*TheContext, "else");
  BasicBlock* MergeBB = BasicBlock::Create(*TheContext, "ifcont");

  Builder->CreateCondBr(CondV, ThenBB, ElseBB);

  // Emit then value.
  Builder->SetInsertPoint(ThenBB);
  Value* ThenV = Then->codegen();
  if (!ThenV)
    return nullptr;
  Builder->CreateBr(MergeBB);
  // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
  ThenBB = Builder->GetInsertBlock();

  // Emit else block.
  TheFunction->insert(TheFunction->end(), ElseBB);
  Builder->SetInsertPoint(ElseBB);
  Value* ElseV = Else->codegen();
  if (!ElseV)
    return nullptr;
  Builder->CreateBr(MergeBB);
  // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
  ElseBB = Builder->GetInsertBlock();

  // Emit merge block.
  TheFunction->insert(TheFunction->end(), MergeBB);
  Builder->SetInsertPoint(MergeBB);

  Type* ThenType = ThenV->getType();
  Type* ElseType = ElseV->getType();

  // Handle type promotion/conversion for different ValueVariant types
  if (ThenType != ElseType)
  {
    // Try to find a common type and promote both values
    Type* CommonType = getCommonType(ThenType, ElseType);
    if (!CommonType)
    {
      LogErrorV("Cannot find common type for 'if' expression branches");
      return nullptr;
    }

    // Promote ThenV to common type if needed
    if (ThenType != CommonType)
    {
      ThenV = promoteValue(ThenV, ThenType, CommonType);
      if (!ThenV)
      {
        LogErrorV("Failed to promote 'then' branch value");
        return nullptr;
      }
    }

    // Promote ElseV to common type if needed
    if (ElseType != CommonType)
    {
      ElseV = promoteValue(ElseV, ElseType, CommonType);
      if (!ElseV)
      {
        LogErrorV("Failed to promote 'else' branch value");
        return nullptr;
      }
    }

    ThenType = CommonType;
  }

  llvm::errs() << "\n>>> Creating PHI with types: " << *ThenV->getType() << " and "
               << *ElseV->getType() << "\n";

  PHINode* PN = Builder->CreatePHI(ThenType, 2, "iftmp");
  PN->addIncoming(ThenV, ThenBB);
  PN->addIncoming(ElseV, ElseBB);
  return PN;
}

// Output for-loop as:
//   var = alloca double
//   ...
//   start = startexpr
//   store start -> var
//   goto loop
// loop:
//   ...
//   bodyexpr
//   ...
// loopend:
//   step = stepexpr
//   endcond = endexpr
//
//   curvar = load var
//   nextvar = curvar + step
//   store nextvar -> var
//   br endcond, loop, endloop
// outloop:
inline auto ForExpr::codegen() -> Value*
{
  llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();

  // Emit the start code first to determine the loop variable type
  Value* StartVal = Start->codegen();
  if (!StartVal)
    return nullptr;

  // Get the type from the start value
  Type* LoopVarType = StartVal->getType();

  // Create an alloca for the variable in the entry block using dynamic type
  AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, LoopVarType, VarName);

  // Store the value into the alloca.
  Builder->CreateStore(StartVal, Alloca);

  // Make the new basic block for the loop header, inserting after current block.
  BasicBlock* LoopBB = BasicBlock::Create(*TheContext, "loop", TheFunction);
  // Insert an explicit fall through from the current block to the LoopBB.
  Builder->CreateBr(LoopBB);
  // Start insertion in LoopBB.
  Builder->SetInsertPoint(LoopBB);

  // Within the loop, the variable is defined equal to the PHI node. If it
  // shadows an existing variable, we have to restore it, so save it now.
  AllocaInst* OldVal   = NamedValues[VarName];
  NamedValues[VarName] = Alloca;

  // Emit the body of the loop. This, like any other expr, can change the
  // current BB. Note that we ignore the value computed by the body, but don't
  // allow an error.
  if (!Body->codegen())
    return nullptr;

  // Emit the step value.
  Value* StepVal = nullptr;
  if (Step)
  {
    StepVal = Step->codegen();
    if (!StepVal)
      return nullptr;
  }
  else
  {
    // If not specified, use appropriate default based on type
    if (LoopVarType->isFloatingPointTy() || LoopVarType->isDoubleTy())
    {
      StepVal = ConstantFP::get(LoopVarType, 1.0);
    }
    else if (LoopVarType->isIntegerTy())
    {
      StepVal = ConstantInt::get(LoopVarType, 1);
    }
    else
    {
      return LogErrorV("Unsupported type for loop variable");
    }
  }

  // Compute the end condition.
  Value* EndCond = End->codegen();
  if (!EndCond)
    return nullptr;

  // Reload, increment, and restore the alloca. This handles the case where
  // the body of the loop mutates the variable.
  Value* CurVar = Builder->CreateLoad(LoopVarType, Alloca, VarName.c_str());

  Value* NextVar = nullptr;
  if (LoopVarType->isFloatingPointTy() || LoopVarType->isDoubleTy())
  {
    NextVar = Builder->CreateFAdd(CurVar, StepVal, "nextvar");
  }
  else if (LoopVarType->isIntegerTy())
  {
    NextVar = Builder->CreateAdd(CurVar, StepVal, "nextvar");
  }
  else
  {
    return LogErrorV("Unsupported type for loop arithmetic");
  }

  Builder->CreateStore(NextVar, Alloca);

  // Convert condition to a bool by comparing non-equal to appropriate zero value
  Value* ZeroValue = nullptr;
  if (EndCond->getType()->isFloatingPointTy() || EndCond->getType()->isDoubleTy())
  {
    ZeroValue = ConstantFP::get(EndCond->getType(), 0.0);
    EndCond   = Builder->CreateFCmpONE(EndCond, ZeroValue, "loopcond");
  }
  else if (EndCond->getType()->isIntegerTy())
  {
    ZeroValue = ConstantInt::get(EndCond->getType(), 0);
    EndCond   = Builder->CreateICmpNE(EndCond, ZeroValue, "loopcond");
  }
  else
  {
    return LogErrorV("Unsupported type for loop condition");
  }

  // Create the "after loop" block and insert it.
  BasicBlock* AfterBB = BasicBlock::Create(*TheContext, "afterloop", TheFunction);
  // Insert the conditional branch into the end of LoopEndBB.
  Builder->CreateCondBr(EndCond, LoopBB, AfterBB);
  // Any new code will be inserted in AfterBB.
  Builder->SetInsertPoint(AfterBB);

  // Restore the unshadowed variable.
  if (OldVal)
    NamedValues[VarName] = OldVal;
  else
    NamedValues.erase(VarName);

  // for expr returns zero value of the loop variable type
  return Constant::getNullValue(LoopVarType);
}

inline auto VarExpr::codegen() -> llvm::Value*
{
  llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();

  // Generate initializer
  llvm::Value* InitVal = nullptr;
  if (Init)
  {
    InitVal = Init->codegen();
    if (!InitVal)
      return nullptr;
  }
  else
  {
    // Default to 0.0
    InitVal = llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0));
  }

  llvm::Type* InitType = InitVal->getType();
  InitType->print(llvm::errs());

  // Allocate space for the variable in the entry block
  llvm::AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, InitType, VarName);

  // Store the initializer value
  Builder->CreateStore(InitVal, Alloca);

  // Register in symbol table
  NamedValues[VarName] = Alloca;

  // Return the value just assigned (or null if you'd prefer this to be void)
  return InitVal;
}

inline auto Prototype::codegen() -> llvm::Function*
{
  // Make the function type: RetType(ArgType, ArgType, ...) etc.
  FunctionType* FT = FunctionType::get(RetType, ArgTypes, false); // Use stored return type

  llvm::Function* F =
    llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto& Arg : F->args())
    Arg.setName(Args[Idx++]);

  return F;
}

inline auto FunctionalAST::codegen() -> llvm::Function*
{
  // Transfer ownership of the prototype to the FunctionProtos map.
  auto& P = *Proto;
  fprintf(stderr, "-- Generating Code for '%s'\n", P.getName().c_str());
  FunctionProtos[Proto->getName()] = std::move(Proto);
  llvm::Function* TheFunction      = getFunction(P.getName());
  if (!TheFunction)
    return nullptr;

  // If this is an operator, install it.
  if (P.isBinaryOp())
    Mare::Parser::BinopPrecedence[P.getOperatorName()] = P.getBinaryPrecedence();

  // Create a new basic block to start insertion into.
  BasicBlock* BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
  Builder->SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  for (auto& Arg : TheFunction->args())
  {
    // Create an alloca for this variable.
    AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getType(), Arg.getName());

    // Store the initial value into the alloca.
    Builder->CreateStore(&Arg, Alloca);

    // Add arguments to variable symbol table.
    NamedValues[std::string(Arg.getName())] = Alloca;
  }

  if (Value* RetVal = Body->codegen())
  {
    // If function return type is void, we do not return a value.
    if (!Builder->GetInsertBlock()->getTerminator())
    {
      if (P.getReturnType()->isVoidTy())
        Builder->CreateRetVoid();
      else
        Builder->CreateRet(RetVal);
    }

    // Validate the generated code, checking for consistency.
    verifyFunction(*TheFunction);

    return TheFunction;
  }

  // Error reading body, remove function.
  TheFunction->eraseFromParent();

  if (P.isBinaryOp())
    Mare::Parser::BinopPrecedence.erase(P.getOperatorName());
  return nullptr;
}

inline auto BlockExpr::codegen() -> llvm::Value*
{
  llvm::Value* Last = nullptr;

  for (auto& Expr : Expressions)
  {
    Last = Expr->codegen();
    if (!Last)
      return nullptr;

    // If the current basic block now ends in a return, break early
    llvm::BasicBlock* BB = Builder->GetInsertBlock();
    if (BB && BB->getTerminator())
      break;
  }

  return Last;
}

auto ReturnExpr::codegen() -> llvm::Value*
{
  if (Exp)
  {
    llvm::Value* RetVal = Exp->codegen();
    if (!RetVal)
      return nullptr;

    return Builder->CreateRet(RetVal);
  }

  // For void return
  return Builder->CreateRetVoid();
}

} // namespace Mare
