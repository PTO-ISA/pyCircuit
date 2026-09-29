#include "Dialect/ACIR/ACIRNumericExactScalar.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>
namespace acir::ac {
namespace {

llvm::APInt bits(llvm::StringRef text, unsigned width = 256) {
  bool negative = text.consume_front("-");
  llvm::APInt value(width, text, 10);
  return negative ? -value : value;
}

std::string decimal(llvm::APInt value) {
  llvm::SmallString<96> text;
  llvm::APSInt(std::move(value), false).toString(text);
  return text.str().str();
}

std::string add(llvm::StringRef lhs, llvm::StringRef rhs) {
  return decimal(bits(lhs) + bits(rhs));
}

unsigned widthFor(llvm::StringRef lower, llvm::StringRef upper, bool isSigned) {
  llvm::APInt low = bits(lower), high = bits(add(upper, "-1"));
  if (isSigned)
    return std::max(1u,
                    std::max(llvm::APSInt(low, false).getSignificantBits(),
                             llvm::APSInt(high, false).getSignificantBits()));
  return std::max(1u, high.getActiveBits());
}

MathIntAttr mathInt(mlir::MLIRContext &context, llvm::StringRef text) {
  bool negative = text.consume_front("-");
  llvm::APInt value(std::max<unsigned>(8, text.size() * 4 + 4), text, 10);
  if (negative)
    value = -value;
  return MathIntAttr::get(&context, llvm::APSInt(std::move(value), !negative));
}

mlir::DictionaryAttr occurrence(mlir::OpBuilder &b, unsigned index) {
  auto site = b.getDictionaryAttr({
      b.getNamedAttr("definition",
                     mlir::FlatSymbolRefAttr::get(b.getContext(), "d1.Unit")),
      b.getNamedAttr("ast_path",
                     b.getArrayAttr({b.getDictionaryAttr({
                         b.getNamedAttr("kind", b.getStringAttr("index")),
                         b.getNamedAttr("value", b.getI64IntegerAttr(index)),
                     })})),
  });
  return b.getDictionaryAttr({b.getNamedAttr("site", site),
                              b.getNamedAttr("expansion", b.getArrayAttr({}))});
}

mlir::DictionaryAttr valueID(mlir::OpBuilder &b, unsigned origin,
                             unsigned slot) {
  return b.getDictionaryAttr(
      {b.getNamedAttr("origin", occurrence(b, origin)),
       b.getNamedAttr("slot", b.getI32IntegerAttr(slot))});
}

mlir::DictionaryAttr intDomain(mlir::OpBuilder &b, llvm::StringRef lower,
                               llvm::StringRef upper, bool isSigned,
                               std::optional<unsigned> forcedWidth = {}) {
  unsigned width = forcedWidth.value_or(widthFor(lower, upper, isSigned));
  return b.getDictionaryAttr({
      b.getNamedAttr("kind", b.getStringAttr("integer")),
      b.getNamedAttr("storage", mlir::TypeAttr::get(b.getIntegerType(width))),
      b.getNamedAttr("lower", mathInt(*b.getContext(), lower)),
      b.getNamedAttr("upper", mathInt(*b.getContext(), upper)),
      b.getNamedAttr("interpretation",
                     b.getStringAttr(isSigned ? "signed" : "unsigned")),
  });
}

mlir::DictionaryAttr boolDomain(mlir::OpBuilder &b) {
  return b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("bool"))});
}

mlir::DictionaryAttr ref(mlir::OpBuilder &b, llvm::StringRef kind,
                         unsigned index) {
  return b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr(kind)),
       b.getNamedAttr("index", b.getI32IntegerAttr(index))});
}

mlir::DictionaryAttr constantRef(mlir::OpBuilder &b, llvm::StringRef value) {
  return b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("constant")),
       b.getNamedAttr("value", mathInt(*b.getContext(), value))});
}

mlir::DictionaryAttr node(mlir::OpBuilder &b, mlir::DictionaryAttr id,
                          llvm::StringRef op, mlir::ArrayAttr operands) {
  auto none =
      b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("none"))});
  return b.getDictionaryAttr({b.getNamedAttr("id", id),
                              b.getNamedAttr("operator", b.getStringAttr(op)),
                              b.getNamedAttr("operands", operands),
                              b.getNamedAttr("target", none)});
}

