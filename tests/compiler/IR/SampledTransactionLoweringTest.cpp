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
  struct AuthoredEffects {
    SourceExpectOp methodCheck;
    SourceObserveOp methodObservation;
    SourceExpectOp directCheck;
    SourceObserveOp directObservation;
  };
  struct AuthoredBranch {
    SourceBranchOp operation;
    SourceStateWriteOp whenTrue, whenFalse;
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

  AuthoredEffects addAuthoredEffects(mlir::ModuleOp file) {
    auto owner = module(file);
    auto bump = named<SourceMethodOp>(owner, "bump");
    auto &methodBody = bump.getBody().front();
    auto methodPath = methodBody.getArgument(1);
    auto old = first<SourceStateReadOp>(bump.getBody()).getResult();
    auto constants = owner.getBody().front().getOps<BitsConstantOp>();
    auto falseCondition = (*std::next(constants.begin())).clone();
    methodBody.getOperations().insert(methodBody.getTerminator()->getIterator(),
                                      falseCondition.getOperation());
    builder.setInsertionPoint(methodBody.getTerminator());
    mlir::OperationState methodCheckState(bump.getLoc(),
                                          SourceExpectOp::getOperationName());
    methodCheckState.addOperands({falseCondition->getResult(0), methodPath});
    methodCheckState.addAttribute("kind", builder.getStringAttr("assert"));
    methodCheckState.addAttribute("location", span(20));
    methodCheckState.addAttribute("occurrence", occurrence("Owner", 20));
    methodCheckState.addAttribute("ac.message",
                                  builder.getStringAttr("method-check"));
    auto methodCheck =
        mlir::cast<SourceExpectOp>(builder.create(methodCheckState));
    auto valueItem = [&](unsigned ordinal) {
      return builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr("value")),
           builder.getNamedAttr("ordinal",
                                builder.getI32IntegerAttr(ordinal))});
    };
    mlir::OperationState methodObservationState(
        bump.getLoc(), SourceObserveOp::getOperationName());
    methodObservationState.addOperands(
        {methodPath, old, methodBody.getArgument(0), old});
    methodObservationState.addAttribute("kind", builder.getStringAttr("log"));
    methodObservationState.addAttribute(
        "spec",
        builder.getDictionaryAttr(
            {builder.getNamedAttr("level", builder.getStringAttr("info")),
             builder.getNamedAttr("event", builder.getStringAttr("method")),
             builder.getNamedAttr(
                 "items", builder.getArrayAttr(
                              {valueItem(0), valueItem(1), valueItem(2)}))}));
    methodObservationState.addAttribute("occurrence", occurrence("Owner", 21));
    methodObservationState.addAttribute("ac.location", span(21));
    auto methodObservation =
        mlir::cast<SourceObserveOp>(builder.create(methodObservationState));

    auto run = named<SourceTransactionOp>(owner, "run");
    auto &runBody = run.getBody().front();
    auto path = runBody.getArgument(0);
    auto calls = llvm::to_vector(runBody.getOps<SourceMethodCallOp>());
    builder.setInsertionPoint(runBody.getTerminator());
    mlir::OperationState directCheckState(run.getLoc(),
                                          SourceExpectOp::getOperationName());
    directCheckState.addOperands({path, path});
    directCheckState.addAttribute("kind", builder.getStringAttr("assert"));
    directCheckState.addAttribute("location", span(22));
    directCheckState.addAttribute("occurrence", occurrence("Owner", 22));
    auto directCheck =
        mlir::cast<SourceExpectOp>(builder.create(directCheckState));
    mlir::OperationState directObservationState(
        run.getLoc(), SourceObserveOp::getOperationName());
    directObservationState.addOperands({path, calls.front().getResult(0)});
    directObservationState.addAttribute("kind",
                                        builder.getStringAttr("report"));
    directObservationState.addAttribute(
        "spec", builder.getDictionaryAttr({builder.getNamedAttr(
                    "name", builder.getStringAttr("direct"))}));
    directObservationState.addAttribute("occurrence", occurrence("Owner", 23));
    directObservationState.addAttribute("ac.location", span(23));
    auto directObservation =
        mlir::cast<SourceObserveOp>(builder.create(directObservationState));
    return {methodCheck, methodObservation, directCheck, directObservation};
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

  AuthoredBranch addAuthoredBranch(mlir::ModuleOp file) {
    auto owner = module(file);
    auto run = named<SourceTransactionOp>(owner, "run");
    auto &body = run.getBody().front();
    auto constants = owner.getBody().front().getOps<BitsConstantOp>();
    auto zero = *std::next(constants.begin(), 3);
    llvm::SmallVector<mlir::Value> captures(run->getOperands());
    captures.push_back(zero.getResult());
    run->setOperands(captures);
    for (auto &region : run->getRegions())
      region.front().addArgument(zero.getResult().getType(), run.getLoc());

    mlir::OperationState state(run.getLoc(),
                               SourceBranchOp::getOperationName());
    state.addOperands(body.getArgument(0));
    state.addTypes(body.getArgument(1).getType());
    state.addAttribute("occurrence", occurrence("Owner", 24));
    state.addAttribute("ac.condition_location", span(24));
    state.addRegion();
    state.addRegion();
    builder.setInsertionPointToStart(&body);
    auto split = mlir::cast<SourceBranchOp>(builder.create(state));
    SourceStateWriteOp writes[2];
    for (unsigned arm = 0; arm != 2; ++arm) {
      auto *block = new mlir::Block;
      split->getRegion(arm).push_back(block);
      builder.setInsertionPointToEnd(block);
      mlir::Value value =
          arm == 0 ? body.getArgument(1) : body.getArguments().back();
      mlir::OperationState write(run.getLoc(),
                                 SourceStateWriteOp::getOperationName());
      write.addOperands({body.getArgument(0), value});
      write.addAttribute("state", builder.getStringAttr("saved"));
      write.addAttribute("selectors", builder.getArrayAttr({}));
      write.addAttribute("occurrence", occurrence("Owner", 25 + arm));
      writes[arm] = mlir::cast<SourceStateWriteOp>(builder.create(write));
      mlir::OperationState yield(run.getLoc(), YieldOp::getOperationName());
      yield.addOperands(value);
      builder.create(yield);
    }
    auto firstCall = *body.getOps<SourceMethodCallOp>().begin();
    firstCall->setOperand(1, split.getResult(0));
    for (auto write :
         llvm::make_early_inc_range(body.getOps<SourceStateWriteOp>()))
      if (write.getState() == "saved")
        write.erase();
    return {split, writes[0], writes[1]};
  }
  AuthoredBranch addConditionalArmEffects(mlir::ModuleOp file) {
    auto effects = addAuthoredEffects(file);
    auto split = addAuthoredBranch(file);
    auto run = named<SourceTransactionOp>(module(file), "run");
    auto &body = run.getBody().front();
    auto &arm = split.operation.getThenRegion().front();
    auto firstCall = *body.getOps<SourceMethodCallOp>().begin();
    auto *conditional = firstCall->clone();
    conditional->setOperand(1, body.getArgument(1));
    conditional->setAttr("occurrence", occurrence("Owner", 27));
    arm.getOperations().insert(arm.getTerminator()->getIterator(), conditional);
    effects.directCheck->moveBefore(arm.getTerminator());
    effects.directObservation->setOperand(1, body.getArgument(1));
    effects.directObservation->moveBefore(arm.getTerminator());
    return split;
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
       BodyEffectsMaterializeExactExpandedLayoutAndSourceIdentities) {
  AuthoredEffects source;
  auto staged =
      stage([&](mlir::ModuleOp file) { source = addAuthoredEffects(file); });
  ASSERT_TRUE(staged);
  EXPECT_TRUE(mlir::succeeded(verify(*staged))) << diagnostics;
  expectOriginalUnchanged(*staged);
  auto index = transactionIndex(*staged, "run");
  const auto &prepared = staged->prepared.transactions[index];
  const auto &plan = *prepared.plan;
  auto &record = staged->candidate.transactions[index];
  ASSERT_EQ(plan.bodyChecks.size(), 3u);
  ASSERT_EQ(plan.observations.size(), 3u);
  ASSERT_EQ(record.bodyChecks.size(), plan.bodyChecks.size());
  ASSERT_EQ(record.observations.size(), plan.observations.size());
  auto metadata =
      record.rule->getAttrOfType<mlir::DictionaryAttr>("ac.sampled_admission");
  ASSERT_TRUE(metadata);
  for (llvm::StringRef field : {"clock_capture", "reset_capture", "fire_result",
                                "controls", "body_checks", "observations"})
    EXPECT_TRUE(metadata.get(field)) << field.str();
  EXPECT_EQ(metadata.size(), 6u);
  auto guardRows = metadata.getAs<mlir::ArrayAttr>("controls");
  auto checkRows = metadata.getAs<mlir::ArrayAttr>("body_checks");
  auto observationRows = metadata.getAs<mlir::ArrayAttr>("observations");
  ASSERT_EQ(guardRows.size(), plan.controls.size());
  ASSERT_EQ(checkRows.size(), plan.bodyChecks.size());
  ASSERT_EQ(observationRows.size(), plan.observations.size());
  EXPECT_EQ(prepared.totalResults, record.rule.getNumResults());
  uint64_t expectedCheckBase = prepared.fireResult + 1;
  for (const auto &observation : plan.observations)
    expectedCheckBase += 1 + observation.values.size();
  EXPECT_EQ(prepared.checkBase, expectedCheckBase);

  uint64_t observationResult = prepared.fireResult + 1;
  for (auto [ordinal, terminal] : llvm::enumerate(record.observations)) {
    const auto &sourceRecord = plan.observations[ordinal];
    EXPECT_EQ(terminal.getPath(), record.rule.getResult(observationResult));
    EXPECT_EQ(terminal.getValues().size(), sourceRecord.values.size());
    for (auto [valueOrdinal, value] : llvm::enumerate(terminal.getValues()))
      EXPECT_EQ(value,
                record.rule.getResult(observationResult + 1 + valueOrdinal));
    auto id =
        terminal->getAttrOfType<mlir::DictionaryAttr>("ac.observation_id");
    ASSERT_TRUE(id);
    EXPECT_EQ(id.getAs<mlir::IntegerAttr>("ordinal").getInt(), ordinal);
    EXPECT_EQ(id.get("site"), sourceRecord.occurrence);
    EXPECT_EQ(
        id.get("registration"),
        plan.transaction->getAttrOfType<mlir::DictionaryAttr>("occurrence"));
    auto row = mlir::cast<mlir::DictionaryAttr>(observationRows[ordinal]);
    auto rowID = row.getAs<mlir::DictionaryAttr>("id");
    EXPECT_EQ(id.get("invocation"), rowID.get("invocation"));
    EXPECT_EQ(terminal.getKindAttr(), sourceRecord.kind);
    EXPECT_EQ(terminal.getSpec(), sourceRecord.spec);
    EXPECT_EQ(terminal->getAttr("ac.location"), sourceRecord.location);
    EXPECT_EQ(terminal.getLoc(), sourceRecord.operation->getLoc());
    observationResult += 1 + sourceRecord.values.size();
  }
  EXPECT_EQ(observationResult, prepared.checkBase);

  for (auto [ordinal, terminal] : llvm::enumerate(record.bodyChecks)) {
    const auto &sourceRecord = plan.bodyChecks[ordinal];
    auto conditionResult =
        prepared.checkBase + 2 * (plan.controls.size() + ordinal);
    EXPECT_EQ(terminal.getCondition(), record.rule.getResult(conditionResult));
    EXPECT_EQ(terminal.getPath(), record.rule.getResult(conditionResult + 1));
    EXPECT_TRUE(terminal.getSampling().empty());
    auto id = terminal->getAttrOfType<mlir::DictionaryAttr>("ac.check_id");
    ASSERT_TRUE(id);
    EXPECT_EQ(id.getAs<mlir::IntegerAttr>("obligation").getInt(),
              plan.controls.size() + ordinal);
    EXPECT_EQ(id.get("check"), sourceRecord.occurrence);
    EXPECT_EQ(
        id.get("registration"),
        plan.transaction->getAttrOfType<mlir::DictionaryAttr>("occurrence"));
    EXPECT_EQ(terminal->getAttr("location"), sourceRecord.location);
    EXPECT_EQ(terminal->getAttrOfType<mlir::StringAttr>("ac.message"),
              sourceRecord.message);
    EXPECT_EQ(terminal.getLoc(), sourceRecord.operation->getLoc());
  }
  EXPECT_EQ(plan.bodyChecks[0].operation, source.methodCheck);
  EXPECT_EQ(plan.bodyChecks[1].operation, source.methodCheck);
  EXPECT_EQ(plan.bodyChecks[2].operation, source.directCheck);
  EXPECT_EQ(plan.observations[0].operation, source.methodObservation);
  EXPECT_EQ(plan.observations[1].operation, source.methodObservation);
  EXPECT_EQ(plan.observations[2].operation, source.directObservation);
  auto firstInvocation = mlir::cast<mlir::DictionaryAttr>(checkRows[0])
                             .getAs<mlir::ArrayAttr>("invocation");
  auto secondInvocation = mlir::cast<mlir::DictionaryAttr>(checkRows[1])
                              .getAs<mlir::ArrayAttr>("invocation");
  auto directInvocation = mlir::cast<mlir::DictionaryAttr>(checkRows[2])
                              .getAs<mlir::ArrayAttr>("invocation");
  EXPECT_FALSE(firstInvocation.empty());
  EXPECT_FALSE(secondInvocation.empty());
  EXPECT_NE(firstInvocation, secondInvocation);
  EXPECT_TRUE(directInvocation.empty());
}

