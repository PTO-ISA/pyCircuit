#include "ExecutionProjection.h"
#include "mlir/IR/AttrTypeSubElements.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringMap.h"

using namespace mlir;

namespace acir::compiler {

LogicalResult validateExecutionModelDeclaration(Operation *owner) {
  auto raw = owner->getAttr("ac.execution_model");
  if (!raw)
    return success();
  if (auto dictionary = dyn_cast<DictionaryAttr>(raw)) {
    if (auto profile = dictionary.getAs<StringAttr>("profile")) {
      auto state = dictionary.getAs<IntegerAttr>("state_output");
      if (profile.getValue() != "clocked_fixedpoint_v1" ||
          dictionary.size() != 2 || !state || state.getInt() < 0 ||
          state.getInt() > 7)
        return owner->emitOpError(
            "clocked fixed-point declaration requires its profile and finite "
            "state_output ordinal");
      return success();
    }
  }
  auto contract = dyn_cast<DictionaryAttr>(raw);
  if (!contract || contract.size() != 13)
    return owner->emitOpError(
        "execution model requires its complete closed role/timing declaration");
  for (StringRef key :
       {"worker", "ingress", "egress", "process", "snapshot", "submit", "take",
        "transfer", "enabled", "valid", "accepted", "wait_state"}) {
    auto value = contract.getAs<StringAttr>(key);
    if (!value || (value.getValue().empty() && key != "wait_state"))
      return owner->emitOpError()
             << "execution model requires a literal role: " << key;
  }
  auto interval = contract.getAs<IntegerAttr>("interval");
  if (!interval || (interval.getInt() != 1 && interval.getInt() != 2) ||
      ((interval.getInt() == 1) !=
       contract.getAs<StringAttr>("wait_state").getValue().empty()))
    return owner->emitOpError("execution model admits II1 or II2 with matching "
                              "private wait-state ownership");
  if (contract.getAs<StringAttr>("accepted") ==
      contract.getAs<StringAttr>("wait_state"))
    return owner->emitOpError("architectural acceptance and private waiting "
                              "require distinct state owners");
  return success();
}

namespace {

Operation *make(OpBuilder &builder, Location location, StringRef name,
                ValueRange operands, TypeRange results,
                ArrayRef<NamedAttribute> attributes = {},
                unsigned regions = 0) {
  OperationState state(location, name);
  state.addOperands(operands);
  state.addTypes(results);
  state.addAttributes(attributes);
  for (unsigned i = 0; i < regions; ++i)
    state.addRegion();
  return builder.create(state);
}

struct ModelPlan {
  DictionaryAttr contract;
  llvm::StringMap<std::string> roles;
  ac::InstanceOp worker, ingress, egress;
  ac::ModuleOp workerDefinition;
  ac::SourceMethodOp process, snapshot, submit, transfer;
  ac::SourceStateOp accepted;
  SmallVector<ac::SourceStateOp> waiting;
  Type requestType, responseType, enableType;
};

bool callOn(ac::SourceMethodCallOp call, StringRef instance, StringRef method) {
  auto path = call.getInstancePath();
  return path.size() == 1 && cast<StringAttr>(path[0]).getValue() == instance &&
         call.getCallee().getLeafReference().getValue() == method;
}

bool literalIs(Value value, uint64_t expected, ac::HardwareAnalysis &analysis,
               Operation *site, unsigned depth = 0) {
  if (depth > 128)
    return false;
  if (auto extract = value.getDefiningOp<ac::BitsExtractOp>()) {
    auto low = analysis.evaluateStatic(extract.getLow(), {}, site);
    auto integer =
        succeeded(low) ? dyn_cast<ac::MathIntAttr>(*low) : ac::MathIntAttr();
    if ((expected == 0 ||
         (expected == 1 && integer && integer.getCanonicalValue() == "0")) &&
        literalIs(extract.getInput(), expected, analysis, site, depth + 1))
      return true;
  }
  if (auto resize = value.getDefiningOp<ac::BitsResizeOp>()) {
    auto width = analysis.getPackedWidth(resize.getInput().getType(), {}, site);
    if ((expected == 0 ||
         (expected == 1 &&
          (resize.getMode() != "sext" || (succeeded(width) && *width > 1)))) &&
        literalIs(resize.getInput(), expected, analysis, site, depth + 1))
      return true;
  }
  auto constant = value.getDefiningOp<ac::BitsConstantOp>();
  if (!constant)
    return false;
  auto evaluated = analysis.evaluateStatic(constant.getValue(), {}, site);
  auto integer = succeeded(evaluated) ? dyn_cast<ac::MathIntAttr>(*evaluated)
                                      : ac::MathIntAttr();
  uint64_t actual = 0;
  return integer && !integer.getValue().getValue().getAsInteger(10, actual) &&
         actual == expected;
}

bool nonzero(Value value, ac::HardwareAnalysis &analysis, Operation *site,
             unsigned depth = 0) {
  if (depth > 128)
    return false;
  if (literalIs(value, 1, analysis, site))
    return true;
  if (auto constant = value.getDefiningOp<ac::BitsConstantOp>()) {
    auto evaluated = analysis.evaluateStatic(constant.getValue(), {}, site);
    auto integer = succeeded(evaluated) ? dyn_cast<ac::MathIntAttr>(*evaluated)
                                        : ac::MathIntAttr();
    return integer && integer.getValue().getValue() != "0";
  }
  if (auto resize = value.getDefiningOp<ac::BitsResizeOp>()) {
    auto before =
        analysis.getPackedWidth(resize->getOperand(0).getType(), {}, site);
    auto after = analysis.getPackedWidth(value.getType(), {}, site);
    return succeeded(before) && succeeded(after) && *after >= *before &&
           nonzero(resize->getOperand(0), analysis, site, depth + 1);
  }
  if (auto binary = value.getDefiningOp<ac::BitsBinaryOp>())
    if (binary.getOpcode() == "or" &&
        (nonzero(binary.getLhs(), analysis, site, depth + 1) ||
         nonzero(binary.getRhs(), analysis, site, depth + 1)))
      return true;
  auto select = value.getDefiningOp<ac::BitsSelectOp>();
  if (!select)
    return false;
  bool left = nonzero(select.getTrueValue(), analysis, site, depth + 1);
  bool right = nonzero(select.getFalseValue(), analysis, site, depth + 1);
  if (left && right)
    return true;
  auto compare = select.getCondition().getDefiningOp<ac::BitsCompareOp>();
  if (!compare || !literalIs(compare.getRhs(), 0, analysis, site))
    return false;
  return (compare.getPredicate() == "eq" && left &&
          select.getFalseValue() == compare.getLhs()) ||
         (compare.getPredicate() == "ne" && right &&
          select.getTrueValue() == compare.getLhs());
}

// Known-preserving scalar operations and state are the complete worker slice.
// In particular an opaque Table read, X literal or unproved zero denominator
// cannot turn an already accepted timed request into a later hidden failure.
bool totalScalarOperation(Operation *op, ac::HardwareAnalysis &analysis) {
  if (isa<ac::ModuleOp, ac::SourceMethodOp, ac::SourceStateOp,
          ac::SourceStateReadOp, ac::SourceStateWriteOp, ac::SourceBranchOp,
          ac::YieldOp, ac::BitsConstantOp, ac::BitsUnaryOp, ac::BitsCompareOp,
          ac::BitsSelectOp, ac::BitsConcatOp, ac::BitsExtractOp,
          ac::BitsResizeOp, ac::StructCreateOp, ac::StructGetOp>(op))
    return true;
  if (auto constant = dyn_cast<ac::BitsConstant4Op>(op))
    return constant.getPattern().find_first_of("xXzZ") == StringRef::npos;
  if (auto binary = dyn_cast<ac::BitsBinaryOp>(op))
    return (binary.getOpcode() != "udiv" && binary.getOpcode() != "urem") ||
           nonzero(binary.getRhs(), analysis, op);
  if (auto expect = dyn_cast<ac::SourceExpectOp>(op))
    return expect.getKind() == "guard_known";
  return false;
}

bool closedKnown(Value value, ac::HardwareAnalysis &analysis,
                 unsigned depth = 0) {
  if (depth > 128)
    return false;
  auto operation = value.getDefiningOp();
  if (!operation || !totalScalarOperation(operation, analysis) ||
      (!operation->getName().getStringRef().starts_with("ac.bits.") &&
       !isa<ac::StructCreateOp, ac::StructGetOp>(operation)))
    return false;
  for (Value operand : operation->getOperands())
    if (!closedKnown(operand, analysis, depth + 1))
      return false;
  return true;
}

bool directPredicate(Value condition, Value source,
                     ac::HardwareAnalysis &analysis, Operation *site,
                     unsigned depth = 0) {
  if (depth > 128)
    return false;
  if (condition == source)
    return true;
  if (auto compare = condition.getDefiningOp<ac::BitsCompareOp>()) {
    auto width = analysis.getPackedWidth(source.getType(), {}, site);
    if (succeeded(width) && *width == 1 &&
        directPredicate(compare->getOperand(0), source, analysis, site,
                        depth + 1) &&
        ((compare.getPredicate() == "ne" &&
          literalIs(compare->getOperand(1), 0, analysis, site)) ||
         (compare.getPredicate() == "eq" &&
          literalIs(compare->getOperand(1), 1, analysis, site))))
      return true;
  }
  if (auto extract = condition.getDefiningOp<ac::BitsExtractOp>()) {
    auto a = analysis.getPackedWidth(condition.getType(), {}, site);
    auto b = analysis.getPackedWidth(extract.getInput().getType(), {}, site);
    auto low = analysis.evaluateStatic(extract.getLow(), {}, site);
    auto integer =
        succeeded(low) ? dyn_cast<ac::MathIntAttr>(*low) : ac::MathIntAttr();
    if (succeeded(a) && succeeded(b) && *a == *b && integer &&
        integer.getValue().getValue() == "0")
      return directPredicate(extract.getInput(), source, analysis, site,
                             depth + 1);
  }
  return false;
}

LogicalResult verifyTransport(ac::ModuleOp root, ModelPlan &plan,
                              ac::HardwareAnalysis &analysis) {
  SmallVector<ac::SourceMethodCallOp> submitCalls, transferCalls, takeCalls;
  plan.submit.walk(
      [&](ac::SourceMethodCallOp call) { submitCalls.push_back(call); });
  plan.transfer.walk(
      [&](ac::SourceMethodCallOp call) { transferCalls.push_back(call); });
  ac::SourceMethodOp take;
  for (auto method : root.getBody().front().getOps<ac::SourceMethodOp>())
    if (method.getSymName() == plan.roles["take"])
      take = method;
  if (take)
    take.walk([&](ac::SourceMethodCallOp call) { takeCalls.push_back(call); });
  if (submitCalls.size() != 1 || transferCalls.size() != 3 ||
      takeCalls.size() != 1 ||
      !callOn(submitCalls[0], plan.roles["ingress"], "push") ||
      !callOn(takeCalls[0], plan.roles["egress"], "pop"))
    return root.emitOpError(
        "execution transport requires one admission, ordered worker transfer "
        "and one result consumption");
  auto pop = transferCalls[0], process = transferCalls[1],
       push = transferCalls[2];
  if (!callOn(pop, plan.roles["ingress"], "pop") ||
      !callOn(process, plan.roles["worker"], plan.roles["process"]) ||
      !callOn(push, plan.roles["egress"], "push") || pop.getNumResults() != 1 ||
      process.getArguments().size() != 1 ||
      process.getArguments()[0] != pop.getResult(0) ||
      push.getArguments().size() != 1 ||
      push.getArguments()[0] != process.getResult(0) ||
      pop->getBlock() != process->getBlock() ||
      !pop->isBeforeInBlock(process) ||
      submitCalls[0].getArguments()[0] !=
          plan.submit.getBody().front().getArgument(0))
    return root.emitOpError(
        "execution transport must snapshot the admitted request and preserve "
        "its single FIFO worker/result dependency");
  Value enabled, valid;
  plan.submit.walk([&](ac::StructGetOp field) {
    if (field.getField() == plan.roles["enabled"] &&
        field.getValue() == plan.submit.getBody().front().getArgument(0))
      enabled = field.getResult();
  });
  plan.transfer.walk([&](ac::StructGetOp field) {
    if (field.getField() == plan.roles["valid"] &&
        field.getValue() == process.getResult(0))
      valid = field.getResult();
  });
  auto admission = dyn_cast<ac::SourceBranchOp>(submitCalls[0]->getParentOp());
  auto publication = dyn_cast<ac::SourceBranchOp>(push->getParentOp());
  if (!enabled || !valid || !admission || !publication ||
      submitCalls[0]->getParentRegion() != &admission->getRegion(0) ||
      push->getParentRegion() != &publication->getRegion(0) ||
      publication->getBlock() != process->getBlock() ||
      !directPredicate(admission.getCondition(), enabled, analysis, root) ||
      !directPredicate(publication.getCondition(), valid, analysis, root))
    return root.emitOpError("execution admission and result publication must "
                            "match their declared predicates without additional "
                            "publication conditions");
  unsigned acceptedWrites = 0;
  unsigned admissionBranches = 0;
  bool unsafe = false;
  plan.submit.walk([&](Operation *operation) {
    if (isa<ac::SourceBranchOp>(operation))
      ++admissionBranches;
    if (auto write = dyn_cast<ac::SourceStateWriteOp>(operation)) {
      ++acceptedWrites;
      auto add = write.getValue().getDefiningOp<ac::BitsBinaryOp>();
      auto read =
          add ? add->getOperand(0).getDefiningOp<ac::SourceStateReadOp>()
              : ac::SourceStateReadOp();
      unsafe |= write.getState() != plan.roles["accepted"] ||
                !write.getSelectors().empty() || !add ||
                add.getOpcode() != "add" || !read ||
                read.getState() != write.getState() ||
                !literalIs(add->getOperand(1), 1, analysis, root) ||
                write->getParentRegion() != submitCalls[0]->getParentRegion();
    }
    if (isa<ac::SourceObserveOp, ac::SourceExpectOp>(operation))
      unsafe = true;
  });
  for (auto method :
       {plan.submit, plan.transfer, take, plan.snapshot, plan.process}) {
    auto yield = cast<ac::YieldOp>(method.getGuard().front().getTerminator());
    if (!literalIs(yield->getOperand(0), 1, analysis, root))
      unsafe = true;
  }
  if (acceptedWrites != 1 || admissionBranches != 1 || unsafe)
    return root.emitOpError(
        "execution admission requires only its enabled exactly-once counter "
        "effect and total callable guards");
  auto returned =
      cast<ac::YieldOp>(plan.submit.getBody().front().getTerminator());
  if (returned->getNumOperands() != 1 ||
      !literalIs(returned->getOperand(0), 1, analysis, root))
    return plan.submit.emitOpError(
        "execution submission must acknowledge one logical offer without a "
        "time-dependent result");
  take.walk([&](Operation *op) {
    if (isa<ac::SourceStateWriteOp, ac::SourceObserveOp, ac::SourceExpectOp>(
            op))
      unsafe = true;
  });
  auto taken = cast<ac::YieldOp>(take.getBody().front().getTerminator());
  if (take.getFunctionType().getNumInputs() || taken->getNumOperands() != 1 ||
      taken->getOperand(0) != takeCalls[0].getResult(0))
    unsafe = true;
  for (auto method : {plan.submit, plan.transfer, take})
    method.walk([&](Operation *op) {
      if (!isa<ac::SourceMethodCallOp>(op) &&
          !totalScalarOperation(op, analysis))
        unsafe = true;
    });
  unsigned registrations = 0;
  for (auto tx : root.getBody().front().getOps<ac::SourceTransactionOp>()) {
    ++registrations;
    SmallVector<ac::SourceMethodCallOp> calls;
    tx.walk([&](ac::SourceMethodCallOp call) { calls.push_back(call); });
    if (calls.size() != 1 || !calls[0].getInstancePath().empty() ||
        calls[0].getCallee().getLeafReference().getValue() !=
            plan.roles["transfer"])
      unsafe = true;
    auto guard = cast<ac::YieldOp>(tx.getGuard().front().getTerminator());
    if (!literalIs(guard->getOperand(0), 1, analysis, root))
      unsafe = true;
    // A registration is a pure unconditional invocation, never another owner
    // of state, a clock counter, a cancellation predicate or a second effect.
    tx.walk([&](Operation *op) {
      if (op == tx.getOperation() ||
          isa<ac::SourceMethodCallOp, ac::YieldOp>(op))
        return;
      if (op->getNumResults() != 1 || !closedKnown(op->getResult(0), analysis))
        unsafe = true;
    });
  }
  if (registrations != 1)
    unsafe = true;
  Value gap;
  SmallVector<ac::SourceStateWriteOp> gapWrites;
  ac::SourceBranchOp gapBranch;
  plan.transfer.walk([&](ac::SourceStateReadOp read) {
    if (read.getState() == plan.roles["wait_state"] &&
        !plan.roles["wait_state"].empty())
      gap = read.getValue();
  });
  plan.transfer.walk([&](ac::SourceBranchOp branch) {
    if (directPredicate(branch.getCondition(), valid, analysis, root))
      return;
    if (gap && directPredicate(branch.getCondition(), gap, analysis, root) &&
        !gapBranch) {
      gapBranch = branch;
      return;
    }
    unsafe = true;
  });
  plan.transfer.walk([&](ac::SourceStateWriteOp write) {
    if (plan.roles["wait_state"].empty() ||
        write.getState() != plan.roles["wait_state"] ||
        !write.getSelectors().empty())
      unsafe = true;
    else
      gapWrites.push_back(write);
  });
  plan.transfer.walk([&](Operation *op) {
    if (isa<ac::SourceObserveOp, ac::SourceExpectOp>(op))
      unsafe = true;
  });
  if (plan.contract.getAs<IntegerAttr>("interval").getInt() == 2) {
    auto width = gap ? analysis.getPackedWidth(gap.getType(), {}, root)
                     : FailureOr<uint64_t>(failure());
    if (failed(width) || *width != 1 || !gapBranch || gapWrites.size() != 2 ||
        pop->getParentRegion() != &gapBranch->getRegion(1))
      unsafe = true;
    unsigned clearCount = 0, setCount = 0;
    for (auto write : gapWrites) {
      if (!gapBranch) {
        unsafe = true;
        continue;
      }
      bool clears = literalIs(write.getValue(), 0, analysis, root) &&
                    write->getParentRegion() == &gapBranch->getRegion(0);
      bool sets = literalIs(write.getValue(), 1, analysis, root) &&
                  write->getParentRegion() == &gapBranch->getRegion(1);
      if (!clears && !sets)
        unsafe = true;
      clearCount += clears;
      setCount += sets;
    }
    if (clearCount != 1 || setCount != 1)
      unsafe = true;
  } else if (gap || gapBranch || !gapWrites.empty())
    unsafe = true;
  if (unsafe)
    return root.emitOpError(
        "extra arbitration, cancellation, timing-dependent effects or an "
        "unproved II controller prevents projection");
  return success();
}

FailureOr<ModelPlan> planModel(mlir::ModuleOp package, ac::ModuleOp root) {
  ModelPlan plan;
  plan.contract = root->getAttrOfType<DictionaryAttr>("ac.execution_model");
  if (!plan.contract)
    return root.emitOpError(
        "execution selection requires an explicit model contract");
  if (failed(validateExecutionModelDeclaration(root)))
    return failure();
  for (StringRef key :
       {"worker", "ingress", "egress", "process", "snapshot", "submit", "take",
        "transfer", "enabled", "valid", "accepted", "wait_state"})
    plan.roles[key] = plan.contract.getAs<StringAttr>(key).getValue().str();
  auto signature = root.getFunctionType();
  if (signature.getNumInputs() != 2 || signature.getNumResults() ||
      !root->hasAttr("ac.domain_inputs") || !root.getParameters().empty())
    return root.emitOpError("execution model requires one closed domain and a "
                            "callable-only interface");
  ac::HardwareAnalysis analysis(package);
  for (Operation &op : root.getBody().front()) {
    if (isa<ac::SourceMethodOp, ac::SourceTransactionOp, ac::SourceStateOp,
            ac::InstanceOp, ac::YieldOp>(op))
      continue;
    if (op.getNumResults() != 1 || !closedKnown(op.getResult(0), analysis))
      return op.emitOpError(
          "execution wrapper may not own additional observable work");
  }
  unsigned allocations = 0;
  for (auto instance : root.getBody().front().getOps<ac::InstanceOp>()) {
    ++allocations;
    if (instance.getInstanceName() == plan.roles["worker"])
      plan.worker = instance;
    else if (instance.getInstanceName() == plan.roles["ingress"])
      plan.ingress = instance;
    else if (instance.getInstanceName() == plan.roles["egress"])
      plan.egress = instance;
  }
  if (allocations != 3 || !plan.worker || !plan.ingress || !plan.egress)
    return root.emitOpError("execution model requires exactly one worker and "
                            "two exclusively owned queues");
  plan.workerDefinition =
      dyn_cast_or_null<ac::ModuleOp>(analysis.resolveCallee(plan.worker));
  if (!plan.workerDefinition)
    return root.emitOpError(
        "execution worker requires a supplied source definition");
  if (plan.workerDefinition.getFunctionType().getNumInputs() != 2 ||
      !plan.workerDefinition.getParameters().empty() ||
      !plan.workerDefinition.getTypeParameters().empty() ||
      plan.worker.getInputs().size() != 2 ||
      plan.worker.getInputs()[0] != root.getBody().front().getArgument(0) ||
      plan.worker.getInputs()[1] != root.getBody().front().getArgument(1))
    return root.emitOpError(
        "execution worker must share the model's sole clock/reset domain and "
        "have no hidden timed inputs");
  for (auto queue : {plan.ingress, plan.egress}) {
    auto definition = analysis.resolveCallee(queue);
    if (analysis.getPrimitiveKind(definition) != "fifo_downstream_pop")
      return queue.emitOpError("execution transport must be a canonical FIFO");
    if (queue.getInputs()[0] != root.getBody().front().getArgument(0) ||
        queue.getInputs()[1] != root.getBody().front().getArgument(1))
      return queue.emitOpError(
          "cross-domain execution transport is unsupported");
    for (Attribute raw : queue.getParameters()) {
      auto value =
          analysis.evaluateStatic(cast<ac::StaticExprAttr>(raw), {}, queue);
      auto integer = succeeded(value) ? dyn_cast<ac::MathIntAttr>(*value)
                                      : ac::MathIntAttr();
      uint64_t scalar = 0;
      if (!integer || integer.getValue().getValue().getAsInteger(10, scalar) ||
          scalar > 255)
        return queue.emitOpError(
            "execution transport requires bounded static timing/capacity");
    }
  }
  for (auto method :
       plan.workerDefinition.getBody().front().getOps<ac::SourceMethodOp>())
    if (method.getSymName() == plan.roles["process"])
      plan.process = method;
  for (auto method : root.getBody().front().getOps<ac::SourceMethodOp>()) {
    auto name = method.getSymName();
    if (name == plan.roles["snapshot"])
      plan.snapshot = method;
    else if (name == plan.roles["submit"])
      plan.submit = method;
    else if (name == plan.roles["transfer"])
      plan.transfer = method;
    else if (name != plan.roles["take"])
      return method.emitOpError(
          "extra callable/autonomous work prevents functional projection");
  }
  if (!plan.process || !plan.snapshot || !plan.submit || !plan.transfer ||
      plan.process.getFunctionType().getNumInputs() != 1 ||
      plan.process.getFunctionType().getNumResults() != 1 ||
      plan.submit.getFunctionType().getNumInputs() != 1 ||
      plan.snapshot.getFunctionType().getNumInputs())
    return root.emitOpError(
        "execution roles require one deterministic request/response signature "
        "and read-only snapshot");
  plan.requestType = plan.process.getFunctionType().getInput(0);
  plan.responseType = plan.process.getFunctionType().getResult(0);
  if (!ac::areEquivalentHardwareTypes(
          plan.requestType, plan.submit.getFunctionType().getInput(0)))
    return root.emitOpError("submit and worker request types differ");
  for (auto state : root.getBody().front().getOps<ac::SourceStateOp>()) {
    if (state.getInstanceName() == plan.roles["accepted"])
      plan.accepted = state;
    else if (state.getInstanceName() == plan.roles["wait_state"])
      plan.waiting.push_back(state);
    else
      return state.emitOpError("architectural timer or additional wrapper "
                               "state cannot be projected");
  }
  if (!plan.accepted || !isa<ac::BitsType>(plan.accepted.getQ().getType()))
    return root.emitOpError("execution acceptance counter must be explicitly "
                            "owned finite Bits state");
  if (!closedKnown(plan.accepted.getInitial(), analysis))
    return plan.accepted.emitOpError(
        "execution acceptance state requires a closed known initial value");
  for (auto state : plan.waiting)
    if (!closedKnown(state.getInitial(), analysis))
      return state.emitOpError(
          "private timing state requires a closed known initial value");
  bool unsafe = false;
  Operation *partialOperation = nullptr;
  plan.snapshot.walk([&](Operation *op) {
    if (auto call = dyn_cast<ac::SourceMethodCallOp>(op)) {
      if (call.getInstancePath().size() != 1 ||
          cast<StringAttr>(call.getInstancePath()[0]).getValue() !=
              plan.roles["worker"] ||
          call.getCallee().getLeafReference().getValue() ==
              plan.roles["process"])
        unsafe = true;
    } else if (!totalScalarOperation(op, analysis))
      unsafe = true;
    if (isa<ac::SourceStateWriteOp, ac::SourceObserveOp, ac::SourceExpectOp>(
            op))
      unsafe = true;
    if (auto read = dyn_cast<ac::SourceStateReadOp>(op))
      if (read.getState() == plan.roles["wait_state"] &&
          !plan.roles["wait_state"].empty())
        unsafe = true;
  });
  plan.workerDefinition.walk([&](Operation *op) {
    if (!totalScalarOperation(op, analysis)) {
      unsafe = true;
      if (!partialOperation)
        partialOperation = op;
    }
    if (isa<ac::InstanceOp, ac::SourceTransactionOp, ac::SourceObserveOp,
            ac::SourceMethodCallOp>(op))
      unsafe = true;
    if (auto state = dyn_cast<ac::SourceStateOp>(op))
      if (!closedKnown(state.getInitial(), analysis))
        unsafe = true;
    if (auto expect = dyn_cast<ac::SourceExpectOp>(op))
      if (expect.getKind() != "guard_known")
        unsafe = true;
  });
  for (auto method :
       plan.workerDefinition.getBody().front().getOps<ac::SourceMethodOp>()) {
    if (method == plan.process)
      continue;
    method.walk([&](Operation *op) {
      if (isa<ac::SourceStateWriteOp, ac::SourceExpectOp>(op))
        unsafe = true;
    });
    auto guard = cast<ac::YieldOp>(method.getGuard().front().getTerminator());
    if (!literalIs(guard->getOperand(0), 1, analysis, method))
      unsafe = true;
  }
  if (partialOperation) {
    auto diagnostic = partialOperation->emitError(
        "worker operation is not proved total on known requests: ");
    diagnostic << partialOperation->getName();
    if (auto binary = dyn_cast<ac::BitsBinaryOp>(partialOperation)) {
      diagnostic << " opcode=" << binary.getOpcode();
      if (auto denominator = binary.getRhs().getDefiningOp())
        diagnostic << " denominator=" << denominator->getName();
    }
    return failure();
  }
  if (unsafe)
    return root.emitOpError("worker resources/architectural time/observable "
                            "waiting or non-total effects prevent projection");
  auto request = dyn_cast<ac::StructType>(plan.requestType);
  if (!request)
    return root.emitOpError(
        "execution request requires a nominal finite record");
  for (auto definition : analysis.getStructs())
    if (definition.getSymName() == request.getName())
      for (Attribute raw : definition.getFields()) {
        auto field = cast<DictionaryAttr>(raw);
        if (field.getAs<StringAttr>("name").getValue() == plan.roles["enabled"])
          plan.enableType = field.getAs<TypeAttr>("type").getValue();
      }
  auto width = plan.enableType
                   ? analysis.getPackedWidth(plan.enableType, {}, root)
                   : FailureOr<uint64_t>(failure());
  if (failed(width) || *width != 1)
    return root.emitOpError("execution enabled field requires one bit");
  if (failed(verifyTransport(root, plan, analysis)))
    return failure();
  return plan;
}

// The accepted request is a finite flat Bits record in this profile. Checking
// each leaf preserves inactive X data while rejecting an active unknown payload
// before either queue acceptance or an architectural worker/counter effect.
LogicalResult requireKnownRequest(mlir::ModuleOp package, OpBuilder &builder,
                                  Value request, Value path, Type bit,
                                  DictionaryAttr origin, DictionaryAttr span) {
  ac::HardwareAnalysis analysis(package);
  auto type = dyn_cast<ac::StructType>(request.getType());
  auto record = type ? analysis.lookupStruct(type) : ac::StructOp();
  if (!record)
    return package.emitError("execution request must be a nominal Bits record");
  auto loc = request.getLoc();
  auto one =
      make(builder, loc, ac::BitsConstantOp::getOperationName(), {}, {bit},
           {builder.getNamedAttr("value", cast<ac::BitsType>(bit).getWidth())})
          ->getResult(0);
  Value known = one;
  for (Attribute raw : record.getFields()) {
    auto field = cast<DictionaryAttr>(raw);
    auto payload = field.getAs<TypeAttr>("type").getValue();
    if (!isa<ac::BitsType>(payload))
      return record.emitOpError(
          "execution request fields require finite Bits payloads");
    auto value =
        make(builder, loc, ac::StructGetOp::getOperationName(), {request},
             {payload},
             {builder.getNamedAttr("field", field.getAs<StringAttr>("name"))})
            ->getResult(0);
    auto equal =
        make(builder, loc, ac::BitsCompareOp::getOperationName(),
             {value, value}, {bit},
             {builder.getNamedAttr("predicate", builder.getStringAttr("eq"))})
            ->getResult(0);
    auto observer =
        make(builder, loc, ac::BitsCompareOp::getOperationName(), {equal, one},
             {bit},
             {builder.getNamedAttr("predicate",
                                   builder.getStringAttr("case_eq")),
              builder.getNamedAttr("ac.verification_observer",
                                   builder.getStringAttr("is_known")),
              builder.getNamedAttr("ac.origin", origin)})
            ->getResult(0);
    known = make(builder, loc, ac::BitsBinaryOp::getOperationName(),
                 {known, observer}, {bit},
                 {builder.getNamedAttr("opcode", builder.getStringAttr("and"))})
                ->getResult(0);
  }
  make(builder, loc, ac::SourceExpectOp::getOperationName(), {known, path}, {},
       {builder.getNamedAttr("kind", builder.getStringAttr("assert")),
        builder.getNamedAttr("location", span),
        builder.getNamedAttr("occurrence", origin),
        builder.getNamedAttr(
            "ac.message",
            builder.getStringAttr(
                "execution admission requires known request data"))});
  return success();
}

// Remove only captures proved unused in both regions. This makes it possible
// to retain the original architectural snapshot Method without retaining any
// transport allocation as a hidden functional effect.
void removeUnusedCaptures(ac::SourceMethodOp method) {
  unsigned explicitInputs = method.getFunctionType().getNumInputs();
  for (unsigned n = method.getCaptures().size(); n; --n) {
    unsigned index = explicitInputs + n - 1;
    auto &guard = method.getGuard().front();
    auto &body = method.getBody().front();
    if (!guard.getArgument(index).use_empty() ||
        !body.getArgument(index).use_empty())
      continue;
    guard.eraseArgument(index);
    body.eraseArgument(index);
    method->eraseOperand(n - 1);
  }
}

// Both strategies expose the same typed request/response protocol. The timing
// view retains its queues and autonomous transfer; the functional view exposes
// a quiescent transaction entry with an always-ready result consumer.
class ModelInterface {
public:
  ModelInterface(ac::ModuleOp root, ModelPlan &plan, bool timing)
      : root(root), plan(plan), builder(root.getContext()), loc(root.getLoc()),
        origin(root->getAttrOfType<DictionaryAttr>("ac.origin")),
        span(
            plan.snapshot->getAttrOfType<DictionaryAttr>("ac.guard_location")) {
    auto &graph = root.getBody().front();
    bit = graph.getArgument(0).getType();
    request = graph.insertArgument(0u, plan.requestType, loc);
    active = graph.insertArgument(1u, bit, loc);
    if (timing)
      consume = graph.insertArgument(2u, bit, loc);
    unsigned n = timing ? 3 : 2;
    clock = graph.getArgument(n);
    reset = graph.getArgument(n + 1);
    builder.setInsertionPoint(graph.getTerminator());
    one = make(builder, loc, ac::BitsConstantOp::getOperationName(), {}, {bit},
               {builder.getNamedAttr("value",
                                     cast<ac::BitsType>(bit).getWidth())})
              ->getResult(0);
  }

