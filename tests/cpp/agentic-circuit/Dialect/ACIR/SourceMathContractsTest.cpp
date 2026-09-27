#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/ACIRTypes.h"

#include "mlir/AsmParser/AsmParser.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <initializer_list>
#include <string>
#include <utility>

namespace acir::ac {
namespace {

using Field = std::pair<llvm::StringRef, mlir::Attribute>;

mlir::DictionaryAttr dictionary(mlir::OpBuilder &builder,
                                std::initializer_list<Field> fields) {
  llvm::SmallVector<mlir::NamedAttribute> attributes;
  for (const auto &[name, value] : fields)
    attributes.push_back(builder.getNamedAttr(name, value));
  return builder.getDictionaryAttr(attributes);
}

MathIntAttr math(mlir::MLIRContext &context, llvm::StringRef spelling) {
  bool negative = spelling.consume_front("-");
  unsigned width = static_cast<unsigned>(spelling.size() * 4 + 2);
  llvm::APInt bits(width, spelling, 10);
  if (negative)
    bits = -bits;
  return MathIntAttr::get(
      &context, llvm::APSInt(std::move(bits), /*isUnsigned=*/!negative));
}

mlir::IntegerAttr u64(mlir::OpBuilder &builder, uint64_t value) {
  return builder.getIntegerAttr(builder.getI64Type(), value);
}

mlir::DictionaryAttr site(mlir::OpBuilder &builder) {
  return dictionary(builder,
                    {{"definition", mlir::FlatSymbolRefAttr::get(
                                        builder.getContext(), "demo.Compute")},
                     {"ast_path", builder.getArrayAttr({})}});
}

mlir::DictionaryAttr occurrence(mlir::OpBuilder &builder) {
  return dictionary(builder, {{"site", site(builder)},
                              {"expansion", builder.getArrayAttr({})}});
}

mlir::DictionaryAttr sourceSpan(mlir::OpBuilder &builder) {
  return dictionary(builder, {{"path", builder.getStringAttr("demo.py")},
                              {"line", u64(builder, 7)},
                              {"column", u64(builder, 3)},
                              {"end_line", u64(builder, 7)},
                              {"end_column", u64(builder, 12)}});
}

mlir::DictionaryAttr sourceOwner(mlir::OpBuilder &builder) {
  return dictionary(builder, {{"package", builder.getStringAttr("demo")},
                              {"path", builder.getStringAttr("demo.py")}});
}

mlir::DictionaryAttr integerDomain(mlir::OpBuilder &builder, unsigned width) {
  llvm::SmallString<32> upper;
  llvm::APInt::getOneBitSet(width + 1, width)
      .toString(upper, 10, /*Signed=*/false);
  return dictionary(
      builder, {{"kind", builder.getStringAttr("integer")},
                {"storage", mlir::TypeAttr::get(builder.getIntegerType(width))},
                {"lower", math(*builder.getContext(), "0")},
                {"upper", math(*builder.getContext(), upper)},
                {"interpretation", builder.getStringAttr("unsigned")}});
}

mlir::DictionaryAttr signedI1Domain(mlir::OpBuilder &builder) {
  return dictionary(builder,
                    {{"kind", builder.getStringAttr("integer")},
                     {"storage", mlir::TypeAttr::get(builder.getI1Type())},
                     {"lower", math(*builder.getContext(), "-1")},
                     {"upper", math(*builder.getContext(), "1")},
                     {"interpretation", builder.getStringAttr("signed")}});
}

mlir::DictionaryAttr rangeCheck(mlir::OpBuilder &builder) {
  return dictionary(builder, {{"leaf", site(builder)},
                              {"kind", builder.getStringAttr("range")},
                              {"obligation", u64(builder, 0)},
                              {"location", sourceSpan(builder)}});
}

mlir::Value unrealized(mlir::OpBuilder &builder, mlir::Location location,
                       mlir::Type type) {
  mlir::OperationState state(location, "builtin.unrealized_conversion_cast");
  state.addTypes(type);
  return builder.create(state)->getResult(0);
}

struct MathGraph {
  mlir::OwningOpRef<mlir::ModuleOp> module;
  ModuleOp hardware;
  RuleOp rule;
  MathConstantOp lhs;
  MathConstantOp rhs;
  MathFromBitsOp fromBits;
  MathBinaryOp add;
  MathBinaryOp andBits;
  MathToBitsOp toBits;
};

struct HelperGraph {
  mlir::OwningOpRef<mlir::ModuleOp> module;
  mlir::func::FuncOp helper;
  MathFromBitsOp fromBits;
};

MathGraph buildMathGraph(mlir::MLIRContext &context, unsigned width = 8,
                         bool signedI1 = false) {
  mlir::OpBuilder builder(&context);
  auto location = mlir::FileLineColLoc::get(&context, "demo.py", 7, 3);
  auto module = mlir::ModuleOp::create(location);
  module->setAttr("ac.stage", builder.getStringAttr("source"));
  module->setAttr("ac.unit_kind", builder.getStringAttr("implementation"));
  module->setAttr("ac.source_owner", sourceOwner(builder));
  builder.setInsertionPointToStart(module.getBody());

  auto domain =
      signedI1 ? signedI1Domain(builder) : integerDomain(builder, width);
  auto formal =
      dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                           {"parameter", builder.getStringAttr("input")},
                           {"ordinal", builder.getUnitAttr()}});
  auto port =
      dictionary(builder, {{"parameter", builder.getStringAttr("input")},
                           {"ordinal", builder.getUnitAttr()},
                           {"role", builder.getStringAttr("current")},
                           {"type", domain},
                           {"origin", occurrence(builder)},
                           {"location", sourceSpan(builder)}});
  mlir::OperationState moduleState(location, ModuleOp::getOperationName());
  moduleState.addAttribute("name", builder.getStringAttr("Compute"));
  moduleState.addAttribute("sym_name", builder.getStringAttr("demo.Compute"));
  moduleState.addAttribute("ac.source_owner", sourceOwner(builder));
  moduleState.addAttribute("ac.origin", occurrence(builder));
  moduleState.addAttribute("ac.ports", builder.getArrayAttr({port}));
  moduleState.addRegion();
  auto hardware = mlir::cast<ModuleOp>(builder.create(moduleState));
  auto *moduleBody = new mlir::Block();
  hardware.getBody().push_back(moduleBody);
  auto physical = builder.getIntegerType(width);
  moduleBody->addArgument(DffeType::get(&context, physical), location);

