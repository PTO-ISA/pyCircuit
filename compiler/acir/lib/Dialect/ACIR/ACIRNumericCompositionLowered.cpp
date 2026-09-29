#include "ACIRNumericCompositionDetail.h"

#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"

#include <algorithm>

using namespace mlir;

namespace acir::ac::composition_detail {
namespace {

struct Interval {
  APSInt lower;
  APSInt upper;
  IntegerType storage;
  bool isUnsigned;
};

Value stripExtension(Value value);
bool matchesFit(Value actual, Value source, unsigned width, bool isUnsigned,
                bool allowNarrow = false);

APSInt mathValue(MathIntAttr attr) {
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
int compareMath(const APSInt &left, const APSInt &right) {
  unsigned width = std::max(left.getBitWidth(), right.getBitWidth()) + 1;
  APInt lhs = extend(left, width), rhs = extend(right, width);
  if (lhs == rhs)
    return 0;
  return lhs.slt(rhs) ? -1 : 1;
}
APSInt addMath(const APSInt &left, const APSInt &right) {
  unsigned width = std::max(left.getBitWidth(), right.getBitWidth()) + 1;
  APInt value = extend(left, width) + extend(right, width);
  bool negative = value.isNegative();
  return APSInt(std::move(value), !negative);
}
unsigned canonicalWidth(const APSInt &lower, const APSInt &upper) {
  APSInt maximum = upper;
  --maximum;
  if (!lower.isNegative())
    return std::max(1u, maximum.getActiveBits());
  for (unsigned width = 1; width <= 64; ++width)
    if (lower.isSignedIntN(width) && maximum.isSignedIntN(width))
      return width;
  return 0;
}
bool exactInterval(const Interval &actual, const APSInt &lower,
                   const APSInt &upper) {
  return compareMath(actual.lower, lower) == 0 &&
         compareMath(actual.upper, upper) == 0 &&
         actual.storage.getWidth() == canonicalWidth(lower, upper) &&
         actual.isUnsigned == !lower.isNegative();
}
FailureOr<Interval> interval(DictionaryAttr domain, Operation *owner) {
  auto storage = domain ? domain.getAs<TypeAttr>("storage") : TypeAttr();
  auto type =
      storage ? dyn_cast<IntegerType>(storage.getValue()) : IntegerType();
  auto lower = domain ? domain.getAs<MathIntAttr>("lower") : MathIntAttr();
  auto upper = domain ? domain.getAs<MathIntAttr>("upper") : MathIntAttr();
  auto interpretation =
      domain ? domain.getAs<StringAttr>("interpretation") : StringAttr();
  if (!type || !lower || !upper || !interpretation)
    return owner->emitOpError() << "composition boundary domain is incomplete";
  return Interval{mathValue(lower), mathValue(upper), type,
                  interpretation.getValue() == "unsigned"};
}
bool boolConstant(Value value, bool expected) {
  auto op = value.getDefiningOp<arith::ConstantOp>();
  auto attr = op ? dyn_cast<IntegerAttr>(op.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) &&
         attr.getValue() == APInt(1, expected);
}
bool andPair(Value value, Value left, Value right) {
  auto op = value.getDefiningOp<arith::AndIOp>();
  return op && ((op.getLhs() == left && op.getRhs() == right) ||
                (op.getLhs() == right && op.getRhs() == left));
}
bool endpoint(Value value, const APSInt &expected, bool isUnsigned) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto attr =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  if (!attr)
    return false;
  APSInt actual(attr.getValue(), isUnsigned);
  return compareMath(actual, expected) == 0;
}
bool boundMatches(Value condition, Value value, const Interval &input,
                  const APSInt &limit, bool lower) {
  bool alwaysTrue = lower ? compareMath(input.lower, limit) >= 0
                          : compareMath(input.upper, limit) <= 0;
  bool alwaysFalse = lower ? compareMath(input.upper, limit) <= 0
                           : compareMath(input.lower, limit) >= 0;
  if (alwaysTrue || alwaysFalse)
    return boolConstant(condition, alwaysTrue);
  auto cmp = condition.getDefiningOp<arith::CmpIOp>();
  auto predicate = lower ? (input.isUnsigned ? arith::CmpIPredicate::ule
                                             : arith::CmpIPredicate::sle)
                         : (input.isUnsigned ? arith::CmpIPredicate::ult
                                             : arith::CmpIPredicate::slt);
  Value actualValue = cmp ? (lower ? cmp.getRhs() : cmp.getLhs()) : Value();
  auto actualType = actualValue ? dyn_cast<IntegerType>(actualValue.getType())
                                : IntegerType();
  return cmp && actualType && cmp.getPredicate() == predicate &&
         matchesFit(actualValue, value, actualType.getWidth(),
                    input.isUnsigned) &&
         endpoint(lower ? cmp.getLhs() : cmp.getRhs(), limit, input.isUnsigned);
}
ValueBindingOp binding(Block &body, DictionaryAttr id) {
  ValueBindingOp found;
  for (ValueBindingOp candidate : body.getOps<ValueBindingOp>())
    if (candidate.getIdAttr() == id) {
      if (found)
        return {};
      found = candidate;
    }
  return found;
}
bool refIndex(Attribute raw, unsigned &index) {
  auto ref = dyn_cast<DictionaryAttr>(raw);
  auto value = ref ? ref.getAs<IntegerAttr>("index") : IntegerAttr();
  if (!ref || ref.size() != 2 || !value ||
      ref.getAs<StringAttr>("kind") !=
          StringAttr::get(ref.getContext(), "node") ||
      !value.getType().isInteger(32) || value.getValue().getActiveBits() > 32)
    return false;
  index = static_cast<unsigned>(value.getValue().getZExtValue());
  return true;
}
Value stripExtension(Value value) {
  if (auto op = value.getDefiningOp<arith::ExtUIOp>())
    return op.getIn();
  if (auto op = value.getDefiningOp<arith::ExtSIOp>())
    return op.getIn();
  if (auto op = value.getDefiningOp<arith::TruncIOp>())
    return op.getIn();
  return value;
}
bool matchesFit(Value actual, Value source, unsigned width, bool isUnsigned,
                bool allowNarrow) {
  auto sourceType = dyn_cast<IntegerType>(source.getType());
  auto actualType = dyn_cast<IntegerType>(actual.getType());
  if (!sourceType || !actualType || actualType.getWidth() != width)
    return false;
  if (sourceType.getWidth() == width)
    return actual == source;
  if (sourceType.getWidth() < width) {
    if (isUnsigned) {
      auto extension = actual.getDefiningOp<arith::ExtUIOp>();
      return extension && extension.getIn() == source;
    }
    auto extension = actual.getDefiningOp<arith::ExtSIOp>();
    return extension && extension.getIn() == source;
  }
  auto trunc =
      allowNarrow ? actual.getDefiningOp<arith::TruncIOp>() : arith::TruncIOp();
  return trunc && trunc.getIn() == source;
}
bool validEquation(Value actual, Value path, Value lhs, Value rhs) {
  auto outer = actual.getDefiningOp<arith::AndIOp>();
  if (!outer)
    return false;
  Value inner = outer.getLhs() == rhs   ? outer.getRhs()
                : outer.getRhs() == rhs ? outer.getLhs()
                                        : Value();
  return inner && andPair(inner, path, lhs);
}
bool exactConstant(Value value, MathIntAttr expected, DictionaryAttr domain) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto attr =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  auto interpretation = domain.getAs<StringAttr>("interpretation");
  if (!attr || !interpretation)
    return false;
  llvm::APSInt actual(attr.getValue(), interpretation.getValue() == "unsigned");
  SmallString<32> text;
  actual.toString(text);
  return text == expected.getCanonicalValue();
}
LogicalResult verifyBoundary(RuleOp rule, ValueBindingOp source,
                             ValueBindingOp result) {
  auto emit = [&] { return rule.emitOpError(); };
  auto input = interval(source.getDomainAttr(), result);
  auto output = interval(result.getDomainAttr(), result);
  auto select = result.getValue().getDefiningOp<arith::SelectOp>();
  auto zero = select ? select.getFalseValue().getDefiningOp<arith::ConstantOp>()
                     : arith::ConstantOp();
  auto zeroAttr = zero ? dyn_cast<IntegerAttr>(zero.getValue()) : IntegerAttr();
  auto valid = result.getValid().getDefiningOp<arith::AndIOp>();
  Value demand, safety;
  if (valid) {
    if (andPair(valid.getLhs(), result.getPath(), source.getValid())) {
      demand = valid.getLhs();
      safety = valid.getRhs();
    } else if (andPair(valid.getRhs(), result.getPath(), source.getValid())) {
      demand = valid.getRhs();
      safety = valid.getLhs();
    }
  }
  auto safetyAnd =
      safety ? safety.getDefiningOp<arith::AndIOp>() : arith::AndIOp();
  if (failed(input) || failed(output) || !output->isUnsigned)
    return emit() << "composition boundary cannot decode logical intervals";
  if (!select || select.getCondition() != result.getValid())
    return emit() << "composition boundary result is not selected by validity";
  if (!zeroAttr || !zeroAttr.getValue().isZero())
    return emit() << "composition boundary fallback is not typed zero";
  if (!valid || !demand || !safetyAnd)
    return emit() << "composition boundary validity lacks demand/safety AND";
  if (!boundMatches(safetyAnd.getLhs(), source.getValue(), *input,
                    output->lower, true))
    return emit() << "composition boundary lower proof is invalid";
  if (!boundMatches(safetyAnd.getRhs(), source.getValue(), *input,
                    output->upper, false))
    return emit() << "composition boundary upper proof is invalid";
  if (!matchesFit(select.getTrueValue(), source.getValue(),
                  output->storage.getWidth(), input->isUnsigned,
                  /*allowNarrow=*/true))
    return emit() << "composition boundary converted value is redirected";
  bool expect = false;
  for (SourceExpectOp check : rule.getBody().front().getOps<SourceExpectOp>())
    if (check.getKind() == "range" && check.getCondition() == safety &&
        check.getPath() == demand)
      expect = true;
  return expect ? success()
                : emit() << "composition boundary has no retained range check";
}

} // namespace

