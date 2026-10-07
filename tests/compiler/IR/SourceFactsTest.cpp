#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/ACIRTypes.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <initializer_list>
#include <string>
#include <utility>

namespace acir::ac {
namespace {

using Field = std::pair<llvm::StringRef, mlir::Attribute>;

mlir::DictionaryAttr dictionary(mlir::Builder &builder,
                                std::initializer_list<Field> fields) {
  llvm::SmallVector<mlir::NamedAttribute> attributes;
  for (const auto &[name, value] : fields)
    attributes.push_back(builder.getNamedAttr(name, value));
  return builder.getDictionaryAttr(attributes);
}

mlir::IntegerAttr u32(mlir::Builder &builder, uint32_t value) {
  return builder.getIntegerAttr(builder.getI32Type(), value);
}

mlir::IntegerAttr u64(mlir::Builder &builder, uint64_t value) {
  return builder.getIntegerAttr(builder.getI64Type(), value);
}

mlir::DictionaryAttr sourceOwner(mlir::Builder &builder) {
  return dictionary(builder, {{"package", builder.getStringAttr("demo")},
                              {"path", builder.getStringAttr("facts.py")}});
}

mlir::DictionaryAttr occurrence(mlir::Builder &builder, uint64_t astIndex) {
  auto path = builder.getArrayAttr(
      {dictionary(builder, {{"kind", builder.getStringAttr("index")},
                            {"value", u64(builder, astIndex)}})});
  auto site = dictionary(
      builder, {{"definition", mlir::FlatSymbolRefAttr::get(
                                   builder.getContext(), "demo.Facts")},
                {"ast_path", path}});
  return dictionary(builder,
                    {{"site", site}, {"expansion", builder.getArrayAttr({})}});
}

mlir::DictionaryAttr logicalU8(mlir::Builder &builder,
                               uint64_t upperBound = 256) {
  return dictionary(
      builder,
      {{"kind", builder.getStringAttr("integer")},
       {"storage", mlir::TypeAttr::get(builder.getI8Type())},
       {"lower", MathIntAttr::get(builder.getContext(),
                                  llvm::APSInt(llvm::APInt(2, 0), true))},
       {"upper",
        MathIntAttr::get(builder.getContext(),
                         llvm::APSInt(llvm::APInt(9, upperBound), true))},
       {"interpretation", builder.getStringAttr("unsigned")}});
}

mlir::DictionaryAttr formalState(mlir::Builder &builder,
                                 llvm::StringRef parameter) {
  return dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                              {"parameter", builder.getStringAttr(parameter)},
                              {"ordinal", builder.getUnitAttr()}});
}

mlir::DictionaryAttr valueID(mlir::Builder &builder, uint64_t originIndex,
                             uint32_t slot = 0) {
  return dictionary(builder, {{"origin", occurrence(builder, originIndex)},
                              {"slot", u32(builder, slot)}});
}

mlir::DictionaryAttr useID(mlir::Builder &builder, uint64_t originIndex,
                           llvm::StringRef role = "next", uint32_t slot = 0) {
  return dictionary(builder, {{"origin", occurrence(builder, originIndex)},
                              {"role", builder.getStringAttr(role)},
                              {"slot", u32(builder, slot)}});
}

mlir::DictionaryAttr nextValueTarget(mlir::Builder &builder,
                                      llvm::StringRef parameter = "state") {
  return dictionary(builder, {{"kind", builder.getStringAttr("next_value")},
                              {"state", formalState(builder, parameter)}});
}

mlir::DictionaryAttr port(mlir::Builder &builder, llvm::StringRef parameter,
                          llvm::StringRef role, uint64_t originIndex,
                          mlir::DictionaryAttr logical = {}) {
  if (!logical)
    logical = logicalU8(builder);
  return dictionary(
      builder,
      {{"parameter", builder.getStringAttr(parameter)},
       {"ordinal", builder.getUnitAttr()},
       {"role", builder.getStringAttr(role)},
       {"type", logical},
       {"origin", occurrence(builder, originIndex)},
       {"location",
        dictionary(builder, {{"path", builder.getStringAttr("facts.py")},
                             {"line", u64(builder, 4)},
                             {"column", u64(builder, 11)},
                             {"end_line", u64(builder, 4)},
                             {"end_column", u64(builder, 16)}})}});
}

