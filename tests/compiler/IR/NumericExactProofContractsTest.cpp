#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
namespace acir::ac {
namespace {
using Field = std::pair<llvm::StringRef, mlir::Attribute>;
mlir::DictionaryAttr dictionary(mlir::OpBuilder &builder,
                                std::initializer_list<Field> fields) {
  llvm::SmallVector<mlir::NamedAttribute> result;
  for (const auto &[name, value] : fields)
    result.push_back(builder.getNamedAttr(name, value));
  return builder.getDictionaryAttr(result);
}
MathIntAttr mathInteger(mlir::MLIRContext &context, llvm::StringRef spelling) {
  bool negative = spelling.consume_front("-");
  const unsigned width = static_cast<unsigned>(spelling.size() * 4 + 2);
  llvm::APInt value(width, spelling, 10);
  if (negative)
    value = -value;
  return MathIntAttr::get(&context, llvm::APSInt(std::move(value), !negative));
}
mlir::DictionaryAttr occurrence(mlir::OpBuilder &builder,
                                llvm::StringRef module = "demo.Compute",
                                uint64_t line = 7) {
  auto site = builder.getDictionaryAttr({
      builder.getNamedAttr("definition", mlir::FlatSymbolRefAttr::get(
                                             builder.getContext(), module)),
      builder.getNamedAttr(
          "ast_path",
          builder.getArrayAttr({builder.getDictionaryAttr({
              builder.getNamedAttr("kind", builder.getStringAttr("index")),
              builder.getNamedAttr("value", builder.getI64IntegerAttr(line)),
          })})),
  });
  return builder.getDictionaryAttr({
      builder.getNamedAttr("site", site),
      builder.getNamedAttr("expansion", builder.getArrayAttr({})),
  });
}
mlir::DictionaryAttr sourceOwner(mlir::OpBuilder &builder) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("package", builder.getStringAttr("demo")),
      builder.getNamedAttr("path", builder.getStringAttr("demo.py")),
  });
}
mlir::DictionaryAttr singletonDomain(mlir::OpBuilder &builder, unsigned width,
                                     llvm::StringRef value,
                                     llvm::StringRef lower,
                                     llvm::StringRef upper, bool isUnsigned) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("storage",
                           mlir::TypeAttr::get(builder.getIntegerType(width))),
      builder.getNamedAttr("lower", mathInteger(*builder.getContext(), lower)),
      builder.getNamedAttr("upper", mathInteger(*builder.getContext(), upper)),
      builder.getNamedAttr(
          "interpretation",
          builder.getStringAttr(isUnsigned ? "unsigned" : "signed")),
  });
}
mlir::APInt bitPattern(unsigned width, llvm::StringRef spelling) {
  bool negative = spelling.consume_front("-");
  llvm::APInt value(width, spelling, 10);
  return negative ? -value : value;
}
mlir::IntegerAttr integerConstant(mlir::OpBuilder &builder, unsigned width,
                                  llvm::StringRef spelling) {
  return builder.getIntegerAttr(builder.getIntegerType(width),
                                bitPattern(width, spelling));
}
struct ProofFixture {
  mlir::OwningOpRef<mlir::ModuleOp> file;
  ac::ModuleOp hardware;
  RuleOp rule;
  mlir::arith::ConstantOp actualValue;
  mlir::arith::ConstantOp trueValue;
  ValueBindingOp binding;
  NumericProofOp proof;
  mlir::DictionaryAttr valueID;
  mlir::DictionaryAttr domain;
  mlir::DictionaryAttr node;
};
ProofFixture
buildConstantFixture(mlir::MLIRContext &context, llvm::StringRef value = "5",
                     unsigned width = 3, llvm::StringRef lower = "5",
                     llvm::StringRef upper = "6", bool isUnsigned = true) {
  mlir::OpBuilder builder(&context);
  const mlir::Location location =
      mlir::FileLineColLoc::get(&context, "demo.py", 7, 3);
  const mlir::StringRef definition = "demo.Compute";
  auto owner = sourceOwner(builder);
  auto origin = occurrence(builder, definition);
  auto domain =
      singletonDomain(builder, width, value, lower, upper, isUnsigned);
  auto valueID = builder.getDictionaryAttr({
      builder.getNamedAttr("origin", origin),
      builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
  });
  auto specKey = builder.getDictionaryAttr({
      builder.getNamedAttr("definition",
                           mlir::FlatSymbolRefAttr::get(&context, definition)),
      builder.getNamedAttr("arguments", builder.getArrayAttr({})),
  });
  auto registration = occurrence(builder, definition, 10);
  auto proofScope = builder.getDictionaryAttr({
      builder.getNamedAttr("specialization", specKey),
      builder.getNamedAttr("registration", registration),
  });
  auto constantRef = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("constant")),
      builder.getNamedAttr("value", mathInteger(context, value)),
  });
  auto target = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("none")),
  });
  auto node = builder.getDictionaryAttr({
      builder.getNamedAttr("id", valueID),
      builder.getNamedAttr("operator", builder.getStringAttr("constant")),
      builder.getNamedAttr("operands", builder.getArrayAttr({constantRef})),
      builder.getNamedAttr("target", target),
  });
  auto moduleFile = mlir::ModuleOp::create(location);
  moduleFile->setAttr("ac.stage", builder.getStringAttr("source"));
  moduleFile->setAttr("ac.unit_kind", builder.getStringAttr("implementation"));
  moduleFile->setAttr("ac.source_owner", owner);
  builder.setInsertionPointToStart(moduleFile.getBody());
  mlir::OperationState hardwareState(location, ModuleOp::getOperationName());
  hardwareState.addAttribute("name", builder.getStringAttr("Compute"));
  hardwareState.addAttribute("sym_name", builder.getStringAttr(definition));
  hardwareState.addAttribute("ac.source_owner", owner);
  hardwareState.addAttribute("ac.origin", origin);
  hardwareState.addAttribute("ac.ports", builder.getArrayAttr({}));
  hardwareState.addAttribute(
      "ac.control_ports",
      builder.getDictionaryAttr({
          builder.getNamedAttr("clock", builder.getI32IntegerAttr(0)),
          builder.getNamedAttr("reset", builder.getI32IntegerAttr(1)),
      }));
  hardwareState.addRegion();
  ModuleOp hardware = mlir::cast<ModuleOp>(builder.create(hardwareState));
  auto *hardwareBody = new mlir::Block();
  hardware.getBody().push_back(hardwareBody);
  hardwareBody->addArgument(builder.getI1Type(), location);
  hardwareBody->addArgument(builder.getI1Type(), location);
  builder.setInsertionPointToEnd(hardwareBody);
  mlir::OperationState ruleState(location, RuleOp::getOperationName());
  ruleState.addAttribute("name", builder.getStringAttr("constant"));
  ruleState.addAttribute("registration", registration);
  ruleState.addAttribute("operandSegmentSizes",
                         builder.getDenseI32ArrayAttr({0, 0}));
  ruleState.addAttribute("ac.source_owner", owner);
  ruleState.addAttribute("ac.origin", origin);
  ruleState.addAttribute("ac.input_bindings", builder.getArrayAttr({}));
  ruleState.addAttribute("ac.output_bindings", builder.getArrayAttr({}));
  ruleState.addAttribute("ac.input_types", builder.getArrayAttr({}));
  ruleState.addAttribute("ac.output_types", builder.getArrayAttr({}));
  ruleState.addAttribute("ac.proof_scope", proofScope);
  ruleState.addAttribute("ac.required_numeric", builder.getArrayAttr({node}));
  ruleState.addRegion();
  RuleOp rule = mlir::cast<RuleOp>(builder.create(ruleState));
  auto *ruleBody = new mlir::Block();
  rule.getBody().push_back(ruleBody);
  builder.setInsertionPointToEnd(ruleBody);

  auto makeInt = [&](llvm::StringRef spelling) {
    mlir::OperationState state(location,
                               mlir::arith::ConstantOp::getOperationName());
    state.addAttribute("value", integerConstant(builder, width, spelling));
    state.addTypes(builder.getIntegerType(width));
    return mlir::cast<mlir::arith::ConstantOp>(builder.create(state));
  };
  auto actualValue = makeInt(value);
  auto trueValue = mlir::arith::ConstantOp::create(
      builder, location, builder.getI1Type(), builder.getBoolAttr(true));
  mlir::OperationState bindingState(location,
                                    ValueBindingOp::getOperationName());
  bindingState.addOperands(
      {actualValue.getResult(), trueValue.getResult(), trueValue.getResult()});
  bindingState.addAttribute("id", valueID);
  bindingState.addAttribute("domain", domain);
  auto binding = mlir::cast<ValueBindingOp>(builder.create(bindingState));

  mlir::OperationState proofState(location, NumericProofOp::getOperationName());
  proofState.addOperands(
      {trueValue.getResult(), actualValue.getResult(), trueValue.getResult()});
  proofState.addAttribute("operand_segment_sizes",
                          builder.getDenseI32ArrayAttr({1, 0, 0, 1, 1, 0, 0}));
  proofState.addAttribute("mode", builder.getStringAttr("exact"));
  proofState.addAttribute("result_domain", domain);
  proofState.addAttribute("input_ids", builder.getArrayAttr({}));
  proofState.addAttribute("input_domains", builder.getArrayAttr({}));
  proofState.addAttribute("result_id", valueID);
  proofState.addAttribute("obligations", builder.getArrayAttr({node}));
  proofState.addAttribute("checks", builder.getArrayAttr({}));
  proofState.addAttribute("origin", origin);
  auto proof = mlir::cast<NumericProofOp>(builder.create(proofState));
  mlir::OperationState ruleYield(location, YieldOp::getOperationName());
  builder.create(ruleYield);
  builder.setInsertionPointToEnd(hardwareBody);
  mlir::OperationState moduleYield(location, YieldOp::getOperationName());
  builder.create(moduleYield);

  return {std::move(moduleFile),
          hardware,
          rule,
          actualValue,
          trueValue,
          binding,
          proof,
          valueID,
          domain,
          node};
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
void expectRejected(const Verification &result) { EXPECT_FALSE(result.passed); }

