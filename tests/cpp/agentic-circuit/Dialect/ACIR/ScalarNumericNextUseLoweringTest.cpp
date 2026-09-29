#include "Compiler/ScalarNumericLowering.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <functional>
#include <optional>
#include <string>

namespace acir::compiler::numeric_next_test {
mlir::OwningOpRef<mlir::ModuleOp> compileCounter(mlir::MLIRContext &context,
                                                 bool lower);
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
    if (auto compare = value.getDefiningOp<mlir::arith::CmpIOp>()) {
      auto left = visit(compare.getLhs()), right = visit(compare.getRhs());
      if (!left || !right)
        return std::nullopt;
      bool result = false;
      switch (compare.getPredicate()) {
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

class ScalarNumericNextUseLoweringTest : public ::testing::Test {
protected:
  ScalarNumericNextUseLoweringTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect,
                        mlir::scf::SCFDialect>();
  }
  mlir::MLIRContext context;
};

TEST_F(ScalarNumericNextUseLoweringTest,
       HelperLowersAll256ValuesAndExactUseYieldAssociation) {
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
  auto enabled = yield.getValues()[1].getDefiningOp<mlir::arith::AndIOp>();
  ASSERT_TRUE(enabled);
  EXPECT_EQ(enabled.getLhs(), result.getValid());
  EXPECT_EQ(enabled.getRhs(), use->getOperand(2));
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

TEST_F(ScalarNumericNextUseLoweringTest,
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

TEST_F(ScalarNumericNextUseLoweringTest,
       InvalidSourceMaskRollsBackTheWholeUnitByteForByte) {
  auto file = numeric_next_test::compileCounter(context, false);
  ASSERT_TRUE(file);
  auto constants = all(file->getOperation(), "ac.math.constant");
  ASSERT_EQ(constants.size(), 2u);
  mlir::Operation *mask = nullptr;
  for (mlir::Operation *constant : constants) {
    auto value = constant->getAttrOfType<MathIntAttr>("value");
    if (value && value.getCanonicalValue() == "255")
      mask = constant;
  }
  ASSERT_NE(mask, nullptr);
  mask->setAttr(
      "value",
      MathIntAttr::get(&context, llvm::APSInt(llvm::APInt(8, 127), true)));
  std::string before = dump(file->getOperation());
  EXPECT_TRUE(mlir::failed(lowerExactInputAddTransactional(*file)));
  EXPECT_EQ(dump(file->getOperation()), before);
}

TEST_F(ScalarNumericNextUseLoweringTest,
       ForgedValueEnableMaskDomainAndValueIDAreRejected) {
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
  expectRejected("wrong mask", [&](mlir::ModuleOp file) {
    for (mlir::Operation *operation :
         all(file.getOperation(), "arith.constant")) {
      auto value = operation->getAttrOfType<mlir::IntegerAttr>("value");
      if (value && value.getType().getIntOrFloatBitWidth() == 8 &&
          value.getValue().getZExtValue() == 255) {
        operation->setAttr("value",
                           mlir::IntegerAttr::get(value.getType(), 127));
        return;
      }
    }
    ADD_FAILURE() << "missing lowered mask constant";
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

TEST_F(ScalarNumericNextUseLoweringTest,
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
    auto *proof = all(file.getOperation(), "ac.numeric.proof").front();
    auto domains = proof->getAttrOfType<mlir::ArrayAttr>("input_domains");
    mlir::Builder builder(&context);
    proof->setAttr(
        "input_domains",
        builder.getArrayAttr(
            {upper255(builder, mlir::cast<mlir::DictionaryAttr>(domains[0]))}));
  });
  expectRejected("segments", [&](mlir::ModuleOp file) {
    auto *proof = all(file.getOperation(), "ac.numeric.proof").front();
    mlir::Builder builder(&context);
    proof->setAttr("operand_segment_sizes",
                   builder.getDenseI32ArrayAttr({1, 1, 1, 1, 0, 1, 0}));
  });
}

TEST_F(ScalarNumericNextUseLoweringTest,
       FlagsReadSubstitutionConstantDomainAndExtraOpsAreRejected) {
  auto original = numeric_next_test::compileCounter(context, true);
  ASSERT_TRUE(original);
  auto expectRejected = [&](llvm::StringRef label, auto mutate) {
    SCOPED_TRACE(label.str());
    auto damaged = mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(original->clone()));
    mutate(*damaged);
    EXPECT_TRUE(mlir::failed(mlir::verify(*damaged)));
  };
  expectRejected("overflow flags", [&](mlir::ModuleOp file) {
    auto add = mlir::cast<mlir::arith::AddIOp>(
        only(file.getOperation(), "arith.addi"));
    add.setOverflowFlags(mlir::arith::IntegerOverflowFlags::nsw);
  });
  expectRejected("input binding", [&](mlir::ModuleOp file) {
    auto bindings = all(file.getOperation(), "ac.value.binding");
    ASSERT_EQ(bindings.size(), 6u);
    bindings[0]->setOperand(0, bindings[4]->getOperand(0));
  });
  expectRejected("lifted binding", [&](mlir::ModuleOp file) {
    auto bindings = all(file.getOperation(), "ac.value.binding");
    ASSERT_EQ(bindings.size(), 6u);
    bindings[1]->setOperand(0, bindings[4]->getOperand(0));
  });
  expectRejected("constant domain", [&](mlir::ModuleOp file) {
    auto bindings = all(file.getOperation(), "ac.value.binding");
    ASSERT_EQ(bindings.size(), 6u);
    auto word = bindings[4]->getAttrOfType<mlir::DictionaryAttr>("domain");
    bindings[2]->setAttr("domain", word);
  });
  expectRejected("extra constant", [&](mlir::ModuleOp file) {
    RuleOp rule = onlyRule(file);
    mlir::OpBuilder builder(rule.getBody().front().getTerminator());
    (void)mlir::arith::ConstantOp::create(
        builder, rule.getLoc(), builder.getI8Type(),
        builder.getIntegerAttr(builder.getI8Type(), 42));
  });
}

TEST_F(ScalarNumericNextUseLoweringTest,
       SourceValidityChainCannotBeReplacedByLiteralTrue) {
  auto file = numeric_next_test::compileCounter(context, false);
  ASSERT_TRUE(file);
  auto binaries = all(file->getOperation(), "ac.math.binary");
  ASSERT_EQ(binaries.size(), 2u);
  auto mask = mlir::cast<MathBinaryOp>(binaries[1]);
  mask.getLhsValidMutable().assign(mask.getPath());
  EXPECT_TRUE(mlir::failed(mlir::verify(*file)));
}

} // namespace
} // namespace acir::compiler
