#include "ACIRFinalContracts.h"
#include "ACIRFinalUses.h"
#include "ACIRNumericComposition.h"
#include "ACIRNumericExactInputAdd.h"
#include "ACIRNumericExactScalar.h"
#include "ACIRNumericMask.h"
#include "ACIRNumericNextUse.h"
#include "ACIRNumericProof.h"
#include "ACIRNumericRange.h"
#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Location.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSet.h"
#include <algorithm>
#include <optional>
using namespace mlir;
namespace acir::ac {
namespace {
LogicalResult verifyProofOwner(Operation *operation, RuleOp &rule) {
  rule = operation->getParentOfType<RuleOp>();
  if (!rule || rule.getBody().getBlocks().size() != 1 ||
      operation->getBlock() != &rule.getBody().front())
    return operation->emitOpError()
           << "numeric evidence must be in one rule's top computation block";
  auto file = operation->getParentOfType<mlir::ModuleOp>();
  auto stage =
      file ? file->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  if (!file || !stage ||
      (stage.getValue() != "source" && stage.getValue() != "linked" &&
       stage.getValue() != "final"))
    return operation->emitOpError() << "numeric evidence requires source, "
                                       "linked or final semantic phase";
  if (stage.getValue() == "final") {
    auto kind = file->getAttrOfType<StringAttr>("ac.unit_kind");
    auto owner = file->getAttrOfType<DictionaryAttr>("ac.source_owner");
    if (!kind || kind.getValue() != "implementation" ||
        failed(detail::verifySourceOwner(
            owner, [&] { return operation->emitOpError(); })))
      return operation->emitOpError()
             << "final numeric evidence requires retained implementation "
                "metadata";
  }
  auto scope = rule->getAttrOfType<DictionaryAttr>("ac.proof_scope");
  if (failed(detail::verifyProofScope(
          scope, [&] { return operation->emitOpError(); })) ||
      scope.get("registration") != rule.getRegistrationAttr())
    return operation->emitOpError()
           << "rule proof_scope must match its registered occurrence";
  auto specialization = scope.getAs<DictionaryAttr>("specialization");
  auto definition = specialization
                        ? specialization.getAs<FlatSymbolRefAttr>("definition")
                        : FlatSymbolRefAttr();
  auto arguments = specialization ? specialization.getAs<ArrayAttr>("arguments")
                                  : ArrayAttr();
  auto module = rule->getParentOfType<ModuleOp>();
  auto symbol =
      module ? module->getAttrOfType<StringAttr>("sym_name") : StringAttr();
  if (!module || !symbol || !definition || !arguments || !arguments.empty() ||
      definition.getValue() != symbol.getValue())
    return operation->emitOpError()
           << "B0 proof_scope requires the enclosing module's empty SpecKey";
  return success();
}
LogicalResult verifyB0EvidenceOrigin(Operation *operation,
                                     DictionaryAttr occurrence) {
  auto emit = [&] { return operation->emitOpError(); };
  auto file = operation->getParentOfType<mlir::ModuleOp>();
  auto sourceOwner =
      file ? file->getAttrOfType<DictionaryAttr>("ac.source_owner")
           : DictionaryAttr();
  auto ownerPath =
      sourceOwner ? sourceOwner.getAs<StringAttr>("path") : StringAttr();
  auto hardware = operation->getParentOfType<ModuleOp>();
  auto symbol =
      hardware ? hardware->getAttrOfType<StringAttr>("sym_name") : StringAttr();
  auto location = dyn_cast<FileLineColLoc>(operation->getLoc());
  auto site =
      occurrence ? occurrence.getAs<DictionaryAttr>("site") : DictionaryAttr();
  auto definition =
      site ? site.getAs<FlatSymbolRefAttr>("definition") : FlatSymbolRefAttr();
  auto expansion =
      occurrence ? occurrence.getAs<ArrayAttr>("expansion") : ArrayAttr();
  if (!sourceOwner || failed(detail::verifySourceOwner(sourceOwner, emit)) ||
      !ownerPath || !location || location.getFilename() != ownerPath.getValue())
    return emit() << "B0 evidence requires a FileLineColLoc matching its "
                     "source owner path";
  if (!symbol || !site || !definition ||
      definition.getValue() != symbol.getValue() || !expansion ||
      !expansion.empty())
    return emit() << "B0 evidence origin must be a direct definition Site with "
                     "empty expansion";
  return success();
}
FailureOr<StringRef> finalOpcode(NumericProofOp proof) {
  auto obligations = proof.getObligationsAttr();
  auto finalNode =
      obligations && !obligations.empty()
          ? dyn_cast<DictionaryAttr>(obligations[obligations.size() - 1])
          : DictionaryAttr();
  auto opcode =
      finalNode ? finalNode.getAs<StringAttr>("operator") : StringAttr();
  if (!finalNode || finalNode.size() != 4 || !opcode)
    return proof.emitOpError()
           << "exact input witness has no closed final NumericNode";
  return opcode.getValue();
}
bool isCompareOpcode(StringRef opcode) {
  return llvm::StringSwitch<bool>(opcode)
      .Cases({"eq", "ne", "lt", "le", "gt", "ge"}, true)
      .Default(false);
}
LogicalResult verifyBindingDomain(DictionaryAttr domain, Type valueType,
                                  ValueBindingOp binding) {
  Operation *operation = binding;
  auto emit = [&] { return operation->emitOpError(); };
  auto kind = domain.getAs<StringAttr>("kind");
  if (!kind)
    return emit() << "ValueBinding domain has no kind";
  if (kind.getValue() == "bool") {
    if (domain.size() != 1 || !valueType.isInteger(1))
      return emit() << "exact compare result binding requires bool i1";
    RuleOp rule = binding->getParentOfType<RuleOp>();
    SmallVector<NumericProofOp> proofs(
        rule.getBody().front().getOps<NumericProofOp>());
    if (proofs.size() != 1 ||
        proofs.front().getResultIdAttr() != binding.getIdAttr())
      return emit() << "bool binding must be the unique proof result";
    auto opcode = finalOpcode(proofs.front());
    if (failed(opcode) || !isCompareOpcode(*opcode) ||
        failed(verifyExactInputConstantCompareWitness(rule, binding,
                                                      proofs.front())))
      return emit() << "bool binding requires an exact comparison witness";
    return success();
  }
  if (failed(detail::verifyLogicalTypeStructure(domain, emit)))
    return failure();
  if (kind.getValue() != "integer" || domain.size() != 5)
    return emit() << "constant-only numeric witness supports integer domains";
  auto storage = domain.getAs<TypeAttr>("storage");
  auto interpretation = domain.getAs<StringAttr>("interpretation");
  auto integer =
      storage ? dyn_cast<IntegerType>(storage.getValue()) : IntegerType();
  if (!storage || !interpretation ||
      (interpretation.getValue() != "signed" &&
       interpretation.getValue() != "unsigned") ||
      !integer || !integer.isSignless() || integer.getWidth() == 0 ||
      integer.getWidth() > 64 || storage.getValue() != valueType)
    return emit() << "ValueBinding integer domain must exactly match signless "
                     "i1..i64 storage and interpretation";
  return success();
}
FailureOr<ValueBindingOp> findBinding(RuleOp rule, DictionaryAttr id,
                                      ac::detail::EmitError emitError) {
  std::optional<ValueBindingOp> found;
  for (ValueBindingOp binding : rule.getBody().front().getOps<ValueBindingOp>())
    if (binding.getIdAttr() == id) {
      if (found)
        return emitError() << "ValueID has duplicate ValueBinding operations";
      found = binding;
    }
  if (!found)
    return emitError() << "NumericProof ValueID has no scoped ValueBinding";
  return *found;
}
std::string canonical(APSInt value) {
  SmallString<64> text;
  value.toString(text);
  return text.str().str();
}
bool isTrueI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(1) &&
         integer.getValue().isOne();
}
Operation *i1ConstantOperation(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(1) ? constant.getOperation()
                                                   : nullptr;
}
LogicalResult verifySegments(NumericProofOp proof) {
  auto fail = [&] { return proof.emitOpError(); };
  auto segments = proof.getOperandSegmentSizesAttr();
  auto inputIDs = proof.getInputIdsAttr();
  auto inputDomains = proof.getInputDomainsAttr();
  if (!segments || !inputIDs || !inputDomains)
    return fail() << "exact proof requires segments and input profile arrays";
  ArrayRef<int32_t> sizes = segments.asArrayRef();
  const bool b0 = inputIDs.empty() && inputDomains.empty() &&
                  sizes == ArrayRef<int32_t>({1, 0, 0, 1, 1, 0, 0}) &&
                  proof->getNumOperands() == 3;
  const bool b1b = inputIDs.size() == 1 && inputDomains.size() == 1 &&
                   sizes == ArrayRef<int32_t>({1, 1, 1, 1, 1, 0, 0}) &&
                   proof->getNumOperands() == 5;
  const bool checked = inputIDs.size() == 1 && inputDomains.size() == 1 &&
                       sizes == ArrayRef<int32_t>({1, 1, 1, 1, 1, 1, 1}) &&
                       proof->getNumOperands() == 7;
  if (!b0 && !b1b && !checked)
    return fail() << "exact proof input arrays, segments and operand count "
                     "must match B0 or N0-B1b profile";
  return success();
}
LogicalResult verifyConstantNode(NumericProofOp proof, DictionaryAttr resultID,
                                 DictionaryAttr resultDomain,
                                 Value actualResult, Value actualValid,
                                 Value path, ValueBindingOp binding) {
  auto emit = [&] { return proof.emitOpError(); };
  auto obligations = proof.getObligationsAttr();
  if (!obligations || obligations.size() != 1)
    return emit() << "exact constant proof requires one NumericNode";
  auto node = dyn_cast<DictionaryAttr>(obligations[0]);
  auto id = node ? node.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto operation = node ? node.getAs<StringAttr>("operator") : StringAttr();
  auto operands = node ? node.getAs<ArrayAttr>("operands") : ArrayAttr();
  auto target = node ? node.getAs<DictionaryAttr>("target") : DictionaryAttr();
  if (!node || node.size() != 4 || !id || !operation ||
      operation.getValue() != "constant" || !operands || operands.size() != 1 ||
      !target || target.size() != 1 ||
      target.getAs<StringAttr>("kind") !=
          StringAttr::get(proof.getContext(), "none") ||
      id != resultID || failed(detail::verifyValueID(id, emit)) ||
      id != proof.getResultIdAttr())
    return emit() << "exact B0 witness requires one final constant node";
  auto constantRef = dyn_cast<DictionaryAttr>(operands[0]);
  auto refKind =
      constantRef ? constantRef.getAs<StringAttr>("kind") : StringAttr();
  auto constant =
      constantRef ? constantRef.getAs<MathIntAttr>("value") : MathIntAttr();
  if (!constantRef || constantRef.size() != 2 || !refKind ||
      refKind.getValue() != "constant" || !constant)
    return emit() << "constant NumericRef requires one MathInt value";
  auto domainKind = resultDomain.getAs<StringAttr>("kind");
  auto storage = resultDomain.getAs<TypeAttr>("storage");
  auto lower = resultDomain.getAs<MathIntAttr>("lower");
  auto upper = resultDomain.getAs<MathIntAttr>("upper");
  auto interpretation = resultDomain.getAs<StringAttr>("interpretation");
  auto integer =
      storage ? dyn_cast<IntegerType>(storage.getValue()) : IntegerType();
  if (failed(detail::verifyLogicalTypeStructure(
          resultDomain, [&] { return proof.emitOpError(); })) ||
      !domainKind || domainKind.getValue() != "integer" ||
      resultDomain.size() != 5 || !storage || !integer ||
      !integer.isSignless() || !lower || !upper || !interpretation ||
      (interpretation.getValue() != "signed" &&
       interpretation.getValue() != "unsigned"))
    return emit() << "constant result_domain must be a closed integer domain";
  APSInt value(constant.getCanonicalValue());
  APSInt lowerValue(lower.getCanonicalValue());
  APSInt upperValue(upper.getCanonicalValue());
  if (canonical(value) != canonical(lowerValue))
    return emit() << "constant result_domain lower bound must equal its value";
  APSInt expectedUpper = value.extend(value.getBitWidth() + 1);
  ++expectedUpper;
  if (canonical(expectedUpper) != canonical(upperValue))
    return emit() << "constant result_domain must be singleton [c,c+1)";
  const bool isUnsigned = interpretation.getValue() == "unsigned";
  if ((isUnsigned && value.isNegative()) ||
      (!isUnsigned && !value.isNegative()))
    return emit()
           << "constant value sign must match its integer interpretation";
  unsigned minimumWidth = std::max(1u, isUnsigned ? value.getActiveBits()
                                                  : value.getSignificantBits());
  if (integer.getWidth() != minimumWidth || integer.getWidth() > 64)
    return emit() << "constant result_domain storage must use minimum signless "
                     "i1..i64 width";
  auto boundConstant = binding.getValue().getDefiningOp<arith::ConstantOp>();
  auto boundInteger = boundConstant
                          ? dyn_cast<IntegerAttr>(boundConstant.getValue())
                          : IntegerAttr();
  auto pathConstant = i1ConstantOperation(path);
  auto validConstant = i1ConstantOperation(actualValid);
  if (!boundConstant || !pathConstant || !validConstant ||
      boundConstant->getBlock() != proof->getBlock() ||
      pathConstant->getBlock() != proof->getBlock() ||
      validConstant->getBlock() != proof->getBlock() ||
      !boundConstant->isBeforeInBlock(binding) ||
      !pathConstant->isBeforeInBlock(binding) ||
      !validConstant->isBeforeInBlock(binding) ||
      !binding->isBeforeInBlock(proof))
    return emit() << "B0 constants, binding and proof must be ordered in "
                     "the same rule block";
  RuleOp rule = proof->getParentOfType<RuleOp>();
  llvm::SmallPtrSet<Operation *, 3> allowedConstants;
  allowedConstants.insert(boundConstant.getOperation());
  allowedConstants.insert(pathConstant);
  allowedConstants.insert(validConstant);
  auto requiredChecks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  auto requiredObservations =
      rule->getAttrOfType<ArrayAttr>("ac.required_observations");
  if ((requiredChecks && !requiredChecks.empty()) ||
      (requiredObservations && !requiredObservations.empty()))
    return emit() << "B0 constant proof-bearing rule cannot own checks or "
                     "observations";
  for (Operation &nested : proof->getBlock()->getOperations()) {
    if (isa<arith::ConstantOp>(nested)) {
      if (!allowedConstants.contains(&nested))
        return emit() << "B0 rule has an unowned arithmetic constant";
      continue;
    }
    if (isa<ValueBindingOp, NumericProofOp, YieldOp>(nested))
      continue;
    return emit() << "B0 proof-bearing rule contains an unsupported operation";
  }
  if (!boundInteger || boundInteger.getType() != storage.getValue() ||
      actualResult != binding.getValue() || actualValid != binding.getValid() ||
      path != binding.getPath() || binding.getIdAttr() != resultID ||
      binding.getDomainAttr() != resultDomain)
    return emit() << "constant proof actual result must equal its ValueBinding";
  APSInt actualValue(boundInteger.getValue(), isUnsigned);
  if (canonical(actualValue) != canonical(value))
    return emit() << "constant proof bit pattern disagrees with MathInt value";
  if (!isTrueI1(path) || !isTrueI1(actualValid) ||
      !isTrueI1(binding.getPath()) || !isTrueI1(binding.getValid()))
    return emit() << "constant proof requires literal-true path and validity";
  return success();
}
LogicalResult verifyConstantWitness(RuleOp rule, ValueBindingOp binding,
                                    NumericProofOp proof) {
  auto emit = [&] { return proof.emitOpError(); };
  auto scope = rule->getAttrOfType<DictionaryAttr>("ac.proof_scope");
  if (!scope ||
      failed(detail::verifyProofScope(scope,
                                      [&] { return proof.emitOpError(); })) ||
      scope.get("registration") != rule.getRegistrationAttr())
    return emit() << "NumericProof scope must match its registered rule";
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  auto inputIDs = proof.getInputIdsAttr();
  auto inputDomains = proof.getInputDomainsAttr();
  auto checks = proof.getChecksAttr();
  auto resultID = proof.getResultIdAttr();
  auto resultDomain = proof.getResultDomainAttr();
  auto origin = proof.getOriginAttr();
  auto obligations = proof.getObligationsAttr();
  if (!required || !obligations || required != obligations ||
      (obligations.size() != 1 && obligations.size() != 3) || !inputIDs ||
      !inputIDs.empty() || !inputDomains || !inputDomains.empty() || !checks ||
      !checks.empty() || !resultID || !resultDomain ||
      failed(detail::verifyValueID(resultID,
                                   [&] { return proof.emitOpError(); })) ||
      failed(detail::verifyOccurrence(origin,
                                      [&] { return proof.emitOpError(); })) ||
      failed(verifyB0EvidenceOrigin(proof, origin)) ||
      resultID.get("origin") != origin || !proof.getModeAttr() ||
      proof.getModeAttr().getValue() != "exact" || proof->hasAttr("width"))
    return emit() << "B0 NumericProof requires one exact result, no inputs, "
                     "checks, low_bits width, or unsupported obligations";
  if (obligations.size() == 1)
    return verifyConstantNode(proof, resultID, resultDomain,
                              proof->getOperand(1), proof->getOperand(2),
                              proof->getOperand(0), binding);
  return verifyExactConstantAddWitness(rule, binding, proof);
}

} // namespace
LogicalResult ValueBindingOp::verify() {
  RuleOp rule = (*this)->getParentOfType<RuleOp>();
  if (!rule || rule.getBody().getBlocks().size() != 1 ||
      (*this)->getBlock() != &rule.getBody().front())
    return emitOpError()
           << "B0 ValueBinding must be in a rule's top computation block";
  auto id = getIdAttr();
  auto domain = getDomainAttr();
  RuleOp owner;
  if (failed(verifyProofOwner(*this, owner)) || !id ||
      failed(detail::verifyValueID(id, [&] { return emitOpError(); })) ||
      failed(verifyB0EvidenceOrigin(
          *this, id ? id.getAs<DictionaryAttr>("origin") : DictionaryAttr())) ||
      !domain || (*this)->getAttrs().size() != 2)
    return emitOpError()
           << "ValueBinding requires exact ID, domain and rule scope";
  if (hasGenericFinalUses(rule))
    return verifyGenericFinalUses(rule);
  if (hasNumericCompositionContract(rule))
    return success();
  if (failed(verifyBindingDomain(domain, getValue().getType(), *this)))
    return failure();
  return verifyRuleNumericClosure(rule);
}
LogicalResult NumericProofOp::verify() {
  RuleOp rule = (*this)->getParentOfType<RuleOp>();
  if (!rule || rule.getBody().getBlocks().size() != 1 ||
      (*this)->getBlock() != &rule.getBody().front())
    return emitOpError()
           << "B0 NumericProof must be in a rule's top computation block";
  RuleOp owner;
  if (failed(verifyProofOwner(*this, owner)))
    return failure();
  if (hasNumericCompositionContract(rule))
    return success();
  if (hasNumericNextUseContract(rule))
    return verifyNumericNextUseClosure(rule);
  if (failed(verifySegments(*this)))
    return failure();
  auto mode = getModeAttr();
  const bool exact = mode && mode.getValue() == "exact";
  const bool lowBits = mode && mode.getValue() == "low_bits";
  if (!exact && !lowBits)
    return emitOpError() << "numeric proof mode must be exact or low_bits";
  auto obligations = getObligationsAttr();
  const bool checked = exact && obligations && obligations.size() == 2;
  if ((!checked && !getChecksAttr().empty()) ||
      (checked && getChecksAttr().size() != 1))
    return emitOpError() << "numeric proof checks do not match its profile";
  auto width = (*this)->getAttrOfType<IntegerAttr>("width");
  if ((exact && (width || (*this)->getAttrs().size() != 9)) ||
      (lowBits && (!width || (*this)->getAttrs().size() != 10)))
    return emitOpError() << "NumericProof mode/width schema is not closed";
  static const StringSet<> allowedAttributes = [] {
    StringSet<> names;
    for (StringRef name :
         {"operand_segment_sizes", "mode", "result_domain", "input_ids",
          "input_domains", "result_id", "obligations", "checks", "origin"})
      names.insert(name);
    names.insert("width");
    return names;
  }();
  for (NamedAttribute attribute : (*this)->getAttrs())
    if (!allowedAttributes.contains(attribute.getName().strref()))
      return emitOpError() << "B0 NumericProof rejects unknown attributes";
  if (failed(verifyRuleNumericClosure(rule)))
    return failure();
  auto binding =
      findBinding(rule, getResultIdAttr(), [&] { return emitOpError(); });
  if (failed(binding))
    return failure();
  if (getInputIdsAttr().empty()) {
    if (!getInputDomainsAttr().empty())
      return emitOpError() << "B0 input domains must be empty";
    return verifyConstantWitness(rule, *binding, *this);
  }
  if (getInputIdsAttr().size() != 1 || getInputDomainsAttr().size() != 1)
    return emitOpError() << "unsupported numeric proof input profile";
  if (lowBits)
    return verifyLowBitsInputMaskWitness(rule, *binding, *this);
  if (checked)
    return verifyCheckedToBitsWitness(rule, *binding, *this);
  return verifyExactInputWitness(rule, *binding, *this);
}
LogicalResult verifyExactInputWitness(RuleOp rule, ValueBindingOp resultBinding,
                                      NumericProofOp proof) {
  auto opcode = finalOpcode(proof);
  if (failed(opcode))
    return failure();
  if (*opcode == "add")
    return verifyExactInputConstantAddWitness(rule, resultBinding, proof);
  if (*opcode == "sub")
    return verifyExactInputConstantSubWitness(rule, resultBinding, proof);
  if (*opcode == "and_bits")
    return verifyExactInputConstantMaskWitness(rule, resultBinding, proof);
  if (isCompareOpcode(*opcode))
    return verifyExactInputConstantCompareWitness(rule, resultBinding, proof);
  return proof.emitOpError()
         << "unsupported exact input scalar opcode '" << *opcode << "'";
}
LogicalResult verifyRuleNumericClosure(RuleOp rule) {
  auto error = [&] { return rule.emitOpError(); };
  if (!rule || rule.getBody().getBlocks().size() != 1)
    return error() << "numeric closure requires one rule body";
  if (hasNumericCompositionContract(rule))
    return verifyNumericCompositionClosure(rule);
  if (hasNumericNextUseContract(rule))
    return verifyNumericNextUseClosure(rule);
  Block &body = rule.getBody().front();
  SmallVector<ValueBindingOp> bindings(body.getOps<ValueBindingOp>());
  SmallVector<NumericProofOp> proofs(body.getOps<NumericProofOp>());
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  const bool hasProofScope = rule->hasAttr("ac.proof_scope");
  if (bindings.empty() && proofs.empty() && !required && !hasProofScope)
    return success();
  if (!required)
    return error() << "proof-scoped rule must retain ac.required_numeric";
  if (required.empty() && bindings.empty() && proofs.empty() && !hasProofScope)
    return success();
  if (required.empty() && bindings.empty() && proofs.empty())
    return error() << "proof_scope has no required_numeric witness packet";
  if (bindings.empty() && proofs.empty() && !required.empty())
    return verifySourceNumericClosure(rule, required);
  if (!required ||
      (required.size() != 1 && required.size() != 2 && required.size() != 3 &&
       required.size() != 5) ||
      proofs.size() != 1)
    return error()
           << "exact numeric proof requires one or three nodes and one proof";
  auto proofNode = proofs.front().getObligationsAttr();
  if (proofNode != required)
    return error()
           << "required_numeric must structurally equal proof obligations";
  auto inputIDs = proofs.front().getInputIdsAttr();
  const bool b1b = inputIDs && inputIDs.size() == 1;
  auto mode = proofs.front().getModeAttr();
  const bool lowBits = mode && mode.getValue() == "low_bits";
  const bool checked =
      mode && mode.getValue() == "exact" && required.size() == 2;
  const size_t bindingOffset = b1b ? 1 : 0;
  if ((!b1b && !rule.getInputs().empty()) || !rule.getTargets().empty() ||
      (b1b && ((lowBits && required.size() != 5) ||
               (!lowBits && !checked && required.size() != 3) ||
               rule.getInputs().size() != 1)) ||
      bindings.size() !=
          (lowBits ? required.size() : required.size() + bindingOffset))
    return error()
           << "numeric proof rule ports/bindings do not match B0/B1b profile";
  if (b1b &&
      (failed(detail::verifyValueID(dyn_cast<DictionaryAttr>(inputIDs[0]),
                                    [&] { return rule.emitOpError(); })) ||
       bindings[0].getIdAttr() != inputIDs[0]))
    return error() << "B1b input ID must equal its first ValueBinding";
  llvm::DenseSet<Attribute> ids;
  size_t bindingIndex = bindingOffset;
  for (auto [index, raw] : llvm::enumerate(required)) {
    auto node = dyn_cast<DictionaryAttr>(raw);
    auto id = node ? node.getAs<DictionaryAttr>("id") : DictionaryAttr();
    const bool elidedLowBitsArithmetic = lowBits && index == 2;
    if (!node || node.size() != 4 ||
        failed(detail::verifyValueID(id, [&] { return rule.emitOpError(); })) ||
        !ids.insert(id).second ||
        (!elidedLowBitsArithmetic &&
         (bindingIndex >= bindings.size() ||
          bindings[bindingIndex].getIdAttr() != id ||
          !bindings[bindingIndex]->isBeforeInBlock(proofs.front()))))
      return error() << "required numeric node and ValueBinding IDs/order "
                        "must match without duplicates";
    if (!elidedLowBitsArithmetic)
      ++bindingIndex;
  }
  if (bindingIndex != bindings.size())
    return error() << "numeric proof has an unexpected retained binding";
  auto finalNode = dyn_cast<DictionaryAttr>(required[required.size() - 1]);
  if (!finalNode ||
      proofs.front().getResultIdAttr() != finalNode.getAs<DictionaryAttr>("id"))
    return error() << "numeric proof result ID must be the final recipe node";
  return success();
}
} // namespace acir::ac
