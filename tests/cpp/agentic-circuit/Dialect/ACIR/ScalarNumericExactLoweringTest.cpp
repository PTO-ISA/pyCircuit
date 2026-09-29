#include "Compiler/ScalarNumericLowering.h"
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
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace acir::compiler {
namespace {
using namespace acir::ac;

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
mlir::DictionaryAttr occurrence(mlir::OpBuilder &b, unsigned n) {
  auto index=b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr("index")),b.getNamedAttr("value",b.getI64IntegerAttr(n))});
  auto site=b.getDictionaryAttr({b.getNamedAttr("definition",mlir::FlatSymbolRefAttr::get(b.getContext(),"d1.Unit")),b.getNamedAttr("ast_path",b.getArrayAttr({index}))});
  return b.getDictionaryAttr({b.getNamedAttr("site",site),b.getNamedAttr("expansion",b.getArrayAttr({}))});
}
mlir::DictionaryAttr id(mlir::OpBuilder &b,unsigned origin,unsigned slot){return b.getDictionaryAttr({b.getNamedAttr("origin",occurrence(b,origin)),b.getNamedAttr("slot",b.getI32IntegerAttr(slot))});}
mlir::DictionaryAttr domain(mlir::OpBuilder &b,llvm::StringRef lo,llvm::StringRef hi){bool s=bits(lo).isNegative();return b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr("integer")),b.getNamedAttr("storage",mlir::TypeAttr::get(b.getIntegerType(widthFor(lo,hi,s)))),b.getNamedAttr("lower",mathInt(*b.getContext(),lo)),b.getNamedAttr("upper",mathInt(*b.getContext(),hi)),b.getNamedAttr("interpretation",b.getStringAttr(s?"signed":"unsigned"))});}
mlir::DictionaryAttr ref(mlir::OpBuilder &b,llvm::StringRef kind,unsigned n){return b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr(kind)),b.getNamedAttr("index",b.getI32IntegerAttr(n))});}
mlir::DictionaryAttr cref(mlir::OpBuilder &b,llvm::StringRef v){return b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr("constant")),b.getNamedAttr("value",mathInt(*b.getContext(),v))});}
mlir::DictionaryAttr node(mlir::OpBuilder &b,mlir::DictionaryAttr value,llvm::StringRef op,mlir::ArrayAttr operands){return b.getDictionaryAttr({b.getNamedAttr("id",value),b.getNamedAttr("operator",b.getStringAttr(op)),b.getNamedAttr("operands",operands),b.getNamedAttr("target",b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr("none"))}))});}
mlir::DictionaryAttr formal(mlir::OpBuilder &b){return b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr("formal")),b.getNamedAttr("parameter",b.getStringAttr("source")),b.getNamedAttr("ordinal",b.getUnitAttr())});}
// clang-format on

struct Fixture {
  mlir::OwningOpRef<mlir::ModuleOp> file;
  ModuleOp module;
  RuleOp rule;
  SourceReadOp read;
  MathFromBitsOp lift;
  MathConstantOp constant;
  mlir::Operation *operation;
  mlir::arith::ConstantOp truth;
  mlir::ArrayAttr nodes;
  mlir::DictionaryAttr inputDomain, inputID, liftID, constantID, resultID;
  std::string opcode;
};

