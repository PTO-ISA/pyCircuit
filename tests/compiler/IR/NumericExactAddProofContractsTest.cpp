#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <functional>
#include <string>
#include <utility>
#include <vector>
namespace acir::ac {
namespace {

llvm::APInt integerBits(llvm::StringRef text, unsigned width) {
  bool negative = text.consume_front("-");
  llvm::APInt value(width, text, 10);
  return negative ? -value : value;
}
MathIntAttr mathInteger(mlir::MLIRContext &context, llvm::StringRef text) {
  unsigned width = static_cast<unsigned>(text.size() * 4 + 8);
  bool negative = text.consume_front("-");
  llvm::APInt value(width, text, 10);
  if (negative)
    value = -value;
  return MathIntAttr::get(&context, llvm::APSInt(std::move(value), !negative));
}
std::string addOne(llvm::StringRef text) {
  llvm::APInt value = integerBits(text, 256);
  ++value;
  llvm::APSInt signedValue(value, false);
  llvm::SmallString<64> result;
  signedValue.toString(result);
  return result.str().str();
}
std::string sumValues(llvm::StringRef lhs, llvm::StringRef rhs) {
  llvm::APInt result = integerBits(lhs, 256) + integerBits(rhs, 256);
  llvm::APSInt signedValue(result, false);
  llvm::SmallString<64> text;
  signedValue.toString(text);
  return text.str().str();
}
unsigned minimumWidth(llvm::StringRef text, bool isSigned) {
  llvm::APInt value = integerBits(text, 256);
  llvm::APSInt typed(value, !isSigned);
  return std::max(1u, isSigned ? typed.getSignificantBits()
                               : typed.getActiveBits());
}

mlir::DictionaryAttr occurrence(mlir::OpBuilder &builder,
                                llvm::StringRef definition = "demo.Compute",
                                unsigned line = 7) {
  auto site = builder.getDictionaryAttr({
      builder.getNamedAttr("definition", mlir::FlatSymbolRefAttr::get(
                                             builder.getContext(), definition)),
      builder.getNamedAttr(
          "ast_path",
          builder.getArrayAttr({
              builder.getDictionaryAttr({
                  builder.getNamedAttr("kind", builder.getStringAttr("index")),
                  builder.getNamedAttr("value",
                                       builder.getI64IntegerAttr(line)),
              }),
          })),
  });
  return builder.getDictionaryAttr({
      builder.getNamedAttr("site", site),
      builder.getNamedAttr("expansion", builder.getArrayAttr({})),
  });
}

mlir::DictionaryAttr valueID(mlir::OpBuilder &builder,
                             mlir::DictionaryAttr origin, unsigned slot) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("origin", origin),
      builder.getNamedAttr("slot", builder.getI32IntegerAttr(slot)),
  });
}

mlir::DictionaryAttr integerDomain(mlir::OpBuilder &builder, unsigned width,
                                   llvm::StringRef lower, llvm::StringRef upper,
                                   bool isSigned) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("storage",
                           mlir::TypeAttr::get(builder.getIntegerType(width))),
      builder.getNamedAttr("lower", mathInteger(*builder.getContext(), lower)),
      builder.getNamedAttr("upper", mathInteger(*builder.getContext(), upper)),
      builder.getNamedAttr(
          "interpretation",
          builder.getStringAttr(isSigned ? "signed" : "unsigned")),
  });
}

mlir::DictionaryAttr constantReference(mlir::OpBuilder &builder,
                                       llvm::StringRef value) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("constant")),
      builder.getNamedAttr("value", mathInteger(*builder.getContext(), value)),
  });
}

mlir::DictionaryAttr nodeReference(mlir::OpBuilder &builder, unsigned index) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("node")),
      builder.getNamedAttr("index", builder.getI32IntegerAttr(index)),
  });
}

mlir::IntegerAttr integerAttr(mlir::OpBuilder &builder, unsigned width,
                              llvm::StringRef value) {
  return builder.getIntegerAttr(builder.getIntegerType(width),
                                integerBits(value, width));
}

struct AddFixture {
  mlir::OwningOpRef<mlir::ModuleOp> file;
  RuleOp rule;
  mlir::arith::ConstantOp lhsConstant, rhsConstant;
  mlir::Operation *lhsExtension = nullptr, *rhsExtension = nullptr;
  mlir::Value lhsActual, rhsActual;
  mlir::arith::AddIOp sum;
  ValueBindingOp lhsBinding, rhsBinding, resultBinding;
  NumericProofOp proof;
  mlir::DictionaryAttr lhsID, rhsID, resultID;
  mlir::ArrayAttr nodes;
};