  builder.setInsertionPointToEnd(moduleBody);
  mlir::OperationState ruleState(location, RuleOp::getOperationName());
  ruleState.addOperands(moduleBody->getArgument(0));
  ruleState.addAttribute("name", builder.getStringAttr("compute"));
  ruleState.addAttribute("registration", occurrence(builder));
  ruleState.addAttribute("operandSegmentSizes",
                         builder.getDenseI32ArrayAttr({1, 0}));
  ruleState.addAttribute("ac.source_owner", sourceOwner(builder));
  ruleState.addAttribute("ac.origin", occurrence(builder));
  ruleState.addAttribute("ac.input_bindings", builder.getArrayAttr({formal}));
  ruleState.addAttribute("ac.output_bindings", builder.getArrayAttr({}));
  ruleState.addAttribute("ac.input_types", builder.getArrayAttr({domain}));
  ruleState.addAttribute("ac.output_types", builder.getArrayAttr({}));
  for (unsigned index = 0; index < 4; ++index)
    ruleState.addRegion();
  auto rule = mlir::cast<RuleOp>(builder.create(ruleState));
  auto *ruleBody = new mlir::Block();
  rule.getBody().push_back(ruleBody);
  ruleBody->addArgument(physical, location);
  builder.setInsertionPointToEnd(ruleBody);

  auto makeConstant = [&](llvm::StringRef value) {
    mlir::OperationState state(location, MathConstantOp::getOperationName());
    state.addAttribute("value", math(context, value));
    state.addAttribute("ac.origin", occurrence(builder));
    state.addTypes(MathIntType::get(&context));
    return mlir::cast<MathConstantOp>(builder.create(state));
  };
  MathConstantOp lhs = makeConstant("255");
  MathConstantOp rhs = makeConstant("1");

