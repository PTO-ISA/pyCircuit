#include "Dialect.h"
#include "Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "mlir/Pass/Pass.h"
using namespace mlir;
namespace {
void expand(mlir::ModuleOp module) {
  SmallVector<Operation *> accesses;
  module.walk([&](Operation *op) {
    if (isa<acir::ReadOp, acir::EffectOp>(op) && op->hasAttr("resources"))
      accesses.push_back(op);
  });
  for (auto *op : accesses) {
    OpBuilder b(op);
    auto loc = op->getLoc();
    bool read = isa<acir::ReadOp>(op);
    bool pop = op->getAttrOfType<StringAttr>("method").getValue() == "pop";
    unsigned indexArg = read || pop ? 1 : 2;
    auto index = op->getOperand(indexArg);
    auto resources = op->getAttrOfType<ArrayAttr>("resources");
    auto ports = op->getAttrOfType<ArrayAttr>("ports");
    Value data, ready;
    if (read) {
      OperationState zero(loc, "acir.aggregate");
      zero.addTypes(op->getResult(0).getType());
      data = b.create(zero)->getResult(0);
      ready = arith::ConstantIntOp::create(b, loc, 0, 1);
    }
    for (auto [i, resource] : llvm::enumerate(resources)) {
      auto coordinate = arith::ConstantIntOp::create(
          b, loc, i, cast<IntegerType>(index.getType()).getWidth());
      Value selected = arith::CmpIOp::create(b, loc, arith::CmpIPredicate::eq,
                                             index, coordinate);
      if (op->getNumOperands() > indexArg + 1) {
        OperationState extract(loc, "acir.extract");
        extract.addOperands({op->getOperand(indexArg + 1), coordinate});
        extract.addTypes(b.getI1Type());
        selected = arith::AndIOp::create(b, loc, selected,
                                         b.create(extract)->getResult(0));
      }
      Value enable = arith::AndIOp::create(b, loc, op->getOperand(0), selected);
      OperationState state(loc, op->getName());
      state.addAttributes(op->getAttrs());
      state.attributes.erase("resources");
      state.attributes.erase("ports");
      state.attributes.erase("array");
      state.attributes.set("resource", resource);
      state.attributes.set("port", ports[i]);
      state.addOperands(enable);
      if (!read && !pop)
        state.addOperands(op->getOperand(1));
      state.addTypes(op->getResultTypes());
      auto *physical = b.create(state);
      if (read) {
        data = arith::SelectOp::create(b, loc, selected, physical->getResult(0),
                                       data);
        auto available =
            arith::AndIOp::create(b, loc, selected, physical->getResult(1));
        ready = arith::OrIOp::create(b, loc, ready, available);
      }
    }
    if (read) {
      op->getResult(0).replaceAllUsesWith(data);
      op->getResult(1).replaceAllUsesWith(ready);
    }
    op->erase();
  }
}
struct Expansion : PassWrapper<Expansion, OperationPass<mlir::ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(Expansion)
  StringRef getArgument() const final { return "acir-expand-resources"; }
  StringRef getDescription() const final {
    return "Expand logical indexed accesses for RTL";
  }
  void runOnOperation() override { expand(getOperation()); }
};
} // namespace
std::unique_ptr<Pass> acir::createResourceExpansionPass() {
  return std::make_unique<Expansion>();
}
