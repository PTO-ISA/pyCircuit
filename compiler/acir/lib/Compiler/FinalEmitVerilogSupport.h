#ifndef ACIR_LIB_COMPILER_FINALEMITVERILOGSUPPORT_H
#define ACIR_LIB_COMPILER_FINALEMITVERILOGSUPPORT_H

#include "FinalCppNames.h"
#include "FinalEmitVerilog.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::compiler {
namespace {

struct Port {
  size_t index = 0;
  std::string parameter;
  std::string emittedName;
  std::string role;
  DictionaryAttr formalState;
  DictionaryAttr stateID;
  unsigned width = 0;
};

struct Family {
  FlatSymbolRefAttr definition;
  std::string name;
  size_t representative = 0;
  SmallVector<size_t> instances;
  SmallVector<Port> current;
  SmallVector<Port> next;
};

inline std::string apIntLiteral(const APInt &value) {
  SmallString<32> text;
  value.toString(text, 10, false, false);
  return text.str().str();
}

inline std::string svType(unsigned width) {
  return width == 1 ? "logic" : "logic [" + std::to_string(width - 1) + ":0]";
}

inline std::string svLiteral(const APInt &value, unsigned width) {
  return std::to_string(width) + "'d" + apIntLiteral(value);
}

inline bool sameFormal(DictionaryAttr left, DictionaryAttr right) {
  return left && right && left.get("parameter") == right.get("parameter") &&
         left.get("ordinal") == right.get("ordinal");
}

inline FailureOr<const FinalProgram::StateCarrierSnapshot *>
findCarrier(const FinalProgram &program, DictionaryAttr stateID,
            ac::detail::EmitError emitError) {
  for (const auto &carrier : program.stateCarriers())
    if (carrier.stateID == stateID)
      return &carrier;
  return emitError() << "RTL emitter cannot resolve a frozen StateID carrier";
}

inline FailureOr<SmallVector<Port>> portsFor(const FinalProgram &program,
                                             size_t instanceOrdinal,
                                             ac::detail::EmitError emitError) {
  const auto &instance = program.instances()[instanceOrdinal];
  ArrayAttr attrs = instance.module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!attrs)
    return emitError() << "RTL emitter requires frozen final ports";
  ac::ModuleOp module = instance.module;
  if (module.getBody().empty() ||
      module.getBody().front().getNumArguments() != attrs.size() + 2)
    return emitError() << "RTL frozen formal argument layout is incomplete";
  SmallVector<Port> result;
  llvm::StringMap<StringAttr> sourceByName;
  for (auto [index, raw] : llvm::enumerate(attrs)) {
    auto port = dyn_cast<DictionaryAttr>(raw);
    auto parameter = port ? port.getAs<StringAttr>("parameter") : StringAttr();
    auto role = port ? port.getAs<StringAttr>("role") : StringAttr();
    if (!port || !parameter || !role ||
        (role.getValue() != "current" && role.getValue() != "next"))
      return emitError() << "RTL emitter found a malformed final port";
    auto emittedName = legalizeIdentifier(parameter.getValue(), emitError);
    if (failed(emittedName))
      return failure();
    auto [named, inserted] = sourceByName.try_emplace(*emittedName, parameter);
    if (!inserted && named->second != parameter)
      return emitError() << "legalized RTL connection parameter names collide";
    Value formalHandle = module.getBody().front().getArgument(index + 2);
    auto alias = llvm::find_if(program.stateAliases(), [&](const auto &entry) {
      return entry.view == instance.view && entry.handle == formalHandle;
    });
    if (alias == program.stateAliases().end())
      return emitError() << "RTL formal has no frozen StateID alias";
    if (!alias->formalState || !sameFormal(alias->formalState, port))
      return emitError() << "RTL formal identity disagrees with its PortSlot";
    auto carrier = findCarrier(program, alias->stateID, emitError);
    if (failed(carrier))
      return failure();
    result.push_back({index, parameter.getValue().str(),
                      std::move(*emittedName), role.getValue().str(),
                      alias->formalState, alias->stateID, (*carrier)->width});
  }
  return result;
}

