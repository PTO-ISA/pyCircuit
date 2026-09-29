#include "Compiler/ScalarNumericLowering.h"
#include "NumericFiniteSSAOracle.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace acir::compiler {
namespace {

using namespace acir::ac;
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

std::string addDecimal(llvm::StringRef lhs, llvm::StringRef rhs) {
  return decimal(integerBits(lhs) + integerBits(rhs));
}

std::vector<std::string> representativeValues(llvm::StringRef lower,
                                              llvm::StringRef upper) {
  if (lower.size() < 10 && upper.size() < 10) {
    const int64_t first = std::stoll(lower.str());
    const int64_t end = std::stoll(upper.str());
    if (end - first <= 32) {
      std::vector<std::string> values;
      for (int64_t value = first; value < end; ++value)
        values.push_back(std::to_string(value));
      return values;
    }
  }
  return {lower.str(), addDecimal(lower, "1"), addDecimal(upper, "-1")};
}

unsigned intervalWidth(llvm::StringRef lower, llvm::StringRef upper,
                       bool isSigned) {
  std::string last = addDecimal(upper, "-1");
  auto signedWidth = [](llvm::StringRef value) {
    llvm::APSInt parsed(integerBits(value), false);
    return std::max(1u, parsed.getSignificantBits());
  };
  return isSigned ? std::max(signedWidth(lower), signedWidth(last))
                  : std::max(1u, integerBits(last).getActiveBits());
}

MathIntAttr mathInteger(mlir::MLIRContext &context, llvm::StringRef text) {
  unsigned width = static_cast<unsigned>(text.size() * 4 + 8);
  bool negative = text.consume_front("-");
  llvm::APInt value(width, text, 10);
  if (negative)
    value = -value;
  return MathIntAttr::get(&context, llvm::APSInt(std::move(value), !negative));
}

// clang-format off
mlir::DictionaryAttr occurrence(mlir::OpBuilder &b, unsigned line) {
  auto index = b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("index")),
                                    b.getNamedAttr("value", b.getI64IntegerAttr(line))});
  auto site = b.getDictionaryAttr({b.getNamedAttr("definition", mlir::FlatSymbolRefAttr::get(b.getContext(), "demo.Compute")),
                                   b.getNamedAttr("ast_path", b.getArrayAttr({index}))});
  return b.getDictionaryAttr({b.getNamedAttr("site", site), b.getNamedAttr("expansion", b.getArrayAttr({}))});
}
mlir::DictionaryAttr valueID(mlir::OpBuilder &b, mlir::DictionaryAttr origin, unsigned slot = 0) {
  return b.getDictionaryAttr({b.getNamedAttr("origin", origin), b.getNamedAttr("slot", b.getI32IntegerAttr(slot))});
}
mlir::DictionaryAttr integerDomain(mlir::OpBuilder &b, llvm::StringRef lower,
                                   llvm::StringRef upper) {
  const bool sign = integerBits(lower).isNegative();
  return b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("integer")),
      b.getNamedAttr("storage", mlir::TypeAttr::get(b.getIntegerType(intervalWidth(lower, upper, sign)))),
      b.getNamedAttr("lower", mathInteger(*b.getContext(), lower)), b.getNamedAttr("upper", mathInteger(*b.getContext(), upper)),
      b.getNamedAttr("interpretation", b.getStringAttr(sign ? "signed" : "unsigned"))});
}
mlir::DictionaryAttr inputRef(mlir::OpBuilder &b) {
  return b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("input")), b.getNamedAttr("index", b.getI32IntegerAttr(0))});
}
mlir::DictionaryAttr constantRef(mlir::OpBuilder &b, llvm::StringRef value) {
  return b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("constant")), b.getNamedAttr("value", mathInteger(*b.getContext(), value))});
}
mlir::DictionaryAttr nodeRef(mlir::OpBuilder &b, unsigned index) {
  return b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("node")), b.getNamedAttr("index", b.getI32IntegerAttr(index))});
}
mlir::DictionaryAttr formalInput(mlir::OpBuilder &b) {
  return b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("formal")),
      b.getNamedAttr("parameter", b.getStringAttr("source")), b.getNamedAttr("ordinal", b.getUnitAttr())});
}
mlir::DictionaryAttr currentPort(mlir::OpBuilder &b, mlir::DictionaryAttr domain, mlir::DictionaryAttr origin) {
  auto location = b.getDictionaryAttr({b.getNamedAttr("path", b.getStringAttr("demo.py")),
      b.getNamedAttr("line", b.getI64IntegerAttr(4)), b.getNamedAttr("column", b.getI64IntegerAttr(3)),
      b.getNamedAttr("end_line", b.getI64IntegerAttr(4)), b.getNamedAttr("end_column", b.getI64IntegerAttr(8))});
  return b.getDictionaryAttr({b.getNamedAttr("parameter", b.getStringAttr("source")), b.getNamedAttr("ordinal", b.getUnitAttr()),
      b.getNamedAttr("role", b.getStringAttr("current")), b.getNamedAttr("type", domain),
      b.getNamedAttr("origin", origin), b.getNamedAttr("location", location)});
}
// clang-format on

