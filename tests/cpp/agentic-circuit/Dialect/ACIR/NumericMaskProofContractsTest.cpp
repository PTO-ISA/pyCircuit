#include "Compiler/ScalarNumericLowering.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
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

namespace acir::compiler {
namespace {
using namespace acir::ac;

llvm::APInt bits(llvm::StringRef text, unsigned width = 256) {
  bool negative = text.consume_front("-");
  llvm::APInt value(width, text, 10);
  return negative ? -value : value;
}
std::string decimal(llvm::APInt value) {
  llvm::SmallString<128> text;
  llvm::APSInt(std::move(value), false).toString(text);
  return text.str().str();
}
std::string unsignedDecimal(llvm::APInt value) {
  llvm::SmallString<128> text;
  llvm::APSInt(std::move(value), true).toString(text);
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
mlir::DictionaryAttr occurrence(mlir::OpBuilder &b,unsigned n){auto index=b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr("index")),b.getNamedAttr("value",b.getI64IntegerAttr(n))});auto site=b.getDictionaryAttr({b.getNamedAttr("definition",mlir::FlatSymbolRefAttr::get(b.getContext(),"mask.Unit")),b.getNamedAttr("ast_path",b.getArrayAttr({index}))});return b.getDictionaryAttr({b.getNamedAttr("site",site),b.getNamedAttr("expansion",b.getArrayAttr({}))});}
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
  llvm::SmallVector<MathConstantOp> constants;
  llvm::SmallVector<MathBinaryOp> binaries;
  mlir::ArrayAttr nodes;
  mlir::DictionaryAttr inputID, fromID, kID, arithmeticID, maskID, resultID;
  bool lowBits;
  unsigned width;
};

