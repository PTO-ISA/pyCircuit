#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/APSInt.h"
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

namespace acir::ac {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;

  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-reg-contracts", path));
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
  auto buffer = llvm::MemoryBuffer::getFile(path);
  EXPECT_TRUE(static_cast<bool>(buffer));
  return buffer ? buffer.get()->getBuffer().str() : std::string{};
}

struct ProcessResult {
  int status;
  std::string output;
};

ProcessResult runProcess(llvm::StringRef program,
                         const std::vector<std::string> &ownedArguments,
                         llvm::StringRef logPath) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {
      std::nullopt, logPath, logPath};
  int status =
      llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
  return {status, readFile(logPath)};
}

ProcessResult compileSource(TemporaryDirectory &temporary, llvm::StringRef stem,
                            llvm::StringRef sourceText) {
  std::string sourceRoot = temporary.child((stem + "-source").str());
  EXPECT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
  std::string source = sourceRoot + "/" + stem.str() + ".py";
  std::string capture = temporary.child((stem + ".transport.mlir").str());
  std::string body = temporary.child((stem + ".body.mlir").str());
  std::string interface = temporary.child((stem + ".interface.mlir").str());
  std::string pythonLog = temporary.child((stem + ".python.log").str());
  std::string compileLog = temporary.child((stem + ".compile.log").str());
  writeFile(source, sourceText);

  static constexpr llvm::StringLiteral captureScript = R"py(
import sys
from pathlib import Path
repo = Path(sys.argv[1])
sys.path[:0] = [str(repo / "python/semantic-core/src"),
                str(repo / "python"), str(repo)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
source = Path(sys.argv[2])
root = Path(sys.argv[3])
Path(sys.argv[4]).write_text(
    _emit_source_transport(_capture_source_file(source, source_root=root)),
    encoding="utf-8")
)py";
  ProcessResult captured =
      runProcess(ACIR_TEST_PYTHON,
                 {ACIR_TEST_PYTHON, "-c", captureScript.str(),
                  ACIR_TEST_REPO_ROOT, source, sourceRoot, capture},
                 pythonLog);
  if (captured.status != 0)
    return captured;

  return runProcess(ACIR_TEST_SOURCE_UNIT_HARNESS,
                    {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", capture,
                     "--package", "verify", "--path", (stem + ".py").str(),
                     "--body-out", body, "--interface-out", interface},
                    compileLog);
}

mlir::Operation *findOnly(mlir::ModuleOp module, llvm::StringRef name) {
  mlir::Operation *found = nullptr;
  module.walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == name) {
      EXPECT_EQ(found, nullptr) << "duplicate operation " << name.str();
      found = operation;
    }
  });
  return found;
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

class RegContractsTest : public ::testing::Test {
protected:
  RegContractsTest() : context(dialects) {
    dialects.insert<ACIRDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    static constexpr llvm::StringLiteral source = R"py(
from typing import Annotated
from pycircuit import module, rule

Word = Annotated[int, range(16)]

@module
def Counter():
    q: Word = 3

    @rule
    def advance():
        nonlocal q
        q = q
        return

    advance()
)py";
    auto compiled = compileSource(temporary, "counter", source);
    ASSERT_EQ(compiled.status, 0) << compiled.output;
    body = mlir::parseSourceFile<mlir::ModuleOp>(
        temporary.child("counter.body.mlir"), &context);
    ASSERT_TRUE(body);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*body)));
  }

  mlir::OwningOpRef<mlir::ModuleOp> cloneBody() {
    return mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>((*body)->clone()));
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  mlir::OwningOpRef<mlir::ModuleOp> body;
};

TEST_F(RegContractsTest, V08_RegUsesModuleClockResetAndRuleYieldsDEPair) {
  auto *module = findOnly(*body, "ac.module");
  auto *reg = findOnly(*body, "ac.reg");
  auto *rule = findOnly(*body, "ac.rule");
  ASSERT_NE(module, nullptr);
  ASSERT_NE(reg, nullptr);
  ASSERT_NE(rule, nullptr);
  ASSERT_TRUE(module->getRegion(0).hasOneBlock());
  mlir::Block &moduleBody = module->getRegion(0).front();

  EXPECT_EQ(reg->getNumOperands(), 2u);
  EXPECT_EQ(reg->getOperand(0), moduleBody.getArgument(0));
  EXPECT_EQ(reg->getOperand(1), moduleBody.getArgument(1));
  EXPECT_EQ(reg->getNumResults(), 1u);
  EXPECT_TRUE(mlir::isa<RegType>(reg->getResult(0).getType()));

  EXPECT_EQ(rule->getNumRegions(), 1u);
  ASSERT_TRUE(rule->getRegion(0).hasOneBlock());
  EXPECT_EQ(rule->getNumResults(), 2u);
  EXPECT_FALSE(rule->getResult(0).getType().isInteger(1));
  EXPECT_TRUE(rule->getResult(1).getType().isInteger(1));
  mlir::Operation &yield = rule->getRegion(0).front().back();
  EXPECT_EQ(yield.getName().getStringRef(), "ac.yield");
  EXPECT_EQ(yield.getNumOperands(), 2u);
  EXPECT_EQ(yield.getOperand(0).getType(), rule->getResult(0).getType());
  EXPECT_TRUE(yield.getOperand(1).getType().isInteger(1));
}