struct SourceFactsGraph {
  mlir::OwningOpRef<mlir::ModuleOp> container;
  ModuleOp module;
  RuleOp rule;
  mlir::Operation *read;
  mlir::Operation *use;
  mlir::Operation *secondUse;
  mlir::arith::ConstantOp valid;
  mlir::arith::ConstantOp path;
};

SourceFactsGraph buildSourceFactsGraph(mlir::MLIRContext &context,
                                       bool twoTargets = false) {
  mlir::OpBuilder builder(&context);
  auto location = mlir::FileLineColLoc::get(&context, "facts.py", 9, 7);
  auto container = mlir::ModuleOp::create(location);
  container->setAttr("ac.stage", builder.getStringAttr("source"));
  container->setAttr("ac.unit_kind", builder.getStringAttr("implementation"));
  container->setAttr("ac.source_owner", sourceOwner(builder));
  builder.setInsertionPointToStart(container.getBody());

  mlir::OperationState moduleState(location, ModuleOp::getOperationName());
  moduleState.addAttribute("name", builder.getStringAttr("Facts"));
  moduleState.addAttribute("sym_name", builder.getStringAttr("demo.Facts"));
  moduleState.addAttribute("ac.source_owner", sourceOwner(builder));
  moduleState.addAttribute("ac.origin", occurrence(builder, 0));
  llvm::SmallVector<mlir::Attribute> ports{
      port(builder, twoTargets ? "source" : "state", "current", 1)};
  llvm::SmallVector<llvm::StringRef> targetNames;
  if (twoTargets)
    targetNames.append({"left", "right"});
  else
    targetNames.push_back("state");
  for (auto [index, target] : llvm::enumerate(targetNames))
    ports.push_back(port(builder, target, "next", index + 2));
  moduleState.addAttribute("ac.ports", builder.getArrayAttr(ports));
  moduleState.addAttribute("ac.control_ports",
                           dictionary(builder,
                                      {{"clock", builder.getI32IntegerAttr(0)},
                                       {"reset", builder.getI32IntegerAttr(1)}}));
  moduleState.addRegion();
  auto module = mlir::cast<ModuleOp>(builder.create(moduleState));
  auto *moduleBody = new mlir::Block();
  module.getBody().push_back(moduleBody);
  moduleBody->addArgument(builder.getI1Type(), location);
  moduleBody->addArgument(builder.getI1Type(), location);
  moduleBody->addArgument(RegType::get(&context, builder.getI8Type()),
                          location);
  for (size_t index = 0; index < targetNames.size(); ++index)
    moduleBody->addArgument(RegType::get(&context, builder.getI8Type()),
                            location);

  builder.setInsertionPointToEnd(moduleBody);
  mlir::OperationState ruleState(location, RuleOp::getOperationName());
  llvm::SmallVector<mlir::Value> ruleOperands{moduleBody->getArgument(2)};
  llvm::SmallVector<mlir::Type> ruleResults;
  llvm::SmallVector<mlir::Attribute> outputBindings, outputTypes;
  for (auto [index, target] : llvm::enumerate(targetNames)) {
    ruleOperands.push_back(moduleBody->getArgument(index + 3));
    ruleResults.append({builder.getI8Type(), builder.getI1Type()});
    outputBindings.push_back(formalState(builder, target));
    outputTypes.push_back(logicalU8(builder));
  }
  ruleState.addOperands(ruleOperands);
  ruleState.addTypes(ruleResults);
  ruleState.addAttribute("name", builder.getStringAttr("advance"));
  ruleState.addAttribute("registration", occurrence(builder, 2));
  ruleState.addAttribute("operandSegmentSizes",
                         builder.getDenseI32ArrayAttr(
                             {1, static_cast<int32_t>(targetNames.size())}));
  ruleState.addAttribute("ac.source_owner", sourceOwner(builder));
  ruleState.addAttribute("ac.origin", occurrence(builder, 3));
  ruleState.addAttribute("ac.input_bindings",
                         builder.getArrayAttr({formalState(
                             builder, twoTargets ? "source" : "state")}));
  ruleState.addAttribute("ac.output_bindings",
                         builder.getArrayAttr(outputBindings));
  ruleState.addAttribute("ac.input_types",
                         builder.getArrayAttr({logicalU8(builder)}));
  ruleState.addAttribute("ac.output_types", builder.getArrayAttr(outputTypes));
  ruleState.addRegion();
  auto rule = mlir::cast<RuleOp>(builder.create(ruleState));
  auto *ruleBody = new mlir::Block();
  rule.getBody().push_back(ruleBody);
  ruleBody->addArgument(builder.getI8Type(), location);
  builder.setInsertionPointToEnd(ruleBody);

  mlir::OperationState readState(location, "ac.source.read");
  readState.addOperands(ruleBody->getArgument(0));
  readState.addTypes(builder.getI8Type());
  readState.addAttribute("ac.origin", occurrence(builder, 4));
  mlir::Operation *read = builder.create(readState);
  auto valid = mlir::arith::ConstantOp::create(
      builder, location, builder.getI1Type(), builder.getBoolAttr(true));
  auto path = mlir::arith::ConstantOp::create(
      builder, location, builder.getI1Type(), builder.getBoolAttr(true));

  mlir::OperationState useState(location, "ac.source.use");
  useState.addOperands(
      {read->getResult(0), valid.getResult(), path.getResult()});
  useState.addTypes({builder.getI8Type(), builder.getI1Type()});
  useState.addAttribute("id", useID(builder, 5));
  useState.addAttribute("source", valueID(builder, 4));
  useState.addAttribute("target",
                        nextValueTarget(builder, targetNames.front()));
  mlir::Operation *use = builder.create(useState);

  mlir::Operation *secondUse = nullptr;
  if (twoTargets) {
    mlir::OperationState secondUseState(location, "ac.source.use");
    secondUseState.addOperands(
        {read->getResult(0), valid.getResult(), path.getResult()});
    secondUseState.addTypes({builder.getI8Type(), builder.getI1Type()});
    secondUseState.addAttribute("id", useID(builder, 6));
    secondUseState.addAttribute("source", valueID(builder, 4));
    secondUseState.addAttribute("target",
                                nextValueTarget(builder, targetNames.back()));
    secondUse = builder.create(secondUseState);
  }

  mlir::OperationState ruleYield(location, YieldOp::getOperationName());
  ruleYield.addOperands(use->getResults());
  if (secondUse)
    ruleYield.addOperands(secondUse->getResults());
  builder.create(ruleYield);
  builder.setInsertionPointToEnd(moduleBody);
  builder.create(mlir::OperationState(location, YieldOp::getOperationName()));

  return {
      std::move(container), module, rule, read, use, secondUse, valid, path};
}