void useCoordinatedForeignOrigin(ProofFixture &fixture,
                                 mlir::MLIRContext &context) {
  mlir::OpBuilder builder(&context);
  auto origin = occurrence(builder, "demo.Other");
  auto id = builder.getDictionaryAttr({
      builder.getNamedAttr("origin", origin),
      builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
  });
  fixture.binding->setAttr("id", id);
  fixture.proof->setAttr("result_id", id);
  fixture.proof->setAttr("origin", origin);
  auto node = builder.getDictionaryAttr({
      builder.getNamedAttr("id", id),
      builder.getNamedAttr("operator", builder.getStringAttr("constant")),
      builder.getNamedAttr("operands", fixture.node.get("operands")),
      builder.getNamedAttr("target", fixture.node.get("target")),
  });
  fixture.rule->setAttr("ac.required_numeric", builder.getArrayAttr({node}));
  fixture.proof->setAttr("obligations", builder.getArrayAttr({node}));
}
class ConstantExactProofContractsTest : public ::testing::Test {
protected:
  ConstantExactProofContractsTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect,
                        mlir::func::FuncDialect>();
  }
  mlir::MLIRContext context;
};

TEST_F(ConstantExactProofContractsTest,
       ExactSingletonConstantProofClosesRuleScopeAndBinding) {
  for (const auto &boundary : std::vector<
           std::tuple<std::string, unsigned, std::string, std::string, bool>>{
           {"0", 1, "0", "1", true},
           {"1", 1, "1", "2", true},
           {"-1", 1, "-1", "0", false},
           {"18446744073709551615", 64, "18446744073709551615",
            "18446744073709551616", true},
           {"-9223372036854775808", 64, "-9223372036854775808",
            "-9223372036854775807", false},
       }) {
    const auto &[value, width, lower, upper, isUnsigned] = boundary;
    SCOPED_TRACE(value);
    auto fixture =
        buildConstantFixture(context, value, width, lower, upper, isUnsigned);
    EXPECT_TRUE(mlir::succeeded(mlir::verify(*fixture.file)));
  }
}

