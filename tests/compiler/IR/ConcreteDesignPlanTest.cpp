#include "Compiler/ConcreteDesignPlan.h"
#include "Compiler/HardwareEmitCommon.h"
#include "Compiler/StructuralCppBackend.h"
#include "Compiler/VerilogBackend.h"
#include "Compiler/VerilogEmissionPlan.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/FinalHardwarePackage.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "gtest/gtest.h"

#include <limits>

namespace {
namespace ac = acir::ac;
namespace compiler = acir::compiler;

class ConcreteDesignPlanTest : public ::testing::Test {
protected:
  mlir::MLIRContext context;
  mlir::OwningOpRef<mlir::ModuleOp> package;

  void SetUp() override {
    context.loadDialect<ac::ACIRDialect>();
    package = mlir::parseSourceFile<mlir::ModuleOp>(
        PYCIRCUIT_TEST_CONCRETE_PLAN_INPUT, &context);
    ASSERT_TRUE(package);
    mlir::Builder builder(&context);
    (*package)->setAttr("ac.stage", builder.getStringAttr("final"));
    (*package)->setAttr("hwir.hardware_only", builder.getUnitAttr());
  }

  mlir::OwningOpRef<mlir::ModuleOp> parseFrom(const char *path) {
    auto parsed = mlir::parseSourceFile<mlir::ModuleOp>(path, &context);
    if (parsed) {
      mlir::Builder builder(&context);
      (*parsed)->setAttr("ac.stage", builder.getStringAttr("final"));
      (*parsed)->setAttr("hwir.hardware_only", builder.getUnitAttr());
    }
    return parsed;
  }

  mlir::OwningOpRef<mlir::ModuleOp> parseHWIRFrom(const char *path) {
    auto parsed = parseFrom(path);
    if (!parsed)
      return {};
    // Production Structural C++ consumes the independently verified HWIR
    // package, including serialized output-dependency facts.
    mlir::PassManager passes(&context);
    passes.addPass(acir::createFinalACIRToHWIRPass());
    if (mlir::failed(passes.run(*parsed)))
      return {};
    return parsed;
  }
};

TEST_F(ConcreteDesignPlanTest,
       BuildsOneClosedSpecializationWithConcretePortsAndChildren) {
  ac::HardwareAnalysis analysis(*package);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  ASSERT_TRUE(
      mlir::succeeded(compiler::verifyStructuralCppInput(*package, analysis)));

  auto plan = compiler::buildConcreteDesignPlan(*package, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  ASSERT_EQ(plan->modules.size(), 1u);
  ASSERT_EQ(plan->rootModule, 0u);

  compiler::ConcreteModulePlan &top = plan->modules.front();
  EXPECT_EQ(top.key.definition.getSymName(), "top");
  EXPECT_TRUE(top.cppName.starts_with("top__h"));
  ASSERT_EQ(top.inputs.size(), 5u);
  EXPECT_EQ(top.inputs[0].type.packedWidth, 1u);
  EXPECT_EQ(top.inputs[3].type.packedWidth, 8u);
  ASSERT_EQ(top.outputs.size(), 2u);
  EXPECT_EQ(top.outputs[0].type.packedWidth, 8u);
  EXPECT_EQ(top.outputs[1].type.packedWidth, 8u);

  // Values are planned once per specialization. Inputs occupy the first IDs;
  // operation results follow body/result order, with deterministic local names.
  ASSERT_GE(top.values.size(), top.inputs.size());
  for (auto [id, value] : llvm::enumerate(top.values)) {
    EXPECT_EQ(value.id, id);
    EXPECT_TRUE(value.cppName == "input_" + std::to_string(id) ||
                value.cppName == "value_" + std::to_string(id));
    EXPECT_GT(value.type.packedWidth, 0u);
  }
  for (auto [port, input] : llvm::enumerate(top.inputs)) {
    EXPECT_EQ(input.value, port);
    EXPECT_EQ(top.values[input.value].kind,
              compiler::ConcreteValueKind::ModuleInput);
    EXPECT_EQ(top.values[input.value].type.packedWidth, input.type.packedWidth);
  }
  for (const compiler::ConcretePortPlan &output : top.outputs) {
    ASSERT_LT(output.value, top.values.size());
    EXPECT_EQ(top.values[output.value].type.packedWidth,
              output.type.packedWidth);
  }

  ASSERT_EQ(top.children.size(), 2u);
  for (const compiler::ConcreteChildPlan &child : top.children) {
    EXPECT_EQ(child.kind, compiler::ConcreteChildKind::Primitive);
    EXPECT_EQ(child.primitiveKind.getValue(), "dffe");
    EXPECT_FALSE(child.moduleIndex.has_value());
    EXPECT_FALSE(child.queue.has_value());
  }
  EXPECT_EQ(top.children[0].instanceName.getValue(), "left");
  EXPECT_EQ(top.children[1].instanceName.getValue(), "right");

  // The concrete schedule retains HardwareAnalysis' verified dependency
  // order. Construction-only analysis objects are deliberately released
  // before the plan reaches the renderer.
  EXPECT_TRUE(top.schedule.schedule.empty());
  EXPECT_TRUE(top.schedule.dependencies.empty());
  EXPECT_FALSE(plan->dependencies.nodes.empty());
  EXPECT_TRUE(plan->dependencies.edges.empty());
  ASSERT_FALSE(top.concreteSchedule.steps.empty());
  EXPECT_EQ(top.concreteSchedule.steps.back().kind,
            compiler::ConcreteScheduleStepKind::MaterializeOutput);
  unsigned materializedOutputs = 0;
  for (const compiler::ConcreteScheduleStep &step :
       top.concreteSchedule.steps) {
    for (compiler::ConcreteValueId id : step.inputs)
      EXPECT_LT(id, top.values.size());
    for (compiler::ConcreteValueId id : step.outputs)
      EXPECT_LT(id, top.values.size());
    if (step.kind == compiler::ConcreteScheduleStepKind::MaterializeOutput) {
      ASSERT_TRUE(step.portIndex.has_value());
      ASSERT_EQ(step.inputs.size(), 1u);
      EXPECT_EQ(step.inputs[0], top.outputs[*step.portIndex].value);
      ++materializedOutputs;
    }
  }
  EXPECT_EQ(materializedOutputs, top.outputs.size());

  // The root protocol records the global Check branch once. Module-local
  // lifecycle lists only direct children; recursive module calls own their
  // descendants, preserving hierarchy during Xfer and Discard.
  ASSERT_EQ(plan->lifecycle.work.size(), 1u);
  ASSERT_EQ(plan->lifecycle.check.size(), 1u);
  ASSERT_EQ(plan->lifecycle.xfer.size(), 1u);
  ASSERT_EQ(plan->lifecycle.discard.size(), 1u);
  EXPECT_EQ(plan->lifecycle.work[0].kind,
            compiler::ConcreteLifecycleStepKind::InvokeRootWork);
  EXPECT_EQ(plan->lifecycle.check[0].kind,
            compiler::ConcreteLifecycleStepKind::WholeSystemCheck);
  EXPECT_EQ(plan->lifecycle.xfer[0].kind,
            compiler::ConcreteLifecycleStepKind::InvokeRootXfer);
  EXPECT_EQ(plan->lifecycle.discard[0].kind,
            compiler::ConcreteLifecycleStepKind::InvokeRootDiscard);
  EXPECT_EQ(plan->lifecycle.work[0].moduleIndex, plan->rootModule);

  ASSERT_EQ(top.lifecycle.xfer.size(), top.children.size());
  ASSERT_EQ(top.lifecycle.discard.size(), top.children.size());
  for (auto [childIndex, step] : llvm::enumerate(top.lifecycle.xfer)) {
    EXPECT_EQ(step.kind,
              compiler::ConcreteLifecycleStepKind::InvokePrimitiveXfer);
    ASSERT_TRUE(step.childIndex.has_value());
    EXPECT_EQ(*step.childIndex, childIndex);
    EXPECT_EQ(step.moduleIndex, plan->rootModule);
  }
  for (const compiler::ConcreteLifecycleStep &step : top.lifecycle.discard)
    EXPECT_EQ(step.kind,
              compiler::ConcreteLifecycleStepKind::InvokePrimitiveDiscard);
}

TEST_F(ConcreteDesignPlanTest, BuildsConcreteVerilogOccurrencePlans) {
  ac::HardwareAnalysis analysis(*package);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  compiler::HardwareEmitContext emitter(*package, analysis);
  ASSERT_TRUE(mlir::succeeded(emitter.prepare()));
  auto top = mlir::cast<ac::ModuleOp>(analysis.lookupDefinition("top"));

  auto modulePlan = compiler::buildVerilogModuleEmissionPlan(emitter, top);
  ASSERT_TRUE(mlir::succeeded(modulePlan));
  EXPECT_TRUE(modulePlan->parameters.empty());
  ASSERT_EQ(modulePlan->ports.size(), 7u);
  const llvm::StringRef expectedPortNames[] = {"clk",  "rst", "en", "d",
                                               "init", "qa",  "qb"};
  for (auto [index, port] : llvm::enumerate(modulePlan->ports)) {
    EXPECT_EQ(port.name, expectedPortNames[index]);
    EXPECT_EQ(port.direction, index < 5
                                  ? compiler::VerilogPortDirection::Input
                                  : compiler::VerilogPortDirection::Output);
    EXPECT_FALSE(port.type.empty());
  }
  ASSERT_EQ(modulePlan->outputAssignments.size(), 2u);
  EXPECT_EQ(modulePlan->outputAssignments[0].name, "qa");
  EXPECT_EQ(modulePlan->outputAssignments[1].name, "qb");
  auto yield = mlir::cast<ac::YieldOp>(top.getBody().front().back());
  EXPECT_EQ(modulePlan->outputAssignments[0].value, yield.getValues()[0]);
  EXPECT_EQ(modulePlan->outputAssignments[1].value, yield.getValues()[1]);
  ASSERT_EQ(modulePlan->occurrences.size(), 2u);
  EXPECT_TRUE(modulePlan->queues.empty());

  auto wrapperPlan = compiler::buildVerilogWrapperEmissionPlan(
      emitter, top, emitter.names(top).rtl);
  ASSERT_TRUE(mlir::succeeded(wrapperPlan));
  ASSERT_EQ(wrapperPlan->ports.size(), 7u);
  ASSERT_EQ(wrapperPlan->connections.size(), 7u);
  EXPECT_TRUE(wrapperPlan->calleeParameters.empty());
  for (auto [index, port] : llvm::enumerate(wrapperPlan->ports))
    EXPECT_EQ(port.name, expectedPortNames[index]);
  for (auto [index, connection] : llvm::enumerate(wrapperPlan->connections)) {
    EXPECT_EQ(connection.name, expectedPortNames[index]);
    EXPECT_EQ(connection.expression, expectedPortNames[index]);
  }

  llvm::StringMap<const compiler::VerilogOccurrencePlan *> byName;
  for (const auto &entry : modulePlan->occurrences)
    byName.try_emplace(entry.second.instanceName, &entry.second);
  for (llvm::StringRef name : {"left", "right"}) {
    auto found = byName.find(name);
    ASSERT_NE(found, byName.end());
    const compiler::VerilogOccurrencePlan &plan = *found->second;
    EXPECT_TRUE(plan.primitive);
    EXPECT_FALSE(plan.collection);
    EXPECT_EQ(plan.moduleName, "dffe");
    ASSERT_EQ(plan.dataPorts.size(), 6u);
    ASSERT_EQ(plan.parameters.size(), 1u);
    ASSERT_TRUE(plan.parameters[0].name.has_value());
    EXPECT_EQ(*plan.parameters[0].name, "T");
    EXPECT_EQ(plan.parameters[0].value, "logic [(8)-1:0]");
    EXPECT_FALSE(plan.parameters[0].managed);
    EXPECT_TRUE(plan.managedPorts.empty());
    EXPECT_TRUE(plan.lifecycleWires.empty());
    const llvm::StringRef expectedNames[] = {"clk", "rst",  "en",
                                             "d",   "init", "q"};
    for (auto [ordinal, port] : llvm::enumerate(plan.dataPorts)) {
      EXPECT_EQ(port.name, expectedNames[ordinal]);
      EXPECT_EQ(port.source, ordinal < 5
                                 ? compiler::VerilogValueSource::Operand
                                 : compiler::VerilogValueSource::Result);
      EXPECT_EQ(port.ordinal, ordinal < 5 ? ordinal : 0u);
      EXPECT_TRUE(port.collectionWidth.empty());
    }
  }
}

TEST_F(ConcreteDesignPlanTest,
       VerilogSourceShardsPreserveWholeModuleBoundaries) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_DEDUP_CHAIN);
  ASSERT_TRUE(parsed);
  ac::markHardwareOnlyPackage(*parsed);
  mlir::PassManager passes(&context);
  passes.addPass(acir::createFinalACIRToHWIRPass());
  ASSERT_TRUE(mlir::succeeded(passes.run(*parsed)));
  auto plan = compiler::prepareVerilog(*parsed);
  ASSERT_TRUE(mlir::succeeded(plan));

