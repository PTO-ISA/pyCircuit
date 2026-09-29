#include "NumericFiniteSSAOracle.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <utility>
#include <vector>
namespace acir::ac {
namespace {
llvm::APInt integerBits(llvm::StringRef text, unsigned width = 256) {
  bool negative = text.consume_front("-");
  llvm::APInt value(width, text, 10);
  return negative ? -value : value;
}
std::string decimal(llvm::APInt value) {
  llvm::APSInt signedValue(std::move(value), false);
  llvm::SmallString<80> text;
  signedValue.toString(text);
  return text.str().str();
}
std::string addDecimal(llvm::StringRef value, llvm::StringRef amount) {
  return decimal(integerBits(value) + integerBits(amount));
}
unsigned signedWidth(llvm::StringRef value) {
  llvm::APSInt signedValue(integerBits(value), false);
  return std::max(1u, signedValue.getSignificantBits());
}
unsigned domainWidth(llvm::StringRef lower, llvm::StringRef upper,
                     bool isSigned) {
  std::string last = addDecimal(upper, "-1");
  if (isSigned)
    return std::max(signedWidth(lower), signedWidth(last));
  return std::max(1u, integerBits(last).getActiveBits());
}
MathIntAttr mathInteger(mlir::MLIRContext &context, llvm::StringRef text) {
  unsigned width = static_cast<unsigned>(text.size() * 4 + 8);
  bool negative = text.consume_front("-");
  llvm::APInt value(width, text, 10);
  if (negative)
    value = -value;
  return MathIntAttr::get(&context, llvm::APSInt(std::move(value), !negative));
}
mlir::DictionaryAttr occurrence(mlir::OpBuilder &builder, unsigned line) {
  auto site = builder.getDictionaryAttr({
      builder.getNamedAttr(
          "definition",
          mlir::FlatSymbolRefAttr::get(builder.getContext(), "demo.Compute")),
      builder.getNamedAttr(
          "ast_path",
          builder.getArrayAttr({
              builder.getDictionaryAttr({
                  builder.getNamedAttr("kind", builder.getStringAttr("index")),
                  builder.getNamedAttr("value",
                                       builder.getI64IntegerAttr(line)),
              }),
          })),
  });
  return builder.getDictionaryAttr({
      builder.getNamedAttr("site", site),
      builder.getNamedAttr("expansion", builder.getArrayAttr({})),
  });
}
mlir::DictionaryAttr valueID(mlir::OpBuilder &builder,
                             mlir::DictionaryAttr origin) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("origin", origin),
      builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
  });
}
mlir::DictionaryAttr integerDomain(mlir::OpBuilder &builder,
                                   llvm::StringRef lower, llvm::StringRef upper,
                                   bool isSigned) {
  unsigned width = domainWidth(lower, upper, isSigned);
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("storage",
                           mlir::TypeAttr::get(builder.getIntegerType(width))),
      builder.getNamedAttr("lower", mathInteger(*builder.getContext(), lower)),
      builder.getNamedAttr("upper", mathInteger(*builder.getContext(), upper)),
      builder.getNamedAttr(
          "interpretation",
          builder.getStringAttr(isSigned ? "signed" : "unsigned")),
  });
}
mlir::DictionaryAttr scalarPort(mlir::OpBuilder &builder,
                                mlir::DictionaryAttr domain,
                                mlir::DictionaryAttr origin) {
  auto span = builder.getDictionaryAttr({
      builder.getNamedAttr("path", builder.getStringAttr("demo.py")),
      builder.getNamedAttr("line", builder.getI64IntegerAttr(4)),
      builder.getNamedAttr("column", builder.getI64IntegerAttr(3)),
      builder.getNamedAttr("end_line", builder.getI64IntegerAttr(4)),
      builder.getNamedAttr("end_column", builder.getI64IntegerAttr(8)),
  });
  return builder.getDictionaryAttr({
      builder.getNamedAttr("parameter", builder.getStringAttr("source")),
      builder.getNamedAttr("ordinal", builder.getUnitAttr()),
      builder.getNamedAttr("role", builder.getStringAttr("current")),
      builder.getNamedAttr("type", domain),
      builder.getNamedAttr("origin", origin),
      builder.getNamedAttr("location", span),
  });
}
mlir::DictionaryAttr formalState(mlir::OpBuilder &builder) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("formal")),
      builder.getNamedAttr("parameter", builder.getStringAttr("source")),
      builder.getNamedAttr("ordinal", builder.getUnitAttr()),
  });
}
mlir::DictionaryAttr constantRef(mlir::OpBuilder &builder,
                                 llvm::StringRef constant) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("constant")),
      builder.getNamedAttr("value",
                           mathInteger(*builder.getContext(), constant)),
  });
}
mlir::DictionaryAttr inputRef(mlir::OpBuilder &builder, mlir::Attribute index) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("input")),
      builder.getNamedAttr("index", index),
  });
}
mlir::DictionaryAttr nodeRef(mlir::OpBuilder &builder, unsigned index) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("node")),
      builder.getNamedAttr("index", builder.getI32IntegerAttr(index)),
  });
}
mlir::IntegerAttr integerAttr(mlir::OpBuilder &builder, unsigned width,
                              llvm::StringRef value) {
  return builder.getIntegerAttr(builder.getIntegerType(width),
                                integerBits(value, width));
}
struct VariableAddFixture {
  mlir::OwningOpRef<mlir::ModuleOp> file;
  ModuleOp hardware;
  RuleOp rule;
  mlir::BlockArgument current;
  SourceReadOp read;
  mlir::arith::ConstantOp one, truth;
  mlir::Operation *inputExtend = nullptr;
  mlir::Operation *constantExtend = nullptr;
  mlir::Value extendedInput, extendedConstant;
  mlir::arith::AddIOp sum;
  ValueBindingOp inputBinding, liftBinding, constantBinding, resultBinding;
  NumericProofOp proof;
  mlir::DictionaryAttr inputID, liftID, constantID, resultID;
  mlir::DictionaryAttr inputDomain, constantDomain, resultDomain;
  mlir::ArrayAttr nodes;
  std::string lower, upper, resultLower, resultUpper;
  unsigned inputWidth = 0, constantWidth = 0, resultWidth = 0;
  bool inputSigned = false;
};
VariableAddFixture buildVariableAddFixture(mlir::MLIRContext &context,
                                           llvm::StringRef lower,
                                           llvm::StringRef upper,
                                           llvm::StringRef constant = "1") {
  VariableAddFixture f;
  f.lower = lower.str();
  f.upper = upper.str();
  mlir::OpBuilder builder(&context);
  f.resultLower = addDecimal(lower, constant);
  f.resultUpper = addDecimal(upper, constant);
  f.inputSigned = integerBits(lower).isNegative();
  f.inputDomain = integerDomain(builder, lower, upper, f.inputSigned);
  f.inputWidth = domainWidth(lower, upper, f.inputSigned);
  f.constantWidth = domainWidth(constant, addDecimal(constant, "1"), false);
  bool resultSigned = integerBits(f.resultLower).isNegative();
  f.resultWidth = domainWidth(f.resultLower, f.resultUpper, resultSigned);
  auto location = mlir::FileLineColLoc::get(&context, "demo.py", 7, 3);
  auto moduleOrigin = occurrence(builder, 1);
  auto readOrigin = occurrence(builder, 2);
  auto liftOrigin = occurrence(builder, 3);
  auto constantOrigin = occurrence(builder, 4);
  auto resultOrigin = occurrence(builder, 5);
  f.inputID = valueID(builder, readOrigin);
  f.liftID = valueID(builder, liftOrigin);
  f.constantID = valueID(builder, constantOrigin);
  f.resultID = valueID(builder, resultOrigin);
  f.constantDomain =
      integerDomain(builder, constant, addDecimal(constant, "1"), false);
  f.resultDomain =
      integerDomain(builder, f.resultLower, f.resultUpper, resultSigned);
  auto port = scalarPort(builder, f.inputDomain, occurrence(builder, 6));
  auto inputBinding = formalState(builder);
  auto none = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("none")),
  });
  auto fromBitsNode = builder.getDictionaryAttr({
      builder.getNamedAttr("id", f.liftID),
      builder.getNamedAttr("operator", builder.getStringAttr("from_bits")),
      builder.getNamedAttr(
          "operands", builder.getArrayAttr(
                          {inputRef(builder, builder.getI32IntegerAttr(0))})),
      builder.getNamedAttr("target", none),
  });
  auto constantNode = builder.getDictionaryAttr({
      builder.getNamedAttr("id", f.constantID),
      builder.getNamedAttr("operator", builder.getStringAttr("constant")),
      builder.getNamedAttr(
          "operands", builder.getArrayAttr({constantRef(builder, constant)})),
      builder.getNamedAttr("target", none),
  });
  auto addNode = builder.getDictionaryAttr({
      builder.getNamedAttr("id", f.resultID),
      builder.getNamedAttr("operator", builder.getStringAttr("add")),
      builder.getNamedAttr(
          "operands",
          builder.getArrayAttr({nodeRef(builder, 0), nodeRef(builder, 1)})),
      builder.getNamedAttr("target", none),
  });
  f.nodes = builder.getArrayAttr({fromBitsNode, constantNode, addNode});

  f.file = mlir::ModuleOp::create(location);
  f.file.get()->setAttr("ac.stage", builder.getStringAttr("source"));
  f.file.get()->setAttr("ac.unit_kind",
                        builder.getStringAttr("implementation"));
  auto owner = builder.getDictionaryAttr({
      builder.getNamedAttr("package", builder.getStringAttr("demo")),
      builder.getNamedAttr("path", builder.getStringAttr("demo.py")),
  });
  f.file.get()->setAttr("ac.source_owner", owner);
  builder.setInsertionPointToStart(f.file->getBody());
  mlir::OperationState moduleState(location, ModuleOp::getOperationName());
  moduleState.addAttribute("name", builder.getStringAttr("Compute"));
  moduleState.addAttribute("sym_name", builder.getStringAttr("demo.Compute"));
  moduleState.addAttribute("ac.source_owner", owner);
  moduleState.addAttribute("ac.origin", moduleOrigin);
  moduleState.addAttribute("ac.ports", builder.getArrayAttr({port}));
  moduleState.addAttribute(
      "ac.control_ports",
      builder.getDictionaryAttr({
          builder.getNamedAttr("clock", builder.getI32IntegerAttr(0)),
          builder.getNamedAttr("reset", builder.getI32IntegerAttr(1)),
      }));
  moduleState.addRegion();
  f.hardware = mlir::cast<ModuleOp>(builder.create(moduleState));
  auto *moduleBody = new mlir::Block();
  f.hardware.getBody().push_back(moduleBody);
  moduleBody->addArgument(builder.getI1Type(), location);
  moduleBody->addArgument(builder.getI1Type(), location);
  auto inputHandleType =
      RegType::get(&context, builder.getIntegerType(f.inputWidth));
  moduleBody->addArgument(inputHandleType, location);
  f.current = moduleBody->getArgument(2);

  builder.setInsertionPointToEnd(moduleBody);
  auto specialization = builder.getDictionaryAttr({
      builder.getNamedAttr(
          "definition", mlir::FlatSymbolRefAttr::get(&context, "demo.Compute")),
      builder.getNamedAttr("arguments", builder.getArrayAttr({})),
  });
  auto registration = occurrence(builder, 10);
  auto proofScope = builder.getDictionaryAttr({
      builder.getNamedAttr("specialization", specialization),
      builder.getNamedAttr("registration", registration),
  });
  mlir::OperationState ruleState(location, RuleOp::getOperationName());
  ruleState.addOperands(f.current);
  ruleState.addAttribute("name", builder.getStringAttr("variable_add"));
  ruleState.addAttribute("registration", registration);
  ruleState.addAttribute("operandSegmentSizes",
                         builder.getDenseI32ArrayAttr({1, 0}));
  ruleState.addAttribute("ac.source_owner", owner);
  ruleState.addAttribute("ac.origin", occurrence(builder, 11));
  ruleState.addAttribute("ac.input_bindings",
                         builder.getArrayAttr({inputBinding}));
  ruleState.addAttribute("ac.output_bindings", builder.getArrayAttr({}));
  ruleState.addAttribute("ac.input_types",
                         builder.getArrayAttr({f.inputDomain}));
  ruleState.addAttribute("ac.output_types", builder.getArrayAttr({}));
  ruleState.addAttribute("ac.proof_scope", proofScope);
  ruleState.addAttribute("ac.required_numeric", f.nodes);
  ruleState.addAttribute("ac.required_checks", builder.getArrayAttr({}));
  ruleState.addRegion();
  f.rule = mlir::cast<RuleOp>(builder.create(ruleState));
  auto *body = new mlir::Block();
  f.rule.getBody().push_back(body);
  body->addArgument(builder.getIntegerType(f.inputWidth), location);
  builder.setInsertionPointToEnd(body);
  mlir::OperationState readState(location, SourceReadOp::getOperationName());
  readState.addOperands(body->getArgument(0));
  readState.addTypes(builder.getIntegerType(f.inputWidth));
  readState.addAttribute("ac.origin", readOrigin);
  f.read = mlir::cast<SourceReadOp>(builder.create(readState));
  f.truth = mlir::arith::ConstantOp::create(
      builder, location, builder.getI1Type(), builder.getBoolAttr(true));
  f.one = mlir::arith::ConstantOp::create(
      builder, location, builder.getIntegerType(f.constantWidth),
      integerAttr(builder, f.constantWidth, constant));
  auto extend = [&](mlir::Value value, unsigned fromWidth, bool sign,
                    mlir::Operation *&extension) {
    if (fromWidth == f.resultWidth)
      return value;
    if (sign)
      extension =
          mlir::arith::ExtSIOp::create(
              builder, location, builder.getIntegerType(f.resultWidth), value)
              .getOperation();
    else
      extension =
          mlir::arith::ExtUIOp::create(
              builder, location, builder.getIntegerType(f.resultWidth), value)
              .getOperation();
    return mlir::Value(extension->getResult(0));
  };
  f.extendedInput =
      extend(f.read.getResult(), f.inputWidth, f.inputSigned, f.inputExtend);
  f.extendedConstant =
      extend(f.one.getResult(), f.constantWidth, false, f.constantExtend);
  f.sum = mlir::arith::AddIOp::create(builder, location,
                                      builder.getIntegerType(f.resultWidth),
                                      f.extendedInput, f.extendedConstant);
  auto makeBinding = [&](mlir::Value value, mlir::DictionaryAttr id,
                         mlir::DictionaryAttr domain) {
    mlir::OperationState state(location, ValueBindingOp::getOperationName());
    state.addOperands({value, f.truth.getResult(), f.truth.getResult()});
    state.addAttribute("id", id);
    state.addAttribute("domain", domain);
    return mlir::cast<ValueBindingOp>(builder.create(state));
  };
  f.inputBinding = makeBinding(f.read.getResult(), f.inputID, f.inputDomain);
  f.liftBinding = makeBinding(f.read.getResult(), f.liftID, f.inputDomain);
  f.constantBinding =
      makeBinding(f.one.getResult(), f.constantID, f.constantDomain);
  f.resultBinding = makeBinding(f.sum.getResult(), f.resultID, f.resultDomain);
  mlir::OperationState proofState(location, NumericProofOp::getOperationName());
  proofState.addOperands({f.truth.getResult(), f.read.getResult(),
                          f.truth.getResult(), f.sum.getResult(),
                          f.truth.getResult()});
  proofState.addAttribute("operand_segment_sizes",
                          builder.getDenseI32ArrayAttr({1, 1, 1, 1, 1, 0, 0}));
  proofState.addAttribute("mode", builder.getStringAttr("exact"));
  proofState.addAttribute("result_domain", f.resultDomain);
  proofState.addAttribute("input_ids", builder.getArrayAttr({f.inputID}));
  proofState.addAttribute("input_domains",
                          builder.getArrayAttr({f.inputDomain}));
  proofState.addAttribute("result_id", f.resultID);
  proofState.addAttribute("obligations", f.nodes);
  proofState.addAttribute("checks", builder.getArrayAttr({}));
  proofState.addAttribute("origin", resultOrigin);
  f.proof = mlir::cast<NumericProofOp>(builder.create(proofState));
  builder.create<YieldOp>(location, mlir::ValueRange{});
  builder.setInsertionPointToEnd(moduleBody);
  builder.create<YieldOp>(location, mlir::ValueRange{});
  return f;
}