Fixture build(mlir::MLIRContext &context, llvm::StringRef opcode,
              llvm::StringRef lower = "0", llvm::StringRef upper = "8",
              llvm::StringRef k = "1") {
  mlir::OpBuilder b(&context);
  auto loc = mlir::FileLineColLoc::get(&context, "d1.py", 7, 3);
  auto owner =
      b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr("d1")),
                           b.getNamedAttr("path", b.getStringAttr("d1.py"))});
  auto inputDomain = domain(b, lower, upper);
  auto inputID = id(b, 20, 0), liftID = id(b, 21, 3);
  auto constantID = id(b, 22, 5), resultID = id(b, 23, 7);
  auto nodes = b.getArrayAttr({
      node(b, liftID, "from_bits", b.getArrayAttr({ref(b, "input", 0)})),
      node(b, constantID, "constant", b.getArrayAttr({cref(b, k)})),
      node(b, resultID, opcode,
           b.getArrayAttr({ref(b, "node", 0), ref(b, "node", 1)})),
  });
  auto file = mlir::ModuleOp::create(loc);
  file->setAttr("ac.stage", b.getStringAttr("source"));
  file->setAttr("ac.unit_kind", b.getStringAttr("implementation"));
  file->setAttr("ac.source_owner", owner);
  b.setInsertionPointToStart(file.getBody());
  mlir::OperationState moduleState(loc, ModuleOp::getOperationName());
  moduleState.addAttribute("name", b.getStringAttr("Unit"));
  moduleState.addAttribute("sym_name", b.getStringAttr("d1.Unit"));
  moduleState.addAttribute("ac.source_owner", owner);
  moduleState.addAttribute("ac.origin", occurrence(b, 1));
  auto span = b.getDictionaryAttr(
      {b.getNamedAttr("path", b.getStringAttr("d1.py")),
       b.getNamedAttr("line", b.getI64IntegerAttr(2)),
       b.getNamedAttr("column", b.getI64IntegerAttr(1)),
       b.getNamedAttr("end_line", b.getI64IntegerAttr(2)),
       b.getNamedAttr("end_column", b.getI64IntegerAttr(7))});
  auto port = b.getDictionaryAttr(
      {b.getNamedAttr("parameter", b.getStringAttr("source")),
       b.getNamedAttr("ordinal", b.getUnitAttr()),
       b.getNamedAttr("role", b.getStringAttr("current")),
       b.getNamedAttr("type", inputDomain),
       b.getNamedAttr("origin", occurrence(b, 2)),
       b.getNamedAttr("location", span)});
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
  auto spec = b.getDictionaryAttr(
      {b.getNamedAttr("definition",
                      mlir::FlatSymbolRefAttr::get(&context, "d1.Unit")),
       b.getNamedAttr("arguments", b.getArrayAttr({}))});
  mlir::OperationState ruleState(loc, RuleOp::getOperationName());
  ruleState.addOperands(moduleBody->getArgument(2));
  ruleState.addAttribute("name", b.getStringAttr("scalar"));
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
  readState.addAttribute("ac.origin", occurrence(b, 20));
  auto read = mlir::cast<SourceReadOp>(b.create(readState));
  auto truth = mlir::arith::ConstantOp::create(b, loc, b.getI1Type(),
                                               b.getBoolAttr(true));
  auto lift = mlir::cast<MathFromBitsOp>([&] {
    mlir::OperationState state(loc, MathFromBitsOp::getOperationName());
    state.addOperands(read.getResult());
    state.addAttribute("domain", inputDomain);
    state.addAttribute("ac.origin", occurrence(b, 21));
    state.addTypes(MathIntType::get(&context));
    return b.create(state);
  }());
  auto constant = mlir::cast<MathConstantOp>([&] {
    mlir::OperationState state(loc, MathConstantOp::getOperationName());
    state.addAttribute("value", mathInt(context, k));
    state.addAttribute("ac.origin", occurrence(b, 22));
    state.addTypes(MathIntType::get(&context));
    return b.create(state);
  }());
  mlir::OperationState operationState(
      loc, opcode == "sub" ? MathBinaryOp::getOperationName()
                           : MathCompareOp::getOperationName());
  operationState.addOperands({truth.getResult(), lift.getResult(),
                              truth.getResult(), constant.getResult(),
                              truth.getResult()});
  operationState.addAttribute(opcode == "sub" ? "operator" : "predicate",
                              b.getStringAttr(opcode));
  operationState.addAttribute("ac.origin", occurrence(b, 23));
  operationState.addTypes({opcode == "sub"
                               ? mlir::Type(MathIntType::get(&context))
                               : mlir::Type(b.getI1Type()),
                           b.getI1Type()});
  auto *operation = b.create(operationState);
  YieldOp::create(b, loc, mlir::ValueRange{});
  b.setInsertionPointToEnd(moduleBody);
  YieldOp::create(b, loc, mlir::ValueRange{});
  return {std::move(file), module,    rule,       read,     lift,
          constant,        operation, truth,      nodes,    inputDomain,
          inputID,         liftID,    constantID, resultID, opcode.str()};
}

