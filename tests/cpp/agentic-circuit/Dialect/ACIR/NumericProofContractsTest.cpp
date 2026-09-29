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
  moduleState.addAttribute(
      "ac.control_ports",
      dictionary(builder, {{"clock", builder.getI32IntegerAttr(0)},
                           {"reset", builder.getI32IntegerAttr(1)}}));
  moduleState.addRegion();
  auto hardware = mlir::cast<ModuleOp>(builder.create(moduleState));
  auto *moduleBody = new mlir::Block();
  hardware.getBody().push_back(moduleBody);
  auto physical = builder.getIntegerType(width);
  moduleBody->addArgument(builder.getI1Type(), location);
  moduleBody->addArgument(builder.getI1Type(), location);
  moduleBody->addArgument(RegType::get(&context, physical), location);

  builder.setInsertionPointToEnd(moduleBody);
  mlir::OperationState ruleState(location, RuleOp::getOperationName());
  ruleState.addOperands(moduleBody->getArgument(2));
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
      makeBinary("and_bits", fromBits.getResult(), fromBits.getResult());

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

MathCompareOp buildCompare(MathGraph &graph, mlir::OpBuilder &builder,
                           llvm::StringRef predicate) {
  builder.setInsertionPoint(graph.rule.getBody().front().getTerminator());
  mlir::OperationState state(graph.add.getLoc(),
                             MathCompareOp::getOperationName());
  state.addOperands({graph.add.getPath(), graph.fromBits.getResult(),
                     graph.add.getLhsValid(), graph.fromBits.getResult(),
                     graph.add.getRhsValid()});
  state.addAttribute("predicate", builder.getStringAttr(predicate));
  state.addAttribute("ac.origin", occurrence(builder));
  state.addTypes({builder.getI1Type(), builder.getI1Type()});
  return mlir::cast<MathCompareOp>(builder.create(state));
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

class NumericProofContractsTest : public ::testing::Test {
protected:
  NumericProofContractsTest() {
    context.loadDialect<ACIRDialect, mlir::func::FuncDialect>();
  }

  mlir::MLIRContext context;
};

TEST_F(NumericProofContractsTest,
       AdmitsSubtractionInClosedMathBinaryFoundation) {
  MathGraph graph = buildMathGraph(context);
  graph.add->setAttr("operator", mlir::StringAttr::get(&context, "sub"));
  EXPECT_TRUE(verify(context, *graph.module).passed)
      << "the approved source-math foundation includes exact subtraction";
}

TEST_F(NumericProofContractsTest,
       RequiresCompareOperationForApprovedPredicates) {
  EXPECT_TRUE(mlir::OperationName("ac.math.compare", &context).isRegistered())
      << "approved C2 compare operation must be registered";
}

TEST_F(NumericProofContractsTest,
       MathCompareAcceptsAllClosedPredicatesAndProducesExactBool) {
  for (llvm::StringRef predicate : {"eq", "ne", "lt", "le", "gt", "ge"}) {
    SCOPED_TRACE(predicate.str());
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    auto compare = buildCompare(graph, builder, predicate);
    ASSERT_TRUE(compare.getResult().getType().isInteger(1));
    EXPECT_TRUE(verify(context, compare).passed)
        << "accepted predicate must pass the exact compare verifier";
  }
  {
    MathGraph graph = buildMathGraph(context, 1, /*signedI1=*/true);
    mlir::OpBuilder builder(&context);
    auto compare = buildCompare(graph, builder, "lt");
    ASSERT_TRUE(compare.getResult().getType().isInteger(1));
    EXPECT_TRUE(verify(context, compare).passed)
        << "signed i1 compares math sign extension and still yields bool i1";
  }
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    auto compare = buildCompare(graph, builder, "eq");
    compare->setAttr("ac.check_template", rangeCheck(builder));
    expectRejected(verify(context, compare), "carry no check_template");
  }
}

TEST_F(NumericProofContractsTest,
       MathCompareRejectsPredicateDomainAndResultTypeDrift) {
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    auto compare = buildCompare(graph, builder, "approx");
    expectRejected(verify(context, compare), "predicate must be one of");
  }
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    auto compare = buildCompare(graph, builder, "lt");
    compare->setAttr("domain", integerDomain(builder, 7));
    expectRejected(verify(context, compare), "no finite storage domain");
  }
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    auto compare = buildCompare(graph, builder, "eq");
    compare.getResult().setType(builder.getI8Type());
    expectRejected(verify(context, compare), "bool i1");
  }
}

TEST_F(NumericProofContractsTest,
       MathBinaryRejectsMismatchedBitDomainAndMathematicalDomainAttribute) {
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    graph.andBits->setAttr("domain", integerDomain(builder, 7));
    expectRejected(verify(context, graph.andBits), "no bit domain");
  }
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    graph.add->setAttr("domain", integerDomain(builder, 8));
    expectRejected(verify(context, graph.add),
                   "mathematical results have no bit domain");
  }
}

