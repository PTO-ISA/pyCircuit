#pragma once
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include <functional>
#include <map>
#include <set>
#include <sstream>
namespace acir {
// Backend-only statement planning. No frontend AST or scheduler lives here.
std::string cppIdentifier(llvm::StringRef name);
std::string cppSourceName(mlir::Value value);
struct CppPlan {
  llvm::DenseMap<mlir::Value, std::set<int64_t>> transactions;
  llvm::DenseMap<mlir::Value, unsigned> uses;
  llvm::DenseMap<mlir::Value, unsigned> boundary;
  std::vector<std::vector<mlir::Value>> blocks;
  explicit CppPlan(mlir::ModuleOp module);
};
struct CppStatements {
  const CppPlan &plan;
  std::function<std::string(mlir::Type)> type;
  std::function<std::string(mlir::Value, llvm::ArrayRef<std::string>)>
      expression;
  std::function<std::string(mlir::Value)> shared;
  llvm::DenseMap<mlir::Value, std::string> available, computed;
  llvm::DenseSet<mlir::Value> ignored;
  std::vector<mlir::Value> facts;
  std::map<std::string, unsigned> names;
  std::set<std::string> assigned{"ctx", "Context"};
  std::ostringstream code;
  std::string render() const { return code.str(); }
  unsigned depth = 0;
  CppStatements(
      const CppPlan &plan, std::function<std::string(mlir::Type)> type,
      std::function<std::string(mlir::Value, llvm::ArrayRef<std::string>)>
          expression,
      std::function<std::string(mlir::Value)> shared);
  std::string name(std::string base);
  void line(const std::string &text);
  std::string value(mlir::Value v);
  void assume(mlir::Value condition);
  void conditional(const std::string &condition,
                   const std::function<void()> &body);
};
} // namespace acir
