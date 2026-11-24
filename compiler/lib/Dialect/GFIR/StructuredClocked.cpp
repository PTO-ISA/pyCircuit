#include "Dialect/GFIR/StructuredClocked.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSwitch.h"
#include <algorithm>
#include <limits>

using namespace mlir;

namespace acir::gfir::detail {
namespace {

using Access = StructuredFifoAccess;

bool isBitOne(const ac::HardwareAnalysis &hardware, Type type, Operation *op) {
  auto width = hardware.getPackedWidth(type, {}, op);
  return isa<ac::BitsType>(type) && succeeded(width) && *width == 1;
}

LogicalResult verifyValueType(const ac::HardwareAnalysis &hardware, Type type,
                              Operation *site, bool allowTable = false) {
  if (auto table = dyn_cast<ac::TableType>(type); allowTable && table) {
    if (table.getShape().size() != 1)
      return site->emitOpError("clocked GFIR requires rank-one Tables");
    auto size = hardware.getTableSize(table, {}, site);
    if (failed(size) || *size == 0)
      return failure();
    return verifyValueType(hardware, table.getElementType(), site);
  }
  if (!isa<ac::BitsType, ac::StructType>(type))
    return site->emitOpError("clocked GFIR values require Bits or a Struct of Bits");
  auto leaves = hardware.getLeaves(type, {}, site);
  if (failed(leaves))
    return failure();
  if (leaves->empty() || llvm::any_of(*leaves, [](const ac::PackedLeaf &leaf) {
        return !isa<ac::BitsType>(leaf.type) || leaf.width == 0 ||
               leaf.width > std::numeric_limits<unsigned>::max() - leaf.low;
      }))
    return site->emitOpError("clocked GFIR value has an unsupported layout");
  return success();
}

LogicalResult verifyInitializer(Value root, Operation *site,
                                ac::detail::SourceCheckBudget &budget) {
  SmallVector<Value> pending{root};
  llvm::DenseSet<Value> visited;
  while (!pending.empty()) {
    Value current = pending.pop_back_val();
    if (!visited.insert(current).second)
      continue;
    auto *op = current.getDefiningOp();
    if (!isa_and_nonnull<ac::BitsConstantOp, ac::BitsConstant4Op,
                         ac::StructCreateOp, ac::TableSplatOp>(op))
      return site->emitOpError("initializer requires a closed constant value");
    if (failed(budget.debit(1 + op->getNumOperands(), site)))
      return failure();
    pending.append(op->operand_begin(), op->operand_end());
  }
  return success();
}

struct BodyUses {
  SmallVector<unsigned> methods, states, writes;
  uint64_t expandedWork = 0;
};

class ClockedAnalyzer {
public:
  ClockedAnalyzer(ModuleOp module, const ac::HardwareAnalysis &hardware,
                  ac::detail::SourceCheckBudget &budget)
      : module(module), hardware(hardware), budget(budget) {}
  FailureOr<StructuredClocked> run();

private:
  LogicalResult collect();
  LogicalResult bindState(Operation *op, FlatSymbolRefAttr target, Value value,
                          Value index, BodyUses *body, bool write);
  LogicalResult bindMethod(CallOp call, BodyUses &body);
  LogicalResult bindFifo(CallOp call, std::optional<unsigned> rule,
                         llvm::SmallDenseSet<unsigned> &popped);
  LogicalResult verifyBody(Operation *owner, Block &block, BodyUses &uses,
                           std::optional<unsigned> rule);
  LogicalResult resolveMethodEffects();