mlir::IntegerAttr integerAttr(mlir::OpBuilder &b, unsigned width,
                              llvm::StringRef value) {
  return b.getIntegerAttr(b.getIntegerType(width), bits(value, width));
}

struct Fixture {
  mlir::OwningOpRef<mlir::ModuleOp> file;
  ModuleOp hardware;
  RuleOp rule;
  mlir::BlockArgument current;
  SourceReadOp read;
  mlir::arith::ConstantOp constant, truth;
  mlir::Operation *inputExt = nullptr, *constantExt = nullptr;
  mlir::Operation *actual = nullptr;
  ValueBindingOp inputBinding, liftBinding, constantBinding, resultBinding;
  NumericProofOp proof;
  mlir::DictionaryAttr inputID, liftID, constantID, resultID;
  mlir::DictionaryAttr inputDomain, constantDomain, resultDomain;
  mlir::ArrayAttr obligations;
  std::string op, lower, upper, k;
  unsigned inputWidth = 0, constantWidth = 0, workWidth = 0;
  bool inputSigned = false, workSigned = false;
};

mlir::arith::CmpIPredicate predicate(llvm::StringRef op, bool isSigned) {
  if (op == "eq")
    return mlir::arith::CmpIPredicate::eq;
  if (op == "ne")
    return mlir::arith::CmpIPredicate::ne;
  if (op == "lt")
    return isSigned ? mlir::arith::CmpIPredicate::slt
                    : mlir::arith::CmpIPredicate::ult;
  if (op == "le")
    return isSigned ? mlir::arith::CmpIPredicate::sle
                    : mlir::arith::CmpIPredicate::ule;
  if (op == "gt")
    return isSigned ? mlir::arith::CmpIPredicate::sgt
                    : mlir::arith::CmpIPredicate::ugt;
  return isSigned ? mlir::arith::CmpIPredicate::sge
                  : mlir::arith::CmpIPredicate::uge;
}

