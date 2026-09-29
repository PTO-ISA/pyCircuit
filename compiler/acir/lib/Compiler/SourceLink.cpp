#include "SourceLink.h"
#include "CheckGraph.h"
#include "ModuleGraph.h"
#include "ObservationGraph.h"
#include "ProposalGraph.h"

#include "mlir/IR/Verifier.h"
#include "llvm/ADT/DenseMap.h"

using namespace mlir;

namespace acir::compiler {
namespace {

LogicalResult preflightSourceLinkPair(SourceLinkUnit unit, unsigned index,
                                      ac::detail::EmitError emitError) {
  auto headerOwner =
      unit.header->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto bodyOwner = unit.body->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto headerKind = unit.header->getAttrOfType<StringAttr>("ac.unit_kind");
  auto bodyKind = unit.body->getAttrOfType<StringAttr>("ac.unit_kind");
  auto headerStage = unit.header->getAttrOfType<StringAttr>("ac.stage");
  auto bodyStage = unit.body->getAttrOfType<StringAttr>("ac.stage");
  if (failed(ac::detail::verifySourceOwner(headerOwner, emitError)) ||
      failed(ac::detail::verifySourceOwner(bodyOwner, emitError)) ||
      headerOwner != bodyOwner)
    return emitError() << "source link unit[" << index
                       << "] body/header SourceOwner mismatch";
  if (!headerKind || headerKind.getValue() != "interface" || !headerStage ||
      headerStage.getValue() != "source")
    return emitError() << "source link unit[" << index
                       << "] owning header stage/unit_kind is invalid";
  if (!bodyKind ||
      (bodyKind.getValue() != "implementation" &&
       bodyKind.getValue() != "declarations") ||
      !bodyStage || bodyStage.getValue() != "source")
    return emitError() << "source link unit[" << index
                       << "] body stage/unit_kind is invalid";
  for (StringRef name : {"ac.interfaces", "ac.exports", "ac.import_bindings"})
    if (!unit.header->getAttr(name) ||
        unit.header->getAttr(name) != unit.body->getAttr(name))
      return emitError() << "source link unit[" << index
                         << "] body/header metadata differs for " << name;
  return success();
}

} // namespace

FailureOr<SourceHeaderRegistry>
admitSourceLinkUnits(llvm::ArrayRef<SourceLinkUnit> units,
                     ac::detail::EmitError emitError) {
  if (units.empty())
    return emitError() << "source link requires at least one complete unit";

  SmallVector<ModuleOp> headers;
  headers.reserve(units.size());
  llvm::DenseMap<Attribute, unsigned> ownerIndices;
  for (auto [index, unit] : llvm::enumerate(units)) {
    if (!unit.body || !unit.header)
      return emitError() << "source link unit[" << index
                         << "] requires both body and owning header";
    if (failed(preflightSourceLinkPair(unit, index, emitError)))
      return failure();
    auto owner = unit.header->getAttrOfType<DictionaryAttr>("ac.source_owner");
    auto inserted = ownerIndices.try_emplace(owner, index);
    if (!inserted.second)
      return emitError() << "duplicate source link SourceOwner in unit["
                         << inserted.first->second << "] and unit[" << index
                         << ']';
    headers.push_back(unit.header);
  }

  // Build one authority registry for the complete link input. This closes the
  // provider, namespace, declaration, and helper snapshots before any body is
  // passed to the native verifier or a later evaluator.
  auto registry = SourceHeaderRegistry::create(headers, emitError);
  if (failed(registry))
    return failure();

  for (auto [index, unit] : llvm::enumerate(units)) {
    if (failed(
            registry->verifyBodySnapshots(unit.body, unit.header, emitError)))
      return emitError() << "source link body/header admission failed for "
                            "unit["
                         << index << ']';
    if (failed(verify(unit.body)))
      return emitError() << "source link body failed native verification for "
                            "unit["
                         << index << ']';
  }
  auto moduleGraph = buildSourceModuleGraph(units, *registry, emitError);
  if (failed(moduleGraph))
    return emitError() << "source link module graph admission failed";
  auto checks = buildSourceCheckGraph(*moduleGraph, emitError);
  if (failed(checks) || failed(verifySourceChecks(*checks, emitError)))
    return emitError() << "source link check graph admission failed";
  auto proposals = buildSourceProposalGraph(*moduleGraph, &*checks, emitError);
  if (failed(proposals) || failed(verifySourceProposals(*proposals, emitError)))
    return emitError() << "source link proposal graph admission failed";
  auto observations = buildSourceObservationGraph(*moduleGraph, emitError);
  if (failed(observations) ||
      failed(verifySourceObservations(*observations, emitError)))
    return emitError() << "source link observation graph admission failed";
  return std::move(*registry);
}

} // namespace acir::compiler