  // One byte is deliberately below either rendered module. This exercises the
  // same partitioner as the production 1 MiB target without a giant fixture.
  auto parts = compiler::emitVerilog(*plan, 1);
  ASSERT_TRUE(mlir::succeeded(parts));
  ASSERT_EQ(parts->sourceGroups.size(), 2u);
  EXPECT_EQ(parts->sourceGroups[0].path, "sources/source_unit_0_0.sv");
  EXPECT_EQ(parts->sourceGroups[1].path, "sources/source_unit_0_1.sv");
  EXPECT_EQ(parts->sourceGroups[0].sourceOwner,
            parts->sourceGroups[1].sourceOwner);

  llvm::StringSet<> definitions;
  for (const auto &shard : parts->sourceGroups) {
    ASSERT_EQ(shard.definitions.size(), 1u);
    EXPECT_TRUE(
        definitions.insert(shard.definitions.front().getValue()).second);
    EXPECT_EQ(llvm::StringRef(shard.text).count("module "), 1u);
    EXPECT_EQ(llvm::StringRef(shard.text).count("endmodule"), 1u);
    // Oversized modules remain intact rather than violating the target by
    // cutting their SystemVerilog text.
    EXPECT_GT(shard.text.size(), 1u);
  }
  EXPECT_EQ(definitions.size(), 2u);
}

TEST_F(ConcreteDesignPlanTest, BuildsConcreteVerilogModuleParameters) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_VERILOG_GENERIC_INPUT);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  compiler::HardwareEmitContext emitter(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(emitter.prepare()));
  auto family = mlir::cast<ac::ModuleOp>(analysis.lookupDefinition("family"));

  auto plan = compiler::buildVerilogModuleEmissionPlan(emitter, family);
  ASSERT_TRUE(mlir::succeeded(plan));
  ASSERT_EQ(plan->parameters.size(), 3u);
  EXPECT_EQ(plan->parameters[0],
            "parameter longint signed pyc_parameter_N = 1");
  EXPECT_EQ(plan->parameters[1],
            "parameter longint signed pyc_parameter_I = 1");
  EXPECT_EQ(plan->parameters[2],
            "parameter type pyc_parameter_T = logic [0:0]");
}

TEST_F(ConcreteDesignPlanTest, BuildsConcreteVerilogCombinationalPlans) {
  auto check = [&](const char *path, size_t expected,
                   llvm::ArrayRef<compiler::VerilogCombinationalKind> kinds) {
    auto parsed = parseFrom(path);
    ASSERT_TRUE(parsed);
    ac::HardwareAnalysis analysis(*parsed);
    ASSERT_TRUE(mlir::succeeded(analysis.verify()));
    compiler::HardwareEmitContext emitter(*parsed, analysis);
    ASSERT_TRUE(mlir::succeeded(emitter.prepare()));
    auto top = mlir::cast<ac::ModuleOp>(analysis.lookupDefinition("top"));
    auto plan = compiler::buildVerilogModuleEmissionPlan(emitter, top);
    ASSERT_TRUE(mlir::succeeded(plan));
    ASSERT_EQ(plan->combinational.size(), expected);
    for (compiler::VerilogCombinationalKind kind : kinds)
      EXPECT_TRUE(llvm::any_of(plan->combinational, [&](const auto &entry) {
        return entry.second.kind == kind;
      }));
  };

  check(PYCIRCUIT_TEST_VERILOG_COMBINATIONAL_INPUT, 8,
        {compiler::VerilogCombinationalKind::Constant,
         compiler::VerilogCombinationalKind::Binary,
         compiler::VerilogCombinationalKind::Select});
  check(PYCIRCUIT_TEST_VERILOG_COMBINATIONAL_EXTENDED_INPUT, 4,
        {compiler::VerilogCombinationalKind::Compare,
         compiler::VerilogCombinationalKind::Resize,
         compiler::VerilogCombinationalKind::Concat,
         compiler::VerilogCombinationalKind::Extract});
}

TEST_F(ConcreteDesignPlanTest, PlansExactFourStateVerilogConstants) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_FOUR_STATE_CONSTANT_INPUT);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  compiler::HardwareEmitContext emitter(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(emitter.prepare()));
  auto top = mlir::cast<ac::ModuleOp>(analysis.lookupDefinition("top"));

  auto plan = compiler::buildVerilogModuleEmissionPlan(emitter, top);
  ASSERT_TRUE(mlir::succeeded(plan));
  auto pattern = llvm::find_if(plan->combinational, [](const auto &entry) {
    return entry.second.kind ==
               compiler::VerilogCombinationalKind::BitsConstant4 &&
           entry.second.staticValue == "1'bz";
  });
  ASSERT_NE(pattern, plan->combinational.end());
}

TEST_F(ConcreteDesignPlanTest, BuildsConcreteVerilogEnumAndStructPlans) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_AGGREGATE);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  compiler::HardwareEmitContext emitter(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(emitter.prepare()));
  auto top = mlir::cast<ac::ModuleOp>(analysis.lookupDefinition("top"));

  auto plan = compiler::buildVerilogModuleEmissionPlan(emitter, top);
  ASSERT_TRUE(mlir::succeeded(plan));
  for (compiler::VerilogCombinationalKind kind : {
           compiler::VerilogCombinationalKind::EnumCreate,
           compiler::VerilogCombinationalKind::EnumToBits,
           compiler::VerilogCombinationalKind::StructCreate,
           compiler::VerilogCombinationalKind::StructGet,
       })
    EXPECT_TRUE(llvm::any_of(plan->combinational, [&](const auto &entry) {
      return entry.second.kind == kind;
    }));
  auto enumCreate = llvm::find_if(plan->combinational, [](const auto &entry) {
    return entry.second.kind == compiler::VerilogCombinationalKind::EnumCreate;
  });
  ASSERT_NE(enumCreate, plan->combinational.end());
  EXPECT_EQ(enumCreate->second.staticValue, "2'd1");
  auto structGet = llvm::find_if(plan->combinational, [](const auto &entry) {
    return entry.second.kind == compiler::VerilogCombinationalKind::StructGet;
  });
  ASSERT_NE(structGet, plan->combinational.end());
  EXPECT_EQ(structGet->second.staticValue, "low");
}

TEST_F(ConcreteDesignPlanTest, BuildsConcreteVerilogTableAndMergePlans) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_VERILOG_TABLE_INPUT);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  compiler::HardwareEmitContext emitter(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(emitter.prepare()));
  auto top = mlir::cast<ac::ModuleOp>(analysis.lookupDefinition("transport"));

  auto plan = compiler::buildVerilogModuleEmissionPlan(emitter, top);
  ASSERT_TRUE(mlir::succeeded(plan));
  EXPECT_TRUE(llvm::any_of(plan->tables, [](const auto &entry) {
    return entry.second.kind == compiler::VerilogTableKind::Create;
  }));
  EXPECT_TRUE(llvm::any_of(plan->tables, [](const auto &entry) {
    return entry.second.kind == compiler::VerilogTableKind::Get;
  }));
  EXPECT_TRUE(llvm::any_of(plan->tables, [](const auto &entry) {
    return entry.second.kind == compiler::VerilogTableKind::ValueMerge;
  }));
  auto merge = llvm::find_if(plan->tables, [](const auto &entry) {
    return entry.second.kind == compiler::VerilogTableKind::ValueMerge &&
           !entry.second.mergeWrites.empty() &&
           entry.second.mergeWrites.front().fieldSuffix == ".groups";
  });
  ASSERT_NE(merge, plan->tables.end());
  ASSERT_EQ(merge->second.mergeWrites.size(), 1u);
  EXPECT_EQ(merge->second.mergeWrites[0].guardOperand, 1u);
  EXPECT_EQ(merge->second.mergeWrites[0].valueOperand, 2u);
  ASSERT_EQ(merge->second.mergePassthroughs.size(), 1u);
  EXPECT_EQ(merge->second.mergePassthroughs[0], ".marker");
  ASSERT_EQ(merge->second.mergeEnableOperands.size(), 1u);
  EXPECT_EQ(merge->second.mergeEnableOperands[0], 1u);
  for (const auto &entry : plan->tables) {
    EXPECT_EQ(entry.second.resultTypes.size(), entry.first->getNumResults());
    if (entry.second.kind == compiler::VerilogTableKind::Create ||
        entry.second.kind == compiler::VerilogTableKind::Get) {
      EXPECT_FALSE(entry.second.elementCount.empty());
      EXPECT_FALSE(entry.second.elementWidth.empty());
    }
  }
}

