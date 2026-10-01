#include "ACIRNumericNextUse.h"

#include "ACIRFinalContracts.h"
#include "ACIRFinalRecordUses.h"
#include "ACIRFinalUses.h"
#include "ACIRNumericComposition.h"
#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::ac {
namespace {

bool trueI1(Value value) {
  auto op = value.getDefiningOp<arith::ConstantOp>();
  auto attr = op ? dyn_cast<IntegerAttr>(op.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) && attr.getValue().isOne();
}

bool integerConstant(Value value, uint64_t expected) {
  auto op = value.getDefiningOp<arith::ConstantOp>();
  auto attr = op ? dyn_cast<IntegerAttr>(op.getValue()) : IntegerAttr();
  return attr && attr.getType().isSignlessInteger() &&
         attr.getValue().getLimitedValue() == expected;
}

bool mathConstant(MathConstantOp op, StringRef expected) {
  auto value = op ? op->getAttrOfType<MathIntAttr>("value") : MathIntAttr();
  return value && value.getCanonicalValue() == expected;
}

bool isWord256(DictionaryAttr domain) {
  auto storage = domain ? domain.getAs<TypeAttr>("storage") : TypeAttr();
  auto lower = domain ? domain.getAs<MathIntAttr>("lower") : MathIntAttr();
  auto upper = domain ? domain.getAs<MathIntAttr>("upper") : MathIntAttr();
  return domain && domain.size() == 5 &&
         domain.getAs<StringAttr>("kind") ==
             StringAttr::get(domain.getContext(), "integer") &&
         storage && storage.getValue().isInteger(8) && lower && upper &&
         lower.getCanonicalValue() == "0" &&
         upper.getCanonicalValue() == "256" &&
         domain.getAs<StringAttr>("interpretation") ==
             StringAttr::get(domain.getContext(), "unsigned");
}

bool isUnsignedDomain(DictionaryAttr domain, StringRef lower, StringRef upper,
                      unsigned width) {
  auto storage = domain ? domain.getAs<TypeAttr>("storage") : TypeAttr();
  auto lowerAttr = domain ? domain.getAs<MathIntAttr>("lower") : MathIntAttr();
  auto upperAttr = domain ? domain.getAs<MathIntAttr>("upper") : MathIntAttr();
  return domain && domain.size() == 5 && storage &&
         storage.getValue().isInteger(width) && lowerAttr && upperAttr &&
         lowerAttr.getCanonicalValue() == lower &&
         upperAttr.getCanonicalValue() == upper &&
         domain.getAs<StringAttr>("interpretation") ==
             StringAttr::get(domain.getContext(), "unsigned");
}

LogicalResult proofProvenance(NumericProofOp proof, RuleOp rule,
                              DictionaryAttr expectedOrigin) {
  auto emit = [&] { return proof.emitOpError(); };
  auto file = rule->getParentOfType<mlir::ModuleOp>();
  auto module = rule->getParentOfType<ModuleOp>();
  auto fileOwner = file ? file->getAttrOfType<DictionaryAttr>("ac.source_owner")
                        : DictionaryAttr();
  auto path = fileOwner ? fileOwner.getAs<StringAttr>("path") : StringAttr();
  auto moduleOwner =
      module ? module->getAttrOfType<DictionaryAttr>("ac.source_owner")
             : DictionaryAttr();
  auto symbol = module ? module.getSymNameAttr() : StringAttr();
  auto location = dyn_cast<FileLineColLoc>(proof.getLoc());
  auto origin = proof.getOriginAttr();
  auto site = origin ? origin.getAs<DictionaryAttr>("site") : DictionaryAttr();
  auto definition =
      site ? site.getAs<FlatSymbolRefAttr>("definition") : FlatSymbolRefAttr();
  auto expansion = origin ? origin.getAs<ArrayAttr>("expansion") : ArrayAttr();
  if (!fileOwner || failed(detail::verifySourceOwner(fileOwner, emit)) ||
      moduleOwner != fileOwner || !path || !location ||
      location.getFilename() != path.getValue() || origin != expectedOrigin ||
      failed(detail::verifyOccurrence(origin, emit)) || !site || !definition ||
      !symbol || definition.getValue() != symbol.getValue() || !expansion ||
      !expansion.empty())
    return emit() << "U1 proof provenance is outside its source module";
  return success();
}

struct Packet {
  ArrayAttr required;
  DictionaryAttr ids[6];
  DictionaryAttr useID;
  DictionaryAttr state;
  DictionaryAttr checkID;
  DictionaryAttr checkBinding;
};

bool ref(Attribute raw, StringRef kind, unsigned index) {
  auto dict = dyn_cast<DictionaryAttr>(raw);
  auto ordinal = dict ? dict.getAs<IntegerAttr>("index") : IntegerAttr();
  if (!dict || dict.size() != 2 ||
      dict.getAs<StringAttr>("kind") !=
          StringAttr::get(dict.getContext(), kind) ||
      !ordinal)
    return false;
  return ordinal.getType().isInteger(32) &&
         ordinal.getValue() == APInt(32, index);
}

FailureOr<Packet> packet(RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  auto uses = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
  auto yields = rule->getAttrOfType<ArrayAttr>("ac.yield_bindings");
  auto checks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  if (!required || required.size() != 6 || !uses || uses.size() != 1 ||
      !yields || yields.size() != 1 || !checks || checks.size() != 1)
    return emit() << "U1 requires six numeric nodes and one use/yield/check";
  Packet result{required};
  static constexpr StringLiteral opcodes[] = {
      "from_bits", "constant", "add", "constant", "and_bits", "to_bits"};
  static constexpr unsigned operands[] = {1, 1, 2, 1, 2, 1};
  llvm::DenseSet<Attribute> ids;
  for (unsigned i = 0; i != 6; ++i) {
    auto node = dyn_cast<DictionaryAttr>(required[i]);
    auto target =
        node ? node.getAs<DictionaryAttr>("target") : DictionaryAttr();
    auto kind = target ? target.getAs<StringAttr>("kind") : StringAttr();
    auto refs = node ? node.getAs<ArrayAttr>("operands") : ArrayAttr();
    result.ids[i] = node ? node.getAs<DictionaryAttr>("id") : DictionaryAttr();
    bool targetOK =
        i == 5
            ? target && target.size() == 2 && kind &&
                  kind.getValue() == "integer_boundary" &&
                  isWord256(target.getAs<DictionaryAttr>("domain"))
            : target && target.size() == 1 && kind && kind.getValue() == "none";
    if (!node || node.size() != 4 ||
        node.getAs<StringAttr>("operator") !=
            StringAttr::get(rule.getContext(), opcodes[i]) ||
        !refs || refs.size() != operands[i] || !targetOK ||
        failed(detail::verifyValueID(result.ids[i], emit)) ||
        !ids.insert(result.ids[i]).second)
      return emit() << "U1 numeric recipe is not canonical";
  }
  auto refs = [&](unsigned i) {
    return cast<DictionaryAttr>(required[i]).getAs<ArrayAttr>("operands");
  };
  auto literal = [&](unsigned i, StringRef value) {
    auto operand = dyn_cast<DictionaryAttr>(refs(i)[0]);
    auto integer =
        operand ? operand.getAs<MathIntAttr>("value") : MathIntAttr();
    return operand && operand.size() == 2 &&
           operand.getAs<StringAttr>("kind") ==
               StringAttr::get(rule.getContext(), "constant") &&
           integer && integer.getCanonicalValue() == value;
  };
  if (!ref(refs(0)[0], "input", 0) || !literal(1, "1") ||
      !ref(refs(2)[0], "node", 0) || !ref(refs(2)[1], "node", 1) ||
      !literal(3, "255") || !ref(refs(4)[0], "node", 2) ||
      !ref(refs(4)[1], "node", 3) || !ref(refs(5)[0], "node", 4))
    return emit() << "U1 numeric recipe references are invalid";

  auto requiredUse = dyn_cast<DictionaryAttr>(uses[0]);
  auto target = requiredUse ? requiredUse.getAs<DictionaryAttr>("target")
                            : DictionaryAttr();
  result.useID =
      requiredUse ? requiredUse.getAs<DictionaryAttr>("id") : DictionaryAttr();
  result.state =
      target ? target.getAs<DictionaryAttr>("state") : DictionaryAttr();
  if (!requiredUse || requiredUse.size() != 3 ||
      failed(detail::verifyUseID(result.useID, emit)) ||
      requiredUse.getAs<DictionaryAttr>("value") != result.ids[5] || !target ||
      target.size() != 2 ||
      target.getAs<StringAttr>("kind") !=
          StringAttr::get(rule.getContext(), "next_scalar") ||
      failed(detail::verifyStateRef(result.state, emit)))
    return emit() << "U1 RequiredUse is not the boundary value assignment";
  auto role = result.useID.getAs<StringAttr>("role");
  auto slot = detail::decodeU32(result.useID.getAs<IntegerAttr>("slot"),
                                "U1 next UseID slot", emit);
  if (!role || role.getValue() != "next" || failed(slot) || *slot != 0)
    return emit() << "U1 requires next UseID slot zero";

  auto binding = dyn_cast<DictionaryAttr>(yields[0]);
  auto data =
      binding ? binding.getAs<IntegerAttr>("data_operand") : IntegerAttr();
  auto enable =
      binding ? binding.getAs<IntegerAttr>("enable_operand") : IntegerAttr();
  auto contributions =
      binding ? binding.getAs<ArrayAttr>("contributions") : ArrayAttr();
  auto contribution = contributions && contributions.size() == 1
                          ? dyn_cast<DictionaryAttr>(contributions[0])
                          : DictionaryAttr();
  auto dataIndex = detail::decodeU32(data, "U1 yield data operand", emit);
  auto enableIndex = detail::decodeU32(enable, "U1 yield enable operand", emit);
  if (!binding || binding.size() != 4 || failed(dataIndex) || *dataIndex != 0 ||
      failed(enableIndex) || *enableIndex != 1 ||
      binding.getAs<DictionaryAttr>("target") != result.state ||
      !contribution || contribution.size() != 2 ||
      contribution.getAs<DictionaryAttr>("use") != result.useID ||
      !contribution.getAs<UnitAttr>("selection_ordinal"))
    return emit() << "U1 YieldBinding is not the exact scalar contribution";
  auto requiredCheck = dyn_cast<DictionaryAttr>(checks[0]);
  result.checkID = requiredCheck ? requiredCheck.getAs<DictionaryAttr>("id")
                                 : DictionaryAttr();
  auto checkObligation = result.checkID
                             ? result.checkID.getAs<IntegerAttr>("obligation")
                             : IntegerAttr();
  auto decodedCheck =
      detail::decodeU64(checkObligation, "U1 check obligation", emit);
  if (!requiredCheck || requiredCheck.size() != 3 ||
      requiredCheck.getAs<StringAttr>("kind") !=
          StringAttr::get(rule.getContext(), "range") ||
      failed(detail::verifyCheckID(result.checkID, emit)) ||
      result.checkID.getAs<DictionaryAttr>("registration") !=
          rule.getRegistrationAttr() ||
      result.checkID.getAs<DictionaryAttr>("check") !=
          result.useID.getAs<DictionaryAttr>("origin") ||
      failed(decodedCheck) || *decodedCheck != 0)
    return emit() << "U1 range check is not the assignment obligation";
  result.checkBinding =
      Builder(rule.getContext())
          .getDictionaryAttr({
              Builder(rule.getContext()).getNamedAttr("id", result.checkID),
              Builder(rule.getContext()).getNamedAttr("owner", result.ids[5]),
              Builder(rule.getContext())
                  .getNamedAttr(
                      "kind",
                      Builder(rule.getContext()).getStringAttr("range")),
              Builder(rule.getContext())
                  .getNamedAttr(
                      "operand_ordinal",
                      Builder(rule.getContext()).getI32IntegerAttr(0)),
          });
  return result;
}

LogicalResult common(RuleOp rule, const Packet &p) {
  auto emit = [&] { return rule.emitOpError(); };
  if (rule.getInputs().size() != 1 || rule.getTargets().size() != 1 ||
      rule.getBody().getBlocks().size() != 1)
    return emit() << "U1 requires one current and one scalar next target";
  auto inputBindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto outputBindings = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto outputTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  if (!inputBindings || inputBindings.size() != 1 || !outputBindings ||
      outputBindings.size() != 1 || inputBindings[0] != p.state ||
      outputBindings[0] != p.state || !inputTypes || inputTypes.size() != 1 ||
      !outputTypes || outputTypes.size() != 1 ||
      inputTypes[0] != outputTypes[0] ||
      !isWord256(dyn_cast<DictionaryAttr>(inputTypes[0])))
    return emit() << "U1 current/next state and Word domain must be identical";
  auto scope = rule->getAttrOfType<DictionaryAttr>("ac.proof_scope");
  if (!scope || failed(detail::verifyProofScope(scope, emit)) ||
      scope.get("registration") != rule.getRegistrationAttr())
    return emit() << "U1 proof scope is not canonical";
  auto module = rule->getParentOfType<ModuleOp>();
  auto specialization = scope.getAs<DictionaryAttr>("specialization");
  auto definition = specialization
                        ? specialization.getAs<FlatSymbolRefAttr>("definition")
                        : FlatSymbolRefAttr();
  auto arguments = specialization ? specialization.getAs<ArrayAttr>("arguments")
                                  : ArrayAttr();
  if (!module || !definition || definition.getValue() != module.getSymName() ||
      !arguments || !arguments.empty())
    return emit() << "U1 proof scope must name the enclosing empty SpecKey";
  auto readOrigin = [&]() -> DictionaryAttr {
    auto reads = rule.getBody().front().getOps<SourceReadOp>();
    if (!llvm::hasSingleElement(reads))
      return {};
    SourceReadOp read = *reads.begin();
    return read->getAttrOfType<DictionaryAttr>("ac.origin");
  }();
  auto slot = [&](DictionaryAttr id, uint32_t expected) {
    auto decoded = detail::decodeU32(id.getAs<IntegerAttr>("slot"),
                                     "U1 ValueID slot", emit);
    return succeeded(decoded) && *decoded == expected;
  };
  bool final = isFinalImplementation(rule);
  if ((!final && (!readOrigin ||
                  p.ids[0].getAs<DictionaryAttr>("origin") != readOrigin)) ||
      !slot(p.ids[0], 1) ||
      p.ids[4].getAs<DictionaryAttr>("origin") !=
          p.ids[5].getAs<DictionaryAttr>("origin") ||
      !slot(p.ids[4], 0) || !slot(p.ids[5], 1))
    return emit() << "U1 read/mask/boundary identities are not canonical";
  return success();
}

LogicalResult verifySource(RuleOp rule, const Packet &p) {
  auto emit = [&] { return rule.emitOpError(); };
  Block &body = rule.getBody().front();
  SmallVector<SourceReadOp> reads(body.getOps<SourceReadOp>());
  SmallVector<MathFromBitsOp> from(body.getOps<MathFromBitsOp>());
  SmallVector<MathConstantOp> constants(body.getOps<MathConstantOp>());
  SmallVector<MathBinaryOp> binaries(body.getOps<MathBinaryOp>());
  SmallVector<MathToBitsOp> boundaries(body.getOps<MathToBitsOp>());
  SmallVector<SourceUseOp> uses(body.getOps<SourceUseOp>());
  SmallVector<SourceExpectOp> expects(body.getOps<SourceExpectOp>());
  if (reads.size() != 1 || from.size() != 1 || constants.size() != 2 ||
      binaries.size() != 2 || boundaries.size() != 1 || uses.size() != 1 ||
      expects.size() != 1 || !body.getOps<ValueBindingOp>().empty() ||
      !body.getOps<NumericProofOp>().empty() ||
      !body.getOps<ValueUseOp>().empty())
    return emit() << "U1 source inventory is not closed";
  auto add = binaries[0], mask = binaries[1];
  auto addOpcode = add->getAttrOfType<StringAttr>("operator");
  auto maskOpcode = mask->getAttrOfType<StringAttr>("operator");
  auto logical = cast<ArrayAttr>(rule->getAttr("ac.input_types"))[0];
  if (from[0].getValue() != reads[0].getResult() ||
      from[0].getDomainAttr() != logical || !mathConstant(constants[0], "1") ||
      !mathConstant(constants[1], "255") || !addOpcode ||
      addOpcode.getValue() != "add" || !maskOpcode ||
      maskOpcode.getValue() != "and_bits" ||
      add.getLhs() != from[0].getResult() ||
      add.getRhs() != constants[0].getResult() ||
      mask.getLhs() != add.getResult() ||
      mask.getRhs() != constants[1].getResult() ||
      boundaries[0].getValue() != mask.getResult() ||
      boundaries[0].getValueValid() != mask.getValid() ||
      !isWord256(boundaries[0].getDomainAttr()) ||
      uses[0].getValue() != boundaries[0].getResult() ||
      uses[0].getValid() != boundaries[0].getValid() ||
      !trueI1(uses[0].getPath()) || uses[0].getIdAttr() != p.useID ||
      uses[0].getSourceAttr() != p.ids[5] ||
      uses[0].getTargetAttr().getAs<DictionaryAttr>("state") != p.state)
    return emit() << "U1 source SSA/identity chain is invalid";
  if (!trueI1(add.getPath()) || !trueI1(add.getLhsValid()) ||
      !trueI1(add.getRhsValid()) || !trueI1(mask.getPath()) ||
      mask.getLhsValid() != add.getValid() || !trueI1(mask.getRhsValid()))
    return emit() << "U1 source arithmetic validity chain is invalid";
  if (!trueI1(boundaries[0].getPath()) ||
      expects[0].getCondition() != boundaries[0].getValid() ||
      expects[0].getPath() != boundaries[0].getPath() ||
      expects[0]->getAttrOfType<DictionaryAttr>("ac.check_id") != p.checkID)
    return emit() << "U1 boundary check is not attached to its conversion";
  auto yield = dyn_cast<YieldOp>(body.getTerminator());
  if (!yield || yield.getValues().size() != 2 ||
      yield.getValues()[0] != uses[0].getData() ||
      yield.getValues()[1] != uses[0].getEnabled())
    return emit() << "U1 source yield must be the original SourceUse";
  DictionaryAttr producerOrigins[] = {
      from[0]->getAttrOfType<DictionaryAttr>("ac.origin"),
      constants[0]->getAttrOfType<DictionaryAttr>("ac.origin"),
      add->getAttrOfType<DictionaryAttr>("ac.origin"),
      constants[1]->getAttrOfType<DictionaryAttr>("ac.origin"),
      mask->getAttrOfType<DictionaryAttr>("ac.origin"),
      mask->getAttrOfType<DictionaryAttr>("ac.origin")};
  unsigned slots[] = {1, 0, 0, 0, 0, 1};
  for (unsigned i = 0; i != 6; ++i) {
    auto slot = detail::decodeU32(p.ids[i].getAs<IntegerAttr>("slot"),
                                  "U1 ValueID slot", emit);
    if (p.ids[i].getAs<DictionaryAttr>("origin") != producerOrigins[i] ||
        failed(slot) || *slot != slots[i])
      return emit() << "U1 ValueID does not match its source producer";
  }
  for (Operation &operation : body) {
    if (isa<SourceReadOp, MathFromBitsOp, MathConstantOp, MathBinaryOp,
            MathToBitsOp, SourceUseOp, SourceExpectOp, YieldOp>(operation))
      continue;
    if (auto constant = dyn_cast<arith::ConstantOp>(operation)) {
      if (!trueI1(constant.getResult()) || constant.getResult().use_empty())
        return emit() << "U1 source control constant is invalid or unused";
      continue;
    }
    return emit() << "U1 source rule contains an unsupported extra operation";
  }
  return success();
}

ValueBindingOp findBinding(Block &body, DictionaryAttr id) {
  ValueBindingOp found;
  for (auto binding : body.getOps<ValueBindingOp>())
    if (binding.getIdAttr() == id) {
      if (found)
        return {};
      found = binding;
    }
  return found;
}

bool unsignedFit(Value actual, Value source) {
  if (actual == source)
    return true;
  auto extension = actual.getDefiningOp<arith::ExtUIOp>();
  return extension && extension.getIn() == source;
}

LogicalResult verifyLowered(RuleOp rule, const Packet &p) {
  auto emit = [&] { return rule.emitOpError(); };
  Block &body = rule.getBody().front();
  SmallVector<SourceReadOp> reads(body.getOps<SourceReadOp>());
  SmallVector<ValueBindingOp> bindings(body.getOps<ValueBindingOp>());
  SmallVector<NumericProofOp> proofs(body.getOps<NumericProofOp>());
  SmallVector<ValueUseOp> uses(body.getOps<ValueUseOp>());
  SmallVector<SourceExpectOp> expects(body.getOps<SourceExpectOp>());
  bool final = isFinalImplementation(rule);
  if (reads.size() != (final ? 0u : 1u) || bindings.size() != 6 ||
      proofs.size() != 2 || uses.size() != 1 || expects.size() != 1 ||
      !body.getOps<SourceUseOp>().empty() ||
      !body.getOps<MathFromBitsOp>().empty() ||
      !body.getOps<MathConstantOp>().empty() ||
      !body.getOps<MathBinaryOp>().empty() ||
      !body.getOps<MathToBitsOp>().empty())
    return emit() << "U1 lowered inventory is not closed";
  Builder builder(rule.getContext());
  DictionaryAttr inputID;
  Value inputValue;
  if (final) {
    inputID = bindings[0].getIdAttr();
    inputValue = body.getArgument(0);
    auto inputSlot = detail::decodeU32(
        inputID ? inputID.getAs<IntegerAttr>("slot") : IntegerAttr(),
        "U1 final input ValueID slot", emit);
    if (!inputID || failed(detail::verifyValueID(inputID, emit)) ||
        failed(inputSlot) || *inputSlot != 0 ||
        inputID.getAs<DictionaryAttr>("origin") !=
            p.ids[0].getAs<DictionaryAttr>("origin"))
      return emit() << "U1 final I/F identities must share origin at slots 0/1";
  } else {
    inputID = builder.getDictionaryAttr({
        builder.getNamedAttr(
            "origin", reads[0]->getAttrOfType<DictionaryAttr>("ac.origin")),
        builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
    });
    inputValue = reads[0].getResult();
  }
  DictionaryAttr ids[] = {inputID,  p.ids[0], p.ids[1],
                          p.ids[3], p.ids[4], p.ids[5]};
  ValueBindingOp actual[6];
  for (unsigned i = 0; i != 6; ++i) {
    actual[i] = findBinding(body, ids[i]);
    if (!actual[i] || actual[i] != bindings[i])
      return emit() << "U1 lowered bindings are missing or reordered";
  }
  if (actual[0].getValue() != inputValue ||
      actual[1].getValue() != inputValue ||
      !isWord256(actual[0].getDomainAttr()) ||
      actual[1].getDomainAttr() != actual[0].getDomainAttr() ||
      !isUnsignedDomain(actual[2].getDomainAttr(), "1", "2", 1) ||
      !isUnsignedDomain(actual[3].getDomainAttr(), "255", "256", 8) ||
      !isWord256(actual[4].getDomainAttr()) ||
      actual[5].getDomainAttr() != actual[4].getDomainAttr())
    return emit()
           << "U1 lowered binding domains or input authority are invalid";
  for (ValueBindingOp binding : actual)
    if (!trueI1(binding.getValid()) || !trueI1(binding.getPath()))
      return emit() << "U1 lowered binding controls must be literal true";
  auto add = actual[4].getValue().getDefiningOp<arith::AndIOp>();
  auto arithmetic =
      add ? add.getLhs().getDefiningOp<arith::AddIOp>() : arith::AddIOp();
  if (!arithmetic && add)
    arithmetic = add.getRhs().getDefiningOp<arith::AddIOp>();
  Value maskValue = add && arithmetic && add.getLhs() == arithmetic.getResult()
                        ? add.getRhs()
                    : add ? add.getLhs()
                          : Value();
  if (!arithmetic || arithmetic.getLhs() != inputValue ||
      !unsignedFit(arithmetic.getRhs(), actual[2].getValue()) ||
      arithmetic.getOverflowFlags() != arith::IntegerOverflowFlags::none)
    return emit() << "U1 finite add SSA is invalid";
  if (!integerConstant(actual[2].getValue(), 1))
    return emit() << "U1 finite one constant is invalid";
  if (!integerConstant(actual[3].getValue(), 255))
    return emit() << "U1 finite mask constant is invalid";
  if (maskValue != actual[3].getValue() || !add->getAttrs().empty())
    return emit() << "U1 finite and mask operand is invalid";
  if (actual[5].getValue() != add.getResult() ||
      actual[5].getValid() != actual[4].getValid() ||
      actual[5].getPath() != actual[4].getPath())
    return emit() << "U1 finite boundary binding is invalid";
  auto low = proofs[0], boundary = proofs[1];
  if (failed(proofProvenance(low, rule,
                             p.ids[4].getAs<DictionaryAttr>("origin"))) ||
      failed(proofProvenance(boundary, rule,
                             p.ids[5].getAs<DictionaryAttr>("origin"))))
    return failure();
  auto firstFive = builder.getArrayAttr(p.required.getValue().take_front(5));
  auto finalOnly = builder.getArrayAttr({p.required[5]});
  auto width = low->getAttrOfType<IntegerAttr>("width");
  auto decodedWidth = detail::decodeU32(width, "U1 low_bits width", emit);
  SmallVector<int32_t> lowSegments{1, 1, 1, 1, 1, 0, 0};
  SmallVector<int32_t> boundarySegments{1, 1, 1, 1, 1, 1, 1};
  if (low->getAttrs().size() != 10 || boundary->getAttrs().size() != 9 ||
      low.getOperandSegmentSizes() != ArrayRef<int32_t>(lowSegments) ||
      boundary.getOperandSegmentSizes() !=
          ArrayRef<int32_t>(boundarySegments) ||
      low.getMode() != "low_bits" || failed(decodedWidth) ||
      *decodedWidth != 8 || low.getObligationsAttr() != firstFive ||
      low.getResultIdAttr() != p.ids[4] ||
      low.getInputIdsAttr() != builder.getArrayAttr({inputID}) ||
      low.getInputDomainsAttr() !=
          builder.getArrayAttr({actual[0].getDomainAttr()}) ||
      low.getResultDomainAttr() != actual[4].getDomainAttr() ||
      low.getChecksAttr() != builder.getArrayAttr({}) ||
      low.getOriginAttr() != p.ids[4].getAs<DictionaryAttr>("origin") ||
      boundary.getMode() != "exact" || boundary->hasAttr("width") ||
      boundary.getObligationsAttr() != finalOnly ||
      boundary.getInputIdsAttr() != builder.getArrayAttr({p.ids[4]}) ||
      boundary.getInputDomainsAttr() !=
          builder.getArrayAttr({actual[4].getDomainAttr()}) ||
      boundary.getResultIdAttr() != p.ids[5] ||
      boundary.getChecksAttr() != builder.getArrayAttr({p.checkBinding}) ||
      boundary.getResultDomainAttr() != actual[5].getDomainAttr() ||
      boundary.getOriginAttr() != p.ids[5].getAs<DictionaryAttr>("origin"))
    return emit() << "U1 requires exact low_bits and boundary witnesses";
  if (low->getNumOperands() != 5 || low->getOperand(0) != actual[0].getPath() ||
      low->getOperand(1) != actual[0].getValue() ||
      low->getOperand(2) != actual[0].getValid() ||
      low->getOperand(3) != actual[4].getValue() ||
      low->getOperand(4) != actual[4].getValid() ||
      boundary->getNumOperands() != 7 ||
      boundary->getOperand(0) != actual[4].getPath() ||
      boundary->getOperand(1) != actual[4].getValue() ||
      boundary->getOperand(2) != actual[4].getValid() ||
      boundary->getOperand(3) != actual[5].getValue() ||
      boundary->getOperand(4) != actual[5].getValid() ||
      boundary->getOperand(5) != expects[0].getCondition() ||
      boundary->getOperand(6) != expects[0].getPath() ||
      !trueI1(boundary->getOperand(5)))
    return emit() << "U1 proof operands do not bind actual finite SSA";
  if (uses[0].getIdAttr() != p.useID || uses[0].getSourceAttr() != p.ids[5] ||
      uses[0].getValue() != actual[5].getValue() ||
      uses[0].getValid() != actual[5].getValid() || !trueI1(uses[0].getPath()))
    return emit() << "U1 ValueUse does not bind the required boundary value";
  auto yield = dyn_cast<YieldOp>(body.getTerminator());
  auto enable = yield && yield.getValues().size() == 2
                    ? yield.getValues()[1].getDefiningOp<arith::AndIOp>()
                    : arith::AndIOp();
  if (!yield || yield.getValues()[0] != actual[5].getValue() || !enable ||
      enable.getLhs() != uses[0].getValid() ||
      enable.getRhs() != uses[0].getPath() || !enable->getAttrs().empty())
    return emit() << "U1 yield is not the exact ValueUse data/enable binding";
  unsigned adds = 0, ands = 0, extensions = 0, constants = 0;
  for (Operation &operation : body) {
    if (isa<arith::AddIOp>(operation))
      ++adds;
    else if (isa<arith::AndIOp>(operation))
      ++ands;
    else if (isa<arith::ExtUIOp>(operation))
      ++extensions;
    else if (auto constant = dyn_cast<arith::ConstantOp>(operation)) {
      ++constants;
      if (constant->getAttrs().size() != 1 || constant.getResult().use_empty())
        return emit() << "U1 lowered constant is malformed or unused";
    } else if (isa<SourceReadOp, ValueBindingOp, NumericProofOp, ValueUseOp,
                   SourceExpectOp, YieldOp>(operation))
      continue;
    else
      return emit()
             << "U1 lowered rule contains an unsupported extra operation";
  }
  if (adds != 1 || ands != 2 || extensions != 1 || constants != 4)
    return emit() << "U1 lowered finite operation inventory is not exact";
  return success();
}

} // namespace

