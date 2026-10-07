#include "Compiler/NumericLowering.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <functional>
#include <optional>
#include <string>

namespace acir::compiler::numeric_next_test {
mlir::OwningOpRef<mlir::ModuleOp>
compileCounter(mlir::MLIRContext &context, bool lower,
               llvm::StringRef expression = "(state + 1) & 255");
}

namespace acir::compiler {
namespace {

using namespace acir::ac;

std::string dump(mlir::Operation *operation) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  operation->print(stream,
                   mlir::OpPrintingFlags().enableDebugInfo(true, false));
  return result;
}

mlir::Operation *only(mlir::Operation *root, llvm::StringRef name) {
  mlir::Operation *result = nullptr;
  root->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() != name)
      return;
    EXPECT_EQ(result, nullptr) << "duplicate operation " << name.str();
    result = operation;
  });
  return result;
}

llvm::SmallVector<mlir::Operation *> all(mlir::Operation *root,
                                         llvm::StringRef name) {
  llvm::SmallVector<mlir::Operation *> result;
  root->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == name)
      result.push_back(operation);
  });
  return result;
}

RuleOp onlyRule(mlir::ModuleOp file) {
  auto module = *file.getOps<ModuleOp>().begin();
  return *module.getBody().front().getOps<RuleOp>().begin();
}

std::optional<llvm::APInt> evaluate(mlir::Value root, mlir::Value source,
                                    llvm::APInt sourceValue) {
  std::function<std::optional<llvm::APInt>(mlir::Value)> visit =
      [&](mlir::Value value) -> std::optional<llvm::APInt> {
    if (value == source)
      return sourceValue;
    if (auto constant = value.getDefiningOp<mlir::arith::ConstantOp>()) {
      auto integer = mlir::dyn_cast<mlir::IntegerAttr>(constant.getValue());
      return integer ? std::optional(integer.getValue()) : std::nullopt;
    }
    if (auto extension = value.getDefiningOp<mlir::arith::ExtUIOp>()) {
      auto input = visit(extension.getIn());
      auto type = mlir::cast<mlir::IntegerType>(extension.getType());
      return input ? std::optional(input->zext(type.getWidth())) : std::nullopt;
    }
    if (auto extension = value.getDefiningOp<mlir::arith::ExtSIOp>()) {
      auto input = visit(extension.getIn());
      auto type = mlir::cast<mlir::IntegerType>(extension.getType());
      return input ? std::optional(input->sext(type.getWidth())) : std::nullopt;
    }
    if (auto truncation = value.getDefiningOp<mlir::arith::TruncIOp>()) {
      auto input = visit(truncation.getIn());
      auto type = mlir::cast<mlir::IntegerType>(truncation.getType());
      return input ? std::optional(input->trunc(type.getWidth()))
                   : std::nullopt;
    }
    auto binary = [&](mlir::Value lhs, mlir::Value rhs,
                      auto operation) -> std::optional<llvm::APInt> {
      auto left = visit(lhs), right = visit(rhs);
      if (!left || !right || left->getBitWidth() != right->getBitWidth())
        return std::nullopt;
      return operation(*left, *right);
    };
    if (auto add = value.getDefiningOp<mlir::arith::AddIOp>())
      return binary(add.getLhs(), add.getRhs(), std::plus<llvm::APInt>());
    if (auto bitAnd = value.getDefiningOp<mlir::arith::AndIOp>())
      return binary(bitAnd.getLhs(), bitAnd.getRhs(),
                    std::bit_and<llvm::APInt>());
    if (auto bitOr = value.getDefiningOp<mlir::arith::OrIOp>())
      return binary(bitOr.getLhs(), bitOr.getRhs(), std::bit_or<llvm::APInt>());
    if (auto bitXor = value.getDefiningOp<mlir::arith::XOrIOp>())
      return binary(bitXor.getLhs(), bitXor.getRhs(),
                    std::bit_xor<llvm::APInt>());
    if (auto select = value.getDefiningOp<mlir::arith::SelectOp>()) {
      auto condition = visit(select.getCondition());
      return condition ? visit(condition->isOne() ? select.getTrueValue()
                                                  : select.getFalseValue())
                       : std::nullopt;
    }
    if (auto compare = value.getDefiningOp<mlir::arith::CmpIOp>()) {
      auto left = visit(compare.getLhs()), right = visit(compare.getRhs());
      if (!left || !right)
        return std::nullopt;
      bool result = false;
      switch (compare.getPredicate()) {
      case mlir::arith::CmpIPredicate::uge:
        result = left->uge(*right);
        break;
      case mlir::arith::CmpIPredicate::ugt:
        result = left->ugt(*right);
        break;
      case mlir::arith::CmpIPredicate::sge:
        result = left->sge(*right);
        break;
      case mlir::arith::CmpIPredicate::sgt:
        result = left->sgt(*right);
        break;
      case mlir::arith::CmpIPredicate::ne:
        result = *left != *right;
        break;
      case mlir::arith::CmpIPredicate::eq:
        result = *left == *right;
        break;
      case mlir::arith::CmpIPredicate::ule:
        result = left->ule(*right);
        break;
      case mlir::arith::CmpIPredicate::ult:
        result = left->ult(*right);
        break;
      case mlir::arith::CmpIPredicate::sle:
        result = left->sle(*right);
        break;
      case mlir::arith::CmpIPredicate::slt:
        result = left->slt(*right);
        break;
      default:
        return std::nullopt;
      }
      return llvm::APInt(1, result);
    }
    if (auto branch = value.getDefiningOp<mlir::scf::IfOp>()) {
      auto condition = visit(branch.getCondition());
      if (!condition)
        return std::nullopt;
      mlir::Region &region =
          condition->isOne() ? branch.getThenRegion() : branch.getElseRegion();
      auto yield =
          mlir::cast<mlir::scf::YieldOp>(region.front().getTerminator());
      return visit(yield.getResults()[mlir::cast<mlir::OpResult>(value)
                                          .getResultNumber()]);
    }
    return std::nullopt;
  };
  return visit(root);
}

