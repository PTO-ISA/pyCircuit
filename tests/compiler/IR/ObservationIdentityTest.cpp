#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "Dialect/ACIR/HardwareSourceChecks.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <functional>
#include <string>

namespace {
namespace ac = acir::ac;
using namespace mlir;

class ObservationIdentityTest : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder b{&context};
  OwningOpRef<mlir::ModuleOp> package;
  std::string diagnostics;

  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  Location loc(StringRef path = "root.py") {
    return FileLineColLoc::get(&context, path, 1, 1);
  }
  NamedAttribute f(StringRef name, Attribute value) {
    return b.getNamedAttr(name, value);
  }
  DictionaryAttr d(std::initializer_list<NamedAttribute> fields) {
    return b.getDictionaryAttr(fields);
  }
  DictionaryAttr occurrence(StringRef owner, uint64_t index) {
    return d(
        {f("site",
           d({f("definition", FlatSymbolRefAttr::get(&context, owner)),
              f("ast_path", b.getArrayAttr({d(
                                {f("kind", b.getStringAttr("index")),
                                 f("value", b.getI64IntegerAttr(index))})}))})),
         f("expansion", b.getArrayAttr({}))});
  }
  DictionaryAttr span(StringRef path) {
    return d({f("path", b.getStringAttr(path)),
              f("line", b.getI64IntegerAttr(1)),
              f("column", b.getI64IntegerAttr(1)),
              f("end_line", b.getI64IntegerAttr(1)),
              f("end_column", b.getI64IntegerAttr(2))});
  }
  DictionaryAttr owner(ArrayRef<DictionaryAttr> path = {}) {
    SmallVector<Attribute> values(path.begin(), path.end());
    return d({f("instance_path", b.getArrayAttr(values))});
  }
  DictionaryAttr frame(StringRef caller, uint64_t site,
                       DictionaryAttr receiver) {
    return d({f("call", occurrence(caller, site)), f("receiver", receiver)});
  }
  DictionaryAttr identity(StringRef registrationOwner, uint64_t registration,
                          StringRef siteOwner, uint64_t site, int64_t ordinal,
                          ArrayRef<DictionaryAttr> invocation = {},
                          DictionaryAttr logicalOwner = {}) {
    SmallVector<Attribute> frames(invocation.begin(), invocation.end());
    if (!logicalOwner)
      logicalOwner = owner();
    return d({f("registration", occurrence(registrationOwner, registration)),
              f("site", occurrence(siteOwner, site)),
              f("ordinal", b.getI64IntegerAttr(ordinal)),
              f("invocation", b.getArrayAttr(frames)),
              f("owner", logicalOwner)});
  }
  ac::StaticExprAttr literal(StringRef ownerName, uint64_t value) {
    std::string text =
        "#ac.static_expr<{kind = \"literal\", value = {kind = \"integer\", "
        "value = #ac.math_int<" +
        std::to_string(value) + ">}, origin = {site = {definition = @" +
        ownerName.str() +
        ", ast_path = []}, expansion = []}, location = {path = \"root.py\", "
        "line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 "
        ": i64}}>";
    return cast<ac::StaticExprAttr>(parseAttribute(text, &context));
  }
  Type bit(StringRef ownerName = "Root") {
    return ac::BitsType::get(&context, literal(ownerName, 1));
  }
  Operation *op(StringRef name, ValueRange inputs = {}, TypeRange results = {},
                ArrayRef<NamedAttribute> attrs = {}) {
    OperationState state(loc(), name);
    state.addOperands(inputs);
    state.addTypes(results);
    state.addAttributes(attrs);
    return b.create(state);
  }
  ac::ModuleOp module(StringRef name, StringRef path) {
    b.setInsertionPointToEnd(package->getBody());
    OperationState state(loc(path), ac::ModuleOp::getOperationName());
    state.addAttributes(
        {f("sym_name", b.getStringAttr(name)),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr(path))})),
         f("parameters", b.getArrayAttr({})),
         f("type_parameters", b.getArrayAttr({})),
         f("input_names", b.getArrayAttr({})),
         f("output_names", b.getArrayAttr({})),
         f("function_type", TypeAttr::get(b.getFunctionType({}, {})))});
    state.addRegion();
    auto result = cast<ac::ModuleOp>(b.create(state));
    result.getBody().push_back(new Block);
    return result;
  }
  Value constant(ac::ModuleOp definition, bool value) {
    b.setInsertionPointToEnd(&definition.getBody().front());
    return op(ac::BitsConstantOp::getOperationName(), {},
              {bit(definition.getSymName())},
              {f("value", literal(definition.getSymName(), value))})
        ->getResult(0);
  }
  ac::SourceObserveOp observe(ac::ModuleOp physical, StringRef kind,
                              StringRef name, DictionaryAttr id,
                              StringRef sourcePath, bool pathValue = true) {
    Value path = constant(physical, pathValue);
    SmallVector<NamedAttribute> spec;
    if (kind == "print")
      spec = {f("items",
                b.getArrayAttr({d({f("kind", b.getStringAttr("value")),
                                   f("ordinal", b.getI32IntegerAttr(0))})})),
              f("sep", b.getStringAttr(" ")), f("end", b.getStringAttr("\n"))};
    else if (kind == "log")
      spec = {f("level", b.getStringAttr("info")),
              f("event", b.getStringAttr(name)),
              f("items",
                b.getArrayAttr({d({f("kind", b.getStringAttr("value")),
                                   f("ordinal", b.getI32IntegerAttr(0))})}))};
    else
      spec = {f("name", b.getStringAttr(name))};
    return cast<ac::SourceObserveOp>(op(
        ac::SourceObserveOp::getOperationName(), {path, path}, {},
        {f("kind", b.getStringAttr(kind)), f("spec", b.getDictionaryAttr(spec)),
         f("ac.location", span(sourcePath)), f("ac.observation_id", id)}));
  }
  ac::InstanceOp instance(ac::ModuleOp parent, ac::ModuleOp child,
                          uint64_t index, StringRef name) {
    b.setInsertionPointToEnd(&parent.getBody().front());
    return cast<ac::InstanceOp>(
        op(ac::InstanceOp::getOperationName(), {}, {},
           {f("callee", FlatSymbolRefAttr::get(&context, child.getSymName())),
            f("instance_name", b.getStringAttr(name)),
            f("parameters", b.getArrayAttr({})),
            f("type_arguments", b.getArrayAttr({})),
            f("occurrence", occurrence(parent.getSymName(), index))}));
  }
  ac::CollectionOp collection(ac::ModuleOp parent, ac::ModuleOp child,
                              uint64_t index, uint64_t extent, StringRef name) {
    b.setInsertionPointToEnd(&parent.getBody().front());
    auto shape = b.getArrayAttr({literal(parent.getSymName(), extent)});
    return cast<ac::CollectionOp>(
        op(ac::CollectionOp::getOperationName(), {}, {},
           {f("callee", FlatSymbolRefAttr::get(&context, child.getSymName())),
            f("instance_name", b.getStringAttr(name)),
            f("parameters", b.getArrayAttr({})),
            f("type_arguments", b.getArrayAttr({})), f("shape", shape),
            f("occurrence", occurrence(parent.getSymName(), index))}));
  }
  void finish(ac::ModuleOp definition) {
    b.setInsertionPointToEnd(&definition.getBody().front());
    op(ac::YieldOp::getOperationName());
  }
  void system(ac::ModuleOp root) {
    b.setInsertionPointToEnd(package->getBody());
    op(ac::SystemOp::getOperationName(), {}, {},
       {f("entry",
          d({f("callee", FlatSymbolRefAttr::get(&context, root.getSymName())),
             f("parameters", b.getArrayAttr({})),
             f("type_arguments", b.getArrayAttr({}))})),
        f("domain", b.getStringAttr("default"))});
  }
  void beginPackage() { package = mlir::ModuleOp::create(loc()); }
  template <typename Function> LogicalResult capture(Function function) {
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &error) {
      llvm::raw_string_ostream stream(diagnostics);
      error.print(stream);
      return success();
    });
    return function();
  }
  FailureOr<ac::HardwareSourceCheckPlan>
  plan(ac::HardwareSourceCheckLimits limits = {}) {
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &error) {
      llvm::raw_string_ostream stream(diagnostics);
      error.print(stream);
      return success();
    });
    return ac::HardwareAnalysis(*package).getSourceCheckPlan(limits);
  }
};

