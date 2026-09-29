#include "FinalEmitVerilogSupport.h"
#include "llvm/Support/raw_ostream.h"
#include <tuple>

using namespace mlir;

namespace acir::compiler {
FailureOr<FinalVerilogEmission>
emitFinalVerilogPartsBody(const FinalProgram &program,
                          ac::detail::EmitError emitError) {
  auto instances = program.instances();
  if (instances.empty() || program.rootInstanceOrdinal() >= instances.size() ||
      program.postOrderInstanceOrdinals().size() != instances.size())
    return emitError()
           << "RTL emitter requires a complete frozen instance tree";

  SmallVector<size_t> familyFor;
  auto familiesOr = buildFamilies(program, familyFor, emitError);
  if (failed(familiesOr))
    return failure();
  SmallVector<Family, 0> families = std::move(*familiesOr);
  const size_t root = program.rootInstanceOrdinal();
  const Family &rootFamily = families[familyFor[root]];
  if (!rootFamily.current.empty() || !rootFamily.next.empty())
    return emitError()
           << "bounded RTL system top requires a portless selected root";

  SmallVector<DenseMap<size_t, size_t>> stateRanks(instances.size());
  SmallVector<DenseMap<size_t, size_t>> checkRanks(instances.size());
  SmallVector<DenseMap<size_t, size_t>> observationRanks(instances.size());
  DenseMap<Attribute, size_t> ownerByState;
  DenseMap<const InstanceView *, size_t> instanceByView;
  for (const auto &instance : instances) {
    auto rank = [](ArrayRef<size_t> values, DenseMap<size_t, size_t> &map) {
      for (auto [index, value] : llvm::enumerate(values))
        map.try_emplace(value, index);
    };
    rank(instance.ownedStateOrdinals, stateRanks[instance.ordinal]);
    rank(instance.checkOrdinals, checkRanks[instance.ordinal]);
    rank(instance.observationOrdinals, observationRanks[instance.ordinal]);
    instanceByView.try_emplace(instance.view, instance.ordinal);
    for (size_t state : instance.ownedStateOrdinals)
      ownerByState.try_emplace(program.proposals().states[state].stateID,
                               instance.ordinal);
  }

  SmallVector<const FinalProgram::StateCarrierSnapshot *> carriers;
  SmallVector<std::string> initials;
  for (const StateProposals &state : program.proposals().states) {
    auto carrier = findCarrier(program, state.stateID, emitError);
    if (failed(carrier))
      return failure();
    auto integer = dyn_cast<IntegerAttr>((*carrier)->initializer);
    if (!integer || (*carrier)->width == 0 || (*carrier)->width > 64 ||
        !(*carrier)->domain || (*carrier)->domain.getValue() != "default" ||
        !(*carrier)->view || !(*carrier)->view->module ||
        (*carrier)->view->module.getBody().empty() ||
        (*carrier)->view->module.getBody().front().getNumArguments() < 2 ||
        (*carrier)->clock !=
            (*carrier)->view->module.getBody().front().getArgument(0) ||
        (*carrier)->reset !=
            (*carrier)->view->module.getBody().front().getArgument(1))
      return emitError()
             << "RTL owned state control/type contract is unsupported";
    carriers.push_back(*carrier);
    initials.push_back(svLiteral(integer.getValue(), (*carrier)->width));
  }

  auto pathFromRoot = [&](size_t target) -> FailureOr<std::string> {
    if (target >= instances.size())
      return emitError() << "RTL instance path ordinal is invalid";
    SmallVector<size_t> path;
    for (size_t cursor = target; cursor != root;) {
      path.push_back(cursor);
      if (!instances[cursor].parentOrdinal)
        return emitError() << "RTL instance is detached from frozen root";
      cursor = *instances[cursor].parentOrdinal;
    }
    std::string result = "root_.";
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
      const size_t child = *it;
      const size_t parent = *instances[child].parentOrdinal;
      auto position = llvm::find(instances[parent].childOrdinals, child);
      if (position == instances[parent].childOrdinals.end())
        return emitError() << "RTL path child is absent from frozen parent";
      result +=
          "child_" +
          std::to_string(position - instances[parent].childOrdinals.begin()) +
          ".";
    }
    return result;
  };