struct SourceNumericFixture {
  mlir::OwningOpRef<mlir::ModuleOp> file;
  ModuleOp module;
  RuleOp rule;
  SourceReadOp read;
  MathFromBitsOp fromBits;
  MathConstantOp constant;
  MathBinaryOp add;
  mlir::arith::ConstantOp truth;
  mlir::ArrayAttr nodes;
  mlir::DictionaryAttr inputDomain, inputID, fromID, constantID, addID;
};

SourceNumericFixture buildSourceFixture(mlir::MLIRContext &context,
                                        llvm::StringRef lower = "0",
                                        llvm::StringRef upper = "8",
                                        llvm::StringRef k = "1") {
  mlir::OpBuilder b(&context);
  auto loc = mlir::FileLineColLoc::get(&context, "demo.py", 7, 3);
  auto owner = b.getDictionaryAttr({
      b.getNamedAttr("package", b.getStringAttr("demo")),
      b.getNamedAttr("path", b.getStringAttr("demo.py")),
  });
  auto moduleOrigin = occurrence(b, 1), readOrigin = occurrence(b, 7);
  auto fromOrigin = occurrence(b, 8), constantOrigin = occurrence(b, 9);
  auto addOrigin = occurrence(b, 10);
  auto inputDomain = integerDomain(b, lower, upper);
  auto inputID = valueID(b, readOrigin);
  auto fromID = valueID(b, fromOrigin, 3);
  auto constantID = valueID(b, constantOrigin, 5);
  auto addID = valueID(b, addOrigin, 7);
  auto none = b.getDictionaryAttr({
      b.getNamedAttr("kind", b.getStringAttr("none")),
  });
  auto fromNode = b.getDictionaryAttr({
      b.getNamedAttr("id", fromID),
      b.getNamedAttr("operator", b.getStringAttr("from_bits")),
      b.getNamedAttr("operands", b.getArrayAttr({inputRef(b)})),
      b.getNamedAttr("target", none),
  });
  auto constantNode = b.getDictionaryAttr({
      b.getNamedAttr("id", constantID),
      b.getNamedAttr("operator", b.getStringAttr("constant")),
      b.getNamedAttr("operands", b.getArrayAttr({constantRef(b, k)})),
      b.getNamedAttr("target", none),
  });
  auto addNode = b.getDictionaryAttr({
      b.getNamedAttr("id", addID),
      b.getNamedAttr("operator", b.getStringAttr("add")),
      b.getNamedAttr("operands",
                     b.getArrayAttr({nodeRef(b, 0), nodeRef(b, 1)})),
      b.getNamedAttr("target", none),
  });
  auto nodes = b.getArrayAttr({fromNode, constantNode, addNode});
  auto file = mlir::ModuleOp::create(loc);
  file->setAttr("ac.stage", b.getStringAttr("source"));
  file->setAttr("ac.unit_kind", b.getStringAttr("implementation"));
  file->setAttr("ac.source_owner", owner);
  b.setInsertionPointToStart(file.getBody());
  mlir::OperationState moduleState(loc, ModuleOp::getOperationName());
  moduleState.addAttribute("name", b.getStringAttr("Compute"));
  moduleState.addAttribute("sym_name", b.getStringAttr("demo.Compute"));
  moduleState.addAttribute("ac.source_owner", owner);
  moduleState.addAttribute("ac.origin", moduleOrigin);
  moduleState.addAttribute(
      "ac.ports",
      b.getArrayAttr({currentPort(b, inputDomain, occurrence(b, 6))}));
  moduleState.addAttribute("ac.control_ports",
                           b.getDictionaryAttr({
                               b.getNamedAttr("clock", b.getI32IntegerAttr(0)),
                               b.getNamedAttr("reset", b.getI32IntegerAttr(1)),
                           }));
  moduleState.addRegion();
  auto module = mlir::cast<ModuleOp>(b.create(moduleState));
  auto *moduleBody = new mlir::Block();
  module.getBody().push_back(moduleBody);
  moduleBody->addArgument(b.getI1Type(), loc);
  moduleBody->addArgument(b.getI1Type(), loc);
  auto inputType = cast<mlir::TypeAttr>(inputDomain.get("storage")).getValue();
  moduleBody->addArgument(RegType::get(&context, inputType), loc);
  b.setInsertionPointToEnd(moduleBody);
  auto spec = b.getDictionaryAttr({
      b.getNamedAttr("definition",
                     mlir::FlatSymbolRefAttr::get(&context, "demo.Compute")),
      b.getNamedAttr("arguments", b.getArrayAttr({})),
  });
  auto registration = occurrence(b, 2);
  auto proofScope = b.getDictionaryAttr({
      b.getNamedAttr("specialization", spec),
      b.getNamedAttr("registration", registration),
  });
  mlir::OperationState ruleState(loc, RuleOp::getOperationName());
  ruleState.addOperands(moduleBody->getArgument(2));
  ruleState.addAttribute("name", b.getStringAttr("compute"));
  ruleState.addAttribute("registration", registration);
  ruleState.addAttribute("operandSegmentSizes", b.getDenseI32ArrayAttr({1, 0}));
  ruleState.addAttribute("ac.source_owner", owner);
  ruleState.addAttribute("ac.origin", occurrence(b, 3));
  ruleState.addAttribute("ac.input_bindings", b.getArrayAttr({formalInput(b)}));
  ruleState.addAttribute("ac.output_bindings", b.getArrayAttr({}));
  ruleState.addAttribute("ac.input_types", b.getArrayAttr({inputDomain}));
  ruleState.addAttribute("ac.output_types", b.getArrayAttr({}));
  ruleState.addAttribute("ac.proof_scope", proofScope);
  ruleState.addAttribute("ac.required_numeric", nodes);
  ruleState.addAttribute("ac.required_checks", b.getArrayAttr({}));
  ruleState.addRegion();
  auto rule = mlir::cast<RuleOp>(b.create(ruleState));
  auto *body = new mlir::Block();
  rule.getBody().push_back(body);
  body->addArgument(inputType, loc);
  b.setInsertionPointToEnd(body);
  mlir::OperationState readState(loc, SourceReadOp::getOperationName());
  readState.addOperands(body->getArgument(0));
  readState.addTypes(inputType);
  readState.addAttribute("ac.origin", readOrigin);
  auto read = mlir::cast<SourceReadOp>(b.create(readState));
  auto truth = mlir::arith::ConstantOp::create(b, loc, b.getI1Type(),
                                               b.getBoolAttr(true));
  auto from = mlir::cast<MathFromBitsOp>([&] {
    mlir::OperationState state(loc, MathFromBitsOp::getOperationName());
    state.addOperands(read.getResult());
    state.addAttribute("domain", inputDomain);
    state.addAttribute("ac.origin", fromOrigin);
    state.addTypes(MathIntType::get(&context));
    return b.create(state);
  }());
  auto constant = mlir::cast<MathConstantOp>([&] {
    mlir::OperationState state(loc, MathConstantOp::getOperationName());
    state.addAttribute("value", mathInteger(context, k));
    state.addAttribute("ac.origin", constantOrigin);
    state.addTypes(MathIntType::get(&context));
    return b.create(state);
  }());
  mlir::OperationState addState(loc, MathBinaryOp::getOperationName());
  addState.addOperands({truth.getResult(), from.getResult(), truth.getResult(),
                        constant.getResult(), truth.getResult()});
  addState.addAttribute("operator", b.getStringAttr("add"));
  addState.addAttribute("ac.origin", addOrigin);
  addState.addTypes({MathIntType::get(&context), b.getI1Type()});
  auto add = mlir::cast<MathBinaryOp>(b.create(addState));
  YieldOp::create(b, loc, mlir::ValueRange{});
  b.setInsertionPointToEnd(moduleBody);
  YieldOp::create(b, loc, mlir::ValueRange{});
  return {std::move(file), module, rule,       read,  from,
          constant,        add,    truth,      nodes, inputDomain,
          inputID,         fromID, constantID, addID};
}

