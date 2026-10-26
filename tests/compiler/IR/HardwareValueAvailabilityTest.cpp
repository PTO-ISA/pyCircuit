#include "Dialect/ACIR/HardwareValueAvailability.h"
#include "Dialect/ACIR/HardwareSampledAdmission.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <functional>
#include <utility>

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
  std::pair<Value, Value> dottedFieldFixture() {
    build(false);
    b.setInsertionPoint(module);
    auto nestedType =
        ac::StructType::get(&context, b.getStringAttr("CollisionNested"));
    auto rootType =
        ac::StructType::get(&context, b.getStringAttr("CollisionRoot"));
    auto field = [&](StringRef name, Type type) {
      return b.getDictionaryAttr(
          {f("name", b.getStringAttr(name)), f("type", TypeAttr::get(type))});
    };
    op(ac::StructOp::getOperationName(), {}, {},
       {f("sym_name", b.getStringAttr("CollisionNested")),
        f("fields", b.getArrayAttr({field("b", bits(8))}))});
    // The native Struct contract admits these distinct, nonempty names.
    // A flat field named "a.b" is different from the path ["a", "b"].
    op(ac::StructOp::getOperationName(), {}, {},
       {f("sym_name", b.getStringAttr("CollisionRoot")),
        f("fields",
          b.getArrayAttr({field("a.b", bits(8)), field("a", nestedType)}))});
    b.setInsertionPoint(yield);
    auto nested =
        op(ac::StructCreateOp::getOperationName(), {payload}, {nestedType})
            ->getResult(0);
    auto record = op(ac::StructCreateOp::getOperationName(),
                     {constant(13, 8), nested}, {rootType})
                      ->getResult(0);
    auto flat = op(ac::StructGetOp::getOperationName(), {record}, {bits(8)},
                   {f("field", b.getStringAttr("a.b"))})
                    ->getResult(0);
    auto child = op(ac::StructGetOp::getOperationName(), {record}, {nestedType},
                    {f("field", b.getStringAttr("a"))})
                     ->getResult(0);
    auto deep = op(ac::StructGetOp::getOperationName(), {child}, {bits(8)},
                   {f("field", b.getStringAttr("b"))})
                    ->getResult(0);
    return {flat, deep};
  }
  std::string printPackage() {
    std::string text;
    llvm::raw_string_ostream out(text);
    package->print(out);
    return text;
  }
  ac::TableMapOp rowMap(ValueRange tables, ValueRange captures,
                       TypeRange results,
                       const std::function<SmallVector<Value>(Block *)> &body) {
    OpBuilder::InsertionGuard insertion(b);
    OperationState state(b.getUnknownLoc(), ac::TableMapOp::getOperationName());
    state.addOperands(tables);
    state.addOperands(captures);
    state.addTypes(results);
    state.addAttribute("shape", b.getArrayAttr({literal(2)}));
    state.addAttribute("operandSegmentSizes", b.getDenseI32ArrayAttr(
        {int32_t(tables.size()), int32_t(captures.size())}));
    state.addRegion();
    auto map = cast<ac::TableMapOp>(b.create(state));
    auto *block = new Block;
    map.getBody().push_back(block);
    block->addArgument(bits(1), b.getUnknownLoc());
    for (Value table : tables)
      block->addArgument(cast<ac::TableType>(table.getType()).getElementType(),
                        b.getUnknownLoc());
    for (Value capture : captures)
      block->addArgument(capture.getType(), b.getUnknownLoc());
    b.setInsertionPointToEnd(block);
    auto values = body(block);
    op(ac::YieldOp::getOperationName(), values, {});
    return map;
  }
  struct RowFixture {
    ac::TableMapOp data, companion;
    ac::TableFoldOp folded;
  };
  RowFixture rowFixture(bool overwritePartial = false, bool record = false) {
    build(false);
    Type element = bits(8);
    if (record) {
      b.setInsertionPoint(module);
      element = ac::StructType::get(&context, b.getStringAttr("Row"));
      auto field = [&](StringRef name) {
        return b.getDictionaryAttr({f("name", b.getStringAttr(name)),
                                    f("type", TypeAttr::get(bits(8)))});
      };
      op(ac::StructOp::getOperationName(), {}, {},
         {f("sym_name", b.getStringAttr("Row")),
          f("fields", b.getArrayAttr({field("a"), field("b")}))});
    }
    b.setInsertionPoint(yield);
    auto shape = b.getArrayAttr({literal(2)});
    auto tableType = ac::TableType::get(&context, shape, element);
    auto validityType = ac::TableType::get(&context, shape, bits(1));
    auto wrap = [&](Value value) {
      return record ? op(ac::StructCreateOp::getOperationName(),
                         {value, constant(13, 8)}, {element})->getResult(0)
                    : value;
    };
    auto base = op(ac::TableCreateOp::getOperationName(),
                   {wrap(overwritePartial ? payload : constant(7, 8)),
                    wrap(constant(9, 8))}, {tableType})->getResult(0);
    auto replacement = overwritePartial ? constant(42, 8) : payload;
    auto guard = [&](Value ordinal, Value condition) {
      auto equal = op(ac::BitsCompareOp::getOperationName(),
                      {ordinal, constant(0)}, {bits(1)},
                      {f("predicate", b.getStringAttr("eq"))})->getResult(0);
      return op(ac::BitsBinaryOp::getOperationName(), {equal, condition},
                {bits(1)}, {f("opcode", b.getStringAttr("and"))})->getResult(0);
    };
    auto data = rowMap({base}, {replacement, selected}, {tableType, tableType},
        [&](Block *block) -> SmallVector<Value> {
      auto condition = guard(block->getArgument(0), block->getArgument(3));
      auto path = record ? b.getArrayAttr({b.getStringAttr("a")})
                         : b.getArrayAttr({});
      auto merge = op(ac::ValueMergeOp::getOperationName(),
                      {block->getArgument(1), condition, block->getArgument(2)},
                      {element, bits(1)},
                      {f("operandSegmentSizes", b.getDenseI32ArrayAttr({1, 1, 1})),
                       f("paths", b.getArrayAttr({path}))});
      return {merge->getResult(0), block->getArgument(1)};
    });
    b.setInsertionPoint(yield);
    auto prior = op(ac::TableCreateOp::getOperationName(),
                    {overwritePartial ? available : one, one},
                    {validityType})->getResult(0);
    auto companion = rowMap({prior},
        {overwritePartial ? one : available, selected}, {validityType},
        [&](Block *block) -> SmallVector<Value> {
      auto condition = guard(block->getArgument(0), block->getArgument(3));
      return {choose(condition, block->getArgument(2), block->getArgument(1))};
    });
    b.setInsertionPoint(yield);
    auto folded = cast<ac::TableFoldOp>(op(ac::TableFoldOp::getOperationName(),
        {companion->getResult(0)}, {bits(1)}, {f("kind", b.getStringAttr("and"))}));
    yield->setOperands(ValueRange{data->getResult(0), data->getResult(1),
                                  folded.getResult(), overwritePartial ? available : one});
    module->setAttr("function_type", TypeAttr::get(b.getFunctionType(
        {bits(8), bits(1), bits(1)}, {tableType, tableType, bits(1), bits(1)})));
    module->setAttr("output_names", b.getStrArrayAttr(
        {"changed", "untouched", "changed_available", "untouched_available"}));
    NamedAttrList layout(module->getAttrOfType<DictionaryAttr>("ac.internal_ports"));
    layout.set("source_outputs", b.getI64IntegerAttr(2));
    module->setAttr("ac.internal_ports", layout.getDictionary(&context));
    module->setAttr("ac.output_validity", b.getArrayAttr(
        {b.getI64IntegerAttr(2), b.getI64IntegerAttr(3)}));
    return {data, companion, folded};
  }
};

