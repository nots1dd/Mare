#pragma once

#include <utility>

#include "Compiler.hpp"

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

namespace GooAST
{

/// ExprAST - Base class for all expression nodes.
class ExprAST
{
public:
  virtual ~ExprAST() = default;

  virtual auto codegen() -> Value* = 0;
};

class BlockExprAST : public ExprAST
{
  std::vector<std::unique_ptr<ExprAST>> Expressions;

public:
  BlockExprAST(std::vector<std::unique_ptr<ExprAST>> Exprs) : Expressions(std::move(Exprs)) {}

  auto codegen() -> llvm::Value* override;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST
{
  double Val;

public:
  NumberExprAST(double Val) : Val(Val) {}

  auto codegen() -> Value* override;
};

class StringExprAST : public ExprAST
{
  std::string Val;

public:
  StringExprAST(std::string Val) : Val(std::move(Val)) {}

  auto codegen() -> llvm::Value* override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST
{
  std::string Name;
  Type*       VarType; // Store the type
public:
  VariableExprAST(std::string Name, Type* type = nullptr) : Name(std::move(Name)), VarType(type) {}

  auto               codegen() -> Value* override;
  [[nodiscard]] auto getName() const -> const std::string& { return Name; }
  void               setType(Type* type) { VarType = type; }
  [[nodiscard]] auto getType() const -> Type* { return VarType; }
};

/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST
{
  char                     Opcode;
  std::unique_ptr<ExprAST> Operand;

public:
  UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
      : Opcode(Opcode), Operand(std::move(Operand))
  {
  }

  auto codegen() -> Value* override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST
{
  char                     Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
      : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS))
  {
  }

  auto codegen() -> Value* override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST
{
  std::string                           Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(std::string Callee, std::vector<std::unique_ptr<ExprAST>> Args)
      : Callee(std::move(Callee)), Args(std::move(Args))
  {
  }

  auto codegen() -> Value* override;
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST
{
  std::unique_ptr<ExprAST> Cond, Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
            std::unique_ptr<ExprAST> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else))
  {
  }

  auto codegen() -> Value* override;
};

/// ForExprAST - Expression class for for/in.
class ForExprAST : public ExprAST
{
  std::string              VarName;
  std::unique_ptr<ExprAST> Start, End, Step, Body;

public:
  ForExprAST(std::string VarName, std::unique_ptr<ExprAST> Start, std::unique_ptr<ExprAST> End,
             std::unique_ptr<ExprAST> Step, std::unique_ptr<ExprAST> Body)
      : VarName(std::move(VarName)), Start(std::move(Start)), End(std::move(End)),
        Step(std::move(Step)), Body(std::move(Body))
  {
  }

  auto codegen() -> Value* override;
};

/// VarExprAST - Expression class for var keyword
class VarExprAST : public ExprAST
{
  std::string              VarName;
  std::unique_ptr<ExprAST> Init;

public:
  VarExprAST(std::string name, std::unique_ptr<ExprAST> init)
      : VarName(std::move(name)), Init(std::move(init))
  {
  }

  auto codegen() -> llvm::Value* override;
};

class ReturnExprAST : public ExprAST
{
  std::unique_ptr<ExprAST> Expr;

public:
  ReturnExprAST(std::unique_ptr<ExprAST> Expr) : Expr(std::move(Expr)) {}

  auto codegen() -> llvm::Value* override;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes), as well as if it is an operator.
class PrototypeAST
{
  std::string              Name;
  std::vector<std::string> Args;
  std::vector<llvm::Type*> ArgTypes;
  bool                     IsOperator;
  unsigned                 Precedence; // Precedence if a binary op.
  llvm::Type*              RetType;

public:
  PrototypeAST(std::string Name, std::vector<std::string> Args, std::vector<llvm::Type*> ArgTypes,
               llvm::Type* RetType, bool IsOperator = false, unsigned Prec = 0)
      : Name(std::move(Name)), Args(std::move(Args)), ArgTypes(std::move(ArgTypes)),
        RetType(RetType), IsOperator(IsOperator), Precedence(Prec)
  {
    assert(Args.size() == ArgTypes.size() && "Argument names and types must match in count");
  }

  auto               codegen() -> Function*;
  [[nodiscard]] auto getName() const -> const std::string& { return Name; }
  [[nodiscard]] auto getArgs() const -> const std::vector<std::string>& { return Args; }
  [[nodiscard]] auto getArgTypes() const -> const std::vector<llvm::Type*>& { return ArgTypes; }

  [[nodiscard]] auto isUnaryOp() const -> bool { return IsOperator && Args.size() == 1; }
  [[nodiscard]] auto isBinaryOp() const -> bool { return IsOperator && Args.size() == 2; }

  [[nodiscard]] auto getOperatorName() const -> char
  {
    assert(isUnaryOp() || isBinaryOp());
    return Name[Name.size() - 1];
  }

  [[nodiscard]] auto getBinaryPrecedence() const -> unsigned { return Precedence; }

  [[nodiscard]] auto getReturnType() const -> llvm::Type*
  {
    return RetType;
  } // Getter for return type
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST
{
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST>      Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body)
      : Proto(std::move(Proto)), Body(std::move(Body))
  {
  }

  auto               codegen() -> Function*;
  [[nodiscard]] auto getName() const -> const std::string&
  {
    if (!Proto)
      throw std::runtime_error("FunctionAST::getName() called with null Prototype");
    return Proto->getName();
  }
  [[nodiscard]] auto getReturnType() const -> llvm::Type* { return Proto->getReturnType(); }
};

} // namespace GooAST
