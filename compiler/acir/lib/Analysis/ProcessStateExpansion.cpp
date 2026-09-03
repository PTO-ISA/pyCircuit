#include "ModelAnalysisInternal.h"
#include "ProcessStatePlanInternal.h"

#include "acir/Analysis/ModelAnalysis.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

#include <map>

using namespace mlir;

namespace acir::detail {
namespace {

struct ExpansionContext {
  std::vector<ProcessCallSitePlan> callSites;
  std::vector<uint64_t> iterations;
};

struct ExpansionTask {
  Operation *operation = nullptr;
  ExpansionContext context;
};

std::string contextKey(const ExpansionContext &context, size_t iterationCount) {
  std::string key;
  llvm::raw_string_ostream stream(key);
  stream << 'c' << context.callSites.size() << ':';
  for (const ProcessCallSitePlan &site : context.callSites) {
    stream << site.operationPath().size() << ':' << site.operationPath() << '[';
    for (uint64_t iteration : site.iterationVector())
      stream << iteration << ',';
    stream << "]";
  }
  stream << "i" << iterationCount << ':';
  for (uint64_t iteration :
       llvm::ArrayRef(context.iterations).take_front(iterationCount))
    stream << iteration << ',';
  return key;
}

std::string processDefinitionKey(ac::ProcessOp process) {
  ac::ModuleOp owner = process->getParentOfType<ac::ModuleOp>();
  return ("@" + owner.getSymName() + "::@" + process.getSymName()).str();
}

void indexOperationTree(Operation *root, StringRef rootPath,
                        DenseMap<Operation *, std::string> &paths,
                        DenseMap<Block *, std::string> &blockPaths) {
  struct Task {
    Operation *operation;
    std::string path;
  };
  SmallVector<Task> pending{{root, rootPath.str()}};
  while (!pending.empty()) {
    Task task = std::move(pending.back());
    pending.pop_back();
    paths[task.operation] = task.path;
    SmallVector<Task> children;
    for (auto [regionIndex, region] :
         llvm::enumerate(task.operation->getRegions()))
      for (auto [blockIndex, block] : llvm::enumerate(region)) {
        std::string blockPath = (task.path + "/r" + llvm::Twine(regionIndex) +
                                 "/b" + llvm::Twine(blockIndex))
                                    .str();
        blockPaths[&block] = blockPath;
        for (auto [operationIndex, child] : llvm::enumerate(block))
          children.push_back(
              {&child, (blockPath + "/o" + llvm::Twine(operationIndex)).str()});
      }
    for (Task &child : llvm::reverse(children))
      pending.push_back(std::move(child));
  }
}

} // namespace

FailureOr<ExpandedProcess>
PlanSetBuilder::expandProcess(ac::ProcessOp process,
                              const ac::RawModelStructureLimits &limits) {
  if (failed(ac::verifyProcessLowerability(process, limits)))
    return failure();

  ExpandedProcess expanded;
  expanded.process = process;
  expanded.definitionKey = processDefinitionKey(process);
  uint64_t expansionNodes = 0;
  uint64_t expansionEdges = 0;
  bool budgetFailed = false;
  auto reserveNodes = [&](Operation *origin, uint64_t count) {
    if (count > limits.maxNodes - expansionNodes) {
      origin->emitOpError(
          "pure process expansion exceeds ACIR node/edge limits");
      budgetFailed = true;
      return false;
    }
    expansionNodes += count;
    return true;
  };
  auto reserveEdges = [&](Operation *origin, uint64_t count) {
    if (count > limits.maxEdges - expansionEdges) {
      origin->emitOpError(
          "pure process expansion exceeds ACIR node/edge limits");
      budgetFailed = true;
      return false;
    }
    expansionEdges += count;
    return true;
  };

  ModuleOp file = process->getParentOfType<ModuleOp>();
  FailureOr<ValidatedPureCallGraph> callGraph =
      validatePureProcessCallGraph(file);
  if (failed(callGraph))
    return failure();

  DenseMap<Operation *, std::string> operationPaths;
  DenseMap<Block *, std::string> blockPaths;
  indexOperationTree(process, expanded.definitionKey, operationPaths,
                     blockPaths);
  for (const ValidatedPureFunction &entry : callGraph->functions) {
    func::FuncOp function = entry.function;
    indexOperationTree(
        function,
        (expanded.definitionKey + "/func/@" + function.getSymName()).str(),
        operationPaths, blockPaths);
  }

  auto makeCallSite = [&](Operation *operation,
                          const ExpansionContext &context) {
    auto impl = std::make_shared<ProcessCallSitePlan::Impl>();
    impl->operation = operation;
    impl->operationPath = operationPaths.lookup(operation);
    impl->iterationVector = context.iterations;
    return ProcessCallSitePlan(impl);
  };
  auto makeOriginalOccurrence = [&](Operation *operation,
                                    const ExpansionContext &context) {
    auto original = std::make_shared<ProcessOriginalOccurrence::Impl>();
    original->operation = operation;
    original->operationPath = operationPaths.lookup(operation);
    original->callSites = context.callSites;
    original->iterationVector = context.iterations;
    auto occurrence = std::make_shared<ProcessOccurrenceId::Impl>();
    occurrence->kind = ProcessOccurrenceKind::Original;
    occurrence->original = ProcessOriginalOccurrence(original);
    return ProcessOccurrenceId(occurrence);
  };
  auto makeSyntheticLoopOccurrence = [&](scf::ForOp loop,
                                         const ExpansionContext &context,
                                         ProcessLoopPhase phase) {
    ProcessOccurrenceId anchor =
        makeOriginalOccurrence(loop.getOperation(), context);
    auto loopImpl = std::make_shared<ProcessSyntheticLoopOccurrence::Impl>();
    loopImpl->anchor = anchor;
    loopImpl->phase = phase;
    auto occurrence = std::make_shared<ProcessOccurrenceId::Impl>();
    occurrence->kind = ProcessOccurrenceKind::SyntheticLoop;
    occurrence->syntheticLoop = ProcessSyntheticLoopOccurrence(loopImpl);
    return ProcessOccurrenceId(occurrence);
  };
  auto makeSyntheticConstantOccurrence =
      [&](scf::ForOp loop, const ExpansionContext &context, uint32_t ordinal) {
        ProcessOccurrenceId anchor =
            makeOriginalOccurrence(loop.getOperation(), context);
        auto constant =
            std::make_shared<ProcessSyntheticConstantOccurrence::Impl>();
        constant->anchor = anchor;
        constant->constant = ordinal;
        auto occurrence = std::make_shared<ProcessOccurrenceId::Impl>();
        occurrence->kind = ProcessOccurrenceKind::SyntheticConstant;
        occurrence->syntheticConstant =
            ProcessSyntheticConstantOccurrence(constant);
        return ProcessOccurrenceId(occurrence);
      };
  auto makeValueAtDefinition = [&](Value value,
                                   const ExpansionContext &context) {
    Operation *owner = value.getDefiningOp();
    auto coordinateImpl = std::make_shared<ProcessValueCoordinate::Impl>();
    std::string path;
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      owner = argument.getOwner()->getParentOp();
      coordinateImpl->kind = ProcessValueCoordinateKind::BlockArgument;
      coordinateImpl->ownerPath = blockPaths.lookup(argument.getOwner());
      coordinateImpl->index = argument.getArgNumber();
      path = coordinateImpl->ownerPath + "/a" +
             std::to_string(argument.getArgNumber());
    } else {
      coordinateImpl->kind = ProcessValueCoordinateKind::Result;
      coordinateImpl->ownerPath = operationPaths.lookup(owner);
      coordinateImpl->index = cast<OpResult>(value).getResultNumber();
      path = coordinateImpl->ownerPath + "/v" +
             std::to_string(coordinateImpl->index);
    }
    ProcessValueCoordinate coordinate(coordinateImpl);
    auto originalImpl = std::make_shared<ProcessOriginalPlannedValue::Impl>();
    originalImpl->value = value;
    originalImpl->occurrence = makeOriginalOccurrence(owner, context);
    originalImpl->coordinate = coordinate;
    originalImpl->path = std::move(path);
    auto planned = std::make_shared<ProcessPlannedValue::Impl>();
    planned->kind = ProcessPlannedValueKind::Original;
    planned->type = value.getType();
    planned->original = ProcessOriginalPlannedValue(originalImpl);
    return ProcessPlannedValue(planned);
  };
  DenseMap<Value, std::map<std::string, ProcessPlannedValue>> valueEnvironment;
  uint64_t valueLookupProbes = 0;
  uint64_t maxValueLookupProbes = 0;
  auto defineValue = [&](Value value, const ExpansionContext &context) {
    std::string key = contextKey(context, context.iterations.size());
    auto &bindings = valueEnvironment[value];
    if (auto found = bindings.find(key); found != bindings.end())
      return found->second;
    ProcessPlannedValue planned = makeValueAtDefinition(value, context);
    bindings.emplace(std::move(key), planned);
    return planned;
  };
  auto resolveValue = [&](Value value, const ExpansionContext &context) {
    auto found = valueEnvironment.find(value);
    uint64_t probes = 0;
    if (found != valueEnvironment.end()) {
      size_t iterationCount = context.iterations.size();
      while (true) {
        ++probes;
        auto binding = found->second.find(contextKey(context, iterationCount));
        if (binding != found->second.end()) {
          valueLookupProbes += probes;
          maxValueLookupProbes = std::max(maxValueLookupProbes, probes);
          return binding->second;
        }
        if (iterationCount == 0)
          break;
        --iterationCount;
      }
    }
    valueLookupProbes += probes;
    maxValueLookupProbes = std::max(maxValueLookupProbes, probes);
    return defineValue(value, context);
  };
  auto makeSyntheticValue = [&](scf::ForOp loop,
                                const ExpansionContext &context,
                                uint32_t ordinal) {
    auto coordinateImpl = std::make_shared<ProcessValueCoordinate::Impl>();
    coordinateImpl->kind = ProcessValueCoordinateKind::Result;
    coordinateImpl->ownerPath = operationPaths.lookup(loop);
    coordinateImpl->index = 0;
    auto syntheticImpl = std::make_shared<ProcessSyntheticPlannedValue::Impl>();
    syntheticImpl->occurrence =
        makeSyntheticConstantOccurrence(loop, context, ordinal);
    syntheticImpl->coordinate = ProcessValueCoordinate(coordinateImpl);
    auto planned = std::make_shared<ProcessPlannedValue::Impl>();
    planned->kind = ProcessPlannedValueKind::Synthetic;
    planned->type = loop.getInductionVar().getType();
    planned->synthetic = ProcessSyntheticPlannedValue(syntheticImpl);
    return ProcessPlannedValue(planned);
  };
  auto makeSyntheticLoopValue = [&](scf::ForOp loop,
                                    const ExpansionContext &context,
                                    ProcessLoopPhase phase, Type type,
                                    uint32_t index) {
    auto coordinateImpl = std::make_shared<ProcessValueCoordinate::Impl>();
    coordinateImpl->kind = ProcessValueCoordinateKind::Result;
    coordinateImpl->ownerPath = operationPaths.lookup(loop);
    coordinateImpl->index = index;
    auto syntheticImpl = std::make_shared<ProcessSyntheticPlannedValue::Impl>();
    syntheticImpl->occurrence =
        makeSyntheticLoopOccurrence(loop, context, phase);
    syntheticImpl->coordinate = ProcessValueCoordinate(coordinateImpl);
    auto planned = std::make_shared<ProcessPlannedValue::Impl>();
    planned->kind = ProcessPlannedValueKind::Synthetic;
    planned->type = type;
    planned->synthetic = ProcessSyntheticPlannedValue(syntheticImpl);
    return ProcessPlannedValue(planned);
  };
  auto makeScalar = [&](StringRef name, bool predicate) {
    auto impl = std::make_shared<ProcessScalarOperationPlan::Impl>();
    impl->name = name.str();
    impl->properties = "{}";
    if (predicate) {
      auto attribute = std::make_shared<ProcessScalarAttribute::Impl>();
      attribute->name = "predicate";
      attribute->value = "2 : i64";
      impl->attributes.push_back(ProcessScalarAttribute(attribute));
    }
    return ProcessScalarOperationPlan(impl);
  };
  auto addForwarding = [&](Value from, const ExpansionContext &fromContext,
                           Value to, const ExpansionContext &toContext) {
    Operation *origin = from.getDefiningOp();
    if (!origin)
      origin = cast<BlockArgument>(from).getOwner()->getParentOp();
    if (!reserveEdges(origin, 1))
      return;
    expanded.forwarding.push_back(
        {resolveValue(from, fromContext), defineValue(to, toContext)});
  };
  auto addOriginalAction = [&](Operation *operation,
                               const ExpansionContext &context) {
    ExpandedAction action;
    action.operation = operation;
    action.operationPath = operationPaths.lookup(operation);
    action.callSites = context.callSites;
    action.iterationVector = context.iterations;
    action.occurrence = makeOriginalOccurrence(operation, context);
    for (Value operand : operation->getOperands())
      action.operands.push_back(resolveValue(operand, context));
    for (Value result : operation->getResults())
      action.results.push_back(defineValue(result, context));
    expanded.actions.push_back(std::move(action));
  };

