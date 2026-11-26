#include "ACIRSourceContracts.h"
#include "SystemSampling.h"
#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
namespace acir::ac {
namespace {
mlir::FailureOr<HardwareResetSemantics> readResetSemantics(SystemOp system) {
  auto raw = system->getAttr("ac.execution_policy");
  if (!raw)
    return HardwareResetSemantics::Strict;
  auto policy = mlir::dyn_cast<mlir::DictionaryAttr>(raw);
  auto reset =
      policy ? policy.getAs<mlir::StringAttr>("reset") : mlir::StringAttr();
  if (!policy || policy.size() != 1 || !reset ||
      (reset.getValue() != "strict" && reset.getValue() != "if_one"))
    return system.emitOpError()
           << "ac.execution_policy requires exactly reset = strict or if_one";
  return reset.getValue() == "if_one" ? HardwareResetSemantics::IfOne
                                      : HardwareResetSemantics::Strict;
}
} // namespace

mlir::FailureOr<HardwareResetSemantics>
getHardwareResetSemantics(mlir::ModuleOp package) {
  if (!package)
    return mlir::failure();
  auto systems = package.getOps<SystemOp>();
  if (!llvm::hasSingleElement(systems))
    return package.emitOpError()
           << "reset policy requires exactly one root ac.system";
  auto system = *systems.begin();
  auto misplaced = package.walk([&](mlir::Operation *operation) {
    if (!operation->hasAttr("ac.execution_policy"))
      return mlir::WalkResult::advance();
    auto stage = package->getAttrOfType<mlir::StringAttr>("ac.stage");
    if (operation != system.getOperation() ||
        (stage && stage.getValue() != "final" && stage.getValue() != "routed") ||
        package->hasAttr("ac.unit_kind")) {
      operation->emitOpError() << "ac.execution_policy belongs only to the "
                                  "executable root ac.system";
      return mlir::WalkResult::interrupt();
    }
    return mlir::WalkResult::advance();
  });
  if (misplaced.wasInterrupted())
    return mlir::failure();
  return readResetSemantics(system);
}

mlir::FailureOr<mlir::ArrayAttr>
getSystemSamples(mlir::Operation *declaration) {
  if (!declaration)
    return mlir::failure();
  auto raw = declaration->getAttr("ac.system_samples");
  if (!raw)
    return mlir::ArrayAttr::get(declaration->getContext(), {});
  auto error = [&] { return declaration->emitOpError(); };
  auto kind = declaration->getAttrOfType<mlir::StringAttr>("ac.root_kind");
  auto metadata = mlir::dyn_cast<mlir::DictionaryAttr>(raw);
  auto rows =
      metadata ? metadata.getAs<mlir::ArrayAttr>("rows") : mlir::ArrayAttr();
  if (!mlir::isa<ModuleOp, ModuleImportOp>(declaration) ||
      !mlir::isa_and_nonnull<mlir::ModuleOp>(declaration->getParentOp()) ||
      !kind || kind.getValue() != "system" || !metadata ||
      (metadata.size() != 3 && metadata.size() != 4) || !rows || rows.empty() ||
      rows.size() > HardwareSourceCheckLimits{}.maxWorkUnits)
    return error() << "ac.system_samples requires a bounded nonempty rows, "
                      "origin and location dictionary on a System declaration";
  if (mlir::failed(detail::verifySourceEffectMetadata(
          metadata.getAs<mlir::DictionaryAttr>("origin"),
          metadata.getAs<mlir::DictionaryAttr>("location"), declaration,
          error)))
    return mlir::failure();
  auto inputs = readSystemSampleInputs(declaration);
  if (mlir::failed(inputs) || (metadata.size() == 4 && inputs->empty()))
    return mlir::failure();
  unsigned width = inputs->empty() ? 2 : inputs->size();
  for (auto rawRow : rows) {
    auto row = mlir::dyn_cast<mlir::StringAttr>(rawRow);
    if (!row || row.getValue().size() != width ||
        !llvm::all_of(row.getValue(), [](char bit) {
          return bit == '0' || bit == '1' || bit == 'x' || bit == 'z';
        }))
      return error()
             << "System sample rows require one canonical 0/1/x/z character "
                "per sampled control";
  }
  return rows;
}

mlir::FailureOr<llvm::SmallVector<unsigned>>
readSystemSampleInputs(mlir::Operation *declaration) {
  auto metadata =
      declaration->getAttrOfType<mlir::DictionaryAttr>("ac.system_samples");
  if (!metadata || !metadata.get("inputs"))
    return llvm::SmallVector<unsigned>{};
  auto ordinals = metadata.getAs<mlir::ArrayAttr>("inputs");
  auto signature = declaration->getAttrOfType<mlir::TypeAttr>("function_type");
  auto type = signature
                  ? mlir::dyn_cast<mlir::FunctionType>(signature.getValue())
                  : mlir::FunctionType();
  if (!ordinals || ordinals.empty() || !type)
    return declaration->emitOpError() << "explicit System samples must cover "
                                         "its closed logical input boundary";
  llvm::DenseSet<unsigned> seen;
  llvm::SmallVector<unsigned> result;
  for (auto raw : ordinals) {
    auto ordinal = mlir::dyn_cast<mlir::IntegerAttr>(raw);
    if (!ordinal || ordinal.getInt() < 0 ||
        ordinal.getValue().getActiveBits() > 32 ||
        ordinal.getValue().getZExtValue() >= type.getNumInputs() ||
        !seen.insert(ordinal.getValue().getZExtValue()).second)
      return declaration->emitOpError()
             << "System sample inputs require unique logical input ordinals";
    unsigned index = ordinal.getValue().getZExtValue();
    result.push_back(index);
  }
  return result;
}

mlir::LogicalResult SystemOp::verify() {
  if (!mlir::isa<mlir::ModuleOp>((*this)->getParentOp()) ||
      getDomain() != "default")
    return emitOpError()
           << "system requires package placement and default domain";
  if (mlir::failed(readResetSemantics(*this)))
    return mlir::failure();
  auto entry = getEntry();
  if (!entry || entry.size() != 3 ||
      !entry.getAs<mlir::FlatSymbolRefAttr>("callee") ||
      !entry.getAs<mlir::ArrayAttr>("parameters") ||
      !entry.getAs<mlir::ArrayAttr>("type_arguments"))
    return emitOpError()
           << "system entry requires callee, parameters and type_arguments";
  for (auto arg : entry.getAs<mlir::ArrayAttr>("parameters"))
    if (!mlir::isa<StaticExprAttr>(arg))
      return emitOpError() << "root integer arguments require StaticExpr";
  for (auto arg : entry.getAs<mlir::ArrayAttr>("type_arguments")) {
    auto type = mlir::dyn_cast<mlir::TypeAttr>(arg);
    if (!type || !mlir::isa<BitsType, StructType>(type.getValue()))
      return emitOpError()
             << "root type arguments require finite hardware payload";
  }
  return mlir::success();
}
} // namespace acir::ac
