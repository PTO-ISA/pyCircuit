#include "acir/CodeGen/QueueGraphGenerator.h"

#include "acir/Bindings/Binding.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MathExtras.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <system_error>

namespace acir::codegen {
namespace {

llvm::Error costError(const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument),
      "ACLOWER-QUEUE-COST: " + message);
}

bool isLowerHex(llvm::StringRef value) {
  return llvm::all_of(value, [](char character) {
    return (character >= '0' && character <= '9') ||
           (character >= 'a' && character <= 'f');
  });
}

bool isSha256(llvm::StringRef value) {
  return value.consume_front("sha256:") && value.size() == 64 &&
         isLowerHex(value);
}

bool isRevision(llvm::StringRef value) {
  return value.size() == 40 && isLowerHex(value);
}

std::optional<uint64_t> integerWidth(llvm::StringRef type) {
  if (!type.consume_front("i"))
    return std::nullopt;
  uint64_t width = 0;
  return !type.getAsInteger(10, width) && width > 0 && width <= 64
             ? std::optional<uint64_t>(width)
             : std::nullopt;
}

std::optional<uint64_t> rangeWidth(llvm::StringRef type) {
  if (!type.consume_front("!ac.range<") || !type.consume_back(">"))
    return std::nullopt;
  auto [lowerText, upperText] = type.split(',');
  uint64_t lower = 0;
  uint64_t upper = 0;
  if (lowerText.trim().getAsInteger(10, lower) ||
      upperText.trim().getAsInteger(10, upper) || lower > upper)
    return std::nullopt;
  return upper == std::numeric_limits<uint64_t>::max()
             ? std::optional<uint64_t>(64)
             : std::optional<uint64_t>(
                   std::max<uint64_t>(1, llvm::Log2_64_Ceil(upper + 1)));
}

std::optional<llvm::StringRef> nominalName(llvm::StringRef type,
                                           llvm::StringRef prefix) {
  if (!type.consume_front(prefix) || !type.consume_back(">"))
    return std::nullopt;
  auto [scope, name] = type.rsplit("::@");
  if (name.empty() || scope.empty())
    return std::nullopt;
  return name;
}

llvm::Expected<uint64_t> typeWidth(const QueueGraphPlan &plan,
                                   llvm::StringRef type) {
  if (auto width = integerWidth(type))
    return *width;
  if (auto width = rangeWidth(type))
    return *width;
  if (auto name = nominalName(type, "!ac.enum<")) {
    auto found = llvm::find_if(plan.enums, [&](const QueueEnumPlan &item) {
      return item.name == *name;
    });
    if (found != plan.enums.end())
      return found->width;
  }
  if (auto name = nominalName(type, "!ac.struct<")) {
    auto found = llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &item) {
      return item.name == *name;
    });
    if (found != plan.payloads.end()) {
      uint64_t width = 0;
      for (const QueuePayloadFieldPlan &field : found->fields) {
        if (field.width > std::numeric_limits<uint64_t>::max() - width)
          return costError("payload width overflows uint64_t");
        width += field.width;
      }
      return width;
    }
  }
  auto aggregate =
      llvm::find_if(plan.aggregates, [&](const QueueAggregatePlan &item) {
        return item.type == type;
      });
  if (aggregate != plan.aggregates.end())
    return aggregate->width;
  return costError("type has no verified packed width: " + type);
}

uint64_t maxArrayExtent(const QueueGraphPlan &plan, llvm::StringRef type,
                        llvm::StringSet<> &active) {
  if (!active.insert(type).second)
    return 0;
  auto finish = [&](uint64_t value) {
    active.erase(type);
    return value;
  };
  auto aggregate =
      llvm::find_if(plan.aggregates, [&](const QueueAggregatePlan &item) {
        return item.type == type;
      });
  if (aggregate != plan.aggregates.end()) {
    uint64_t result = aggregate->kind == "array" ? aggregate->length : 0;
    for (const std::string &element : aggregate->elements)
      result = std::max(result, maxArrayExtent(plan, element, active));
    return finish(result);
  }
  if (auto name = nominalName(type, "!ac.struct<")) {
    auto payload =
        llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &item) {
          return item.name == *name;
        });
    uint64_t result = 0;
    if (payload != plan.payloads.end())
      for (const QueuePayloadFieldPlan &field : payload->fields)
        result = std::max(result, maxArrayExtent(plan, field.type, active));
    return finish(result);
  }
  return finish(0);
}

