#include "Dialect/ACIR/ACIRNumericRange.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

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

struct SourceRange {
  ac::SourceReadOp read;
  ac::MathFromBitsOp from;
  ac::MathToBitsOp convert;
  ac::SourceExpectOp expect;
  DictionaryAttr inputID;
  DictionaryAttr fromID;
  DictionaryAttr resultID;
  DictionaryAttr inputDomain;
  DictionaryAttr resultDomain;
  DictionaryAttr checkID;
  DictionaryAttr checkBinding;
  Value path;
  Value valid;
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

int compare(const APSInt &lhs, const APSInt &rhs) {
  unsigned width = std::max(lhs.getBitWidth(), rhs.getBitWidth()) + 1;
  APInt left = extend(lhs, width), right = extend(rhs, width);
  if (left == right)
    return 0;
  return left.slt(right) ? -1 : 1;
}

FailureOr<Domain> parseDomain(DictionaryAttr attr, Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (!attr || failed(ac::detail::verifyLogicalTypeStructure(attr, emit)))
    return failure();
  auto kind = attr.getAs<StringAttr>("kind");
  auto storageAttr = attr.getAs<TypeAttr>("storage");
  auto lowerAttr = attr.getAs<ac::MathIntAttr>("lower");
  auto upperAttr = attr.getAs<ac::MathIntAttr>("upper");
  auto interpretation = attr.getAs<StringAttr>("interpretation");
  auto storage = storageAttr ? dyn_cast<IntegerType>(storageAttr.getValue())
                             : IntegerType();
  if (attr.size() != 5 || !kind || kind.getValue() != "integer" || !storage ||
      !storage.isSignless() || storage.getWidth() == 0 ||
      storage.getWidth() > 64 || !lowerAttr || !upperAttr || !interpretation ||
      (interpretation.getValue() != "signed" &&
       interpretation.getValue() != "unsigned"))
    return emit() << "checked to_bits requires a closed i1..i64 domain";
  APSInt lower = mathValue(lowerAttr), upper = mathValue(upperAttr);
  bool isUnsigned = interpretation.getValue() == "unsigned";
  if (compare(lower, upper) >= 0 || (isUnsigned && lower.isNegative()) ||
      (!isUnsigned && !lower.isNegative()))
    return emit() << "checked to_bits domain interval/sign is invalid";
  return Domain{attr, lower, upper, storage, isUnsigned};
}

FailureOr<DictionaryAttr> recipeNode(Attribute raw, StringRef opcode,
                                     unsigned operands, bool boundary,
                                     Operation *owner) {
  auto node = dyn_cast<DictionaryAttr>(raw);
  auto id = node ? node.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto operation = node ? node.getAs<StringAttr>("operator") : StringAttr();
  auto refs = node ? node.getAs<ArrayAttr>("operands") : ArrayAttr();
  auto target = node ? node.getAs<DictionaryAttr>("target") : DictionaryAttr();
  auto targetKind = target ? target.getAs<StringAttr>("kind") : StringAttr();
  bool targetOK = boundary ? target && target.size() == 2 && targetKind &&
                                 targetKind.getValue() == "integer_boundary" &&
                                 target.getAs<DictionaryAttr>("domain")
                           : target && target.size() == 1 && targetKind &&
                                 targetKind.getValue() == "none";
  if (!node || node.size() != 4 || !id || !operation ||
      operation.getValue() != opcode || !refs || refs.size() != operands ||
      !targetOK || failed(ac::detail::verifyValueID(id, [&] {
        return owner->emitOpError();
      })))
    return owner->emitOpError() << "checked to_bits recipe node is invalid";
  return node;
}

bool ref(Attribute raw, StringRef kind, unsigned index, Operation *owner) {
  auto dictionary = dyn_cast<DictionaryAttr>(raw);
  auto actualKind =
      dictionary ? dictionary.getAs<StringAttr>("kind") : StringAttr();
  auto actualIndex =
      dictionary ? dictionary.getAs<IntegerAttr>("index") : IntegerAttr();
  if (!dictionary || dictionary.size() != 2 || !actualKind ||
      actualKind.getValue() != kind || !actualIndex)
    return false;
  auto decoded = ac::detail::decodeU32(actualIndex, "to_bits recipe index",
                                       [&] { return owner->emitOpError(); });
  return succeeded(decoded) && *decoded == index;
}

bool boolConstant(Value value, bool expected) {
  auto op = value.getDefiningOp<arith::ConstantOp>();
  auto attr = op ? dyn_cast<IntegerAttr>(op.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) &&
         attr.getValue() == APInt(1, expected);
}

bool approvedPath(Value path, ac::RuleOp rule) {
  if (boolConstant(path, true) || boolConstant(path, false))
    return true;
  auto read = path.getDefiningOp<ac::SourceReadOp>();
  auto current =
      read ? dyn_cast<BlockArgument>(read.getCurrent()) : BlockArgument();
  auto types = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto logical = current && types && current.getArgNumber() < types.size()
                     ? dyn_cast<DictionaryAttr>(types[current.getArgNumber()])
                     : DictionaryAttr();
  return read && read->getBlock() == &rule.getBody().front() && current &&
         logical &&
         logical.getAs<StringAttr>("kind") ==
             StringAttr::get(rule.getContext(), "bool");
}

FailureOr<SourceRange> inspect(ac::RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  if (rule.getBody().getBlocks().size() != 1 || !rule.getTargets().empty())
    return emit() << "D3 requires one input-only rule block";
  Block &body = rule.getBody().front();
  SmallVector<ac::SourceReadOp> reads(body.getOps<ac::SourceReadOp>());
  SmallVector<ac::MathFromBitsOp> from(body.getOps<ac::MathFromBitsOp>());
  SmallVector<ac::MathToBitsOp> conversions(body.getOps<ac::MathToBitsOp>());
  SmallVector<ac::SourceExpectOp> expects(body.getOps<ac::SourceExpectOp>());
  if (from.size() != 1 || conversions.size() != 1 || expects.size() != 1 ||
      !body.getOps<ac::MathConstantOp>().empty() ||
      !body.getOps<ac::MathBinaryOp>().empty() ||
      !body.getOps<ac::MathCompareOp>().empty() ||
      !body.getOps<ac::ValueBindingOp>().empty() ||
      !body.getOps<ac::NumericProofOp>().empty())
    return emit() << "D3 source inventory is not read/F/to_bits/range expect";
  auto declaredTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  ac::SourceReadOp integerRead;
  for (auto read : reads) {
    auto current = dyn_cast<BlockArgument>(read.getCurrent());
    auto logical =
        current && declaredTypes &&
                current.getArgNumber() < declaredTypes.size()
            ? dyn_cast<DictionaryAttr>(declaredTypes[current.getArgNumber()])
            : DictionaryAttr();
    if (logical && logical.getAs<StringAttr>("kind") ==
                       StringAttr::get(rule.getContext(), "integer")) {
      if (integerRead)
        return emit() << "D3 has multiple integer SourceReads";
      integerRead = read;
    }
  }
  if (!integerRead)
    return emit() << "D3 has no integer SourceRead";
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  auto fromNode = required && required.size() == 2
                      ? recipeNode(required[0], "from_bits", 1, false, rule)
                      : FailureOr<DictionaryAttr>(failure());
  auto resultNode = required && required.size() == 2
                        ? recipeNode(required[1], "to_bits", 1, true, rule)
                        : FailureOr<DictionaryAttr>(failure());
  if (failed(fromNode) || failed(resultNode) ||
      !ref((*fromNode).getAs<ArrayAttr>("operands")[0], "input", 0, rule) ||
      !ref((*resultNode).getAs<ArrayAttr>("operands")[0], "node", 0, rule))
    return emit() << "D3 required_numeric recipe is not canonical";
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto inputBindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto outputTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  auto outputBindings = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto observations =
      rule->getAttrOfType<ArrayAttr>("ac.required_observations");
  if (!inputTypes || !inputBindings ||
      inputTypes.size() != rule.getInputs().size() ||
      inputBindings.size() != rule.getInputs().size() || !outputTypes ||
      !outputTypes.empty() || !outputBindings || !outputBindings.empty() ||
      (observations && !observations.empty()) ||
      !rule->getAttrOfType<DictionaryAttr>("ac.proof_scope"))
    return emit() << "D3 rule metadata is not closed";
  auto current = dyn_cast<BlockArgument>(integerRead.getCurrent());
  if (!current || current.getOwner() != &body ||
      current.getArgNumber() >= inputTypes.size())
    return emit() << "D3 integer read is not a real rule input";
  auto inputDomain =
      dyn_cast<DictionaryAttr>(inputTypes[current.getArgNumber()]);
  auto resultDomain = conversions[0].getDomainAttr();
  auto parsedInput = parseDomain(inputDomain, rule);
  auto parsedResult = parseDomain(resultDomain, rule);
  if (failed(parsedInput) || failed(parsedResult) ||
      from[0].getValue() != integerRead.getResult() ||
      from[0].getDomainAttr() != inputDomain ||
      conversions[0].getValue() != from[0].getResult() ||
      conversions[0].getResult().getType() != parsedResult->storage ||
      !conversions[0].getResult().use_empty() ||
      !llvm::hasSingleElement(conversions[0].getValid().getUses()) ||
      expects[0].getCondition() != conversions[0].getValid() ||
      expects[0].getPath() != conversions[0].getPath() ||
      expects[0].getKind() != "range" ||
      !approvedPath(conversions[0].getPath(), rule))
    return emit() << "D3 source SSA/path/domain is invalid";
  auto resultTarget = (*resultNode).getAs<DictionaryAttr>("target");
  if (resultTarget.getAs<DictionaryAttr>("domain") != resultDomain)
    return emit() << "D3 NumericTarget differs from destination domain";
  auto checkID = expects[0]->getAttrOfType<DictionaryAttr>("ac.check_id");
  auto checkTemplate =
      conversions[0]->getAttrOfType<DictionaryAttr>("ac.check_template");
  auto requiredChecks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  auto registration = checkID ? checkID.getAs<DictionaryAttr>("registration")
                              : DictionaryAttr();
  auto checkOccurrence =
      checkID ? checkID.getAs<DictionaryAttr>("check") : DictionaryAttr();
  auto obligation =
      checkID ? checkID.getAs<IntegerAttr>("obligation") : IntegerAttr();
  auto location = expects[0]->getAttrOfType<DictionaryAttr>("location");
  Builder builder(rule.getContext());
  auto requiredCheck =
      checkID
          ? builder.getDictionaryAttr({
                builder.getNamedAttr("id", checkID),
                builder.getNamedAttr("kind", builder.getStringAttr("range")),
                builder.getNamedAttr("location", location),
            })
          : DictionaryAttr();
  if (!checkID || failed(ac::detail::verifyCheckID(checkID, emit)) ||
      registration != rule.getRegistrationAttr() || !checkOccurrence ||
      !obligation || !checkTemplate || checkTemplate.size() != 4 ||
      checkTemplate.getAs<DictionaryAttr>("leaf") !=
          checkOccurrence.getAs<DictionaryAttr>("site") ||
      checkTemplate.getAs<StringAttr>("kind") !=
          StringAttr::get(rule.getContext(), "range") ||
      checkTemplate.getAs<IntegerAttr>("obligation") != obligation ||
      checkTemplate.getAs<DictionaryAttr>("location") != location ||
      !requiredChecks || requiredChecks.size() != 1 ||
      requiredChecks[0] != requiredCheck)
    return emit() << "D3 check template/CheckID/required check do not match";
  Builder ids(rule.getContext());
  auto inputID = ids.getDictionaryAttr({
      ids.getNamedAttr("origin",
                       integerRead->getAttrOfType<DictionaryAttr>("ac.origin")),
      ids.getNamedAttr("slot", ids.getI32IntegerAttr(0)),
  });
  auto fromID = (*fromNode).getAs<DictionaryAttr>("id");
  auto resultID = (*resultNode).getAs<DictionaryAttr>("id");
  if (inputID == fromID || inputID == resultID || fromID == resultID ||
      fromID.getAs<DictionaryAttr>("origin") !=
          from[0]->getAttrOfType<DictionaryAttr>("ac.origin") ||
      resultID.getAs<DictionaryAttr>("origin") !=
          conversions[0]->getAttrOfType<DictionaryAttr>("ac.origin"))
    return emit() << "D3 I/F/S ValueIDs are invalid";
  auto checkBinding = ids.getDictionaryAttr({
      ids.getNamedAttr("id", checkID),
      ids.getNamedAttr("owner", resultID),
      ids.getNamedAttr("kind", ids.getStringAttr("range")),
      ids.getNamedAttr("operand_ordinal", ids.getI32IntegerAttr(0)),
  });
  if (boolConstant(conversions[0].getPath(), true) &&
      boolConstant(conversions[0].getValueValid(), true) &&
      (compare(parsedInput->upper, parsedResult->lower) <= 0 ||
       compare(parsedInput->lower, parsedResult->upper) >= 0))
    return emit() << "statically demanded checked value is always out of range";
  return SourceRange{integerRead,
                     from[0],
                     conversions[0],
                     expects[0],
                     inputID,
                     fromID,
                     resultID,
                     inputDomain,
                     resultDomain,
                     checkID,
                     checkBinding,
                     conversions[0].getPath(),
                     conversions[0].getValueValid()};
}

Value boolean(OpBuilder &builder, Location location, bool value) {
  return arith::ConstantOp::create(builder, location,
                                   builder.getBoolAttr(value));
}

Value bound(OpBuilder &builder, Location location, Value value,
            const Domain &input, const APSInt &endpoint, bool lower) {
  bool alwaysTrue = lower ? compare(input.lower, endpoint) >= 0
                          : compare(input.upper, endpoint) <= 0;
  bool alwaysFalse = lower ? compare(input.upper, endpoint) <= 0
                           : compare(input.lower, endpoint) >= 0;
  if (alwaysTrue || alwaysFalse)
    return boolean(builder, location, alwaysTrue);
  auto constant = arith::ConstantOp::create(
      builder, location,
      IntegerAttr::get(input.storage,
                       extend(endpoint, input.storage.getWidth())));
  auto predicate = lower ? (input.isUnsigned ? arith::CmpIPredicate::ule
                                             : arith::CmpIPredicate::sle)
                         : (input.isUnsigned ? arith::CmpIPredicate::ult
                                             : arith::CmpIPredicate::slt);
  return lower ? Value(arith::CmpIOp::create(builder, location, predicate,
                                             constant, value))
               : Value(arith::CmpIOp::create(builder, location, predicate,
                                             value, constant));
}

Value convert(OpBuilder &builder, Location location, Value value,
              const Domain &input, const Domain &result) {
  unsigned from = input.storage.getWidth(), to = result.storage.getWidth();
  if (from == to)
    return value;
  if (from > to)
    return arith::TruncIOp::create(builder, location, result.storage, value);
  return input.isUnsigned ? Value(arith::ExtUIOp::create(builder, location,
                                                         result.storage, value))
                          : Value(arith::ExtSIOp::create(
                                builder, location, result.storage, value));
}

ac::ValueBindingOp bind(OpBuilder &builder, Location location, Value value,
                        Value valid, Value path, DictionaryAttr id,
                        DictionaryAttr domain) {
  OperationState state(location, ac::ValueBindingOp::getOperationName());
  state.addOperands({value, valid, path});
  state.addAttribute("id", id);
  state.addAttribute("domain", domain);
  return cast<ac::ValueBindingOp>(builder.create(state));
}

} // namespace

