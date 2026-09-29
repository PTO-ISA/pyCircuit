#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/FileSystem.h"
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

class TemporaryDirectory {
public:
  TemporaryDirectory() {
    EXPECT_FALSE(llvm::sys::fs::createUniqueDirectory("numeric-next", path));
  }
  ~TemporaryDirectory() {
    EXPECT_FALSE(llvm::sys::fs::remove_directories(path));
  }

  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }

private:
  llvm::SmallString<256> path;
};

int run(llvm::StringRef program, const std::vector<std::string> &owned,
        llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : owned)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
}

void write(llvm::StringRef path, llvm::StringRef contents) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  ASSERT_FALSE(error);
  output << contents;
}

} // namespace

namespace numeric_next_test {

mlir::OwningOpRef<mlir::ModuleOp> compileCounter(mlir::MLIRContext &context,
                                                 bool lower) {
  TemporaryDirectory temporary;
  std::string sourceRoot = temporary.child("src");
  EXPECT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
  llvm::SmallString<256> source(sourceRoot);
  llvm::sys::path::append(source, "counter.py");
  write(source, R"py(from typing import Annotated
from pycircuit import module, rule
Word = Annotated[int, range(256)]
@module
def Counter():
    state: Word = 0
    @rule
    def advance():
        nonlocal state
        state = (state + 1) & 255
        return
    advance()
)py");
  std::string capture = temporary.child("capture.mlir");
  std::string body = temporary.child("body.mlir");
  std::string interface = temporary.child("interface.mlir");
  std::string log = temporary.child("compile.log");
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
  if (run(ACIR_TEST_PYTHON,
          {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
           source.str().str(), sourceRoot, capture},
          log) != 0)
    return {};
  std::vector<std::string> arguments = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                        "--capture",
                                        capture,
                                        "--package",
                                        "numeric_next",
                                        "--path",
                                        "counter.py",
                                        "--body-out",
                                        body,
                                        "--interface-out",
                                        interface};
  if (lower)
    arguments.push_back("--lower-numeric");
  if (run(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log) != 0)
    return {};
  return mlir::parseSourceFile<mlir::ModuleOp>(body, &context);
}

} // namespace numeric_next_test

namespace {

using numeric_next_test::compileCounter;

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

mlir::OwningOpRef<mlir::ModuleOp> clone(mlir::ModuleOp module) {
  return mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(module->clone()));
}

RuleOp onlyRule(mlir::ModuleOp file) {
  auto modules = file.getOps<ModuleOp>();
  EXPECT_EQ(std::distance(modules.begin(), modules.end()), 1);
  ModuleOp module = *modules.begin();
  auto rules = module.getBody().front().getOps<RuleOp>();
  EXPECT_EQ(std::distance(rules.begin(), rules.end()), 1);
  return *rules.begin();
}

class NumericNextUseContractsTest : public ::testing::Test {
protected:
  NumericNextUseContractsTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect,
                        mlir::scf::SCFDialect>();
  }
  mlir::MLIRContext context;
};

TEST_F(NumericNextUseContractsTest,
       SourceInventoryOwnsOneAssignmentUseAndYieldBinding) {
  auto file = compileCounter(context, false);
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*file)));
  RuleOp rule = onlyRule(*file);
  auto requiredNumeric =
      rule->getAttrOfType<mlir::ArrayAttr>("ac.required_numeric");
  auto requiredUses = rule->getAttrOfType<mlir::ArrayAttr>("ac.required_uses");
  auto yieldBindings =
      rule->getAttrOfType<mlir::ArrayAttr>("ac.yield_bindings");
  ASSERT_TRUE(requiredNumeric && requiredUses && yieldBindings);
  EXPECT_EQ(requiredNumeric.size(), 6u);
  EXPECT_EQ(requiredUses.size(), 1u);
  EXPECT_EQ(yieldBindings.size(), 1u);
  EXPECT_NE(only(file->getOperation(), "ac.source.use"), nullptr);
}

