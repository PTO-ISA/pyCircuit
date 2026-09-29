#include "Dialect/ACIR/ACIRNumericComposition.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {

using namespace acir::ac;

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("numeric-composition", path));
  }
  ~TemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
};

void writeFile(llvm::StringRef path, llvm::StringRef contents) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  ASSERT_FALSE(error);
  output << contents;
}

std::string readFile(llvm::StringRef path) {
  auto contents = llvm::MemoryBuffer::getFile(path);
  return contents ? contents.get()->getBuffer().str() : std::string{};
}

int run(llvm::StringRef program, const std::vector<std::string> &arguments,
        llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> refs;
  for (const std::string &argument : arguments)
    refs.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, refs, std::nullopt, redirects);
}

struct Compiled {
  int status = -1;
  std::string output;
  mlir::OwningOpRef<mlir::ModuleOp> body;
};

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

class NumericCompositionContractsTest : public ::testing::Test {
protected:
  NumericCompositionContractsTest() : context(dialects) {
    dialects.insert<ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    writeFile(sourceRoot + "/composition.py", R"py(from typing import Annotated
from pycircuit import rule, system
Word = Annotated[int, range(256)]
@system
def CompositionAuthority():
    guard: bool = True
    left: Word = 1
    right: Word = 2
    sink: Word = 0
    @rule
    def choose():
        nonlocal sink
        if guard and left < 4:
            assert left != 0, "left is live"
            sink = (left + 1) & 255
        else:
            assert right != 0, "right is live"
            sink = (right + 1) & 255
    choose()
)py");
  }

  Compiled compile(bool lower) {
    const std::string source = sourceRoot + "/composition.py";
    const std::string capture = temporary.child("capture.mlir");
    const std::string bodyPath =
        temporary.child(lower ? "lowered.mlir" : "source.mlir");
    const std::string headerPath = temporary.child("interface.mlir");
    static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
root = Path(sys.argv[1])
sys.path[:0] = [str(root / "python/semantic-core/src"),
                str(root / "python/pycircuit/src"), str(root)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
source = Path(sys.argv[2])
capture = _capture_source_file(source, source_root=Path(sys.argv[3]))
Path(sys.argv[4]).write_text(_emit_source_transport(capture), encoding="utf-8")
)py";
    const std::string log =
        temporary.child(lower ? "lowered.log" : "source.log");
    int status = run(ACIR_TEST_PYTHON,
                     {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
                      source, sourceRoot, capture},
                     log);
    if (status != 0)
      return {status, readFile(log), {}};
    std::vector<std::string> arguments = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                          "--capture",
                                          capture,
                                          "--package",
                                          "composition",
                                          "--path",
                                          "composition.py",
                                          "--body-out",
                                          bodyPath,
                                          "--interface-out",
                                          headerPath};
    if (lower)
      arguments.push_back("--lower-numeric");
    status = run(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log);
    mlir::OwningOpRef<mlir::ModuleOp> body;
    if (status == 0)
      body = mlir::parseSourceFile<mlir::ModuleOp>(bodyPath, &context);
    return {status, readFile(log), std::move(body)};
  }

  mlir::OwningOpRef<mlir::ModuleOp> clone(mlir::ModuleOp module) {
    return mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(module->clone()));
  }

  RuleOp onlyRule(mlir::ModuleOp file) {
    RuleOp result;
    file.walk([&](RuleOp candidate) { result = candidate; });
    return result;
  }

  void expectRejected(mlir::ModuleOp original, llvm::StringRef label,
                      llvm::function_ref<void(mlir::ModuleOp)> mutate) {
    SCOPED_TRACE(label.str());
    auto damaged = clone(original);
    mutate(*damaged);
    EXPECT_TRUE(mlir::failed(mlir::verify(*damaged)));
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot;
};

