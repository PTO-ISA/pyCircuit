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
  Value conjunction(Value left, Value right) {
    return op(ac::BitsBinaryOp::getOperationName(), {left, right}, {bits(1)},
              {f("opcode", b.getStringAttr("and"))})
        ->getResult(0);
  }
  struct MergeFixture {
    ac::ValueMergeOp merge;
    Value field, otherField, whole, premature, condition;
  };
  MergeFixture aggregateMergeFixture(bool table, bool partialGuard = false,
                                     bool identity = false,
                                     StringRef selector = "") {
    build(false);
    b.setInsertionPoint(module);
    auto record = ac::StructType::get(&context, b.getStringAttr("Pair"));
    auto field = [&](StringRef name) {
      return b.getDictionaryAttr({f("name", b.getStringAttr(name)),
                                  f("type", TypeAttr::get(bits(8)))});
    };
    op(ac::StructOp::getOperationName(), {}, {},
       {f("sym_name", b.getStringAttr("Pair")),
        f("fields", b.getArrayAttr({field("a"), field("b")}))});
    b.setInsertionPoint(yield);
    Value condition = selected;
    if (!selector.empty())
      condition = op(ac::BitsConstant4Op::getOperationName(), {}, {bits(1)},
                     {f("pattern", b.getStringAttr(selector))})
                      ->getResult(0);
    Value guardAvailable = one;
    if (partialGuard) {
      guardAvailable = module.getBody().front().addArgument(bits(1), b.getUnknownLoc());
      module->setAttr("input_names", b.getStrArrayAttr(
          {"payload", "selected", "available", "guard_available"}));
      module->setAttr("ac.input_validity", b.getArrayAttr(
          {b.getI64IntegerAttr(2), b.getI64IntegerAttr(3)}));
      NamedAttrList layout(module->getAttrOfType<DictionaryAttr>("ac.internal_ports"));
      layout.set("root_inputs", b.getArrayAttr(
          {b.getI64IntegerAttr(1), b.getI64IntegerAttr(1)}));
      module->setAttr("ac.internal_ports", layout.getDictionary(&context));
    }
    Value yes = op(ac::StructCreateOp::getOperationName(),
                   {payload, constant(13, 8)}, {record})
                    ->getResult(0);
    Value no = op(ac::StructCreateOp::getOperationName(),
                  {constant(21, 8), payload}, {record})
                   ->getResult(0);
    auto shape = b.getArrayAttr({literal(2)});
    if (table) {
      auto type = ac::TableType::get(&context, shape, record);
      auto trueRows =
          op(ac::TableCreateOp::getOperationName(), {yes, no}, {type})
              ->getResult(0);
      no = op(ac::TableCreateOp::getOperationName(), {no, yes}, {type})
               ->getResult(0);
      yes = trueRows;
    }
    SmallVector<Value> operands{no};
    if (!identity) {
      operands.push_back(condition);
      operands.push_back(yes);
    }
    auto merge = cast<ac::ValueMergeOp>(op(
        ac::ValueMergeOp::getOperationName(), operands, {no.getType(), bits(1)},
        {f("operandSegmentSizes",
           b.getDenseI32ArrayAttr({1, identity ? 0 : 1, identity ? 0 : 1})),
         f("paths", identity ? b.getArrayAttr({})
                             : b.getArrayAttr({b.getArrayAttr({})}))}));
    // Build each independently expected field before any row/field reduction.
    Value a = identity ? one : choose(condition, available, one);
    Value other = identity ? available : choose(condition, one, available);
    if (partialGuard) {
      a = conjunction(guardAvailable, a);
      other = conjunction(guardAvailable, other);
    }
    Value whole = conjunction(a, other);
    Value premature =
        identity ? available : choose(condition, available, available);
    if (partialGuard)
      premature = conjunction(guardAvailable, premature);
    if (table && !identity) {
      auto boolTable = ac::TableType::get(&context, shape, bits(1));
      auto forward = op(ac::TableCreateOp::getOperationName(), {available, one},
                        {boolTable})
                         ->getResult(0);
      auto reverse = op(ac::TableCreateOp::getOperationName(), {one, available},
                        {boolTable})
                         ->getResult(0);
      auto rows = rowMap(
          {forward, reverse}, {condition, guardAvailable},
          {boolTable}, [&](Block *block) -> SmallVector<Value> {
            auto first = choose(block->getArgument(3), block->getArgument(1),
                                block->getArgument(2));
            auto second = choose(block->getArgument(3), block->getArgument(2),
                                 block->getArgument(1));
            return {
                conjunction(block->getArgument(4), conjunction(first, second))};
          });
      b.setInsertionPoint(yield);
      whole = op(ac::TableFoldOp::getOperationName(), {rows->getResult(0)},
                 {bits(1)}, {f("kind", b.getStringAttr("and"))})
                  ->getResult(0);
    }
    SmallVector<Type> inputs{bits(8), bits(1), bits(1)};
    if (partialGuard)
      inputs.push_back(bits(1));
    module->setAttr("function_type", TypeAttr::get(b.getFunctionType(
        inputs, {no.getType(), bits(1)})));
    yield->setOperands({merge.getNext(), whole});
    return {merge, a, other, whole, premature, condition};
  }
  void pruneUnusedCompanionAlternatives() {
    // Fixtures author multiple independent expectations. Keep only the selected
    // expectation before verification: dead multi-op companion routes are not
    // authorized private-control transports, even when they expose no output.
    bool changed;
    do {
      changed = false;
      for (Operation &operation : llvm::make_early_inc_range(
               llvm::reverse(module.getBody().front()))) {
        if (isa<ac::BitsSelectOp, ac::BitsBinaryOp, ac::TableCreateOp,
                ac::TableSplatOp, ac::TableMapOp, ac::TableFoldOp>(&operation) &&
            operation.use_empty()) {
          operation.erase();
          changed = true;
        }
      }
    } while (changed);
  }
  void exposeMergeField(const MergeFixture &fixture, bool table, Value index,
                        StringRef field, Value expected) {
    b.setInsertionPoint(yield);
    auto merge = fixture.merge;
    Value value = merge.getNext();
    if (table)
      value =
          op(ac::TableGetOp::getOperationName(), {value, index},
             {cast<ac::TableType>(value.getType()).getElementType(), bits(1)})
              ->getResult(0);
    value = op(ac::StructGetOp::getOperationName(), {value}, {bits(8)},
               {f("field", b.getStringAttr(field))})
                ->getResult(0);
    yield->setOperands({value, expected});
    module->setAttr("function_type",
                    TypeAttr::get(b.getFunctionType({bits(8), bits(1), bits(1)},
                                                    {bits(8), bits(1)})));
  }
  struct NestedFixture {
    ac::ValueMergeOp merge;
    Value availability, guard, expected;
  };
  NestedFixture nestedFixture(unsigned mode = 0, StringRef pattern = "") {
  build(false);
  b.setInsertionPoint(module);
  auto shape = b.getArrayAttr({literal(2)});
  auto inner = ac::TableType::get(&context, shape, bits(8));
  auto record = ac::StructType::get(&context, b.getStringAttr("NestedTable"));
  op(ac::StructOp::getOperationName(), {}, {},
     {f("sym_name", b.getStringAttr("NestedTable")),
      f("fields", b.getArrayAttr({b.getDictionaryAttr(
          {f("name", b.getStringAttr("t")), f("type", TypeAttr::get(inner))})}))});
  auto outer = ac::TableType::get(&context, shape, record);
  auto &body = module.getBody().front();
  body.getArgument(0).setType(outer);
  body.getArgument(1).setType(outer);
  auto guardAvailability = body.addArgument(bits(1), b.getUnknownLoc());
  module->setAttr("input_names", b.getStrArrayAttr(
      {"base", "candidate", "guard", "guard_available"}));
  module->setAttr("function_type", TypeAttr::get(b.getFunctionType(
      {outer, outer, bits(1), bits(1)}, {outer, bits(1)})));
  module->setAttr("ac.input_validity", b.getArrayAttr(
      {b.getI64IntegerAttr(mode == 1 ? 3 : -1),
       b.getI64IntegerAttr((mode == 2 || mode == 3) ? 3 : -1),
       b.getI64IntegerAttr(mode == 0 ? 3 : -1)}));
  NamedAttrList layout(module->getAttrOfType<DictionaryAttr>("ac.internal_ports"));
  layout.set("source_inputs", b.getI64IntegerAttr(3));
  module->setAttr("ac.internal_ports", layout.getDictionary(&context));
  b.setInsertionPoint(yield);
  auto guard = (mode == 1 || mode == 2) ? constant(mode == 1 ? 1 : 0) : body.getArgument(2);
  if (!pattern.empty()) {
    auto value = pattern == "0" || pattern == "1" ? constant(pattern == "1")
        : op(ac::BitsConstant4Op::getOperationName(), {}, {bits(1)},
             {f("pattern", b.getStringAttr(pattern))})->getResult(0);
    // Equal data arms retain the input guard's availability while fixing its
    // payload, including Z. Raw data and its availability remain distinct.
    guard = mode == 0 ? choose(guard, value, value) : value;
  }
  auto merged = op(ac::ValueMergeOp::getOperationName(),
      {body.getArgument(0), guard, body.getArgument(1)},
      {outer, bits(1)},
      {f("paths", b.getArrayAttr({b.getArrayAttr({})})),
       f("operandSegmentSizes", b.getDenseI32ArrayAttr({1, 1, 1}))});
  auto expected = mode == 3 ? choose(guard, guardAvailability, one)
                  : mode ? one : guardAvailability;
  yield->setOperands({merged->getResult(0), expected});
  return {cast<ac::ValueMergeOp>(merged), guardAvailability, guard, expected};
  }

};

