#include "ScalarNumericLoweringDetail.h"

#include "Dialect/ACIR/ACIRNumericComposition.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallString.h"

#include <algorithm>

using namespace mlir;

namespace acir::compiler {
namespace {

struct Domain {
  DictionaryAttr attr;
  APSInt lower;
  APSInt upper;
  IntegerType storage;
  bool isUnsigned;
};

struct FiniteNode {
  DictionaryAttr id;
  Operation *source = nullptr;
  Value value;
  Value valid;
  Value path;
  Domain domain;
  DictionaryAttr inputID;
};

APSInt mathValue(ac::MathIntAttr attr) {
  StringRef text = attr.getCanonicalValue();
  bool negative = text.consume_front("-");
  unsigned width = std::max(2u, unsigned(text.size() * 4 + 2));
  APInt bits(width, text, 10);
  if (negative)
    bits = -bits;
  return APSInt(std::move(bits), !negative);
}

APInt extend(const APSInt &value, unsigned width) {
  return value.isUnsigned() ? value.zextOrTrunc(width)
                            : value.sextOrTrunc(width);
}

int compare(const APSInt &left, const APSInt &right) {
  unsigned width = std::max(left.getBitWidth(), right.getBitWidth()) + 1;
  APInt lhs = extend(left, width), rhs = extend(right, width);
  if (lhs == rhs)
    return 0;
  return lhs.slt(rhs) ? -1 : 1;
}

APSInt add(const APSInt &left, const APSInt &right) {
  unsigned width = std::max(left.getBitWidth(), right.getBitWidth()) + 1;
  APInt value = extend(left, width) + extend(right, width);
  return APSInt(std::move(value), !value.isNegative());
}

FailureOr<Domain> domain(DictionaryAttr attr, Operation *owner) {
  auto storage = attr ? attr.getAs<TypeAttr>("storage") : TypeAttr();
  auto type =
      storage ? dyn_cast<IntegerType>(storage.getValue()) : IntegerType();
  auto lower = attr ? attr.getAs<ac::MathIntAttr>("lower") : ac::MathIntAttr();
  auto upper = attr ? attr.getAs<ac::MathIntAttr>("upper") : ac::MathIntAttr();
  auto interpretation =
      attr ? attr.getAs<StringAttr>("interpretation") : StringAttr();
  if (!type || !lower || !upper || !interpretation)
    return owner->emitOpError() << "composition integer domain is incomplete";
  return Domain{attr, mathValue(lower), mathValue(upper), type,
                interpretation.getValue() == "unsigned"};
}

FailureOr<Domain> makeDomain(Builder &builder, APSInt lower, APSInt upper,
                             Operation *owner) {
  if (compare(lower, upper) >= 0)
    return owner->emitOpError() << "composition interval is empty";
  bool isUnsigned = !lower.isNegative();
  APSInt maximum = upper;
  --maximum;
  unsigned width = 0;
  if (isUnsigned)
    width = std::max(1u, maximum.getActiveBits());
  else
    for (unsigned candidate = 1; candidate <= 64; ++candidate)
      if (lower.isSignedIntN(candidate) && maximum.isSignedIntN(candidate)) {
        width = candidate;
        break;
      }
  if (!width || width > 64)
    return owner->emitOpError() << "composition interval exceeds i64";
  auto attr = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("storage",
                           TypeAttr::get(builder.getIntegerType(width))),
      builder.getNamedAttr("lower",
                           ac::MathIntAttr::get(builder.getContext(), lower)),
      builder.getNamedAttr("upper",
                           ac::MathIntAttr::get(builder.getContext(), upper)),
      builder.getNamedAttr(
          "interpretation",
          builder.getStringAttr(isUnsigned ? "unsigned" : "signed")),
  });
  return Domain{attr, lower, upper, builder.getIntegerType(width), isUnsigned};
}

Value fit(OpBuilder &builder, Location location, Value value, unsigned width,
          bool isUnsigned) {
  auto source = cast<IntegerType>(value.getType());
  auto target = builder.getIntegerType(width);
  if (source.getWidth() == width)
    return value;
  if (source.getWidth() > width)
    return arith::TruncIOp::create(builder, location, target, value);
  return isUnsigned
             ? Value(arith::ExtUIOp::create(builder, location, target, value))
             : Value(arith::ExtSIOp::create(builder, location, target, value));
}

Value conjunction(OpBuilder &builder, Location location, ValueRange values) {
  Value result = values.front();
  for (Value value : values.drop_front())
    result = arith::AndIOp::create(builder, location, result, value);
  return result;
}