inline LogicalResult verifyRtlNames(const FinalProgram &program,
                                    const Family &family,
                                    ac::detail::EmitError emitError) {
  const auto &instance = program.instances()[family.representative];
  llvm::StringSet<> names;
  for (StringRef fixed :
       {"clk", "reset", "root_commit_ok", "subtree_error", "local_error"})
    names.insert(fixed);
  for (size_t index = 0; index < instance.ownedStateOrdinals.size(); ++index) {
    std::string suffix = std::to_string(index);
    for (const std::string &name :
         {"q" + suffix, "d" + suffix, "q" + suffix + "_e", "initial" + suffix})
      names.insert(name);
  }
  for (size_t index = 0; index < instance.checkOrdinals.size(); ++index)
    names.insert("check_failed_" + std::to_string(index));
  for (size_t index = 0; index < instance.observationOrdinals.size(); ++index) {
    names.insert("obs_value_" + std::to_string(index));
    names.insert("obs_path_" + std::to_string(index));
  }
  for (auto [index, child] : llvm::enumerate(instance.childOrdinals)) {
    std::string prefix = "child_" + std::to_string(index);
    names.insert(prefix);
    names.insert(prefix + "_subtree_error");
    auto ports = portsFor(program, child, emitError);
    if (failed(ports))
      return failure();
    for (const Port &port : *ports)
      if (port.role == "next") {
        names.insert(prefix + "_" + port.emittedName + "_d");
        names.insert(prefix + "_" + port.emittedName + "_e");
      }
  }
  for (const Port &port : family.current)
    if (!names.insert(port.emittedName + "_q").second)
      return emitError() << "legalized RTL identifier collides with a "
                            "generated local or port";
  for (const Port &port : family.next)
    if (!names.insert(port.emittedName + "_d").second ||
        !names.insert(port.emittedName + "_e").second)
      return emitError() << "legalized RTL identifier collides with a "
                            "generated local or port";
  // Expression temporaries are v<ordinal>; formal names always carry _q/_d/_e
  // and therefore cannot occupy that namespace.
  return success();
}