Fixture build(mlir::MLIRContext &context, bool lowBits, llvm::StringRef opcode,
              llvm::StringRef lower, llvm::StringRef upper, llvm::StringRef k,
              unsigned width) {
  mlir::OpBuilder b(&context);
  auto loc = mlir::FileLineColLoc::get(&context, "mask.py", 7, 3);
  auto inputDomain = domain(b, lower, upper);
  auto inputID = id(b, 20, 0), fromID = id(b, 21, 2);
  auto kID = id(b, 22, 4), arithmeticID = id(b, 23, 6);
  auto maskID = id(b, 24, 8), resultID = id(b, 25, 10);
  llvm::APInt maskBits(width + 1, 0);
  maskBits.setLowBits(width);
  std::string mask = decimal(maskBits);
  llvm::SmallVector<mlir::Attribute> nodes;
  nodes.push_back(
      node(b, fromID, "from_bits", b.getArrayAttr({ref(b, "input", 0)})));
  if (lowBits) {
    nodes.push_back(node(b, kID, "constant", b.getArrayAttr({cref(b, k)})));
    nodes.push_back(
        node(b, arithmeticID, opcode,
             b.getArrayAttr({ref(b, "node", 0), ref(b, "node", 1)})));
    nodes.push_back(
        node(b, maskID, "constant", b.getArrayAttr({cref(b, mask)})));
    nodes.push_back(
        node(b, resultID, "and_bits",
             b.getArrayAttr({ref(b, "node", 2), ref(b, "node", 3)})));
  } else {
    nodes.push_back(
        node(b, maskID, "constant", b.getArrayAttr({cref(b, mask)})));
    nodes.push_back(
        node(b, resultID, "and_bits",
             b.getArrayAttr({ref(b, "node", 0), ref(b, "node", 1)})));
  }
  auto required = b.getArrayAttr(nodes);
  auto owner =
      b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr("mask")),
                           b.getNamedAttr("path", b.getStringAttr("mask.py"))});
  auto file = mlir::ModuleOp::create(loc);
  file->setAttr("ac.stage", b.getStringAttr("source"));
  file->setAttr("ac.unit_kind", b.getStringAttr("implementation"));
  file->setAttr("ac.source_owner", owner);
  b.setInsertionPointToStart(file.getBody());
  mlir::OperationState moduleState(loc, ModuleOp::getOperationName());
  moduleState.addAttribute("name", b.getStringAttr("Unit"));
  moduleState.addAttribute("sym_name", b.getStringAttr("mask.Unit"));
  moduleState.addAttribute("ac.source_owner", owner);
  moduleState.addAttribute("ac.origin", occurrence(b, 1));
  auto span = b.getDictionaryAttr(
      {b.getNamedAttr("path", b.getStringAttr("mask.py")),
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
                      mlir::FlatSymbolRefAttr::get(&context, "mask.Unit")),
       b.getNamedAttr("arguments", b.getArrayAttr({}))});
  mlir::OperationState ruleState(loc, RuleOp::getOperationName());
  ruleState.addOperands(moduleBody->getArgument(2));
  ruleState.addAttribute("name", b.getStringAttr("mask"));
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
  ruleState.addAttribute("ac.required_numeric", required);
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
  mlir::OperationState liftState(loc, MathFromBitsOp::getOperationName());
  liftState.addOperands(read.getResult());
  liftState.addAttribute("domain", inputDomain);
  liftState.addAttribute("ac.origin", occurrence(b, 21));
  liftState.addTypes(MathIntType::get(&context));
  auto lift = mlir::cast<MathFromBitsOp>(b.create(liftState));
  llvm::SmallVector<MathConstantOp> constants;
  auto makeConstant = [&](llvm::StringRef value, unsigned origin) {
    mlir::OperationState state(loc, MathConstantOp::getOperationName());
    state.addAttribute("value", mathInt(context, value));
    state.addAttribute("ac.origin", occurrence(b, origin));
    state.addTypes(MathIntType::get(&context));
    auto result = mlir::cast<MathConstantOp>(b.create(state));
    constants.push_back(result);
    return result;
  };
  llvm::SmallVector<MathBinaryOp> binaries;
  auto makeBinary = [&](llvm::StringRef op, mlir::Value lhs, mlir::Value rhs,
                        unsigned origin) {
    mlir::OperationState state(loc, MathBinaryOp::getOperationName());
    state.addOperands(
        {truth.getResult(), lhs, truth.getResult(), rhs, truth.getResult()});
    state.addAttribute("operator", b.getStringAttr(op));
    state.addAttribute("ac.origin", occurrence(b, origin));
    state.addTypes({MathIntType::get(&context), b.getI1Type()});
    auto result = mlir::cast<MathBinaryOp>(b.create(state));
    binaries.push_back(result);
    return result;
  };
  if (lowBits) {
    auto constant = makeConstant(k, 22);
    auto arithmetic =
        makeBinary(opcode, lift.getResult(), constant.getResult(), 23);
    auto maskConstant = makeConstant(mask, 24);
    (void)makeBinary("and_bits", arithmetic.getResult(),
                     maskConstant.getResult(), 25);
  } else {
    auto maskConstant = makeConstant(mask, 24);
    (void)makeBinary("and_bits", lift.getResult(), maskConstant.getResult(),
                     25);
  }
  YieldOp::create(b, loc, mlir::ValueRange{});
  b.setInsertionPointToEnd(moduleBody);
  YieldOp::create(b, loc, mlir::ValueRange{});
  return {std::move(file), module,   rule,     read,    constants,
          binaries,        required, inputID,  fromID,  kID,
          arithmeticID,    maskID,   resultID, lowBits, width};
}

std::string dump(mlir::Operation *op) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  op->print(stream, mlir::OpPrintingFlags().enableDebugInfo(
                        /*enable=*/true, /*prettyForm=*/false));
  return text;
}
ValueBindingOp binding(mlir::Operation *root, mlir::DictionaryAttr id) {
  ValueBindingOp result;
  root->walk([&](ValueBindingOp candidate) {
    if (candidate.getIdAttr() == id)
      result = candidate;
  });
  return result;
}
NumericProofOp proof(mlir::Operation *root) {
  NumericProofOp result;
  root->walk([&](NumericProofOp candidate) { result = candidate; });
  return result;
}