  mlir::OperationState fromState(location, MathFromBitsOp::getOperationName());
  fromState.addOperands(ruleBody->getArgument(0));
  fromState.addAttribute("domain", domain);
  fromState.addAttribute("ac.origin", occurrence(builder));
  fromState.addTypes(MathIntType::get(&context));
  auto fromBits = mlir::cast<MathFromBitsOp>(builder.create(fromState));

  mlir::Value path = unrealized(builder, location, builder.getI1Type());
  mlir::Value valid = unrealized(builder, location, builder.getI1Type());
  auto makeBinary = [&](llvm::StringRef operation, mlir::Value left,
                        mlir::Value right) {
    mlir::OperationState state(location, MathBinaryOp::getOperationName());
    state.addOperands({path, left, valid, right, valid});
    state.addAttribute("operator", builder.getStringAttr(operation));
    state.addAttribute("ac.origin", occurrence(builder));
    state.addTypes({MathIntType::get(&context), builder.getI1Type()});
    return mlir::cast<MathBinaryOp>(builder.create(state));
  };
  MathBinaryOp add = makeBinary("add", lhs.getResult(), rhs.getResult());
  MathBinaryOp andBits =
      makeBinary("and_bits", add.getResult(), fromBits.getResult());

  mlir::OperationState toState(location, MathToBitsOp::getOperationName());
  toState.addOperands({path, add.getResult(), add.getValid()});
  toState.addAttribute("domain", domain);
  toState.addAttribute("ac.origin", occurrence(builder));
  toState.addAttribute("ac.check_template", rangeCheck(builder));
  toState.addTypes({physical, builder.getI1Type()});
  auto toBits = mlir::cast<MathToBitsOp>(builder.create(toState));
  mlir::OperationState ruleYield(location, YieldOp::getOperationName());
  builder.create(ruleYield);
  builder.setInsertionPointToEnd(moduleBody);
  mlir::OperationState moduleYield(location, YieldOp::getOperationName());
  builder.create(moduleYield);
  return {std::move(module), hardware, rule,    lhs,   rhs,
          fromBits,          add,      andBits, toBits};
}

HelperGraph buildHelperGraph(mlir::MLIRContext &context,
                             bool recordConstructor = false) {
  mlir::OpBuilder builder(&context);
  auto location = mlir::FileLineColLoc::get(&context, "demo.py", 11, 3);
  auto module = mlir::ModuleOp::create(location);
  module->setAttr("ac.stage", builder.getStringAttr("source"));
  module->setAttr("ac.unit_kind", builder.getStringAttr("interface"));
  module->setAttr("ac.source_owner", sourceOwner(builder));
  builder.setInsertionPointToStart(module.getBody());

  auto domain = integerDomain(builder, 8);
  auto recordType =
      StructType::get(&context, builder.getStringAttr("demo.Record"));
  mlir::Type dataResult = recordConstructor ? mlir::Type(recordType)
                                            : mlir::Type(builder.getI8Type());
  auto helper = mlir::func::FuncOp::create(
      location,
      recordConstructor ? "demo.Record.__init__" : "demo.value_helper",
      builder.getFunctionType({builder.getI8Type(), builder.getI1Type()},
                              {dataResult, builder.getI1Type()}));
  helper->setAttr("ac.source_owner", sourceOwner(builder));
  helper->setAttr("ac.origin", occurrence(builder));
  helper->setAttr("ac.declaration_role", builder.getStringAttr("definition"));
  helper->setAttr("ac.helper_kind",
                  builder.getStringAttr(recordConstructor ? "record_constructor"
                                                          : "value"));
  auto parameter = dictionary(
      builder,
      {{"name", builder.getStringAttr("value")},
       {"binding", builder.getStringAttr("positional_or_keyword")},
       {"constraint",
        dictionary(builder, {{"kind", builder.getStringAttr("logical")},
                             {"type", domain}})},
       {"default",
        dictionary(builder, {{"present", builder.getBoolAttr(false)}})},
       {"origin", occurrence(builder)},
       {"location", sourceSpan(builder)}});
  helper->setAttr("ac.parameters", builder.getArrayAttr({parameter}));
  helper->setAttr("ac.return_form", builder.getStringAttr("single"));
  mlir::DictionaryAttr resultLogical =
      recordConstructor
          ? dictionary(builder, {{"kind", builder.getStringAttr("record")},
                                 {"symbol", mlir::FlatSymbolRefAttr::get(
                                                &context, "demo.Record")}})
          : domain;
  helper->setAttr("ac.result_constraints",
                  builder.getArrayAttr({dictionary(
                      builder, {{"kind", builder.getStringAttr("logical")},
                                {"type", resultLogical}})}));
  helper->setAttr("ac.check_templates", builder.getArrayAttr({}));
  if (recordConstructor)
    helper->setAttr("ac.record",
                    mlir::FlatSymbolRefAttr::get(&context, "demo.Record"));
  module.getBody()->push_back(helper);

  mlir::Block *body = helper.addEntryBlock();
  builder.setInsertionPointToEnd(body);
  mlir::OperationState fromState(location, MathFromBitsOp::getOperationName());
  fromState.addOperands(body->getArgument(0));
  fromState.addAttribute("domain", domain);
  fromState.addAttribute("ac.origin", occurrence(builder));
  fromState.addTypes(MathIntType::get(&context));
  auto fromBits = mlir::cast<MathFromBitsOp>(builder.create(fromState));
  mlir::Value returned = recordConstructor
                             ? unrealized(builder, location, recordType)
                             : body->getArgument(0);
  mlir::func::ReturnOp::create(
      builder, location, mlir::ValueRange{returned, body->getArgument(1)});
  return {std::move(module), helper, fromBits};
}