LogicalResult lowerCheckedToBitsRule(ModuleOp unit, ac::RuleOp rule) {
  auto source = inspect(rule);
  if (failed(source) || !unit ||
      rule->getParentOfType<mlir::ModuleOp>() != unit)
    return failure();
  auto input = parseDomain(source->inputDomain, rule);
  auto resultDomain = parseDomain(source->resultDomain, rule);
  if (failed(input) || failed(resultDomain))
    return failure();
  Block &body = rule.getBody().front();
  OpBuilder builder(source->convert);
  Value demand = arith::AndIOp::create(builder, source->convert.getLoc(),
                                       source->path, source->valid);
  Value lower =
      bound(builder, source->convert.getLoc(), source->read.getResult(), *input,
            resultDomain->lower, true);
  Value upper =
      bound(builder, source->convert.getLoc(), source->read.getResult(), *input,
            resultDomain->upper, false);
  Value safety =
      arith::AndIOp::create(builder, source->convert.getLoc(), lower, upper);
  Value resultValid =
      arith::AndIOp::create(builder, source->convert.getLoc(), demand, safety);
  auto branch = scf::IfOp::create(builder, source->convert.getLoc(),
                                  TypeRange{resultDomain->storage}, resultValid,
                                  true, true);
  OpBuilder thenBuilder =
      OpBuilder::atBlockEnd(&branch.getThenRegion().front());
  Value converted = convert(thenBuilder, source->convert.getLoc(),
                            source->read.getResult(), *input, *resultDomain);
  scf::YieldOp::create(thenBuilder, source->convert.getLoc(), converted);
  OpBuilder elseBuilder =
      OpBuilder::atBlockEnd(&branch.getElseRegion().front());
  Value zero = arith::ConstantOp::create(
      elseBuilder, source->convert.getLoc(),
      IntegerAttr::get(resultDomain->storage,
                       APInt(resultDomain->storage.getWidth(), 0)));
  scf::YieldOp::create(elseBuilder, source->convert.getLoc(), zero);
  source->expect.getConditionMutable().assign(safety);
  source->expect.getPathMutable().assign(demand);
  builder.setInsertionPointAfter(source->expect);
  auto inputBinding =
      bind(builder, source->read.getLoc(), source->read.getResult(),
           source->valid, source->path, source->inputID, source->inputDomain);
  (void)bind(builder, source->from.getLoc(), source->read.getResult(),
             source->valid, source->path, source->fromID, source->inputDomain);
  auto resultBinding =
      bind(builder, source->convert.getLoc(), branch.getResult(0), resultValid,
           demand, source->resultID, source->resultDomain);
  OperationState state(source->convert.getLoc(),
                       ac::NumericProofOp::getOperationName());
  state.addOperands({demand, inputBinding.getValue(), source->valid,
                     branch.getResult(0), resultValid, safety, demand});
  state.addAttribute("operand_segment_sizes",
                     builder.getDenseI32ArrayAttr({1, 1, 1, 1, 1, 1, 1}));
  state.addAttribute("mode", builder.getStringAttr("exact"));
  state.addAttribute("result_domain", source->resultDomain);
  state.addAttribute("input_ids", builder.getArrayAttr({source->inputID}));
  state.addAttribute("input_domains",
                     builder.getArrayAttr({source->inputDomain}));
  state.addAttribute("result_id", source->resultID);
  state.addAttribute("obligations",
                     rule->getAttrOfType<ArrayAttr>("ac.required_numeric"));
  state.addAttribute("checks", builder.getArrayAttr({source->checkBinding}));
  state.addAttribute(
      "origin", source->convert->getAttrOfType<DictionaryAttr>("ac.origin"));
  auto proof = cast<ac::NumericProofOp>(builder.create(state));
  source->convert.erase();
  source->from.erase();
  return ac::verifyCheckedToBitsWitness(rule, resultBinding, proof);
}

} // namespace acir::compiler