uint64_t maxArrayExtent(const QueueGraphPlan &plan, llvm::StringRef type) {
  llvm::StringSet<> active;
  return maxArrayExtent(plan, type, active);
}

llvm::json::Object provenanceJson(
    const QueueSourceProvenancePlan &provenance) {
  llvm::json::Array origins;
  for (const QueueSourceOriginPlan &origin : provenance.origins) {
    llvm::json::Array frames;
    for (const QueueSourceFramePlan &frame : origin) {
      llvm::json::Object value{{"column", frame.column},
                               {"file", frame.file},
                               {"kind", frame.kind},
                               {"line", frame.line}};
      if (!frame.symbol.empty())
        value["symbol"] = frame.symbol;
      frames.push_back(std::move(value));
    }
    origins.push_back(llvm::json::Object{{"frames", std::move(frames)}});
  }
  return llvm::json::Object{{"origins", std::move(origins)}};
}

llvm::json::Object metric(llvm::StringRef status, llvm::StringRef stage,
                          llvm::StringRef unit, uint64_t value) {
  return llvm::json::Object{{"stage", stage},
                            {"status", status},
                            {"unit", unit},
                            {"value", value}};
}

llvm::json::Object unmodeled(llvm::StringRef stage, llvm::StringRef unit,
                             llvm::StringRef reason) {
  return llvm::json::Object{{"reason", reason},
                            {"stage", stage},
                            {"status", "not_modeled"},
                            {"unit", unit}};
}

struct PayloadCost {
  uint64_t logicalBits = 0;
  uint64_t carrierBits = 0;
  uint64_t packedBytes = 0;
};

llvm::Expected<PayloadCost> queuePayloadCost(const QueueGraphPlan &plan,
                                             const QueuePlan &queue) {
  auto physical = typeWidth(plan, queue.payloadType);
  if (!physical)
    return physical.takeError();
  PayloadCost result;
  result.logicalBits = queue.payloadProjection
                           ? queue.payloadProjection->logicalBits
                           : *physical;
  result.carrierBits = queue.payloadProjection
                           ? queue.payloadProjection->carrierBits
                           : *physical;
  result.packedBytes = (result.carrierBits + 7) / 8;
  return result;
}

llvm::json::Object queueReference(const QueueGraphPlan &plan,
                                  const QueuePlan &queue,
                                  const PayloadCost &cost) {
  llvm::json::Object result{{"carrier_bits", cost.carrierBits},
                            {"depth", queue.depth},
                            {"lanes", queue.lanes},
                            {"logical_bits", cost.logicalBits},
                            {"name", queue.name},
                            {"packed_bytes", cost.packedBytes},
                            {"rate", queue.rate},
                            {"scope", queue.scope}};
  if (queue.payloadProjection)
    result["projection_profile"] = queue.payloadProjection->profile;
  return result;
}

uint64_t expressionNodeCount(const QueueExpressionPlan &expression) {
  uint64_t result = 1;
  for (const QueueExpressionPlan &nested : expression.nestedExpressions)
    result += expressionNodeCount(nested);
  return result;
}

uint64_t scanBound(const QueueGraphPlan &plan,
                   const QueueExpressionPlan &expression) {
  if (expression.kind != "table_match")
    return 0;
  if (!expression.hasDomainProjection) {
    auto table = llvm::find_if(plan.tables, [&](const TablePlan &item) {
      return item.name == expression.table;
    });
    return table == plan.tables.end() ? 0 : table->entries;
  }
  uint64_t result = 1;
  for (uint64_t extent : expression.domainShape) {
    if (extent == 0 || result > std::numeric_limits<uint64_t>::max() / extent)
      return std::numeric_limits<uint64_t>::max();
    result *= extent;
  }
  return result;
}

struct DepthCost {
  bool modeled = true;
  uint64_t value = 0;
};

struct DepthEvaluation {
  bool modeled = true;
  uint64_t maximum = 0;
  uint64_t selected = 0;
};

