#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace acir::ac {
mlir::LogicalResult verifyCheckedToBitsWitness(RuleOp rule,
                                               ValueBindingOp result,
                                               NumericProofOp proof);
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
std::string plus(llvm::StringRef lhs, llvm::StringRef rhs) {
  return decimal(bits(lhs) + bits(rhs));
}
unsigned widthFor(llvm::StringRef lower, llvm::StringRef upper, bool sign) {
  llvm::APInt low = bits(lower), high = bits(plus(upper, "-1"));
  if (sign)
    return std::max(1u,
                    std::max(llvm::APSInt(low, false).getSignificantBits(),
                             llvm::APSInt(high, false).getSignificantBits()));
  return std::max(1u, high.getActiveBits());
}
MathIntAttr mathInt(mlir::MLIRContext &c, llvm::StringRef text) {
  bool negative = text.consume_front("-");
  llvm::APInt value(std::max<unsigned>(8, text.size() * 4 + 4), text, 10);
  if (negative)
    value = -value;
  return MathIntAttr::get(&c, llvm::APSInt(std::move(value), !negative));
}

// clang-format off
mlir::DictionaryAttr occurrence(mlir::OpBuilder &b,unsigned n){auto index=b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr("index")),b.getNamedAttr("value",b.getI64IntegerAttr(n))});auto site=b.getDictionaryAttr({b.getNamedAttr("definition",mlir::FlatSymbolRefAttr::get(b.getContext(),"range.Unit")),b.getNamedAttr("ast_path",b.getArrayAttr({index}))});return b.getDictionaryAttr({b.getNamedAttr("site",site),b.getNamedAttr("expansion",b.getArrayAttr({}))});}
mlir::DictionaryAttr id(mlir::OpBuilder &b,unsigned origin,unsigned slot){return b.getDictionaryAttr({b.getNamedAttr("origin",occurrence(b,origin)),b.getNamedAttr("slot",b.getI32IntegerAttr(slot))});}
mlir::DictionaryAttr domain(mlir::OpBuilder &b,llvm::StringRef lo,llvm::StringRef hi){bool s=bits(lo).isNegative();return b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr("integer")),b.getNamedAttr("storage",mlir::TypeAttr::get(b.getIntegerType(widthFor(lo,hi,s)))),b.getNamedAttr("lower",mathInt(*b.getContext(),lo)),b.getNamedAttr("upper",mathInt(*b.getContext(),hi)),b.getNamedAttr("interpretation",b.getStringAttr(s?"signed":"unsigned"))});}
mlir::DictionaryAttr ref(mlir::OpBuilder &b,llvm::StringRef kind,unsigned n){return b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr(kind)),b.getNamedAttr("index",b.getI32IntegerAttr(n))});}
mlir::DictionaryAttr node(mlir::OpBuilder &b,mlir::DictionaryAttr value,llvm::StringRef op,mlir::ArrayAttr operands,mlir::DictionaryAttr target){return b.getDictionaryAttr({b.getNamedAttr("id",value),b.getNamedAttr("operator",b.getStringAttr(op)),b.getNamedAttr("operands",operands),b.getNamedAttr("target",target)});}
mlir::DictionaryAttr formal(mlir::OpBuilder &b){return b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr("formal")),b.getNamedAttr("parameter",b.getStringAttr("source")),b.getNamedAttr("ordinal",b.getUnitAttr())});}
mlir::DictionaryAttr span(mlir::OpBuilder &b){return b.getDictionaryAttr({b.getNamedAttr("path",b.getStringAttr("range.py")),b.getNamedAttr("line",b.getI64IntegerAttr(7)),b.getNamedAttr("column",b.getI64IntegerAttr(3)),b.getNamedAttr("end_line",b.getI64IntegerAttr(7)),b.getNamedAttr("end_column",b.getI64IntegerAttr(12))});}
mlir::DictionaryAttr checkID(mlir::OpBuilder &b,mlir::DictionaryAttr registration,unsigned site,unsigned obligation=0){return b.getDictionaryAttr({b.getNamedAttr("registration",registration),b.getNamedAttr("check",occurrence(b,site)),b.getNamedAttr("obligation",b.getI64IntegerAttr(obligation))});}
// clang-format on

