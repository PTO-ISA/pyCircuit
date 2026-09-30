#include "FinalEmitCppHierarchy.h"
#include "FinalCppEmission.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace acir::compiler {
namespace {

std::string cppType(unsigned width) {
  return width == 1 ? "bool" : "::std::uint64_t";
}

FailureOr<const FinalProgram::StateAliasSnapshot *>
findAlias(const FinalProgram &program, InstanceView *view, Value handle,
          ac::detail::EmitError emitError) {
  const FinalProgram::StateAliasSnapshot *result = nullptr;
  for (const auto &alias : program.stateAliases())
    if (alias.view == view && alias.handle == handle) {
      if (result)
        return emitError() << "C++ frozen handle alias is ambiguous";
      result = &alias;
    }
  if (!result)
    return emitError() << "C++ frozen handle alias is missing";
  return result;
}

FailureOr<unsigned> carrierWidth(const FinalProgram &program,
                                 DictionaryAttr stateID,
                                 ac::detail::EmitError emitError) {
  for (const auto &carrier : program.stateCarriers())
    if (carrier.stateID == stateID)
      return carrier.width;
  return emitError() << "C++ hierarchy cannot resolve StateID carrier";
}

FailureOr<SmallVector<InputPort>>
frozenInputs(const FinalProgram &program, size_t instanceOrdinal,
             ac::detail::EmitError emitError) {
  const auto &instance = program.instances()[instanceOrdinal];
  ArrayAttr ports = instance.module->getAttrOfType<ArrayAttr>("ac.ports");
  ac::ModuleOp module = instance.module;
  if (!ports || module.getBody().empty() ||
      module.getBody().front().getNumArguments() != ports.size() + 2)
    return emitError() << "C++ frozen formal argument layout is incomplete";
  SmallVector<InputPort> result;
  for (auto [portIndex, raw] : llvm::enumerate(ports)) {
    auto port = dyn_cast<DictionaryAttr>(raw);
    auto role = port ? port.getAs<StringAttr>("role") : StringAttr();
    if (!port || !role)
      return emitError() << "C++ final PortSlot is malformed";
    if (role.getValue() != "current")
      continue;
    Value formalHandle = module.getBody().front().getArgument(portIndex + 2);
    auto alias = findAlias(program, instance.view, formalHandle, emitError);
    if (failed(alias) || !(*alias)->formalState ||
        !sameFormal((*alias)->formalState, port))
      return emitError() << "C++ current PortSlot alias changed";
    auto parameter = port.getAs<StringAttr>("parameter");
    auto width = carrierWidth(program, (*alias)->stateID, emitError);
    if (!parameter || failed(width))
      return failure();
    result.push_back({portIndex, result.size(), *width, (*alias)->formalState,
                      (*alias)->stateID});
  }
  return result;
}

} // namespace

bool sameFormal(DictionaryAttr left, DictionaryAttr right) {
  return left && right && left.get("parameter") == right.get("parameter") &&
         left.get("ordinal") == right.get("ordinal");
}

FailureOr<std::string> actualReadBindingKey(const FinalProgram &program,
                                            size_t instanceOrdinal,
                                            Value handle,
                                            ac::detail::EmitError emitError) {
  const auto &instance = program.instances()[instanceOrdinal];
  auto alias = findAlias(program, instance.view, handle, emitError);
  if (failed(alias))
    return failure();
  if ((*alias)->formalState) {
    auto parameter = (*alias)->formalState.getAs<StringAttr>("parameter");
    auto ordinal = (*alias)->formalState.get("ordinal");
    if (!parameter || !ordinal)
      return emitError() << "C++ formal read identity is malformed";
    std::string key = "formal:" + parameter.getValue().str() + ":";
    llvm::raw_string_ostream output(key);
    output << ordinal;
    output.flush();
    return key;
  }
  auto owned = llvm::find_if(instance.ownedStateOrdinals, [&](size_t global) {
    return program.proposals().states[global].stateID == (*alias)->stateID;
  });
  if (owned == instance.ownedStateOrdinals.end())
    return emitError() << "C++ child input is neither owned nor a formal";
  return "owned:" + std::to_string(owned - instance.ownedStateOrdinals.begin());
}

FailureOr<std::string>
frozenPlacementName(const FinalProgram::FinalInstanceSnapshot &instance,
                    ac::detail::EmitError emitError) {
  if (!instance.placement)
    return emitError() << "C++ child instance has no frozen placement";
  auto name = instance.placement->getAttrOfType<StringAttr>("name");
  if (!name || name.getValue().empty())
    return emitError() << "C++ child placement has no stable source name";
  return name.getValue().str();
}