  ac::SourceTransactionOp transaction(StringRef name, TypeRange results,
                                      DictionaryAttr declaration,
                                      DictionaryAttr registration,
                                      unsigned guardArgument) {
    builder.setInsertionPoint(root.getBody().front().getTerminator());
    SmallVector<Value> captures{request, one, active};
    if (consume)
      captures.push_back(consume);
    SmallVector<Value> operands{clock, reset};
    llvm::append_range(operands, captures);
    auto op = make(builder, loc, ac::SourceTransactionOp::getOperationName(),
                   operands, results,
                   {builder.getNamedAttr("name", builder.getStringAttr(name)),
                    builder.getNamedAttr("declaration", declaration),
                    builder.getNamedAttr("occurrence", registration),
                    builder.getNamedAttr("ac.guard_location", span)},
                   2);
    for (Region &region : op->getRegions()) {
      auto block = new Block();
      region.push_back(block);
      for (Value value : captures)
        block->addArgument(value.getType(), loc);
    }
    builder.setInsertionPointToEnd(&op->getRegion(0).front());
    make(builder, loc, ac::YieldOp::getOperationName(),
         {op->getRegion(0).front().getArgument(guardArgument)}, {});
    builder.setInsertionPointToEnd(&op->getRegion(1).front());
    return cast<ac::SourceTransactionOp>(op);
  }

