#include "Compiler/HardwareEmitCommon.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "Dialect/ACIR/HardwareSourceChecks.h"
#include "mlir/AsmParser/AsmParser.h"
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
  ac::StaticExprAttr parameterRef(StringRef ownerName, StringRef name) {
    std::string text =
        "#ac.static_expr<{kind = \"reference\", ref = {kind = \"parameter\", "
        "owner = @" +
        ownerName.str() + ", name = \"" + name.str() +
        "\"}, origin = {site = {definition = @" + ownerName.str() +
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
         f("output_names", b.getArrayAttr({b.getStringAttr("out")})),
         f("function_type",
           TypeAttr::get(b.getFunctionType({}, {bit(name)})))});
    state.addRegion();
    auto result = cast<ac::ModuleOp>(b.create(state));
    result.getBody().push_back(new Block);
    return result;
  }
  ac::ModuleOp parameterizedModule(StringRef name, StringRef path) {
    auto result = module(name, path);
    result->setAttr(
        "parameters",
        b.getArrayAttr(
            {d({f("name", b.getStringAttr("WIDTH")),
                f("type", TypeAttr::get(ac::MathIntType::get(&context)))})}));
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
  ac::SourceObserveOp multiLog(ac::ModuleOp physical, StringRef name,
                               DictionaryAttr id, unsigned valueCount) {
    Value path = constant(physical, true);
    SmallVector<Value> operands{path};
    SmallVector<Attribute> items;
    for (unsigned index = 0; index < valueCount; ++index) {
      operands.push_back(constant(physical, index & 1));
      items.push_back(d({f("kind", b.getStringAttr("value")),
                         f("ordinal", b.getI32IntegerAttr(index))}));
    }
    auto spec = d({f("level", b.getStringAttr("info")),
                   f("event", b.getStringAttr(name)),
                   f("items", b.getArrayAttr(items))});
    return cast<ac::SourceObserveOp>(
        op(ac::SourceObserveOp::getOperationName(), operands, {},
           {f("kind", b.getStringAttr("log")), f("spec", spec),
            f("ac.location", span("root.py")), f("ac.observation_id", id)}));
  }
  ac::InstanceOp instance(ac::ModuleOp parent, ac::ModuleOp child,
                          uint64_t index, StringRef name,
                          ArrayAttr parameters) {
    b.setInsertionPointToEnd(&parent.getBody().front());
    return cast<ac::InstanceOp>(op(
        ac::InstanceOp::getOperationName(), {}, {bit(child.getSymName())},
        {f("callee", FlatSymbolRefAttr::get(&context, child.getSymName())),
         f("instance_name", b.getStringAttr(name)), f("parameters", parameters),
         f("type_arguments", b.getArrayAttr({})),
         f("occurrence", occurrence(parent.getSymName(), index))}));
  }
  ac::InstanceOp instance(ac::ModuleOp parent, ac::ModuleOp child,
                          uint64_t index, StringRef name) {
    return instance(parent, child, index, name, b.getArrayAttr({}));
  }
  ac::CollectionOp collection(ac::ModuleOp parent, ac::ModuleOp child,
                              uint64_t index, ac::StaticExprAttr extent,
                              StringRef name) {
    b.setInsertionPointToEnd(&parent.getBody().front());
    auto shape = b.getArrayAttr({extent});
    auto result = ac::TableType::get(&context, shape, bit(child.getSymName()));
    return cast<ac::CollectionOp>(
        op(ac::CollectionOp::getOperationName(), {}, {result},
           {f("callee", FlatSymbolRefAttr::get(&context, child.getSymName())),
            f("instance_name", b.getStringAttr(name)),
            f("parameters", b.getArrayAttr({})),
            f("type_arguments", b.getArrayAttr({})), f("shape", shape),
            f("occurrence", occurrence(parent.getSymName(), index))}));
  }
  ac::CollectionOp collection(ac::ModuleOp parent, ac::ModuleOp child,
                              uint64_t index, uint64_t extent, StringRef name) {
    return collection(parent, child, index,
                      literal(parent.getSymName(), extent), name);
  }
  void finish(ac::ModuleOp definition) {
    b.setInsertionPointToEnd(&definition.getBody().front());
    op(ac::YieldOp::getOperationName(), {constant(definition, false)});
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
  void downgradeToOldObservationID(ac::SourceObserveOp observe) {
    auto closed = observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
    observe->setAttr("ac.observation_id",
                     d({f("registration", closed.get("registration")),
                        f("site", closed.get("site")),
                        f("ordinal", closed.get("ordinal"))}));
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
                b.getIntegerAttr(
                    IntegerType::get(&context, 64, IntegerType::Signed), 0)),
              f("invocation", valid.get("invocation")),
              f("owner", valid.get("owner"))}),
           d({f("registration", valid.get("registration")),
              f("site", valid.get("site")), f("ordinal", valid.get("ordinal")),
              f("invocation", b.getArrayAttr({})),
              f("owner", owner({occurrence("Root", 7)}))}),
           d({f("registration", valid.get("registration")),
              f("site", valid.get("site")), f("ordinal", valid.get("ordinal")),
              f("invocation", b.getArrayAttr({frame(
                                  "Root", 8, owner({occurrence("Root", 7)}))})),
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
    EXPECT_TRUE(failed(plan())) << kind.str() << diagnostics;
    EXPECT_NE(diagnostics.find("span must match logical owner"),
              std::string::npos)
        << kind.str() << diagnostics;
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
        << kind.str() << diagnostics;
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
       SuppliedOrdinalOrdersWholeMultiValueGroupsWithoutInterleaving) {
  beginPackage();
  auto root = module("Root", "root.py");
  auto later = multiLog(root, "later", identity("Root", 1, "Root", 2, 9), 2);
  auto earlier =
      multiLog(root, "earlier", identity("Root", 1, "Root", 3, 3), 1);
  finish(root);
  system(root);

  ac::HardwareAnalysis analysis(*package);
  acir::compiler::HardwareEmitContext emitter(*package, analysis);
  ASSERT_TRUE(succeeded(emitter.prepareNativeChecks())) << diagnostics;
  auto emission = acir::compiler::observationEmissionPlan(emitter);
  ASSERT_TRUE(succeeded(emission)) << diagnostics;
  ASSERT_EQ(emission->occurrences.size(), 3u);
  EXPECT_EQ(emission->occurrences[0].site.op, earlier.getOperation());
  EXPECT_EQ(emission->occurrences[0].site.valueIndex, 0u);
  EXPECT_EQ(emission->occurrences[1].site.op, later.getOperation());
  EXPECT_EQ(emission->occurrences[1].site.valueIndex, 0u);
  EXPECT_EQ(emission->occurrences[2].site.op, later.getOperation());
  EXPECT_EQ(emission->occurrences[2].site.valueIndex, 1u);

  auto metadata = acir::compiler::observationSiteMetadataJson(earlier);
  ASSERT_TRUE(succeeded(metadata));
  EXPECT_NE(metadata->find("\"invocation\":[]"), std::string::npos);
  EXPECT_NE(metadata->find("\"ordinal\":3"), std::string::npos);
  EXPECT_NE(metadata->find("\"site\":"), std::string::npos);
}

TEST_F(ObservationIdentityTest,
       RepeatedSameSiteInvocationsKeepEachMultiValueGroupContiguous) {
  beginPackage();
  auto root = module("Root", "root.py");
  auto self = owner();
  auto first = multiLog(
      root, "repeated",
      identity("Root", 1, "Root", 2, 5, {frame("Root", 10, self)}, self), 2);
  auto second = multiLog(
      root, "repeated",
      identity("Root", 1, "Root", 2, 5, {frame("Root", 11, self)}, self), 2);
  finish(root);
  system(root);

  ac::HardwareAnalysis analysis(*package);
  acir::compiler::HardwareEmitContext emitter(*package, analysis);
  ASSERT_TRUE(succeeded(emitter.prepareNativeChecks())) << diagnostics;
  auto emission = acir::compiler::observationEmissionPlan(emitter);
  ASSERT_TRUE(succeeded(emission)) << diagnostics;
  ASSERT_EQ(emission->occurrences.size(), 4u);
  EXPECT_EQ(emission->occurrences[0].site.op, first.getOperation());
  EXPECT_EQ(emission->occurrences[0].site.valueIndex, 0u);
  EXPECT_EQ(emission->occurrences[1].site.op, first.getOperation());
  EXPECT_EQ(emission->occurrences[1].site.valueIndex, 1u);
  EXPECT_EQ(emission->occurrences[2].site.op, second.getOperation());
  EXPECT_EQ(emission->occurrences[2].site.valueIndex, 0u);
  EXPECT_EQ(emission->occurrences[3].site.op, second.getOperation());
  EXPECT_EQ(emission->occurrences[3].site.valueIndex, 1u);
  EXPECT_EQ(emission->occurrences[0].site.siteKey,
            emission->occurrences[1].site.siteKey);
  EXPECT_EQ(emission->occurrences[2].site.siteKey,
            emission->occurrences[3].site.siteKey);
  EXPECT_LT(emission->occurrences[1].site.siteKey,
            emission->occurrences[2].site.siteKey);

  auto firstMetadata = acir::compiler::observationSiteMetadataJson(first);
  auto secondMetadata = acir::compiler::observationSiteMetadataJson(second);
  ASSERT_TRUE(succeeded(firstMetadata));
  ASSERT_TRUE(succeeded(secondMetadata));
  EXPECT_NE(*firstMetadata, *secondMetadata);
  EXPECT_NE(firstMetadata->find("\"invocation\":[{"), std::string::npos);
  EXPECT_NE(secondMetadata->find("\"invocation\":[{"), std::string::npos);
  EXPECT_NE(firstMetadata->find("\"ordinal\":5"), std::string::npos);
  EXPECT_NE(secondMetadata->find("\"ordinal\":5"), std::string::npos);
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

TEST_F(ObservationIdentityTest,
       LogicalOwnerResolutionKeepsFullBindingsIsolated) {
  beginPackage();
  auto child = parameterizedModule("Child", "child.py");
  finish(child);
  auto root = module("Root", "root.py");
  auto left =
      instance(root, child, 4, "left", b.getArrayAttr({literal("Root", 3)}));
  auto right =
      instance(root, child, 5, "right", b.getArrayAttr({literal("Root", 5)}));
  auto leftOwner = owner({left.getOccurrence()});
  auto rightOwner = owner({right.getOccurrence()});
  auto leftObserve = observe(root, "log", "left",
                             identity("Root", 1, "Child", 8, 0,
                                      {frame("Root", 6, leftOwner)}, leftOwner),
                             "child.py");
  auto rightObserve =
      observe(root, "log", "right",
              identity("Root", 1, "Child", 9, 1, {frame("Root", 7, rightOwner)},
                       rightOwner),
              "child.py");
  finish(root);
  system(root);
  auto checked = plan();
  ASSERT_TRUE(succeeded(checked)) << diagnostics;

  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings physical;
  physical.owner = root;
  auto noCharge = [](uint64_t, Operation *) { return success(); };
  auto resolvedLeft = ac::detail::resolveObservationOwner(analysis, leftObserve,
                                                          physical, noCharge);
  auto resolvedRight = ac::detail::resolveObservationOwner(
      analysis, rightObserve, physical, noCharge);
  ASSERT_TRUE(succeeded(resolvedLeft)) << diagnostics;
  ASSERT_TRUE(succeeded(resolvedRight)) << diagnostics;
  EXPECT_NE(resolvedLeft->bindings.integers.lookup("WIDTH"),
            resolvedRight->bindings.integers.lookup("WIDTH"));
  ASSERT_EQ(resolvedLeft->path.size(), 1u);
  ASSERT_EQ(resolvedRight->path.size(), 1u);
  EXPECT_EQ(resolvedLeft->path.front(), left);
  EXPECT_EQ(resolvedRight->path.front(), right);
}

TEST_F(ObservationIdentityTest,
       CompletedObservationProofCacheKeepsFullBindingsInBothOrders) {
  beginPackage();
  auto child = parameterizedModule("Child", "child.py");
  b.setInsertionPointToEnd(&child.getBody().front());
  auto parameterPath =
      op(ac::BitsConstantOp::getOperationName(), {},
         {ac::BitsType::get(&context, parameterRef("Child", "WIDTH"))},
         {f("value", literal("Child", 1))})
          ->getResult(0);
  auto payload = constant(child, true);
  op(ac::SourceObserveOp::getOperationName(), {parameterPath, payload}, {},
     {f("kind", b.getStringAttr("report")),
      f("spec", d({f("name", b.getStringAttr("width"))})),
      f("ac.location", span("child.py")),
      f("ac.observation_id", identity("Child", 1, "Child", 2, 0))});
  finish(child);
  ac::HardwareBindings widthOne, widthTwo;
  widthOne.owner = child;
  widthTwo.owner = child;
  widthOne.integers["WIDTH"] =
      cast<ac::MathIntAttr>(parseAttribute("#ac.math_int<1>", &context));
  widthTwo.integers["WIDTH"] =
      cast<ac::MathIntAttr>(parseAttribute("#ac.math_int<2>", &context));
  ac::HardwareAnalysis analysis(*package);
  auto noCharge = [](uint64_t, Operation *) { return success(); };
  auto verifyScopes =
      [&](ArrayRef<std::pair<ac::ModuleOp, ac::HardwareBindings>> scopes) {
        return capture([&] {
          return ac::detail::verifySourceCheckDependencies(analysis, scopes,
                                                           noCharge);
        });
      };
  const std::pair<ac::ModuleOp, ac::HardwareBindings> valid{child, widthOne};
  const std::pair<ac::ModuleOp, ac::HardwareBindings> invalid{child, widthTwo};
  SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>> validOnly{valid};
  EXPECT_TRUE(succeeded(verifyScopes(validOnly))) << diagnostics;
  std::string before;
  llvm::raw_string_ostream(before) << *package;
  for (auto scopes :
       {SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>>{valid,
                                                                   invalid},
        SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>>{invalid,
                                                                   valid}}) {
    EXPECT_TRUE(failed(verifyScopes(scopes))) << diagnostics;
    EXPECT_NE(diagnostics.find("source check operand must have width one"),
              std::string::npos)
        << diagnostics;
    EXPECT_EQ(diagnostics.find("analysis work budget"), std::string::npos)
        << diagnostics;
    std::string current;
    llvm::raw_string_ostream(current) << *package;
    EXPECT_EQ(current, before);
  }
}

TEST_F(ObservationIdentityTest,
       PhysicalPayloadPathAndPublishedLogicalOwnerRemainDistinct) {
  beginPackage();
  auto child = module("Child", "child.py");
  finish(child);
  auto root = module("Root", "root.py");
  auto allocation = instance(root, child, 4, "child");
  auto childOwner = owner({allocation.getOccurrence()});
  auto relocated = observe(root, "log", "relocated",
                           identity("Root", 1, "Child", 9, 7,
                                    {frame("Root", 8, childOwner)}, childOwner),
                           "child.py");
  finish(root);
  system(root);

  ASSERT_EQ(relocated.getPath().getDefiningOp()->getParentOp(),
            root.getOperation());
  ac::HardwareAnalysis analysis(*package);
  acir::compiler::HardwareEmitContext emitter(*package, analysis);
  ASSERT_TRUE(succeeded(emitter.prepareNativeChecks())) << diagnostics;
  auto emission = acir::compiler::observationEmissionPlan(emitter);
  ASSERT_TRUE(succeeded(emission)) << diagnostics;
  ASSERT_EQ(emission->occurrences.size(), 1u);
  EXPECT_TRUE(emission->occurrences.front().path.empty());
  ASSERT_EQ(emission->occurrences.front().logicalOwnerPath.size(), 1u);
  EXPECT_EQ(emission->occurrences.front().logicalOwnerPath.front().first,
            allocation.getOperation());

  auto metadata = acir::compiler::observationSiteMetadataJson(relocated);
  ASSERT_TRUE(succeeded(metadata));
  EXPECT_NE(metadata->find("\"ordinal\":7"), std::string::npos);
  EXPECT_NE(metadata->find("\"invocation\":[{"), std::string::npos);
  auto native = acir::compiler::cppObservationRunnerMetadata(emitter);
  ASSERT_TRUE(succeeded(native));
  EXPECT_NE(native->find("root/child"), std::string::npos);
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
  auto leafOwner =
      owner({middleAllocation.getOccurrence(), leafAllocation.getOccurrence()});
  auto enterMiddle = frame("Root", 8, middleOwner);
  auto enterLeaf = frame("Middle", 9, leafOwner);
  observe(
      root, "report", "shared",
      identity("Root", 1, "Leaf", 10, 0, {enterMiddle, enterLeaf}, leafOwner),
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
  EXPECT_TRUE(succeeded(plan())) << diagnostics;
}

TEST_F(ObservationIdentityTest,
       ReusedDefinitionWithDifferentCollectionGeometryRejectsEmissionPlan) {
  beginPackage();
  auto leaf = module("Leaf", "leaf.py");
  observe(leaf, "log", "lane", identity("Leaf", 1, "Leaf", 2, 0), "leaf.py");
  finish(leaf);
  auto middle = parameterizedModule("Middle", "middle.py");
  collection(middle, leaf, 3, parameterRef("Middle", "WIDTH"), "lanes");
  finish(middle);
  auto root = module("Root", "root.py");
  instance(root, middle, 4, "narrow", b.getArrayAttr({literal("Root", 2)}));
  instance(root, middle, 5, "wide", b.getArrayAttr({literal("Root", 3)}));
  finish(root);
  system(root);

  ASSERT_TRUE(succeeded(plan())) << diagnostics;
  ac::HardwareAnalysis analysis(*package);
  acir::compiler::HardwareEmitContext emitter(*package, analysis);
  ASSERT_TRUE(succeeded(emitter.prepareNativeChecks())) << diagnostics;
  EXPECT_TRUE(failed(acir::compiler::observationEmissionPlan(emitter)));
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
  EXPECT_NE(
      diagnostics.find("observation owner requires unique ordinary instances"),
      std::string::npos)
      << diagnostics;
}

TEST_F(ObservationIdentityTest,
       InvocationCallAndReceiverMustFollowActualHierarchy) {
  for (bool wrongCall : {true, false}) {
    beginPackage();
    auto child = module("Child", "child.py");
    finish(child);
    auto root = module("Root", "root.py");
    auto allocation = instance(root, child, 4, "child");
    auto receiver = owner(
        {wrongCall ? allocation.getOccurrence() : occurrence("Root", 99)});
    auto call = frame(wrongCall ? "Child" : "Root", 8, receiver);
    observe(root, "log", "invalid",
            identity("Root", 1, "Child", 9, 0, {call}, receiver), "child.py");
    finish(root);
    system(root);
    EXPECT_TRUE(failed(plan())) << diagnostics;
    EXPECT_NE(diagnostics.find(wrongCall ? "invocation call"
                                         : "ordinary definitions"),
              std::string::npos)
        << diagnostics;
  }
}

TEST_F(ObservationIdentityTest, TinyBudgetFailureDoesNotMutateIR) {
  beginPackage();
  auto root = module("Root", "root.py");
  ac::SourceObserveOp last;
  for (unsigned index = 0; index < 16; ++index)
    last = observe(root, "log", (Twine("event_") + Twine(index)).str(),
                   identity("Root", 1, "Root", 2 + index, index), "root.py");
  downgradeToOldObservationID(last);
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

TEST_F(ObservationIdentityTest,
       DefaultBudgetPrepaysPairwiseIdentityValidationBeforeLateBadID) {
  beginPackage();
  auto root = module("Root", "root.py");
  ac::SourceObserveOp last;
  for (unsigned index = 0; index < 2048; ++index)
    last = observe(root, "log", (Twine("event_") + Twine(index)).str(),
                   identity("Root", 1, "Root", 2 + index, index), "root.py");
  downgradeToOldObservationID(last);
  finish(root);
  system(root);
  std::string before;
  llvm::raw_string_ostream(before) << *package;
  EXPECT_TRUE(failed(plan())) << diagnostics;
  EXPECT_NE(diagnostics.find("analysis work budget"), std::string::npos)
      << diagnostics;
  EXPECT_NE(diagnostics.find("phase raw preflight"), std::string::npos)
      << diagnostics;
  EXPECT_NE(diagnostics.find("'ac.module' op"), std::string::npos)
      << diagnostics;
  EXPECT_EQ(diagnostics.find("ObservationID must contain exactly"),
            std::string::npos)
      << diagnostics;
  std::string after;
  llvm::raw_string_ostream(after) << *package;
  EXPECT_EQ(after, before);
}

TEST_F(ObservationIdentityTest,
       DefaultBudgetPrepaysLargeMetadataBeforeLateBadID) {
  beginPackage();
  auto root = module("Root", "root.py");
  std::string event(16 * 1024 * 1024, 'x');
  auto last =
      observe(root, "log", event, identity("Root", 1, "Root", 2, 0), "root.py");
  downgradeToOldObservationID(last);
  finish(root);
  system(root);
  std::string before;
  llvm::raw_string_ostream(before) << *package;
  EXPECT_TRUE(failed(plan())) << diagnostics;
  EXPECT_NE(diagnostics.find("analysis work budget"), std::string::npos)
      << diagnostics;
  EXPECT_NE(diagnostics.find("phase raw preflight"), std::string::npos)
      << diagnostics;
  EXPECT_NE(diagnostics.find("'ac.module' op"), std::string::npos)
      << diagnostics;
  EXPECT_EQ(diagnostics.find("ObservationID must contain exactly"),
            std::string::npos)
      << diagnostics;
  std::string after;
  llvm::raw_string_ostream(after) << *package;
  EXPECT_EQ(after, before);
}

} // namespace