struct Verification {
  bool passed;
  std::string diagnostic;
};

Verification verify(mlir::MLIRContext &context, mlir::Operation *operation) {
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream(diagnostic) << value;
    return mlir::success();
  });
  return {mlir::succeeded(mlir::verify(operation)), diagnostic};
}

void expectRejected(const Verification &result, llvm::StringRef fragment) {
  EXPECT_FALSE(result.passed);
  EXPECT_NE(result.diagnostic.find(fragment.str()), std::string::npos)
      << result.diagnostic;
}

class SourceMathContractsTest : public ::testing::Test {
protected:
  SourceMathContractsTest() {
    context.loadDialect<ACIRDialect, mlir::func::FuncDialect>();
  }

  mlir::MLIRContext context;
};

TEST_F(SourceMathContractsTest, ParsesPrintsAndVerifiesClosedFoundation) {
  MathGraph graph = buildMathGraph(context);
  ASSERT_TRUE(verify(context, *graph.module).passed);

  std::string printed;
  llvm::raw_string_ostream(printed) << *graph.module;
  EXPECT_NE(printed.find("ac.math.constant"), std::string::npos);
  EXPECT_NE(printed.find("ac.math.from_bits"), std::string::npos);
  EXPECT_NE(printed.find("\"ac.math.binary\""), std::string::npos);
  EXPECT_NE(printed.find("operator = \"add\""), std::string::npos);
  EXPECT_NE(printed.find("operator = \"and_bits\""), std::string::npos);
  EXPECT_NE(printed.find("ac.math.to_bits"), std::string::npos);

  auto reparsed = mlir::parseSourceString<mlir::ModuleOp>(printed, &context);
  ASSERT_TRUE(reparsed) << printed;
  EXPECT_TRUE(verify(context, *reparsed).passed);
}

TEST_F(SourceMathContractsTest,
       KeepsMathematical255PlusOneAheadOfExplicitBoundaryConversion) {
  MathGraph graph = buildMathGraph(context);
  ASSERT_TRUE(verify(context, *graph.module).passed);
  EXPECT_EQ(graph.lhs.getValue().getCanonicalValue(), "255");
  EXPECT_EQ(graph.rhs.getValue().getCanonicalValue(), "1");
  EXPECT_TRUE(mlir::isa<MathIntType>(graph.add.getResult().getType()));
  EXPECT_EQ(graph.toBits.getValue(), graph.add.getResult());
  EXPECT_TRUE(graph.toBits.getResult().getType().isInteger(8));
}