struct Fixture {
  mlir::OwningOpRef<mlir::ModuleOp> file;
  ModuleOp module;
  RuleOp rule;
  SourceReadOp read;
  SourceExpectOp expect;
  ValueBindingOp input, lifted, result;
  NumericProofOp proof;
  mlir::Value demand, safety, resultValid, converted;
  mlir::arith::CmpIOp lowerCmp, upperCmp;
  mlir::DictionaryAttr inputID, fromID, resultID, checkIdentity;
  mlir::DictionaryAttr inputDomain, resultDomain;
};

mlir::Value convert(mlir::OpBuilder &b, mlir::Location loc, mlir::Value value,
                    mlir::IntegerType target, bool inputSigned) {
  auto source = mlir::cast<mlir::IntegerType>(value.getType());
  if (source == target)
    return value;
  if (source.getWidth() > target.getWidth())
    return mlir::arith::TruncIOp::create(b, loc, target, value);
  return inputSigned
             ? mlir::Value(mlir::arith::ExtSIOp::create(b, loc, target, value))
             : mlir::Value(mlir::arith::ExtUIOp::create(b, loc, target, value));
}

Fixture build(mlir::MLIRContext &context, llvm::StringRef inputLower,
              llvm::StringRef inputUpper, llvm::StringRef lower,
              llvm::StringRef upper, bool path = true, bool valid = true) {
  mlir::OpBuilder b(&context);
  auto loc = mlir::FileLineColLoc::get(&context, "range.py", 7, 3);
  auto inputDomain = domain(b, inputLower, inputUpper);
  auto resultDomain = domain(b, lower, upper);
  auto inputID = id(b, 20, 0), fromID = id(b, 21, 3), resultID = id(b, 22, 7);
  auto none =
      b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("none"))});
  auto boundary = b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("integer_boundary")),
       b.getNamedAttr("domain", resultDomain)});
  auto obligations = b.getArrayAttr(
      {node(b, fromID, "from_bits", b.getArrayAttr({ref(b, "input", 0)}), none),
       node(b, resultID, "to_bits", b.getArrayAttr({ref(b, "node", 0)}),
            boundary)});
  auto owner = b.getDictionaryAttr(
      {b.getNamedAttr("package", b.getStringAttr("range")),
       b.getNamedAttr("path", b.getStringAttr("range.py"))});
  auto file = mlir::ModuleOp::create(loc);
  file->setAttr("ac.stage", b.getStringAttr("source"));
  file->setAttr("ac.unit_kind", b.getStringAttr("implementation"));
  file->setAttr("ac.source_owner", owner);
  b.setInsertionPointToStart(file.getBody());
  mlir::OperationState moduleState(loc, ModuleOp::getOperationName());
  moduleState.addAttribute("name", b.getStringAttr("Unit"));
  moduleState.addAttribute("sym_name", b.getStringAttr("range.Unit"));
  moduleState.addAttribute("ac.source_owner", owner);
  moduleState.addAttribute("ac.origin", occurrence(b, 1));
  auto port = b.getDictionaryAttr(
      {b.getNamedAttr("parameter", b.getStringAttr("source")),
       b.getNamedAttr("ordinal", b.getUnitAttr()),
       b.getNamedAttr("role", b.getStringAttr("current")),
       b.getNamedAttr("type", inputDomain),
       b.getNamedAttr("origin", occurrence(b, 2)),
       b.getNamedAttr("location", span(b))});
  moduleState.addAttribute("ac.ports", b.getArrayAttr({port}));
  moduleState.addAttribute(
      "ac.control_ports",
      b.getDictionaryAttr({b.getNamedAttr("clock", b.getI32IntegerAttr(0)),
                           b.getNamedAttr("reset", b.getI32IntegerAttr(1))}));
  moduleState.addRegion();
  auto module = mlir::cast<ModuleOp>(b.create(moduleState));
  auto *moduleBody = new mlir::Block();
  module.getBody().push_back(moduleBody);
  moduleBody->addArgument(b.getI1Type(), loc);
  moduleBody->addArgument(b.getI1Type(), loc);
  auto inputType =
      mlir::cast<mlir::TypeAttr>(inputDomain.get("storage")).getValue();
  moduleBody->addArgument(RegType::get(&context, inputType), loc);
  b.setInsertionPointToEnd(moduleBody);
  auto registration = occurrence(b, 10);
  auto checkIdentity = checkID(b, registration, 30);
  auto requiredCheck =
      b.getDictionaryAttr({b.getNamedAttr("id", checkIdentity),
                           b.getNamedAttr("kind", b.getStringAttr("range")),
                           b.getNamedAttr("location", span(b))});
  auto spec = b.getDictionaryAttr(
      {b.getNamedAttr("definition",
                      mlir::FlatSymbolRefAttr::get(&context, "range.Unit")),
       b.getNamedAttr("arguments", b.getArrayAttr({}))});
  mlir::OperationState ruleState(loc, RuleOp::getOperationName());
  ruleState.addOperands(moduleBody->getArgument(2));
  ruleState.addAttribute("name", b.getStringAttr("range"));
  ruleState.addAttribute("registration", registration);
  ruleState.addAttribute("operandSegmentSizes", b.getDenseI32ArrayAttr({1, 0}));
  ruleState.addAttribute("ac.source_owner", owner);
  ruleState.addAttribute("ac.origin", occurrence(b, 11));
  ruleState.addAttribute("ac.input_bindings", b.getArrayAttr({formal(b)}));
  ruleState.addAttribute("ac.output_bindings", b.getArrayAttr({}));
  ruleState.addAttribute("ac.input_types", b.getArrayAttr({inputDomain}));
  ruleState.addAttribute("ac.output_types", b.getArrayAttr({}));
  ruleState.addAttribute(
      "ac.proof_scope",
      b.getDictionaryAttr({b.getNamedAttr("specialization", spec),
                           b.getNamedAttr("registration", registration)}));
  ruleState.addAttribute("ac.required_numeric", obligations);
  ruleState.addAttribute("ac.required_checks", b.getArrayAttr({requiredCheck}));
  ruleState.addRegion();
  auto rule = mlir::cast<RuleOp>(b.create(ruleState));
  auto *body = new mlir::Block();
  rule.getBody().push_back(body);
  body->addArgument(inputType, loc);
  b.setInsertionPointToEnd(body);
  mlir::OperationState readState(loc, SourceReadOp::getOperationName());
  readState.addOperands(body->getArgument(0));
  readState.addTypes(inputType);
  readState.addAttribute("ac.origin", occurrence(b, 20));
  auto read = mlir::cast<SourceReadOp>(b.create(readState));
  auto p = mlir::arith::ConstantOp::create(b, loc, b.getI1Type(),
                                           b.getBoolAttr(path));
  auto v = mlir::arith::ConstantOp::create(b, loc, b.getI1Type(),
                                           b.getBoolAttr(valid));
  auto demand = mlir::arith::AndIOp::create(b, loc, p, v);
  auto sourceType = mlir::cast<mlir::IntegerType>(inputType);
  bool sourceSigned = bits(inputLower).isNegative();
  bool lowerAlways = lower == inputLower, upperAlways = upper == inputUpper;
  unsigned compareWidth =
      sourceType.getWidth() + (!sourceSigned && bits(lower).isNegative());
  if (!lowerAlways)
    compareWidth = std::max(compareWidth, widthFor(lower, plus(lower, "1"),
                                                   bits(lower).isNegative()));
  if (!upperAlways)
    compareWidth = std::max(compareWidth, widthFor(upper, plus(upper, "1"),
                                                   bits(upper).isNegative()));
  mlir::Value truth;
  if (lowerAlways || upperAlways)
    truth = mlir::arith::ConstantOp::create(b, loc, b.getI1Type(),
                                            b.getBoolAttr(true));
  auto compareType = b.getIntegerType(compareWidth);
  auto sourceValue =
      convert(b, loc, read.getResult(), compareType, sourceSigned);
  auto constant = [&](llvm::StringRef text) {
    return mlir::Value(mlir::arith::ConstantOp::create(
        b, loc, compareType,
        b.getIntegerAttr(compareType, bits(text, compareWidth))));
  };
  bool compareSigned = sourceSigned || bits(lower).isNegative();
  mlir::arith::CmpIOp lowerCmp, upperCmp;
  mlir::Value lowerSafe, upperSafe;
  if (!lowerAlways) {
    lowerCmp = mlir::arith::CmpIOp::create(
        b, loc,
        compareSigned ? mlir::arith::CmpIPredicate::sle
                      : mlir::arith::CmpIPredicate::ule,
        constant(lower), sourceValue);
    lowerSafe = lowerCmp;
  } else
    lowerSafe = truth;
  if (!upperAlways) {
    upperCmp = mlir::arith::CmpIOp::create(
        b, loc,
        compareSigned ? mlir::arith::CmpIPredicate::slt
                      : mlir::arith::CmpIPredicate::ult,
        sourceValue, constant(upper));
    upperSafe = upperCmp;
  } else
    upperSafe = truth;
  auto safety = mlir::arith::AndIOp::create(b, loc, lowerSafe, upperSafe);
  auto resultValid = mlir::arith::AndIOp::create(b, loc, demand, safety);
  auto resultType = mlir::cast<mlir::IntegerType>(
      resultDomain.getAs<mlir::TypeAttr>("storage").getValue());
  auto branch = mlir::scf::IfOp::create(b, loc, resultType, resultValid,
                                        /*withElseRegion=*/true);
  b.setInsertionPointToEnd(&branch.getThenRegion().front());
  auto converted = convert(b, loc, read.getResult(), resultType, sourceSigned);
  mlir::scf::YieldOp::create(b, loc, converted);
  b.setInsertionPointToEnd(&branch.getElseRegion().front());
  auto zero = mlir::arith::ConstantOp::create(b, loc, resultType,
                                              b.getIntegerAttr(resultType, 0));
  mlir::scf::YieldOp::create(b, loc, zero.getResult());
  b.setInsertionPointAfter(branch);
  mlir::OperationState expectState(loc, SourceExpectOp::getOperationName());
  expectState.addOperands({safety, demand});
  expectState.addAttribute("kind", b.getStringAttr("range"));
  expectState.addAttribute("location", span(b));
  expectState.addAttribute("ac.check_id", checkIdentity);
  auto expect = mlir::cast<SourceExpectOp>(b.create(expectState));
  auto bind = [&](mlir::Value value, mlir::Value valueValid,
                  mlir::Value valuePath, mlir::DictionaryAttr valueID,
                  mlir::DictionaryAttr valueDomain) {
    mlir::OperationState state(loc, ValueBindingOp::getOperationName());
    state.addOperands({value, valueValid, valuePath});
    state.addAttribute("id", valueID);
    state.addAttribute("domain", valueDomain);
    return mlir::cast<ValueBindingOp>(b.create(state));
  };
  auto input = bind(read.getResult(), v, p, inputID, inputDomain);
  auto lifted = bind(read.getResult(), v, p, fromID, inputDomain);
  auto result =
      bind(branch.getResult(0), resultValid, demand, resultID, resultDomain);
  auto checkBinding = b.getDictionaryAttr(
      {b.getNamedAttr("id", checkIdentity), b.getNamedAttr("owner", resultID),
       b.getNamedAttr("kind", b.getStringAttr("range")),
       b.getNamedAttr("operand_ordinal", b.getI32IntegerAttr(0))});
  mlir::OperationState proofState(loc, NumericProofOp::getOperationName());
  proofState.addOperands({demand, read.getResult(), v, branch.getResult(0),
                          resultValid, safety, demand});
  proofState.addAttribute("operand_segment_sizes",
                          b.getDenseI32ArrayAttr({1, 1, 1, 1, 1, 1, 1}));
  proofState.addAttribute("mode", b.getStringAttr("exact"));
  proofState.addAttribute("result_domain", resultDomain);
  proofState.addAttribute("input_ids", b.getArrayAttr({inputID}));
  proofState.addAttribute("input_domains", b.getArrayAttr({inputDomain}));
  proofState.addAttribute("result_id", resultID);
  proofState.addAttribute("obligations", obligations);
  proofState.addAttribute("checks", b.getArrayAttr({checkBinding}));
  proofState.addAttribute("origin", occurrence(b, 22));
  auto proof = mlir::cast<NumericProofOp>(b.create(proofState));
  YieldOp::create(b, loc, mlir::ValueRange{});
  b.setInsertionPointToEnd(moduleBody);
  YieldOp::create(b, loc, mlir::ValueRange{});
  return {std::move(file),
          module,
          rule,
          read,
          expect,
          input,
          lifted,
          result,
          proof,
          demand,
          safety,
          resultValid,
          branch.getResult(0),
          lowerCmp,
          upperCmp,
          inputID,
          fromID,
          resultID,
          checkIdentity,
          inputDomain,
          resultDomain};
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
  return {
      mlir::succeeded(verifyCheckedToBitsWitness(f.rule, f.result, f.proof)),
      diagnostic};
}

