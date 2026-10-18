#include "Dialect/ACIR/HardwareValueAvailability.h"
#include "Dialect/ACIR/HardwareSampledAdmission.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

namespace {
namespace ac = acir::ac;
using namespace mlir;

class HardwareValueAvailabilityTest : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder b{&context};
  OwningOpRef<mlir::ModuleOp> package;
  ac::ModuleOp module;
  ac::YieldOp yield;
  Value payload, selected, available, one;
  std::string diagnostics;

  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  NamedAttribute f(StringRef name, Attribute attr) {
    return b.getNamedAttr(name, attr);
  }
  ac::StaticExprAttr literal(unsigned value) {
    return cast<ac::StaticExprAttr>(parseAttribute(
        "#ac.static_expr<{kind = \"literal\", value = {kind = \"integer\", "
        "value = #ac.math_int<" +
            std::to_string(value) +
            ">}, origin = {site = {definition = @Root, ast_path = []}, "
            "expansion = []}, "
            "location = {path = \"availability.py\", line = 1 : i64, column = "
            "1 : i64, "
            "end_line = 1 : i64, end_column = 2 : i64}}>",
        &context));
  }
  Type bits(unsigned width) {
    return ac::BitsType::get(&context, literal(width));
  }
  Operation *op(StringRef name, ValueRange inputs, TypeRange results,
                ArrayRef<NamedAttribute> attrs = {}) {
    OperationState state(b.getUnknownLoc(), name);
    state.addOperands(inputs);
    state.addTypes(results);
    state.addAttributes(attrs);
    return b.create(state);
  }
  Value constant(unsigned value, unsigned width = 1) {
    return op(ac::BitsConstantOp::getOperationName(), {}, {bits(width)},
              {f("value", literal(value))})
        ->getResult(0);
  }
  Value choose(Value condition, Value yes, Value no) {
    return op(ac::BitsSelectOp::getOperationName(), {condition, yes, no},
              {yes.getType()})
        ->getResult(0);
  }
  void build(bool selectPayload = true) {
    package = mlir::ModuleOp::create(b.getUnknownLoc());
    (*package)->setAttr("ac.stage", b.getStringAttr("final"));
    b.setInsertionPointToStart(package->getBody());
    OperationState state(b.getUnknownLoc(), ac::ModuleOp::getOperationName());
    state.addAttributes(
        {f("sym_name", b.getStringAttr("Root")),
         f("source_owner",
           b.getDictionaryAttr(
               {f("package", b.getStringAttr("")),
                f("path", b.getStringAttr("availability.py"))})),
         f("parameters", b.getArrayAttr({})),
         f("type_parameters", b.getArrayAttr({})),
         f("input_names",
           b.getStrArrayAttr({"payload", "selected", "available"})),
         f("output_names", b.getStrArrayAttr({"result", "result_available"})),
         f("function_type",
           TypeAttr::get(b.getFunctionType({bits(8), bits(1), bits(1)},
                                           {bits(8), bits(1)}))),
         f("ac.internal_ports",
           b.getDictionaryAttr(
               {f("source_inputs", b.getI64IntegerAttr(2)),
                f("source_outputs", b.getI64IntegerAttr(1)),
                f("root_inputs", b.getArrayAttr({b.getI64IntegerAttr(1)}))})),
         f("ac.input_validity",
           b.getArrayAttr({b.getI64IntegerAttr(2), b.getI64IntegerAttr(-1)})),
         f("ac.output_validity", b.getArrayAttr({b.getI64IntegerAttr(1)}))});
    state.addRegion();
    module = cast<ac::ModuleOp>(b.create(state));
    auto *body = new Block;
    module.getBody().push_back(body);
    payload = body->addArgument(bits(8), b.getUnknownLoc());
    selected = body->addArgument(bits(1), b.getUnknownLoc());
    available = body->addArgument(bits(1), b.getUnknownLoc());
    b.setInsertionPointToEnd(body);
    one = constant(1);
    auto result =
        selectPayload ? choose(selected, payload, constant(42, 8)) : payload;
    auto validity =
        selectPayload ? choose(selected, available, one) : available;
    yield = cast<ac::YieldOp>(
        op(ac::YieldOp::getOperationName(), {result, validity}, {}));
  }
  ac::ValueRequireOp require() {
    b.setInsertionPoint(yield);
    auto source = literal(1).getTree();
    auto required = cast<ac::ValueRequireOp>(
        op(ac::ValueRequireOp::getOperationName(),
           {payload, available, selected}, {bits(1)},
           {f("occurrence", source.get("origin")),
            f("source_location", source.get("location"))}));
    yield->setOperands(ValueRange{required.getResult()});
    module->setAttr("function_type",
                    TypeAttr::get(b.getFunctionType({bits(8), bits(1), bits(1)},
                                                    {bits(1)})));
    module->setAttr("output_names", b.getStrArrayAttr({"result"}));
    module->setAttr("ac.output_validity",
                    b.getArrayAttr({b.getI64IntegerAttr(-1)}));
    return required;
  }
  bool provesTransactionWiring() {
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &diagnostic) {
      llvm::raw_string_ostream stream(diagnostics);
      diagnostic.print(stream);
      return success();
    });
    ac::HardwareAnalysis analysis(*package);
    ac::HardwareBindings bindings;
    bindings.owner = module;
    ac::detail::SourceCheckBudget budget({1 << 22});
    ac::detail::SampledAdmissionValidation validation(analysis, budget);
    return succeeded(ac::detail::verifyTransactionWiring(
        analysis, {{module, bindings}}, validation, budget));
  }
  bool proves() {
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &diagnostic) {
      llvm::raw_string_ostream stream(diagnostics);
      diagnostic.print(stream);
      return success();
    });
    return succeeded(ac::detail::verifyHardwareValueAvailability(
        ac::HardwareAnalysis(*package)));
  }
};