TEST_F(ObservationIdentityTest, ClosedIdentitySchemaAcceptsOnlyFiveFields) {
  auto valid = identity("Root", 1, "Root", 2, 3);
  EXPECT_TRUE(succeeded(capture([&] {
    return ac::detail::verifyObservationID(valid,
                                           [&] { return emitError(loc()); });
  }))) << diagnostics;

  for (DictionaryAttr invalid : {
           d({f("registration", valid.get("registration")),
              f("site", valid.get("site")),
              f("ordinal", valid.get("ordinal"))}),
           d({f("registration", valid.get("registration")),
              f("site", valid.get("site")), f("ordinal", valid.get("ordinal")),
              f("invocation", valid.get("invocation"))}),
           d({f("registration", valid.get("registration")),
              f("site", valid.get("site")), f("ordinal", valid.get("ordinal")),
              f("invocation", valid.get("invocation")),
              f("owner", valid.get("owner")), f("extra", b.getUnitAttr())}),
           identity("Root", 1, "Root", 2, -1),
           d({f("registration", valid.get("registration")),
              f("site", valid.get("site")),
              f("ordinal",
                b.getIntegerAttr(IntegerType::get(&context, 64,
                                                   IntegerType::Signed),
                                 0)),
              f("invocation", valid.get("invocation")),
              f("owner", valid.get("owner"))}),
           d({f("registration", valid.get("registration")),
              f("site", valid.get("site")), f("ordinal", valid.get("ordinal")),
              f("invocation", b.getArrayAttr({})),
              f("owner", owner({occurrence("Root", 7)}))}),
           d({f("registration", valid.get("registration")),
              f("site", valid.get("site")), f("ordinal", valid.get("ordinal")),
              f("invocation",
                b.getArrayAttr({frame("Root", 8,
                                      owner({occurrence("Root", 7)}))})),
              f("owner", owner({occurrence("Root", 9)}))}),
       }) {
    EXPECT_TRUE(failed(capture([&] {
      return ac::detail::verifyObservationID(invalid,
                                             [&] { return emitError(loc()); });
    }))) << diagnostics;
  }
}