std::string print(mlir::Operation *op) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  op->print(stream);
  return text;
}

bool hasMathCarrier(mlir::Operation *root) {
  bool found = false;
  root->walk([&](mlir::Operation *op) {
    found |= mlir::isa<MathFromBitsOp, MathConstantOp, MathBinaryOp>(op);
    for (mlir::Type type : op->getResultTypes())
      found |= mlir::isa<MathIntType>(type);
  });
  return found;
}

NumericProofOp onlyProof(mlir::Operation *root) {
  NumericProofOp proof;
  root->walk([&](NumericProofOp op) {
    EXPECT_FALSE(proof);
    proof = op;
  });
  return proof;
}

ValueBindingOp bindingFor(mlir::Operation *root, mlir::DictionaryAttr id) {
  ValueBindingOp result;
  root->walk([&](ValueBindingOp binding) {
    if (binding.getIdAttr() == id)
      result = binding;
  });
  return result;
}

RuleOp appendClonedRule(SourceNumericFixture &fixture,
                        mlir::MLIRContext &context, unsigned line,
                        llvm::StringRef name) {
  auto second = mlir::cast<RuleOp>(fixture.rule->clone());
  mlir::OpBuilder builder(&context);
  auto registration = occurrence(builder, line);
  second->setAttr("registration", registration);
  auto scope = builder.getDictionaryAttr({
      builder.getNamedAttr("specialization",
                           mlir::cast<mlir::DictionaryAttr>(
                               fixture.rule->getAttr("ac.proof_scope"))
                               .get("specialization")),
      builder.getNamedAttr("registration", registration),
  });
  second->setAttr("ac.proof_scope", scope);
  second->setAttr("name", builder.getStringAttr(name));
  auto &moduleBody = fixture.module.getBody().front();
  moduleBody.getOperations().insert(moduleBody.getTerminator()->getIterator(),
                                    second.getOperation());
  return second;
}