TEST_F(RegContractsTest, V08_RejectsForgedClockAndWrongProposalArity) {
  auto forged = cloneBody();
  auto *module = findOnly(*forged, "ac.module");
  auto *reg = findOnly(*forged, "ac.reg");
  mlir::OpBuilder builder(&context);
  builder.setInsertionPointToStart(&module->getRegion(0).front());
  auto fakeClock = builder.create<mlir::arith::ConstantOp>(
      module->getLoc(), builder.getI1Type(), builder.getBoolAttr(false));
  reg->setOperand(0, fakeClock.getResult());
  EXPECT_TRUE(mlir::failed(mlir::verify(*forged)));

  auto wrongArity = cloneBody();
  auto *rule = findOnly(*wrongArity, "ac.rule");
  rule->getRegion(0).front().back().eraseOperand(1);
  EXPECT_TRUE(mlir::failed(mlir::verify(*wrongArity)));
}

TEST_F(RegContractsTest, V08_RetiredDffSchemasDoNotParse) {
  EXPECT_FALSE(mlir::parseSourceString<mlir::ModuleOp>(R"mlir(
    module {
      %q = "ac.dff"() : () -> !ac.dff<i8>
    }
  )mlir",
                                                       &context));
  EXPECT_FALSE(mlir::parseSourceString<mlir::ModuleOp>(R"mlir(
    module {
      %q = "ac.dffe"() : () -> !ac.dffe<i8>
    }
  )mlir",
                                                       &context));
}

TEST_F(RegContractsTest, V08_RuleSourceOwnerMustMatchItsModuleAuthority) {
  auto missing = cloneBody();
  auto *rule = findOnly(*missing, "ac.rule");
  ASSERT_NE(rule, nullptr);
  rule->removeAttr("ac.source_owner");
  EXPECT_TRUE(mlir::failed(mlir::verify(*missing)));

  auto foreign = cloneBody();
  rule = findOnly(*foreign, "ac.rule");
  mlir::Builder builder(&context);
  rule->setAttr(
      "ac.source_owner",
      builder.getDictionaryAttr({
          builder.getNamedAttr("package", builder.getStringAttr("foreign")),
          builder.getNamedAttr("path", builder.getStringAttr("other.py")),
      }));
  EXPECT_TRUE(mlir::failed(mlir::verify(*foreign)));
}

TEST_F(RegContractsTest, V09_RejectsSameWidthButDifferentLogicalType) {
  auto mutated = cloneBody();
  auto *rule = findOnly(*mutated, "ac.rule");
  mlir::Builder builder(&context);
  auto inputTypes = rule->getAttrOfType<mlir::ArrayAttr>("ac.input_types");
  ASSERT_EQ(inputTypes.size(), 1u);
  auto logical = mlir::cast<mlir::DictionaryAttr>(inputTypes[0]);
  auto upper = logical.getAs<MathIntAttr>("upper");
  ASSERT_TRUE(upper);
  auto narrowedUpper = MathIntAttr::get(&context, llvm::APSInt("15"));
  ASSERT_NE(upper, narrowedUpper);
  logical = replaceField(builder, logical, "upper", narrowedUpper);
  rule->setAttr("ac.input_types", builder.getArrayAttr({logical}));

  EXPECT_TRUE(mlir::failed(mlir::verify(*mutated)));
}