  Operation *call(ac::SourceTransactionOp tx, ac::SourceMethodOp method,
                  ac::ModuleOp owner, ArrayRef<StringRef> path,
                  ValueRange arguments, DictionaryAttr site) {
    auto &body = tx.getBody().front();
    builder.setInsertionPointToEnd(&body);
    SmallVector<Value> operands{body.getArgument(1)};
    llvm::append_range(operands, arguments);
    return make(
        builder, loc, ac::SourceMethodCallOp::getOperationName(), operands,
        method.getFunctionType().getResults(),
        {builder.getNamedAttr(
             "callee",
             SymbolRefAttr::get(root.getContext(), owner.getSymName(),
                                {FlatSymbolRefAttr::get(root.getContext(),
                                                        method.getSymName())})),
         builder.getNamedAttr("instance_path", builder.getStrArrayAttr(path)),
         builder.getNamedAttr("occurrence", site)});
  }

  void yield(ValueRange values) {
    make(builder, loc, ac::YieldOp::getOperationName(), values, {});
  }

  void finish(Value ack, Value completion, ValueRange snapshots,
              StringRef mode) {
    SmallVector<Value> outputs{ack, completion};
    llvm::append_range(outputs, snapshots);
    auto &graph = root.getBody().front();
    graph.getTerminator()->setOperands(outputs);
    SmallVector<Type> types;
    for (Value value : outputs)
      types.push_back(value.getType());
    root.setFunctionType(
        FunctionType::get(root.getContext(), graph.getArgumentTypes(), types));
    SmallVector<Attribute> inputNames{builder.getStringAttr("request"),
                                      builder.getStringAttr("active")};
    if (consume)
      inputNames.push_back(builder.getStringAttr("consume"));
    unsigned sourceInputs = inputNames.size();
    inputNames.push_back(builder.getStringAttr("pyc_clk"));
    inputNames.push_back(builder.getStringAttr("pyc_rst"));
    root.setInputNamesAttr(builder.getArrayAttr(inputNames));
    SmallVector<Attribute> names{builder.getStringAttr("ack"),
                                 builder.getStringAttr("completion")};
    for (unsigned n = 0; n < snapshots.size(); ++n)
      names.push_back(builder.getStringAttr("snapshot_" + std::to_string(n)));
    root.setOutputNamesAttr(builder.getArrayAttr(names));
    root->setAttr("ac.domain_inputs",
                  builder.getDictionaryAttr(
                      {builder.getNamedAttr(
                           "clock", builder.getI64IntegerAttr(sourceInputs)),
                       builder.getNamedAttr("reset", builder.getI64IntegerAttr(
                                                         sourceInputs + 1))}));
    SmallVector<Attribute> parameters;
    for (unsigned n = 0; n < sourceInputs; ++n) {
      Type type = graph.getArgument(n).getType();
      auto constraint = builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr("hardware")),
           builder.getNamedAttr("type", TypeAttr::get(type)),
           builder.getNamedAttr(
               "source_kind",
               builder.getStringAttr(n == 0 ? "nominal" : "fixed_bits"))});
      parameters.push_back(builder.getDictionaryAttr(
          {builder.getNamedAttr("name", inputNames[n]),
           builder.getNamedAttr("binding",
                                builder.getStringAttr("positional_or_keyword")),
           builder.getNamedAttr("constraint", constraint),
           builder.getNamedAttr("default",
                                builder.getDictionaryAttr({builder.getNamedAttr(
                                    "present", builder.getBoolAttr(false))})),
           builder.getNamedAttr("origin", origin),
           builder.getNamedAttr("location", span)}));
    }
    SmallVector<Attribute> constraints{builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("hardware")),
         builder.getNamedAttr("type", TypeAttr::get(bit)),
         builder.getNamedAttr("source_kind",
                              builder.getStringAttr("fixed_bits"))})};
    llvm::append_range(constraints, plan.process->getAttrOfType<ArrayAttr>(
                                        "ac.result_constraints"));
    llvm::append_range(constraints, plan.snapshot->getAttrOfType<ArrayAttr>(
                                        "ac.result_constraints"));
    root->setAttr("ac.parameters", builder.getArrayAttr(parameters));
    root->setAttr("ac.result_constraints", builder.getArrayAttr(constraints));
    root->setAttr("ac.return_form", builder.getStringAttr("tuple"));
    root->setAttr("ac.result_availability",
                  builder.getArrayAttr(SmallVector<Attribute>(
                      types.size(), builder.getStringAttr("compiler"))));
    root->setAttr("ac.execution_selection", builder.getStringAttr(mode));
  }

  ac::SourceTransactionOp snapshot() {
    auto tx = transaction("execution_snapshot",
                          plan.snapshot.getFunctionType().getResults(),
                          plan.snapshot.getOccurrence(), origin, 1);
    auto op =
        call(tx, plan.snapshot, root, {}, {}, plan.snapshot.getOccurrence());
    yield(op->getResults());
    return tx;
  }

  ac::ModuleOp root;
  ModelPlan &plan;
  OpBuilder builder;
  Location loc;
  DictionaryAttr origin, span;
  Type bit;
  Value request, active, consume, clock, reset, one;
};