Fixture build(mlir::MLIRContext &context, llvm::StringRef op,
              llvm::StringRef lower = "0", llvm::StringRef upper = "8",
              llvm::StringRef constant = "1") {
  Fixture f;
  f.op = op.str();
  f.lower = lower.str();
  f.upper = upper.str();
  f.k = constant.str();
  f.inputSigned = bits(lower).isNegative();
  bool constantSigned = bits(constant).isNegative();
  mlir::OpBuilder b(&context);
  f.inputWidth = widthFor(lower, upper, f.inputSigned);
  f.constantWidth = widthFor(constant, add(constant, "1"), constantSigned);
  f.inputDomain = intDomain(b, lower, upper, f.inputSigned);
  f.constantDomain = intDomain(b, constant, add(constant, "1"), constantSigned);
  bool comparison = op != "sub";
  if (comparison) {
    f.workSigned = f.inputSigned || constantSigned;
    f.workWidth =
        std::max(widthFor(lower, upper, f.workSigned),
                 widthFor(constant, add(constant, "1"), f.workSigned));
    f.resultDomain = boolDomain(b);
  } else {
    std::string resultLower = add(lower, decimal(-bits(constant)));
    std::string resultUpper = add(upper, decimal(-bits(constant)));
    f.workSigned = bits(resultLower).isNegative();
    f.workWidth = widthFor(resultLower, resultUpper, f.workSigned);
    f.workWidth = std::max({f.workWidth, f.inputWidth, f.constantWidth});
    f.resultDomain =
        intDomain(b, resultLower, resultUpper, f.workSigned, f.workWidth);
  }
  f.inputID = valueID(b, 20, 0);
  f.liftID = valueID(b, 21, 7);
  f.constantID = valueID(b, 22, 3);
  f.resultID = valueID(b, 23, 9);
  f.obligations = b.getArrayAttr({
      node(b, f.liftID, "from_bits", b.getArrayAttr({ref(b, "input", 0)})),
      node(b, f.constantID, "constant",
           b.getArrayAttr({constantRef(b, constant)})),
      node(b, f.resultID, op,
           b.getArrayAttr({ref(b, "node", 0), ref(b, "node", 1)})),
  });

  auto loc = mlir::FileLineColLoc::get(&context, "d1.py", 8, 2);
  f.file = mlir::ModuleOp::create(loc);
  auto owner =
      b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr("d1")),
                           b.getNamedAttr("path", b.getStringAttr("d1.py"))});
  f.file.get()->setAttr("ac.stage", b.getStringAttr("source"));
  f.file.get()->setAttr("ac.unit_kind", b.getStringAttr("implementation"));
  f.file.get()->setAttr("ac.source_owner", owner);
  b.setInsertionPointToStart(f.file->getBody());
  mlir::OperationState moduleState(loc, ModuleOp::getOperationName());
  moduleState.addAttribute("name", b.getStringAttr("Unit"));
  moduleState.addAttribute("sym_name", b.getStringAttr("d1.Unit"));
  moduleState.addAttribute("ac.source_owner", owner);
  moduleState.addAttribute("ac.origin", occurrence(b, 1));
  auto port = b.getDictionaryAttr({
      b.getNamedAttr("parameter", b.getStringAttr("source")),
      b.getNamedAttr("ordinal", b.getUnitAttr()),
      b.getNamedAttr("role", b.getStringAttr("current")),
      b.getNamedAttr("type", f.inputDomain),
      b.getNamedAttr("origin", occurrence(b, 2)),
      b.getNamedAttr("location",
                     b.getDictionaryAttr({
                         b.getNamedAttr("path", b.getStringAttr("d1.py")),
                         b.getNamedAttr("line", b.getI64IntegerAttr(2)),
                         b.getNamedAttr("column", b.getI64IntegerAttr(1)),
                         b.getNamedAttr("end_line", b.getI64IntegerAttr(2)),
                         b.getNamedAttr("end_column", b.getI64IntegerAttr(7)),
                     })),
  });
  moduleState.addAttribute("ac.ports", b.getArrayAttr({port}));
  moduleState.addAttribute("ac.control_ports",
                           b.getDictionaryAttr({
                               b.getNamedAttr("clock", b.getI32IntegerAttr(0)),
                               b.getNamedAttr("reset", b.getI32IntegerAttr(1)),
                           }));
  moduleState.addRegion();
  f.hardware = mlir::cast<ModuleOp>(b.create(moduleState));
  auto *moduleBody = new mlir::Block();
  f.hardware.getBody().push_back(moduleBody);
  moduleBody->addArgument(b.getI1Type(), loc);
  moduleBody->addArgument(b.getI1Type(), loc);
  moduleBody->addArgument(
      RegType::get(&context, b.getIntegerType(f.inputWidth)), loc);
  f.current = moduleBody->getArgument(2);

  b.setInsertionPointToEnd(moduleBody);
  auto registration = occurrence(b, 10);
  auto inputBinding = b.getDictionaryAttr({
      b.getNamedAttr("kind", b.getStringAttr("formal")),
      b.getNamedAttr("parameter", b.getStringAttr("source")),
      b.getNamedAttr("ordinal", b.getUnitAttr()),
  });
  auto spec = b.getDictionaryAttr({
      b.getNamedAttr("definition",
                     mlir::FlatSymbolRefAttr::get(&context, "d1.Unit")),
      b.getNamedAttr("arguments", b.getArrayAttr({})),
  });
  mlir::OperationState ruleState(loc, RuleOp::getOperationName());
  ruleState.addOperands(f.current);
  ruleState.addAttribute("name", b.getStringAttr("scalar"));
  ruleState.addAttribute("registration", registration);
  ruleState.addAttribute("operandSegmentSizes", b.getDenseI32ArrayAttr({1, 0}));
  ruleState.addAttribute("ac.source_owner", owner);
  ruleState.addAttribute("ac.origin", occurrence(b, 11));
  ruleState.addAttribute("ac.input_bindings", b.getArrayAttr({inputBinding}));
  ruleState.addAttribute("ac.output_bindings", b.getArrayAttr({}));
  ruleState.addAttribute("ac.input_types", b.getArrayAttr({f.inputDomain}));
  ruleState.addAttribute("ac.output_types", b.getArrayAttr({}));
  ruleState.addAttribute("ac.proof_scope",
                         b.getDictionaryAttr({
                             b.getNamedAttr("specialization", spec),
                             b.getNamedAttr("registration", registration),
                         }));
  ruleState.addAttribute("ac.required_numeric", f.obligations);
  ruleState.addAttribute("ac.required_checks", b.getArrayAttr({}));
  ruleState.addRegion();
  f.rule = mlir::cast<RuleOp>(b.create(ruleState));
  auto *body = new mlir::Block();
  f.rule.getBody().push_back(body);
  body->addArgument(b.getIntegerType(f.inputWidth), loc);
  b.setInsertionPointToEnd(body);
  mlir::OperationState readState(loc, SourceReadOp::getOperationName());
  readState.addOperands(body->getArgument(0));
  readState.addTypes(b.getIntegerType(f.inputWidth));
  readState.addAttribute("ac.origin", occurrence(b, 20));
  f.read = mlir::cast<SourceReadOp>(b.create(readState));
  f.truth = mlir::arith::ConstantOp::create(b, loc, b.getI1Type(),
                                            b.getBoolAttr(true));
  f.constant = mlir::arith::ConstantOp::create(
      b, loc, b.getIntegerType(f.constantWidth),
      integerAttr(b, f.constantWidth, constant));
  auto extend = [&](mlir::Value value, unsigned from, bool isSigned,
                    mlir::Operation *&created) -> mlir::Value {
    if (from == f.workWidth)
      return value;
    created = isSigned ? mlir::arith::ExtSIOp::create(
                             b, loc, b.getIntegerType(f.workWidth), value)
                             .getOperation()
                       : mlir::arith::ExtUIOp::create(
                             b, loc, b.getIntegerType(f.workWidth), value)
                             .getOperation();
    return created->getResult(0);
  };
  mlir::Value lhs =
      extend(f.read.getResult(), f.inputWidth, f.inputSigned, f.inputExt);
  mlir::Value rhs = extend(f.constant.getResult(), f.constantWidth,
                           constantSigned, f.constantExt);
  if (comparison)
    f.actual = mlir::arith::CmpIOp::create(b, loc, predicate(op, f.workSigned),
                                           lhs, rhs)
                   .getOperation();
  else
    f.actual = mlir::arith::SubIOp::create(b, loc, lhs, rhs).getOperation();
  auto bind = [&](mlir::Value value, mlir::DictionaryAttr id,
                  mlir::DictionaryAttr domain) {
    mlir::OperationState state(loc, ValueBindingOp::getOperationName());
    state.addOperands({value, f.truth.getResult(), f.truth.getResult()});
    state.addAttribute("id", id);
    state.addAttribute("domain", domain);
    return mlir::cast<ValueBindingOp>(b.create(state));
  };
  f.inputBinding = bind(f.read.getResult(), f.inputID, f.inputDomain);
  f.liftBinding = bind(f.read.getResult(), f.liftID, f.inputDomain);
  f.constantBinding =
      bind(f.constant.getResult(), f.constantID, f.constantDomain);
  f.resultBinding = bind(f.actual->getResult(0), f.resultID, f.resultDomain);
  mlir::OperationState proofState(loc, NumericProofOp::getOperationName());
  proofState.addOperands({f.truth.getResult(), f.read.getResult(),
                          f.truth.getResult(), f.actual->getResult(0),
                          f.truth.getResult()});
  proofState.addAttribute("operand_segment_sizes",
                          b.getDenseI32ArrayAttr({1, 1, 1, 1, 1, 0, 0}));
  proofState.addAttribute("mode", b.getStringAttr("exact"));
  proofState.addAttribute("result_domain", f.resultDomain);
  proofState.addAttribute("input_ids", b.getArrayAttr({f.inputID}));
  proofState.addAttribute("input_domains", b.getArrayAttr({f.inputDomain}));
  proofState.addAttribute("result_id", f.resultID);
  proofState.addAttribute("obligations", f.obligations);
  proofState.addAttribute("checks", b.getArrayAttr({}));
  proofState.addAttribute("origin", occurrence(b, 23));
  f.proof = mlir::cast<NumericProofOp>(b.create(proofState));
  YieldOp::create(b, loc, mlir::ValueRange{});
  b.setInsertionPointToEnd(moduleBody);
  YieldOp::create(b, loc, mlir::ValueRange{});
  return f;
}