TEST_F(HardwareValueAvailabilityTest,
       AggregateMergeSelectsEachFieldBeforeWholeReduction) {
  for (bool table : {false, true}) {
    for (StringRef selector : {StringRef(""), StringRef("0"), StringRef("1"),
                               StringRef("x"), StringRef("z")}) {
      for (unsigned mutation = 0; mutation < 3; ++mutation) {
        SCOPED_TRACE(table);
        SCOPED_TRACE(selector.str());
        SCOPED_TRACE(mutation);
        // Known selectors legitimately agree with premature reduction.
        if (mutation == 1 && (selector == "0" || selector == "1"))
          continue;
        auto fixture = aggregateMergeFixture(table, false, false, selector);
        if (mutation)
          yield->setOperand(1, mutation == 1 ? fixture.premature : one);
        pruneUnusedCompanionAlternatives();
        ASSERT_TRUE(succeeded(verify(*package)));
        auto before = printPackage();
        // At selector X/Z and available=0, the correct result is X. Folding
        // each arm before selection incorrectly returns 0.
        EXPECT_EQ(proves(), mutation == 0) << diagnostics;
        if (mutation)
          EXPECT_NE(diagnostics.find("availability differs from actual payload SSA"),
                    std::string::npos) << diagnostics;
        EXPECT_EQ(before, printPackage());
      }
    }
  }
}

