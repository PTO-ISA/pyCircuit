#include "acir/CodeGen/Generator.h"
#include "acir/Dialect/ACSim/ACSimDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Index/IR/IndexDialect.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/Error.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace acir::codegen {
namespace {

bool hasError(llvm::Error error) {
  if (!error)
    return false;
  llvm::consumeError(std::move(error));
  return true;
}

llvm::Expected<ModelPlan> fixturePlan(mlir::MLIRContext &context) {
  context
      .loadDialect<acsim::ACSimDialect, mlir::arith::ArithDialect,
                   mlir::cf::ControlFlowDialect, mlir::index::IndexDialect>();
  auto file =
      mlir::parseSourceFile<mlir::ModuleOp>(ACSIM_VALID_TEST_FILE, &context);
  if (!file)
    return llvm::createStringError("failed to parse canonical ACSim fixture");
  return buildModelPlan(*file);
}

llvm::Expected<ModelPlan> processControlFlowPlan(mlir::MLIRContext &context) {
  context
      .loadDialect<acsim::ACSimDialect, mlir::arith::ArithDialect,
                   mlir::cf::ControlFlowDialect, mlir::index::IndexDialect>();
  auto file = mlir::parseSourceFile<mlir::ModuleOp>(
      ACSIM_PROCESS_CONTROL_FLOW_TEST_FILE, &context);
  if (!file)
    return llvm::createStringError("failed to parse process CFG fixture");
  return buildModelPlan(*file);
}

llvm::Expected<ModelPlan> reusableModulePlan(mlir::MLIRContext &context) {
  context.loadDialect<acsim::ACSimDialect>();
  auto file = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/ACSim/reusable-modules.mlir", &context);
  if (!file)
    return llvm::createStringError("failed to parse reusable module fixture");
  return buildModelPlan(*file);
}

const GeneratedFile *findFile(const SourceBundle &bundle,
                              llvm::StringRef path) {
  auto found = std::find_if(
      bundle.files.begin(), bundle.files.end(),
      [&](const GeneratedFile &file) { return file.relativePath == path; });
  return found == bundle.files.end() ? nullptr : &*found;
}

TEST(GeneratorTest, EmitsExactOrderedFileSetAndTypedOwnership) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto bundle = generateModelSources(*plan);
  if (!bundle) {
    ADD_FAILURE() << llvm::toString(bundle.takeError());
    return;
  }

  std::vector<std::string> paths;
  for (const GeneratedFile &file : bundle->files)
    paths.push_back(file.relativePath);
  const std::vector<std::string> expected = {
      "include/generated/dispatch.h",
      "include/generated/model.h",
      "include/generated/modules/Top_s2100000000000000.h",
      "include/generated/processes/tick_s2300000000000000.h",
      "src/generated/main.cpp",
      "src/generated/model.cpp",
      "src/generated/modules/Top_s2100000000000000.cpp",
      "src/generated/processes/tick_s2300000000000000.cpp"};
  EXPECT_EQ(paths, expected);

  const GeneratedFile *header =
      findFile(*bundle, "include/generated/modules/Top_s2100000000000000.h");
  const GeneratedFile *source =
      findFile(*bundle, "src/generated/modules/Top_s2100000000000000.cpp");
  ASSERT_NE(header, nullptr);
  ASSERT_NE(source, nullptr);
  EXPECT_NE(header->content.find(
                "class Top_s2100000000000000 final : public gfsim::Module"),
            std::string::npos);
  EXPECT_NE(header->content.find("gfsim::Fifo fifo_;"), std::string::npos);
  EXPECT_NE(header->content.find("std::array<gfsim::Fifo, 2> lanes_;"),
            std::string::npos);
  EXPECT_NE(source->content.find("fifo_(\"fifo\", nextObjectId++, this)"),
            std::string::npos);
  EXPECT_NE(
      source->content.find("gfsim::Fifo(\"lanes[0]\", nextObjectId++, this)"),
      std::string::npos);
  EXPECT_NE(
      source->content.find("tick_(\"tick\", nextObjectId++, this, lanes_[0])"),
      std::string::npos);
  EXPECT_NE(source->content.find("attachChild(fifo_)"), std::string::npos);
  EXPECT_NE(header->content.find("static_assert(gfsim::StatefulModel<"),
            std::string::npos);
  EXPECT_NE(header->content.find("decltype(auto) out_port()"),
            std::string::npos);
  EXPECT_NE(header->content.find("decltype(auto) memory()"), std::string::npos);
  EXPECT_NE(header->content.find("decltype(auto) out()"), std::string::npos);
  EXPECT_NE(
      source->content.find("bindStatic(lanes_[0].output(), lanes_[1].input())"),
      std::string::npos);
  EXPECT_NE(source->content.find(
                "bindStatic(lanes_[0].initiator(), lanes_[1].target())"),
            std::string::npos);
  EXPECT_NE(source->content.find("return gfsim::is_ready(gfsim::is_ready())"),
            std::string::npos);
  EXPECT_FALSE(hasError(validateSourceBundle(*plan, *bundle)));
}