TEST_F(HardwareValueAvailabilityTest,
       ValidityOnlyPhysicalSuffixNeedsNoTransactionCompanion) {
  build();
  ASSERT_FALSE(module->hasAttr("ac.transaction_wiring"));
  ASSERT_TRUE(proves()) << diagnostics;
  EXPECT_TRUE(provesTransactionWiring()) << diagnostics;
}

TEST_F(HardwareValueAvailabilityTest,
       UnexplainedPrivateInputOrOutputStillRequiresTransactionCompanion) {
  for (bool input : {false, true}) {
    SCOPED_TRACE(input);
    build();
    auto signature = module.getFunctionType();
    SmallVector<Type> inputs(signature.getInputs()), outputs(signature.getResults());
    if (input) {
      module.getBody().front().addArgument(bits(1), b.getUnknownLoc());
      inputs.push_back(bits(1));
      module->setAttr("input_names", b.getStrArrayAttr({"payload", "selected", "available", "unexplained"}));
      NamedAttrList layout(module->getAttrOfType<DictionaryAttr>("ac.internal_ports"));
      layout.set("root_inputs", b.getArrayAttr({b.getI64IntegerAttr(1), b.getI64IntegerAttr(0)}));
      module->setAttr("ac.internal_ports", layout.getDictionary(&context));
    } else {
      outputs.push_back(bits(1));
      SmallVector<Value> values(yield.getValues());
      values.push_back(one);
      yield->setOperands(values);
      module->setAttr("output_names", b.getStrArrayAttr({"result", "result_available", "unexplained"}));
    }
    module->setAttr("function_type", TypeAttr::get(b.getFunctionType(inputs, outputs)));
    ASSERT_TRUE(succeeded(ac::getModulePortLayout(module)));
    EXPECT_FALSE(provesTransactionWiring());
    EXPECT_NE(diagnostics.find("transaction_wiring companion"), std::string::npos)
        << diagnostics;
  }
}