TEST_F(HardwareValueAvailabilityTest,
       AggregateMergePreservesConstantAndDynamicRowFieldAvailability) {
  for (unsigned row = 0; row < 3; ++row) {
    for (StringRef field : {StringRef("a"), StringRef("b")}) {
      for (unsigned mutation = 0; mutation < 4; ++mutation) {
        SCOPED_TRACE(row);
        SCOPED_TRACE(field.str());
        SCOPED_TRACE(mutation);
        auto fixture = aggregateMergeFixture(true);
        b.setInsertionPoint(yield);
        Value index = row == 2 ? selected : constant(row);
        Value first = field == "a" ? fixture.field : fixture.otherField;
        Value second = field == "a" ? fixture.otherField : fixture.field;
        Value expected = row ? second : first;
        if (row == 2) {
          // Preserve raw TableGet's baseline membership cascade, including its
          // availability-one out-of-range default. A direct one-bit select is
          // not equivalent when the index and both selected rows are unknown.
          auto equal = [&](unsigned ordinal) {
            return op(ac::BitsCompareOp::getOperationName(),
                {index, constant(ordinal)}, {bits(1)},
                {f("predicate", b.getStringAttr("eq"))})->getResult(0);
          };
          expected = choose(equal(0), first, choose(equal(1), second, one));
        }
        exposeMergeField(fixture, true, index, field, expected);
        // Whole broadcast, crossed row, and forced-one companions are wrong
        // for this independently demanded field.
        if (mutation)
          yield->setOperand(1, mutation == 1 ? fixture.whole
                               : mutation == 2 ? (row ? first : second)
                               : one);
        pruneUnusedCompanionAlternatives();
        ASSERT_TRUE(succeeded(verify(*package)));
        auto before = printPackage();
        EXPECT_EQ(proves(), mutation == 0) << diagnostics;
        if (mutation)
          EXPECT_NE(diagnostics.find("availability differs from actual payload SSA"),
                    std::string::npos) << diagnostics;
        EXPECT_EQ(before, printPackage());
      }
    }
  }
}