TEST(GeneratorTest, EmitsReusableNestedModulesWithContextDenseIds) {
  mlir::MLIRContext context;
  auto plan = reusableModulePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));

  auto bundle = generateModelSources(*plan);
  if (!bundle) {
    ADD_FAILURE() << llvm::toString(bundle.takeError());
    return;
  }
  const GeneratedFile *top =
      findFile(*bundle, "src/generated/modules/Top_sa000000000000000.cpp");
  const GeneratedFile *leaf =
      findFile(*bundle, "src/generated/modules/Leaf_s8000000000000000.cpp");
  const GeneratedFile *dispatch =
      findFile(*bundle, "include/generated/dispatch.h");
  ASSERT_NE(top, nullptr);
  ASSERT_NE(leaf, nullptr);
  ASSERT_NE(dispatch, nullptr);
  EXPECT_NE(top->content.find("left_(\"left\", gfsim::kInvalidObjectId, this, "
                              "nextObjectId)"),
            std::string::npos);
  EXPECT_NE(top->content.find("for (auto &element0 : right_)"),
            std::string::npos);
  EXPECT_NE(leaf->content.find("child_(\"child\", nextObjectId++, this)"),
            std::string::npos);
  EXPECT_NE(dispatch->content.find("model.top_.left_.child_"),
            std::string::npos);
  EXPECT_NE(dispatch->content.find("model.top_.right_[1].pulse_"),
            std::string::npos);
}

TEST(GeneratorTest, RecursivelyAttachesMultidimensionalArrays) {
  mlir::MLIRContext context;
  context.loadDialect<acsim::ACSimDialect>();
  auto file = mlir::parseSourceFile<mlir::ModuleOp>(
      ACIR_TEST_SOURCE_DIR "/tests/mlir/agentic-circuit/CodeGen/multidimensional-array.mlir",
      &context);
  ASSERT_TRUE(static_cast<bool>(file));
  auto plan = buildModelPlan(*file);
  ASSERT_TRUE(static_cast<bool>(plan));

  auto bundle = generateModelSources(*plan);
  ASSERT_TRUE(static_cast<bool>(bundle));
  const GeneratedFile *source =
      findFile(*bundle, "src/generated/modules/Top_s2000000000000000.cpp");
  ASSERT_NE(source, nullptr);
  EXPECT_NE(source->content.find("for (auto &element0 : counters_)"),
            std::string::npos);
  EXPECT_NE(source->content.find("for (auto &element1 : element0)"),
            std::string::npos);
  EXPECT_NE(source->content.find("attachChild(element1)"), std::string::npos);
}

TEST(GeneratorTest, RepeatedGenerationIsByteIdentical) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto first = generateModelSources(*plan);
  auto second = generateModelSources(*plan);
  ASSERT_TRUE(static_cast<bool>(first));
  ASSERT_TRUE(static_cast<bool>(second));
  EXPECT_EQ(first->buildFingerprint, second->buildFingerprint);
  ASSERT_EQ(first->files.size(), second->files.size());
  for (size_t index = 0; index < first->files.size(); ++index) {
    EXPECT_EQ(first->files[index].relativePath,
              second->files[index].relativePath);
    EXPECT_EQ(first->files[index].content, second->files[index].content);
    EXPECT_EQ(first->files[index].fingerprint,
              second->files[index].fingerprint);
  }
}

