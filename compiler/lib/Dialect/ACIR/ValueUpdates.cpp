#include "pycircuit/Dialect/ACIR/ValueUpdates.h"
#include "TransactionalRepresentation.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
using namespace mlir;
namespace acir::ac {
namespace {
StaticExprAttr number(Builder &b, uint64_t value) {
  return detail::transactionalInteger(
      b, value, detail::transactionalOccurrence(b, "__value_update__"),
      detail::transactionalSpan(b));
}
Value make(OpBuilder &b, Location loc, StringRef name, ValueRange operands,
           Type type, ArrayRef<NamedAttribute> attrs = {}) {
  OperationState state(loc, name);
  state.addOperands(operands);
  state.addTypes(type);
  state.addAttributes(attrs);
  return b.create(state)->getResult(0);
}
FailureOr<uint64_t> bound(StaticExprAttr expr, HardwareAnalysis &hw,
                          Operation *site) {
  auto value = hw.evaluateStatic(expr, {}, site);
  auto integer =
      succeeded(value) ? dyn_cast<MathIntAttr>(*value) : MathIntAttr();
  uint64_t result;
  if (!integer || integer.getCanonicalValue().getAsInteger(10, result))
    return site->emitOpError(
        "update bound requires a nonnegative static integer");
  return result;
}
LogicalResult verifyTarget(Type base, Type replacement, ArrayAttr fields,
                           StaticExprAttr low, StaticExprAttr high,
                           Operation *site) {
  HardwareAnalysis hw(site->getParentOfType<mlir::ModuleOp>());
  Type selected = base;
  if (!fields)
    return site->emitOpError("update requires a field path");
  for (Attribute raw : fields) {
    auto name = dyn_cast<StringAttr>(raw);
    auto structure = dyn_cast<StructType>(selected);
    auto decl = structure ? hw.lookupStruct(structure) : StructOp();
    Type next;
    if (name && decl)
      for (auto field : decl.getFields()) {
        auto data = cast<DictionaryAttr>(field);
        if (data.getAs<StringAttr>("name") == name)
          next = data.getAs<TypeAttr>("type").getValue();
      }
    if (!next)
      return site->emitOpError("update requires a declared Struct field path");
    selected = next;
  }
  if (bool(low) != bool(high))
    return site->emitOpError("update requires both slice bounds");
  if (low) {
    if (!isa<BitsType>(selected) || !isa<BitsType>(replacement))
      return site->emitOpError("slice updates require Bits payloads");
    if (failed(StaticExprAttr::verify([&] { return site->emitOpError(); },
                                      low.getTree())) ||
        failed(StaticExprAttr::verify([&] { return site->emitOpError(); },
                                      high.getTree())))
      return failure();
    if (!hw.isStaticEvaluable(low, {}) || !hw.isStaticEvaluable(high, {}) ||
        !hw.isStaticEvaluable(cast<BitsType>(selected).getWidth(), {}))
      return success();
    auto target = resolveValueUpdateTarget(base, fields, low, high, site);
    if (failed(target))
      return failure();
    auto replacementWidth = hw.getPackedWidth(replacement, {}, site);
    if (failed(replacementWidth))
      return failure();
    if (*replacementWidth != target->sliceHigh - target->sliceLow)
      return site->emitOpError(
          "update replacement type differs from selected interval");
  } else if (!areEquivalentHardwareTypes(selected, replacement))
    return site->emitOpError(
        "update replacement type differs from selected field");
  return success();
}
} // namespace
FailureOr<ValueUpdateTarget>
resolveValueUpdateTarget(Type root, ArrayAttr fields, StaticExprAttr low,
                         StaticExprAttr high, Operation *site) {
  HardwareAnalysis hw(site->getParentOfType<mlir::ModuleOp>());
  ValueUpdateTarget target;
  target.fields = fields;
  Type selected = root;
  uint64_t offset = 0;
  for (Attribute raw : fields) {
    auto structure = dyn_cast<StructType>(selected);
    auto decl = structure ? hw.lookupStruct(structure) : StructOp();
    auto name = dyn_cast<StringAttr>(raw);
    if (!decl || !name)
      return site->emitOpError("invalid update field path");
    uint64_t suffix = 0;
    Type next;
    for (auto field : llvm::reverse(decl.getFields())) {
      auto data = cast<DictionaryAttr>(field);
      Type type = data.getAs<TypeAttr>("type").getValue();
      if (data.getAs<StringAttr>("name") == name) {
        next = type;
        break;
      }
      auto width = hw.getPackedWidth(type, {}, site);
      if (failed(width))
        return failure();
      suffix += *width;
    }
    if (!next)
      return site->emitOpError("unknown update field");
    offset += suffix;
    selected = next;
  }
  auto width = hw.getPackedWidth(selected, {}, site);
  if (failed(width))
    return failure();
  target.leafType = target.targetType = selected;
  target.sliceHigh = *width;
  if (low || high) {
    if (!low || !high || !isa<BitsType>(selected))
      return site->emitOpError(
          "static slice update requires Bits and two bounds");
    auto lo = bound(low, hw, site), hi = bound(high, hw, site);
    if (failed(lo) || failed(hi))
      return failure();
    if (*lo >= *hi || *hi > *width)
      return site->emitOpError("update interval exceeds Bits payload");
    target.sliced = true;
    target.sliceLow = *lo;
    target.sliceHigh = *hi;
    Builder b(site->getContext());
    target.targetType = BitsType::get(b.getContext(), number(b, *hi - *lo));
  }
  target.low = offset + target.sliceLow;
  target.high = offset + target.sliceHigh;
  return target;
}
FailureOr<Value> replaceValue(OpBuilder &b, Location loc, Value base,
                              Value replacement,
                              const ValueUpdateTarget &target) {
  HardwareAnalysis hw(
      b.getInsertionBlock()->getParentOp()->getParentOfType<mlir::ModuleOp>());
  std::function<FailureOr<Value>(Value, unsigned)> rebuild =
      [&](Value old, unsigned depth) -> FailureOr<Value> {
    if (depth < target.fields.size()) {
      auto decl = hw.lookupStruct(cast<StructType>(old.getType()));
      SmallVector<Value> values;
      for (auto raw : decl.getFields()) {
        auto field = cast<DictionaryAttr>(raw);
        auto name = field.getAs<StringAttr>("name");
        Type type = field.getAs<TypeAttr>("type").getValue();
        if (name == target.fields[depth] && depth + 1 == target.fields.size() &&
            !target.sliced) {
          values.push_back(replacement);
          continue;
        }
        Value child = make(b, loc, "ac.struct.get", old, type,
                           {b.getNamedAttr("field", name)});
        if (name == target.fields[depth]) {
          auto updated = rebuild(child, depth + 1);
          if (failed(updated))
            return failure();
          child = *updated;
        }
        values.push_back(child);
      }
      return make(b, loc, "ac.struct.create", values, old.getType());
    }
    if (!target.sliced)
      return replacement;
    auto width = hw.getPackedWidth(old.getType(), {},
                                   b.getInsertionBlock()->getParentOp());
    if (failed(width))
      return failure();
    if (target.sliceLow == 0 && target.sliceHigh == *width) {
      if (replacement.getType() == old.getType())
        return replacement;
      // Equal widths can retain different static-expression/provenance types.
      // Carry the base's exact type at the immutable replacement boundary.
      return make(b, loc, "ac.bits.extract", replacement, old.getType(),
                  {b.getNamedAttr("low", number(b, 0))});
    }
    SmallVector<Value> parts;
    auto extract = [&](uint64_t low, uint64_t high) {
      return make(b, loc, "ac.bits.extract", old,
                  BitsType::get(b.getContext(), number(b, high - low)),
                  {b.getNamedAttr("low", number(b, low))});
    };
    if (target.sliceHigh < *width)
      parts.push_back(extract(target.sliceHigh, *width));
    parts.push_back(replacement);
    if (target.sliceLow)
      parts.push_back(extract(0, target.sliceLow));
    return make(b, loc, "ac.bits.concat", parts, old.getType());
  };
  return rebuild(base, 0);
}
LogicalResult StructUpdateOp::verify() {
  if (getBase().getType() != getResult().getType() || getFields().empty())
    return emitOpError(
        "Struct update preserves its nominal type and selects a field");
  return verifyTarget(getBase().getType(), getReplacement().getType(),
                      getFields(), {}, {}, *this);
}
LogicalResult BitsUpdateOp::verify() {
  if (!areEquivalentHardwareTypes(getBase().getType(), getResult().getType()))
    return emitOpError("Bits update preserves its base type");
  return verifyTarget(getBase().getType(), getReplacement().getType(),
                      ArrayAttr::get(getContext(), {}), getLow(), getHigh(),
                      *this);
}
LogicalResult normalizeValueUpdates(mlir::ModuleOp package) {
  SmallVector<Operation *> updates;
  package.walk([&](Operation *op) {
    if (isa<StructUpdateOp, BitsUpdateOp>(op))
      updates.push_back(op);
  });
  for (Operation *op : updates) {
    bool state = false;
    Type base = op->getOperand(0).getType();
    if (state)
      base = cast<StateType>(base).getPayload();
    auto fields = op->getAttrOfType<ArrayAttr>("fields");
    if (!fields)
      fields = ArrayAttr::get(package.getContext(), {});
    auto target = resolveValueUpdateTarget(
        base, fields, op->getAttrOfType<StaticExprAttr>("low"),
        op->getAttrOfType<StaticExprAttr>("high"), op);
    if (failed(target))
      return failure();
    if (state)
      continue;
    OpBuilder builder(op);
    Operation *previous = op->getPrevNode();
    auto result = replaceValue(builder, op->getLoc(), op->getOperand(0),
                               op->getOperand(1), *target);
    if (failed(result))
      return failure();
    // Preserve expression provenance on generated operations without changing
    // nominal definitions or the original Linked snapshot.
    for (Operation *created = previous ? previous->getNextNode()
                                       : &op->getBlock()->front();
         created != op; created = created->getNextNode())
      if (auto origin = op->getAttr("ac.origin"))
        created->setAttr("ac.origin", origin);
    op->replaceAllUsesWith(ValueRange{*result});
    op->erase();
  }
  return success();
}
} // namespace acir::ac