TEST_F(HardwareValueAvailabilityTest,
       MissingOrForgedValidityMapDoesNotExemptArbitrarySuffix) {
  for (unsigned mutation = 0; mutation < 3; ++mutation) {
    SCOPED_TRACE(mutation);
    build();
    if (mutation == 0) {
      module->removeAttr("ac.input_validity");
      module->removeAttr("ac.output_validity");
    } else if (mutation == 1) {
      module->setAttr("ac.input_validity", b.getArrayAttr({b.getI64IntegerAttr(1), b.getI64IntegerAttr(-1)}));
    } else {
      module->setAttr("ac.output_validity", b.getArrayAttr({b.getI64IntegerAttr(0)}));
    }
    EXPECT_FALSE(provesTransactionWiring());
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(HardwareValueAvailabilityTest,
       WellFormedCompanionMapCannotRelabelActualDFFEEnableRoute) {
  build(false);
  ASSERT_TRUE(provesTransactionWiring()) << diagnostics;
  b.setInsertionPointToEnd(package->getBody());
  op(ac::ModuleImportOp::getOperationName(), {}, {}, {
      f("semantic_kind", b.getStringAttr("hardware")),
      f("sym_name", b.getStringAttr("storage")),
      f("source_owner", b.getDictionaryAttr({f("package", b.getStringAttr("gfsim")), f("path", b.getStringAttr("dffe.py"))})),
      f("parameters", b.getArrayAttr({})), f("type_parameters", b.getArrayAttr({})),
      f("function_type", TypeAttr::get(b.getFunctionType({bits(1), bits(1), bits(1), bits(8), bits(8)}, {bits(8)}))),
      f("input_names", b.getStrArrayAttr({"clk", "rst", "en", "d", "init"})),
      f("output_names", b.getStrArrayAttr({"q"})),
      f("primitive_kind", b.getStringAttr("dffe")),
      f("dependency_summary", b.getArrayAttr({b.getDictionaryAttr({
          f("output", b.getDictionaryAttr({f("port", b.getI64IntegerAttr(0)), f("path", b.getArrayAttr({}))})),
          f("inputs", b.getArrayAttr({}))})}))});
  b.setInsertionPoint(yield);
  auto zero = constant(0), initial = constant(0, 8);
  op(ac::InstanceOp::getOperationName(), {selected, zero, available, payload, initial}, {bits(8)}, {
      f("callee", FlatSymbolRefAttr::get(&context, "storage")),
      f("instance_name", b.getStringAttr("actual_state")),
      f("parameters", b.getArrayAttr({})), f("type_arguments", b.getArrayAttr({})),
      f("occurrence", literal(1).getTree().get("origin"))});
  // The port indices/defaults are well formed and cover the suffix exactly;
  // actual E/D use is what prevents availability metadata from authorizing it.
  ASSERT_TRUE(succeeded(ac::getModulePortLayout(module)));
  ASSERT_FALSE(module->hasAttr("ac.transaction_wiring"));
  EXPECT_FALSE(provesTransactionWiring());
  EXPECT_NE(diagnostics.find("data use"), std::string::npos) << diagnostics;
}

TEST_F(HardwareValueAvailabilityTest, SelectedArmOnlyRequiresItsPayload) {
  build();
  EXPECT_TRUE(proves()) << diagnostics;
}

TEST_F(HardwareValueAvailabilityTest,
       CannotSubstituteKnownOneForOutputCompanion) {
  build();
  yield->setOperand(1, one);
  EXPECT_FALSE(proves());
  EXPECT_NE(diagnostics.find("actual payload SSA"), std::string::npos);
}

TEST_F(HardwareValueAvailabilityTest, CannotUseAnotherInputAsCompanion) {
  build(false);
  yield->setOperand(1, selected);
  EXPECT_FALSE(proves());
}

TEST_F(HardwareValueAvailabilityTest,
       UnselectedPayloadDoesNotRequireAvailability) {
  build();
  auto choice = yield->getOperand(0).getDefiningOp<ac::BitsSelectOp>();
  b.setInsertionPoint(choice);
  choice->setOperand(0, constant(0));
  yield->setOperand(1, one);
  EXPECT_TRUE(proves()) << diagnostics;
}

TEST_F(HardwareValueAvailabilityTest, DeclaringOutputTotalDoesNotMakeItTotal) {
  build(false);
  module->setAttr("ac.output_validity",
                  b.getArrayAttr({b.getI64IntegerAttr(-1)}));
  EXPECT_FALSE(proves());
}
TEST_F(HardwareValueAvailabilityTest,
       RequiredAvailabilityHasNoPayloadDependency) {
  build();
  require();
  ASSERT_TRUE(proves()) << diagnostics;
  auto result = ac::HardwareAnalysis(*package).analyzeModule(module);
  ASSERT_TRUE(succeeded(result));
  ASSERT_EQ(result->dependencies.size(), 1u);
  SmallVector<unsigned> inputs;
  for (const auto &input : result->dependencies.front().inputs)
    inputs.push_back(input.port);
  llvm::sort(inputs);
  EXPECT_EQ(inputs, (SmallVector<unsigned>{1, 2}));
}

TEST_F(HardwareValueAvailabilityTest,
       RequiredAvailabilityCannotUseAnotherSignal) {
  build();
  auto required = require();
  required->setOperand(1, selected);
  EXPECT_FALSE(proves());
}

TEST_F(HardwareValueAvailabilityTest, CompanionCannotAlsoEscapeAsOrdinaryData) {
  build();
  b.setInsertionPoint(yield);
  auto leaked = op(ac::BitsResizeOp::getOperationName(), {available}, {bits(8)},
                   {f("mode", b.getStringAttr("zext"))})
                    ->getResult(0);
  yield->setOperand(0, leaked);
  yield->setOperand(1, one);
  EXPECT_FALSE(proves());
  EXPECT_NE(diagnostics.find("data use"), std::string::npos);
}
TEST_F(HardwareValueAvailabilityTest,
       PrivateCaptureExportKeepsItsAvailability) {
  build(false);
  auto layout = cast<DictionaryAttr>(module->getAttr("ac.internal_ports"));
  NamedAttrList fields(layout);
  fields.set("source_outputs", b.getI64IntegerAttr(0));
  module->setAttr("ac.internal_ports", fields.getDictionary(&context));
  module->setAttr("ac.output_validity", b.getArrayAttr({}));
  module->setAttr("ac.routed_output_validity",
                  b.getArrayAttr({b.getDictionaryAttr(
                      {f("payload", b.getI64IntegerAttr(0)),
                       f("validity", b.getI64IntegerAttr(1))})}));
  ASSERT_TRUE(proves()) << diagnostics;
  module->removeAttr("ac.routed_output_validity");
  EXPECT_FALSE(proves());
  EXPECT_NE(diagnostics.find("actual payload SSA"), std::string::npos);
}

TEST_F(HardwareValueAvailabilityTest,
       RoutedCompanionCannotAliasPublicCompanion) {
  build(false);
  module->setAttr("ac.routed_output_validity",
                  b.getArrayAttr({b.getDictionaryAttr(
                      {f("payload", b.getI64IntegerAttr(1)),
                       f("validity", b.getI64IntegerAttr(1))})}));
  EXPECT_FALSE(proves());
}
} // namespace