TEST_F(SourceMathContractsTest,
       AcceptsLinkedLocallyAndRejectsFinalMissingOriginAndUnknownLocation) {
  {
    MathGraph graph = buildMathGraph(context);
    (*graph.module)
        ->setAttr("ac.stage", mlir::StringAttr::get(&context, "linked"));
    EXPECT_TRUE(verify(context, graph.lhs).passed);
    EXPECT_TRUE(verify(context, graph.fromBits).passed);
    EXPECT_TRUE(verify(context, graph.add).passed);
    EXPECT_TRUE(verify(context, graph.toBits).passed);
    EXPECT_FALSE(verify(context, *graph.module).passed)
        << "local math legality is not whole linked-program acceptance";
  }
  {
    MathGraph graph = buildMathGraph(context);
    (*graph.module)
        ->setAttr("ac.stage", mlir::StringAttr::get(&context, "final"));
    expectRejected(verify(context, graph.lhs),
                   "source or linked semantic phase");
  }
  {
    MathGraph graph = buildMathGraph(context);
    graph.add->removeAttr("ac.origin");
    expectRejected(verify(context, *graph.module), "Occurrence");
  }
  {
    MathGraph graph = buildMathGraph(context);
    graph.lhs->setLoc(mlir::UnknownLoc::get(&context));
    expectRejected(verify(context, *graph.module), "source location");
  }
}

TEST_F(SourceMathContractsTest,
       RequiresRuleComputationOrValidatedSourceOwnedHelperContainment) {
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    builder.setInsertionPoint(graph.hardware);
    auto location = mlir::FileLineColLoc::get(&context, "demo.py", 9, 1);
    mlir::OperationState state(location, MathConstantOp::getOperationName());
    state.addAttribute("value", math(context, "3"));
    state.addAttribute("ac.origin", occurrence(builder));
    state.addTypes(MathIntType::get(&context));
    auto topLevel = mlir::cast<MathConstantOp>(builder.create(state));
    expectRejected(verify(context, topLevel), "requires an ac.rule");
  }
  {
    MathGraph graph = buildMathGraph(context);
    auto *readiness = new mlir::Block();
    graph.rule.getReadiness().push_back(readiness);
    mlir::OpBuilder builder = mlir::OpBuilder::atBlockEnd(readiness);
    auto location = mlir::FileLineColLoc::get(&context, "demo.py", 9, 1);
    mlir::OperationState state(location, MathConstantOp::getOperationName());
    state.addAttribute("value", math(context, "3"));
    state.addAttribute("ac.origin", occurrence(builder));
    state.addTypes(MathIntType::get(&context));
    auto misplaced = mlir::cast<MathConstantOp>(builder.create(state));
    expectRejected(verify(context, misplaced), "computation body");
  }
}

TEST_F(SourceMathContractsTest,
       AcceptsValueAndRecordConstructorHelperCalculationBodies) {
  {
    HelperGraph graph = buildHelperGraph(context);
    EXPECT_TRUE(verify(context, *graph.module).passed);
    EXPECT_TRUE(verify(context, graph.fromBits).passed);
  }
  {
    HelperGraph graph = buildHelperGraph(context, /*recordConstructor=*/true);
    EXPECT_TRUE(verify(context, *graph.module).passed);
    EXPECT_TRUE(verify(context, graph.fromBits).passed);
  }
}