TEST_F(HardwareValueAvailabilityTest,
       CompactRowsPreserveResultIdentityAndPartialOverwrite) {
  for (bool partial : {false, true}) {
    SCOPED_TRACE(partial);
    rowFixture(partial);
    ASSERT_TRUE(succeeded(verify(*package)));
    auto original = printPackage();
    EXPECT_TRUE(proves()) << diagnostics;
    EXPECT_TRUE(proves()) << diagnostics;
    EXPECT_EQ(printPackage(), original);
  }
}

TEST_F(HardwareValueAvailabilityTest,
       CompactRowsRejectWrongFoldResultPredicateAndForcedCompanion) {
  for (unsigned mutation = 0; mutation < 4; ++mutation) {
    SCOPED_TRACE(mutation);
    auto rows = rowFixture();
    ASSERT_TRUE(proves()) << diagnostics;
    if (mutation == 0)
      rows.folded->setAttr("kind", b.getStringAttr("or"));
    if (mutation == 1)
      yield->setOperand(0, rows.data->getResult(1));
    if (mutation == 2) {
      auto &body = rows.companion.getBody().front();
      auto compare = *body.getOps<ac::BitsCompareOp>().begin();
      compare->setAttr("predicate", b.getStringAttr("ne"));
      // Flip the scalar selection as well: selecting the other row alone does
      // not change whole-Table availability for this one-partial-row fixture.
      auto selection = *body.getOps<ac::BitsSelectOp>().begin();
      selection->setOperands({selection.getCondition(), selection.getFalseValue(),
                               selection.getTrueValue()});
    }
    if (mutation == 3)
      yield->setOperand(2, one);
    ASSERT_TRUE(succeeded(verify(*package)));
    EXPECT_FALSE(proves()) << diagnostics;
  }
}