struct Verification {
  bool passed;
  std::string diagnostic;
};
Verification verify(mlir::MLIRContext &context, mlir::Operation *op) {
  std::string message;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &diag) {
    llvm::raw_string_ostream(message) << diag;
    return mlir::success();
  });
  return {mlir::succeeded(mlir::verify(op)), message};
}

class NumericVariableAddProofContractsTest : public ::testing::Test {
protected:
  NumericVariableAddProofContractsTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect>();
  }
  mlir::MLIRContext context;
};

TEST_F(NumericVariableAddProofContractsTest, AcceptsCanonicalIntervalCases) {
  struct Example {
    const char *lower, *upper;
  };
  const std::array<Example, 6> examples = {{{"0", "7"},
                                            {"0", "8"},
                                            {"-4", "0"},
                                            {"-1", "1"},
                                            {"0", "2"},
                                            {"0", "18446744073709551615"}}};
  for (const auto &example : examples) {
    SCOPED_TRACE(std::string("[") + example.lower + "," + example.upper +
                 ") + 1");
    auto fixture =
        buildVariableAddFixture(context, example.lower, example.upper);
    auto result = verify(context, *fixture.file);
    ASSERT_TRUE(result.passed) << result.diagnostic;
  }
}

TEST_F(NumericVariableAddProofContractsTest,
       ExhaustivelyMatchesSmallIntervalOracle) {
  for (auto [lower, upper] :
       std::array<std::pair<const char *, const char *>, 5>{
           {{"0", "7"}, {"0", "8"}, {"-4", "0"}, {"-1", "1"}, {"0", "2"}}}) {
    auto f = buildVariableAddFixture(context, lower, upper);
    ASSERT_TRUE(verify(context, *f.file).passed);
    auto type = mlir::cast<mlir::IntegerType>(f.sum.getType());
    for (int value = std::stoi(lower), end = std::stoi(upper); value < end;
         ++value) {
      auto finiteInput = integerBits(std::to_string(value), f.inputWidth);
      auto actual = test::evaluateFiniteSSA(f.proof->getOperand(3),
                                            f.read.getResult(), finiteInput);
      ASSERT_TRUE(actual);
      bool isResultSigned = integerBits(f.resultLower).isNegative();
      llvm::APSInt actualValue(*actual, !isResultSigned);
      llvm::SmallString<64> actualText;
      actualValue.toString(actualText);
      EXPECT_EQ(actualText.str().str(), std::to_string(value + 1));
      EXPECT_EQ(type.getWidth(), f.resultWidth);
      if (value == 2 && std::string(lower) == "0" &&
          std::string(upper) == "7") {
        mlir::Value original = f.sum.getRhs();
        f.sum->setOperand(1, f.sum.getLhs());
        auto changed = test::evaluateFiniteSSA(f.sum.getResult(),
                                               f.read.getResult(), finiteInput);
        ASSERT_TRUE(changed);
        EXPECT_NE(*changed, *actual);
        f.sum->setOperand(1, original);
      }
    }
  }
}