mlir::DictionaryAttr replaceField(mlir::Builder &builder,
                                  mlir::DictionaryAttr dictionary,
                                  llvm::StringRef name,
                                  mlir::Attribute replacement) {
  llvm::SmallVector<mlir::NamedAttribute> fields(dictionary.begin(),
                                                 dictionary.end());
  for (mlir::NamedAttribute &field : fields) {
    if (field.getName() != name)
      continue;
    field = builder.getNamedAttr(name, replacement);
    return builder.getDictionaryAttr(fields);
  }
  ADD_FAILURE() << "missing dictionary field " << name.str();
  return dictionary;
}

mlir::Operation *findOnly(mlir::ModuleOp module, llvm::StringRef name) {
  mlir::Operation *found = nullptr;
  module.walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() != name)
      return;
    EXPECT_EQ(found, nullptr) << "duplicate operation " << name.str();
    found = operation;
  });
  return found;
}

llvm::SmallVector<mlir::Operation *> findAll(mlir::ModuleOp module,
                                             llvm::StringRef name) {
  llvm::SmallVector<mlir::Operation *> found;
  module.walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == name)
      found.push_back(operation);
  });
  return found;
}

struct Verification {
  bool passed;
  std::string diagnostic;
};

Verification verify(mlir::MLIRContext &context, mlir::Operation *operation) {
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream(diagnostic) << value;
    return mlir::success();
  });
  return {mlir::succeeded(mlir::verify(operation)), diagnostic};
}

