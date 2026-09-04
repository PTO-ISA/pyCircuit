#include "acir/CodeGen/QueueGraphPlan.h"

#include "acir/Analysis/ModelAnalysis.h"
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
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>
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

std::optional<unsigned> integerWidth(llvm::StringRef type) {
  if (!type.consume_front("i"))
    return std::nullopt;
  unsigned width = 0;
  if (type.empty() || type.getAsInteger(10, width) || width == 0)
    return std::nullopt;
  return width;
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

using SharedExpression = std::pair<mlir::Value, QueueExpressionPlan>;

llvm::Error
extractExpressions(mlir::Region &region, QueueBlockPlan &plan,
                   llvm::ArrayRef<SharedExpression> sharedExpressions = {}) {
  mlir::Block &block = region.front();
  llvm::DenseMap<mlir::Value, std::string> values;
  for (const auto &[value, expression] : sharedExpressions) {
    values[value] = expression.result;
    if (llvm::none_of(plan.expressions, [&](const QueueExpressionPlan &item) {
          return item.result == expression.result;
        }))
      plan.expressions.push_back(expression);
  }
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
    if (auto get = mlir::dyn_cast<ac::SlotGetOp>(operation)) {
      const std::string base = "v" + std::to_string(plan.expressions.size());
      const std::array<std::pair<mlir::Value, llvm::StringRef>, 2> results = {{
          {get.getValid(), "slot_get_valid"},
          {get.getValue(), "slot_get_value"},
      }};
      for (auto [resultValue, kind] : results) {
        auto resultType = mlir::cast<ac::VarType>(resultValue.getType());
        std::string result =
            base + (kind == "slot_get_valid" ? "_valid" : "_value");
        values[resultValue] = result;
        QueueExpressionPlan expression{
            result, kind.str(), printType(resultType.getElementType()), {}};
        expression.slot = get.getSlot().str();
        plan.expressions.push_back(std::move(expression));
      }
      continue;
    }
    if (auto match = mlir::dyn_cast<ac::TableMatchOp>(operation)) {
      QueueBlockPlan nested;
      if (auto error = extractExpressions(match.getPredicate(), nested))
        return error;
      auto resultType = mlir::cast<ac::VarType>(match.getMask().getType());
      std::string result = "v" + std::to_string(plan.expressions.size());
      values[match.getMask()] = result;
      QueueExpressionPlan expression{
          result, "table_match", printType(resultType.getElementType()), {}};
      expression.table = match.getTable().str();
      expression.nestedExpressions = std::move(nested.expressions);
      expression.nestedYields = std::move(nested.yields);
      plan.expressions.push_back(std::move(expression));
      continue;
    }
    if (auto choose = mlir::dyn_cast<ac::TableChooseOp>(operation)) {
      auto operands = operandNames(choose->getOperands());
      if (!operands)
        return operands.takeError();
      QueueBlockPlan nested;
      if (!choose.getKey().empty())
        if (auto error = extractExpressions(choose.getKey(), nested))
          return error;
      const std::array<std::pair<mlir::Value, llvm::StringRef>, 2> results = {{
          {choose.getIndex(), "table_choose_index"},
          {choose.getValid(), "table_choose_valid"},
      }};
      for (auto [resultValue, kind] : results) {
        auto resultType = mlir::cast<ac::VarType>(resultValue.getType());
        std::string result = "v" + std::to_string(plan.expressions.size());
        values[resultValue] = result;
        QueueExpressionPlan expression{result, kind.str(),
                                       printType(resultType.getElementType()),
                                       *operands};
        expression.table = choose.getTable().str();
        expression.predicate = choose.getPolicy().str();
        expression.nestedExpressions = nested.expressions;
        expression.nestedYields = nested.yields;
        plan.expressions.push_back(std::move(expression));
      }
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
    else if (auto yield = mlir::dyn_cast<ac::SlotYieldOp>(operation))
      yielded.push_back(yield.getValue());
    else if (auto yield = mlir::dyn_cast<ac::TableMatchYieldOp>(operation))
      yielded.push_back(yield.getValue());
    else if (auto yield = mlir::dyn_cast<ac::TableChooseYieldOp>(operation))
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
    if (!epoch || epoch.getValue() != "0.5")
      return planError("module requires ac.contract_epoch exactly '0.5'");
    auto modelKind = module->getAttrOfType<mlir::StringAttr>("ac.model_kind");
    if (!modelKind || modelKind.getValue() != "queue_graph")
      return planError(
          "module requires ac.model_kind exactly 'queue_graph'");
    for (mlir::Operation &operation : module.getBody()->getOperations()) {
      if (mlir::isa<ac::SystemOp, ac::ModuleOp, ac::ModuleExternOp,
                    ac::ModuleGeneratedOp>(operation))
        return planError(
            "structured system/module declaration is not legal in QueueGraph");
    }
    mlir::Operation *unclosed = nullptr;
    module.walk([&](mlir::Operation *operation) {
      if (!unclosed &&
          mlir::isa<ac::RuleOp, ac::TypeConstraintMarkerOp,
                    ac::ValueFactMarkerOp, ac::PendingObligationMarkerOp>(
              operation))
        unclosed = operation;
    });
    if (unclosed)
      return planError("unresolved rule or typed marker reached QueueGraph");
    bool hasFiring = false;
    module.walk([&](ac::FiringOp) { hasFiring = true; });
    if (hasFiring)
      return planError(
          "internal firing requires proved pure-firing canonicalization");
    mlir::LogicalResult loweredRuleProof = mlir::success();
    module.walk([&](ac::TransformOp transform) {
      if (mlir::failed(loweredRuleProof))
        return;
      loweredRuleProof = ac::verifyLoweredRuleTransformContract(transform);
    });
    if (mlir::failed(loweredRuleProof))
      return planError("lowered-rule proof verification failed");
    if (mlir::failed(acir::verifyFrozenFlatQueueGraph(module)))
      return planError("QueueGraph requires verified epoch 0.5 topology freeze");
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
      if (auto slot = mlir::dyn_cast<ac::SlotOp>(operation)) {
        auto input = queueName(slot.getInput(), names);
        if (!input)
          return input.takeError();
        plan.slots.push_back(
            {slot.getSymName().str(),
             printType(mlir::cast<ac::QueueType>(slot.getInput().getType())
                           .getElementType()),
             *input, scopePath(scope), slot.getStableId().str(),
             slot.getOwner().str()});
        continue;
      }
      if (auto match = mlir::dyn_cast<ac::TableMatchOp>(operation)) {
        const std::string name =
            "table_match_" + std::to_string(plan.tableMatches.size());
        QueueBlockPlan predicate;
        if (auto error = extractExpressions(match.getPredicate(), predicate))
          return error;
        if (predicate.yields.size() != 1)
          return planError("table.match predicate must yield one value");
        auto resultType = mlir::cast<ac::VarType>(match.getMask().getType());
        plan.tableMatches.push_back(
            {name, match.getTable().str(), scopePath(scope),
             printType(resultType.getElementType()),
             std::move(predicate.expressions), predicate.yields.front()});
        QueueExpressionPlan reference{
            "shared_match_" + std::to_string(plan.tableMatches.size() - 1),
            "table_match_ref",
            printType(resultType.getElementType()),
            {}};
        reference.field = name;
        reference.table = match.getTable().str();
        sharedExpressions.emplace_back(match.getMask(), std::move(reference));
        continue;
      }
      if (auto choose = mlir::dyn_cast<ac::TableChooseOp>(operation)) {
        auto matchValue = llvm::find_if(
            sharedExpressions, [&](const SharedExpression &candidate) {
              return candidate.first == choose.getMask() &&
                     candidate.second.kind == "table_match_ref";
            });
        if (matchValue == sharedExpressions.end())
          return planError("table.choose requires a shared table.match mask");
        const std::string name =
            "table_selection_" + std::to_string(plan.tableSelections.size());
        QueueBlockPlan key;
        if (choose.getPolicy() != "first") {
          if (auto error = extractExpressions(choose.getKey(), key))
            return error;
          if (key.yields.size() != 1)
            return planError("table.choose key must yield one value");
        }
        auto indexType = mlir::cast<ac::VarType>(choose.getIndex().getType());
        plan.tableSelections.push_back(
            {name, choose.getTable().str(), scopePath(scope),
             matchValue->second.field, choose.getPolicy().str(),
             printType(indexType.getElementType()), std::move(key.expressions),
             key.yields.empty() ? std::string() : key.yields.front()});
        QueueExpressionPlan indexReference{
            "shared_selection_" +
                std::to_string(plan.tableSelections.size() - 1) + "_index",
            "table_selection_index_ref",
            printType(indexType.getElementType()),
            {}};
        indexReference.field = name;
        indexReference.table = choose.getTable().str();
        sharedExpressions.emplace_back(choose.getIndex(),
                                       std::move(indexReference));
        auto validType = mlir::cast<ac::VarType>(choose.getValid().getType());
        QueueExpressionPlan validReference{
            "shared_selection_" +
                std::to_string(plan.tableSelections.size() - 1) + "_valid",
            "table_selection_valid_ref",
            printType(validType.getElementType()),
            {}};
        validReference.field = name;
        validReference.table = choose.getTable().str();
        sharedExpressions.emplace_back(choose.getValid(),
                                       std::move(validReference));
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
          if (auto error =
                  extractExpressions(*policy, blockPlan, sharedExpressions))
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
        std::vector<std::string> inputs;
        if (write.getInput()) {
          auto input = queueName(write.getInput(), names);
          if (!input)
            return input.takeError();
          inputs.push_back(std::move(*input));
        }
        auto name = write->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("table.write requires frozen ac.name");
        QueueBlockPlan blockPlan{
            "table_write", name.getValue().str(), scopePath(scope), inputs, {}};
        blockPlan.table = write.getTable().str();
        blockPlan.writeMode = write.getMode().str();
        for (mlir::Attribute rawField : write.getWriteFields())
          blockPlan.writeFields.push_back(
              mlir::cast<mlir::StringAttr>(rawField).getValue().str());
        std::vector<std::string> policyYields;
        for (mlir::Region *policy :
             {&write.getAddress(), &write.getEnable(), &write.getValue()}) {
          if (auto error =
                  extractExpressions(*policy, blockPlan, sharedExpressions))
            return error;
          if (blockPlan.yields.size() != 1)
            return planError("table write policy must yield one value");
          policyYields.push_back(blockPlan.yields.front());
        }
        blockPlan.yields = std::move(policyYields);
        plan.tableWrites.push_back(
            {blockPlan.table, blockPlan.name, blockPlan.scope,
             inputs.empty() ? std::string() : inputs.front(),
             blockPlan.writeMode, blockPlan.writeFields});
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto write = mlir::dyn_cast<ac::TableMaskedWriteOp>(operation)) {
        auto name = write->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("table.masked_write requires frozen ac.name");
        QueueBlockPlan blockPlan{"table_masked_write",
                                 name.getValue().str(),
                                 scopePath(scope),
                                 {},
                                 {}};
        blockPlan.table = write.getTable().str();
        blockPlan.writeMode = write.getMode().str();
        for (mlir::Attribute rawField : write.getWriteFields())
          blockPlan.writeFields.push_back(
              mlir::cast<mlir::StringAttr>(rawField).getValue().str());
        auto matchValue = llvm::find_if(
            sharedExpressions, [&](const SharedExpression &candidate) {
              return candidate.first == write.getMask() &&
                     candidate.second.kind == "table_match_ref";
            });
        if (matchValue == sharedExpressions.end())
          return planError("masked table write requires a shared match mask");
        blockPlan.expressions.push_back(matchValue->second);
        std::vector<std::string> policyYields{matchValue->second.result};
        for (mlir::Region *policy : {&write.getEnable(), &write.getValue()}) {
          if (auto error =
                  extractExpressions(*policy, blockPlan, sharedExpressions))
            return error;
          if (blockPlan.yields.size() != 1)
            return planError("masked table write policy must yield one value");
          policyYields.push_back(blockPlan.yields.front());
        }
        blockPlan.yields = std::move(policyYields);
        plan.tableMaskedWrites.push_back({blockPlan.table, blockPlan.name,
                                          blockPlan.scope, blockPlan.writeMode,
                                          blockPlan.writeFields});
        plan.blocks.push_back(std::move(blockPlan));
        continue;
      }
      if (auto release = mlir::dyn_cast<ac::SlotReleaseOp>(operation)) {
        auto slot = llvm::find_if(plan.slots, [&](const SlotPlan &candidate) {
          return candidate.name == release.getSlot();
        });
        if (slot == plan.slots.end())
          return planError("slot.release references unknown slot");
        auto name = release->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("slot.release requires frozen ac.name");
        QueueBlockPlan blockPlan{
            "slot", name.getValue().str(), scopePath(scope), {slot->input}, {}};
        blockPlan.slot = slot->name;
        blockPlan.region = printRegion(release.getWhen());
        if (auto error = extractExpressions(release.getWhen(), blockPlan,
                                            sharedExpressions))
          return error;
        if (blockPlan.yields.size() != 1)
          return planError("slot release policy must yield one value");
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
  std::vector<SharedExpression> sharedExpressions;
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
  llvm::StringMap<const TableMatchPlan *> tableMatches;
  for (const TableMatchPlan &match : plan.tableMatches) {
    const TablePlan *table = tables.lookup(match.table);
    auto width = integerWidth(match.resultType);
    if (match.name.empty() || !table || !width || *width != table->entries ||
        match.resultType.empty() || match.yield.empty() ||
        !tableMatches.try_emplace(match.name, &match).second)
      return planError("table match metadata is incomplete or duplicated");
  }
  llvm::StringMap<const TableSelectionPlan *> tableSelections;
  for (const TableSelectionPlan &selection : plan.tableSelections) {
    const TablePlan *table = tables.lookup(selection.table);
    const TableMatchPlan *match = tableMatches.lookup(selection.match);
    auto indexWidth = integerWidth(selection.indexType);
    const unsigned expectedIndexWidth =
        table ? std::max<unsigned>(1, llvm::Log2_64_Ceil(table->entries)) : 0;
    if (selection.name.empty() || !table || !match || !indexWidth ||
        *indexWidth != expectedIndexWidth || match->table != selection.table ||
        selection.indexType.empty() ||
        (selection.policy != "first" && selection.policy != "min" &&
         selection.policy != "max") ||
        (selection.policy == "first" &&
         (!selection.keyExpressions.empty() || !selection.keyYield.empty())) ||
        (selection.policy != "first" && selection.keyYield.empty()) ||
        !tableSelections.try_emplace(selection.name, &selection).second)
      return planError("table selection metadata is incomplete, duplicated, or "
                       "inconsistent");
  }
  auto verifySharedExpression =
      [&](auto &&self, const QueueExpressionPlan &expression) -> llvm::Error {
    if (expression.kind == "table_match_ref") {
      const TableMatchPlan *match = tableMatches.lookup(expression.field);
      if (!match)
        return planError("table_match_ref references unknown match target");
      if (expression.table != match->table)
        return planError("table_match_ref Table provenance is inconsistent");
      if (expression.type != match->resultType)
        return planError("table_match_ref field type is inconsistent");
    } else if (expression.kind == "table_selection_index_ref" ||
               expression.kind == "table_selection_valid_ref") {
      const TableSelectionPlan *selection =
          tableSelections.lookup(expression.field);
      if (!selection)
        return planError(
            "table_selection_ref references unknown selection target");
      if (expression.table != selection->table)
        return planError(
            "table_selection_ref Table provenance is inconsistent");
      const llvm::StringRef expected =
          expression.kind == "table_selection_index_ref"
              ? llvm::StringRef(selection->indexType)
              : llvm::StringRef("i1");
      if (expression.type != expected)
        return planError("table_selection_ref field type is inconsistent");
    }
    for (const QueueExpressionPlan &nested : expression.nestedExpressions)
      if (auto error = self(self, nested))
        return error;
    return llvm::Error::success();
  };
  auto verifySharedExpressions = [&](const auto &expressions) -> llvm::Error {
    for (const QueueExpressionPlan &expression : expressions)
      if (auto error =
              verifySharedExpression(verifySharedExpression, expression))
        return error;
    return llvm::Error::success();
  };
  for (const TableMatchPlan &match : plan.tableMatches)
    if (auto error = verifySharedExpressions(match.expressions))
      return error;
  for (const TableSelectionPlan &selection : plan.tableSelections)
    if (auto error = verifySharedExpressions(selection.keyExpressions))
      return error;
  llvm::StringMap<unsigned> tableReaders;
  llvm::StringMap<llvm::StringSet<>> tableWriterFields;
  llvm::StringSet<> tableReplaceWriters;
  auto verifyWriteFields = [&](llvm::StringRef tableName, llvm::StringRef mode,
                               const std::vector<std::string> &writeFields) {
    const TablePlan *table = tables.lookup(tableName);
    if (!table || writeFields.empty())
      return false;
    llvm::StringSet<> allowed;
    llvm::StringMap<unsigned> ordinals;
    if (llvm::StringRef(table->entryType).starts_with("!ac.struct<")) {
      size_t marker = table->entryType.rfind('@');
      size_t end = table->entryType.rfind('>');
      if (marker == std::string::npos || end == std::string::npos ||
          marker >= end)
        return false;
      llvm::StringRef payloadName(table->entryType.data() + marker + 1,
                                  end - marker - 1);
      auto payload =
          llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &item) {
            return item.name == payloadName;
          });
      if (payload == plan.payloads.end())
        return false;
      for (auto [ordinal, field] : llvm::enumerate(payload->fields)) {
        allowed.insert(field.name);
        ordinals[field.name] = ordinal;
      }
    } else {
      allowed.insert("$entry");
      ordinals["$entry"] = 0;
    }
    llvm::StringSet<> local;
    std::optional<unsigned> previousOrdinal;
    for (const std::string &field : writeFields) {
      if (field.empty() || !allowed.contains(field) ||
          !local.insert(field).second)
        return false;
      unsigned ordinal = ordinals.lookup(field);
      if (previousOrdinal && ordinal <= *previousOrdinal)
        return false;
      previousOrdinal = ordinal;
    }
    if (mode != "field" && mode != "replace")
      return false;
    if (mode == "replace")
      return writeFields.size() == allowed.size() &&
             tableReplaceWriters.insert(tableName).second;
    for (const std::string &field : writeFields)
      if (!tableWriterFields[tableName].insert(field).second)
        return false;
    return true;
  };
  for (const TableReadPlan &read : plan.tableReads) {
    if (!tables.contains(read.table) || read.name.empty() ||
        read.output.empty() || read.depth == 0 || read.latency == 0)
      return planError("table read endpoint metadata is incomplete");
    ++tableReaders[read.table];
  }
  for (const TableWritePlan &write : plan.tableWrites) {
    if (!tables.contains(write.table) || write.name.empty())
      return planError("table write endpoint metadata is incomplete");
    if (!verifyWriteFields(write.table, write.mode, write.writeFields))
      return planError(
          "table write_fields are invalid or overlap another writer");
  }
  for (const TableMaskedWritePlan &write : plan.tableMaskedWrites) {
    if (!tables.contains(write.table) || write.name.empty())
      return planError("masked table write endpoint metadata is incomplete");
    if (write.mode != "field" ||
        !verifyWriteFields(write.table, write.mode, write.writeFields))
      return planError(
          "table write_fields are invalid or overlap another writer");
  }
  for (const auto &entry : tables)
    if (tableReaders[entry.getKey()] +
            (tableWriterFields.contains(entry.getKey()) ? 1U : 0U) +
            (tableReplaceWriters.contains(entry.getKey()) ? 1U : 0U) ==
        0)
      return planError("table '" + entry.getKey() + "' has no endpoints");
  llvm::StringSet<> slotNames;
  for (const SlotPlan &slot : plan.slots)
    if (slot.name.empty() || !slotNames.insert(slot.name).second ||
        slot.payloadType.empty() || slot.input.empty() || slot.scope.empty() ||
        slot.stableId.empty() || slot.ownerPath.empty())
      return planError("slot metadata is incomplete or duplicated");
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
    if (auto error = verifySharedExpressions(block.expressions))
      return error;
    if (block.kind == "memory_request" &&
        !memoryInstances.contains(block.memoryInstance))
      return planError("memory request block references unknown instance");
    if ((block.kind == "table_read" || block.kind == "table_write" ||
         block.kind == "table_masked_write") &&
        !tables.contains(block.table))
      return planError("table endpoint block references unknown table");
    if (block.kind == "table_write") {
      auto endpoint =
          llvm::find_if(plan.tableWrites, [&](const TableWritePlan &write) {
            return write.name == block.name && write.table == block.table &&
                   write.scope == block.scope;
          });
      if (endpoint == plan.tableWrites.end() ||
          endpoint->mode != block.writeMode ||
          endpoint->writeFields != block.writeFields)
        return planError("table write block mode/fields are inconsistent");
    }
    if (block.kind == "table_masked_write") {
      auto endpoint = llvm::find_if(
          plan.tableMaskedWrites, [&](const TableMaskedWritePlan &write) {
            return write.name == block.name && write.table == block.table &&
                   write.scope == block.scope;
          });
      if (endpoint == plan.tableMaskedWrites.end() ||
          endpoint->mode != block.writeMode ||
          endpoint->writeFields != block.writeFields)
        return planError(
            "masked table write block mode/fields are inconsistent");
    }
    if (block.kind == "slot" && !slotNames.contains(block.slot))
      return planError("slot block references unknown slot");
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
  auto expressionJson =
      [&](auto &&self,
          const QueueExpressionPlan &expression) -> llvm::json::Object {
    llvm::json::Array operands;
    for (const std::string &operand : expression.operands)
      operands.push_back(operand);
    llvm::json::Array nested;
    for (const QueueExpressionPlan &item : expression.nestedExpressions)
      nested.push_back(self(self, item));
    llvm::json::Array nestedYields;
    for (const std::string &yield : expression.nestedYields)
      nestedYields.push_back(yield);
    return llvm::json::Object{{"field", expression.field},
                              {"kind", expression.kind},
                              {"literal", expression.literal},
                              {"nested_expressions", std::move(nested)},
                              {"nested_yields", std::move(nestedYields)},
                              {"operands", std::move(operands)},
                              {"predicate", expression.predicate},
                              {"result", expression.result},
                              {"slot", expression.slot},
                              {"table", expression.table},
                              {"type", expression.type}};
  };
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
    for (const QueueExpressionPlan &expression : block.expressions)
      expressions.push_back(expressionJson(expressionJson, expression));
    llvm::json::Array yields;
    for (const std::string &yield : block.yields)
      yields.push_back(yield);
    llvm::json::Array writeFields;
    for (const std::string &field : block.writeFields)
      writeFields.push_back(field);
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
                           {"write_mode", block.writeMode},
                           {"table", block.table},
                           {"slot", block.slot},
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
                           {"write_fields", std::move(writeFields)},
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
  llvm::json::Array tableMatchValues;
  for (const TableMatchPlan &match : tableMatches) {
    llvm::json::Array expressions;
    for (const QueueExpressionPlan &expression : match.expressions)
      expressions.push_back(expressionJson(expressionJson, expression));
    tableMatchValues.push_back(
        llvm::json::Object{{"expressions", std::move(expressions)},
                           {"name", match.name},
                           {"result_type", match.resultType},
                           {"scope", match.scope},
                           {"table", match.table},
                           {"yield", match.yield}});
  }
  llvm::json::Array tableSelectionValues;
  for (const TableSelectionPlan &selection : tableSelections) {
    llvm::json::Array expressions;
    for (const QueueExpressionPlan &expression : selection.keyExpressions)
      expressions.push_back(expressionJson(expressionJson, expression));
    tableSelectionValues.push_back(
        llvm::json::Object{{"index_type", selection.indexType},
                           {"key_expressions", std::move(expressions)},
                           {"key_yield", selection.keyYield},
                           {"match", selection.match},
                           {"name", selection.name},
                           {"policy", selection.policy},
                           {"scope", selection.scope},
                           {"table", selection.table}});
  }
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
  for (const TableWritePlan &write : tableWrites) {
    llvm::json::Array writeFields;
    for (const std::string &field : write.writeFields)
      writeFields.push_back(field);
    tableWriteValues.push_back(
        llvm::json::Object{{"input", write.input},
                           {"mode", write.mode},
                           {"name", write.name},
                           {"scope", write.scope},
                           {"table", write.table},
                           {"write_fields", std::move(writeFields)}});
  }
  llvm::json::Array tableMaskedWriteValues;
  for (const TableMaskedWritePlan &write : tableMaskedWrites) {
    llvm::json::Array writeFields;
    for (const std::string &field : write.writeFields)
      writeFields.push_back(field);
    tableMaskedWriteValues.push_back(
        llvm::json::Object{{"name", write.name},
                           {"mode", write.mode},
                           {"scope", write.scope},
                           {"table", write.table},
                           {"write_fields", std::move(writeFields)}});
  }
  llvm::json::Array slotValues;
  for (const SlotPlan &slot : slots)
    slotValues.push_back(llvm::json::Object{{"input", slot.input},
                                            {"name", slot.name},
                                            {"owner_path", slot.ownerPath},
                                            {"payload_type", slot.payloadType},
                                            {"scope", slot.scope},
                                            {"stable_id", slot.stableId}});
  llvm::json::Object root{
      {"blocks", std::move(blockValues)},
      {"contract_epoch", "0.5"},
      {"memory_instances", std::move(memoryInstanceValues)},
      {"memory_requests", std::move(memoryRequestValues)},
      {"payloads", std::move(payloadValues)},
      {"queues", std::move(queueValues)},
      {"schema", "agentic-circuit-queue-graph-plan"},
      {"scopes", std::move(scopeValues)},
      {"slots", std::move(slotValues)},
      {"specialization", specializationFingerprint.empty()
                             ? llvm::json::Value(nullptr)
                             : llvm::json::Value(specializationFingerprint)},
      {"table_reads", std::move(tableReadValues)},
      {"table_matches", std::move(tableMatchValues)},
      {"table_masked_writes", std::move(tableMaskedWriteValues)},
      {"table_selections", std::move(tableSelectionValues)},
      {"table_writes", std::move(tableWriteValues)},
      {"tables", std::move(tableValues)},
      {"system", system},
      {"version", "0.5"}};
  return bindings::canonicalizeJson(llvm::json::Value(std::move(root)));
}

} // namespace acir::codegen