RuleOp appendRule(Fixture &destination, Fixture &source,
                  mlir::MLIRContext &context, unsigned line) {
  auto cloned = mlir::cast<RuleOp>(source.rule->clone());
  cloned->setOperand(0, destination.module.getBody().front().getArgument(2));
  mlir::OpBuilder b(&context);
  auto registration = occurrence(b, line);
  auto scope = cloned->getAttrOfType<mlir::DictionaryAttr>("ac.proof_scope");
  cloned->setAttr("name", b.getStringAttr("second_mask"));
  cloned->setAttr("registration", registration);
  cloned->setAttr(
      "ac.proof_scope",
      b.getDictionaryAttr({
          b.getNamedAttr("specialization", scope.get("specialization")),
          b.getNamedAttr("registration", registration),
      }));
  auto &body = destination.module.getBody().front();
  body.getOperations().insert(body.getTerminator()->getIterator(), cloned);
  return cloned;
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
  if (auto t = value.getDefiningOp<mlir::arith::TruncIOp>()) {
    auto v = evaluate(t.getIn(), source, input);
    return v ? std::optional(v->trunc(t.getType().getIntOrFloatBitWidth()))
             : std::nullopt;
  }
  auto binary = [&](mlir::Value lhs, mlir::Value rhs,
                    auto operation) -> std::optional<llvm::APInt> {
    auto l = evaluate(lhs, source, input), r = evaluate(rhs, source, input);
    return l && r ? std::optional(operation(*l, *r)) : std::nullopt;
  };
  if (auto add = value.getDefiningOp<mlir::arith::AddIOp>())
    return binary(add.getLhs(), add.getRhs(), std::plus<llvm::APInt>());
  if (auto sub = value.getDefiningOp<mlir::arith::SubIOp>())
    return binary(sub.getLhs(), sub.getRhs(), std::minus<llvm::APInt>());
  if (auto bitAnd = value.getDefiningOp<mlir::arith::AndIOp>())
    return binary(bitAnd.getLhs(), bitAnd.getRhs(),
                  std::bit_and<llvm::APInt>());
  return std::nullopt;
}

class NumericMaskProofContractsTest : public ::testing::Test {
protected:
  NumericMaskProofContractsTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect>();
  }
  mlir::MLIRContext context;
};

TEST_F(NumericMaskProofContractsTest, ExactMaskSignedOracleAndBindings) {
  for (unsigned width : {0u, 1u, 4u, 8u}) {
    SCOPED_TRACE("mask width=" + std::to_string(width));
    auto f = build(context, false, "and_bits", "-4", "4", "0", width);
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    auto input = binding(f.file->getOperation(), f.inputID);
    auto result = binding(f.file->getOperation(), f.resultID);
    ASSERT_TRUE(input && result);
    auto module = *f.file->getOps<ModuleOp>().begin();
    auto rule = *module.getBody().front().getOps<RuleOp>().begin();
    EXPECT_EQ(
        std::distance(rule.getBody().front().getOps<ValueBindingOp>().begin(),
                      rule.getBody().front().getOps<ValueBindingOp>().end()),
        4);
    EXPECT_EQ(
        result.getDomainAttr().getAs<MathIntAttr>("lower").getCanonicalValue(),
        "0");
    EXPECT_EQ(
        result.getDomainAttr().getAs<MathIntAttr>("upper").getCanonicalValue(),
        std::to_string((UINT64_C(1) << width)));
    for (int value = -4; value < 4; ++value) {
      auto actual =
          evaluate(result.getValue(), input.getValue(),
                   bits(std::to_string(value),
                        input.getValue().getType().getIntOrFloatBitWidth()));
      ASSERT_TRUE(actual);
      EXPECT_EQ(actual->getZExtValue(),
                static_cast<uint64_t>(value) & ((UINT64_C(1) << width) - 1));
    }
  }
  auto minusOne = build(context, false, "and_bits", "-1", "0", "0", 8);
  ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*minusOne.file)));
  auto input = binding(minusOne.file->getOperation(), minusOne.inputID);
  auto result = binding(minusOne.file->getOperation(), minusOne.resultID);
  auto actual = evaluate(result.getValue(), input.getValue(), bits("-1", 1));
  ASSERT_TRUE(actual);
  EXPECT_EQ(actual->getZExtValue(), 255u);
}

