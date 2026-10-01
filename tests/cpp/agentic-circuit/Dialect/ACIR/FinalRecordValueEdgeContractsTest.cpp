#include "FinalRecordValueTestFixture.h"

namespace acir::compiler {
namespace {
using namespace mlir;
using testing::FinalRecordValueContractsTest;

TEST_F(FinalRecordValueContractsTest,
       SameTypedIntegerFieldsStillUseDeclarationOrder) {
  installRecordPacket(/*homogeneousIntegers=*/true);
  ac::RuleOp rule = transfer();
  ASSERT_TRUE(succeeded(ac::verifyFinalRuleRegion(rule)));
  ASSERT_TRUE(succeeded(mlir::verify(rule)));
  auto create = sourceCreate(rule);
  ASSERT_TRUE(create);
  ASSERT_EQ(create.getValues()[0].getType(), IntegerType::get(&context, 64));
  ASSERT_EQ(create.getValues()[1].getType(), IntegerType::get(&context, 64));

  Value first = create.getValues()[0];
  create->setOperand(0, create.getValues()[1]);
  create->setOperand(1, first);
  EXPECT_TRUE(failed(ac::verifyFinalRuleRegion(rule)));
  EXPECT_TRUE(failed(mlir::verify(rule)));
}

TEST_F(FinalRecordValueContractsTest,
       SortedRequiredRecordsMayReferenceLaterIDs) {
  installRecordPacket();
  ac::RuleOp rule = transfer();
  Builder builder(&context);
  auto created = sourceCreate(rule);
  ASSERT_TRUE(created);
  auto newCreateID = valueID(builder, 19);
  auto newOrigin = newCreateID.getAs<DictionaryAttr>("origin");
  created->setAttr("ac.origin", newOrigin);

  ac::ValueBindingOp createdBinding;
  for (ac::ValueBindingOp binding :
       rule.getBody().front().getOps<ac::ValueBindingOp>())
    if (binding.getValue() == created.getResult())
      createdBinding = binding;
  ASSERT_TRUE(createdBinding);
  createdBinding->setAttr("id", newCreateID);

  auto uses = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
  auto requiredUse = cast<DictionaryAttr>(uses[0]);
  requiredUse = replace(requiredUse, "value", newCreateID, builder);
  rule->setAttr("ac.required_uses", builder.getArrayAttr({requiredUse}));
  auto actualUse = *rule.getBody().front().getOps<ac::ValueUseOp>().begin();
  actualUse->setAttr("source", newCreateID);

  auto records = rule->getAttrOfType<ArrayAttr>("ac.required_records");
  SmallVector<Attribute> reordered(records.begin(), records.end());
  auto createRequirement = cast<DictionaryAttr>(reordered.back());
  createRequirement = replace(createRequirement, "id", newCreateID, builder);
  reordered.back() = createRequirement;
  std::rotate(reordered.begin(), reordered.end() - 1, reordered.end());
  rule->setAttr("ac.required_records", builder.getArrayAttr(reordered));

  EXPECT_TRUE(succeeded(ac::verifyFinalRuleRegion(rule)));
  EXPECT_TRUE(succeeded(mlir::verify(rule)));
}

TEST_F(FinalRecordValueContractsTest,
       RequiredRecordGraphRejectsDanglingCyclesUnknownFieldsAndUnsortedIDs) {
  auto rejects = [&](llvm::StringRef label, auto mutate) {
    restoreBaseline();
    installRecordPacket();
    ac::RuleOp rule = transfer();
    ASSERT_TRUE(rule);
    mutate(rule);
    EXPECT_TRUE(failed(ac::verifyFinalRuleRegion(rule))) << label.str();
    EXPECT_TRUE(failed(mlir::verify(rule))) << label.str();
  };

  rejects("dangling create field ID", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto records = rule->getAttrOfType<ArrayAttr>("ac.required_records");
    auto create = cast<DictionaryAttr>(records[records.size() - 1]);
    auto missing = valueID(builder, 99);
    create = replace(create, "fields", builder.getArrayAttr({missing, missing}),
                     builder);
    SmallVector<Attribute> updated(records.begin(), records.end());
    updated.back() = create;
    rule->setAttr("ac.required_records", builder.getArrayAttr(updated));
  });
  rejects("record create cycle", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto records = rule->getAttrOfType<ArrayAttr>("ac.required_records");
    auto create = cast<DictionaryAttr>(records[records.size() - 1]);
    auto ownID = create.getAs<DictionaryAttr>("id");
    create = replace(create, "fields", builder.getArrayAttr({ownID, ownID}),
                     builder);
    SmallVector<Attribute> updated(records.begin(), records.end());
    updated.back() = create;
    rule->setAttr("ac.required_records", builder.getArrayAttr(updated));
  });
  rejects("unsorted required-record IDs", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto records = rule->getAttrOfType<ArrayAttr>("ac.required_records");
    SmallVector<Attribute> reversed;
    for (size_t index = records.size(); index > 0; --index)
      reversed.push_back(records[index - 1]);
    rule->setAttr("ac.required_records", builder.getArrayAttr(reversed));
  });
  rejects("unknown ordinal", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto records = rule->getAttrOfType<ArrayAttr>("ac.required_records");
    auto firstGet = cast<DictionaryAttr>(records[1]);
    firstGet =
        replace(firstGet, "field", builder.getI32IntegerAttr(2), builder);
    SmallVector<Attribute> updated(records.begin(), records.end());
    updated[1] = firstGet;
    rule->setAttr("ac.required_records", builder.getArrayAttr(updated));
  });
}