TEST(GeneratorTest, SourceIdentityChangesWithGeneratedTopology) {
  mlir::MLIRContext context;
  auto firstPlan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(firstPlan));
  ModelPlan secondPlan = *firstPlan;
  ASSERT_FALSE(secondPlan.activationEdges.empty());
  secondPlan.activationEdges.erase(secondPlan.activationEdges.begin());

  auto first = generateModelSources(*firstPlan);
  auto second = generateModelSources(secondPlan);
  ASSERT_TRUE(static_cast<bool>(first));
  ASSERT_TRUE(static_cast<bool>(second));
  EXPECT_NE(first->buildFingerprint, second->buildFingerprint);
}

TEST(GeneratorTest, SourceContractRejectsRehashedMutationAndForbiddenTokens) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto bundle = generateModelSources(*plan);
  ASSERT_TRUE(static_cast<bool>(bundle));

  SourceBundle mutated = *bundle;
  mutated.files.back().content.append("// harmless mutation\n");
  mutated.files.back().fingerprint =
      computeFingerprint(mutated.files.back().content);
  EXPECT_TRUE(hasError(validateSourceBundle(*plan, mutated)));

  SourceBundle forbidden = *bundle;
  forbidden.files.back().content.append("// dynamic_cast\n");
  forbidden.files.back().fingerprint =
      computeFingerprint(forbidden.files.back().content);
  auto error = validateSourceBundle(*plan, forbidden);
  if (!error) {
    ADD_FAILURE() << "forbidden generated token was accepted";
    return;
  }
  EXPECT_NE(llvm::toString(std::move(error)).find("forbidden token"),
            std::string::npos);
}

TEST(GeneratorTest, RejectsExecutableCppInStructuredMetadata) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  plan->bindings[1].cppSymbol = "gfsim::Fifo; system(\"bad\")";

  auto bundle = generateModelSources(*plan);
  ASSERT_FALSE(bundle);
  EXPECT_TRUE(hasError(bundle.takeError()));
}

TEST(GeneratorTest, RejectsExecutableThunkInStructuredMetadata) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  plan->bindings[1].entryPoints.work = "fifo_work; inject()";

  auto bundle = generateModelSources(*plan);
  ASSERT_FALSE(bundle);
  EXPECT_TRUE(hasError(bundle.takeError()));
}

TEST(GeneratorTest, EmitsClosedEnumPcProcessWithoutRawFrames) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto bundle = generateModelSources(*plan);
  if (!bundle) {
    ADD_FAILURE() << llvm::toString(bundle.takeError());
    return;
  }

  const GeneratedFile *header =
      findFile(*bundle, "include/generated/processes/tick_s2300000000000000.h");
  const GeneratedFile *source =
      findFile(*bundle, "src/generated/processes/tick_s2300000000000000.cpp");
  ASSERT_NE(header, nullptr);
  ASSERT_NE(source, nullptr);
  EXPECT_NE(header->content.find("enum class Pc : uint8_t"), std::string::npos);
  EXPECT_NE(header->content.find("kFairnessWork = 8"), std::string::npos);
  EXPECT_NE(header->content.find("committed_counter_"), std::string::npos);
  EXPECT_NE(header->content.find("proposed_counter_"), std::string::npos);
  EXPECT_NE(source->content.find("switch (static_cast<Pc>(pc))"),
            std::string::npos);
  EXPECT_NE(source->content.find("ProcessStep::continueAt"), std::string::npos);
  EXPECT_NE(source->content.find("ProcessStep::suspendAt"), std::string::npos);
  EXPECT_NE(source->content.find("ProcessStep::terminate"), std::string::npos);
  EXPECT_NE(source->content.find("ProcessStep::fail(\"invalid_process_pc\")"),
            std::string::npos);
  EXPECT_EQ(source->content.find("std::function"), std::string::npos);
  EXPECT_EQ(source->content.find("co_await"), std::string::npos);
}