bool mathematicalSafety(int64_t value, int64_t lower, int64_t upper) {
  return lower <= value && value < upper;
}

class NumericRangeProofContractsTest : public ::testing::Test {
protected:
  NumericRangeProofContractsTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect,
                        mlir::scf::SCFDialect>();
  }
  mlir::MLIRContext context;
};

TEST_F(NumericRangeProofContractsTest, AcceptsDomainsAndEndpointOracle) {
  struct Case {
    const char *inputLower, *inputUpper, *lower, *upper;
  };
  for (const Case c : std::array<Case, 4>{
           {{"-16", "16", "-4", "5"},
            {"0", "16", "3", "12"},
            {"0", "16", "0", "16"},
            {"0", "18446744073709551616", "0", "18446744073709551616"}}}) {
    auto f = build(context, c.inputLower, c.inputUpper, c.lower, c.upper);
    EXPECT_TRUE(verify(f).passed) << verify(f).diagnostic;
    if (std::string(c.upper).size() < 10) {
      int64_t lower = std::stoll(c.lower), upper = std::stoll(c.upper);
      const std::array<int64_t, 4> values{lower - 1, lower, upper - 1, upper};
      const std::array<bool, 4> expected{false, true, true, false};
      for (auto [value, accepted] : llvm::zip(values, expected))
        EXPECT_EQ(mathematicalSafety(value, lower, upper), accepted);
    }
  }
}