std::string dump(mlir::Operation *op) {
  std::string text;
  llvm::raw_string_ostream(text) << *op;
  return text;
}
bool hasSourceMath(mlir::Operation *root) {
  bool found = false;
  root->walk([&](mlir::Operation *op) {
    found |=
        mlir::isa<MathFromBitsOp, MathConstantOp, MathBinaryOp, MathCompareOp>(
            op);
    for (mlir::Type type : op->getResultTypes())
      found |= mlir::isa<MathIntType>(type);
  });
  return found;
}
ValueBindingOp binding(mlir::Operation *root, mlir::DictionaryAttr wanted) {
  ValueBindingOp result;
  root->walk([&](ValueBindingOp candidate) {
    if (candidate.getIdAttr() == wanted)
      result = candidate;
  });
  return result;
}
NumericProofOp proof(mlir::Operation *root) {
  NumericProofOp result;
  root->walk([&](NumericProofOp candidate) {
    if (!result)
      result = candidate;
  });
  return result;
}

std::optional<llvm::APInt> evaluate(mlir::Value value, mlir::Value source,
                                    llvm::APInt input) {
  if (value == source)
    return input;
  if (auto c = value.getDefiningOp<mlir::arith::ConstantOp>())
    return mlir::cast<mlir::IntegerAttr>(c.getValue()).getValue();
  if (auto e = value.getDefiningOp<mlir::arith::ExtUIOp>()) {
    auto v = evaluate(e.getIn(), source, input);
    return v ? std::optional(v->zext(e.getType().getIntOrFloatBitWidth()))
             : std::nullopt;
  }
  if (auto e = value.getDefiningOp<mlir::arith::ExtSIOp>()) {
    auto v = evaluate(e.getIn(), source, input);
    return v ? std::optional(v->sext(e.getType().getIntOrFloatBitWidth()))
             : std::nullopt;
  }
  if (auto s = value.getDefiningOp<mlir::arith::SubIOp>()) {
    auto l = evaluate(s.getLhs(), source, input),
         r = evaluate(s.getRhs(), source, input);
    return l && r ? std::optional(*l - *r) : std::nullopt;
  }
  if (auto c = value.getDefiningOp<mlir::arith::CmpIOp>()) {
    auto l = evaluate(c.getLhs(), source, input),
         r = evaluate(c.getRhs(), source, input);
    return l && r ? std::optional(llvm::APInt(1, mlir::arith::applyCmpPredicate(
                                                     c.getPredicate(), *l, *r)))
                  : std::nullopt;
  }
  return std::nullopt;
}

RuleOp appendRule(Fixture &destination, Fixture &source,
                  mlir::MLIRContext &context, unsigned line) {
  auto cloned = mlir::cast<RuleOp>(source.rule->clone());
  cloned->setOperand(0, destination.module.getBody().front().getArgument(2));
  mlir::OpBuilder b(&context);
  auto registration = occurrence(b, line);
  cloned->setAttr("registration", registration);
  auto oldScope = cloned->getAttrOfType<mlir::DictionaryAttr>("ac.proof_scope");
  cloned->setAttr(
      "ac.proof_scope",
      b.getDictionaryAttr(
          {b.getNamedAttr("specialization", oldScope.get("specialization")),
           b.getNamedAttr("registration", registration)}));
  auto &body = destination.module.getBody().front();
  body.getOperations().insert(body.getTerminator()->getIterator(), cloned);
  return cloned;
}

void setConstant(RuleOp rule, mlir::MLIRContext &context,
                 llvm::StringRef value) {
  auto constant = *rule.getBody().front().getOps<MathConstantOp>().begin();
  constant->setAttr("value", mathInt(context, value));
  auto nodes = rule->getAttrOfType<mlir::ArrayAttr>("ac.required_numeric");
  auto raw = mlir::cast<mlir::DictionaryAttr>(nodes[1]);
  llvm::SmallVector<mlir::NamedAttribute> fields(raw.begin(), raw.end());
  mlir::OpBuilder b(&context);
  for (auto &field : fields)
    if (field.getName() == "operands")
      field = b.getNamedAttr("operands", b.getArrayAttr({cref(b, value)}));
  llvm::SmallVector<mlir::Attribute> changed(nodes.begin(), nodes.end());
  changed[1] = b.getDictionaryAttr(fields);
  rule->setAttr("ac.required_numeric", b.getArrayAttr(changed));
}

class ScalarNumericExactLoweringTest : public ::testing::Test {
protected:
  ScalarNumericExactLoweringTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect>();
  }
  mlir::MLIRContext context;
};