TEST(GeneratorTest, EmitsTypedLocalDispatchForMultiBlockProcess) {
  mlir::MLIRContext context;
  auto plan = processControlFlowPlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto bundle = generateModelSources(*plan);
  if (!bundle) {
    ADD_FAILURE() << llvm::toString(bundle.takeError());
    return;
  }
  auto source = std::find_if(bundle->files.begin(), bundle->files.end(),
                             [](const GeneratedFile &file) {
                               return file.relativePath.starts_with(
                                   "src/generated/processes/");
                             });
  auto header = std::find_if(bundle->files.begin(), bundle->files.end(),
                             [](const GeneratedFile &file) {
                               return file.relativePath.starts_with(
                                   "include/generated/processes/");
                             });
  ASSERT_NE(header, bundle->files.end());
  ASSERT_NE(source, bundle->files.end());
  EXPECT_NE(
      header->content.find("inline gfsim::ProcessWake impl_wake_next_delta"),
      std::string::npos);
  EXPECT_NE(source->content.find("enum class Block_entry"), std::string::npos);
  EXPECT_NE(source->content.find("std::optional<"), std::string::npos);
  EXPECT_NE(source->content.find("b1_arg0"), std::string::npos);
  EXPECT_NE(source->content.find("if ("), std::string::npos);
  EXPECT_NE(source->content.find("block1_value0 + block1_value0"),
            std::string::npos);
  EXPECT_EQ(source->content.find("std::function"), std::string::npos);
  EXPECT_EQ(source->content.find("coroutine"), std::string::npos);
}

TEST(GeneratorTest, EmitsSortedExactTimeDomainRuntimeMetadata) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  plan->timeDomains = {{"memory", 4, 0, 2}, {"core", 2, 1, 1}};
  std::sort(plan->timeDomains.begin(), plan->timeDomains.end(),
            [](const TimeDomainPlan &left, const TimeDomainPlan &right) {
              return left.name < right.name;
            });

  auto bundle = generateModelSources(*plan);
  ASSERT_TRUE(static_cast<bool>(bundle)) << llvm::toString(bundle.takeError());
  auto header = std::ranges::find(bundle->files, "include/generated/model.h",
                                  &GeneratedFile::relativePath);
  ASSERT_NE(header, bundle->files.end());
  EXPECT_NE(
      header->content.find("inline const std::array<gfsim::TimeDomainRuntime"),
      std::string::npos);
  EXPECT_NE(header->content.find("TimeDomainRuntime{\"core\", 2, 1, 1}"),
            std::string::npos);
  EXPECT_NE(header->content.find("TimeDomainRuntime{\"memory\", 4, 0, 2}"),
            std::string::npos);
}

TEST(GeneratorTest, EmitsManifestAwareMainWithoutPythonOrMlirDependencies) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto bundle = generateModelSources(*plan);
  ASSERT_TRUE(static_cast<bool>(bundle)) << llvm::toString(bundle.takeError());

  auto main = std::ranges::find(bundle->files, "src/generated/main.cpp",
                                &GeneratedFile::relativePath);
  auto model = std::ranges::find(bundle->files, "include/generated/model.h",
                                 &GeneratedFile::relativePath);
  ASSERT_NE(main, bundle->files.end());
  ASSERT_NE(model, bundle->files.end());
  EXPECT_NE(main->content.find("--run-manifest"), std::string::npos);
  EXPECT_NE(main->content.find("--run-result-stage"), std::string::npos);
  EXPECT_NE(main->content.find("gfsim::loadRunManifest"), std::string::npos);
  EXPECT_NE(main->content.find("gfsim::runGeneratedModel"), std::string::npos);
  EXPECT_NE(model->content.find("void configure(const gfsim::RuntimeLimits"),
            std::string::npos);
  EXPECT_NE(model->content.find("gfsim::TerminationResult run()"),
            std::string::npos);
  EXPECT_NE(model->content.find("std::vector<gfsim::StatSnapshot> statistics"),
            std::string::npos);
  EXPECT_NE(model->content.find(
                "std::span<const gfsim::CommittedEvent> observations"),
            std::string::npos);
  EXPECT_EQ(main->content.find("Python"), std::string::npos);
  EXPECT_EQ(main->content.find("mlir"), std::string::npos);
}

