// Enum value resolution and verification (B2 step-2 design r9): the
// HardwareAnalysis members. The declaration grammar lives in the operations
// TU (ACIREnumOps.cpp); the value-operation checks run from the staged rows
// (the source-unit semantic walk and verifyResolvedOperation), so this TU
// joins the ACIRAnalysis target at IX.
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "ACIRSourceContracts.h"
#include "llvm/ADT/StringSet.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"

using namespace mlir;

namespace acir::ac {

EnumOp HardwareAnalysis::lookupEnum(EnumType type) const {
  return dyn_cast_or_null<EnumOp>(lookupDefinition(type.getName().getValue()));
}

FailureOr<EnumDefinitionView>
HardwareAnalysis::resolveEnum(EnumType type, Operation *site) const {
  return detail::resolveNominalEnum(
      type, site, [this](StringRef name) { return lookupDefinition(name); });
}

LogicalResult
HardwareAnalysis::verifyEnumValueOperation(Operation *operation,
                                           const HardwareBindings &bindings,
                                           bool requireResolved) const {
  auto width = [&](Type type, uint64_t expected,
                   StringRef description) -> LogicalResult {
    auto bits = dyn_cast<BitsType>(type);
    if (!bits)
      return operation->emitOpError() << description << " requires bits type";
    auto resolved = resolveType(bits, bindings, operation);
    if (failed(resolved))
      return failure();
    bits = cast<BitsType>(*resolved);
    if (!isStaticEvaluable(bits.getWidth(), bindings)) {
      if (requireResolved || !operation->getParentOfType<ModuleOp>())
        return operation->emitOpError()
               << description << " requires a resolved finite width";
      // The owning module's existing scope verifier checks every reference.
      // Each closed occurrence must call this verifier with requireResolved.
      return success();
    }
    auto actual = getPackedWidth(bits, bindings, operation);
    if (failed(actual))
      return failure();
    if (*actual != expected)
      return operation->emitOpError()
             << description << " width must equal " << expected;
    return success();
  };
  auto definition = [&](Type type) -> FailureOr<EnumDefinitionView> {
    auto nominal = dyn_cast<EnumType>(type);
    if (!nominal)
      return operation->emitOpError()
             << "enum operation requires an explicit nominal enum type";
    return resolveEnum(nominal, operation);
  };

  if (auto create = dyn_cast<EnumCreateOp>(operation)) {
    auto view = definition(create.getResult().getType());
    if (failed(view))
      return failure();
    for (const EnumMember &member : view->members)
      if (member.name == create.getMemberAttr())
        return success();
    return operation->emitOpError()
           << "unknown enum member '" << create.getMember() << "'";
  }
  if (auto toBits = dyn_cast<EnumToBitsOp>(operation)) {
    auto view = definition(toBits.getInput().getType());
    if (failed(view))
      return failure();
    return width(toBits.getResult().getType(), view->width,
                 "enum.to_bits result");
  }
  if (auto fromBits = dyn_cast<EnumFromBitsOp>(operation)) {
    auto view = definition(fromBits.getValue().getType());
    if (failed(view) || failed(width(fromBits.getInput().getType(), view->width,
                                     "enum.from_bits input")))
      return failure();
    return width(fromBits.getMember().getType(), 1,
                 "enum.from_bits membership result");
  }
  return success();
}

} // namespace acir::ac