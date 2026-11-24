#include "Dialect/GFIR/StructuredClocked.h"
#include "mlir/IR/Builders.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/MathExtras.h"
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
  if (!isa<ac::BitsType, ac::StructType, ac::EnumType>(type))
    return site->emitOpError("clocked GFIR requires a closed hardware value type");
  auto leaves = hardware.getLeaves(type, {}, site);
  if (failed(leaves))
    return failure();
  if (leaves->empty() || llvm::any_of(*leaves, [](const ac::PackedLeaf &leaf) {
        return !isStructuredLeaf(leaf.type) || leaf.width == 0 ||
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
  uint64_t sampledControls = 0;
  bool hasFieldWrites = false;
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
  LogicalResult bindRawMemory(CallOp call, unsigned index);
  LogicalResult bindTransactionMemory(CallOp call, unsigned index, unsigned rule);
  LogicalResult verifyBody(Operation *owner, Block &block, BodyUses &uses,
                           std::optional<unsigned> rule);
  LogicalResult resolveMethodEffects();
  LogicalResult buildCheckPlan();

  ModuleOp module;
  const ac::HardwareAnalysis &hardware;
  ac::detail::SourceCheckBudget &budget;
  StructuredClocked plan;
  llvm::StringMap<unsigned> variables, fifos, memories, methods;
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
      if (primitive.getNativeKind() == "memory") {
        auto memory = describeRawMemory(primitive, hardware);
        if (failed(memory))
          return failure();
        memory->instance = instance;
        memories.try_emplace(instance.getSymName(), plan.memories.size());
        plan.memories.push_back(*memory);
        continue;
      }
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
      auto &record = plan.rules.emplace_back();
      record.operation = rule;
    }
  }
  return success();
}

LogicalResult ClockedAnalyzer::bindRawMemory(CallOp call, unsigned index) {
  auto &memory = plan.memories[index];
  if (failed(verifyAttributes(call, {"receiver", "interface"})))
    return failure();
  if (memory.transactional || call.getInterfaceAttr().getValue() != "raw" || memory.raw)
    return call.emitOpError("Memory requires a single raw graph binding");
  auto inputs = call.getInputs().getTypes();
  auto outputs = call.getOutputs().getTypes();
  if (inputs.size() != memory.writeOffset() + 4 ||
      outputs.size() != memory.readPorts())
    return call.emitOpError("raw Memory binding has the wrong port count");
  auto bits = [&](unsigned argument, unsigned width) {
    Type type = inputs[argument];
    auto actual = hardware.getPackedWidth(type, {}, call);
    return isa<ac::BitsType>(type) && succeeded(actual) && *actual == width;
  };
  if (memory.asynchronous()) {
    if (!bits(0, memory.addressWidth))
      return call.emitOpError("byte Memory read address must match the declared depth");
  } else {
    for (unsigned port = 0; port < memory.readPorts(); ++port)
      if (!bits(2 * port, 1) || !bits(2 * port + 1, memory.addressWidth))
        return call.emitOpError("Memory read pins must match the declared depth");
  }
  unsigned write = memory.writeOffset();
  if (!bits(write, 1) || !bits(write + 1, memory.addressWidth) ||
      !ac::areEquivalentHardwareTypes(inputs[write + 2], memory.element) ||
      !bits(write + 3, memory.maskWidth) ||
      !llvm::all_of(outputs, [&](Type type) {
        return ac::areEquivalentHardwareTypes(type, memory.element);
      }))
    return call.emitOpError("Memory write pins and read payload must match its layout");
  memory.raw = call;
  plan.memoryCalls.try_emplace(call, index);
  return success();
}