void expectRejected(const Verification &result) {
  EXPECT_FALSE(result.passed) << "invalid source fact was accepted";
  EXPECT_FALSE(result.diagnostic.empty());
}

bool sourceFactOpsAreRegistered(mlir::MLIRContext &context) {
  return mlir::RegisteredOperationName::lookup("ac.source.read", &context) &&
         mlir::RegisteredOperationName::lookup("ac.source.use", &context);
}

class SourceFactsTest : public ::testing::Test {
protected:
  SourceFactsTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect>();
  }

  mlir::OwningOpRef<mlir::ModuleOp> clone(mlir::ModuleOp module) {
    return mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(module->clone()));
  }

  mlir::MLIRContext context;
};

TEST_F(SourceFactsTest, CanonicalReadAndNextUseCloseTheRuleYield) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context);
  Verification result = verify(context, *graph.container);
  EXPECT_TRUE(result.passed) << result.diagnostic;

  EXPECT_EQ(graph.read->getNumOperands(), 1u);
  EXPECT_EQ(graph.read->getNumResults(), 1u);
  EXPECT_EQ(graph.read->getOperand(0),
            graph.rule.getBody().front().getArgument(0));
  EXPECT_EQ(graph.read->getOperand(0).getType(),
            graph.read->getResult(0).getType());
  EXPECT_EQ(graph.read->getNumRegions(), 0u);

  EXPECT_EQ(graph.use->getNumOperands(), 3u);
  EXPECT_EQ(graph.use->getNumResults(), 2u);
  EXPECT_EQ(graph.use->getOperand(0), graph.read->getResult(0));
  EXPECT_TRUE(graph.use->getOperand(1).getType().isInteger(1));
  EXPECT_TRUE(graph.use->getOperand(2).getType().isInteger(1));
  mlir::Operation &yield = graph.rule.getBody().front().back();
  EXPECT_EQ(yield.getOperand(0), graph.use->getResult(0));
  EXPECT_EQ(yield.getOperand(1), graph.use->getResult(1));
}

TEST_F(SourceFactsTest, UseAndValueIdentitiesAreClosedAndTyped) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context);
  auto id = graph.use->getAttrOfType<mlir::DictionaryAttr>("id");
  auto source = graph.use->getAttrOfType<mlir::DictionaryAttr>("source");
  ASSERT_TRUE(id && source);
  EXPECT_EQ(id.size(), 3u);
  EXPECT_TRUE(id.getAs<mlir::DictionaryAttr>("origin"));
  EXPECT_EQ(id.getAs<mlir::StringAttr>("role").getValue(), "next");
  EXPECT_EQ(id.getAs<mlir::IntegerAttr>("slot").getType(),
            mlir::IntegerType::get(&context, 32));
  EXPECT_TRUE(mlir::succeeded(
      detail::verifyValueID(source, [&]() -> mlir::InFlightDiagnostic {
        return mlir::emitError(mlir::UnknownLoc::get(&context));
      })));
}