TEST_F(ObservationIdentityTest, AllKindsUseClosedStructureAndProvenance) {
  beginPackage();
  auto root = module("Root", "root.py");
  for (auto [ordinal, kind] :
       llvm::enumerate(ArrayRef<StringRef>{"print", "log", "report"}))
    observe(root, kind, (Twine("item_") + Twine(ordinal)).str(),
            identity("Root", 1, "Root", ordinal + 2, ordinal), "root.py");
  finish(root);
  EXPECT_TRUE(succeeded(capture([&] { return mlir::verify(*package); })))
      << diagnostics;

  auto first = *root.getBody().front().getOps<ac::SourceObserveOp>().begin();
  first->setAttr("ac.location", d({f("path", b.getStringAttr("root.py"))}));
  EXPECT_TRUE(failed(capture([&] {
    return ac::detail::verifySourceObservationStructure(first);
  }))) << diagnostics;
}

TEST_F(ObservationIdentityTest, AllKindsRejectForeignLogicalOwnerSpan) {
  for (StringRef kind : {"print", "log", "report"}) {
    beginPackage();
    auto root = module("Root", "root.py");
    observe(root, kind, "item", identity("Root", 1, "Root", 2, 0),
            "foreign.py");
    finish(root);
    system(root);
    EXPECT_TRUE(failed(plan())) << kind << diagnostics;
    EXPECT_NE(diagnostics.find("span must match logical owner"),
              std::string::npos)
        << kind << diagnostics;
  }
}

TEST_F(ObservationIdentityTest, DuplicateFullIdentityRejectsEveryKind) {
  for (StringRef kind : {"print", "log", "report"}) {
    beginPackage();
    auto root = module("Root", "root.py");
    auto id = identity("Root", 1, "Root", 2, 0);
    observe(root, kind, "first", id, "root.py");
    observe(root, kind, kind == "report" ? "second" : "first", id, "root.py");
    finish(root);
    EXPECT_TRUE(failed(capture([&] { return mlir::verify(*package); })))
        << kind << diagnostics;
    EXPECT_NE(diagnostics.find("ObservationID"), std::string::npos)
        << diagnostics;
  }
}

TEST_F(ObservationIdentityTest,
       SameOriginalSiteWithDifferentOrdinalsRemainsDistinct) {
  beginPackage();
  auto root = module("Root", "root.py");
  observe(root, "log", "first", identity("Root", 1, "Root", 2, 9), "root.py");
  observe(root, "log", "second", identity("Root", 1, "Root", 2, 17), "root.py");
  finish(root);
  EXPECT_TRUE(succeeded(capture([&] { return mlir::verify(*package); })))
      << diagnostics;
}

TEST_F(ObservationIdentityTest,
       ParentRelocatedReportCollidesWithChildOrdinaryReport) {
  beginPackage();
  auto child = module("Child", "child.py");
  observe(child, "report", "shared", identity("Child", 1, "Child", 2, 0),
          "child.py");
  finish(child);
  auto root = module("Root", "root.py");
  auto allocation = instance(root, child, 4, "child");
  auto childOwner = owner({allocation.getOccurrence()});
  auto call = frame("Root", 8, childOwner);
  observe(root, "report", "shared",
          identity("Root", 1, "Child", 9, 0, {call}, childOwner), "child.py");
  finish(root);
  system(root);
  EXPECT_TRUE(failed(plan())) << diagnostics;
  EXPECT_NE(diagnostics.find("report"), std::string::npos) << diagnostics;
}