TEST_F(HardwareValueAvailabilityTest,
       DynamicRowMembershipKeepsUnknownIndexDistinctFromUnavailableRows) {
  for (bool collapse : {false, true}) {
    SCOPED_TRACE(collapse);
    build(false);
    b.setInsertionPoint(yield);
    auto type = ac::TableType::get(&context, b.getArrayAttr({literal(2)}), bits(8));
    auto table = op(ac::TableCreateOp::getOperationName(), {payload, payload},
                    {type})->getResult(0);
    auto read = op(ac::TableGetOp::getOperationName(), {table, selected},
                   {bits(8), bits(1)})->getResult(0);
    auto equal = [&](unsigned row) {
      return op(ac::BitsCompareOp::getOperationName(),
          {selected, constant(row)}, {bits(1)},
          {f("predicate", b.getStringAttr("eq"))})->getResult(0);
    };
    // With index X and both payload rows unavailable(0), the baseline raw-read
    // cascade yields X. Collapsing equal rows to availability0 changes it.
    Value companion = collapse ? available : choose(equal(0), available,
        choose(equal(1), available, one));
    yield->setOperands({read, companion});
    ASSERT_TRUE(succeeded(verify(*package)));
    auto before = printPackage();
    EXPECT_EQ(proves(), !collapse) << diagnostics;
    if (collapse)
      EXPECT_NE(diagnostics.find("availability differs from actual payload SSA"),
                std::string::npos) << diagnostics;
    EXPECT_EQ(before, printPackage());
  }
}

TEST_F(HardwareValueAvailabilityTest,
       BaselineNestedTableMergeKeepsGuardAvailability) {
  // All four native profiles were accepted by the clean b986 verifier. The
  // symbolic companion independently covers availability 0, 1 and X.
  for (unsigned mode = 0; mode < 4; ++mode) {
    for (StringRef pattern : {StringRef(""), StringRef("0"), StringRef("1"),
                              StringRef("x"), StringRef("z")}) {
      if ((mode == 1 || mode == 2) && !pattern.empty()) continue;
      SCOPED_TRACE(mode);
      SCOPED_TRACE(pattern.str());
      auto fixture = nestedFixture(mode, pattern);
      ASSERT_TRUE(succeeded(verify(*package)));
      EXPECT_TRUE(proves()) << diagnostics;
    }
  }
}

