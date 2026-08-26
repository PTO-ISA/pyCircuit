#include "acir/CodeGen/QueueGraphPlan.h"
#include "acir/CodeGen/QueueGraphGenerator.h"
#include "acir/CodeGen/QueueGraphPyc.h"

#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "mlir/Dialect/DLTI/DLTI.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/Error.h"
#include "gtest/gtest.h"

namespace acir::codegen {
namespace {

constexpr llvm::StringLiteral kQueueGraph = R"mlir(
module attributes {ac.contract_epoch = "0.2", ac.system = "pipeline"} {
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
module attributes {ac.contract_epoch = "0.2", ac.system = "structured"} {
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
module attributes {ac.contract_epoch = "0.2", ac.system = "bad"} {
  %input = ac.source depth 2 latency 1 {ac.name = "input"} : !ac.queue<i64>
  ac.sink %input {ac.name = "left"} : !ac.queue<i64>
  ac.sink %input {ac.name = "right"} : !ac.queue<i64>
}
)mlir";

constexpr llvm::StringLiteral kObservationUse = R"mlir(
module attributes {ac.contract_epoch = "0.2", ac.system = "observed"} {
  %input = ac.source depth 2 latency 1 {ac.name = "input"} : !ac.queue<i64>
  ac.observe %input name "head" : !ac.queue<i64>
  ac.sink %input {ac.name = "sink_0"} : !ac.queue<i64>
}
)mlir";

TEST(QueueGraphPlanTest, ExtractsFrozenQueueIdentitiesAndTopology) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceString<mlir::ModuleOp>(kQueueGraph, &context);
  ASSERT_TRUE(module);
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

TEST(QueueGraphPlanTest, CanonicalJsonIsByteIdenticalAndClosed) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module = mlir::parseSourceString<mlir::ModuleOp>(kQueueGraph, &context);
  ASSERT_TRUE(module);
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  auto first = plan->canonicalJson();
  ASSERT_TRUE(bool(first)) << llvm::toString(first.takeError());
  auto second = plan->canonicalJson();
  ASSERT_TRUE(bool(second)) << llvm::toString(second.takeError());
  EXPECT_EQ(*first, *second);
  EXPECT_NE(first->find("\"contract_epoch\":\"0.2\""), std::string::npos);
  EXPECT_NE(first->find("\"schema\":\"agentic-circuit-queue-graph-plan\""),
            std::string::npos);
  EXPECT_NE(first->find("\"version\":\"0.2\""), std::string::npos);
  EXPECT_NE(first->find("\"name\":\"merged\""), std::string::npos);
}

TEST(QueueGraphPlanTest, RejectsLegacyContractEpochBeforePlanning) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  std::string legacy = kQueueGraph.str();
  size_t epoch = legacy.find("ac.contract_epoch = \"0.2\"");
  ASSERT_NE(epoch, std::string::npos);
  legacy.replace(epoch, std::string("ac.contract_epoch = \"0.2\"").size(),
                 "ac.contract_epoch = \"0.1\"");
  auto module = mlir::parseSourceString<mlir::ModuleOp>(legacy, &context);
  ASSERT_TRUE(module);
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_FALSE(bool(plan));
  EXPECT_NE(llvm::toString(plan.takeError())
                .find("module requires ac.contract_epoch exactly '0.2'"),
            std::string::npos);
}