TEST_F(NumericProofContractsTest,
       MathFromBitsAcceptsOnlyRuleInputSourceReadProvenance) {
  MathGraph graph = buildMathGraph(context);
  mlir::OpBuilder builder(&context);
  builder.setInsertionPoint(graph.fromBits);
  mlir::OperationState readState(graph.fromBits.getLoc(),
                                 SourceReadOp::getOperationName());
  readState.addOperands(graph.rule.getBody().front().getArgument(0));
  readState.addTypes(builder.getI8Type());
  readState.addAttribute("ac.origin", occurrence(builder));
  auto read = mlir::cast<SourceReadOp>(builder.create(readState));
  graph.fromBits.getValueMutable().assign(read.getResult());

  EXPECT_TRUE(verify(context, graph.fromBits).passed)
      << "the real rule input may flow through its authoritative SourceRead";
}

TEST_F(NumericProofContractsTest,
       MathFromBitsRejectsSourceReadFromAnotherNestedBlock) {
  MathGraph graph = buildMathGraph(context);
  context.allowUnregisteredDialects();
  mlir::OpBuilder builder(&context);
  builder.setInsertionPoint(graph.fromBits);
  mlir::OperationState readState(graph.fromBits.getLoc(),
                                 SourceReadOp::getOperationName());
  readState.addOperands(graph.rule.getBody().front().getArgument(0));
  readState.addTypes(builder.getI8Type());
  readState.addAttribute("ac.origin", occurrence(builder));
  auto read = mlir::cast<SourceReadOp>(builder.create(readState));

  mlir::OperationState containerState(graph.fromBits.getLoc(),
                                      "test.numeric_container");
  containerState.addRegion();
  mlir::Operation *container = builder.create(containerState);
  auto *nested = new mlir::Block();
  container->getRegion(0).push_back(nested);
  builder.setInsertionPointToEnd(nested);
  mlir::OperationState fromState(graph.fromBits.getLoc(),
                                 MathFromBitsOp::getOperationName());
  fromState.addOperands(read.getResult());
  fromState.addAttribute("domain", graph.fromBits.getDomain());
  fromState.addAttribute("ac.origin", occurrence(builder));
  fromState.addTypes(MathIntType::get(&context));
  auto nestedFromBits = mlir::cast<MathFromBitsOp>(builder.create(fromState));
  expectRejected(verify(context, nestedFromBits), "same rule");
}

TEST_F(NumericProofContractsTest,
       MathFromBitsRejectsForgedUseAndSourceReadFromAnotherOwner) {
  {
    MathGraph graph = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    builder.setInsertionPoint(graph.fromBits);
    auto forged =
        unrealized(builder, graph.fromBits.getLoc(), builder.getI8Type());
    graph.fromBits.getValueMutable().assign(forged);
    EXPECT_FALSE(verify(context, graph.fromBits).passed)
        << "an arbitrary i8 SSA producer cannot impersonate a rule read";
  }
  {
    MathGraph graph = buildMathGraph(context);
    MathGraph foreignOwner = buildMathGraph(context);
    mlir::OpBuilder builder(&context);
    builder.setInsertionPoint(foreignOwner.fromBits);
    mlir::OperationState readState(foreignOwner.fromBits.getLoc(),
                                   SourceReadOp::getOperationName());
    readState.addOperands(foreignOwner.rule.getBody().front().getArgument(0));
    readState.addTypes(builder.getI8Type());
    readState.addAttribute("ac.origin", occurrence(builder));
    auto foreignRead = mlir::cast<SourceReadOp>(builder.create(readState));
    mlir::Value original = graph.fromBits.getValue();
    graph.fromBits.getValueMutable().assign(foreignRead.getResult());
    EXPECT_FALSE(verify(context, graph.fromBits).passed)
        << "a read owned by another rule/source module cannot satisfy this use";
    graph.fromBits.getValueMutable().assign(original);
  }
}

TEST_F(NumericProofContractsTest,
       KeepsMathFromBitsDomainAndBoundedWidthExactAfterSourceRead) {
  MathGraph graph = buildMathGraph(context);
  mlir::OpBuilder builder(&context);
  builder.setInsertionPoint(graph.fromBits);
  mlir::OperationState readState(graph.fromBits.getLoc(),
                                 SourceReadOp::getOperationName());
  readState.addOperands(graph.rule.getBody().front().getArgument(0));
  readState.addTypes(builder.getI8Type());
  readState.addAttribute("ac.origin", occurrence(builder));
  auto read = mlir::cast<SourceReadOp>(builder.create(readState));
  graph.fromBits.getValueMutable().assign(read.getResult());
  graph.fromBits->setAttr("domain", integerDomain(builder, 7));
  EXPECT_FALSE(verify(context, graph.fromBits).passed)
      << "SourceRead provenance does not weaken exact domain/width matching";
}

} // namespace
} // namespace acir::ac
