#include "FinalEmitCpp.h"
#include "FinalCppEmission.h"
#include "FinalEmitCppSupport.h"
#include "FinalEmitCppSystem.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"

#include <limits>
#include <tuple>

using namespace mlir;

namespace acir::compiler {

FailureOr<CppEmissionPlan>
buildCppEmissionPlan(const FinalProgram &program, bool sourceOwned,
                     ac::detail::EmitError emitError) {
  auto instances = program.instances();
  if (instances.empty() || program.rootInstanceOrdinal() >= instances.size() ||
      program.postOrderInstanceOrdinals().size() != instances.size())
    return emitError()
           << "C++ emitter requires a complete frozen instance tree";
  const size_t root = program.rootInstanceOrdinal();
  SmallVector<size_t> defByInstance;
  auto groupsOr = buildSpecGroups(program, defByInstance, emitError);
  if (failed(groupsOr))
    return failure();
  auto groups = std::move(*groupsOr);
  auto namesOr = buildCppEmissionNames(program, groups, defByInstance,
                                       sourceOwned, emitError);
  if (failed(namesOr))
    return failure();
  CppEmissionPlan plan;
  plan.names = std::move(*namesOr);
  plan.rootDefinition = plan.names.defByInstance[program.rootInstanceOrdinal()];
  plan.definitions.resize(groups.size());
  auto descriptorsOr = buildObservationDescriptors(program, emitError);
  if (failed(descriptorsOr))
    return failure();
  auto descriptors = std::move(*descriptorsOr);
  if (groups.empty() ||
      descriptors.size() != program.observations().bindings.size())
    return emitError() << "C++ emitter found an incomplete frozen graph";

  SmallVector<DenseMap<size_t, size_t>> stateRanks(instances.size());
  SmallVector<DenseMap<size_t, size_t>> contributionRanks(instances.size());
  SmallVector<DenseMap<size_t, size_t>> checkRanks(instances.size());
  SmallVector<DenseMap<size_t, size_t>> observationRanks(instances.size());
  for (const auto &instance : instances) {
    auto fill = [](ArrayRef<size_t> ordinals, DenseMap<size_t, size_t> &ranks) {
      for (auto [rank, global] : llvm::enumerate(ordinals))
        ranks.try_emplace(global, rank);
    };
    fill(instance.ownedStateOrdinals, stateRanks[instance.ordinal]);
    fill(instance.proposalContributionOrdinals,
         contributionRanks[instance.ordinal]);
    fill(instance.checkOrdinals, checkRanks[instance.ordinal]);
    fill(instance.observationOrdinals, observationRanks[instance.ordinal]);
  }

  DenseMap<Attribute, size_t> stateOwner;
  for (const auto &instance : instances)
    for (size_t state : instance.ownedStateOrdinals)
      if (state < program.proposals().states.size())
        stateOwner.try_emplace(program.proposals().states[state].stateID,
                               instance.ordinal);
  DenseMap<const InstanceView *, size_t> instanceOrdinals;
  for (const auto &instance : instances)
    instanceOrdinals.try_emplace(instance.view, instance.ordinal);

  SmallVector<const FinalProgram::StateCarrierSnapshot *> carriers;
  SmallVector<std::string> initials;
  for (const StateProposals &state : program.proposals().states) {
    auto carrier = findCarrier(program, state.stateID, emitError);
    if (failed(carrier))
      return failure();
    auto integer = dyn_cast<IntegerAttr>((*carrier)->initializer);
    if (!integer)
      return emitError() << "C++ state initializer is not scalar integer";
    carriers.push_back(*carrier);
    if ((*carrier)->width == 1)
      initials.push_back(integer.getValue().isOne() ? "true" : "false");
    else
      initials.push_back("UINT64_C(" + apIntLiteral(integer.getValue()) + ")");
  }

  if (failed(verifySpecGroupLayouts(program, groups, defByInstance, emitError)))
    return failure();

  llvm::raw_string_ostream out(plan.support);
  size_t eventCapacity =
      llvm::count_if(descriptors, [](const auto &item) { return !item.gauge; });
  out << "#include \"gfsim/ObservationSlot.h\"\n"
         "#include \"gfsim/SimDFF.h\"\n"
         "#include \"gfsim/SimSystem.h\"\n"
         "#include <array>\n#include <bit>\n#include <cstdint>\n"
         "#include <exception>\n#include <string>\n#include <utility>\n\n"
         "template<class T> class ReadView {\npublic:\n"
         "  explicit ReadView(const T *value) noexcept : value_(value) {}\n"
         "  const T &Read() const noexcept { return *value_; }\n"
         "  bool IsBoundTo(const T *value) const noexcept { return value_ == "
         "value; }\n"
         "  bool SameBinding(const ReadView<T> &other) const noexcept { return "
         "value_ == other.value_; }\n"
         "private:\n  const T *value_;\n};\n"
         "template<class T> struct ProposalSlot { T data{}; bool enable = "
         "false; bool completed = false; };\n"
         "static constexpr std::int64_t SignExtend(std::uint64_t value, "
         "unsigned width) noexcept {\n"
         "  if (width == 64) return std::bit_cast<std::int64_t>(value);\n"
         "  const std::uint64_t mask = (UINT64_C(1) << width) - 1;\n"
         "  const std::uint64_t sign = UINT64_C(1) << (width - 1);\n"
         "  value &= mask; if (value & sign) value |= ~mask;\n"
         "  return std::bit_cast<std::int64_t>(value);\n}\n"
         "static constexpr std::array<gfsim::ObservationDescriptor, "
      << descriptors.size() << "> kObservationDescriptors{{\n";
  for (const auto &descriptor : descriptors) {
    const auto &observation =
        program.observations().bindings[descriptor.bindingIndex];
    out << "  {" << observation.stableOrdinal << ", " << descriptor.ownerKey
        << ", " << descriptor.registrationKey << ", " << descriptor.siteKey
        << ", gfsim::ObservationKind::"
        << (descriptor.gauge ? "Gauge" : "Event") << "},\n";
  }
  out << "}};\nclass FinalSystem;\n";
  for (size_t def = 0; def < groups.size(); ++def)
    if (!sourceOwned)
      out << "class FinalModuleDef" << def << ";\n";

  SmallVector<size_t> defPostOrder =
      definitionPostOrder(program, defByInstance, groups.size());
  plan.definitionPostOrder = defPostOrder;
  SmallVector<SmallVector<size_t>> parentDefs(groups.size());
  for (size_t def = 0; def < groups.size(); ++def) {
    const auto &representative = instances[groups[def].instances.front()];
    for (size_t child : representative.childOrdinals) {
      size_t childDef = defByInstance[child];
      if (!llvm::is_contained(parentDefs[childDef], def))
        parentDefs[childDef].push_back(def);
    }
  }

  out.flush();
  for (size_t def : defPostOrder) {
    llvm::raw_string_ostream out(plan.definitions[def].declaration);
    const auto &representative = instances[groups[def].instances.front()];
    auto inputs = inputPorts(program, representative.ordinal, emitError);
    if (failed(inputs))
      return failure();
    StringRef classType = plan.names.methodType(def);
    if (sourceOwned)
      out << "template<> class " << plan.names.family(def) << "<>";
    else
      out << "class " << classType;
    out << " final : public gfsim::SimModule {\nprivate:\n"
           "  friend class ";
    out << (sourceOwned ? "::FinalSystem" : "FinalSystem") << ";\n";
    for (size_t parentDef : parentDefs[def])
      out << "  friend class "
          << (sourceOwned ? plan.names.qualifiedType(parentDef)
                          : plan.names.methodType(parentDef))
          << ";\n";
    out << "  gfsim::SimModule *const parent_;\n"
           "  const std::string instance_path_;\n";
    StringRef constructorName =
        sourceOwned ? plan.names.family(def) : classType;
    out << "  " << constructorName << "(const " << classType
        << " &) = delete;\n"
           "  "
        << classType << " &operator=(const " << classType
        << " &) = delete;\n"
           "  "
        << constructorName << "(" << classType
        << " &&) = delete;\n"
           "  "
        << classType << " &operator=(" << classType
        << " &&) = delete;\n"
           "  explicit "
        << constructorName
        << "(gfsim::SimModule *parent, std::string instancePath";
    for (const InputPort &input : *inputs)
      out << ", ReadView<" << cppType(input.width) << "> "
          << plan.names.inputParameter(def, input.portIndex);
    out << ");\n"
           "  void Build() override;\n"
           "  void Work(std::uint64_t epoch) override;\n"
           "  void Xfer() noexcept override;\n"
           "  void DiscardNext() noexcept override;\n"
           "  void Reset() noexcept override;\n"
           "  void ReportStat() override;\n"
           "  bool HasWork() const noexcept override;\n"
           "  void ClearScratch() noexcept;\n"
           "  bool Validate(std::uint64_t epoch) const noexcept;\n"
           "  bool ChecksPass() const noexcept;\n"
           "  bool RegisterAll(gfsim::SimSystem &system) noexcept;\n"
           "  bool FreezeObjects(gfsim::SimModule *expectedParent, const "
           "std::string &expectedPath) noexcept;\n"
           "  void FreezeOwned(bool permit) noexcept;\n";
    for (size_t state = 0; state < representative.ownedStateOrdinals.size();
         ++state) {
      size_t global = representative.ownedStateOrdinals[state];
      out << "  gfsim::SimDFFE<" << cppType(carriers[global]->width) << "> "
          << plan.names.state(def, "q", state) << "{" << initials[global]
          << "};\n"
          << "  " << cppType(carriers[global]->width) << " "
          << plan.names.state(def, "d", state) << "{}; bool "
          << plan.names.state(def, "e", state) << " = false; bool "
          << plan.names.state(def, "frozen_e", state) << " = false;\n";
    }
    for (const InputPort &input : *inputs)
      out << "  ReadView<" << cppType(input.width) << "> "
          << plan.names.input(def, input.portIndex) << ";\n";
    for (size_t contribution = 0;
         contribution < representative.proposalContributionOrdinals.size();
         ++contribution) {
      size_t global = representative.proposalContributionOrdinals[contribution];
      auto type = dyn_cast<IntegerType>(
          program.proposals().contributions[global].data.getType());
      if (!type || type.getWidth() == 0 || type.getWidth() > 64)
        return emitError()
               << "C++ emitter supports scalar proposals up to 64 bits";
      out << "  ProposalSlot<" << cppType(type.getWidth()) << "> proposal_"
          << contribution << "_;\n";
    }
    for (size_t check = 0; check < representative.checkOrdinals.size(); ++check)
      out << "  bool check_ok_" << check << "_ = false;\n";
    for (size_t observation = 0;
         observation < representative.observationOrdinals.size(); ++observation)
      out << "  gfsim::SlotValue observation_" << observation
          << "_value_{}; bool observation_" << observation
          << "_path_ = false; bool observation_" << observation
          << "_completed_ = false;\n";
    for (size_t childPosition = 0;
         childPosition < representative.childOrdinals.size(); ++childPosition) {
      size_t childOrdinal = representative.childOrdinals[childPosition];
      size_t childDef = defByInstance[childOrdinal];
      out << "  "
          << (sourceOwned ? plan.names.qualifiedType(childDef)
                          : plan.names.methodType(childDef))
          << " " << plan.names.child(def, childPosition) << ";\n";
    }
    if (def == defByInstance[root] && !inputs->empty())
      return emitError() << "root C++ class has unbound formal read inputs";
    if (def == defByInstance[root])
      out << "  gfsim::ObservationSlots observations_;\n"
             "  bool observations_configured_ = false;\n";
    out << "  std::uint64_t work_epoch_ = 0; bool work_valid_ = false;\n"
           "  bool commit_frozen_ = false; bool resetting_ = false;\n"
           "  bool objects_frozen_ = false;\n};\n";
  }

  for (size_t def : defPostOrder) {
    llvm::raw_string_ostream out(plan.definitions[def].constructor);
    const auto &representative = instances[groups[def].instances.front()];
    auto inputs = inputPorts(program, representative.ordinal, emitError);
    if (failed(inputs))
      return failure();
    StringRef classType = plan.names.methodType(def);
    StringRef constructorName =
        sourceOwned ? plan.names.family(def) : classType;
    out << classType << "::" << constructorName
        << "(gfsim::SimModule *parent, std::string instancePath";
    for (const InputPort &input : *inputs)
      out << ", ReadView<" << cppType(input.width) << "> "
          << plan.names.inputParameter(def, input.portIndex);
    out << ") : gfsim::SimModule(instancePath), parent_(parent), "
           "instance_path_(std::move(instancePath))";
    for (const InputPort &input : *inputs)
      out << ", " << plan.names.input(def, input.portIndex) << "("
          << plan.names.inputParameter(def, input.portIndex) << ")";
    DenseMap<Attribute, size_t> parentOwned;
    for (auto [local, global] :
         llvm::enumerate(representative.ownedStateOrdinals))
      parentOwned.try_emplace(program.proposals().states[global].stateID,
                              local);
    for (size_t childPosition = 0;
         childPosition < representative.childOrdinals.size(); ++childPosition) {
      size_t childOrdinal = representative.childOrdinals[childPosition];
      auto childInputs = inputPorts(program, childOrdinal, emitError);
      ac::InstanceOp placement = instances[childOrdinal].placement;
      if (failed(childInputs) || !placement)
        return emitError() << "C++ child placement/input snapshot is missing";
      SmallVector<Value> actualHandles(placement.getInputs().begin(),
                                       placement.getInputs().end());
      auto bindings =
          childReadBindings(program, representative.ordinal, childPosition,
                            childOrdinal, *childInputs, actualHandles, *inputs,
                            parentOwned, plan.names, def, emitError);
      if (failed(bindings))
        return failure();
      auto childName = frozenPlacementName(instances[childOrdinal], emitError);
      if (failed(childName))
        return failure();
      out << ", " << plan.names.child(def, childPosition)
          << "(this, instance_path_ + \"/\" + " << cppStringLiteral(*childName);
      for (const ChildReadBinding &binding : *bindings)
        out << ", " << binding.constructorExpression;
      out << ")";
    }
    out << " {\n";
    if (def == defByInstance[root])
      out << "  observations_configured_ = observations_.Configure("
          << "kObservationDescriptors, " << eventCapacity << ");\n";
    out << "}\n";
  }

  for (size_t def : defPostOrder) {
    llvm::raw_string_ostream out(plan.definitions[def].methods);
    const auto &representative = instances[groups[def].instances.front()];
    auto inputs = inputPorts(program, representative.ordinal, emitError);
    if (failed(inputs))
      return failure();
    DenseMap<Attribute, size_t> localOwned;
    for (auto [local, global] :
         llvm::enumerate(representative.ownedStateOrdinals))
      localOwned.try_emplace(program.proposals().states[global].stateID, local);
    CppExpressionEmitter expressions(program, representative.ordinal, *inputs,
                                     std::move(localOwned), plan.names, def,
                                     out, emitError);
    StringRef classType = plan.names.methodType(def);
    out << "void " << classType
        << "::Build() {}\n"
           "void "
        << classType
        << "::ClearScratch() noexcept {\n"
           "  work_valid_ = false; commit_frozen_ = false;\n";
    for (size_t state = 0; state < representative.ownedStateOrdinals.size();
         ++state)
      out << "  " << plan.names.state(def, "d", state) << " = {}; "
          << plan.names.state(def, "e", state) << " = false; "
          << plan.names.state(def, "frozen_e", state) << " = false;\n";
    for (size_t contribution = 0;
         contribution < representative.proposalContributionOrdinals.size();
         ++contribution)
      out << "  proposal_" << contribution << "_ = {};\n";
    for (size_t check = 0; check < representative.checkOrdinals.size(); ++check)
      out << "  check_ok_" << check << "_ = false;\n";
    for (size_t observation = 0;
         observation < representative.observationOrdinals.size(); ++observation)
      out << "  observation_" << observation << "_value_ = {}; "
          << "observation_" << observation << "_path_ = false; "
          << "observation_" << observation << "_completed_ = false;\n";
    out << "}\nvoid " << classType
        << "::Work(std::uint64_t epoch) {\n  ClearScratch(); work_epoch_ = "
           "epoch;\n";
    for (auto [local, global] :
         llvm::enumerate(representative.proposalContributionOrdinals)) {
      const auto &contribution = program.proposals().contributions[global];
      if (!contribution.owner || !contribution.rule)
        return emitError() << "proposal has no frozen owner rule";
      auto data = expressions.emit(contribution.data, *contribution.owner,
                                   contribution.rule);
      auto enabled = expressions.emit(contribution.enabled, *contribution.owner,
                                      contribution.rule);
      if (failed(data) || failed(enabled))
        return failure();
      out << "  proposal_" << local << "_.data = " << *data << "; proposal_"
          << local << "_.enable = " << *enabled << "; proposal_" << local
          << "_.completed = true;\n";
    }
    for (auto [local, global] : llvm::enumerate(representative.checkOrdinals)) {
      const auto &check = program.checks().bindings[global];
      auto path = expressions.emit(check.path, *check.owner, check.rule);
      auto condition =
          expressions.emit(check.condition, *check.owner, check.rule);
      if (failed(path) || failed(condition))
        return failure();
      out << "  check_ok_" << local << "_ = !(" << *path << " && !("
          << *condition << "));\n";
    }
    for (auto [local, global] :
         llvm::enumerate(representative.observationOrdinals)) {
      const auto &observation = program.observations().bindings[global];
      auto value = observationValue(observation, expressions, emitError);
      auto path = expressions.emit(observation.path, *observation.owner,
                                   observation.rule);
      if (failed(value) || failed(path))
        return failure();
      out << "  observation_" << local << "_value_ = " << *value
          << "; observation_" << local << "_path_ = " << *path
          << "; observation_" << local << "_completed_ = true;\n";
    }
    out << "  work_valid_ = true;\n}\n";
    out << "void " << classType
        << "::Xfer() noexcept {\n"
           "  if (resetting_) {\n";
    for (size_t state = 0; state < representative.ownedStateOrdinals.size();
         ++state)
      out << "    " << plan.names.state(def, "q", state) << ".Xfer();\n";
    out << "    resetting_ = false; ClearScratch(); return;\n  }\n"
           "  if (!commit_frozen_) std::terminate();\n";
    for (size_t state = 0; state < representative.ownedStateOrdinals.size();
         ++state)
      out << "  if (" << plan.names.state(def, "q", state)
          << ".HasPending() || !" << plan.names.state(def, "q", state)
          << ".Write(" << plan.names.state(def, "d", state) << ", "
          << plan.names.state(def, "frozen_e", state)
          << ")) std::terminate();\n";
    for (size_t state = 0; state < representative.ownedStateOrdinals.size();
         ++state)
      out << "  " << plan.names.state(def, "q", state) << ".Xfer();\n";
    out << "  ClearScratch();\n}\n"
           "void "
        << classType << "::DiscardNext() noexcept {\n";
    for (size_t state = 0; state < representative.ownedStateOrdinals.size();
         ++state)
      out << "  " << plan.names.state(def, "q", state) << ".DiscardNext();\n";
    out << "  ClearScratch();\n}\nvoid " << classType
        << "::Reset() noexcept {\n  DiscardNext();\n";
    for (size_t state = 0; state < representative.ownedStateOrdinals.size();
         ++state)
      out << "  " << plan.names.state(def, "q", state) << ".Reset();\n";
    out << "  resetting_ = true;\n}\nvoid " << classType
        << "::ReportStat() {}\nbool " << classType
        << "::HasWork() const noexcept { return "
        << (representative.ruleOrdinals.empty() ? "false" : "true")
        << "; }\n"
           "bool "
        << classType
        << "::Validate(std::uint64_t epoch) const noexcept {\n"
           "  if (!objects_frozen_ || !work_valid_ || work_epoch_ != epoch) "
           "return false;\n";
    for (size_t state = 0; state < representative.ownedStateOrdinals.size();
         ++state)
      out << "  if (" << plan.names.state(def, "q", state)
          << ".HasPending()) return false;\n";
    for (size_t contribution = 0;
         contribution < representative.proposalContributionOrdinals.size();
         ++contribution)
      out << "  if (!proposal_" << contribution
          << "_.completed) return false;\n";
    for (size_t observation = 0;
         observation < representative.observationOrdinals.size(); ++observation)
      out << "  if (!observation_" << observation
          << "_completed_) return false;\n";
    for (size_t childPosition = 0;
         childPosition < representative.childOrdinals.size(); ++childPosition) {
      out << "  if (!" << plan.names.child(def, childPosition)
          << ".Validate(epoch)) return false;\n";
    }
    out << "  return true;\n}\nbool " << classType
        << "::ChecksPass() const noexcept {\n";
    for (size_t check = 0; check < representative.checkOrdinals.size(); ++check)
      out << "  if (!check_ok_" << check << "_) return false;\n";
    for (size_t childPosition = 0;
         childPosition < representative.childOrdinals.size(); ++childPosition) {
      out << "  if (!" << plan.names.child(def, childPosition)
          << ".ChecksPass()) return false;\n";
    }
    out << "  return true;\n}\nbool " << classType
        << "::RegisterAll(gfsim::SimSystem &system) noexcept {\n"
           "  if (!system.AddModule(*this)) return false;\n";
    for (size_t childPosition = 0;
         childPosition < representative.childOrdinals.size(); ++childPosition) {
      out << "  if (!" << plan.names.child(def, childPosition)
          << ".RegisterAll(system)) return false;\n";
    }
    out << "  return true;\n}\nbool " << classType
        << "::FreezeObjects(gfsim::SimModule *expectedParent, const "
           "std::string &expectedPath) noexcept {\n"
           "  if (objects_frozen_ || parent_ != expectedParent || "
           "instance_path_ != expectedPath) "
           "return false;\n";
    DenseMap<Attribute, size_t> parentOwned;
    for (auto [local, global] :
         llvm::enumerate(representative.ownedStateOrdinals))
      parentOwned.try_emplace(program.proposals().states[global].stateID,
                              local);
    for (size_t childPosition = 0;
         childPosition < representative.childOrdinals.size(); ++childPosition) {
      size_t childOrdinal = representative.childOrdinals[childPosition];
      auto childInputs = inputPorts(program, childOrdinal, emitError);
      ac::InstanceOp placement = instances[childOrdinal].placement;
      if (failed(childInputs) || !placement)
        return emitError() << "C++ child freeze placement/input is missing";
      SmallVector<Value> actualHandles(placement.getInputs().begin(),
                                       placement.getInputs().end());
      auto childName = frozenPlacementName(instances[childOrdinal], emitError);
      if (failed(childName))
        return failure();
      auto bindings =
          childReadBindings(program, representative.ordinal, childPosition,
                            childOrdinal, *childInputs, actualHandles, *inputs,
                            parentOwned, plan.names, def, emitError);
      if (failed(bindings))
        return failure();
      for (const ChildReadBinding &binding : *bindings)
        out << "  if (!" << binding.freezeExpression << ") return false;\n";
      out << "  if (!" << plan.names.child(def, childPosition)
          << ".FreezeObjects(this, instance_path_ + \"/\" + "
          << cppStringLiteral(*childName) << ")) return false;\n";
    }
    out << "  objects_frozen_ = true; return true;\n}\n"
           "void "
        << classType << "::FreezeOwned(bool permit) noexcept {\n";
    for (size_t state = 0; state < representative.ownedStateOrdinals.size();
         ++state)
      out << "  " << plan.names.state(def, "frozen_e", state) << " = "
          << plan.names.state(def, "e", state) << " && permit;\n";
    out << "  commit_frozen_ = true;\n}\n";
  }

  out.flush();
  llvm::raw_string_ostream system(plan.system);
  if (failed(emitFinalCppSystem(program, system, plan.names, emitError)))
    return failure();
  system.flush();
  return plan;
}

FailureOr<std::string> emitFinalCppBody(const FinalProgram &program,
                                        ac::detail::EmitError emitError) {
  auto plan = buildCppEmissionPlan(program, /*sourceOwned=*/false, emitError);
  if (failed(plan))
    return failure();
  std::string text;
  llvm::raw_string_ostream out(text);
  out << plan->support << plan->legacyForwardDeclarations;
  for (size_t definition : plan->definitionPostOrder)
    out << plan->definitions[definition].declaration;
  for (size_t definition : plan->definitionPostOrder)
    out << plan->definitions[definition].constructor;
  for (size_t definition : plan->definitionPostOrder)
    out << plan->definitions[definition].methods;
  out << plan->system;
  out.flush();
  return text;
}
} // namespace acir::compiler