LogicalResult verifyLowered(RuleOp rule, ArrayAttr required) {
  auto emit = [&] { return rule.emitOpError(); };
  Block &body = rule.getBody().front();
  if (failed(verifyNodeSchema(rule, required)))
    return failure();
  if (llvm::any_of(body,
                   [](Operation &operation) {
                     return operation.getName().getStringRef().starts_with(
                         "ac.math.");
                   }) ||
      !body.getOps<SourceUseOp>().empty())
    return emit() << "lowered composition retains source math/use operations";
  SmallVector<ValueBindingOp> bindings(body.getOps<ValueBindingOp>());
  SmallVector<NumericProofOp> proofs(body.getOps<NumericProofOp>());
  if (bindings.empty() || proofs.empty())
    return emit() << "lowered composition lacks bindings or proofs";
  DenseSet<Attribute> bindingIDs;
  for (ValueBindingOp value : bindings) {
    auto id = value.getIdAttr();
    auto logical = value.getDomainAttr();
    auto kind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
    auto storage = logical ? logical.getAs<TypeAttr>("storage") : TypeAttr();
    bool proofBool = kind && kind.getValue() == "bool" && logical.size() == 1 &&
                     value.getValue().getDefiningOp<arith::CmpIOp>();
    bool logicalBool = kind && kind.getValue() == "bool" &&
                       logical.size() == 2 && storage &&
                       storage.getValue().isInteger(1);
    bool physical =
        kind && (((proofBool || logicalBool) &&
                  value.getValue().getType().isInteger(1)) ||
                 (kind.getValue() == "integer" && logical.size() == 5 &&
                  storage && storage.getValue() == value.getValue().getType()));
    if (value->getAttrs().size() != 2 ||
        failed(detail::verifyValueID(id, emit)) ||
        (!proofBool &&
         failed(detail::verifyLogicalTypeStructure(logical, emit))) ||
        !physical || !bindingIDs.insert(id).second ||
        !safeControl(value.getValid(), rule) ||
        !safeControl(value.getPath(), rule))
      return emit()
             << "composition ValueBinding schema/domain/control is invalid";
  }
  DenseMap<Attribute, unsigned> requiredIndex;
  DenseSet<Attribute> covered;
  for (auto [index, raw] : llvm::enumerate(required)) {
    auto node = dyn_cast<DictionaryAttr>(raw);
    auto id = node ? node.getAs<DictionaryAttr>("id") : DictionaryAttr();
    if (!node || !id || !requiredIndex.try_emplace(id, index).second ||
        !binding(body, id))
      return emit() << "composition required node has no unique binding";
  }
  for (NumericProofOp proof : proofs) {
    auto obligations = proof.getObligationsAttr();
    if (!obligations || obligations.empty() || proof.getMode() != "exact" ||
        proof->getAttrs().size() != 9 || proof->hasAttr("width") ||
        !binding(body, proof.getResultIdAttr()))
      return emit() << "composition proof envelope is invalid";
    SmallVector<unsigned> members;
    DenseSet<unsigned> referenced;
    SmallVector<Attribute> expectedInputIDs, expectedInputDomains;
    SmallVector<Value> expectedInputValues, expectedInputValids;
    Builder attrs(rule.getContext());
    for (Attribute raw : obligations) {
      auto node = dyn_cast<DictionaryAttr>(raw);
      auto id = node ? node.getAs<DictionaryAttr>("id") : DictionaryAttr();
      auto found = requiredIndex.find(id);
      if (!node || found == requiredIndex.end() ||
          required[found->second] != node || !covered.insert(id).second)
        return emit() << "composition proof partition is stale or overlapping";
      if (!members.empty() && found->second <= members.back())
        return emit()
               << "composition proof obligations are not in source order";
      members.push_back(found->second);
      for (Attribute rawRef : node.getAs<ArrayAttr>("operands")) {
        unsigned ref = 0;
        if (refIndex(rawRef, ref))
          referenced.insert(ref);
      }
      if (node.getAs<StringAttr>("operator").getValue() == "from_bits") {
        auto inputID = attrs.getDictionaryAttr({
            attrs.getNamedAttr("origin", id.get("origin")),
            attrs.getNamedAttr("slot", attrs.getI32IntegerAttr(0)),
        });
        auto input = binding(body, inputID);
        if (!input)
          return emit() << "composition proof input has no binding";
        expectedInputIDs.push_back(inputID);
        expectedInputDomains.push_back(input.getDomainAttr());
        expectedInputValues.push_back(input.getValue());
        expectedInputValids.push_back(input.getValid());
      }
    }
    DenseSet<unsigned> memberSet(members.begin(), members.end());
    auto [minimum, maximum] =
        std::minmax_element(members.begin(), members.end());
    if (unsigned(*maximum - *minimum + 1) != members.size())
      return emit() << "composition proof component is not contiguous";
    for (unsigned member : members)
      for (Attribute rawRef : cast<DictionaryAttr>(required[member])
                                  .getAs<ArrayAttr>("operands")) {
        unsigned ref = 0;
        if (refIndex(rawRef, ref) && !memberSet.contains(ref))
          return emit() << "composition proof component is not closed";
      }
    SmallVector<unsigned> roots;
    for (unsigned member : members)
      if (!referenced.contains(member))
        roots.push_back(member);
    if (roots.size() != 1)
      return emit() << "composition proof component has no unique root";
    ValueBindingOp rootBinding =
        binding(body, cast<DictionaryAttr>(required[roots.front()])
                          .getAs<DictionaryAttr>("id"));
    auto sizes = proof.getOperandSegmentSizes();
    if (proof.getResultIdAttr() != rootBinding.getIdAttr() ||
        proof.getResultDomainAttr() != rootBinding.getDomainAttr() ||
        proof.getInputIdsAttr() != attrs.getArrayAttr(expectedInputIDs) ||
        proof.getInputDomainsAttr() !=
            attrs.getArrayAttr(expectedInputDomains) ||
        sizes.size() != 7 || sizes[0] != 1 ||
        sizes[1] != int32_t(expectedInputValues.size()) ||
        sizes[2] != int32_t(expectedInputValids.size()) || sizes[3] != 1 ||
        sizes[4] != 1 || proof->getOperand(0) != rootBinding.getPath())
      return emit() << "composition proof root/input/segments are invalid";
    unsigned cursor = 1;
    for (Value value : expectedInputValues)
      if (proof->getOperand(cursor++) != value)
        return emit() << "composition proof input value is redirected";
    for (Value value : expectedInputValids)
      if (proof->getOperand(cursor++) != value)
        return emit() << "composition proof input valid is redirected";
    if (proof->getOperand(cursor++) != rootBinding.getValue() ||
        proof->getOperand(cursor++) != rootBinding.getValid() ||
        proof.getOriginAttr() !=
            rootBinding.getIdAttr().getAs<DictionaryAttr>("origin"))
      return emit() << "composition proof actual result/origin is invalid";
    if (proof.getChecksAttr().size() != unsigned(sizes[5]) ||
        sizes[5] != sizes[6] ||
        cursor + sizes[5] + sizes[6] != proof->getNumOperands())
      return emit() << "composition proof check segments are invalid";
    for (auto [checkIndex, raw] : llvm::enumerate(proof.getChecksAttr())) {
      auto check = dyn_cast<DictionaryAttr>(raw);
      auto ordinal =
          check ? check.getAs<IntegerAttr>("operand_ordinal") : IntegerAttr();
      auto owner =
          check ? check.getAs<DictionaryAttr>("owner") : DictionaryAttr();
      bool ownedBoundary = llvm::any_of(members, [&](unsigned member) {
        auto node = cast<DictionaryAttr>(required[member]);
        return node.getAs<DictionaryAttr>("id") == owner &&
               node.getAs<StringAttr>("operator").getValue() == "to_bits";
      });
      SourceExpectOp expect;
      auto id = check ? check.getAs<DictionaryAttr>("id") : DictionaryAttr();
      for (SourceExpectOp candidate : body.getOps<SourceExpectOp>())
        if (candidate->getAttrOfType<DictionaryAttr>("ac.check_id") == id) {
          if (expect)
            return emit() << "composition proof CheckID is duplicated";
          expect = candidate;
        }
      if (!check || check.size() != 4 || !ordinal ||
          ordinal.getValue() != APInt(32, checkIndex) || !ownedBoundary ||
          check.getAs<StringAttr>("kind") !=
              StringAttr::get(rule.getContext(), "range") ||
          !expect ||
          proof->getOperand(cursor + checkIndex) != expect.getCondition() ||
          proof->getOperand(cursor + sizes[5] + checkIndex) != expect.getPath())
        return emit() << "composition proof CheckBinding is invalid";
    }
  }
  if (covered.size() != required.size())
    return emit() << "composition proofs do not cover every numeric node";
  for (auto [index, raw] : llvm::enumerate(required)) {
    auto node = cast<DictionaryAttr>(raw);
    auto id = node.getAs<DictionaryAttr>("id");
    auto operation = node.getAs<StringAttr>("operator").getValue();
    auto operands = node.getAs<ArrayAttr>("operands");
    ValueBindingOp result = binding(body, id);
    auto referenced = [&](unsigned operand) -> ValueBindingOp {
      unsigned ref = 0;
      return refIndex(operands[operand], ref) && ref < index
                 ? binding(body, cast<DictionaryAttr>(required[ref])
                                     .getAs<DictionaryAttr>("id"))
                 : ValueBindingOp();
    };
    if (operation == "constant") {
      auto ref = cast<DictionaryAttr>(operands[0]);
      auto actual = interval(result.getDomainAttr(), result);
      APSInt expected = mathValue(ref.getAs<MathIntAttr>("value"));
      APSInt upper = expected.extend(expected.getBitWidth() + 1);
      ++upper;
      if (failed(actual) || !exactInterval(*actual, expected, upper) ||
          !exactConstant(result.getValue(), ref.getAs<MathIntAttr>("value"),
                         result.getDomainAttr()) ||
          !safeControl(result.getValid(), rule) ||
          !safeControl(result.getPath(), rule))
        return emit() << "composition finite constant is invalid";
    } else if (operation == "from_bits") {
      Builder attrs(rule.getContext());
      auto inputID = attrs.getDictionaryAttr({
          attrs.getNamedAttr("origin", id.get("origin")),
          attrs.getNamedAttr("slot", attrs.getI32IntegerAttr(0)),
      });
      auto input = binding(body, inputID);
      Value current = result.getValue();
      auto read = current.getDefiningOp<SourceReadOp>();
      auto argument = read ? dyn_cast<BlockArgument>(read.getCurrent())
                           : dyn_cast<BlockArgument>(current);
      auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
      if (!input || !argument || argument.getOwner() != &body || !inputTypes ||
          argument.getArgNumber() >= inputTypes.size() ||
          inputTypes[argument.getArgNumber()] != result.getDomainAttr() ||
          input.getValue() != result.getValue() ||
          input.getDomainAttr() != result.getDomainAttr() ||
          input.getValid() != result.getValid() ||
          input.getPath() != result.getPath())
        return emit() << "composition finite input binding is invalid";
    } else if (operation == "add") {
      auto add = result.getValue().getDefiningOp<arith::AddIOp>();
      auto lhs = referenced(0), rhs = referenced(1);
      auto lhsDomain = lhs ? interval(lhs.getDomainAttr(), result)
                           : FailureOr<Interval>(failure());
      auto rhsDomain = rhs ? interval(rhs.getDomainAttr(), result)
                           : FailureOr<Interval>(failure());
      auto actual = interval(result.getDomainAttr(), result);
      APSInt expectedLower, expectedUpper;
      if (succeeded(lhsDomain) && succeeded(rhsDomain)) {
        expectedLower = addMath(lhsDomain->lower, rhsDomain->lower);
        expectedUpper = addMath(lhsDomain->upper, rhsDomain->upper);
        --expectedUpper;
      }
      if (!add || !lhs || !rhs || failed(lhsDomain) || failed(rhsDomain) ||
          failed(actual) ||
          !exactInterval(*actual, expectedLower, expectedUpper) ||
          !matchesFit(add.getLhs(), lhs.getValue(), actual->storage.getWidth(),
                      lhsDomain->isUnsigned) ||
          !matchesFit(add.getRhs(), rhs.getValue(), actual->storage.getWidth(),
                      rhsDomain->isUnsigned) ||
          !validEquation(result.getValid(), result.getPath(), lhs.getValid(),
                         rhs.getValid()) ||
          add.getOverflowFlags() != arith::IntegerOverflowFlags::none)
        return emit() << "composition finite addition is invalid";
    } else if (operation == "and_bits") {
      Value raw = result.getValue();
      if (auto trunc = raw.getDefiningOp<arith::TruncIOp>())
        raw = trunc.getIn();
      auto bitAnd = raw.getDefiningOp<arith::AndIOp>();
      auto lhs = referenced(0), rhs = referenced(1);
      auto lhsDomain = lhs ? interval(lhs.getDomainAttr(), result)
                           : FailureOr<Interval>(failure());
      auto rhsDomain = rhs ? interval(rhs.getDomainAttr(), result)
                           : FailureOr<Interval>(failure());
      auto actual = interval(result.getDomainAttr(), result);
      APSInt zero(APInt(2, 0), true), expectedUpper;
      if (succeeded(rhsDomain)) {
        expectedUpper =
            rhsDomain->lower.extend(rhsDomain->lower.getBitWidth() + 1);
        ++expectedUpper;
      }
      unsigned width = succeeded(lhsDomain) && succeeded(rhsDomain)
                           ? std::max(lhsDomain->storage.getWidth(),
                                      rhsDomain->storage.getWidth())
                           : 0;
      bool canonicalResult =
          bitAnd && succeeded(actual) &&
          ((actual->storage.getWidth() == width &&
            result.getValue() == bitAnd) ||
           (actual->storage.getWidth() < width &&
            result.getValue().getDefiningOp<arith::TruncIOp>() &&
            result.getValue().getDefiningOp<arith::TruncIOp>().getIn() ==
                bitAnd.getResult()));
      if (!bitAnd || !lhs || !rhs || failed(lhsDomain) || failed(rhsDomain) ||
          failed(actual) || compareMath(rhsDomain->upper, expectedUpper) != 0 ||
          !expectedUpper.isPowerOf2() ||
          !exactInterval(*actual, zero, expectedUpper) || !canonicalResult ||
          !matchesFit(bitAnd.getLhs(), lhs.getValue(), width,
                      lhsDomain->isUnsigned) ||
          !matchesFit(bitAnd.getRhs(), rhs.getValue(), width,
                      rhsDomain->isUnsigned) ||
          !validEquation(result.getValid(), result.getPath(), lhs.getValid(),
                         rhs.getValid()))
        return emit() << "composition finite bitwise AND is invalid";
    } else if (operation == "eq" || operation == "ne" || operation == "lt") {
      auto compare = result.getValue().getDefiningOp<arith::CmpIOp>();
      auto lhs = referenced(0), rhs = referenced(1);
      auto lhsDomain = lhs ? interval(lhs.getDomainAttr(), result)
                           : FailureOr<Interval>(failure());
      auto rhsDomain = rhs ? interval(rhs.getDomainAttr(), result)
                           : FailureOr<Interval>(failure());
      auto kind = result.getDomainAttr().getAs<StringAttr>("kind");
      unsigned width = succeeded(lhsDomain) && succeeded(rhsDomain)
                           ? std::max(lhsDomain->storage.getWidth(),
                                      rhsDomain->storage.getWidth())
                           : 0;
      auto expected = operation == "eq"   ? arith::CmpIPredicate::eq
                      : operation == "ne" ? arith::CmpIPredicate::ne
                      : succeeded(lhsDomain) && succeeded(rhsDomain) &&
                              lhsDomain->isUnsigned && rhsDomain->isUnsigned
                          ? arith::CmpIPredicate::ult
                          : arith::CmpIPredicate::slt;
      if (!compare || !lhs || !rhs || failed(lhsDomain) || failed(rhsDomain) ||
          !kind || kind.getValue() != "bool" ||
          result.getDomainAttr().size() != 1 || !lhsDomain->isUnsigned ||
          !rhsDomain->isUnsigned || compare.getPredicate() != expected ||
          !matchesFit(compare.getLhs(), lhs.getValue(), width,
                      lhsDomain->isUnsigned) ||
          !matchesFit(compare.getRhs(), rhs.getValue(), width,
                      rhsDomain->isUnsigned) ||
          !validEquation(result.getValid(), result.getPath(), lhs.getValid(),
                         rhs.getValid()))
        return emit() << "composition finite comparison is invalid";
    } else if (operation == "to_bits") {
      auto source = referenced(0);
      if (!source || !isa<IntegerType>(result.getValue().getType()) ||
          failed(verifyBoundary(rule, source, result)))
        return emit() << "composition finite boundary is invalid";
    } else {
      return emit() << "composition finite node opcode is unsupported";
    }
  }
  if (failed(verifyFiniteInventory(rule, body)))
    return failure();
  return verifyUses(rule, body);
}

} // namespace acir::ac::composition_detail