TEST_F(ConcreteDesignPlanTest, BuildsConcreteVerilogTableRegionPlans) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_VERILOG_TABLE_REGION_INPUT);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  compiler::HardwareEmitContext emitter(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(emitter.prepare()));
  auto top = mlir::cast<ac::ModuleOp>(analysis.lookupDefinition("top"));

  auto plan = compiler::buildVerilogModuleEmissionPlan(emitter, top);
  ASSERT_TRUE(mlir::succeeded(plan));
  auto map = llvm::find_if(plan->tables, [](const auto &entry) {
    return entry.second.kind == compiler::VerilogTableKind::Map;
  });
  ASSERT_NE(map, plan->tables.end());
  ASSERT_EQ(map->second.regionArguments.size(), 3u);
  EXPECT_EQ(map->second.regionArguments[0].kind,
            compiler::VerilogRegionArgumentKind::Ordinal);
  EXPECT_EQ(map->second.regionArguments[0].type, "3");
  EXPECT_EQ(map->second.regionArguments[1].kind,
            compiler::VerilogRegionArgumentKind::TableElement);
  EXPECT_EQ(map->second.regionArguments[1].operand, 0u);
  EXPECT_EQ(map->second.regionArguments[2].kind,
            compiler::VerilogRegionArgumentKind::Capture);
  EXPECT_EQ(map->second.regionArguments[2].operand, 1u);
  ASSERT_EQ(map->second.regionYields.size(), 1u);
  EXPECT_TRUE(map->second.regionYields[0].tableSlice);
  EXPECT_EQ(map->second.regionYields[0].result, 0u);

  auto match = llvm::find_if(plan->tables, [](const auto &entry) {
    return entry.second.kind == compiler::VerilogTableKind::Match;
  });
  ASSERT_NE(match, plan->tables.end());
  ASSERT_EQ(match->second.regionArguments.size(), 2u);
  EXPECT_EQ(match->second.regionArguments[0].kind,
            compiler::VerilogRegionArgumentKind::TableElement);
  EXPECT_EQ(match->second.regionArguments[0].operand, 0u);
  EXPECT_EQ(match->second.regionArguments[1].kind,
            compiler::VerilogRegionArgumentKind::Capture);
  EXPECT_EQ(match->second.regionArguments[1].operand, 1u);
  ASSERT_EQ(match->second.regionYields.size(), 1u);
  EXPECT_FALSE(match->second.regionYields[0].tableSlice);

  auto view = llvm::find_if(plan->tables, [](const auto &entry) {
    return entry.second.kind == compiler::VerilogTableKind::View;
  });
  ASSERT_NE(view, plan->tables.end());
  EXPECT_FALSE(view->second.sourceElementCount.empty());
  EXPECT_FALSE(view->second.sourceElementWidth.empty());
  EXPECT_NE(view->second.mappedIndex.find("__pyc_index__"), std::string::npos);

  auto choose = llvm::find_if(plan->tables, [](const auto &entry) {
    return entry.second.kind == compiler::VerilogTableKind::Choose;
  });
  ASSERT_NE(choose, plan->tables.end());
  EXPECT_EQ(choose->second.targetWidth, "3");
  EXPECT_EQ(choose->second.count, 2);
  EXPECT_TRUE(choose->second.mode == "low" || choose->second.mode == "high");

  unsigned foldPlans = 0;
  for (const auto &entry : plan->tables) {
    if (entry.second.kind != compiler::VerilogTableKind::Fold)
      continue;
    ++foldPlans;
    EXPECT_FALSE(entry.second.elementCount.empty());
    EXPECT_FALSE(entry.second.elementWidth.empty());
    EXPECT_FALSE(entry.second.opcode.empty());
    EXPECT_EQ(entry.second.conditionalFold,
              entry.second.mode == "min" || entry.second.mode == "max");
  }
  EXPECT_EQ(foldPlans, 7u);
}

// Ordinary (non-primitive) module instantiation builds a nested module plan and
// links it through ConcreteChildKind::Module + moduleIndex, exercising the
// recursive visit() path and a shared operand (%x wired to both inputs).
TEST_F(ConcreteDesignPlanTest, BuildsModuleHierarchyWithOrdinaryModuleChild) {
  auto parsed = parseHWIRFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_MODULE_CHILD);
  ASSERT_TRUE(parsed);
  auto plan = compiler::prepareStructuralCpp(*parsed);
  ASSERT_TRUE(mlir::succeeded(plan));
  ASSERT_EQ(plan->modules.size(), 2u); // top + child
  ASSERT_EQ(plan->rootModule, 0u);

  compiler::ConcreteModulePlan &top = plan->modules[plan->rootModule];
  EXPECT_EQ(top.key.definition.getSymName(), "top");
  ASSERT_EQ(top.children.size(), 1u);

  const compiler::ConcreteChildPlan &nested = top.children[0];
  EXPECT_EQ(nested.kind, compiler::ConcreteChildKind::Module);
  ASSERT_TRUE(nested.moduleIndex.has_value());
  EXPECT_FALSE(nested.queue.has_value());
  EXPECT_EQ(nested.instanceName.getValue(), "nested");
  ASSERT_TRUE(nested.calleeKey.has_value());
  EXPECT_EQ(
      mlir::SymbolTable::getSymbolName(nested.calleeKey->definition).getValue(),
      "child");
  ASSERT_EQ(nested.inputs.size(), 2u);
  ASSERT_EQ(nested.outputs.size(), 1u);
  EXPECT_EQ(nested.inputs[0].value, top.inputs[0].value);
  EXPECT_EQ(nested.inputs[1].value, top.inputs[0].value);
  EXPECT_EQ(nested.outputs[0].value, top.outputs[0].value);

  compiler::ConcreteModulePlan &child = plan->modules[*nested.moduleIndex];
  EXPECT_EQ(child.key.definition.getSymName(), "child");
  EXPECT_TRUE(child.children.empty());
  ASSERT_EQ(top.lifecycle.xfer.size(), 1u);
  EXPECT_EQ(top.lifecycle.xfer[0].kind,
            compiler::ConcreteLifecycleStepKind::InvokeChildXfer);
  EXPECT_EQ(top.lifecycle.discard[0].kind,
            compiler::ConcreteLifecycleStepKind::InvokeChildDiscard);
  EXPECT_TRUE(child.lifecycle.xfer.empty());
  EXPECT_TRUE(child.lifecycle.discard.empty());
}

// Two instantiations of the same definition with different type bindings
// (!b8 vs !ac.struct<"Packet">) must produce two distinct, stable
// specializations.
TEST_F(ConcreteDesignPlanTest,
       BuildsDistinctSpecializationsForDistinctTypeBindings) {
  auto parsed = parseHWIRFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_SPECIALIZATION);
  ASSERT_TRUE(parsed);
  auto plan = compiler::prepareStructuralCpp(*parsed);
  ASSERT_TRUE(mlir::succeeded(plan));
  // top + two specializations of @identity.
  ASSERT_EQ(plan->modules.size(), 3u);
  ASSERT_EQ(plan->rootModule, 0u);

  compiler::ConcreteModulePlan &top = plan->modules[plan->rootModule];
  ASSERT_EQ(top.children.size(), 2u);
  EXPECT_EQ(top.children[0].instanceName.getValue(), "scalar");
  EXPECT_EQ(top.children[1].instanceName.getValue(), "aggregate");
  for (const compiler::ConcreteChildPlan &child : top.children) {
    EXPECT_EQ(child.kind, compiler::ConcreteChildKind::Module);
    ASSERT_TRUE(child.moduleIndex.has_value());
  }

  unsigned scalarIndex = *top.children[0].moduleIndex;
  unsigned aggregateIndex = *top.children[1].moduleIndex;
  ASSERT_NE(scalarIndex, aggregateIndex);

  compiler::ConcreteModulePlan &scalar = plan->modules[scalarIndex];
  compiler::ConcreteModulePlan &aggregate = plan->modules[aggregateIndex];
  EXPECT_EQ(scalar.key.definition.getSymName(), "identity");
  EXPECT_EQ(aggregate.key.definition.getSymName(), "identity");

  // Same definition, different type bindings => distinct stable names.
  EXPECT_TRUE(scalar.cppName.starts_with("identity__h"));
  EXPECT_TRUE(aggregate.cppName.starts_with("identity__h"));
  EXPECT_NE(scalar.cppName, aggregate.cppName);

  // The specialization captured its type binding: bits<8> vs struct<"Packet">.
  ASSERT_EQ(scalar.inputs.size(), 1u);
  ASSERT_EQ(aggregate.inputs.size(), 1u);
  EXPECT_EQ(scalar.inputs[0].type.packedWidth, 8u);
  EXPECT_NE(aggregate.inputs[0].type.packedWidth,
            scalar.inputs[0].type.packedWidth);
  ASSERT_EQ(scalar.key.types.size(), 1u);
  EXPECT_EQ(scalar.key.types[0].canonicalForm, "bits:8");
  ASSERT_EQ(aggregate.inputs[0].type.runtimeKind,
            compiler::ConcreteTypePlan::RuntimeKind::Struct);
  EXPECT_EQ(aggregate.inputs[0].type.cppTypeName, "::Packet");
  EXPECT_EQ(plan->root, plan->modules[plan->rootModule].key);
}

// Two instances of the same definition with IDENTICAL bindings must collapse
// onto one specialization (the find()/sameBindings() dedup path in visit()).
TEST_F(ConcreteDesignPlanTest, ReusesOneSpecializationForIdenticalBindings) {
  auto parsed = parseHWIRFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_DEDUP);
  ASSERT_TRUE(parsed);
  auto plan = compiler::prepareStructuralCpp(*parsed);
  ASSERT_TRUE(mlir::succeeded(plan));
  // top + exactly one specialization of @identity (not two).
  ASSERT_EQ(plan->modules.size(), 2u);
  ASSERT_EQ(plan->rootModule, 0u);

  compiler::ConcreteModulePlan &top = plan->modules[plan->rootModule];
  ASSERT_EQ(top.children.size(), 2u);
  EXPECT_EQ(top.children[0].instanceName.getValue(), "first");
  EXPECT_EQ(top.children[1].instanceName.getValue(), "second");
  for (const compiler::ConcreteChildPlan &child : top.children) {
    EXPECT_EQ(child.kind, compiler::ConcreteChildKind::Module);
    ASSERT_TRUE(child.moduleIndex.has_value());
  }

  // Both children share the single identity specialization.
  unsigned firstIndex = *top.children[0].moduleIndex;
  unsigned secondIndex = *top.children[1].moduleIndex;
  EXPECT_EQ(firstIndex, secondIndex);
  EXPECT_EQ(plan->modules[firstIndex].key.definition.getSymName(), "identity");
}