TEST_F(HardwareValueAvailabilityTest,
       CompactRowsPreserveNominalFieldsAndRejectRetargeting) {
  auto rows = rowFixture(true, true);
  ASSERT_TRUE(succeeded(verify(*package)));
  ASSERT_TRUE(proves()) << diagnostics;
  auto merge = *rows.data.getBody().front().getOps<ac::ValueMergeOp>().begin();
  merge->setAttr("paths", b.getArrayAttr({b.getArrayAttr({b.getStringAttr("b")})}));
  ASSERT_TRUE(succeeded(verify(*package)));
  EXPECT_FALSE(proves()) << diagnostics;
}

TEST_F(HardwareValueAvailabilityTest,
       CompactRowsRejectAttributeBoundaryPredicateCollision) {
  for (unsigned mutation = 0; mutation < 3; ++mutation) {
    SCOPED_TRACE(mutation);
    auto rows = rowFixture(true);
    ASSERT_TRUE(proves()) << diagnostics;
    auto data = *rows.data.getBody().front().getOps<ac::BitsCompareOp>().begin();
    auto companion =
        *rows.companion.getBody().front().getOps<ac::BitsCompareOp>().begin();
    // Same-predicate metadata is a valid control. These inert strings must not
    // let a different real predicate impersonate the companion's comparison.
    auto prefix = b.getStringAttr("x:predicate:string:ne:z:string:y");
    data->setAttr("a", prefix);
    companion->setAttr("a", prefix);
    if (mutation) {
      data->setAttr("predicate", b.getStringAttr("ne"));
      data->removeAttr("a");
      if (mutation == 2) {
        // Without boundaries both attribute sequences serialize identically:
        // :a:string:x:predicate:string:ne:z:string:y:predicate:string:eq
        data->setAttr("a", b.getStringAttr("x"));
        data->setAttr("z", b.getStringAttr("y:predicate:string:eq"));
      }
      // At selected=1, available=0 the NE payload map retains unavailable row
      // zero; the EQ companion overwrites its availability with known one.
    }
    ASSERT_TRUE(succeeded(verify(*package)));
    auto text = printPackage();
    auto objects = [&] {
      SmallVector<Operation *> result;
      package->walk([&](Operation *operation) { result.push_back(operation); });
      return result;
    };
    auto originalObjects = objects();
    if (!mutation)
      EXPECT_TRUE(proves()) << diagnostics;
    else {
      EXPECT_FALSE(proves()) << diagnostics;
      EXPECT_NE(diagnostics.find("availability differs from actual payload SSA"),
                std::string::npos)
          << diagnostics;
    }
    EXPECT_EQ(printPackage(), text);
    EXPECT_EQ(objects(), originalObjects);
  }
}