struct Verification {
  bool passed;
  std::string diagnostic;
};

Verification verify(Fixture &f) {
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(
      f.rule.getContext(), [&](mlir::Diagnostic &value) {
        llvm::raw_string_ostream(diagnostic) << value;
        return mlir::success();
      });
  mlir::LogicalResult result =
      f.op == "sub"
          ? verifyExactInputConstantSubWitness(f.rule, f.resultBinding, f.proof)
          : verifyExactInputConstantCompareWitness(f.rule, f.resultBinding,
                                                   f.proof);
  return {mlir::succeeded(result), diagnostic};
}

std::optional<llvm::APInt> evaluate(mlir::Value value, SourceReadOp read,
                                    llvm::APInt source) {
  if (value == read.getResult())
    return source;
  if (auto constant = value.getDefiningOp<mlir::arith::ConstantOp>())
    return mlir::cast<mlir::IntegerAttr>(constant.getValue()).getValue();
  if (auto ext = value.getDefiningOp<mlir::arith::ExtUIOp>()) {
    auto input = evaluate(ext.getIn(), read, source);
    return input ? std::optional(
                       input->zext(ext.getType().getIntOrFloatBitWidth()))
                 : std::nullopt;
  }
  if (auto ext = value.getDefiningOp<mlir::arith::ExtSIOp>()) {
    auto input = evaluate(ext.getIn(), read, source);
    return input ? std::optional(
                       input->sext(ext.getType().getIntOrFloatBitWidth()))
                 : std::nullopt;
  }
  if (auto sub = value.getDefiningOp<mlir::arith::SubIOp>()) {
    auto lhs = evaluate(sub.getLhs(), read, source);
    auto rhs = evaluate(sub.getRhs(), read, source);
    return lhs && rhs ? std::optional(*lhs - *rhs) : std::nullopt;
  }
  if (auto cmp = value.getDefiningOp<mlir::arith::CmpIOp>()) {
    auto lhs = evaluate(cmp.getLhs(), read, source);
    auto rhs = evaluate(cmp.getRhs(), read, source);
    if (!lhs || !rhs)
      return std::nullopt;
    return llvm::APInt(
        1, mlir::arith::applyCmpPredicate(cmp.getPredicate(), *lhs, *rhs));
  }
  return std::nullopt;
}

