#ifndef PYCIRCUIT_SOURCE_FORMAL_CANDIDATE_SELECTION_H
#define PYCIRCUIT_SOURCE_FORMAL_CANDIDATE_SELECTION_H

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/MathExtras.h"
#include <functional>

namespace acir::ac::detail {

// Authenticate candidate reconstruction against independently tracked formal
// history. This checks original SSA, with no evaluation or candidate rewrite.
// previousMatches comes from the current frame before this marker is applied;
// the candidate graph or its selector metadata must not supply that history.
inline mlir::LogicalResult verifySourceFormalCandidateSelection(
    SourceFormalWriteOp marker, const HardwareAnalysis &hardware,
    const HardwareBindings &bindings,
    llvm::function_ref<mlir::FailureOr<bool>(mlir::Value)> previousMatches,
    llvm::function_ref<mlir::LogicalResult(uint64_t)> pay) {
  using namespace mlir;
  auto identity = [&](Value value) -> FailureOr<Value> {
    while (auto extract = value.getDefiningOp<BitsExtractOp>()) {
      if (failed(pay(8)))
        return failure();
      auto input = dyn_cast<BitsType>(extract.getInput().getType());
      auto output = dyn_cast<BitsType>(value.getType());
      if (!input || !output ||
          !hardware.isStaticEvaluable(input.getWidth(), bindings) ||
          !hardware.isStaticEvaluable(output.getWidth(), bindings) ||
          !hardware.isStaticEvaluable(extract.getLow(), bindings))
        return value;
      auto before = hardware.getPackedWidth(input, bindings, marker);
      auto after = hardware.getPackedWidth(output, bindings, marker);
      auto low = hardware.evaluateStatic(extract.getLow(), bindings, marker);
      if (failed(before) || failed(after) || failed(low))
        return failure();
      auto offset = dyn_cast<MathIntAttr>(*low);
      if (*before != *after || !offset || offset.getCanonicalValue() != "0")
        return value;
      value = extract.getInput();
    }
    return value;
  };
  auto same = [&](Value left, Value right) -> LogicalResult {
    if (failed(pay(2)))
      return failure();
    auto a = identity(left), b = identity(right);
    return success(succeeded(a) && succeeded(b) && *a == *b);
  };
  SmallVector<StringAttr, 0> fields;
  auto projected = [&](Value value, Value root) {
    for (StringAttr field : llvm::reverse(fields)) {
      auto get = value.getDefiningOp<StructGetOp>();
      if (!get || get.getFieldAttr() != field)
        return false;
      value = get.getValue();
    }
    return succeeded(same(value, root));
  };
  std::function<LogicalResult(Value, Value, Value, Value, unsigned)> check;
  check = [&](Value result, Value condition, Value yes, Value no,
              unsigned depth) -> LogicalResult {
    if (depth > 128 || failed(pay(8 + fields.size())))
      return failure();
    if (yes == no && projected(result, yes))
      return success();
    Type type = result.getType();
    auto scalar = [&](Value selected) -> LogicalResult {
      auto select = selected.getDefiningOp<BitsSelectOp>();
      return success(select && select.getCondition() == condition &&
                     projected(select.getTrueValue(), yes) &&
                     projected(select.getFalseValue(), no));
    };
    if (isa<BitsType>(type))
      return scalar(result);
    if (isa<EnumType>(type)) {
      auto converted = result.getDefiningOp<EnumFromBitsOp>();
      auto select = converted
                        ? converted.getInput().getDefiningOp<BitsSelectOp>()
                        : BitsSelectOp();
      auto trueBits = select
                          ? select.getTrueValue().getDefiningOp<EnumToBitsOp>()
                          : EnumToBitsOp();
      auto falseBits =
          select ? select.getFalseValue().getDefiningOp<EnumToBitsOp>()
                 : EnumToBitsOp();
      return success(converted && converted.getValue() == result && select &&
                     select.getCondition() == condition && trueBits &&
                     falseBits && projected(trueBits.getInput(), yes) &&
                     projected(falseBits.getInput(), no));
    }
    if (auto structure = dyn_cast<StructType>(type)) {
      auto create = result.getDefiningOp<StructCreateOp>();
      auto declaration = hardware.lookupStruct(structure);
      if (!create || !declaration ||
          create.getValues().size() != declaration.getFields().size())
        return failure();
      for (auto [value, raw] :
           llvm::zip(create.getValues(), declaration.getFields())) {
        if (failed(pay(4)))
          return failure();
        fields.push_back(cast<DictionaryAttr>(raw).getAs<StringAttr>("name"));
        auto valid = check(value, condition, yes, no, depth + 1);
        fields.pop_back();
        if (failed(valid))
          return failure();
      }
      return success();
    }
    if (isa<TableType>(type)) {
      auto map = result.getDefiningOp<TableMapOp>();
      if (!map || map.getTables().size() != 3 || !map.getCaptures().empty() ||
          map.getNumResults() != 1 || !map.getBody().hasOneBlock())
        return failure();
      auto guards = map.getTables()[0].getDefiningOp<TableSplatOp>();
      auto &body = map.getBody().front();
      if (!guards || guards.getInput() != condition ||
          body.getNumArguments() != 4 || !projected(map.getTables()[1], yes) ||
          !projected(map.getTables()[2], no))
        return failure();
      auto yield = dyn_cast<YieldOp>(body.getTerminator());
      if (!yield || yield.getValues().size() != 1)
        return failure();
      auto outer = std::move(fields);
      fields.clear();
      auto valid = check(yield.getValues()[0], body.getArgument(1),
                         body.getArgument(2), body.getArgument(3), depth + 1);
      fields = std::move(outer);
      return valid;
    }
    return failure();
  };
  if (marker.getPrevious()) {
    if (!marker.getSelectors().empty() || !marker.getIndices().empty())
      return marker.emitOpError()
             << "forwarded formal proposal must inherit callee write domains";
    if (failed(check(marker.getCandidate(), marker.getPath(),
                     marker.getPayload(), marker.getPrevious(), 0)))
      return marker.emitOpError()
             << "forwarded formal candidate must select its actual proposal "
                "data and previous snapshot";
    return success();
  }
  if (marker.getSelectors().empty()) {
    if (!marker.getIndices().empty() ||
        failed(same(marker.getCandidate(), marker.getPayload())))
      return marker.emitOpError()
             << "whole formal assignment candidate must be its actual RHS";
    return success();
  }
  auto previousProjection = [&](Value value,
                                ArrayRef<StringAttr> path) -> LogicalResult {
    for (StringAttr field : llvm::reverse(path)) {
      if (failed(pay(4)))
        return failure();
      auto normalized = identity(value);
      if (failed(normalized))
        return failure();
      auto get = normalized->getDefiningOp<StructGetOp>();
      if (!get || get.getFieldAttr() != field)
        return failure();
      value = get.getValue();
    }
    auto root = identity(value);
    if (failed(root))
      return failure();
    auto matched = previousMatches(*root);
    return success(succeeded(matched) && *matched);
  };
  auto width = [&](Value value) {
    return hardware.getPackedWidth(value.getType(), bindings, marker);
  };
  auto widened = [&](Value value, Value source,
                     uint64_t required) -> LogicalResult {
    if (failed(pay(6)))
      return failure();
    auto normalized = identity(value);
    auto inputWidth = width(source), resultWidth = width(value);
    if (failed(normalized) || failed(inputWidth) || failed(resultWidth) ||
        *resultWidth != required)
      return failure();
    if (*inputWidth == required)
      return same(*normalized, source);
    auto resize = normalized->getDefiningOp<BitsResizeOp>();
    if (*inputWidth > required || !resize || resize.getMode() != "zext")
      return failure();
    return same(resize.getInput(), source);
  };
  std::function<FailureOr<Value>(Value, unsigned)> selectionCondition;
  selectionCondition = [&](Value result, unsigned depth) -> FailureOr<Value> {
    if (depth > 128 || failed(pay(4)))
      return failure();
    if (auto select = result.getDefiningOp<BitsSelectOp>())
      return select.getCondition();
    if (auto converted = result.getDefiningOp<EnumFromBitsOp>())
      return selectionCondition(converted.getInput(), depth + 1);
    if (auto create = result.getDefiningOp<StructCreateOp>()) {
      if (create.getValues().empty())
        return failure();
      return selectionCondition(create.getValues().front(), depth + 1);
    }
    if (auto map = result.getDefiningOp<TableMapOp>()) {
      if (map.getTables().size() != 3)
        return failure();
      auto guards = map.getTables()[0].getDefiningOp<TableSplatOp>();
      if (guards)
        return guards.getInput();
    }
    return failure();
  };
  SmallVector<StringAttr, 0> previousPath;
  std::function<LogicalResult(Value, unsigned, unsigned)> update;
  update = [&](Value candidate, unsigned next,
               unsigned depth) -> LogicalResult {
    if (depth > 128 || failed(pay(8 + previousPath.size())))
      return failure();
    if (next == marker.getSelectors().size())
      return same(candidate, marker.getPayload());
    auto selector = dyn_cast<DictionaryAttr>(marker.getSelectors()[next]);
    if (!selector || selector.size() != 1)
      return failure();
    if (auto field = selector.getAs<StringAttr>("field")) {
      auto create = candidate.getDefiningOp<StructCreateOp>();
      auto type = dyn_cast<StructType>(candidate.getType());
      auto declaration = type ? hardware.lookupStruct(type) : StructOp();
      if (!create || !declaration ||
          create.getValues().size() != declaration.getFields().size())
        return failure();
      bool selected = false;
      for (auto [value, raw] :
           llvm::zip(create.getValues(), declaration.getFields())) {
        if (failed(pay(4)))
          return failure();
        auto name = cast<DictionaryAttr>(raw).getAs<StringAttr>("name");
        previousPath.push_back(name);
        auto valid = name == field ? update(value, next + 1, depth + 1)
                                   : previousProjection(value, previousPath);
        previousPath.pop_back();
        if (failed(valid))
          return failure();
        selected |= name == field;
      }
      return success(selected);
    }
    auto ordinal = selector.getAs<IntegerAttr>("index");
    auto map = candidate.getDefiningOp<TableMapOp>();
    auto table = dyn_cast<TableType>(candidate.getType());
    if (!ordinal || !ordinal.getType().isSignlessInteger(64) ||
        ordinal.getInt() < 0 ||
        static_cast<uint64_t>(ordinal.getInt()) >= marker.getIndices().size() ||
        !map || !table || table.getShape().size() != 1 ||
        map.getNumResults() != 1 || candidate != map.getResult(0) ||
        map.getShape() != table.getShape() || map.getTables().size() != 3 ||
        !map.getCaptures().empty() || !map.getBody().hasOneBlock() ||
        failed(previousProjection(map.getTables()[0], previousPath)))
      return failure();
    auto index = map.getTables()[1].getDefiningOp<TableSplatOp>();
    auto payload = map.getTables()[2].getDefiningOp<TableSplatOp>();
    if (!index || !payload || index.getShape() != map.getShape() ||
        payload.getShape() != map.getShape() ||
        failed(same(index.getInput(), marker.getIndices()[ordinal.getInt()])) ||
        failed(same(payload.getInput(), marker.getPayload())))
      return failure();
    auto &body = map.getBody().front();
    auto yield = dyn_cast<YieldOp>(body.getTerminator());
    if (body.getNumArguments() != 4 || !yield ||
        yield.getValues().size() != 1 ||
        !areEquivalentHardwareTypes(body.getArgument(1).getType(),
                                    table.getElementType()) ||
        !areEquivalentHardwareTypes(body.getArgument(2).getType(),
                                    index.getInput().getType()) ||
        !areEquivalentHardwareTypes(body.getArgument(3).getType(),
                                    payload.getInput().getType()))
      return failure();
    Value result = yield.getValues()[0];
    auto merge = result.getDefiningOp<ValueMergeOp>();
    Value condition;
    if (next + 1 != marker.getSelectors().size()) {
      SmallVector<Attribute> suffix;
      for (Attribute raw :
           marker.getSelectors().getValue().drop_front(next + 1)) {
        if (failed(pay(4)))
          return failure();
        auto item = dyn_cast<DictionaryAttr>(raw);
        auto name = item && item.size() == 1 ? item.getAs<StringAttr>("field")
                                             : StringAttr();
        if (!name)
          return failure();
        suffix.push_back(name);
      }
      if (!merge || merge.getNext() != result ||
          merge.getBase() != body.getArgument(1) ||
          merge.getGuards().size() != 1 || merge.getValues().size() != 1 ||
          merge.getValues()[0] != body.getArgument(3) ||
          merge.getPaths() !=
              ArrayAttr::get(marker.getContext(),
                             {ArrayAttr::get(marker.getContext(), suffix)}))
        return failure();
      condition = merge.getGuards()[0];
    } else {
      // The complete selected graph below authenticates the extracted control.
      auto selected = selectionCondition(result, depth + 1);
      if (failed(selected))
        return failure();
      condition = *selected;
      if (failed(check(result, condition, body.getArgument(3),
                       body.getArgument(1), depth + 1)))
        return failure();
    }
    auto compare = condition.getDefiningOp<BitsCompareOp>();
    auto size = hardware.getTableSize(table, bindings, marker);
    auto ordinalWidth = width(body.getArgument(0)),
         indexWidth = width(body.getArgument(2));
    auto predicateWidth = width(condition);
    if (!compare || compare.getPredicate() != "eq" || failed(size) ||
        failed(ordinalWidth) || failed(indexWidth) || failed(predicateWidth) ||
        *predicateWidth != 1 ||
        *ordinalWidth != std::max(1u, llvm::Log2_64_Ceil(*size)))
      return failure();
    uint64_t comparisonWidth = std::max(*ordinalWidth, *indexWidth);
    return success(succeeded(widened(compare.getLhs(), body.getArgument(0),
                                     comparisonWidth)) &&
                   succeeded(widened(compare.getRhs(), body.getArgument(2),
                                     comparisonWidth)));
  };
  if (failed(update(marker.getCandidate(), 0, 0)))
    return marker.emitOpError()
           << "selected formal candidate must reconstruct its exact current "
              "formal, RHS and assignment-time indices";
  return success();
}
} // namespace acir::ac::detail
#endif