TEST_F(ScalarNumericExactLoweringTest, HelperLowersAllOperatorsIdempotently) {
  for (llvm::StringRef op : {"sub", "eq", "ne", "lt", "le", "gt", "ge"}) {
    SCOPED_TRACE(op.str());
    auto f = build(context, op, "-3", "4", "-1");
    auto *root = f.file->getOperation();
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*f.file)));
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    EXPECT_EQ(f.file->getOperation(), root);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*f.file)));
    EXPECT_FALSE(hasSourceMath(root));
    auto p = proof(root);
    ASSERT_TRUE(p);
    EXPECT_EQ(p.getObligationsAttr(), f.nodes);
    EXPECT_EQ(p.getResultIdAttr(), f.resultID);
    EXPECT_TRUE(binding(root, f.inputID));
    EXPECT_TRUE(binding(root, f.liftID));
    EXPECT_TRUE(binding(root, f.constantID));
    auto result = binding(root, f.resultID);
    ASSERT_TRUE(result);
    if (op == "sub")
      EXPECT_TRUE(result.getValue().getDefiningOp<mlir::arith::SubIOp>());
    else {
      EXPECT_TRUE(result.getValue().getDefiningOp<mlir::arith::CmpIOp>());
      EXPECT_EQ(
          result.getDomainAttr().getAs<mlir::StringAttr>("kind").getValue(),
          "bool");
    }
    EXPECT_EQ(f.liftID.getAs<mlir::IntegerAttr>("slot").getInt(), 3);
    EXPECT_EQ(f.constantID.getAs<mlir::IntegerAttr>("slot").getInt(), 5);
    EXPECT_EQ(f.resultID.getAs<mlir::IntegerAttr>("slot").getInt(), 7);
    auto once = dump(root);
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    EXPECT_EQ(dump(root), once);
  }
}

TEST_F(ScalarNumericExactLoweringTest, RegisteredPassUsesTheSameTransaction) {
  registerACIRScalarNumericPasses();
  for (llvm::StringRef op : {"sub", "eq", "ne", "lt", "le", "gt", "ge"}) {
    auto f = build(context, op);
    mlir::PassManager manager(&context);
    ASSERT_TRUE(mlir::succeeded(
        mlir::parsePassPipeline("ac-lower-exact-input-add", manager)));
    ASSERT_TRUE(mlir::succeeded(manager.run(*f.file)));
    EXPECT_FALSE(hasSourceMath(f.file->getOperation()));
  }
}

TEST_F(ScalarNumericExactLoweringTest,
       ActualSSAExhaustsSignedAndUnsignedDomains) {
  for (const auto &[lower, upper, k] :
       std::array<std::array<const char *, 3>, 2>{
           {{{"-3", "4", "-1"}}, {{"0", "8", "3"}}}}) {
    for (llvm::StringRef op : {"sub", "eq", "ne", "lt", "le", "gt", "ge"}) {
      auto f = build(context, op, lower, upper, k);
      ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
      auto in = binding(f.file->getOperation(), f.inputID);
      auto out = binding(f.file->getOperation(), f.resultID);
      ASSERT_TRUE(in && out);
      unsigned width =
          mlir::cast<mlir::IntegerType>(in.getValue().getType()).getWidth();
      for (int value = std::stoi(lower); value < std::stoi(upper); ++value) {
        auto actual = evaluate(out.getValue(), in.getValue(),
                               bits(std::to_string(value), width));
        ASSERT_TRUE(actual);
        int constant = std::stoi(k);
        if (op == "sub")
          EXPECT_EQ(*actual, bits(std::to_string(value - constant),
                                  actual->getBitWidth()));
        else {
          bool expected = op == "eq"   ? value == constant
                          : op == "ne" ? value != constant
                          : op == "lt" ? value < constant
                          : op == "le" ? value <= constant
                          : op == "gt" ? value > constant
                                       : value >= constant;
          EXPECT_EQ(actual->getBoolValue(), expected);
        }
      }
    }
  }
}

TEST_F(ScalarNumericExactLoweringTest, TwoRuleSuccessAndLaterFailureAreAtomic) {
  {
    auto sub = build(context, "sub", "0", "8", "1");
    auto compare = build(context, "lt", "0", "8", "3");
    (void)appendRule(sub, compare, context, 40);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*sub.file)));
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*sub.file)));
    unsigned proofs = 0;
    sub.file->walk([&](NumericProofOp) { ++proofs; });
    EXPECT_EQ(proofs, 2u);
  }
  {
    auto first = build(context, "sub", "0", "8", "1");
    auto second = build(context, "sub", "0", "8", "1");
    auto later = appendRule(first, second, context, 41);
    setConstant(later, context, "18446744073709551616");
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*first.file)));
    auto before = dump(first.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*first.file)));
    EXPECT_EQ(dump(first.file->getOperation()), before);
  }
}