TEST_F(SampledTransactionLoweringTest,
       BodyEffectSourceAuthorityRejectsIndependentTerminalMutations) {
  for (unsigned mutation = 0; mutation < 14; ++mutation) {
    SCOPED_TRACE(mutation);
    auto staged = stage([&](mlir::ModuleOp file) { addAuthoredEffects(file); });
    ASSERT_TRUE(staged);
    auto index = transactionIndex(*staged, "run");
    const auto &prepared = staged->prepared.transactions[index];
    auto &record = staged->candidate.transactions[index];
    ASSERT_GE(record.bodyChecks.size(), 3u);
    ASSERT_GE(record.observations.size(), 3u);
    auto check = record.bodyChecks[0];
    auto observation = record.observations[0];
    if (mutation == 0)
      check->setOperand(0, check.getPath());
    if (mutation == 1)
      observation->setOperand(2, observation->getOperand(1));
    if (mutation == 2)
      observation->setAttr("kind", builder.getStringAttr("report"));
    if (mutation == 3)
      observation->setAttr("spec",
                           builder.getDictionaryAttr({builder.getNamedAttr(
                               "name", builder.getStringAttr("forged"))}));
    if (mutation == 4)
      check->setAttr("ac.message", builder.getStringAttr("forged"));
    if (mutation == 5)
      observation->setAttr("ac.location", span(99));
    if (mutation == 6)
      observation->setLoc(
          mlir::FileLineColLoc::get(&context, "forged.py", 9, 3));
    if (mutation >= 7 && mutation <= 9) {
      auto id =
          observation->getAttrOfType<mlir::DictionaryAttr>("ac.observation_id");
      mlir::NamedAttrList fields(id);
      if (mutation == 7)
        fields.set("ordinal", builder.getI64IntegerAttr(99));
      if (mutation == 8)
        fields.set("owner",
                   builder.getDictionaryAttr({builder.getNamedAttr(
                       "instance_path",
                       builder.getArrayAttr({occurrence("Owner", 99)}))}));
      if (mutation == 9)
        fields.set("registration", occurrence("Owner", 99));
      observation->setAttr("ac.observation_id", fields.getDictionary(&context));
    }
    if (mutation == 10)
      record.observations[0] = {};
    if (mutation == 11)
      std::swap(record.observations[0], record.observations[1]);
    if (mutation == 12)
      observation->setOperand(0, record.rule.getResult(prepared.fireResult));
    if (mutation == 13) {
      auto metadata = record.rule->getAttrOfType<mlir::DictionaryAttr>(
          "ac.sampled_admission");
      llvm::SmallVector<mlir::Attribute> rows(
          metadata.getAs<mlir::ArrayAttr>("body_checks").begin(),
          metadata.getAs<mlir::ArrayAttr>("body_checks").end());
      mlir::NamedAttrList changed(mlir::cast<mlir::DictionaryAttr>(rows[0]));
      changed.set("invocation", builder.getArrayAttr({}));
      rows[0] = changed.getDictionary(&context);
      mlir::NamedAttrList fields(metadata);
      fields.set("body_checks", builder.getArrayAttr(rows));
      record.rule->setAttr("ac.sampled_admission",
                           fields.getDictionary(&context));
    }
    rejects(*staged, "transaction original-source proof failed");
  }
}

