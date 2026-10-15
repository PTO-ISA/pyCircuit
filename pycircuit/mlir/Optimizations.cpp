#include "Dialect.h"
#include "Execution.h"
#include "FieldProjection.h"
#include "Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include <optional>

using namespace mlir;
namespace {
bool hasBinding(Location loc) {
  if (auto named = dyn_cast<NameLoc>(loc))
    return named.getName().getValue().starts_with("acir.binding:") ||
           hasBinding(named.getChildLoc());
  if (auto call = dyn_cast<CallSiteLoc>(loc))
    return hasBinding(call.getCallee());
  if (auto fused = dyn_cast<FusedLoc>(loc))
    return llvm::any_of(fused.getLocations(), hasBinding);
  return false;
}
// Names describe values; they must not make otherwise identical computations
// different to canonicalization or CSE. Keep the first source binding as the
// stable primary name and let later aliases refer to the same SSA value.
struct Bindings : PassWrapper<Bindings, OperationPass<mlir::ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(Bindings)
  StringRef getArgument() const final { return "acir-normalize-bindings"; }
  StringRef getDescription() const final {
    return "Preserve primary source names while removing semantic-free aliases";
  }
  void runOnOperation() override {
    SmallVector<acir::BindOp> bindings;
    getOperation().walk([&](Operation *op) {
      if (isMemoryEffectFree(op)) {
        op->removeAttr("acir.id");
        op->removeAttr("acir.source_type");
      }
      if (auto binding = dyn_cast<acir::BindOp>(op))
        bindings.push_back(binding);
    });
    for (auto binding : bindings) {
      Value input = binding->getOperand(0);
      if (auto *definition = input.getDefiningOp()) {
        // A multi-result operation has one location. Keep each result's own
        // binding until transaction extraction creates its individual value.
        if (definition->getNumResults() != 1)
          continue;
        if (!isa<arith::ConstantOp>(definition) &&
            !hasBinding(definition->getLoc()))
          definition->setLoc(binding.getLoc());
      }
      binding->getResult(0).replaceAllUsesWith(input);
      binding.erase();
    }
  }
};
// Fold only statically known, in-bounds accesses. Dynamic indexing retains
// its existing readiness and bounds handling.
std::optional<uint64_t> indexOf(acir::ExtractOp extract) {
  if (auto field = extract.getField()) {
    auto root = extract->getParentOfType<mlir::ModuleOp>();
    if (auto index = acir::fieldIndex(
            extract.getAggregate().getType(),
            StringAttr::get(extract.getContext(), *field), root))
      return *index;
    return {};
  }
  if (extract.getIndices().size() != 1)
    return {};
  if (auto constant =
          extract.getIndices()[0].getDefiningOp<arith::ConstantOp>())
    if (auto index = dyn_cast<IntegerAttr>(constant.getValue()))
      return index.getValue().getLimitedValue();
  return {};
}
} // namespace

OpFoldResult acir::UnaryOp::fold(FoldAdaptor adaptor) {
  if (!adaptor.getInput())
    return {};
  return calculate(*this, {adaptor.getInput()},
                   (*this)->getParentOfType<mlir::ModuleOp>());
}
namespace {
struct BooleanNot : OpRewritePattern<acir::UnaryOp> {
  using OpRewritePattern::OpRewritePattern;
  LogicalResult matchAndRewrite(acir::UnaryOp op,
                                PatternRewriter &rewriter) const override {
    if (op.getOperatorName() != "not" || !op.getInput().getType().isInteger(1))
      return failure();
    auto yes = arith::ConstantIntOp::create(rewriter, op.getLoc(), 1, 1);
    rewriter.replaceOpWithNewOp<arith::XOrIOp>(op, op.getInput(), yes);
    return success();
  }
};
struct TrivialCheck : OpRewritePattern<acir::CheckOp> {
  using OpRewritePattern::OpRewritePattern;
  LogicalResult matchAndRewrite(acir::CheckOp op,
                                PatternRewriter &rewriter) const override {
    auto c = op.getCondition().getDefiningOp<arith::ConstantOp>();
    if (!c || !cast<IntegerAttr>(c.getValue()).getValue().isOne())
      return failure();
    rewriter.eraseOp(op);
    return success();
  }
};
} // namespace
void acir::UnaryOp::getCanonicalizationPatterns(RewritePatternSet &patterns,
                                                MLIRContext *context) {
  patterns.add<BooleanNot>(context);
}
void acir::CheckOp::getCanonicalizationPatterns(RewritePatternSet &patterns,
                                                MLIRContext *context) {
  patterns.add<TrivialCheck>(context);
}
OpFoldResult acir::AssertOp::fold(FoldAdaptor adaptor) {
  auto c = dyn_cast_or_null<IntegerAttr>(adaptor.getCondition());
  return c && c.getValue().isOne() ? OpFoldResult(c) : OpFoldResult();
}

OpFoldResult acir::ExtractOp::fold(FoldAdaptor) {
  if (acir::containsAssertion(getAggregate()))
    return {};
  auto aggregate = getAggregate().getDefiningOp<AggregateOp>();
  auto index = indexOf(*this);
  // An empty aggregate represents zero, rather than explicit elements.
  // Keep a reused record as a shared structured value. Scalar replacement of
  // every field duplicates its branch network at each consumer and destroys
  // the useful computation boundary for either lowering. Tuples used for
  // compiler control plumbing remain freely foldable.
  if (aggregate && isa<StructType>(getAggregate().getType()) &&
      !getAggregate().hasOneUse())
    return {};
  if (aggregate && index && *index < aggregate.getElements().size())
    return aggregate.getElements()[*index];
  return {};
}

OpFoldResult acir::AggregateOp::fold(FoldAdaptor) {
  if (getElements().empty())
    return {};
  Value original;
  for (auto [i, value] : llvm::enumerate(getElements())) {
    auto extract = value.getDefiningOp<ExtractOp>();
    if (!extract || indexOf(extract) != i)
      return {};
    if (!original)
      original = extract.getAggregate();
    if (extract.getAggregate() != original)
      return {};
  }
  return original.getType() == getValue().getType() ? OpFoldResult(original)
                                                    : OpFoldResult();
}

void acir::addCleanupPasses(OpPassManager &pm) {
  pm.addPass(std::make_unique<Bindings>());
  pm.addPass(createCanonicalizerPass());
  pm.addPass(createCSEPass());
  pm.addPass(createCanonicalizerPass());
  pm.addPass(createConditionPass());
  pm.addPass(createCanonicalizerPass());
  pm.addPass(createCSEPass());
}

void acir::addSourcePasses(OpPassManager &pm) {
  // Source stays structured and resource-aware. This pipeline is shared by
  // both backends and introduces no C++ or RTL-specific representation.
  pm.addPass(std::make_unique<Bindings>());
  auto simplify = [](OpPassManager &nested) {
    nested.addPass(createCanonicalizerPass());
    nested.addPass(createCSEPass());
  };
  simplify(pm.nest<func::FuncOp>());
  auto &modules = pm.nest<acir::ModuleOp>();
  simplify(modules.nest<acir::RuleOp>());
  // Module bodies define the static transaction inventory. Keep their call
  // sites, even below a constant-false condition, so optimization does not
  // change Rule identities, schedule IDs or the fire trace schema.
}