TEST_F(HardwareValueAvailabilityTest,
       NestedKnownSelectorAcceptsComputedAndFourStateConstants) {
  for (unsigned mode : {1u, 2u}) {
    for (bool computed : {false, true}) {
      for (bool forged : {false, true}) {
        SCOPED_TRACE(mode);
        SCOPED_TRACE(computed);
        SCOPED_TRACE(forged);
        auto fixture = nestedFixture(mode);
        b.setInsertionPoint(fixture.merge);
        Value guard;
        if (computed) {
          auto tree = literal(mode == 1 ? 1 : 0).getTree();
          NamedAttrList expression(tree);
          expression.erase("value");
          expression.set("kind", b.getStringAttr("binary"));
          expression.set("operator", b.getStringAttr("add"));
          expression.set("lhs", literal(mode == 1 ? 1 : 0));
          expression.set("rhs", literal(0));
          auto value = ac::StaticExprAttr::get(&context, expression.getDictionary(&context));
          guard = op(ac::BitsConstantOp::getOperationName(), {}, {bits(1)},
                      {f("value", value)})->getResult(0);
        } else {
          guard = op(ac::BitsConstant4Op::getOperationName(), {}, {bits(1)},
                      {f("pattern", b.getStringAttr(mode == 1 ? "1" : "0"))})->getResult(0);
        }
        fixture.merge->setOperand(1, guard);
        if (forged) yield->setOperand(1, fixture.availability);
        pruneUnusedCompanionAlternatives();
        ASSERT_TRUE(succeeded(verify(*package)));
        auto before = printPackage();
        EXPECT_EQ(proves(), !forged) << diagnostics;
        EXPECT_EQ(before, printPackage());
      }
    }
  }
}

TEST_F(HardwareValueAvailabilityTest,
       ScalarNominalMergeSelectorsUseSemanticConstantProof) {
  for (unsigned kind = 0; kind < 3; ++kind) {
    for (StringRef pattern : {StringRef("0"), StringRef("1"), StringRef("x"), StringRef("z")}) {
      for (bool computed : {false, true}) {
        if (computed && (pattern == "x" || pattern == "z")) continue;
        for (bool forged : {false, true}) {
          SCOPED_TRACE(kind);
          SCOPED_TRACE(pattern.str());
          SCOPED_TRACE(computed);
          SCOPED_TRACE(forged);
          build(false);
          Type type = bits(8);
          b.setInsertionPoint(module);
          if (kind == 1) {
            type = ac::EnumType::get(&context, b.getStringAttr("Carrier"));
            op(ac::EnumOp::getOperationName(), {}, {},
               {f("sym_name", b.getStringAttr("Carrier")),
                f("width", parseAttribute("#ac.math_int<8>", &context)),
                f("encoding", b.getStringAttr("explicit")),
                f("members", b.getArrayAttr({b.getDictionaryAttr(
                    {f("name", b.getStringAttr("Only")),
                     f("code", parseAttribute("#ac.math_int<7>", &context))})}))});
          } else if (kind == 2) {
            type = ac::StructType::get(&context, b.getStringAttr("Pair"));
            auto field = [&](StringRef name) {
              return b.getDictionaryAttr({f("name", b.getStringAttr(name)),
                                          f("type", TypeAttr::get(bits(8)))});
            };
            op(ac::StructOp::getOperationName(), {}, {},
               {f("sym_name", b.getStringAttr("Pair")),
                f("fields", b.getArrayAttr({field("a"), field("b")}))});
          }
          b.setInsertionPoint(yield);
          auto wrap = [&](Value value) -> Value {
            if (kind == 1)
              return op(ac::EnumFromBitsOp::getOperationName(), {value},
                        {type, bits(1)})->getResult(0);
            if (kind == 2)
              return op(ac::StructCreateOp::getOperationName(), {value, value},
                        {type})->getResult(0);
            return value;
          };
          auto yes = wrap(payload), no = wrap(constant(42, 8));
          Value guard;
          if (computed) {
            NamedAttrList tree(literal(pattern == "1").getTree());
            tree.erase("value");
            tree.set("kind", b.getStringAttr("binary"));
            tree.set("operator", b.getStringAttr("add"));
            tree.set("lhs", literal(pattern == "1"));
            tree.set("rhs", literal(0));
            guard = op(ac::BitsConstantOp::getOperationName(), {}, {bits(1)},
                {f("value", ac::StaticExprAttr::get(&context, tree.getDictionary(&context)))})
                ->getResult(0);
          } else {
            guard = op(ac::BitsConstant4Op::getOperationName(), {}, {bits(1)},
                        {f("pattern", b.getStringAttr(pattern))})->getResult(0);
          }
          auto merge = op(ac::ValueMergeOp::getOperationName(), {no, guard, yes},
              {type, bits(1)},
              {f("paths", b.getArrayAttr({b.getArrayAttr({})})),
               f("operandSegmentSizes", b.getDenseI32ArrayAttr({1, 1, 1}))});
          Value expected = pattern == "0" ? one : pattern == "1" ? available
                           : choose(guard, available, one);
          Value wrong = pattern == "0" ? available : one;
          yield->setOperands({merge->getResult(0), forged ? wrong : expected});
          module->setAttr("function_type", TypeAttr::get(b.getFunctionType(
              {bits(8), bits(1), bits(1)}, {type, bits(1)})));
          pruneUnusedCompanionAlternatives();
          ASSERT_TRUE(succeeded(verify(*package)));
          auto before = printPackage();
          EXPECT_EQ(proves(), !forged) << diagnostics;
          EXPECT_EQ(before, printPackage());
        }
      }
    }
  }
}