class NumericExactScalarProofContractsTest : public ::testing::Test {
protected:
  NumericExactScalarProofContractsTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect>();
  }
  mlir::MLIRContext context;
};

TEST_F(NumericExactScalarProofContractsTest,
       AcceptsSubtractionAndAllComparisonProfiles) {
  for (const auto &[lower, upper, constant] :
       std::array<std::array<const char *, 3>, 5>{{
           {{"0", "8", "1"}},
           {{"-4", "1", "1"}},
           {{"0", "256", "1"}},
           {{"-1", "1", "-1"}},
           {{"0", "256", "-1"}},
       }}) {
    for (llvm::StringRef op : {"sub", "eq", "ne", "lt", "le", "gt", "ge"}) {
      SCOPED_TRACE((op + " [" + lower + "," + upper + ") k=" + constant).str());
      auto f = build(context, op, lower, upper, constant);
      EXPECT_TRUE(verify(f).passed) << verify(f).diagnostic;
      EXPECT_EQ(f.inputID.getAs<mlir::IntegerAttr>("slot").getInt(), 0);
      EXPECT_EQ(f.liftID.getAs<mlir::IntegerAttr>("slot").getInt(), 7);
      EXPECT_EQ(f.constantID.getAs<mlir::IntegerAttr>("slot").getInt(), 3);
      EXPECT_EQ(f.resultID.getAs<mlir::IntegerAttr>("slot").getInt(), 9);
    }
  }
}

TEST_F(NumericExactScalarProofContractsTest, ExhaustiveSmallDomainOracle) {
  for (llvm::StringRef op : {"sub", "eq", "ne", "lt", "le", "gt", "ge"}) {
    auto f = build(context, op, "-3", "4", "-1");
    ASSERT_TRUE(verify(f).passed) << verify(f).diagnostic;
    for (int value = -3; value < 4; ++value) {
      auto actual = evaluate(f.actual->getResult(0), f.read,
                             bits(std::to_string(value), f.inputWidth));
      ASSERT_TRUE(actual);
      if (op == "sub") {
        llvm::APSInt interpreted(*actual, false);
        llvm::SmallString<16> text;
        interpreted.toString(text);
        EXPECT_EQ(text.str(), std::to_string(value + 1));
      } else {
        bool expected = op == "eq"   ? value == -1
                        : op == "ne" ? value != -1
                        : op == "lt" ? value < -1
                        : op == "le" ? value <= -1
                        : op == "gt" ? value > -1
                                     : value >= -1;
        EXPECT_EQ(actual->getBoolValue(), expected);
      }
    }
  }
}

