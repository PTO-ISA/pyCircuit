#include "Compiler/FinalProgram.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <concepts>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace acir::compiler {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-final-realization", path));
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

template <typename Program>
concept HasPostOrderInstanceOrdinals =
    requires(const Program &program) { program.postOrderInstanceOrdinals(); };

template <typename Carrier>
concept HasClockResetFacts = requires(Carrier carrier) {
  carrier.clock;
  carrier.reset;
};

class FinalRegControlClosureTest : public ::testing::Test {
protected:
  FinalRegControlClosureTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("src");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    childSource = sourceRoot + "/child.py";
    rootSource = sourceRoot + "/root.py";
    writeFile(childSource, R"py(from pycircuit import module, rule, log

@module
def Child(first: bool, second: bool, sink: bool):
    private_state: bool = False
    @rule
    def transfer():
        nonlocal private_state, sink
        if first:
            private_state = second
            sink = private_state
            log("info", "state", private_state)
            return
        else:
            assert second, "second input is high on alternate path"
            log("debug", "alternate", second)
            return
    transfer()
)py");
    writeFile(rootSource, R"py(from pycircuit import module
from .child import Child

@module
def Root():
    one: bool = True
    zero: bool = False
    left_sink: bool = False
    right_sink: bool = False
    left = Child(one, zero, left_sink)
    right = Child(zero, one, right_sink)
)py");

    childBodyPath = temporary.child("child.body.mlir");
    childHeaderPath = temporary.child("child.interface.mlir");
    rootBodyPath = temporary.child("root.body.mlir");
    rootHeaderPath = temporary.child("root.interface.mlir");
    ASSERT_TRUE(compileUnit(childSource, "child.py", std::nullopt,
                            childBodyPath, childHeaderPath));
    ASSERT_TRUE(compileUnit(rootSource, "root.py", childHeaderPath,
                            rootBodyPath, rootHeaderPath));
    childBody = mlir::parseSourceFile<mlir::ModuleOp>(childBodyPath, &context);
    childHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(childHeaderPath, &context);
    rootBody = mlir::parseSourceFile<mlir::ModuleOp>(rootBodyPath, &context);
    rootHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(rootHeaderPath, &context);
    ASSERT_TRUE(childBody && childHeader && rootBody && rootHeader);
    ASSERT_FALSE(llvm::sys::fs::remove_directories(sourceRoot));
  }

  bool compileUnit(llvm::StringRef source, llvm::StringRef relativePath,
                   std::optional<llvm::StringRef> dependencyHeader,
                   llvm::StringRef body, llvm::StringRef header) {
    std::string stem = relativePath.str();
    std::replace(stem.begin(), stem.end(), '/', '_');
    std::replace(stem.begin(), stem.end(), '.', '_');
    const std::string capture = temporary.child(stem + ".capture.mlir");
    static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
root = Path(sys.argv[1])
sys.path[:0] = [str(root / "python/semantic-core/src"),
                str(root / "python/pycircuit/src"), str(root)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
capture = _capture_source_file(Path(sys.argv[2]), source_root=Path(sys.argv[3]))
Path(sys.argv[4]).write_text(_emit_source_transport(capture), encoding="utf-8")
)py";
    if (run(ACIR_TEST_PYTHON,
            {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
             source.str(), sourceRoot, capture},
            temporary.child(stem + ".capture.log")) != 0)
      return false;
    std::vector<std::string> args = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                     "--capture",
                                     capture,
                                     "--package",
                                     "verify",
                                     "--path",
                                     relativePath.str()};
    if (dependencyHeader) {
      args.push_back("--header");
      args.push_back(dependencyHeader->str());
    }
    args.insert(args.end(),
                {"--body-out", body.str(), "--interface-out", header.str()});
    const std::string log = temporary.child(stem + ".compile.log");
    if (run(ACIR_TEST_SOURCE_UNIT_HARNESS, args, log) == 0)
      return true;
    llvm::errs() << readFile(log) << "\n";
    return false;
  }

  auto emitError() {
    return [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
  }

  mlir::FailureOr<FinalProgram> build(bool reverse = false) {
    SourceLinkUnit child{*childBody, *childHeader};
    SourceLinkUnit root{*rootBody, *rootHeader};
    llvm::SmallVector<SourceLinkUnit> units =
        reverse ? llvm::SmallVector<SourceLinkUnit>{child, root}
                : llvm::SmallVector<SourceLinkUnit>{root, child};
    auto analysis = buildFinalProgram(units, emitError());
    if (mlir::failed(analysis))
      return mlir::failure();
    return materializeFinalProgram(std::move(*analysis), emitError());
  }

  template <typename Mutate> void expectMutationRejected(Mutate mutate) {
    auto ready = build();
    ASSERT_TRUE(mlir::succeeded(ready));
    ASSERT_TRUE(ready->isEmitReady());
    ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));
    ASSERT_TRUE(mlir::succeeded(emitFinalCpp(*ready, emitError())));
    ASSERT_TRUE(mlir::succeeded(emitFinalVerilog(*ready, emitError())));
    ASSERT_TRUE(mutate(*ready));
    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalCpp(*ready, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalVerilog(*ready, emitError())));
  }

  static ac::RegOp firstOwnedReg(FinalProgram &program) {
    for (const auto &carrier : program.stateCarriers())
      if (auto reg = mlir::dyn_cast_or_null<ac::RegOp>(carrier.declaration))
        return reg;
    return {};
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot, childSource, rootSource;
  std::string childBodyPath, childHeaderPath, rootBodyPath, rootHeaderPath;
  mlir::OwningOpRef<mlir::ModuleOp> childBody, childHeader, rootBody,
      rootHeader;
};

