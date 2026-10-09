#include "Compiler/NumericLowering.h"
#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <functional>
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
  MathFromBitsOp from;
  MathToBitsOp convert;
  SourceExpectOp expect;
  mlir::DictionaryAttr inputID, fromID, resultID, checkIdentity;
  mlir::ArrayAttr nodes;
};

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
  auto nodes = b.getArrayAttr(
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
  auto checkLocation = span(b);
  auto requiredCheck =
      b.getDictionaryAttr({b.getNamedAttr("id", checkIdentity),
                           b.getNamedAttr("kind", b.getStringAttr("range")),
                           b.getNamedAttr("location", checkLocation)});
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
  ruleState.addAttribute("ac.required_numeric", nodes);
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
  mlir::OperationState fromState(loc, MathFromBitsOp::getOperationName());
  fromState.addOperands(read.getResult());
  fromState.addAttribute("domain", inputDomain);
  fromState.addAttribute("ac.origin", occurrence(b, 21));
  fromState.addTypes(MathIntType::get(&context));
  auto from = mlir::cast<MathFromBitsOp>(b.create(fromState));
  auto checkSite = checkIdentity.getAs<mlir::DictionaryAttr>("check");
  auto checkTemplate = b.getDictionaryAttr(
      {b.getNamedAttr("leaf", checkSite.getAs<mlir::DictionaryAttr>("site")),
       b.getNamedAttr("kind", b.getStringAttr("range")),
       b.getNamedAttr("obligation", b.getI64IntegerAttr(0)),
       b.getNamedAttr("location", checkLocation)});
  mlir::OperationState convertState(loc, MathToBitsOp::getOperationName());
  convertState.addOperands({p, from.getResult(), v});
  convertState.addAttribute("domain", resultDomain);
  convertState.addAttribute("ac.origin", occurrence(b, 22));
  convertState.addAttribute("ac.check_template", checkTemplate);
  convertState.addTypes(
      {resultDomain.getAs<mlir::TypeAttr>("storage").getValue(),
       b.getI1Type()});
  auto convert = mlir::cast<MathToBitsOp>(b.create(convertState));
  mlir::OperationState expectState(loc, SourceExpectOp::getOperationName());
  expectState.addOperands({convert.getValid(), convert.getPath()});
  expectState.addAttribute("kind", b.getStringAttr("range"));
  expectState.addAttribute("location", checkLocation);
  expectState.addAttribute("ac.check_id", checkIdentity);
  auto expect = mlir::cast<SourceExpectOp>(b.create(expectState));
  YieldOp::create(b, loc, mlir::ValueRange{});
  b.setInsertionPointToEnd(moduleBody);
  YieldOp::create(b, loc, mlir::ValueRange{});
  return {std::move(file), module,  rule,   read,     from,          convert,
          expect,          inputID, fromID, resultID, checkIdentity, nodes};
}

