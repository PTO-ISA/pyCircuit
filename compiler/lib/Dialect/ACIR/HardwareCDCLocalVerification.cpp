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
                       ArrayRef<StringRef> edges, bool independent) {
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
      version.getInt() != (independent ? 2 : 1))
    return op->emitOpError("requires canonical resource_contract : i64");
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
  for (StringRef name : independent
      ? SmallVector<StringRef>{"write_reset_polarity", "read_reset_polarity"}
      : SmallVector<StringRef>{"reset_polarity"}) {
    auto polarity = op->getAttrOfType<StringAttr>(name);
    if (!polarity ||
        (polarity.getValue() != "high" && polarity.getValue() != "low"))
      return op->emitOpError("resource requires explicit reset polarity");
  }
  if (independent && (op->hasAttr("reset_polarity") ||
      op->getAttrOfType<StringAttr>("reset_policy") !=
          StringAttr::get(op->getContext(), "flush_on_either")))
    return op->emitOpError("independent FIFO requires flush_on_either reset policy");
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
  auto rawContract = operation->getAttrOfType<IntegerAttr>("resource_contract");
  bool independent = rawContract && rawContract.getInt() == 2;
  if (witness.getLogicalKind() != "async_reg" &&
      witness.getLogicalKind() != "async_fifo")
    return witness.emitOpError("unknown logical CDC kind");
  if ((reg && independent) ||
      witness.getImplementation() != (independent ? 3 : 2) ||
      witness.getNumOperands() != (reg ? 5u : (independent ? 10u : 9u)) ||
      witness->getNumResults() || witness->getNumRegions() ||
      witness->getNumSuccessors())
    return witness.emitOpError("invalid CDC witness contract");
  if (failed(
          resource(witness,
                   reg ? ArrayRef<StringRef>{"source_domain", "target_domain"}
                       : ArrayRef<StringRef>{"write_domain", "read_domain"},
                   reg ? ArrayRef<StringRef>{"target_edge"}
                       : ArrayRef<StringRef>{"write_edge", "read_edge"},
                   independent)))
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
  unsigned payload = independent ? 4 : 3;
  for (auto [index, value] : llvm::enumerate(witness.getOperands())) {
    if (!reg && (index == payload || index == payload + 3))
      continue;
    if (failed(verifyLiteralControlWidth(witness, value,
                                         "CDC witness control")))
      return failure();
  }
  if (!reg) {
    auto depth = witness->getAttrOfType<StaticExprAttr>("depth");
    if (!depth ||
        !areEquivalentHardwareTypes(witness.getOperand(payload).getType(),
                                    witness.getOperand(payload + 3).getType()))
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