LogicalResult materializeFunctional(mlir::ModuleOp package, ac::ModuleOp root,
                                    ModelPlan &plan) {
  auto executeOrigin = plan.transfer.getOccurrence();
  auto submitOrigin = plan.submit.getOccurrence();
  auto ingressOrigin = plan.ingress.getOccurrence();
  for (auto method : llvm::make_early_inc_range(
           root.getBody().front().getOps<ac::SourceMethodOp>()))
    if (method != plan.snapshot)
      method.erase();
  for (auto tx : llvm::make_early_inc_range(
           root.getBody().front().getOps<ac::SourceTransactionOp>()))
    tx.erase();
  removeUnusedCaptures(plan.snapshot);
  for (auto queue : {plan.ingress, plan.egress}) {
    if (!queue->use_empty())
      return queue.emitOpError(
          "transport output escapes its declared model roles");
    queue.erase();
  }
  for (auto state : plan.waiting)
    state.erase();
  ModelInterface io(root, plan, false);
  auto execute = io.transaction("functional_execute", {plan.responseType},
                                executeOrigin, submitOrigin, 2);
  auto &guard = execute.getGuard().front();
  io.builder.setInsertionPoint(guard.getTerminator());
  auto enabled =
      make(io.builder, io.loc, ac::StructGetOp::getOperationName(),
           {guard.getArgument(0)}, {plan.enableType},
           {io.builder.getNamedAttr(
               "field", io.builder.getStringAttr(plan.roles["enabled"]))})
          ->getResult(0);
  auto selected =
      make(io.builder, io.loc, ac::BitsBinaryOp::getOperationName(),
           {enabled, guard.getArgument(2)}, {io.bit},
           {io.builder.getNamedAttr("opcode", io.builder.getStringAttr("and"))})
          ->getResult(0);
  guard.getTerminator()->setOperands({selected});
  auto &body = execute.getBody().front();
  io.builder.setInsertionPointToEnd(&body);
  auto path = body.getArgument(1);
  if (failed(requireKnownRequest(package, io.builder, body.getArgument(0), path,
                                 io.bit, io.origin, io.span)))
    return failure();
  auto counterType = plan.accepted.getQ().getType();
  auto attributes = SmallVector<NamedAttribute>{
      io.builder.getNamedAttr("state",
                              io.builder.getStringAttr(plan.roles["accepted"])),
      io.builder.getNamedAttr("selectors", io.builder.getArrayAttr({})),
      io.builder.getNamedAttr("occurrence", io.origin)};
  auto read =
      make(io.builder, io.loc, ac::SourceStateReadOp::getOperationName(),
           {path}, {counterType}, attributes);
  auto one = make(io.builder, io.loc, ac::BitsConstantOp::getOperationName(),
                  {}, {counterType},
                  {io.builder.getNamedAttr(
                      "value", cast<ac::BitsType>(io.bit).getWidth())});
  auto next = make(
      io.builder, io.loc, ac::BitsBinaryOp::getOperationName(),
      {read->getResult(0), one->getResult(0)}, {counterType},
      {io.builder.getNamedAttr("opcode", io.builder.getStringAttr("add"))});
  make(io.builder, io.loc, ac::SourceStateWriteOp::getOperationName(),
       {path, next->getResult(0)}, {}, attributes);
  auto result =
      io.call(execute, plan.process, plan.workerDefinition,
              {plan.roles["worker"]}, {body.getArgument(0)}, io.origin);
  io.yield(result->getResults());
  auto ack = io.transaction("functional_offer", {io.bit}, submitOrigin,
                            ingressOrigin, 2);
  io.yield({ack.getBody().front().getArgument(1)});
  auto snapshot = io.snapshot();
  io.finish(ack.getResult(0), execute.getResult(0), snapshot.getResults(),
            "functional");
  return success();
}