TEST_F(HardwareValueAvailabilityTest,
       NestedUniformMergeRejectsFalseWholeAvailabilityAndEnable) {
  for (unsigned mutation = 0; mutation < 4; ++mutation) {
    SCOPED_TRACE(mutation);
    auto fixture = nestedFixture();
    b.setInsertionPoint(yield);
    Value forged = mutation == 0 ? one : mutation == 1 ? constant(0)
                   : mutation == 2 ? fixture.guard : fixture.merge.getEn();
    yield->setOperand(1, forged);
    pruneUnusedCompanionAlternatives();
    auto before = printPackage();
    ASSERT_TRUE(succeeded(verify(*package)));
    EXPECT_FALSE(proves()) << diagnostics;
    EXPECT_EQ(before, printPackage());
  }
}

TEST_F(HardwareValueAvailabilityTest,
       NestedTotalPayloadUnknownsIdentityAndSharedUniformDAG) {
  for (unsigned variant = 0; variant < 3; ++variant) {
    SCOPED_TRACE(variant); // literal X/Z payload, identity, shared merge DAG
    auto fixture = nestedFixture();
    b.setInsertionPoint(fixture.merge);
    auto type = cast<ac::TableType>(fixture.merge.getNext().getType());
    auto inner = ac::TableType::get(&context, type.getShape(), bits(8));
    auto record = type.getElementType();
    auto wrap = [&](StringRef pattern) {
      auto data = op(ac::BitsConstant4Op::getOperationName(), {}, {bits(8)},
                     {f("pattern", b.getStringAttr(pattern))})->getResult(0);
      auto rows = op(ac::TableSplatOp::getOperationName(), {data}, {inner},
                     {f("shape", inner.getShape())})->getResult(0);
      auto value = op(ac::StructCreateOp::getOperationName(), {rows},
                      {record})->getResult(0);
      return op(ac::TableSplatOp::getOperationName(), {value}, {type},
                 {f("shape", type.getShape())})->getResult(0);
    };
    auto left = wrap("xxzz10xz"), right = wrap("xxzz01zx");
    if (variant == 2) {
      for (unsigned level = 0; level < 8; ++level) {
        auto shared = op(ac::ValueMergeOp::getOperationName(),
            {left, one, right}, {type, bits(1)},
            {f("paths", b.getArrayAttr({b.getArrayAttr({})})),
             f("operandSegmentSizes", b.getDenseI32ArrayAttr({1, 1, 1}))});
        left = right = shared->getResult(0);
      }
    }
    if (variant == 1) {
      fixture.merge->setOperands({left});
      fixture.merge->setAttr("paths", b.getArrayAttr({}));
      fixture.merge->setAttr("operandSegmentSizes", b.getDenseI32ArrayAttr({1, 0, 0}));
      yield->setOperand(1, one);
    } else {
      fixture.merge->setOperand(0, left);
      fixture.merge->setOperand(2, right);
    }
    ASSERT_TRUE(succeeded(verify(*package)));
    auto before = printPackage();
    EXPECT_TRUE(proves()) << diagnostics;
    EXPECT_EQ(before, printPackage());
  }
}

