#pragma once

#include "AST.hpp"
#include "Parser.hpp"

using namespace GooAST;

static std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

inline auto LogErrorV(const char* Str) -> Value*
{
  LogError(Str);
  return nullptr;
}

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
static auto CreateEntryBlockAlloca(llvm::Function* TheFunction, StringRef VarName) -> AllocaInst*
{
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(Type::getDoubleTy(*TheContext), nullptr, VarName);
}

inline auto NumberExprAST::codegen() -> Value*
{
  return ConstantFP::get(*TheContext, APFloat(Val));
}

inline auto VariableExprAST::codegen() -> Value*
{
  // Look this variable up in the function.
  Value* V = NamedValues[Name];
  if (!V)
    return LogErrorV("(Var) Unknown variable name");

  // Load the value.
  return Builder->CreateLoad(Type::getDoubleTy(*TheContext), V, Name.c_str());
}

inline auto UnaryExprAST::codegen() -> Value*
{
  Value* OperandV = Operand->codegen();
  if (!OperandV)
    return nullptr;

  llvm::Function* F = getFunction(std::string("unary") + Opcode);
  if (!F)
    return LogErrorV("Unknown unary operator");

  return Builder->CreateCall(F, OperandV, "unop");
}

inline auto BinaryExprAST::codegen() -> llvm::Value*
{
  // Handle assignment separately
  if (Op == '=')
  {
    auto* LHSE = static_cast<VariableExprAST*>(LHS.get());
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

  // Standard Numeric Operations
  switch (Op)
  {
    case '+':
      return Builder->CreateFAdd(L, R, "addtmp");
    case '-':
      return Builder->CreateFSub(L, R, "subtmp");
    case '*':
      return Builder->CreateFMul(L, R, "multmp");
    case '<':
    {
      L = Builder->CreateFCmpULT(L, R, "cmptmp");
      return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmp");
    }
    default:
      break;
  }

  // Handle user-defined operators
  llvm::Function* F = getFunction(std::string("binary") + Op);
  assert(F && "binary operator not found!");

  llvm::Value* Ops[] = {L, R};
  return Builder->CreateCall(F, Ops, "binop");
}

inline auto CallExprAST::codegen() -> Value*
{
  // Look up the name in the global module table.
  llvm::Function* CalleeF = getFunction(Callee);
  if (!CalleeF)
    return LogErrorV("Unknown function referenced");

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

inline auto StringExprAST::codegen() -> llvm::Value*
{
  // Create a global string constant
  llvm::Value* Str = llvm::ConstantDataArray::getString(*TheContext, Val, true);
  auto*        GV =
    new llvm::GlobalVariable(*TheModule, Str->getType(), true, llvm::GlobalValue::PrivateLinkage,
                             llvm::cast<llvm::Constant>(Str), ".str");

  // Get the pointer type (i8*)
  llvm::Type* Int8Ty    = llvm::Type::getInt8Ty(*TheContext);
  llvm::Type* Int8PtrTy = llvm::PointerType::get(Int8Ty, 0);

  // Generate GEP (use GV->getType()->getElementType())
  llvm::Value* Zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0);
  llvm::Value* GEP  = Builder->CreateGEP(GV->getValueType(), GV, {Zero, Zero}, "strptr");

  // Get or declare printf
  llvm::Function* PrintfFunc = TheModule->getFunction("printf");
  if (!PrintfFunc)
  {
    llvm::FunctionType* PrintfType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*TheContext),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*TheContext), 0)}, true);
    PrintfFunc =
      llvm::Function::Create(PrintfType, llvm::Function::ExternalLinkage, "printf", *TheModule);
  }

  // Create a format string ("%s") as a global constant
  llvm::Value* FormatStr = Builder->CreateGlobalStringPtr("%s", ".fmt");

  // Call printf with (FormatStr, GEP)
  llvm::Value* PrintfCall = Builder->CreateCall(PrintfFunc, {FormatStr, GEP}, "printf_call");

  // Convert printf's return value from i32 to double
  llvm::Value* PrintfCallDouble =
    Builder->CreateSIToFP(PrintfCall, llvm::Type::getDoubleTy(*TheContext), "printf_call_d");

  // Compare printf's return value with 0.0
  llvm::Value* SuccessCheck = Builder->CreateFCmpONE(
    PrintfCallDouble, llvm::ConstantFP::get(llvm::Type::getDoubleTy(*TheContext), 0.0),
    "success_check");

  // Return 0.0 if success, -1.0 if failure
  return Builder->CreateSelect(
    SuccessCheck, llvm::ConstantFP::get(llvm::Type::getDoubleTy(*TheContext), 0.0),
    llvm::ConstantFP::get(llvm::Type::getDoubleTy(*TheContext), -1.0), "ret_val");
}

inline auto getPrintFunction() -> llvm::Function*
{
  // printf function prototype: int printf(char*, ...)
  llvm::FunctionType* PrintfType =
    llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext),
                            {llvm::PointerType::get(llvm::Type::getInt8Ty(*TheContext), 0)},
                            true // Variadic
    );

  llvm::Function* PrintfFunc =
    llvm::Function::Create(PrintfType, llvm::Function::ExternalLinkage, "printf", *TheModule);

  return PrintfFunc;
}