void splitTrueControls(SourceNumericFixture &fixture, unsigned controls) {
  mlir::OpBuilder builder(fixture.add);
  auto makeTrue = [&] {
    return mlir::arith::ConstantOp::create(builder, fixture.add.getLoc(),
                                           builder.getI1Type(),
                                           builder.getBoolAttr(true));
  };
  auto shared = makeTrue();
  fixture.add->setOperand(2, shared);
  fixture.add->setOperand(4, controls == 2 ? shared.getResult()
                                           : makeTrue().getResult());
}

// clang-format off
void appendB0Witness(SourceNumericFixture &fixture, mlir::MLIRContext &context) {
  mlir::OpBuilder b(&context);
  auto loc = fixture.rule.getLoc(); auto origin = occurrence(b, 30);
  auto domain = integerDomain(b, "1", "2"), id = valueID(b, origin, 9);
  auto none = b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("none"))});
  auto node = b.getDictionaryAttr({b.getNamedAttr("id", id), b.getNamedAttr("operator", b.getStringAttr("constant")),
      b.getNamedAttr("operands", b.getArrayAttr({constantRef(b, "1")})), b.getNamedAttr("target", none)});
  auto registration = occurrence(b, 31);
  auto sourceScope = fixture.rule->getAttrOfType<mlir::DictionaryAttr>("ac.proof_scope");
  auto scope = b.getDictionaryAttr({b.getNamedAttr("specialization", sourceScope.get("specialization")),
                                    b.getNamedAttr("registration", registration)});
  mlir::OperationState state(loc, RuleOp::getOperationName());
  state.addAttribute("name", b.getStringAttr("existing_b0")); state.addAttribute("registration", registration);
  state.addAttribute("operandSegmentSizes", b.getDenseI32ArrayAttr({0, 0})); state.addAttribute("ac.source_owner", fixture.rule->getAttr("ac.source_owner"));
  state.addAttribute("ac.origin", origin); for (llvm::StringRef name : {"ac.input_bindings", "ac.output_bindings", "ac.input_types", "ac.output_types"}) state.addAttribute(name, b.getArrayAttr({}));
  state.addAttribute("ac.proof_scope", scope); state.addAttribute("ac.required_numeric", b.getArrayAttr({node}));
  state.addRegion();
  b.setInsertionPoint(fixture.module.getBody().front().getTerminator()); auto rule = mlir::cast<RuleOp>(b.create(state));
  rule.getBody().push_back(new mlir::Block());
  b.setInsertionPointToEnd(&rule.getBody().front());
  auto value = mlir::arith::ConstantOp::create(b, loc, b.getI1Type(), b.getBoolAttr(true));
  auto truth = mlir::arith::ConstantOp::create(b, loc, b.getI1Type(), b.getBoolAttr(true));
  mlir::OperationState binding(loc, ValueBindingOp::getOperationName());
  binding.addOperands({value.getResult(), truth.getResult(), truth.getResult()}); binding.addAttribute("id", id); binding.addAttribute("domain", domain); b.create(binding);
  mlir::OperationState proof(loc, NumericProofOp::getOperationName());
  proof.addOperands({truth.getResult(), value.getResult(), truth.getResult()});
  proof.addAttribute("operand_segment_sizes", b.getDenseI32ArrayAttr({1, 0, 0, 1, 1, 0, 0})); proof.addAttribute("mode", b.getStringAttr("exact"));
  proof.addAttribute("result_domain", domain); proof.addAttribute("input_ids", b.getArrayAttr({})); proof.addAttribute("input_domains", b.getArrayAttr({}));
  proof.addAttribute("result_id", id); proof.addAttribute("obligations", b.getArrayAttr({node})); proof.addAttribute("checks", b.getArrayAttr({})); proof.addAttribute("origin", origin);
  b.create(proof); YieldOp::create(b, loc, mlir::ValueRange{});
}
// clang-format on

