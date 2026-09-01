#include "acir/CodeGen/QueueGraphPlan.h"

#include "acir/Bindings/Binding.h"
#include "acir/CodeGen/Manifest.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/ACIRTypes.h"

#include "mlir/IR/Operation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include <system_error>

namespace acir::codegen {
namespace {

llvm::Error planError(const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument),
      "ACLOWER-QUEUE-PLAN: " + message);
}

std::string printType(mlir::Type type) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  stream << type;
  return result;
}

std::string printAttribute(mlir::Attribute attribute) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  stream << attribute;
  return result;
}

std::string printRegion(mlir::Region &region) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  region.getParentOp()->print(stream);
  return result;
}

std::string scopePath(llvm::ArrayRef<std::string> scope) {
  std::string result;
  for (llvm::StringRef part : scope) {
    result.push_back('/');
    result.append(part);
  }
  return result.empty() ? "/" : result;
}

llvm::Expected<std::string>
queueName(mlir::Value value,
          const llvm::DenseMap<mlir::Value, std::string> &names) {
  auto found = names.find(value);
  if (found == names.end())
    return planError("Queue operand has no frozen logical identity");
  return found->second;
}

llvm::Expected<std::vector<std::string>>
queueNames(mlir::ValueRange values,
           const llvm::DenseMap<mlir::Value, std::string> &names) {
  std::vector<std::string> result;
  for (mlir::Value value : values) {
    auto name = queueName(value, names);
    if (!name)
      return name.takeError();
    result.push_back(std::move(*name));
  }
  return result;
}

llvm::Expected<std::vector<std::string>> outputNames(mlir::Operation *op,
                                                     size_t count) {
  std::vector<std::string> result;
  if (count == 1)
    if (auto name = op->getAttrOfType<mlir::StringAttr>("ac.name"))
      result.push_back(name.getValue().str());
  if (result.empty())
    if (auto names = op->getAttrOfType<mlir::ArrayAttr>("ac.output_names"))
      for (mlir::Attribute value : names) {
        auto name = mlir::dyn_cast<mlir::StringAttr>(value);
        if (!name)
          return planError("ac.output_names must contain only strings");
        result.push_back(name.getValue().str());
      }
  if (result.size() != count)
    return planError("Queue-producing op requires exact frozen output names");
  return result;
}