TEST(QueueGraphPlanTest, ExtractsPayloadAndImmutableVarDag) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, mlir::DLTIDialect>();
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kStructuredTransform, &context);
  ASSERT_TRUE(module);
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
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
  auto source = generateQueueGraphCpp(*plan);
  ASSERT_TRUE(bool(source)) << llvm::toString(source.takeError());
  EXPECT_NE(source->find("gfsim::QueueRoute<std::int64_t, 2"),
            std::string::npos);
  EXPECT_NE(source->find("gfsim::QueueMerge<std::int64_t, 2>"),
            std::string::npos);
  EXPECT_NE(source->find("gfsim::QueueSink<std::int64_t>"), std::string::npos);
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
  EXPECT_NE(cpp->find("std::tuple<std::int64_t> operator()(const "
                      "std::int64_t &item, const std::int64_t &item1)"),
            std::string::npos);
  EXPECT_NE(cpp->find("QueueAtomicTransform<block_0_policy, "
                      "std::tuple<std::int64_t, std::int64_t>, "
                      "std::tuple<std::int64_t>>"),
            std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("pyc.add"), std::string::npos);
  EXPECT_NE(pyc->find("= pyc.wire : i1"), std::string::npos);
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
  EXPECT_NE(cpp->find("gfsim::QueueBarrier<std::tuple<std::uint8_t, "
                      "std::uint16_t>>"),
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
  EXPECT_NE(cpp->find("gfsim::QueueSelect<std::uint8_t, std::uint16_t, 2"),
            std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("select_selector_out_of_range"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.mux"), std::string::npos);
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
  EXPECT_NE(cpp->find("gfsim::QueueReorder<std::int64_t, block_0_policy>"),
            std::string::npos);
  EXPECT_NE(cpp->find(", input_, output_, 4, 0)"), std::string::npos);
  EXPECT_NE(cpp->find("size_t reorder_0_active() const"), std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("pyc.reg"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.ult"), std::string::npos);
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
  EXPECT_NE(cpp->find("gfsim::QueueDependency<std::uint8_t"),
            std::string::npos);
  EXPECT_NE(cpp->find(", input_, output_, 4, 2, 255)"), std::string::npos);
  EXPECT_NE(cpp->find("size_t dependency_0_active() const"), std::string::npos);
  EXPECT_NE(cpp->find("dependency_0_resource_active"), std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("pyc.reg"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.sub"), std::string::npos);
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
  EXPECT_NE(cpp->find("gfsim::QueueCredit<std::uint8_t, block_0_policy>"),
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
  plan.payloads = {
      {"MemoryRequest",
       {{"address", "i4"}, {"write", "i1"}, {"data", "i16"}, {"tag", "i8"}}}};
  constexpr llvm::StringLiteral requestType =
      "!ac.struct<@types::@MemoryRequest>";
  plan.queues = {{"input", requestType.str(), "/", 4, 1},
                 {"output", requestType.str(), "/", 4, 1}};
  plan.blocks.push_back({"source", "input", "/", {}, {"input"}, {4}, {1}});
  QueueBlockPlan memory{"memory",   "output", "/", {"input"},
                        {"output"}, {4},      {1}};
  memory.expressions = {
      {"v0", "get", "i4", {"item"}, "address", "", ""},
      {"v1", "get", "i1", {"item"}, "write", "", ""},
      {"v2", "get", "i16", {"item"}, "data", "", ""},
  };
  memory.yields = {"v0", "v1", "v2"};
  memory.entries = 16;
  memory.init = 0;
  memory.resultField = "data";
  plan.blocks.push_back(std::move(memory));
  plan.blocks.push_back({"sink", "sink_0", "/", {"output"}, {}});

  auto cpp = generateQueueGraphCpp(plan);
  ASSERT_TRUE(bool(cpp)) << llvm::toString(cpp.takeError());
  EXPECT_NE(cpp->find("gfsim::QueueMemory<MemoryRequest, std::uint16_t"),
            std::string::npos);
  EXPECT_NE(cpp->find("result.data = old_data"), std::string::npos);
  EXPECT_NE(cpp->find(", input_, output_, 16, 0)"), std::string::npos);

  auto pyc = generateQueueGraphPyc(plan);
  ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
  EXPECT_NE(pyc->find("pyc.sync_mem"), std::string::npos);
  EXPECT_NE(pyc->find("{depth = 16, name = \"output\"}"), std::string::npos);
  EXPECT_NE(pyc->find("pyc.concat"), std::string::npos);
}

TEST(QueueGraphPlanTest, EmitsQueuePredicateAsPycComparison) {
  struct Case {
    llvm::StringLiteral predicate;
    llvm::StringLiteral opcode;
    bool negated;
  };
  constexpr Case cases[] = {
      {"eq", "pyc.eq", false},   {"ne", "pyc.eq", true},
      {"slt", "pyc.slt", false}, {"sle", "pyc.slt", true},
      {"sgt", "pyc.slt", false}, {"sge", "pyc.slt", true},
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
    auto plan = buildQueueGraphPlan(*module);
    ASSERT_TRUE(bool(plan)) << llvm::toString(plan.takeError());
    auto pyc = generateQueueGraphPyc(*plan);
    ASSERT_TRUE(bool(pyc)) << llvm::toString(pyc.takeError());
    EXPECT_NE(pyc->find(testCase.opcode.str()), std::string::npos);
    EXPECT_NE(pyc->find("pyc.rr_arbiter"), std::string::npos);
    size_t notCount = 0;
    for (size_t offset = 0;
         (offset = pyc->find("pyc.not", offset)) != std::string::npos;
         offset += 7)
      ++notCount;
    // Round-robin selection is now delegated to the qualified PYC arbiter
    // primitive rather than expanded into per-cursor pyc.not/pyc.mux logic.
    EXPECT_EQ(notCount, testCase.negated ? 1u : 0u);
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
  auto plan = buildQueueGraphPlan(*module);
  ASSERT_FALSE(bool(plan));
  EXPECT_NE(llvm::toString(plan.takeError()).find("insert ac.broadcast"),
            std::string::npos);
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
  EXPECT_NE(cpp->find("gfsim::QueueExpect<std::uint8_t, block_0_policy>"),
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

} // namespace
} // namespace acir::codegen