TEST_F(FinalRecordValueContractsTest,
       RecordIdentityAndOwnershipCannotBeRedirectedOrDuplicated) {
  auto rejects = [&](llvm::StringRef label, auto mutate) {
    restoreBaseline();
    installRecordPacket();
    ac::RuleOp rule = transfer();
    ASSERT_TRUE(rule);
    mutate(rule);
    EXPECT_TRUE(failed(ac::verifyFinalRuleRegion(rule))) << label.str();
    EXPECT_TRUE(failed(mlir::verify(rule))) << label.str();
  };

  rejects("foreign nominal symbol", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto records = rule->getAttrOfType<ArrayAttr>("ac.required_records");
    auto read = cast<DictionaryAttr>(records[0]);
    read = replace(read, "record",
                   FlatSymbolRefAttr::get(&context, "decl.provider.Twin"),
                   builder);
    SmallVector<Attribute> updated(records.begin(), records.end());
    updated[0] = read;
    rule->setAttr("ac.required_records", builder.getArrayAttr(updated));
  });
  rejects("redirect read to a different StateRef", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto bindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
    auto stateRef = cast<DictionaryAttr>(bindings[0]);
    auto declaration = occurrence(builder, "decl.root.Root", 88);
    stateRef = replace(stateRef, "declaration", declaration, builder);
    rule->setAttr("ac.input_bindings", builder.getArrayAttr({stateRef}));
  });
  rejects("required read names another StateRef", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto records = rule->getAttrOfType<ArrayAttr>("ac.required_records");
    auto read = cast<DictionaryAttr>(records[0]);
    auto foreign = replace(read.getAs<DictionaryAttr>("state"), "declaration",
                           occurrence(builder, "decl.root.Root", 89), builder);
    read = replace(read, "state", foreign, builder);
    SmallVector<Attribute> updated(records.begin(), records.end());
    updated[0] = read;
    rule->setAttr("ac.required_records", builder.getArrayAttr(updated));
  });
  rejects("provider declaration copied into root owner", [&](ac::RuleOp rule) {
    ac::StructOp record;
    package->walk([&](ac::StructOp candidate) {
      if (candidate.getSymName() == "decl.provider.Pair")
        record = candidate;
    });
    ASSERT_TRUE(record);
    auto rootUnit =
        rule->getParentOfType<ac::ModuleOp>()->getParentOfType<ModuleOp>();
    ASSERT_TRUE(rootUnit);
    rootUnit.getBody()->push_back(record->clone());
  });
  rejects("record declaration nested inside provider module",
          [&](ac::RuleOp rule) {
            ac::StructOp record;
            ac::ModuleOp providerModule;
            package->walk([&](ac::StructOp candidate) {
              if (candidate.getSymName() == "decl.provider.Pair")
                record = candidate;
            });
            package->walk([&](ac::ModuleOp candidate) {
              if (candidate.getSymName() == "decl.provider.Provider")
                providerModule = candidate;
            });
            ASSERT_TRUE(record && providerModule);
            Block &providerBody = providerModule.getBody().front();
            record->moveBefore(&providerBody, providerBody.begin());
            (void)rule;
          });
  rejects("provider owns exactly one canonical declaration",
          [&](ac::RuleOp rule) {
            ac::StructOp record;
            package->walk([&](ac::StructOp candidate) {
              if (candidate.getSymName() == "decl.provider.Pair")
                record = candidate;
            });
            ASSERT_TRUE(record);
            auto ownerUnit = record->getParentOfType<ModuleOp>();
            ASSERT_TRUE(ownerUnit);
            ownerUnit.getBody()->push_back(record->clone());
          });
}