  auto isBelow = [&](size_t child, size_t target) {
    size_t cursor = target;
    while (cursor != child) {
      if (!instances[cursor].parentOrdinal)
        return false;
      cursor = *instances[cursor].parentOrdinal;
    }
    return true;
  };

  std::string text = "/* verilator lint_off MULTITOP */\n";
  llvm::raw_string_ostream out(text);
  SmallVector<size_t> familyOrder(families.size());
  for (size_t index = 0; index < families.size(); ++index)
    familyOrder[index] = index;
  llvm::sort(familyOrder, [&](size_t left, size_t right) {
    auto depth = [&](size_t familyIndex) {
      const auto &instance = instances[families[familyIndex].representative];
      return instance.childOrdinals.size();
    };
    if (depth(left) != depth(right))
      return depth(left) < depth(right);
    return families[left].name < families[right].name;
  });

  for (size_t familyIndex : familyOrder) {
    const Family &family = families[familyIndex];
    const auto &instance = instances[family.representative];
    ac::ModuleOp module = instance.module;
    if (instance.childOrdinals.size() && module.getBody().empty())
      return emitError() << "RTL family module body is missing";
    DenseMap<Attribute, size_t> localOwned;
    for (auto [local, global] : llvm::enumerate(instance.ownedStateOrdinals))
      localOwned.try_emplace(program.proposals().states[global].stateID, local);
    SVExpressionEmitter expressions(program, instance.ordinal, family.current,
                                    std::move(localOwned), emitError);

    out << "module " << family.name << "(input logic clk, input logic reset, "
        << "input logic root_commit_ok, output logic subtree_error";
    for (const Port &port : family.current)
      out << ", input " << svType(port.width) << " " << port.parameter << "_q";
    for (const Port &port : family.next)
      out << ", output " << svType(port.width) << " " << port.parameter
          << "_d, output logic " << port.parameter << "_e";
    out << ");\n";
    for (auto [local, global] : llvm::enumerate(instance.ownedStateOrdinals)) {
      out << "  " << svType(carriers[global]->width) << " q" << local << ";\n  "
          << svType(carriers[global]->width) << " d" << local << ";\n  logic q"
          << local << "_e;\n"
          << "  localparam " << svType(carriers[global]->width) << " initial"
          << local << " = " << initials[global] << ";\n";
    }
    for (size_t observationGlobal : instance.observationOrdinals) {
      auto local = observationRanks[instance.ordinal].find(observationGlobal);
      if (local == observationRanks[instance.ordinal].end())
        return emitError() << "RTL observation rank is missing";
      const ObservationBinding &observation =
          program.observations().bindings[observationGlobal];
      if (observation.values.size() > 1 || !observation.rule ||
          observation.owner != instance.view)
        return emitError()
               << "RTL supports zero or one local observation value";
      out << "  logic [63:0] obs_value_" << local->second << ";\n"
          << "  logic obs_path_" << local->second << ";\n";
    }
    out << "  logic local_error;\n";
    for (auto [childPosition, child] :
         llvm::enumerate(instance.childOrdinals)) {
      (void)child;
      out << "  logic child_" << childPosition << "_subtree_error;\n";
    }
    out << "  always_comb begin\n    local_error = 1'b0;\n";
    for (size_t checkGlobal : instance.checkOrdinals) {
      const CheckBinding &check = program.checks().bindings[checkGlobal];
      if (check.owner != instance.view || !check.rule)
        return emitError()
               << "RTL check owner differs from its frozen instance";
      auto path = expressions.emit(check.path, *check.owner, check.rule);
      auto condition =
          expressions.emit(check.condition, *check.owner, check.rule);
      if (failed(path) || failed(condition))
        return failure();
      out << "    local_error = local_error | (" << *path << " & ~("
          << *condition << "));\n";
    }

    auto resolvePair = [&](DictionaryAttr stateID,
                           size_t from) -> FailureOr<PairText> {
      auto owner = ownerByState.find(stateID);
      auto stateIt = llvm::find_if(program.proposals().states,
                                   [&](const StateProposals &state) {
                                     return state.stateID == stateID;
                                   });
      if (owner == ownerByState.end() ||
          stateIt == program.proposals().states.end())
        return emitError() << "RTL proposal state has no frozen physical owner";
      const size_t globalState =
          static_cast<size_t>(stateIt - program.proposals().states.begin());
      const auto &fromInstance = instances[from];
      if (stateIt->commit.kind == CommitPairKind::Hold) {
        auto local = stateRanks[from].find(globalState);
        if (local != stateRanks[from].end())
          return PairText{"q" + std::to_string(local->second), "1'b0"};
        return PairText{std::to_string(carriers[globalState]->width) + "'d0",
                        "1'b0"};
      }
      if (stateIt->commit.kind != CommitPairKind::Forward ||
          stateIt->commit.contributions.size() != 1)
        return emitError() << "RTL emitter rejects ExclusiveMerge proposals";
      const size_t contributionIndex = stateIt->commit.contributions.front();
      if (contributionIndex >= program.proposals().contributions.size())
        return emitError() << "RTL proposal contribution index is invalid";
      const ProposalContribution &contribution =
          program.proposals().contributions[contributionIndex];
      auto producer = instanceByView.find(contribution.owner);
      if (producer == instanceByView.end())
        return emitError() << "RTL proposal producer is outside instance tree";
      if (producer->second == from) {
        auto data = expressions.emit(contribution.data, *contribution.owner,
                                     contribution.rule);
        auto enable = expressions.emit(contribution.enabled,
                                       *contribution.owner, contribution.rule);
        if (failed(data) || failed(enable))
          return failure();
        return PairText{*data, *enable};
      }
      for (auto [childPosition, child] :
           llvm::enumerate(fromInstance.childOrdinals)) {
        if (!isBelow(child, producer->second))
          continue;
        auto childPorts = portsFor(program, child, emitError);
        if (failed(childPorts))
          return failure();
        for (const Port &port : *childPorts)
          if (port.role == "next" && port.stateID == stateID)
            return PairText{"child_" + std::to_string(childPosition) + "_" +
                                port.parameter + "_d",
                            "child_" + std::to_string(childPosition) + "_" +
                                port.parameter + "_e"};
      }
      return emitError()
             << "RTL cannot route a descendant proposal through its "
                "frozen formal next ports";
    };

    for (auto [local, global] : llvm::enumerate(instance.ownedStateOrdinals)) {
      auto pair = resolvePair(program.proposals().states[global].stateID,
                              instance.ordinal);
      if (failed(pair))
        return failure();
      out << "    d" << local << " = " << pair->data << ";\n"
          << "    q" << local << "_e = (" << pair->enable
          << ") & root_commit_ok;\n";
    }
    for (const Port &port : family.next) {
      auto pair = resolvePair(port.stateID, instance.ordinal);
      if (failed(pair))
        return failure();
      out << "    " << port.parameter << "_d = " << pair->data << ";\n"
          << "    " << port.parameter << "_e = " << pair->enable << ";\n";
    }
    for (size_t observationGlobal : instance.observationOrdinals) {
      const size_t local =
          observationRanks[instance.ordinal].lookup(observationGlobal);
      const ObservationBinding &observation =
          program.observations().bindings[observationGlobal];
      FailureOr<std::string> value =
          observation.values.empty()
              ? FailureOr<std::string>(std::string("64'd0"))
              : expressions.emit(observation.values.front(), *observation.owner,
                                 observation.rule);
      auto path = expressions.emit(observation.path, *observation.owner,
                                   observation.rule);
      if (failed(value) || failed(path))
        return failure();
      unsigned width = 64;
      if (!observation.values.empty()) {
        auto integer =
            dyn_cast<IntegerType>(observation.values.front().getType());
        if (!integer || integer.getWidth() == 0 || integer.getWidth() > 64)
          return emitError() << "RTL observation width is unsupported";
        width = integer.getWidth();
      }
      out << "    obs_value_" << local << " = {{" << (64 - width) << "{1'b0}}, "
          << *value << "};\n"
          << "    obs_path_" << local << " = " << *path << ";\n";
    }
    out << "  end\n"
        << expressions.declarations() << "  assign subtree_error = local_error";
    for (size_t childPosition = 0;
         childPosition < instance.childOrdinals.size(); ++childPosition)
      out << " | child_" << childPosition << ".subtree_error";
    out << ";\n";

    for (auto [childPosition, child] :
         llvm::enumerate(instance.childOrdinals)) {
      const auto &childInstance = instances[child];
      ac::InstanceOp placement = childInstance.placement;
      const Family &childFamily = families[familyFor[child]];
      auto childPorts = portsFor(program, child, emitError);
      if (failed(childPorts))
        return failure();
      for (const Port &port : *childPorts)
        if (port.role == "next")
          out << "  " << svType(port.width) << " child_" << childPosition << "_"
              << port.parameter << "_d;\n  logic child_" << childPosition << "_"
              << port.parameter << "_e;\n";
      out << "  " << childFamily.name << " child_" << childPosition << "(\n"
          << "    .clk(clk), .reset(reset), .root_commit_ok(root_commit_ok),\n"
          << "    .subtree_error(child_" << childPosition << "_subtree_error)";
      if (!childInstance.placement)
        return emitError() << "RTL child has no frozen placement operation";
      size_t currentInputIndex = 0;
      for (const Port &port : *childPorts) {
        if (port.role == "current") {
          if (currentInputIndex >= placement.getInputs().size())
            return emitError()
                   << "RTL child current formal is not parent-bound";
          Value actualHandle = placement.getInputs()[currentInputIndex++];
          auto value =
              expressions.emitStateHandle(actualHandle, *instance.view);
          if (failed(value))
            return failure();
          out << ",\n    ." << port.parameter << "_q(" << *value << ")";
        }
      }
      if (currentInputIndex != placement.getInputs().size())
        return emitError() << "RTL child current formal arity changed";
      for (const Port &port : *childPorts)
        if (port.role == "next")
          out << ",\n    ." << port.parameter << "_d(child_" << childPosition
              << "_" << port.parameter << "_d), ." << port.parameter
              << "_e(child_" << childPosition << "_" << port.parameter << "_e)";
      out << ");\n";
    }
    for (size_t local = 0; local < instance.ownedStateOrdinals.size(); ++local)
      out << "  always_ff @(posedge clk) begin\n"
          << "    if (reset) q" << local << " <= initial" << local << ";\n"
          << "    else if (q" << local << "_e) q" << local << " <= d" << local
          << ";\n  end\n";
    out << "endmodule\n\n";
  }