ValueBindingOp bindingFor(mlir::Operation *root, mlir::DictionaryAttr id) {
  ValueBindingOp result;
  root->walk([&](ValueBindingOp binding) {
    if (binding.getIdAttr() == id)
      result = binding;
  });
  return result;
}

mlir::DictionaryAttr replaceField(mlir::Builder &builder,
                                  mlir::DictionaryAttr dictionary,
                                  llvm::StringRef name,
                                  mlir::Attribute replacement) {
  llvm::SmallVector<mlir::NamedAttribute> fields(dictionary.begin(),
                                                 dictionary.end());
  for (mlir::NamedAttribute &field : fields)
    if (field.getName() == name) {
      field = builder.getNamedAttr(name, replacement);
      return builder.getDictionaryAttr(fields);
    }
  ADD_FAILURE() << "missing dictionary field " << name.str();
  return dictionary;
}

mlir::DictionaryAttr upper255(mlir::Builder &builder,
                              mlir::DictionaryAttr domain) {
  return replaceField(
      builder, domain, "upper",
      MathIntAttr::get(builder.getContext(),
                       llvm::APSInt(llvm::APInt(8, 255), true)));
}

class NumericNextUseLoweringTest : public ::testing::Test {
protected:
  NumericNextUseLoweringTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect,
                        mlir::scf::SCFDialect>();
  }
  mlir::MLIRContext context;
};

TEST_F(NumericNextUseLoweringTest, GenericLoweringEvaluatesAllLegalOldStates) {
  auto file = numeric_next_test::compileCounter(context, false);
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*file)));
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*file)));
  RuleOp rule = onlyRule(*file);
  auto *read = only(file->getOperation(), "ac.source.read");
  auto *use = only(file->getOperation(), "ac.value.use");
  ASSERT_NE(read, nullptr);
  ASSERT_NE(use, nullptr);
  auto sourceID = use->getAttrOfType<mlir::DictionaryAttr>("source");
  auto result = bindingFor(file->getOperation(), sourceID);
  ASSERT_TRUE(result);
  auto yield = mlir::cast<YieldOp>(rule.getBody().front().getTerminator());
  ASSERT_EQ(yield.getValues().size(), 2u);
  EXPECT_EQ(use->getOperand(0), result.getValue());
  EXPECT_EQ(use->getOperand(1), result.getValid());
  EXPECT_EQ(yield.getValues()[0], result.getValue());
  for (unsigned value = 0; value != 256; ++value) {
    SCOPED_TRACE(value);
    llvm::APInt input(8, value);
    auto data = evaluate(result.getValue(), read->getResult(0), input);
    auto valid = evaluate(result.getValid(), read->getResult(0), input);
    auto path = evaluate(use->getOperand(2), read->getResult(0), input);
    auto enable = evaluate(yield.getValues()[1], read->getResult(0), input);
    ASSERT_TRUE(data && valid && path && enable);
    EXPECT_EQ(data->getZExtValue(), (value + 1) & 255);
    EXPECT_TRUE(valid->isOne());
    EXPECT_TRUE(path->isOne());
    EXPECT_TRUE(enable->isOne());
  }
}

