#include "acir/CodeGen/ModelPlan.h"
#include "acir/Dialect/ACSim/ACSimDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Index/IR/IndexDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MemoryBuffer.h"
#include "gtest/gtest.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace acir::codegen {
namespace {

void loadACSimDialects(mlir::MLIRContext &context) {
  context
      .loadDialect<acsim::ACSimDialect, mlir::arith::ArithDialect,
                   mlir::cf::ControlFlowDialect, mlir::index::IndexDialect>();
}

bool hasError(llvm::Error error) {
  if (!error)
    return false;
  llvm::consumeError(std::move(error));
  return true;
}

std::string stablePlanSummary(const ModelPlan &plan) {
  std::ostringstream output;
  output << plan.modelSymbol << '|' << plan.rootSymbol << '|'
         << plan.frozenAcirFingerprint << '\n';
  for (const TypePlan &type : plan.types)
    output << type.symbol << '|' << static_cast<unsigned>(type.kind) << '|'
           << type.cppType << '|' << type.fingerprint << '\n';
  for (const BindingPlan &binding : plan.bindings)
    output << binding.symbol << '|' << binding.cppSymbol << '|'
           << binding.recordFingerprint << '\n';
  for (const ModulePlan &module : plan.modules) {
    output << module.symbol << '|' << module.className << '|'
           << module.placements.size() << '|' << module.projections.size()
           << '\n';
    for (const ProcessPlan &process : module.processes) {
      output << process.symbol << '|' << process.entryPc << '|'
             << process.fairnessWork << '\n';
      for (const PcStatePlan &state : process.states)
        output << state.ordinal << '|' << state.name << '|'
               << state.operations.size() << '|' << state.terminator.index()
               << '\n';
    }
  }
  for (const RuntimeObjectPlan &object : plan.runtimeObjects)
    output << object.objectId << '|' << object.targetSymbol << '|'
           << object.hierarchyPath << '\n';
  for (const ActivationEdgePlan &edge : plan.activationEdges)
    output << edge.sourceId << "->" << edge.targetId << '\n';
  return output.str();
}

TEST(ModelPlanTest, ExtractsClosedIdentitiesTypesAndDenseRuntimePlan) {
  mlir::MLIRContext context;
  loadACSimDialects(context);
  auto file =
      mlir::parseSourceFile<mlir::ModuleOp>(ACSIM_VALID_TEST_FILE, &context);
  ASSERT_TRUE(file);

  auto plan = buildModelPlan(*file);
  if (!plan) {
    ADD_FAILURE() << llvm::toString(plan.takeError());
    return;
  }

  EXPECT_EQ(plan->modelSymbol, "demo");
  EXPECT_EQ(plan->rootSymbol, "Top");
  EXPECT_EQ(plan->contractEpoch, "0.5");
  EXPECT_EQ(plan->frozenAcirFingerprint,
            "sha256:"
            "0000000000000000000000000000000000000000000000000000000000000001");
  EXPECT_EQ(plan->bindingLockFingerprint,
            "sha256:"
            "0000000000000000000000000000000000000000000000000000000000000002");
  EXPECT_EQ(plan->providerFingerprint,
            "sha256:"
            "0000000000000000000000000000000000000000000000000000000000000003");
  EXPECT_EQ(plan->profileFingerprint,
            "sha256:"
            "0000000000000000000000000000000000000000000000000000000000000004");
  EXPECT_EQ(plan->toolchainFingerprint,
            "sha256:"
            "0000000000000000000000000000000000000000000000000000000000000005");
  EXPECT_EQ(plan->schemaSetFingerprint,
            "sha256:"
            "0000000000000000000000000000000000000000000000000000000000000006");

  ASSERT_EQ(plan->types.size(), 24u);
  EXPECT_EQ(plan->types.front().symbol, "comb_domain");
  EXPECT_EQ(plan->types.front().kind, TypeKind::TimeDomain);
  EXPECT_EQ(plan->types.front().cppType, "gfsim::CombinationalDomain");
  EXPECT_EQ(plan->types.back().symbol, "target");
  EXPECT_EQ(plan->types.back().kind, TypeKind::Role);

  ASSERT_EQ(plan->runtimeObjects.size(), 4u);
  for (size_t index = 0; index < plan->runtimeObjects.size(); ++index) {
    EXPECT_EQ(plan->runtimeObjects[index].objectId, index);
    EXPECT_EQ(plan->runtimeObjects[index].activationId, index);
  }
  EXPECT_EQ(plan->runtimeObjects[0].targetSymbol, "Top::fifo");
  EXPECT_EQ(plan->runtimeObjects[0].hierarchyPath, "Top.fifo");
  EXPECT_EQ(plan->runtimeObjects[1].indices, (std::vector<uint64_t>{0}));
  EXPECT_EQ(plan->runtimeObjects[2].indices, (std::vector<uint64_t>{1}));
  EXPECT_EQ(plan->runtimeObjects[3].objectKind, RuntimeObjectKind::Process);

  const std::vector<ActivationEdgePlan> expectedEdges = {
      {0, 0}, {1, 1}, {1, 2}, {1, 3}, {2, 2}, {3, 3}};
  EXPECT_EQ(plan->activationEdges, expectedEdges);
  EXPECT_FALSE(hasError(validateModelPlan(*plan)));
}

TEST(ModelPlanTest, IdentifiesTraceOwnersFromTypedBindingMetadata) {
  mlir::MLIRContext context;
  loadACSimDialects(context);
  auto input = llvm::MemoryBuffer::getFile(ACSIM_VALID_TEST_FILE);
  ASSERT_TRUE(static_cast<bool>(input));
  std::string source = input.get()->getBuffer().str();
  auto replaceAll = [&](std::string_view from, std::string_view to) {
    for (size_t position = source.find(from); position != std::string::npos;
         position = source.find(from, position + to.size()))
      source.replace(position, from.size(), to);
  };
  replaceAll("gfsim::Fifo", "gfsim::TraceSource");
  replaceAll("fifo.schema", "ac.TraceSource");
  auto file = mlir::parseSourceString<mlir::ModuleOp>(source, &context);
  ASSERT_TRUE(file);

  auto plan = buildModelPlan(*file);
  ASSERT_TRUE(static_cast<bool>(plan)) << llvm::toString(plan.takeError());
  ASSERT_EQ(plan->runtimeObjects.size(), 4u);
  EXPECT_TRUE(plan->runtimeObjects[0].traceOwner);
  EXPECT_TRUE(plan->runtimeObjects[1].traceOwner);
  EXPECT_TRUE(plan->runtimeObjects[2].traceOwner);
  EXPECT_FALSE(plan->runtimeObjects[3].traceOwner);
}

TEST(ModelPlanTest, RejectsInputWithoutOneCanonicalACSimModel) {
  mlir::MLIRContext context;
  loadACSimDialects(context);
  auto file =
      mlir::parseSourceString<mlir::ModuleOp>("builtin.module {}", &context);
  ASSERT_TRUE(file);

  auto plan = buildModelPlan(*file);
  ASSERT_FALSE(plan);
  EXPECT_TRUE(hasError(plan.takeError()));
}

TEST(ModelPlanTest, ValidationRejectsDestructionThatIsNotReverseConstruction) {
  mlir::MLIRContext context;
  loadACSimDialects(context);
  auto file =
      mlir::parseSourceFile<mlir::ModuleOp>(ACSIM_VALID_TEST_FILE, &context);
  ASSERT_TRUE(file);
  auto plan = buildModelPlan(*file);
  if (!plan) {
    ADD_FAILURE() << llvm::toString(plan.takeError());
    return;
  }
  ASSERT_GE(plan->destructionOrder.size(), 2u);

  std::swap(plan->destructionOrder[0], plan->destructionOrder[1]);
  EXPECT_TRUE(hasError(validateModelPlan(*plan)));
}

TEST(ModelPlanTest, ExtractsHierarchyBindingsExpressionsAndProcesses) {
  mlir::MLIRContext context;
  loadACSimDialects(context);
  auto file =
      mlir::parseSourceFile<mlir::ModuleOp>(ACSIM_VALID_TEST_FILE, &context);
  ASSERT_TRUE(file);
  auto plan = buildModelPlan(*file);
  if (!plan) {
    ADD_FAILURE() << llvm::toString(plan.takeError());
    return;
  }

  ASSERT_EQ(plan->bindings.size(), 2u);
  EXPECT_EQ(plan->bindings[0].symbol, "pure");
  EXPECT_EQ(plan->bindings[0].effect, BindingEffect::Pure);
  EXPECT_EQ(plan->bindings[0].header, "gfsim/pure.hpp");
  EXPECT_EQ(plan->bindings[1].symbol, "stateful");
  EXPECT_EQ(plan->bindings[1].effect, BindingEffect::Stateful);
  EXPECT_EQ(plan->bindings[1].cppSymbol, "gfsim::Fifo");
  EXPECT_EQ(plan->bindings[1].entryPoints.work, "fifo_work");
  EXPECT_EQ(plan->bindings[1].ports.size(), 2u);
  EXPECT_EQ(plan->bindings[1].resources.size(), 2u);
  EXPECT_EQ(plan->bindings[1].activationSources.size(), 1u);

  ASSERT_EQ(plan->modules.size(), 1u);
  const ModulePlan &module = plan->modules.front();
  EXPECT_EQ(module.symbol, "Top");
  EXPECT_FALSE(module.className.empty());
  ASSERT_EQ(module.placements.size(), 2u);
  EXPECT_EQ(module.placements[0].symbol, "fifo");
  EXPECT_EQ(module.placements[0].kind, PlacementKind::ExternalStateful);
  EXPECT_EQ(module.placements[1].symbol, "lanes");
  EXPECT_EQ(module.placements[1].kind, PlacementKind::HomogeneousArray);
  EXPECT_EQ(module.placements[1].shape, (std::vector<uint64_t>{2}));
  EXPECT_EQ(module.projections.size(), 6u);
  EXPECT_EQ(module.binds.size(), 3u);
  EXPECT_EQ(module.expressions.size(), 3u);
  EXPECT_EQ(module.exports.size(), 3u);
  EXPECT_EQ(module.returnValues.size(), 3u);

  ASSERT_EQ(module.processes.size(), 1u);
  const ProcessPlan &process = module.processes.front();
  EXPECT_EQ(process.symbol, "tick");
  EXPECT_EQ(process.entryPc, "entry");
  EXPECT_EQ(process.fairnessWork, 8u);
  ASSERT_EQ(process.liveSlots.size(), 1u);
  EXPECT_EQ(process.liveSlots[0].name, "counter");
  EXPECT_EQ(process.liveSlots[0].type, "!acsim.value<@cpp_bool>");
  ASSERT_EQ(process.states.size(), 3u);
  EXPECT_EQ(process.states[0].name, "entry");
  EXPECT_EQ(process.states[0].ordinal, 0u);
  EXPECT_EQ(process.states[0].operations.size(), 3u);
  EXPECT_TRUE(
      std::holds_alternative<ContinuePlan>(process.states[0].terminator));
  EXPECT_EQ(process.states[1].name, "wait");
  EXPECT_TRUE(
      std::holds_alternative<SuspendPlan>(process.states[1].terminator));
  EXPECT_EQ(process.states[2].name, "done");
  EXPECT_TRUE(
      std::holds_alternative<TerminatePlan>(process.states[2].terminator));
  EXPECT_FALSE(hasError(validateModelPlan(*plan)));
}

TEST(ModelPlanTest, PreservesEveryBlockAndBranchOperandInProcessStates) {
  mlir::MLIRContext context;
  loadACSimDialects(context);
  auto file = mlir::parseSourceFile<mlir::ModuleOp>(
      ACSIM_PROCESS_CONTROL_FLOW_TEST_FILE, &context);
  ASSERT_TRUE(file);
  auto plan = buildModelPlan(*file);
  if (!plan) {
    ADD_FAILURE() << llvm::toString(plan.takeError());
    return;
  }

  const ProcessPlan &process = plan->modules.front().processes.front();
  ASSERT_EQ(process.states.front().blocks.size(), 3u);
  EXPECT_TRUE(std::holds_alternative<ConditionalBranchPlan>(
      process.states.front().blocks[0].terminator));
  ASSERT_EQ(process.states.front().blocks[1].arguments.size(), 1u);
  EXPECT_EQ(process.states.front().blocks[1].arguments.front().type, "i32");
}

TEST(ModelPlanTest, RejectsBranchOutsideClosedPcBlockSet) {
  mlir::MLIRContext context;
  loadACSimDialects(context);
  auto file = mlir::parseSourceFile<mlir::ModuleOp>(
      ACSIM_PROCESS_CONTROL_FLOW_TEST_FILE, &context);
  ASSERT_TRUE(file);
  auto plan = buildModelPlan(*file);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto &branch = std::get<ConditionalBranchPlan>(plan->modules.front()
                                                     .processes.front()
                                                     .states.front()
                                                     .blocks.front()
                                                     .terminator);
  branch.trueBlock = 99;

  EXPECT_TRUE(hasError(validateModelPlan(*plan)));
}

TEST(ModelPlanTest, ValidationRejectsTransitionOutsideClosedPcSet) {
  mlir::MLIRContext context;
  loadACSimDialects(context);
  auto file =
      mlir::parseSourceFile<mlir::ModuleOp>(ACSIM_VALID_TEST_FILE, &context);
  ASSERT_TRUE(file);
  auto plan = buildModelPlan(*file);
  if (!plan) {
    ADD_FAILURE() << llvm::toString(plan.takeError());
    return;
  }
  auto &transition = std::get<ContinuePlan>(
      plan->modules[0].processes[0].states[0].terminator);
  transition.targetPc = "outside";

  EXPECT_TRUE(hasError(validateModelPlan(*plan)));
}

TEST(ModelPlanTest, IndependentCanonicalParsesProduceIdenticalOwnedPlans) {
  mlir::MLIRContext firstContext;
  mlir::MLIRContext secondContext;
  loadACSimDialects(firstContext);
  loadACSimDialects(secondContext);
  auto firstFile = mlir::parseSourceFile<mlir::ModuleOp>(ACSIM_VALID_TEST_FILE,
                                                         &firstContext);
  auto secondFile = mlir::parseSourceFile<mlir::ModuleOp>(ACSIM_VALID_TEST_FILE,
                                                          &secondContext);
  ASSERT_TRUE(firstFile);
  ASSERT_TRUE(secondFile);
  auto first = buildModelPlan(*firstFile);
  auto second = buildModelPlan(*secondFile);
  if (!first || !second) {
    if (!first)
      ADD_FAILURE() << llvm::toString(first.takeError());
    if (!second)
      ADD_FAILURE() << llvm::toString(second.takeError());
    return;
  }

  EXPECT_EQ(stablePlanSummary(*first), stablePlanSummary(*second));
}

} // namespace
} // namespace acir::codegen