TEST_F(RegContractsTest, V16_ConcreteListMaterializesExactlyThreeRegElements) {
  static constexpr llvm::StringLiteral source = R"py(
from typing import Annotated
from pycircuit import module, rule

Word = Annotated[int, range(16)]

@module
def RegisterBank():
    cells: list[Word] = [3, 7, 11]

    @rule
    def hold():
        return

    hold()
)py";
  auto compiled = compileSource(temporary, "register_bank", source);
  ASSERT_EQ(compiled.status, 0) << compiled.output;
  auto listBody = mlir::parseSourceFile<mlir::ModuleOp>(
      temporary.child("register_bank.body.mlir"), &context);
  ASSERT_TRUE(listBody);
  llvm::SmallVector<mlir::Operation *> regs;
  listBody->walk([&](RegOp operation) { regs.push_back(operation); });
  ASSERT_EQ(regs.size(), 3u);
  mlir::DictionaryAttr declaration =
      regs.front()->getAttrOfType<mlir::DictionaryAttr>("ac.declaration");
  mlir::ArrayAttr shape =
      regs.front()->getAttrOfType<mlir::ArrayAttr>("ac.shape");
  mlir::DictionaryAttr initial =
      regs.front()->getAttrOfType<mlir::DictionaryAttr>("ac.initial_value");
  ASSERT_TRUE(declaration && shape && initial);
  ASSERT_EQ(shape.size(), 1u);
  auto shapeExpression = mlir::cast<StaticExprAttr>(shape[0]).getTree();
  auto shapeValue = shapeExpression.getAs<mlir::DictionaryAttr>("value");
  ASSERT_TRUE(shapeValue);
  EXPECT_EQ(shapeValue.getAs<MathIntAttr>("value").getCanonicalValue(), "3");
  EXPECT_EQ(initial.getAs<mlir::StringAttr>("kind").getValue(), "elements");
  auto resetValues = initial.getAs<mlir::ArrayAttr>("values");
  ASSERT_EQ(resetValues.size(), 3u);
  const std::array<llvm::StringRef, 3> expectedReset = {"3", "7", "11"};
  for (auto [index, raw] : llvm::enumerate(resetValues)) {
    auto expression = mlir::cast<StaticExprAttr>(raw).getTree();
    auto value = expression.getAs<mlir::DictionaryAttr>("value");
    ASSERT_TRUE(value);
    EXPECT_EQ(value.getAs<MathIntAttr>("value").getCanonicalValue(),
              expectedReset[index]);
  }
  for (auto [index, operation] : llvm::enumerate(regs)) {
    auto element = operation->getAttrOfType<mlir::ArrayAttr>("ac.element");
    ASSERT_EQ(element.size(), 1u);
    EXPECT_EQ(mlir::cast<mlir::IntegerAttr>(element[0]).getInt(), index);
    EXPECT_EQ(operation->getAttr("ac.declaration"), declaration);
    EXPECT_EQ(operation->getAttr("ac.shape"), shape);
    EXPECT_EQ(operation->getAttr("ac.initial_value"), initial);
  }
  auto *module = findOnly(*listBody, "ac.module");
  ASSERT_NE(module, nullptr);
  auto ports = module->getAttrOfType<mlir::ArrayAttr>("ac.ports");
  ASSERT_TRUE(ports);
  EXPECT_TRUE(ports.empty());
  bool foundQueue = false;
  listBody->walk([&](mlir::Operation *operation) {
    foundQueue |= operation->getName().getStringRef() == "ac.queue";
  });
  EXPECT_FALSE(foundQueue);
}

TEST_F(RegContractsTest, V16_RejectsEmptyCollectionInitializer) {
  static constexpr llvm::StringLiteral source = R"py(
from typing import Annotated
from pycircuit import module
Word = Annotated[int, range(16)]

@module
def EmptyBank():
    cells: list[Word] = []
)py";
  auto compiled = compileSource(temporary, "empty_bank", source);
  EXPECT_NE(compiled.status, 0);
  EXPECT_NE(
      compiled.output.find("requires a non-empty list literal initializer"),
      std::string::npos)
      << compiled.output;
}

TEST_F(RegContractsTest, V16_RejectsElementTypeAndRangeViolations) {
  static constexpr llvm::StringLiteral wrongType = R"py(
from typing import Annotated
from pycircuit import module
Word = Annotated[int, range(16)]

@module
def WrongTypeBank():
    cells: list[Word] = [3, True, 11]
)py";
  auto typeResult = compileSource(temporary, "wrong_type_bank", wrongType);
  EXPECT_NE(typeResult.status, 0);
  EXPECT_NE(typeResult.output.find("does not match"), std::string::npos)
      << typeResult.output;

  static constexpr llvm::StringLiteral outOfRange = R"py(
from typing import Annotated
from pycircuit import module
Word = Annotated[int, range(16)]