std::string cppStringLiteral(StringRef value) {
  std::string text;
  llvm::raw_string_ostream output(text);
  output << '"';
  output.write_escaped(value, /*UseHexEscapes=*/false);
  output << '"';
  output.flush();
  return text;
}

LogicalResult verifySpecGroupLayouts(const FinalProgram &program,
                                     ArrayRef<SpecGroup> groups,
                                     ArrayRef<size_t> defByInstance,
                                     ac::detail::EmitError emitError) {
  auto instances = program.instances();
  for (const SpecGroup &group : groups) {
    const auto &representative = instances[group.instances.front()];
    auto inputs = frozenInputs(program, representative.ordinal, emitError);
    if (failed(inputs))
      return failure();
    for (size_t ordinal : group.instances) {
      const auto &candidate = instances[ordinal];
      if (candidate.ownedStateOrdinals.size() !=
              representative.ownedStateOrdinals.size() ||
          candidate.proposalContributionOrdinals.size() !=
              representative.proposalContributionOrdinals.size() ||
          candidate.checkOrdinals.size() !=
              representative.checkOrdinals.size() ||
          candidate.observationOrdinals.size() !=
              representative.observationOrdinals.size() ||
          candidate.childOrdinals.size() != representative.childOrdinals.size())
        return emitError()
               << "SpecKey instances have different frozen local shapes";
      auto candidateInputs = frozenInputs(program, ordinal, emitError);
      if (failed(candidateInputs) || candidateInputs->size() != inputs->size())
        return emitError() << "SpecKey instances have different formal inputs";
      for (auto [index, input] : llvm::enumerate(*inputs))
        if ((*candidateInputs)[index].portIndex != input.portIndex ||
            (*candidateInputs)[index].width != input.width ||
            !sameFormal((*candidateInputs)[index].formalState,
                        input.formalState))
          return emitError() << "SpecKey formal input layout changed";
      for (auto [index, global] :
           llvm::enumerate(representative.ownedStateOrdinals)) {
        auto left = carrierWidth(
            program, program.proposals().states[global].stateID, emitError);
        auto right =
            carrierWidth(program,
                         program.proposals()
                             .states[candidate.ownedStateOrdinals[index]]
                             .stateID,
                         emitError);
        if (failed(left) || failed(right) || *left != *right)
          return emitError() << "SpecKey owned state widths changed";
      }
      for (auto [index, contribution] :
           llvm::enumerate(representative.proposalContributionOrdinals)) {
        auto left = dyn_cast<IntegerType>(
            program.proposals().contributions[contribution].data.getType());
        auto right = dyn_cast<IntegerType>(
            program.proposals()
                .contributions[candidate.proposalContributionOrdinals[index]]
                .data.getType());
        if (!left || !right || left.getWidth() != right.getWidth())
          return emitError() << "SpecKey proposal widths changed";
      }
      for (auto [index, child] : llvm::enumerate(representative.childOrdinals))
        if (defByInstance[child] !=
            defByInstance[candidate.childOrdinals[index]])
          return emitError() << "SpecKey child specialization layout changed";
      for (auto [position, representativeChild] :
           llvm::enumerate(representative.childOrdinals)) {
        size_t candidateChild = candidate.childOrdinals[position];
        auto leftName =
            frozenPlacementName(instances[representativeChild], emitError);
        auto rightName =
            frozenPlacementName(instances[candidateChild], emitError);
        if (failed(leftName) || failed(rightName) || *leftName != *rightName)
          return emitError() << "SpecKey child placement names changed";
        auto leftInputs = frozenInputs(program, representativeChild, emitError);
        auto rightInputs = frozenInputs(program, candidateChild, emitError);
        ac::InstanceOp leftPlacement = instances[representativeChild].placement;
        ac::InstanceOp rightPlacement = instances[candidateChild].placement;
        if (failed(leftInputs) || failed(rightInputs) ||
            leftInputs->size() != rightInputs->size() || !leftPlacement ||
            !rightPlacement ||
            leftPlacement.getInputs().size() != leftInputs->size() ||
            rightPlacement.getInputs().size() != rightInputs->size())
          return emitError() << "SpecKey child current input layout changed";
        for (auto [inputIndex, input] : llvm::enumerate(*leftInputs)) {
          if (input.portIndex != (*rightInputs)[inputIndex].portIndex ||
              input.width != (*rightInputs)[inputIndex].width ||
              !sameFormal(input.formalState,
                          (*rightInputs)[inputIndex].formalState))
            return emitError() << "SpecKey child input PortSlot changed";
          auto leftBinding = actualReadBindingKey(
              program, representative.ordinal,
              leftPlacement.getInputs()[inputIndex], emitError);
          auto rightBinding = actualReadBindingKey(
              program, ordinal, rightPlacement.getInputs()[inputIndex],
              emitError);
          if (failed(leftBinding) || failed(rightBinding) ||
              *leftBinding != *rightBinding)
            return emitError()
                   << "SpecKey child placement actual read bindings changed";
        }
      }
    }
  }
  return success();
}