TEST_F(ConcreteDesignPlanTest,
       ExpandsDependencyNodesForChainedIdenticalSpecializations) {
  auto parsed = parseHWIRFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_DEDUP_CHAIN);
  ASSERT_TRUE(parsed);
  auto plan = compiler::prepareStructuralCpp(*parsed);
  ASSERT_TRUE(mlir::succeeded(plan));
  ASSERT_EQ(plan->modules.size(), 2u);
  const compiler::ConcreteModulePlan &top = plan->modules[plan->rootModule];
  ASSERT_EQ(top.children.size(), 3u);
  ASSERT_TRUE(top.children[0].moduleIndex.has_value());
  EXPECT_EQ(top.children[0].moduleIndex, top.children[1].moduleIndex);
  EXPECT_EQ(top.children[1].moduleIndex, top.children[2].moduleIndex);

  unsigned childIndex = *top.children[0].moduleIndex;
  unsigned childScheduleSize = 0;
  for (const compiler::ConcreteDependencyNode &node : plan->dependencies.nodes)
    if (node.moduleIndex == childIndex)
      ++childScheduleSize;
  // Every physical occurrence receives its own copy of the specialization's
  // dependency nodes; otherwise a valid instance chain collapses to a cycle.
  EXPECT_GE(childScheduleSize, 6u);
}

TEST_F(ConcreteDesignPlanTest,
       ClosureDependencyNodesRetainSpecializationOwner) {
  auto parsed = parseHWIRFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_MODULE_CHILD);
  ASSERT_TRUE(parsed);
  auto plan = compiler::prepareStructuralCpp(*parsed);
  ASSERT_TRUE(mlir::succeeded(plan));
  ASSERT_FALSE(plan->dependencies.nodes.empty());
  for (const auto &node : plan->dependencies.nodes) {
    ASSERT_LT(node.moduleIndex, plan->modules.size());
    EXPECT_TRUE(plan->modules[node.moduleIndex].key.definition->isAncestor(
        node.occurrence));
  }
}

TEST_F(ConcreteDesignPlanTest,
       ClosureDependencyIdentityIncludesConcreteSpecialization) {
  auto parsed = parseHWIRFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_SPECIALIZATION);
  ASSERT_TRUE(parsed);
  auto plan = compiler::prepareStructuralCpp(*parsed);
  ASSERT_TRUE(mlir::succeeded(plan));

  llvm::SmallVector<unsigned, 2> identityModules;
  for (auto [moduleIndex, module] : llvm::enumerate(plan->modules))
    if (module.key.definition.getSymName() == "identity")
      identityModules.push_back(static_cast<unsigned>(moduleIndex));
  ASSERT_EQ(identityModules.size(), 2u);

  // Both specializations reference the same definition operations, but each
  // must own distinct closure graph nodes.
  bool foundSharedOccurrence = false;
  for (auto [leftIndex, left] : llvm::enumerate(plan->dependencies.nodes)) {
    if (left.moduleIndex != identityModules[0])
      continue;
    for (auto [rightIndex, right] : llvm::enumerate(plan->dependencies.nodes)) {
      if (right.moduleIndex != identityModules[1] ||
          left.occurrence != right.occurrence)
        continue;
      EXPECT_NE(leftIndex, rightIndex);
      foundSharedOccurrence = true;
    }
  }
  EXPECT_TRUE(foundSharedOccurrence);
}

// A queue op becomes a ConcreteChildKind::Queue child carrying the concrete
// ResolvedQueueConfig (depth from the integer parameter, latency 1).
TEST_F(ConcreteDesignPlanTest, BuildsQueueChildrenWithConcreteConfig) {
  auto parsed = parseHWIRFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_QUEUE);
  ASSERT_TRUE(parsed);
  auto plan = compiler::prepareStructuralCpp(*parsed);
  ASSERT_TRUE(mlir::succeeded(plan));
  // QueueAtomic + LocalWrapper + PopWrapper.
  ASSERT_EQ(plan->modules.size(), 3u);
  ASSERT_EQ(plan->rootModule, 0u);

  compiler::ConcreteModulePlan &root = plan->modules[plan->rootModule];
  EXPECT_EQ(root.key.definition.getSymName(), "QueueAtomic");
  // left (LocalWrapper), right (PopWrapper), parent_reg (dffe primitive).
  ASSERT_EQ(root.children.size(), 3u);
  EXPECT_EQ(root.children[0].instanceName.getValue(), "left");
  EXPECT_EQ(root.children[1].instanceName.getValue(), "right");
  EXPECT_EQ(root.children[2].instanceName.getValue(), "parent_reg");
  EXPECT_EQ(root.children[0].kind, compiler::ConcreteChildKind::Module);
  EXPECT_EQ(root.children[1].kind, compiler::ConcreteChildKind::Module);
  EXPECT_EQ(root.children[2].kind, compiler::ConcreteChildKind::Primitive);

  compiler::ConcreteModulePlan &left =
      plan->modules[*root.children[0].moduleIndex];
  compiler::ConcreteModulePlan &right =
      plan->modules[*root.children[1].moduleIndex];
  EXPECT_EQ(left.key.definition.getSymName(), "LocalWrapper");
  EXPECT_EQ(right.key.definition.getSymName(), "PopWrapper");

  // Each wrapper holds exactly one queue child with a concrete config.
  ASSERT_EQ(left.children.size(), 1u);
  ASSERT_EQ(right.children.size(), 1u);
  EXPECT_EQ(left.children[0].kind, compiler::ConcreteChildKind::Queue);
  EXPECT_EQ(right.children[0].kind, compiler::ConcreteChildKind::Queue);
  ASSERT_TRUE(left.children[0].queue.has_value());
  ASSERT_TRUE(right.children[0].queue.has_value());
  ASSERT_TRUE(left.children[0].primitive.has_value());
  ASSERT_TRUE(right.children[0].primitive.has_value());
  ASSERT_EQ(left.lifecycle.xfer.size(), 1u);
  ASSERT_EQ(right.lifecycle.xfer.size(), 1u);
  EXPECT_EQ(left.lifecycle.xfer[0].kind,
            compiler::ConcreteLifecycleStepKind::InvokePrimitiveXfer);
  EXPECT_EQ(right.lifecycle.discard[0].kind,
            compiler::ConcreteLifecycleStepKind::InvokePrimitiveDiscard);

  // Depth resolves from the wrapper's integer parameter (D=3 vs D=5).
  EXPECT_EQ(left.children[0].queue->depth, 3u);
  EXPECT_EQ(left.children[0].queue->availabilityLatency, 1u);
  EXPECT_EQ(left.children[0].queue->headReadLatency, 0u);
  EXPECT_EQ(right.children[0].queue->depth, 5u);
  EXPECT_EQ(right.children[0].queue->availabilityLatency, 1u);
  EXPECT_EQ(right.children[0].queue->headReadLatency, 0u);

  // Queue feedback is a temporal cut: read the committed head before
  // computing and driving the next input, then sample the next state in
  // Work().  Downstream-pop ready is different because it depends on the
  // current out_ready input and therefore must be read after Work().
  auto stepPosition = [&](compiler::ConcreteScheduleStepKind kind,
                          unsigned child, unsigned port) {
    for (auto [position, step] : llvm::enumerate(root.concreteSchedule.steps))
      if (step.kind == kind && step.childIndex == child &&
          step.portIndex == port)
        return static_cast<unsigned>(position);
    return std::numeric_limits<unsigned>::max();
  };
  auto invokePosition = [&](unsigned child) {
    for (auto [position, step] : llvm::enumerate(root.concreteSchedule.steps))
      if (step.kind == compiler::ConcreteScheduleStepKind::InvokeChildWork &&
          step.childIndex == child)
        return static_cast<unsigned>(position);
    return std::numeric_limits<unsigned>::max();
  };
  for (unsigned child : {0u, 1u}) {
    const unsigned headRead = stepPosition(
        compiler::ConcreteScheduleStepKind::ReadChildOutput, child, 2);
    const unsigned dataDrive = stepPosition(
        compiler::ConcreteScheduleStepKind::DriveChildInput, child, 1);
    const unsigned invoke = invokePosition(child);
    ASSERT_NE(headRead, std::numeric_limits<unsigned>::max());
    ASSERT_NE(dataDrive, std::numeric_limits<unsigned>::max());
    ASSERT_NE(invoke, std::numeric_limits<unsigned>::max());
    EXPECT_LT(headRead, dataDrive);
    EXPECT_LT(dataDrive, invoke);
  }
  const unsigned downstreamReadyRead =
      stepPosition(compiler::ConcreteScheduleStepKind::ReadChildOutput, 1, 0);
  ASSERT_NE(downstreamReadyRead, std::numeric_limits<unsigned>::max());
  EXPECT_LT(invokePosition(1), downstreamReadyRead);

  ASSERT_EQ(root.outputIsTemporal.size(), 7u);
  EXPECT_TRUE(root.outputIsTemporal[0]);
  EXPECT_TRUE(root.outputIsTemporal[1]);
  EXPECT_TRUE(root.outputIsTemporal[2]);
  EXPECT_FALSE(root.outputIsTemporal[3]);
  EXPECT_TRUE(root.outputIsTemporal[4]);
  EXPECT_TRUE(root.outputIsTemporal[5]);
  EXPECT_TRUE(root.outputIsTemporal[6]);
}

TEST_F(ConcreteDesignPlanTest, MaterializesCompleteObservationDescriptors) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_OBSERVATIONS);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  auto plan = compiler::buildConcreteDesignPlan(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  ASSERT_EQ(plan->modules.size(), 1u);
  const auto &module = plan->modules[plan->rootModule];
  ASSERT_EQ(module.observationSites.size(), 3u);
  ASSERT_EQ(plan->observations.size(), 3u);
  EXPECT_EQ(plan->observationEventCapacity, 2u);
  EXPECT_EQ(module.observationSubtreeWidth, 3u);
  for (auto [ordinal, observation] : llvm::enumerate(plan->observations)) {
    EXPECT_EQ(observation.stableOrdinal, ordinal);
    EXPECT_EQ(observation.ownerKey, 0u);
    EXPECT_EQ(observation.moduleIndex, plan->rootModule);
    EXPECT_EQ(observation.localSiteIndex, ordinal);
    EXPECT_EQ(observation.physicalInstancePath, "root");
    EXPECT_EQ(observation.instancePath, "root");
    EXPECT_FALSE(observation.registrationJson.empty());
    EXPECT_FALSE(observation.siteJson.empty());
    EXPECT_FALSE(observation.specJson.empty());
    EXPECT_EQ(observation.payloadWidth, 8u);
  }
  EXPECT_EQ(plan->observations[0].kind,
            compiler::ConcreteObservationKind::Event);
  EXPECT_EQ(plan->observations[0].groupIndex, 0u);
  EXPECT_EQ(plan->observations[0].groupSize, 2u);
  EXPECT_EQ(plan->observations[1].groupIndex, 1u);
  EXPECT_EQ(plan->observations[2].kind,
            compiler::ConcreteObservationKind::Gauge);
  EXPECT_EQ(plan->observations[2].reportName, "progress");

  plan->observations[1].groupIndex = 0;
  EXPECT_TRUE(mlir::failed(compiler::verifyConcreteDesignPlan(*plan)));
}