llvm::Error extractExpressions(mlir::Region &region, QueueBlockPlan &plan) {
  mlir::Block &block = region.front();
  llvm::DenseMap<mlir::Value, std::string> values;
  for (auto [index, argument] : llvm::enumerate(block.getArguments()))
    values[argument] = index == 0 ? "item" : "item" + std::to_string(index);
  auto operandNames = [&](mlir::ValueRange operands)
      -> llvm::Expected<std::vector<std::string>> {
    std::vector<std::string> result;
    for (mlir::Value operand : operands) {
      auto found = values.find(operand);
      if (found == values.end())
        return planError("Var expression operand has no local identity");
      result.push_back(found->second);
    }
    return result;
  };
  auto append = [&](mlir::Operation &operation, llvm::StringRef kind,
                    llvm::StringRef field = {}, llvm::StringRef predicate = {},
                    llvm::StringRef literal = {}) -> llvm::Error {
    if (operation.getNumResults() != 1)
      return planError("Var expression must produce exactly one result");
    auto resultType =
        mlir::dyn_cast<ac::VarType>(operation.getResult(0).getType());
    if (!resultType)
      return planError("Var expression result must be ac.var");
    auto operands = operandNames(operation.getOperands());
    if (!operands)
      return operands.takeError();
    std::string result = "v" + std::to_string(plan.expressions.size());
    values[operation.getResult(0)] = result;
    plan.expressions.push_back(
        {std::move(result), kind.str(), printType(resultType.getElementType()),
         std::move(*operands), field.str(), predicate.str(), literal.str()});
    return llvm::Error::success();
  };

  for (mlir::Operation &operation : block) {
    if (auto constant = mlir::dyn_cast<ac::VarConstantOp>(operation)) {
      if (auto error = append(operation, "constant", {}, {},
                              printAttribute(constant.getValueAttr())))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarAddOp>(operation)) {
      if (auto error = append(operation, "add"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarSubOp>(operation)) {
      if (auto error = append(operation, "sub"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarMulOp>(operation)) {
      if (auto error = append(operation, "mul"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarPopcountOp>(operation)) {
      if (auto error = append(operation, "popcount"))
        return error;
      continue;
    }
    if (auto compare = mlir::dyn_cast<ac::VarCmpOp>(operation)) {
      if (auto error = append(operation, "cmp", {}, compare.getPredicate()))
        return error;
      continue;
    }
    if (auto get = mlir::dyn_cast<ac::VarGetOp>(operation)) {
      if (auto error = append(operation, "get", get.getField()))
        return error;
      continue;
    }
    if (auto with = mlir::dyn_cast<ac::VarWithOp>(operation)) {
      if (auto error = append(operation, "with", with.getField()))
        return error;
      continue;
    }
    if (auto get = mlir::dyn_cast<ac::TableGetOp>(operation)) {
      if (auto error = append(operation, "table_get"))
        return error;
      plan.expressions.back().table = get.getTable().str();
      continue;
    }
    llvm::SmallVector<mlir::Value, 2> yielded;
    if (auto yield = mlir::dyn_cast<ac::TransformYieldOp>(operation))
      yielded.append(yield.getValues().begin(), yield.getValues().end());
    else if (auto yield = mlir::dyn_cast<ac::RouteYieldOp>(operation))
      yielded.push_back(yield.getSelector());
    else if (auto yield = mlir::dyn_cast<ac::SelectYieldOp>(operation))
      yielded.push_back(yield.getSelector());
    else if (auto yield = mlir::dyn_cast<ac::ReorderYieldOp>(operation))
      yielded.push_back(yield.getKey());
    else if (auto yield = mlir::dyn_cast<ac::DependencyYieldOp>(operation))
      yielded.push_back(yield.getValue());
    else if (auto yield = mlir::dyn_cast<ac::CreditYieldOp>(operation))
      yielded.push_back(yield.getCost());
    else if (auto yield = mlir::dyn_cast<ac::MemoryYieldOp>(operation))
      yielded.push_back(yield.getValue());
    else if (auto yield = mlir::dyn_cast<ac::TableYieldOp>(operation))
      yielded.push_back(yield.getValue());
    else if (auto yield = mlir::dyn_cast<ac::ExpectYieldOp>(operation))
      yielded.push_back(yield.getCondition());
    else if (auto yield = mlir::dyn_cast<ac::FeedbackYieldOp>(operation)) {
      yielded.push_back(yield.getValue());
      yielded.push_back(yield.getContinueValue());
    } else
      return planError("unsupported operation in Queue Var region: " +
                       operation.getName().getStringRef());
    auto names = operandNames(yielded);
    if (!names)
      return names.takeError();
    plan.yields = std::move(*names);
  }
  if (plan.yields.empty())
    return planError("Queue Var region has no structured yield");
  return llvm::Error::success();
}

class Extractor {
public:
  explicit Extractor(mlir::ModuleOp module) : module(module) {}

  llvm::Expected<QueueGraphPlan> run() {
    auto epoch = module->getAttrOfType<mlir::StringAttr>("ac.contract_epoch");
    if (!epoch || epoch.getValue() != "0.4")
      return planError("module requires ac.contract_epoch exactly '0.4'");
    auto system = module->getAttrOfType<mlir::StringAttr>("ac.system");
    if (!system || system.getValue().empty())
      return planError("module requires non-empty ac.system");
    plan.system = system.getValue().str();
    if (auto specialization =
            module->getAttrOfType<mlir::StringAttr>("ac.specialization")) {
      if (!isValidFingerprint(specialization.getValue()))
        return planError("module ac.specialization fingerprint is invalid");
      plan.specializationFingerprint = specialization.getValue().str();
    }
    if (auto error = extractBlock(*module.getBody(), {}))
      return std::move(error);
    if (auto error = validateGraph())
      return std::move(error);
    return std::move(plan);
  }

private:
  llvm::Error validateGraph() { return verifyQueueGraphPlan(plan); }

  llvm::Error addQueue(mlir::Value value, llvm::StringRef name, uint64_t depth,
                       uint64_t latency, uint64_t rate,
                       llvm::ArrayRef<std::string> scope) {
    if (name.empty() || !queueIdentities.insert(name).second)
      return planError("Queue logical identities must be non-empty and unique");
    auto queue = mlir::dyn_cast<ac::QueueType>(value.getType());
    if (!queue || depth == 0 || latency == 0 || rate == 0 || rate > depth)
      return planError(
          "Queue plan requires typed positive depth/latency and rate <= depth");
    names[value] = name.str();
    plan.queues.push_back({name.str(), printType(queue.getElementType()),
                           scopePath(scope), depth, latency, rate});
    return llvm::Error::success();
  }

  llvm::Error addOutputs(mlir::Operation *op, mlir::ValueRange outputs,
                         llvm::ArrayRef<int64_t> depths,
                         llvm::ArrayRef<int64_t> latencies,
                         llvm::ArrayRef<std::string> scope,
                         std::vector<std::string> &result) {
    auto frozen = outputNames(op, outputs.size());
    if (!frozen)
      return frozen.takeError();
    if (depths.size() != outputs.size() || latencies.size() != outputs.size())
      return planError("Queue output metadata count mismatch");
    llvm::SmallVector<int64_t> defaultRates(outputs.size(), 1);
    llvm::ArrayRef<int64_t> rates = defaultRates;
    if (auto attribute =
            op->getAttrOfType<mlir::DenseI64ArrayAttr>("ac.output_rates"))
      rates = attribute.asArrayRef();
    if (rates.size() != outputs.size())
      return planError("Queue output rate count must match result count");
    for (size_t index = 0; index < outputs.size(); ++index) {
      if (depths[index] <= 0 || latencies[index] <= 0 || rates[index] <= 0 ||
          rates[index] > depths[index])
        return planError("Queue depth/latency must be positive and rate must "
                         "not exceed depth");
      auto error = addQueue(outputs[index], (*frozen)[index], depths[index],
                            latencies[index], rates[index], scope);
      if (error)
        return error;
    }
    result = std::move(*frozen);
    return llvm::Error::success();
  }

  llvm::Error extractBlock(mlir::Block &block, std::vector<std::string> scope) {
    for (mlir::Operation &operation : block) {
      if (auto typeScope = mlir::dyn_cast<ac::TypeScopeOp>(operation)) {
        for (mlir::Operation &declaration : typeScope.getBody().front()) {
          auto structure = mlir::dyn_cast<ac::StructOp>(declaration);
          if (!structure)
            continue;
          if (!payloadIdentities.insert(structure.getSymName()).second)
            return planError("payload identities must be unique");
          QueuePayloadPlan payload{structure.getSymName().str(), {}};
          for (mlir::Attribute rawField : structure.getFields()) {
            auto field = mlir::dyn_cast<mlir::DictionaryAttr>(rawField);
            auto name = field ? field.getAs<mlir::StringAttr>("name")
                              : mlir::StringAttr();
            auto type =
                field ? field.getAs<mlir::TypeAttr>("type") : mlir::TypeAttr();
            if (!name || !type)
              return planError("struct field requires name and type");
            payload.fields.push_back(
                {name.getValue().str(), printType(type.getValue())});
          }
          plan.payloads.push_back(std::move(payload));
        }
        continue;
      }
      if (auto instance = mlir::dyn_cast<ac::MemoryInstanceOp>(operation)) {
        plan.memoryInstances.push_back(
            {instance.getSymName().str(), printType(instance.getDataType()),
             uint64_t(instance.getEntries()), uint64_t(instance.getInit()),
             uint64_t(instance.getLatency()), instance.getStableId().str(),
             instance.getOwner().str()});
        continue;
      }
      if (auto table = mlir::dyn_cast<ac::TableOp>(operation)) {
        plan.tables.push_back(
            {table.getSymName().str(), printType(table.getEntryType()),
             uint64_t(table.getEntries()), uint64_t(table.getInit()),
             table.getStableId().str(), table.getOwner().str()});
        continue;
      }
      if (auto source = mlir::dyn_cast<ac::SourceOp>(operation)) {
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                source, source->getResults(), {int64_t(source.getDepth())},
                {int64_t(source.getLatency())}, scope, outputs))
          return error;
        plan.blocks.push_back({"source",
                               outputs.front(),
                               scopePath(scope),
                               {},
                               outputs,
                               {uint64_t(source.getDepth())},
                               {uint64_t(source.getLatency())}});
        continue;
      }
      if (auto transform = mlir::dyn_cast<ac::TransformOp>(operation)) {
        auto inputs = queueNames(transform.getInputs(), names);
        if (!inputs)
          return inputs.takeError();
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(transform, transform.getOutputs(),
                           transform.getOutputDepthsAttr().asArrayRef(),
                           transform.getOutputLatenciesAttr().asArrayRef(),
                           scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"transform", outputs.front(), scopePath(scope),
                                 std::move(*inputs), outputs};
        for (int64_t value : transform.getOutputDepths())
          blockPlan.depths.push_back(value);
        for (int64_t value : transform.getOutputLatencies())
          blockPlan.latencies.push_back(value);
        blockPlan.region = printRegion(transform.getBody());
        if (auto error = extractExpressions(transform.getBody(), blockPlan))
          return error;
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto broadcast = mlir::dyn_cast<ac::BroadcastOp>(operation)) {
        auto input = queueName(broadcast.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(broadcast, broadcast.getOutputs(),
                           broadcast.getOutputDepthsAttr().asArrayRef(),
                           broadcast.getOutputLatenciesAttr().asArrayRef(),
                           scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"broadcast",
                                 "broadcast_" + *input,
                                 scopePath(scope),
                                 {*input},
                                 outputs};
        for (int64_t value : broadcast.getOutputDepths())
          blockPlan.depths.push_back(value);
        for (int64_t value : broadcast.getOutputLatencies())
          blockPlan.latencies.push_back(value);
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto fork = mlir::dyn_cast<ac::ForkOp>(operation)) {
        auto input = queueName(fork.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(fork, fork.getOutputs(),
                                    fork.getOutputDepthsAttr().asArrayRef(),
                                    fork.getOutputLatenciesAttr().asArrayRef(),
                                    scope, outputs))
          return error;
        QueueBlockPlan blockPlan{
            "fork", "fork_" + *input, scopePath(scope), {*input}, outputs};
        for (int64_t value : fork.getOutputDepths())
          blockPlan.depths.push_back(value);
        for (int64_t value : fork.getOutputLatencies())
          blockPlan.latencies.push_back(value);
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto route = mlir::dyn_cast<ac::RouteOp>(operation)) {
        auto input = queueName(route.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(route, route.getOutputs(),
                                    route.getOutputDepthsAttr().asArrayRef(),
                                    route.getOutputLatenciesAttr().asArrayRef(),
                                    scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"route",
                                 "route_" + outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs};
        for (int64_t value : route.getOutputDepths())
          blockPlan.depths.push_back(value);
        for (int64_t value : route.getOutputLatencies())
          blockPlan.latencies.push_back(value);
        blockPlan.region = printRegion(route.getSelector());
        if (auto error = extractExpressions(route.getSelector(), blockPlan))
          return error;
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto select = mlir::dyn_cast<ac::SelectOp>(operation)) {
        auto inputs = queueNames(select.getInputs(), names);
        if (!inputs)
          return inputs.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                select, select->getResults(), {int64_t(select.getDepth())},
                {int64_t(select.getLatency())}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"select",
                                 outputs.front(),
                                 scopePath(scope),
                                 std::move(*inputs),
                                 outputs,
                                 {uint64_t(select.getDepth())},
                                 {uint64_t(select.getLatency())}};
        blockPlan.region = printRegion(select.getKey());
        if (auto error = extractExpressions(select.getKey(), blockPlan))
          return error;
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto merge = mlir::dyn_cast<ac::MergeOp>(operation)) {
        auto inputs = queueNames(merge.getInputs(), names);
        if (!inputs)
          return inputs.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                merge, merge->getResults(), {int64_t(merge.getDepth())},
                {int64_t(merge.getLatency())}, scope, outputs))
          return error;
        plan.blocks.push_back({"merge",
                               outputs.front(),
                               scopePath(scope),
                               std::move(*inputs),
                               outputs,
                               {uint64_t(merge.getDepth())},
                               {uint64_t(merge.getLatency())},
                               merge.getPolicy().str()});
        continue;
      }
      if (auto barrier = mlir::dyn_cast<ac::BarrierOp>(operation)) {
        auto inputs = queueNames(barrier.getInputs(), names);
        if (!inputs)
          return inputs.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                barrier, barrier.getOutputs(),
                barrier.getOutputDepthsAttr().asArrayRef(),
                barrier.getOutputLatenciesAttr().asArrayRef(), scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"barrier", outputs.front(), scopePath(scope),
                                 std::move(*inputs), outputs};
        for (int64_t value : barrier.getOutputDepths())
          blockPlan.depths.push_back(value);
        for (int64_t value : barrier.getOutputLatencies())
          blockPlan.latencies.push_back(value);
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto reorder = mlir::dyn_cast<ac::ReorderOp>(operation)) {
        auto input = queueName(reorder.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                reorder, reorder->getResults(), {int64_t(reorder.getDepth())},
                {int64_t(reorder.getLatency())}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"reorder",
                                 outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs,
                                 {uint64_t(reorder.getDepth())},
                                 {uint64_t(reorder.getLatency())}};
        blockPlan.capacity = reorder.getCapacity();
        blockPlan.start = reorder.getStart();
        blockPlan.region = printRegion(reorder.getKey());
        if (auto error = extractExpressions(reorder.getKey(), blockPlan))
          return error;
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto dependency = mlir::dyn_cast<ac::DependencyOp>(operation)) {
        auto input = queueName(dependency.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(dependency, dependency->getResults(),
                           {int64_t(dependency.getDepth())},
                           {int64_t(dependency.getLatency())}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"dependency",
                                 outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs,
                                 {uint64_t(dependency.getDepth())},
                                 {uint64_t(dependency.getLatency())}};
        blockPlan.capacity = dependency.getCapacity();
        blockPlan.noDependency = dependency.getNoDependency();
        blockPlan.resources = dependency.getResources();
        blockPlan.region = printRegion(dependency.getKey());
        std::vector<std::string> policyYields;
        for (mlir::Region *policy :
             {&dependency.getKey(), &dependency.getWaitsFor(),
              &dependency.getResource(), &dependency.getCost()}) {
          if (auto error = extractExpressions(*policy, blockPlan))
            return error;
          if (blockPlan.yields.size() != 1)
            return planError("dependency policy must yield one value");
          policyYields.push_back(blockPlan.yields.front());
        }
        blockPlan.yields = std::move(policyYields);
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto credit = mlir::dyn_cast<ac::CreditOp>(operation)) {
        auto input = queueName(credit.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                credit, credit->getResults(), {int64_t(credit.getDepth())},
                {int64_t(credit.getLatency())}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"credit",
                                 outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs,
                                 {uint64_t(credit.getDepth())},
                                 {uint64_t(credit.getLatency())}};
        blockPlan.credits = credit.getCredits();
        blockPlan.region = printRegion(credit.getCost());
        if (auto error = extractExpressions(credit.getCost(), blockPlan))
          return error;
        if (blockPlan.yields.size() != 1)
          return planError("credit cost must yield one value");
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto memory = mlir::dyn_cast<ac::MemoryRequestOp>(operation)) {
        auto input = queueName(memory.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(memory, memory->getResults(),
                           {int64_t(memory.getDepth())}, {1}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"memory_request",
                                 outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs,
                                 {uint64_t(memory.getDepth())},
                                 {1}};
        blockPlan.resultField = memory.getResultField().str();
        blockPlan.memoryInstance = memory.getInstance().str();
        blockPlan.endpointOrdinal = memory.getOrdinal();
        blockPlan.region = printRegion(memory.getAddress());
        std::vector<std::string> policyYields;
        for (mlir::Region *policy :
             {&memory.getAddress(), &memory.getWrite(), &memory.getData()}) {
          if (auto error = extractExpressions(*policy, blockPlan))
            return error;
          if (blockPlan.yields.size() != 1)
            return planError("memory policy must yield one value");
          policyYields.push_back(blockPlan.yields.front());
        }
        blockPlan.yields = std::move(policyYields);
        plan.memoryRequests.push_back(
            {blockPlan.memoryInstance, blockPlan.name, blockPlan.scope,
             blockPlan.inputs.front(), blockPlan.outputs.front(),
             blockPlan.endpointOrdinal, blockPlan.depths.front(),
             blockPlan.resultField});
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto read = mlir::dyn_cast<ac::TableReadOp>(operation)) {
        std::vector<std::string> inputs;
        if (read.getInput()) {
          auto input = queueName(read.getInput(), names);
          if (!input)
            return input.takeError();
          inputs.push_back(std::move(*input));
        }
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(read, read->getResults(), {int64_t(read.getDepth())},
                           {int64_t(read.getLatency())}, scope, outputs))
          return error;
        auto name = read->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("table.read requires frozen ac.name");
        QueueBlockPlan blockPlan{"table_read",
                                 name.getValue().str(),
                                 scopePath(scope),
                                 inputs,
                                 outputs,
                                 {uint64_t(read.getDepth())},
                                 {uint64_t(read.getLatency())}};
        blockPlan.table = read.getTable().str();
        std::vector<std::string> policyYields;
        for (mlir::Region *policy : {&read.getAddress(), &read.getWhen()}) {
          if (auto error = extractExpressions(*policy, blockPlan))
            return error;
          if (blockPlan.yields.size() != 1)
            return planError("table read policy must yield one value");
          policyYields.push_back(blockPlan.yields.front());
        }
        blockPlan.yields = std::move(policyYields);
        plan.tableReads.push_back(
            {blockPlan.table, blockPlan.name, blockPlan.scope,
             inputs.empty() ? std::string() : inputs.front(), outputs.front(),
             uint64_t(read.getDepth()), uint64_t(read.getLatency())});
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto write = mlir::dyn_cast<ac::TableWriteOp>(operation)) {
        auto input = queueName(write.getInput(), names);
        if (!input)
          return input.takeError();
        auto name = write->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("table.write requires frozen ac.name");
        QueueBlockPlan blockPlan{"table_write",
                                 name.getValue().str(),
                                 scopePath(scope),
                                 {*input},
                                 {}};
        blockPlan.table = write.getTable().str();
        std::vector<std::string> policyYields;
        for (mlir::Region *policy :
             {&write.getAddress(), &write.getEnable(), &write.getValue()}) {
          if (auto error = extractExpressions(*policy, blockPlan))
            return error;
          if (blockPlan.yields.size() != 1)
            return planError("table write policy must yield one value");
          policyYields.push_back(blockPlan.yields.front());
        }
        blockPlan.yields = std::move(policyYields);
        plan.tableWrites.push_back(
            {blockPlan.table, blockPlan.name, blockPlan.scope, *input});
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto feedback = mlir::dyn_cast<ac::FeedbackOp>(operation)) {
        auto input = queueName(feedback.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(feedback, feedback->getResults(),
                           {int64_t(feedback.getDepth())},
                           {int64_t(feedback.getLatency())}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"feedback",
                                 outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs,
                                 {uint64_t(feedback.getDepth())},
                                 {uint64_t(feedback.getLatency())},
                                 "",
                                 uint64_t(feedback.getMaxIterations())};
        blockPlan.region = printRegion(feedback.getBody());
        if (auto error = extractExpressions(feedback.getBody(), blockPlan))
          return error;
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto nested = mlir::dyn_cast<ac::ScopeOp>(operation)) {
        std::vector<std::string> nestedScope = scope;
        nestedScope.push_back(nested.getSymName().str());
        plan.scopes.push_back(scopePath(nestedScope));
        mlir::Block &body = nested.getBody().front();
        if (body.getNumArguments() != nested.getInputs().size())
          return planError("scope input arity mismatch");
        for (size_t index = 0; index < nested.getInputs().size(); ++index) {
          auto name = queueName(nested.getInputs()[index], names);
          if (!name)
            return name.takeError();
          names[body.getArgument(index)] = std::move(*name);
        }
        if (auto error = extractBlock(body, nestedScope))
          return error;
        auto yield = mlir::dyn_cast<ac::ScopeYieldOp>(body.getTerminator());
        bool invalidYield = !yield;
        if (yield)
          invalidYield = yield.getQueues().size() != nested.getOutputs().size();
        if (invalidYield)
          return planError("scope output arity mismatch");
        for (size_t index = 0; index < nested.getOutputs().size(); ++index) {
          auto name = queueName(yield.getQueues()[index], names);
          if (!name)
            return name.takeError();
          names[nested.getOutputs()[index]] = std::move(*name);
        }
        continue;
      }
      auto sink = mlir::dyn_cast<ac::SinkOp>(operation);
      if (sink) {
        auto input = queueName(sink.getInput(), names);
        if (!input)
          return input.takeError();
        auto name = sink->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("sink requires frozen ac.name");
        plan.blocks.push_back(
            {"sink", name.getValue().str(), scopePath(scope), {*input}, {}});
        continue;
      }
      auto observe = mlir::dyn_cast<ac::ObserveOp>(operation);
      if (observe) {
        auto input = queueName(observe.getInput(), names);
        if (!input)
          return input.takeError();
        plan.blocks.push_back({"observe",
                               observe.getName().str(),
                               scopePath(scope),
                               {*input},
                               {}});
        continue;
      }
      auto expect = mlir::dyn_cast<ac::ExpectOp>(operation);
      if (expect) {
        auto input = queueName(expect.getInput(), names);
        if (!input)
          return input.takeError();
        auto name = expect->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("expect requires frozen ac.name");
        QueueBlockPlan blockPlan{
            "expect", name.getValue().str(), scopePath(scope), {*input}, {}};
        blockPlan.message = expect.getMessage().str();
        blockPlan.region = printRegion(expect.getPredicate());
        if (auto error = extractExpressions(expect.getPredicate(), blockPlan))
          return error;
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (mlir::isa<ac::ScopeYieldOp>(operation) ||
          operation.hasTrait<mlir::OpTrait::IsTerminator>() ||
          mlir::isa<ac::TypeScopeOp>(operation))
        continue;
      if (operation.getName().getDialectNamespace() == "ac")
        return planError("unsupported ACIR op in QueueGraph plan: " +
                         operation.getName().getStringRef());
    }
    return llvm::Error::success();
  }

  mlir::ModuleOp module;
  QueueGraphPlan plan;
  llvm::DenseMap<mlir::Value, std::string> names;
  llvm::StringSet<> queueIdentities;
  llvm::StringSet<> payloadIdentities;
};

} // namespace

llvm::Expected<QueueGraphPlan> buildQueueGraphPlan(mlir::ModuleOp module) {
  return Extractor(module).run();
}

llvm::Error verifyQueueGraphPlan(const QueueGraphPlan &plan) {
  if (plan.system.empty() || plan.queues.empty() || plan.blocks.empty())
    return planError("QueueGraph plan is incomplete");
  if (!plan.specializationFingerprint.empty() &&
      !isValidFingerprint(plan.specializationFingerprint))
    return planError("QueueGraph specialization fingerprint is invalid");

  llvm::StringSet<> queueNames;
  llvm::StringMap<const MemoryInstancePlan *> memoryInstances;
  llvm::StringMap<const TablePlan *> tables;
  for (const MemoryInstancePlan &instance : plan.memoryInstances) {
    if (instance.name.empty() ||
        !memoryInstances.try_emplace(instance.name, &instance).second)
      return planError(
          "memory instance identities must be non-empty and unique");
    if (instance.dataType.empty() || instance.entries == 0 ||
        instance.init != 0 || instance.latency == 0 ||
        instance.stableId.empty() || instance.ownerPath.empty())
      return planError("memory instance metadata is incomplete");
  }
  llvm::StringMap<llvm::DenseSet<uint64_t>> endpointOrdinals;
  for (const MemoryRequestPlan &request : plan.memoryRequests) {
    if (!memoryInstances.contains(request.instance))
      return planError("memory request references unknown instance '" +
                       request.instance + "'");
    if (!endpointOrdinals[request.instance].insert(request.ordinal).second)
      return planError("memory request endpoint ordinals must be unique");
  }
  for (const auto &entry : memoryInstances)
    if (!endpointOrdinals.contains(entry.getKey()))
      return planError("memory instance '" + entry.getKey() +
                       "' has no request endpoints");
  for (const auto &entry : endpointOrdinals)
    for (uint64_t ordinal = 0; ordinal < entry.getValue().size(); ++ordinal)
      if (!entry.getValue().contains(ordinal))
        return planError("memory request endpoint ordinals must be contiguous "
                         "from zero");
  for (const TablePlan &table : plan.tables) {
    if (table.name.empty() || !tables.try_emplace(table.name, &table).second)
      return planError("table identities must be non-empty and unique");
    if (table.entryType.empty() || table.entries == 0 || table.init != 0 ||
        table.stableId.empty() || table.ownerPath.empty())
      return planError("table metadata is incomplete");
  }
  llvm::StringMap<unsigned> tableReaders;
  llvm::StringMap<unsigned> tableWriters;
  for (const TableReadPlan &read : plan.tableReads) {
    if (!tables.contains(read.table) || read.name.empty() ||
        read.output.empty() || read.depth == 0 || read.latency == 0)
      return planError("table read endpoint metadata is incomplete");
    ++tableReaders[read.table];
  }
  for (const TableWritePlan &write : plan.tableWrites) {
    if (!tables.contains(write.table) || write.name.empty() ||
        write.input.empty())
      return planError("table write endpoint metadata is incomplete");
    if (++tableWriters[write.table] > 1)
      return planError("table permits at most one write endpoint");
  }
  for (const auto &entry : tables)
    if (tableReaders[entry.getKey()] + tableWriters[entry.getKey()] == 0)
      return planError("table '" + entry.getKey() + "' has no endpoints");
  llvm::StringMap<unsigned> producers;
  llvm::StringMap<unsigned> consumers;
  llvm::StringMap<unsigned> indegree;
  llvm::StringMap<std::vector<std::string>> successors;
  for (const QueuePlan &queue : plan.queues) {
    if (queue.name.empty() || !queueNames.insert(queue.name).second)
      return planError("Queue logical identities must be non-empty and unique");
    if (queue.payloadType.empty() || queue.depth == 0 || queue.latency == 0 ||
        queue.rate == 0 || queue.rate > queue.depth)
      return planError(
          "Queue plan requires typed positive depth/latency and rate <= depth");
    indegree[queue.name] = 0;
  }

  for (const QueueBlockPlan &block : plan.blocks) {
    if (block.kind == "memory_request" &&
        !memoryInstances.contains(block.memoryInstance))
      return planError("memory request block references unknown instance");
    if ((block.kind == "table_read" || block.kind == "table_write") &&
        !tables.contains(block.table))
      return planError("table endpoint block references unknown table");
    for (const QueueExpressionPlan &expression : block.expressions)
      if (expression.kind == "table_get" && !tables.contains(expression.table))
        return planError("table.get expression references unknown table");
    for (const std::string &input : block.inputs)
      if (!queueNames.contains(input))
        return planError("block input references unknown Queue '" + input +
                         "'");
    for (const std::string &output : block.outputs) {
      if (!queueNames.contains(output))
        return planError("block output references unknown Queue '" + output +
                         "'");
      ++producers[output];
    }
    if (block.kind != "observe" && block.kind != "expect")
      for (const std::string &input : block.inputs)
        ++consumers[input];
    for (const std::string &input : block.inputs)
      for (const std::string &output : block.outputs) {
        successors[input].push_back(output);
        ++indegree[output];
      }
  }

  for (const QueuePlan &queue : plan.queues) {
    if (producers[queue.name] != 1)
      return planError("Queue '" + queue.name +
                       "' must have exactly one producer");
    if (consumers[queue.name] == 0)
      return planError("Queue '" + queue.name +
                       "' has no consuming block; connect ac.sink");
    if (consumers[queue.name] > 1)
      return planError("Queue '" + queue.name +
                       "' has multiple consuming blocks; insert ac.broadcast");
  }

  std::vector<std::string> ready;
  for (const QueuePlan &queue : plan.queues)
    if (indegree[queue.name] == 0)
      ready.push_back(queue.name);
  size_t visited = 0;
  for (size_t cursor = 0; cursor < ready.size(); ++cursor) {
    ++visited;
    auto found = successors.find(ready[cursor]);
    if (found == successors.end())
      continue;
    for (const std::string &successor : found->getValue())
      if (--indegree[successor] == 0)
        ready.push_back(successor);
  }
  if (visited != plan.queues.size())
    return planError("QueueGraph contains a cycle; represent stateful loops "
                     "with ac.feedback");
  return llvm::Error::success();
}

llvm::Expected<std::string> QueueGraphPlan::canonicalJson() const {
  llvm::json::Array payloadValues;
  for (const QueuePayloadPlan &payload : payloads) {
    llvm::json::Array fields;
    for (const QueuePayloadFieldPlan &field : payload.fields)
      fields.push_back(
          llvm::json::Object{{"name", field.name}, {"type", field.type}});
    payloadValues.push_back(llvm::json::Object{{"fields", std::move(fields)},
                                               {"name", payload.name}});
  }
  llvm::json::Array scopeValues;
  for (const std::string &scope : scopes)
    scopeValues.push_back(scope);
  llvm::json::Array queueValues;
  for (const QueuePlan &queue : queues)
    queueValues.push_back(
        llvm::json::Object{{"depth", queue.depth},
                           {"latency", queue.latency},
                           {"name", queue.name},
                           {"payload_type", queue.payloadType},
                           {"rate", queue.rate},
                           {"scope", queue.scope}});
  llvm::json::Array blockValues;
  for (const QueueBlockPlan &block : blocks) {
    llvm::json::Array inputs;
    for (const std::string &input : block.inputs)
      inputs.push_back(input);
    llvm::json::Array outputs;
    for (const std::string &output : block.outputs)
      outputs.push_back(output);
    llvm::json::Array depths;
    for (uint64_t depth : block.depths)
      depths.push_back(depth);
    llvm::json::Array latencies;
    for (uint64_t latency : block.latencies)
      latencies.push_back(latency);
    llvm::json::Array expressions;
    for (const QueueExpressionPlan &expression : block.expressions) {
      llvm::json::Array operands;
      for (const std::string &operand : expression.operands)
        operands.push_back(operand);
      expressions.push_back(
          llvm::json::Object{{"field", expression.field},
                             {"kind", expression.kind},
                             {"literal", expression.literal},
                             {"operands", std::move(operands)},
                             {"predicate", expression.predicate},
                             {"result", expression.result},
                             {"table", expression.table},
                             {"type", expression.type}});
    }
    llvm::json::Array yields;
    for (const std::string &yield : block.yields)
      yields.push_back(yield);
    blockValues.push_back(
        llvm::json::Object{{"capacity", block.capacity},
                           {"credits", block.credits},
                           {"depths", std::move(depths)},
                           {"entries", block.entries},
                           {"expressions", std::move(expressions)},
                           {"inputs", std::move(inputs)},
                           {"kind", block.kind},
                           {"latencies", std::move(latencies)},
                           {"max_iterations", block.maxIterations},
                           {"message", block.message},
                           {"memory_instance", block.memoryInstance},
                           {"table", block.table},
                           {"name", block.name},
                           {"no_dependency", block.noDependency},
                           {"endpoint_ordinal", block.endpointOrdinal},
                           {"outputs", std::move(outputs)},
                           {"policy", block.policy},
                           {"region", block.region},
                           {"result_field", block.resultField},
                           {"resources", block.resources},
                           {"scope", block.scope},
                           {"start", block.start},
                           {"init", block.init},
                           {"yields", std::move(yields)}});
  }
  llvm::json::Array memoryInstanceValues;
  for (const MemoryInstancePlan &instance : memoryInstances)
    memoryInstanceValues.push_back(
        llvm::json::Object{{"data_type", instance.dataType},
                           {"entries", instance.entries},
                           {"init", instance.init},
                           {"latency", instance.latency},
                           {"name", instance.name},
                           {"owner_path", instance.ownerPath},
                           {"stable_id", instance.stableId}});
  llvm::json::Array memoryRequestValues;
  for (const MemoryRequestPlan &request : memoryRequests)
    memoryRequestValues.push_back(
        llvm::json::Object{{"depth", request.depth},
                           {"input", request.input},
                           {"instance", request.instance},
                           {"name", request.name},
                           {"ordinal", request.ordinal},
                           {"output", request.output},
                           {"result_field", request.resultField},
                           {"scope", request.scope}});
  llvm::json::Array tableValues;
  for (const TablePlan &table : tables)
    tableValues.push_back(llvm::json::Object{{"entries", table.entries},
                                             {"entry_type", table.entryType},
                                             {"init", table.init},
                                             {"name", table.name},
                                             {"owner_path", table.ownerPath},
                                             {"stable_id", table.stableId}});
  llvm::json::Array tableReadValues;
  for (const TableReadPlan &read : tableReads)
    tableReadValues.push_back(llvm::json::Object{{"depth", read.depth},
                                                 {"input", read.input},
                                                 {"latency", read.latency},
                                                 {"name", read.name},
                                                 {"output", read.output},
                                                 {"scope", read.scope},
                                                 {"table", read.table}});
  llvm::json::Array tableWriteValues;
  for (const TableWritePlan &write : tableWrites)
    tableWriteValues.push_back(llvm::json::Object{{"input", write.input},
                                                  {"name", write.name},
                                                  {"scope", write.scope},
                                                  {"table", write.table}});
  llvm::json::Object root{
      {"blocks", std::move(blockValues)},
      {"contract_epoch", "0.4"},
      {"memory_instances", std::move(memoryInstanceValues)},
      {"memory_requests", std::move(memoryRequestValues)},
      {"payloads", std::move(payloadValues)},
      {"queues", std::move(queueValues)},
      {"schema", "agentic-circuit-queue-graph-plan"},
      {"scopes", std::move(scopeValues)},
      {"specialization", specializationFingerprint.empty()
                             ? llvm::json::Value(nullptr)
                             : llvm::json::Value(specializationFingerprint)},
      {"table_reads", std::move(tableReadValues)},
      {"table_writes", std::move(tableWriteValues)},
      {"tables", std::move(tableValues)},
      {"system", system},
      {"version", "0.4"}};
  return bindings::canonicalizeJson(llvm::json::Value(std::move(root)));
}

} // namespace acir::codegen