mlir::DictionaryAttr replaceNodeField(mlir::OpBuilder &builder,
                                      mlir::Attribute nodeAttribute,
                                      llvm::StringRef name,
                                      mlir::Attribute value) {
  auto node = mlir::cast<mlir::DictionaryAttr>(nodeAttribute);
  llvm::SmallVector<mlir::NamedAttribute> fields(node.begin(), node.end());
  for (auto &field : fields)
    if (field.getName() == name)
      field = builder.getNamedAttr(name, value);
  return builder.getDictionaryAttr(fields);
}

void installNodeMutation(AddFixture &fixture, mlir::OpBuilder &builder,
                         unsigned index, mlir::DictionaryAttr node) {
  llvm::SmallVector<mlir::Attribute> nodes(fixture.nodes.begin(),
                                           fixture.nodes.end());
  nodes[index] = node;
  auto obligations = builder.getArrayAttr(nodes);
  fixture.proof->setAttr("obligations", obligations);
  fixture.rule->setAttr("ac.required_numeric", obligations);
}

AddFixture buildAddFixture(mlir::MLIRContext &context, llvm::StringRef lhs,
                           llvm::StringRef rhs, bool lhsSigned, bool rhsSigned,
                           bool resultSigned) {
  std::string result = sumValues(lhs, rhs);
  unsigned lhsWidth = minimumWidth(lhs, lhsSigned);
  unsigned rhsWidth = minimumWidth(rhs, rhsSigned);
  unsigned resultWidth = minimumWidth(result, resultSigned);
  std::string lhsUpper = addOne(lhs), rhsUpper = addOne(rhs);
  std::string resultUpper = addOne(result);
  mlir::OpBuilder builder(&context);
  auto location = mlir::FileLineColLoc::get(&context, "demo.py", 7, 3);
  auto origin = occurrence(builder);
  auto lhsID = valueID(builder, origin, 0);
  auto rhsID = valueID(builder, origin, 1);
  auto resultID = valueID(builder, origin, 2);
  auto lhsDomain = integerDomain(builder, lhsWidth, lhs, lhsUpper, lhsSigned);
  auto rhsDomain = integerDomain(builder, rhsWidth, rhs, rhsUpper, rhsSigned);
  auto resultDomain =
      integerDomain(builder, resultWidth, result, resultUpper, resultSigned);
  auto target = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("none")),
  });
  auto makeConstantNode = [&](mlir::DictionaryAttr id, llvm::StringRef value) {
    return builder.getDictionaryAttr({
        builder.getNamedAttr("id", id),
        builder.getNamedAttr("operator", builder.getStringAttr("constant")),
        builder.getNamedAttr(
            "operands",
            builder.getArrayAttr({constantReference(builder, value)})),
        builder.getNamedAttr("target", target),
    });
  };
  auto lhsNode = makeConstantNode(lhsID, lhs);
  auto rhsNode = makeConstantNode(rhsID, rhs);
  auto resultNode = builder.getDictionaryAttr({
      builder.getNamedAttr("id", resultID),
      builder.getNamedAttr("operator", builder.getStringAttr("add")),
      builder.getNamedAttr("operands",
                           builder.getArrayAttr({nodeReference(builder, 0),
                                                 nodeReference(builder, 1)})),
      builder.getNamedAttr("target", target),
  });
  auto nodes = builder.getArrayAttr({lhsNode, rhsNode, resultNode});
  auto file = mlir::ModuleOp::create(location);
  file->setAttr("ac.stage", builder.getStringAttr("source"));
  file->setAttr("ac.unit_kind", builder.getStringAttr("implementation"));
  auto owner = builder.getDictionaryAttr({
      builder.getNamedAttr("package", builder.getStringAttr("demo")),
      builder.getNamedAttr("path", builder.getStringAttr("demo.py")),
  });
  file->setAttr("ac.source_owner", owner);
  builder.setInsertionPointToStart(file.getBody());
  mlir::OperationState hardwareState(location, ModuleOp::getOperationName());
  hardwareState.addAttribute("name", builder.getStringAttr("Compute"));
  hardwareState.addAttribute("sym_name", builder.getStringAttr("demo.Compute"));
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
  auto hardware = mlir::cast<ModuleOp>(builder.create(hardwareState));
  auto *hardwareBlock = new mlir::Block();
  hardware.getBody().push_back(hardwareBlock);
  hardwareBlock->addArgument(builder.getI1Type(), location);
  hardwareBlock->addArgument(builder.getI1Type(), location);
  builder.setInsertionPointToEnd(hardwareBlock);
  auto specialization = builder.getDictionaryAttr({
      builder.getNamedAttr(
          "definition", mlir::FlatSymbolRefAttr::get(&context, "demo.Compute")),
      builder.getNamedAttr("arguments", builder.getArrayAttr({})),
  });
  auto registration = occurrence(builder, "demo.Compute", 10);
  auto proofScope = builder.getDictionaryAttr({
      builder.getNamedAttr("specialization", specialization),
      builder.getNamedAttr("registration", registration),
  });
  mlir::OperationState ruleState(location, RuleOp::getOperationName());
  ruleState.addAttribute("name", builder.getStringAttr("constant_add"));
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
  ruleState.addAttribute("ac.required_numeric", nodes);
  ruleState.addRegion();
  auto rule = mlir::cast<RuleOp>(builder.create(ruleState));
  auto *body = new mlir::Block();
  rule.getBody().push_back(body);
  builder.setInsertionPointToEnd(body);
  auto makeConstant = [&](unsigned width, llvm::StringRef value) {
    mlir::OperationState state(location,
                               mlir::arith::ConstantOp::getOperationName());
    state.addAttribute("value", integerAttr(builder, width, value));
    state.addTypes(builder.getIntegerType(width));
    return mlir::cast<mlir::arith::ConstantOp>(builder.create(state));
  };
  auto lhsConstant = makeConstant(lhsWidth, lhs);
  auto rhsConstant = makeConstant(rhsWidth, rhs);
  auto makeExtension = [&](mlir::Value source, unsigned sourceWidth,
                           bool isSigned, mlir::Operation *&created) {
    if (sourceWidth == resultWidth)
      return source;
    if (isSigned) {
      created =
          mlir::arith::ExtSIOp::create(
              builder, location, builder.getIntegerType(resultWidth), source)
              .getOperation();
    } else {
      created =
          mlir::arith::ExtUIOp::create(
              builder, location, builder.getIntegerType(resultWidth), source)
              .getOperation();
    }
    return mlir::Value(created->getResult(0));
  };
  mlir::Operation *lhsExtension = nullptr, *rhsExtension = nullptr;
  auto lhsActual =
      makeExtension(lhsConstant.getResult(), lhsWidth, lhsSigned, lhsExtension);
  auto rhsActual =
      makeExtension(rhsConstant.getResult(), rhsWidth, rhsSigned, rhsExtension);
  auto sum = mlir::arith::AddIOp::create(builder, location,
                                         builder.getIntegerType(resultWidth),
                                         lhsActual, rhsActual);
  auto trueValue = mlir::arith::ConstantOp::create(
      builder, location, builder.getI1Type(), builder.getBoolAttr(true));
  auto makeBinding = [&](mlir::Value value, mlir::DictionaryAttr id,
                         mlir::DictionaryAttr domain) {
    mlir::OperationState state(location, ValueBindingOp::getOperationName());
    state.addOperands({value, trueValue.getResult(), trueValue.getResult()});
    state.addAttribute("id", id);
    state.addAttribute("domain", domain);
    return mlir::cast<ValueBindingOp>(builder.create(state));
  };
  auto lhsBinding = makeBinding(lhsConstant.getResult(), lhsID, lhsDomain);
  auto rhsBinding = makeBinding(rhsConstant.getResult(), rhsID, rhsDomain);
  auto resultBinding = makeBinding(sum.getResult(), resultID, resultDomain);
  mlir::OperationState proofState(location, NumericProofOp::getOperationName());
  proofState.addOperands(
      {trueValue.getResult(), sum.getResult(), trueValue.getResult()});
  proofState.addAttribute("operand_segment_sizes",
                          builder.getDenseI32ArrayAttr({1, 0, 0, 1, 1, 0, 0}));
  proofState.addAttribute("mode", builder.getStringAttr("exact"));
  proofState.addAttribute("result_domain", resultDomain);
  proofState.addAttribute("input_ids", builder.getArrayAttr({}));
  proofState.addAttribute("input_domains", builder.getArrayAttr({}));
  proofState.addAttribute("result_id", resultID);
  proofState.addAttribute("obligations", nodes);
  proofState.addAttribute("checks", builder.getArrayAttr({}));
  proofState.addAttribute("origin", origin);
  auto proof = mlir::cast<NumericProofOp>(builder.create(proofState));
  mlir::OperationState ruleYield(location, YieldOp::getOperationName());
  builder.create(ruleYield);
  builder.setInsertionPointToEnd(hardwareBlock);
  mlir::OperationState moduleYield(location, YieldOp::getOperationName());
  builder.create(moduleYield);
  return {
      std::move(file), rule,          lhsConstant, rhsConstant, lhsExtension,
      rhsExtension,    lhsActual,     rhsActual,   sum,         lhsBinding,
      rhsBinding,      resultBinding, proof,       lhsID,       rhsID,
      resultID,        nodes};
}
struct Verification {
  bool passed;
  std::string diagnostic;
};
Verification verify(mlir::MLIRContext &context, mlir::Operation *operation) {
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &diag) {
    llvm::raw_string_ostream(diagnostic) << diag;
    return mlir::success();
  });
  return {mlir::succeeded(mlir::verify(operation)), diagnostic};
}
class NumericExactAddProofContractsTest : public ::testing::Test {
protected:
  NumericExactAddProofContractsTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect>();
  }
  mlir::MLIRContext context;
};
TEST_F(NumericExactAddProofContractsTest,
       AcceptsExactConstantAdditionAcrossFiniteWidth) {
  struct Case {
    const char *lhs, *rhs;
    bool lhsS, rhsS, resultS;
  };
  const std::array<Case, 7> cases = {
      {{"1", "1", false, false, false},
       {"3", "1", false, false, false},
       {"-1", "-1", true, true, true},
       {"-2", "-1", true, true, true},
       {"-1", "1", true, false, false},
       {"18446744073709551614", "1", false, false, false},
       {"-9223372036854775807", "-1", true, true, true}}};
  for (const Case test : cases) {
    SCOPED_TRACE(std::string(test.lhs) + " + " + test.rhs);
    auto fixture = buildAddFixture(context, test.lhs, test.rhs, test.lhsS,
                                   test.rhsS, test.resultS);
    auto result = verify(context, *fixture.file);
    EXPECT_TRUE(result.passed) << result.diagnostic;
  }
}
TEST_F(NumericExactAddProofContractsTest, RejectsAddWitnessMutations) {
  using Mutation = std::function<void(AddFixture &)>;
  std::vector<std::pair<std::string, Mutation>> mutations;
  mutations.push_back({"node-order", [&](AddFixture &f) {
                         mlir::OpBuilder b(&context);
                         auto n = f.nodes;
                         auto nodes = b.getArrayAttr({n[1], n[0], n[2]});
                         f.proof->setAttr("obligations", nodes);
                         f.rule->setAttr("ac.required_numeric", nodes);
                       }});
  mutations.push_back({"bad-reference", [&](AddFixture &f) {
                         mlir::OpBuilder b(&context);
                         auto ref = [&](bool index) {
                           return b.getDictionaryAttr({
                               b.getNamedAttr("kind", b.getStringAttr("node")),
                               b.getNamedAttr("index", b.getBoolAttr(index)),
                           });
                         };
                         auto refs = b.getArrayAttr({ref(false), ref(true)});
                         auto node =
                             replaceNodeField(b, f.nodes[2], "operands", refs);
                         installNodeMutation(f, b, 2, node);
                       }});
  mutations.push_back({"node-id", [&](AddFixture &f) {
                         mlir::OpBuilder b(&context);
                         auto node =
                             replaceNodeField(b, f.nodes[2], "id", f.lhsID);
                         installNodeMutation(f, b, 2, node);
                       }});
  mutations.push_back({"result-id", [&](AddFixture &f) {
                         f.proof->setAttr("result_id", f.lhsID);
                       }});
  mutations.push_back({"swap-input-binding-ids", [&](AddFixture &f) {
                         f.lhsBinding->setAttr("id", f.rhsID);
                         f.rhsBinding->setAttr("id", f.lhsID);
                       }});
  mutations.push_back({"proof-origin", [&](AddFixture &f) {
                         mlir::OpBuilder b(&context);
                         f.proof->setAttr("origin",
                                          occurrence(b, "demo.Other"));
                       }});
  mutations.push_back({"recipe-sub", [&](AddFixture &f) {
                         mlir::OpBuilder b(&context);
                         auto node = replaceNodeField(b, f.nodes[2], "operator",
                                                      b.getStringAttr("sub"));
                         installNodeMutation(f, b, 2, node);
                       }});
  mutations.push_back(
      {"target", [&](AddFixture &f) {
         mlir::OpBuilder b(&context);
         auto target = b.getDictionaryAttr(
             {b.getNamedAttr("kind", b.getStringAttr("boundary"))});
         auto node = replaceNodeField(b, f.nodes[2], "target", target);
         installNodeMutation(f, b, 2, node);
       }});
  mutations.push_back({"actual-subi", [&](AddFixture &f) {
                         mlir::OpBuilder b(f.sum);
                         auto sub = mlir::arith::SubIOp::create(
                             b, f.sum.getLoc(), f.sum.getType(), f.sum.getLhs(),
                             f.sum.getRhs());
                         f.resultBinding->setOperand(0, sub.getResult());
                         f.proof->setOperand(1, sub.getResult());
                         f.sum.erase();
                       }});
  mutations.push_back({"missing-extension", [&](AddFixture &f) {
                         f.sum->setOperand(0, f.lhsConstant.getResult());
                       }});
  mutations.push_back({"wrong-extension-source", [&](AddFixture &f) {
                         f.lhsExtension->setOperand(0,
                                                    f.rhsConstant.getResult());
                       }});
  mutations.push_back({"signedness-extension", [&](AddFixture &f) {
                         mlir::OpBuilder b(f.lhsExtension);
                         mlir::OperationState s(f.lhsExtension->getLoc(),
                                                "arith.extsi");
                         s.addOperands(f.lhsConstant.getResult());
                         s.addTypes(f.lhsExtension->getResult(0).getType());
                         auto *ext = b.create(s);
                         f.sum->setOperand(0, ext->getResult(0));
                       }});
  mutations.push_back(
      {"constant-pattern", [&](AddFixture &f) {
         mlir::OpBuilder b(&context);
         f.lhsConstant->setAttr(
             "value",
             integerAttr(b, f.lhsConstant.getType().getIntOrFloatBitWidth(),
                         "2"));
       }});
  mutations.push_back({"constant-recipe-value", [&](AddFixture &f) {
                         mlir::OpBuilder b(&context);
                         auto ref =
                             constantReference(b, "73786976294838206467");
                         auto node = replaceNodeField(b, f.nodes[0], "operands",
                                                      b.getArrayAttr({ref}));
                         installNodeMutation(f, b, 0, node);
                       }});
  mutations.push_back({"result-width", [&](AddFixture &f) {
                         mlir::OpBuilder b(&context);
                         f.proof->setAttr("result_domain",
                                          integerDomain(b, 1, "2", "3", false));
                       }});
  mutations.push_back(
      {"result-range", [&](AddFixture &f) {
         mlir::OpBuilder b(&context);
         f.proof->setAttr("result_domain",
                          integerDomain(b,
                                        f.sum.getType().getIntOrFloatBitWidth(),
                                        "0", "3", false));
       }});
  mutations.push_back(
      {"input-domain-signedness", [&](AddFixture &f) {
         mlir::OpBuilder b(f.sum);
         auto signedConstant = mlir::arith::ConstantOp::create(
             b, f.lhsBinding.getLoc(), b.getIntegerType(2),
             integerAttr(b, 2, "1"));
         f.lhsBinding->setAttr("domain", integerDomain(b, 2, "1", "2", true));
         f.lhsBinding->setOperand(0, signedConstant.getResult());
         f.sum->setOperand(0, signedConstant.getResult());
         f.lhsExtension->erase();
         f.lhsConstant.erase();
       }});
  mutations.push_back({"false-controls", [&](AddFixture &f) {
                         mlir::OpBuilder b(f.lhsBinding);
                         auto falseValue = mlir::arith::ConstantOp::create(
                             b, f.lhsBinding.getLoc(), b.getI1Type(),
                             b.getBoolAttr(false));
                         for (ValueBindingOp binding :
                              {f.lhsBinding, f.rhsBinding, f.resultBinding}) {
                           binding->setOperand(1, falseValue.getResult());
                           binding->setOperand(2, falseValue.getResult());
                         }
                         f.proof->setOperand(0, falseValue.getResult());
                         f.proof->setOperand(2, falseValue.getResult());
                       }});
  mutations.push_back({"required-mismatch", [&](AddFixture &f) {
                         mlir::OpBuilder b(&context);
                         f.rule->setAttr(
                             "ac.required_numeric",
                             b.getArrayAttr({f.nodes[0], f.nodes[1]}));
                       }});
  mutations.push_back(
      {"extra-required-node", [&](AddFixture &f) {
         mlir::OpBuilder b(&context);
         f.rule->setAttr(
             "ac.required_numeric",
             b.getArrayAttr({f.nodes[0], f.nodes[1], f.nodes[2], f.nodes[2]}));
       }});
  mutations.push_back({"nonempty-inputs", [&](AddFixture &f) {
                         mlir::OpBuilder b(&context);
                         f.proof->setAttr("input_ids",
                                          b.getArrayAttr({f.lhsID}));
                       }});
  mutations.push_back({"nonempty-checks", [&](AddFixture &f) {
                         mlir::OpBuilder b(&context);
                         f.proof->setAttr("checks",
                                          b.getArrayAttr({f.nodes[0]}));
                       }});
  mutations.push_back(
      {"low-bits", [&](AddFixture &f) {
         f.proof->setAttr("mode", mlir::StringAttr::get(&context, "low_bits"));
       }});
  mutations.push_back(
      {"forbidden-width-attribute", [&](AddFixture &f) {
         f.proof->setAttr(
             "width",
             mlir::IntegerAttr::get(mlir::IntegerType::get(&context, 32), 2));
       }});
  mutations.push_back({"extra-operation", [&](AddFixture &f) {
                         mlir::OpBuilder b(f.resultBinding);
                         (void)mlir::arith::AddIOp::create(
                             b, f.sum.getLoc(), f.sum.getType(), f.sum.getLhs(),
                             f.sum.getRhs());
                       }});
  for (const auto &[name, mutate] : mutations) {
    SCOPED_TRACE(name);
    auto f = buildAddFixture(context, "1", "1", false, false, false);
    auto baseline = verify(context, *f.file);
    ASSERT_TRUE(baseline.passed)
        << "value-input baseline must verify: " << baseline.diagnostic;
    mutate(f);
    EXPECT_FALSE(verify(context, *f.file).passed);
  }
}
TEST_F(NumericExactAddProofContractsTest, RejectsUnsupportedRecipes) {
  for (llvm::StringRef op : {"sub", "compare", "and_bits", "from_bits"}) {
    auto f = buildAddFixture(context, "1", "1", false, false, false);
    mlir::OpBuilder b(&context);
    auto n = mlir::cast<mlir::DictionaryAttr>(f.nodes[2]);
    llvm::SmallVector<mlir::NamedAttribute> a(n.begin(), n.end());
    for (auto &x : a)
      if (x.getName() == "operator")
        x = b.getNamedAttr("operator", b.getStringAttr(op));
    auto nodes =
        b.getArrayAttr({f.nodes[0], f.nodes[1], b.getDictionaryAttr(a)});
    f.proof->setAttr("obligations", nodes);
    f.rule->setAttr("ac.required_numeric", nodes);
    EXPECT_FALSE(verify(context, *f.file).passed) << op.str();
  }
}
TEST_F(NumericExactAddProofContractsTest,
       RejectsUnrepresentableAndNarrowingResults) {
  EXPECT_FALSE(verify(context, *buildAddFixture(context, "18446744073709551615",
                                                "1", false, false, false)
                                    .file)
                   .passed);
  EXPECT_FALSE(verify(context, *buildAddFixture(context, "-9223372036854775808",
                                                "-1", true, true, true)
                                    .file)
                   .passed);
  EXPECT_FALSE(
      verify(context,
             *buildAddFixture(context, "8", "-8", false, true, false).file)
          .passed);
}
TEST_F(NumericExactAddProofContractsTest,
       CommutativeAddAcceptsSwappedActualOperands) {
  auto f = buildAddFixture(context, "255", "1", false, false, false);
  f.sum->setOperands({f.rhsActual, f.lhsActual});
  EXPECT_TRUE(verify(context, *f.file).passed);
}
} // namespace
} // namespace acir::ac