bool hasNumericNextUseContract(RuleOp rule) {
  auto required = rule ? rule->getAttrOfType<ArrayAttr>("ac.required_numeric")
                       : ArrayAttr();
  return rule && required && required.size() == 6 &&
         rule.getInputs().size() == 1 && rule.getTargets().size() == 1 &&
         rule->hasAttr("ac.required_uses") &&
         rule->hasAttr("ac.yield_bindings");
}

LogicalResult verifyNumericNextUseClosure(RuleOp rule) {
  auto parsed = packet(rule);
  if (failed(parsed) || failed(common(rule, *parsed)))
    return failure();
  Block &body = rule.getBody().front();
  bool source = !body.getOps<MathToBitsOp>().empty() ||
                !body.getOps<SourceUseOp>().empty();
  return source ? verifySource(rule, *parsed) : verifyLowered(rule, *parsed);
}

LogicalResult ValueUseOp::verify() {
  auto rule = (*this)->getParentOfType<RuleOp>();
  if (!rule || (*this)->getBlock() != &rule.getBody().front() ||
      (*this)->getAttrs().size() != 2 ||
      failed(detail::verifyUseID(getIdAttr(), [&] { return emitOpError(); })) ||
      failed(detail::verifyValueID(getSourceAttr(),
                                   [&] { return emitOpError(); })))
    return emitOpError() << "ValueUse requires exact rule-scoped identity";
  if (hasFinalRecordUses(rule))
    return verifyFinalRecordUses(rule);
  if (isa<StructType>(getValue().getType()))
    return emitOpError()
           << "record ValueUse requires ac.required_records final closure";
  auto scalarType = dyn_cast<IntegerType>(getValue().getType());
  if (!scalarType || !scalarType.isSignless())
    return emitOpError()
           << "ValueUse value must be signless integer or verified record";
  if (hasGenericFinalUses(rule))
    return verifyGenericFinalUses(rule);
  if (hasNumericCompositionContract(rule))
    return success();
  return verifyNumericNextUseClosure(rule);
}

} // namespace acir::ac
