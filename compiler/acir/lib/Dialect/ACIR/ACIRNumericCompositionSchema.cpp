#include "ACIRNumericCompositionDetail.h"

#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSwitch.h"

#include <numeric>

using namespace mlir;

namespace acir::ac::composition_detail {
namespace {

bool nodeRef(Attribute raw, unsigned &index) {
  auto ref = dyn_cast<DictionaryAttr>(raw);
  auto value = ref ? ref.getAs<IntegerAttr>("index") : IntegerAttr();
  if (!ref || ref.size() != 2 || !value || !value.getType().isInteger(32) ||
      ref.getAs<StringAttr>("kind") !=
          StringAttr::get(ref.getContext(), "node") ||
      value.getValue().getActiveBits() > 32)
    return false;
  index = value.getValue().getZExtValue();
  return true;
}

unsigned root(SmallVectorImpl<unsigned> &parents, unsigned value) {
  while (parents[value] != value) {
    parents[value] = parents[parents[value]];
    value = parents[value];
  }
  return value;
}

void join(SmallVectorImpl<unsigned> &parents, unsigned left, unsigned right) {
  left = root(parents, left);
  right = root(parents, right);
  if (left != right)
    parents[right] = left;
}

} // namespace

bool valueIDMatchesOriginSlot(DictionaryAttr id, DictionaryAttr origin,
                              uint32_t expectedSlot) {
  auto slot = id ? id.getAs<IntegerAttr>("slot") : IntegerAttr();
  return id && id.size() == 2 && origin &&
         id.getAs<DictionaryAttr>("origin") == origin && slot &&
         slot.getType().isInteger(32) &&
         slot.getValue() == APInt(32, expectedSlot);
}

bool rhsIdentityMatchesUse(DictionaryAttr useID, DictionaryAttr valueID) {
  auto useOrigin =
      useID ? useID.getAs<DictionaryAttr>("origin") : DictionaryAttr();
  auto valueOrigin =
      valueID ? valueID.getAs<DictionaryAttr>("origin") : DictionaryAttr();
  auto useSite =
      useOrigin ? useOrigin.getAs<DictionaryAttr>("site") : DictionaryAttr();
  auto valueSite = valueOrigin ? valueOrigin.getAs<DictionaryAttr>("site")
                               : DictionaryAttr();
  auto usePath = useSite ? useSite.getAs<ArrayAttr>("ast_path") : ArrayAttr();
  auto valuePath =
      valueSite ? valueSite.getAs<ArrayAttr>("ast_path") : ArrayAttr();
  if (!useOrigin || !valueOrigin || !useSite || !valueSite || !usePath ||
      !valuePath || valuePath.size() != usePath.size() + 1 ||
      useOrigin.getAs<ArrayAttr>("expansion") !=
          valueOrigin.getAs<ArrayAttr>("expansion") ||
      useSite.getAs<FlatSymbolRefAttr>("definition") !=
          valueSite.getAs<FlatSymbolRefAttr>("definition"))
    return false;
  for (auto [index, component] : llvm::enumerate(usePath))
    if (valuePath[index] != component)
      return false;
  auto leaf = dyn_cast<DictionaryAttr>(valuePath[valuePath.size() - 1]);
  return leaf && leaf.size() == 2 &&
         leaf.getAs<StringAttr>("kind") ==
             StringAttr::get(valueID.getContext(), "field") &&
         leaf.getAs<StringAttr>("name") ==
             StringAttr::get(valueID.getContext(), "value");
}

bool directUseAuthority(Value value, DictionaryAttr logical,
                        DictionaryAttr useID, DictionaryAttr valueID,
                        RuleOp rule) {
  auto literal = value.getDefiningOp<arith::ConstantOp>();
  auto literalValue =
      literal ? dyn_cast<IntegerAttr>(literal.getValue()) : IntegerAttr();
  auto kind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
  auto storage = logical ? logical.getAs<TypeAttr>("storage") : TypeAttr();
  if (literal && literal->getAttrs().size() == 1 && literal->hasAttr("value") &&
      literalValue && literalValue.getType().isInteger(1) && kind &&
      kind.getValue() == "bool" && storage && storage.getValue().isInteger(1) &&
      rhsIdentityMatchesUse(useID, valueID) &&
      valueIDMatchesOriginSlot(valueID, valueID.getAs<DictionaryAttr>("origin"),
                               0))
    return true;
  auto read = value.getDefiningOp<SourceReadOp>();
  auto current =
      read ? dyn_cast<BlockArgument>(read.getCurrent()) : BlockArgument();
  auto inputs = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  return read && current && current.getOwner() == &rule.getBody().front() &&
         inputs && current.getArgNumber() < inputs.size() &&
         dyn_cast<DictionaryAttr>(inputs[current.getArgNumber()]) == logical &&
         valueIDMatchesOriginSlot(
             valueID, read->getAttrOfType<DictionaryAttr>("ac.origin"), 0);
}

LogicalResult verifyNodeSchema(RuleOp rule, ArrayAttr required) {
  auto emit = [&] { return rule.emitOpError(); };
  SmallVector<unsigned> parents(required.size());
  std::iota(parents.begin(), parents.end(), 0);
  DenseSet<Attribute> ids;
  for (auto [index, raw] : llvm::enumerate(required)) {
    auto node = dyn_cast<DictionaryAttr>(raw);
    auto id = node ? node.getAs<DictionaryAttr>("id") : DictionaryAttr();
    auto opcode = node ? node.getAs<StringAttr>("operator") : StringAttr();
    auto operands = node ? node.getAs<ArrayAttr>("operands") : ArrayAttr();
    auto target =
        node ? node.getAs<DictionaryAttr>("target") : DictionaryAttr();
    bool supported =
        opcode && llvm::StringSwitch<bool>(opcode.getValue())
                      .Cases({"from_bits", "constant", "add", "and_bits", "eq",
                              "ne", "lt", "to_bits"},
                             true)
                      .Default(false);
    if (opcode && !supported)
      return emit() << "numeric composition supports only add, and_bits, eq, "
                       "ne, lt and to_bits";
    unsigned arity = opcode && (opcode.getValue() == "from_bits" ||
                                opcode.getValue() == "constant" ||
                                opcode.getValue() == "to_bits")
                         ? 1
                         : 2;
    auto targetKind = target ? target.getAs<StringAttr>("kind") : StringAttr();
    bool targetOK = false;
    if (opcode && opcode.getValue() == "to_bits") {
      auto domain =
          target ? target.getAs<DictionaryAttr>("domain") : DictionaryAttr();
      auto kind = domain ? domain.getAs<StringAttr>("kind") : StringAttr();
      auto interpretation =
          domain ? domain.getAs<StringAttr>("interpretation") : StringAttr();
      auto lower = domain ? domain.getAs<MathIntAttr>("lower") : MathIntAttr();
      targetOK = target && target.size() == 2 && targetKind &&
                 targetKind.getValue() == "integer_boundary" && domain &&
                 succeeded(detail::verifyLogicalTypeStructure(domain, emit)) &&
                 kind && kind.getValue() == "integer" && interpretation &&
                 interpretation.getValue() == "unsigned" && lower &&
                 !lower.getCanonicalValue().starts_with("-");
    } else {
      targetOK = target && target.size() == 1 && targetKind &&
                 targetKind.getValue() == "none";
    }
    uint32_t expectedSlot = opcode && (opcode.getValue() == "from_bits" ||
                                       opcode.getValue() == "to_bits")
                                ? 1
                                : 0;
    if (!node || node.size() != 4 || !supported || !operands ||
        operands.size() != arity || !targetOK ||
        failed(detail::verifyValueID(id, emit)) ||
        !valueIDMatchesOriginSlot(
            id, id ? id.getAs<DictionaryAttr>("origin") : DictionaryAttr(),
            expectedSlot) ||
        !ids.insert(id).second)
      return emit() << "composition NumericNode schema is invalid";

    if (opcode.getValue() == "from_bits") {
      auto input = dyn_cast<DictionaryAttr>(operands[0]);
      auto ordinal = input ? input.getAs<IntegerAttr>("index") : IntegerAttr();
      if (!input || input.size() != 2 || !ordinal ||
          !ordinal.getType().isInteger(32) || !ordinal.getValue().isZero() ||
          input.getAs<StringAttr>("kind") !=
              StringAttr::get(rule.getContext(), "input"))
        return emit() << "composition from_bits input recipe is invalid";
      continue;
    }
    if (opcode.getValue() == "constant") {
      auto value = dyn_cast<DictionaryAttr>(operands[0]);
      auto integer = value ? value.getAs<MathIntAttr>("value") : MathIntAttr();
      if (!value || value.size() != 2 || !integer ||
          value.getAs<StringAttr>("kind") !=
              StringAttr::get(rule.getContext(), "constant") ||
          integer.getCanonicalValue().starts_with("-"))
        return emit() << "composition constant recipe is invalid";
      continue;
    }
    for (Attribute rawRef : operands) {
      unsigned reference = 0;
      if (!nodeRef(rawRef, reference) || reference >= index)
        return emit() << "composition node reference is invalid";
      join(parents, index, reference);
    }
    if (opcode.getValue() != "to_bits") {
      unsigned rhs = 0;
      auto rhsNode = nodeRef(operands[1], rhs)
                         ? dyn_cast<DictionaryAttr>(required[rhs])
                         : DictionaryAttr();
      auto rhsOpcode =
          rhsNode ? rhsNode.getAs<StringAttr>("operator") : StringAttr();
      if (!rhsOpcode || rhsOpcode.getValue() != "constant")
        return emit() << "numeric composition requires a constant RHS";
    }
  }

  DenseMap<unsigned, unsigned> inputs;
  for (auto [index, raw] : llvm::enumerate(required)) {
    auto node = cast<DictionaryAttr>(raw);
    if (node.getAs<StringAttr>("operator").getValue() == "from_bits")
      ++inputs[root(parents, index)];
  }
  for (unsigned index = 0; index < required.size(); ++index)
    if (inputs[root(parents, index)] > 1)
      return emit()
             << "numeric composition allows at most one input per component";
  return success();
}

} // namespace acir::ac::composition_detail