std::string dump(mlir::Operation *op) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  op->print(stream, mlir::OpPrintingFlags().enableDebugInfo(true, false));
  return text;
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
  root->walk([&](NumericProofOp candidate) { result = candidate; });
  return result;
}
bool hasSourceMath(mlir::Operation *root) {
  bool found = false;
  root->walk([&](mlir::Operation *op) {
    found |= mlir::isa<MathFromBitsOp, MathToBitsOp>(op);
    for (mlir::Type type : op->getResultTypes())
      found |= mlir::isa<MathIntType>(type);
  });
  return found;
}
RuleOp appendRule(Fixture &destination, Fixture &source,
                  mlir::MLIRContext &context, unsigned line) {
  auto cloned = mlir::cast<RuleOp>(source.rule->clone());
  cloned->setOperand(0, destination.module.getBody().front().getArgument(2));
  mlir::OpBuilder b(&context);
  auto registration = occurrence(b, line);
  auto scope = cloned->getAttrOfType<mlir::DictionaryAttr>("ac.proof_scope");
  cloned->setAttr("name", b.getStringAttr("second_range"));
  cloned->setAttr("registration", registration);
  cloned->setAttr(
      "ac.proof_scope",
      b.getDictionaryAttr(
          {b.getNamedAttr("specialization", scope.get("specialization")),
           b.getNamedAttr("registration", registration)}));
  auto required = cloned->getAttrOfType<mlir::ArrayAttr>("ac.required_checks");
  auto expect = *cloned.getBody().front().getOps<SourceExpectOp>().begin();
  auto oldID = expect->getAttrOfType<mlir::DictionaryAttr>("ac.check_id");
  auto newID = checkID(b, registration, line + 1);
  expect->setAttr("ac.check_id", newID);
  auto item = mlir::cast<mlir::DictionaryAttr>(required[0]);
  llvm::SmallVector<mlir::NamedAttribute> fields(item.begin(), item.end());
  for (auto &field : fields)
    if (field.getName() == "id")
      field = b.getNamedAttr("id", newID);
  cloned->setAttr("ac.required_checks",
                  b.getArrayAttr({b.getDictionaryAttr(fields)}));
  auto convert = *cloned.getBody().front().getOps<MathToBitsOp>().begin();
  auto templ =
      convert->getAttrOfType<mlir::DictionaryAttr>("ac.check_template");
  llvm::SmallVector<mlir::NamedAttribute> templateFields(templ.begin(),
                                                         templ.end());
  for (auto &field : templateFields)
    if (field.getName() == "leaf")
      field = b.getNamedAttr("leaf", newID.getAs<mlir::DictionaryAttr>("check")
                                         .getAs<mlir::DictionaryAttr>("site"));
  convert->setAttr("ac.check_template", b.getDictionaryAttr(templateFields));
  (void)oldID;
  auto &body = destination.module.getBody().front();
  body.getOperations().insert(body.getTerminator()->getIterator(), cloned);
  return cloned;
}

class NumericRangeLoweringTest : public ::testing::Test {
protected:
  NumericRangeLoweringTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect,
                        mlir::scf::SCFDialect>();
  }
  mlir::MLIRContext context;
};

TEST_F(NumericRangeLoweringTest, HelperLowersExactContractIdempotently) {
  for (const auto &[inputLower, inputUpper, lower, upper] :
       std::array<std::array<const char *, 4>, 4>{
           {{{"-16", "16", "-4", "5"}},
            {{"0", "16", "3", "12"}},
            {{"0", "16", "0", "16"}},
            {{"0", "18446744073709551616", "0", "18446744073709551616"}}}}) {
    auto f = build(context, inputLower, inputUpper, lower, upper);
    auto *root = f.file->getOperation();
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*f.file)));
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    EXPECT_EQ(f.file->getOperation(), root);
    EXPECT_FALSE(hasSourceMath(root));
    auto p = proof(root);
    auto input = binding(root, f.inputID), from = binding(root, f.fromID);
    auto result = binding(root, f.resultID);
    ASSERT_TRUE(p && input && from && result);
    EXPECT_EQ(input.getValue(), from.getValue());
    EXPECT_EQ(p->getOperand(0), p->getOperand(6));
    auto demand = p->getOperand(6).getDefiningOp<mlir::arith::AndIOp>();
    auto resultValid = p->getOperand(4).getDefiningOp<mlir::arith::AndIOp>();
    ASSERT_TRUE(demand && resultValid);
    EXPECT_EQ(resultValid.getLhs(), p->getOperand(6));
    EXPECT_EQ(resultValid.getRhs(), p->getOperand(5));
    auto branch = result.getValue().getDefiningOp<mlir::scf::IfOp>();
    ASSERT_TRUE(branch);
    EXPECT_EQ(branch.getCondition(), p->getOperand(4));
    auto rule = p->getParentOfType<RuleOp>();
    auto expect = *rule.getBody().front().getOps<SourceExpectOp>().begin();
    EXPECT_EQ(expect.getCondition(), p->getOperand(5));
    EXPECT_EQ(expect.getPath(), p->getOperand(6));
    auto once = dump(root);
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    EXPECT_EQ(dump(root), once);
    auto reparsed = mlir::parseSourceString<mlir::ModuleOp>(once, &context);
    ASSERT_TRUE(reparsed);
    EXPECT_TRUE(mlir::succeeded(mlir::verify(*reparsed)));
  }
}

TEST_F(NumericRangeLoweringTest, RegisteredPassLowersRangeSource) {
  registerACIRNumericPasses();
  auto f = build(context, "0", "16", "4", "8");
  mlir::PassManager manager(&context);
  ASSERT_TRUE(mlir::succeeded(
      mlir::parsePassPipeline("ac-lower-exact-input-add", manager)));
  ASSERT_TRUE(mlir::succeeded(manager.run(*f.file)));
  EXPECT_FALSE(hasSourceMath(f.file->getOperation()));
}