DepthEvaluation evaluateExpressionDepth(
    const QueueGraphPlan &plan,
    const std::vector<QueueExpressionPlan> &expressions,
    llvm::StringMap<uint64_t> depths,
    std::optional<llvm::StringRef> selectedResult,
    llvm::StringSet<> &activeHelpers) {
  uint64_t maximum = 0;
  for (const QueueExpressionPlan &expression : expressions) {
    if (!expression.nestedExpressions.empty() ||
        expression.kind.starts_with("table_") ||
        expression.kind == "snapshot_set")
      return {false, 0, 0};
    uint64_t inputDepth = 0;
    for (const std::string &operand : expression.operands)
      inputDepth = std::max(inputDepth, depths.lookup(operand));
    if (expression.kind == "helper_call") {
      auto helper = llvm::find_if(
          plan.helpers, [&](const QueueHelperPlan &candidate) {
            return candidate.name == expression.field;
          });
      if (helper == plan.helpers.end() ||
          expression.operands.size() != helper->inputNames.size() ||
          expression.laneOrdinal >= helper->yields.size() ||
          !activeHelpers.insert(helper->name).second)
        return {false, 0, 0};
      llvm::StringMap<uint64_t> helperInputs;
      for (auto [name, operand] :
           llvm::zip_equal(helper->inputNames, expression.operands))
        helperInputs[name] = depths.lookup(operand);
      const DepthEvaluation helperDepth = evaluateExpressionDepth(
          plan, helper->expressions, std::move(helperInputs),
          helper->yields[expression.laneOrdinal], activeHelpers);
      activeHelpers.erase(helper->name);
      if (!helperDepth.modeled)
        return {false, 0, 0};
      depths[expression.result] = helperDepth.selected;
      maximum = std::max(maximum, helperDepth.maximum);
      continue;
    }
    uint64_t operationCost = expression.kind == "constant" ||
                                     expression.kind == "enum_constant"
                                 ? 0
                                 : 1;
    if (expression.kind == "cmp" &&
        (expression.predicate == "ne" || expression.predicate == "sge" ||
         expression.predicate == "uge" || expression.predicate == "sle" ||
         expression.predicate == "ule"))
      operationCost = 2;
    if (expression.kind == "array_get_dynamic" ||
        expression.kind == "array_update_dynamic") {
      const uint64_t count = expression.selectionCount;
      const uint64_t ordinalWidth =
          std::max<uint64_t>(1, llvm::Log2_64_Ceil(count));
      const bool widensIndex = expression.indexWidth < ordinalWidth;
      const uint64_t selectionDepth =
          expression.kind == "array_get_dynamic"
              ? static_cast<uint64_t>(llvm::Log2_64_Ceil(count))
              : uint64_t{1};
      operationCost = expression.kind == "array_get_dynamic"
                          ? (count == 1 ? uint64_t{1}
                                        : selectionDepth +
                                              (widensIndex ? 2 : 1))
                          : (widensIndex ? uint64_t{4} : uint64_t{3});
    }
    const uint64_t depth = inputDepth + operationCost;
    depths[expression.result] = depth;
    maximum = std::max(maximum, depth);
  }
  return {true, maximum,
          selectedResult ? depths.lookup(*selectedResult) : maximum};
}

DepthCost expressionDepth(const QueueGraphPlan &plan,
                          const QueueBlockPlan &block) {
  llvm::StringSet<> activeHelpers;
  const DepthEvaluation result = evaluateExpressionDepth(
      plan, block.expressions, {}, std::nullopt, activeHelpers);
  return {result.modeled, result.maximum};
}

std::string blockIdentity(const QueueBlockPlan &block, size_t index) {
  if (!block.stableId.empty())
    return block.stableId;
  return "block:" + block.scope + ":" + block.kind + ":" + block.name +
         ":" + std::to_string(index);
}

llvm::Expected<llvm::json::Object>
moduleCost(const QueueGraphPlan &plan, llvm::StringRef instancePath,
           uint64_t &totalLogicalBits, uint64_t &totalCarrierBits,
           uint64_t &totalStorageBytes, uint64_t &totalNodes,
           uint64_t &totalMaxDepth, uint64_t &modeledRules,
           uint64_t &totalRules) {
  uint64_t moduleLogicalBits = 0;
  uint64_t moduleCarrierBits = 0;
  uint64_t moduleStorageBytes = 0;
  uint64_t moduleNodes = 0;
  uint64_t moduleMaxDepth = 0;
  bool moduleDepthModeled = true;
  llvm::StringMap<const QueuePlan *> queues;
  llvm::StringMap<PayloadCost> queueCosts;
  llvm::json::Array queueValues;
  for (const QueuePlan &queue : plan.queues) {
    auto cost = queuePayloadCost(plan, queue);
    if (!cost)
      return cost.takeError();
    queues[queue.name] = &queue;
    queueCosts[queue.name] = *cost;
    queueValues.push_back(queueReference(plan, queue, *cost));
    moduleLogicalBits += cost->logicalBits;
    moduleCarrierBits += cost->carrierBits;
    const uint64_t storage = cost->packedBytes * queue.depth;
    moduleStorageBytes += storage;
  }

  llvm::json::Array rules;
  for (auto [index, block] : llvm::enumerate(plan.blocks)) {
    ++totalRules;
    llvm::json::Array inputs;
    llvm::json::Array outputs;
    for (const std::string &name : block.inputs) {
      const QueuePlan *queue = queues.lookup(name);
      if (queue)
        inputs.push_back(queueReference(plan, *queue, queueCosts.lookup(name)));
    }
    for (const std::string &name : block.outputs) {
      const QueuePlan *queue = queues.lookup(name);
      if (queue)
        outputs.push_back(queueReference(plan, *queue, queueCosts.lookup(name)));
    }
    uint64_t nodes = 0;
    uint64_t scans = 0;
    uint64_t dynamicExpansion = 1;
    uint64_t fixedExpansionBound = 0;
    for (const std::string &name : block.inputs)
      if (const QueuePlan *queue = queues.lookup(name))
        fixedExpansionBound = std::max(
            fixedExpansionBound, maxArrayExtent(plan, queue->payloadType));
    for (const std::string &name : block.outputs)
      if (const QueuePlan *queue = queues.lookup(name))
        fixedExpansionBound = std::max(
            fixedExpansionBound, maxArrayExtent(plan, queue->payloadType));
    for (const QueueExpressionPlan &expression : block.expressions) {
      nodes += expressionNodeCount(expression);
      scans = std::max(scans, scanBound(plan, expression));
      fixedExpansionBound = std::max(
          fixedExpansionBound, maxArrayExtent(plan, expression.type));
      if (expression.kind == "array_get_dynamic" ||
          expression.kind == "array_update_dynamic")
        dynamicExpansion =
            std::max(dynamicExpansion, expression.selectionCount);
    }
    moduleNodes += nodes;
    const DepthCost depth = expressionDepth(plan, block);
    if (depth.modeled) {
      ++modeledRules;
      moduleMaxDepth = std::max(moduleMaxDepth, depth.value);
    } else {
      moduleDepthModeled = false;
    }
    llvm::json::Object metrics;
    metrics["queuegraph_nodes"] =
        metric("exact", "verified_queuegraph", "nodes", nodes);
    metrics["logic_depth"] =
        depth.modeled
            ? llvm::json::Value(metric("static_upper_bound",
                                       "queuegraph_to_pyc_pre_dce",
                                       "unit_cost_levels", depth.value))
            : llvm::json::Value(unmodeled(
                  "queuegraph_to_pyc_pre_dce", "unit_cost_levels",
                  "nested, helper, or Table-dependent expression graph"));
    metrics["table_scan_bound"] =
        scans == 0
            ? llvm::json::Value(metric("not_applicable", "verified_queuegraph",
                                       "entries_per_attempt", 0))
            : llvm::json::Value(metric("static_upper_bound",
                                       "verified_queuegraph",
                                       "entries_per_attempt", scans));
    metrics["dynamic_array_expansion_factor"] =
        dynamicExpansion == 1
            ? llvm::json::Value(unmodeled(
                  "verified_queuegraph", "lanes",
                  "frontend scalar combinator groups are not yet retained"))
            : llvm::json::Value(metric("exact", "verified_queuegraph",
                                       "lanes", dynamicExpansion));
    metrics["fixed_array_expansion_bound"] =
        fixedExpansionBound == 0
            ? llvm::json::Value(metric("not_applicable", "verified_queuegraph",
                                       "lanes", 0))
            : llvm::json::Value(metric("static_upper_bound",
                                       "verified_queuegraph", "lanes",
                                       fixedExpansionBound));
    const bool simpleTransform = block.kind == "transform" &&
                                 block.inputs.size() == 1 &&
                                 block.outputs.size() == 1;
    metrics["explicit_copy_sites"] =
        simpleTransform
            ? llvm::json::Value(
                  metric("exact", "generated_gfsim_policy", "sites", 0))
            : llvm::json::Value(unmodeled(
                  "generated_gfsim_policy", "sites",
                  "block-specific prepare/publish path is not modeled"));
    metrics["output_materialization_sites"] =
        simpleTransform
            ? llvm::json::Value(
                  metric("exact", "generated_gfsim_policy", "sites", 1))
            : llvm::json::Value(unmodeled(
                  "generated_gfsim_policy", "sites",
                  "block-specific prepare/publish path is not modeled"));
    metrics["output_move_sites"] =
        simpleTransform
            ? llvm::json::Value(
                  metric("exact", "generated_gfsim_policy", "sites", 1))
            : llvm::json::Value(unmodeled(
                  "generated_gfsim_policy", "sites",
                  "block-specific prepare/publish path is not modeled"));
    metrics["runtime_vector_construction_sites"] =
        simpleTransform
            ? llvm::json::Value(
                  metric("exact", "generated_gfsim_policy", "sites", 0))
            : llvm::json::Value(unmodeled(
                  "generated_gfsim_runtime", "sites",
                  "runtime template internals require a separate measured lane"));

    llvm::json::Object value{{"display_rule_name", block.displayRuleName},
                             {"identity", blockIdentity(block, index)},
                             {"inputs", std::move(inputs)},
                             {"kind", block.kind},
                             {"metrics", std::move(metrics)},
                             {"name", block.name},
                             {"outputs", std::move(outputs)},
                             {"scope", block.scope}};
    if (!block.sourceProvenance.origins.empty())
      value["source_provenance"] = provenanceJson(block.sourceProvenance);
    rules.push_back(std::move(value));
  }

  totalLogicalBits += moduleLogicalBits;
  totalCarrierBits += moduleCarrierBits;
  totalStorageBytes += moduleStorageBytes;
  totalNodes += moduleNodes;
  totalMaxDepth = std::max(totalMaxDepth, moduleMaxDepth);

  llvm::json::Array sharedCosts;
  for (const TableMatchPlan &match : plan.tableMatches) {
    uint64_t bound = 0;
    if (match.hasDomainProjection) {
      bound = 1;
      for (uint64_t extent : match.domainShape)
        bound *= extent;
    } else {
      auto table = llvm::find_if(plan.tables, [&](const TablePlan &item) {
        return item.name == match.table;
      });
      if (table != plan.tables.end())
        bound = table->entries;
    }
    llvm::json::Object value{
        {"identity", "table_match:" + match.scope + ":" + match.name},
        {"kind", "table_match"},
        {"name", match.name},
        {"scope", match.scope},
        {"table", match.table},
        {"scan_bound",
         metric("static_upper_bound", "verified_queuegraph",
                "entries_per_attempt", bound)}};
    if (!match.sourceProvenance.origins.empty())
      value["source_provenance"] = provenanceJson(match.sourceProvenance);
    sharedCosts.push_back(std::move(value));
  }

  llvm::json::Object totals{
      {"carrier_queue_bits", metric("exact", "verified_queuegraph", "bits",
                                    moduleCarrierBits)},
      {"logical_queue_bits", metric("exact", "verified_queuegraph", "bits",
                                    moduleLogicalBits)},
      {"logic_depth",
       moduleDepthModeled
           ? llvm::json::Value(metric("static_upper_bound",
                                      "queuegraph_to_pyc_pre_dce",
                                      "unit_cost_levels", moduleMaxDepth))
           : llvm::json::Value(unmodeled(
                 "queuegraph_to_pyc_pre_dce", "unit_cost_levels",
                 "one or more rules use an unmodeled expression graph"))},
      {"queue_storage_packed_bytes",
       metric("static_upper_bound", "generated_gfsim", "bytes",
              moduleStorageBytes)},
      {"queuegraph_nodes",
       metric("exact", "verified_queuegraph", "nodes", moduleNodes)}};

  return llvm::json::Object{
      {"definition", plan.definition.empty() ? llvm::json::Value(nullptr)
                                              : llvm::json::Value(plan.definition)},
      {"instance_path", instancePath.str()},
      {"queues", std::move(queueValues)},
      {"rules", std::move(rules)},
      {"shared_costs", std::move(sharedCosts)},
      {"specialization",
       plan.specializationFingerprint.empty()
           ? llvm::json::Value(nullptr)
           : llvm::json::Value(plan.specializationFingerprint)},
      {"system", plan.system},
      {"totals", std::move(totals)}};
}

} // namespace

llvm::Expected<std::string> generateQueueGraphCostReport(
    const QueueGraphPlan &plan, llvm::StringRef sdkProductVersion,
    llvm::StringRef sdkSourceRevision, llvm::StringRef queueGraphSha256,
    llvm::StringRef sourceMapSha256) {
  if (auto error = verifyQueueGraphPlan(plan))
    return std::move(error);
  if (sdkProductVersion.empty())
    return costError("SDK product version is required");
  if (!isRevision(sdkSourceRevision))
    return costError("SDK source revision must be 40 lowercase hex digits");
  if (!isSha256(queueGraphSha256) || !isSha256(sourceMapSha256))
    return costError("report identity requires canonical SHA-256 fingerprints");

  uint64_t totalLogicalBits = 0;
  uint64_t totalCarrierBits = 0;
  uint64_t totalStorageBytes = 0;
  uint64_t totalNodes = 0;
  uint64_t totalMaxDepth = 0;
  uint64_t modeledRules = 0;
  uint64_t totalRules = 0;
  llvm::json::Array modules;
  auto root = moduleCost(plan, "/", totalLogicalBits, totalCarrierBits,
                         totalStorageBytes, totalNodes, totalMaxDepth,
                         modeledRules, totalRules);
  if (!root)
    return root.takeError();
  modules.push_back(std::move(*root));
  auto appendInstances = [&](auto &self, const QueueGraphPlan &parent,
                             llvm::StringRef parentPath) -> llvm::Error {
    for (const QueueModuleInstancePlan &instance : parent.moduleInstances) {
      auto found = llvm::find_if(
          parent.moduleSpecializations,
          [&](const std::shared_ptr<QueueGraphPlan> &candidate) {
            return candidate && candidate->definition == instance.definition &&
                   candidate->specializationFingerprint ==
                       instance.specializationFingerprint;
          });
      if (found == parent.moduleSpecializations.end())
        return costError("module instance specialization is unresolved");
      llvm::StringRef relativeScope(instance.scope);
      while (relativeScope.consume_front("/")) {
      }
      while (relativeScope.consume_back("/")) {
      }
      std::string path = parentPath.str();
      if (!path.ends_with('/'))
        path.push_back('/');
      if (!relativeScope.empty())
        path.append(relativeScope.str()).push_back('/');
      path.append(instance.name);
      auto child = moduleCost(**found, path, totalLogicalBits,
                              totalCarrierBits, totalStorageBytes, totalNodes,
                              totalMaxDepth, modeledRules, totalRules);
      if (!child)
        return child.takeError();
      modules.push_back(std::move(*child));
      if (auto error = self(self, **found, path))
        return error;
    }
    return llvm::Error::success();
  };
  if (auto error = appendInstances(appendInstances, plan, "/"))
    return std::move(error);

  llvm::json::Object coverage{
      {"logic_depth_modeled_rules", modeledRules},
      {"rules", totalRules},
      {"runtime_heap_allocations", "not_modeled"},
      {"sizeof_payload_types", "not_modeled"}};
  llvm::json::Object identity{
      {"product_version", sdkProductVersion},
      {"queuegraph_sha256", queueGraphSha256},
      {"source_map_sha256", sourceMapSha256},
      {"specialization",
       plan.specializationFingerprint.empty()
           ? llvm::json::Value(nullptr)
           : llvm::json::Value(plan.specializationFingerprint)},
      {"system", plan.system},
      {"toolchain_revision", sdkSourceRevision}};
  llvm::json::Object models{
      {"logic_depth", "pyc_check_logic_depth_unit_cost"},
      {"runtime", "gfsim_generated_static_v1"}};
  llvm::json::Object totals{
      {"carrier_queue_bits", metric("exact", "verified_queuegraph", "bits",
                                    totalCarrierBits)},
      {"logical_queue_bits", metric("exact", "verified_queuegraph", "bits",
                                    totalLogicalBits)},
      {"logic_depth",
       modeledRules == totalRules
           ? llvm::json::Value(metric("static_upper_bound",
                                      "queuegraph_to_pyc_pre_dce",
                                      "unit_cost_levels", totalMaxDepth))
           : llvm::json::Value(unmodeled(
                 "queuegraph_to_pyc_pre_dce", "unit_cost_levels",
                 "one or more rules use an unmodeled expression graph"))},
      {"queue_storage_packed_bytes",
       metric("static_upper_bound", "generated_gfsim", "bytes",
              totalStorageBytes)},
      {"queuegraph_nodes",
       metric("exact", "verified_queuegraph", "nodes", totalNodes)}};
  llvm::json::Object document{{"contract_epoch", "0.5"},
                              {"coverage", std::move(coverage)},
                              {"identity", std::move(identity)},
                              {"models", std::move(models)},
                              {"modules", std::move(modules)},
                              {"schema", "agentic-circuit-emitted-cost"},
                              {"totals", std::move(totals)},
                              {"version", "1"}};
  return bindings::canonicalizeJson(llvm::json::Value(std::move(document)));
}

} // namespace acir::codegen