TEST_F(SourceFactsTest, TwoTargetUsesOwnTheirCorrespondingYieldPairs) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context, /*twoTargets=*/true);
  ASSERT_NE(graph.secondUse, nullptr);
  Verification result = verify(context, *graph.container);
  EXPECT_TRUE(result.passed) << result.diagnostic;

  mlir::Operation &yield = graph.rule.getBody().front().back();
  ASSERT_EQ(yield.getNumOperands(), 4u);
  EXPECT_EQ(yield.getOperand(0), graph.use->getResult(0));
  EXPECT_EQ(yield.getOperand(1), graph.use->getResult(1));
  EXPECT_EQ(yield.getOperand(2), graph.secondUse->getResult(0));
  EXPECT_EQ(yield.getOperand(3), graph.secondUse->getResult(1));
}

TEST_F(SourceFactsTest, TwoTargetUsesRejectCrossPairFanoutAndSwap) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context, /*twoTargets=*/true);

  auto fanout = clone(*graph.container);
  auto uses = findAll(*fanout, "ac.source.use");
  ASSERT_EQ(uses.size(), 2u);
  auto *rule = findOnly(*fanout, "ac.rule");
  mlir::Operation &yield = rule->getRegion(0).front().back();
  yield.getOpOperand(2).set(uses[0]->getResult(0));
  yield.getOpOperand(3).set(uses[0]->getResult(1));
  expectRejected(verify(context, *fanout));

  auto swapped = clone(*graph.container);
  uses = findAll(*swapped, "ac.source.use");
  ASSERT_EQ(uses.size(), 2u);
  rule = findOnly(*swapped, "ac.rule");
  mlir::Operation &swappedYield = rule->getRegion(0).front().back();
  swappedYield.getOpOperand(0).set(uses[1]->getResult(0));
  swappedYield.getOpOperand(1).set(uses[1]->getResult(1));
  swappedYield.getOpOperand(2).set(uses[0]->getResult(0));
  swappedYield.getOpOperand(3).set(uses[0]->getResult(1));
  expectRejected(verify(context, *swapped));
}

TEST_F(SourceFactsTest, TwoTargetUseRejectsCombinerCrossTargetLeakage) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context, /*twoTargets=*/true);
  auto leaked = clone(*graph.container);
  auto uses = findAll(*leaked, "ac.source.use");
  ASSERT_EQ(uses.size(), 2u);
  auto *rule = findOnly(*leaked, "ac.rule");
  mlir::Operation &yield = rule->getRegion(0).front().back();
  mlir::Value secondPath = uses[1]->getOperand(2);
  mlir::OpBuilder builder(&context);
  builder.setInsertionPoint(&yield);
  mlir::OperationState selectState(yield.getLoc(),
                                   mlir::arith::SelectOp::getOperationName());
  selectState.addOperands(
      {uses[0]->getResult(1), uses[1]->getResult(0), uses[0]->getResult(0)});
  selectState.addTypes(builder.getI8Type());
  mlir::Operation *select = builder.create(selectState);

  // use0 still owns its direct pair, while use1 retains its own path and
  // enabled result.  Only target1 data now additionally depends on use0.
  yield.getOpOperand(2).set(select->getResult(0));
  EXPECT_EQ(yield.getOperand(0), uses[0]->getResult(0));
  EXPECT_EQ(yield.getOperand(1), uses[0]->getResult(1));
  EXPECT_EQ(yield.getOperand(3), uses[1]->getResult(1));
  EXPECT_EQ(uses[1]->getOperand(2), secondPath);
  expectRejected(verify(context, *leaked));
}

TEST_F(SourceFactsTest, TwoTargetUsesRejectDuplicateUseIdentity) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context, /*twoTargets=*/true);
  auto duplicated = clone(*graph.container);
  auto uses = findAll(*duplicated, "ac.source.use");
  ASSERT_EQ(uses.size(), 2u);
  uses[1]->setAttr("id", uses[0]->getAttr("id"));
  expectRejected(verify(context, *duplicated));
}