LogicalResult ClockedAnalyzer::bindTransactionMemory(CallOp call, unsigned index,
                                                    unsigned rule) {
  auto &memory = plan.memories[index];
  if (!memory.transactional || !call.getInterfaceAttr())
    return call.emitOpError("Rule Memory access requires a transaction declaration");
  StringRef name = call.getInterfaceAttr().getValue();
  StructuredMemoryAccess access;
  unsigned port = 0;
  if (name == "request0" || name == "request1") {
    access = StructuredMemoryAccess::Request;
    port = name == "request1";
  } else if (name == "result0" || name == "result1") {
    access = StructuredMemoryAccess::Result;
    port = name == "result1";
  } else if (name == "write") {
    access = StructuredMemoryAccess::Write;
  } else {
    return call.emitOpError("unknown transaction Memory interface");
  }
  if (port >= memory.readPorts())
    return call.emitOpError("Memory read port is not declared");
  auto inputs = call.getInputs().getTypes();
  auto outputs = call.getOutputs().getTypes();
  auto bits = [&](unsigned i, unsigned width) {
    if (i >= inputs.size() || !isa<ac::BitsType>(inputs[i]))
      return false;
    auto actual = hardware.getPackedWidth(inputs[i], {}, call);
    return succeeded(actual) && *actual == width;
  };
  if (access == StructuredMemoryAccess::Result) {
    if (!inputs.empty() || outputs.size() != 1 ||
        !ac::areEquivalentHardwareTypes(outputs.front(), memory.element))
      return call.emitOpError("Memory result requires () -> element");
  } else if (access == StructuredMemoryAccess::Request) {
    if (inputs.size() != 1 || !outputs.empty() || !bits(0, memory.addressWidth))
      return call.emitOpError("Memory request requires (address) -> ()");
    if (memory.requests[port])
      return call.emitOpError("Memory read port has multiple contributors");
    memory.requests[port] = call;
  } else {
    if (inputs.size() != 3 || !outputs.empty() || !bits(0, memory.addressWidth) ||
        !ac::areEquivalentHardwareTypes(inputs[1], memory.element) ||
        !bits(2, memory.maskWidth))
      return call.emitOpError("Memory write requires (address, element, mask) -> ()");
    if (memory.writer)
      return call.emitOpError("Memory write port has multiple contributors");
    memory.writer = call;
  }
  plan.transactionMemoryCalls.try_emplace(call, StructuredMemoryUse{index, access, port, rule});
  return success();
}