TEST_F(FinalRecordValueContractsTest,
       ProviderImplementationDeclaresRecordsBeforeItsModule) {
  ModuleOp providerUnit;
  package->walk([&](ModuleOp candidate) {
    auto owner = candidate->getAttrOfType<DictionaryAttr>("ac.source_owner");
    auto path = owner ? owner.getAs<StringAttr>("path") : StringAttr();
    if (path && path.getValue() == "provider.py")
      providerUnit = candidate;
  });
  ASSERT_TRUE(providerUnit);
  EXPECT_EQ(providerUnit->getAttrOfType<StringAttr>("ac.unit_kind").getValue(),
            "implementation");
  bool sawProviderModule = false;
  size_t declarationCount = 0;
  for (Operation &operation : providerUnit.getBody()->getOperations()) {
    if (isa<ac::StructOp>(operation)) {
      EXPECT_FALSE(sawProviderModule)
          << "source-owned declarations precede the unit's ac.module";
      ++declarationCount;
    }
    if (auto module = dyn_cast<ac::ModuleOp>(operation))
      sawProviderModule |= module.getSymName() == "decl.provider.Provider";
  }
  EXPECT_EQ(declarationCount, 2u);
  EXPECT_TRUE(sawProviderModule);
  installRecordPacket();
  EXPECT_TRUE(succeeded(ac::verifyFinalRuleRegion(transfer())));
}

TEST_F(FinalRecordValueContractsTest,
       SyntheticSelectorsAndResetPlaceholdersHaveExactUses) {
  auto rejects = [&](llvm::StringRef label, auto mutate) {
    restoreBaseline();
    installRecordPacket();
    ac::RuleOp rule = transfer();
    ASSERT_TRUE(rule);
    mutate(rule);
    EXPECT_TRUE(failed(ac::verifyFinalRuleRegion(rule))) << label.str();
    EXPECT_TRUE(failed(mlir::verify(rule))) << label.str();
  };

  rejects(
      "selector cannot use field truth as its condition", [&](ac::RuleOp rule) {
        auto selects =
            llvm::to_vector(rule.getBody().front().getOps<arith::SelectOp>());
        ASSERT_EQ(selects.size(), 2u);
        auto field = sourceGet(rule);
        ASSERT_TRUE(field);
        auto other =
            std::next(rule.getBody().front().getOps<ac::StructGetOp>().begin());
        ASSERT_NE(other,
                  rule.getBody().front().getOps<ac::StructGetOp>().end());
        auto otherOp = *other;
        selects[1]->setOperand(0, otherOp.getResult());
      });
  rejects("synthetic zero cannot feed a source binding", [&](ac::RuleOp rule) {
    Builder builder(&context);
    arith::ConstantOp zero;
    ac::ValueBindingOp source;
    for (arith::ConstantOp constant :
         rule.getBody().front().getOps<arith::ConstantOp>()) {
      auto integer = dyn_cast<IntegerAttr>(constant.getValue());
      if (integer && integer.getType().isInteger(64) &&
          integer.getValue().isZero()) {
        zero = constant;
        break;
      }
    }
    for (ac::ValueBindingOp binding :
         rule.getBody().front().getOps<ac::ValueBindingOp>())
      if (binding.getValue().getType().isInteger(64)) {
        source = binding;
        break;
      }
    ASSERT_TRUE(zero && source);
    auto id = valueID(builder, 90);
    OperationState bindingState(source.getLoc(),
                                ac::ValueBindingOp::getOperationName());
    bindingState.addOperands({zero, source.getValid(), source.getPath()});
    bindingState.addAttribute("id", id);
    bindingState.addAttribute("domain", source.getDomainAttr());
    OpBuilder ops(source);
    (void)ops.create(bindingState);
  });
  rejects("synthetic zero bits are fixed", [&](ac::RuleOp rule) {
    auto constants =
        llvm::to_vector(rule.getBody().front().getOps<arith::ConstantOp>());
    bool changed = false;
    for (arith::ConstantOp constant : constants) {
      auto value = dyn_cast<IntegerAttr>(constant.getValue());
      if (value && value.getType().isInteger(64) && value.getValue().isZero()) {
        constant.setValueAttr(IntegerAttr::get(value.getType(), 1));
        changed = true;
        break;
      }
    }
    ASSERT_TRUE(changed);
  });
  rejects(
      "domain-external field truth is not a source path", [&](ac::RuleOp rule) {
        ac::StructGetOp hi;
        for (ac::StructGetOp candidate :
             rule.getBody().front().getOps<ac::StructGetOp>())
          if (candidate.getField() == "hi")
            hi = candidate;
        ASSERT_TRUE(hi);
        auto createdBinding = [&]() {
          ac::ValueBindingOp result;
          for (ac::ValueBindingOp binding :
               rule.getBody().front().getOps<ac::ValueBindingOp>())
            if (mlir::isa<ac::StructType>(binding.getValue().getType()) &&
                binding.getIdAttr() ==
                    cast<DictionaryAttr>(rule->getAttrOfType<ArrayAttr>(
                                             "ac.required_records")[3])
                        .getAs<DictionaryAttr>("id"))
              result = binding;
          return result;
        }();
        ASSERT_TRUE(createdBinding);
        auto use = *rule.getBody().front().getOps<ac::ValueUseOp>().begin();
        createdBinding.getPathMutable().assign(hi.getResult());
        use->setOperand(2, hi.getResult());
        OpBuilder builder(use);
        auto aggregate = arith::AndIOp::create(builder, use.getLoc(),
                                               hi.getResult(), use.getValid());
        for (arith::SelectOp select :
             rule.getBody().front().getOps<arith::SelectOp>())
          select->setOperand(0, aggregate);
        auto yield = cast<ac::YieldOp>(rule.getBody().front().getTerminator());
        yield->setOperand(1, aggregate);
      });
  rejects("selected create cannot be observed through another use",
          [&](ac::RuleOp rule) {
            auto yield =
                cast<ac::YieldOp>(rule.getBody().front().getTerminator());
            auto selected =
                yield.getValues()[0].getDefiningOp<ac::StructCreateOp>();
            ASSERT_TRUE(selected);
            OpBuilder builder(yield);
            OperationState getState(yield.getLoc(),
                                    ac::StructGetOp::getOperationName());
            getState.addOperands(selected.getResult());
            getState.addTypes(builder.getI64Type());
            getState.addAttribute("field", builder.getStringAttr("lo"));
            (void)builder.create(getState);
          });
}