TEST_F(NumericVariableAddProofContractsTest, RejectsBoundedInputMutations) {
  using Mutation = std::function<void(VariableAddFixture &)>;
  std::vector<std::pair<std::string, Mutation>> mutations;
  mutations.push_back({"segments", [&](auto &f) {
                         f.proof->setAttr("operand_segment_sizes",
                                          mlir::DenseI32ArrayAttr::get(
                                              &context, {1, 0, 0, 1, 1, 0, 0}));
                         f.proof->eraseOperands(1, 2);
                       }});
  mutations.push_back({"input-lists", [&](auto &f) {
                         mlir::OpBuilder b(&context);
                         f.proof->setAttr("input_ids", b.getArrayAttr({}));
                         f.proof->setAttr("input_domains", b.getArrayAttr({}));
                       }});
  mutations.push_back({"input-id", [&](auto &f) {
                         f.proof->setAttr(
                             "input_ids",
                             mlir::ArrayAttr::get(&context, {f.liftID}));
                       }});
  mutations.push_back({"input-domain", [&](auto &f) {
                         mlir::OpBuilder b(&context);
                         f.proof->setAttr("input_domains",
                                          b.getArrayAttr({f.constantDomain}));
                       }});
  mutations.push_back({"input-value-bypass", [&](auto &f) {
                         f.proof->setOperand(
                             1, f.rule.getBody().front().getArgument(0));
                       }});
  mutations.push_back({"input-valid-mismatch", [&](auto &f) {
                         f.proof->setOperand(2, f.one.getResult());
                       }});
  mutations.push_back({"lift-value", [&](auto &f) {
                         f.liftBinding->setOperand(0, f.one.getResult());
                       }});
  mutations.push_back({"lift-domain", [&](auto &f) {
                         mlir::OpBuilder b(&context);
                         f.liftBinding->setAttr("domain", f.constantDomain);
                       }});
  mutations.push_back({"binding-order-id", [&](auto &f) {
                         f.inputBinding->setAttr("id", f.liftID);
                       }});
  mutations.push_back(
      {"input-authority-range", [&](auto &f) {
         mlir::OpBuilder b(&context);
         auto narrow = integerDomain(b, f.lower, addDecimal(f.upper, "-1"),
                                     f.inputSigned);
         f.rule->setAttr("ac.input_types", b.getArrayAttr({narrow}));
         f.inputBinding->setAttr("domain", narrow);
         f.liftBinding->setAttr("domain", narrow);
         f.proof->setAttr("input_domains", b.getArrayAttr({narrow}));
       }});
  mutations.push_back({"source-read-origin", [&](auto &f) {
                         mlir::OpBuilder b(&context);
                         f.read->setAttr("ac.origin", occurrence(b, 30));
                       }});
  mutations.push_back({"wrong-read-current", [&](auto &f) {
                         f.read->setOperand(
                             0, f.hardware.getBody().front().getArgument(0));
                       }});
  mutations.push_back({"wrong-proof-path", [&](auto &f) {
                         f.proof->setOperand(0, f.sum.getResult());
                       }});
  mutations.push_back({"false-controls", [&](auto &f) {
                         mlir::OpBuilder b(f.inputBinding);
                         auto no = mlir::arith::ConstantOp::create(
                             b, f.inputBinding.getLoc(), b.getI1Type(),
                             b.getBoolAttr(false));
                         f.inputBinding->setOperand(1, no.getResult());
                         f.proof->setOperand(2, no.getResult());
                       }});
  mutations.push_back({"false-input-path", [&](auto &f) {
                         mlir::OpBuilder b(f.inputBinding);
                         auto no = mlir::arith::ConstantOp::create(
                             b, f.inputBinding.getLoc(), b.getI1Type(),
                             b.getBoolAttr(false));
                         f.inputBinding->setOperand(2, no.getResult());
                         f.proof->setOperand(0, no.getResult());
                       }});
  mutations.push_back({"final-valid-ssa", [&](auto &f) {
                         f.proof->setOperand(4, f.one.getResult());
                       }});
  mutations.push_back({"signedness-extension", [&](auto &f) {
                         mlir::OpBuilder b(f.inputExtend);
                         auto ext = mlir::arith::ExtSIOp::create(
                             b, f.inputExtend->getLoc(),
                             f.inputExtend->getResult(0).getType(),
                             f.read.getResult());
                         f.sum->setOperand(0, ext.getResult());
                       }});
  mutations.push_back({"wrong-extension-source", [&](auto &f) {
                         if (f.inputExtend)
                           f.inputExtend->setOperand(0, f.one.getResult());
                         else
                           f.sum->setOperand(0, f.one.getResult());
                       }});
  mutations.push_back({"actual-sub", [&](auto &f) {
                         mlir::OpBuilder b(f.sum);
                         auto sub = mlir::arith::SubIOp::create(
                             b, f.sum.getLoc(), f.sum.getType(), f.sum.getLhs(),
                             f.sum.getRhs());
                         f.resultBinding->setOperand(0, sub.getResult());
                         f.proof->setOperand(3, sub.getResult());
                         f.sum.erase();
                       }});
  mutations.push_back({"actual-result", [&](auto &f) {
                         f.proof->setOperand(3, f.one.getResult());
                       }});
  mutations.push_back({"result-domain", [&](auto &f) {
                         mlir::OpBuilder b(&context);
                         auto wrong = integerDomain(b, "0", "2", false);
                         f.resultBinding->setAttr("domain", wrong);
                         f.proof->setAttr("result_domain", wrong);
                       }});
  mutations.push_back(
      {"required-mismatch", [&](auto &f) {
         f.rule->setAttr(
             "ac.required_numeric",
             mlir::ArrayAttr::get(&context, {f.nodes[0], f.nodes[1]}));
       }});
  mutations.push_back(
      {"extra-source-read", [&](auto &f) {
         mlir::OpBuilder b(f.inputBinding);
         mlir::OperationState state(f.read.getLoc(),
                                    SourceReadOp::getOperationName());
         state.addOperands(f.rule.getBody().front().getArgument(0));
         state.addTypes(f.rule.getBody().front().getArgument(0).getType());
         state.addAttribute("ac.origin", occurrence(b, 31));
         (void)b.create(state);
       }});
  for (const auto &[name, mutate] : mutations) {
    SCOPED_TRACE(name);
    auto f = buildVariableAddFixture(context, "0", "8");
    auto baseline = verify(context, *f.file);
    ASSERT_TRUE(baseline.passed) << baseline.diagnostic;
    mutate(f);
    EXPECT_FALSE(verify(context, *f.file).passed);
  }
}

TEST_F(NumericVariableAddProofContractsTest,
       RejectsUnrepresentableAndUnsupportedInputs) {
  EXPECT_FALSE(
      verify(
          context,
          *buildVariableAddFixture(context, "0", "18446744073709551616").file)
          .passed);
  EXPECT_FALSE(
      verify(context, *buildVariableAddFixture(context, "-8", "0", "8").file)
          .passed);
  auto f = buildVariableAddFixture(context, "0", "7");
  mlir::OpBuilder b(&context);
  f.proof->setAttr("mode", b.getStringAttr("low_bits"));
  EXPECT_FALSE(verify(context, *f.file).passed);
}

} // namespace
} // namespace acir::ac