LogicalResult ClockedAnalyzer::bindState(Operation *op, FlatSymbolRefAttr target,
                                          Value value, Value index,
                                          BodyUses *body, bool write) {
  if (failed(verifyAttributes(op, write ? ArrayRef<StringRef>{"state", "fields"}
                                       : ArrayRef<StringRef>{"state"})))
    return failure();
  auto found = variables.find(target.getValue());
  if (found == variables.end())
    return op->emitOpError("state reference must resolve to a local gfir.var");
  auto ordinal = found->second;
  Type type = plan.variables[ordinal].getInitial().getType();
  if (auto fields = op->getAttrOfType<ArrayAttr>("fields")) {
    StructuredWritePath path{fields, {}, 0, 0};
    uint64_t low = 0;
    for (Attribute raw : fields) {
      auto structure = dyn_cast<ac::StructType>(type);
      auto declaration = structure ? hardware.lookupStruct(structure) : ac::StructOp();
      if (!declaration)
        return op->emitOpError("field path requires a declared Struct at every step");
      if (failed(budget.debit(declaration.getFields().size(), op)))
        return failure();
      Type selected;
      uint64_t offset = 0;
      for (Attribute item : llvm::reverse(declaration.getFields())) {
        auto field = cast<DictionaryAttr>(item);
        Type fieldType = field.getAs<TypeAttr>("type").getValue();
        if (field.getAs<StringAttr>("name") == raw) {
          selected = fieldType;
          break;
        }
        auto width = hardware.getPackedWidth(fieldType, {}, op);
        if (failed(width) || *width > std::numeric_limits<unsigned>::max() - offset)
          return op->emitOpError("field path exceeds the supported packed width");
        offset += *width;
      }
      if (!selected)
        return op->emitOpError("field path names an undeclared Struct field");
      if (offset > std::numeric_limits<unsigned>::max() - low)
        return op->emitOpError("field path exceeds the supported packed width");
      low += offset;
      type = selected;
      path.types.push_back(type);
    }
    auto width = hardware.getPackedWidth(type, {}, op);
    if (failed(width) || *width > std::numeric_limits<unsigned>::max() - low)
      return op->emitOpError("field path exceeds the supported packed width");
    path.low = low;
    path.width = *width;
    plan.writePaths.try_emplace(op, std::move(path));
    body->hasFieldWrites = true;
  }
  if (index) {
    auto table = dyn_cast<ac::TableType>(type);
    if (!table || !isa<ac::BitsType>(index.getType()))
      return op->emitOpError("indexed state write requires Table and Bits index");
    if (plan.writePaths.contains(op) &&
        (table.getShape().size() != 1 || !isa<ac::BitsType>(table.getElementType())))
      return op->emitOpError("indexed field writes require a rank-one Bits Table");
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
  bool isArm = isa<IfOp>(owner);
  bool isLoop = isa<LoopOp>(owner);
  if (isLoop && failed(verifyAttributes(owner, {"trip_count"})))
    return failure();
  if (!isArm && !isLoop && failed(verifyAttributes(owner, {"sym_name", "function_type"})))
    return failure();
  llvm::SmallDenseSet<unsigned> popped;
  for (Type type : block.getArgumentTypes().drop_front(isLoop ? 1 : 0))
    if (failed(verifyValueType(hardware, type, owner)))
      return failure();
  for (Operation &op : block) {
    if (isLoop && isa<LoopOp, IfOp, CallOp, GuardOp, YieldOp>(op))
      return op.emitOpError("fixed-range body supports values and state accesses only");
    auto work = 1 + uint64_t(op.getNumOperands()) + op.getNumResults();
    if (failed(budget.debit(work, &op)))
      return failure();
    uses.expandedWork = ac::detail::SourceCheckBudget::add(uses.expandedWork, work);
    if (auto loop = dyn_cast<LoopOp>(op)) {
      if (failed(verifyAttributes(loop, {"trip_count"})))
        return failure();
      for (Type type : loop.getInitial().getTypes())
        if (failed(verifyValueType(hardware, type, loop)))
          return failure();
      if (!isBitOne(hardware, loop.getResultTypes().back(), loop))
        return loop.emitOpError("natural completion result requires Bits1");
      BodyUses iteration;
      if (failed(verifyBody(loop, loop.getBody().front(), iteration, rule)))
        return failure();
      uint64_t count = loop.getTripCountAttr().getValue().getZExtValue();
      uint64_t expansion = ac::detail::SourceCheckBudget::multiply(count, iteration.expandedWork);
      if (failed(budget.debit(expansion, loop)))
        return failure();
      uses.expandedWork = ac::detail::SourceCheckBudget::add(uses.expandedWork, expansion);
      if (count != 0) {
        uses.states.append(iteration.states.begin(), iteration.states.end());
        uses.writes.append(iteration.writes.begin(), iteration.writes.end());
        uses.hasFieldWrites |= iteration.hasFieldWrites;
      }
    } else if (auto ordinal = dyn_cast<OrdinalBitsOp>(op)) {
      if (!isLoop || failed(verifyAttributes(ordinal, {})))
        return ordinal.emitOpError("requires a fixed-range body");
      auto width = hardware.getPackedWidth(ordinal.getValue().getType(), {}, ordinal);
      if (failed(width) || *width == 0 || *width > std::numeric_limits<unsigned>::max())
        return ordinal.emitOpError("ordinal requires a closed positive Bits width");
      uint64_t count = cast<LoopOp>(owner).getTripCountAttr().getValue().getZExtValue();
      if (count > 1 && *width < 64 && (count - 1) >> *width)
        return ordinal.emitOpError("Bits width cannot represent every range ordinal");
    } else if (auto read = dyn_cast<ReadOp>(op)) {
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
        if (isArm)
          return call.emitOpError("conditional resource calls require selected admission");
        if (!rule)
          return call.emitOpError("native resource access is limited to a Rule in this profile");
        auto memory = memories.find(call.getReceiverAttr().getValue());
        if (memory != memories.end()) {
          if (failed(bindTransactionMemory(call, memory->second, *rule)))
            return failure();
          uses.sampledControls += std::max<unsigned>(1, call.getInputs().size());
        } else if (failed(bindFifo(call, rule, popped))) {
          return failure();
        }
      } else if (failed(bindMethod(call, uses))) {
        return failure();
      }
    } else if (auto guard = dyn_cast<GuardOp>(op)) {
      if (failed(verifyAttributes(guard, {})))
        return failure();
      if (!isBitOne(hardware, guard.getPredicate().getType(), guard))
        return guard.emitOpError("guard requires one bit");
      ++uses.sampledControls;
    } else if (auto branch = dyn_cast<IfOp>(op)) {
      if (failed(verifyAttributes(branch, {})))
        return failure();
      if (!isBitOne(hardware, branch.getPredicate().getType(), branch))
        return branch.emitOpError("condition requires one bit");
      for (Type type : branch.getResultTypes())
        if (failed(verifyValueType(hardware, type, branch, /*allowTable=*/true)))
          return failure();
      unsigned firstWrite = uses.writes.size();
      size_t firstCall = uses.methods.size();
      uint64_t earlierControls = uses.sampledControls;
      for (Region *arm : {&branch.getThenBody(), &branch.getElseBody()})
        if (failed(verifyBody(branch, arm->front(), uses, rule)))
          return failure();
      if (uses.methods.size() != firstCall || uses.sampledControls != earlierControls) {
        plan.checkedBranches.insert(branch);
        ++uses.sampledControls;
      }
      SmallVector<unsigned> written(uses.writes.begin() + firstWrite,
                                    uses.writes.end());
      if (failed(budget.debit(written.size(), branch)))
        return failure();
      llvm::sort(written);
      written.erase(std::unique(written.begin(), written.end()), written.end());
      plan.conditionalWrites.insert(written.begin(), written.end());
      plan.branchWrites.try_emplace(branch, std::move(written));
    } else if (isa<YieldOp, StepOp>(op)) {
      if (failed(verifyAttributes(&op, {})))
        return failure();
    } else {
      if (!isCombinational(&op) && !isa<ac::TableGetOp, ac::TableSplatOp>(op))
        return op.emitOpError("operation is unavailable in a clocked GFIR body");
      if (failed(hardware.verifyResolvedOperation(&op, {})))
        return failure();
      for (Type type : op.getResultTypes())
        if (failed(verifyValueType(hardware, type, &op,
                                   /*allowTable=*/isa<ac::TableSplatOp>(op))))
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
    into.sampledControls =
        ac::detail::SourceCheckBudget::add(into.sampledControls, from.sampledControls);
    into.hasFieldWrites |= from.hasFieldWrites;
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
  for (auto [index, uses] : llvm::enumerate(methodUses))
    if (uses.sampledControls != 0)
      plan.sampledMethods.insert(index);
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
    rule.sampled = body.sampledControls != 0;
    if (rule.sampled) {
      if (body.hasFieldWrites)
        return rule.operation.emitOpError("sampled guards do not yet support field writes");
      if (!rule.requirements.empty())
        return rule.operation.emitOpError("sampled guards cannot yet combine with resource admission");
      if (llvm::any_of(rule.states, [&](unsigned index) {
            return isa<ac::TableType>(plan.variables[index].getInitial().getType());
          }))
        return rule.operation.emitOpError("sampled guards require scalar or Struct state");
    }
  }
  return success();
}

LogicalResult ClockedAnalyzer::buildCheckPlan() {
  if (llvm::none_of(plan.rules, [](const StructuredRule &rule) { return rule.sampled; }))
    return success();
  using Budget = ac::detail::SourceCheckBudget;
  Builder builder(module.getContext());
  auto symbol = FlatSymbolRefAttr::get(module.getSymNameAttr());
  auto sitePath = module.getSourceOwner().getAs<StringAttr>("path");
  auto field = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("field")),
      builder.getNamedAttr("name", builder.getStringAttr("ordered"))});
  auto empty = builder.getArrayAttr({});
  auto indexed = module.walk([&](Operation *op) {
    if (failed(budget.debit(1, op)))
      return WalkResult::interrupt();
    auto index = builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("index")),
        builder.getNamedAttr("value", builder.getI64IntegerAttr(plan.orderedOccurrences.size()))});
    auto site = builder.getDictionaryAttr({
        builder.getNamedAttr("definition", symbol),
        builder.getNamedAttr("ast_path", builder.getArrayAttr({field, index}))});
    plan.orderedOccurrences.try_emplace(op, builder.getDictionaryAttr({
        builder.getNamedAttr("site", site), builder.getNamedAttr("expansion", empty)}));
    return WalkResult::advance();
  });
  if (indexed.wasInterrupted())
    return failure();

  SmallVector<uint64_t> methodSizes(plan.methods.size());
  auto visit = [&](auto &&recurse, Block &body, auto &&onCheck, auto &&onCall) -> void {
    for (Operation &op : body) {
      if (isa<GuardOp>(op) || plan.checkedBranches.contains(&op))
        onCheck(&op);
      if (auto memory = plan.transactionMemoryCalls.find(&op);
          memory != plan.transactionMemoryCalls.end())
        for (unsigned i = 0, count = std::max<unsigned>(1, op.getNumOperands()); i < count; ++i)
          onCheck(&op);
      if (auto method = plan.methodCalls.find(&op); method != plan.methodCalls.end())
        onCall(&op, method->second);
      if (auto branch = dyn_cast<IfOp>(op)) {
        recurse(recurse, branch.getThenBody().front(), onCheck, onCall);
        recurse(recurse, branch.getElseBody().front(), onCheck, onCall);
      }
    }
  };
  auto assignOffsets = [&](Block &body) {
    uint64_t next = 1;
    visit(visit, body, [&](Operation *op) {
            plan.checkOffsets.try_emplace(op, next);
            next = Budget::add(next, 1);
          },
          [&](Operation *call, unsigned method) {
            plan.checkOffsets.try_emplace(call, next);
            next = Budget::add(next, methodSizes[method]);
          });
    return next;
  };
  for (unsigned method : plan.methodOrder)
    methodSizes[method] = assignOffsets(plan.methods[method].getBody().front());
  for (auto &rule : plan.rules) {
    if (!rule.sampled)
      continue;
    auto registration = plan.orderedOccurrences.lookup(rule.operation);
    auto &body = rule.operation.getBody().front();
    auto count = assignOffsets(body);
    if (failed(budget.debit(count, rule.operation)))
      return failure();
    rule.checks.reserve(count);
    auto add = [&](Operation *op) {
      auto loc = dyn_cast<FileLineColLoc>(op->getLoc());
      uint64_t line = loc ? std::max<uint64_t>(loc.getLine(), 1) : 1;
      uint64_t col = loc ? std::max<uint64_t>(loc.getColumn(), 1) : 1;
      auto location = builder.getDictionaryAttr({
          builder.getNamedAttr("path", sitePath),
          builder.getNamedAttr("line", builder.getI64IntegerAttr(line)),
          builder.getNamedAttr("column", builder.getI64IntegerAttr(col)),
          builder.getNamedAttr("end_line", builder.getI64IntegerAttr(line)),
          builder.getNamedAttr("end_column", builder.getI64IntegerAttr(col + 1))});
      auto id = builder.getDictionaryAttr({
          builder.getNamedAttr("registration", registration),
          builder.getNamedAttr("check", plan.orderedOccurrences.lookup(op)),
          builder.getNamedAttr("obligation", builder.getI64IntegerAttr(rule.checks.size()))});
      rule.checks.push_back({id, location});
    };
    add(rule.operation);
    auto expand = [&](auto &&self, Operation *, unsigned method) -> void {
      auto definition = plan.methods[method];
      add(definition);
      visit(visit, definition.getBody().front(), add,
            [&](Operation *call, unsigned next) { self(self, call, next); });
    };
    visit(visit, body, add,
          [&](Operation *call, unsigned method) { expand(expand, call, method); });
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
      auto receiver = call.getReceiverAttr();
      auto memory = receiver ? memories.find(receiver.getValue()) : memories.end();
      if (memory != memories.end()) {
        if (failed(bindRawMemory(call, memory->second)))
          return failure();
      } else if (failed(verifyAttributes(call, {"receiver", "interface"})) ||
                 failed(bindFifo(call, std::nullopt, popped))) {
        return failure();
      }
    } else if (auto read = dyn_cast<ReadOp>(op)) {
      if (failed(bindState(read, read.getStateAttr(), read.getValue(), {}, nullptr, false)))
        return failure();
    }
  }
  for (const auto &memory : plan.memories)
    if (!memory.transactional && !memory.raw)
      return memory.instance->emitOpError("raw Memory instance requires a graph binding");
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
  if (!plan.transactionMemoryCalls.empty() && !plan.fifos.empty())
    return module.emitOpError("transaction Memory cannot yet combine with native FIFO");
  if (failed(resolveMethodEffects()) || failed(buildCheckPlan()))
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
  if (failed(verifyAttributes(primitive, {"sym_name", "native_kind", "version",
                                         "element", "depth", "latency", "ready_policy"})))
    return failure();
  if (primitive.getNativeKind() != "fifo" || primitive.getVersion() != 1)
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

