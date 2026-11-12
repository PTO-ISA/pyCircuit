#include "PackageVerification.h"
#include "ACIRSourceContracts.h"
#include "EntryBoundary.h"
#include "HardwareSourceChecks.h"
#include "HardwareValueAvailability.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Interfaces/LogicalStateInterfaces.h"

using namespace mlir;
namespace acir::ac {
const detail::PackageStagePolicy &detail::PackageStagePolicy::routed() {
  static const PackageStagePolicy policy = {
      .stageRequired = true,
      .stageValue = "routed",
      .admitRoutedLogicalStateByName = true,
      .rejectUnloweredInstanceBindings = true,
      .requireExternalContract = true,
      .requireImplementedImports = false,
      .anchorProvenanceTopLevelOnly = false,
      .rejectUnloweredTransactions = false,
      .combinationalExternalAdmission = true,
  };
  return policy;
}
const detail::PackageStagePolicy &detail::PackageStagePolicy::hardware() {
  static const PackageStagePolicy policy = {
      .stageRequired = false,
      .stageValue = "final",
      .admitRoutedLogicalStateByName = false,
      .rejectUnloweredInstanceBindings = false,
      .requireExternalContract = false,
      .requireImplementedImports = true,
      .anchorProvenanceTopLevelOnly = true,
      .rejectUnloweredTransactions = true,
      .combinationalExternalAdmission = false,
  };
  return policy;
}
LogicalResult detail::verifyHardwarePackageStage(
    const HardwareAnalysis &analysis, const PackageStagePolicy &policy) {
  auto package = analysis.getPackage();
  if (package->hasAttr("ac.transactional"))
    return package.emitOpError()
           << "transactional resource packages cannot enter legacy routed or final hardware analysis";
  auto rawStage = package->getAttr("ac.stage");
  auto stage = dyn_cast_or_null<StringAttr>(rawStage);
  auto marker = dyn_cast_or_null<IntegerAttr>(package->getAttr("ac.routed_contract"));
  bool boundary = llvm::any_of(package.getOps<SystemOp>(), [](SystemOp system) {
    return system->hasAttr("ac.entry_boundary");
  });
  if (package->hasAttr("ac.unit_kind") ||
      (policy.stageRequired
           ? !stage || stage.getValue() != policy.stageValue
           : rawStage && (!stage || stage.getValue() != policy.stageValue))) {
    if (policy.stageRequired)
      return package.emitOpError()
             << "routed verification requires ac.stage = routed";
    return package.emitOpError()
           << "executable hardware verification requires final hardware; routed packages require state refinement";
  }
  // Shared unconditionally by both stages: a package under routed
  // verification, an entry boundary or a retained routed contract must carry
  // the regenerated marker (4, or 5 with an entry boundary); final packages
  // re-validate it rather than trusting the routed pass.
  if ((policy.stageRequired || boundary || package->hasAttr("ac.routed_contract")) &&
      (!marker || !marker.getType().isSignlessInteger(64) ||
       marker.getValue() != (boundary ? 5 : 4)))
    return package.emitOpError()
           << "unsupported or missing ac.routed_contract; regenerate the routed artifact";
  auto closed = package.walk([&](Operation *operation) {
    // HWIR rejects every routed logical-state interface unconditionally,
    // including retained dead regions; source state forms stay rejected by
    // their own declaration contract.
    if (!policy.admitRoutedLogicalStateByName &&
        operation != package.getOperation() &&
        isa<acir::LogicalStateOpInterface,
            acir::LogicalStateUpdateOpInterface>(operation)) {
      operation->emitOpError()
          << "logical state cannot cross into executable final hardware";
      return WalkResult::interrupt();
    }
    // A reference denotes a source allocation, never a runtime payload or an
    // inert annotation on a closed hardware graph. Routing must consume both
    // bindings and callable contracts before either standalone boundary.
    for (StringRef name : {"ac.reference_parameters", "ac.reference_bindings",
                           "ac.reference", "ac.callable_after", "ac.after"}) {
      if (operation->hasAttr(name)) {
        operation->emitOpError()
            << "unresolved source reference or callable order cannot cross "
               "the routed/final boundary: "
            << name;
        return WalkResult::interrupt();
      }
    }
    if (operation->hasAttr("ac.anchor_provenance") &&
        (!policy.anchorProvenanceTopLevelOnly ||
         operation != package.getOperation())) {
      operation->emitOpError()
          << "ac.anchor_provenance belongs only to the top-level final package";
      return WalkResult::interrupt();
    }
    if (operation != package.getOperation() && operation->hasAttr("ac.routed_contract")) {
      operation->emitOpError() << "ac.routed_contract belongs only to the top-level package";
      return WalkResult::interrupt();
    }
    if (policy.rejectUnloweredInstanceBindings && isa<InstanceOp>(operation) &&
        (operation->hasAttr("ac.source_unbound_push") ||
         operation->hasAttr("ac.source_unbound_pop") ||
         operation->hasAttr("ac.source_memory") ||
         operation->hasAttr("ac.payload_constraint"))) {
      operation->emitOpError() << "unlowered source resource binding cannot cross the routed boundary";
      return WalkResult::interrupt();
    }
    if (!policy.admitRoutedLogicalStateByName ||
        operation == package.getOperation() ||
        isa<ModuleOp, ModuleImportOp, StructOp, EnumOp, SystemOp, InstanceOp,
            CollectionOp, RuleOp, YieldOp, SourceExpectOp,
            SourceObserveOp>(operation) ||
        isCombinationalHardwareOperation(operation))
      return WalkResult::advance();
    // Closed routed admission: exactly the two concrete operations the GFIR
    // dialect registers (see GFIROps.td). Generic construction can build
    // unregistered raw IR with a forged gfir.* name, so the registered
    // status and the full name are both required; no interface, callback,
    // namespace or third-party dialect can extend this list, and no
    // dialect pointer is dereferenced before native verification.
    mlir::OperationName name = operation->getName();
    if (name.isRegistered() &&
        (name.getStringRef() == "gfir.state" ||
         name.getStringRef() == "gfir.state.update"))
      return WalkResult::advance();
    operation->emitOpError() << "operation outside closed routed hardware package";
    return WalkResult::interrupt();
  });
  return failure(closed.wasInterrupted());
}
LogicalResult detail::verifyHardwarePackageEnvelope(
    const HardwareAnalysis &analysis, const PackageStagePolicy &policy,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  auto package = analysis.getPackage();
  if (!package)
    return failure();
  if (failed(verifyEntryBoundary(analysis, false, debit)))
    return failure();
  auto registrations = package.walk([&](Operation *operation) {
    if (!operation->hasAttr("ac.registration_path") &&
        !operation->hasAttr("ac.lexical_registration") &&
        !operation->hasAttr("ac.registration_owner"))
      return WalkResult::advance();
    auto rule = dyn_cast<RuleOp>(operation);
    if (!rule) {
      operation->emitOpError()
          << "routed registration metadata requires a sampled Rule";
      return WalkResult::interrupt();
    }
    return failed(verifyRoutedRegistration(analysis, rule, debit))
               ? WalkResult::interrupt()
               : WalkResult::advance();
  });
  if (registrations.wasInterrupted())
    return failure();
  if (failed(verifyTransactionReturns(analysis, debit)))
    return failure();
  auto layouts = package.walk([&](Operation *operation) {
    if (!operation->hasAttr("ac.internal_ports") &&
        !operation->hasAttr("ac.input_validity") &&
        !operation->hasAttr("ac.output_validity") &&
        !operation->hasAttr("ac.routed_output_validity"))
      return WalkResult::advance();
    if (!isa<ModuleOp>(operation)) {
      operation->emitOpError()
          << "ac.internal_ports is permitted only on a real module "
             "in a routed or final package";
      return WalkResult::interrupt();
    }
    return failed(getModulePortLayout(operation)) ? WalkResult::interrupt()
                                                  : WalkResult::advance();
  });
  if (layouts.wasInterrupted())
    return failure();
  if (policy.rejectUnloweredTransactions &&
      failed(verifyNoUnloweredTransactions(package)))
    return failure();
  if (failed(collectFinalSourceUnits(package,
                                     [&] { return package.emitOpError(); })))
    return failure();
  for (Operation &op : package.getBody()->getOperations())
    if (!isa<ModuleOp, ModuleImportOp, StructOp, EnumOp, SystemOp>(op))
      return op.emitOpError()
             << "operation outside closed executable hardware package";
  for (auto record : analysis.getStructs())
    if (failed(analysis.getPackedWidth(
            StructType::get(record.getContext(), record.getSymNameAttr()), {},
            record)))
      return failure();
  for (auto enumeration : package.getOps<EnumOp>())
    if (failed(analysis.resolveEnum(EnumType::get(enumeration.getContext(),
                                                  enumeration.getSymNameAttr()),
                                    enumeration)))
      return failure();
  for (auto imported : package.getOps<ModuleImportOp>()) {
    if (failed(analysis.verifyImport(imported)))
      return failure();
    if (analysis.getPrimitiveKind(imported).empty()) {
      if (!imported->hasAttr("ac.external_contract") &&
          policy.requireExternalContract)
        return imported.emitOpError()
               << "routed ordinary module import requires ac.external_contract";
      if (policy.requireImplementedImports)
        return imported.emitOpError()
               << "executable hardware package has unresolved ordinary module import; implementation is required for execution completeness";
    }
  }
  return verifyHardwareValueAvailability(analysis, debit);
}
} // namespace acir::ac
