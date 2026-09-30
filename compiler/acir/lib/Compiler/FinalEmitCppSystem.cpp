#include "FinalEmitCppSystem.h"
#include "FinalEmitCppSupport.h"
#include "FinalRuntimeMetadata.h"

namespace acir::compiler {
mlir::LogicalResult emitFinalCppSystem(const FinalProgram &program,
                                       llvm::raw_ostream &out,
                                       const CppEmissionNames &names,
                                       ac::detail::EmitError emitError) {
  auto instances = program.instances();
  size_t root = program.rootInstanceOrdinal();
  SmallVector<size_t> defByInstance;
  auto groupsOr = buildSpecGroups(program, defByInstance, emitError);
  auto descriptorsOr = buildObservationDescriptors(program, emitError);
  if (failed(groupsOr) || failed(descriptorsOr))
    return failure();
  auto &groups = *groupsOr;
  auto &descriptors = *descriptorsOr;
  SmallVector<DenseMap<size_t, size_t>> stateRanks(instances.size()),
      contributionRanks(instances.size()), observationRanks(instances.size()),
      checkRanks(instances.size());
  DenseMap<Attribute, size_t> stateOwner;
  DenseMap<const InstanceView *, size_t> instanceOrdinals;
  for (const auto &instance : instances) {
    instanceOrdinals.try_emplace(instance.view, instance.ordinal);
    auto fill = [](ArrayRef<size_t> ordinals, DenseMap<size_t, size_t> &ranks) {
      for (auto [rank, global] : llvm::enumerate(ordinals))
        ranks.try_emplace(global, rank);
    };
    fill(instance.ownedStateOrdinals, stateRanks[instance.ordinal]);
    fill(instance.proposalContributionOrdinals,
         contributionRanks[instance.ordinal]);
    fill(instance.observationOrdinals, observationRanks[instance.ordinal]);
    fill(instance.checkOrdinals, checkRanks[instance.ordinal]);
    for (size_t state : instance.ownedStateOrdinals)
      stateOwner.try_emplace(program.proposals().states[state].stateID,
                             instance.ordinal);
  }
  auto objectPath = [&](size_t target) -> FailureOr<std::string> {
    SmallVector<size_t> path;
    for (size_t cursor = target; cursor != root;) {
      path.push_back(cursor);
      if (!instances[cursor].parentOrdinal)
        return emitError() << "instance path is detached from frozen root";
      cursor = *instances[cursor].parentOrdinal;
    }
    std::string result = "root_";
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
      size_t child = *it;
      size_t parent = *instances[child].parentOrdinal;
      const auto &actualParent = instances[parent];
      auto actualPos = llvm::find(actualParent.childOrdinals, child);
      const auto &rep =
          instances[groups[defByInstance[parent]].instances.front()];
      if (actualPos == actualParent.childOrdinals.end())
        return emitError() << "instance child is absent from frozen parent";
      size_t childPosition =
          static_cast<size_t>(actualPos - actualParent.childOrdinals.begin());
      if (childPosition >= rep.childOrdinals.size())
        return emitError() << "instance child is absent from SpecKey layout";
      result += "." + names.child(defByInstance[parent], childPosition);
    }
    return result;
  };
  const size_t rootDef = defByInstance[root];
  out << "using FinalModel = " << names.qualifiedType(rootDef)
      << ";\n"
         "class FinalSystem final : public gfsim::SimSystem {\npublic:\n"
         "  FinalSystem() : root_(nullptr, \"root\") {\n    if "
         "(!root_.RegisterAll(*this) || "
         "!AttachObservations(root_.observations_)) std::terminate();\n  }\n"
         "  FinalSystem(const FinalSystem &) = delete;\n"
         "  FinalSystem &operator=(const FinalSystem &) = delete;\n"
         "  FinalSystem(FinalSystem &&) = delete;\n"
         "  FinalSystem &operator=(FinalSystem &&) = delete;\n"
         "  gfsim::ObservationSlots &Observations() noexcept { return "
         "root_.observations_; }\nprotected:\n"
         "  bool FinalizeBuild() noexcept override { return "
         "root_.FreezeObjects(nullptr, \"root\"); }\n"
         "  bool Precommit(std::uint64_t epoch) noexcept override {\n"
         "    source_failure_ = {};\n"
         "    if (!root_.Validate(epoch) || !root_.observations_configured_) "
         "return false;\n";
  for (auto [global, check] : llvm::enumerate(program.checks().bindings)) {
    auto owner = instanceOrdinals.find(check.owner);
    if (owner == instanceOrdinals.end())
      return emitError() << "check owner is outside frozen instances";
    auto local = checkRanks[owner->second].find(global);
    auto path = objectPath(owner->second);
    auto location = runtimeMetadataJson(check.location, emitError);
    auto id = runtimeMetadataJson(check.checkID, emitError);
    if (local == checkRanks[owner->second].end() || failed(path) ||
        failed(location) || failed(id))
      return failure();
    std::string ownerText;
    llvm::raw_string_ostream ownerStream(ownerText);
    ownerStream << check.ownerRef;
    out << "    if (!" << *path << ".check_ok_" << local->second << "_) {\n"
        << "      source_failure_ = {gfsim::SimFailurePhase::Check, "
           "\"source_check_failed\", \"source "
        << check.kind.getValue() << " check failed\", "
        << cppStringLiteral(ownerText) << ", " << cppStringLiteral(*location)
        << ", " << cppStringLiteral(*id)
        << "};\n"
           "      return false;\n    }\n";
  }
  out << "    bool accepted = true;\n";
  for (auto [slot, descriptor] : llvm::enumerate(descriptors)) {
    size_t global = descriptor.bindingIndex;
    auto ownerIt =
        instanceOrdinals.find(program.observations().bindings[global].owner);
    if (ownerIt == instanceOrdinals.end())
      return emitError() << "observation owner is outside frozen instances";
    size_t owner = ownerIt->second;
    auto local = observationRanks[owner].find(global);
    if (local == observationRanks[owner].end())
      return emitError() << "observation is outside its frozen owner slice";
    auto path = objectPath(owner);
    if (failed(path))
      return failure();
    out << "    accepted &= root_.observations_.Stage(" << slot << ", " << *path
        << ".observation_" << local->second << "_value_, true, " << *path
        << ".observation_" << local->second << "_path_, epoch);\n";
  }
  out << "    if (!accepted) return false;\n";