TEST_F(NumericNextUseLoweringTest,
       PassPathIsEquivalentAndSecondLoweringIsIdempotent) {
  auto helper = numeric_next_test::compileCounter(context, false);
  auto pass = numeric_next_test::compileCounter(context, false);
  ASSERT_TRUE(helper && pass);
  ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*helper)));
  mlir::PassManager manager(&context);
  manager.addPass(createLowerExactInputAddPass());
  ASSERT_TRUE(mlir::succeeded(manager.run(*pass)));
  EXPECT_EQ(dump(helper->getOperation()), dump(pass->getOperation()));
  std::string once = dump(helper->getOperation());
  ASSERT_TRUE(mlir::succeeded(lowerExactInputAddTransactional(*helper)));
  EXPECT_EQ(dump(helper->getOperation()), once);
}

TEST_F(NumericNextUseLoweringTest,
       SourceMetadataMismatchRollsBackWholeUnitByteForByte) {
  auto file = numeric_next_test::compileCounter(context, false);
  ASSERT_TRUE(file);
  auto constants = all(file->getOperation(), "ac.math.constant");
  ASSERT_FALSE(constants.empty());
  // Change actual source SSA while retaining the original numeric obligation.
  // The smaller integer itself is valid; disagreement with its source identity
  // and obligation is the rejected contract.
  constants.front()->setAttr(
      "value",
      MathIntAttr::get(&context, llvm::APSInt(llvm::APInt(8, 7), true)));
  std::string before = dump(file->getOperation());
  EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*file)));
  EXPECT_EQ(dump(file->getOperation()), before);
}

TEST_F(NumericNextUseLoweringTest,
       ForgedValueEnableDomainAndValueIDAreRejected) {
  auto original = numeric_next_test::compileCounter(context, true);
  ASSERT_TRUE(original);
  auto expectRejected = [&](llvm::StringRef label, auto mutate) {
    SCOPED_TRACE(label.str());
    auto damaged = mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(original->clone()));
    mutate(*damaged);
    EXPECT_TRUE(mlir::failed(mlir::verify(*damaged)));
  };
  expectRejected("wrong value", [&](mlir::ModuleOp file) {
    auto *use = only(file.getOperation(), "ac.value.use");
    auto bindings = all(file.getOperation(), "ac.value.binding");
    auto wrong = mlir::cast<ValueBindingOp>(bindings.front());
    use->setOperand(0, wrong.getValue());
  });
  expectRejected("wrong enable", [&](mlir::ModuleOp file) {
    RuleOp rule = onlyRule(file);
    auto yield = mlir::cast<YieldOp>(rule.getBody().front().getTerminator());
    auto *use = only(file.getOperation(), "ac.value.use");
    yield->setOperand(1, use->getOperand(2));
  });
  expectRejected("wrong domain", [&](mlir::ModuleOp file) {
    auto *use = only(file.getOperation(), "ac.value.use");
    auto result =
        bindingFor(file.getOperation(),
                   use->getAttrOfType<mlir::DictionaryAttr>("source"));
    auto domain = result.getDomainAttr();
    mlir::Builder builder(&context);
    llvm::SmallVector<mlir::NamedAttribute> fields(domain.begin(),
                                                   domain.end());
    for (mlir::NamedAttribute &field : fields)
      if (field.getName() == "upper")
        field = builder.getNamedAttr(
            "upper", MathIntAttr::get(&context,
                                      llvm::APSInt(llvm::APInt(8, 255), true)));
    result->setAttr("domain", builder.getDictionaryAttr(fields));
  });
  expectRejected("wrong ValueID", [&](mlir::ModuleOp file) {
    auto *use = only(file.getOperation(), "ac.value.use");
    auto bindings = all(file.getOperation(), "ac.value.binding");
    use->setAttr("source",
                 bindings.front()->getAttrOfType<mlir::DictionaryAttr>("id"));
  });
}