TEST_F(ConcreteDesignPlanTest, RunsObservationStagingCommitAndDiscard) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_OBSERVATIONS);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  auto plan = compiler::buildConcreteDesignPlan(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(mlir::succeeded(compiler::emitStructuralCpp(*plan, stream)));
  const std::string &root = plan->modules[plan->rootModule].cppName;
  stream
      << "#include <array>\n"
      << "int main() {\n"
      << "  constexpr std::array<gfsim::ObservationDescriptor, 3> d{{\n"
      << "    {0,0,0,0,gfsim::ObservationKind::Event,0,2,8},\n"
      << "    {1,0,0,0,gfsim::ObservationKind::Event,1,2,8},\n"
      << "    {2,0,1,1,gfsim::ObservationKind::Gauge,0,1,8}}};\n"
      << "  gfsim::ObservationSlots slots; if(!slots.Configure(d,2)) return "
         "1;\n"
      << "  pyc_observation_sink sink{&slots,nullptr}; " << root
      << " dut(\"root\",&sink);\n"
      << "  auto on=gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{1});\n"
      << "  dut.path=on; "
         "dut.left=gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{7});\n"
      << "  dut.right=gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{9});\n"
      << "  class S final: public gfsim::SimSystem { bool Precheck() noexcept "
         "override{return true;} } system;\n"
      << "  sink.system=&system; dut.Work(); if(!slots.Precommit(0)) return 2; "
         "slots.Xfer(1);\n"
      << "  if(slots.Events().size()!=2 || slots.Gauges().size()!=1) return "
         "3;\n"
      << "  if(slots.Events()[0].value!=gfsim::SlotValue::Unsigned(7) || "
         "slots.Events()[1].value!=gfsim::SlotValue::Unsigned(9)) return 4;\n"
      << "  if(slots.Gauges()[0].value!=gfsim::SlotValue::Unsigned(7)) return "
         "5;\n"
      << "  dut.Work(); slots.DiscardNext(); slots.Xfer(2);\n"
      << "  if(!slots.Events().empty() || slots.Gauges()[0].lastUpdate!=1) "
         "return 6;\n"
      << "  return 0; }\n";
  llvm::SmallString<128> source, executable;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-observation", "cpp", descriptor, source));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-observation", "exe", descriptor, executable));
  ASSERT_FALSE(llvm::sys::fs::remove(executable));
  llvm::SmallVector<llvm::StringRef> compile = {PYCIRCUIT_TEST_CXX,
                                                "-std=c++20",
                                                "-I",
                                                PYCIRCUIT_TEST_REPO_ROOT
                                                "/include",
                                                source,
                                                "-o",
                                                executable};
  ASSERT_EQ(llvm::sys::ExecuteAndWait(compile.front(), compile), 0);
  llvm::SmallVector<llvm::StringRef> run = {executable};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(run.front(), run), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(source));
  EXPECT_FALSE(llvm::sys::fs::remove(executable));
}

TEST_F(ConcreteDesignPlanTest, RunsWideFourStateObservationStaging) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_OBSERVATIONS_WIDE);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  auto plan = compiler::buildConcreteDesignPlan(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  ASSERT_EQ(plan->observations.size(), 1u);
  EXPECT_EQ(plan->observations[0].payloadWidth, 130u);

  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(mlir::succeeded(compiler::emitStructuralCpp(*plan, stream)));
  const std::string &root = plan->modules[plan->rootModule].cppName;
  stream
      << "#include <array>\n"
      << "int main() {\n"
      << "  constexpr std::array<gfsim::ObservationDescriptor,1> d{{"
         "{0,0,0,0,gfsim::ObservationKind::Event,0,1,130}}};\n"
      << "  gfsim::ObservationSlots slots; if(!slots.Configure(d,1)) return "
         "1;\n"
      << "  pyc_observation_sink sink{&slots,nullptr}; " << root
      << " dut(\"root\",&sink);\n"
      << "  class S final: public gfsim::SimSystem { bool Precheck() noexcept "
         "override{return true;} } system; sink.system=&system;\n"
      << "  dut.path=gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{1});\n"
      << "  gfsim::Bits<130> value{},known{},z{}; value.setWord(0,1); "
         "known.setWord(0,1); z.setWord(2,1);\n"
      << "  "
         "dut.value=gfsim::wire<gfsim::Bits<130>>::fromPacked(gfsim::FourState<"
         "130>::fromMasks(value,known,z));\n"
      << "  dut.Work(); if(!slots.Precommit(0)) return 2; slots.Xfer(1);\n"
      << "  if(slots.Events().size()!=1) return 3; const auto "
         "&v=slots.Events()[0].value;\n"
      << "  if(v.kind!=gfsim::SlotValueKind::Bits4 || v.width!=130 || "
         "v.valueWords[0]!=1 || v.knownWords[0]!=1 || v.zWords[2]!=1) return "
         "4;\n"
      << "  dut.Work(); slots.DiscardNext(); "
         "if(slots.Events()[0].value.zWords[2]!=1) return 5;\n"
      << "  return 0; }\n";
  llvm::SmallString<128> source, executable;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-wide-observation", "cpp", descriptor, source));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-wide-observation", "exe", descriptor, executable));
  ASSERT_FALSE(llvm::sys::fs::remove(executable));
  llvm::SmallVector<llvm::StringRef> compile = {PYCIRCUIT_TEST_CXX,
                                                "-std=c++20",
                                                "-I",
                                                PYCIRCUIT_TEST_REPO_ROOT
                                                "/include",
                                                source,
                                                "-o",
                                                executable};
  ASSERT_EQ(llvm::sys::ExecuteAndWait(compile.front(), compile), 0);
  llvm::SmallVector<llvm::StringRef> run = {executable};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(run.front(), run), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(source));
  EXPECT_FALSE(llvm::sys::fs::remove(executable));
}

// A sync_mem primitive instance becomes a ConcreteChildKind::Primitive child;
// its ADDR_WIDTH/DEPTH integer parameters are concrete and resolve via the
// import's parameter schema (requireConcreteBindings).
TEST_F(ConcreteDesignPlanTest, BuildsMemoryPrimitiveChild) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_MEMORY);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  ASSERT_TRUE(
      mlir::succeeded(compiler::verifyStructuralCppInput(*parsed, analysis)));

  auto plan = compiler::buildConcreteDesignPlan(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  // The memory primitive is not a module specialization.
  ASSERT_EQ(plan->modules.size(), 1u);
  ASSERT_EQ(plan->rootModule, 0u);

  compiler::ConcreteModulePlan &top = plan->modules[plan->rootModule];
  EXPECT_EQ(top.key.definition.getSymName(), "top");

  ASSERT_EQ(top.children.size(), 1u);
  const compiler::ConcreteChildPlan &ram = top.children[0];
  EXPECT_EQ(ram.kind, compiler::ConcreteChildKind::Primitive);
  EXPECT_EQ(ram.primitiveKind.getValue(), "sync_mem");
  EXPECT_EQ(ram.instanceName.getValue(), "ram");
  EXPECT_FALSE(ram.moduleIndex.has_value());
  EXPECT_FALSE(ram.queue.has_value());
  ASSERT_TRUE(ram.primitive.has_value());
  EXPECT_EQ(ram.primitive->integers.size(), 2u);
  ASSERT_EQ(ram.primitive->types.size(), 1u);
  EXPECT_EQ(ram.primitive->types[0].packedWidth, 13u);
  ASSERT_EQ(top.lifecycle.xfer.size(), 1u);
  ASSERT_EQ(top.lifecycle.discard.size(), 1u);
  EXPECT_EQ(top.lifecycle.xfer[0].kind,
            compiler::ConcreteLifecycleStepKind::InvokePrimitiveXfer);
  EXPECT_EQ(top.lifecycle.discard[0].kind,
            compiler::ConcreteLifecycleStepKind::InvokePrimitiveDiscard);

  // Address width (ADDR_WIDTH=4) and payload width (T=13) resolved concretely.
  ASSERT_EQ(top.inputs.size(), 8u);
  EXPECT_EQ(top.inputs[3].type.packedWidth, 4u);  // raddr
  EXPECT_EQ(top.inputs[6].type.packedWidth, 13u); // wdata
  ASSERT_EQ(top.outputs.size(), 1u);
  EXPECT_EQ(top.outputs[0].type.packedWidth, 13u); // rdata
}

TEST_F(ConcreteDesignPlanTest, MaterializesAllRuntimeTypeMappings) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_TYPES);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));

  auto plan = compiler::buildConcreteDesignPlan(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  ASSERT_EQ(plan->modules.size(), 1u);
  const compiler::ConcreteModulePlan &top = plan->modules.front();
  ASSERT_EQ(top.inputs.size(), 4u);
  ASSERT_EQ(top.outputs.size(), 4u);

  using RuntimeKind = compiler::ConcreteTypePlan::RuntimeKind;
  EXPECT_EQ(top.inputs[0].type.runtimeKind, RuntimeKind::Bits);
  EXPECT_EQ(top.inputs[0].type.cppTypeName, "gfsim::Bits<8>");
  EXPECT_EQ(top.inputs[0].type.packedWidth, 8u);

  EXPECT_EQ(top.inputs[1].type.runtimeKind, RuntimeKind::Table);
  EXPECT_EQ(top.inputs[1].type.cppTypeName,
            "gfsim::table<gfsim::Bits<8>, 2, 3>");
  EXPECT_EQ(top.inputs[1].type.shape, (llvm::SmallVector<uint64_t, 2>{2, 3}));
  EXPECT_EQ(top.inputs[1].type.packedWidth, 48u);

  EXPECT_EQ(top.inputs[2].type.runtimeKind, RuntimeKind::Struct);
  EXPECT_EQ(top.inputs[2].type.cppTypeName, "::types::Packet");
  EXPECT_EQ(top.inputs[3].type.runtimeKind, RuntimeKind::Enum);
  EXPECT_EQ(top.inputs[3].type.cppTypeName, "::types::Mode");
  EXPECT_EQ(top.inputs[3].type.packedWidth, 2u);

  for (auto [port, output] : llvm::enumerate(top.outputs)) {
    const compiler::ConcreteValuePlan &value = top.values[output.value];
    EXPECT_EQ(output.type.runtimeKind, top.inputs[port].type.runtimeKind);
    EXPECT_EQ(output.type.cppTypeName, top.inputs[port].type.cppTypeName);
    EXPECT_EQ(value.type.cppTypeName, output.type.cppTypeName);
    EXPECT_EQ(value.type.packedWidth, output.type.packedWidth);
  }
}