LogicalResult materializeTiming(ac::ModuleOp root, ModelPlan &plan) {
  ModelInterface io(root, plan, true);
  auto offer =
      io.transaction("timing_offer", {io.bit}, plan.submit.getOccurrence(),
                     plan.ingress.getOccurrence(), 2);
  auto submitted = io.call(offer, plan.submit, root, {},
                           {offer.getBody().front().getArgument(0)},
                           plan.ingress.getOccurrence());
  io.yield(submitted->getResults());
  ac::SourceMethodOp take;
  for (auto method : root.getBody().front().getOps<ac::SourceMethodOp>())
    if (method.getSymName() == plan.roles["take"])
      take = method;
  auto retire =
      io.transaction("timing_consume", {plan.responseType},
                     take.getOccurrence(), plan.egress.getOccurrence(), 3);
  auto completed =
      io.call(retire, take, root, {}, {}, plan.egress.getOccurrence());
  io.yield(completed->getResults());
  auto snapshot = io.snapshot();
  io.finish(offer.getResult(0), retire.getResult(0), snapshot.getResults(),
            "timing");
  return success();
}

ac::ModuleOp deriveView(mlir::ModuleOp package, ac::ModuleOp model,
                        StringRef mode) {
  std::string oldName = model.getSymName().str();
  std::string base = oldName + "__pyc_" + mode.str(), name = base;
  unsigned collision = 0;
  while (SymbolTable::lookupSymbolIn(package, name))
    name = base + "_" + std::to_string(++collision);
  auto view = cast<ac::ModuleOp>(model->clone());
  view.setSymName(name);
  AttrTypeReplacer replacer;
  replacer.addReplacement(
      [&](SymbolRefAttr symbol) -> std::optional<Attribute> {
        if (symbol.getRootReference().getValue() != oldName)
          return std::nullopt;
        return SymbolRefAttr::get(package.getContext(), name,
                                  symbol.getNestedReferences());
      });
  replacer.recursivelyReplaceElementsIn(view, true, false, true);
  view->setAttr("ac.execution_parent",
                FlatSymbolRefAttr::get(package.getContext(), oldName));
  view->setAttr("ac.execution_origin", model->getAttr("ac.origin"));
  package.getBody()->push_back(view);
  return view;
}