TEST_F(NumericExactScalarProofContractsTest, RejectsSSAAndProofMutations) {
  using Mutation = std::function<void(Fixture &)>;
  std::vector<std::pair<const char *, Mutation>> cases;
  cases.push_back({"wrong-predicate", [](auto &f) {
                     auto cmp = mlir::cast<mlir::arith::CmpIOp>(f.actual);
                     cmp.setPredicate(mlir::arith::CmpIPredicate::ne);
                   }});
  cases.push_back({"signedness", [](auto &f) {
                     auto cmp = mlir::cast<mlir::arith::CmpIOp>(f.actual);
                     cmp.setPredicate(mlir::arith::CmpIPredicate::ult);
                   }});
  cases.push_back({"altered-result-id", [&](auto &f) {
                     mlir::OpBuilder b(&context);
                     f.resultBinding->setAttr("id", valueID(b, 23, 10));
                   }});
  cases.push_back({"altered-domain", [&](auto &f) {
                     mlir::OpBuilder b(&context);
                     f.resultBinding->setAttr("domain",
                                              intDomain(b, "0", "2", false));
                   }});
  cases.push_back({"altered-constant", [&](auto &f) {
                     mlir::OpBuilder b(&context);
                     f.constant->setAttr("value",
                                         integerAttr(b, f.constantWidth, "0"));
                   }});
  cases.push_back({"nontrue-controls", [&](auto &f) {
                     mlir::OpBuilder b(f.inputBinding);
                     auto no = mlir::arith::ConstantOp::create(
                         b, f.inputBinding.getLoc(), b.getI1Type(),
                         b.getBoolAttr(false));
                     f.inputBinding->setOperand(1, no.getResult());
                   }});
  cases.push_back({"extra-evidence", [&](auto &f) {
                     mlir::OpBuilder b(f.inputBinding);
                     (void)mlir::arith::ConstantOp::create(
                         b, f.inputBinding.getLoc(), b.getI8Type(),
                         b.getI8IntegerAttr(4));
                   }});
  cases.push_back(
      {"cross-rule-evidence", [](auto &f) {
         auto other = mlir::cast<RuleOp>(f.rule->clone());
         mlir::Block &moduleBody = f.hardware.getBody().front();
         moduleBody.getOperations().insert(
             moduleBody.getTerminator()->getIterator(), other);
         f.constantBinding->moveBefore(
             &other.getBody().front(),
             other.getBody().front().getTerminator()->getIterator());
       }});
  cases.push_back({"second-runtime-input", [&](auto &f) {
                     mlir::OpBuilder b(f.inputBinding);
                     mlir::OperationState state(
                         f.read.getLoc(), SourceReadOp::getOperationName());
                     state.addOperands(f.rule.getBody().front().getArgument(0));
                     state.addTypes(f.read.getType());
                     state.addAttribute("ac.origin", occurrence(b, 31));
                     (void)b.create(state);
                   }});
  cases.push_back({"targets", [&](auto &f) {
                     f.rule->insertOperands(1, f.current);
                     f.rule->setAttr(
                         "operandSegmentSizes",
                         mlir::DenseI32ArrayAttr::get(&context, {1, 1}));
                   }});
  cases.push_back({"checks", [&](auto &f) {
                     mlir::OpBuilder b(&context);
                     f.rule->setAttr("ac.required_checks",
                                     b.getArrayAttr({b.getDictionaryAttr({})}));
                   }});
  // clang-format off
  cases.push_back({"wrong-proof-filename", [&](auto &f) { f.proof->setLoc(mlir::FileLineColLoc::get(&context, "wrong.py", 8, 2)); }});
  cases.push_back({"owner-mismatch", [&](auto &f) { mlir::OpBuilder b(&context); f.hardware->setAttr("ac.source_owner", b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr("d1")), b.getNamedAttr("path", b.getStringAttr("other.py"))})); }});
  cases.push_back({"wrong-proof-definition", [&](auto &f) { mlir::OpBuilder b(&context); auto origin=f.proof.getOriginAttr(); auto site=origin.template getAs<mlir::DictionaryAttr>("site"); llvm::SmallVector<mlir::NamedAttribute> fields(site.begin(),site.end()); for(auto &field:fields) if(field.getName()=="definition") field=b.getNamedAttr("definition",mlir::FlatSymbolRefAttr::get(&context,"other.Unit")); f.proof->setAttr("origin",b.getDictionaryAttr({b.getNamedAttr("site",b.getDictionaryAttr(fields)),b.getNamedAttr("expansion",b.getArrayAttr({}))})); }});
  cases.push_back({"nonempty-origin-expansion", [&](auto &f) { mlir::OpBuilder b(&context); auto origin=f.proof.getOriginAttr(); f.proof->setAttr("origin",b.getDictionaryAttr({b.getNamedAttr("site",origin.get("site")),b.getNamedAttr("expansion",b.getArrayAttr({origin.get("site")}))})); }});
  // clang-format on
  for (auto &[name, mutate] : cases) {
    SCOPED_TRACE(name);
    auto f =
        build(context, name == std::string("altered-domain") ? "sub" : "lt",
              "-4", "4", "1");
    ASSERT_TRUE(verify(f).passed) << verify(f).diagnostic;
    mutate(f);
    EXPECT_FALSE(verify(f).passed);
  }
}