TEST_F(HardwareValueAvailabilityTest,
       CompactCompanionCannotAlsoFeedARawRegionPredicate) {
  auto rows = rowFixture();
  ASSERT_TRUE(proves()) << diagnostics;
  auto &body = rows.companion.getBody().front();
  b.setInsertionPoint(body.getTerminator());
  // This additional predicate is not an availability transfer. Even an
  // otherwise correct folded companion cannot authorize the extra use.
  op(ac::BitsCompareOp::getOperationName(),
     {body.getArgument(2), constant(0)}, {bits(1)},
     {f("predicate", b.getStringAttr("eq"))});
  ASSERT_TRUE(succeeded(verify(*package)));
  EXPECT_FALSE(proves());
  EXPECT_NE(diagnostics.find("data use"), std::string::npos) << diagnostics;
}

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
       DottedAndNestedFieldsKeepTheirActualAvailabilityInEitherOperandOrder) {
  for (bool flatFirst : {true, false}) {
    SCOPED_TRACE(flatFirst);
    auto [flat, deep] = dottedFieldFixture();
    auto sum = op(ac::BitsBinaryOp::getOperationName(),
                  flatFirst ? ValueRange{flat, deep} : ValueRange{deep, flat},
                  {bits(8)}, {f("opcode", b.getStringAttr("add"))})
                   ->getResult(0);
    yield->setOperand(0, sum);
    auto original = printPackage();
    for (bool forged : {false, true, false}) {
      SCOPED_TRACE(forged);
      yield->setOperand(1, forged ? one : available);
      ASSERT_TRUE(succeeded(verify(*package)));
      auto before = printPackage();
      // Both add operands are live: the literal flat field is total and the
      // nested field is the original partial input, in either evaluation order.
      EXPECT_EQ(proves(), !forged) << diagnostics;
      EXPECT_EQ(proves(), !forged) << diagnostics;
      EXPECT_EQ(printPackage(), before);
    }
    EXPECT_EQ(printPackage(), original);
  }
}

TEST_F(HardwareValueAvailabilityTest,
       PartialNestedProjectionCannotRelabelALaterTotalDottedField) {
  auto [flat, deep] = dottedFieldFixture();
  yield->setOperands(ValueRange{deep, flat, available, one});
  module->setAttr("function_type", TypeAttr::get(b.getFunctionType(
                                       {bits(8), bits(1), bits(1)},
                                       {bits(8), bits(8), bits(1), bits(1)})));
  module->setAttr("output_names",
                  b.getStrArrayAttr({"nested", "flat", "nested_available",
                                     "flat_available"}));
  NamedAttrList layout(
      module->getAttrOfType<DictionaryAttr>("ac.internal_ports"));
  layout.set("source_outputs", b.getI64IntegerAttr(2));
  module->setAttr("ac.internal_ports", layout.getDictionary(&context));
  module->setAttr(
      "ac.output_validity",
      b.getArrayAttr({b.getI64IntegerAttr(2), b.getI64IntegerAttr(3)}));
  auto original = printPackage();
  for (bool forged : {false, true, false}) {
    SCOPED_TRACE(forged);
    yield->setOperand(3, forged ? available : one);
    ASSERT_TRUE(succeeded(verify(*package)));
    auto before = printPackage();
    // This one full verification visits the nested projection first, then the
    // total flat projection. A cached partial result must not cross that path.
    EXPECT_EQ(proves(), !forged) << diagnostics;
    EXPECT_EQ(proves(), !forged) << diagnostics;
    EXPECT_EQ(printPackage(), before);
  }
  EXPECT_EQ(printPackage(), original);
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