void setRuleConstant(RuleOp rule, mlir::MLIRContext &context,
                     llvm::StringRef value) {
  auto constant = *rule.getBody().front().getOps<MathConstantOp>().begin();
  constant->setAttr("value", mathInteger(context, value));
  auto nodes = rule->getAttrOfType<mlir::ArrayAttr>("ac.required_numeric");
  auto node = mlir::cast<mlir::DictionaryAttr>(nodes[1]);
  llvm::SmallVector<mlir::NamedAttribute> fields(node.begin(), node.end());
  for (auto &field : fields) {
    if (field.getName() != "operands")
      continue;
    mlir::OpBuilder builder(&context);
    field = mlir::NamedAttribute(
        field.getName(), builder.getArrayAttr({constantRef(builder, value)}));
  }
  llvm::SmallVector<mlir::Attribute> changed(nodes.begin(), nodes.end());
  changed[1] = mlir::DictionaryAttr::get(&context, fields);
  rule->setAttr("ac.required_numeric", mlir::ArrayAttr::get(&context, changed));
}

class ScalarNumericLoweringTest : public ::testing::Test {
protected:
  ScalarNumericLoweringTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect>();
  }
  mlir::MLIRContext context;
};

TEST_F(ScalarNumericLoweringTest, TransactionalHelperLowersAndIsIdempotent) {
  auto fixture = buildSourceFixture(context);
  auto *root = fixture.file->getOperation();
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*fixture.file)));
  ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*fixture.file)));
  EXPECT_EQ(fixture.file->getOperation(), root);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*fixture.file)));
  EXPECT_FALSE(hasMathCarrier(fixture.file->getOperation()));
  auto proof = onlyProof(fixture.file->getOperation());
  ASSERT_TRUE(proof);
  EXPECT_EQ(proof.getObligationsAttr(), fixture.nodes);
  EXPECT_EQ(proof.getInputIdsAttr().size(), 1u);
  EXPECT_EQ(proof.getInputIdsAttr()[0], fixture.inputID);
  EXPECT_EQ(proof.getInputDomainsAttr()[0], fixture.inputDomain);
  EXPECT_EQ(proof.getResultIdAttr(), fixture.addID);
  auto i = bindingFor(fixture.file->getOperation(), fixture.inputID);
  auto f = bindingFor(fixture.file->getOperation(), fixture.fromID);
  auto c = bindingFor(fixture.file->getOperation(), fixture.constantID);
  auto s = bindingFor(fixture.file->getOperation(), fixture.addID);
  ASSERT_TRUE(i && f && c && s);
  auto module = *fixture.file->getOps<ModuleOp>().begin();
  auto rule = *module.getBody().front().getOps<RuleOp>().begin();
  auto read = *rule.getBody().front().getOps<SourceReadOp>().begin();
  EXPECT_EQ(read->getAttrOfType<mlir::DictionaryAttr>("ac.origin"),
            fixture.inputID.getAs<mlir::DictionaryAttr>("origin"));
  EXPECT_EQ(i.getValue(), read.getResult());
  EXPECT_EQ(f.getValue(), read.getResult());
  EXPECT_EQ(i.getValue(), f.getValue());
  auto add = s.getValue().getDefiningOp<mlir::arith::AddIOp>();
  ASSERT_TRUE(add);
  EXPECT_EQ(proof->getOperand(3), add.getResult());
  auto first = print(fixture.file->getOperation());
  ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*fixture.file)));
  EXPECT_EQ(print(fixture.file->getOperation()), first);
}