TEST_F(NumericCompositionContractsTest,
       SourceClosesProofLocalInputsPolarityChecksAndMergedWrites) {
  Compiled compiled = compile(false);
  ASSERT_EQ(compiled.status, 0) << compiled.output;
  ASSERT_TRUE(compiled.body);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*compiled.body)));
  RuleOp rule = onlyRule(*compiled.body);
  ASSERT_TRUE(rule);
  ASSERT_TRUE(hasNumericCompositionContract(rule));
  ASSERT_TRUE(mlir::succeeded(verifyNumericCompositionClosure(rule)));

  auto required = rule->getAttrOfType<mlir::ArrayAttr>("ac.required_numeric");
  unsigned fromBits = 0;
  for (mlir::Attribute raw : required) {
    auto node = mlir::cast<mlir::DictionaryAttr>(raw);
    auto opcode = node.getAs<mlir::StringAttr>("operator");
    if (!opcode || opcode.getValue() != "from_bits")
      continue;
    ++fromBits;
    auto operands = node.getAs<mlir::ArrayAttr>("operands");
    auto input = mlir::cast<mlir::DictionaryAttr>(operands[0]);
    EXPECT_EQ(input.getAs<mlir::StringAttr>("kind").getValue(), "input");
    EXPECT_EQ(input.getAs<mlir::IntegerAttr>("index").getInt(), 0);
  }
  EXPECT_GE(fromBits, 4u);
  EXPECT_EQ(llvm::range_size(rule.getBody().front().getOps<SourceUseOp>()), 2u);
  EXPECT_EQ(llvm::range_size(rule.getBody().front().getOps<SourceExpectOp>()),
            4u);

  expectRejected(
      *compiled.body, "same-type actual source swap", [&](mlir::ModuleOp file) {
        RuleOp candidate = onlyRule(file);
        auto lifts = llvm::to_vector(
            candidate.getBody().front().getOps<MathFromBitsOp>());
        ASSERT_GE(lifts.size(), 2u);
        MathFromBitsOp first, second;
        for (MathFromBitsOp left : lifts)
          for (MathFromBitsOp right : lifts) {
            auto leftRead = left.getValue().getDefiningOp<SourceReadOp>();
            auto rightRead = right.getValue().getDefiningOp<SourceReadOp>();
            if (leftRead && rightRead &&
                leftRead.getCurrent() != rightRead.getCurrent()) {
              first = left;
              second = right;
            }
          }
        ASSERT_TRUE(first && second);
        mlir::Value original = first.getValue();
        first.getValueMutable().assign(second.getValue());
        second.getValueMutable().assign(original);
      });
  expectRejected(*compiled.body, "domain swap", [&](mlir::ModuleOp file) {
    RuleOp candidate = onlyRule(file);
    auto lift = *candidate.getBody().front().getOps<MathFromBitsOp>().begin();
    mlir::Builder builder(&context);
    auto domain = lift.getDomainAttr();
    llvm::SmallVector<mlir::NamedAttribute> fields(domain.begin(),
                                                   domain.end());
    for (mlir::NamedAttribute &field : fields)
      if (field.getName() == "upper")
        field = builder.getNamedAttr(
            "upper", MathIntAttr::get(&context,
                                      llvm::APSInt(llvm::APInt(8, 128), true)));
    lift->setAttr("domain", builder.getDictionaryAttr(fields));
  });
  expectRejected(
      *compiled.body, "nonlocal recipe input index", [&](mlir::ModuleOp file) {
        RuleOp candidate = onlyRule(file);
        auto required =
            candidate->getAttrOfType<mlir::ArrayAttr>("ac.required_numeric");
        llvm::SmallVector<mlir::Attribute> nodes(required.begin(),
                                                 required.end());
        mlir::Builder builder(&context);
        for (mlir::Attribute &raw : nodes) {
          auto node = mlir::cast<mlir::DictionaryAttr>(raw);
          auto opcode = node.getAs<mlir::StringAttr>("operator");
          if (!opcode || opcode.getValue() != "from_bits")
            continue;
          auto input = builder.getDictionaryAttr({
              builder.getNamedAttr("kind", builder.getStringAttr("input")),
              builder.getNamedAttr("index", builder.getI32IntegerAttr(1)),
          });
          raw = replaceField(builder, node, "operands",
                             builder.getArrayAttr({input}));
          break;
        }
        candidate->setAttr("ac.required_numeric", builder.getArrayAttr(nodes));
      });
  expectRejected(*compiled.body, "overlapping paths", [&](mlir::ModuleOp file) {
    RuleOp candidate = onlyRule(file);
    auto uses =
        llvm::to_vector(candidate.getBody().front().getOps<SourceUseOp>());
    ASSERT_EQ(uses.size(), 2u);
    uses[1].getPathMutable().assign(uses[0].getPath());
  });
  expectRejected(
      *compiled.body, "assert continuation bypass", [&](mlir::ModuleOp file) {
        RuleOp candidate = onlyRule(file);
        llvm::SmallVector<SourceExpectOp> assertions;
        for (SourceExpectOp check :
             candidate.getBody().front().getOps<SourceExpectOp>())
          if (check.getKind() == "assert")
            assertions.push_back(check);
        auto uses =
            llvm::to_vector(candidate.getBody().front().getOps<SourceUseOp>());
        ASSERT_EQ(assertions.size(), 2u);
        ASSERT_EQ(uses.size(), 2u);
        uses[0].getPathMutable().assign(assertions[0].getPath());
      });
  expectRejected(*compiled.body, "removed checks", [&](mlir::ModuleOp file) {
    RuleOp candidate = onlyRule(file);
    candidate->removeAttr("ac.required_checks");
    llvm::SmallVector<SourceExpectOp> checks(
        candidate.getBody().front().getOps<SourceExpectOp>());
    for (SourceExpectOp check : checks)
      check.erase();
  });
  expectRejected(
      *compiled.body, "unknown control attribute", [&](mlir::ModuleOp file) {
        RuleOp candidate = onlyRule(file);
        auto control =
            *candidate.getBody().front().getOps<mlir::arith::AndIOp>().begin();
        control->setAttr("forged", mlir::UnitAttr::get(&context));
      });
}

