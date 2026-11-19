#include "ConversionCommon.h"

#include "Dialect/ACIR/SourceCheckBudget.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

#include "mlir/IR/Builders.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"

using namespace mlir;

namespace acir::detail {

ac::StaticExprAttr bitsLiteralAttribute(Type payload, DictionaryAttr occurrence,
                                        MLIRContext *context, bool one) {
  Builder builder(context);
  auto type = cast<ac::BitsType>(payload);
  auto literal = ac::MathIntAttr::get(
      context, llvm::APSInt(llvm::APInt(1, one ? 1 : 0), true));
  auto tree = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
       builder.getNamedAttr(
           "value",
           builder.getDictionaryAttr(
               {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
                builder.getNamedAttr("value", literal)})),
       builder.getNamedAttr("origin", occurrence),
       builder.getNamedAttr("location",
                            type.getWidth().getTree().get("location"))});
  return ac::StaticExprAttr::get(context, tree);
}

bool charge(ac::detail::SourceCheckBudget &budget, Operation *site,
            uint64_t units) {
  return succeeded(budget.debit(units, site));
}

bool prepayOperationHeader(Operation *op,
                           ac::detail::SourceCheckBudget &budget) {
  return charge(budget, op, 3 + uint64_t(op->getNumOperands()) * 2 +
                                uint64_t(op->getNumResults()) * 2 + op->getNumRegions() +
                                op->getRawDictionaryAttrs().size() +
                                op->getPropertiesStorageSize());
}

LogicalResult prepayClone(Operation *root, ac::detail::SourceCheckBudget &budget) {
  SmallVector<Operation *> pending;
  if (!charge(budget, root))
    return failure();
  pending.push_back(root);
  while (!pending.empty()) {
    Operation *op = pending.pop_back_val();
    if (!prepayOperationHeader(op, budget))
      return failure();
    for (Region &region : op->getRegions())
      for (Block &block : region) {
        if (!charge(budget, op, 1 + uint64_t(block.getNumArguments()) * 2))
          return failure();
        for (Operation &nested : block) {
          if (!charge(budget, op))
            return failure();
          pending.push_back(&nested);
        }
      }
  }
  return success();
}

bool knownTrue(Value value, const ac::HardwareAnalysis &hardware,
               ac::detail::SourceCheckBudget &budget, unsigned depth) {
  Operation *site = value.getDefiningOp();
  if (!site || depth > 64 || !charge(budget, site)) return false;
  Type type = value.getType();
  if (!isa<ac::BitsType>(type)) return false;
  auto width = hardware.getPackedWidth(type, {}, site);
  if (failed(width) || *width != 1) return false;
  if (auto rule = value.getDefiningOp<ac::RuleOp>()) {
    auto *terminator = rule.getBody().front().getTerminator();
    auto yielded = dyn_cast_or_null<ac::YieldOp>(terminator);
    auto result = dyn_cast<OpResult>(value);
    if (!yielded || !result ||
        result.getResultNumber() >= yielded.getNumOperands())
      return false;
    return knownTrue(yielded.getOperand(result.getResultNumber()), hardware,
                     budget, depth + 1);
  }
  if (auto constant = value.getDefiningOp<ac::BitsConstantOp>()) {
    auto evaluated =
        hardware.evaluateStatic(constant.getValue(), {}, constant);
    if (failed(evaluated)) return false;
    auto integer = dyn_cast<ac::MathIntAttr>(*evaluated);
    if (!integer) return false;
    llvm::APSInt number(integer.getCanonicalValue());
    return number.getBitWidth() == 1 && number.getBoolValue();
  }
  if (auto binary = value.getDefiningOp<ac::BitsBinaryOp>()) {
    StringRef opcode = binary.getOpcode();
    if (opcode == "and")
      return knownTrue(binary.getLhs(), hardware, budget, depth + 1) &&
             knownTrue(binary.getRhs(), hardware, budget, depth + 1);
    if (opcode == "or") {
      bool lhs = knownTrue(binary.getLhs(), hardware, budget, depth + 1);
      if (lhs) return true;
      return knownTrue(binary.getRhs(), hardware, budget, depth + 1);
    }
    return false;
  }
  if (auto compare = value.getDefiningOp<ac::BitsCompareOp>()) {
    auto constant = [&](Value operand) -> std::optional<llvm::APSInt> {
      auto op = operand.getDefiningOp<ac::BitsConstantOp>();
      if (!op) return std::nullopt;
      auto evaluated = hardware.evaluateStatic(op.getValue(), {}, op);
      if (failed(evaluated)) return std::nullopt;
      auto integer = dyn_cast<ac::MathIntAttr>(*evaluated);
      if (!integer) return std::nullopt;
      return llvm::APSInt(integer.getCanonicalValue());
    };
    auto lhs = constant(compare.getLhs());
    auto rhs = constant(compare.getRhs());
    if (!lhs || !rhs) return false;
    StringRef predicate = compare.getPredicate();
    if (predicate == "eq" || predicate == "case_eq") return *lhs == *rhs;
    if (predicate == "ne") return *lhs != *rhs;
    if (predicate == "ult") return lhs->ult(*rhs);
    if (predicate == "ule") return lhs->ule(*rhs);
    if (predicate == "ugt") return lhs->ugt(*rhs);
    if (predicate == "uge") return lhs->uge(*rhs);
    if (predicate == "slt") return lhs->slt(*rhs);
    if (predicate == "sle") return lhs->sle(*rhs);
    if (predicate == "sgt") return lhs->sgt(*rhs);
    if (predicate == "sge") return lhs->sge(*rhs);
    return false;
  }
  if (auto select = value.getDefiningOp<ac::BitsSelectOp>()) {
    if (!knownTrue(select.getCondition(), hardware, budget, depth + 1))
      return false;
    return knownTrue(select.getTrueValue(), hardware, budget, depth + 1);
  }
  return false;
}

void acceptTransactionCandidate(mlir::ModuleOp input, mlir::ModuleOp staged) {
  // All verification finishes before the first original mutation.
  input->setAttrs(staged->getAttrs());
  input->getRegion(0).takeBody(staged->getRegion(0));
}

} // namespace acir::detail