TEST_F(SampledTransactionLoweringTest,
       BodyEffectTerminalInventoryRejectsMissingExtraAndReorderedOperations) {
  for (unsigned mutation = 0; mutation < 4; ++mutation) {
    SCOPED_TRACE(mutation);
    auto staged = stage([&](mlir::ModuleOp file) { addAuthoredEffects(file); });
    ASSERT_TRUE(staged);
    auto index = transactionIndex(*staged, "run");
    auto &record = staged->candidate.transactions[index];
    auto owner = record.rule->getParentOfType<ModuleOp>();
    if (mutation == 0)
      record.bodyChecks.front().erase();
    if (mutation == 1) {
      auto *copy = record.bodyChecks.front()->clone();
      owner.getBody().front().getOperations().insert(
          owner.getBody().front().getTerminator()->getIterator(), copy);
    }
    if (mutation == 2)
      record.observations.front().erase();
    if (mutation == 3) {
      auto first = record.bodyChecks[0].getOperation();
      auto second = record.observations[0].getOperation();
      first->moveAfter(second);
    }
    rejects(*staged, "transaction original-source proof failed");
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
  rejects(*staged, "original control raw/path/suffix differs");
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
       BranchDataIntentAndResultSelectionsRemainSourceAuthenticated) {
  for (unsigned mutation = 0; mutation != 10; ++mutation) {
    SCOPED_TRACE(mutation);
    AuthoredBranch source;
    auto staged =
        stage([&](mlir::ModuleOp file) { source = addAuthoredBranch(file); });
    ASSERT_TRUE(staged);
    auto index = transactionIndex(*staged, "run");
    auto &prepared = staged->prepared.transactions[index];
    const auto &plan = *prepared.plan;
    auto &record = staged->candidate.transactions[index];
    ASSERT_EQ(plan.branches.size(), 1u);
    ASSERT_EQ(plan.joins.size(), 1u);
    ASSERT_EQ(plan.branchResults.size(), 1u);
    const auto &join = plan.joins.front();
    auto data = mlir::cast<BitsSelectOp>(
        record.versions[join.result].data.getDefiningOp());
    auto intent = mlir::cast<BitsSelectOp>(
        record.versions[join.result].rawIntent.getDefiningOp());
    auto &resultRecord = record.branchResults.front();
    ASSERT_EQ(resultRecord.generated.size(), 1u);
    auto result =
        mlir::dyn_cast<BitsSelectOp>(resultRecord.value.getDefiningOp());
    ASSERT_EQ(resultRecord.generated.front(), result.getOperation());
    ASSERT_EQ(resultRecord.value, result.getResult());
    ASSERT_TRUE(data);
    ASSERT_TRUE(intent);
    ASSERT_TRUE(result);
    EXPECT_EQ(data.getCondition(), intent.getCondition());
    EXPECT_EQ(data.getCondition(), result.getCondition());
    EXPECT_EQ(data->getOperand(1), record.versions[join.whenTrue].data);
    EXPECT_EQ(data->getOperand(2), record.versions[join.whenFalse].data);
    EXPECT_EQ(intent->getOperand(1), record.versions[join.whenTrue].rawIntent);
    EXPECT_EQ(intent->getOperand(2), record.versions[join.whenFalse].rawIntent);

    if (mutation == 0) {
      mlir::Value replacement;
      for (auto argument : record.rule.getBody().front().getArguments())
        if (argument.getType() == data.getCondition().getType() &&
            argument != data.getCondition()) {
          replacement = argument;
          break;
        }
      ASSERT_TRUE(replacement);
      data->setOperand(0, replacement);
    } else if (mutation == 1) {
      data->setOperands(
          {data.getCondition(), data->getOperand(2), data->getOperand(1)});
    } else if (mutation == 2) {
      intent->setOperands({intent.getCondition(), intent->getOperand(2),
                           intent->getOperand(1)});
    } else if (mutation == 3) {
      result->setOperands({result.getCondition(), result->getOperand(2),
                           result->getOperand(1)});
    } else if (mutation == 4) {
      auto original = plan.branchResults.front().result;
      auto oldValue = resultRecord.value;
      auto replacement = data.getResult();
      // Make the candidate and every locator agree on the substitution. The
      // independent proof must still authenticate the original branch result,
      // rather than rejecting only an erased or stale auxiliary mapping.
      oldValue.replaceAllUsesWith(replacement);
      auto replaceMapping = [&](mlir::IRMapping &mapping) {
        llvm::SmallVector<mlir::Value> keys;
        for (const auto &entry : mapping.getValueMap())
          if (entry.second == oldValue)
            keys.push_back(entry.first);
        for (auto key : keys)
          mapping.map(key, replacement);
      };
      replaceMapping(staged->candidate.packageMap);
      for (auto &frame : record.frames)
        replaceMapping(frame);
      for (auto &value : record.boundValues)
        if (value == oldValue)
          value = replacement;
      record.frames[plan.branchResults.front().frame].map(
          plan.values[original].value, replacement);
      resultRecord.value = replacement;
      resultRecord.generated = {data.getOperation()};
      result.erase();
    } else {
      auto output = llvm::find_if(prepared.outputs, [&](const auto &candidate) {
        return candidate.binding.owner == join.owner;
      });
      ASSERT_NE(output, prepared.outputs.end());
      auto *yield = record.rule.getBody().front().getTerminator();
      if (mutation == 5) {
        yield->setOperand(output->enableResult,
                          record.versions[join.result].rawIntent);
      } else if (mutation == 6) {
        yield->setOperand(output->enableResult,
                          yield->getOperand(prepared.fireResult));
      } else if (mutation == 7) {
        auto knownOne = record.versions[join.whenTrue].rawIntent;
        record.versions[join.result].rawIntent.replaceAllUsesWith(knownOne);
        record.versions[join.result].rawIntent = knownOne;
      } else if (mutation == 8) {
        prepared.outputs.erase(output);
      } else {
        auto replacement =
            llvm::find_if(prepared.outputs, [&](const auto &candidate) {
              return candidate.binding.owner != join.owner;
            });
        ASSERT_NE(replacement, prepared.outputs.end());
        output->binding = replacement->binding;
      }
    }
    rejects(*staged, "");
    if (mutation == 4) {
      EXPECT_NE(diagnostics.find("transaction original-source proof failed"),
                std::string::npos)
          << diagnostics;
      EXPECT_EQ(diagnostics.find("not live"), std::string::npos) << diagnostics;
      EXPECT_EQ(diagnostics.find("erased"), std::string::npos) << diagnostics;
    }
  }
}

TEST_F(SampledTransactionLoweringTest,
       ConditionalControlAndEffectPathsUseDedicatedSlotsAndExactFire) {
  auto staged = stage([&](mlir::ModuleOp file) { addConditionalArmEffects(file); });
  ASSERT_TRUE(staged);
  EXPECT_TRUE(mlir::succeeded(verify(*staged))) << diagnostics;
  expectOriginalUnchanged(*staged);
  auto index = transactionIndex(*staged, "run");
  const auto &prepared = staged->prepared.transactions[index];
  const auto &plan = *prepared.plan;
  auto &record = staged->candidate.transactions[index];
  ASSERT_EQ(plan.selections.size(), 1u);
  EXPECT_EQ(plan.controls.size(), plan.guards.size() + plan.selections.size());
  EXPECT_EQ(record.controls.size(), plan.controls.size());
  EXPECT_EQ(record.prefixes.size(), plan.controls.size() + 1);
  EXPECT_EQ(record.selections.size(), plan.selections.size());
  EXPECT_TRUE(record.rule.getResult(prepared.fireResult).use_empty());
  auto *yield = record.rule.getBody().front().getTerminator();
  auto fire = yield->getOperand(prepared.fireResult);
  auto metadata = record.rule->getAttrOfType<mlir::DictionaryAttr>(
      "ac.sampled_admission");
  EXPECT_FALSE(metadata.get("guards"));
  ASSERT_TRUE(metadata.getAs<mlir::ArrayAttr>("controls"));
  EXPECT_EQ(metadata.size(), 6u);
  for (auto [ordinal, control] : llvm::enumerate(record.controls)) {
    auto id = control.terminal->getAttrOfType<mlir::DictionaryAttr>("ac.check_id");
    ASSERT_TRUE(id);
    EXPECT_EQ(id.getAs<mlir::IntegerAttr>("obligation").getInt(), ordinal);
  }
  bool checkedConditional = false, observedConditional = false;
  auto requirePath = [&](mlir::Value path) {
    auto conjunction = path.getDefiningOp<BitsBinaryOp>();
    ASSERT_TRUE(conjunction);
    EXPECT_EQ(conjunction.getOpcode(), "and");
    EXPECT_EQ(conjunction->getOperand(0), fire);
  };
  for (auto [ordinal, effect] : llvm::enumerate(plan.bodyChecks)) {
    if (!effect.point.path.branch)
      continue;
    checkedConditional = true;
    auto slot = prepared.checkBase + 2 * (plan.controls.size() + ordinal) + 1;
    EXPECT_EQ(record.bodyChecks[ordinal].getPath(), record.rule.getResult(slot));
    requirePath(yield->getOperand(slot));
    EXPECT_NE(slot, prepared.fireResult);
  }
  for (auto [ordinal, effect] : llvm::enumerate(plan.observations)) {
    if (!effect.point.path.branch)
      continue;
    observedConditional = true;
    auto slot = prepared.observationPathResults[ordinal];
    EXPECT_EQ(record.observations[ordinal].getPath(), record.rule.getResult(slot));
    requirePath(yield->getOperand(slot));
    EXPECT_NE(slot, prepared.fireResult);
  }
  EXPECT_TRUE(checkedConditional);
  EXPECT_TRUE(observedConditional);
}

TEST_F(SampledTransactionLoweringTest,
       ConditionalPlanLocatorsCannotAuthenticateChangedSourceSemantics) {
  for (unsigned mutation = 0; mutation != 15; ++mutation) {
    SCOPED_TRACE(mutation);
    auto staged = stage([&](mlir::ModuleOp file) { addConditionalArmEffects(file); });
    ASSERT_TRUE(staged);
    auto index = transactionIndex(*staged, "run");
    auto &plan = const_cast<TransactionStatePlan &>(
        *staged->prepared.transactions[index].plan);
    ASSERT_EQ(plan.branches.size(), 1u);
    ASSERT_EQ(plan.selections.size(), 1u);
    auto conditional = llvm::find_if(plan.calls, [&](const auto &call) {
      return bool(plan.frames[call.callee].entry.path.branch);
    });
    ASSERT_NE(conditional, plan.calls.end());
    auto &frame = plan.frames[conditional->callee];
    auto &branch = plan.branches.front();
    if (mutation == 0)
      branch.condition = plan.clockValue;
    if (mutation == 1)
      branch.conditionLocation = span(99);
    if (mutation == 2)
      branch.entry.path = TransactionSourcePath{0, false};
    if (mutation == 3)
      ++branch.thenExitPrefix;
    if (mutation == 4)
      branch.selection.reset();
    if (mutation == 5)
      plan.selections.front().control = 0;
    if (mutation == 6)
      plan.controls[plan.guards[frame.guard].control].path = {};
    if (mutation == 7)
      frame.entry.path.whenTrue = false;
    if (mutation == 8)
      frame.parent = conditional->callee;
    if (mutation == 9) {
      ASSERT_FALSE(conditional->arguments.empty());
      conditional->arguments.front() = plan.owners.front().oldQ;
    }
    if (mutation == 10) {
      ASSERT_FALSE(conditional->results.empty());
      conditional->results.front().result = conditional->results.front().yielded;
    }
    if (mutation == 11) {
      ASSERT_FALSE(conditional->results.empty());
      plan.values[conditional->results.front().result].validity = frame.entry;
    }
    if (mutation == 12) {
      auto event = llvm::find_if(plan.events, [&](const auto &event) {
        return event.kind == TransactionStateEventKind::CallExit &&
               event.call && &plan.calls[*event.call] == &*conditional;
      });
      ASSERT_NE(event, plan.events.end());
      event->point.path = {};
    }
    if (mutation == 13) {
      auto effect = llvm::find_if(plan.bodyChecks, [](const auto &effect) {
        return bool(effect.point.path.branch);
      });
      ASSERT_NE(effect, plan.bodyChecks.end());
      effect->point.path = {};
    }
    if (mutation == 14) {
      auto version = llvm::find_if(plan.versions, [](const auto &version) {
        return version.kind == StateVersionKind::Write &&
               bool(version.validity.path.branch);
      });
      ASSERT_NE(version, plan.versions.end());
      version->validity.path = {};
    }
    rejects(*staged, "");
  }
}

TEST_F(SampledTransactionLoweringTest,
       OldGuardOnlySchemaCannotReplaceExactControlInventory) {
  auto staged = stage();
  ASSERT_TRUE(staged);
  auto index = transactionIndex(*staged, "run");
  auto rule = staged->candidate.transactions[index].rule;
  auto metadata = rule->getAttrOfType<mlir::DictionaryAttr>("ac.sampled_admission");
  mlir::NamedAttrList fields(metadata);
  fields.set("guards", fields.get("controls"));
  fields.erase("controls");
  rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  rejects(*staged, "");
}

TEST_F(SampledTransactionLoweringTest,
       BranchPreparationBudgetFailurePreservesOriginalPackage) {
  auto original = parse();
  ASSERT_TRUE(original);
  addAuthoredBranch(*original);
  ASSERT_TRUE(mlir::succeeded(capture([&] { return mlir::verify(*original); })))
      << diagnostics;
  auto originalText = print(original->getOperation());
  auto originalOperations = operations(*original);
  SourceTransactionStateAnalysis analysis(original->getOperation());
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  HardwareSourceCheckLimits limits;
  limits.maxWorkUnits = 1;
  detail::SourceCheckBudget budget(limits);
  auto prepared = capture([&] {
    return acir::detail::prepareTransactionLowering(*original, analysis,
                                                    budget);
  });
  EXPECT_TRUE(mlir::failed(prepared));
  EXPECT_FALSE(diagnostics.empty());
  EXPECT_EQ(print(original->getOperation()), originalText);
  EXPECT_EQ(operations(*original), originalOperations);
}

TEST_F(SampledTransactionLoweringTest,
       FinalProofCannotReplaceOriginalBranchHistory) {
  AuthoredBranch source;
  auto staged =
      stage([&](mlir::ModuleOp file) { source = addAuthoredBranch(file); });
  ASSERT_TRUE(staged);
  expectStandaloneFinalValid(*staged);
  source.operation->setAttr("ac.condition_location", span(91));
  auto changed = print(staged->original.get().getOperation());
  EXPECT_TRUE(mlir::failed(verify(*staged)));
  EXPECT_NE(diagnostics.find("original branch"), std::string::npos)
      << diagnostics;
  EXPECT_EQ(print(staged->original.get().getOperation()), changed);
  expectStandaloneFinalValid(*staged);
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
      reason = "original call yielded-result identity/validity differs";
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