ac::ValueBindingOp bind(OpBuilder &builder, Location location, Value value,
                        Value valid, Value path, DictionaryAttr id,
                        DictionaryAttr logical) {
  OperationState state(location, ac::ValueBindingOp::getOperationName());
  state.addOperands({value, valid, path});
  state.addAttribute("id", id);
  state.addAttribute("domain", logical);
  return cast<ac::ValueBindingOp>(builder.create(state));
}

unsigned nodeRef(Attribute raw, Operation *owner) {
  auto ref = dyn_cast<DictionaryAttr>(raw);
  auto value = ref ? ref.getAs<IntegerAttr>("index") : IntegerAttr();
  auto decoded = ac::detail::decodeU32(value, "composition node ref",
                                       [&] { return owner->emitOpError(); });
  return succeeded(decoded) ? *decoded : ~0u;
}

Value sourceValid(Operation *operation) {
  Value result = operation->getResult(0);
  for (OpOperand &use : result.getUses()) {
    Operation *user = use.getOwner();
    if (auto binary = dyn_cast<ac::MathBinaryOp>(user))
      return use.getOperandNumber() == 1 ? binary.getLhsValid()
                                         : binary.getRhsValid();
    if (auto compare = dyn_cast<ac::MathCompareOp>(user))
      return use.getOperandNumber() == 1 ? compare.getLhsValid()
                                         : compare.getRhsValid();
    if (auto boundary = dyn_cast<ac::MathToBitsOp>(user))
      return boundary.getValueValid();
  }
  return {};
}

arith::CmpIPredicate predicate(StringRef opcode, bool isSigned) {
  if (opcode == "eq")
    return arith::CmpIPredicate::eq;
  if (opcode == "ne")
    return arith::CmpIPredicate::ne;
  return isSigned ? arith::CmpIPredicate::slt : arith::CmpIPredicate::ult;
}

StringRef sourceOpcode(Operation *operation) {
  if (auto binary = dyn_cast<ac::MathBinaryOp>(operation))
    return binary->getAttrOfType<StringAttr>("operator").getValue();
  if (auto compare = dyn_cast<ac::MathCompareOp>(operation))
    return compare.getPredicate();
  if (isa<ac::MathFromBitsOp>(operation))
    return "from_bits";
  if (isa<ac::MathConstantOp>(operation))
    return "constant";
  if (isa<ac::MathToBitsOp>(operation))
    return "to_bits";
  return {};
}

Value bound(OpBuilder &builder, Location location, Value value,
            const Domain &input, const APSInt &endpoint, bool lower) {
  bool alwaysTrue = lower ? compare(input.lower, endpoint) >= 0
                          : compare(input.upper, endpoint) <= 0;
  bool alwaysFalse = lower ? compare(input.upper, endpoint) <= 0
                           : compare(input.lower, endpoint) >= 0;
  if (alwaysTrue || alwaysFalse)
    return arith::ConstantOp::create(builder, location,
                                     builder.getBoolAttr(alwaysTrue));
  auto constant = arith::ConstantOp::create(
      builder, location,
      IntegerAttr::get(input.storage,
                       extend(endpoint, input.storage.getWidth())));
  auto pred = lower ? (input.isUnsigned ? arith::CmpIPredicate::ule
                                        : arith::CmpIPredicate::sle)
                    : (input.isUnsigned ? arith::CmpIPredicate::ult
                                        : arith::CmpIPredicate::slt);
  return lower ? Value(arith::CmpIOp::create(builder, location, pred, constant,
                                             value))
               : Value(arith::CmpIOp::create(builder, location, pred, value,
                                             constant));
}

} // namespace