// The plan publishes a specialization manifest: one entry per module
// specialization, tying its stable C++ class name back to the HWIR definition
// and the concrete integer/type arguments that selected it.
TEST_F(ConcreteDesignPlanTest, MaterializesSpecializationManifest) {
  auto parsed = parseHWIRFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_QUEUE);
  ASSERT_TRUE(parsed);
  auto plan = compiler::prepareStructuralCpp(*parsed);
  ASSERT_TRUE(mlir::succeeded(plan));

  ASSERT_EQ(plan->manifest.size(), plan->modules.size());
  ASSERT_EQ(plan->manifest.size(), 3u);
  EXPECT_EQ(plan->manifest[0].definition, "@QueueAtomic");
  EXPECT_TRUE(plan->manifest[0].cppName.starts_with("QueueAtomic__h"));
  EXPECT_TRUE(plan->manifest[0].integerArguments.empty());
  EXPECT_EQ(plan->manifest[1].definition, "@LocalWrapper");
  ASSERT_EQ(plan->manifest[1].integerArguments.size(), 1u);
  EXPECT_EQ(plan->manifest[1].integerArguments[0].name, "D");
  EXPECT_EQ(plan->manifest[1].integerArguments[0].value, "3");
  EXPECT_EQ(plan->manifest[2].definition, "@PopWrapper");
  ASSERT_EQ(plan->manifest[2].integerArguments.size(), 1u);
  EXPECT_EQ(plan->manifest[2].integerArguments[0].name, "D");
  EXPECT_EQ(plan->manifest[2].integerArguments[0].value, "5");

  auto typed = parseHWIRFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_SPECIALIZATION);
  ASSERT_TRUE(typed);
  auto typedPlan = compiler::prepareStructuralCpp(*typed);
  ASSERT_TRUE(mlir::succeeded(typedPlan));
  ASSERT_EQ(typedPlan->manifest.size(), 3u);
  EXPECT_EQ(typedPlan->manifest[0].definition, "@top");
  EXPECT_TRUE(typedPlan->manifest[0].typeArguments.empty());
  ASSERT_EQ(typedPlan->manifest[1].definition, "@identity");
  ASSERT_EQ(typedPlan->manifest[1].typeArguments.size(), 1u);
  EXPECT_EQ(typedPlan->manifest[1].typeArguments[0].name, "T");
  EXPECT_EQ(typedPlan->manifest[1].typeArguments[0].value, "bits:8");
  ASSERT_EQ(typedPlan->manifest[2].definition, "@identity");
  ASSERT_EQ(typedPlan->manifest[2].typeArguments.size(), 1u);
  EXPECT_EQ(typedPlan->manifest[2].typeArguments[0].name, "T");
  EXPECT_TRUE(typedPlan->manifest[2].typeArguments[0].value.starts_with(
      "struct:Packet:"));
}

TEST_F(ConcreteDesignPlanTest, EmitsMinimalStructuralCppSlice) {
  ac::HardwareAnalysis analysis(*package);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(
      mlir::succeeded(compiler::emitStructuralCpp(*package, analysis, stream)));
  EXPECT_NE(text.find("class top__h"), std::string::npos);
  EXPECT_NE(text.find("gfsim::dffe<gfsim::Bits<8>>"), std::string::npos);
  EXPECT_NE(text.find("gfsim::bit_not"), std::string::npos);
  EXPECT_NE(text.find("pyc_left.Work();"), std::string::npos);
  EXPECT_NE(text.find("pyc_right.Work();"), std::string::npos);
  EXPECT_NE(text.find("pyc_left.Xfer();"), std::string::npos);
  EXPECT_EQ(text.find("concrete-plan emission is not implemented"),
            std::string::npos);

  llvm::SmallString<128> path;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile("pycircuit-structural-cpp",
                                                  "cpp", descriptor, path));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  llvm::SmallVector<llvm::StringRef> arguments = {PYCIRCUIT_TEST_CXX,
                                                  "-std=c++20",
                                                  "-fsyntax-only",
                                                  "-I",
                                                  PYCIRCUIT_TEST_REPO_ROOT
                                                  "/include",
                                                  path};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(arguments.front(), arguments), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(path));
}

TEST_F(ConcreteDesignPlanTest, ExternalizesLargeWorkIntoBoundedSourceShards) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_COMBINATIONAL);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  auto plan = compiler::buildConcreteDesignPlan(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  auto &module = plan->modules[plan->rootModule];
  ASSERT_FALSE(module.concreteSchedule.steps.empty());

  // Model a flattened accelerator without making the fixture carry thousands
  // of authored operations. The rolling XOR reads its own previous result, so
  // the value dependency must survive every generated helper boundary.
  const auto original = module.concreteSchedule.steps;
  auto binary = llvm::find_if(original, [](const auto &step) {
    return step.compute &&
           step.compute->kind == compiler::ConcreteComputeKind::BitsBinary &&
           step.compute->opcode == "bit_and";
  });
  ASSERT_NE(binary, original.end());
  compiler::ConcreteScheduleStep rolling = *binary;
  rolling.compute->opcode = "bit_xor";
  rolling.inputs[0] = rolling.outputs[0];
  rolling.inputs[1] = module.inputs[1].value;
  const size_t insertion = std::distance(original.begin(), binary) + 1;
  module.concreteSchedule.steps.clear();
  module.concreteSchedule.steps.append(original.begin(),
                                       original.begin() + insertion);
  for (unsigned repetition = 0; repetition < 12001; ++repetition)
    module.concreteSchedule.steps.push_back(rolling);
  module.concreteSchedule.steps.append(original.begin() + insertion,
                                       original.end());
  // Lifecycle projections used to remain as three monolithic inline methods
  // even after Work itself was externalized. Exercise the same flattened
  // shape in every execution phase so the source-size guards cover all four
  // generated paths.
  for (auto *projection : {&module.pureOutputRefresh, &module.temporalRefresh,
                           &module.resetPreparation}) {
    projection->steps.clear();
    for (unsigned repetition = 0; repetition < 12001; ++repetition)
      projection->steps.push_back(rolling);
  }

  const auto output = llvm::find_if(module.outputs, [&](const auto &port) {
    return port.value == rolling.outputs[0];
  });
  ASSERT_NE(output, module.outputs.end());

  std::string header;
  llvm::raw_string_ostream headerStream(header);
  ASSERT_TRUE(
      mlir::succeeded(compiler::emitStructuralCppHeader(*plan, headerStream)));
  auto shards = compiler::emitStructuralCppWorkShards(*plan);
  ASSERT_TRUE(mlir::succeeded(shards));
  ASSERT_GT(shards->size(), 1u);

  auto count = [](llvm::StringRef text, llvm::StringRef needle) {
    size_t occurrences = 0;
    for (size_t offset = 0;
         (offset = text.find(needle, offset)) != llvm::StringRef::npos;
         offset += needle.size())
      ++occurrences;
    return occurrences;
  };
  const std::string definitionPrefix =
      "PYCIRCUIT_NOINLINE void " + module.cppName + "::pyc_work_chunk_";
  EXPECT_EQ(header.find(definitionPrefix), std::string::npos);
  for (llvm::StringRef prefix :
       {"pyc_refresh_pure_chunk_", "pyc_refresh_temporal_chunk_",
        "pyc_prepare_reset_chunk_"}) {
    EXPECT_EQ(header.find(module.cppName + "::" + prefix.str()),
              std::string::npos);
    EXPECT_GT(count(header, "PYCIRCUIT_NOINLINE void " + prefix.str()), 1u);
  }
  const size_t helperDeclarations =
      count(header, "PYCIRCUIT_NOINLINE void pyc_work_chunk_");
  EXPECT_GT(helperDeclarations, 1u);

  size_t helperDefinitions = 0;
  size_t lifecycleDefinitions = 0;
  for (auto [index, shard] : llvm::enumerate(*shards)) {
    EXPECT_EQ(shard.sourceUnit, 0u);
    EXPECT_EQ(shard.shard, index);
    EXPECT_EQ(shard.path, "work_unit_0_" + std::to_string(index) + ".cpp");
    EXPECT_EQ(count(shard.text, "#include \"design.hpp\""), 1u);
    EXPECT_LE(shard.text.size(), 1024u * 1024u);
    helperDefinitions += count(shard.text, definitionPrefix);
    for (llvm::StringRef prefix :
         {"pyc_refresh_pure_chunk_", "pyc_refresh_temporal_chunk_",
          "pyc_prepare_reset_chunk_"})
      lifecycleDefinitions +=
          count(shard.text, "PYCIRCUIT_NOINLINE void " + module.cppName +
                                "::" + prefix.str());

    // Every noinline definition must independently remain below the hard
    // function budget, regardless of how several helpers share one TU.
    llvm::StringRef remaining(shard.text);
    while (true) {
      const size_t start = remaining.find("PYCIRCUIT_NOINLINE void ");
      if (start == llvm::StringRef::npos)
        break;
      remaining = remaining.drop_front(start);
      const size_t next =
          remaining.drop_front(1).find("PYCIRCUIT_NOINLINE void ");
      const size_t bytes =
          next == llvm::StringRef::npos ? remaining.size() : next + 1;
      EXPECT_LE(bytes, 512u * 1024u);
      remaining = remaining.drop_front(bytes);
    }
  }
  EXPECT_EQ(helperDefinitions, helperDeclarations);
  EXPECT_EQ(
      lifecycleDefinitions,
      count(header, "PYCIRCUIT_NOINLINE void pyc_refresh_pure_chunk_") +
          count(header, "PYCIRCUIT_NOINLINE void pyc_refresh_temporal_chunk_") +
          count(header, "PYCIRCUIT_NOINLINE void pyc_prepare_reset_chunk_"));

  llvm::SmallString<128> directory;
  ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory(
      "pycircuit-structural-work-shards", directory));
  auto writeFile = [&](llvm::StringRef name,
                       llvm::StringRef contents) -> std::string {
    llvm::SmallString<128> path(directory);
    llvm::sys::path::append(path, name);
    std::error_code error;
    llvm::raw_fd_ostream file(path, error);
    if (error)
      return {};
    file << contents;
    file.close();
    return path.str().str();
  };

  std::vector<std::string> sources;
  ASSERT_FALSE(writeFile("design.hpp", header).empty());
  for (unsigned sourceUnit = 0; sourceUnit < plan->sourceUnits.size();
       ++sourceUnit) {
    std::string source;
    llvm::raw_string_ostream sourceStream(source);
    ASSERT_TRUE(mlir::succeeded(compiler::emitStructuralCppSourceUnit(
        *plan, sourceUnit, sourceStream)));
    sources.push_back(writeFile(
        "source_unit_" + std::to_string(sourceUnit) + ".cpp", source));
    ASSERT_FALSE(sources.back().empty());
  }
  for (const auto &shard : *shards) {
    sources.push_back(writeFile(shard.path, shard.text));
    ASSERT_FALSE(sources.back().empty());
  }
  sources.push_back(
      writeFile("main.cpp",
                "#include \"design.hpp\"\nint main() { " + module.cppName +
                    " dut(\"root\"); dut." + module.inputs[0].cppName + " = " +
                    "gfsim::wire<" + module.inputs[0].type.cppTypeName +
                    ">::known(" + module.inputs[0].type.cppTypeName +
                    "{0xf0}); dut." + module.inputs[1].cppName +
                    " = gfsim::wire<" + module.inputs[1].type.cppTypeName +
                    ">::known(" + module.inputs[1].type.cppTypeName +
                    "{0x0f}); dut." + module.inputs[2].cppName +
                    " = gfsim::wire<" + module.inputs[2].type.cppTypeName +
                    ">::known(" + module.inputs[2].type.cppTypeName +
                    "{1}); dut.Work(); return " + "!dut." + output->cppName +
                    ".isFullyKnown() || dut." + output->cppName +
                    ".value() != " + output->type.cppTypeName + "{0x0f}; }\n"));
  ASSERT_FALSE(sources.back().empty());

  llvm::SmallString<128> runtimeInclude(PYCIRCUIT_TEST_REPO_ROOT);
  llvm::sys::path::append(runtimeInclude, "include");
  llvm::SmallString<128> executable(directory);
  llvm::sys::path::append(executable, "large-work");
  llvm::SmallVector<llvm::StringRef> compile = {
      PYCIRCUIT_TEST_CXX, "-std=c++20", "-O0",    "-I",
      runtimeInclude,     "-I",         directory};
  for (const std::string &source : sources)
    compile.push_back(source);
  compile.append({"-o", executable});
  ASSERT_EQ(llvm::sys::ExecuteAndWait(compile.front(), compile), 0);
  llvm::SmallVector<llvm::StringRef> run = {executable};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(run.front(), run), 0);
  EXPECT_FALSE(llvm::sys::fs::remove_directories(directory));
}

