#ifndef PYCIRCUIT_DIALECT_GFIR_STRUCTUREDTABLEMAP_H
#define PYCIRCUIT_DIALECT_GFIR_STRUCTUREDTABLEMAP_H

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include <optional>

namespace acir::gfir::detail {

class TableMapRow {
public:
  struct Uniform {
    mlir::Value value;
    unsigned operand;
    bool splat;
  };

  TableMapRow(ac::TableMapOp map, unsigned result)
      : map(map), body(map.getBody().front()),
        value(llvm::cast<ac::YieldOp>(body.getTerminator()).getValues()[result]) {}

  std::optional<unsigned> tableOperand(mlir::Value row) {
    auto input = operand(row);
    if (input && *input < map.getTables().size())
      return input;
    return std::nullopt;
  }

  std::optional<Uniform> uniform(mlir::Value row) {
    auto input = operand(row);
    if (!input)
      return std::nullopt;
    auto bound = map->getOperand(*input);
    if (*input >= map.getTables().size())
      return Uniform{bound, *input, false};
    if (auto splat = bound.getDefiningOp<ac::TableSplatOp>())
      return Uniform{splat.getInput(), *input, true};
    return std::nullopt;
  }

  std::optional<Uniform> matchingIndex(mlir::Value condition) {
    auto compare = condition.getDefiningOp<ac::BitsCompareOp>();
    if (!compare || compare.getPredicate() != "eq")
      return std::nullopt;
    auto matchesOrdinal = [&](mlir::Value operand) {
      if (auto resize = operand.getDefiningOp<ac::BitsResizeOp>();
          resize && resize.getMode() == "zext")
        operand = resize.getInput();
      return operand == body.getArgument(0);
    };
    if (matchesOrdinal(compare.getLhs()))
      return uniform(compare.getRhs());
    if (matchesOrdinal(compare.getRhs()))
      return uniform(compare.getLhs());
    return std::nullopt;
  }

  mlir::Value getValue() const { return value; }

private:
  std::optional<unsigned> operand(mlir::Value row) const {
    auto argument = llvm::dyn_cast<mlir::BlockArgument>(row);
    if (!argument || argument.getOwner() != &body || argument.getArgNumber() == 0)
      return std::nullopt;
    return argument.getArgNumber() - 1;
  }

  ac::TableMapOp map;
  mlir::Block &body;
  mlir::Value value;
};

} // namespace acir::gfir::detail

#endif // PYCIRCUIT_DIALECT_GFIR_STRUCTUREDTABLEMAP_H