TEST_F(NumericRangeProofContractsTest, DemandSafetyTruthTableIsExact) {
  for (bool path : {false, true})
    for (bool valid : {false, true}) {
      auto f = build(context, "0", "16", "4", "8", path, valid);
      ASSERT_TRUE(verify(f).passed) << verify(f).diagnostic;
      EXPECT_EQ(f.expect.getCondition(), f.safety);
      EXPECT_EQ(f.expect.getPath(), f.demand);
      auto validAnd = f.resultValid.getDefiningOp<mlir::arith::AndIOp>();
      ASSERT_TRUE(validAnd);
      EXPECT_EQ(validAnd.getLhs(), f.demand);
      EXPECT_EQ(validAnd.getRhs(), f.safety);
      bool demand = path && valid;
      for (int value : {3, 4, 7, 8}) {
        bool safety = mathematicalSafety(value, 4, 8);
        EXPECT_EQ(demand && safety, path && valid && 4 <= value && value < 8);
        if (!demand)
          EXPECT_FALSE(demand && !safety)
              << "inactive unsafe conversion must not report a check failure";
        if (demand && !safety) {
          EXPECT_TRUE(demand && !safety);
          EXPECT_FALSE(demand && safety)
              << "active unsafe conversion must make result invalid";
        }
      }
    }
}