TEST_F(ScalarNumericLoweringTest, RemovesOnlyDeadSplitTrueControls) {
  for (unsigned controls : {2u, 3u}) {
    auto fixture = buildSourceFixture(context);
    splitTrueControls(fixture, controls);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*fixture.file)));
    ASSERT_TRUE(
        mlir::succeeded(lowerExactInputAddTransactional(*fixture.file)));
    fixture.file->walk([&](mlir::arith::ConstantOp op) {
      EXPECT_FALSE(op.getResult().use_empty());
    });
  }
}

TEST_F(ScalarNumericLoweringTest, RegisteredPassLowersFrozenSourceRecipe) {
  registerACIRScalarNumericPasses();
  auto fixture = buildSourceFixture(context, "0", "8");
  mlir::PassManager manager(&context);
  ASSERT_TRUE(mlir::succeeded(
      mlir::parsePassPipeline("ac-lower-exact-input-add", manager)));
  ASSERT_TRUE(mlir::succeeded(manager.run(*fixture.file)));
  EXPECT_FALSE(hasMathCarrier(fixture.file->getOperation()));
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*fixture.file)));
}

TEST_F(ScalarNumericLoweringTest, SourceNoOpUnitRemainsByteEquivalent) {
  mlir::OpBuilder builder(&context);
  auto unit = mlir::ModuleOp::create(builder.getUnknownLoc());
  unit->setAttr("ac.stage", builder.getStringAttr("source"));
  unit->setAttr("ac.unit_kind", builder.getStringAttr("implementation"));
  const auto before = print(unit.getOperation());
  ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(unit)));
  EXPECT_EQ(print(unit.getOperation()), before);
}

