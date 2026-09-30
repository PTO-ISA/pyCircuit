#ifndef ACIR_LIB_COMPILER_FINALEMITCPPSUPPORT_H
#define ACIR_LIB_COMPILER_FINALEMITCPPSUPPORT_H

#include "FinalCppEmission.h"
#include "FinalEmitCpp.h"
#include "FinalEmitCppHierarchy.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
#include <limits>
#include <string>
#include <tuple>

using namespace mlir;

namespace acir::compiler {
namespace {

struct ObservationDescriptorEmission {
  size_t bindingIndex = 0;
  uint64_t ownerKey = 0;
  uint64_t registrationKey = 0;
  uint64_t siteKey = 0;
  bool gauge = false;
};

::std::string attrText(Attribute attribute) {
  ::std::string text;
  llvm::raw_string_ostream output(text);
  output << attribute;
  output.flush();
  return text;
}

::std::string apIntLiteral(const APInt &value) {
  SmallString<32> text;
  value.toString(text, 10, /*signed=*/false, /*formatAsCLiteral=*/false);
  return text.str().str();
}

::std::string cppType(unsigned width) {
  return width == 1 ? "bool" : "::std::uint64_t";
}

FailureOr<const FinalProgram::StateCarrierSnapshot *>
findCarrier(const FinalProgram &program, DictionaryAttr stateID,
            ac::detail::EmitError emitError) {
  for (const auto &carrier : program.stateCarriers())
    if (carrier.stateID == stateID)
      return &carrier;
  return emitError() << "C++ emitter cannot resolve a frozen StateID carrier";
}

FailureOr<SmallVector<SpecGroup>>
buildSpecGroups(const FinalProgram &program, SmallVector<size_t> &defByInstance,
                ac::detail::EmitError emitError) {
  SmallVector<SpecGroup> groups;
  for (const auto &instance : program.instances()) {
    if (!instance.definition || !instance.staticArguments)
      return emitError() << "frozen instance lacks an exact SpecKey";
    auto found = llvm::find_if(groups, [&](const SpecGroup &group) {
      return group.definition == instance.definition &&
             group.staticArguments == instance.staticArguments;
    });
    if (found == groups.end()) {
      groups.push_back({instance.definition,
                        instance.staticArguments,
                        instance.definition.getValue().str() + "\n" +
                            attrText(instance.staticArguments),
                        {}});
      found = ::std::prev(groups.end());
    }
    found->instances.push_back(instance.ordinal);
  }
  llvm::sort(groups, [](const SpecGroup &left, const SpecGroup &right) {
    return left.orderKey < right.orderKey;
  });
  defByInstance.resize(program.instances().size());
  for (size_t def = 0; def < groups.size(); ++def)
    for (size_t instance : groups[def].instances)
      defByInstance[instance] = def;
  return groups;
}

SmallVector<size_t> definitionPostOrder(const FinalProgram &program,
                                        ArrayRef<size_t> defByInstance,
                                        size_t groupCount) {
  SmallVector<size_t> order;
  SmallVector<bool> seen(groupCount, false);
  for (size_t instanceOrdinal : program.postOrderInstanceOrdinals()) {
    size_t def = defByInstance[instanceOrdinal];
    if (!seen[def]) {
      seen[def] = true;
      order.push_back(def);
    }
  }
  return order;
}

FailureOr<SmallVector<ObservationDescriptorEmission>>
buildObservationDescriptors(const FinalProgram &program,
                            ac::detail::EmitError emitError) {
  SmallVector<ObservationDescriptorEmission> result;
  DenseMap<Attribute, uint64_t> owners;
  for (const auto &instance : program.instances())
    if (!instance.view || !instance.owner ||
        !owners.try_emplace(instance.owner, instance.ordinal).second)
      return emitError() << "C++ emitter requires unique module owners";
  for (auto [bindingIndex, observation] :
       llvm::enumerate(program.observations().bindings)) {
    if (!observation.owner || !observation.rule ||
        observation.values.size() > 1)
      return emitError() << "C++ emitter supports scalar observations only";
    auto owner = owners.find(observation.ownerRef);
    if (owner == owners.end())
      return emitError() << "observation owner is outside frozen modules";
    ::std::optional<uint64_t> ruleOrdinal;
    uint64_t ordinal = 0;
    for (ac::RuleOp rule :
         observation.owner->module.getBody().front().getOps<ac::RuleOp>()) {
      if (rule == observation.rule)
        ruleOrdinal = ordinal;
      ++ordinal;
    }
    if (!ruleOrdinal)
      return emitError() << "observation rule is outside its owner";
    result.push_back({bindingIndex, owner->second, *ruleOrdinal,
                      observation.requiredIndex,
                      observation.kind.getValue() == "report"});
  }
  llvm::sort(result, [](const auto &left, const auto &right) {
    return ::std::tie(left.ownerKey, left.registrationKey, left.siteKey,
                      left.bindingIndex) <
           ::std::tie(right.ownerKey, right.registrationKey, right.siteKey,
                      right.bindingIndex);
  });
  return result;
}

bool sameFormal(DictionaryAttr left, DictionaryAttr right) {
  return left && right && left.get("parameter") == right.get("parameter") &&
         left.get("ordinal") == right.get("ordinal");
}

FailureOr<SmallVector<InputPort>> inputPorts(const FinalProgram &program,
                                             size_t instanceOrdinal,
                                             ac::detail::EmitError emitError) {
  const auto &instance = program.instances()[instanceOrdinal];
  ArrayAttr ports = instance.module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!ports)
    return emitError() << "C++ emitter requires frozen final ports";
  ac::ModuleOp module = instance.module;
  if (module.getBody().empty() ||
      module.getBody().front().getNumArguments() != ports.size() + 2)
    return emitError() << "C++ frozen formal argument layout is incomplete";
  SmallVector<InputPort> result;
  for (auto [portIndex, rawPort] : llvm::enumerate(ports)) {
    auto port = dyn_cast<DictionaryAttr>(rawPort);
    auto role = port ? port.getAs<StringAttr>("role") : StringAttr();
    if (!port || !role)
      return emitError() << "C++ emitter found a malformed final port";
    if (role.getValue() != "current")
      continue;
    Value formalHandle = module.getBody().front().getArgument(portIndex + 2);
    auto alias = llvm::find_if(program.stateAliases(), [&](const auto &item) {
      return item.view == instance.view && item.handle == formalHandle;
    });
    if (alias == program.stateAliases().end())
      return emitError() << "current formal has no frozen StateID alias";
    if (!alias->formalState || !sameFormal(alias->formalState, port))
      return emitError() << "current formal handle disagrees with its PortSlot";
    auto carrier = findCarrier(program, alias->stateID, emitError);
    if (failed(carrier))
      return failure();
    result.push_back({portIndex, result.size(), (*carrier)->width,
                      alias->formalState, alias->stateID});
  }
  return result;
}

class CppExpressionEmitter {
public:
  CppExpressionEmitter(const FinalProgram &program, size_t instanceOrdinal,
                       ArrayRef<InputPort> inputs,
                       DenseMap<Attribute, size_t> localOwned,
                       const CppEmissionNames &names, size_t definitionIndex,
                       llvm::raw_ostream &statements,
                       ac::detail::EmitError emitError)
      : program(program), instanceOrdinal(instanceOrdinal), inputs(inputs),
        localOwned(::std::move(localOwned)), names(names),
        definitionIndex(definitionIndex), statements(statements),
        emitError(emitError) {}

