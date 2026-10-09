#ifndef ACIR_TESTS_NUMERICFINITESSAORACLE_H
#define ACIR_TESTS_NUMERICFINITESSAORACLE_H

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/APInt.h"

#include <functional>
#include <optional>

namespace acir::ac::test {

inline std::optional<llvm::APInt>
evaluateFiniteSSA(mlir::Value root, mlir::Value sourceReadResult,
                  const llvm::APInt &sourceValue) {
  std::function<std::optional<llvm::APInt>(mlir::Value)> evaluate =
      [&](mlir::Value value) -> std::optional<llvm::APInt> {
    if (value == sourceReadResult)
      return sourceValue;
    if (auto constant = value.getDefiningOp<mlir::arith::ConstantOp>()) {
      auto integer = mlir::dyn_cast<mlir::IntegerAttr>(constant.getValue());
      return integer ? std::optional<llvm::APInt>(integer.getValue())
                     : std::nullopt;
    }
    if (auto extension = value.getDefiningOp<mlir::arith::ExtUIOp>()) {
      auto input = evaluate(extension.getIn());
      auto type = mlir::dyn_cast<mlir::IntegerType>(extension.getType());
      return input && type
                 ? std::optional<llvm::APInt>(input->zext(type.getWidth()))
                 : std::nullopt;
    }
    if (auto extension = value.getDefiningOp<mlir::arith::ExtSIOp>()) {
      auto input = evaluate(extension.getIn());
      auto type = mlir::dyn_cast<mlir::IntegerType>(extension.getType());
      return input && type
                 ? std::optional<llvm::APInt>(input->sext(type.getWidth()))
                 : std::nullopt;
    }
    if (auto add = value.getDefiningOp<mlir::arith::AddIOp>()) {
      auto lhs = evaluate(add.getLhs());
      auto rhs = evaluate(add.getRhs());
      if (lhs && rhs && lhs->getBitWidth() == rhs->getBitWidth())
        return *lhs + *rhs;
    }
    return std::nullopt;
  };
  return evaluate(root);
}

} // namespace acir::ac::test

#endif // ACIR_TESTS_NUMERICFINITESSAORACLE_H
