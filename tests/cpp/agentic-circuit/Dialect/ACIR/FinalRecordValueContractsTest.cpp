#include "FinalRecordValueTestFixture.h"

namespace acir::compiler {
namespace {
using namespace mlir;
using testing::FinalRecordValueContractsTest;

TEST(FinalRecordValueAttributePlacement, RequiredRecordsIsRuleOnlyMetadata) {
  MLIRContext context;
  context.loadDialect<ac::ACIRDialect, arith::ArithDialect>();
  OpBuilder builder(&context);
  Location location = UnknownLoc::get(&context);
  auto package = ModuleOp::create(location);
  builder.setInsertionPointToStart(package.getBody());
  auto constant = arith::ConstantOp::create(
      builder, location, builder.getI1Type(), builder.getBoolAttr(true));
  constant->setAttr("ac.required_records", builder.getArrayAttr({}));

  EXPECT_TRUE(failed(mlir::verify(constant)));
}

TEST_F(FinalRecordValueContractsTest,
       ExistingFinalRuleRegionAdmitsTheExactFiniteRecordValuePacket) {
  installRecordPacket();
  ac::RuleOp rule = transfer();

  EXPECT_TRUE(succeeded(ac::verifyFinalRuleRegion(rule)));
  EXPECT_TRUE(succeeded(mlir::verify(rule)));
  EXPECT_TRUE(failed(ac::verifyFinalHardware(*package)))
      << "S3 record reset materialization remains intentionally unadmitted";

  std::string text;
  llvm::raw_string_ostream stream(text);
  package->print(stream, OpPrintingFlags().enableDebugInfo());
  stream.flush();
  MLIRContext fresh;
  fresh.loadDialect<ac::ACIRDialect, arith::ArithDialect>();
  ParserConfig config(&fresh, /*verifyAfterParse=*/false);
  auto parsed = parseSourceString<ModuleOp>(text, config);
  ASSERT_TRUE(parsed);
  ac::RuleOp reparsed;
  parsed->walk([&](ac::RuleOp candidate) {
    if (candidate.getName() == "transfer")
      reparsed = candidate;
  });
  ASSERT_TRUE(reparsed);
  EXPECT_TRUE(succeeded(ac::verifyFinalRuleRegion(reparsed)));
  EXPECT_TRUE(succeeded(mlir::verify(reparsed)));
  EXPECT_TRUE(failed(ac::verifyFinalHardware(*parsed)));
}

TEST_F(FinalRecordValueContractsTest,
       RequiredRecordsCannotBeAttachedToAnUnrelatedScalarOperation) {
  ac::RuleOp rule = transfer();
  ASSERT_TRUE(rule);
  auto constant = *rule.getBody().front().getOps<arith::ConstantOp>().begin();
  Builder builder(&context);
  constant->setAttr("ac.required_records", builder.getArrayAttr({}));

  EXPECT_TRUE(failed(mlir::verify(constant)));
}

TEST_F(FinalRecordValueContractsTest,
       RecordMetadataAndActualSSARejectIndependentTampering) {
  auto rejects = [&](llvm::StringRef label, auto mutate) {
    restoreBaseline();
    installRecordPacket();
    ac::RuleOp rule = transfer();
    ASSERT_TRUE(rule);
    mutate(rule);
    EXPECT_TRUE(failed(ac::verifyFinalRuleRegion(rule))) << label.str();
    EXPECT_TRUE(failed(mlir::verify(rule))) << label.str();
  };

  rejects("missing required-record obligations",
          [&](ac::RuleOp rule) { rule->removeAttr("ac.required_records"); });
  rejects("empty required-record obligations", [&](ac::RuleOp rule) {
    Builder builder(&context);
    rule->setAttr("ac.required_records", builder.getArrayAttr({}));
  });
  rejects("duplicate required-record identity", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto records = rule->getAttrOfType<ArrayAttr>("ac.required_records");
    SmallVector<Attribute> duplicate(records.begin(), records.end());
    duplicate.push_back(records[0]);
    rule->setAttr("ac.required_records", builder.getArrayAttr(duplicate));
  });
  rejects("unknown required-record variant", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto records = rule->getAttrOfType<ArrayAttr>("ac.required_records");
    auto read = cast<DictionaryAttr>(records[0]);
    SmallVector<Attribute> changed(records.begin(), records.end());
    changed[0] = replace(read, "kind", builder.getStringAttr("write"), builder);
    rule->setAttr("ac.required_records", builder.getArrayAttr(changed));
  });
  rejects("create operand order", [&](ac::RuleOp rule) {
    auto create = sourceCreate(rule);
    ASSERT_TRUE(create);
    create->setOperand(0, create.getValues()[1]);
    create->setOperand(1, create.getValues()[0]);
  });
  rejects("get field identity", [&](ac::RuleOp rule) {
    auto get = sourceGet(rule);
    ASSERT_TRUE(get);
    get->setAttr("field", StringAttr::get(&context, "hi"));
  });
  rejects("binding domain", [&](ac::RuleOp rule) {
    auto decl = [&]() {
      ac::StructOp result;
      package->walk([&](ac::StructOp candidate) {
        if (candidate.getSymName() == "decl.provider.Pair")
          result = candidate;
      });
      return result;
    }();
    ASSERT_TRUE(decl);
    auto wrongField = cast<DictionaryAttr>(decl.getFields()[1]);
    auto wrongType = cast<DictionaryAttr>(wrongField.get("type"));
    bool changed = false;
    for (ac::ValueBindingOp binding :
         rule.getBody().front().getOps<ac::ValueBindingOp>()) {
      if (binding.getValue().getType().isInteger(64)) {
        binding->setAttr("domain", wrongType);
        changed = true;
        break;
      }
    }
    ASSERT_TRUE(changed);
  });
  rejects("ValueUse source", [&](ac::RuleOp rule) {
    auto use = *rule.getBody().front().getOps<ac::ValueUseOp>().begin();
    auto records = rule->getAttrOfType<ArrayAttr>("ac.required_records");
    auto read = cast<DictionaryAttr>(records[0]);
    use->setAttr("source", read.getAs<DictionaryAttr>("id"));
  });
  rejects("selector enable does not match yield", [&](ac::RuleOp rule) {
    auto selects =
        llvm::to_vector(rule.getBody().front().getOps<arith::SelectOp>());
    ASSERT_EQ(selects.size(), 2u);
    auto yield = cast<ac::YieldOp>(rule.getBody().front().getTerminator());
    OpBuilder builder(selects[1]);
    auto falseCondition = arith::ConstantOp::create(
        builder, selects[1].getLoc(), builder.getI1Type(),
        builder.getBoolAttr(false));
    selects[1]->setOperand(0, falseCondition);
  });
  rejects("selector true and false arms", [&](ac::RuleOp rule) {
    auto select = *rule.getBody().front().getOps<arith::SelectOp>().begin();
    Value trueValue = select.getTrueValue();
    select.getTrueValueMutable().assign(select.getFalseValue());
    select.getFalseValueMutable().assign(trueValue);
  });
  rejects("nonzero zero placeholder", [&](ac::RuleOp rule) {
    bool changed = false;
    for (arith::ConstantOp constant :
         rule.getBody().front().getOps<arith::ConstantOp>()) {
      auto integer = dyn_cast<IntegerAttr>(constant.getValue());
      if (integer && integer.getType().isInteger(64) &&
          integer.getValue().isZero()) {
        constant.setValueAttr(IntegerAttr::get(integer.getType(), 1));
        changed = true;
        break;
      }
    }
    ASSERT_TRUE(changed);
  });
  rejects("hidden scalar arithmetic", [&](ac::RuleOp rule) {
    auto get = sourceGet(rule);
    ASSERT_TRUE(get);
    OpBuilder builder(get);
    (void)arith::AddIOp::create(builder, get.getLoc(), get.getResult(),
                                get.getResult());
  });
  rejects("orphan source record producer", [&](ac::RuleOp rule) {
    auto create = sourceCreate(rule);
    ASSERT_TRUE(create);
    OpBuilder builder(create->getBlock(), std::next(create->getIterator()));
    OperationState state(create.getLoc(), ac::StructGetOp::getOperationName());
    state.addOperands(create.getResult());
    state.addTypes(builder.getI8Type());
    state.addAttribute("field", builder.getStringAttr("lo"));
    (void)builder.create(state);
  });
  rejects("record binding rejects an unrelated physical type",
          [&](ac::RuleOp rule) {
            auto binding =
                *rule.getBody().front().getOps<ac::ValueBindingOp>().begin();
            OpBuilder builder(binding);
            auto floatType = builder.getF32Type();
            auto one =
                arith::ConstantOp::create(builder, binding.getLoc(), floatType,
                                          builder.getFloatAttr(floatType, 1.0));
            binding.getValueMutable().assign(one);
          });
  rejects("extra binding attributes", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto binding = *rule.getBody().front().getOps<ac::ValueBindingOp>().begin();
    binding->setAttr("unapproved", builder.getUnitAttr());
  });
  rejects("source-linked get requires origin", [&](ac::RuleOp rule) {
    auto get = sourceGet(rule);
    ASSERT_TRUE(get);
    get->removeAttr("ac.origin");
  });
  rejects("source-linked create requires origin", [&](ac::RuleOp rule) {
    auto create = sourceCreate(rule);
    ASSERT_TRUE(create);
    create->removeAttr("ac.origin");
  });
  rejects("ValueUse rejects unrelated physical types", [&](ac::RuleOp rule) {
    auto use = *rule.getBody().front().getOps<ac::ValueUseOp>().begin();
    OpBuilder builder(use);
    auto floatType = builder.getF32Type();
    auto one = arith::ConstantOp::create(builder, use.getLoc(), floatType,
                                         builder.getFloatAttr(floatType, 1.0));
    use.getValueMutable().assign(one);
  });
}

} // namespace
} // namespace acir::compiler