TEST_F(ConcreteDesignPlanTest, HostDrivenDutKeepsCombinationalRootsRunnable) {
  ac::HardwareAnalysis analysis(*package);
  auto plan = compiler::buildConcreteDesignPlan(*package, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(mlir::succeeded(compiler::emitStructuralCpp(*plan, stream)));
  ASSERT_TRUE(
      mlir::succeeded(compiler::emitStructuralCppHostApi(*plan, stream)));
  EXPECT_NE(text.find("return pyc_host_driven_ ||"), std::string::npos);
  EXPECT_NE(text.find("root_(\"root\", &sink_, 0, false, true)"),
            std::string::npos);
}

TEST_F(ConcreteDesignPlanTest, RunsMinimalStructuralCppSlice) {
  ac::HardwareAnalysis analysis(*package);
  auto plan = compiler::buildConcreteDesignPlan(*package, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(
      mlir::succeeded(compiler::emitStructuralCpp(*package, analysis, stream)));
  const std::string &root = plan->modules[plan->rootModule].cppName;
  stream
      << "int main() {\n  " << root << " dut(\"dut\");\n"
      << "  auto bit0 = "
         "gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{0});\n"
      << "  auto bit1 = "
         "gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{1});\n"
      << "  dut.clk = bit0; dut.rst = bit0; dut.en = bit1;\n"
      << "  dut.d = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{0x5a});\n"
      << "  dut.init = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{0});\n"
      << "  dut.Work(); dut.Xfer();\n"
      << "  dut.clk = bit1; dut.Work(); dut.Xfer(); dut.Work();\n"
      << "  if (!dut.qa.isFullyKnown()) return 11;\n"
      << "  if (!dut.qb.isFullyKnown()) return 12;\n"
      << "  if (dut.qa.value() != gfsim::Bits<8>{0x5a}) return 2;\n"
      << "  if (dut.qb.value() != gfsim::Bits<8>{0xa5}) return 3;\n"
      << "  return 0;\n}\n";
  llvm::SmallString<128> source, executable;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-runtime", "cpp", descriptor, source));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-runtime", "exe", descriptor, executable));
  ASSERT_FALSE(llvm::sys::fs::remove(executable));
  llvm::SmallVector<llvm::StringRef> compile = {PYCIRCUIT_TEST_CXX,
                                                "-std=c++20",
                                                "-I",
                                                PYCIRCUIT_TEST_REPO_ROOT
                                                "/include",
                                                source,
                                                "-o",
                                                executable};
  ASSERT_EQ(llvm::sys::ExecuteAndWait(compile.front(), compile), 0);
  llvm::SmallVector<llvm::StringRef> run = {executable};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(run.front(), run), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(source));
  EXPECT_FALSE(llvm::sys::fs::remove(executable));
}

TEST_F(ConcreteDesignPlanTest, RunsDffThroughSimSystemLifecycle) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_DFF);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  auto plan = compiler::buildConcreteDesignPlan(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  const auto &child = plan->modules[plan->rootModule].children.front();
  ASSERT_TRUE(child.primitive.has_value());
  EXPECT_EQ(child.primitive->inputMembers,
            (llvm::SmallVector<std::string, 8>{"clk", "rst", "d", "init"}));

  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(
      mlir::succeeded(compiler::emitStructuralCpp(*parsed, analysis, stream)));
  const std::string &root = plan->modules[plan->rootModule].cppName;
  stream << "#include <gfsim/SimSystem.h>\n"
         << "class TestSystem final : public gfsim::SimSystem { protected: "
            "bool Precheck() noexcept override { return true; } };\n"
         << "int main() { " << root << " dut(\"dut\"); TestSystem system; "
         << "if (!system.AddModule(dut)) return 1; system.Build(); "
         << "auto b0=gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{0}); "
         << "auto b1=gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{1}); "
         << "dut.clk=b0; dut.rst=b0; "
            "dut.d=gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{0x5a}); "
         << "dut.init=gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{0}); "
         << "system.Reset(); if (system.Step()!=gfsim::SimStepResult::Running) "
            "return 2; "
         << "dut.clk=b1; if (system.Step()!=gfsim::SimStepResult::Running) "
            "return 3; "
         << "dut.clk=b0; if (system.Step()!=gfsim::SimStepResult::Running) "
            "return 4; "
         << "return !dut.q.isFullyKnown() || "
            "dut.q.value()!=gfsim::Bits<8>{0x5a}; }\n";
  llvm::SmallString<128> source, executable;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile("pycircuit-structural-dff",
                                                  "cpp", descriptor, source));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-dff", "exe", descriptor, executable));
  ASSERT_FALSE(llvm::sys::fs::remove(executable));
  llvm::SmallVector<llvm::StringRef> compile = {PYCIRCUIT_TEST_CXX,
                                                "-std=c++20",
                                                "-I",
                                                PYCIRCUIT_TEST_REPO_ROOT
                                                "/include",
                                                source,
                                                "-o",
                                                executable};
  ASSERT_EQ(llvm::sys::ExecuteAndWait(compile.front(), compile), 0);
  llvm::SmallVector<llvm::StringRef> run = {executable};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(run.front(), run), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(source));
  EXPECT_FALSE(llvm::sys::fs::remove(executable));
}

TEST_F(ConcreteDesignPlanTest, EmitsOrdinaryModuleHierarchySlice) {
  auto parsed = parseHWIRFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_MODULE_CHILD);
  ASSERT_TRUE(parsed);
  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(mlir::succeeded(compiler::emitStructuralCpp(*parsed, stream)));
  EXPECT_NE(text.find("gfsim::add"), std::string::npos);
  EXPECT_NE(text.find("pyc_nested.Work();"), std::string::npos);
  EXPECT_LT(text.find("class child__h"), text.find("class top__h"));

  llvm::SmallString<128> path;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile("pycircuit-structural-module",
                                                  "cpp", descriptor, path));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  llvm::SmallVector<llvm::StringRef> arguments = {PYCIRCUIT_TEST_CXX,
                                                  "-std=c++20",
                                                  "-fsyntax-only",
                                                  "-I",
                                                  PYCIRCUIT_TEST_REPO_ROOT
                                                  "/include",
                                                  path};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(arguments.front(), arguments), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(path));
}

// The combinational vertical slice renders a bits constant, bitwise and/or/xor
// and a select mux; the generated program must pass a C++ syntax check.
TEST_F(ConcreteDesignPlanTest, EmitsCombinationalComputeSlice) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_COMBINATIONAL);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(
      mlir::succeeded(compiler::emitStructuralCpp(*parsed, analysis, stream)));
  EXPECT_NE(text.find("gfsim::bit_and("), std::string::npos);
  EXPECT_NE(text.find("gfsim::bit_or("), std::string::npos);
  EXPECT_NE(text.find("gfsim::bit_xor("), std::string::npos);
  EXPECT_NE(text.find("gfsim::select("), std::string::npos);
  EXPECT_NE(text.find("::known(gfsim::Bits<8>{5ULL})"), std::string::npos);
  EXPECT_NE(text.find("::known(gfsim::Bits<8>{18ULL})"), std::string::npos);
  EXPECT_NE(text.find("::known(gfsim::Bits<8>{165ULL})"), std::string::npos);

  llvm::SmallString<128> path;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile("pycircuit-structural-comb",
                                                  "cpp", descriptor, path));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  llvm::SmallVector<llvm::StringRef> arguments = {PYCIRCUIT_TEST_CXX,
                                                  "-std=c++20",
                                                  "-fsyntax-only",
                                                  "-I",
                                                  PYCIRCUIT_TEST_REPO_ROOT
                                                  "/include",
                                                  path};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(arguments.front(), arguments), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(path));
}