FailureOr<StructuredMemory>
describeRawMemory(PrimitiveOp primitive, const ac::HardwareAnalysis &hardware) {
  if (failed(verifyAttributes(primitive, {"sym_name", "native_kind", "version",
                                         "element", "kind", "depth", "access"})))
    return failure();
  auto kind = primitive->getAttrOfType<StringAttr>("kind");
  auto element = primitive->getAttrOfType<TypeAttr>("element");
  auto access = primitive->getAttrOfType<StringAttr>("access");
  auto depth = primitive->getAttrOfType<IntegerAttr>("depth");
  if (primitive.getVersion() != 1 || !kind ||
      (kind.getValue() != "sync_mem" && kind.getValue() != "sync_mem_dp" &&
       kind.getValue() != "byte_mem") ||
      !access || (access.getValue() != "raw" && access.getValue() != "transaction") || !element ||
      !depth || !depth.getType().isSignlessInteger(64) || depth.getInt() <= 0)
    return primitive.emitOpError("expected Memory v1 with access, kind, element and positive i64 depth");
  auto type = element.getValue();
  bool transactional = access.getValue() == "transaction";
  if (transactional && (kind.getValue() == "byte_mem" || !isa<ac::BitsType>(type)))
    return primitive.emitOpError("transaction Memory requires synchronous Bits storage");
  if (failed(verifyValueType(hardware, type, primitive)))
    return failure();
  auto width = hardware.getPackedWidth(type, {}, primitive);
  if (failed(width))
    return failure();
  if (*width == 0 || *width > std::numeric_limits<unsigned>::max() ||
      (kind.getValue() == "byte_mem" && *width % 8 != 0))
    return primitive.emitOpError("Memory payload has an unsupported packed width");
  uint64_t size = depth.getValue().getZExtValue();
  return StructuredMemory{InstanceOp{}, type, kind.getValue(), size,
                          std::max(1u, llvm::Log2_64_Ceil(size)),
                          unsigned(*width / 8 + (*width % 8 != 0)), CallOp{}, transactional};
}

FailureOr<StructuredClocked>
analyzeClocked(ModuleOp module, const ac::HardwareAnalysis &hardware,
               ac::detail::SourceCheckBudget &budget) {
  return ClockedAnalyzer(module, hardware, budget).run();
}

} // namespace acir::gfir::detail