TEST(GeneratorTest, EmitsStaticTypedTraceOwnerInjection) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  ASSERT_FALSE(plan->runtimeObjects.empty());
  plan->runtimeObjects.front().traceOwner = true;

  auto bundle = generateModelSources(*plan);
  ASSERT_TRUE(static_cast<bool>(bundle)) << llvm::toString(bundle.takeError());
  const GeneratedFile *header = findFile(*bundle, "include/generated/model.h");
  const GeneratedFile *source = findFile(*bundle, "src/generated/model.cpp");
  ASSERT_NE(header, nullptr);
  ASSERT_NE(source, nullptr);
  EXPECT_NE(header->content.find("bool loadTrace(gfsim::PtoTraceDocument"),
            std::string::npos);
  EXPECT_NE(source->content.find(
                "return top_.fifo_.loadDocument(std::move(document));"),
            std::string::npos);
}

TEST(GeneratorTest, EmitsClosedTraceOwnerCardinalityFailures) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));

  auto withoutOwner = generateModelSources(*plan);
  ASSERT_TRUE(static_cast<bool>(withoutOwner));
  const GeneratedFile *emptySource =
      findFile(*withoutOwner, "src/generated/model.cpp");
  ASSERT_NE(emptySource, nullptr);
  EXPECT_NE(emptySource->content.find("return document.records.empty();"),
            std::string::npos);

  ASSERT_GE(plan->runtimeObjects.size(), 2u);
  plan->runtimeObjects[0].traceOwner = true;
  plan->runtimeObjects[1].traceOwner = true;
  auto multipleOwners = generateModelSources(*plan);
  ASSERT_TRUE(static_cast<bool>(multipleOwners));
  const GeneratedFile *multipleSource =
      findFile(*multipleOwners, "src/generated/model.cpp");
  ASSERT_NE(multipleSource, nullptr);
  EXPECT_NE(multipleSource->content.find(
                "bool Model::loadTrace(gfsim::PtoTraceDocument document) {\n"
                "  return false;\n"),
            std::string::npos);
}

TEST(GeneratorTest, RejectsMismatchedMultiBlockSuccessorType) {
  mlir::MLIRContext context;
  auto plan = processControlFlowPlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto &branch = std::get<ConditionalBranchPlan>(plan->modules.front()
                                                     .processes.front()
                                                     .states.front()
                                                     .blocks.front()
                                                     .terminator);
  branch.trueArguments.front() = branch.condition;

  auto bundle = generateModelSources(*plan);
  ASSERT_FALSE(bundle);
  EXPECT_NE(llvm::toString(bundle.takeError()).find("ACLOWER-PROCESS-STATE"),
            std::string::npos);
}

TEST(GeneratorTest, RejectsMultiBlockSuccessorValueOutsideSourceBlock) {
  mlir::MLIRContext context;
  auto plan = processControlFlowPlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto &branch = std::get<ConditionalBranchPlan>(plan->modules.front()
                                                     .processes.front()
                                                     .states.front()
                                                     .blocks.front()
                                                     .terminator);
  branch.trueArguments.front() = plan->modules.front()
                                     .processes.front()
                                     .states.front()
                                     .blocks[1]
                                     .arguments.front()
                                     .name;

  auto bundle = generateModelSources(*plan);
  ASSERT_FALSE(bundle);
  EXPECT_NE(llvm::toString(bundle.takeError()).find("ACLOWER-PROCESS-STATE"),
            std::string::npos);
}

TEST(GeneratorTest, EmitsTypedScalarOperationsWithoutRuntimeHelpers) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto &operations = plan->modules[0].processes[0].states[0].operations;
  operations.insert(operations.begin(),
                    ConstantPlan{"constant_value", "i32", 7});
  operations.insert(operations.begin() + 1,
                    ArithmeticPlan{"arith.addi",
                                   {"constant_value", "constant_value"},
                                   {"sum"},
                                   {"i32"},
                                   {}});

  auto bundle = generateModelSources(*plan);
  ASSERT_TRUE(static_cast<bool>(bundle));
  const GeneratedFile *source =
      findFile(*bundle, "src/generated/processes/tick_s2300000000000000.cpp");
  ASSERT_NE(source, nullptr);
  EXPECT_NE(source->content.find("std::int32_t constant_value = 7;"),
            std::string::npos);
  EXPECT_NE(
      source->content.find("auto sum = (constant_value + constant_value);"),
      std::string::npos);
  EXPECT_EQ(source->content.find("acsim_generated::arith_addi"),
            std::string::npos);
}