TEST_F(NumericRangeProofContractsTest, RejectsSafetyAndCheckMutations) {
  using Mutation = std::function<void(Fixture &)>;
  std::vector<std::pair<const char *, Mutation>> cases;
  cases.push_back({"upper-inclusive", [](auto &f) {
                     f.upperCmp.setPredicate(mlir::arith::CmpIPredicate::ule);
                   }});
  cases.push_back({"wrong-signedness", [](auto &f) {
                     f.lowerCmp.setPredicate(mlir::arith::CmpIPredicate::ule);
                     f.upperCmp.setPredicate(mlir::arith::CmpIPredicate::ult);
                   }});
  cases.push_back({"result-valid-as-check-path", [](auto &f) {
                     f.expect->setOperand(1, f.resultValid);
                     f.proof->setOperand(6, f.resultValid);
                   }});
  cases.push_back({"missing-check-binding", [](auto &f) {
                     f.proof->setAttr("checks", mlir::ArrayAttr::get(
                                                    f.rule.getContext(), {}));
                   }});
  cases.push_back({"malformed-check-binding", [&](auto &f) {
                     mlir::OpBuilder b(&context);
                     f.proof->setAttr(
                         "checks", b.getArrayAttr({b.getDictionaryAttr({})}));
                   }});
  cases.push_back({"duplicate-check-binding", [&](auto &f) {
                     mlir::OpBuilder b(&context);
                     auto check = f.proof.getChecksAttr()[0];
                     f.proof->setAttr("checks", b.getArrayAttr({check, check}));
                   }});
  cases.push_back(
      {"stale-check-binding", [&](auto &f) {
         mlir::OpBuilder b(&context);
         auto check =
             mlir::cast<mlir::DictionaryAttr>(f.proof.getChecksAttr()[0]);
         llvm::SmallVector<mlir::NamedAttribute> fields(check.begin(),
                                                        check.end());
         for (auto &field : fields)
           if (field.getName() == "id")
             field = b.getNamedAttr(
                 "id", checkID(b, f.rule.getRegistrationAttr(), 99));
         f.proof->setAttr("checks",
                          b.getArrayAttr({b.getDictionaryAttr(fields)}));
       }});
  cases.push_back({"missing-expect", [](auto &f) { f.expect.erase(); }});
  cases.push_back({"duplicate-expect", [](auto &f) {
                     f.expect->getBlock()->getOperations().insert(
                         f.expect->getIterator(), f.expect->clone());
                   }});
  cases.push_back({"stale-check-id", [&](auto &f) {
                     mlir::OpBuilder b(&context);
                     f.expect->setAttr(
                         "ac.check_id",
                         checkID(b, f.rule.getRegistrationAttr(), 99));
                   }});
  for (auto &[name, mutate] : cases) {
    SCOPED_TRACE(name);
    auto f = build(context, "-16", "16", "-4", "5");
    ASSERT_TRUE(verify(f).passed) << verify(f).diagnostic;
    mutate(f);
    EXPECT_FALSE(verify(f).passed);
  }
}