TEST_F(NumericNextUseContractsTest,
       LoweredInventoryOwnsTwoProofsOneCheckAndOneValueUse) {
  auto file = compileCounter(context, true);
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*file)));
  EXPECT_EQ(all(file->getOperation(), "ac.numeric.proof").size(), 2u);
  EXPECT_EQ(all(file->getOperation(), "ac.value.binding").size(), 6u);
  EXPECT_NE(only(file->getOperation(), "ac.expect"), nullptr);
  EXPECT_NE(only(file->getOperation(), "ac.value.use"), nullptr);
  EXPECT_EQ(only(file->getOperation(), "ac.source.use"), nullptr);
}

TEST_F(NumericNextUseContractsTest,
       MissingOrDuplicateProofCheckAndRequiredUseAreRejected) {
  auto original = compileCounter(context, true);
  ASSERT_TRUE(original);
  {
    auto damaged = clone(*original);
    all(damaged->getOperation(), "ac.numeric.proof").front()->erase();
    EXPECT_TRUE(mlir::failed(mlir::verify(*damaged)));
  }
  {
    auto damaged = clone(*original);
    RuleOp rule = onlyRule(*damaged);
    rule->removeAttr("ac.required_checks");
    EXPECT_TRUE(mlir::failed(mlir::verify(*damaged)));
  }
  {
    auto damaged = clone(*original);
    RuleOp rule = onlyRule(*damaged);
    rule->removeAttr("ac.required_uses");
    EXPECT_TRUE(mlir::failed(mlir::verify(*damaged)));
  }
  {
    auto damaged = clone(*original);
    RuleOp rule = onlyRule(*damaged);
    auto uses = rule->getAttrOfType<mlir::ArrayAttr>("ac.required_uses");
    mlir::Builder builder(&context);
    rule->setAttr("ac.required_uses", builder.getArrayAttr({uses[0], uses[0]}));
    EXPECT_TRUE(mlir::failed(mlir::verify(*damaged)));
  }
}

TEST_F(NumericNextUseContractsTest,
       SwappedYieldDataEnableAndForgedTargetAreRejected) {
  auto original = compileCounter(context, true);
  ASSERT_TRUE(original);
  {
    auto damaged = clone(*original);
    RuleOp rule = onlyRule(*damaged);
    auto yield = mlir::cast<YieldOp>(rule.getBody().front().getTerminator());
    mlir::Value data = yield.getValues()[0];
    yield->setOperand(0, yield.getValues()[1]);
    yield->setOperand(1, data);
    EXPECT_TRUE(mlir::failed(mlir::verify(*damaged)));
  }
  {
    auto damaged = clone(*original);
    RuleOp rule = onlyRule(*damaged);
    auto bindings = rule->getAttrOfType<mlir::ArrayAttr>("ac.yield_bindings");
    auto binding = mlir::cast<mlir::DictionaryAttr>(bindings[0]);
    mlir::Builder builder(&context);
    auto forged = builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("formal")),
        builder.getNamedAttr("parameter", builder.getStringAttr("forged")),
        builder.getNamedAttr("ordinal", builder.getUnitAttr()),
    });
    rule->setAttr("ac.yield_bindings",
                  builder.getArrayAttr(
                      {replaceField(builder, binding, "target", forged)}));
    EXPECT_TRUE(mlir::failed(mlir::verify(*damaged)));
  }
}

TEST_F(NumericNextUseContractsTest, WrongValueUseIdentityIsRejected) {
  auto file = compileCounter(context, true);
  ASSERT_TRUE(file);
  auto *use = only(file->getOperation(), "ac.value.use");
  ASSERT_NE(use, nullptr);
  auto bindings = all(file->getOperation(), "ac.value.binding");
  ASSERT_GE(bindings.size(), 2u);
  auto wrong = bindings.front()->getAttrOfType<mlir::DictionaryAttr>("id");
  ASSERT_TRUE(wrong);
  ASSERT_NE(wrong, use->getAttrOfType<mlir::DictionaryAttr>("source"));
  use->setAttr("source", wrong);
  EXPECT_TRUE(mlir::failed(mlir::verify(*file)));
}

} // namespace
} // namespace acir::compiler
