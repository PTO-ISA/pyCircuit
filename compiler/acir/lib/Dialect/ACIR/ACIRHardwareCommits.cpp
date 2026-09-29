#include "ACIRHardwareClosureDetail.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::ac::hardware_detail {
namespace {

struct Driver {
  Value data;
  Value enable;
};

bool falseI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto attr =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) && attr.getValue().isZero();
}

bool resetValue(Value value, RegOp reg) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto attr = constant ? constant.getValue() : Attribute();
  return attr && attr == reg->getAttr("ac.initial_value");
}

LogicalResult addDriver(DenseMap<Attribute, Driver> &drivers,
                        DictionaryAttr target, Value data, Value enable,
                        ac::detail::EmitError emitError) {
  if (!drivers.try_emplace(target, Driver{data, enable}).second)
    return emitError() << "final commit target has multiple direct drivers";
  return success();
}

FailureOr<Definition> childDefinition(Closure &closure, InstanceOp child) {
  auto key = specKey(child.getCalleeAttr(), child);
  if (failed(key))
    return failure();
  auto found = closure.definitions.find(*key);
  if (found == closure.definitions.end())
    return child.emitOpError() << "final commit child definition is missing";
  return found->second;
}

LogicalResult verifyModuleCommits(Closure &closure, Definition definition,
                                  ac::detail::EmitError emitError) {
  ModuleOp module = definition.module;
  Block &body = module.getBody().front();
  Builder builder(module.getContext());
  auto targets = module->getAttrOfType<ArrayAttr>("ac.commit_targets");
  auto yield = dyn_cast_or_null<YieldOp>(body.getTerminator());
  if (!targets || !yield || yield.getValues().size() != 2 * targets.size())
    return emitError() << "final module commit target/yield arity is invalid";

  SmallVector<DictionaryAttr> expected;
  DenseMap<Attribute, RegOp> owned;
  for (RegOp reg : body.getOps<RegOp>()) {
    DictionaryAttr state = ownedRef(builder, reg);
    if (!owned.try_emplace(state, reg).second)
      return emitError() << "final module repeats an owned register identity";
    expected.push_back(state);
  }
  auto ports = module->getAttrOfType<ArrayAttr>("ac.ports");
  for (Attribute raw : ports) {
    auto port = cast<DictionaryAttr>(raw);
    if (port.getAs<StringAttr>("role").getValue() == "next")
      expected.push_back(formalRef(builder, port));
  }
  if (expected.size() != targets.size())
    return emitError() << "final module commit table has stale cardinality";
  DenseSet<Attribute> uniqueTargets;
  for (auto [index, raw] : llvm::enumerate(targets)) {
    auto target = dyn_cast<DictionaryAttr>(raw);
    if (!target || failed(detail::verifyStateRef(target, emitError)) ||
        target != expected[index] || !uniqueTargets.insert(target).second)
      return emitError() << "final module commit targets are missing, "
                            "reordered or duplicate";
  }

  DenseMap<Attribute, Driver> drivers;
  for (RuleOp rule : body.getOps<RuleOp>()) {
    auto outputs = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
    if (!outputs || outputs.size() != rule.getTargets().size() ||
        rule.getNextValues().size() != 2 * outputs.size())
      return emitError() << "final rule driver metadata is incomplete";
    for (auto [index, raw] : llvm::enumerate(outputs))
      if (failed(addDriver(drivers, cast<DictionaryAttr>(raw),
                           rule.getNextValues()[2 * index],
                           rule.getNextValues()[2 * index + 1], emitError)))
        return failure();
  }
  for (InstanceOp child : body.getOps<InstanceOp>()) {
    auto childDef = childDefinition(closure, child);
    auto bindings = child->getAttrOfType<ArrayAttr>("ac.port_bindings");
    auto childPorts =
        succeeded(childDef)
            ? (*childDef).module->getAttrOfType<ArrayAttr>("ac.ports")
            : ArrayAttr();
    if (failed(childDef) || !bindings || !childPorts ||
        bindings.size() != childPorts.size())
      return emitError() << "final child driver metadata is incomplete";
    size_t resultIndex = 0;
    for (auto [portIndex, rawPort] : llvm::enumerate(childPorts)) {
      auto port = cast<DictionaryAttr>(rawPort);
      if (port.getAs<StringAttr>("role").getValue() != "next")
        continue;
      auto binding = cast<DictionaryAttr>(bindings[portIndex]);
      auto target = binding.getAs<DictionaryAttr>("target");
      if (resultIndex + 1 >= child.getNextValues().size() ||
          failed(addDriver(drivers, target, child.getNextValues()[resultIndex],
                           child.getNextValues()[resultIndex + 1], emitError)))
        return failure();
      resultIndex += 2;
    }
    if (resultIndex != child.getNextValues().size())
      return emitError() << "final child has extra proposal results";
  }

  for (auto [index, target] : llvm::enumerate(expected)) {
    Value data = yield.getValues()[2 * index];
    Value enable = yield.getValues()[2 * index + 1];
    auto driver = drivers.find(target);
    if (driver != drivers.end()) {
      if (driver->second.data != data || driver->second.enable != enable)
        return emitError()
               << "final module yield redirects a direct proposal driver";
      continue;
    }
    auto reg = owned.find(target);
    if (reg == owned.end() || !resetValue(data, reg->second) ||
        !falseI1(enable))
      return emitError()
             << "undriven final state must hold reset image with false enable";
  }
  for (const auto &[target, driver] : drivers)
    if (!uniqueTargets.contains(target))
      return emitError() << "final module has a driver outside commit targets";
  return success();
}

} // namespace

LogicalResult verifyCommits(Closure &closure, ac::detail::EmitError emitError) {
  for (const auto &[key, definition] : closure.definitions)
    if (failed(verifyModuleCommits(closure, definition, emitError)))
      return failure();
  return success();
}

} // namespace acir::ac::hardware_detail