TEST_F(NumericMaskProofContractsTest,
       LowBitsModuloOracleAndNoIntermediateBinding) {
  for (llvm::StringRef opcode : {"add", "sub"}) {
    for (unsigned width : {1u, 4u, 8u}) {
      SCOPED_TRACE(std::string(opcode) + " width=" + std::to_string(width));
      auto f = build(context, true, opcode, "0", "16", "1", width);
      ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
      auto p = proof(f.file->getOperation());
      auto input = binding(f.file->getOperation(), f.inputID);
      auto result = binding(f.file->getOperation(), f.resultID);
      ASSERT_TRUE(p && input && result);
      EXPECT_EQ(p.getModeAttr().getValue(), "low_bits");
      EXPECT_EQ(p->getAttrOfType<mlir::IntegerAttr>("width").getInt(), width);
      EXPECT_FALSE(binding(f.file->getOperation(), f.arithmeticID));
      EXPECT_EQ(result.getDomainAttr()
                    .getAs<MathIntAttr>("lower")
                    .getCanonicalValue(),
                "0");
      EXPECT_EQ(result.getDomainAttr()
                    .getAs<MathIntAttr>("upper")
                    .getCanonicalValue(),
                unsignedDecimal(llvm::APInt(width + 1, 1).shl(width)));
      for (unsigned value = 0; value < 16; ++value) {
        auto actual = evaluate(
            result.getValue(), input.getValue(),
            llvm::APInt(input.getValue().getType().getIntOrFloatBitWidth(),
                        value));
        ASSERT_TRUE(actual);
        uint64_t expected = opcode == "add" ? value + 1 : value - 1;
        EXPECT_EQ(actual->getZExtValue(),
                  expected & ((UINT64_C(1) << width) - 1));
      }
    }
  }
}

TEST_F(NumericMaskProofContractsTest, Width64AndArbitraryConstantResidue) {
  for (llvm::StringRef opcode : {"add", "sub"}) {
    auto f = build(context, true, opcode, "0", "256",
                   "1361129467683753853853498429727072845825", 64);
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    auto k = binding(f.file->getOperation(), f.kID);
    auto result = binding(f.file->getOperation(), f.resultID);
    ASSERT_TRUE(k && result);
    EXPECT_EQ(
        mlir::cast<mlir::IntegerAttr>(
            k.getValue().getDefiningOp<mlir::arith::ConstantOp>().getValue())
            .getValue()
            .getZExtValue(),
        1u);
    if (opcode == "sub") {
      auto input = binding(f.file->getOperation(), f.inputID);
      auto actual =
          evaluate(result.getValue(), input.getValue(), llvm::APInt(8, 0));
      ASSERT_TRUE(actual);
      EXPECT_EQ(actual->getZExtValue(), UINT64_MAX);
    }
  }
  auto add = build(context, true, "add", "0", "256", "1", 8);
  ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*add.file)));
  auto result = binding(add.file->getOperation(), add.resultID);
  auto input = binding(add.file->getOperation(), add.inputID);
  EXPECT_EQ(evaluate(result.getValue(), input.getValue(), llvm::APInt(8, 255))
                ->getZExtValue(),
            0u);
}