TEST_F(HardwareValueAvailabilityTest,
       SymbolicNestedUniformMergeRetainsDeclarationShape) {
  auto fixture = nestedFixture(3);
  auto extent = cast<ac::StaticExprAttr>(parseAttribute(
      "#ac.static_expr<{kind = \"reference\", ref = {kind = \"parameter\", "
      "owner = @Root, name = \"N\"}, origin = {site = {definition = @Root, "
      "ast_path = []}, expansion = []}, location = {path = \"availability.py\", "
      "line = 1 : i64, column = 1 : i64, end_line = 1 : i64, "
      "end_column = 2 : i64}}>", &context));
  module->setAttr("parameters", b.getArrayAttr({b.getDictionaryAttr(
      {f("name", b.getStringAttr("N")),
       f("type", TypeAttr::get(parseType("!ac.math_int", &context)))})}));
  auto type = ac::TableType::get(&context, b.getArrayAttr({extent}),
      cast<ac::TableType>(fixture.merge.getNext().getType()).getElementType());
  auto &body = module.getBody().front();
  body.getArgument(0).setType(type);
  body.getArgument(1).setType(type);
  fixture.merge.getNext().setType(type);
  module->setAttr("function_type", TypeAttr::get(b.getFunctionType(
      {type, type, bits(1), bits(1)}, {type, bits(1)})));
  ASSERT_TRUE(succeeded(verify(*package)));
  auto before = printPackage();
  EXPECT_TRUE(proves()) << diagnostics;
  EXPECT_EQ(before, printPackage());
}

TEST_F(HardwareValueAvailabilityTest,
       NestedNonuniformFieldsAndRowsCannotMasqueradeAsUniformZero) {
  for (bool fields : {false, true}) {
    for (bool uniform : {false, true}) {
      SCOPED_TRACE(fields);
      SCOPED_TRACE(uniform);
      build(false);
      b.setInsertionPoint(module);
      auto shape = b.getArrayAttr({literal(2)});
      Type element = bits(8);
      if (fields) {
        element = ac::StructType::get(&context, b.getStringAttr("InnerPair"));
        auto field = [&](StringRef name) {
          return b.getDictionaryAttr({f("name", b.getStringAttr(name)),
                                      f("type", TypeAttr::get(bits(8)))});
        };
        op(ac::StructOp::getOperationName(), {}, {},
           {f("sym_name", b.getStringAttr("InnerPair")),
            f("fields", b.getArrayAttr({field("a"), field("b")}))});
      }
      auto inner = ac::TableType::get(&context, shape, element);
      auto record = ac::StructType::get(&context, b.getStringAttr("Nested"));
      op(ac::StructOp::getOperationName(), {}, {},
         {f("sym_name", b.getStringAttr("Nested")),
          f("fields", b.getArrayAttr({b.getDictionaryAttr(
              {f("name", b.getStringAttr("t")), f("type", TypeAttr::get(inner))})}))});
      auto outer = ac::TableType::get(&context, shape, record);
      b.setInsertionPoint(yield);
      auto known = constant(7, 8);
      auto wrap = [&](bool reverse) {
        Value a = uniform || !reverse ? payload : known;
        Value bValue = uniform || reverse ? payload : known;
        Value rows;
        if (fields) {
          auto pair = op(ac::StructCreateOp::getOperationName(), {a, bValue},
                         {element})->getResult(0);
          rows = op(ac::TableSplatOp::getOperationName(), {pair}, {inner},
                    {f("shape", shape)})->getResult(0);
        } else {
          rows = op(ac::TableCreateOp::getOperationName(), {a, bValue},
                    {inner})->getResult(0);
        }
        auto entry = op(ac::StructCreateOp::getOperationName(), {rows},
                        {record})->getResult(0);
        return op(ac::TableSplatOp::getOperationName(), {entry}, {outer},
                  {f("shape", shape)})->getResult(0);
      };
      auto no = wrap(false), yes = wrap(true);
      auto merged = op(ac::ValueMergeOp::getOperationName(), {no, selected, yes},
          {outer, bits(1)},
          {f("paths", b.getArrayAttr({b.getArrayAttr({})})),
           f("operandSegmentSizes", b.getDenseI32ArrayAttr({1, 1, 1}))});
      // At available=0, nonuniform arms have complementary [0,1]/[1,0]
      // leaves. Their whole folds are0, but selection by X must stay X.
      // Uniform arms genuinely share A for every nested row and field.
      yield->setOperands({merged->getResult(0), available});
      module->setAttr("function_type", TypeAttr::get(b.getFunctionType(
          {bits(8), bits(1), bits(1)}, {outer, bits(1)})));
      ASSERT_TRUE(succeeded(verify(*package)));
      auto before = printPackage();
      EXPECT_EQ(proves(), uniform) << diagnostics;
      EXPECT_EQ(before, printPackage());
    }
  }
}