TEST_F(SourceFactsTest,
       ReadValueCannotFeedSameWidthTargetWithDifferentLogicalRange) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context, /*twoTargets=*/true);
  auto mismatched = clone(*graph.container);
  mlir::Builder builder(&context);
  // Both [0,256) and [0,255) require and declare i8 storage.  Rejection must
  // therefore come from unequal LogicalTypes, not a minimal-width i7 check.
  auto narrower = logicalU8(builder, 255);

  auto *module = findOnly(*mismatched, "ac.module");
  auto ports = module->getAttrOfType<mlir::ArrayAttr>("ac.ports");
  llvm::SmallVector<mlir::Attribute> changedPorts(ports.begin(), ports.end());
  changedPorts[1] =
      replaceField(builder, mlir::cast<mlir::DictionaryAttr>(changedPorts[1]),
                   "type", narrower);
  module->setAttr("ac.ports", builder.getArrayAttr(changedPorts));

  auto *rule = findOnly(*mismatched, "ac.rule");
  auto outputTypes = rule->getAttrOfType<mlir::ArrayAttr>("ac.output_types");
  llvm::SmallVector<mlir::Attribute> changedTypes(outputTypes.begin(),
                                                  outputTypes.end());
  changedTypes[0] = narrower;
  rule->setAttr("ac.output_types", builder.getArrayAttr(changedTypes));
  expectRejected(verify(context, *mismatched));
}

TEST_F(SourceFactsTest, ReadRejectsWrongArityTypeOriginAndRuleContext) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context);

  auto wrongArity = clone(*graph.container);
  auto *read = findOnly(*wrongArity, "ac.source.read");
  read->insertOperands(1, read->getOperand(0));
  expectRejected(verify(context, *wrongArity));

  auto wrongType = clone(*graph.container);
  read = findOnly(*wrongType, "ac.source.read");
  read->getResult(0).setType(mlir::IntegerType::get(&context, 1));
  expectRejected(verify(context, *wrongType));

  auto wrongOrigin = clone(*graph.container);
  read = findOnly(*wrongOrigin, "ac.source.read");
  read->setAttr("ac.origin", mlir::DictionaryAttr::get(&context));
  expectRejected(verify(context, *wrongOrigin));

  auto wrongSource = clone(*graph.container);
  read = findOnly(*wrongSource, "ac.source.read");
  mlir::OpBuilder at(read);
  auto local = mlir::arith::ConstantOp::create(
      at, read->getLoc(), at.getI8Type(), at.getI8IntegerAttr(7));
  read->getOpOperand(0).set(local.getResult());
  expectRejected(verify(context, *wrongSource));

  auto wrongOwner = clone(*graph.container);
  auto *rule = findOnly(*wrongOwner, "ac.rule");
  mlir::Builder builder(&context);
  rule->setAttr(
      "ac.source_owner",
      dictionary(builder, {{"package", builder.getStringAttr("other")},
                           {"path", builder.getStringAttr("other.py")}}));
  expectRejected(verify(context, *wrongOwner));

  auto wrongContext = clone(*graph.container);
  read = findOnly(*wrongContext, "ac.source.read");
  read->moveBefore(findOnly(*wrongContext, "ac.module"));
  expectRejected(verify(context, read));
}

TEST_F(SourceFactsTest, UseRejectsWrongArityAndFiniteTypes) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context);

  auto wrongArity = clone(*graph.container);
  auto *use = findOnly(*wrongArity, "ac.source.use");
  use->eraseOperand(2);
  expectRejected(verify(context, *wrongArity));

  auto wrongEnabled = clone(*graph.container);
  use = findOnly(*wrongEnabled, "ac.source.use");
  use->getResult(1).setType(mlir::IntegerType::get(&context, 8));
  expectRejected(verify(context, *wrongEnabled));

  auto wrongPath = clone(*graph.container);
  use = findOnly(*wrongPath, "ac.source.use");
  use->getOpOperand(2).set(use->getOperand(0));
  expectRejected(verify(context, *wrongPath));

  auto wrongContext = clone(*graph.container);
  use = findOnly(*wrongContext, "ac.source.use");
  use->moveBefore(findOnly(*wrongContext, "ac.module"));
  expectRejected(verify(context, use));
}