TEST_F(NumericMaskProofContractsTest, RejectsSourceMutationsTransactionally) {
  using Mutation = std::function<void(Fixture &)>;
  std::vector<std::pair<const char *, Mutation>> cases;
  cases.push_back({"changed-mask", [&](auto &f) {
                     f.constants.back()->setAttr("value",
                                                 mathInt(context, "254"));
                   }});
  cases.push_back(
      {"omitted-obligation", [&](auto &f) {
         f.rule->setAttr(
             "ac.required_numeric",
             mlir::ArrayAttr::get(&context, f.nodes.getValue().drop_back()));
       }});
  cases.push_back({"unsupported-operator", [&](auto &f) {
                     f.binaries.front()->setAttr(
                         "operator",
                         mlir::StringAttr::get(&context, "and_bits"));
                   }});
  cases.push_back({"extra-use", [](auto &f) {
                     f.binaries.front()->setOperand(1, f.read.getResult());
                   }});
  cases.push_back({"dropped-check", [&](auto &f) {
                     mlir::OpBuilder b(&context);
                     f.rule->setAttr("ac.required_checks",
                                     b.getArrayAttr({b.getDictionaryAttr({})}));
                   }});
  for (auto &[name, mutate] : cases) {
    auto f = build(context, true, "add", "0", "256", "1", 8);
    mutate(f);
    auto before = dump(f.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*f.file))) << name;
    EXPECT_EQ(dump(f.file->getOperation()), before) << name;
  }
  auto negative = build(context, true, "sub", "-1", "8", "1", 8);
  auto before = dump(negative.file->getOperation());
  EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*negative.file)));
  EXPECT_EQ(dump(negative.file->getOperation()), before);
}

TEST_F(NumericMaskProofContractsTest, RejectsDamagedWitnessShapes) {
  for (unsigned badWidth : {0u, 7u, 65u}) {
    auto f = build(context, true, "add", "0", "256", "1", 8);
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    auto p = proof(f.file->getOperation());
    p->setAttr("width", mlir::IntegerAttr::get(
                            mlir::IntegerType::get(&context, 32), badWidth));
    EXPECT_TRUE(mlir::failed(mlir::verify(*f.file)));
  }
  {
    auto f = build(context, true, "sub", "0", "256", "1", 8);
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    auto p = proof(f.file->getOperation());
    auto result = binding(f.file->getOperation(), f.resultID);
    mlir::OpBuilder b(result);
    mlir::OperationState state(result.getLoc(),
                               ValueBindingOp::getOperationName());
    state.addOperands({result.getValue(), result.getValid(), result.getPath()});
    state.addAttribute("id", f.arithmeticID);
    state.addAttribute("domain", result.getDomainAttr());
    b.create(state);
    EXPECT_TRUE(mlir::failed(mlir::verify(*f.file)));
    EXPECT_TRUE(p);
  }
  {
    auto f = build(context, false, "and_bits", "-4", "4", "0", 8);
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    auto result = binding(f.file->getOperation(), f.resultID);
    auto andi = result.getValue().getDefiningOp<mlir::arith::AndIOp>();
    ASSERT_TRUE(andi);
    mlir::OpBuilder b(andi);
    auto trunc = mlir::arith::TruncIOp::create(b, andi.getLoc(), b.getI1Type(),
                                               andi.getLhs());
    auto extend = mlir::arith::ExtSIOp::create(
        b, andi.getLoc(), andi.getLhs().getType(), trunc.getResult());
    andi->setOperand(0, extend.getResult());
    EXPECT_TRUE(mlir::failed(mlir::verify(*f.file)));
  }
}

TEST_F(NumericMaskProofContractsTest, PlainU64PlusOneRemainsRejected) {
  auto f = build(context, true, "add", "0", "18446744073709551616", "1", 64);
  f.binaries.back().erase();
  f.constants.back().erase();
  f.rule->setAttr(
      "ac.required_numeric",
      mlir::ArrayAttr::get(&context, f.nodes.getValue().take_front(3)));
  auto before = dump(f.file->getOperation());
  EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*f.file)));
  EXPECT_EQ(dump(f.file->getOperation()), before);
}