inline FailureOr<SmallVector<Family, 0>>
buildFamilies(const FinalProgram &program, SmallVectorImpl<size_t> &familyFor,
              ac::detail::EmitError emitError) {
  SmallVector<Family, 0> families;
  familyFor.resize(program.instances().size());
  for (const auto &instance : program.instances()) {
    if (!instance.definition || !instance.staticArguments ||
        !instance.staticArguments.empty())
      return emitError()
             << "RTL hierarchy emitter supports empty staticArguments only";
    auto found = llvm::find_if(families, [&](const Family &family) {
      return family.definition == instance.definition;
    });
    if (found == families.end()) {
      families.push_back({instance.definition,
                          instance.definition.getValue().str(),
                          instance.ordinal,
                          {},
                          {},
                          {}});
      found = std::prev(families.end());
    }
    found->instances.push_back(instance.ordinal);
  }
  llvm::sort(families, [](const Family &left, const Family &right) {
    return left.name < right.name;
  });
  for (size_t index = 1; index < families.size(); ++index)
    if (families[index - 1].name == families[index].name)
      return emitError() << "source-derived RTL family names are ambiguous";
  for (size_t familyIndex = 0; familyIndex < families.size(); ++familyIndex) {
    Family &family = families[familyIndex];
    family.representative = family.instances.front();
    auto repPorts = portsFor(program, family.representative, emitError);
    if (failed(repPorts))
      return failure();
    for (const Port &port : *repPorts)
      (port.role == "current" ? family.current : family.next).push_back(port);
    const auto &representative = program.instances()[family.representative];
    for (size_t ordinal : family.instances) {
      const auto &candidate = program.instances()[ordinal];
      if (candidate.staticArguments != representative.staticArguments ||
          candidate.ownedStateOrdinals.size() !=
              representative.ownedStateOrdinals.size() ||
          candidate.childOrdinals.size() !=
              representative.childOrdinals.size() ||
          candidate.proposalContributionOrdinals.size() !=
              representative.proposalContributionOrdinals.size() ||
          candidate.checkOrdinals.size() !=
              representative.checkOrdinals.size() ||
          candidate.observationOrdinals.size() !=
              representative.observationOrdinals.size())
        return emitError()
               << "same-definition RTL instances have different shapes";
      auto candidatePorts = portsFor(program, ordinal, emitError);
      if (failed(candidatePorts) || candidatePorts->size() != repPorts->size())
        return emitError()
               << "same-definition RTL instances have different ports";
      for (auto [portIndex, port] : llvm::enumerate(*repPorts))
        if ((*candidatePorts)[portIndex].role != port.role ||
            (*candidatePorts)[portIndex].parameter != port.parameter ||
            (*candidatePorts)[portIndex].width != port.width ||
            !sameFormal((*candidatePorts)[portIndex].formalState,
                        port.formalState))
          return emitError() << "same-definition RTL port layout changed";
      for (auto [stateIndex, state] :
           llvm::enumerate(representative.ownedStateOrdinals)) {
        auto rep = findCarrier(
            program, program.proposals().states[state].stateID, emitError);
        auto actual =
            findCarrier(program,
                        program.proposals()
                            .states[candidate.ownedStateOrdinals[stateIndex]]
                            .stateID,
                        emitError);
        if (failed(rep) || failed(actual) || (*rep)->width != (*actual)->width)
          return emitError() << "same-definition RTL state layout changed";
      }
      familyFor[ordinal] = familyIndex;
    }
  }
  return families;
}

class SVExpressionEmitter {
public:
  SVExpressionEmitter(const FinalProgram &program, size_t instanceOrdinal,
                      ArrayRef<Port> inputs,
                      DenseMap<Attribute, size_t> localOwned,
                      ac::detail::EmitError emitError)
      : program(program), instanceOrdinal(instanceOrdinal), inputs(inputs),
        localOwned(std::move(localOwned)), emitError(emitError) {}

  FailureOr<std::string> emit(Value value, InstanceView &owner,
                              ac::RuleOp rule) {
    if (program.instances()[instanceOrdinal].view != &owner)
      return emitError() << "RTL expression owner differs from family instance";
    active.clear();
    return emitImpl(value, owner, rule);
  }

  StringRef declarations() const { return wires; }

  FailureOr<std::string> emitStateHandle(Value handle, InstanceView &owner) {
    if (program.instances()[instanceOrdinal].view != &owner)
      return emitError() << "RTL actual belongs to another family instance";
    return read(handle, owner);
  }

private:
  FailureOr<std::string> read(Value handle, InstanceView &owner) {
    const FinalProgram::StateAliasSnapshot *resolved = nullptr;
    for (const auto &alias : program.stateAliases())
      if (alias.view == &owner && alias.handle == handle) {
        if (resolved)
          return emitError() << "RTL rule input handle has ambiguous aliases";
        resolved = &alias;
      }
    if (!resolved)
      return emitError() << "RTL rule input handle has no frozen StateID alias";

    if (resolved->formalState) {
      const Port *formal = nullptr;
      for (const Port &port : inputs) {
        if (!sameFormal(port.formalState, resolved->formalState))
          continue;
        if (formal)
          return emitError()
                 << "RTL rule input maps to multiple current formals";
        formal = &port;
      }
      if (!formal)
        return emitError()
               << "RTL rule input formal has no current family port";
      return formal->emittedName + "_q";
    }

    auto owned = localOwned.find(resolved->stateID);
    if (owned != localOwned.end())
      return "q" + std::to_string(owned->second);
    return emitError() << "RTL expression reads non-owned, non-formal state";
  }