TEST_F(SourceFactsTest, NextUseRejectsMalformedIdentitiesAndTargetState) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context);

  auto wrongRole = clone(*graph.container);
  auto *use = findOnly(*wrongRole, "ac.source.use");
  mlir::Builder builder(&context);
  // helper_result/helper_return structure is a deferred source provenance responsibility.
  // This mutation freezes only the approved rule that next_value uses role
  // "next"; it does not define or reject either deferred target form.
  use->setAttr("id", useID(builder, 5, "helper_return"));
  expectRejected(verify(context, *wrongRole));

  auto wrongUseSlot = clone(*graph.container);
  use = findOnly(*wrongUseSlot, "ac.source.use");
  use->setAttr("id",
               dictionary(builder, {{"origin", occurrence(builder, 5)},
                                    {"role", builder.getStringAttr("next")},
                                    {"slot", builder.getBoolAttr(false)}}));
  expectRejected(verify(context, *wrongUseSlot));

  auto wrongUseOrigin = clone(*graph.container);
  use = findOnly(*wrongUseOrigin, "ac.source.use");
  use->setAttr("id",
               dictionary(builder, {{"origin", dictionary(builder, {})},
                                    {"role", builder.getStringAttr("next")},
                                    {"slot", u32(builder, 0)}}));
  expectRejected(verify(context, *wrongUseOrigin));

  auto wrongValue = clone(*graph.container);
  use = findOnly(*wrongValue, "ac.source.use");
  use->setAttr("source",
               dictionary(builder, {{"origin", occurrence(builder, 4)},
                                    {"slot", builder.getBoolAttr(false)}}));
  expectRejected(verify(context, *wrongValue));

  auto wrongState = clone(*graph.container);
  use = findOnly(*wrongState, "ac.source.use");
  use->setAttr("target", nextValueTarget(builder, "other"));
  expectRejected(verify(context, *wrongState));
}

TEST_F(SourceFactsTest, NextValueRejectsNonzeroUseSlot) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context);
  auto nonzeroValueSlot = clone(*graph.container);
  auto *use = findOnly(*nonzeroValueSlot, "ac.source.use");
  mlir::Builder builder(&context);
  use->setAttr("id", useID(builder, 5, "next", 1));
  expectRejected(verify(context, *nonzeroValueSlot));
}

TEST_F(SourceFactsTest, CurrentPayloadCannotBypassSourceRead) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context);
  graph.use->getOpOperand(0).set(graph.rule.getBody().front().getArgument(0));
  expectRejected(verify(context, *graph.container));
}

TEST_F(SourceFactsTest, UseDataAndEnabledMustReachTheirOwnYieldPair) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context);
  mlir::Operation &yield = graph.rule.getBody().front().back();
  yield.getOpOperand(0).set(graph.read->getResult(0));
  yield.getOpOperand(1).set(graph.path.getResult());
  expectRejected(verify(context, *graph.container));
}

TEST_F(SourceFactsTest, RepeatedReadsOfOneCurrentKeepDistinctOrigins) {
  ASSERT_TRUE(sourceFactOpsAreRegistered(context))
      << "source provenance: ac.source.read/use are not registered";
  SourceFactsGraph graph = buildSourceFactsGraph(context);
  mlir::OpBuilder builder(graph.use);
  mlir::OperationState secondState(graph.read->getLoc(), "ac.source.read");
  secondState.addOperands(graph.read->getOperand(0));
  secondState.addTypes(graph.read->getResult(0).getType());
  secondState.addAttribute("ac.origin", occurrence(builder, 6));
  mlir::Operation *second = builder.create(secondState);

  Verification result = verify(context, *graph.container);
  EXPECT_TRUE(result.passed) << result.diagnostic;
  EXPECT_EQ(graph.read->getOperand(0), second->getOperand(0));
  EXPECT_NE(graph.read->getResult(0), second->getResult(0));
  EXPECT_NE(graph.read->getAttr("ac.origin"), second->getAttr("ac.origin"));
}

} // namespace
} // namespace acir::ac