TEST_F(ConcreteDesignPlanTest, RunsConstantSelectComputeSlice) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_COMBINATIONAL);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  auto plan = compiler::buildConcreteDesignPlan(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));

  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(
      mlir::succeeded(compiler::emitStructuralCpp(*parsed, analysis, stream)));
  const std::string &root = plan->modules[plan->rootModule].cppName;
  stream
      << "int main() {\n  " << root << " dut(\"dut\");\n"
      << "  dut.a = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{0x3c});\n"
      << "  dut.b = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{0xc3});\n"
      << "  dut.sel = gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{0});\n"
      << "  dut.Work();\n"
      << "  if (!dut.k.isFullyKnown() || dut.k.value() != gfsim::Bits<8>{5}) "
         "return 1;\n"
      << "  if (!dut.constant_mux_out.isFullyKnown() || "
         "dut.constant_mux_out.value() != gfsim::Bits<8>{165}) return 2;\n"
      << "  if (!dut.mux_out.isFullyKnown() || dut.mux_out.value() != "
         "gfsim::Bits<8>{0xc3}) return 3;\n"
      << "  dut.sel = gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{1});\n"
      << "  dut.Work();\n"
      << "  if (!dut.constant_mux_out.isFullyKnown() || "
         "dut.constant_mux_out.value() != gfsim::Bits<8>{18}) return 4;\n"
      << "  if (!dut.mux_out.isFullyKnown() || dut.mux_out.value() != "
         "gfsim::Bits<8>{0x3c}) return 5;\n"
      << "  return 0;\n}\n";

  llvm::SmallString<128> source, executable;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-constant-select", "cpp", descriptor, source));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-constant-select", "exe", descriptor, executable));
  ASSERT_FALSE(llvm::sys::fs::remove(executable));
  llvm::SmallVector<llvm::StringRef> compile = {PYCIRCUIT_TEST_CXX,
                                                "-std=c++20",
                                                "-I",
                                                PYCIRCUIT_TEST_REPO_ROOT
                                                "/include",
                                                source,
                                                "-o",
                                                executable};
  ASSERT_EQ(llvm::sys::ExecuteAndWait(compile.front(), compile), 0);
  llvm::SmallVector<llvm::StringRef> run = {executable};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(run.front(), run), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(source));
  EXPECT_FALSE(llvm::sys::fs::remove(executable));
}

TEST_F(ConcreteDesignPlanTest, RunsCompareResizeConcatExtractMappings) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_UNMAPPED);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  auto plan = compiler::buildConcreteDesignPlan(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(
      mlir::succeeded(compiler::emitStructuralCpp(*parsed, analysis, stream)));
  const std::string &root = plan->modules[plan->rootModule].cppName;
  stream
      << "int main() { " << root << " dut(\"dut\"); "
      << "dut.a=gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{0x12}); "
      << "dut.b=gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{0x34}); "
         "dut.Work(); "
      << "if(dut.eq_out.value()!=gfsim::Bits<1>{0}) return 1; "
      << "if(dut.wide_out.value()!=gfsim::Bits<16>{0x12}) return 2; "
      << "if(dut.cat_out.value()!=gfsim::Bits<16>{0x1234}) return 3; "
      << "if(dut.slice_out.value()!=gfsim::Bits<4>{2}) return 4; return 0; }\n";
  llvm::SmallString<128> source, executable;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-extended-bits", "cpp", descriptor, source));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-extended-bits", "exe", descriptor, executable));
  ASSERT_FALSE(llvm::sys::fs::remove(executable));
  llvm::SmallVector<llvm::StringRef> compile = {PYCIRCUIT_TEST_CXX,
                                                "-std=c++20",
                                                "-I",
                                                PYCIRCUIT_TEST_REPO_ROOT
                                                "/include",
                                                source,
                                                "-o",
                                                executable};
  ASSERT_EQ(llvm::sys::ExecuteAndWait(compile.front(), compile), 0);
  llvm::SmallVector<llvm::StringRef> run = {executable};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(run.front(), run), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(source));
  EXPECT_FALSE(llvm::sys::fs::remove(executable));
}

TEST_F(ConcreteDesignPlanTest, EmitsAllRuntimeBackedBinaryOpcodes) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_UNSUPPORTED_BINARY);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  std::string text;
  llvm::raw_string_ostream stream(text);
  EXPECT_TRUE(
      mlir::succeeded(compiler::emitStructuralCpp(*parsed, analysis, stream)));
  EXPECT_NE(text.find("gfsim::mul("), std::string::npos);
}

TEST_F(ConcreteDesignPlanTest, EmitsAggregateDffPayloadDuringPlanning) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_DFF_STRUCT);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  auto plan = compiler::buildConcreteDesignPlan(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  ASSERT_EQ(plan->modules.size(), 1u);
  ASSERT_EQ(plan->modules.front().children.size(), 1u);
  const auto &storage = plan->modules.front().children.front();
  ASSERT_TRUE(storage.primitive.has_value());
  EXPECT_EQ(storage.primitive->runtimeCppType, "gfsim::dffe<::Pair>");

  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(mlir::succeeded(compiler::emitStructuralCpp(*plan, stream)));
  EXPECT_NE(text.find("gfsim::dffe<::Pair>"), std::string::npos);
}

TEST_F(ConcreteDesignPlanTest, EmitsMemoryPrimitive) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_MEMORY);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(
      mlir::succeeded(compiler::emitStructuralCpp(*parsed, analysis, stream)));
  EXPECT_NE(text.find("gfsim::sync_mem<gfsim::Bits<13>, 4, 7>"),
            std::string::npos);
  llvm::SmallString<128> path;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile("pycircuit-structural-memory",
                                                  "cpp", descriptor, path));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  llvm::SmallVector<llvm::StringRef> arguments = {PYCIRCUIT_TEST_CXX,
                                                  "-std=c++20",
                                                  "-fsyntax-only",
                                                  "-I",
                                                  PYCIRCUIT_TEST_REPO_ROOT
                                                  "/include",
                                                  path};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(arguments.front(), arguments), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(path));
}

TEST_F(ConcreteDesignPlanTest, EmitsQueuePrimitive) {
  auto parsed = parseHWIRFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_QUEUE);
  ASSERT_TRUE(parsed);
  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(mlir::succeeded(compiler::emitStructuralCpp(*parsed, stream)));
  EXPECT_NE(text.find("gfsim::fifo<gfsim::Bits<13>, 3"), std::string::npos);
  EXPECT_NE(text.find("gfsim::fifo<gfsim::Bits<13>, 5"), std::string::npos);
  llvm::SmallString<128> path;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile("pycircuit-structural-queue",
                                                  "cpp", descriptor, path));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  llvm::SmallVector<llvm::StringRef> arguments = {PYCIRCUIT_TEST_CXX,
                                                  "-std=c++20",
                                                  "-fsyntax-only",
                                                  "-I",
                                                  PYCIRCUIT_TEST_REPO_ROOT
                                                  "/include",
                                                  path};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(arguments.front(), arguments), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(path));
}

TEST_F(ConcreteDesignPlanTest, RunsPackedStructAndEnumMappings) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_AGGREGATE);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  auto plan = compiler::buildConcreteDesignPlan(*parsed, analysis);
  ASSERT_TRUE(mlir::succeeded(plan));
  std::string text;
  llvm::raw_string_ostream stream(text);
  ASSERT_TRUE(
      mlir::succeeded(compiler::emitStructuralCpp(*parsed, analysis, stream)));
  const std::string &root = plan->modules[plan->rootModule].cppName;
  stream
      << "int main(){ " << root << " dut(\"dut\"); "
      << "dut.high=gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{0xab}); "
      << "dut.low=gfsim::wire<gfsim::Bits<4>>::known(gfsim::Bits<4>{5}); "
         "dut.Work(); "
      << "if(dut.packet.value()!=types::Packet{0xab5}) return 1; "
      << "if(dut.field.value()!=gfsim::Bits<4>{5}) return 2; "
      << "if(dut.mode_bits.value()!=gfsim::Bits<2>{1}) return 3; return 0;}\n";
  llvm::SmallString<128> source, executable;
  int descriptor = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-aggregate", "cpp", descriptor, source));
  {
    llvm::raw_fd_ostream file(descriptor, true);
    file << text;
  }
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
      "pycircuit-structural-aggregate", "exe", descriptor, executable));
  ASSERT_FALSE(llvm::sys::fs::remove(executable));
  llvm::SmallVector<llvm::StringRef> compile = {PYCIRCUIT_TEST_CXX,
                                                "-std=c++20",
                                                "-I",
                                                PYCIRCUIT_TEST_REPO_ROOT
                                                "/include",
                                                source,
                                                "-o",
                                                executable};
  ASSERT_EQ(llvm::sys::ExecuteAndWait(compile.front(), compile), 0);
  llvm::SmallVector<llvm::StringRef> run = {executable};
  EXPECT_EQ(llvm::sys::ExecuteAndWait(run.front(), run), 0);
  EXPECT_FALSE(llvm::sys::fs::remove(source));
  EXPECT_FALSE(llvm::sys::fs::remove(executable));
}

TEST_F(ConcreteDesignPlanTest, AdmissionRequiresFinalStage) {
  (*package)->removeAttr("ac.stage");
  ac::HardwareAnalysis analysis(*package);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  EXPECT_TRUE(
      mlir::failed(compiler::verifyStructuralCppInput(*package, analysis)));
}

// A package with more than one ac.system is rejected by the shared hardware
// authority that verifyStructuralCppInput delegates to.
TEST_F(ConcreteDesignPlanTest, AdmissionRejectsMultipleSystems) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_TWO_SYSTEMS);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  EXPECT_TRUE(
      mlir::failed(compiler::verifyStructuralCppInput(*parsed, analysis)));
}

TEST_F(ConcreteDesignPlanTest, AdmissionRequiresTheHardwareOnlyBoundary) {
  (*package)->removeAttr("hwir.hardware_only");
  ac::HardwareAnalysis analysis(*package);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  EXPECT_TRUE(
      mlir::failed(compiler::verifyStructuralCppInput(*package, analysis)));
}

TEST_F(ConcreteDesignPlanTest, AdmissionRequiresAUnitHardwareOnlyCertificate) {
  mlir::Builder builder(&context);
  (*package)->setAttr("hwir.hardware_only", builder.getStringAttr("forged"));
  ac::HardwareAnalysis analysis(*package);
  ASSERT_TRUE(mlir::succeeded(analysis.verify()));
  EXPECT_TRUE(
      mlir::failed(compiler::verifyStructuralCppInput(*package, analysis)));
}

// Unknown primitive kinds are rejected at parse time by
// ModuleImportOp::verify() -> primitiveSchema, so the plan-builder
// isKnownPrimitive() guard can never observe one. This documents the real
// enforcement point and guards it from regressing.
TEST_F(ConcreteDesignPlanTest, UnknownPrimitiveKindIsRejectedAtParse) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_UNKNOWN_PRIMITIVE);
  EXPECT_FALSE(parsed);
}

// A combinational cycle through a primitive read path is rejected by the
// shared combinational-cycle analysis before any plan is built.
TEST_F(ConcreteDesignPlanTest, AdmissionRejectsCombinationalCycle) {
  auto parsed = parseFrom(PYCIRCUIT_TEST_STRUCTURAL_CPP_CYCLE);
  ASSERT_TRUE(parsed);
  ac::HardwareAnalysis analysis(*parsed);
  EXPECT_TRUE(
      mlir::failed(compiler::verifyStructuralCppInput(*parsed, analysis)));
}

} // namespace