TEST_F(FinalRegControlClosureTest, EmitReadyCarriersMatchTheirOwnerControls) {
  auto ready = build();
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_TRUE(ready->isEmitReady());
  ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));
  ASSERT_FALSE(ready->stateCarriers().empty());
  for (const auto &carrier : ready->stateCarriers()) {
    auto reg = mlir::dyn_cast_or_null<ac::RegOp>(carrier.declaration);
    ASSERT_TRUE(reg);
    ASSERT_NE(carrier.view, nullptr);
    auto module = carrier.view->module;
    ASSERT_TRUE(module);
    auto &entry = module.getBody().front();
    ASSERT_GE(entry.getNumArguments(), 2u);
    EXPECT_EQ(reg->getOperand(0), entry.getArgument(0))
        << "StateCarrier clock must be its owner's clock argument";
    EXPECT_EQ(reg->getOperand(1), entry.getArgument(1))
        << "StateCarrier reset must be its owner's reset argument";
    if constexpr (HasClockResetFacts<decltype(carrier)>) {
      EXPECT_EQ(carrier.clock, entry.getArgument(0));
      EXPECT_EQ(carrier.reset, entry.getArgument(1));
    }
  }
  for (const auto &instance : ready->instances()) {
    auto controls = instance.module->getAttrOfType<mlir::DictionaryAttr>(
        "ac.control_ports");
    ASSERT_TRUE(controls);
    EXPECT_EQ(controls.getAs<mlir::IntegerAttr>("clock").getInt(), 0);
    EXPECT_EQ(controls.getAs<mlir::IntegerAttr>("reset").getInt(), 1);
  }
}

TEST_F(FinalRegControlClosureTest, RejectsSwappedRegisterClockAndReset) {
  expectMutationRejected([](FinalProgram &program) {
    auto reg = firstOwnedReg(program);
    if (!reg)
      return false;
    auto clock = reg->getOperand(0);
    reg->setOperand(0, reg->getOperand(1));
    reg->setOperand(1, clock);
    return true;
  });
}

TEST_F(FinalRegControlClosureTest,
       RejectsRegisterClockOrResetRedirectedToConstant) {
  for (unsigned operand : {0u, 1u})
    expectMutationRejected([operand](FinalProgram &program) {
      auto reg = firstOwnedReg(program);
      if (!reg)
        return false;
      mlir::OpBuilder builder(reg);
      auto constant =
          mlir::arith::ConstantIntOp::create(builder, reg.getLoc(), 0, 1);
      reg->setOperand(operand, constant.getResult());
      return true;
    });
}