  FailureOr<::std::string> emit(Value value, InstanceView &owner,
                                ac::RuleOp rule) {
    if (program.instances()[instanceOrdinal].view != &owner)
      return emitError()
             << "expression owner differs from SpecKey representative";
    active.clear();
    return emitImpl(value, owner, rule);
  }

private:
  FailureOr<::std::string> stateRead(Value handle, InstanceView &owner) {
    const FinalProgram::StateAliasSnapshot *resolved = nullptr;
    for (const auto &alias : program.stateAliases())
      if (alias.view == &owner && alias.handle == handle) {
        if (resolved)
          return emitError() << "C++ rule input handle has ambiguous aliases";
        resolved = &alias;
      }
    if (!resolved)
      return emitError() << "C++ rule input handle has no frozen StateID alias";

    if (resolved->formalState) {
      const InputPort *formal = nullptr;
      for (const InputPort &input : inputs) {
        if (!sameFormal(input.formalState, resolved->formalState))
          continue;
        if (formal)
          return emitError()
                 << "C++ rule input maps to multiple current formals";
        formal = &input;
      }
      if (!formal)
        return emitError()
               << "C++ rule input formal has no current family port";
      ::std::string inputName = names.input(definitionIndex, formal->portIndex);
      if (inputName.empty())
        return emitError() << "C++ rule input has no emitted source name";
      return inputName + ".Read()";
    }

    auto owned = localOwned.find(resolved->stateID);
    if (owned != localOwned.end())
      return names.state(definitionIndex, "q", owned->second) + ".Read()";
    return emitError()
           << "SpecKey rule input is neither owned nor a formal view";
  }