  FailureOr<std::string> emitImpl(Value value, InstanceView &owner,
                                  ac::RuleOp rule) {
    if (!value)
      return emitError() << "RTL expression has an empty SSA value";
    if (auto found = cache.find(value); found != cache.end())
      return found->second;
    if (!active.insert(value).second)
      return emitError() << "RTL expression contains an SSA cycle";
    auto finish = [&](std::string text) -> FailureOr<std::string> {
      active.erase(value);
      auto type = dyn_cast<IntegerType>(value.getType());
      if (!type || !type.getWidth() || type.getWidth() > 64)
        return emitError() << "RTL SSA temporary requires finite integer type";
      std::string name = "v" + std::to_string(cache.size());
      wires += "  wire [" + std::to_string(type.getWidth() - 1) + ":0] " +
               name + " = " + text + ";\n";
      cache.try_emplace(value, name);
      return name;
    };
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      if (!rule || argument.getOwner() != &rule.getBody().front() ||
          argument.getArgNumber() >= rule.getInputs().size())
        return emitError() << "RTL block argument is not a rule input";
      auto text = read(rule.getInputs()[argument.getArgNumber()], owner);
      if (failed(text))
        return failure();
      return finish(*text);
    }
    if (auto constant = value.getDefiningOp<arith::ConstantOp>()) {
      auto integer = dyn_cast<IntegerAttr>(constant.getValue());
      auto type = dyn_cast<IntegerType>(constant.getType());
      if (!integer || !type || type.getWidth() == 0 || type.getWidth() > 64)
        return emitError() << "RTL supports scalar constants up to 64 bits";
      return finish(svLiteral(integer.getValue(), type.getWidth()));
    }
    auto binary = [&](Value lhsValue, Value rhsValue,
                      StringRef operation) -> FailureOr<std::string> {
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
      return finish("($unsigned(" + *lhs + ") " + symbol.str() + " $unsigned(" +
                    *rhs + "))");
    }
    if (auto trunc = value.getDefiningOp<arith::TruncIOp>()) {
      auto input = emitImpl(trunc.getIn(), owner, rule);
      if (failed(input))
        return failure();
      auto type = cast<IntegerType>(trunc.getType());
      return finish(std::to_string(type.getWidth()) + "'($unsigned(" + *input +
                    "))");
    }
    if (auto extension = value.getDefiningOp<arith::ExtUIOp>()) {
      auto input = emitImpl(extension.getIn(), owner, rule);
      if (failed(input))
        return failure();
      auto type = cast<IntegerType>(value.getType());
      return finish(std::to_string(type.getWidth()) + "'($unsigned(" + *input +
                    "))");
    }
    if (auto add = value.getDefiningOp<arith::AddIOp>()) {
      auto type = dyn_cast<IntegerType>(value.getType());
      if (!type || type.getWidth() == 0 || type.getWidth() > 64 ||
          add.getOverflowFlags() != arith::IntegerOverflowFlags::none)
        return emitError() << "RTL finite addition requires flag-free i1..i64";
      auto lhs = emitImpl(add.getLhs(), owner, rule);
      auto rhs = emitImpl(add.getRhs(), owner, rule);
      if (failed(lhs) || failed(rhs))
        return failure();
      return finish(std::to_string(type.getWidth()) + "'($unsigned(" + *lhs +
                    ") + $unsigned(" + *rhs + "))");
    }
    Operation *definition = value.getDefiningOp();
    return emitError() << "RTL rejects unsupported expression operation '"
                       << (definition ? definition->getName().getStringRef()
                                      : StringRef("<block-arg>"))
                       << "'";
  }

  const FinalProgram &program;
  size_t instanceOrdinal;
  ArrayRef<Port> inputs;
  DenseMap<Attribute, size_t> localOwned;
  ac::detail::EmitError emitError;
  std::string wires;
  DenseMap<Value, std::string> cache;
  DenseSet<Value> active;
};

struct PairText {
  std::string data;
  std::string enable;
};

} // namespace
} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALEMITVERILOGSUPPORT_H
