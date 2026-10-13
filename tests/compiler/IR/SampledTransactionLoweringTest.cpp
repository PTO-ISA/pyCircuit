#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "Transforms/LowerTransactions.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace acir::ac::test {
std::string transactionLoweringFixture();
}

namespace acir::ac {
namespace {

using ::acir::detail::MaterializedTransactionLowering;
using ::acir::detail::PreparedTransactionLowering;

class SampledTransactionLoweringTest : public ::testing::Test {
protected:
  static constexpr uint64_t workLimit = 1024 * 1024;

  struct StagedLowering {
    mlir::OwningOpRef<mlir::ModuleOp> original;
    std::unique_ptr<SourceTransactionStateAnalysis> analysis;
    PreparedTransactionLowering prepared;
    MaterializedTransactionLowering candidate;
    std::string originalText;
    std::vector<mlir::Operation *> originalOperations;
  };

  mlir::MLIRContext context;
  mlir::OpBuilder builder{&context};
  std::string diagnostics;

  void SetUp() override { context.loadDialect<ACIRDialect>(); }

  template <typename Function> decltype(auto) capture(Function function) {
    diagnostics.clear();
    mlir::ScopedDiagnosticHandler handler(
        &context, [&](mlir::Diagnostic &diagnostic) {
          llvm::raw_string_ostream stream(diagnostics);
          diagnostic.print(stream);
          return mlir::success();
        });
    return function();
  }

  std::string print(mlir::Operation *operation) {
    std::string text;
    llvm::raw_string_ostream stream(text);
    operation->print(stream);
    stream << '\n';
    return text;
  }

  mlir::OwningOpRef<mlir::ModuleOp> parse() {
    auto file = mlir::parseSourceString<mlir::ModuleOp>(
        test::transactionLoweringFixture(),
        mlir::ParserConfig(&context, false));
    EXPECT_TRUE(file);
    if (file)
      EXPECT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*file); })))
          << diagnostics;
    return file;
  }

  ModuleOp module(mlir::ModuleOp file, llvm::StringRef name = "Owner") {
    return mlir::cast<ModuleOp>(mlir::SymbolTable::lookupSymbolIn(file, name));
  }

  template <typename Op> Op named(ModuleOp definition, llvm::StringRef name) {
    for (auto op : definition.getBody().front().getOps<Op>()) {
      auto symbol = op->template getAttrOfType<mlir::StringAttr>("sym_name");
      auto plain = op->template getAttrOfType<mlir::StringAttr>("name");
      if ((symbol && symbol.getValue() == name) ||
          (plain && plain.getValue() == name))
        return op;
    }
    ADD_FAILURE() << "missing " << name.str();
    return {};
  }

  InstanceOp instance(ModuleOp definition, llvm::StringRef name) {
    for (auto operation : definition.getBody().front().getOps<InstanceOp>())
      if (operation.getInstanceName() == name)
        return operation;
    ADD_FAILURE() << "missing instance " << name.str();
    return {};
  }

  template <typename Op> Op first(mlir::Region &region) {
    auto operations = region.front().getOps<Op>();
    EXPECT_NE(operations.begin(), operations.end());
    return *operations.begin();
  }

  std::vector<mlir::Operation *> operations(mlir::ModuleOp file) {
    std::vector<mlir::Operation *> result;
    file.walk([&](mlir::Operation *operation) { result.push_back(operation); });
    return result;
  }

  std::unique_ptr<StagedLowering>
  stage(std::function<void(mlir::ModuleOp)> mutate = {}) {
    auto original = parse();
    if (!original)
      return {};
    if (mutate)
      mutate(*original);
    if (mlir::failed(capture([&] { return mlir::verify(*original); }))) {
      ADD_FAILURE() << diagnostics;
      return {};
    }
    auto originalText = print(original->getOperation());
    auto originalOperations = operations(*original);
    auto analysis = std::make_unique<SourceTransactionStateAnalysis>(
        original->getOperation());
    if (!analysis->isValid()) {
      ADD_FAILURE() << diagnostics;
      return {};
    }
    HardwareSourceCheckLimits limits;
    limits.maxWorkUnits = workLimit;
    detail::SourceCheckBudget budget(limits);
    auto prepared = capture([&] {
      return acir::detail::prepareTransactionLowering(*original, *analysis,
                                                      budget);
    });
    if (mlir::failed(prepared)) {
      ADD_FAILURE() << diagnostics;
      return {};
    }
    auto candidate = capture([&] {
      return acir::detail::materializeTransactionLowering(*prepared, budget);
    });
    if (mlir::failed(candidate)) {
      ADD_FAILURE() << diagnostics;
      return {};
    }
    auto staged = std::make_unique<StagedLowering>();
    staged->original = std::move(original);
    staged->analysis = std::move(analysis);
    staged->prepared = std::move(*prepared);
    staged->candidate = std::move(*candidate);
    staged->originalText = std::move(originalText);
    staged->originalOperations = std::move(originalOperations);
    return staged;
  }

  mlir::LogicalResult verify(StagedLowering &staged) {
    HardwareSourceCheckLimits limits;
    limits.maxWorkUnits = workLimit;
    detail::SourceCheckBudget budget(limits);
    return capture([&] {
      return acir::detail::verifyTransactionMaterialization(
          staged.prepared, staged.candidate, budget);
    });
  }

  void expectOriginalUnchanged(const StagedLowering &staged) {
    EXPECT_EQ(print(staged.original.get().getOperation()), staged.originalText);
    EXPECT_EQ(operations(*staged.original), staged.originalOperations);
  }

  void rejects(StagedLowering &staged, llvm::StringRef reason) {
    EXPECT_TRUE(mlir::failed(verify(staged)));
    EXPECT_NE(diagnostics.find(reason.str()), std::string::npos) << diagnostics;
    expectOriginalUnchanged(staged);
  }

  unsigned transactionIndex(const StagedLowering &staged,
                            llvm::StringRef name) {
    for (auto [index, prepared] : llvm::enumerate(staged.prepared.transactions))
      if (prepared.plan->transaction->getAttrOfType<mlir::StringAttr>("name")
              .getValue() == name)
        return index;
    ADD_FAILURE() << "missing prepared transaction " << name.str();
    return 0;
  }

  mlir::DictionaryAttr span(unsigned line) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("path", builder.getStringAttr("lower.py")),
         builder.getNamedAttr("line", builder.getI64IntegerAttr(line)),
         builder.getNamedAttr("column", builder.getI64IntegerAttr(1)),
         builder.getNamedAttr("end_line", builder.getI64IntegerAttr(line)),
         builder.getNamedAttr("end_column", builder.getI64IntegerAttr(2))});
  }

  mlir::DictionaryAttr occurrence(llvm::StringRef owner, unsigned index) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr(
             "site", builder.getDictionaryAttr(
                         {builder.getNamedAttr(
                              "definition",
                              mlir::FlatSymbolRefAttr::get(&context, owner)),
                          builder.getNamedAttr(
                              "ast_path",
                              builder.getArrayAttr({builder.getDictionaryAttr(
                                  {builder.getNamedAttr(
                                       "kind", builder.getStringAttr("index")),
                                   builder.getNamedAttr(
                                       "value", builder.getI64IntegerAttr(
                                                    index))})}))})),
         builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
  }

  ModuleOp addPlainDefinition(mlir::ModuleOp file, llvm::StringRef name) {
    auto model = module(file);
    auto system = *file.getOps<SystemOp>().begin();
    builder.setInsertionPoint(system);
    mlir::OperationState state(model.getLoc(), ModuleOp::getOperationName());
    state.addAttributes(model->getAttrs());
    state.attributes.set("sym_name", builder.getStringAttr(name));
    state.attributes.set("function_type",
                         mlir::TypeAttr::get(builder.getFunctionType({}, {})));
    state.attributes.set("input_names", builder.getArrayAttr({}));
    state.attributes.set("output_names", builder.getArrayAttr({}));
    state.attributes.erase("ac.domain_inputs");
    state.addRegion();
    auto plain = mlir::cast<ModuleOp>(builder.create(state));
    auto *block = new mlir::Block;
    plain.getBody().push_back(block);
    builder.setInsertionPointToEnd(block);
    builder.create<YieldOp>(plain.getLoc(), mlir::ValueRange{});
    return plain;
  }

  ModuleOp addIsolatedUnwrittenStateDefinition(mlir::ModuleOp file) {
    auto source = module(file);
    auto system = *file.getOps<SystemOp>().begin();
    auto *copy = source->clone();
    copy->setAttr("sym_name", builder.getStringAttr("Idle"));
    file.getBody()->getOperations().insert(system->getIterator(), copy);
    auto idle = mlir::cast<ModuleOp>(copy);
    SourceStateOp kept;
    for (auto state : idle.getBody().front().getOps<SourceStateOp>())
      if (state.getInstanceName() == "hold")
        kept = state;
    EXPECT_TRUE(kept);
    idle.getBody().front().getTerminator()->setOperands(kept.getQ());
    for (auto transaction : llvm::make_early_inc_range(
             idle.getBody().front().getOps<SourceTransactionOp>()))
      transaction.erase();
    for (auto method : llvm::make_early_inc_range(
             idle.getBody().front().getOps<SourceMethodOp>()))
      method.erase();
    for (auto state : llvm::make_early_inc_range(
             idle.getBody().front().getOps<SourceStateOp>()))
      if (state != kept)
        state.erase();
    kept->setAttr("occurrence", occurrence("Idle", 0));
    idle->setAttr("function_type", mlir::TypeAttr::get(builder.getFunctionType(
                                       idle.getFunctionType().getInputs(),
                                       {kept.getQ().getType()})));
    idle->setAttr("output_names",
                  builder.getArrayAttr({builder.getStringAttr("hold")}));
    return idle;
  }

  RuleOp addForgedAssert(ModuleOp definition) {
    auto &body = definition.getBody().front();
    auto condition = body.getArgument(1);
    auto location = span(3);
    auto registration = occurrence(definition.getSymName(), 80);
    auto check = occurrence(definition.getSymName(), 81);
    auto id = builder.getDictionaryAttr(
        {builder.getNamedAttr("registration", registration),
         builder.getNamedAttr("check", check),
         builder.getNamedAttr("obligation", builder.getI64IntegerAttr(0))});
    auto required = builder.getArrayAttr({builder.getDictionaryAttr(
        {builder.getNamedAttr("id", id),
         builder.getNamedAttr("kind", builder.getStringAttr("assert")),
         builder.getNamedAttr("location", location)})});
    builder.setInsertionPoint(body.getTerminator());
    mlir::OperationState state(definition.getLoc(), RuleOp::getOperationName());
    state.addOperands(condition);
    state.addTypes({condition.getType(), condition.getType()});
    state.addAttribute("name", builder.getStringAttr("forged_assert"));
    state.addAttribute("occurrence", registration);
    state.addAttribute("ac.required_checks", required);
    state.addRegion();
    auto rule = mlir::cast<RuleOp>(builder.create(state));
    auto *ruleBody = new mlir::Block;
    rule.getBody().push_back(ruleBody);
    auto argument =
        ruleBody->addArgument(condition.getType(), definition.getLoc());
    builder.setInsertionPointToEnd(ruleBody);
    builder.create<YieldOp>(definition.getLoc(),
                            mlir::ValueRange{argument, argument});
    builder.setInsertionPoint(body.getTerminator());
    mlir::OperationState expect(definition.getLoc(),
                                SourceExpectOp::getOperationName());
    expect.addOperands(rule->getResults());
    expect.addAttribute("kind", builder.getStringAttr("assert"));
    expect.addAttribute("location", location);
    expect.addAttribute("ac.check_id", id);
    expect.addAttribute("ac.message", builder.getStringAttr("forged"));
    builder.create(expect);
    return rule;
  }

  void expectStandaloneFinalValid(StagedLowering &staged) {
    EXPECT_TRUE(mlir::succeeded(capture([&] {
      return mlir::verify(*staged.candidate.module);
    }))) << diagnostics;
    EXPECT_TRUE(mlir::succeeded(capture([&] {
      return verifyHardwarePackage(*staged.candidate.module);
    }))) << diagnostics;
  }

  void removeWatchWrite(SourceTransactionOp watch, bool removeRead) {
    auto write = first<SourceStateWriteOp>(watch.getBody());
    write.erase();
    if (removeRead)
      first<SourceStateReadOp>(watch.getBody()).erase();
  }

  void setWatchGuard(SourceTransactionOp watch, mlir::Value predicate) {
    llvm::SmallVector<mlir::Value> operands(watch->getOperands());
    operands.push_back(predicate);
    watch->setOperands(operands);
    for (auto &region : watch->getRegions())
      region.front().addArgument(predicate.getType(), watch.getLoc());
    watch.getGuard().front().getTerminator()->setOperand(
        0, watch.getGuard().front().getArguments().back());
  }

  TableMapOp addNestedPureRegion(mlir::ModuleOp file) {
    auto owner = module(file);
    auto bump = named<SourceMethodOp>(owner, "bump");
    auto &body = bump.getBody().front();
    auto next = *std::next(body.getOps<BitsBinaryOp>().begin());
    auto one =
        (*owner.getBody().front().getOps<BitsConstantOp>().begin()).getValue();
    auto shape = builder.getArrayAttr({one});
    auto table = TableType::get(&context, shape, next.getResult().getType());
    builder.setInsertionPoint(next->getNextNode());
    mlir::OperationState splat(next.getLoc(), TableSplatOp::getOperationName());
    splat.addOperands(next.getResult());
    splat.addTypes(table);
    splat.addAttribute("shape", shape);
    auto *input = builder.create(splat);
    mlir::OperationState map(next.getLoc(), TableMapOp::getOperationName());
    map.addOperands(input->getResults());
    map.addTypes(table);
    map.addAttribute("shape", shape);
    map.addAttribute("operandSegmentSizes",
                     builder.getDenseI32ArrayAttr({1, 0}));
    map.addRegion();
    auto mapped = mlir::cast<TableMapOp>(builder.create(map));
    auto *local = new mlir::Block;
    mapped->getRegion(0).push_back(local);
    local->addArgument(BitsType::get(&context, one), mapped->getLoc());
    auto element =
        local->addArgument(next.getResult().getType(), mapped->getLoc());
    builder.setInsertionPointToEnd(local);
    mlir::OperationState add(mapped->getLoc(),
                             BitsBinaryOp::getOperationName());
    add.addOperands({element, element});
    add.addTypes(element.getType());
    add.addAttribute("opcode", builder.getStringAttr("add"));
    auto *twice = builder.create(add);
    mlir::OperationState yield(mapped->getLoc(), YieldOp::getOperationName());
    yield.addOperands(twice->getResults());
    builder.create(yield);
    return mapped;
  }
};

TEST_F(SampledTransactionLoweringTest,
       ProductionSeamAcceptsEverySupportedSampledTransactionShape) {
  for (unsigned shape = 0; shape < 5; ++shape) {
    SCOPED_TRACE(shape);
    auto staged = stage([&](mlir::ModuleOp file) {
      auto owner = module(file);
      auto watch = named<SourceTransactionOp>(owner, "watch");
      auto constants = owner.getBody().front().getOps<BitsConstantOp>();
      auto no = *std::next(constants.begin());
      if (shape == 1)
        setWatchGuard(watch, no.getResult());
      if (shape == 2)
        setWatchGuard(watch, owner.getBody().front().getArgument(2));
      if (shape == 3)
        removeWatchWrite(watch, false);
      if (shape == 4)
        removeWatchWrite(watch, true);
    });
    ASSERT_TRUE(staged);
    EXPECT_TRUE(mlir::succeeded(verify(*staged))) << diagnostics;
    expectOriginalUnchanged(*staged);
    auto index = shape == 0 ? transactionIndex(*staged, "run")
                            : transactionIndex(*staged, "watch");
    auto &prepared = staged->prepared.transactions[index];
    auto &record = staged->candidate.transactions[index];
    EXPECT_EQ(record.rule->getAttr("ac.sampled_admission"),
              prepared.expectedAdmission);
    EXPECT_EQ(record.rule->getAttr("ac.required_checks"),
              prepared.expectedRequiredChecks);
  }
}

TEST_F(SampledTransactionLoweringTest,
       MatchingLegalGuardPrefixMutationCannotForgeOriginalPredicate) {
  auto staged = stage();
  ASSERT_TRUE(staged);
  auto index = transactionIndex(*staged, "watch");
  auto &prepared = staged->prepared.transactions[index];
  auto rule = staged->candidate.transactions[index].rule;
  auto *yield = rule.getBody().front().getTerminator();
  auto oldRaw = yield->getOperand(prepared.fireResult + 1);
  auto replacement = rule.getBody().front().getArgument(prepared.resetCapture);
  ASSERT_EQ(oldRaw.getType(), replacement.getType());
  BitsCompareOp comparison;
  for (auto candidate : rule.getBody().front().getOps<BitsCompareOp>())
    if (candidate->getOperand(0) == oldRaw ||
        candidate->getOperand(1) == oldRaw) {
      comparison = candidate;
      break;
    }
  ASSERT_TRUE(comparison);
  for (unsigned operand = 0; operand < 2; ++operand)
    if (comparison->getOperand(operand) == oldRaw)
      comparison->setOperand(operand, replacement);
  yield->setOperand(prepared.fireResult + 1, replacement);
  rejects(*staged, "original raw guard predicate differs");
}

TEST_F(SampledTransactionLoweringTest,
       MatchingRequiredCheckAndExpectSpanCannotForgeOriginalLocation) {
  auto staged = stage();
  ASSERT_TRUE(staged);
  auto index = transactionIndex(*staged, "watch");
  auto &prepared = staged->prepared.transactions[index];
  auto rule = staged->candidate.transactions[index].rule;
  auto checks = rule->getAttrOfType<mlir::ArrayAttr>("ac.required_checks");
  ASSERT_EQ(checks.size(), 1u);
  mlir::NamedAttrList fields(mlir::cast<mlir::DictionaryAttr>(checks[0]));
  auto replacement = span(2);
  fields.set("location", replacement);
  auto changed = builder.getArrayAttr({fields.getDictionary(&context)});
  rule->setAttr("ac.required_checks", changed);
  auto id = fields.get("id");
  unsigned matches = 0;
  auto owner = rule->getParentOfType<ModuleOp>();
  for (auto expect : owner.getBody().front().getOps<SourceExpectOp>())
    if (expect->getAttr("ac.check_id") == id) {
      expect->setAttr("location", replacement);
      ++matches;
    }
  ASSERT_EQ(matches, 1u);
  rejects(*staged, "original transaction layout/domain/check metadata differs");
}

TEST_F(SampledTransactionLoweringTest,
       LegalCandidateExpressionCannotReplaceOriginalFinalRhs) {
  auto staged = stage();
  ASSERT_TRUE(staged);
  auto index = transactionIndex(*staged, "run");
  auto &prepared = staged->prepared.transactions[index];
  ASSERT_FALSE(prepared.outputs.empty());
  auto rule = staged->candidate.transactions[index].rule;
  auto *yield = rule.getBody().front().getTerminator();
  auto candidateIndex = prepared.plan->transaction->getNumResults();
  auto original = yield->getOperand(candidateIndex);
  mlir::Value replacement;
  for (auto argument : rule.getBody().front().getArguments())
    if (argument.getType() == original.getType() && argument != original) {
      replacement = argument;
      break;
    }
  ASSERT_TRUE(replacement);
  yield->setOperand(candidateIndex, replacement);
  rejects(*staged, "original final candidate RHS differs");
}

TEST_F(SampledTransactionLoweringTest,
       ChangedReadOrCallMapAndAllConsumersCannotSelfCertify) {
  for (bool callResult : {false, true}) {
    SCOPED_TRACE(callResult);
    auto staged = stage();
    ASSERT_TRUE(staged);
    auto index = transactionIndex(*staged, "run");
    const auto &plan = *staged->prepared.transactions[index].plan;
    auto &record = staged->candidate.transactions[index];
    InvocationFrameId frame;
    BoundValueId id;
    mlir::Value original;
    llvm::StringRef reason;
    if (callResult) {
      ASSERT_FALSE(plan.calls.empty());
      const auto &result = plan.calls.front().results.front();
      frame = plan.calls.front().caller;
      id = result.result;
      original = plan.values[id].value;
      reason = "original call yielded-result binding differs";
    } else {
      ASSERT_FALSE(plan.reads.empty());
      const auto &read = plan.reads.front();
      frame = read.frame;
      id = read.result;
      original = read.operation->getResult(0);
      reason = "original read selected version differs";
    }
    auto mapped = record.frames[frame].lookupOrNull(original);
    ASSERT_TRUE(mapped);
    mlir::Value replacement;
    for (auto argument : record.rule.getBody().front().getArguments())
      if (argument.getType() == mapped.getType() && argument != mapped) {
        replacement = argument;
        break;
      }
    ASSERT_TRUE(replacement);
    mapped.replaceAllUsesWith(replacement);
    record.frames[frame].map(original, replacement);
    record.boundValues[id] = replacement;
    rejects(*staged, reason);
  }
}

TEST_F(SampledTransactionLoweringTest,
       ActualCloneOpcodeAttributeOperandAndNestedYieldRemainSourceBound) {
  for (unsigned mutation = 0; mutation < 4; ++mutation) {
    SCOPED_TRACE(mutation);
    TableMapOp nested;
    auto staged = stage([&](mlir::ModuleOp file) {
      if (mutation == 3)
        nested = addNestedPureRegion(file);
    });
    ASSERT_TRUE(staged);
    auto index = transactionIndex(*staged, "run");
    const auto &plan = *staged->prepared.transactions[index].plan;
    auto &record = staged->candidate.transactions[index];
    if (mutation == 3) {
      mlir::Operation *clone = nullptr;
      for (auto &frame : record.frames)
        if ((clone = frame.lookupOrNull(nested.getOperation())))
          break;
      ASSERT_NE(clone, nullptr);
      auto mapped = mlir::cast<TableMapOp>(clone);
      auto *yield = mapped->getRegion(0).front().getTerminator();
      yield->setOperand(0, mapped->getRegion(0).front().getArgument(1));
      rejects(*staged, "actual clone");
      continue;
    }
    BitsBinaryOp original;
    BitsBinaryOp clone;
    for (auto [frameIndex, frame] : llvm::enumerate(record.frames))
      for (const auto &entry : frame.getOperationMap())
        if (auto binary = mlir::dyn_cast<BitsBinaryOp>(entry.first)) {
          original = binary;
          clone = mlir::dyn_cast<BitsBinaryOp>(entry.second);
          if (clone)
            break;
        }
    ASSERT_TRUE(original);
    ASSERT_TRUE(clone);
    if (mutation == 0)
      clone->setAttr("opcode", builder.getStringAttr("xor"));
    if (mutation == 1)
      clone->setAttr("source_mutation", builder.getUnitAttr());
    if (mutation == 2)
      clone->setOperand(0, clone->getOperand(1));
    rejects(*staged, "actual clone");
  }
}

TEST_F(SampledTransactionLoweringTest,
       WrongFrameParentResultOrdinalAndErasedTargetsRejectSafely) {
  for (unsigned mutation = 0; mutation < 4; ++mutation) {
    SCOPED_TRACE(mutation);
    auto staged = stage();
    ASSERT_TRUE(staged);
    auto index = transactionIndex(*staged, "run");
    const auto &plan = *staged->prepared.transactions[index].plan;
    auto &record = staged->candidate.transactions[index];
    if (mutation == 0) {
      auto owner = module(*staged->original);
      auto mappedOwner = mlir::cast<ModuleOp>(
          staged->candidate.packageMap.lookup(owner.getOperation()));
      auto mappedImport =
          *staged->candidate.module->getOps<ModuleImportOp>().begin();
      staged->candidate.packageMap.map(owner.getOperation(),
                                       mappedImport.getOperation());
      rejects(*staged, "original definition owner differs");
      staged->candidate.packageMap.map(owner.getOperation(),
                                       mappedOwner.getOperation());
      continue;
    }
    if (mutation == 1) {
      std::optional<unsigned> sourceFrame, wrongFrame;
      for (auto [frame, mapping] : llvm::enumerate(record.frames))
        if (!mapping.getOperationMap().empty()) {
          if (!sourceFrame)
            sourceFrame = frame;
          else if (frame != *sourceFrame) {
            wrongFrame = frame;
            break;
          }
        }
      ASSERT_TRUE(sourceFrame);
      ASSERT_TRUE(wrongFrame);
      auto source =
          record.frames[*sourceFrame].getOperationMap().begin()->first;
      auto wrong = record.frames[*wrongFrame].getOperationMap().begin()->second;
      record.frames[*sourceFrame].map(source, wrong);
      rejects(*staged, "transaction original-source proof failed");
      continue;
    }
    if (mutation == 2) {
      ASSERT_GE(plan.transaction->getNumResults(), 2u);
      staged->candidate.packageMap.map(plan.transaction->getResult(0),
                                       record.rule->getResult(1));
      rejects(*staged, "transaction original-source proof failed");
      continue;
    }
    mlir::Operation *original = nullptr;
    mlir::Operation *clone = nullptr;
    unsigned mappedFrame = 0;
    for (auto [frameIndex, frame] : llvm::enumerate(record.frames))
      for (const auto &entry : frame.getOperationMap())
        if (mlir::isa<BitsBinaryOp>(entry.first)) {
          original = entry.first;
          clone = entry.second;
          mappedFrame = frameIndex;
          break;
        }
    ASSERT_NE(original, nullptr);
    ASSERT_NE(clone, nullptr);
    ASSERT_GT(clone->getNumResults(), 0u);
    ASSERT_GT(clone->getNumOperands(), 0u);
    clone->getResult(0).replaceAllUsesWith(clone->getOperand(0));
    clone->erase();
    EXPECT_EQ(record.frames[mappedFrame].lookupOrNull(original), clone);
    rejects(*staged, "invocation map contains erased clone");
  }
}

TEST_F(SampledTransactionLoweringTest,
       OriginalHistoricalCompanionSetRejectsMissingAndExtraDefinitions) {
  for (bool extra : {false, true}) {
    SCOPED_TRACE(extra);
    auto staged = stage([&](mlir::ModuleOp file) {
      if (extra)
        addPlainDefinition(file, "Plain");
      else
        addIsolatedUnwrittenStateDefinition(file);
    });
    ASSERT_TRUE(staged);
    if (!extra) {
      auto idle = module(*staged->candidate.module, "Idle");
      ASSERT_TRUE(idle->hasAttr("ac.transaction_wiring"));
      idle->removeAttr("ac.transaction_wiring");
      rejects(*staged,
              "exact original affected-definition companion inventory differs");
      continue;
    }
    auto plain = module(*staged->candidate.module, "Plain");
    plain->setAttr(
        "ac.transaction_wiring",
        builder.getDictionaryAttr(
            {builder.getNamedAttr("states", builder.getArrayAttr({})),
             builder.getNamedAttr("incoming", builder.getArrayAttr({})),
             builder.getNamedAttr("writers", builder.getArrayAttr({}))}));
    rejects(*staged,
            "exact original affected-definition companion inventory differs");
  }
}

TEST_F(SampledTransactionLoweringTest,
       OtherwiseValidFreshAssertDffeAndNestedOpCannotEscapeSourceProof) {
  for (unsigned mutation = 0; mutation < 3; ++mutation) {
    SCOPED_TRACE(mutation);
    auto staged = stage();
    ASSERT_TRUE(staged);
    auto owner = module(*staged->candidate.module);
    if (mutation == 0) {
      addForgedAssert(owner);
    } else if (mutation == 1) {
      auto original = instance(owner, "hold");
      auto *forged = original->clone();
      forged->setAttr("instance_name", builder.getStringAttr("forged_hold"));
      forged->setAttr("occurrence", occurrence("Owner", 82));
      owner.getBody().front().getOperations().insert(
          owner.getBody().front().getTerminator()->getIterator(), forged);
    } else {
      auto index = transactionIndex(*staged, "run");
      auto rule = staged->candidate.transactions[index].rule;
      mlir::Value left, right;
      for (auto first : rule.getBody().front().getArguments())
        for (auto second : rule.getBody().front().getArguments())
          if (first != second && first.getType() == second.getType() &&
              mlir::isa<BitsType>(first.getType())) {
            left = first;
            right = second;
          }
      ASSERT_TRUE(left);
      ASSERT_TRUE(right);
      builder.setInsertionPoint(rule.getBody().front().getTerminator());
      mlir::OperationState forged(rule.getLoc(),
                                  BitsBinaryOp::getOperationName());
      forged.addOperands({left, right});
      forged.addTypes(left.getType());
      forged.addAttribute("opcode", builder.getStringAttr("add"));
      builder.create(forged);
    }
    expectStandaloneFinalValid(*staged);
    EXPECT_TRUE(mlir::failed(verify(*staged)));
    EXPECT_NE(diagnostics.find("transaction original-source proof failed"),
              std::string::npos)
        << diagnostics;
    EXPECT_NE(diagnostics.find("operation inventory differs"),
              std::string::npos)
        << diagnostics;
    expectOriginalUnchanged(*staged);
  }
}

TEST_F(SampledTransactionLoweringTest,
       RealTwoOwnerDataSwapFailsSharedPhysicalWiringProof) {
  auto staged = stage();
  ASSERT_TRUE(staged);
  auto owner = module(*staged->candidate.module);
  auto count = instance(owner, "count");
  auto saved = instance(owner, "saved");
  auto countData = count->getOperand(3);
  count->setOperand(3, saved->getOperand(3));
  saved->setOperand(3, countData);
  EXPECT_TRUE(mlir::succeeded(capture([&] {
    return mlir::verify(*staged->candidate.module);
  }))) << diagnostics;
  EXPECT_TRUE(mlir::failed(capture(
      [&] { return verifyHardwarePackage(*staged->candidate.module); })));
  EXPECT_FALSE(diagnostics.empty());
  EXPECT_TRUE(mlir::failed(verify(*staged)));
  EXPECT_FALSE(diagnostics.empty());
  expectOriginalUnchanged(*staged);
}

TEST_F(SampledTransactionLoweringTest,
       ProductionSeamFinalVerificationRejectsAuthenticUnusedSourceCycle) {
  BitsBinaryOp originalCycle;
  auto staged = stage([&](mlir::ModuleOp file) {
    auto owner = module(file);
    auto &body = owner.getBody().front();
    auto seed = body.getArgument(0);
    builder.setInsertionPoint(body.getTerminator());
    mlir::OperationState state(owner.getLoc(),
                               BitsBinaryOp::getOperationName());
    state.addOperands({seed, seed});
    state.addTypes(seed.getType());
    state.addAttribute("opcode", builder.getStringAttr("add"));
    originalCycle = mlir::cast<BitsBinaryOp>(builder.create(state));
    originalCycle->setOperand(0, originalCycle.getResult());
  });
  ASSERT_TRUE(staged)
      << "analysis, preparation and materialization must accept the authentic "
         "source graph before final dependency verification";
  auto mappedCycle = mlir::dyn_cast_or_null<BitsBinaryOp>(
      staged->candidate.packageMap.lookupOrNull(originalCycle.getOperation()));
  ASSERT_TRUE(mappedCycle);
  EXPECT_EQ(mappedCycle->getOperand(0), mappedCycle.getResult());
  EXPECT_TRUE(mlir::failed(verify(*staged)));
  EXPECT_NE(diagnostics.find("combinational cycle"), std::string::npos)
      << diagnostics;
  EXPECT_EQ(diagnostics.find("transaction original-source proof failed"),
            std::string::npos)
      << diagnostics;
  expectOriginalUnchanged(*staged);
}

TEST_F(SampledTransactionLoweringTest,
       ProductionSeamAcceptsValidForwardModuleDagIndependentOfBlockOrder) {
  BitsBinaryOp originalProducer, originalConsumer;
  auto staged = stage([&](mlir::ModuleOp file) {
    auto owner = module(file);
    auto &body = owner.getBody().front();
    auto seed = body.getArgument(0);
    builder.setInsertionPoint(body.getTerminator());
    mlir::OperationState producer(owner.getLoc(),
                                  BitsBinaryOp::getOperationName());
    producer.addOperands({seed, seed});
    producer.addTypes(seed.getType());
    producer.addAttribute("opcode", builder.getStringAttr("add"));
    originalProducer = mlir::cast<BitsBinaryOp>(builder.create(producer));
    mlir::OperationState consumer(owner.getLoc(),
                                  BitsBinaryOp::getOperationName());
    consumer.addOperands({originalProducer.getResult(), seed});
    consumer.addTypes(seed.getType());
    consumer.addAttribute("opcode", builder.getStringAttr("add"));
    originalConsumer = mlir::cast<BitsBinaryOp>(builder.create(consumer));
    originalConsumer->moveBefore(originalProducer);
  });
  ASSERT_TRUE(staged)
      << "native verification, analysis, preparation and materialization must "
         "admit a valid forward module DAG";
  auto mappedProducer = mlir::dyn_cast_or_null<BitsBinaryOp>(
      staged->candidate.packageMap.lookupOrNull(
          originalProducer.getOperation()));
  auto mappedConsumer = mlir::dyn_cast_or_null<BitsBinaryOp>(
      staged->candidate.packageMap.lookupOrNull(
          originalConsumer.getOperation()));
  ASSERT_TRUE(mappedProducer);
  ASSERT_TRUE(mappedConsumer);
  EXPECT_TRUE(mappedConsumer->isBeforeInBlock(mappedProducer));
  EXPECT_EQ(mappedConsumer->getOperand(0), mappedProducer.getResult());
  EXPECT_TRUE(mlir::succeeded(verify(*staged))) << diagnostics;
  expectOriginalUnchanged(*staged);
}

TEST_F(SampledTransactionLoweringTest,
       EveryBudgetAndSourceProofFailureLeavesOriginalObjectUntouched) {
  auto original = parse();
  ASSERT_TRUE(original);
  auto text = print(original->getOperation());
  auto pointers = operations(*original);
  auto analysis = std::make_unique<SourceTransactionStateAnalysis>(
      original->getOperation());
  ASSERT_TRUE(analysis->isValid()) << diagnostics;
  HardwareSourceCheckLimits tinyLimits;
  tinyLimits.maxWorkUnits = 1;
  detail::SourceCheckBudget tiny(tinyLimits);
  auto prepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*original, *analysis, tiny);
  });
  EXPECT_TRUE(mlir::failed(prepared));
  EXPECT_NE(diagnostics.find("budget"), std::string::npos) << diagnostics;
  EXPECT_EQ(print(original->getOperation()), text);
  EXPECT_EQ(operations(*original), pointers);

  auto staged = stage();
  ASSERT_TRUE(staged);
  auto index = transactionIndex(*staged, "run");
  staged->candidate.transactions[index].rule->removeAttr(
      "ac.sampled_admission");
  rejects(*staged, "original transaction layout/domain/check metadata differs");

  detail::SourceCheckBudget verifyTiny(tinyLimits);
  EXPECT_TRUE(mlir::failed(capture([&] {
    return acir::detail::verifyTransactionMaterialization(
        staged->prepared, staged->candidate, verifyTiny);
  })));
  EXPECT_NE(diagnostics.find("budget"), std::string::npos) << diagnostics;
  expectOriginalUnchanged(*staged);
}

} // namespace
} // namespace acir::ac