TEST_F(NumericRangeProofContractsTest,
       RejectsTruncatedEvidenceCrossRuleAndSequentialChecks) {
  {
    auto f = build(context, "-16", "16", "-4", "5");
    mlir::OpBuilder b(f.lowerCmp);
    auto truncated = mlir::arith::TruncIOp::create(
        b, f.lowerCmp.getLoc(), b.getI1Type(), f.read.getResult());
    auto widened = mlir::arith::ExtSIOp::create(
        b, f.lowerCmp.getLoc(), f.lowerCmp.getLhs().getType(), truncated);
    f.lowerCmp->setOperand(1, widened);
    f.upperCmp->setOperand(0, widened);
    EXPECT_FALSE(verify(f).passed) << "checking post-truncation is forbidden";
  }
  {
    auto f = build(context, "0", "16", "4", "8");
    auto other = mlir::cast<RuleOp>(f.rule->clone());
    auto &moduleBody = f.module.getBody().front();
    moduleBody.getOperations().insert(moduleBody.getTerminator()->getIterator(),
                                      other);
    f.expect->moveBefore(
        &other.getBody().front(),
        other.getBody().front().getTerminator()->getIterator());
    EXPECT_FALSE(verify(f).passed);
  }
  {
    auto f = build(context, "0", "16", "4", "8");
    f.expect->getBlock()->getOperations().insert(f.expect->getIterator(),
                                                 f.expect->clone());
    EXPECT_FALSE(verify(f).passed)
        << "two sequential checks remain unsupported";
  }
}