TEST_F(SourceMathContractsTest,
       HelperFromBitsRequiresAuthoritativeOwnerParameterAndSsa) {
  {
    HelperGraph graph = buildHelperGraph(context);
    mlir::OpBuilder builder(&context);
    graph.helper->setAttr("ac.source_owner", dictionary(builder, {}));
    expectRejected(verify(context, graph.fromBits), "SourceOwner");
  }
  {
    HelperGraph graph = buildHelperGraph(context);
    mlir::OpBuilder builder(&context);
    auto parameters =
        graph.helper->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
    auto parameter = mlir::cast<mlir::DictionaryAttr>(parameters[0]);
    llvm::SmallVector<mlir::NamedAttribute> fields(parameter.begin(),
                                                   parameter.end());
    auto signedDomain = dictionary(
        builder, {{"kind", builder.getStringAttr("integer")},
                  {"storage", mlir::TypeAttr::get(builder.getI8Type())},
                  {"lower", math(context, "-128")},
                  {"upper", math(context, "128")},
                  {"interpretation", builder.getStringAttr("signed")}});
    for (auto &field : fields)
      if (field.getName() == "constraint")
        field = builder.getNamedAttr(
            "constraint",
            dictionary(builder, {{"kind", builder.getStringAttr("logical")},
                                 {"type", signedDomain}}));
    graph.helper->setAttr(
        "ac.parameters",
        builder.getArrayAttr({builder.getDictionaryAttr(fields)}));
    expectRejected(verify(context, graph.fromBits),
                   "exact logical integer domain");
  }
  {
    HelperGraph graph = buildHelperGraph(context);
    mlir::OpBuilder builder(&context);
    builder.setInsertionPoint(graph.fromBits);
    auto forged =
        unrealized(builder, graph.fromBits.getLoc(), builder.getI8Type());
    graph.fromBits.getValueMutable().assign(forged);
    expectRejected(verify(context, graph.fromBits),
                   "actual helper data parameter");
  }
}

TEST_F(SourceMathContractsTest,
       ImportSnapshotOwnerMustComeFromDeclaredInterfaceClosure) {
  HelperGraph graph = buildHelperGraph(context);
  mlir::OpBuilder builder(&context);
  auto dependency =
      dictionary(builder, {{"package", builder.getStringAttr("dependency")},
                           {"path", builder.getStringAttr("demo.py")}});
  graph.helper->setAttr("ac.declaration_role",
                        builder.getStringAttr("import_snapshot"));
  graph.helper->setAttr("ac.source_owner", dependency);
  (*graph.module)
      ->setAttr("ac.interfaces",
                builder.getArrayAttr({sourceOwner(builder), dependency}));
  EXPECT_TRUE(verify(context, graph.fromBits).passed);
  (*graph.module)
      ->setAttr("ac.interfaces", builder.getArrayAttr({sourceOwner(builder)}));
  expectRejected(verify(context, graph.fromBits),
                 "helper owner contract is incomplete");
}

TEST_F(SourceMathContractsTest,
       PreservesSignedI1IntegerMeaningAndRejectsBoolDomainSubstitution) {
  MathGraph graph = buildMathGraph(context, 1, /*signedI1=*/true);
  ASSERT_TRUE(verify(context, *graph.module).passed);
  EXPECT_EQ(graph.fromBits.getDomain()
                .getAs<mlir::StringAttr>("interpretation")
                .getValue(),
            "signed");
  mlir::OpBuilder builder(&context);
  graph.fromBits->setAttr(
      "domain",
      dictionary(builder,
                 {{"kind", builder.getStringAttr("bool")},
                  {"storage", mlir::TypeAttr::get(builder.getI1Type())}}));
  expectRejected(verify(context, *graph.module),
                 "domain must be LogicalType.Integer");
}

TEST_F(SourceMathContractsTest,
       AcceptsRealOwnedDffeAndRejectsRedirectedOwnedHandle) {
  MathGraph graph = buildMathGraph(context);
  mlir::OpBuilder builder(&context);
  builder.setInsertionPoint(graph.rule);
  mlir::OperationState state(graph.rule.getLoc(), DffeOp::getOperationName());
  state.addAttribute("name", builder.getStringAttr("owned"));
  state.addAttribute("ac.source_owner", sourceOwner(builder));
  state.addAttribute("ac.declaration", occurrence(builder));
  state.addAttribute("ac.logical_element", graph.fromBits.getDomain());
  state.addAttribute("ac.shape", builder.getArrayAttr({}));
  state.addAttribute(
      "ac.initial_value",
      dictionary(builder, {{"kind", builder.getStringAttr("scalar")}}));
  state.addAttribute("ac.domain", builder.getStringAttr("default"));
  state.addTypes(DffeType::get(&context, builder.getI8Type()));
  auto owned = mlir::cast<DffeOp>(builder.create(state));
  graph.rule.getInputsMutable().assign(owned.getState());
  auto binding = dictionary(builder, {{"kind", builder.getStringAttr("owned")},
                                      {"declaration", occurrence(builder)},
                                      {"element", builder.getArrayAttr({})}});
  graph.rule->setAttr("ac.input_bindings", builder.getArrayAttr({binding}));
  EXPECT_TRUE(verify(context, graph.fromBits).passed);

  builder.setInsertionPoint(graph.rule);
  mlir::Value redirected =
      unrealized(builder, graph.rule.getLoc(), owned.getState().getType());
  graph.rule.getInputsMutable().assign(redirected);
  expectRejected(verify(context, graph.fromBits), "real scalar DFFE");
}

