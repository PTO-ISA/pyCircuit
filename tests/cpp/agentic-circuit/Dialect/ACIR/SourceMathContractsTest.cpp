#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/ACIRTypes.h"

#include "mlir/AsmParser/AsmParser.h"
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
  MathConstantOp lhs;
  MathConstantOp rhs;
  MathFromBitsOp fromBits;
  MathBinaryOp add;
  MathBinaryOp andBits;
  MathToBitsOp toBits;
};

MathGraph buildMathGraph(mlir::MLIRContext &context) {
  mlir::OpBuilder builder(&context);
  auto location = mlir::FileLineColLoc::get(&context, "demo.py", 7, 3);
  auto module = mlir::ModuleOp::create(location);
  module->setAttr("ac.stage", builder.getStringAttr("source"));
  module->setAttr("ac.unit_kind", builder.getStringAttr("implementation"));
  builder.setInsertionPointToStart(module.getBody());

  auto makeConstant = [&](llvm::StringRef value) {
    mlir::OperationState state(location, MathConstantOp::getOperationName());
    state.addAttribute("value", math(context, value));
    state.addAttribute("ac.origin", occurrence(builder));
    state.addTypes(MathIntType::get(&context));
    return mlir::cast<MathConstantOp>(builder.create(state));
  };
  MathConstantOp lhs = makeConstant("255");
  MathConstantOp rhs = makeConstant("1");

  mlir::Value bounded = unrealized(builder, location, builder.getI8Type());
  mlir::OperationState fromState(location, MathFromBitsOp::getOperationName());
  fromState.addOperands(bounded);
  fromState.addAttribute("domain", integerDomain(builder, 8));
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
  toState.addAttribute("domain", integerDomain(builder, 8));
  toState.addAttribute("ac.origin", occurrence(builder));
  toState.addAttribute("ac.check_template", rangeCheck(builder));
  toState.addTypes({builder.getI8Type(), builder.getI1Type()});
  auto toBits = mlir::cast<MathToBitsOp>(builder.create(toState));
  return {std::move(module), lhs, rhs, fromBits, add, andBits, toBits};
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
  SourceMathContractsTest() { context.loadDialect<ACIRDialect>(); }

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
       RejectsWrongStageMissingOriginAndUnknownLocation) {
  {
    MathGraph graph = buildMathGraph(context);
    (*graph.module)
        ->setAttr("ac.stage", mlir::StringAttr::get(&context, "linked"));
    expectRejected(verify(context, *graph.module),
                   "source-stage implementation unit");
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
