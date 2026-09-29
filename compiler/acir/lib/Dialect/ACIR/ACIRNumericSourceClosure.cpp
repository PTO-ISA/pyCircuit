#include "ACIRNumericProof.h"

#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"

using namespace mlir;

namespace acir::ac {
namespace {

bool isTrueI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(1) &&
         integer.getValue().isOne();
}

} // namespace

LogicalResult verifySourceNumericClosure(RuleOp rule, ArrayAttr required) {
  auto emit = [&] { return rule.emitOpError(); };
  auto file = rule->getParentOfType<mlir::ModuleOp>();
  auto stage =
      file ? file->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  auto kind =
      file ? file->getAttrOfType<StringAttr>("ac.unit_kind") : StringAttr();
  if (!file || !stage || stage.getValue() != "source" || !kind ||
      kind.getValue() != "implementation" ||
      (required.size() != 2 && required.size() != 3 && required.size() != 5) ||
      rule.getInputs().size() != 1 || !rule.getTargets().empty() ||
      rule.getBody().getBlocks().size() != 1)
    return emit() << "source numeric closure requires the N0-C1 profile";
  Block &body = rule.getBody().front();
  SmallVector<SourceReadOp> reads(body.getOps<SourceReadOp>());
  SmallVector<MathFromBitsOp> fromBits(body.getOps<MathFromBitsOp>());
  SmallVector<MathConstantOp> constants(body.getOps<MathConstantOp>());
  SmallVector<MathBinaryOp> binaries(body.getOps<MathBinaryOp>());
  SmallVector<MathCompareOp> compares(body.getOps<MathCompareOp>());
  SmallVector<MathToBitsOp> conversions(body.getOps<MathToBitsOp>());
  SmallVector<SourceExpectOp> expects(body.getOps<SourceExpectOp>());
  auto binaryOpcode = [](MathBinaryOp operation) {
    return operation->getAttrOfType<StringAttr>("operator");
  };
  auto hasOpcode = [&](unsigned index, StringRef expected) {
    auto node = dyn_cast<DictionaryAttr>(required[index]);
    auto opcode = node ? node.getAs<StringAttr>("operator") : StringAttr();
    return node && node.size() == 4 && opcode && opcode.getValue() == expected;
  };
  if (required.size() == 2) {
    auto fromNode = dyn_cast<DictionaryAttr>(required[0]);
    auto convertNode = dyn_cast<DictionaryAttr>(required[1]);
    auto fromOperands =
        fromNode ? fromNode.getAs<ArrayAttr>("operands") : ArrayAttr();
    auto convertOperands =
        convertNode ? convertNode.getAs<ArrayAttr>("operands") : ArrayAttr();
    auto target = convertNode ? convertNode.getAs<DictionaryAttr>("target")
                              : DictionaryAttr();
    auto targetKind = target ? target.getAs<StringAttr>("kind") : StringAttr();
    auto refIs = [&](Attribute raw, StringRef kind, unsigned index) {
      auto ref = dyn_cast<DictionaryAttr>(raw);
      auto actualKind = ref ? ref.getAs<StringAttr>("kind") : StringAttr();
      auto actualIndex = ref ? ref.getAs<IntegerAttr>("index") : IntegerAttr();
      auto decoded =
          detail::decodeU32(actualIndex, "D3 source NumericRef index", emit);
      return ref && ref.size() == 2 && actualKind &&
             actualKind.getValue() == kind && succeeded(decoded) &&
             *decoded == index;
    };
    auto requiredChecks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
    auto observations =
        rule->getAttrOfType<ArrayAttr>("ac.required_observations");
    bool inventory =
        reads.size() == 1 && fromBits.size() == 1 && conversions.size() == 1 &&
        expects.size() == 1 && constants.empty() && binaries.empty() &&
        compares.empty() && body.getOps<ValueBindingOp>().empty() &&
        body.getOps<NumericProofOp>().empty();
    if (!inventory || !hasOpcode(0, "from_bits") || !hasOpcode(1, "to_bits") ||
        !fromOperands || fromOperands.size() != 1 || !convertOperands ||
        convertOperands.size() != 1 || !refIs(fromOperands[0], "input", 0) ||
        !refIs(convertOperands[0], "node", 0) || !target ||
        target.size() != 2 || !targetKind ||
        targetKind.getValue() != "integer_boundary" ||
        target.getAs<DictionaryAttr>("domain") !=
            conversions[0].getDomainAttr())
      return emit() << "D3 source recipe/inventory is not closed";
    auto checkID = expects[0]->getAttrOfType<DictionaryAttr>("ac.check_id");
    auto checkTemplate =
        conversions[0]->getAttrOfType<DictionaryAttr>("ac.check_template");
    auto requiredCheck = requiredChecks && requiredChecks.size() == 1
                             ? dyn_cast<DictionaryAttr>(requiredChecks[0])
                             : DictionaryAttr();
    auto registration = checkID ? checkID.getAs<DictionaryAttr>("registration")
                                : DictionaryAttr();
    auto checkOccurrence =
        checkID ? checkID.getAs<DictionaryAttr>("check") : DictionaryAttr();
    auto obligation =
        checkID ? checkID.getAs<IntegerAttr>("obligation") : IntegerAttr();
    auto location = expects[0]->getAttrOfType<DictionaryAttr>("location");
    if (fromBits[0].getValue() != reads[0].getResult() ||
        conversions[0].getValue() != fromBits[0].getResult() ||
        expects[0].getCondition() != conversions[0].getValid() ||
        expects[0].getPath() != conversions[0].getPath() ||
        expects[0].getKind() != "range" || !checkID ||
        failed(detail::verifyCheckID(checkID, emit)) ||
        registration != rule.getRegistrationAttr() || !checkOccurrence ||
        !obligation || !checkTemplate || checkTemplate.size() != 4 ||
        checkTemplate.getAs<DictionaryAttr>("leaf") !=
            checkOccurrence.getAs<DictionaryAttr>("site") ||
        checkTemplate.getAs<StringAttr>("kind") !=
            StringAttr::get(rule.getContext(), "range") ||
        checkTemplate.getAs<IntegerAttr>("obligation") != obligation ||
        checkTemplate.getAs<DictionaryAttr>("location") != location ||
        !requiredCheck || requiredCheck.size() != 3 ||
        requiredCheck.getAs<DictionaryAttr>("id") != checkID ||
        requiredCheck.getAs<StringAttr>("kind") !=
            StringAttr::get(rule.getContext(), "range") ||
        requiredCheck.getAs<DictionaryAttr>("location") != location ||
        !rule.getTargets().empty() || (observations && !observations.empty()))
      return emit() << "D3 source SSA/check linkage is not exact";
    return success();
  }
  const bool exactMask = required.size() == 3 && reads.size() == 1 &&
                         fromBits.size() == 1 && constants.size() == 1 &&
                         binaries.size() == 1 && compares.empty() &&
                         binaryOpcode(binaries[0]) &&
                         binaryOpcode(binaries[0]).getValue() == "and_bits";
  const bool lowBitsMask =
      required.size() == 5 && reads.size() == 1 && fromBits.size() == 1 &&
      constants.size() == 2 && binaries.size() == 2 && compares.empty() &&
      binaryOpcode(binaries[0]) && binaryOpcode(binaries[1]) &&
      (binaryOpcode(binaries[0]).getValue() == "add" ||
       binaryOpcode(binaries[0]).getValue() == "sub") &&
      binaryOpcode(binaries[1]).getValue() == "and_bits";
  if (exactMask || lowBitsMask) {
    bool controlsAreTrue = llvm::all_of(binaries, [&](MathBinaryOp operation) {
      return isTrueI1(operation.getPath()) &&
             isTrueI1(operation.getLhsValid()) &&
             isTrueI1(operation.getRhsValid());
    });
    bool exactSSA = fromBits[0].getValue() == reads[0].getResult() &&
                    binaries[0].getLhs() == fromBits[0].getResult() &&
                    binaries[0].getRhs() == constants[0].getResult();
    bool lowSSA =
        !lowBitsMask || (binaries[1].getLhs() == binaries[0].getResult() &&
                         binaries[1].getRhs() == constants[1].getResult());
    bool ordered =
        hasOpcode(0, "from_bits") && hasOpcode(1, "constant") &&
        (exactMask ? hasOpcode(2, "and_bits")
                   : (hasOpcode(2, binaryOpcode(binaries[0]).getValue()) &&
                      hasOpcode(3, "constant") && hasOpcode(4, "and_bits")));
    if (!controlsAreTrue || !exactSSA || !lowSSA || !ordered ||
        !body.getOps<ValueBindingOp>().empty() ||
        !body.getOps<NumericProofOp>().empty())
      return emit() << "source mask closure SSA/recipe is not exact";
    return success();
  }
  if (reads.size() != 1 || fromBits.size() != 1 || constants.size() != 1 ||
      binaries.size() + compares.size() != 1 ||
      !body.getOps<ValueBindingOp>().empty() ||
      !body.getOps<NumericProofOp>().empty())
    return emit() << "source numeric closure requires one read/F/C/scalar "
                     "operation and no evidence";
  Operation *scalar = binaries.empty() ? compares.front().getOperation()
                                       : binaries.front().getOperation();
  auto opcode = binaries.empty()
                    ? compares.front().getPredicateAttr()
                    : scalar->getAttrOfType<StringAttr>("operator");
  if (!opcode ||
      !llvm::StringSwitch<bool>(opcode.getValue())
           .Cases({"add", "sub", "eq", "ne", "lt", "le", "gt", "ge"}, true)
           .Default(false) ||
      (opcode.getValue() == "add" || opcode.getValue() == "sub") !=
          !binaries.empty() ||
      !isTrueI1(scalar->getOperand(0)) || !isTrueI1(scalar->getOperand(2)) ||
      !isTrueI1(scalar->getOperand(4)) ||
      fromBits.front().getValue() != reads.front().getResult() ||
      !((scalar->getOperand(1) == fromBits.front().getResult() &&
         scalar->getOperand(3) == constants.front().getResult()) ||
        (opcode.getValue() == "add" &&
         scalar->getOperand(3) == fromBits.front().getResult() &&
         scalar->getOperand(1) == constants.front().getResult())))
    return emit() << "source numeric closure SSA/control graph is not exact";
  StringRef opcodes[] = {"from_bits", "constant", opcode.getValue()};
  for (auto [index, expectedOpcode] : llvm::enumerate(opcodes)) {
    auto node = dyn_cast<DictionaryAttr>(required[index]);
    auto operation = node ? node.getAs<StringAttr>("operator") : StringAttr();
    if (!node || node.size() != 4 || !operation ||
        operation.getValue() != expectedOpcode)
      return emit()
             << "source numeric obligation order must be from_bits/C/scalar";
  }
  return success();
}

} // namespace acir::ac
