#include "ACIRSourceContracts.h"
#include "HardwareSourceChecks.h"
#include "HardwareValueAvailability.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
using namespace mlir;
namespace acir::ac {
namespace {
LogicalResult primitiveSchema(const HardwareAnalysis &analysis,
                              ModuleImportOp op) {
  StringRef kind = analysis.getPrimitiveKind(op);
  if (kind.empty())
    return success();
  bool fifo = kind == "fifo_local" || kind == "fifo_downstream_pop";
  if (fifo && op.getSymName() != (Twine("pycircuit.__builtins__.") + kind).str())
    return op.emitOpError() << "trusted FIFO requires its canonical builtin symbol";
  SmallVector<StringRef> inputs, outputs;
  if (kind == "dff") {
    inputs = {"clk", "rst", "d", "init"};
    outputs = {"q"};
  } else if (kind == "dffe") {
    inputs = {"clk", "rst", "en", "d", "init"};
    outputs = {"q"};
  } else if (kind == "sync_mem") {
    inputs = {"clk",    "rst",   "ren",   "raddr",
              "wvalid", "waddr", "wdata", "wstrb"};
    outputs = {"rdata", "rlive"};
  } else if (kind == "sync_mem_dp") {
    inputs = {"clk",    "rst",    "ren0",  "raddr0", "ren1",
              "raddr1", "wvalid", "waddr", "wdata",  "wstrb"};
    outputs = {"rdata0", "rdata1", "rlive0", "rlive1"};
  } else if (kind == "byte_mem") {
    inputs = {"clk", "rst", "raddr", "wvalid", "waddr", "wdata", "wstrb"};
    outputs = {"rdata"};
  } else if (fifo) {
    inputs = {"clk", "rst", "in_valid", "in_data", "out_ready"};
    outputs = {"ready", "valid", "data", "local_capacity"};
  } else
    return op.emitOpError() << "unknown trusted primitive kind";
  if (op.getInputNames().size() != inputs.size() ||
      op.getOutputNames().size() != outputs.size())
    return op.emitOpError() << "trusted primitive pin count mismatch";
  for (auto [raw, name] : llvm::zip(op.getInputNames(), inputs))
    if (cast<StringAttr>(raw).getValue() != name)
      return op.emitOpError() << "trusted primitive input pin schema mismatch";
  for (auto [raw, name] : llvm::zip(op.getOutputNames(), outputs))
    if (cast<StringAttr>(raw).getValue() != name)
      return op.emitOpError() << "trusted primitive output pin schema mismatch";
  if (op.getTypeParameters().size() != 1 ||
      cast<StringAttr>(op.getTypeParameters()[0]).getValue() != "T")
    return op.emitOpError()
           << "trusted primitive requires payload type formal T";
  bool memory = kind == "sync_mem" || kind == "sync_mem_dp" || kind == "byte_mem";
  if (op.getParameters().size() != (memory || fifo ? 2 : 0))
    return op.emitOpError()
           << "trusted primitive integer parameter schema mismatch";
  constexpr StringLiteral memoryParameters[] = {"ADDR_WIDTH", "DEPTH"};
  if (memory)
    for (auto [raw, name] : llvm::zip(op.getParameters(), memoryParameters))
      if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue() !=
          name)
        return op.emitOpError()
               << "trusted memory requires ADDR_WIDTH, DEPTH parameters";
  constexpr StringLiteral fifoParameters[] = {"DEPTH", "AVAILABILITY_LATENCY"};
  if (fifo)
    for (auto [raw, name] : llvm::zip(op.getParameters(), fifoParameters))
      if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue() != name)
        return op.emitOpError()
               << "trusted FIFO requires DEPTH, AVAILABILITY_LATENCY parameters";
  auto sig = op.getFunctionType();
  auto payload = TypeParamType::get(
      op.getContext(), FlatSymbolRefAttr::get(op.getContext(), op.getSymName()),
      StringAttr::get(op.getContext(), "T"));
  for (auto [index, name] : llvm::enumerate(inputs)) {
    Type type = sig.getInput(index);
    if (name == "d" || name == "init" || name == "wdata" || name == "in_data") {
      if (type != payload)
        return op.emitOpError()
               << "trusted primitive data pins require payload T";
      continue;
    }
    auto bits = dyn_cast<BitsType>(type);
    if (!bits)
      return op.emitOpError()
             << "trusted primitive control/address/strobe pins require bits";
    if (name.contains("addr")) {
      auto tree = bits.getWidth().getTree();
      auto ref = tree.getAs<DictionaryAttr>("ref");
      if (!ref || !ref.getAs<StringAttr>("name") ||
          ref.getAs<StringAttr>("name").getValue() != "ADDR_WIDTH" ||
          ref.getAs<FlatSymbolRefAttr>("owner").getValue() != op.getSymName())
        return op.emitOpError()
               << "trusted memory address width must reference ADDR_WIDTH";
    } else if (name != "wstrb") {
      auto value = analysis.evaluateStatic(bits.getWidth(), {}, op);
      if (failed(value))
        return failure();
      auto integer = dyn_cast<MathIntAttr>(*value);
      if (!integer || integer.getCanonicalValue() != "1")
        return op.emitOpError()
               << "trusted primitive control pin must be one bit";
    }
  }
  for (auto [index, type] : llvm::enumerate(sig.getResults())) {
    bool memoryLive = (kind == "sync_mem" && index == 1) ||
                      (kind == "sync_mem_dp" && index >= 2);
    if ((fifo && index != 2) || memoryLive) {
      auto bits = dyn_cast<BitsType>(type);
      StringRef message = memoryLive
          ? "trusted synchronous memory live output must be one bit"
          : "trusted FIFO status output must be one bit";
      if (!bits)
        return op.emitOpError() << message;
      auto width = analysis.evaluateStatic(bits.getWidth(), {}, op);
      auto integer = succeeded(width) ? dyn_cast<MathIntAttr>(*width) : MathIntAttr();
      if (!integer || integer.getCanonicalValue() != "1")
        return op.emitOpError() << message;
    } else if (type != payload)
      return op.emitOpError() << "trusted primitive output requires payload T";
  }
  auto deps = analysis.getImportDependencies(op, {});
  if (failed(deps))
    return failure();
  for (auto [index, dep] : llvm::enumerate(*deps)) {
    if (kind == "fifo_downstream_pop" && index == 0) {
      if (dep.inputs.size() != 1 || dep.inputs[0].port != 4 ||
          !dep.inputs[0].path.empty())
        return op.emitOpError() << "FIFO ready must depend only on out_ready";
    } else if (kind == "byte_mem") {
      if (dep.inputs.size() != 1 || dep.inputs[0].port != 2 ||
          !dep.inputs[0].path.empty())
        return op.emitOpError() << "byte_mem output must depend only on raddr";
    } else if (!dep.inputs.empty())
      return op.emitOpError()
             << "registered outputs must be temporal dependency cuts";
  }
  return success();
}
} // namespace
LogicalResult HardwareAnalysis::verifyImport(ModuleImportOp imported) const {
  if (failed(detail::verifyModuleImportSummarySelection(imported)))
    return failure();
  if (imported.getSemanticKind() != "hardware")
    return imported.emitOpError()
           << "hardware analysis cannot consume a transactions import";
  if (failed(primitiveSchema(*this, imported)))
    return failure();
  return success(succeeded(getImportDependencies(imported, {})));
}
LogicalResult detail::verifyHardwarePackageStage(
    const HardwareAnalysis &analysis, PackageVerificationProfile profile) {
  auto package = analysis.getPackage();
  auto rawStage = package->getAttr("ac.stage");
  auto stage = dyn_cast_or_null<StringAttr>(rawStage);
  auto marker = dyn_cast_or_null<IntegerAttr>(package->getAttr("ac.routed_contract"));
  bool routed = profile == PackageVerificationProfile::Routed;
  if (package->hasAttr("ac.unit_kind") ||
      (routed ? !stage || stage.getValue() != "routed"
              : rawStage && (!stage || stage.getValue() != "final")))
    return package.emitOpError()
           << (routed ? "routed verification requires ac.stage = routed"
                      : "executable hardware verification requires final hardware; routed packages require state refinement");
  if ((routed || package->hasAttr("ac.routed_contract")) &&
      (!marker || !marker.getType().isSignlessInteger(64) ||
       marker.getValue() != 2))
    return package.emitOpError()
           << "unsupported or missing ac.routed_contract; regenerate the routed artifact";
  auto closed = package.walk([&](Operation *operation) {
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
        (operation != package.getOperation() || routed)) {
      operation->emitOpError()
          << "ac.anchor_provenance belongs only to the top-level final package";
      return WalkResult::interrupt();
    }
    if (operation != package.getOperation() && operation->hasAttr("ac.routed_contract")) {
      operation->emitOpError() << "ac.routed_contract belongs only to the top-level package";
      return WalkResult::interrupt();
    }
    if (routed && isa<InstanceOp>(operation) &&
        (operation->hasAttr("ac.source_unbound_push") ||
         operation->hasAttr("ac.source_unbound_pop") ||
         operation->hasAttr("ac.source_memory") ||
         operation->hasAttr("ac.payload_constraint"))) {
      operation->emitOpError() << "unlowered source resource binding cannot cross the routed boundary";
      return WalkResult::interrupt();
    }
    if (!routed || operation == package.getOperation() ||
        isa<ModuleOp, ModuleImportOp, StructOp, EnumOp, SystemOp, InstanceOp,
            CollectionOp, RuleOp, YieldOp, SourceExpectOp, SourceObserveOp,
            SourceStateOp, StateUpdateOp>(operation) ||
        isCombinationalHardwareOperation(operation))
      return WalkResult::advance();
    operation->emitOpError() << "operation outside closed routed hardware package";
    return WalkResult::interrupt();
  });
  return failure(closed.wasInterrupted());
}
LogicalResult detail::verifyHardwarePackageEnvelope(
    const HardwareAnalysis &analysis, PackageVerificationProfile profile,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  auto package = analysis.getPackage();
  if (!package)
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
  if (profile == PackageVerificationProfile::Hardware &&
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
          profile == PackageVerificationProfile::Routed)
        return imported.emitOpError()
               << "routed ordinary module import requires ac.external_contract";
      if (profile == PackageVerificationProfile::Hardware)
        return imported.emitOpError()
               << "executable hardware package has unresolved ordinary module import; implementation is required for execution completeness";
    }
  }
  return verifyHardwareValueAvailability(analysis, debit);
}
LogicalResult HardwareAnalysis::verify() const {
  // The shared builder orders raw budget preflight, structural verification,
  // envelope/schema validation and compact closure analysis consistently.
  return success(succeeded(getSourceCheckPlan()));
}
} // namespace acir::ac