TEST_F(NumericRangeProofContractsTest, RejectsGuardedConversionMutations) {
  {
    auto f = build(context, "-16", "16", "-4", "5");
    ASSERT_TRUE(verify(f).passed) << verify(f).diagnostic;
    auto branch = f.converted.getDefiningOp<mlir::scf::IfOp>();
    auto yield =
        *branch.getThenRegion().front().getOps<mlir::scf::YieldOp>().begin();
    auto inside = yield.getOperand(0).getDefiningOp<mlir::arith::TruncIOp>();
    ASSERT_TRUE(inside);
    mlir::OpBuilder b(branch);
    auto outside = mlir::arith::TruncIOp::create(
        b, branch.getLoc(), inside.getType(), f.read.getResult());
    yield->setOperand(0, outside);
    inside.erase();
    EXPECT_FALSE(verify(f).passed)
        << "conversion cannot be hoisted from scf.if";
  }
  for (bool thenRegion : {true, false}) {
    auto f = build(context, "-16", "16", "-4", "5");
    ASSERT_TRUE(verify(f).passed) << verify(f).diagnostic;
    auto branch = f.converted.getDefiningOp<mlir::scf::IfOp>();
    auto &block = thenRegion ? branch.getThenRegion().front()
                             : branch.getElseRegion().front();
    auto yield = block.getTerminator();
    mlir::OpBuilder b(yield);
    (void)mlir::arith::ConstantOp::create(b, branch.getLoc(), b.getI1Type(),
                                          b.getBoolAttr(true));
    EXPECT_FALSE(verify(f).passed)
        << (thenRegion ? "extra then op" : "extra else op");
  }
  {
    auto f = build(context, "-16", "16", "-4", "5");
    ASSERT_TRUE(verify(f).passed) << verify(f).diagnostic;
    auto branch = f.converted.getDefiningOp<mlir::scf::IfOp>();
    mlir::OpBuilder b(branch);
    auto duplicate =
        mlir::arith::AndIOp::create(b, branch.getLoc(), f.demand, f.safety);
    branch.getConditionMutable().assign(duplicate);
    EXPECT_FALSE(verify(f).passed)
        << "branch must consume the unique result-valid SSA";
  }
}

TEST_F(NumericRangeProofContractsTest, RejectsProofProvenanceMutations) {
  using Mutation = std::function<void(Fixture &)>;
  std::vector<Mutation> cases;
  // clang-format off
  cases.push_back([&](auto &f){f.proof->setLoc(mlir::FileLineColLoc::get(&context,"wrong.py",7,3));});
  cases.push_back([&](auto &f){mlir::OpBuilder b(&context);f.module->setAttr("ac.source_owner",b.getDictionaryAttr({b.getNamedAttr("package",b.getStringAttr("range")),b.getNamedAttr("path",b.getStringAttr("other.py"))}));});
  cases.push_back([&](auto &f){mlir::OpBuilder b(&context);auto origin=f.proof.getOriginAttr();auto site=origin.template getAs<mlir::DictionaryAttr>("site");llvm::SmallVector<mlir::NamedAttribute> fields(site.begin(),site.end());for(auto &field:fields)if(field.getName()=="definition")field=b.getNamedAttr("definition",mlir::FlatSymbolRefAttr::get(&context,"other.Unit"));f.proof->setAttr("origin",b.getDictionaryAttr({b.getNamedAttr("site",b.getDictionaryAttr(fields)),b.getNamedAttr("expansion",b.getArrayAttr({}))}));});
  cases.push_back([&](auto &f){mlir::OpBuilder b(&context);auto origin=f.proof.getOriginAttr();auto frame=b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr("call")),b.getNamedAttr("site",origin.get("site")),b.getNamedAttr("callee",mlir::FlatSymbolRefAttr::get(&context,"range.Helper"))});f.proof->setAttr("origin",b.getDictionaryAttr({b.getNamedAttr("site",origin.get("site")),b.getNamedAttr("expansion",b.getArrayAttr({frame}))}));});
  // clang-format on
  for (auto &mutate : cases) {
    auto f = build(context, "0", "16", "4", "8");
    ASSERT_TRUE(verify(f).passed) << verify(f).diagnostic;
    mutate(f);
    EXPECT_FALSE(verify(f).passed);
  }
}

} // namespace
} // namespace acir::ac