// Runtime stutter reduction is relative to the explicit event/level ABI.
// Its witness must cover every storage bit; this source proof admits no hidden
// resources, observations, stateful descendants or implicit clock-data
// behavior.
bool knownAggregate(Value value, ac::HardwareAnalysis &analysis,
                    unsigned depth = 0) {
  if (depth > 128 || !value.getDefiningOp())
    return false;
  Operation *op = value.getDefiningOp();
  if (!isa<ac::TableCreateOp, ac::TableSplatOp>(op) &&
      (!totalScalarOperation(op, analysis) ||
       (!op->getName().getStringRef().starts_with("ac.bits.") &&
        !isa<ac::StructCreateOp, ac::StructGetOp>(op))))
    return false;
  return llvm::all_of(op->getOperands(), [&](Value operand) {
    return knownAggregate(operand, analysis, depth + 1);
  });
}

LogicalResult validateFixedPoint(mlir::ModuleOp package, ac::ModuleOp root) {
  auto declaration = root->getAttrOfType<DictionaryAttr>("ac.execution_model");
  if (failed(validateExecutionModelDeclaration(root)))
    return failure();
  unsigned output = declaration.getAs<IntegerAttr>("state_output").getInt();
  auto type = root.getFunctionType();
  auto domains = root->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
  if (!domains || type.getNumInputs() != 3 || type.getNumResults() != 2 ||
      output >= 2 || !root.getParameters().empty() ||
      !root.getTypeParameters().empty() ||
      root->hasAttr("ac.reference_parameters") || root->hasAttr("ac.root_kind"))
    return root.emitOpError(
        "fixed-point profile requires one data input, two results, one "
        "clock/reset domain and no references/System");
  auto states =
      llvm::to_vector(root.getBody().front().getOps<ac::SourceStateOp>());
  if (states.size() != 1 || root.getBody().front().getTerminator()->getOperand(
                                output) != states[0].getQ())
    return root.emitOpError("fixed-point state output must be the complete "
                            "identity of the sole aggregate owner");
  ac::HardwareAnalysis analysis(package);
  if (!knownAggregate(states[0].getInitial(), analysis))
    return states[0].emitOpError(
        "fixed-point aggregate requires closed known initialization");
  llvm::SmallPtrSet<Operation *, 16> active, proved;
  std::function<LogicalResult(ac::ModuleOp, bool)> visit =
      [&](ac::ModuleOp module, bool isRoot) -> LogicalResult {
    if (proved.contains(module))
      return success();
    if (!active.insert(module).second)
      return module.emitOpError("fixed-point module hierarchy must be acyclic");
    auto leave = llvm::scope_exit([&] { active.erase(module); });
    auto domain = module->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
    auto clock = domain ? domain.getAs<IntegerAttr>("clock") : IntegerAttr();
    auto reset = domain ? domain.getAs<IntegerAttr>("reset") : IntegerAttr();
    if ((isRoot && (!clock || !reset)) || bool(clock) != bool(reset) ||
        !module.getParameters().empty() ||
        !module.getTypeParameters().empty() ||
        module->hasAttr("ac.reference_parameters"))
      return module.emitOpError("fixed-point hierarchy requires a closed "
                                "actual domain and no references/parameters");
    if (clock) {
      for (auto ordinal : {clock.getInt(), reset.getInt()}) {
        if (ordinal < 0 ||
            ordinal >= module.getBody().front().getNumArguments())
          return module.emitOpError(
              "fixed-point domain ordinal is outside the interface");
        for (OpOperand &use :
             module.getBody().front().getArgument(ordinal).getUses()) {
          auto *owner = use.getOwner();
          bool control =
              isa<ac::SourceStateOp, ac::SourceTransactionOp>(owner) &&
              use.getOperandNumber() < 2;
          if (auto child = dyn_cast<ac::InstanceOp>(owner)) {
            auto callee =
                dyn_cast_or_null<ac::ModuleOp>(analysis.resolveCallee(child));
            auto childDomain =
                callee
                    ? callee->getAttrOfType<DictionaryAttr>("ac.domain_inputs")
                    : DictionaryAttr();
            if (childDomain)
              for (StringRef role : {"clock", "reset"})
                if (auto pin = childDomain.getAs<IntegerAttr>(role))
                  control |= pin.getInt() == use.getOperandNumber();
          }
          if (!control)
            return owner->emitOpError("fixed-point clock/reset may only be "
                                      "lifecycle controls, never data");
        }
      }
    }
    // Stateless Rule components may have ordinary data ports. Their complete
    // source bodies are checked below; only the root owns persistent state.

    for (auto instance : module.getBody().front().getOps<ac::InstanceOp>()) {
      auto child =
          dyn_cast_or_null<ac::ModuleOp>(analysis.resolveCallee(instance));
      auto childDomain =
          child ? child->getAttrOfType<DictionaryAttr>("ac.domain_inputs")
                : DictionaryAttr();
      auto childClock =
          childDomain ? childDomain.getAs<IntegerAttr>("clock") : IntegerAttr();
      auto childReset =
          childDomain ? childDomain.getAs<IntegerAttr>("reset") : IntegerAttr();
      bool sameDomain = !childDomain || childDomain.empty();
      if (childClock && childReset && clock && reset &&
          childClock.getInt() < instance.getInputs().size() &&
          childReset.getInt() < instance.getInputs().size())
        sameDomain = instance.getInputs()[childClock.getInt()] ==
                         module.getBody().front().getArgument(clock.getInt()) &&
                     instance.getInputs()[childReset.getInt()] ==
                         module.getBody().front().getArgument(reset.getInt());
      if (!child || !sameDomain || analysis.getPrimitiveKind(child) != "" ||
          !instance.getParameters().empty() ||
          !instance.getTypeArguments().empty())
        return instance.emitOpError(
            "fixed-point descendants must be stateless owned source modules in "
            "the same actual domain");
      if (failed(visit(child, false)))
        return failure();
    }
    bool unsafe = false;
    Operation *first = nullptr;
    module.walk([&](Operation *op) {
      bool allowed =
          totalScalarOperation(op, analysis) ||
          isa<ac::SourceMethodCallOp, ac::SourceTransactionOp, ac::InstanceOp>(
              op) ||
          (op->getName().getStringRef().starts_with("ac.table.") &&
           !op->getName().getStringRef().contains("write"));
      if (!allowed || (!isRoot && isa<ac::SourceStateOp, ac::SourceStateReadOp,
                                      ac::SourceStateWriteOp>(op))) {
        unsafe = true;
        if (!first)
          first = op;
      }
    });
    if (unsafe)
      return first->emitOpError("operation is outside the fixed-point "
                                "finite-state/pure-child profile");
    for (auto method : module.getBody().front().getOps<ac::SourceMethodOp>()) {
      auto guard = cast<ac::YieldOp>(method.getGuard().front().getTerminator());
      if (!literalIs(guard->getOperand(0), 1, analysis, method))
        return method.emitOpError(
            "fixed-point callable guards must be total/unconditional");
    }
    for (auto rule :
         module.getBody().front().getOps<ac::SourceTransactionOp>()) {
      auto guard = cast<ac::YieldOp>(rule.getGuard().front().getTerminator());
      if (!literalIs(guard->getOperand(0), 1, analysis, rule))
        return rule.emitOpError(
            "fixed-point Rules must be total/unconditional");
    }
    if (isRoot &&
        std::distance(
            module.getBody().front().getOps<ac::SourceTransactionOp>().begin(),
            module.getBody().front().getOps<ac::SourceTransactionOp>().end()) !=
            1)
      return module.emitOpError(
          "fixed-point root requires one atomic clocked Rule");
    proved.insert(module);
    return success();
  };
  return visit(root, true);
}

} // namespace