TEST_F(NumericExactScalarProofContractsTest,
       RejectsSwappedSubBadExtensionAndCapabilityProfiles) {
  {
    auto f = build(context, "sub", "0", "8", "1");
    ASSERT_TRUE(verify(f).passed);
    auto sub = mlir::cast<mlir::arith::SubIOp>(f.actual);
    mlir::Value lhs = sub.getLhs();
    sub->setOperand(0, sub.getRhs());
    sub->setOperand(1, lhs);
    EXPECT_FALSE(verify(f).passed) << "k-x is outside N0-D1";
  }
  {
    auto f = build(context, "sub", "-4", "4", "1");
    ASSERT_TRUE(verify(f).passed);
    ASSERT_NE(f.inputExt, nullptr);
    mlir::OpBuilder b(f.inputExt);
    auto bad = mlir::arith::ExtUIOp::create(b, f.inputExt->getLoc(),
                                            f.inputExt->getResult(0).getType(),
                                            f.read.getResult());
    f.actual->setOperand(0, bad.getResult());
    EXPECT_FALSE(verify(f).passed);
  }
  {
    auto f = build(context, "sub", "0", "8", "1");
    ASSERT_TRUE(verify(f).passed);
    mlir::OpBuilder b(f.resultBinding);
    auto truncated = mlir::arith::TruncIOp::create(
        b, f.resultBinding.getLoc(), b.getI1Type(), f.actual->getResult(0));
    auto narrow = intDomain(b, "0", "2", false);
    f.resultBinding->setOperand(0, truncated.getResult());
    f.resultBinding->setAttr("domain", narrow);
    f.proof->setOperand(3, truncated.getResult());
    f.proof->setAttr("result_domain", narrow);
    EXPECT_FALSE(verify(f).passed) << "premature result narrowing is forbidden";
  }
  {
    auto f = build(context, "eq", "0", "8", "1");
    mlir::OpBuilder b(&context);
    auto integerBool = intDomain(b, "0", "2", false);
    f.resultBinding->setAttr("domain", integerBool);
    f.proof->setAttr("result_domain", integerBool);
    EXPECT_FALSE(verify(f).passed) << "comparison result domain is source bool";
  }
  auto almostWideSub = build(context, "sub", "0", "18446744073709551615", "1");
  EXPECT_FALSE(verify(almostWideSub).passed);
  auto wideSub = build(context, "sub", "0", "18446744073709551616", "1");
  EXPECT_FALSE(verify(wideSub).passed)
      << "an exact i65 runtime intermediate is capability rejection";
  auto wideCompare = build(context, "lt", "0", "18446744073709551616", "-1");
  EXPECT_FALSE(verify(wideCompare).passed)
      << "comparison common representation must remain within 64 bits";
}

} // namespace
} // namespace acir::ac