FailureOr<ChildReadBinding> childReadBinding(
    const FinalProgram &program, size_t parentOrdinal, size_t childPosition,
    size_t childOrdinal, const InputPort &childInput, Value actualHandle,
    ArrayRef<InputPort> parentInputs, DenseMap<Attribute, size_t> parentOwned,
    const CppEmissionNames &names, size_t parentDefinition,
    ac::detail::EmitError emitError) {
  const auto &parent = program.instances()[parentOrdinal];
  if (childPosition >= parent.childOrdinals.size() ||
      parent.childOrdinals[childPosition] != childOrdinal)
    return emitError() << "C++ child placement position is not frozen";
  auto alias = findAlias(program, parent.view, actualHandle, emitError);
  if (failed(alias))
    return failure();
  if ((*alias)->stateID != childInput.stateID)
    return emitError()
           << "C++ placement actual StateID disagrees with frozen child formal";
  auto width = carrierWidth(program, childInput.stateID, emitError);
  if (failed(width) || *width != childInput.width)
    return emitError() << "C++ child formal width disagrees with carrier";

  const std::string childMember = names.child(parentDefinition, childPosition);
  if (childMember.empty())
    return emitError() << "C++ child has no emitted source member name";
  const std::string childInputMember =
      childMember + "." +
      names.input(names.defByInstance[childOrdinal], childInput.portIndex);
  if (childInputMember == childMember + ".")
    return emitError() << "C++ child input has no emitted source member name";
  if ((*alias)->formalState) {
    const InputPort *parentInput = nullptr;
    for (const InputPort &input : parentInputs) {
      if (!sameFormal(input.formalState, (*alias)->formalState))
        continue;
      if (parentInput)
        return emitError() << "C++ placement actual maps to multiple formals";
      parentInput = &input;
    }
    if (!parentInput || parentInput->width != childInput.width)
      return emitError()
             << "C++ placement actual formal has no compatible current input";
    std::string parentInputName =
        names.input(parentDefinition, parentInput->portIndex);
    if (parentInputName.empty())
      return emitError()
             << "C++ parent input has no emitted source member name";
    return ChildReadBinding{parentInputName, childInputMember +
                                                 ".SameBinding(" +
                                                 parentInputName + ")"};
  }
  auto owned = parentOwned.find((*alias)->stateID);
  if (owned == parentOwned.end())
    return emitError() << "C++ placement actual is not parent-owned/readable";
  const std::string storage = names.state(parentDefinition, "q", owned->second);
  return ChildReadBinding{
      "::ReadView<" + cppType(childInput.width) + ">(&" + storage + ".Read())",
      childInputMember + ".IsBoundTo(&" + storage + ".Read())"};
}

FailureOr<SmallVector<ChildReadBinding>> childReadBindings(
    const FinalProgram &program, size_t parentOrdinal, size_t childPosition,
    size_t childOrdinal, ArrayRef<InputPort> childInputs,
    ArrayRef<Value> actualHandles, ArrayRef<InputPort> parentInputs,
    DenseMap<Attribute, size_t> parentOwned, const CppEmissionNames &names,
    size_t parentDefinition, ac::detail::EmitError emitError) {
  if (actualHandles.size() != childInputs.size())
    return emitError() << "C++ child placement input arity changed";
  SmallVector<ChildReadBinding> result;
  for (auto [index, input] : llvm::enumerate(childInputs)) {
    auto binding =
        childReadBinding(program, parentOrdinal, childPosition, childOrdinal,
                         input, actualHandles[index], parentInputs, parentOwned,
                         names, parentDefinition, emitError);
    if (failed(binding))
      return failure();
    result.push_back(std::move(*binding));
  }
  return result;
}

} // namespace acir::compiler
