#include "HardwareCDCLocalVerification.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "ACIRSourceContracts.h"
#include "IR/StateDeclarationContracts.h"
#include "pycircuit/Dialect/ACIR/SourceEffects.h"
#include "llvm/ADT/STLExtras.h"
using namespace mlir;
namespace acir::ac {
namespace {
LogicalResult resource(Operation *op, ArrayRef<StringRef> domains,
                       ArrayRef<StringRef> edges) {
  auto owner = dyn_cast_or_null<ModuleOp>(op->getParentOp());
  auto package =
      owner ? dyn_cast<mlir::ModuleOp>(owner->getParentOp()) : mlir::ModuleOp();
  auto stage =
      package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  auto contract =
      package ? package->getAttrOfType<IntegerAttr>("ac.routed_contract")
              : IntegerAttr();
  if (!owner || !stage ||
      (stage.getValue() != "routed" && stage.getValue() != "final") ||
      !contract || contract.getInt() != 7)
    return op->emitOpError(
        "CDC witness requires a contract-7 routed or final module graph");
  auto version = op->getAttrOfType<IntegerAttr>("resource_contract");
  if (!version || !version.getType().isSignlessInteger(64) ||
      version.getInt() != 1)
    return op->emitOpError("requires resource_contract = 1 : i64");
  auto name = op->getAttrOfType<StringAttr>("instance_name");
  if (!name || name.getValue().empty())
    return op->emitOpError("requires resource identity");
  auto occurrence = op->getAttrOfType<DictionaryAttr>("occurrence");
  if (failed(detail::verifyOccurrence(occurrence,
                                      [&] { return op->emitOpError(); })))
    return failure();
  for (auto &other : owner.getBody().front()) {
    if (&other == op || !other.hasAttr("instance_name"))
      continue;
    if (other.getAttr("instance_name") == name)
      return op->emitOpError(
          "resource name and occurrence must be unique within its owner");
  }
  for (auto field : domains) {
    auto domain = op->getAttrOfType<StringAttr>(field);
    auto declared = owner->getAttrOfType<ArrayAttr>("ac.domain_params");
    if (!domain || !declared || !llvm::is_contained(declared, domain))
      return op->emitOpError(
          "resource endpoint must name a declared logical domain");
  }
  for (auto field : edges) {
    auto edge = op->getAttrOfType<StringAttr>(field);
    if (!edge || (edge.getValue() != "posedge" && edge.getValue() != "negedge"))
      return op->emitOpError("resource endpoint requires an explicit edge");
  }
  auto polarity = op->getAttrOfType<StringAttr>("reset_polarity");
  if (!polarity ||
      (polarity.getValue() != "high" && polarity.getValue() != "low"))
    return op->emitOpError("resource requires whole-reset polarity");
  return success();
}
} // namespace
LogicalResult verifyCDCRefinementContract(Operation *operation) {
  auto witness = dyn_cast<CDCRefinementOp>(operation);
  auto rawKind = operation->getAttrOfType<StringAttr>("logical_kind");
  auto rawImplementation =
      operation->getAttrOfType<IntegerAttr>("implementation");
  if (!witness || !rawKind || !rawImplementation ||
      !rawImplementation.getType().isSignlessInteger(64))
    return operation->emitOpError("invalid typed CDC witness attributes");
  bool reg = witness.getLogicalKind() == "async_reg";
  if (witness.getLogicalKind() != "async_reg" &&
      witness.getLogicalKind() != "async_fifo")
    return witness.emitOpError("unknown logical CDC kind");
  if (witness.getImplementation() != 1 ||
      witness.getNumOperands() != (reg ? 5u : 9u) ||
      witness->getNumResults() || witness->getNumRegions() ||
      witness->getNumSuccessors())
    return witness.emitOpError("invalid CDC witness contract");
  if (failed(
          resource(witness,
                   reg ? ArrayRef<StringRef>{"source_domain", "target_domain"}
                       : ArrayRef<StringRef>{"write_domain", "read_domain"},
                   reg ? ArrayRef<StringRef>{"target_edge"}
                       : ArrayRef<StringRef>{"write_edge", "read_edge"})))
    return failure();
  auto source = witness->getAttrOfType<DictionaryAttr>("ac.cdc_source");
  if (!source || source.size() != 2 ||
      !source.getAs<StringAttr>("instance") ||
      !source.getAs<StringAttr>("declaration") ||
      source.getAs<StringAttr>("instance").getValue().empty() ||
      source.getAs<StringAttr>("declaration").getValue().empty() ||
      witness->hasAttr("callee") || witness->hasAttr("parameters") ||
      witness->hasAttr("type_arguments"))
    return witness.emitOpError("requires exact inert source provenance");
  for (auto [index, value] : llvm::enumerate(witness.getOperands())) {
    if (!reg && (index == 3 || index == 6))
      continue;
    if (failed(verifyLiteralControlWidth(witness, value,
                                         "CDC witness control")))
      return failure();
  }
  if (!reg) {
    auto depth = witness->getAttrOfType<IntegerAttr>("depth");
    if (!depth || !depth.getType().isSignlessInteger(64) ||
        depth.getInt() <= 0 ||
        !areEquivalentHardwareTypes(witness.getOperand(3).getType(),
                                    witness.getOperand(6).getType()))
      return witness.emitOpError("invalid FIFO witness token/capacity");
  }
  return success();
}
LogicalResult CDCRefinementOp::verify() {
  if (failed(verifyCDCRefinementContract(*this)))
    return failure();
  return success();
}
void CDCRefinementOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(MemoryEffects::Read::get(), SourceStateResource::get());
}
} // namespace acir::ac