TEST_F(FinalRegControlClosureTest,
       RejectsRegisterControlRedirectedToAnotherModule) {
  expectMutationRejected([](FinalProgram &program) {
    auto reg = firstOwnedReg(program);
    if (!reg)
      return false;
    auto owner = mlir::cast<ac::ModuleOp>(reg->getParentOp());
    mlir::Value foreign;
    for (const auto &instance : program.instances()) {
      if (instance.module == owner)
        continue;
      auto module = instance.module;
      auto &entry = module.getBody().front();
      if (entry.getNumArguments() >= 2) {
        foreign = entry.getArgument(0);
        break;
      }
    }
    if (!foreign)
      return false;
    reg->setOperand(0, foreign);
    return true;
  });
}

TEST_F(FinalRegControlClosureTest, RejectsControlPortMetadataMutations) {
  for (unsigned badClock : {1u, 2u})
    expectMutationRejected([badClock](FinalProgram &program) {
      auto module = program.stateCarriers().front().view->module;
      mlir::Builder builder(module.getContext());
      auto controls = builder.getDictionaryAttr({
          builder.getNamedAttr("clock", builder.getI64IntegerAttr(badClock)),
          builder.getNamedAttr("reset", builder.getI64IntegerAttr(1)),
      });
      module->setAttr("ac.control_ports", controls);
      return true;
    });
  expectMutationRejected([](FinalProgram &program) {
    auto module = program.stateCarriers().front().view->module;
    mlir::Builder builder(module.getContext());
    module->setAttr(
        "ac.control_ports",
        builder.getDictionaryAttr({
            builder.getNamedAttr("clock", builder.getI64IntegerAttr(0)),
            builder.getNamedAttr("reset", builder.getI64IntegerAttr(0)),
        }));
    return true;
  });
  expectMutationRejected([](FinalProgram &program) {
    auto module = program.stateCarriers().front().view->module;
    mlir::Builder builder(module.getContext());
    auto controls = builder.getDictionaryAttr({
        builder.getNamedAttr("clock", builder.getI64IntegerAttr(0)),
        builder.getNamedAttr("reset", builder.getI64IntegerAttr(1)),
        builder.getNamedAttr("domain", builder.getStringAttr("foreign")),
    });
    module->setAttr("ac.control_ports", controls);
    return true;
  });
}

TEST_F(FinalRegControlClosureTest, RejectsWrongOwnerControlArgumentType) {
  for (unsigned index : {0u, 1u})
    expectMutationRejected([index](FinalProgram &program) {
      auto reg = firstOwnedReg(program);
      if (!reg)
        return false;
      auto module = mlir::cast<ac::ModuleOp>(reg->getParentOp());
      auto argument = module.getBody().front().getArgument(index);
      argument.setType(mlir::IntegerType::get(module.getContext(), 8));
      return true;
    });
}

TEST_F(FinalRegControlClosureTest,
       RejectsControlMutationAcrossRepeatedChildInstances) {
  auto ready = build();
  ASSERT_TRUE(mlir::succeeded(ready));
  const auto &root = ready->instances()[ready->rootInstanceOrdinal()];
  ASSERT_EQ(root.childOrdinals.size(), 2u);
  const auto &left = ready->instances()[root.childOrdinals[0]];
  const auto &right = ready->instances()[root.childOrdinals[1]];
  ASSERT_EQ(left.definition, right.definition);
  auto reg = firstOwnedReg(*ready);
  ASSERT_TRUE(reg);
  auto ownerModule = mlir::cast<ac::ModuleOp>(reg->getParentOp());
  reg->setOperand(1, ownerModule.getBody().front().getArgument(0));
  EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())));
  EXPECT_TRUE(mlir::failed(emitFinalCpp(*ready, emitError())));
  EXPECT_TRUE(mlir::failed(emitFinalVerilog(*ready, emitError())));
}

} // namespace
} // namespace acir::compiler