TEST_F(ConstantExactProofContractsTest,
       RejectsIntegerDomainsBeyondTheApprovedSixtyFourBitCapability) {
  for (const auto &boundary :
       std::vector<std::tuple<std::string, std::string, std::string, bool>>{
           {"18446744073709551616", "18446744073709551616",
            "18446744073709551617", true},
           {"-9223372036854775809", "-9223372036854775809",
            "-9223372036854775808", false},
       }) {
    const auto &[value, lower, upper, isUnsigned] = boundary;
    auto fixture =
        buildConstantFixture(context, value, 65, lower, upper, isUnsigned);
    EXPECT_FALSE(mlir::succeeded(mlir::verify(*fixture.file))) << value;
  }
}

TEST_F(ConstantExactProofContractsTest,
       RejectsEachExactWitnessContractMutationIndependently) {
  using Mutation = std::function<void(ProofFixture &)>;
  std::vector<std::pair<std::string, Mutation>> mutations;
  mutations.push_back({"duplicate-binding", [&](ProofFixture &fixture) {
                         auto *duplicate = fixture.binding->clone();
                         fixture.proof->getBlock()->getOperations().insert(
                             fixture.proof->getIterator(), duplicate);
                       }});
  mutations.push_back(
      {"cross-scope-id", [&](ProofFixture &fixture) {
         mlir::OpBuilder builder(&context);
         fixture.binding->setAttr(
             "id",
             builder.getDictionaryAttr({
                 builder.getNamedAttr("origin",
                                      occurrence(builder, "demo.Other")),
                 builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
             }));
       }});
  mutations.push_back(
      {"scope-registration", [&](ProofFixture &fixture) {
         mlir::OpBuilder builder(&context);
         auto scope = fixture.rule->getAttrOfType<mlir::DictionaryAttr>(
             "ac.proof_scope");
         auto changed = builder.getDictionaryAttr({
             builder.getNamedAttr("specialization",
                                  scope.get("specialization")),
             builder.getNamedAttr("registration",
                                  occurrence(builder, "demo.Compute", 99)),
         });
         fixture.rule->setAttr("ac.proof_scope", changed);
       }});
  mutations.push_back(
      {"scope-specialization", [&](ProofFixture &fixture) {
         mlir::OpBuilder builder(&context);
         auto scope = fixture.rule->getAttrOfType<mlir::DictionaryAttr>(
             "ac.proof_scope");
         auto badSpec = builder.getDictionaryAttr({
             builder.getNamedAttr("definition", mlir::FlatSymbolRefAttr::get(
                                                    &context, "demo.Other")),
             builder.getNamedAttr("arguments", builder.getArrayAttr({})),
         });
         fixture.rule->setAttr(
             "ac.proof_scope",
             builder.getDictionaryAttr({
                 builder.getNamedAttr("specialization", badSpec),
                 builder.getNamedAttr("registration",
                                      scope.get("registration")),
             }));
       }});
  mutations.push_back({"missing-required", [&](ProofFixture &fixture) {
                         fixture.rule->setAttr(
                             "ac.required_numeric",
                             mlir::ArrayAttr::get(&context, {}));
                       }});
  mutations.push_back(
      {"extra-required", [&](ProofFixture &fixture) {
         fixture.rule->setAttr(
             "ac.required_numeric",
             mlir::ArrayAttr::get(&context, {fixture.node, fixture.node}));
       }});
  mutations.push_back({"low-bits-mode", [&](ProofFixture &fixture) {
                         fixture.proof->setAttr(
                             "mode",
                             mlir::StringAttr::get(&context, "low_bits"));
                       }});
  mutations.push_back(
      {"forbidden-width", [&](ProofFixture &fixture) {
         fixture.proof->setAttr(
             "width",
             mlir::IntegerAttr::get(mlir::IntegerType::get(&context, 32), 3));
       }});
  mutations.push_back({"segment-order", [&](ProofFixture &fixture) {
                         mlir::OpBuilder builder(&context);
                         fixture.proof->setAttr("operand_segment_sizes",
                                                builder.getDenseI32ArrayAttr(
                                                    {1, 0, 0, 1, 0, 1, 0}));
                       }});
  mutations.push_back({"too-few-operands", [&](ProofFixture &fixture) {
                         fixture.proof->eraseOperand(2);
                       }});
  mutations.push_back({"too-many-operands", [&](ProofFixture &fixture) {
                         fixture.proof->insertOperands(
                             fixture.proof->getNumOperands(),
                             fixture.trueValue.getResult());
                       }});
  mutations.push_back({"inputs", [&](ProofFixture &fixture) {
                         mlir::OpBuilder builder(&context);
                         fixture.proof->setAttr(
                             "input_ids",
                             builder.getArrayAttr({fixture.valueID}));
                       }});
  mutations.push_back({"checks", [&](ProofFixture &fixture) {
                         mlir::OpBuilder builder(&context);
                         fixture.proof->setAttr(
                             "checks", builder.getArrayAttr({fixture.node}));
                       }});
  mutations.push_back({"empty-obligations", [&](ProofFixture &fixture) {
                         fixture.proof->setAttr(
                             "obligations", mlir::ArrayAttr::get(&context, {}));
                       }});
  mutations.push_back(
      {"multiple-obligations", [&](ProofFixture &fixture) {
         mlir::OpBuilder builder(&context);
         fixture.proof->setAttr(
             "obligations", builder.getArrayAttr({fixture.node, fixture.node}));
       }});
  mutations.push_back(
      {"unknown-operator", [&](ProofFixture &fixture) {
         mlir::OpBuilder builder(&context);
         auto node = fixture.node;
         node = builder.getDictionaryAttr({
             builder.getNamedAttr("id", node.get("id")),
             builder.getNamedAttr("operator", builder.getStringAttr("add")),
             builder.getNamedAttr("operands", node.get("operands")),
             builder.getNamedAttr("target", node.get("target")),
         });
         fixture.proof->setAttr("obligations", builder.getArrayAttr({node}));
       }});
  mutations.push_back(
      {"bad-reference", [&](ProofFixture &fixture) {
         mlir::OpBuilder builder(&context);
         auto ref = builder.getDictionaryAttr({
             builder.getNamedAttr("kind", builder.getStringAttr("node")),
             builder.getNamedAttr("index", builder.getI32IntegerAttr(0)),
         });
         auto node = builder.getDictionaryAttr({
             builder.getNamedAttr("id", fixture.valueID),
             builder.getNamedAttr("operator",
                                  builder.getStringAttr("constant")),
             builder.getNamedAttr("operands", builder.getArrayAttr({ref})),
             builder.getNamedAttr("target", fixture.node.get("target")),
         });
         fixture.proof->setAttr("obligations", builder.getArrayAttr({node}));
       }});
  mutations.push_back(
      {"target-not-none", [&](ProofFixture &fixture) {
         mlir::OpBuilder builder(&context);
         auto node = builder.getDictionaryAttr({
             builder.getNamedAttr("id", fixture.valueID),
             builder.getNamedAttr("operator",
                                  builder.getStringAttr("constant")),
             builder.getNamedAttr("operands", fixture.node.get("operands")),
             builder.getNamedAttr(
                 "target",
                 builder.getDictionaryAttr({
                     builder.getNamedAttr(
                         "kind", builder.getStringAttr("integer_boundary")),
                     builder.getNamedAttr("domain", fixture.domain),
                 })),
         });
         fixture.proof->setAttr("obligations", builder.getArrayAttr({node}));
       }});
  mutations.push_back(
      {"result-id", [&](ProofFixture &fixture) {
         mlir::OpBuilder builder(&context);
         auto id = builder.getDictionaryAttr({
             builder.getNamedAttr("origin", fixture.valueID.get("origin")),
             builder.getNamedAttr("slot", builder.getI32IntegerAttr(1)),
         });
         fixture.proof->setAttr("result_id", id);
       }});
  mutations.push_back({"actual-result", [&](ProofFixture &fixture) {
                         fixture.proof->setOperand(
                             1, fixture.trueValue.getResult());
                       }});
  mutations.push_back({"actual-valid", [&](ProofFixture &fixture) {
                         fixture.proof->setOperand(
                             2, fixture.actualValue.getResult());
                       }});
  mutations.push_back({"path", [&](ProofFixture &fixture) {
                         fixture.proof->setOperand(
                             0, fixture.actualValue.getResult());
                       }});
  mutations.push_back({"binding-ssa", [&](ProofFixture &fixture) {
                         fixture.binding->setOperand(
                             0, fixture.trueValue.getResult());
                       }});
  mutations.push_back({"binding-valid", [&](ProofFixture &fixture) {
                         fixture.binding->setOperand(
                             1, fixture.actualValue.getResult());
                       }});
  mutations.push_back({"false-path-valid", [&](ProofFixture &fixture) {
                         mlir::OpBuilder builder(&context);
                         fixture.trueValue->setAttr(
                             "value", integerConstant(builder, 1, "0"));
                       }});
  mutations.push_back({"constant-bit-pattern", [&](ProofFixture &fixture) {
                         mlir::OpBuilder builder(&context);
                         fixture.actualValue->setAttr(
                             "value", integerConstant(builder, 3, "6"));
                       }});
  mutations.push_back({"binding-domain-width", [&](ProofFixture &fixture) {
                         mlir::OpBuilder builder(&context);
                         fixture.binding->setAttr(
                             "domain",
                             singletonDomain(builder, 4, "5", "5", "6", true));
                       }});
  mutations.push_back({"result-storage-width", [&](ProofFixture &fixture) {
                         mlir::OpBuilder builder(&context);
                         auto domain =
                             singletonDomain(builder, 4, "5", "5", "6", true);
                         fixture.proof->setAttr("result_domain", domain);
                       }});
  mutations.push_back(
      {"signedness-width", [&](ProofFixture &fixture) {
         mlir::OpBuilder builder(&context);
         auto value = mlir::arith::ConstantOp::create(
             builder, fixture.binding.getLoc(), builder.getIntegerType(4),
             integerConstant(builder, 4, "5"));
         auto domain = singletonDomain(builder, 4, "5", "5", "6", false);
         fixture.binding->setOperand(0, value.getResult());
         fixture.binding->setAttr("domain", domain);
         fixture.proof->setOperand(1, value.getResult());
         fixture.proof->setAttr("result_domain", domain);
         fixture.actualValue.erase();
       }});
  mutations.push_back(
      {"range-lower", [&](ProofFixture &fixture) {
         mlir::OpBuilder builder(&context);
         auto domain = fixture.domain;
         llvm::SmallVector<mlir::NamedAttribute> attrs(domain.begin(),
                                                       domain.end());
         for (auto &field : attrs)
           if (field.getName() == "lower")
             field = builder.getNamedAttr("lower", mathInteger(context, "4"));
         fixture.proof->setAttr("result_domain",
                                builder.getDictionaryAttr(attrs));
       }});
  mutations.push_back(
      {"range-upper", [&](ProofFixture &fixture) {
         mlir::OpBuilder builder(&context);
         llvm::SmallVector<mlir::NamedAttribute> attrs(fixture.domain.begin(),
                                                       fixture.domain.end());
         for (auto &field : attrs)
           if (field.getName() == "upper")
             field = builder.getNamedAttr("upper", mathInteger(context, "7"));
         fixture.proof->setAttr("result_domain",
                                builder.getDictionaryAttr(attrs));
       }});
  mutations.push_back({"nonconstant-result-ssa", [&](ProofFixture &fixture) {
                         mlir::OpBuilder builder(fixture.binding);
                         auto sum = builder.create<mlir::arith::AddIOp>(
                             fixture.actualValue.getLoc(),
                             fixture.actualValue.getResult(),
                             fixture.actualValue.getResult());
                         fixture.binding->setOperand(0, sum.getResult());
                         fixture.proof->setOperand(1, sum.getResult());
                       }});
  mutations.push_back({"proof-origin", [&](ProofFixture &fixture) {
                         mlir::OpBuilder builder(&context);
                         fixture.proof->setAttr(
                             "origin", occurrence(builder, "demo.Other"));
                       }});
  mutations.push_back({"coordinated-foreign-origin", [&](ProofFixture &f) {
                         useCoordinatedForeignOrigin(f, context);
                       }});
  mutations.push_back(
      {"unknown-binding-and-proof-location", [&](ProofFixture &fixture) {
         fixture.binding->setLoc(mlir::UnknownLoc::get(&context));
         fixture.proof->setLoc(mlir::UnknownLoc::get(&context));
       }});
  mutations.push_back({"proof-before-binding", [&](ProofFixture &fixture) {
                         fixture.proof->moveBefore(fixture.binding);
                       }});
  mutations.push_back(
      {"nested-proof-block", [&](ProofFixture &fixture) {
         auto location = fixture.proof.getLoc();
         auto function = mlir::func::FuncOp::create(
             location, "nested", mlir::FunctionType::get(&context, {}, {}));
         fixture.proof->getBlock()->getOperations().insert(
             fixture.proof->getIterator(), function.getOperation());
         mlir::Block *nested = function.addEntryBlock();
         fixture.proof->moveBefore(nested, nested->end());
       }});
  for (const auto &[name, mutate] : mutations) {
    SCOPED_TRACE(name);
    auto fixture = buildConstantFixture(context);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*fixture.file)))
        << "baseline exact proof fixture must verify before mutation";
    mutate(fixture);
    expectRejected(verify(context, *fixture.file));
  }
}
} // namespace
} // namespace acir::ac