  ModuleOp module;
  const ac::HardwareAnalysis &hardware;
  ac::detail::SourceCheckBudget &budget;
  StructuredClocked plan;
  llvm::StringMap<unsigned> variables, fifos, methods;
  SmallVector<BodyUses> methodUses, ruleUses;
};

LogicalResult ClockedAnalyzer::collect() {
  for (Operation &op : module.getBody().front()) {
    if (auto var = dyn_cast<VarOp>(op)) {
      if (failed(verifyAttributes(var, {"sym_name"})) ||
          failed(verifyValueType(hardware, var.getInitial().getType(), var, true)) ||
          failed(verifyInitializer(var.getInitial(), var, budget)))
        return failure();
      variables.try_emplace(var.getSymName(), plan.variables.size());
      plan.variables.push_back(var);
    } else if (auto instance = dyn_cast<InstanceOp>(op)) {
      auto primitive = dyn_cast_or_null<PrimitiveOp>(
          hardware.lookupDefinition(instance.getCalleeAttr().getValue()));
      if (!primitive)
        continue;
      auto *definition = primitive.getOperation();
      fifos.try_emplace(instance.getSymName(), plan.fifos.size());
      plan.fifos.push_back({
          instance, definition->getAttrOfType<TypeAttr>("element").getValue(),
          definition->getAttrOfType<IntegerAttr>("depth").getValue().getZExtValue(),
          definition->getAttrOfType<IntegerAttr>("latency").getValue().getZExtValue(),
          definition->getAttrOfType<StringAttr>("ready_policy").getValue() ==
              "downstream_pop", {}, {}, {}, {}});
    } else if (auto method = dyn_cast<MethodOp>(op)) {
      methods.try_emplace(method.getSymName(), plan.methods.size());
      plan.methods.push_back(method);
    } else if (auto rule = dyn_cast<RuleOp>(op)) {
      plan.rules.push_back({rule, {}, {}, {}, {}, {}});
    }
  }
  return success();
}

LogicalResult ClockedAnalyzer::bindState(Operation *op, FlatSymbolRefAttr target,
                                          Value value, Value index,
                                          BodyUses *body, bool write) {
  if (failed(verifyAttributes(op, {"state"})))
    return failure();
  auto found = variables.find(target.getValue());
  if (found == variables.end())
    return op->emitOpError("state reference must resolve to a local gfir.var");
  auto ordinal = found->second;
  Type type = plan.variables[ordinal].getInitial().getType();
  if (index) {
    auto table = dyn_cast<ac::TableType>(type);
    if (!table || !isa<ac::BitsType>(index.getType()))
      return op->emitOpError("indexed state write requires Table and Bits index");
    if (failed(hardware.verifySourceIndex(index, table, {}, op)))
      return failure();
    type = table.getElementType();
  }
  if (!ac::areEquivalentHardwareTypes(value.getType(), type))
    return op->emitOpError("state access payload type does not match its target");
  if (!body && isa<ac::TableType>(type))
    return op->emitOpError("Table state reads require an ordered body");
  plan.stateAccesses.try_emplace(op, ordinal);
  if (body) {
    body->states.push_back(ordinal);
    if (write)
      body->writes.push_back(ordinal);
  }
  return success();
}

LogicalResult ClockedAnalyzer::bindMethod(CallOp call, BodyUses &body) {
  auto interface = call.getInterfaceAttr();
  if (!interface)
    return call.emitOpError("self requires an interface name");
  auto found = methods.find(interface.getValue());
  if (found == methods.end())
    return call.emitOpError("self interface must resolve to a local gfir.method");
  auto type = plan.methods[found->second].getFunctionType();
  auto equal = [](TypeRange actual, TypeRange formal) {
    return actual.size() == formal.size() &&
           llvm::all_of(llvm::zip(actual, formal), [](auto pair) {
             return ac::areEquivalentHardwareTypes(std::get<0>(pair), std::get<1>(pair));
           });
  };
  if (!equal(call.getInputs().getTypes(), type.getInputs()) ||
      !equal(call.getOutputs().getTypes(), type.getResults()))
    return call.emitOpError("method operands and results must match its signature");
  body.methods.push_back(found->second);
  plan.methodCalls.try_emplace(call, found->second);
  return success();
}

LogicalResult ClockedAnalyzer::bindFifo(CallOp call, std::optional<unsigned> rule,
                                       llvm::SmallDenseSet<unsigned> &popped) {
  auto interface = call.getInterfaceAttr();
  auto receiver = call.getReceiverAttr();
  if (!interface || !receiver)
    return call.emitOpError("FIFO calls require an instance and interface");
  auto found = fifos.find(receiver.getValue());
  if (found == fifos.end())
    return call.emitOpError("FIFO interface requires a local native FIFO instance");
  auto &fifo = plan.fifos[found->second];
  auto access = llvm::StringSwitch<std::optional<Access>>(interface.getValue())
                    .Case("raw_in", Access::RawIn).Case("raw_out", Access::RawOut)
                    .Case("raw_status", Access::Status).Case("in", Access::In)
                    .Case("out", Access::Out).Case("view", Access::View)
                    .Default(std::nullopt);
  if (!access)
    return call.emitOpError("unknown native FIFO interface");
  auto check = [&](TypeRange types, std::initializer_list<bool> payload) {
    if (types.size() != payload.size())
      return false;
    return llvm::all_of(llvm::zip(types, payload), [&](auto pair) {
      auto [type, data] = pair;
      return data ? ac::areEquivalentHardwareTypes(type, fifo.element)
                  : isBitOne(hardware, type, call);
    });
  };
  auto inputs = call.getInputs().getTypes();
  auto outputs = call.getOutputs().getTypes();
  auto duplicate = [&] { return call.emitOpError("FIFO direction has multiple drive bindings"); };
  switch (*access) {
  case Access::RawIn:
    if (rule || !check(inputs, {false, true}) || !check(outputs, {false}))
      return call.emitOpError("raw_in requires graph signature (bit, element) -> bit");
    if (fifo.rawInput || fifo.producer)
      return duplicate();
    fifo.rawInput = call;
    break;
  case Access::RawOut:
    if (rule || !check(inputs, {false}) || !check(outputs, {false, true}))
      return call.emitOpError("raw_out requires graph signature (bit) -> (bit, element)");
    if (fifo.rawOutput || fifo.consumer)
      return duplicate();
    fifo.rawOutput = call;
    break;
  case Access::Status:
    if (rule || !inputs.empty() || !check(outputs, {false, false, true}))
      return call.emitOpError("raw_status requires graph signature () -> (bit, bit, element)");
    break;
  case Access::In:
    if (!rule || !check(inputs, {true}) || !outputs.empty())
      return call.emitOpError("in requires Rule signature (element) -> ()");
    if (fifo.rawInput || fifo.producer)
      return duplicate();
    fifo.producer = *rule;
    plan.rules[*rule].requirements.push_back(
        {found->second, true, popped.contains(found->second)});
    break;
  case Access::Out:
    if (!rule || !inputs.empty() || !check(outputs, {true}))
      return call.emitOpError("out requires Rule signature () -> element");
    if (fifo.rawOutput || fifo.consumer)
      return duplicate();
    fifo.consumer = *rule;
    popped.insert(found->second);
    plan.rules[*rule].requirements.push_back({found->second, false, false});
    break;
  case Access::View:
    if (!rule || !inputs.empty() || !check(outputs, {true}))
      return call.emitOpError("view requires Rule signature () -> element");
    plan.rules[*rule].requirements.push_back({found->second, false, false});
    break;
  }
  plan.fifoCalls.try_emplace(call, StructuredFifoUse{found->second, *access});
  return success();
}

LogicalResult ClockedAnalyzer::verifyBody(Operation *owner, Block &block,
                                         BodyUses &uses,
                                         std::optional<unsigned> rule) {
  if (failed(verifyAttributes(owner, {"sym_name", "function_type"})))
    return failure();
  llvm::SmallDenseSet<unsigned> popped;
  for (Type type : block.getArgumentTypes())
    if (failed(verifyValueType(hardware, type, owner)))
      return failure();
  for (Operation &op : block) {
    auto work = 1 + uint64_t(op.getNumOperands()) + op.getNumResults();
    if (failed(budget.debit(work, &op)))
      return failure();
    uses.expandedWork = ac::detail::SourceCheckBudget::add(uses.expandedWork, work);
    if (auto read = dyn_cast<ReadOp>(op)) {
      if (failed(bindState(read, read.getStateAttr(), read.getValue(), {}, &uses, false)))
        return failure();
    } else if (auto write = dyn_cast<WriteOp>(op)) {
      if (failed(bindState(write, write.getStateAttr(), write.getValue(),
                           write.getIndex(), &uses, true)))
        return failure();
    } else if (auto call = dyn_cast<CallOp>(op)) {
      if (failed(verifyAttributes(call, {"receiver", "interface"})))
        return failure();
      if (call.getReceiverAttr()) {
        if (!rule)
          return call.emitOpError("native resource access is limited to a Rule in this profile");
        if (failed(bindFifo(call, rule, popped)))
          return failure();
      } else if (failed(bindMethod(call, uses))) {
        return failure();
      }
    } else if (isa<YieldOp>(op)) {
      if (failed(verifyAttributes(&op, {})))
        return failure();
    } else {
      if (!isCombinational(&op) && !isa<ac::TableGetOp>(op))
        return op.emitOpError("operation is unavailable in a clocked GFIR body");
      if (failed(hardware.verifyResolvedOperation(&op, {})))
        return failure();
      for (Type type : op.getResultTypes())
        if (failed(verifyValueType(hardware, type, &op)))
          return failure();
    }
  }
  return success();
}

LogicalResult ClockedAnalyzer::resolveMethodEffects() {
  SmallVector<SmallVector<unsigned>> users(methodUses.size());
  SmallVector<unsigned> pending(methodUses.size()), order;
  auto merge = [&](BodyUses &into, const BodyUses &from, Operation *site) {
    if (failed(budget.debit(into.states.size() + into.writes.size() +
                                from.states.size() + from.writes.size(), site)))
      return failure();
    auto append = [](SmallVector<unsigned> &to, ArrayRef<unsigned> source) {
      to.append(source.begin(), source.end());
      llvm::sort(to);
      to.erase(std::unique(to.begin(), to.end()), to.end());
    };
    append(into.states, from.states);
    append(into.writes, from.writes);
    into.expandedWork =
        ac::detail::SourceCheckBudget::add(into.expandedWork, from.expandedWork);
    return success();
  };
  for (auto [index, body] : llvm::enumerate(methodUses)) {
    pending[index] = body.methods.size();
    if (body.methods.empty())
      order.push_back(index);
    for (unsigned callee : body.methods)
      users[callee].push_back(index);
  }
  for (unsigned head = 0; head < order.size(); ++head) {
    unsigned done = order[head];
    for (unsigned caller : users[done]) {
      if (failed(merge(methodUses[caller], methodUses[done], plan.methods[caller])))
        return failure();
      if (--pending[caller] == 0)
        order.push_back(caller);
    }
  }
  if (order.size() != methodUses.size())
    return module.emitOpError("recursive GFIR methods are unsupported");
  plan.methodOrder = std::move(order);
  llvm::SmallDenseSet<unsigned> written;
  for (auto [index, body] : llvm::enumerate(ruleUses)) {
    auto &rule = plan.rules[index];
    for (unsigned callee : body.methods)
      if (failed(merge(body, methodUses[callee], rule.operation)))
        return failure();
    if (failed(merge(body, BodyUses{}, rule.operation)))
      return failure();
    if (failed(budget.debit(body.expandedWork, rule.operation)))
      return failure();
    for (unsigned state : body.writes)
      if (!written.insert(state).second)
        return rule.operation.emitOpError("state write overlaps another Rule in this profile");
    rule.states = std::move(body.states);
    rule.writes = std::move(body.writes);
  }
  return success();
}

FailureOr<StructuredClocked> ClockedAnalyzer::run() {
  if (failed(collect()))
    return failure();
  methodUses.resize(plan.methods.size());
  ruleUses.resize(plan.rules.size());
  llvm::SmallDenseSet<unsigned> popped;
  for (Operation &op : module.getBody().front()) {
    if (auto call = dyn_cast<CallOp>(op); call && call.getInterfaceAttr()) {
      if (failed(verifyAttributes(call, {"receiver", "interface"})) ||
          failed(bindFifo(call, std::nullopt, popped)))
        return failure();
    } else if (auto read = dyn_cast<ReadOp>(op)) {
      if (failed(bindState(read, read.getStateAttr(), read.getValue(), {}, nullptr, false)))
        return failure();
    }
  }
  for (auto [index, method] : llvm::enumerate(plan.methods)) {
    for (Type type : method.getFunctionType().getResults())
      if (failed(verifyValueType(hardware, type, method)))
        return failure();
    if (failed(verifyBody(method, method.getBody().front(), methodUses[index], std::nullopt)))
      return failure();
  }
  for (auto [index, rule] : llvm::enumerate(plan.rules)) {
    auto operation = rule.operation;
    for (Type type : operation.getFunctionType().getResults())
      if (failed(verifyValueType(hardware, type, operation)))
        return failure();
    if (failed(verifyBody(operation, operation.getBody().front(),
                          ruleUses[index], unsigned(index))))
      return failure();
  }
  if (failed(resolveMethodEffects()))
    return failure();
  return std::move(plan);
}

} // namespace

bool hasGeneratedDomain(RootOp root) {
  auto domains = root->getAttrOfType<ArrayAttr>("domains");
  if (!domains || domains.size() != 1)
    return false;
  auto domain = dyn_cast<DictionaryAttr>(domains[0]);
  if (!domain || domain.size() != 4)
    return false;
  auto equals = [](DictionaryAttr attrs, StringRef key, StringRef expected) {
    auto value = attrs.getAs<StringAttr>(key);
    return value && value.getValue() == expected;
  };
  auto generated = [&](StringRef key, StringRef expected) {
    auto source = domain.getAs<DictionaryAttr>(key);
    return source && source.size() == 1 && equals(source, "generated", expected);
  };
  return equals(domain, "name", "default") && equals(domain, "edge", "rising") &&
         generated("clock", "pyc_clk") && generated("reset", "pyc_rst");
}

LogicalResult verifyFifoPrimitive(PrimitiveOp primitive,
                                   const ac::HardwareAnalysis &hardware) {
  if (failed(verifyAttributes(primitive, {"sym_name", "kind", "version", "element",
                                         "depth", "latency", "ready_policy"})))
    return failure();
  if (primitive.getKind() != "fifo" || primitive.getVersion() != 1)
    return primitive.emitOpError("clocked GFIR supports native FIFO version 1");
  auto element = primitive->getAttrOfType<TypeAttr>("element");
  auto policy = primitive->getAttrOfType<StringAttr>("ready_policy");
  auto positive = [&](StringRef key) {
    auto value = primitive->getAttrOfType<IntegerAttr>(key);
    return value && value.getType().isSignlessInteger(64) && value.getInt() > 0;
  };
  if (!element || !positive("depth") || !positive("latency") || !policy ||
      (policy.getValue() != "local_occupancy" && policy.getValue() != "downstream_pop"))
    return primitive.emitOpError("FIFO requires element, positive i64 depth/latency and ready_policy");
  return verifyValueType(hardware, element.getValue(), primitive);
}

FailureOr<StructuredClocked>
analyzeClocked(ModuleOp module, const ac::HardwareAnalysis &hardware,
               ac::detail::SourceCheckBudget &budget) {
  return ClockedAnalyzer(module, hardware, budget).run();
}

} // namespace acir::gfir::detail