TEST_F(NumericCompositionContractsTest,
       LoweredClosureRejectsInputProofAndArithmeticMutation) {
  Compiled compiled = compile(true);
  ASSERT_EQ(compiled.status, 0) << compiled.output;
  ASSERT_TRUE(compiled.body);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*compiled.body)));
  RuleOp rule = onlyRule(*compiled.body);
  ASSERT_TRUE(hasNumericCompositionContract(rule));
  ASSERT_TRUE(mlir::succeeded(verifyNumericCompositionClosure(rule)));
  EXPECT_TRUE(rule.getBody().front().getOps<MathFromBitsOp>().empty());
  EXPECT_FALSE(rule.getBody().front().getOps<NumericProofOp>().empty());

  expectRejected(
      *compiled.body, "lowered input swap", [&](mlir::ModuleOp file) {
        RuleOp candidate = onlyRule(file);
        llvm::SmallVector<ValueBindingOp> inputs;
        for (ValueBindingOp binding :
             candidate.getBody().front().getOps<ValueBindingOp>()) {
          auto read = binding.getValue().getDefiningOp<SourceReadOp>();
          if (read && read.getCurrent().getType().isInteger(8))
            inputs.push_back(binding);
        }
        ASSERT_GE(inputs.size(), 2u);
        ValueBindingOp first, second;
        for (ValueBindingOp left : inputs)
          for (ValueBindingOp right : inputs) {
            auto leftRead = left.getValue().getDefiningOp<SourceReadOp>();
            auto rightRead = right.getValue().getDefiningOp<SourceReadOp>();
            if (leftRead.getCurrent() != rightRead.getCurrent()) {
              first = left;
              second = right;
            }
          }
        ASSERT_TRUE(first && second);
        mlir::Value original = first.getValue();
        first.getValueMutable().assign(second.getValue());
        second.getValueMutable().assign(original);
      });
  expectRejected(
      *compiled.body, "proof input ID swap", [&](mlir::ModuleOp file) {
        RuleOp candidate = onlyRule(file);
        auto proofs = llvm::to_vector(
            candidate.getBody().front().getOps<NumericProofOp>());
        ASSERT_GE(proofs.size(), 2u);
        auto first = proofs[0].getInputIdsAttr();
        proofs[0]->setAttr("input_ids", proofs.back().getInputIdsAttr());
        proofs.back()->setAttr("input_ids", first);
      });
  expectRejected(
      *compiled.body, "flagged arithmetic", [&](mlir::ModuleOp file) {
        mlir::arith::AddIOp add;
        file.walk([&](mlir::arith::AddIOp candidate) { add = candidate; });
        ASSERT_TRUE(add);
        add.setOverflowFlags(mlir::arith::IntegerOverflowFlags::nsw);
      });
  expectRejected(
      *compiled.body, "duplicate proof kernel", [&](mlir::ModuleOp file) {
        RuleOp candidate = onlyRule(file);
        NumericProofOp proof =
            *candidate.getBody().front().getOps<NumericProofOp>().begin();
        proof->getBlock()->getOperations().insert(proof->getIterator(),
                                                  proof->clone());
      });
  expectRejected(
      *compiled.body, "unknown proof attribute", [&](mlir::ModuleOp file) {
        auto proof =
            *onlyRule(file).getBody().front().getOps<NumericProofOp>().begin();
        proof->setAttr("forged", mlir::UnitAttr::get(&context));
      });
  expectRejected(
      *compiled.body, "unknown binding attribute", [&](mlir::ModuleOp file) {
        auto binding =
            *onlyRule(file).getBody().front().getOps<ValueBindingOp>().begin();
        binding->setAttr("forged", mlir::UnitAttr::get(&context));
      });
  expectRejected(*compiled.body, "unknown constant attribute",
                 [&](mlir::ModuleOp file) {
                   mlir::arith::ConstantOp constant;
                   file.walk([&](mlir::arith::ConstantOp candidate) {
                     if (!constant && candidate.getType().isInteger(8))
                       constant = candidate;
                   });
                   ASSERT_TRUE(constant);
                   constant->setAttr("forged", mlir::UnitAttr::get(&context));
                 });
  expectRejected(*compiled.body, "unknown AND attribute",
                 [&](mlir::ModuleOp file) {
                   auto operation = *onlyRule(file)
                                         .getBody()
                                         .front()
                                         .getOps<mlir::arith::AndIOp>()
                                         .begin();
                   operation->setAttr("forged", mlir::UnitAttr::get(&context));
                 });
  for (auto [label, flags] :
       std::array<std::pair<llvm::StringRef, mlir::arith::IntegerOverflowFlags>,
                  2>{{{"trunci nsw", mlir::arith::IntegerOverflowFlags::nsw},
                      {"trunci nuw", mlir::arith::IntegerOverflowFlags::nuw}}})
    expectRejected(*compiled.body, label, [&](mlir::ModuleOp file) {
      mlir::arith::TruncIOp truncation;
      file.walk([&](mlir::arith::TruncIOp candidate) {
        if (!truncation)
          truncation = candidate;
      });
      ASSERT_TRUE(truncation);
      truncation.setOverflowFlags(flags);
    });
  expectRejected(*compiled.body, "signed extension substitution",
                 [&](mlir::ModuleOp file) {
                   mlir::arith::ExtUIOp extension;
                   file.walk([&](mlir::arith::ExtUIOp candidate) {
                     if (!extension)
                       extension = candidate;
                   });
                   ASSERT_TRUE(extension);
                   mlir::OpBuilder builder(extension);
                   auto replacement = mlir::arith::ExtSIOp::create(
                       builder, extension.getLoc(), extension.getType(),
                       extension.getIn());
                   extension.getResult().replaceAllUsesWith(replacement);
                   extension.erase();
                 });
  expectRejected(
      *compiled.body, "narrow result domain", [&](mlir::ModuleOp file) {
        NumericProofOp proof;
        for (NumericProofOp candidate :
             onlyRule(file).getBody().front().getOps<NumericProofOp>()) {
          auto storage =
              candidate.getResultDomainAttr().getAs<mlir::TypeAttr>("storage");
          if (!proof && storage && storage.getValue().isInteger(8))
            proof = candidate;
        }
        ASSERT_TRUE(proof);
        mlir::Builder builder(&context);
        proof->setAttr(
            "result_domain",
            replaceField(builder, proof.getResultDomainAttr(), "storage",
                         mlir::TypeAttr::get(builder.getIntegerType(7))));
      });
  expectRejected(*compiled.body, "rogue cmpi path", [&](mlir::ModuleOp file) {
    RuleOp candidate = onlyRule(file);
    auto use = *candidate.getBody().front().getOps<ValueUseOp>().begin();
    mlir::OpBuilder builder(use);
    auto zero = mlir::arith::ConstantOp::create(
        builder, use.getLoc(), builder.getI8Type(),
        builder.getIntegerAttr(builder.getI8Type(), 0));
    auto rogue = mlir::arith::CmpIOp::create(
        builder, use.getLoc(), mlir::arith::CmpIPredicate::eq, zero, zero);
    use.getPathMutable().assign(rogue);
  });
  expectRejected(
      *compiled.body, "cross-proof partition", [&](mlir::ModuleOp file) {
        RuleOp candidate = onlyRule(file);
        auto proofs = llvm::to_vector(
            candidate.getBody().front().getOps<NumericProofOp>());
        auto required =
            candidate->getAttrOfType<mlir::ArrayAttr>("ac.required_numeric");
        ASSERT_GE(proofs.size(), 2u);
        mlir::Builder builder(&context);
        proofs[0]->setAttr("obligations", builder.getArrayAttr(
                                              {required[required.size() - 1]}));
      });
  expectRejected(
      *compiled.body, "bool domain integer substitution",
      [&](mlir::ModuleOp file) {
        RuleOp candidate = onlyRule(file);
        ValueBindingOp boolean;
        for (ValueBindingOp binding :
             candidate.getBody().front().getOps<ValueBindingOp>())
          if (!boolean && binding.getValue().getType().isInteger(1))
            boolean = binding;
        ASSERT_TRUE(boolean);
        mlir::Builder builder(&context);
        auto domain = builder.getDictionaryAttr({
            builder.getNamedAttr("kind", builder.getStringAttr("integer")),
            builder.getNamedAttr("storage",
                                 mlir::TypeAttr::get(builder.getI1Type())),
            builder.getNamedAttr("lower",
                                 MathIntAttr::get(&context, llvm::APSInt("0"))),
            builder.getNamedAttr("upper",
                                 MathIntAttr::get(&context, llvm::APSInt("2"))),
            builder.getNamedAttr("interpretation",
                                 builder.getStringAttr("unsigned")),
        });
        boolean->setAttr("domain", domain);
      });
}

} // namespace
} // namespace acir::compiler