TEST_F(ObservationIdentityTest, SameReportNameInDistinctInstancesIsLegal) {
  beginPackage();
  auto child = module("Child", "child.py");
  observe(child, "report", "shared", identity("Child", 1, "Child", 2, 0),
          "child.py");
  finish(child);
  auto root = module("Root", "root.py");
  instance(root, child, 4, "left");
  instance(root, child, 5, "right");
  finish(root);
  system(root);
  EXPECT_TRUE(succeeded(plan())) << diagnostics;
}

TEST_F(ObservationIdentityTest, MultilevelRelocationCollisionRejects) {
  beginPackage();
  auto leaf = module("Leaf", "leaf.py");
  observe(leaf, "report", "shared", identity("Leaf", 1, "Leaf", 2, 0),
          "leaf.py");
  finish(leaf);
  auto middle = module("Middle", "middle.py");
  auto leafAllocation = instance(middle, leaf, 3, "leaf");
  finish(middle);
  auto root = module("Root", "root.py");
  auto middleAllocation = instance(root, middle, 4, "middle");
  auto middleOwner = owner({middleAllocation.getOccurrence()});
  auto leafOwner = owner(
      {middleAllocation.getOccurrence(), leafAllocation.getOccurrence()});
  auto enterMiddle = frame("Root", 8, middleOwner);
  auto enterLeaf = frame("Middle", 9, leafOwner);
  observe(root, "report", "shared",
          identity("Root", 1, "Leaf", 10, 0, {enterMiddle, enterLeaf},
                   leafOwner),
          "leaf.py");
  finish(root);
  system(root);
  EXPECT_TRUE(failed(plan())) << diagnostics;
  EXPECT_NE(diagnostics.find("report"), std::string::npos) << diagnostics;
}

TEST_F(ObservationIdentityTest, FalsePathDuplicateReportStillRejects) {
  beginPackage();
  auto root = module("Root", "root.py");
  observe(root, "report", "shared", identity("Root", 1, "Root", 2, 0),
          "root.py", false);
  observe(root, "report", "shared", identity("Root", 1, "Root", 3, 1),
          "root.py", false);
  finish(root);
  EXPECT_TRUE(failed(capture([&] { return mlir::verify(*package); })))
      << diagnostics;
}

TEST_F(ObservationIdentityTest,
       NestedCollectionsProveIndependentLanesWithoutExtentExpansion) {
  beginPackage();
  auto leaf = module("Leaf", "leaf.py");
  observe(leaf, "report", "shared", identity("Leaf", 1, "Leaf", 2, 0),
          "leaf.py");
  finish(leaf);
  auto middle = module("Middle", "middle.py");
  collection(middle, leaf, 3, 65521, "leaves");
  finish(middle);
  auto root = module("Root", "root.py");
  collection(root, middle, 4, 65519, "middles");
  finish(root);
  system(root);
  EXPECT_TRUE(succeeded(plan({200000}))) << diagnostics;
}

TEST_F(ObservationIdentityTest, OwnerPathCannotCrossCollection) {
  beginPackage();
  auto child = module("Child", "child.py");
  finish(child);
  auto root = module("Root", "root.py");
  auto lanes = collection(root, child, 4, 2, "children");
  auto invalidOwner = owner({lanes.getOccurrence()});
  auto call = frame("Root", 8, invalidOwner);
  observe(root, "report", "shared",
          identity("Root", 1, "Child", 9, 0, {call}, invalidOwner), "child.py");
  finish(root);
  system(root);
  EXPECT_TRUE(failed(plan())) << diagnostics;
  EXPECT_NE(diagnostics.find("Collection"), std::string::npos) << diagnostics;
}

TEST_F(ObservationIdentityTest, TinyBudgetFailureDoesNotMutateIR) {
  beginPackage();
  auto root = module("Root", "root.py");
  for (unsigned index = 0; index < 16; ++index)
    observe(root, "log", (Twine("event_") + Twine(index)).str(),
            identity("Root", 1, "Root", 2 + index, index), "root.py");
  finish(root);
  system(root);
  std::string before;
  llvm::raw_string_ostream(before) << *package;
  EXPECT_TRUE(failed(plan({32}))) << diagnostics;
  std::string after;
  llvm::raw_string_ostream(after) << *package;
  EXPECT_EQ(after, before);
  EXPECT_NE(diagnostics.find("analysis work budget"), std::string::npos)
      << diagnostics;
}

} // namespace