TEST_F(NumericNextUseLoweringTest,
       ProofLocationOriginScopeSegmentsAndDomainsAreAuthoritative) {
  auto original = numeric_next_test::compileCounter(context, true);
  ASSERT_TRUE(original);
  auto expectRejected = [&](llvm::StringRef label, auto mutate) {
    SCOPED_TRACE(label.str());
    auto damaged = mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(original->clone()));
    mutate(*damaged);
    EXPECT_TRUE(mlir::failed(mlir::verify(*damaged)));
  };
  expectRejected("proof location", [&](mlir::ModuleOp file) {
    all(file.getOperation(), "ac.numeric.proof")
        .front()
        ->setLoc(mlir::UnknownLoc::get(&context));
  });
  expectRejected("proof definition", [&](mlir::ModuleOp file) {
    auto *proof = all(file.getOperation(), "ac.numeric.proof").front();
    auto origin = proof->getAttrOfType<mlir::DictionaryAttr>("origin");
    auto site = origin.getAs<mlir::DictionaryAttr>("site");
    mlir::Builder builder(&context);
    site =
        replaceField(builder, site, "definition",
                     mlir::FlatSymbolRefAttr::get(&context, "forged.Counter"));
    proof->setAttr("origin", replaceField(builder, origin, "site", site));
  });
  expectRejected("proof expansion", [&](mlir::ModuleOp file) {
    auto *proof = all(file.getOperation(), "ac.numeric.proof").front();
    auto origin = proof->getAttrOfType<mlir::DictionaryAttr>("origin");
    auto site = origin.getAs<mlir::DictionaryAttr>("site");
    mlir::Builder builder(&context);
    auto frame = builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("call")),
        builder.getNamedAttr("site", site),
        builder.getNamedAttr(
            "callee", mlir::FlatSymbolRefAttr::get(&context, "forged.Helper")),
    });
    proof->setAttr("origin", replaceField(builder, origin, "expansion",
                                          builder.getArrayAttr({frame})));
  });
  expectRejected("proof scope", [&](mlir::ModuleOp file) {
    RuleOp rule = onlyRule(file);
    auto scope = rule->getAttrOfType<mlir::DictionaryAttr>("ac.proof_scope");
    auto specialization = scope.getAs<mlir::DictionaryAttr>("specialization");
    mlir::Builder builder(&context);
    specialization =
        replaceField(builder, specialization, "definition",
                     mlir::FlatSymbolRefAttr::get(&context, "forged.Counter"));
    rule->setAttr(
        "ac.proof_scope",
        replaceField(builder, scope, "specialization", specialization));
  });
  expectRejected("input domain", [&](mlir::ModuleOp file) {
    NumericProofOp proof;
    for (auto *candidate : all(file.getOperation(), "ac.numeric.proof"))
      if (!mlir::cast<NumericProofOp>(candidate)
               .getInputDomainsAttr()
               .empty()) {
        proof = mlir::cast<NumericProofOp>(candidate);
        break;
      }
    ASSERT_TRUE(proof);
    auto domains = proof.getInputDomainsAttr();
    mlir::Builder builder(&context);
    llvm::SmallVector<mlir::Attribute> changed(domains.begin(), domains.end());
    changed.front() =
        upper255(builder, mlir::cast<mlir::DictionaryAttr>(changed.front()));
    proof->setAttr("input_domains", builder.getArrayAttr(changed));
  });
  expectRejected("segments", [&](mlir::ModuleOp file) {
    auto *proof = all(file.getOperation(), "ac.numeric.proof").front();
    mlir::Builder builder(&context);
    auto segments =
        proof->getAttrOfType<mlir::DenseI32ArrayAttr>("operand_segment_sizes");
    ASSERT_TRUE(segments);
    llvm::SmallVector<int32_t> changed(segments.asArrayRef());
    ++changed.front();
    proof->setAttr("operand_segment_sizes",
                   builder.getDenseI32ArrayAttr(changed));
  });
}

TEST_F(NumericNextUseLoweringTest,
       SourceValidityChainCannotBeReplacedByLiteralTrue) {
  auto file = numeric_next_test::compileCounter(context, false);
  ASSERT_TRUE(file);
  auto binaries = all(file->getOperation(), "ac.math.binary");
  ASSERT_FALSE(binaries.empty());
  auto binary = mlir::cast<MathBinaryOp>(binaries.back());
  ASSERT_NE(binary.getLhsValid(), binary.getPath());
  binary.getLhsValidMutable().assign(binary.getPath());
  EXPECT_TRUE(mlir::failed(mlir::verify(*file)));
}

} // namespace
} // namespace acir::compiler