TEST_F(SourceMathContractsTest,
       RejectsForgedRuleDomainsPortsAndUnknownSsaProducers) {
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    graph.rule->setAttr("ac.input_types",
                        builder.getArrayAttr({integerDomain(builder, 7)}));
    expectRejected(verify(context, graph.fromBits),
                   "equal its rule current declaration");
  }
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    auto binding =
        dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                             {"parameter", builder.getStringAttr("forged")},
                             {"ordinal", builder.getUnitAttr()}});
    graph.rule->setAttr("ac.input_bindings", builder.getArrayAttr({binding}));
    expectRejected(verify(context, graph.fromBits), "PortSlot domain disagree");
  }
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    builder.setInsertionPoint(graph.fromBits);
    auto forged =
        unrealized(builder, graph.fromBits.getLoc(), builder.getI8Type());
    graph.fromBits.getValueMutable().assign(forged);
    expectRejected(verify(context, graph.fromBits),
                   "actual ac.rule current argument");
  }
}

TEST_F(SourceMathContractsTest,
       RejectsUnsupportedOperatorsAndChecksOnSafeFoundationOperators) {
  for (llvm::StringRef operation :
       {"sub", "mul", "floordiv", "mod", "or_bits", "xor_bits", "shl", "shr"}) {
    MathGraph graph = buildMathGraph(context);
    graph.add->setAttr("operator", mlir::StringAttr::get(&context, operation));
    expectRejected(verify(context, *graph.module), "not implemented");
  }
  {
    MathGraph graph = buildMathGraph(context);
    graph.add->removeAttr("operator");
    expectRejected(verify(context, *graph.module),
                   "requires StringAttr 'operator'");
  }
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    graph.add->setAttr("ac.check_template", rangeCheck(builder));
    expectRejected(verify(context, *graph.module),
                   "must not carry a check_template");
  }
}

TEST_F(SourceMathContractsTest,
       RejectsDomainTypeDriftAndMalformedRangeCheckTemplates) {
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    graph.fromBits->setAttr("domain", integerDomain(builder, 7));
    expectRejected(verify(context, *graph.module),
                   "exactly match bounded bits");
  }
  {
    MathGraph graph = buildMathGraph(context);
    graph.toBits->removeAttr("ac.check_template");
    expectRejected(verify(context, *graph.module),
                   "check_template must contain exactly four fields");
  }
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    graph.toBits->setAttr(
        "ac.check_template",
        dictionary(builder, {{"leaf", site(builder)},
                             {"kind", builder.getStringAttr("shift")},
                             {"obligation", u64(builder, 0)},
                             {"location", sourceSpan(builder)}}));
    expectRejected(verify(context, *graph.module),
                   "does not match the source obligation");
  }
}

TEST_F(SourceMathContractsTest, RejectsNonI1ValidityAndNonMathOperands) {
  {
    MathGraph graph = buildMathGraph(context);
    graph.add.getLhsValidMutable().assign(graph.fromBits.getValue());
    expectRejected(verify(context, *graph.module), "operand #2");
  }
  {
    MathGraph graph = buildMathGraph(context);
    graph.toBits.getValueMutable().assign(graph.fromBits.getValue());
    expectRejected(verify(context, *graph.module), "operand #1");
  }
}

} // namespace
} // namespace acir::ac