  FailureOr<::std::string> emitImpl(Value value, InstanceView &owner,
                                    ac::RuleOp rule) {
    if (!value)
      return emitError() << "C++ expression is missing an SSA value";
    if (auto found = cache.find(value); found != cache.end())
      return found->second;
    if (!active.insert(value).second)
      return emitError() << "C++ expression contains an SSA cycle";
    auto finish = [&](::std::string text) -> FailureOr<::std::string> {
      active.erase(value);
      auto type = dyn_cast<IntegerType>(value.getType());
      if (!type || !type.getWidth() || type.getWidth() > 64)
        return emitError() << "C++ SSA temporary requires finite integer type";
      ::std::string name = "v" + ::std::to_string(cache.size());
      statements << "  const " << cppType(type.getWidth()) << " " << name
                 << " = " << text << ";\n";
      cache.try_emplace(value, name);
      return name;
    };
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      if (!rule || argument.getOwner() != &rule.getBody().front() ||
          argument.getArgNumber() >= rule.getInputs().size())
        return emitError()
               << "C++ expression block argument is not a rule input";
      auto expression =
          stateRead(rule.getInputs()[argument.getArgNumber()], owner);
      if (failed(expression))
        return failure();
      return finish(*expression);
    }
    if (auto constant = value.getDefiningOp<arith::ConstantOp>()) {
      auto integer = dyn_cast<IntegerAttr>(constant.getValue());
      auto type = dyn_cast<IntegerType>(constant.getType());
      if (!integer || !type || type.getWidth() == 0 || type.getWidth() > 64)
        return emitError()
               << "C++ emitter supports bounded integer constants only";
      if (type.getWidth() == 1)
        return finish(integer.getValue().isOne() ? "true" : "false");
      return finish("UINT64_C(" + apIntLiteral(integer.getValue()) + ")");
    }
    auto binary = [&](Value lhsValue, Value rhsValue,
                      StringRef operation) -> FailureOr<::std::string> {
      auto lhs = emitImpl(lhsValue, owner, rule);
      auto rhs = emitImpl(rhsValue, owner, rule);
      if (failed(lhs) || failed(rhs))
        return failure();
      return finish("(" + *lhs + " " + operation.str() + " " + *rhs + ")");
    };
    if (auto operation = value.getDefiningOp<arith::AndIOp>())
      return binary(operation.getLhs(), operation.getRhs(), "&");
    if (auto operation = value.getDefiningOp<arith::OrIOp>())
      return binary(operation.getLhs(), operation.getRhs(), "|");
    if (auto operation = value.getDefiningOp<arith::XOrIOp>())
      return binary(operation.getLhs(), operation.getRhs(), "^");
    if (auto select = value.getDefiningOp<arith::SelectOp>()) {
      auto condition = emitImpl(select.getCondition(), owner, rule);
      auto yes = emitImpl(select.getTrueValue(), owner, rule);
      auto no = emitImpl(select.getFalseValue(), owner, rule);
      if (failed(condition) || failed(yes) || failed(no))
        return failure();
      return finish("(" + *condition + " ? " + *yes + " : " + *no + ")");
    }
    if (auto compare = value.getDefiningOp<arith::CmpIOp>()) {
      StringRef symbol;
      switch (compare.getPredicate()) {
      case arith::CmpIPredicate::eq:
        symbol = "==";
        break;
      case arith::CmpIPredicate::ne:
        symbol = "!=";
        break;
      case arith::CmpIPredicate::ult:
        symbol = "<";
        break;
      case arith::CmpIPredicate::ule:
        symbol = "<=";
        break;
      case arith::CmpIPredicate::ugt:
        symbol = ">";
        break;
      case arith::CmpIPredicate::uge:
        symbol = ">=";
        break;
      default:
        return emitError()
               << "finite comparison requires an unsigned predicate";
      }
      auto lhs = emitImpl(compare.getLhs(), owner, rule);
      auto rhs = emitImpl(compare.getRhs(), owner, rule);
      if (failed(lhs) || failed(rhs))
        return failure();
      return finish("(static_cast<::std::uint64_t>(" + *lhs + ") " +
                    symbol.str() + " static_cast<::std::uint64_t>(" + *rhs +
                    "))");
    }
    if (auto trunc = value.getDefiningOp<arith::TruncIOp>()) {
      auto input = emitImpl(trunc.getIn(), owner, rule);
      auto type = cast<IntegerType>(trunc.getType());
      if (failed(input) || type.getWidth() == 0 || type.getWidth() >= 64)
        return emitError()
               << "finite truncation requires a target narrower than i64";
      return finish(
          "(static_cast<::std::uint64_t>(" + *input + ") & UINT64_C(" +
          ::std::to_string((uint64_t(1) << type.getWidth()) - 1) + "))");
    }
    if (auto extension = value.getDefiningOp<arith::ExtUIOp>()) {
      auto input = emitImpl(extension.getIn(), owner, rule);
      if (failed(input))
        return failure();
      return finish("static_cast<::std::uint64_t>(" + *input + ")");
    }
    if (auto add = value.getDefiningOp<arith::AddIOp>()) {
      auto type = dyn_cast<IntegerType>(value.getType());
      if (!type || type.getWidth() == 0 || type.getWidth() > 64 ||
          add.getOverflowFlags() != arith::IntegerOverflowFlags::none)
        return emitError() << "C++ finite addition requires flag-free i1..i64";
      auto lhs = emitImpl(add.getLhs(), owner, rule);
      auto rhs = emitImpl(add.getRhs(), owner, rule);
      if (failed(lhs) || failed(rhs))
        return failure();
      ::std::string sum = "(static_cast<::std::uint64_t>(" + *lhs +
                          ") + static_cast<::std::uint64_t>(" + *rhs + "))";
      if (type.getWidth() < 64)
        sum = "(" + sum + " & UINT64_C(" +
              ::std::to_string((uint64_t(1) << type.getWidth()) - 1) + "))";
      return finish(sum);
    }
    return emitError() << "C++ emitter rejects unsupported SSA operation '"
                       << value.getDefiningOp()->getName() << "'";
  }