TEST_F(ScalarNumericLoweringTest,
       AcceptedIntervalsMatchIndependentFiniteSSAOracle) {
  struct Case {
    const char *lower, *upper;
  };
  constexpr std::array<Case, 6> cases = {{{"0", "7"},
                                          {"0", "8"},
                                          {"-4", "0"},
                                          {"-1", "1"},
                                          {"0", "2"},
                                          {"0", "18446744073709551615"}}};
  for (const Case c : cases) {
    SCOPED_TRACE(std::string("[") + c.lower + "," + c.upper + ")");
    auto fixture = buildSourceFixture(context, c.lower, c.upper);
    ASSERT_TRUE(
        mlir::succeeded(lowerExactInputAddTransactional(*fixture.file)));
    auto input = bindingFor(fixture.file->getOperation(), fixture.inputID);
    auto result = bindingFor(fixture.file->getOperation(), fixture.addID);
    ASSERT_TRUE(input && result);
    auto proof = onlyProof(fixture.file->getOperation());
    ASSERT_TRUE(proof);
    auto module = *fixture.file->getOps<ModuleOp>().begin();
    auto rule = *module.getBody().front().getOps<RuleOp>().begin();
    auto sourceRead = *rule.getBody().front().getOps<SourceReadOp>().begin();
    for (const std::string &sample : representativeValues(c.lower, c.upper)) {
      auto source = integerBits(
          sample,
          mlir::cast<mlir::IntegerType>(input.getValue().getType()).getWidth());
      auto evaluated = acir::ac::test::evaluateFiniteSSA(
          result.getValue(), sourceRead.getResult(), source);
      ASSERT_TRUE(evaluated);
      EXPECT_EQ(*evaluated,
                integerBits(addDecimal(sample, "1"), evaluated->getBitWidth()));
    }
    EXPECT_EQ(proof->getOperand(3), result.getValue());
  }
}