TEST_F(HardwareValueAvailabilityTest,
       SymbolicUniformAggregateMergeSelectsFieldsBeforeSplat) {
  auto fixture = aggregateMergeFixture(false);
  auto extent = cast<ac::StaticExprAttr>(parseAttribute(
      "#ac.static_expr<{kind = \"reference\", ref = {kind = \"parameter\", "
      "owner = @Root, name = \"N\"}, origin = {site = {definition = @Root, "
      "ast_path = []}, expansion = []}, location = {path = "
      "\"availability.py\", "
      "line = 1 : i64, column = 1 : i64, end_line = 1 : i64, "
      "end_column = 2 : i64}}>",
      &context));
  module->setAttr(
      "parameters",
      b.getArrayAttr({b.getDictionaryAttr(
          {f("name", b.getStringAttr("N")),
           f("type", TypeAttr::get(parseType("!ac.math_int", &context)))})}));
  auto shape = b.getArrayAttr({extent});
  auto table =
      ac::TableType::get(&context, shape, fixture.merge.getNext().getType());
  b.setInsertionPoint(fixture.merge);
  for (unsigned operand : {0u, 2u}) {
    auto splat =
        op(ac::TableSplatOp::getOperationName(),
           {fixture.merge->getOperand(operand)}, {table}, {f("shape", shape)})
            ->getResult(0);
    fixture.merge->setOperand(operand, splat);
  }
  fixture.merge.getNext().setType(table);
  module->setAttr("function_type",
                  TypeAttr::get(b.getFunctionType({bits(8), bits(1), bits(1)},
                                                  {table, bits(1)})));
  ASSERT_TRUE(succeeded(verify(*package)));
  ASSERT_TRUE(proves()) << diagnostics;
  yield->setOperand(1, fixture.premature);
  auto before = printPackage();
  EXPECT_FALSE(proves()) << diagnostics;
  EXPECT_EQ(before, printPackage());
  yield->setOperand(1, fixture.whole);
  EXPECT_TRUE(proves()) << diagnostics;
}

TEST_F(HardwareValueAvailabilityTest,
       AggregateMergeRequiresGuardAndPreservesZeroGuardIdentity) {
  for (bool table : {false, true}) {
    for (bool identity : {false, true}) {
      for (bool forged : {false, true}) {
        SCOPED_TRACE(table);
        SCOPED_TRACE(identity);
        SCOPED_TRACE(forged);
        auto fixture = aggregateMergeFixture(table, !identity, identity);
        if (forged) {
          b.setInsertionPoint(yield);
          auto withoutGuard = identity ? one : conjunction(
              choose(selected, available, one), choose(selected, one, available));
          yield->setOperand(1, withoutGuard);
        }
        pruneUnusedCompanionAlternatives();
        ASSERT_TRUE(succeeded(verify(*package)));
        auto before = printPackage();
        EXPECT_EQ(proves(), !forged) << diagnostics;
        if (forged)
          EXPECT_NE(diagnostics.find("availability differs from actual payload SSA"),
                    std::string::npos) << diagnostics;
        EXPECT_EQ(before, printPackage());
      }
    }
  }
}

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