  SmallVector<ExpansionTask> pending;
  auto pushBlock = [&](Block &block, const ExpansionContext &context) {
    SmallVector<Operation *> operations;
    for (Operation &operation : block)
      operations.push_back(&operation);
    for (Operation *operation : llvm::reverse(operations)) {
      if (!reserveNodes(operation, 1) ||
          !reserveEdges(operation, operation->getNumOperands()))
        return;
      pending.push_back({operation, context});
    }
  };
  for (BlockArgument argument : process.getBody().front().getArguments())
    (void)defineValue(argument, {});
  pushBlock(process.getBody().front(), {});
  if (budgetFailed)
    return failure();
  while (!pending.empty()) {
    ExpansionTask task = std::move(pending.back());
    pending.pop_back();
    Operation *operation = task.operation;
    if (auto call = dyn_cast<func::CallOp>(operation)) {
      const ValidatedPureFunction *callee = callGraph->lookup(call.getCallee());
      assert(callee && "validated graph must contain every reachable callee");
      ExpansionContext calleeContext = task.context;
      calleeContext.callSites.push_back(makeCallSite(operation, task.context));
      if (calleeContext.callSites.size() > kMaxPureCallDepth) {
        call.emitOpError() << "pure func.call expansion exceeds ACIR "
                              "depth limit "
                           << kMaxPureCallDepth;
        return failure();
      }
      func::FuncOp calleeFunction = callee->function;
      Block &entry = calleeFunction.getBody().front();
      for (auto [operand, argument] :
           llvm::zip(call.getOperands(), entry.getArguments()))
        addForwarding(operand, task.context, argument, calleeContext);
      for (Operation &calleeOperation : entry) {
        if (auto returnOp = dyn_cast<func::ReturnOp>(calleeOperation))
          for (auto [returned, result] :
               llvm::zip(returnOp.getOperands(), call.getResults()))
            addForwarding(returned, calleeContext, result, task.context);
      }
      pushBlock(entry, calleeContext);
      if (budgetFailed)
        return failure();
      continue;
    }
    if (isa<func::ReturnOp, scf::YieldOp, scf::ConditionOp>(operation))
      continue;
    if (auto forOp = dyn_cast<scf::ForOp>(operation)) {
      FailureOr<ac::StaticForTripCount> staticTrip =
          ac::analyzeStaticFor(forOp);
      if (succeeded(staticTrip)) {
        auto yield = cast<scf::YieldOp>(forOp.getBody()->getTerminator());
        if (staticTrip->tripCount == 0)
          for (auto [init, result] :
               llvm::zip(forOp.getInitArgs(), forOp.getResults()))
            addForwarding(init, task.context, result, task.context);
        for (uint64_t iteration = 0; iteration < staticTrip->tripCount;
             ++iteration) {
          ExpansionContext bodyContext = task.context;
          bodyContext.iterations.push_back(iteration);
          ProcessPlannedValue induction = makeSyntheticValue(
              forOp, bodyContext, static_cast<uint32_t>(iteration));
          ExpandedAction constant;
          constant.kind = ProcessActionKind::Constant;
          constant.operation = operation;
          constant.operationPath = operationPaths.lookup(operation);
          constant.callSites = task.context.callSites;
          constant.iterationVector = bodyContext.iterations;
          constant.occurrence = induction.synthetic().occurrence();
          constant.results.push_back(induction);
          if (!reserveNodes(operation, 1))
            return failure();
          expanded.actions.push_back(std::move(constant));
          if (!reserveEdges(operation, 1))
            return failure();
          expanded.forwarding.push_back(
              {induction, defineValue(forOp.getInductionVar(), bodyContext)});
          if (iteration == 0)
            for (auto [init, argument] :
                 llvm::zip(forOp.getInitArgs(), forOp.getRegionIterArgs()))
              addForwarding(init, task.context, argument, bodyContext);
          if (iteration + 1 < staticTrip->tripCount) {
            ExpansionContext nextContext = task.context;
            nextContext.iterations.push_back(iteration + 1);
            for (auto [yielded, argument] :
                 llvm::zip(yield.getOperands(), forOp.getRegionIterArgs()))
              addForwarding(yielded, bodyContext, argument, nextContext);
          } else {
            for (auto [yielded, result] :
                 llvm::zip(yield.getOperands(), forOp.getResults()))
              addForwarding(yielded, bodyContext, result, task.context);
          }
        }
        for (uint64_t iteration = staticTrip->tripCount; iteration > 0;
             --iteration) {
          ExpansionContext bodyContext = task.context;
          bodyContext.iterations.push_back(iteration - 1);
          pushBlock(*forOp.getBody(), bodyContext);
          if (budgetFailed)
            return failure();
        }
        continue;
      }

      ProcessPlannedValue induction = makeSyntheticLoopValue(
          forOp, task.context, ProcessLoopPhase::Initialize,
          forOp.getInductionVar().getType(), 0);
      ProcessPlannedValue condition = makeSyntheticLoopValue(
          forOp, task.context, ProcessLoopPhase::Condition,
          IntegerType::get(process.getContext(), 1), 0);
      ProcessPlannedValue nextInduction = makeSyntheticLoopValue(
          forOp, task.context, ProcessLoopPhase::Increment,
          forOp.getInductionVar().getType(), 0);
      for (auto [kind, phase] : {std::pair{ProcessActionKind::ForInitialize,
                                           ProcessLoopPhase::Initialize},
                                 std::pair{ProcessActionKind::ForCondition,
                                           ProcessLoopPhase::Condition},
                                 std::pair{ProcessActionKind::ForIncrement,
                                           ProcessLoopPhase::Increment}}) {
        ExpandedAction action;
        action.kind = kind;
        action.operation = operation;
        action.operationPath = operationPaths.lookup(operation);
        action.callSites = task.context.callSites;
        action.iterationVector = task.context.iterations;
        action.occurrence =
            makeSyntheticLoopOccurrence(forOp, task.context, phase);
        if (kind == ProcessActionKind::ForInitialize) {
          action.operands.push_back(
              resolveValue(forOp.getLowerBound(), task.context));
          action.results.push_back(induction);
          for (Value init : forOp.getInitArgs()) {
            ProcessPlannedValue initial = resolveValue(init, task.context);
            action.operands.push_back(initial);
            action.results.push_back(initial);
          }
        } else if (kind == ProcessActionKind::ForCondition) {
          action.operands = {induction,
                             resolveValue(forOp.getUpperBound(), task.context)};
          action.results = {condition};
          action.scalarOperation = makeScalar("arith.cmpi", true);
        } else {
          action.operands = {induction,
                             resolveValue(forOp.getStep(), task.context)};
          action.results = {nextInduction};
          action.scalarOperation = makeScalar("arith.addi", false);
        }
        if (!reserveNodes(operation, 1))
          return failure();
        expanded.actions.push_back(std::move(action));
      }
      if (!reserveEdges(operation, 1))
        return failure();
      expanded.forwarding.push_back(
          {induction, defineValue(forOp.getInductionVar(), task.context)});
      if (!reserveEdges(operation, 1))
        return failure();
      expanded.forwarding.push_back(
          {nextInduction, resolveValue(forOp.getInductionVar(), task.context)});
      auto yield = cast<scf::YieldOp>(forOp.getBody()->getTerminator());
      for (auto [init, argument] :
           llvm::zip(forOp.getInitArgs(), forOp.getRegionIterArgs()))
        addForwarding(init, task.context, argument, task.context);
      for (auto [yielded, argument] :
           llvm::zip(yield.getOperands(), forOp.getRegionIterArgs()))
        addForwarding(yielded, task.context, argument, task.context);
      for (auto [yielded, result] :
           llvm::zip(yield.getOperands(), forOp.getResults()))
        addForwarding(yielded, task.context, result, task.context);
      pushBlock(*forOp.getBody(), task.context);
      if (budgetFailed)
        return failure();
      continue;
    }
    if (auto ifOp = dyn_cast<scf::IfOp>(operation)) {
      pushBlock(ifOp.getThenRegion().front(), task.context);
      if (!ifOp.getElseRegion().empty())
        pushBlock(ifOp.getElseRegion().front(), task.context);
      if (budgetFailed)
        return failure();
      continue;
    }
    if (auto whileOp = dyn_cast<scf::WhileOp>(operation)) {
      pushBlock(whileOp.getBefore().front(), task.context);
      pushBlock(whileOp.getAfter().front(), task.context);
      if (budgetFailed)
        return failure();
      continue;
    }
    addOriginalAction(operation, task.context);
  }

  expanded.expandedNodes = expansionNodes;
  expanded.expandedEdges = expansionEdges;
  expanded.valueLookupProbes = valueLookupProbes;
  expanded.maxValueLookupProbes = maxValueLookupProbes;
  return budgetFailed ? FailureOr<ExpandedProcess>(failure())
                      : FailureOr<ExpandedProcess>(std::move(expanded));
}

FailureOr<ExpandedProcess>
expandProcessForPlanning(ac::ProcessOp process,
                         const ac::RawModelStructureLimits &limits) {
  return PlanSetBuilder::expandProcess(process, limits);
}

} // namespace acir::detail