inline auto IfExprAST::codegen() -> Value*
{
  Value* CondV = Cond->codegen();
  if (!CondV)
    return nullptr;

  // Convert condition to a bool by comparing non-equal to 0.0.
  CondV = Builder->CreateFCmpONE(CondV, ConstantFP::get(*TheContext, APFloat(0.0)), "ifcond");

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
  PHINode* PN = Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, "iftmp");

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
inline auto ForExprAST::codegen() -> Value*
{
  llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();

  // Create an alloca for the variable in the entry block.
  AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, VarName);

  // Emit the start code first, without 'variable' in scope.
  Value* StartVal = Start->codegen();
  if (!StartVal)
    return nullptr;

  // Store the value into the alloca.
  Builder->CreateStore(StartVal, Alloca);

  // Make the new basic block for the loop header, inserting after current
  // block.
  BasicBlock* LoopBB = BasicBlock::Create(*TheContext, "loop", TheFunction);

  // Insert an explicit fall through from the current block to the LoopBB.
  Builder->CreateBr(LoopBB);

  // Start insertion in LoopBB.
  Builder->SetInsertPoint(LoopBB);

  // Within the loop, the variable is defined equal to the PHI node.  If it
  // shadows an existing variable, we have to restore it, so save it now.
  AllocaInst* OldVal   = NamedValues[VarName];
  NamedValues[VarName] = Alloca;

  // Emit the body of the loop.  This, like any other expr, can change the
  // current BB.  Note that we ignore the value computed by the body, but don't
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
    // If not specified, use 1.0.
    StepVal = ConstantFP::get(*TheContext, APFloat(1.0));
  }

  // Compute the end condition.
  Value* EndCond = End->codegen();
  if (!EndCond)
    return nullptr;

  // Reload, increment, and restore the alloca.  This handles the case where
  // the body of the loop mutates the variable.
  Value* CurVar  = Builder->CreateLoad(Type::getDoubleTy(*TheContext), Alloca, VarName.c_str());
  Value* NextVar = Builder->CreateFAdd(CurVar, StepVal, "nextvar");
  Builder->CreateStore(NextVar, Alloca);

  // Convert condition to a bool by comparing non-equal to 0.0.
  EndCond = Builder->CreateFCmpONE(EndCond, ConstantFP::get(*TheContext, APFloat(0.0)), "loopcond");

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

  // for expr always returns 0.0.
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

inline auto VarExprAST::codegen() -> Value*
{
  std::vector<AllocaInst*> OldBindings;

  llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();

  // Register all variables and emit their initializer.
  for (auto& i : VarNames)
  {
    const std::string& VarName = i.first;
    ExprAST*           Init    = i.second.get();

    // Emit the initializer before adding the variable to scope, this prevents
    // the initializer from referencing the variable itself, and permits stuff
    // like this:
    //  var a = 1 in
    //    var a = a in ...   # refers to outer 'a'.
    Value* InitVal;
    if (Init)
    {
      InitVal = Init->codegen();
      if (!InitVal)
        return nullptr;
    }
    else
    { // If not specified, use 0.0.
      InitVal = ConstantFP::get(*TheContext, APFloat(0.0));
    }

    AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
    Builder->CreateStore(InitVal, Alloca);

    // Remember the old variable binding so that we can restore the binding when
    // we unrecurse.
    OldBindings.push_back(NamedValues[VarName]);

    // Remember this binding.
    NamedValues[VarName] = Alloca;
  }

  // Codegen the body, now that all vars are in scope.
  Value* BodyVal = Body->codegen();
  if (!BodyVal)
    return nullptr;

  // Pop all our variables from scope.
  for (unsigned i = 0, e = VarNames.size(); i != e; ++i)
    NamedValues[VarNames[i].first] = OldBindings[i];

  // Return the body computation.
  return BodyVal;
}

inline auto PrototypeAST::codegen() -> llvm::Function*
{
  // Make the function type: RetType(ArgType, ArgType, ...) etc.
  std::vector<Type*> ArgTypes(Args.size(), Type::getDoubleTy(*TheContext));
  FunctionType*      FT = FunctionType::get(RetType, ArgTypes, false); // Use stored return type

  llvm::Function* F =
    llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto& Arg : F->args())
    Arg.setName(Args[Idx++]);

  return F;
}

inline auto FunctionAST::codegen() -> llvm::Function*
{
  // Transfer ownership of the prototype to the FunctionProtos map.
  auto& P                          = *Proto;
  FunctionProtos[Proto->getName()] = std::move(Proto);
  llvm::Function* TheFunction      = getFunction(P.getName());
  if (!TheFunction)
    return nullptr;

  // If this is an operator, install it.
  if (P.isBinaryOp())
    BinopPrecedence[P.getOperatorName()] = P.getBinaryPrecedence();

  // Create a new basic block to start insertion into.
  BasicBlock* BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
  Builder->SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  for (auto& Arg : TheFunction->args())
  {
    // Create an alloca for this variable.
    AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName());

    // Store the initial value into the alloca.
    Builder->CreateStore(&Arg, Alloca);

    // Add arguments to variable symbol table.
    NamedValues[std::string(Arg.getName())] = Alloca;
  }

  if (Value* RetVal = Body->codegen())
  {
    // If function return type is void, we do not return a value.
    if (P.getReturnType()->isVoidTy())
      Builder->CreateRetVoid();
    else
      Builder->CreateRet(RetVal);

    // Validate the generated code, checking for consistency.
    verifyFunction(*TheFunction);

    return TheFunction;
  }

  // Error reading body, remove function.
  TheFunction->eraseFromParent();

  if (P.isBinaryOp())
    BinopPrecedence.erase(P.getOperatorName());
  return nullptr;
}