TEST_F(ScalarNumericLoweringTest, RejectsSourceInventoryAndRecipeMutations) {
  using Mutation = std::function<void(SourceNumericFixture &)>;
  std::vector<std::pair<std::string, Mutation>> mutations;
  // clang-format off
  mutations.push_back({"missing-required", [&](auto &f) { f.rule->setAttr("ac.required_numeric", mlir::ArrayAttr::get(&context, {})); }});
  mutations.push_back({"missing-proof-scope", [&](auto &f) { f.rule->removeAttr("ac.proof_scope"); }});
  mutations.push_back({"duplicate-ids", [&](auto &f) {
    auto duplicate = mlir::cast<mlir::DictionaryAttr>(f.nodes[1]);
    llvm::SmallVector<mlir::NamedAttribute> fields(duplicate.begin(), duplicate.end());
    for (auto &field : fields) if (field.getName() == "id") field = mlir::NamedAttribute(field.getName(), f.fromID);
    f.rule->setAttr("ac.required_numeric", mlir::ArrayAttr::get(&context, {f.nodes[0], mlir::DictionaryAttr::get(&context, fields), f.nodes[2]}));
  }});
  mutations.push_back({"wrong-origin", [&](auto &f) { mlir::OpBuilder b(&context); f.add->setAttr("ac.origin", occurrence(b, 99)); }});
  mutations.push_back({"fake-current", [&](auto &f) { f.fromBits->setOperand(0, f.rule.getBody().front().getArgument(0)); }});
  mutations.push_back({"unsupported-op", [&](auto &f) { f.add->setAttr("operator", mlir::StringAttr::get(&context, "sub")); }});
  mutations.push_back({"extra-math-op", [&](auto &f) {
    mlir::OpBuilder b(f.add); mlir::OperationState state(f.add.getLoc(), MathBinaryOp::getOperationName());
    state.addOperands({f.truth.getResult(), f.fromBits.getResult(), f.truth.getResult(), f.constant.getResult(), f.truth.getResult()});
    state.addAttribute("operator", b.getStringAttr("add")); state.addAttribute("ac.origin", occurrence(b, 77));
    state.addTypes({MathIntType::get(&context), b.getI1Type()}); (void)b.create(state);
  }});
  // clang-format on
  for (const auto &[name, mutate] : mutations) {
    SCOPED_TRACE(name);
    auto fixture = buildSourceFixture(context);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*fixture.file)));
    mutate(fixture);
    const auto before = print(fixture.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*fixture.file)));
    EXPECT_EQ(print(fixture.file->getOperation()), before);
  }
}

TEST_F(ScalarNumericLoweringTest, RejectsCapabilityWithoutChangingCallerIR) {
  for (const auto &[lower, upper, constant] :
       std::array<std::tuple<const char *, const char *, const char *>, 2>{
           {{"0", "18446744073709551616", "1"}, {"-8", "0", "8"}}}) {
    auto fixture = buildSourceFixture(context, lower, upper, constant);
    auto before = print(fixture.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*fixture.file)));
    EXPECT_EQ(print(fixture.file->getOperation()), before);
  }
}

TEST_F(ScalarNumericLoweringTest, LowersTwoValidRulesInOneTransaction) {
  auto fixture = buildSourceFixture(context);
  (void)appendClonedRule(fixture, context, 20, "second");
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*fixture.file)));
  ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*fixture.file)));
  EXPECT_FALSE(hasMathCarrier(fixture.file->getOperation()));
  unsigned proofs = 0;
  fixture.file->walk([&](NumericProofOp) { ++proofs; });
  EXPECT_EQ(proofs, 2u);
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*fixture.file)));
}

TEST_F(ScalarNumericLoweringTest,
       LaterRuleCapabilityFailureRollsBackWholeVerifiedUnit) {
  auto fixture = buildSourceFixture(context);
  auto second = appendClonedRule(fixture, context, 20, "second");
  setRuleConstant(second, context, "18446744073709551616");
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*fixture.file)));
  const auto before = print(fixture.file->getOperation());
  EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*fixture.file)));
  EXPECT_EQ(print(fixture.file->getOperation()), before);
}

TEST_F(ScalarNumericLoweringTest, RejectsDamagedLoweredWitnessUnchanged) {
  auto fixture = buildSourceFixture(context);
  ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*fixture.file)));
  auto proof = onlyProof(fixture.file->getOperation());
  ASSERT_TRUE(proof);
  proof->setAttr("result_id", fixture.inputID);
  const auto damaged = print(fixture.file->getOperation());
  EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*fixture.file)));
  EXPECT_EQ(print(fixture.file->getOperation()), damaged);
}

TEST_F(ScalarNumericLoweringTest, RejectsMixedSourceAndB0EvidenceUnchanged) {
  auto fixture = buildSourceFixture(context);
  appendB0Witness(fixture, context);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*fixture.file)));
  const auto before = print(fixture.file->getOperation());
  EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*fixture.file)));
  EXPECT_EQ(print(fixture.file->getOperation()), before);
}

} // namespace
} // namespace acir::compiler
