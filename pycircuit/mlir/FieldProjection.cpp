#include "FieldProjection.h"
#include "Dialect.h"
#include "Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/DenseSet.h"
#include <functional>
#include <map>
using namespace mlir;
bool acir::containsAssertion(Value value, bool explicitOnly) {
  llvm::DenseSet<Value> seen;
  std::function<bool(Value)> visit = [&](Value v) {
    if (!seen.insert(v).second)
      return false;
    auto *op = v.getDefiningOp();
    if (!op)
      return false;
    auto kind = op->getAttrOfType<StringAttr>("check_kind");
    if (isa<AssertOp>(op) &&
        (!explicitOnly || !kind || kind.getValue() != "return_coverage"))
      return true;
    unsigned first = isa<ReadOp>(op) ? 1 : 0;
    return llvm::any_of(op->getOperands().drop_front(first), visit);
  };
  return visit(value);
}
std::optional<unsigned> acir::fieldIndex(Type type, Attribute field,
                                         mlir::ModuleOp module) {
  if (auto name = dyn_cast<StringAttr>(field)) {
    auto record = dyn_cast<StructType>(type);
    if (!record)
      return {};
    auto fields = module->getAttrOfType<DictionaryAttr>("acir.model")
                      .getAs<DictionaryAttr>("types")
                      .getAs<ArrayAttr>(record.getName());
    for (auto [i, item] : llvm::enumerate(fields))
      if (cast<DictionaryAttr>(item).getAs<StringAttr>("name") == name)
        return i;
  } else if (auto index = dyn_cast<IntegerAttr>(field)) {
    if (auto tuple = dyn_cast<TupleType>(type))
      if (index.getValue().ult(tuple.size()))
        return index.getInt();
  }
  return {};
}
namespace {
struct Projections : PassWrapper<Projections, OperationPass<mlir::ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(Projections)
  StringRef getArgument() const final { return "acir-project-fields"; }
  StringRef getDescription() const final {
    return "Resolve static field dependencies without cloning their "
           "computations";
  }
  void runOnOperation() override {
    auto module = getOperation();
    OpBuilder b(module.getContext());
    std::map<std::tuple<Block *, const void *, const void *>, Value> cache;
    DenseMap<Value, bool> assertions;
    std::function<Value(Value, Attribute, Type, Location)> project =
        [&](Value value, Attribute field, Type type, Location loc) -> Value {
      auto key =
          std::make_tuple(b.getInsertionBlock(), value.getAsOpaquePointer(),
                          field.getAsOpaquePointer());
      if (auto found = cache.find(key); found != cache.end())
        return found->second;
      auto *op = value.getDefiningOp();
      auto n = op ? op->getName().getStringRef() : StringRef();
      Value result;
      if (n == "acir.bind" || n == "acir.export")
        result = project(op->getOperand(0), field, type, loc);
      else if (auto aggregate = dyn_cast_or_null<acir::AggregateOp>(op)) {
        auto index = acir::fieldIndex(value.getType(), field, module);
        if (index && *index < aggregate.getElements().size())
          result = aggregate.getElements()[*index];
      } else if (auto select = dyn_cast_or_null<arith::SelectOp>(op)) {
        auto yes = project(select.getTrueValue(), field, type, loc);
        auto no = project(select.getFalseValue(), field, type, loc);
        result =
            arith::SelectOp::create(b, loc, select.getCondition(), yes, no);
      } else if (n == "acir.update") {
        auto path = op->getAttrOfType<ArrayAttr>("path");
        // A record update's first step has no dynamic coordinate. Remaining
        // dynamic coordinates are retained in the projected nested update.
        if (!path.empty() && isa<StringAttr>(path[0]) &&
            isa<StringAttr>(field)) {
          if (path[0] != field)
            result = project(op->getOperand(0), field, type, loc);
          else if (path.size() == 1)
            result = op->getOperand(1);
          else {
            auto old = project(op->getOperand(0), field, type, loc);
            OperationState state(loc, "acir.update");
            state.addOperands(old);
            state.addOperands(op->getOperands().drop_front());
            state.addTypes(type);
            state.addAttributes(op->getAttrs());
            state.attributes.set("path",
                                 b.getArrayAttr(path.getValue().drop_front()));
            result = b.create(state)->getResult(0);
          }
        }
      }
      if (!result) {
        OperationState state(loc, "acir.extract");
        state.addOperands(value);
        state.addTypes(type);
        if (isa<StringAttr>(field))
          state.addAttribute("field", field);
        else
          state.addOperands(
              arith::ConstantOp::create(b, loc, cast<TypedAttr>(field))
                  .getResult());
        result = b.create(state)->getResult(0);
      }
      return cache[key] = result;
    };
    SmallVector<acir::ExtractOp> extracts;
    module.walk([&](acir::ExtractOp op) { extracts.push_back(op); });
    for (auto op : extracts) {
      Attribute field = op->getAttr("field");
      if (!field && op.getIndices().size() == 1)
        if (auto c = op.getIndices()[0].getDefiningOp<arith::ConstantOp>())
          field = c.getValue();
      if (!field ||
          !acir::fieldIndex(op.getAggregate().getType(), field, module))
        continue;
      auto *definition = op.getAggregate().getDefiningOp();
      if (!definition || isa<acir::ReadOp>(definition))
        continue;
      // Standalone transaction inputs may embed an assertion in a record's
      // data expression rather than in fire checks. Keep that evaluation
      // obligation; ordinary source assertions already have separate checks.
      if (!assertions.count(op.getAggregate()))
        assertions[op.getAggregate()] =
            acir::containsAssertion(op.getAggregate());
      if (assertions.at(op.getAggregate()))
        continue;
      b.setInsertionPoint(op);
      auto replacement = project(op.getAggregate(), field,
                                 op.getValue().getType(), op.getLoc());
      op.getValue().replaceAllUsesWith(replacement);
      op.erase();
    }
  }
};
} // namespace
std::unique_ptr<Pass> acir::createFieldProjectionPass() {
  return std::make_unique<Projections>();
}