LogicalResult selectExecutionModel(mlir::ModuleOp package, ac::ModuleOp &root,
                                   StringRef execution) {
  if (!execution.empty() && execution != "timing" && execution != "functional")
    return root.emitOpError("execution must be timing or functional");
  auto fixed = root->getAttrOfType<DictionaryAttr>("ac.execution_model");
  if (fixed && fixed.getAs<StringAttr>("profile")) {
    if (failed(validateFixedPoint(package, root)))
      return failure();
    Builder builder(package.getContext());
    package->setAttr(
        "ac.execution_selection",
        builder.getStringAttr(execution.empty() ? "timing" : execution));
    package->setAttr("ac.execution_protocol",
                     builder.getStringAttr("clocked_fixedpoint_v1"));
    package->setAttr("ac.execution_state_output", fixed.get("state_output"));
    return success();
  }
  SmallVector<std::pair<ac::ModuleOp, ModelPlan>, 0> plans;
  bool selectedModel = false;
  for (auto module : package.getOps<ac::ModuleOp>()) {
    if (!module->hasAttr("ac.execution_model"))
      continue;
    auto declaration =
        module->getAttrOfType<DictionaryAttr>("ac.execution_model");
    if (declaration && declaration.getAs<StringAttr>("profile")) {
      // Supplied units retain every definition. A clocked profile which is
      // not the selected root remains ordinary clocked hardware; its closed
      // declaration is not a request/FIFO role dictionary.
      if (failed(validateFixedPoint(package, module)))
        return failure();
      continue;
    }
    auto plan = planModel(package, module);
    if (failed(plan))
      return failure();
    selectedModel |= module == root;
    plans.emplace_back(module, std::move(*plan));
  }
  if (plans.empty()) {
    if (execution.empty())
      return success();
    return root.emitOpError(
        "execution selection requires a declared single-worker model");
  }
  StringRef mode = execution.empty() ? "timing" : execution;
  if (mode == "functional" && !selectedModel)
    return root.emitOpError(
        "functional selection requires the model itself as top; enclosing "
        "timers or Systems are not projected");
  if (selectedModel) {
    root = deriveView(package, root, mode);
    auto derived = planModel(package, root);
    if (failed(derived))
      return failure();
    Builder builder(package.getContext());
    root->setAttr(
        "ac.execution_timing",
        builder.getDictionaryAttr(
            {builder.getNamedAttr("ingress", derived->ingress.getParameters()),
             builder.getNamedAttr("egress", derived->egress.getParameters()),
             builder.getNamedAttr("interval",
                                  derived->contract.get("interval"))}));
    if (mode == "functional") {
      if (failed(materializeFunctional(package, root, *derived)))
        return failure();
    } else {
      SmallVector<ac::SourceMethodCallOp> calls;
      derived->submit.walk(
          [&](ac::SourceMethodCallOp call) { calls.push_back(call); });
      auto push = calls.front();
      OpBuilder at(push);
      if (failed(requireKnownRequest(
              package, at, push.getArguments()[0], push.getPath(),
              root.getBody().front().getArgument(0).getType(),
              push.getOccurrence(),
              derived->submit->getAttrOfType<DictionaryAttr>(
                  "ac.guard_location"))))
        return failure();
      if (failed(materializeTiming(root, *derived)))
        return failure();
    }
  }
  for (auto &[module, plan] : plans) {
    SmallVector<ac::SourceMethodCallOp> calls;
    plan.submit.walk(
        [&](ac::SourceMethodCallOp call) { calls.push_back(call); });
    auto push = calls.front();
    OpBuilder builder(push);
    if (failed(requireKnownRequest(
            package, builder, push.getArguments()[0], push.getPath(),
            module.getBody().front().getArgument(0).getType(),
            push.getOccurrence(),
            plan.submit->getAttrOfType<DictionaryAttr>("ac.guard_location"))))
      return failure();
    module->setAttr("ac.execution_selection", builder.getStringAttr("timing"));
  }
  Builder builder(package.getContext());
  package->setAttr("ac.execution_selection", builder.getStringAttr(mode));
  package->setAttr(
      "ac.execution_protocol",
      builder.getStringAttr(selectedModel ? "request_response_v1" : "system"));
  if (selectedModel)
    package->setAttr(
        "ac.execution_publication_field",
        root->getAttrOfType<DictionaryAttr>("ac.execution_model").get("valid"));
  return verify(package);
}
} // namespace acir::compiler