TEST_F(FinalRecordValueContractsTest,
       RecordEvidenceRemainsWithinTheApprovedFiniteRuleSlice) {
  auto rejects = [&](llvm::StringRef label, auto mutate) {
    restoreBaseline();
    installRecordPacket();
    ac::RuleOp rule = transfer();
    ASSERT_TRUE(rule);
    mutate(rule);
    EXPECT_TRUE(failed(ac::verifyFinalRuleRegion(rule))) << label.str();
    EXPECT_TRUE(failed(mlir::verify(rule))) << label.str();
  };

  rejects("numeric proof profile", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto malformedNode = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("input"))});
    rule->setAttr("ac.required_numeric", builder.getArrayAttr({malformedNode}));
  });
  rejects("expanded source origin", [&](ac::RuleOp rule) {
    Builder builder(&context);
    auto get = sourceGet(rule);
    auto origin = get->getAttrOfType<DictionaryAttr>("ac.origin");
    auto frame = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("call")),
         builder.getNamedAttr("site", origin.getAs<DictionaryAttr>("site")),
         builder.getNamedAttr(
             "callee", FlatSymbolRefAttr::get(&context, "decl.root.helper"))});
    get->setAttr("ac.origin", replace(origin, "expansion",
                                      builder.getArrayAttr({frame}), builder));
  });
  rejects("index values are outside record closure", [&](ac::RuleOp rule) {
    auto get = sourceGet(rule);
    OpBuilder builder(get);
    auto index = arith::IndexCastOp::create(
        builder, get.getLoc(), builder.getIndexType(), get.getResult());
    (void)index;
  });
  rejects("SCF is outside the final record rule slice", [&](ac::RuleOp rule) {
    context.allowUnregisteredDialects(true);
    auto select = *rule.getBody().front().getOps<arith::SelectOp>().begin();
    OpBuilder builder(select);
    OperationState scfState(select.getLoc(), "scf.if");
    scfState.addOperands(select.getCondition());
    (void)builder.create(scfState);
  });
  rejects("unproved check or observation evidence", [&](ac::RuleOp rule) {
    Builder builder(&context);
    rule->setAttr("ac.required_observations",
                  builder.getArrayAttr({builder.getDictionaryAttr({})}));
  });
  rejects("check evidence is outside this value slice", [&](ac::RuleOp rule) {
    Builder builder(&context);
    rule->setAttr("ac.required_checks",
                  builder.getArrayAttr({builder.getDictionaryAttr({})}));
  });
  rejects("helper-return use is outside this source fixture",
          [&](ac::RuleOp rule) {
            Builder builder(&context);
            auto uses = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
            auto use = cast<DictionaryAttr>(uses[0]);
            auto target = use.getAs<DictionaryAttr>("target");
            target = replace(target, "kind",
                             builder.getStringAttr("helper_return"), builder);
            use = replace(use, "target", target, builder);
            rule->setAttr("ac.required_uses", builder.getArrayAttr({use}));
          });
}

} // namespace
} // namespace acir::compiler