TEST_F(NumericMaskProofContractsTest, RejectsProofProvenanceMutations) {
  using Mutation = std::function<void(NumericProofOp)>;
  std::vector<std::pair<const char *, Mutation>> cases;
  // clang-format off
  cases.push_back({"wrong-filename", [&](auto p) { p->setLoc(mlir::FileLineColLoc::get(&context, "wrong.py", 7, 3)); }});
  cases.push_back({"owner-mismatch", [&](auto p) { mlir::OpBuilder b(&context); p.getOperation()->template getParentOfType<ModuleOp>()->setAttr("ac.source_owner", b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr("mask")), b.getNamedAttr("path", b.getStringAttr("other.py"))})); }});
  cases.push_back({"wrong-definition", [&](auto p) { mlir::OpBuilder b(&context); auto origin=p.getOriginAttr(); auto site=origin.template getAs<mlir::DictionaryAttr>("site"); llvm::SmallVector<mlir::NamedAttribute> fields(site.begin(),site.end()); for(auto &field:fields) if(field.getName()=="definition") field=b.getNamedAttr("definition",mlir::FlatSymbolRefAttr::get(&context,"other.Unit")); p->setAttr("origin",b.getDictionaryAttr({b.getNamedAttr("site",b.getDictionaryAttr(fields)),b.getNamedAttr("expansion",b.getArrayAttr({}))})); }});
  cases.push_back({"nonempty-expansion", [&](auto p) { mlir::OpBuilder b(&context); auto origin=p.getOriginAttr(); auto frame=b.getDictionaryAttr({b.getNamedAttr("kind",b.getStringAttr("call")),b.getNamedAttr("site",origin.get("site")),b.getNamedAttr("callee",mlir::FlatSymbolRefAttr::get(&context,"mask.Helper"))}); p->setAttr("origin",b.getDictionaryAttr({b.getNamedAttr("site",origin.get("site")),b.getNamedAttr("expansion",b.getArrayAttr({frame}))})); }});
  // clang-format on
  for (bool lowBits : {false, true})
    for (auto &[name, mutate] : cases) {
      SCOPED_TRACE(std::string(lowBits ? "low_bits/" : "exact/") + name);
      auto f = build(context, lowBits, lowBits ? "add" : "and_bits", "0", "16",
                     "1", 4);
      ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
      auto p = proof(f.file->getOperation());
      ASSERT_TRUE(p);
      mutate(p);
      EXPECT_TRUE(mlir::failed(mlir::verify(*f.file)));
    }
}

TEST_F(NumericMaskProofContractsTest,
       TransactionIdentityIdempotenceAndRollback) {
  for (bool lowBits : {false, true}) {
    auto f = build(context, lowBits, lowBits ? "add" : "and_bits", "0", "16",
                   "1", 4);
    auto *root = f.file->getOperation();
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    EXPECT_EQ(f.file->getOperation(), root);
    auto once = dump(root);
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    EXPECT_EQ(dump(root), once);
    auto reparsed = mlir::parseSourceString<mlir::ModuleOp>(once, &context);
    ASSERT_TRUE(reparsed);
    EXPECT_TRUE(mlir::succeeded(mlir::verify(*reparsed)));
  }
  {
    auto exact = build(context, false, "and_bits", "0", "16", "0", 4);
    auto modular = build(context, true, "sub", "0", "16", "1", 4);
    (void)appendRule(exact, modular, context, 40);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*exact.file)));
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*exact.file)));
    unsigned proofs = 0;
    exact.file->walk([&](NumericProofOp) { ++proofs; });
    EXPECT_EQ(proofs, 2u);
  }
  {
    auto exact = build(context, false, "and_bits", "0", "16", "0", 4);
    auto invalid = build(context, true, "add", "0", "16", "-1", 4);
    (void)appendRule(exact, invalid, context, 41);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*exact.file)));
    auto before = dump(exact.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*exact.file)));
    EXPECT_EQ(dump(exact.file->getOperation()), before);
  }
}

} // namespace
} // namespace acir::compiler