  out << "module FinalModel(input logic clk, input logic reset);\n"
         "  logic root_commit_ok;\n  logic root_subtree_error;\n"
         "  assign root_commit_ok = ~root_subtree_error;\n"
      << "  " << families[familyFor[root]].name
      << " root_(.clk(clk), .reset(reset), .root_commit_ok(root_commit_ok), "
         ".subtree_error(root_subtree_error));\n"
         "  wire global_permit = root_commit_ok;\n";
  for (auto [local, global] :
       llvm::enumerate(instances[root].ownedStateOrdinals))
    out << "  wire"
        << (carriers[global]->width == 1
                ? std::string()
                : " [" + std::to_string(carriers[global]->width - 1) + ":0]")
        << " q" << local << ";\n  assign q" << local << " = root_.q" << local
        << ";\n";
  for (const ObservationBinding &observation : program.observations().bindings)
    out << "  // observation " << observation.stableOrdinal
        << " kind=" << observation.kind.getValue() << "\n";
  out << "endmodule\n\n";

  struct Record {
    size_t binding = 0;
    size_t owner = 0;
    uint64_t registration = 0;
    uint64_t site = 0;
  };
  SmallVector<Record> records;
  for (auto [bindingIndex, observation] :
       llvm::enumerate(program.observations().bindings)) {
    if (!observation.owner || !observation.rule ||
        observation.values.size() > 1 ||
        observation.valueConstraints.size() != observation.values.size())
      return emitError() << "RTL supports zero or one scalar observation value";
    auto owner = instanceByView.find(observation.owner);
    if (owner == instanceByView.end())
      return emitError() << "RTL observation owner is outside frozen instances";
    std::optional<uint64_t> registration;
    uint64_t ordinal = 0;
    for (ac::RuleOp rule :
         observation.owner->module.getBody().front().getOps<ac::RuleOp>()) {
      if (rule == observation.rule)
        registration = ordinal;
      ++ordinal;
    }
    if (!registration)
      return emitError() << "RTL observation rule is outside frozen owner";
    if (!observation.values.empty()) {
      auto integer =
          dyn_cast<IntegerType>(observation.values.front().getType());
      if (!integer || integer.getWidth() == 0 || integer.getWidth() > 64)
        return emitError()
               << "RTL observations require scalar values up to 64 bits";
    }
    records.push_back({bindingIndex, owner->second, *registration,
                       observation.requiredIndex});
  }
  llvm::sort(records, [&](const Record &left, const Record &right) {
    return program.observations().bindings[left.binding].stableOrdinal <
           program.observations().bindings[right.binding].stableOrdinal;
  });
  // Role boundary: the hardware RTL artifact ends at the FinalModel top above.
  // Everything from here is the simulation observation wrapper that carries the
  // runtime-glue role and instantiates that top.
  out.flush();
  std::string glueText;
  llvm::raw_string_ostream glueOut(glueText);
  glueOut << "module FinalModelSim(input logic clk, input logic reset);\n"
             "  FinalModel dut(.clk(clk), .reset(reset));\n"
             "  logic [63:0] evaluation_epoch = 64'd0;\n"
             "  logic [63:0] captured_evaluation_epoch;\n"
             "  logic [63:0] captured_commit_epoch;\n";
  for (size_t index = 0; index < records.size(); ++index)
    glueOut << "  logic [63:0] captured_value_" << index << ";\n"
            << "  logic captured_valid_" << index << ";\n";
  glueOut << "  always @(posedge clk) begin\n"
             "    if (reset) begin\n"
             "      evaluation_epoch = 64'd0;\n"
             "      captured_evaluation_epoch = 64'd0;\n"
             "      captured_commit_epoch = 64'd0;\n";
  for (size_t index = 0; index < records.size(); ++index)
    glueOut << "      captured_value_" << index << " = 64'd0; captured_valid_"
            << index << " = 1'b0;\n";
  glueOut << "    end else if (dut.root_commit_ok) begin\n"
             "      captured_evaluation_epoch = evaluation_epoch;\n"
             "      captured_commit_epoch = evaluation_epoch + 64'd1;\n";
  for (auto [recordIndex, record] : llvm::enumerate(records)) {
    auto local = observationRanks[record.owner].find(record.binding);
    if (local == observationRanks[record.owner].end())
      return emitError() << "RTL observation is outside its owner snapshot";
    auto instancePath = pathFromRoot(record.owner);
    if (failed(instancePath))
      return failure();
    const std::string modulePath = "dut." + *instancePath;
    glueOut << "      captured_value_" << recordIndex << " = " << modulePath
            << "obs_value_" << local->second << ";\n"
            << "      captured_valid_" << recordIndex << " = " << modulePath
            << "obs_path_" << local->second << ";\n";
  }
  glueOut << "      evaluation_epoch = evaluation_epoch + 64'd1;\n";
  for (auto [recordIndex, record] : llvm::enumerate(records)) {
    const ObservationBinding &observation =
        program.observations().bindings[record.binding];
    glueOut << "      if (captured_valid_" << recordIndex
            << ") $strobe(\"AC_OBS " << observation.stableOrdinal << " "
            << record.owner << " " << record.registration << " " << record.site
            << " %0d %0d %0d\", captured_value_" << recordIndex
            << ", captured_evaluation_epoch, captured_commit_epoch);\n";
  }
  glueOut << "    end else begin\n";
  for (size_t index = 0; index < records.size(); ++index)
    glueOut << "      captured_valid_" << index << " = 1'b0;\n";
  glueOut << "    end\n  end\nendmodule\n";
  glueOut.flush();
  return FinalVerilogEmission{std::move(text), std::move(glueText)};
}

} // namespace acir::compiler