  for (auto [stateGlobal, state] :
       llvm::enumerate(program.proposals().states)) {
    auto ownerFound = stateOwner.find(state.stateID);
    if (ownerFound == stateOwner.end())
      return emitError() << "proposal StateID has no physical owner instance";
    size_t owner = ownerFound->second;
    auto localState = stateRanks[owner].find(stateGlobal);
    auto ownerPath = objectPath(owner);
    if (localState == stateRanks[owner].end() || failed(ownerPath))
      return emitError() << "owned state is outside its frozen local slice";
    const size_t ownerDef = defByInstance[owner];
    const auto dName = names.state(ownerDef, "d", localState->second);
    const auto qName = names.state(ownerDef, "q", localState->second);
    const auto eName = names.state(ownerDef, "e", localState->second);
    if (state.commit.kind == CommitPairKind::Hold) {
      out << "    " << *ownerPath << "." << dName << " = " << *ownerPath << "."
          << qName << ".Read(); " << *ownerPath << "." << eName
          << " = false;\n";
    } else if (state.commit.kind == CommitPairKind::Forward) {
      if (state.commit.contributions.size() != 1)
        return emitError() << "Forward commit requires one contribution";
      size_t contributionGlobal = state.commit.contributions.front();
      if (contributionGlobal >= program.proposals().contributions.size())
        return emitError() << "Forward contribution ordinal is invalid";
      const auto &contribution =
          program.proposals().contributions[contributionGlobal];
      auto producerFound = instanceOrdinals.find(contribution.owner);
      if (producerFound == instanceOrdinals.end())
        return emitError() << "Forward producer is outside frozen instances";
      size_t producer = producerFound->second;
      auto localContribution =
          contributionRanks[producer].find(contributionGlobal);
      auto producerPath = objectPath(producer);
      if (localContribution == contributionRanks[producer].end() ||
          failed(producerPath))
        return emitError() << "Forward contribution is outside producer slice";
      out << "    " << *ownerPath << "." << dName << " = " << *producerPath
          << ".proposal_" << localContribution->second << "_.data; "
          << *ownerPath << "." << eName << " = " << *producerPath
          << ".proposal_" << localContribution->second << "_.enable;\n";
    } else {
      return emitError() << "C++ emitter does not support ExclusiveMerge";
    }
  }
  for (size_t ordinal : program.postOrderInstanceOrdinals()) {
    auto path = objectPath(ordinal);
    if (failed(path))
      return failure();
    out << "    " << *path << ".FreezeOwned(true);\n";
  }
  out << "    return true;\n  }\n"
         "  gfsim::SimFailureInfo DescribeFailure(gfsim::SimFailurePhase "
         "phase) const noexcept override {\n"
         "    if (phase == gfsim::SimFailurePhase::Check && "
         "!source_failure_.code.empty()) return source_failure_;\n"
         "    return gfsim::SimSystem::DescribeFailure(phase);\n  }\n"
         "private:\n  gfsim::SimFailureInfo source_failure_;\n  "
      << names.qualifiedType(rootDef) << " root_;\n};\n";
  return success();
}
} // namespace acir::compiler