TEST_F(NumericRangeLoweringTest, EndpointTruthTableIsIndependent) {
  for (const auto &[lower, upper] :
       std::array<std::pair<int, int>, 2>{{{-4, 5}, {3, 12}}}) {
    const std::array<int, 4> samples{lower - 1, lower, upper - 1, upper};
    const std::array<bool, 4> expected{false, true, true, false};
    for (size_t i = 0; i < samples.size(); ++i)
      EXPECT_EQ(lower <= samples[i] && samples[i] < upper, expected[i]);
  }
}

TEST_F(NumericRangeLoweringTest, TwoRuleSuccessAndLaterRollback) {
  {
    auto first = build(context, "0", "16", "2", "12");
    auto second = build(context, "0", "16", "4", "8");
    (void)appendRule(first, second, context, 40);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*first.file)));
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*first.file)));
    unsigned proofs = 0;
    first.file->walk([&](NumericProofOp) { ++proofs; });
    EXPECT_EQ(proofs, 2u);
  }
  {
    auto first = build(context, "0", "16", "2", "12");
    auto disjoint = build(context, "0", "16", "16", "32");
    (void)appendRule(first, disjoint, context, 50);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*first.file)));
    auto before = dump(first.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*first.file)));
    EXPECT_EQ(dump(first.file->getOperation()), before);
  }
}

TEST_F(NumericRangeLoweringTest, RejectsSourceCheckMutationsUnchanged) {
  using Mutation = std::function<void(Fixture &)>;
  std::vector<std::pair<const char *, Mutation>> cases;
  cases.push_back({"missing", [](auto &f) { f.expect.erase(); }});
  cases.push_back({"duplicate", [](auto &f) {
                     f.expect->getBlock()->getOperations().insert(
                         f.expect->getIterator(), f.expect->clone());
                   }});
  cases.push_back({"stale", [&](auto &f) {
                     mlir::OpBuilder b(&context);
                     f.expect->setAttr(
                         "ac.check_id",
                         checkID(b, f.rule.getRegistrationAttr(), 99));
                   }});
  cases.push_back(
      {"multiple", [&](auto &f) {
         mlir::OpBuilder b(&context);
         auto duplicate = mlir::cast<SourceExpectOp>(f.expect->clone());
         duplicate->setAttr("ac.check_id",
                            checkID(b, f.rule.getRegistrationAttr(), 31, 1));
         f.expect->getBlock()->getOperations().insert(f.expect->getIterator(),
                                                      duplicate);
       }});
  cases.push_back(
      {"cross-rule", [&](auto &f) {
         auto other = mlir::cast<RuleOp>(f.rule->clone());
         auto &body = f.module.getBody().front();
         body.getOperations().insert(body.getTerminator()->getIterator(),
                                     other);
         f.expect->moveBefore(
             &other.getBody().front(),
             other.getBody().front().getTerminator()->getIterator());
       }});
  for (auto &[name, mutate] : cases) {
    auto f = build(context, "0", "16", "4", "8");
    mutate(f);
    auto before = dump(f.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*f.file))) << name;
    EXPECT_EQ(dump(f.file->getOperation()), before) << name;
  }
}

TEST_F(NumericRangeLoweringTest, DamagedAndMixedOutputRejectUnchanged) {
  {
    auto f = build(context, "0", "16", "4", "8");
    ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*f.file)));
    proof(f.file->getOperation())->setAttr("result_id", f.inputID);
    auto before = dump(f.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*f.file)));
    EXPECT_EQ(dump(f.file->getOperation()), before);
  }
  {
    auto lowered = build(context, "0", "16", "4", "8");
    auto source = build(context, "0", "16", "2", "12");
    ASSERT_TRUE(
        mlir::succeeded(lowerExactInputAddTransactional(*lowered.file)));
    auto module = *lowered.file->getOps<ModuleOp>().begin();
    Fixture destination;
    destination.module = module;
    (void)appendRule(destination, source, context, 60);
    auto before = dump(lowered.file->getOperation());
    EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*lowered.file)));
    EXPECT_EQ(dump(lowered.file->getOperation()), before);
  }
}

} // namespace
} // namespace acir::compiler