@module
def RangeBank():
    cells: list[Word] = [3, 16, 11]
)py";
  auto rangeResult = compileSource(temporary, "range_bank", outOfRange);
  EXPECT_NE(rangeResult.status, 0);
  EXPECT_NE(rangeResult.output.find("outside expected [lower, upper) bounds"),
            std::string::npos)
      << rangeResult.output;
}

TEST_F(RegContractsTest, V16_RejectsCollectionAuthorityDivergence) {
  static constexpr llvm::StringLiteral source = R"py(
from typing import Annotated
from pycircuit import module, rule

Word = Annotated[int, range(16)]

@module
def RegisterBank():
    cells: list[Word] = [3, 7, 11]

    @rule
    def hold():
        return

    hold()
)py";
  auto compiled = compileSource(temporary, "mutation_bank", source);
  ASSERT_EQ(compiled.status, 0) << compiled.output;
  auto original = mlir::parseSourceFile<mlir::ModuleOp>(
      temporary.child("mutation_bank.body.mlir"), &context);
  ASSERT_TRUE(original);

  auto collectRegs = [](mlir::ModuleOp module) {
    llvm::SmallVector<RegOp> regs;
    module.walk([&](RegOp operation) { regs.push_back(operation); });
    return regs;
  };

  auto duplicate = mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>((*original)->clone()));
  auto duplicateRegs = collectRegs(*duplicate);
  ASSERT_EQ(duplicateRegs.size(), 3u);
  duplicateRegs[1]->setAttr("ac.element",
                            duplicateRegs[0]->getAttr("ac.element"));
  EXPECT_TRUE(mlir::failed(mlir::verify(*duplicate)));

  auto missing = mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>((*original)->clone()));
  auto missingRegs = collectRegs(*missing);
  ASSERT_EQ(missingRegs.size(), 3u);
  ASSERT_TRUE(missingRegs[1]->use_empty());
  missingRegs[1]->erase();
  EXPECT_TRUE(mlir::failed(mlir::verify(*missing)));

  auto divergent = mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>((*original)->clone()));
  auto divergentRegs = collectRegs(*divergent);
  ASSERT_EQ(divergentRegs.size(), 3u);
  mlir::Builder builder(&context);
  auto initial =
      divergentRegs[1]->getAttrOfType<mlir::DictionaryAttr>("ac.initial_value");
  auto values = initial.getAs<mlir::ArrayAttr>("values");
  ASSERT_EQ(values.size(), 3u);
  llvm::SmallVector<mlir::Attribute> changedValues(values.begin(),
                                                   values.end());
  auto middle = mlir::cast<StaticExprAttr>(changedValues[1]);
  auto tree = middle.getTree();
  auto value = tree.getAs<mlir::DictionaryAttr>("value");
  ASSERT_TRUE(value);
  value = replaceField(builder, value, "value",
                       MathIntAttr::get(&context, llvm::APSInt("8")));
  tree = replaceField(builder, tree, "value", value);
  changedValues[1] = StaticExprAttr::get(&context, tree);
  initial = replaceField(builder, initial, "values",
                         builder.getArrayAttr(changedValues));
  divergentRegs[1]->setAttr("ac.initial_value", initial);
  EXPECT_TRUE(mlir::failed(mlir::verify(*divergent)));
}

TEST_F(RegContractsTest, V17_ValueElementIsEmptyAndBoolIndexIsRejected) {
  auto *reg = findOnly(*body, "ac.reg");
  ASSERT_NE(reg, nullptr);
  auto element = reg->getAttrOfType<mlir::ArrayAttr>("ac.element");
  ASSERT_TRUE(element);
  EXPECT_TRUE(element.empty());

  auto mutated = cloneBody();
  reg = findOnly(*mutated, "ac.reg");
  mlir::Builder builder(&context);
  auto initial = reg->getAttrOfType<mlir::DictionaryAttr>("ac.initial_value");
  auto initializer = initial.getAs<StaticExprAttr>("value");
  ASSERT_TRUE(initializer);
  reg->setAttr("ac.shape", builder.getArrayAttr({initializer}));
  reg->setAttr(
      "ac.initial_value",
      replaceField(builder, initial, "kind", builder.getStringAttr("repeat")));
  reg->setAttr("ac.element",
               builder.getArrayAttr({builder.getBoolAttr(false)}));
  EXPECT_TRUE(mlir::failed(mlir::verify(*mutated)));
}

} // namespace
} // namespace acir::ac