LogicalResult lowerNumericCompositionRule(ModuleOp unit, ac::RuleOp rule) {
  if (!unit || !rule || failed(ac::verifyNumericCompositionClosure(rule)))
    return failure();
  Block &body = rule.getBody().front();
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  SmallVector<Operation *> candidates, math;
  for (Operation &operation : body)
    if (operation.getName().getStringRef().starts_with("ac.math."))
      candidates.push_back(&operation);
  DenseSet<Operation *> claimed;
  for (Attribute raw : required) {
    auto node = cast<DictionaryAttr>(raw);
    auto id = node.getAs<DictionaryAttr>("id");
    Operation *found = nullptr;
    for (Operation *candidate : candidates)
      if (!claimed.contains(candidate) &&
          sourceOpcode(candidate) ==
              node.getAs<StringAttr>("operator").getValue() &&
          candidate->getAttrOfType<DictionaryAttr>("ac.origin") ==
              id.getAs<DictionaryAttr>("origin")) {
        if (found)
          return rule.emitOpError()
                 << "composition lowering source node is ambiguous";
        found = candidate;
      }
    if (!found)
      return rule.emitOpError()
             << "composition lowering source node is missing";
    claimed.insert(found);
    math.push_back(found);
  }
  if (candidates.size() != required.size())
    return rule.emitOpError() << "composition lowering math inventory changed";
  Builder attrs(unit.getContext());
  SmallVector<FiniteNode> nodes;
  SmallVector<ac::SourceExpectOp> rangeExpects(required.size());
  DenseMap<Attribute, ac::ValueBindingOp> allBindings;
  nodes.reserve(required.size());
  for (auto [index, raw] : llvm::enumerate(required)) {
    auto recipe = cast<DictionaryAttr>(raw);
    auto id = recipe.getAs<DictionaryAttr>("id");
    StringRef opcode = recipe.getAs<StringAttr>("operator").getValue();
    auto refs = recipe.getAs<ArrayAttr>("operands");
    Operation *source = math[index];
    OpBuilder builder(source);
    Location location = source->getLoc();
    FiniteNode node;
    node.id = id;
    node.source = source;
    if (opcode == "from_bits") {
      auto from = cast<ac::MathFromBitsOp>(source);
      auto read = from.getValue().getDefiningOp<ac::SourceReadOp>();
      Value valid = sourceValid(source);
      node.value = read.getResult();
      node.valid = valid;
      node.path = valid;
      auto planned = domain(from.getDomainAttr(), source);
      if (failed(planned))
        return failure();
      node.domain = *planned;
      node.inputID = attrs.getDictionaryAttr({
          attrs.getNamedAttr("origin",
                             read->getAttrOfType<DictionaryAttr>("ac.origin")),
          attrs.getNamedAttr("slot", attrs.getI32IntegerAttr(0)),
      });
    } else if (opcode == "constant") {
      auto constant = cast<ac::MathConstantOp>(source);
      APSInt value = mathValue(constant.getValueAttr());
      APSInt upper = value.extend(value.getBitWidth() + 1);
      ++upper;
      auto planned = makeDomain(attrs, value, upper, source);
      if (failed(planned))
        return failure();
      node.domain = *planned;
      node.value = arith::ConstantOp::create(
          builder, location,
          IntegerAttr::get(node.domain.storage,
                           extend(value, node.domain.storage.getWidth())));
      node.valid = sourceValid(source);
      node.path = node.valid;
    } else {
      unsigned lhsIndex = nodeRef(refs[0], source);
      FiniteNode &lhs = nodes[lhsIndex];
      Value sourcePath = source->getOperand(0);
      if (opcode == "to_bits") {
        auto boundary = cast<ac::MathToBitsOp>(source);
        auto planned = domain(boundary.getDomainAttr(), source);
        if (failed(planned))
          return failure();
        node.domain = *planned;
        Value demand = conjunction(builder, location,
                                   {sourcePath, boundary.getValueValid()});
        Value lower = bound(builder, location, lhs.value, lhs.domain,
                            node.domain.lower, true);
        Value upper = bound(builder, location, lhs.value, lhs.domain,
                            node.domain.upper, false);
        Value safety = conjunction(builder, location, {lower, upper});
        node.valid = conjunction(builder, location, {demand, safety});
        node.path = sourcePath;
        Value converted =
            fit(builder, location, lhs.value, node.domain.storage.getWidth(),
                lhs.domain.isUnsigned);
        Value zero = arith::ConstantOp::create(
            builder, location, IntegerAttr::get(node.domain.storage, 0));
        node.value = arith::SelectOp::create(builder, location, node.valid,
                                             converted, zero);
        for (ac::SourceExpectOp expect : body.getOps<ac::SourceExpectOp>())
          if (expect.getCondition() == boundary.getValid()) {
            expect.getConditionMutable().assign(safety);
            expect.getPathMutable().assign(demand);
            rangeExpects[index] = expect;
          }
      } else {
        unsigned rhsIndex = nodeRef(refs[1], source);
        FiniteNode &rhs = nodes[rhsIndex];
        node.path = sourcePath;
        node.valid = conjunction(
            builder, location,
            {sourcePath, source->getOperand(2), source->getOperand(4)});
        if (opcode == "add") {
          APSInt upper = add(lhs.domain.upper, rhs.domain.upper);
          --upper;
          auto planned = makeDomain(
              attrs, add(lhs.domain.lower, rhs.domain.lower), upper, source);
          if (failed(planned))
            return failure();
          node.domain = *planned;
          Value left =
              fit(builder, location, lhs.value, node.domain.storage.getWidth(),
                  lhs.domain.isUnsigned);
          Value right =
              fit(builder, location, rhs.value, node.domain.storage.getWidth(),
                  rhs.domain.isUnsigned);
          node.value = arith::AddIOp::create(builder, location, left, right);
        } else if (opcode == "and_bits") {
          APSInt mask = rhs.domain.lower;
          APSInt upper = mask.extend(mask.getBitWidth() + 1);
          ++upper;
          auto planned =
              makeDomain(attrs, APSInt(APInt(2, 0), true), upper, source);
          if (failed(planned))
            return failure();
          node.domain = *planned;
          unsigned width = std::max(lhs.domain.storage.getWidth(),
                                    rhs.domain.storage.getWidth());
          Value left =
              fit(builder, location, lhs.value, width, lhs.domain.isUnsigned);
          Value right =
              fit(builder, location, rhs.value, width, rhs.domain.isUnsigned);
          Value result = arith::AndIOp::create(builder, location, left, right);
          node.value = fit(builder, location, result,
                           node.domain.storage.getWidth(), true);
        } else if (opcode == "eq" || opcode == "ne" || opcode == "lt") {
          node.domain = Domain{attrs.getDictionaryAttr({attrs.getNamedAttr(
                                   "kind", attrs.getStringAttr("bool"))}),
                               APSInt(), APSInt(), attrs.getI1Type(), true};
          unsigned width = std::max(lhs.domain.storage.getWidth(),
                                    rhs.domain.storage.getWidth());
          Value left =
              fit(builder, location, lhs.value, width, lhs.domain.isUnsigned);
          Value right =
              fit(builder, location, rhs.value, width, rhs.domain.isUnsigned);
          node.value = arith::CmpIOp::create(
              builder, location,
              predicate(opcode,
                        !lhs.domain.isUnsigned || !rhs.domain.isUnsigned),
              left, right);
        } else {
          return rule.emitOpError()
                 << "composition lowering encountered an unsupported opcode";
        }
      }
    }
    source->getResult(0).replaceAllUsesWith(node.value);
    if (source->getNumResults() == 2)
      source->getResult(1).replaceAllUsesWith(node.valid);
    nodes.push_back(node);
  }

  auto yield = cast<ac::YieldOp>(body.getTerminator());
  OpBuilder evidence(yield);
  for (FiniteNode &node : nodes) {
    if (node.inputID && !allBindings.contains(node.inputID))
      allBindings.try_emplace(node.inputID,
                              bind(evidence, node.source->getLoc(), node.value,
                                   node.valid, node.path, node.inputID,
                                   node.domain.attr));
    allBindings.try_emplace(node.id, bind(evidence, node.source->getLoc(),
                                          node.value, node.valid, node.path,
                                          node.id, node.domain.attr));
  }

  SmallVector<unsigned> consumers(nodes.size());
  for (auto [index, raw] : llvm::enumerate(required)) {
    auto refs = cast<DictionaryAttr>(raw).getAs<ArrayAttr>("operands");
    for (Attribute rawRef : refs) {
      auto ref = dyn_cast<DictionaryAttr>(rawRef);
      if (ref && ref.getAs<StringAttr>("kind") ==
                     StringAttr::get(rule.getContext(), "node"))
        ++consumers[nodeRef(ref, rule)];
    }
  }
  DenseSet<unsigned> assigned;
  for (unsigned root = 0; root < nodes.size(); ++root) {
    if (consumers[root])
      continue;
    DenseSet<unsigned> component;
    SmallVector<unsigned> pending{root};
    while (!pending.empty()) {
      unsigned current = pending.pop_back_val();
      if (!component.insert(current).second)
        continue;
      for (Attribute rawRef : cast<DictionaryAttr>(required[current])
                                  .getAs<ArrayAttr>("operands")) {
        auto ref = dyn_cast<DictionaryAttr>(rawRef);
        if (ref && ref.getAs<StringAttr>("kind") ==
                       StringAttr::get(rule.getContext(), "node"))
          pending.push_back(nodeRef(ref, rule));
      }
    }
    SmallVector<Attribute> obligations, inputIDs, inputDomains;
    SmallVector<Attribute> checkBindings;
    SmallVector<Value> inputValues, inputValids, checkConditions, checkPaths;
    for (unsigned index = 0; index < nodes.size(); ++index)
      if (component.contains(index)) {
        assigned.insert(index);
        obligations.push_back(required[index]);
        if (nodes[index].inputID) {
          inputIDs.push_back(nodes[index].inputID);
          inputDomains.push_back(nodes[index].domain.attr);
          inputValues.push_back(nodes[index].value);
          inputValids.push_back(nodes[index].valid);
        }
        if (rangeExpects[index]) {
          ac::SourceExpectOp expect = rangeExpects[index];
          checkConditions.push_back(expect.getCondition());
          checkPaths.push_back(expect.getPath());
          checkBindings.push_back(attrs.getDictionaryAttr({
              attrs.getNamedAttr(
                  "id", expect->getAttrOfType<DictionaryAttr>("ac.check_id")),
              attrs.getNamedAttr("owner", nodes[index].id),
              attrs.getNamedAttr("kind", attrs.getStringAttr("range")),
              attrs.getNamedAttr("operand_ordinal",
                                 attrs.getI32IntegerAttr(checkBindings.size())),
          }));
        }
      }
    OperationState proof(nodes[root].source->getLoc(),
                         ac::NumericProofOp::getOperationName());
    proof.addOperands(nodes[root].path);
    proof.addOperands(inputValues);
    proof.addOperands(inputValids);
    proof.addOperands(nodes[root].value);
    proof.addOperands(nodes[root].valid);
    proof.addOperands(checkConditions);
    proof.addOperands(checkPaths);
    proof.addAttribute(
        "operand_segment_sizes",
        attrs.getDenseI32ArrayAttr(
            {1, int32_t(inputValues.size()), int32_t(inputValids.size()), 1, 1,
             int32_t(checkConditions.size()), int32_t(checkPaths.size())}));
    proof.addAttribute("mode", attrs.getStringAttr("exact"));
    proof.addAttribute("result_domain", nodes[root].domain.attr);
    proof.addAttribute("input_ids", attrs.getArrayAttr(inputIDs));
    proof.addAttribute("input_domains", attrs.getArrayAttr(inputDomains));
    proof.addAttribute("result_id", nodes[root].id);
    proof.addAttribute("obligations", attrs.getArrayAttr(obligations));
    proof.addAttribute("checks", attrs.getArrayAttr(checkBindings));
    proof.addAttribute(
        "origin",
        nodes[root].source->getAttrOfType<DictionaryAttr>("ac.origin"));
    evidence.create(proof);
  }
  if (assigned.size() != nodes.size())
    return rule.emitOpError() << "composition node graph has no root partition";

  SmallVector<ac::SourceUseOp> sourceUses(body.getOps<ac::SourceUseOp>());
  auto outputTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  for (ac::SourceUseOp use : sourceUses) {
    DictionaryAttr sourceID = use.getSourceAttr();
    if (!allBindings.contains(sourceID)) {
      auto outputs = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
      auto state = use.getTargetAttr().getAs<DictionaryAttr>("state");
      auto found = llvm::find(outputs.getValue(), state);
      if (found == outputs.end())
        return rule.emitOpError() << "composition use target is absent";
      auto logical = cast<DictionaryAttr>(outputTypes[found - outputs.begin()]);
      allBindings.try_emplace(
          sourceID, bind(evidence, use.getLoc(), use.getValue(), use.getValid(),
                         use.getValid(), sourceID, logical));
    }
    OpBuilder marker(use);
    OperationState state(use.getLoc(), ac::ValueUseOp::getOperationName());
    state.addOperands({use.getValue(), use.getValid(), use.getPath()});
    state.addAttribute("id", use.getIdAttr());
    state.addAttribute("source", sourceID);
    auto valueUse = cast<ac::ValueUseOp>(marker.create(state));
    marker.setInsertionPointAfter(valueUse);
    Value enabled = arith::AndIOp::create(marker, use.getLoc(), use.getValid(),
                                          use.getPath());
    use.getData().replaceAllUsesWith(use.getValue());
    use.getEnabled().replaceAllUsesWith(enabled);
  }
  for (ac::SourceUseOp use : sourceUses)
    use.erase();
  for (Operation *operation : llvm::reverse(math))
    operation->erase();
  bool changed = true;
  while (changed) {
    changed = false;
    for (Operation &operation :
         llvm::make_early_inc_range(llvm::reverse(body))) {
      if (!isa<arith::ConstantOp, arith::AndIOp, arith::OrIOp, arith::XOrIOp,
               arith::ExtUIOp, arith::ExtSIOp, arith::TruncIOp, arith::AddIOp,
               arith::CmpIOp, arith::SelectOp>(operation) ||
          llvm::any_of(operation.getResults(),
                       [](Value result) { return !result.use_empty(); }))
        continue;
      operation.erase();
      changed = true;
    }
  }
  return ac::verifyNumericCompositionClosure(rule);
}

} // namespace acir::compiler