TEST(GeneratorTest, ResolvesStaticTypeTemplateArgumentsThroughTypePlan) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto &binding = plan->bindings[1];
  binding.cppSymbol = "gfsim::FifoTemplate";
  binding.parameters.push_back(
      {.name = "T",
       .acirType = "!ac.static_type<ac.T>",
       .cppType = "cpp_bool",
       .canonicalValue = "cpp_bool",
       .ordinal = 0,
       .mapping = ParameterMappingKind::TemplateArgument});

  auto bundle = generateModelSources(*plan);
  ASSERT_TRUE(static_cast<bool>(bundle));
  const GeneratedFile *header =
      findFile(*bundle, "include/generated/modules/Top_s2100000000000000.h");
  ASSERT_NE(header, nullptr);
  EXPECT_NE(header->content.find("gfsim::FifoTemplate<bool> fifo_"),
            std::string::npos);
  EXPECT_EQ(header->content.find("gfsim::FifoTemplate<\"cpp_bool\">"),
            std::string::npos);
}

TEST(GeneratorTest, RejectsProcessValueFromAnotherPc) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto &store = std::get<LiveStorePlan>(
      plan->modules[0].processes[0].states[0].operations[1]);
  store.sourceValue = "value_from_another_pc";

  auto bundle = generateModelSources(*plan);
  ASSERT_FALSE(bundle);
  EXPECT_NE(llvm::toString(bundle.takeError()).find("ACLOWER-PROCESS-STATE"),
            std::string::npos);
}

TEST(GeneratorTest, RejectsProcessEffectMismatch) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto &operations = plan->modules[0].processes[0].states[2].operations;
  auto found = std::find_if(operations.begin(), operations.end(), [](auto &op) {
    return std::holds_alternative<InlineCallPlan>(op);
  });
  ASSERT_NE(found, operations.end());
  auto &call = std::get<InlineCallPlan>(*found);
  call.callee = "stateful";

  auto bundle = generateModelSources(*plan);
  ASSERT_FALSE(bundle);
  EXPECT_NE(llvm::toString(bundle.takeError()).find("ACLOWER-PROCESS-STATE"),
            std::string::npos);
}

TEST(GeneratorTest, RejectsProcessSuspendWithoutExactWake) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto &suspend =
      std::get<SuspendPlan>(plan->modules[0].processes[0].states[1].terminator);
  suspend.wakeValue = "missing_wake";

  auto bundle = generateModelSources(*plan);
  ASSERT_FALSE(bundle);
  EXPECT_NE(llvm::toString(bundle.takeError()).find("ACLOWER-PROCESS-STATE"),
            std::string::npos);
}

TEST(GeneratorTest, EmitsDenseDispatchAndCanonicalActivation) {
  mlir::MLIRContext context;
  auto plan = fixturePlan(context);
  ASSERT_TRUE(static_cast<bool>(plan));
  auto bundle = generateModelSources(*plan);
  ASSERT_TRUE(static_cast<bool>(bundle));
  const GeneratedFile *dispatch =
      findFile(*bundle, "include/generated/dispatch.h");
  ASSERT_NE(dispatch, nullptr);

  EXPECT_NE(dispatch->content.find("std::array<gfsim::DispatchRow, 4>"),
            std::string::npos);
  EXPECT_LT(dispatch->content.find("makeDispatchRow(&model.top_.fifo_)"),
            dispatch->content.find("makeDispatchRow(&model.top_.lanes_[0])"));
  EXPECT_LT(dispatch->content.find("makeDispatchRow(&model.top_.lanes_[1])"),
            dispatch->content.find("makeDispatchRow(&model.top_.tick_)"));
  EXPECT_NE(dispatch->content.find("kActivationOffsets"), std::string::npos);
  EXPECT_NE(dispatch->content.find("{0, 1, 4, 5, 6}"), std::string::npos);
  EXPECT_NE(dispatch->content.find("kActivationTargets"), std::string::npos);
  EXPECT_NE(dispatch->content.find("{0, 1, 2, 3, 2, 3}"), std::string::npos);
}

} // namespace
} // namespace acir::codegen
