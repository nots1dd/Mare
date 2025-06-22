#pragma once

#include "Compiler.hpp"
#include "Globals.hpp"
#include <utility>

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

namespace Mare
{

/// Expr - Base class for all expression nodes.
class Expr
{

public:
  virtual ~Expr() = default;

  virtual auto codegen() -> Value* = 0;
};

class BlockExpr : public Expr
{
  std::vector<std::unique_ptr<Expr>> Expressions;

public:
  BlockExpr(std::vector<std::unique_ptr<Expr>> Exprs) : Expressions(std::move(Exprs)) {}

  auto codegen() -> llvm::Value* override;
};

/// NumberExpr - Expression class for numeric literals like "1.0".
class NumberExpr : public Expr
{
private:
  Global::ValueVariant Val;
  llvm::Type*          ValType; // Set during parsing based on token

public:
  NumberExpr(Global::ValueVariant Val, llvm::Type* Type) : Val(std::move(Val)), ValType(Type) {}

  auto codegen() -> llvm::Value* override;
};

class StringExpr : public Expr
{
  std::string Val;

public:
  StringExpr(std::string Val) : Val(std::move(Val)) {}

  auto codegen() -> llvm::Value* override;
};

/// VariableExpr - Expression class for referencing a variable, like "a".
class VariableExpr : public Expr
{
  std::string Name;
  Type*       VarType; // Store the type
public:
  VariableExpr(std::string Name, Type* type = nullptr) : Name(std::move(Name)), VarType(type) {}

  auto               codegen() -> Value* override;
  [[nodiscard]] auto getName() const -> const std::string& { return Name; }
  void               setType(Type* type) { VarType = type; }
  [[nodiscard]] auto getType() const -> Type* { return VarType; }
};

/// UnaryExpr - Expression class for a unary operator.
class UnaryExpr : public Expr
{
  char                  Opcode;
  std::unique_ptr<Expr> Operand;

public:
  UnaryExpr(char Opcode, std::unique_ptr<Expr> Operand)
      : Opcode(Opcode), Operand(std::move(Operand))
  {
  }

  auto codegen() -> Value* override;
};

/// BinaryExpr - Expression class for a binary operator.
class BinaryExpr : public Expr
{
  char                  Op;
  std::unique_ptr<Expr> LHS, RHS;

public:
  BinaryExpr(char Op, std::unique_ptr<Expr> LHS, std::unique_ptr<Expr> RHS)
      : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS))
  {
  }

  auto codegen() -> Value* override;
};

/// CallExpr - Expression class for function calls.
class CallExpr : public Expr
{
  std::string                        Callee;
  std::vector<std::unique_ptr<Expr>> Args;

public:
  CallExpr(std::string Callee, std::vector<std::unique_ptr<Expr>> Args)
      : Callee(std::move(Callee)), Args(std::move(Args))
  {
  }

  auto codegen() -> Value* override;
};

/// IfExpr - Expression class for if/then/else.
class IfExpr : public Expr
{
  std::unique_ptr<Expr> Cond, Then, Else;

public:
  IfExpr(std::unique_ptr<Expr> Cond, std::unique_ptr<Expr> Then, std::unique_ptr<Expr> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else))
  {
  }

  auto codegen() -> Value* override;
};

/// ForExpr - Expression class for for/in.
class ForExpr : public Expr
{
  std::string           VarName;
  std::unique_ptr<Expr> Start, End, Step, Body;

public:
  ForExpr(std::string VarName, std::unique_ptr<Expr> Start, std::unique_ptr<Expr> End,
          std::unique_ptr<Expr> Step, std::unique_ptr<Expr> Body)
      : VarName(std::move(VarName)), Start(std::move(Start)), End(std::move(End)),
        Step(std::move(Step)), Body(std::move(Body))
  {
  }

  auto codegen() -> Value* override;
};

/// VarExpr - Expression class for var keyword
class VarExpr : public Expr
{
  std::string           VarName;
  std::unique_ptr<Expr> Init;

public:
  VarExpr(std::string name, std::unique_ptr<Expr> init)
      : VarName(std::move(name)), Init(std::move(init))
  {
  }

  auto codegen() -> llvm::Value* override;
};

class ReturnExpr : public Expr
{
  std::unique_ptr<Expr> Exp;

public:
  ReturnExpr(std::unique_ptr<Expr> Exp) : Exp(std::move(Exp)) {}

  auto codegen() -> llvm::Value* override;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes), as well as if it is an operator.
class Prototype
{
  std::string              Name;
  std::vector<std::string> Args;
  std::vector<llvm::Type*> ArgTypes;
  bool                     IsOperator;
  unsigned                 Precedence; // Precedence if a binary op.
  llvm::Type*              RetType;

public:
  Prototype(std::string Name, std::vector<std::string> Args, std::vector<llvm::Type*> ArgTypes,
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
class FunctionalAST
{
  std::unique_ptr<Prototype> Proto;
  std::unique_ptr<Expr>      Body;

public:
  FunctionalAST(std::unique_ptr<Prototype> Proto, std::unique_ptr<Expr> Body)
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

} // namespace Mare