  const FinalProgram &program;
  size_t instanceOrdinal;
  ArrayRef<InputPort> inputs;
  DenseMap<Attribute, size_t> localOwned;
  const CppEmissionNames &names;
  size_t definitionIndex;
  llvm::raw_ostream &statements;
  ac::detail::EmitError emitError;
  DenseMap<Value, ::std::string> cache;
  DenseSet<Value> active;
};

FailureOr<::std::string> observationValue(const ObservationBinding &observation,
                                          CppExpressionEmitter &expressions,
                                          ac::detail::EmitError emitError) {
  if (!observation.valueConstraints ||
      observation.valueConstraints.size() != observation.values.size())
    return emitError() << "observation constraints are incomplete";
  if (observation.values.empty())
    return ::std::string("::gfsim::SlotValue::Unsigned(UINT64_C(0))");
  if (observation.values.size() != 1 || !observation.owner || !observation.rule)
    return emitError() << "C++ emitter supports zero or one observation value";
  Value value = observation.values.front();
  auto expression =
      expressions.emit(value, *observation.owner, observation.rule);
  if (failed(expression))
    return failure();
  auto integer = dyn_cast<IntegerType>(value.getType());
  auto constraint = dyn_cast<DictionaryAttr>(observation.valueConstraints[0]);
  auto kind = constraint ? constraint.getAs<StringAttr>("kind") : StringAttr();
  auto logical =
      constraint ? constraint.getAs<DictionaryAttr>("type") : DictionaryAttr();
  auto logicalKind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
  auto storage = logical ? logical.getAs<TypeAttr>("storage") : TypeAttr();
  if (!integer || integer.getWidth() == 0 || integer.getWidth() > 64 || !kind ||
      kind.getValue() != "logical" || !logical || !logicalKind || !storage ||
      storage.getValue() != value.getType())
    return emitError() << "observation logical type is malformed";
  if (logicalKind.getValue() == "bool" && integer.getWidth() == 1)
    return "::gfsim::SlotValue::Bool(" + *expression + ")";
  if (logicalKind.getValue() != "integer")
    return emitError() << "observation logical type is unsupported";
  auto interpretation = logical.getAs<StringAttr>("interpretation");
  if (!interpretation)
    return emitError() << "integer observation has no interpretation";
  if (interpretation.getValue() == "signed")
    return "::gfsim::SlotValue::Signed(::SignExtend(static_cast<::std::uint64_"
           "t>(" +
           *expression + "), " + ::std::to_string(integer.getWidth()) + "))";
  if (interpretation.getValue() == "unsigned")
    return "::gfsim::SlotValue::Unsigned(static_cast<::std::uint64_t>(" +
           *expression + "))";
  return emitError() << "integer observation interpretation is invalid";
}

} // namespace
} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALEMITCPPSUPPORT_H
