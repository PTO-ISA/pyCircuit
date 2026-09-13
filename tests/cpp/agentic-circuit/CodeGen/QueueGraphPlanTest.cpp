#include "acir/CodeGen/QueueGraphPlan.h"
#include "acir/CodeGen/QueueGraphGenerator.h"
#include "acir/CodeGen/QueueGraphPyc.h"
#include "acir/Transforms/Passes.h"

#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "mlir/Dialect/DLTI/DLTI.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <optional>
#include <system_error>

namespace acir::codegen {
namespace {

bool freezeQueueGraph(mlir::ModuleOp module) {
  mlir::PassManager manager(module.getContext());
  manager.addPass(acir::createFreezeTopologyPass());
  return mlir::succeeded(manager.run(module));
}

void expectCppCompiles(llvm::StringRef source) {
  llvm::SmallString<256> directory;
  ASSERT_FALSE(
      llvm::sys::fs::createUniqueDirectory("acir-queue-graph", directory));
  struct Cleanup {
    llvm::SmallString<256> path;
    ~Cleanup() { llvm::sys::fs::remove_directories(path); }
  } cleanup{directory};

  llvm::SmallString<256> input(directory);
  llvm::sys::path::append(input, "model.cpp");
  std::error_code error;
  llvm::raw_fd_ostream output(input, error);
  ASSERT_FALSE(error);
  output << source;
  output.close();

  llvm::SmallString<256> log(directory);
  llvm::sys::path::append(log, "compile.log");
  const std::array<std::string, 5> ownedArguments = {
      ACIR_TEST_CXX_COMPILER,
      "-std=c++20",
      "-I" ACIR_TEST_SOURCE_DIR "/simulator/gfsim/include",
      "-fsyntax-only",
      input.str().str(),
  };
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {
      std::nullopt, log.str(), log.str()};
  const int status = llvm::sys::ExecuteAndWait(
      ACIR_TEST_CXX_COMPILER, arguments, std::nullopt, redirects);
  auto logBuffer = llvm::MemoryBuffer::getFile(log);
  ASSERT_EQ(status, 0) << (logBuffer ? logBuffer.get()->getBuffer().str()
                                     : std::string{});
}

void expectCppRuns(llvm::StringRef source) {
  llvm::SmallString<256> directory;
  ASSERT_FALSE(
      llvm::sys::fs::createUniqueDirectory("acir-queue-module", directory));
  struct Cleanup {
    llvm::SmallString<256> path;
    ~Cleanup() { llvm::sys::fs::remove_directories(path); }
  } cleanup{directory};

  llvm::SmallString<256> input(directory);
  llvm::sys::path::append(input, "model.cpp");
  std::error_code error;
  llvm::raw_fd_ostream output(input, error);
  ASSERT_FALSE(error);
  output << source;
  output.close();

  llvm::SmallString<256> executable(directory);
  llvm::sys::path::append(executable, "model");
  llvm::SmallString<256> log(directory);
  llvm::sys::path::append(log, "run.log");
  const std::array<std::string, 6> ownedArguments = {
      ACIR_TEST_CXX_COMPILER,
      "-std=c++20",
      "-I" ACIR_TEST_SOURCE_DIR "/simulator/gfsim/include",
      input.str().str(),
      "-o",
      executable.str().str(),
  };
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {
      std::nullopt, log.str(), log.str()};
  int status = llvm::sys::ExecuteAndWait(ACIR_TEST_CXX_COMPILER, arguments,
                                         std::nullopt, redirects);
  auto logBuffer = llvm::MemoryBuffer::getFile(log);
  ASSERT_EQ(status, 0) << (logBuffer ? logBuffer.get()->getBuffer().str()
                                     : std::string{});
  const std::array<llvm::StringRef, 1> runArguments = {executable.str()};
  status = llvm::sys::ExecuteAndWait(executable, runArguments, std::nullopt,
                                     redirects);
  logBuffer = llvm::MemoryBuffer::getFile(log);
  EXPECT_EQ(status, 0) << (logBuffer ? logBuffer.get()->getBuffer().str()
                                     : std::string{});
}

void expectModelBundleRuns(const std::vector<QueueGraphGeneratedFile> &bundle) {
  llvm::SmallString<256> directory;
  ASSERT_FALSE(
      llvm::sys::fs::createUniqueDirectory("acir-model-bundle", directory));
  struct Cleanup {
    llvm::SmallString<256> path;
    ~Cleanup() { llvm::sys::fs::remove_directories(path); }
  } cleanup{directory};

  std::vector<std::string> sources;
  for (const QueueGraphGeneratedFile &file : bundle) {
    llvm::SmallString<256> path(directory);
    llvm::sys::path::append(path, file.relativePath);
    llvm::SmallString<256> parent(path);
    llvm::sys::path::remove_filename(parent);
    ASSERT_FALSE(llvm::sys::fs::create_directories(parent));
    std::error_code error;
    llvm::raw_fd_ostream output(path, error);
    ASSERT_FALSE(error);
    output << file.content;
    output.close();
    if (llvm::StringRef(file.relativePath).ends_with(".cpp"))
      sources.push_back(path.str().str());
  }

  llvm::SmallString<256> harness(directory);
  llvm::sys::path::append(harness, "harness.cpp");
  std::error_code error;
  llvm::raw_fd_ostream output(harness, error);
  ASSERT_FALSE(error);
  output << R"cpp(#include "generated/model.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
int main() {
  const AgenticModelApiV1 *api = agentic_model_query_v1();
  if (!api || api->struct_size != sizeof(*api) || api->abi_version != 1 ||
      std::strcmp(api->sdk_product_version, "6.0.0") != 0)
    return 1;
  AgenticModelV1 *model = nullptr;
  if (api->create(&model) != AGENTIC_MODEL_STATUS_V1_OK || !model)
    return 2;
  const auto bytes = [](std::string_view value) {
    return reinterpret_cast<const std::uint8_t *>(value.data());
  };
  AgenticModelStepResultV1 early{};
  early.struct_size = sizeof(early);
  if (api->step(model, &early) != AGENTIC_MODEL_STATUS_V1_INVALID_STATE)
    return 3;
  AgenticModelBufferV1 error{};
  if (api->last_error(model, &error) != AGENTIC_MODEL_STATUS_V1_OK ||
      error.size == 0)
    return 4;
  if (api->configure_json(model, nullptr, 1) !=
      AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT)
    return 5;
  constexpr std::string_view badConfig = "{ }";
  if (api->configure_json(model, bytes(badConfig), badConfig.size()) !=
      AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT)
    return 6;
  constexpr std::string_view badLimits =
      "{\"deadlock_window\":0,\"max_domain_cycles\":{},\"max_ticks\":null,"
      "\"schema\":\"agentic-model-config\",\"version\":\"1\"}";
  if (api->configure_json(model, bytes(badLimits), badLimits.size()) !=
      AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT)
    return 61;
  constexpr std::string_view config = "{}\n";
  if (api->configure_json(model, bytes(config), config.size()) !=
      AGENTIC_MODEL_STATUS_V1_OK)
    return 7;
  if (api->last_error(model, &error) != AGENTIC_MODEL_STATUS_V1_OK ||
      error.size != 0)
    return 71;
  if (api->configure_json(model, bytes(config), config.size()) !=
      AGENTIC_MODEL_STATUS_V1_INVALID_STATE)
    return 8;
  if (api->reset(model) != AGENTIC_MODEL_STATUS_V1_OK)
    return 12;
  AgenticModelStepResultV1 step{};
  if (api->step(model, &step) != AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT)
    return 13;
  step.struct_size = sizeof(step);
  if (api->step(model, &step) != AGENTIC_MODEL_STATUS_V1_OK)
    return 14;
  const AgenticModelStepStateV1 firstState = step.state;
  if (api->reset(model) != AGENTIC_MODEL_STATUS_V1_OK)
    return 15;
  step = {};
  step.struct_size = sizeof(step);
  if (api->step(model, &step) != AGENTIC_MODEL_STATUS_V1_OK)
    return 16;
  if (step.state != firstState)
    return 17;
  AgenticModelBufferV1 buffer{};
  if (api->statistics_json(model, &buffer) != AGENTIC_MODEL_STATUS_V1_OK)
    return 18;
  const std::string firstStatistics(
      reinterpret_cast<const char *>(buffer.data), buffer.size);
  if (firstStatistics.size() < 3 || firstStatistics.front() != '[' ||
      !firstStatistics.ends_with("]\n") ||
      api->statistics_json(model, &buffer) != AGENTIC_MODEL_STATUS_V1_OK ||
      std::string(reinterpret_cast<const char *>(buffer.data), buffer.size) !=
          firstStatistics)
    return 181;
  api->destroy(model);

  model = nullptr;
  constexpr std::string_view fullConfig =
      "{\"deadlock_window\":8,\"max_domain_cycles\":{\"cycle\":64},"
      "\"max_ticks\":1024,\"schema\":\"agentic-model-config\","
      "\"version\":\"1\"}\n";
  if (api->create(&model) != AGENTIC_MODEL_STATUS_V1_OK || !model ||
      api->configure_json(model, bytes(fullConfig), fullConfig.size()) !=
          AGENTIC_MODEL_STATUS_V1_OK)
    return 19;
  if (api->reset(model) != AGENTIC_MODEL_STATUS_V1_OK)
    return 20;
  step = {};
  step.struct_size = sizeof(step);
  if (api->step(model, &step) != AGENTIC_MODEL_STATUS_V1_OK)
    return 21;
  api->destroy(model);
  return 0;
}
)cpp";
  output.close();

  llvm::SmallString<256> executable(directory);
  llvm::sys::path::append(executable, "model");
  llvm::SmallString<256> log(directory);
  llvm::sys::path::append(log, "run.log");
  llvm::SmallString<256> include(directory);
  llvm::sys::path::append(include, "include");
  std::vector<std::string> ownedArguments = {
      ACIR_TEST_CXX_COMPILER,
      "-std=c++20",
      "-fvisibility=hidden",
      "-I" ACIR_TEST_SOURCE_DIR "/simulator/gfsim/include",
      "-I" + include.str().str(),
  };
  ownedArguments.insert(ownedArguments.end(), sources.begin(), sources.end());
  ownedArguments.push_back(harness.str().str());
  ownedArguments.push_back(ACIR_TEST_BINARY_DIR "/gfsim/libgfsim.a");
  ownedArguments.push_back("-o");
  ownedArguments.push_back(executable.str().str());
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {
      std::nullopt, log.str(), log.str()};
  int status = llvm::sys::ExecuteAndWait(ACIR_TEST_CXX_COMPILER, arguments,
                                         std::nullopt, redirects);
  auto logBuffer = llvm::MemoryBuffer::getFile(log);
  ASSERT_EQ(status, 0) << (logBuffer ? logBuffer.get()->getBuffer().str()
                                     : std::string{});
  const std::array<llvm::StringRef, 1> runArguments = {executable.str()};
  status = llvm::sys::ExecuteAndWait(executable, runArguments, std::nullopt,
                                     redirects);
  logBuffer = llvm::MemoryBuffer::getFile(log);
  EXPECT_EQ(status, 0) << (logBuffer ? logBuffer.get()->getBuffer().str()
                                     : std::string{});
}

constexpr llvm::StringLiteral kQueueGraph = R"mlir(
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "pipeline"} {
  %input = ac.source depth 4 latency 1 {ac.name = "input"} : !ac.queue<i64>
  %left, %right = ac.route %input depths [2, 2] latencies [1, 1] {
  ^selector(%item: !ac.var<i64>):
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    %selected = ac.var.cmp "eq" %item, %zero : !ac.var<i64> -> !ac.var<i1>
    ac.route.yield %selected : !ac.var<i1>
  } {ac.output_names = ["left", "right"]} : !ac.queue<i64> -> (!ac.queue<i64>, !ac.queue<i64>)
  %merged = ac.merge %left, %right policy "round_robin" depth 3 latency 1 {ac.name = "merged"} : (!ac.queue<i64>, !ac.queue<i64>) -> !ac.queue<i64>
  ac.sink %merged {ac.name = "sink_0"} : !ac.queue<i64>
}
)mlir";

constexpr llvm::StringLiteral kStructuredTransform = R"mlir(
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "structured"} {
  ac.type_scope @types {
    ac.struct @Item fields [{name = "value", type = i64}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Item> = {abi_alignment = 8 : i64, endianness = "little", preferred_alignment = 8 : i64, size = 8 : i64}>}
  %input = ac.source depth 2 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Item>>
  %output = ac.transform %input depths [2] latencies [1] {
  ^body(%item: !ac.var<!ac.struct<@types::@Item>>):
    %value = ac.var.get %item field "value" : !ac.var<!ac.struct<@types::@Item>> -> !ac.var<i64>
    %one = ac.var.constant 1 : i64 as !ac.var<i64>
    %sum = ac.var.add %value, %one : !ac.var<i64>
    %updated = ac.var.with %item, %sum field "value" : !ac.var<!ac.struct<@types::@Item>>, !ac.var<i64> -> !ac.var<!ac.struct<@types::@Item>>
    ac.transform.yield %updated : !ac.var<!ac.struct<@types::@Item>>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Item>>) -> !ac.queue<!ac.struct<@types::@Item>>
  ac.sink %output {ac.name = "sink_0"} : !ac.queue<!ac.struct<@types::@Item>>
}
)mlir";

constexpr llvm::StringLiteral kMultipleConsumers = R"mlir(
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "bad"} {
  %input = ac.source depth 2 latency 1 {ac.name = "input"} : !ac.queue<i64>
  ac.sink %input {ac.name = "left"} : !ac.queue<i64>
  ac.sink %input {ac.name = "right"} : !ac.queue<i64>
}
)mlir";

constexpr llvm::StringLiteral kObservationUse = R"mlir(
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "observed"} {
  %input = ac.source depth 2 latency 1 {ac.name = "input"} : !ac.queue<i64>
  ac.observe %input name "head" : !ac.queue<i64>
  ac.sink %input {ac.name = "sink_0"} : !ac.queue<i64>
}
)mlir";

constexpr llvm::StringLiteral kStatefulFiring = R"mlir(
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "stateful"} {
  ac.table @table entry i8 entries 2 init 0 owner "/" stable_id "table/table"
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.firing %input depths [1] latencies [1]
      stable_id "install" domain "cycle" {
  ^body(%item: !ac.var<i8>):
    %index = ac.var.constant 1 : i2 as !ac.var<i2>
    %enabled = ac.var.constant true as !ac.var<i1>
    ac.firing.condition %enabled : !ac.var<i1>
    ac.table.propose @table [%index] = %item when %enabled : !ac.var<i1>
        mode "replace"
        write_fields ["$entry"] : !ac.var<i2>, !ac.var<i8>
    ac.firing.output %item when %enabled ordinal 0 : !ac.var<i8>, !ac.var<i1>
    ac.firing.yield %item : !ac.var<i8>
  } {ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @table}], ac.arbitration_membership = [], ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}], ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, resource = @table}], ac.guard_kind = #ac<rule_guard_kind always>, ac.initially_active = false, ac.name = "output", ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}], ac.rule_definition = "install", ac.rule_footprints = [{access = "replace", fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @table}], ac.rule_priority = 0 : i64, ac.schedule_kind = #ac<rule_schedule_kind lexical_priority>, ac.state_accesses = [{fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind replace>, resource = @table}], ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @table}]} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}
)mlir";

QueueGraphPlan sharedReferencePlan() {
  QueueGraphPlan plan;
  plan.system = "shared_reference";
  plan.queues = {{"input", "i8", "/", 1, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
  QueueBlockPlan slot{"slot", "release", "/", {"input"}, {}};
  slot.slot = "pending";
  slot.expressions = {
      {"m", "table_match_ref", "i4", {}, "match", "", "", "issue"},
      {"i",
       "table_selection_index_ref",
       "i2",
       {},
       "selection",
       "",
       "",
       "issue"},
      {"v",
       "table_selection_valid_ref",
       "i1",
       {},
       "selection",
       "",
       "",
       "issue"},
  };
  plan.blocks.push_back(std::move(slot));
  plan.tables = {{"issue", "i8", 4, 0, "table-id", "/"}};
  plan.tableMatches = {{"match", "issue", "/", "i4", {}, "predicate"}};
  plan.tableSelections = {
      {"selection", "issue", "/", "match", "first", "i2", {}, ""}};
  plan.tableReads = {{"issue", "read", "/", "", "unused", 1, 1}};
  plan.slots = {{"pending", "i8", "input", "/", "slot-id", "/"}};
  return plan;
}

QueueGraphPlan inlineFirstChoicePlan(unsigned width, unsigned indexWidth) {
  const std::string maskType = "i" + std::to_string(width);
  const std::string indexType = "i" + std::to_string(indexWidth);
  const std::string payloadName = "Choice" + std::to_string(width);
  const std::string payloadType = "!ac.struct<@types::@" + payloadName + ">";

  QueueGraphPlan plan;
  plan.system = "first_choice_" + std::to_string(width);
  plan.payloads = {
      {payloadName, {{"index", indexType, indexWidth}, {"valid", "i1", 1}}}};
  plan.tables = {{"entries", "i1", width, 0, "table/entries", "/"}};
  plan.tableReads = {{"entries", "read", "/", "", "unused", 1, 1}};
  plan.queues = {{"input", maskType, "/", 1, 1},
                 {"output", payloadType, "/", 1, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
  QueueBlockPlan transform{"firing",   "choose", "/", {"input"},
                           {"output"}, {1},      {1}};
  QueueExpressionPlan mask{"mask", "table_match", maskType, {}};
  mask.table = "entries";
  mask.nestedExpressions = {{"matched", "constant", "i1", {}, "", "", "true"}};
  mask.nestedYields = {"matched"};
  transform.expressions.push_back(std::move(mask));
  QueueExpressionPlan index{
      "selected_index", "table_choose_index", indexType, {"mask"}};
  index.table = "entries";
  index.predicate = "first";
  transform.expressions.push_back(index);
  QueueExpressionPlan valid{
      "selected_valid", "table_choose_valid", "i1", {"mask"}};
  valid.table = "entries";
  valid.predicate = "first";
  transform.expressions.push_back(valid);
  QueueExpressionPlan result{"result",
                             "record_create",
                             payloadType,
                             {"selected_index", "selected_valid"}};
  result.width = indexWidth + 1;
  transform.expressions.push_back(std::move(result));
  transform.yields = {"result"};
  transform.guard = "selected_valid";
  transform.outputPresence = {{0, "result", "selected_valid"}};
  plan.blocks.push_back(std::move(transform));
  plan.blocks.push_back({"sink", "sink", "/", {"output"}, {}});
  return plan;
}

QueueGraphPlan dualInlineMatchPlan() {
  QueueGraphPlan plan = inlineFirstChoicePlan(4, 2);
  plan.system = "dual_inline_match";
  plan.queues[0].payloadType = "i1";
  QueueBlockPlan &firing = plan.blocks[1];
  QueueExpressionPlan &firstMask = firing.expressions[0];
  firstMask.operands = {"item"};
  firstMask.nestedExpressions = {
      {"same", "cmp", "i1", {"entry", "item"}, "", "eq"},
      {"condition", "constant", "i1", {}, "", "", "true"},
      {"matched", "and", "i1", {"same", "condition"}},
  };
  firstMask.nestedYields = {"matched"};

  QueueExpressionPlan secondMask{"second_mask", "table_match", "i4", {"item"}};
  secondMask.table = "entries";
  secondMask.nestedExpressions = {
      {"same", "cmp", "i1", {"entry", "item"}, "", "eq"},
      {"condition", "constant", "i1", {}, "", "", "false"},
      {"matched", "and", "i1", {"same", "condition"}},
  };
  secondMask.nestedYields = {"matched"};
  QueueExpressionPlan secondIndex{
      "second_index", "table_choose_index", "i2", {"second_mask"}};
  secondIndex.table = "entries";
  secondIndex.predicate = "first";
  QueueExpressionPlan secondValid{
      "second_valid", "table_choose_valid", "i1", {"second_mask"}};
  secondValid.table = "entries";
  secondValid.predicate = "first";
  firing.expressions.insert(
      firing.expressions.end() - 1,
      {std::move(secondMask), std::move(secondIndex), std::move(secondValid)});
  QueueExpressionPlan &result = firing.expressions.back();
  result.operands.push_back("second_index");
  result.operands.push_back("second_valid");
  result.width = 6;
  plan.payloads[0].fields.push_back({"second_index", "i2", 2});
  plan.payloads[0].fields.push_back({"second_valid", "i1", 1});
  return plan;
}

QueueGraphPlan aggregateTableBorrowPlan(QueueGraphPlan plan) {
  constexpr llvm::StringLiteral nestedType =
      "!ac.struct<@types::@BorrowNested>";
  constexpr llvm::StringLiteral entryType = "!ac.struct<@types::@BorrowEntry>";
  plan.system = "aggregate_borrow";
  plan.payloads = {
      {"BorrowNested",
       {{"key", "i16", 16}, {"lane0", "i64", 64}, {"lane1", "i64", 64}}},
      {"BorrowEntry",
       {{"valid", "i1", 1},
        {"nested", nestedType.str(), 144},
        {"echo", "i16", 16}}},
  };
  plan.tables.front().entryType = entryType.str();
  for (QueuePlan &queue : plan.queues)
    queue.payloadType = entryType.str();
  QueueBlockPlan &firing =
      *llvm::find_if(plan.blocks, [](const QueueBlockPlan &block) {
        return block.kind == "firing";
      });
  firing.expressions = {
      {"index", "constant", "i1", {}, "", "", "1 : i1"},
      {"stored", "table_get", entryType.str(), {"index"}, "", "", "", "table"},
      {"nested", "get", nestedType.str(), {"stored"}, "nested"},
      {"key", "get", "i16", {"nested"}, "key"},
      {"updated", "with", entryType.str(), {"stored", "key"}, "echo"},
      {"enabled", "constant", "i1", {}, "", "", "true"},
  };
  firing.yields = {"updated"};
  firing.guard = "enabled";
  firing.stateWrites = {{"table",
                         "index",
                         "item",
                         "enabled",
                         "replace",
                         {"valid", "nested", "echo"}}};
  firing.outputPresence = {{0, "updated", "enabled"}};
  return plan;
}

QueueGraphPlan aggregateMetadataPlan() {
  QueueGraphPlan plan = sharedReferencePlan();
  plan.payloads = {{"Packet",
                    {{"pair", "tuple<i3, i5>", 8},
                     {"lanes", "!ac.value_array<4 x i4>", 16}}}};
  plan.aggregates = {
      {"tuple<i3, i5>", "tuple", {"i3", "i5"}, 2, 8},
      {"!ac.value_array<4 x i4>", "array", {"i4"}, 4, 16},
  };
  return plan;
}

QueueGraphPlan statelessOptionalMultiOutputPlan() {
  QueueGraphPlan plan;
  plan.system = "stateless_optional_multi_output";
  plan.queues = {{"input", "i8", "/", 1, 1},
                 {"narrow", "i8", "/", 1, 1},
                 {"wide", "i16", "/", 1, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
  QueueBlockPlan firing{"firing",           "publish", "/",   {"input"},
                        {"narrow", "wide"}, {1, 1},    {1, 1}};
  firing.guard = "enabled";
  firing.expressions = {
      {"wide_value", "constant", "i16", {}, "", "", "42 : i16"},
      {"disabled", "constant", "i1", {}, "", "", "false"},
      {"enabled", "constant", "i1", {}, "", "", "true"},
  };
  firing.yields = {"item", "wide_value"};
  firing.outputPresence = {{0, "item", "disabled"},
                           {1, "wide_value", "enabled"}};
  plan.blocks.push_back(std::move(firing));
  plan.blocks.push_back({"sink", "narrow_sink", "/", {"narrow"}, {}});
  plan.blocks.push_back({"sink", "wide_sink", "/", {"wide"}, {}});

  using Kind = QueueActivationNodeKind;
  const QueueActivationNodePlan input{Kind::Queue, 0};
  const QueueActivationNodePlan narrow{Kind::Queue, 1};
  const QueueActivationNodePlan wide{Kind::Queue, 2};
  const QueueActivationNodePlan worker{Kind::Block, 1};
  const QueueActivationNodePlan narrowSink{Kind::Block, 2};
  const QueueActivationNodePlan wideSink{Kind::Block, 3};
  plan.activationEdges = {{input, worker},
                          {narrow, worker},
                          {narrow, narrowSink},
                          {wide, worker},
                          {wide, wideSink}};
  plan.workClosureEdges = {{worker, input},
                           {worker, narrow},
                           {worker, wide},
                           {narrowSink, narrow},
                           {wideSink, wide}};
  return plan;
}

QueueGraphPlan eightOutputCommitGroupPlan() {
  QueueGraphPlan plan = statelessOptionalMultiOutputPlan();
  plan.system = "seven_optional_plus_ack";
  plan.blocks.resize(2);
  QueueBlockPlan &firing = plan.blocks[1];
  firing.outputPresence[0].present = "disabled";
  firing.outputPresence[1].present = "disabled";
  for (uint64_t ordinal = 2; ordinal < 8; ++ordinal) {
    const std::string type = "i" + std::to_string(8 + ordinal);
    const std::string queue = "effect" + std::to_string(ordinal);
    const std::string value = "effect_value" + std::to_string(ordinal);
    plan.queues.push_back({queue, type, "/", 1, 1});
    firing.outputs.push_back(queue);
    firing.depths.push_back(1);
    firing.latencies.push_back(1);
    firing.expressions.push_back(
        {value, "constant", type, {}, "", "", "0 : " + type});
    firing.yields.push_back(value);
    firing.outputPresence.push_back(
        {ordinal, value, ordinal == 7 ? "enabled" : "disabled"});
  }
  const std::vector<std::string> outputs = firing.outputs;
  for (uint64_t ordinal = 0; ordinal < 8; ++ordinal)
    plan.blocks.push_back({"sink",
                           "sink" + std::to_string(ordinal),
                           "/",
                           {outputs[ordinal]},
                           {}});

  using Kind = QueueActivationNodeKind;
  const QueueActivationNodePlan worker{Kind::Block, 1};
  plan.activationEdges.clear();
  plan.workClosureEdges.clear();
  plan.activationEdges.push_back({{Kind::Queue, 0}, worker});
  plan.workClosureEdges.push_back({worker, {Kind::Queue, 0}});
  for (uint64_t ordinal = 0; ordinal < 8; ++ordinal) {
    const QueueActivationNodePlan queue{Kind::Queue, ordinal + 1};
    const QueueActivationNodePlan sink{Kind::Block, ordinal + 2};
    plan.activationEdges.push_back({queue, worker});
    plan.activationEdges.push_back({queue, sink});
    plan.workClosureEdges.push_back({worker, queue});
  }
  for (uint64_t ordinal = 0; ordinal < 8; ++ordinal)
    plan.workClosureEdges.push_back(
        {{Kind::Block, ordinal + 2}, {Kind::Queue, ordinal + 1}});
  return plan;
}

QueueGraphPlan aggregateExpressionPlan() {
  QueueGraphPlan plan;
  plan.system = "aggregate_expression";
  plan.aggregates = {
      {"tuple<i3, i5>", "tuple", {"i3", "i5"}, 2, 8},
  };
  plan.queues = {{"input", "tuple<i3, i5>", "/", 1, 1},
                 {"output", "tuple<i3, i5>", "/", 1, 1}};

  QueueBlockPlan source;
  source.kind = "source";
  source.name = "input";
  source.scope = "/";
  source.outputs = {"input"};
  source.depths = {1};
  source.latencies = {1};
  plan.blocks.push_back(std::move(source));

  QueueBlockPlan transform;
  transform.kind = "transform";
  transform.name = "output";
  transform.scope = "/";
  transform.inputs = {"input"};
  transform.outputs = {"output"};
  transform.depths = {1};
  transform.latencies = {1};
  QueueExpressionPlan first;
  first.result = "first";
  first.kind = "aggregate_get";
  first.type = "i3";
  first.operands = {"item"};
  first.lsb = 5;
  first.width = 3;
  transform.expressions.push_back(std::move(first));
  QueueExpressionPlan second;
  second.result = "second";
  second.kind = "aggregate_get";
  second.type = "i5";
  second.operands = {"item"};
  second.lsb = 0;
  second.width = 5;
  transform.expressions.push_back(std::move(second));
  QueueExpressionPlan packed;
  packed.result = "packed";
  packed.kind = "tuple_create";
  packed.type = "tuple<i3, i5>";
  packed.operands = {"first", "second"};
  packed.width = 8;
  transform.expressions.push_back(std::move(packed));
  transform.yields = {"packed"};
  plan.blocks.push_back(std::move(transform));

  QueueBlockPlan sink;
  sink.kind = "sink";
  sink.name = "sink";
  sink.scope = "/";
  sink.inputs = {"output"};
  plan.blocks.push_back(std::move(sink));
  return plan;
}

TEST(QueueGraphPlanTest, ExtractsFrozenQueueIdentitiesAndTopology) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceString<mlir::ModuleOp>(kQueueGraph, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  EXPECT_EQ(plan->system, "pipeline");
  ASSERT_EQ(plan->queues.size(), 4u);
  EXPECT_EQ(plan->queues[0].name, "input");
  EXPECT_EQ(plan->queues[1].name, "left");
  EXPECT_EQ(plan->queues[2].name, "right");
  EXPECT_EQ(plan->queues[3].name, "merged");
  ASSERT_EQ(plan->blocks.size(), 4u);
  EXPECT_EQ(plan->blocks[0].kind, "source");
  EXPECT_EQ(plan->blocks[1].kind, "route");
  EXPECT_EQ(plan->blocks[2].kind, "merge");
  EXPECT_EQ(plan->blocks[3].kind, "sink");
  EXPECT_EQ(plan->blocks[2].policy, "round_robin");
}

TEST(QueueGraphPlanTest, RejectsRawUnfrozenQueueGraph) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceString<mlir::ModuleOp>(kQueueGraph, &context);
  ASSERT_TRUE(module);
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_FALSE(bool(plan));
  EXPECT_NE(llvm::toString(plan.takeError())
                .find("QueueGraph requires verified epoch 0.5 topology freeze"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, CanonicalJsonIsByteIdenticalAndClosed) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceString<mlir::ModuleOp>(kQueueGraph, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  auto first = plan->canonicalJson();
  ASSERT_TRUE(bool(first)) << llvm::toString(first.takeError());
  auto second = plan->canonicalJson();
  ASSERT_TRUE(bool(second)) << llvm::toString(second.takeError());
  EXPECT_EQ(*first, *second);
  EXPECT_NE(first->find("\"contract_epoch\":\"0.5\""), std::string::npos);
  EXPECT_NE(first->find("\"schema\":\"agentic-circuit-queue-graph-plan\""),
            std::string::npos);
  EXPECT_NE(first->find("\"version\":\"0.5\""), std::string::npos);
  EXPECT_NE(first->find("\"name\":\"merged\""), std::string::npos);
}

TEST(QueueGraphPlanTest, PreservesJitSpecializationIdentity) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  constexpr llvm::StringLiteral fingerprint =
      "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  std::string specialized = kQueueGraph.str();
  size_t system = specialized.find("ac.system = \"pipeline\"");
  ASSERT_NE(system, std::string::npos);
  specialized.insert(system,
                     ("ac.specialization = \"" + fingerprint + "\", ").str());
  auto module = mlir::parseSourceString<mlir::ModuleOp>(specialized, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  EXPECT_EQ(plan->specializationFingerprint, fingerprint);
  auto json = plan->canonicalJson();
  ASSERT_TRUE(bool(json)) << llvm::toString(json.takeError());
  EXPECT_NE(json->find(fingerprint), std::string::npos);
  auto cpp = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find(("// Specialization: " + fingerprint).str()),
            std::string::npos);

  specialized.replace(specialized.find(fingerprint), fingerprint.size(),
                      "sha256:bad");
  module = mlir::parseSourceString<mlir::ModuleOp>(specialized, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  plan = buildQueueGraphPlan(*module);
  ASSERT_FALSE(bool(plan));
  EXPECT_NE(llvm::toString(plan.takeError()).find("fingerprint is invalid"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsRecursiveNestedPayloadDefinitions) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStructuredTransform, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  plan->payloads = {
      QueuePayloadPlan{"Left", {{"right", "!ac.struct<@types::@Right>"}}},
      QueuePayloadPlan{"Right", {{"left", "!ac.struct<@types::@Left>"}}},
  };

  auto error = verifyQueueGraphPlan(*plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("contain a cycle"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, BackendsRejectRecursivePayloadsWithoutRecursing) {
  QueueGraphPlan plan;
  plan.system = "recursive";
  plan.payloads = {
      QueuePayloadPlan{"Left", {{"right", "!ac.struct<@types::@Right>", 1}}},
      QueuePayloadPlan{"Right", {{"left", "!ac.struct<@types::@Left>", 1}}},
  };
  plan.queues = {{"input", "!ac.struct<@types::@Left>", "/", 1, 1}};
  plan.blocks = {{"source", "input", "/", {}, {"input"}, {1}, {1}},
                 {"sink", "sink", "/", {"input"}, {}}};

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_FALSE(bool(cpp));
  EXPECT_NE(llvm::toString(cpp.takeError()).find("contain a cycle"),
            std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_FALSE(bool(pyc));
  EXPECT_NE(llvm::toString(pyc.takeError()).find("contain a cycle"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsMalformedNominalEnumMetadata) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStructuredTransform, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  plan->enums = {QueueEnumPlan{"Mode", {"IDLE", "RUN", "WAIT"}, {}, 3}};

  auto widthError = verifyQueueGraphPlan(*plan);
  ASSERT_TRUE(bool(widthError));
  EXPECT_NE(llvm::toString(std::move(widthError)).find("width is inconsistent"),
            std::string::npos);

  plan->enums.front().width = 2;
  plan->enums.front().enumerants = {"IDLE", "IDLE"};
  auto memberError = verifyQueueGraphPlan(*plan);
  ASSERT_TRUE(bool(memberError));
  EXPECT_NE(llvm::toString(std::move(memberError)).find("must be non-empty"),
            std::string::npos);

  plan->enums = {QueueEnumPlan{"Mode", {"IDLE", "RUN", "WAIT"}, {0, 3}, 4}};
  auto shapeError = verifyQueueGraphPlan(*plan);
  ASSERT_TRUE(bool(shapeError));
  EXPECT_NE(llvm::toString(std::move(shapeError)).find("shape is inconsistent"),
            std::string::npos);

  plan->enums.front().values = {0, 3, 9};
  plan->enums.front().width = 3;
  auto valueError = verifyQueueGraphPlan(*plan);
  ASSERT_TRUE(bool(valueError));
  EXPECT_NE(
      llvm::toString(std::move(valueError)).find("values are inconsistent"),
      std::string::npos);
}

TEST(QueueGraphPlanTest, RecomputesStaticTypeMetadataAndRejectsForgery) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStructuredTransform, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());

  plan->staticTypeBindings = {{"WIDTH", 64}};
  plan->staticTypeChecks = {
      QueueStaticTypeCheckPlan{"Item.value:bits", {"param:WIDTH"}, 64}};
  EXPECT_FALSE(bool(verifyQueueGraphPlan(*plan)));
  auto json = plan->canonicalJson();
  ASSERT_TRUE(bool(json)) << llvm::toString(json.takeError());
  EXPECT_NE(json->find("static_type_bindings"), std::string::npos);

  plan->staticTypeChecks.front().result = 63;
  auto error = verifyQueueGraphPlan(*plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("result is inconsistent"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RecomputesAggregateStaticTypeMetadata) {
  QueueGraphPlan plan = aggregateMetadataPlan();
  plan.staticTypeBindings = {{"PAIR_WIDTH", 5}, {"LANES", 4}};
  plan.staticTypeChecks = {
      {"Packet.pair.tuple_1:bits", {"param:PAIR_WIDTH"}, 5},
      {"Packet.lanes:array_length", {"param:LANES"}, 4},
  };
  auto verification = verifyQueueGraphPlan(plan);
  EXPECT_FALSE(bool(verification)) << llvm::toString(std::move(verification));

  plan.staticTypeBindings.push_back({"UNUSED", 1});
  auto unused = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(unused));
  EXPECT_NE(llvm::toString(std::move(unused)).find("not referenced"),
            std::string::npos);
  plan.staticTypeBindings.pop_back();

  plan.staticTypeChecks.push_back(plan.staticTypeChecks.front());
  auto duplicate = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(duplicate));
  EXPECT_NE(llvm::toString(std::move(duplicate)).find("must be complete"),
            std::string::npos);
  plan.staticTypeChecks.pop_back();

  plan.staticTypeBindings[0].second = 4;
  plan.staticTypeChecks[0].result = 4;
  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("bits width is inconsistent"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, VerifiesScalarAndNominalStaticTypeIdentity) {
  QueueGraphPlan scalar = aggregateMetadataPlan();
  scalar.staticTypeBindings = {{"WIDTH", 8}};
  scalar.staticTypeChecks = {
      {"interface.system.shared_reference.input.input:bits",
       {"param:WIDTH"},
       8,
       "i8"}};
  auto scalarError = verifyQueueGraphPlan(scalar);
  EXPECT_FALSE(bool(scalarError)) << llvm::toString(std::move(scalarError));
  scalar.queues.front().payloadType = "i7";
  auto forgedScalar = verifyQueueGraphPlan(scalar);
  ASSERT_TRUE(bool(forgedScalar));
  EXPECT_NE(llvm::toString(std::move(forgedScalar)).find("actual endpoint"),
            std::string::npos);

  QueueGraphPlan nominal = aggregateMetadataPlan();
  nominal.payloads = {
      {"Entry__p435bcab52046", {{"a", "i4", 4}, {"b", "i4", 4}}}};
  nominal.staticTypeBindings = {{"N", 4}};
  nominal.staticTypeChecks = {
      {"Entry__p435bcab52046.a:bits", {"param:N"}, 4},
      {"Entry__p435bcab52046.b:bits", {"param:N"}, 4},
  };
  nominal.staticTypeIdentities = {{
      "Entry",
      "Entry__p435bcab52046",
      "sha256:435bcab5204689d904ed5c8b819738da32e8f7399aa0d0b2c027b7da04417318",
      {{"N", "N", 4}},
      {"Entry__p435bcab52046.a:bits", "Entry__p435bcab52046.b:bits"},
  }};
  auto nominalError = verifyQueueGraphPlan(nominal);
  EXPECT_FALSE(bool(nominalError)) << llvm::toString(std::move(nominalError));

  nominal.staticTypeChecks.pop_back();
  auto missing = verifyQueueGraphPlan(nominal);
  ASSERT_TRUE(bool(missing));
  EXPECT_NE(llvm::toString(std::move(missing)).find("targets are inconsistent"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, SerializesExplicitEnumValuesAsUnsignedHex) {
  QueueGraphPlan plan = aggregateMetadataPlan();
  plan.enums = {QueueEnumPlan{"Wide",
                              {"LOW", "HIGH", "MAX"},
                              {uint64_t{0x7fff'ffff'ffff'ffff},
                               uint64_t{0x8000'0000'0000'0000},
                               uint64_t{0xffff'ffff'ffff'ffff}},
                              64}};
  auto verification = verifyQueueGraphPlan(plan);
  EXPECT_FALSE(bool(verification)) << llvm::toString(std::move(verification));

  auto json = plan.canonicalJson();
  ASSERT_TRUE(bool(json)) << llvm::toString(json.takeError());
  EXPECT_NE(json->find("0x7fffffffffffffff"), std::string::npos);
  EXPECT_NE(json->find("0x8000000000000000"), std::string::npos);
  EXPECT_NE(json->find("0xffffffffffffffff"), std::string::npos);
  EXPECT_NE(json->find("unsigned_hex"), std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsTupleAggregateWidthMismatch) {
  QueueGraphPlan plan = aggregateMetadataPlan();
  plan.aggregates[0].width = 9;

  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("tuple aggregate width"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsMalformedValueArrayMetadata) {
  QueueGraphPlan malformedElements = aggregateMetadataPlan();
  malformedElements.aggregates[1].elements.push_back("i4");
  auto elementError = verifyQueueGraphPlan(malformedElements);
  ASSERT_TRUE(bool(elementError));
  EXPECT_NE(
      llvm::toString(std::move(elementError)).find("value-array metadata"),
      std::string::npos);

  QueueGraphPlan malformedLength = aggregateMetadataPlan();
  malformedLength.aggregates[1].length = 0;
  auto lengthError = verifyQueueGraphPlan(malformedLength);
  ASSERT_TRUE(bool(lengthError));
  EXPECT_NE(llvm::toString(std::move(lengthError)).find("value-array metadata"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, AcceptsAggregateMetadataWiderThanSixtyFourBits) {
  QueueGraphPlan plan = aggregateExpressionPlan();
  plan.aggregates[0] = {"tuple<i64, i64>", "tuple", {"i64", "i64"}, 2, 128};
  for (QueuePlan &queue : plan.queues)
    queue.payloadType = "tuple<i64, i64>";
  QueueBlockPlan &transform = plan.blocks[1];
  transform.expressions[0].type = "i64";
  transform.expressions[0].lsb = 64;
  transform.expressions[0].width = 64;
  transform.expressions[1].type = "i64";
  transform.expressions[1].width = 64;
  transform.expressions[2].type = "tuple<i64, i64>";
  transform.expressions[2].width = 128;

  auto error = verifyQueueGraphPlan(plan);
  EXPECT_FALSE(bool(error)) << llvm::toString(std::move(error));
}

TEST(QueueGraphPlanTest, RejectsAggregateMetadataOutsideBackendStorageWidth) {
  QueueGraphPlan plan = aggregateMetadataPlan();
  plan.aggregates[0].width = kMaximumPackedValueWidth + 1;

  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("aggregate type metadata"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, AcceptsExactMaximumBackendStorageWidth) {
  QueueGraphPlan plan;
  plan.system = "maximum_width";
  plan.payloads = {
      {"Packet",
       {{"bits", "!ac.value_array<65536 x i1>", kMaximumPackedValueWidth}}}};
  plan.aggregates = {{"!ac.value_array<65536 x i1>",
                      "array",
                      {"i1"},
                      kMaximumPackedValueWidth,
                      kMaximumPackedValueWidth}};
  plan.queues = {{"input", "!ac.struct<@types::@Packet>", "/", 1, 1}};
  plan.blocks = {{"source", "input", "/", {}, {"input"}, {1}, {1}},
                 {"sink", "sink", "/", {"input"}, {}}};

  auto verification = verifyQueueGraphPlan(plan);
  EXPECT_FALSE(bool(verification)) << llvm::toString(std::move(verification));
}

TEST(QueueGraphPlanTest, RejectsRecursivePayloadWidthOutsideBackendDomain) {
  constexpr uint64_t maximum = kMaximumPackedValueWidth;
  QueueGraphPlan plan;
  plan.system = "overflow";
  plan.payloads = {
      {"Packet",
       {{"huge", "!ac.value_array<65536 x i1>", maximum}, {"extra", "i1", 1}}}};
  plan.aggregates = {
      {"!ac.value_array<65536 x i1>", "array", {"i1"}, maximum, maximum}};
  plan.queues = {{"input", "!ac.struct<@types::@Packet>", "/", 1, 1}};
  plan.blocks = {{"source", "input", "/", {}, {"input"}, {1}, {1}},
                 {"sink", "sink", "/", {"input"}, {}}};

  auto verification = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(verification));
  EXPECT_NE(llvm::toString(std::move(verification))
                .find("payload width exceeds the backend template domain"),
            std::string::npos);

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_FALSE(bool(cpp));
  EXPECT_NE(llvm::toString(cpp.takeError())
                .find("payload width exceeds the backend template domain"),
            std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_FALSE(bool(pyc));
  EXPECT_NE(llvm::toString(pyc.takeError())
                .find("payload width exceeds the backend template domain"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsAggregateBitWidthArithmeticOverflow) {
  QueueGraphPlan plan = aggregateMetadataPlan();
  plan.payloads[0].fields[1].type = "!ac.value_array<huge x i8>";
  plan.payloads[0].fields[1].width = 8;
  plan.aggregates[1] = {"!ac.value_array<huge x i8>",
                        "array",
                        {"i8"},
                        (uint64_t{1} << 61) + 1,
                        8};

  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("overflows uint64_t"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsPayloadFieldWidthMismatch) {
  QueueGraphPlan plan = aggregateMetadataPlan();
  plan.payloads[0].fields[0].width = 7;

  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("payload field width"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsAggregateCreateOperandTypeOrArityForgery) {
  QueueGraphPlan swapped = aggregateExpressionPlan();
  swapped.blocks[1].expressions[2].operands = {"second", "first"};
  auto typeError = verifyQueueGraphPlan(swapped);
  ASSERT_TRUE(bool(typeError));
  EXPECT_NE(llvm::toString(std::move(typeError)).find("operand type"),
            std::string::npos);

  QueueGraphPlan shortTuple = aggregateExpressionPlan();
  shortTuple.blocks[1].expressions[2].operands.pop_back();
  auto arityError = verifyQueueGraphPlan(shortTuple);
  ASSERT_TRUE(bool(arityError));
  EXPECT_NE(llvm::toString(std::move(arityError)).find("operand arity"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsAggregateGetCrossElementSliceForgery) {
  QueueGraphPlan plan = aggregateExpressionPlan();
  plan.blocks[1].expressions[0].lsb = 4;

  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("exact declared element"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsResidualAggregateComparisonBeforeCodegen) {
  QueueGraphPlan plan = aggregateExpressionPlan();
  QueueBlockPlan &transform = plan.blocks[1];
  transform.expressions.push_back(
      {"same", "cmp", "i1", {"item", "item"}, "", "eq"});
  transform.yields = {"same"};
  plan.queues[1].payloadType = "i1";
  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error))
                .find("residual aggregate comparison must be lowered"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsResidualInvariantBeforeCodegen) {
  QueueGraphPlan plan = aggregateExpressionPlan();
  QueueBlockPlan &transform = plan.blocks[1];
  transform.expressions.push_back({"valid", "invariant", "i1", {"item"}});
  transform.yields = {"valid"};
  plan.queues[1].payloadType = "i1";
  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error))
                .find("residual ac.var.invariant must be lowered"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, VerifiesNestedTableMatchCaptureTypes) {
  QueueGraphPlan plan;
  plan.system = "captured_match";
  plan.tables = {{"entries", "i8", 1, 0, "table/entries", "/"}};
  plan.queues = {{"input", "i8", "/", 1, 1}, {"output", "i1", "/", 1, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
  QueueBlockPlan transform{"firing",   "output", "/", {"input"},
                           {"output"}, {1},      {1}};
  transform.expressions.push_back(
      {"captured", "constant", "i8", {}, "", "", "7 : i8"});
  QueueExpressionPlan match{"matched", "table_match", "i1", {"captured"}};
  match.table = "entries";
  match.nestedExpressions.push_back(
      {"same", "cmp", "i1", {"entry", "captured"}, "", "eq"});
  match.nestedYields = {"same"};
  transform.expressions.push_back(std::move(match));
  transform.yields = {"matched"};
  transform.guard = "matched";
  transform.stateReservations.push_back(
      {"entries", "", "", "matched", "all", {"$entry"}});
  plan.blocks.push_back(std::move(transform));
  plan.blocks.push_back({"sink", "sink", "/", {"output"}, {}});

  auto error = verifyQueueGraphPlan(plan);
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
}

TEST(QueueGraphPlanTest, PreservesExactU64MaskedMatchAndRejectsForgedMetadata) {
  QueueGraphPlan plan;
  plan.system = "masked_match";
  plan.queues = {{"input", "i64", "/", 1, 1}, {"output", "i1", "/", 1, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
  QueueBlockPlan transform{"transform", "output", "/", {"input"},
                           {"output"},  {1},      {1}};
  QueueExpressionPlan match;
  match.result = "matched";
  match.kind = "masked_match";
  match.type = "i1";
  match.operands = {"item"};
  match.mask = "0x8000000000000001";
  match.value = "0x8000000000000001";
  transform.expressions.push_back(match);
  transform.yields = {"matched"};
  plan.blocks.push_back(std::move(transform));
  plan.blocks.push_back({"sink", "sink", "/", {"output"}, {}});

  auto error = verifyQueueGraphPlan(plan);
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
  auto json = plan.canonicalJson();
  ASSERT_TRUE(bool(json)) << llvm::toString(json.takeError());
  EXPECT_NE(json->find("\"mask\":\"0x8000000000000001\""), std::string::npos);
  EXPECT_NE(json->find("\"value\":\"0x8000000000000001\""), std::string::npos);
  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  expectCppCompiles(*cpp);
  std::string executable = *cpp;
  executable.append(R"cpp(
int main() {
  ac_generated::block_0_policy match;
  return match(gfsim::UInt<64>{0x8000000000000001ULL}) == 1 &&
                 match(gfsim::UInt<64>{0x8000000000000000ULL}) == 0 &&
                 match(gfsim::UInt<64>{0xffffffffffffffffULL}) == 1
             ? 0
             : 1;
}
)cpp");
  expectCppRuns(executable);
  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("pyc.and"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.cmp"), std::string::npos);
  EXPECT_NE(pyc->find("predicate = \"eq\""), std::string::npos);

  for (const std::pair<std::string, std::string> &forgery : {
           std::pair<std::string, std::string>{"9223372036854775809",
                                               match.value},
           {"0x08000000000000001", match.value},
           {"0x8000000000000000", "0x0000000000000001"},
       }) {
    QueueGraphPlan malformed = plan;
    malformed.blocks[1].expressions[0].mask = forgery.first;
    malformed.blocks[1].expressions[0].value = forgery.second;
    auto malformedError = verifyQueueGraphPlan(malformed);
    ASSERT_TRUE(bool(malformedError));
    EXPECT_NE(llvm::toString(std::move(malformedError))
                  .find("masked_match mask/value metadata"),
              std::string::npos);
  }
}

TEST(QueueGraphPlanTest,
     PreservesReusableModuleSpecializationAndRunsIndependentInstances) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR
      "/tests/mlir/agentic-circuit/Transforms/queue-module-freeze.mlir",
      &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  EXPECT_EQ(plan->definition, "Top");
  ASSERT_EQ(plan->moduleSpecializations.size(), 1u);
  ASSERT_EQ(plan->moduleInstances.size(), 2u);
  EXPECT_EQ(plan->moduleSpecializations.front()->definition, "Increment");
  EXPECT_EQ(plan->moduleInstances[0].specializationFingerprint,
            plan->moduleInstances[1].specializationFingerprint);
  EXPECT_EQ(plan->moduleInstances[0].specializationFingerprint,
            plan->moduleSpecializations.front()->specializationFingerprint);
  EXPECT_EQ(plan->moduleInstances[0].lexicalOrder, 2u);
  EXPECT_EQ(plan->moduleInstances[1].lexicalOrder, 3u);
  QueueGraphPlan malformedOrder = *plan;
  malformedOrder.moduleInstances[1].lexicalOrder =
      malformedOrder.moduleInstances[0].lexicalOrder;
  auto orderError = verifyQueueGraphPlan(malformedOrder);
  ASSERT_TRUE(bool(orderError));
  EXPECT_NE(llvm::toString(std::move(orderError))
                .find("lexical orders must be unique"),
            std::string::npos);
  auto pyc = generateQueueGraphPyc(*plan);
  ASSERT_FALSE(bool(pyc));
  EXPECT_NE(llvm::toString(pyc.takeError())
                .find("module-preserving QueueGraph PYC lowering is not "
                      "implemented"),
            std::string::npos);

  auto generated = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef source(*generated);
  EXPECT_NE(source.find("#include \"gfsim/priority_encode.h\""),
            llvm::StringRef::npos);
  size_t classBegin = source.find("class Module_Increment final");
  ASSERT_NE(classBegin, llvm::StringRef::npos);
  size_t classEnd = source.find(" final", classBegin);
  ASSERT_NE(classEnd, llvm::StringRef::npos);
  llvm::StringRef implementation =
      source.slice(classBegin + std::string("class ").size(), classEnd);
  EXPECT_EQ(source.count(("class " + implementation + " final").str()), 1u);
  EXPECT_EQ(source.count(("  " + implementation + " instance_").str()), 2u);
  const size_t dispatch = source.find("dispatch_rows()");
  ASSERT_NE(dispatch, llvm::StringRef::npos);
  const size_t broadcastRow =
      source.find("makeDispatchRow(&block_0_)", dispatch);
  const size_t leftRow = source.find("instance_0_.dispatch_row(0)", dispatch);
  const size_t rightRow = source.find("instance_1_.dispatch_row(0)", dispatch);
  const size_t firstSinkRow =
      source.find("makeDispatchRow(&block_1_)", dispatch);
  EXPECT_LT(broadcastRow, leftRow);
  EXPECT_LT(leftRow, rightRow);
  EXPECT_LT(rightRow, firstSinkRow);

  std::string executableSource = *generated;
  executableSource.append(R"cpp(
int main() {
  ac_generated::ReusedPipeline model;
  if (!model.input().proposePush(gfsim::UInt<8>{5}))
    return 1;
  model.input().doXfer({0, 0});
  auto rows = model.dispatch_rows();
  for (unsigned tick = 1; tick != 8; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }
  const auto &left = model.sink_0_values();
  const auto &right = model.sink_1_values();
  return left.size() == 1 && right.size() == 1 && left[0] == 6 &&
                 right[0] == 6
             ? 0
             : 2;
}
)cpp");
  expectCppRuns(executableSource);
}

TEST(QueueGraphPlanTest,
     ReadableSpecializationClassesUseLocalSuffixOnlyForCollisions) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR
      "/tests/mlir/agentic-circuit/Transforms/queue-module-freeze.mlir",
      &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->moduleSpecializations.size(), 1u);

  QueueGraphPlan colliding = *plan;
  auto second = std::make_shared<QueueGraphPlan>(
      *colliding.moduleSpecializations.front());
  second->specializationFingerprint =
      "sha256:1111111111111111111111111111111111111111111111111111111111111111";
  colliding.moduleSpecializations.push_back(second);
  auto generated = generateQueueGraphCpp(colliding);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef firstFingerprint =
      colliding.moduleSpecializations.front()->specializationFingerprint;
  ASSERT_TRUE(firstFingerprint.consume_front("sha256:"));
  EXPECT_NE(generated->find("class Module_Increment_s" +
                            firstFingerprint.take_front(16).str()),
            std::string::npos);
  EXPECT_NE(generated->find("class Module_Increment_s1111111111111111"),
            std::string::npos);

  second->specializationFingerprint =
      colliding.moduleSpecializations.front()->specializationFingerprint;
  second->specializationFingerprint.back() =
      second->specializationFingerprint.back() == '0' ? '1' : '0';
  auto ambiguous = generateQueueGraphCpp(colliding);
  ASSERT_FALSE(bool(ambiguous));
  EXPECT_NE(llvm::toString(ambiguous.takeError())
                .find("collide after local fingerprint disambiguation"),
            std::string::npos);
}

TEST(QueueGraphPlanTest,
     ReusesStatefulModuleImplementationWithIndependentPersistentState) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/Transforms/"
                           "queue-stateful-module-freeze.mlir",
      &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->moduleSpecializations.size(), 1u);
  ASSERT_EQ(plan->moduleInstances.size(), 2u);
  const QueueGraphPlan &specialization = *plan->moduleSpecializations.front();
  ASSERT_EQ(specialization.blocks.size(), 1u);
  ASSERT_EQ(specialization.tables.size(), 1u);
  EXPECT_FALSE(plan->activationEdges.empty());
  EXPECT_FALSE(plan->workClosureEdges.empty());
  EXPECT_FALSE(specialization.activationEdges.empty());
  EXPECT_FALSE(specialization.workClosureEdges.empty());
  EXPECT_EQ(specialization.blocks.front().kind, "firing");
  EXPECT_EQ(specialization.tables.front().name, "sum");

  auto generated = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef source(*generated);
  size_t classBegin = source.find("class Module_Accumulator final");
  ASSERT_NE(classBegin, llvm::StringRef::npos);
  size_t classEnd = source.find(" final", classBegin);
  ASSERT_NE(classEnd, llvm::StringRef::npos);
  llvm::StringRef implementation =
      source.slice(classBegin + std::string("class ").size(), classEnd);
  EXPECT_EQ(source.count(("class " + implementation + " final").str()), 1u);
  EXPECT_EQ(source.count(("  " + implementation + " instance_").str()), 2u);
  EXPECT_EQ(source.count("gfsim::SimTable<gfsim::UInt<8>> state_sum_;"), 1u);
  EXPECT_NE(source.find("activation_offsets()"), llvm::StringRef::npos);
  EXPECT_NE(source.find("activation_complete() { return true; }"),
            llvm::StringRef::npos);
  EXPECT_NE(source.find("activation_targets()"), llvm::StringRef::npos);
  EXPECT_NE(source.find("initial_work_ids()"), llvm::StringRef::npos);
  EXPECT_NE(source.find("work_closure_offsets()"), llvm::StringRef::npos);
  EXPECT_NE(source.find("work_closure_targets()"), llvm::StringRef::npos);
  EXPECT_NE(source.find("offer_left_input"), llvm::StringRef::npos);
  EXPECT_NE(source.find("schedule_initial_work"), llvm::StringRef::npos);

  QueueGraphPlan forgedActivation = *plan;
  forgedActivation.activationEdges.pop_back();
  auto activationError = verifyQueueGraphPlan(forgedActivation);
  ASSERT_TRUE(bool(activationError));
  EXPECT_NE(llvm::toString(std::move(activationError))
                .find("activation, Work closure, and initial frontier"),
            std::string::npos);
  QueueGraphPlan forgedClosure = *plan;
  forgedClosure.workClosureEdges.pop_back();
  auto closureError = verifyQueueGraphPlan(forgedClosure);
  ASSERT_TRUE(bool(closureError));
  EXPECT_NE(llvm::toString(std::move(closureError))
                .find("activation, Work closure, and initial frontier"),
            std::string::npos);

  std::string executableSource = *generated;
  executableSource.append(R"cpp(
namespace {
void runTicks(ac_generated::StatefulReuse &model, unsigned first,
              unsigned limit) {
  auto rows = model.dispatch_rows();
  for (unsigned tick = first; tick != limit; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }
}
} // namespace

int main() {
  ac_generated::StatefulReuse model;
  if (!model.left_input().proposePush(gfsim::UInt<8>{1}) ||
      !model.right_input().proposePush(gfsim::UInt<8>{10}))
    return 1;
  model.left_input().doXfer({0, 0});
  model.right_input().doXfer({0, 0});
  runTicks(model, 1, 8);
  if (!model.left_input().proposePush(gfsim::UInt<8>{2}))
    return 2;
  model.left_input().doXfer({8, 0});
  runTicks(model, 9, 16);
  const auto &left = model.sink_0_values();
  const auto &right = model.sink_1_values();
  return left.size() == 2 && left[0] == 1 && left[1] == 3 &&
                 right.size() == 1 && right[0] == 10
             ? 0
             : 3;
}
)cpp");
  expectCppRuns(executableSource);
}

TEST(QueueGraphPlanTest,
     ReusesMultiRuleModuleAndUsesExplicitPriorityIndependentOfSourceOrder) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/Transforms/"
                           "queue-multi-rule-module-freeze.mlir",
      &context);
  ASSERT_TRUE(module);
  mlir::SymbolTable symbols(*module);
  auto definition = symbols.lookup<ac::ModuleOp>("DualAccumulator");
  ASSERT_TRUE(definition);
  definition->setAttr(
      "ac.input_display_names",
      mlir::ArrayAttr::get(&context,
                           {mlir::StringAttr::get(&context, "completion-in"),
                            mlir::StringAttr::get(&context, "completion_in")}));
  definition->setAttr(
      "ac.output_display_names",
      mlir::ArrayAttr::get(&context,
                           {mlir::StringAttr::get(&context, "result"),
                            mlir::StringAttr::get(&context, "result")}));
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->moduleSpecializations.size(), 1u);
  ASSERT_EQ(plan->moduleInstances.size(), 2u);
  const QueueGraphPlan &specialization = *plan->moduleSpecializations.front();
  ASSERT_EQ(specialization.interfaceInputs.size(), 2u);
  ASSERT_EQ(specialization.interfaceOutputs.size(), 2u);
  EXPECT_EQ(specialization.interfaceInputs[0].displayName, "completion-in");
  EXPECT_EQ(specialization.interfaceInputs[1].displayName, "completion_in");
  EXPECT_EQ(specialization.interfaceOutputs[0].displayName, "result");
  EXPECT_EQ(specialization.interfaceOutputs[1].displayName, "result");
  ASSERT_EQ(specialization.blocks.size(), 2u);
  EXPECT_EQ(specialization.blocks[0].stableId, "accumulate_b");
  EXPECT_EQ(specialization.blocks[0].priority, 0u);
  EXPECT_EQ(specialization.blocks[1].stableId, "accumulate_a");
  EXPECT_EQ(specialization.blocks[1].priority, 1u);

  auto generated = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef source(*generated);
  size_t classBegin = source.find("class Module_DualAccumulator");
  ASSERT_NE(classBegin, llvm::StringRef::npos);
  size_t classEnd = source.find(" final", classBegin);
  ASSERT_NE(classEnd, llvm::StringRef::npos);
  llvm::StringRef implementation =
      source.slice(classBegin + std::string("class ").size(), classEnd);
  EXPECT_EQ(source.count(("class " + implementation + " final").str()), 1u);
  EXPECT_EQ(source.count(("  " + implementation + " instance_").str()), 2u);
  EXPECT_NE(source.find((implementation + "_rule_accumulate_b_policy").str()),
            llvm::StringRef::npos);
  EXPECT_NE(source.find((implementation + "_rule_accumulate_a_policy").str()),
            llvm::StringRef::npos);
  EXPECT_NE(source.find("gfsim::SimQueue<gfsim::UInt<8>> &completion_in"),
            llvm::StringRef::npos);
  EXPECT_NE(source.find("gfsim::SimQueue<gfsim::UInt<8>> &completion_in_2"),
            llvm::StringRef::npos);
  EXPECT_NE(source.find("gfsim::SimQueue<gfsim::UInt<8>> &result"),
            llvm::StringRef::npos);
  EXPECT_NE(source.find("gfsim::SimQueue<gfsim::UInt<8>> &result_2"),
            llvm::StringRef::npos);

  std::string executableSource = *generated;
  executableSource.append(R"cpp(
int main() {
  ac_generated::MultiRuleReuse model;
  if (!model.left_a().proposePush(gfsim::UInt<8>{1}) ||
      !model.left_b().proposePush(gfsim::UInt<8>{2}) ||
      !model.right_b().proposePush(gfsim::UInt<8>{10}))
    return 1;
  model.left_a().doXfer({0, 0});
  model.left_b().doXfer({0, 0});
  model.right_b().doXfer({0, 0});
  auto rows = model.dispatch_rows();
  constexpr auto arbitrationOrder =
      ac_generated::MultiRuleReuse::arbitration_order();
  for (unsigned tick = 1; tick != 12; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (gfsim::ObjectId id : arbitrationOrder)
      rows[id].xfer(rows[id].object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }
  const auto &leftA = model.sink_0_values();
  const auto &leftB = model.sink_1_values();
  const auto &rightA = model.sink_2_values();
  const auto &rightB = model.sink_3_values();
  return leftA.size() == 1 && leftA[0] == 3 && leftB.size() == 1 &&
                 leftB[0] == 2 && rightA.empty() && rightB.size() == 1 &&
                 rightB[0] == 10
             ? 0
             : 2;
}
)cpp");
  expectCppRuns(executableSource);
}

TEST(QueueGraphPlanTest, WriterPriorityCanonicalizesDeclarationOrder) {
  auto writer = [](llvm::StringRef name, uint64_t rank,
                   bool reverseMembership = false) {
    QueueBlockPlan block;
    block.kind = "firing";
    block.name = name.str();
    block.stableId = name.str();
    block.arbitrationMembership = {
        {"z_state", name.str(), "priority", rank, "winner_takes_transaction"},
        {"a_state", name.str(), "priority", rank, "winner_takes_transaction"}};
    if (reverseMembership)
      std::reverse(block.arbitrationMembership.begin(),
                   block.arbitrationMembership.end());
    return block;
  };
  QueueGraphPlan first;
  first.blocks = {writer("later", 1), writer("winner", 0, true)};
  QueueGraphPlan second;
  second.blocks = {writer("winner", 0), writer("later", 1, true)};
  EXPECT_FALSE(bool(resolveQueueWriterPriorities(first)));
  EXPECT_FALSE(bool(resolveQueueWriterPriorities(second)));
  ASSERT_EQ(first.blocks.size(), 2u);
  EXPECT_EQ(first.blocks[0].stableId, "winner");
  EXPECT_EQ(first.blocks[1].stableId, "later");
  ASSERT_EQ(second.blocks.size(), 2u);
  for (size_t index = 0; index < first.blocks.size(); ++index) {
    EXPECT_EQ(first.blocks[index].stableId, second.blocks[index].stableId);
    EXPECT_EQ(first.blocks[index].priority, second.blocks[index].priority);
  }
  auto firstJson = first.canonicalJson();
  auto secondJson = second.canonicalJson();
  ASSERT_TRUE(bool(firstJson)) << llvm::toString(firstJson.takeError());
  ASSERT_TRUE(bool(secondJson)) << llvm::toString(secondJson.takeError());
  EXPECT_EQ(*firstJson, *secondJson);
  EXPECT_NE(firstJson->find("\"stable_id\":\"winner\""), std::string::npos);
  EXPECT_NE(firstJson->find("\"policy\":\"priority\""), std::string::npos);
  EXPECT_NE(firstJson->find("\"resolution\":\"winner_takes_transaction\""),
            std::string::npos);
}

TEST(QueueGraphPlanTest, WriterPriorityRejectsCrossOwnerPrecedenceCycle) {
  QueueGraphPlan plan;
  QueueBlockPlan first;
  first.kind = "firing";
  first.stableId = "first";
  first.arbitrationMembership = {
      {"left", "first", "priority", 0, "winner_takes_transaction"},
      {"right", "first", "priority", 1, "winner_takes_transaction"}};
  QueueBlockPlan second;
  second.kind = "firing";
  second.stableId = "second";
  second.arbitrationMembership = {
      {"left", "second", "priority", 1, "winner_takes_transaction"},
      {"right", "second", "priority", 0, "winner_takes_transaction"}};
  plan.blocks = {std::move(first), std::move(second)};
  llvm::Error error = resolveQueueWriterPriorities(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(
      llvm::toString(std::move(error)).find("precedence contains a cycle"),
      std::string::npos);
}

TEST(QueueGraphPlanTest,
     ReusesMultiOwnerModuleWithAtomicIndependentInstanceState) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/Transforms/"
                           "queue-multi-owner-module-freeze.mlir",
      &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->moduleSpecializations.size(), 1u);
  ASSERT_EQ(plan->moduleInstances.size(), 2u);
  const QueueGraphPlan &specialization = *plan->moduleSpecializations.front();
  ASSERT_EQ(specialization.blocks.size(), 1u);
  ASSERT_EQ(specialization.tables.size(), 2u);
  ASSERT_EQ(specialization.blocks.front().stateWrites.size(), 2u);
  EXPECT_EQ(specialization.blocks.front().stateWrites[0].table, "cursor");
  EXPECT_EQ(specialization.blocks.front().stateWrites[1].table, "total");

  auto generated = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef source(*generated);
  size_t classBegin = source.find("class Module_StatePair");
  ASSERT_NE(classBegin, llvm::StringRef::npos);
  size_t classEnd = source.find(" final", classBegin);
  ASSERT_NE(classEnd, llvm::StringRef::npos);
  llvm::StringRef implementation =
      source.slice(classBegin + std::string("class ").size(), classEnd);
  EXPECT_EQ(source.count(("class " + implementation + " final").str()), 1u);
  EXPECT_EQ(source.count(("  " + implementation + " instance_").str()), 2u);
  EXPECT_NE(source.find("gfsim::QueueStateTransition<"), llvm::StringRef::npos);

  std::string executableSource = *generated;
  executableSource.append(R"cpp(
namespace {
void runTicks(ac_generated::MultiOwnerReuse &model, unsigned first,
              unsigned limit) {
  auto rows = model.dispatch_rows();
  for (unsigned tick = first; tick != limit; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }
}
} // namespace

int main() {
  ac_generated::MultiOwnerReuse model;
  if (!model.left_input().proposePush(gfsim::UInt<8>{3}) ||
      !model.right_input().proposePush(gfsim::UInt<8>{10}))
    return 1;
  model.left_input().doXfer({0, 0});
  model.right_input().doXfer({0, 0});
  runTicks(model, 1, 8);
  if (!model.left_input().proposePush(gfsim::UInt<8>{5}))
    return 2;
  model.left_input().doXfer({8, 0});
  runTicks(model, 9, 16);
  const auto &left = model.sink_0_values();
  const auto &right = model.sink_1_values();
  return left.size() == 2 && left[0] == 4 && left[1] == 10 &&
                 right.size() == 1 && right[0] == 11
             ? 0
             : 3;
}
)cpp");
  expectCppRuns(executableSource);
}

TEST(QueueGraphPlanTest,
     StructuredGeneratorPreservesOrderedRepeatedWritesPerOwner) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/Transforms/"
                           "queue-multi-owner-module-freeze.mlir",
      &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->moduleSpecializations.size(), 1u);
  QueueGraphPlan &specialization = *plan->moduleSpecializations.front();
  QueueBlockPlan &firing = specialization.blocks.front();
  ASSERT_EQ(firing.stateWrites.size(), 2u);
  StateWritePlan repeated = firing.stateWrites.front();
  repeated.index = "second_index";
  firing.expressions.push_back(
      {"second_index", "constant", "i1", {}, "", "", "1 : i1"});
  specialization.tables.front().entries = 2;
  specialization.tables.front().shape = {2};
  specialization.tables.front().axisWidths = {1};
  firing.stateWrites.push_back(std::move(repeated));

  auto generated = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef source(*generated);
  const size_t first =
      source.find("state_cursor_writes.emplace_back(static_cast<size_t>("
                  "state_cursor_index), state_cursor_next)");
  const size_t second =
      source.find("state_cursor_writes.emplace_back(static_cast<size_t>("
                  "state_cursor_2_index), state_cursor_2_next)");
  ASSERT_NE(first, llvm::StringRef::npos);
  ASSERT_NE(second, llvm::StringRef::npos);
  EXPECT_LT(first, second);
  EXPECT_NE(source.find("state_total_writes.emplace_back(static_cast<size_t>("
                        "state_total_index), state_total_next)"),
            llvm::StringRef::npos);
  EXPECT_NE(source.find("std::move(state_cursor_writes), "
                        "std::move(state_total_writes)"),
            llvm::StringRef::npos);
  expectCppCompiles(*generated);
}

TEST(QueueGraphPlanTest,
     ReusesCombinedMultiRuleMultiOwnerModuleWithAtomicArbitration) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/Transforms/"
                           "queue-multi-rule-multi-owner-module.mlir",
      &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->moduleSpecializations.size(), 1u);
  ASSERT_EQ(plan->moduleInstances.size(), 2u);
  const QueueGraphPlan &specialization = *plan->moduleSpecializations.front();
  ASSERT_EQ(specialization.blocks.size(), 2u);
  ASSERT_EQ(specialization.tables.size(), 2u);
  ASSERT_EQ(specialization.blocks[0].stateWrites.size(), 2u);
  ASSERT_EQ(specialization.blocks[1].stateWrites.size(), 2u);
  EXPECT_EQ(specialization.blocks[0].priority, 0u);
  EXPECT_EQ(specialization.blocks[1].priority, 1u);

  auto generated = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef source(*generated);
  size_t classBegin = source.find("class Module_DualState");
  ASSERT_NE(classBegin, llvm::StringRef::npos);
  size_t classEnd = source.find(" final", classBegin);
  ASSERT_NE(classEnd, llvm::StringRef::npos);
  llvm::StringRef implementation =
      source.slice(classBegin + std::string("class ").size(), classEnd);
  EXPECT_EQ(source.count(("class " + implementation + " final").str()), 1u);
  EXPECT_EQ(source.count(("  " + implementation + " instance_").str()), 2u);
  EXPECT_EQ(source.count("gfsim::QueueStateTransition<"), 2u);

  std::string executableSource = *generated;
  executableSource.append(R"cpp(
int main() {
  ac_generated::CombinedReuse model;
  if (!model.left_a().proposePush(gfsim::UInt<8>{1}) ||
      !model.left_b().proposePush(gfsim::UInt<8>{2}) ||
      !model.right_b().proposePush(gfsim::UInt<8>{10}))
    return 1;
  model.left_a().doXfer({0, 0});
  model.left_b().doXfer({0, 0});
  model.right_b().doXfer({0, 0});
  auto rows = model.dispatch_rows();
  for (unsigned tick = 1; tick != 12; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }
  const auto &leftA = model.sink_0_values();
  const auto &leftB = model.sink_1_values();
  const auto &rightA = model.sink_2_values();
  const auto &rightB = model.sink_3_values();
  return leftA.size() == 1 && leftA[0] == 2 && leftB.size() == 1 &&
                 leftB[0] == 5 && rightA.empty() && rightB.size() == 1 &&
                 rightB[0] == 11
             ? 0
             : 2;
}
)cpp");
  expectCppRuns(executableSource);
}

TEST(QueueGraphPlanTest,
     EmitsAtomicStatelessMultiOutputModuleWithoutPartialPublication) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/Transforms/"
                           "queue-stateless-multi-output-module.mlir",
      &context);
  ASSERT_TRUE(module);
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->moduleSpecializations.size(), 1u);
  const QueueGraphPlan &specialization = *plan->moduleSpecializations.front();
  ASSERT_EQ(specialization.blocks.size(), 1u);
  EXPECT_TRUE(specialization.tables.empty());
  EXPECT_EQ(specialization.interfaceInputs.size(), 1u);
  EXPECT_EQ(specialization.interfaceOutputs.size(), 4u);
  EXPECT_EQ(specialization.blocks.front().outputs.size(), 4u);

  auto generated = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef source(*generated);
  const size_t classBegin = source.find("class Module_Fanout");
  ASSERT_NE(classBegin, llvm::StringRef::npos);
  const size_t classEnd = source.find(" final", classBegin);
  ASSERT_NE(classEnd, llvm::StringRef::npos);
  const llvm::StringRef implementation =
      source.slice(classBegin + std::string("class ").size(), classEnd);
  EXPECT_EQ(source.count(("class " + implementation + " final").str()), 1u);
  EXPECT_EQ(source.count(("  " + implementation + " instance_").str()), 1u);
  EXPECT_NE(source.find("gfsim::QueueStateTransition<"), llvm::StringRef::npos);
  EXPECT_NE(source.find("std::tuple<gfsim::UInt<8>, gfsim::UInt<8>, "
                        "gfsim::UInt<8>, gfsim::UInt<8>>"),
            llvm::StringRef::npos);

  std::string executableSource = *generated;
  llvm::raw_string_ostream harness(executableSource);
  harness << R"cpp(
int main() {
  gfsim::Module root("root", gfsim::kRootObjectId);
  gfsim::SimQueue<gfsim::UInt<8>> input("input", 0, &root, 1);
  gfsim::SimQueue<gfsim::UInt<8>> first("first", 1, &root, 1);
  gfsim::SimQueue<gfsim::UInt<8>> second("second", 2, &root, 1);
  gfsim::SimQueue<gfsim::UInt<8>> third("third", 3, &root, 1);
  gfsim::SimQueue<gfsim::UInt<8>> fourth("fourth", 4, &root, 1);
  ac_generated::)cpp"
          << implementation
          << R"cpp( fanout("fanout", 5, &root, input, first, second, third,
                 fourth);

  if (!third.proposePush(gfsim::UInt<8>{99}) ||
      !input.proposePush(gfsim::UInt<8>{7}))
    return 1;
  third.doXfer({0, 0});
  input.doXfer({0, 0});

  auto row = fanout.dispatch_row(0);
  const gfsim::Epoch blocked{1, 0};
  row.work(row.object, blocked);
  row.xfer(row.object, blocked, gfsim::XferPhase::Arbitrate);
  if (row.xfer(row.object, blocked, gfsim::XferPhase::Probe) ||
      input.committedSize() != 1 || input.totalPops() != 0 ||
      !first.isEmpty() || !second.isEmpty() || !fourth.isEmpty() ||
      first.hasPendingCommit() || second.hasPendingCommit() ||
      fourth.hasPendingCommit() || third.committedSize() != 1)
    return 2;

  if (!third.proposePop())
    return 3;
  third.doXfer(blocked);

  const gfsim::Epoch accepted{2, 0};
  row.work(row.object, accepted);
  row.xfer(row.object, accepted, gfsim::XferPhase::Arbitrate);
  if (!row.xfer(row.object, accepted, gfsim::XferPhase::Probe) ||
      !input.hasPendingCommit() || !first.hasPendingCommit() ||
      !second.hasPendingCommit() || !third.hasPendingCommit() ||
      !fourth.hasPendingCommit())
    return 4;

  input.doXfer(accepted);
  first.doXfer(accepted);
  second.doXfer(accepted);
  third.doXfer(accepted);
  fourth.doXfer(accepted);
  row.xfer(row.object, accepted, gfsim::XferPhase::Commit);
  if (!input.isEmpty() || input.totalPops() != 1 ||
      first.committedValues() != std::vector<gfsim::UInt<8>>{{7}} ||
      second.committedValues() != std::vector<gfsim::UInt<8>>{{8}} ||
      third.committedValues() != std::vector<gfsim::UInt<8>>{{9}} ||
      fourth.committedValues() != std::vector<gfsim::UInt<8>>{{10}})
    return 5;

  const gfsim::Epoch idle{3, 0};
  row.work(row.object, idle);
  row.xfer(row.object, idle, gfsim::XferPhase::Arbitrate);
  row.xfer(row.object, idle, gfsim::XferPhase::Commit);
  return input.totalPops() == 1 && first.totalPushes() == 1 &&
                 second.totalPushes() == 1 && third.totalPushes() == 2 &&
                 fourth.totalPushes() == 1
             ? 0
             : 6;
}
)cpp";
  harness.flush();
  expectCppRuns(executableSource);
}

TEST(QueueGraphPlanTest,
     PreservesNestedSpecializationReuseWithoutFlatteningChildBodies) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/Transforms/"
                           "queue-nested-module-freeze.mlir",
      &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->moduleSpecializations.size(), 1u);
  ASSERT_EQ(plan->moduleInstances.size(), 2u);
  const QueueGraphPlan &wrapper = *plan->moduleSpecializations.front();
  EXPECT_EQ(wrapper.definition, "Wrapper");
  ASSERT_EQ(wrapper.moduleSpecializations.size(), 1u);
  ASSERT_EQ(wrapper.moduleInstances.size(), 1u);
  EXPECT_EQ(wrapper.moduleSpecializations.front()->definition, "Increment");
  EXPECT_EQ(wrapper.moduleInstances.front().specializationFingerprint,
            wrapper.moduleSpecializations.front()->specializationFingerprint);

  auto generated = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef source(*generated);
  EXPECT_EQ(source.count("class Module_Increment"), 1u);
  EXPECT_EQ(source.count("class Module_Wrapper"), 1u);
  EXPECT_EQ(source.count(" child_0_;"), 1u);

  std::string executableSource = *generated;
  executableSource.append(R"cpp(
int main() {
  ac_generated::NestedReuse model;
  if (!model.left_input().proposePush(gfsim::UInt<8>{5}) ||
      !model.right_input().proposePush(gfsim::UInt<8>{10}))
    return 1;
  model.left_input().doXfer({0, 0});
  model.right_input().doXfer({0, 0});
  auto rows = model.dispatch_rows();
  for (unsigned tick = 1; tick != 8; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }
  const auto &left = model.sink_0_values();
  const auto &right = model.sink_1_values();
  return left.size() == 1 && left[0] == 6 && right.size() == 1 &&
                 right[0] == 11
             ? 0
             : 2;
}
)cpp");
  expectCppRuns(executableSource);
}

TEST(QueueGraphPlanTest,
     OwnsInternalQueuesPerMixedNestedSpecializationInstance) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/Transforms/"
                           "queue-mixed-nested-module.mlir",
      &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->moduleSpecializations.size(), 1u);
  const QueueGraphPlan &parent = *plan->moduleSpecializations.front();
  EXPECT_EQ(parent.definition, "PrepareAndIncrement");
  ASSERT_EQ(parent.blocks.size(), 1u);
  ASSERT_EQ(parent.moduleInstances.size(), 1u);
  ASSERT_EQ(parent.moduleSpecializations.size(), 1u);
  ASSERT_EQ(parent.queues.size(), 2u);
  EXPECT_EQ(parent.blocks.front().outputs.front(), "prepared");
  EXPECT_EQ(parent.moduleInstances.front().inputs.front(), "prepared");

  auto generated = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef source(*generated);
  EXPECT_EQ(source.count("class Module_Increment"), 1u);
  EXPECT_EQ(source.count("class Module_PrepareAndIncrement"), 1u);
  EXPECT_EQ(source.count("gfsim::SimQueue<gfsim::UInt<8>> queue_0_;"), 1u);
  EXPECT_NE(source.find("activation_complete() { return true; }"),
            llvm::StringRef::npos);

  std::string executableSource = *generated;
  executableSource.append(R"cpp(
int main() {
  ac_generated::MixedNestedReuse model;
  if (!model.left_input().proposePush(gfsim::UInt<8>{5}) ||
      !model.right_input().proposePush(gfsim::UInt<8>{10}))
    return 1;
  model.left_input().doXfer({0, 0});
  model.right_input().doXfer({0, 0});
  auto rows = model.dispatch_rows();
  for (unsigned tick = 1; tick != 10; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }
  const auto &left = model.sink_0_values();
  const auto &right = model.sink_1_values();
  return left.size() == 1 && left[0] == 7 && right.size() == 1 &&
                 right[0] == 12
             ? 0
             : 2;
}
)cpp");
  expectCppRuns(executableSource);
}

TEST(QueueGraphPlanTest, PreservesQueueRateAndRejectsUnspecializedPycLanes) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  std::string rated = kQueueGraph.str();
  size_t attributes = rated.find("{ac.name = \"input\"}");
  ASSERT_NE(attributes, std::string::npos);
  rated.replace(attributes, std::string("{ac.name = \"input\"}").size(),
                "{ac.name = \"input\", "
                "ac.output_rates = array<i64: 2>}");
  for (size_t offset = 0;
       (offset = rated.find("!ac.queue<i64>", offset)) != std::string::npos;
       offset += std::string("!ac.queue<i64, lanes=2, rate=2>").size())
    rated.replace(offset, std::string("!ac.queue<i64>").size(),
                  "!ac.queue<i64, lanes=2, rate=2>");
  auto module = mlir::parseSourceString<mlir::ModuleOp>(rated, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_FALSE(plan->queues.empty());
  EXPECT_EQ(plan->queues.front().rate, 2u);
  auto json = plan->canonicalJson();
  ASSERT_TRUE(bool(json)) << llvm::toString(json.takeError());
  EXPECT_NE(json->find("\"rate\":2"), std::string::npos);
  auto cpp = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find(", nullptr, 1, 2, 2)"), std::string::npos);
  auto pyc = generateQueueGraphPyc(*plan);
  ASSERT_FALSE(bool(pyc));
  EXPECT_FALSE(llvm::toString(pyc.takeError()).empty());
}

TEST(QueueGraphPlanTest, RejectsLegacyContractEpochBeforePlanning) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  std::string legacy = kQueueGraph.str();
  size_t epoch = legacy.find("ac.contract_epoch = \"0.5\"");
  ASSERT_NE(epoch, std::string::npos);
  legacy.replace(epoch, std::string("ac.contract_epoch = \"0.5\"").size(),
                 "ac.contract_epoch = \"0.4\"");
  auto module = mlir::parseSourceString<mlir::ModuleOp>(legacy, &context);
  ASSERT_TRUE(module);
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_FALSE(bool(plan));
  EXPECT_NE(llvm::toString(plan.takeError())
                .find("module requires ac.contract_epoch exactly '0.5'"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, ExtractsPayloadAndImmutableVarDag) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStructuredTransform, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->payloads.size(), 1u);
  EXPECT_EQ(plan->payloads[0].name, "Item");
  ASSERT_EQ(plan->payloads[0].fields.size(), 1u);
  EXPECT_EQ(plan->payloads[0].fields[0].name, "value");
  ASSERT_EQ(plan->blocks.size(), 3u);
  const QueueBlockPlan &transform = plan->blocks[1];
  ASSERT_EQ(transform.expressions.size(), 4u);
  EXPECT_EQ(transform.expressions[0].kind, "get");
  EXPECT_EQ(transform.expressions[1].kind, "constant");
  EXPECT_EQ(transform.expressions[2].kind, "add");
  EXPECT_EQ(transform.expressions[3].kind, "with");
  ASSERT_EQ(transform.yields.size(), 1u);
  EXPECT_EQ(transform.yields[0], "v3");
}

TEST(QueueGraphPlanTest, NativeGeneratorConsumesOnlyExtractedPlan) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceString<mlir::ModuleOp>(kQueueGraph, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  auto source = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(source)) << llvm::toString(source.takeError());
  EXPECT_NE(source->find("gfsim::QueueRoute<gfsim::UInt<64>, 2"),
            std::string::npos);
  EXPECT_NE(source->find("gfsim::QueueMerge<gfsim::UInt<64>, 2>"),
            std::string::npos);
  EXPECT_NE(source->find("gfsim::QueueSink<gfsim::UInt<64>>"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, EmitsClosedOpaqueRuntimeAbiBundle) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceString<mlir::ModuleOp>(kQueueGraph, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());

  auto bundle = generateQueueGraphModelBundle(
      *plan, {.sdkProductVersion = "6.0.0",
              .sdkSourceRevision = "c053fe2a00000000000000000000000000000000"});
  ASSERT_TRUE(bool(bundle)) << llvm::toString(bundle.takeError());
  ASSERT_EQ(bundle->size(), 3u);
  EXPECT_EQ((*bundle)[0].relativePath, "include/generated/model.h");
  EXPECT_EQ((*bundle)[1].relativePath, "src/generated/model.cpp");
  EXPECT_EQ((*bundle)[2].relativePath, "src/generated/queuegraph.cpp");

  const llvm::StringRef header((*bundle)[0].content);
  EXPECT_NE(header.find("gfsim/model_api.h"), llvm::StringRef::npos);
  EXPECT_EQ(header.find("class "), llvm::StringRef::npos);
  EXPECT_EQ(header.find("SimSystem"), llvm::StringRef::npos);

  const llvm::StringRef model((*bundle)[1].content);
  EXPECT_EQ(model.count("agentic_model_query_v1"), 1u);
  EXPECT_NE(model.find("AgenticModelApiV1 api"), llvm::StringRef::npos);
  EXPECT_NE(model.find("\"6.0.0\""), llvm::StringRef::npos);
  EXPECT_NE(model.find("createModel, destroyModel"), llvm::StringRef::npos);
  EXPECT_EQ(model.find("gfsim::SimSystem"), llvm::StringRef::npos);
  EXPECT_EQ(model.find("load_trace_json"), llvm::StringRef::npos);
  EXPECT_EQ(model.find("observations_json"), llvm::StringRef::npos);
  EXPECT_EQ(model.find("trace_position"), llvm::StringRef::npos);

  const llvm::StringRef queueGraph((*bundle)[2].content);
  EXPECT_NE(queueGraph.find("gfsim::SimSystem system"), llvm::StringRef::npos);
  EXPECT_NE(queueGraph.find("system.statistics()"), llvm::StringRef::npos);
  EXPECT_EQ(queueGraph.find("loadPtoTraceText"), llvm::StringRef::npos);
  EXPECT_EQ(queueGraph.find("system.observations()"), llvm::StringRef::npos);
  EXPECT_NE(queueGraph.find("std::vector<gfsim::DispatchRow> rows"),
            llvm::StringRef::npos);
  EXPECT_EQ(queueGraph.count("runtime->model.reset();"), 1u);
  expectModelBundleRuns(*bundle);
}

TEST(QueueGraphPlanTest, ModelBundleRequiresExplicitSdkIdentity) {
  QueueGraphPlan plan;
  auto missingProduct = generateQueueGraphModelBundle(
      plan, {.sdkSourceRevision = "c053fe2a00000000000000000000000000000000"});
  ASSERT_FALSE(bool(missingProduct));
  EXPECT_NE(llvm::toString(missingProduct.takeError())
                .find("SDK product version is required"),
            std::string::npos);

  auto missingRevision =
      generateQueueGraphModelBundle(plan, {.sdkProductVersion = "6.0.0"});
  ASSERT_FALSE(bool(missingRevision));
  EXPECT_NE(llvm::toString(missingRevision.takeError())
                .find("SDK source revision is required"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, EmitsCanonicalScalarQueuePyc) {
  QueueGraphPlan plan;
  plan.system = "scalar_pipeline";
  plan.queues = {{"input", "i64", "/", 2, 1}, {"output", "i64", "/", 2, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {2}, {1}});
  QueueBlockPlan transform{"transform", "output", "/", {"input"},
                           {"output"},  {2},      {1}};
  transform.expressions = {{"v0", "constant", "i64", {}, "", "", "1 : i64"},
                           {"v1", "add", "i64", {"item", "v0"}, "", "", ""}};
  transform.yields = {"v1"};
  plan.blocks.push_back(std::move(transform));
  plan.blocks.push_back({"sink", "sink_0", "/", {"output"}, {}});
  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_EQ(std::count(pyc->begin(), pyc->end(), '\n') > 5, true);
  EXPECT_NE(pyc->find("pyc.fifo"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.add"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.frontend.contract = \"pycircuit\""),
            std::string::npos);
}

TEST(QueueGraphPlanTest, LowersUnsignedAcirShrToCanonicalPycLshr) {
  QueueGraphPlan plan;
  plan.system = "unsigned_shift";
  plan.queues = {{"input", "i8", "/", 2, 1}, {"output", "i8", "/", 2, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {2}, {1}});
  QueueBlockPlan transform{"transform", "output", "/", {"input"},
                           {"output"},  {2},      {1}};
  transform.expressions = {{"v0", "constant", "i8", {}, "", "", "1 : i8"},
                           {"v1", "shr", "i8", {"item", "v0"}, "", "", ""}};
  transform.yields = {"v1"};
  plan.blocks.push_back(std::move(transform));
  plan.blocks.push_back({"sink", "sink_0", "/", {"output"}, {}});

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("pyc.lshr"), std::string::npos);
  EXPECT_EQ(pyc->find("pyc.shr"), std::string::npos);
}

TEST(QueueGraphPlanTest, EmitsAtomicTransformWithIndependentArity) {
  QueueGraphPlan plan;
  plan.system = "atomic_sum";
  plan.queues = {{"left", "i64", "/", 2, 1},
                 {"right", "i64", "/", 2, 1},
                 {"sum", "i64", "/", 2, 1}};
  plan.blocks.push_back({"source", "left", "/", {}, {"left"}, {2}, {1}});
  plan.blocks.push_back({"source", "right", "/", {}, {"right"}, {2}, {1}});
  QueueBlockPlan transform{"transform", "sum", "/", {"left", "right"},
                           {"sum"},     {2},   {1}};
  transform.expressions = {{"v0", "add", "i64", {"item", "item1"}, "", "", ""}};
  transform.yields = {"v0"};
  plan.blocks.push_back(std::move(transform));
  plan.blocks.push_back({"sink", "sink_0", "/", {"sum"}, {}});

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("std::tuple<gfsim::UInt<64>> operator()(const "
                      "gfsim::UInt<64> &item, const gfsim::UInt<64> &item1)"),
            std::string::npos);
  EXPECT_NE(cpp->find("QueueAtomicTransform<block_0_policy, "
                      "std::tuple<gfsim::UInt<64>, gfsim::UInt<64>>, "
                      "std::tuple<gfsim::UInt<64>>>"),
            std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("pyc.add"), std::string::npos);
  EXPECT_NE(pyc->find("= pyc.wire : i1"), std::string::npos);
}

TEST(QueueGraphPlanTest,
     EmitsStatelessHeterogeneousOptionalMultiOutputTransition) {
  QueueGraphPlan plan = statelessOptionalMultiOutputPlan();
  auto error = verifyQueueGraphPlan(plan);
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("gfsim::StateTransitionPlan<std::tuple<>, "
                      "std::tuple<gfsim::UInt<8>, gfsim::UInt<16>>>"),
            std::string::npos);
  EXPECT_NE(cpp->find("output_narrow_present ? std::optional<gfsim::UInt<8>>"),
            std::string::npos);
  EXPECT_NE(cpp->find("output_wide_present ? std::optional<gfsim::UInt<16>>"),
            std::string::npos);
  expectCppCompiles(*cpp);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("%out0_ready: i1"), std::string::npos);
  EXPECT_NE(pyc->find("%out1_ready: i1"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.assign"), std::string::npos);
  EXPECT_GE(std::count(pyc->begin(), pyc->end(), '\n'), 20);
}

TEST(QueueGraphPlanTest, CompilesSevenOptionalOutputsPlusMandatoryAck) {
  QueueGraphPlan plan = eightOutputCommitGroupPlan();
  auto error = verifyQueueGraphPlan(plan);
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  for (unsigned ordinal = 0; ordinal < 8; ++ordinal)
    EXPECT_NE(cpp->find("output_" + plan.blocks[1].outputs[ordinal] +
                        "_present ? std::optional<"),
              std::string::npos);
  EXPECT_NE(cpp->find("gfsim::UInt<15>"), std::string::npos);
  expectCppCompiles(*cpp);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  size_t constants = 0;
  for (size_t offset = 0;
       (offset = pyc->find(" = pyc.constant ", offset)) != std::string::npos;
       offset += 16)
    ++constants;
  EXPECT_EQ(constants, 9u);
}

TEST(QueueGraphPlanTest, RejectsNoncanonicalOutputPresenceOrdinalOrder) {
  QueueGraphPlan plan = statelessOptionalMultiOutputPlan();
  QueueBlockPlan &firing = plan.blocks[1];
  std::swap(firing.outputPresence[0], firing.outputPresence[1]);
  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error))
                .find("output presence ordinals must be sorted"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsDuplicateOutputPresenceOrdinal) {
  QueueGraphPlan plan = statelessOptionalMultiOutputPlan();
  plan.blocks[1].outputPresence[1].ordinal = 0;
  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(
      llvm::toString(std::move(error)).find("cover each output exactly once"),
      std::string::npos);
}

TEST(QueueGraphPlanTest, EmitsHeterogeneousBarrierForBothBackends) {
  QueueGraphPlan plan;
  plan.system = "barrier";
  plan.queues = {{"left", "i8", "/", 2, 1},
                 {"right", "i16", "/", 2, 1},
                 {"left_ready", "i8", "/", 2, 1},
                 {"right_ready", "i16", "/", 2, 1}};
  plan.blocks.push_back({"source", "left", "/", {}, {"left"}, {2}, {1}});
  plan.blocks.push_back({"source", "right", "/", {}, {"right"}, {2}, {1}});
  plan.blocks.push_back({"barrier",
                         "left_ready",
                         "/",
                         {"left", "right"},
                         {"left_ready", "right_ready"},
                         {2, 2},
                         {1, 1}});
  plan.blocks.push_back({"sink", "sink_0", "/", {"left_ready"}, {}});
  plan.blocks.push_back({"sink", "sink_1", "/", {"right_ready"}, {}});

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("gfsim::QueueBarrier<std::tuple<gfsim::UInt<8>, "
                      "gfsim::UInt<16>>>"),
            std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("%in0_valid"), std::string::npos);
  EXPECT_NE(pyc->find("%out1_ready"), std::string::npos);
}

TEST(QueueGraphPlanTest, EmitsStaticQueueCollectionSelectForBothBackends) {
  QueueGraphPlan plan;
  plan.system = "select";
  plan.queues = {{"control", "i8", "/", 1, 1},
                 {"left", "i16", "/", 1, 1},
                 {"right", "i16", "/", 1, 1},
                 {"selected", "i16", "/", 2, 1}};
  plan.blocks.push_back({"source", "control", "/", {}, {"control"}, {1}, {1}});
  plan.blocks.push_back({"source", "left", "/", {}, {"left"}, {1}, {1}});
  plan.blocks.push_back({"source", "right", "/", {}, {"right"}, {1}, {1}});
  QueueBlockPlan select{
      "select",     "selected", "/", {"control", "left", "right"},
      {"selected"}, {2},        {1}};
  select.yields = {"item"};
  plan.blocks.push_back(std::move(select));
  plan.blocks.push_back({"sink", "sink_0", "/", {"selected"}, {}});

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("gfsim::QueueSelect<gfsim::UInt<8>, gfsim::UInt<16>, 2"),
            std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("select_selector_out_of_range"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.select"), std::string::npos);
}

TEST(QueueGraphPlanTest, EmitsTypedReorderForBothBackends) {
  QueueGraphPlan plan;
  plan.system = "ordered";
  plan.queues = {{"input", "i64", "/", 4, 1}, {"output", "i64", "/", 4, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {4}, {1}});
  QueueBlockPlan reorder{"reorder",  "output", "/", {"input"},
                         {"output"}, {4},      {1}};
  reorder.yields = {"item"};
  reorder.capacity = 4;
  reorder.start = 0;
  plan.blocks.push_back(std::move(reorder));
  plan.blocks.push_back({"sink", "sink_0", "/", {"output"}, {}});

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("gfsim::QueueReorder<gfsim::UInt<64>, block_0_policy>"),
            std::string::npos);
  EXPECT_NE(cpp->find(", input_, output_, 4, 0)"), std::string::npos);
  EXPECT_NE(cpp->find("size_t reorder_0_active() const"), std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("pyc.reg"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.cmp"), std::string::npos);
  EXPECT_NE(pyc->find("predicate = \"ult\""), std::string::npos);
  EXPECT_GE(std::count(pyc->begin(), pyc->end(), '\n'), 40);
}

TEST(QueueGraphPlanTest, EmitsTypedDependencyForBothBackends) {
  QueueGraphPlan plan;
  plan.system = "dependent";
  plan.queues = {{"input", "i8", "/", 4, 1}, {"output", "i8", "/", 4, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {4}, {1}});
  QueueBlockPlan dependency{"dependency", "output", "/", {"input"},
                            {"output"},   {4},      {1}};
  dependency.expressions = {
      {"v0", "constant", "i8", {}, "", "", "255 : i8"},
      {"v1", "constant", "i1", {}, "", "", "0 : i1"},
      {"v2", "constant", "i8", {}, "", "", "1 : i8"},
  };
  dependency.yields = {"item", "v0", "v1", "v2"};
  dependency.capacity = 4;
  dependency.resources = 2;
  dependency.noDependency = 255;
  plan.blocks.push_back(std::move(dependency));
  plan.blocks.push_back({"sink", "sink_0", "/", {"output"}, {}});

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("gfsim::QueueDependency<gfsim::UInt<8>"),
            std::string::npos);
  EXPECT_NE(cpp->find(", input_, output_, 4, 2, 255)"), std::string::npos);
  EXPECT_NE(cpp->find("size_t dependency_0_active() const"), std::string::npos);
  EXPECT_NE(cpp->find("dependency_0_resource_active"), std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("pyc.reg"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.sub"), std::string::npos);
}

TEST(QueueGraphPlanTest, ScheduleV2UsesPersistentGfsimAndRejectsPycUntilLanes) {
  QueueGraphPlan plan;
  plan.system = "schedule_v2";
  plan.queues = {{"input", "i8", "/", 4, 1}, {"output", "i8", "/", 4, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {4}, {1}});
  QueueBlockPlan schedule{"dependency", "output", "/", {"input"},
                          {"output"},   {4},      {1}};
  schedule.expressions = {
      {"key", "constant", "i8", {}, "", "", "1 : i8"},
      {"predecessor", "constant", "i8", {}, "", "", "255 : i8"},
      {"resource", "constant", "i2", {}, "", "", "0 : i2"},
      {"cost", "constant", "i8", {}, "", "", "1 : i8"},
  };
  schedule.yields = {"key", "predecessor", "resource", "cost"};
  schedule.capacity = 4;
  schedule.resources = 2;
  schedule.noDependency = 255;
  schedule.provider = "v2";
  plan.blocks.push_back(std::move(schedule));
  plan.blocks.push_back({"sink", "sink_0", "/", {"output"}, {}});

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("gfsim::Schedule<gfsim::UInt<8>, 4, 2, 255"),
            std::string::npos);
  EXPECT_NE(cpp->find(", input_, output_)"), std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_FALSE(bool(pyc));
  EXPECT_NE(llvm::toString(pyc.takeError())
                .find("schedule v2 PYC provider requires issue #21"),
            std::string::npos);

  plan.blocks[1].noDependency = 127;
  auto invalid = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(invalid));
  EXPECT_NE(llvm::toString(std::move(invalid)).find("all-ones sentinel"),
            std::string::npos);

  plan.blocks[1].noDependency = 255;
  plan.blocks[1].expressions[1].type = "i16";
  auto mismatched = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(mismatched));
  EXPECT_NE(llvm::toString(std::move(mismatched))
                .find("matching key and predecessor"),
            std::string::npos);

  plan.blocks[1].expressions = {
      {"predecessor", "constant", "i8", {}, "", "", "255 : i8"},
      {"resource", "constant", "i2", {}, "", "", "0 : i2"},
      {"cost", "constant", "i8", {}, "", "", "1 : i8"},
  };
  plan.blocks[1].yields = {"item", "predecessor", "resource", "cost"};
  auto directRoot = verifyQueueGraphPlan(plan);
  EXPECT_FALSE(bool(directRoot)) << llvm::toString(std::move(directRoot));
  auto directRootCpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(directRootCpp)) << llvm::toString(directRootCpp.takeError());
  EXPECT_NE(directRootCpp->find("gfsim::Schedule<gfsim::UInt<8>, 4, 2, 255"),
            std::string::npos);

  const QueueBlockPlan validProvider = plan.blocks[1];
  auto expectMalformedProvider = [&](QueueBlockPlan malformed) {
    plan.blocks[1] = std::move(malformed);
    auto error = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(error));
    EXPECT_NE(llvm::toString(std::move(error))
                  .find("schedule provider metadata is unsupported"),
              std::string::npos);
  };
  QueueBlockPlan malformed = validProvider;
  malformed.inputs.clear();
  expectMalformedProvider(malformed);
  malformed = validProvider;
  malformed.outputs.clear();
  expectMalformedProvider(malformed);
  malformed = validProvider;
  malformed.capacity = 0;
  expectMalformedProvider(malformed);
  malformed = validProvider;
  malformed.resources = 0;
  expectMalformedProvider(malformed);
}

TEST(QueueGraphPlanTest, EmitsTypedCreditWindowForBothBackends) {
  QueueGraphPlan plan;
  plan.system = "credited";
  plan.queues = {{"input", "i8", "/", 4, 1}, {"output", "i8", "/", 4, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {4}, {1}});
  QueueBlockPlan credit{"credit",   "output", "/", {"input"},
                        {"output"}, {4},      {1}};
  credit.yields = {"item"};
  credit.credits = 2;
  plan.blocks.push_back(std::move(credit));
  plan.blocks.push_back({"sink", "sink_0", "/", {"output"}, {}});

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("gfsim::QueueCredit<gfsim::UInt<8>, block_0_policy>"),
            std::string::npos);
  EXPECT_NE(cpp->find(", input_, output_, 2)"), std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("pyc.reg"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.sub"), std::string::npos);
}

TEST(QueueGraphPlanTest, EmitsOldDataMemoryForBothBackends) {
  QueueGraphPlan plan;
  plan.system = "memory_pipeline";
  plan.payloads = {{"MemoryRequest",
                    {{"address", "i4", 4},
                     {"write", "i1", 1},
                     {"data", "i16", 16},
                     {"tag", "i8", 8}}}};
  constexpr llvm::StringLiteral requestType =
      "!ac.struct<@types::@MemoryRequest>";
  plan.queues = {{"input0", requestType.str(), "/", 4, 1},
                 {"input1", requestType.str(), "/", 4, 1},
                 {"output0", requestType.str(), "/", 4, 1},
                 {"output1", requestType.str(), "/", 4, 1}};
  plan.blocks.push_back({"source", "input0", "/", {}, {"input0"}, {4}, {1}});
  plan.blocks.push_back({"source", "input1", "/", {}, {"input1"}, {4}, {1}});
  plan.memoryInstances.push_back({"sram", "i16", 15, 0, 3, "memory/sram", "/"});
  QueueBlockPlan memory{"memory_request", "output0", "/", {"input0"},
                        {"output0"},      {4},       {1}};
  memory.expressions = {
      {"v0", "get", "i4", {"item"}, "address", "", ""},
      {"v1", "get", "i1", {"item"}, "write", "", ""},
      {"v2", "get", "i16", {"item"}, "data", "", ""},
  };
  memory.yields = {"v0", "v1", "v2"};
  memory.resultField = "data";
  memory.memoryInstance = "sram";
  memory.endpointOrdinal = 0;
  plan.memoryRequests.push_back(
      {"sram", "output0", "/", "input0", "output0", 0, 4, "data"});
  plan.blocks.push_back(memory);
  memory.name = "output1";
  memory.inputs = {"input1"};
  memory.outputs = {"output1"};
  memory.endpointOrdinal = 1;
  plan.memoryRequests.push_back(
      {"sram", "output1", "/", "input1", "output1", 1, 4, "data"});
  plan.blocks.push_back(std::move(memory));
  plan.blocks.push_back({"sink", "sink_0", "/", {"output0"}, {}});
  plan.blocks.push_back({"sink", "sink_1", "/", {"output1"}, {}});

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(
      cpp->find("gfsim::QueueMemoryArbiter<MemoryRequest, gfsim::UInt<16>"),
      std::string::npos);
  EXPECT_NE(cpp->find("result.data = old_data"), std::string::npos);
  EXPECT_NE(cpp->find("std::array<gfsim::SimQueue<MemoryRequest> *, "
                      "2>{&input0_, &input1_}"),
            std::string::npos);
  expectCppCompiles(*cpp);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("pyc.sub"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.sync_mem"), std::string::npos);
  EXPECT_EQ(pyc->find("pyc.sync_mem", pyc->find("pyc.sync_mem") + 1),
            std::string::npos);
  EXPECT_NE(pyc->find("{depth = 15, name = \"sram\"}"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.concat"), std::string::npos);
  EXPECT_NE(pyc->find("memory_address_out_of_range"), std::string::npos);
}

TEST(QueueGraphPlanTest, NativeTableKeyConvertsExactWidthValue) {
  QueueGraphPlan plan = sharedReferencePlan();
  plan.blocks.clear();
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
  plan.queues.push_back({"output", "i13", "/", 1, 1});
  QueueBlockPlan read{"table_read", "read", "/", {"input"},
                      {"output"},   {1},    {1}};
  read.table = "issue";
  read.expressions = {
      {"address", "constant", "i2", {}, "", "", "0 : i2"},
      {"enabled", "constant", "i1", {}, "", "", "true"},
  };
  read.yields = {"address", "enabled"};
  plan.blocks.push_back(std::move(read));
  plan.blocks.push_back({"sink", "sink_0", "/", {"output"}, {}});
  plan.slots.clear();
  plan.tableReads = {{"issue", "read", "/", "input", "output", 1, 1}};
  plan.tables[0].entryType = "i13";
  plan.tableMatches = {{"match",
                        "issue",
                        "/",
                        "i4",
                        {{"v0", "constant", "i1", {}, "", "", "true"}},
                        "v0"}};
  plan.tableSelections[0].policy = "min";
  plan.tableSelections[0].keyOrdering = "unsigned";
  plan.tableSelections[0].keyYield = "item";

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("return [&]()"), std::string::npos);
  expectCppCompiles(*cpp);
}

TEST(QueueGraphPlanTest, EmitsQueuePredicateAsPycComparison) {
  struct Case {
    llvm::StringLiteral predicate;
    llvm::StringLiteral opcode;
    bool negated;
  };
  constexpr Case cases[] = {
      {"eq", "predicate = \"eq\"", false},
      {"ne", "predicate = \"eq\"", true},
      {"slt", "predicate = \"slt\"", false},
      {"sle", "predicate = \"slt\"", true},
      {"sgt", "predicate = \"slt\"", false},
      {"sge", "predicate = \"slt\"", true},
  };
  for (const Case &testCase : cases) {
    SCOPED_TRACE(testCase.predicate.str());
    mlir::MLIRContext context;
    context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
    std::string source = kQueueGraph.str();
    const std::string original = "ac.var.cmp \"eq\"";
    size_t predicate = source.find(original);
    ASSERT_NE(predicate, std::string::npos);
    source.replace(predicate, original.size(),
                   "ac.var.cmp \"" + testCase.predicate.str() + "\"");
    auto module = mlir::parseSourceString<mlir::ModuleOp>(source, &context);
    ASSERT_TRUE(module);
    ASSERT_TRUE(freezeQueueGraph(*module));
    auto plan = buildQueueGraphPlan(*module);
    ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
    auto pyc = generateQueueGraphPyc(*plan);
    ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
    const size_t comparison = pyc->find(testCase.opcode.str());
    ASSERT_NE(comparison, std::string::npos);
    EXPECT_EQ(pyc->find("pyc.rr_arbiter"), std::string::npos);
    EXPECT_NE(pyc->find("primitive_id = \"control.rr_arbiter.v1\""),
              std::string::npos);
    const size_t comparisonEnd = pyc->find('\n', comparison);
    ASSERT_NE(comparisonEnd, std::string::npos);
    const size_t nextEnd = pyc->find('\n', comparisonEnd + 1);
    ASSERT_NE(nextEnd, std::string::npos);
    const llvm::StringRef nextLine(pyc->data() + comparisonEnd + 1,
                                   nextEnd - comparisonEnd - 1);
    EXPECT_EQ(nextLine.contains("pyc.not"), testCase.negated);
  }
}

TEST(QueueGraphPlanTest, EmitsOneReadyValidStagePerQueueLatency) {
  QueueGraphPlan plan;
  plan.system = "latency_pipeline";
  plan.queues = {{"input", "i64", "/", 2, 1}, {"output", "i64", "/", 4, 3}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {2}, {1}});
  QueueBlockPlan transform{"transform", "output", "/", {"input"},
                           {"output"},  {4},      {3}};
  transform.yields = {"item"};
  plan.blocks.push_back(std::move(transform));
  plan.blocks.push_back({"sink", "sink_0", "/", {"output"}, {}});
  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  size_t count = 0;
  for (size_t offset = 0;
       (offset = pyc->find("pyc.fifo", offset)) != std::string::npos;
       offset += 8)
    ++count;
  EXPECT_EQ(count, 4u);
  EXPECT_NE(pyc->find("{depth = 4}"), std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsImplicitMultipleConsumers) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kMultipleConsumers, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_FALSE(bool(plan));
  EXPECT_NE(llvm::toString(plan.takeError()).find("insert ac.broadcast"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, FlatGeneratorPreservesOrderedRepeatedWritesPerOwner) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStatefulFiring, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  QueueBlockPlan &firing =
      *llvm::find_if(plan->blocks, [](const QueueBlockPlan &block) {
        return block.kind == "firing";
      });
  ASSERT_EQ(firing.stateWrites.size(), 1u);
  StateWritePlan secondOwner = firing.stateWrites.front();
  secondOwner.table = "shadow";
  StateWritePlan repeated = firing.stateWrites.front();
  repeated.index = "other_index";
  firing.expressions.push_back(
      {"other_index", "constant", "i2", {}, "", "", "0 : i2"});
  firing.stateWrites.push_back(std::move(secondOwner));
  firing.stateWrites.push_back(std::move(repeated));
  plan->tables.push_back({"shadow", "i8", 2, 0, "table/shadow", "/"});

  auto generated = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef source(*generated);
  const size_t first = source.find(
      "state_table_writes.emplace_back(static_cast<size_t>(state_table_index), "
      "state_table_next)");
  const size_t second =
      source.find("state_table_writes.emplace_back(static_cast<size_t>("
                  "state_table_2_index), state_table_2_next)");
  ASSERT_NE(first, llvm::StringRef::npos);
  ASSERT_NE(second, llvm::StringRef::npos);
  EXPECT_LT(first, second);
  EXPECT_NE(source.find("state_shadow_writes.emplace_back(static_cast<size_t>("
                        "state_shadow_index), state_shadow_next)"),
            llvm::StringRef::npos);
  EXPECT_NE(source.find("std::move(state_table_writes), "
                        "std::move(state_shadow_writes)"),
            llvm::StringRef::npos);
  expectCppCompiles(*generated);
}

TEST(QueueGraphPlanTest,
     BorrowsAggregateTableReadsAndMaterializesOutputsBeforeCommit) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStatefulFiring, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto extracted = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(extracted)) << llvm::toString(extracted.takeError());
  QueueGraphPlan plan = aggregateTableBorrowPlan(std::move(*extracted));
  auto generated = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  llvm::StringRef source(*generated);
  ASSERT_NE(
      source.find("const auto &stored = table_table->at(static_cast<size_t>("
                  "index))"),
      llvm::StringRef::npos)
      << source.str();
  EXPECT_NE(source.find("const auto &nested = stored.nested"),
            llvm::StringRef::npos);
  EXPECT_NE(source.find("auto key = nested.key"), llvm::StringRef::npos);
  EXPECT_NE(source.find("auto updated = stored"), llvm::StringRef::npos);
  EXPECT_EQ(source.find("auto stored = table_table->at"),
            llvm::StringRef::npos);

  std::string executableSource = *generated;
  executableSource.append(R"cpp(
int main() {
  using gfsim::UInt;
  ac_generated::AggregateBorrow model;
  const ac_generated::BorrowEntry initial{
      UInt<1>{1},
      ac_generated::BorrowNested{UInt<16>{7}, UInt<64>{11}, UInt<64>{12}},
      UInt<16>{0}};
  const ac_generated::BorrowEntry replacement{
      UInt<1>{1},
      ac_generated::BorrowNested{UInt<16>{99}, UInt<64>{21}, UInt<64>{22}},
      UInt<16>{0}};
  auto rows = model.dispatch_rows();
  gfsim::SimTable<ac_generated::BorrowEntry> *table = nullptr;
  for (auto &row : rows) {
    auto *object = static_cast<gfsim::SimObject *>(row.object);
    if (row.kind == gfsim::ObjectKind::Memory && object->name() == "table")
      table = dynamic_cast<gfsim::SimTable<ac_generated::BorrowEntry> *>(object);
  }
  if (table == nullptr || !table->initializeEntry(1, initial) ||
      !model.input().proposePush(replacement))
    return 1;
  model.input().doXfer({0, 0});
  auto runTick = [&](unsigned tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  };
  runTick(1);
  runTick(2);
  const auto &values = model.sink_0_values();
  if (values.size() != 1)
    return 2;
  const auto &output = values.front();
  const auto &current = table->at(1);
  return output.nested.key == UInt<16>{7} && output.echo == UInt<16>{7} &&
                 output.nested.lane0 == UInt<64>{11} &&
                 current.nested.key == UInt<16>{99}
             ? 0
             : 3;
}
)cpp");
  expectCppRuns(executableSource);
}

TEST(QueueGraphPlanTest, BorrowsCheckedAggregateReadsWithoutDroppingChecks) {
  constexpr llvm::StringLiteral entryType = "!ac.struct<@types::@CheckedEntry>";
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/CodeGen/"
                           "table-endpoint-value-constraints.mlir",
      &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto extracted = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(extracted)) << llvm::toString(extracted.takeError());
  QueueGraphPlan plan = std::move(*extracted);
  plan.payloads = {{"CheckedEntry",
                    {{"valid", "i1", 1},
                     {"key", "i16", 16},
                     {"lane0", "i64", 64},
                     {"lane1", "i64", 64}}}};
  auto flags = llvm::find_if(plan.tables, [](const TablePlan &table) {
    return table.name == "flags";
  });
  ASSERT_NE(flags, plan.tables.end());
  flags->entryType = entryType.str();
  auto endpoint = llvm::find_if(plan.tableReads, [](const TableReadPlan &read) {
    return read.table == "flags";
  });
  ASSERT_NE(endpoint, plan.tableReads.end());
  auto output = llvm::find_if(plan.queues, [&](const QueuePlan &queue) {
    return queue.name == endpoint->output;
  });
  ASSERT_NE(output, plan.queues.end());
  output->payloadType = entryType.str();

  QueueBlockPlan *readBlock = nullptr;
  QueueExpressionPlan *read = nullptr;
  for (QueueBlockPlan &block : plan.blocks)
    for (QueueExpressionPlan &expression : block.expressions)
      if (expression.kind == "table_get" && expression.table == "flags") {
        readBlock = &block;
        read = &expression;
      }
  ASSERT_NE(readBlock, nullptr);
  ASSERT_NE(read, nullptr);
  const std::string readResult = read->result;
  read->type = entryType.str();
  auto position = llvm::find_if(readBlock->expressions,
                                [&](const QueueExpressionPlan &expression) {
                                  return expression.result == readResult;
                                });
  ASSERT_NE(position, readBlock->expressions.end());
  readBlock->expressions.insert(
      std::next(position),
      {"checked_valid", "get", "i1", {readResult}, "valid"});
  for (std::string &yield : readBlock->yields)
    if (yield == readResult)
      yield = "checked_valid";

  auto generated = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  const std::string borrowed = "const auto &" + readResult +
                               " = table->checkedAt(static_cast<size_t>(item))";
  EXPECT_NE(generated->find(borrowed), std::string::npos);
  EXPECT_NE(generated->find("auto checked_valid = " + readResult + ".valid"),
            std::string::npos);
  EXPECT_EQ(generated->find("auto " + readResult + " = table->checkedAt"),
            std::string::npos);
  expectCppCompiles(*generated);
}

TEST(QueueGraphPlanTest, OwnerWriteExclusionProofUsesBoundedSharedDagKeys) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStatefulFiring, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  QueueBlockPlan &firing =
      *llvm::find_if(plan->blocks, [](const QueueBlockPlan &block) {
        return block.kind == "firing";
      });

  firing.expressions.push_back(
      {"same", "cmp", "i1", {"item", "item"}, "", "eq"});
  std::string previous = "same";
  for (unsigned index = 0; index < 64; ++index) {
    const std::string next = "shared" + std::to_string(index);
    firing.expressions.push_back({next, "and", "i1", {previous, previous}});
    previous = next;
  }
  firing.expressions.push_back(
      {"false_value", "constant", "i1", {}, "", "", "false"});
  firing.expressions.push_back(
      {"opposite", "cmp", "i1", {previous, "false_value"}, "", "eq"});
  firing.stateWrites.front().present = previous;
  StateWritePlan exclusive = firing.stateWrites.front();
  exclusive.present = "opposite";
  firing.stateWrites.push_back(std::move(exclusive));

  auto error = verifyQueueGraphPlan(*plan);
  EXPECT_FALSE(bool(error)) << llvm::toString(std::move(error));
}

TEST(QueueGraphPlanTest, RejectsOutOfRangeConstantTableFiringPlan) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStatefulFiring, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  auto firing = llvm::find_if(plan->blocks, [](const QueueBlockPlan &block) {
    return block.kind == "firing";
  });
  ASSERT_NE(firing, plan->blocks.end());
  auto constant = llvm::find_if(firing->expressions,
                                [](const QueueExpressionPlan &expression) {
                                  return expression.kind == "constant";
                                });
  ASSERT_NE(constant, firing->expressions.end());
  constant->literal = "3 : i2";

  auto error = verifyQueueGraphPlan(*plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("statically safe"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RecomputesBoundedFiringIndexConstraints) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStatefulFiring, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto extracted = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(extracted)) << llvm::toString(extracted.takeError());

  auto configure = [](QueueGraphPlan plan, llvm::StringRef payload,
                      llvm::StringRef index) {
    for (QueuePlan &queue : plan.queues)
      queue.payloadType = payload.str();
    plan.tables.front().entryType = payload.str();
    plan.tables.front().entries = 5;
    plan.tables.front().shape = {5};
    plan.tables.front().axisWidths = {3};
    QueueBlockPlan &firing =
        *llvm::find_if(plan.blocks, [](const QueueBlockPlan &block) {
          return block.kind == "firing";
        });
    firing.stateWrites.front().index = index.str();
    return plan;
  };
  auto firing = [](QueueGraphPlan &plan) -> QueueBlockPlan & {
    return *llvm::find_if(plan.blocks, [](const QueueBlockPlan &block) {
      return block.kind == "firing";
    });
  };
  auto expectAccepted = [&](QueueGraphPlan plan) {
    auto error = verifyQueueGraphPlan(plan);
    ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
  };
  auto expectRejected = [&](QueueGraphPlan plan) {
    auto error = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(error));
    EXPECT_NE(llvm::toString(std::move(error)).find("statically safe"),
              std::string::npos);
  };

  // A u2 root domain is [0,3], which is a safe subset of a five-entry Table.
  QueueGraphPlan u2 = configure(*extracted, "i2", "item");
  expectAccepted(u2);
  auto cpp = generateQueueGraphCpp(u2);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  expectCppCompiles(*cpp);

  // A raw u3 root is [0,7], so the same access must fail closed.
  expectRejected(configure(*extracted, "i3", "item"));

  QueueGraphPlan arithmetic = configure(*extracted, "i3", "sum");
  firing(arithmetic)
      .expressions.push_back({"two_a", "constant", "i3", {}, "", "", "2 : i3"});
  firing(arithmetic)
      .expressions.push_back({"two_b", "constant", "i3", {}, "", "", "2 : i3"});
  firing(arithmetic)
      .expressions.push_back({"sum", "add", "i3", {"two_a", "two_b"}});
  expectAccepted(arithmetic);

  QueueGraphPlan masked = configure(*extracted, "i3", "masked");
  firing(masked).expressions.push_back(
      {"mask", "constant", "i3", {}, "", "", "-4 : i3"});
  firing(masked).expressions.push_back(
      {"masked", "and", "i3", {"item", "mask"}});
  expectAccepted(masked);

  QueueGraphPlan selected = configure(*extracted, "i3", "selected");
  firing(selected).expressions.push_back(
      {"one", "constant", "i3", {}, "", "", "1 : i3"});
  firing(selected).expressions.push_back(
      {"four", "constant", "i3", {}, "", "", "-4 : i3"});
  firing(selected).expressions.push_back(
      {"less", "cmp", "i1", {"item", "four"}, "", "ult"});
  firing(selected).expressions.push_back(
      {"selected", "value_select", "i3", {"less", "one", "four"}});
  expectAccepted(selected);

  QueueGraphPlan priority = configure(*extracted, "i5", "priority");
  firing(priority).expressions.push_back(
      {"priority", "priority_index", "i3", {"item"}, "", "low"});
  expectAccepted(priority);
}

TEST(QueueGraphPlanTest, RecomputesTableEndpointAndObservationConstraints) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/CodeGen/"
                           "table-endpoint-value-constraints.mlir",
      &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto extracted = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(extracted)) << llvm::toString(extracted.takeError());
  auto baseError = verifyQueueGraphPlan(*extracted);
  ASSERT_FALSE(bool(baseError)) << llvm::toString(std::move(baseError));

  auto widenInput = [](QueueGraphPlan &plan, llvm::StringRef name) {
    auto queue = llvm::find_if(plan.queues, [&](const QueuePlan &candidate) {
      return candidate.name == name;
    });
    ASSERT_NE(queue, plan.queues.end());
    queue->payloadType = "i3";
  };
  auto expectRejected = [&](QueueGraphPlan plan, llvm::StringRef queue,
                            llvm::StringRef diagnostic) {
    widenInput(plan, queue);
    auto error = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(error));
    EXPECT_NE(llvm::toString(std::move(error)).find(diagnostic),
              std::string::npos);
  };

  expectRejected(*extracted, "read_input",
                 "Table endpoint address is not statically safe");
  expectRejected(*extracted, "write_input",
                 "Table endpoint address is not statically safe");
  expectRejected(*extracted, "get_input",
                 "Table observation index is not statically safe");
}

TEST(QueueGraphPlanTest, RejectsTableFiringPlanTypeAndOwnershipBypasses) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStatefulFiring, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());

  QueueGraphPlan mismatched = *plan;
  auto output = llvm::find_if(mismatched.queues, [](const QueuePlan &queue) {
    return queue.name == "output";
  });
  ASSERT_NE(output, mismatched.queues.end());
  output->payloadType = "i16";
  auto typeError = verifyQueueGraphPlan(mismatched);
  ASSERT_TRUE(bool(typeError));
  EXPECT_NE(llvm::toString(std::move(typeError)).find("must match"),
            std::string::npos);

  QueueGraphPlan conflicting = *plan;
  conflicting.tableWrites.push_back(
      {"table", "extra", "/", "", "field", {"$entry"}});
  auto ownershipError = verifyQueueGraphPlan(conflicting);
  ASSERT_TRUE(bool(ownershipError));
  EXPECT_NE(
      llvm::toString(std::move(ownershipError)).find("state firing write"),
      std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsForgedFrozenFiringBeforePlanExtraction) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStatefulFiring, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  ac::FiringOp firing;
  module->walk([&](ac::FiringOp candidate) { firing = candidate; });
  ASSERT_TRUE(firing);
  firing->setAttr("handshake",
                  mlir::StringAttr::get(&context, "ready_valid_1x1"));

  auto plan = buildQueueGraphPlan(*module);
  ASSERT_FALSE(bool(plan));
  EXPECT_NE(
      llvm::toString(plan.takeError()).find("failed operation verification"),
      std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsInvalidSharedTableReferenceTargets) {
  for (size_t index = 0; index < 3; ++index) {
    QueueGraphPlan plan = sharedReferencePlan();
    plan.blocks[1].expressions[index].field = "missing";
    auto error = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(error));
    EXPECT_NE(llvm::toString(std::move(error)).find("unknown"),
              std::string::npos);
  }
}

TEST(QueueGraphPlanTest, RejectsInvalidSharedTableReferenceProvenance) {
  for (size_t index = 0; index < 3; ++index) {
    QueueGraphPlan plan = sharedReferencePlan();
    plan.blocks[1].expressions[index].table = "other";
    auto error = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(error));
    EXPECT_NE(llvm::toString(std::move(error)).find("provenance"),
              std::string::npos);
  }
}

TEST(QueueGraphPlanTest, RejectsInvalidSharedTableReferenceFieldTypes) {
  for (size_t index = 0; index < 3; ++index) {
    QueueGraphPlan plan = sharedReferencePlan();
    plan.blocks[1].expressions[index].type = "i8";
    auto error = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(error));
    EXPECT_NE(llvm::toString(std::move(error)).find("field type"),
              std::string::npos);
  }
}

TEST(QueueGraphPlanTest, RejectsInvalidSharedTableWidths) {
  QueueGraphPlan plan = sharedReferencePlan();
  plan.tableMatches[0].resultType = "i3";
  auto matchError = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(matchError));
  EXPECT_NE(llvm::toString(std::move(matchError)).find("table match metadata"),
            std::string::npos);

  plan = sharedReferencePlan();
  plan.tableSelections[0].indexType = "i3";
  auto selectionError = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(selectionError));
  EXPECT_NE(llvm::toString(std::move(selectionError))
                .find("table selection metadata"),
            std::string::npos);
}

TEST(QueueGraphPlanTest,
     MultiSelectionAndQueueLanesRemainCanonicalPlanDimensions) {
  QueueGraphPlan plan = sharedReferencePlan();
  TableSelectionPlan &selection = plan.tableSelections.front();
  selection.count = 2;
  selection.stableId = "issue/first2";
  for (QueueExpressionPlan &expression : plan.blocks[1].expressions)
    if (expression.kind == "table_selection_index_ref" ||
        expression.kind == "table_selection_valid_ref")
      expression.selectionCount = 2;
  QueueExpressionPlan secondIndex{"i1", "table_selection_index_ref", "i2", {}};
  secondIndex.field = selection.name;
  secondIndex.table = selection.table;
  secondIndex.selectionCount = 2;
  secondIndex.laneOrdinal = 1;
  QueueExpressionPlan secondValid{"v1", "table_selection_valid_ref", "i1", {}};
  secondValid.field = selection.name;
  secondValid.table = selection.table;
  secondValid.selectionCount = 2;
  secondValid.laneOrdinal = 1;
  plan.blocks[1].expressions.push_back(std::move(secondIndex));
  plan.blocks[1].expressions.push_back(std::move(secondValid));
  plan.queues.front().lanes = 3;
  plan.queues.front().rate = 2;
  plan.queues.front().depth = 3;
  plan.queues.front().laneOrdinals = {0, 1, 2};
  plan.blocks.front().depths.front() = 3;

  auto json = plan.canonicalJson();
  ASSERT_TRUE(bool(json)) << llvm::toString(json.takeError());
  EXPECT_NE(json->find("\"lanes\":3"), std::string::npos);
  EXPECT_NE(json->find("\"lane_ordinals\":[0,1,2]"), std::string::npos);
  EXPECT_NE(json->find("\"count\":2"), std::string::npos);
  EXPECT_NE(json->find("\"lane_ordinal\":1"), std::string::npos);
}

TEST(QueueGraphPlanTest, GeneratedRoundRobinSelectionAdvancesOnAcceptedFiring) {
  QueueGraphPlan plan = inlineFirstChoicePlan(4, 2);
  plan.system = "shared_round_robin";
  plan.tableMatches = {{"match",
                        "entries",
                        "/",
                        "i4",
                        {{"present", "constant", "i1", {}, "", "", "true"}},
                        "present"}};
  TableSelectionPlan selection{"selection",   "entries", "/", "match",
                               "round_robin", "i2",      {},  ""};
  selection.count = 2;
  selection.stableId = "entries/rr";
  selection.initialCursor = 2;
  plan.tableSelections = {selection};
  QueueBlockPlan &firing = plan.blocks[1];
  QueueExpressionPlan index{
      "selected_index", "table_selection_index_ref", "i2", {}};
  index.field = "selection";
  index.table = "entries";
  index.predicate = "round_robin";
  index.selectionCount = 2;
  QueueExpressionPlan valid{
      "selected_valid", "table_selection_valid_ref", "i1", {}};
  valid.field = "selection";
  valid.table = "entries";
  valid.predicate = "round_robin";
  valid.selectionCount = 2;
  QueueExpressionPlan secondIndex{
      "second_selected_index", "table_selection_index_ref", "i2", {}};
  secondIndex.field = "selection";
  secondIndex.table = "entries";
  secondIndex.predicate = "round_robin";
  secondIndex.selectionCount = 2;
  secondIndex.laneOrdinal = 1;
  QueueExpressionPlan secondValid{
      "second_selected_valid", "table_selection_valid_ref", "i1", {}};
  secondValid.field = "selection";
  secondValid.table = "entries";
  secondValid.predicate = "round_robin";
  secondValid.selectionCount = 2;
  secondValid.laneOrdinal = 1;
  QueueExpressionPlan result{"result",
                             "record_create",
                             "!ac.struct<@types::@Choice4>",
                             {"selected_index", "selected_valid",
                              "second_selected_index",
                              "second_selected_valid"}};
  result.width = 6;
  plan.payloads.front().fields.push_back({"second_index", "i2", 2});
  plan.payloads.front().fields.push_back({"second_valid", "i1", 1});
  firing.expressions = {std::move(index), std::move(valid),
                        std::move(secondIndex), std::move(secondValid),
                        std::move(result)};
  firing.yields = {"result"};
  firing.guard = "selected_valid";
  firing.outputPresence = {{0, "result", "selected_valid"}};

  auto generated = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  EXPECT_NE(generated->find("TableMultiSelectionCache<"), std::string::npos);
  EXPECT_NE(generated->find("selection->accept(epoch)"), std::string::npos);
  expectCppCompiles(*generated);
}

TEST(QueueGraphPlanTest, MultiSelectionTableReadsBecomeOnePrefixTransaction) {
  constexpr llvm::StringLiteral source = R"mlir(
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "multi_read"} {
  ac.table @entries entry i8 entries 4 init 0 owner "/" stable_id "table/entries"
  %mask = ac.table.match @entries predicate {
  ^predicate(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } -> !ac.var<i4>
  %i0, %i1, %v0, %v1 = ac.table.choose @entries %mask : !ac.var<i4>
      count 2 policy #ac<table_selection_policy first>
      stable_id "entries/first2" key {} ->
      !ac.var<i2>, !ac.var<i2>, !ac.var<i1>, !ac.var<i1>
  %first = ac.table.read @entries depth 1 latency 1 address {
  ^address:
    ac.table.yield %i0 : !ac.var<i2>
  } when {
  ^when:
    ac.table.yield %v0 : !ac.var<i1>
  } {ac.name = "first"} -> !ac.queue<i8>
  %second = ac.table.read @entries depth 1 latency 1 address {
  ^address:
    ac.table.yield %i1 : !ac.var<i2>
  } when {
  ^when:
    ac.table.yield %v1 : !ac.var<i1>
  } {ac.name = "second"} -> !ac.queue<i8>
  ac.sink %first {ac.name = "first_sink"} : !ac.queue<i8>
  ac.sink %second {ac.name = "second_sink"} : !ac.queue<i8>
}
)mlir";
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceString<mlir::ModuleOp>(source, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  auto grouped = llvm::find_if(plan->blocks, [](const QueueBlockPlan &block) {
    return block.kind == "table_read_group";
  });
  ASSERT_NE(grouped, plan->blocks.end());
  EXPECT_EQ(grouped->selectionCount, 2u);
  EXPECT_EQ(grouped->outputs.size(), 2u);
  EXPECT_EQ(llvm::count_if(plan->blocks,
                           [](const QueueBlockPlan &block) {
                             return block.kind == "table_read";
                           }),
            0u);
  auto generated = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  EXPECT_NE(generated->find("TableSelectionReadGroup<"), std::string::npos);
  expectCppCompiles(*generated);

  QueueGraphPlan partial = *plan;
  auto partialGroup = llvm::find_if(partial.blocks, [](const auto &block) {
    return block.kind == "table_read_group";
  });
  partialGroup->outputs.pop_back();
  llvm::Error partialError = verifyQueueGraphPlan(partial);
  ASSERT_TRUE(bool(partialError));
  EXPECT_NE(llvm::toString(std::move(partialError)).find("group metadata"),
            std::string::npos);

  QueueGraphPlan ungrouped = *plan;
  auto ungroupedBlock =
      llvm::find_if(ungrouped.blocks, [](const QueueBlockPlan &block) {
        return block.kind == "table_read_group";
      });
  ASSERT_NE(ungroupedBlock, ungrouped.blocks.end());
  ungroupedBlock->kind = "table_read";
  llvm::Error ungroupedError = verifyQueueGraphPlan(ungrouped);
  ASSERT_TRUE(bool(ungroupedError));
  EXPECT_NE(llvm::toString(std::move(ungroupedError))
                .find("exactly one atomic prefix consumer"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, FirstTableChooseUsesSharedScalarPriorityEncoder) {
  struct Case {
    unsigned width;
    unsigned indexWidth;
    uint64_t single;
    uint64_t multiple;
    unsigned expectedSingle;
    unsigned expectedMultiple;
  };
  constexpr Case cases[] = {
      {1, 1, 1, 1, 0, 0},
      {16, 4, uint64_t{1} << 15, (uint64_t{1} << 9) | (uint64_t{1} << 3), 15,
       3},
      {64, 6, uint64_t{1} << 63, (uint64_t{1} << 63) | (uint64_t{1} << 7), 63,
       7},
  };
  for (const Case &testCase : cases) {
    SCOPED_TRACE(testCase.width);
    QueueGraphPlan plan =
        inlineFirstChoicePlan(testCase.width, testCase.indexWidth);
    auto cpp = generateQueueGraphCpp(plan);
    ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
    EXPECT_NE(cpp->find("gfsim::priorityEncode(gfsim::UInt<" +
                        std::to_string(testCase.width) + ">"),
              std::string::npos);
    EXPECT_EQ(cpp->find("for (std::size_t index = 0; index < table"),
              cpp->rfind("for (std::size_t index = 0; index < table"));
    size_t encoders = 0;
    for (size_t offset = 0; (offset = cpp->find("gfsim::priorityEncode(",
                                                offset)) != std::string::npos;
         offset += 22)
      ++encoders;
    EXPECT_EQ(encoders, 1u);
    expectCppCompiles(*cpp);

    const std::string marker = "auto choice_selected_index = ";
    const size_t statementBegin = cpp->find(marker);
    ASSERT_NE(statementBegin, std::string::npos);
    const size_t statementEnd = cpp->find('\n', statementBegin);
    ASSERT_NE(statementEnd, std::string::npos);
    const std::string generatedStatement =
        cpp->substr(statementBegin, statementEnd - statementBegin + 1);
    std::string executable = "#include \"gfsim/priority_encode.h\"\n";
    llvm::raw_string_ostream harness(executable);
    harness << "\nint main() {\n"
            << "  auto choose = [](auto mask) {\n    " << generatedStatement
            << "    return choice_selected_index;\n  };\n"
            << "  auto zero = choose(gfsim::UInt<" << testCase.width
            << ">{0});\n"
            << "  auto single = choose(gfsim::UInt<" << testCase.width << ">{"
            << testCase.single << "ULL});\n"
            << "  auto multiple = choose(gfsim::UInt<" << testCase.width << ">{"
            << testCase.multiple << "ULL});\n"
            << "  return !zero.valid && zero.index == 0 && single.valid && "
               "single.index == "
            << testCase.expectedSingle
            << " && multiple.valid && multiple.index == "
            << testCase.expectedMultiple << " ? 0 : 1;\n}\n";
    harness.flush();
    expectCppRuns(executable);
  }
}

TEST(QueueGraphPlanTest, FusesPureSameSnapshotMatchesAndSharesPredicateDag) {
  EXPECT_TRUE(isEffectFreeTableMatchExpression(
      {"value", "get", "i1", {"entry"}, "valid"}));
  EXPECT_FALSE(isEffectFreeTableMatchExpression(
      {"value", "table_get", "i1", {"index"}, "", "", "", "entries"}));
  EXPECT_FALSE(
      isEffectFreeTableMatchExpression({"value", "slot_get_value", "i1", {}}));
  QueueExpressionPlan nested{"value", "get", "i1", {"entry"}, "valid"};
  nested.nestedExpressions = {{"inner", "constant", "i1", {}, "", "", "true"}};
  EXPECT_FALSE(isEffectFreeTableMatchExpression(nested));
  auto loopCount = [](llvm::StringRef source) {
    size_t count = 0;
    for (size_t offset = 0;
         (offset = source.find("for (std::size_t index = 0; index < ",
                               offset)) != llvm::StringRef::npos;
         offset += 8)
      ++count;
    return count;
  };
  QueueGraphPlan fused = dualInlineMatchPlan();
  auto fusedCpp = generateQueueGraphCpp(fused);
  ASSERT_TRUE(bool(fusedCpp)) << llvm::toString(fusedCpp.takeError());
  EXPECT_EQ(loopCount(*fusedCpp), 1u);
  EXPECT_EQ(llvm::StringRef(*fusedCpp).count("fused_match_"), 4u);
  const size_t fusedBegin = fusedCpp->find("auto [fused_match_");
  ASSERT_NE(fusedBegin, std::string::npos);
  const size_t fusedEnd = fusedCpp->find("}();", fusedBegin);
  ASSERT_NE(fusedEnd, std::string::npos);
  const llvm::StringRef predicates(fusedCpp->data() + fusedBegin,
                                   fusedEnd - fusedBegin);
  EXPECT_EQ(predicates.count("entry == item"), 1u);
  expectCppCompiles(*fusedCpp);

  QueueGraphPlan differentCapture = dualInlineMatchPlan();
  QueueBlockPlan &firing = differentCapture.blocks[1];
  firing.expressions.insert(firing.expressions.begin(),
                            {"other", "constant", "i1", {}, "", "", "false"});
  auto secondMask = llvm::find_if(firing.expressions,
                                  [](const QueueExpressionPlan &expression) {
                                    return expression.result == "second_mask";
                                  });
  ASSERT_NE(secondMask, firing.expressions.end());
  secondMask->operands = {"other"};
  for (QueueExpressionPlan &nested : secondMask->nestedExpressions)
    for (std::string &operand : nested.operands)
      if (operand == "item")
        operand = "other";
  auto fallbackCpp = generateQueueGraphCpp(differentCapture);
  ASSERT_TRUE(bool(fallbackCpp)) << llvm::toString(fallbackCpp.takeError());
  EXPECT_EQ(loopCount(*fallbackCpp), 2u);
  EXPECT_EQ(fallbackCpp->find("fused_match_"), std::string::npos);
  expectCppCompiles(*fallbackCpp);

  QueueGraphPlan snapshot = dualInlineMatchPlan();
  QueueBlockPlan &snapshotFiring = snapshot.blocks[1];
  QueueExpressionPlan &snapshotMask = snapshotFiring.expressions[0];
  snapshotMask.nestedExpressions.insert(
      snapshotMask.nestedExpressions.begin(),
      {{"snapshot_index", "constant", "i2", {}, "", "", "0 : i2"},
       {"snapshot_value",
        "table_get",
        "i1",
        {"snapshot_index"},
        "",
        "",
        "",
        "entries"}});
  QueueExpressionPlan snapshotSet{
      "snapshot", "snapshot_set", "state_reservation", {}};
  snapshotSet.field = "mask";
  snapshotSet.table = "entries";
  snapshotSet.predicate = "complete";
  snapshotFiring.expressions.insert(snapshotFiring.expressions.end() - 1,
                                    std::move(snapshotSet));
  auto snapshotCpp = generateQueueGraphCpp(snapshot);
  ASSERT_TRUE(bool(snapshotCpp)) << llvm::toString(snapshotCpp.takeError());
  EXPECT_EQ(loopCount(*snapshotCpp), 2u);
  EXPECT_EQ(snapshotCpp->find("fused_match_"), std::string::npos);
  EXPECT_NE(snapshotCpp->find("StateReservation snapshot"), std::string::npos);
  expectCppCompiles(*snapshotCpp);

  QueueGraphPlan rootYield = dualInlineMatchPlan();
  for (QueueExpressionPlan &candidate : rootYield.blocks[1].expressions)
    if (candidate.kind == "table_match") {
      candidate.nestedExpressions.clear();
      candidate.nestedYields = {"entry"};
    }
  auto rootYieldCpp = generateQueueGraphCpp(rootYield);
  ASSERT_TRUE(bool(rootYieldCpp)) << llvm::toString(rootYieldCpp.takeError());
  EXPECT_EQ(loopCount(*rootYieldCpp), 1u);
  EXPECT_EQ(llvm::StringRef(*rootYieldCpp).count("fused_match_"), 4u);
  expectCppCompiles(*rootYieldCpp);

  QueueGraphPlan dominatedCapture = dualInlineMatchPlan();
  QueueBlockPlan &dominatedFiring = dominatedCapture.blocks[1];
  dominatedFiring.expressions.insert(
      dominatedFiring.expressions.begin(),
      {"capture", "constant", "i1", {}, "", "", "false"});
  for (QueueExpressionPlan &candidate : dominatedFiring.expressions)
    if (candidate.kind == "table_match") {
      candidate.operands = {"capture"};
      for (QueueExpressionPlan &nested : candidate.nestedExpressions)
        for (std::string &operand : nested.operands)
          if (operand == "item")
            operand = "capture";
    }
  auto dominatedCpp = generateQueueGraphCpp(dominatedCapture);
  ASSERT_TRUE(bool(dominatedCpp)) << llvm::toString(dominatedCpp.takeError());
  EXPECT_EQ(loopCount(*dominatedCpp), 1u);
  expectCppCompiles(*dominatedCpp);

  QueueGraphPlan lateCapture = dualInlineMatchPlan();
  QueueBlockPlan &lateFiring = lateCapture.blocks[1];
  auto lateSecond = llvm::find_if(lateFiring.expressions,
                                  [](const QueueExpressionPlan &candidate) {
                                    return candidate.result == "second_mask";
                                  });
  ASSERT_NE(lateSecond, lateFiring.expressions.end());
  lateFiring.expressions.insert(
      lateSecond, {"late", "constant", "i1", {}, "", "", "false"});
  for (QueueExpressionPlan &candidate : lateFiring.expressions)
    if (candidate.kind == "table_match") {
      candidate.operands = {"late"};
      for (QueueExpressionPlan &nested : candidate.nestedExpressions)
        for (std::string &operand : nested.operands)
          if (operand == "item")
            operand = "late";
    }
  auto lateError = verifyQueueGraphPlan(lateCapture);
  ASSERT_TRUE(bool(lateError));
  llvm::consumeError(std::move(lateError));

  QueueGraphPlan differentTable = dualInlineMatchPlan();
  differentTable.tables.push_back({"other", "i1", 4, 0, "table/other", "/"});
  differentTable.tableReads.push_back(
      {"other", "other_read", "/", "", "other_unused", 1, 1});
  for (QueueExpressionPlan &candidate : differentTable.blocks[1].expressions)
    if (candidate.result == "second_mask" ||
        candidate.result == "second_index" ||
        candidate.result == "second_valid")
      candidate.table = "other";
  auto differentTableCpp = generateQueueGraphCpp(differentTable);
  ASSERT_TRUE(bool(differentTableCpp))
      << llvm::toString(differentTableCpp.takeError());
  EXPECT_EQ(loopCount(*differentTableCpp), 2u);
  EXPECT_EQ(differentTableCpp->find("fused_match_"), std::string::npos);
  expectCppCompiles(*differentTableCpp);

  QueueGraphPlan wide = dualInlineMatchPlan();
  wide.system = "wide_dual_inline_match";
  wide.tables[0].entries = 65;
  for (QueueExpressionPlan &candidate : wide.blocks[1].expressions) {
    if (candidate.kind == "table_match")
      candidate.type = "!ac.value_array<2 x i64>";
    if (candidate.kind == "table_choose_index")
      candidate.type = "i7";
  }
  wide.payloads[0].fields[0] = {"index", "i7", 7};
  wide.payloads[0].fields[2] = {"second_index", "i7", 7};
  wide.blocks[1].expressions.back().width = 16;
  auto wideCpp = generateQueueGraphCpp(wide);
  ASSERT_TRUE(bool(wideCpp)) << llvm::toString(wideCpp.takeError());
  EXPECT_EQ(loopCount(*wideCpp), 3u);
  EXPECT_EQ(llvm::StringRef(*wideCpp).count("[index / 64] |= "), 2u);
  std::string wideExecutable = *wideCpp;
  wideExecutable.append(R"cpp(
int main() {
  using gfsim::UInt;
  ac_generated::WideDualInlineMatch model;
  auto rows = model.dispatch_rows();
  gfsim::SimTable<UInt<1>> *table = nullptr;
  for (auto &row : rows) {
    auto *object = static_cast<gfsim::SimObject *>(row.object);
    if (row.kind == gfsim::ObjectKind::Memory && object->name() == "entries")
      table = dynamic_cast<gfsim::SimTable<UInt<1>> *>(object);
  }
  if (table == nullptr || !table->initializeEntry(64, UInt<1>{1}) ||
      !model.input().proposePush(UInt<1>{1}))
    return 1;
  model.input().doXfer({0, 0});
  for (unsigned tick = 1; tick != 3; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows) row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }
  const auto &values = model.sink_0_values();
  return values.size() == 1 && values[0].valid == UInt<1>{1} &&
                 values[0].index == UInt<7>{64} &&
                 values[0].second_valid == UInt<1>{0} &&
                 values[0].second_index == UInt<7>{0}
             ? 0
             : 2;
}
)cpp");
  expectCppRuns(wideExecutable);
}

TEST(QueueGraphPlanTest,
     MultidimensionalTablePlanEmitsTypedInitAndProjectedSelection) {
  QueueGraphPlan plan = inlineFirstChoicePlan(4, 2);
  TablePlan &table = plan.tables.front();
  table.shape = {2, 2};
  table.axisWidths = {1, 1};
  table.layout = "row_major";
  table.layoutVersion = 1;
  table.schemaId =
      "sha256:4a70f7d8db73203752e325ee25c9904cdc9b1e136078b0955f7b8642c05d560b";
  table.initVersion = 1;
  table.hasTypedSchema = true;
  table.initImage = {{"integer", "i1", "0", {}, {}},
                     {"integer", "i1", "1", {}, {}},
                     {"integer", "i1", "1", {}, {}},
                     {"integer", "i1", "0", {}, {}}};
  QueueExpressionPlan &mask = plan.blocks[1].expressions.front();
  mask.type = "i2";
  mask.domainAxes = {1};
  mask.domainShape = {2};
  mask.domainStrides = {1};
  mask.domainOffset = 2;
  mask.hasDomainProjection = true;

  auto json = plan.canonicalJson();
  ASSERT_TRUE(bool(json)) << llvm::toString(json.takeError());
  EXPECT_NE(json->find("\"shape\":[2,2]"), std::string::npos);
  EXPECT_NE(json->find("\"init_version\":1"), std::string::npos);
  EXPECT_NE(json->find("\"domain_offset\":2"), std::string::npos);

  auto generated = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  EXPECT_NE(generated->find("std::vector<gfsim::UInt<1>>{gfsim::UInt<1>{0}, "
                            "gfsim::UInt<1>{1}, gfsim::UInt<1>{1}, "
                            "gfsim::UInt<1>{0}}"),
            std::string::npos);
  EXPECT_NE(generated->find("TableDomainProjection(4, "), std::string::npos);

  QueueGraphPlan forged = plan;
  forged.tables.front().schemaId = "sha256:forged";
  llvm::Error schemaError = verifyQueueGraphPlan(forged);
  ASSERT_TRUE(bool(schemaError));
  EXPECT_NE(llvm::toString(std::move(schemaError)).find("schema_id"),
            std::string::npos);

  std::string executable = *generated;
  executable.append(R"cpp(
int main() {
  using gfsim::UInt;
  ac_generated::FirstChoice4 model;
  auto rows = model.dispatch_rows();
  gfsim::SimTable<UInt<1>> *table = nullptr;
  for (auto &row : rows) {
    auto *object = static_cast<gfsim::SimObject *>(row.object);
    if (row.kind == gfsim::ObjectKind::Memory && object->name() == "entries")
      table = dynamic_cast<gfsim::SimTable<UInt<1>> *>(object);
  }
  if (table == nullptr || table->at(1) != UInt<1>{1} ||
      table->at(2) != UInt<1>{1} || !model.input().proposePush(UInt<4>{0}))
    return 1;
  model.input().doXfer({0, 0});
  for (unsigned tick = 1; tick != 3; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows) row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }
  const auto &values = model.sink_0_values();
  return values.size() == 1 && values[0].valid == UInt<1>{1} &&
                 values[0].index == UInt<2>{2}
             ? 0
             : 2;
}
)cpp");
  expectCppRuns(executable);
}

TEST(QueueGraphPlanTest,
     MultidimensionalDynamicCoordinateFailsBeforeWriteProposal) {
  QueueGraphPlan plan;
  plan.system = "multidimensional_index";
  plan.queues = {{"input", "i1", "/", 1, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
  TablePlan table{"tiles", "i8", 8, 0, "table/tiles", "/"};
  table.shape = {1, 8};
  table.axisWidths = {1, 3};
  table.layout = "row_major";
  table.layoutVersion = 1;
  table.schemaId =
      "sha256:f463610aee6b6eee51dd35c20fe85d789a9a616536b9c37eab9d54b5ce40f3aa";
  table.hasTypedSchema = true;
  plan.tables.push_back(std::move(table));
  QueueBlockPlan writer{"table_write", "writer", "/", {"input"}, {}};
  writer.table = "tiles";
  writer.writeMode = "replace";
  writer.writeFields = {"$entry"};
  writer.expressions = {
      {"column", "constant", "i3", {}, "", "", "0 : i3"},
      {"flat", "table_index", "i3", {"item", "column"}},
      {"enabled", "constant", "i1", {}, "", "", "true"},
      {"value", "constant", "i8", {}, "", "", "9 : i8"},
  };
  writer.expressions[1].table = "tiles";
  writer.yields = {"flat", "enabled", "value"};
  plan.blocks.push_back(std::move(writer));
  plan.tableWrites = {{"tiles", "writer", "/", "input", "replace", {"$entry"}}};

  auto generated = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  EXPECT_NE(generated->find("coordinate_0 >= 1"), std::string::npos);
  EXPECT_NE(generated->find("std::optional<std::size_t> flat_checked"),
            std::string::npos);

  std::string executable = *generated;
  executable.append(R"cpp(
int main() {
  using gfsim::UInt;
  ac_generated::MultidimensionalIndex model;
  auto rows = model.dispatch_rows();
  if (!model.input().proposePush(UInt<1>{1})) return 1;
  model.input().doXfer({0, 0});
  const gfsim::Epoch epoch{1, 0};
  for (auto &row : rows) row.work(row.object, epoch);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
  auto *writer = static_cast<gfsim::SimObject *>(rows[1].object);
  if (model.input().committedSize() != 1) return 2;
  if (writer->runtimeFailureCode() != "table_index_out_of_range") return 3;
  if (writer->hasPendingCommit()) return 4;
  return 0;
}
)cpp");
  expectCppRuns(executable);
}

TEST(QueueGraphPlanTest, TypedAggregateTableImageEmitsAndResetRestoresIt) {
  QueueGraphPlan plan = inlineFirstChoicePlan(2, 1);
  plan.system = "typed_aggregate_init";
  plan.payloads.push_back(
      {"InitEntry", {{"value", "i8", 8}, {"valid", "i1", 1}}});
  TablePlan &table = plan.tables.front();
  table.entryType = "!ac.struct<@types::@InitEntry>";
  table.shape = {2};
  table.axisWidths = {1};
  table.layout = "row_major";
  table.layoutVersion = 1;
  table.schemaId =
      "sha256:449a42ef009910a4ff74ed0d4f267848682de3a97b81df9045daf2ca9a674d52";
  table.initVersion = 1;
  table.hasTypedSchema = true;
  table.initImage = {
      {"struct",
       table.entryType,
       "",
       {"value", "valid"},
       {{"integer", "i8", "7", {}, {}}, {"integer", "i1", "1", {}, {}}}},
      {"struct",
       table.entryType,
       "",
       {"value", "valid"},
       {{"integer", "i8", "9", {}, {}}, {"integer", "i1", "0", {}, {}}}},
  };

  auto generated = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  EXPECT_NE(generated->find("InitEntry{gfsim::UInt<8>{7}, gfsim::UInt<1>{1}}"),
            std::string::npos);

  std::string executable = *generated;
  executable.append(R"cpp(
int main() {
  using gfsim::UInt;
  ac_generated::TypedAggregateInit model;
  auto rows = model.dispatch_rows();
  gfsim::SimTable<ac_generated::InitEntry> *table = nullptr;
  for (auto &row : rows) {
    auto *object = static_cast<gfsim::SimObject *>(row.object);
    if (row.kind == gfsim::ObjectKind::Memory && object->name() == "entries")
      table = dynamic_cast<gfsim::SimTable<ac_generated::InitEntry> *>(object);
  }
  if (table == nullptr || table->at(0).value != UInt<8>{7} ||
      table->at(0).valid != UInt<1>{1})
    return 1;
  if (!table->proposeWrite(
          99, 0, ac_generated::InitEntry{UInt<8>{42}, UInt<1>{0}},
          gfsim::TableFullEntryMerge<ac_generated::InitEntry>::fields,
          gfsim::TableFullEntryMerge<ac_generated::InitEntry>{},
          gfsim::TableWriteMode::Replace))
    return 2;
  table->doXfer({1, 0});
  if (table->at(0).value != UInt<8>{42}) return 3;
  table->reset();
  return table->at(0).value == UInt<8>{7} &&
                 table->at(0).valid == UInt<1>{1} &&
                 table->at(1).value == UInt<8>{9} &&
                 table->at(1).valid == UInt<1>{0}
             ? 0
             : 4;
}
)cpp");
  expectCppRuns(executable);
}

TEST(QueueGraphPlanTest, KeyedTableChooseRetainsSelectionLoop) {
  auto loopCount = [](llvm::StringRef source) {
    size_t count = 0;
    for (size_t offset = 0;
         (offset = source.find("for (std::size_t index = 0; index < ",
                               offset)) != llvm::StringRef::npos;
         offset += 8)
      ++count;
    return count;
  };
  QueueGraphPlan firstPlan = inlineFirstChoicePlan(16, 4);
  auto firstCpp = generateQueueGraphCpp(firstPlan);
  ASSERT_TRUE(bool(firstCpp)) << llvm::toString(firstCpp.takeError());
  RecordProperty("first16_cpp_bytes", std::to_string(firstCpp->size()));

  for (llvm::StringRef policy : {"min", "max"}) {
    QueueGraphPlan keyedPlan = inlineFirstChoicePlan(16, 4);
    for (QueueExpressionPlan &expression : keyedPlan.blocks[1].expressions) {
      if (expression.kind != "table_choose_index" &&
          expression.kind != "table_choose_valid")
        continue;
      expression.predicate = policy.str();
      expression.nestedExpressions = {
          {"selection_key", "constant", "i8", {}, "", "", "0 : i8"}};
      expression.nestedYields = {"selection_key"};
    }
    auto keyedCpp = generateQueueGraphCpp(keyedPlan);
    ASSERT_TRUE(bool(keyedCpp)) << llvm::toString(keyedCpp.takeError());
    RecordProperty((policy + "16_cpp_bytes").str(),
                   std::to_string(keyedCpp->size()));
    EXPECT_EQ(keyedCpp->find("gfsim::priorityEncode(gfsim::UInt<16>"),
              std::string::npos);
    EXPECT_EQ(loopCount(*keyedCpp), 2u);
    EXPECT_LT(firstCpp->size(), keyedCpp->size());
  }

  QueueGraphPlan snapshotPlan = inlineFirstChoicePlan(16, 4);
  for (QueueExpressionPlan &expression : snapshotPlan.blocks[1].expressions) {
    if (expression.kind != "table_choose_index" &&
        expression.kind != "table_choose_valid")
      continue;
    expression.predicate = "min";
    expression.nestedExpressions = {
        {"snapshot_index", "constant", "i4", {}, "", "", "0 : i4"},
        {"snapshot_value",
         "table_get",
         "i1",
         {"snapshot_index"},
         "",
         "",
         "",
         "entries"},
    };
    expression.nestedYields = {"snapshot_value"};
  }
  QueueExpressionPlan snapshot{
      "snapshot", "snapshot_set", "state_reservation", {}};
  snapshot.field = "selected_index";
  snapshot.table = "entries";
  snapshot.predicate = "complete";
  snapshotPlan.blocks[1].expressions.push_back(std::move(snapshot));
  auto snapshotCpp = generateQueueGraphCpp(snapshotPlan);
  ASSERT_TRUE(bool(snapshotCpp)) << llvm::toString(snapshotCpp.takeError());
  RecordProperty("snapshot16_cpp_bytes", std::to_string(snapshotCpp->size()));
  EXPECT_EQ(snapshotCpp->find("gfsim::priorityEncode(gfsim::UInt<16>"),
            std::string::npos);
  EXPECT_EQ(loopCount(*snapshotCpp), 2u);

  QueueGraphPlan widePlan = inlineFirstChoicePlan(64, 6);
  widePlan.system = "wide_first_choice";
  widePlan.tables[0].entries = 65;
  widePlan.queues[0].payloadType = "i1";
  widePlan.payloads[0].fields[0] = {"index", "i7", 7};
  widePlan.blocks[1].expressions.clear();
  QueueExpressionPlan wideMask{
      "mask", "table_match", "!ac.value_array<2 x i64>", {}};
  wideMask.table = "entries";
  wideMask.nestedExpressions = {
      {"matched", "constant", "i1", {}, "", "", "true"}};
  wideMask.nestedYields = {"matched"};
  widePlan.blocks[1].expressions.push_back(std::move(wideMask));
  QueueExpressionPlan wideIndex{
      "selected_index", "table_choose_index", "i7", {"mask"}};
  wideIndex.table = "entries";
  wideIndex.predicate = "first";
  widePlan.blocks[1].expressions.push_back(wideIndex);
  QueueExpressionPlan wideValid{
      "selected_valid", "table_choose_valid", "i1", {"mask"}};
  wideValid.table = "entries";
  wideValid.predicate = "first";
  widePlan.blocks[1].expressions.push_back(wideValid);
  QueueExpressionPlan wideResult{"result",
                                 "record_create",
                                 widePlan.queues[1].payloadType,
                                 {"selected_index", "selected_valid"}};
  wideResult.width = 8;
  widePlan.blocks[1].expressions.push_back(std::move(wideResult));
  auto wideCpp = generateQueueGraphCpp(widePlan);
  ASSERT_TRUE(bool(wideCpp)) << llvm::toString(wideCpp.takeError());
  RecordProperty("first65_cpp_bytes", std::to_string(wideCpp->size()));
  EXPECT_EQ(wideCpp->find("gfsim::priorityEncode"), std::string::npos);
  EXPECT_EQ(loopCount(*wideCpp), 2u);
}

TEST(QueueGraphPlanTest, VerifiesInlineTableChooseProvenanceAndPairs) {
  auto rejected = [](QueueGraphPlan plan, llvm::StringRef diagnostic) {
    SCOPED_TRACE(diagnostic.str());
    auto error = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(error));
    EXPECT_NE(llvm::toString(std::move(error)).find(diagnostic),
              std::string::npos);
  };

  QueueGraphPlan oversized = inlineFirstChoicePlan(4, 2);
  // An i8 mask could previously select forged bit 7 from a four-entry Table.
  oversized.blocks[1].expressions[0].type = "i8";
  rejected(std::move(oversized), "mask width");

  QueueGraphPlan arbitraryMask = inlineFirstChoicePlan(4, 2);
  arbitraryMask.blocks[1].expressions[0] = {
      "mask", "or", "i4", {"item", "item"}};
  rejected(std::move(arbitraryMask), "same Table match");

  QueueGraphPlan wrongTable = inlineFirstChoicePlan(4, 2);
  wrongTable.tables.push_back({"other", "i1", 4, 0, "table/other", "/"});
  wrongTable.tableReads.push_back(
      {"other", "other_read", "/", "", "other_unused", 1, 1});
  wrongTable.blocks[1].expressions[1].table = "other";
  rejected(std::move(wrongTable), "same Table match");

  QueueGraphPlan badIndex = inlineFirstChoicePlan(4, 2);
  badIndex.blocks[1].expressions[1].type = "i3";
  rejected(std::move(badIndex), "result type");

  QueueGraphPlan badValid = inlineFirstChoicePlan(4, 2);
  badValid.blocks[1].expressions[2].type = "i2";
  rejected(std::move(badValid), "result type");

  QueueGraphPlan badMetadata = inlineFirstChoicePlan(4, 2);
  badMetadata.blocks[1].expressions[1].field = "forged";
  rejected(std::move(badMetadata), "index before valid");

  QueueGraphPlan reversed = inlineFirstChoicePlan(4, 2);
  std::swap(reversed.blocks[1].expressions[1],
            reversed.blocks[1].expressions[2]);
  rejected(std::move(reversed), "index before valid");

  QueueGraphPlan mismatchedPair = inlineFirstChoicePlan(4, 2);
  for (QueueExpressionPlan &expression : mismatchedPair.blocks[1].expressions) {
    if (expression.kind != "table_choose_index" &&
        expression.kind != "table_choose_valid")
      continue;
    expression.predicate = "min";
    expression.nestedExpressions = {
        {"key", "constant", "i8", {}, "", "", "0 : i8"}};
    expression.nestedYields = {"key"};
    if (expression.kind == "table_choose_valid")
      expression.nestedExpressions[0].width = 1;
  }
  rejected(std::move(mismatchedPair), "index before valid");

  auto appendKeyedPair = [](QueueGraphPlan &plan, llvm::StringRef suffix,
                            llvm::StringRef policy, llvm::StringRef literal) {
    QueueExpressionPlan index{
        "index_" + suffix.str(), "table_choose_index", "i2", {"mask"}};
    index.table = "entries";
    index.predicate = policy.str();
    index.nestedExpressions = {{"key_" + suffix.str(),
                                "constant",
                                "i8",
                                {},
                                "",
                                "",
                                literal.str() + " : i8"}};
    index.nestedYields = {"key_" + suffix.str()};
    QueueExpressionPlan valid{
        "valid_" + suffix.str(), "table_choose_valid", "i1", {"mask"}};
    valid.table = index.table;
    valid.predicate = index.predicate;
    valid.nestedExpressions = index.nestedExpressions;
    valid.nestedYields = index.nestedYields;
    plan.blocks[1].expressions.push_back(std::move(index));
    plan.blocks[1].expressions.push_back(std::move(valid));
  };

  QueueGraphPlan independent = inlineFirstChoicePlan(4, 2);
  QueueExpressionPlan repeatedIndex = independent.blocks[1].expressions[1];
  repeatedIndex.result = "repeated_first_index";
  QueueExpressionPlan repeatedValid = independent.blocks[1].expressions[2];
  repeatedValid.result = "repeated_first_valid";
  independent.blocks[1].expressions.push_back(std::move(repeatedIndex));
  independent.blocks[1].expressions.push_back(std::move(repeatedValid));
  appendKeyedPair(independent, "min0", "min", "0");
  appendKeyedPair(independent, "min1", "min", "1");
  appendKeyedPair(independent, "max0", "max", "0");
  auto error = verifyQueueGraphPlan(independent);
  EXPECT_FALSE(bool(error)) << llvm::toString(std::move(error));
}

TEST(QueueGraphPlanTest, InlineMultiSelectionPreservesSegmentedDistinctLanes) {
  QueueGraphPlan plan = inlineFirstChoicePlan(4, 2);
  QueueBlockPlan &firing = plan.blocks[1];
  QueueExpressionPlan index0 = firing.expressions[1];
  QueueExpressionPlan valid0 = firing.expressions[2];
  index0.field = valid0.field = "entries/inline-first2";
  index0.selectionCount = valid0.selectionCount = 2;
  QueueExpressionPlan index1 = index0;
  index1.result = "selected_index_1";
  index1.laneOrdinal = 1;
  QueueExpressionPlan valid1 = valid0;
  valid1.result = "selected_valid_1";
  valid1.laneOrdinal = 1;
  firing.expressions.erase(firing.expressions.begin() + 1,
                           firing.expressions.begin() + 3);
  firing.expressions.insert(firing.expressions.begin() + 1,
                            {std::move(index0), std::move(index1),
                             std::move(valid0), std::move(valid1)});
  QueueExpressionPlan &result = firing.expressions.back();
  result.operands = {"selected_index_1", "selected_valid_1"};

  llvm::Error error = verifyQueueGraphPlan(plan);
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
  auto generated = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(generated)) << llvm::toString(generated.takeError());
  EXPECT_NE(generated->find("if (global_index == choice_"), std::string::npos);
  expectCppCompiles(*generated);
}

TEST(QueueGraphPlanTest, DuplicateKeyedChoicesKeepIndependentSnapshotEffects) {
  QueueGraphPlan plan = inlineFirstChoicePlan(16, 4);
  QueueBlockPlan &firing = plan.blocks[1];
  auto makeKey = [](QueueExpressionPlan &expression) {
    expression.predicate = "min";
    expression.nestedExpressions = {
        {"key_index", "constant", "i4", {}, "", "", "0 : i4"},
        {"key_value", "table_get", "i1", {"key_index"}, "", "", "", "entries"},
    };
    expression.nestedYields = {"key_value"};
  };
  makeKey(firing.expressions[1]);
  makeKey(firing.expressions[2]);
  QueueExpressionPlan secondIndex = firing.expressions[1];
  secondIndex.result = "second_index";
  QueueExpressionPlan secondValid = firing.expressions[2];
  secondValid.result = "second_valid";
  firing.expressions.insert(firing.expressions.begin() + 3,
                            std::move(secondIndex));
  firing.expressions.insert(firing.expressions.begin() + 4,
                            std::move(secondValid));
  QueueExpressionPlan &result = firing.expressions[5];
  result.operands = {"selected_index", "selected_valid", "second_index",
                     "second_valid"};
  result.width = 10;
  plan.payloads[0].fields.push_back({"second_index", "i4", 4});
  plan.payloads[0].fields.push_back({"second_valid", "i1", 1});
  firing.stateReservations = {
      {"entries", "", "selected_index", "selected_valid", "set", {"$entry"}},
      {"entries", "", "second_index", "second_valid", "set", {"$entry"}},
  };
  firing.table = "entries";
  firing.tableIndex = "selected_index";
  firing.tableValue = "selected_valid";
  firing.writeMode = "replace";
  firing.writeFields = {"$entry"};

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_EQ(cpp->find("gfsim::priorityEncode"), std::string::npos);
  EXPECT_NE(cpp->find("StateReservation snapshot_set_0_0"), std::string::npos);
  EXPECT_NE(cpp->find("StateReservation snapshot_set_0_1"), std::string::npos);
  EXPECT_EQ(cpp->find("snapshot_set_0_0 = snapshot_set_0_1"),
            std::string::npos);
  expectCppCompiles(*cpp);
}

TEST(QueueGraphPlanTest, DistinctKeyMetadataGeneratesIndependentScans) {
  QueueGraphPlan plan = inlineFirstChoicePlan(16, 4);
  plan.tables[0].entryType = "i8";
  QueueBlockPlan &firing = plan.blocks[1];
  auto setKey = [](QueueExpressionPlan &expression, llvm::StringRef mask,
                   llvm::StringRef value) {
    expression.predicate = "min";
    expression.nestedExpressions = {
        {"key_index", "constant", "i4", {}, "", "", "0 : i4"},
        {"key_value", "table_get", "i8", {"key_index"}, "", "", "", "entries"},
        {"key_match", "masked_match", "i1", {"key_value"}},
    };
    expression.nestedExpressions.back().mask = mask.str();
    expression.nestedExpressions.back().value = value.str();
    expression.nestedYields = {"key_match"};
  };
  setKey(firing.expressions[1], "0x01", "0x01");
  setKey(firing.expressions[2], "0x01", "0x01");
  QueueExpressionPlan secondIndex = firing.expressions[1];
  secondIndex.result = "second_index";
  setKey(secondIndex, "0x02", "0x02");
  QueueExpressionPlan secondValid = firing.expressions[2];
  secondValid.result = "second_valid";
  setKey(secondValid, "0x02", "0x02");
  firing.expressions.insert(firing.expressions.begin() + 3,
                            std::move(secondIndex));
  firing.expressions.insert(firing.expressions.begin() + 4,
                            std::move(secondValid));
  QueueExpressionPlan &result = firing.expressions[5];
  result.operands = {"selected_index", "selected_valid", "second_index",
                     "second_valid"};
  result.width = 10;
  plan.payloads[0].fields.push_back({"second_index", "i4", 4});
  plan.payloads[0].fields.push_back({"second_valid", "i1", 1});

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  size_t loops = 0;
  for (size_t offset = 0;
       (offset = cpp->find("for (std::size_t index = 0; index < ", offset)) !=
       std::string::npos;
       offset += 8)
    ++loops;
  EXPECT_EQ(loops, 3u);
  EXPECT_EQ(cpp->find("gfsim::priorityEncode"), std::string::npos);
  expectCppCompiles(*cpp);
}

TEST(QueueGraphPlanTest, RejectsMalformedPriorityExpressionPlan) {
  auto makePlan = [] {
    QueueGraphPlan plan;
    plan.system = "priority";
    plan.queues = {{"input", "i3", "/", 1, 1}, {"output", "i3", "/", 1, 1}};
    plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
    QueueBlockPlan transform{"transform", "output", "/", {"input"}, {"output"}};
    transform.expressions = {
        {"v0", "priority_index", "i2", {"item"}, "", "low", ""},
        {"v1", "priority_valid", "i1", {"item"}, "", "low", ""},
    };
    transform.yields = {"item"};
    plan.blocks.push_back(std::move(transform));
    plan.blocks.push_back({"sink", "sink", "/", {"output"}, {}});
    return plan;
  };

  QueueGraphPlan plan = makePlan();
  EXPECT_FALSE(bool(verifyQueueGraphPlan(plan)));

  plan = makePlan();
  plan.blocks[1].expressions[0].predicate = "middle";
  auto predicateError = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(predicateError));
  EXPECT_NE(
      llvm::toString(std::move(predicateError)).find("priority expression"),
      std::string::npos);

  plan = makePlan();
  plan.blocks[1].expressions[0].operands.push_back("item");
  auto arityError = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(arityError));
  EXPECT_NE(llvm::toString(std::move(arityError)).find("priority expression"),
            std::string::npos);

  plan = makePlan();
  plan.blocks[1].expressions[0].type = "i1";
  auto typeError = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(typeError));
  EXPECT_NE(llvm::toString(std::move(typeError)).find("result type"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsMalformedPopcountExpressionPlan) {
  auto makePlan = [] {
    QueueGraphPlan plan;
    plan.system = "popcount";
    plan.queues = {{"input", "i13", "/", 1, 1}, {"output", "i13", "/", 1, 1}};
    plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
    QueueBlockPlan transform{"transform", "output", "/", {"input"}, {"output"}};
    transform.expressions = {{"v0", "popcount", "i4", {"item"}}};
    transform.yields = {"item"};
    plan.blocks.push_back(std::move(transform));
    plan.blocks.push_back({"sink", "sink", "/", {"output"}, {}});
    return plan;
  };

  QueueGraphPlan plan = makePlan();
  EXPECT_FALSE(bool(verifyQueueGraphPlan(plan)));

  plan = makePlan();
  plan.blocks[1].expressions[0].operands.push_back("item");
  auto arityError = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(arityError));
  EXPECT_NE(llvm::toString(std::move(arityError)).find("popcount expression"),
            std::string::npos);

  plan = makePlan();
  plan.blocks[1].expressions[0].type = "i3";
  auto typeError = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(typeError));
  EXPECT_NE(llvm::toString(std::move(typeError)).find("result type"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsMalformedCountZerosExpressionPlan) {
  auto makePlan = [] {
    QueueGraphPlan plan;
    plan.system = "count_leading_zeros";
    plan.queues = {{"input", "i13", "/", 1, 1}, {"output", "i13", "/", 1, 1}};
    plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
    QueueBlockPlan transform{"transform", "output", "/", {"input"}, {"output"}};
    transform.expressions = {{"v0", "count_zeros", "i4", {"item"}}};
    transform.expressions[0].predicate = "leading";
    transform.yields = {"item"};
    plan.blocks.push_back(std::move(transform));
    plan.blocks.push_back({"sink", "sink", "/", {"output"}, {}});
    return plan;
  };

  QueueGraphPlan plan = makePlan();
  EXPECT_FALSE(bool(verifyQueueGraphPlan(plan)));

  plan = makePlan();
  plan.blocks[1].expressions[0].operands.push_back("item");
  auto arityError = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(arityError));
  EXPECT_NE(
      llvm::toString(std::move(arityError)).find("count_zeros expression"),
      std::string::npos);

  plan = makePlan();
  plan.blocks[1].expressions[0].predicate = "middle";
  auto directionError = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(directionError));
  EXPECT_NE(llvm::toString(std::move(directionError)).find("direction"),
            std::string::npos);

  plan = makePlan();
  plan.blocks[1].expressions[0].type = "i3";
  auto typeError = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(typeError));
  EXPECT_NE(llvm::toString(std::move(typeError)).find("result type"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsMalformedUnsignedDivRemExpressionPlan) {
  auto makePlan = [](llvm::StringRef kind) {
    QueueGraphPlan plan;
    plan.system = "unsigned_div_rem";
    plan.queues = {{"input", "i8", "/", 1, 1},
                   {"output", "i8", "/", 1, 1}};
    plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
    QueueBlockPlan transform{"transform", "output", "/", {"input"}, {"output"}};
    transform.expressions = {
        {"divisor", "constant", "i8", {}, "", "", "3 : i8"},
        {"result", kind.str(), "i8", {"item", "divisor"}},
    };
    transform.yields = {"result"};
    plan.blocks.push_back(std::move(transform));
    plan.blocks.push_back({"sink", "sink", "/", {"output"}, {}});
    return plan;
  };

  for (llvm::StringRef kind : {"udiv", "urem"}) {
    QueueGraphPlan plan = makePlan(kind);
    EXPECT_FALSE(bool(verifyQueueGraphPlan(plan)));

    plan = makePlan(kind);
    plan.blocks[1].expressions[1].operands.pop_back();
    auto arityError = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(arityError));
    EXPECT_NE(llvm::toString(std::move(arityError)).find("contract is malformed"),
              std::string::npos);

    plan = makePlan(kind);
    plan.blocks[1].expressions[1].operands[1] = "missing";
    auto missingError = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(missingError));
    EXPECT_NE(llvm::toString(std::move(missingError)).find("share one i1..i64"),
              std::string::npos);

    plan = makePlan(kind);
    plan.blocks[1].expressions[0].type = "i7";
    auto mismatchError = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(mismatchError));
    EXPECT_NE(llvm::toString(std::move(mismatchError)).find("share one i1..i64"),
              std::string::npos);

    plan = makePlan(kind);
    plan.blocks[1].expressions[1].type = "i65";
    auto widthError = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(widthError));
    EXPECT_NE(llvm::toString(std::move(widthError)).find("share one i1..i64"),
              std::string::npos);

    plan = makePlan(kind);
    plan.blocks[1].expressions[0].type = "tuple<i8>";
    plan.blocks[1].expressions[1].type = "tuple<i8>";
    auto aggregateError = verifyQueueGraphPlan(plan);
    ASSERT_TRUE(bool(aggregateError));
    EXPECT_NE(llvm::toString(std::move(aggregateError)).find("share one i1..i64"),
              std::string::npos);
  }
}

TEST(QueueGraphPlanTest, RejectsUnconsumedQueueAsStaticDeadlockRisk) {
  QueueGraphPlan plan;
  plan.system = "unconsumed";
  plan.queues = {{"input", "i8", "/", 1, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("has no consuming block"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsRawQueueCycleOutsideFeedbackBlock) {
  QueueGraphPlan plan;
  plan.system = "cycle";
  plan.queues = {{"a", "i8", "/", 1, 1}, {"b", "i8", "/", 1, 1}};
  plan.blocks.push_back({"transform", "a", "/", {"b"}, {"a"}, {1}, {1}});
  plan.blocks.push_back({"transform", "b", "/", {"a"}, {"b"}, {1}, {1}});
  auto error = verifyQueueGraphPlan(plan);
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error))
                .find("represent stateful loops with ac.feedback"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, ObservationDoesNotConsumeQueue) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kObservationUse, &context);
  ASSERT_TRUE(module);
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->blocks.size(), 3u);
  EXPECT_EQ(plan->blocks[1].kind, "observe");
}

TEST(QueueGraphPlanTest, VerificationLeafRunsInGfsimAndRejectsPycDesign) {
  QueueGraphPlan plan;
  plan.system = "verified";
  plan.queues = {{"input", "i8", "/", 1, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
  QueueBlockPlan expect{"expect", "expect_1", "/", {"input"}, {}};
  expect.expressions = {
      {"v0", "constant", "i8", {}, "", "", "0 : i8"},
      {"v1", "cmp", "i1", {"item", "v0"}, "", "sgt", ""},
  };
  expect.yields = {"v1"};
  expect.message = "positive";
  plan.blocks.push_back(std::move(expect));
  plan.blocks.push_back({"sink", "sink_0", "/", {"input"}, {}});

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("gfsim::QueueExpect<gfsim::UInt<8>, block_0_policy>"),
            std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_FALSE(bool(pyc));
  EXPECT_NE(llvm::toString(pyc.takeError())
                .find("cannot appear in a design hierarchy"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsMalformedPycFeedbackContract) {
  QueueGraphPlan plan;
  plan.system = "bad_feedback";
  plan.queues = {{"input", "i64", "/", 1, 1}, {"output", "i64", "/", 1, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {1}, {1}});
  QueueBlockPlan feedback{"feedback", "output", "/", {"input"}, {"output"}};
  feedback.yields = {"item", "condition"};
  feedback.maxIterations = 0;
  plan.blocks.push_back(std::move(feedback));
  plan.blocks.push_back({"sink", "sink_0", "/", {"output"}, {}});
  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_FALSE(bool(pyc));
  EXPECT_NE(
      llvm::toString(pyc.takeError()).find("feedback contract is unsupported"),
      std::string::npos);
}

TEST(QueueGraphPlanTest, BackendsRejectUncatalogedApplicationBlock) {
  QueueGraphPlan plan;
  plan.system = "bad_dispatch";
  plan.queues = {{"input", "i64", "/", 1, 1}};
  plan.blocks.push_back({"dispatch", "dispatch", "/", {"input"}, {}});
  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_FALSE(bool(cpp));
  EXPECT_NE(llvm::toString(cpp.takeError())
                .find("official opcode has no gfsim lowering: 'dispatch'"),
            std::string::npos);
  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_FALSE(bool(pyc));
  EXPECT_NE(llvm::toString(pyc.takeError())
                .find("official opcode has no PYC lowering: 'dispatch'"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, PreservesReadableRuleSourceAndLocalNames) {
  EXPECT_EQ(legalizeQueueGraphIdentifier("old-value"), "old_value");
  EXPECT_EQ(legalizeQueueGraphIdentifier("class"), "class_");

  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStatefulFiring, &context);
  ASSERT_TRUE(module);
  ac::FiringOp firing;
  module->walk([&](ac::FiringOp candidate) { firing = candidate; });
  ASSERT_TRUE(firing);
  unsigned named = 0;
  firing.getBody().walk([&](mlir::Operation *operation) {
    if (named == 2 || operation->getNumResults() != 1 ||
        !mlir::isa<ac::VarType>(operation->getResult(0).getType()))
      return;
    operation->setAttr("ac.display_name",
                       mlir::StringAttr::get(&context, "old-value"));
    ++named;
  });
  firing->setLoc(mlir::FileLineColLoc::get(
      &context, "/tmp/project/examples/agentic/readable.py", 120, 5));
  ASSERT_TRUE(freezeQueueGraph(*module));
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  const QueueBlockPlan &block =
      *llvm::find_if(plan->blocks, [](const QueueBlockPlan &candidate) {
        return candidate.kind == "firing";
      });
  EXPECT_EQ(block.displayRuleName, "install");
  EXPECT_EQ(block.sourceFile, "examples/agentic/readable.py");
  EXPECT_EQ(block.expressions[0].result, "old_value");
  EXPECT_EQ(block.expressions[1].result, "old_value_2");
  auto cpp = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("struct rule_install_policy"), std::string::npos);
  EXPECT_NE(cpp->find("// source: examples/agentic/readable.py:120:5"),
            std::string::npos);
  EXPECT_NE(cpp->find("state_table_next"), std::string::npos);
  EXPECT_NE(cpp->find("output_output"), std::string::npos);
  EXPECT_NE(cpp->find("rule_condition"), std::string::npos);
  EXPECT_NE(cpp->find("nullopt means this rule performs no transition"),
            std::string::npos);
  expectCppCompiles(*cpp);
}

TEST(QueueGraphPlanTest, DisplayNamesDoNotChangeStructuredFingerprints) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto parse = [&]() {
    return mlir::parseSourceFile<mlir::ModuleOp>(
        ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/Transforms/"
                             "queue-module-freeze.mlir",
        &context);
  };
  auto baseline = parse();
  auto displayed = parse();
  ASSERT_TRUE(baseline);
  ASSERT_TRUE(displayed);
  mlir::SymbolTable preFreezeSymbols(*displayed);
  auto displayedModule = preFreezeSymbols.lookup<ac::ModuleOp>("Increment");
  ASSERT_TRUE(displayedModule);
  displayedModule->setAttr(
      "ac.input_display_names",
      mlir::ArrayAttr::get(&context,
                           {mlir::StringAttr::get(&context, "request")}));
  displayedModule->setAttr(
      "ac.output_display_names",
      mlir::ArrayAttr::get(&context,
                           {mlir::StringAttr::get(&context, "response")}));
  bool labeled = false;
  displayed->walk([&](ac::VarAddOp operation) {
    if (labeled)
      return;
    operation->setAttr("ac.display_name",
                       mlir::StringAttr::get(&context, "next_value"));
    operation->setAttr("ac.source_file",
                       mlir::StringAttr::get(&context, "src/readable.py"));
    operation->setAttr(
        "ac.source_line",
        mlir::IntegerAttr::get(mlir::IntegerType::get(&context, 64), 12));
    operation->setAttr(
        "ac.source_column",
        mlir::IntegerAttr::get(mlir::IntegerType::get(&context, 64), 3));
    labeled = true;
  });
  ASSERT_TRUE(labeled);
  ASSERT_TRUE(freezeQueueGraph(*baseline));
  ASSERT_TRUE(freezeQueueGraph(*displayed));
  mlir::SymbolTable baselineSymbols(*baseline);
  mlir::SymbolTable displayedSymbols(*displayed);
  auto baselineDefinition = baselineSymbols.lookup<ac::ModuleOp>("Increment");
  auto displayedDefinition = displayedSymbols.lookup<ac::ModuleOp>("Increment");
  ASSERT_TRUE(baselineDefinition);
  ASSERT_TRUE(displayedDefinition);
  EXPECT_EQ(baselineDefinition->getAttr("ac.definition_fingerprint"),
            displayedDefinition->getAttr("ac.definition_fingerprint"));
  ac::InstanceOp baselineInstance;
  ac::InstanceOp displayedInstance;
  baseline->walk([&](ac::InstanceOp operation) {
    if (!baselineInstance)
      baselineInstance = operation;
  });
  displayed->walk([&](ac::InstanceOp operation) {
    if (!displayedInstance)
      displayedInstance = operation;
  });
  ASSERT_TRUE(baselineInstance);
  ASSERT_TRUE(displayedInstance);
  EXPECT_EQ(baselineInstance->getAttr("ac.specialization"),
            displayedInstance->getAttr("ac.specialization"));
}

TEST(QueueGraphPlanTest, RejectsMalformedModuleInterfaceDisplayNames) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto rejects = [&](mlir::Attribute attribute, llvm::StringRef expected) {
    auto module = mlir::parseSourceFile<mlir::ModuleOp>(
        ACIR_TEST_SOURCE_DIR
        "/tests/mlir/agentic-circuit/Transforms/queue-module-freeze.mlir",
        &context);
    ASSERT_TRUE(module);
    mlir::SymbolTable symbols(*module);
    auto definition = symbols.lookup<ac::ModuleOp>("Increment");
    ASSERT_TRUE(definition);
    definition->setAttr("ac.input_display_names", attribute);
    ASSERT_TRUE(freezeQueueGraph(*module));
    auto plan = buildQueueGraphPlan(*module);
    ASSERT_FALSE(bool(plan));
    EXPECT_NE(llvm::toString(plan.takeError()).find(expected),
              std::string::npos);
  };
  rejects(mlir::StringAttr::get(&context, "request"),
          "must be an array of strings");
  rejects(mlir::ArrayAttr::get(&context, {}),
          "must match the module interface arity");
  rejects(mlir::ArrayAttr::get(&context, {mlir::StringAttr::get(&context, "")}),
          "must contain only non-empty strings");
}

} // namespace
} // namespace acir::codegen