TEST_F(ScalarNumericExactLoweringTest, RejectsInvalidSourceWithoutMutation) {
  using Mutation = std::function<void(Fixture &)>;
  std::vector<std::pair<const char *, Mutation>> cases;
  cases.push_back(
      {"wrong-recipe", [&](auto &f) {
         mlir::OpBuilder b(&context);
         auto nodes = llvm::to_vector(f.nodes);
         nodes[2] =
             node(b, f.resultID, "eq",
                  b.getArrayAttr({ref(b, "node", 0), ref(b, "node", 1)}));
         f.rule->setAttr("ac.required_numeric", b.getArrayAttr(nodes));
       }});
  cases.push_back({"swapped-sub", [](auto &f) {
                     f.operation->setOperand(1, f.constant.getResult());
                     f.operation->setOperand(3, f.lift.getResult());
                   }});
  cases.push_back(
      {"unsupported-mul", [&](auto &f) {
         mlir::OpBuilder b(&context);
         f.operation->setAttr("operator", b.getStringAttr("mul"));
         auto nodes = llvm::to_vector(f.nodes);
         nodes[2] =
             node(b, f.resultID, "mul",
                  b.getArrayAttr({ref(b, "node", 0), ref(b, "node", 1)}));
         f.rule->setAttr("ac.required_numeric", b.getArrayAttr(nodes));
       }});
  cases.push_back({"extra-op", [&](auto &f) {
                     mlir::OpBuilder b(f.operation);
                     (void)mlir::arith::ConstantOp::create(
                         b, f.operation->getLoc(), b.getI8Type(),
                         b.getI8IntegerAttr(1));
                   }});
  cases.push_back(
      {"target", [](auto &f) {
         f.rule->insertOperands(1, f.module.getBody().front().getArgument(2));
         f.rule->setAttr(
             "operandSegmentSizes",
             mlir::DenseI32ArrayAttr::get(f.rule.getContext(), {1, 1}));
       }});
  cases.push_back({"check", [&](auto &f) {
                     mlir::OpBuilder b(&context);
                     f.rule->setAttr("ac.required_checks",
                                     b.getArrayAttr({b.getDictionaryAttr({})}));
                   }});
  for (auto &[name, mutate] : cases) {
    SCOPED_TRACE(name);
    const bool subtraction = name == std::string("wrong-recipe") ||
                             name == std::string("swapped-sub") ||
                             name == std::string("unsupported-mul");
    auto f = build(context, subtraction ? "sub" : "lt");
    mutate(f);
    auto before = dump(f.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*f.file)));
    EXPECT_EQ(dump(f.file->getOperation()), before);
  }
}

TEST_F(ScalarNumericExactLoweringTest, DamagedAndMixedUnitsRejectUnchanged) {
  {
    auto f = build(context, "sub");
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    proof(f.file->getOperation())->setAttr("result_id", f.inputID);
    auto before = dump(f.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*f.file)));
    EXPECT_EQ(dump(f.file->getOperation()), before);
  }
  {
    auto lowered = build(context, "sub");
    auto source = build(context, "lt");
    auto saved = mlir::cast<RuleOp>(source.rule->clone());
    ASSERT_TRUE(
        mlir::succeeded(lowerExactInputAddTransactional(*lowered.file)));
    auto destination = *lowered.file->getOps<ModuleOp>().begin();
    saved->setOperand(0, destination.getBody().front().getArgument(2));
    mlir::OpBuilder b(&context);
    auto registration = occurrence(b, 50);
    auto scope = saved->getAttrOfType<mlir::DictionaryAttr>("ac.proof_scope");
    saved->setAttr("registration", registration);
    saved->setAttr(
        "ac.proof_scope",
        b.getDictionaryAttr({
            b.getNamedAttr("specialization", scope.get("specialization")),
            b.getNamedAttr("registration", registration),
        }));
    auto &body = destination.getBody().front();
    body.getOperations().insert(body.getTerminator()->getIterator(), saved);
    auto before = dump(lowered.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*lowered.file)));
    EXPECT_EQ(dump(lowered.file->getOperation()), before);
  }
}

} // namespace
} // namespace acir::compiler
