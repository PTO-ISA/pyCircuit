#include "Compiler/SourceLink.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
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
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {

struct LinkTemporaryDirectory {
  llvm::SmallString<256> path;
  LinkTemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-source-link", path));
  }
  ~LinkTemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
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

int run(llvm::StringRef program, const std::vector<std::string> &owned,
        llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : owned)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
}

int emitCapture(llvm::StringRef source, llvm::StringRef root,
                llvm::StringRef output, llvm::StringRef log) {
  static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
root = Path(sys.argv[1])
sys.path[:0] = [str(root / "python/semantic-core/src"),
                str(root / "python"), str(root)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
source = Path(sys.argv[2])
capture = _capture_source_file(source, source_root=Path(sys.argv[3]))
Path(sys.argv[4]).write_text(_emit_source_transport(capture), encoding="utf-8")
)py";
  return run(ACIR_TEST_PYTHON,
             {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
              source.str(), root.str(), output.str()},
             log);
}

int compileUnit(llvm::StringRef capture, llvm::StringRef path,
                llvm::StringRef body, llvm::StringRef header,
                llvm::StringRef log,
                llvm::ArrayRef<std::string> dependencies = {}) {
  std::vector<std::string> arguments = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                        "--capture",
                                        capture.str(),
                                        "--package",
                                        "demo",
                                        "--path",
                                        path.str()};
  for (const std::string &dependency : dependencies) {
    arguments.push_back("--header");
    arguments.push_back(dependency);
  }
  arguments.insert(arguments.end(),
                   {"--body-out", body.str(), "--interface-out", header.str()});
  return run(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log);
}

mlir::OwningOpRef<mlir::ModuleOp> parse(llvm::StringRef path,
                                        mlir::MLIRContext &context) {
  return mlir::parseSourceFile<mlir::ModuleOp>(path, &context);
}

template <typename OpTy>
OpTy findSymbol(mlir::ModuleOp module, llvm::StringRef name) {
  return mlir::dyn_cast_or_null<OpTy>(
      mlir::SymbolTable::lookupSymbolIn(module, name));
}

TEST(SourceResourceBinding, ParentAllocationAndExportsPreserveIdentity) {
  LinkTemporaryDirectory temporary;
  std::string sourceRoot = temporary.child("source");
  ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
  auto providerSource = sourceRoot + "/provider.py";
  auto topSource = sourceRoot + "/top.py";
  writeFile(providerSource, R"py(import pycircuit as ac
@ac.module
def Identity(src: ac.u8) -> ac.u8:
    return src
@ac.module
def Producer(src: ac.u8) -> ac.u8:
    @ac.rule
    def advance(value: ac.u8) -> ac.u8:
        return value + value + 1
    result = advance(src)
    forwarded = result
    return forwarded
)py");
  writeFile(topSource, R"py(import pycircuit as ac
from .provider import Identity, Producer
@ac.module
def Top() -> ac.u8:
    state: ac.u8 = 0
    destination: ac.u8 = 7
    destination = Producer(state)
    explicit = ac.queue[ac.u8](depth=5)
    explicit = Producer(state)
    default = Producer(explicit)
    alias = Identity(state)
    return alias
@ac.module
def Conflict() -> ac.u8:
    first: ac.u8 = 0
    second: ac.u8 = 1
    first = Identity(second)
    return first
)py");
  auto log = temporary.child("compile.log");
  auto providerCapture = temporary.child("provider.transport.mlir");
  auto topCapture = temporary.child("top.transport.mlir");
  auto providerBody = temporary.child("provider.body.mlir");
  auto providerHeader = temporary.child("provider.header.mlir");
  auto topBody = temporary.child("top.body.mlir");
  auto topHeader = temporary.child("top.header.mlir");
  ASSERT_EQ(emitCapture(providerSource, sourceRoot, providerCapture, log), 0)
      << readFile(log);
  ASSERT_EQ(compileUnit(providerCapture, "provider.py", providerBody,
                        providerHeader, log),
            0)
      << readFile(log);
  ASSERT_EQ(emitCapture(topSource, sourceRoot, topCapture, log), 0)
      << readFile(log);
  ASSERT_EQ(compileUnit(topCapture, "top.py", topBody, topHeader, log,
                        {providerHeader}),
            0)
      << readFile(log);
  mlir::DialectRegistry dialects;
  dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect,
                  mlir::func::FuncDialect>();
  mlir::MLIRContext context(dialects);
  context.loadAllAvailableDialects();
  auto pb = parse(providerBody, context), ph = parse(providerHeader, context);
  auto tb = parse(topBody, context), th = parse(topHeader, context);
  ASSERT_TRUE(pb && ph && tb && th);
  unsigned prematureAllocations = 0, deferredEndpoints = 0;
  pb->walk([&](ac::ResourceAllocOp) { ++prematureAllocations; });
  pb->walk([&](ac::ResourceConnectionOp) { ++deferredEndpoints; });
  EXPECT_EQ(prematureAllocations, 0u);
  EXPECT_GT(deferredEndpoints, 0u);
  auto linked = linkSourceUnits(
      {{*pb, *ph}, {*tb, *th}}, "demo.top.Top", [&] { return tb->emitError(); },
      SourceLinkTarget::Linked);
  ASSERT_TRUE(mlir::succeeded(linked));
  std::string linkedText;
  llvm::raw_string_ostream stream(linkedText);
  (*linked)->print(stream);
  auto reread = mlir::parseSourceString<mlir::ModuleOp>(linkedText, &context);
  ASSERT_TRUE(reread);
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*reread)));
  unsigned states = 0, queues = 0;
  llvm::SmallVector<int64_t> depths;
  (*linked)->walk([&](ac::ResourceAllocOp allocation) {
    if (mlir::isa<ac::StateType>(allocation.getResource().getType()))
      ++states;
    else {
      ++queues;
      depths.push_back(
          allocation->getAttrOfType<mlir::IntegerAttr>("depth").getInt());
      EXPECT_EQ(allocation->getParentOfType<ac::ModuleOp>().getSymName(),
                "demo.top.Top");
    }
  });
  EXPECT_EQ(states, 2u);
  EXPECT_EQ(queues, 2u);
  unsigned pops = 0, pushes = 0;
  (*linked)->walk([&](ac::TransactionMethodCallOp call) {
    if (call.getCalleeAttr().getRootReference().getValue() == "pop")
      ++pops;
    if (call.getCalleeAttr().getRootReference().getValue() == "push") {
      ++pushes;
      EXPECT_TRUE(mlir::isa<ac::QueueType>(call.getReceiver().getType()));
    }
  });
  EXPECT_EQ(pops, 1u);
  EXPECT_EQ(pushes, 2u);
  llvm::sort(depths);
  EXPECT_EQ(depths, (llvm::SmallVector<int64_t>{2, 5}));
  auto root = findSymbol<ac::ModuleOp>(**linked, "demo.top.Top");
  auto output = mlir::cast<ac::YieldOp>(root.getBody().front().getTerminator());
  ASSERT_EQ(output.getNumOperands(), 1u);
  EXPECT_TRUE(mlir::isa<ac::StateType>(output.getOperand(0).getType()));
  auto exported = output.getOperand(0).getDefiningOp<ac::InstanceOp>();
  ASSERT_TRUE(exported);
  auto state = exported.getOperand(0).getDefiningOp<ac::ResourceAllocOp>();
  ASSERT_TRUE(state);
  EXPECT_EQ(state.getInstanceName(), "state");
  // Queue execution is intentionally not exercised by this structural case.
  mlir::ScopedDiagnosticHandler diagnostics(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  auto conflicting = linkSourceUnits(
      {{*pb, *ph}, {*tb, *th}}, "demo.top.Conflict",
      [&] { return tb->emitError(); }, SourceLinkTarget::Linked);
  EXPECT_TRUE(mlir::failed(conflicting));
}

TEST(SourceResourceBinding, GenericMethodsCloseScalarTupleAndIgnoredUses) {
  LinkTemporaryDirectory temporary;
  std::string sourceRoot = temporary.child("source");
  ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
  auto source = sourceRoot + "/methods.py";
  writeFile(source, R"py(import pycircuit as ac
@ac.module
def Provider():
    count: ac.u8 = 3
    @ac.method
    def read() -> ac.u8:
        return count
    @ac.method
    def echo(value: ac.u16) -> ac.u16:
        return value
    @ac.method
    def pair() -> tuple[ac.u8, ac.u8]:
        return count, count
@ac.module
def Client(peer: ac.module):
    @ac.rule
    def use() -> None:
        alias = peer.read()
        widened: ac.u16 = alias
        first, second = peer.pair()
        peer.read()
        peer.pair()
        narrow: ac.u8 = 1
        peer.echo(narrow + narrow)
        ac.log("info", "values", widened, first, second)
    use()
@ac.system
def Top():
    provider: Provider
    client: Client
    client(provider)
)py");
  auto log = temporary.child("compile.log");
  auto capture = temporary.child("methods.transport.mlir");
  auto body = temporary.child("methods.body.mlir");
  auto header = temporary.child("methods.header.mlir");
  ASSERT_EQ(emitCapture(source, sourceRoot, capture, log), 0) << readFile(log);
  ASSERT_EQ(compileUnit(capture, "methods.py", body, header, log), 0)
      << readFile(log);
  mlir::DialectRegistry dialects;
  dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect,
                  mlir::func::FuncDialect>();
  mlir::MLIRContext context(dialects);
  context.loadAllAvailableDialects();
  auto parsedBody = parse(body, context), parsedHeader = parse(header, context);
  ASSERT_TRUE(parsedBody && parsedHeader);
  auto linked = linkSourceUnits(
      {{*parsedBody, *parsedHeader}}, "demo.methods.Top",
      [&] { return parsedBody->emitError(); }, SourceLinkTarget::Linked);
  ASSERT_TRUE(mlir::succeeded(linked));
  EXPECT_TRUE(mlir::succeeded(mlir::verify(**linked)));
  unsigned reads = 0, pairs = 0, echoes = 0, pending = 0, narrowSums = 0;
  ac::HardwareAnalysis analysis(**linked);
  (*linked)->walk([&](mlir::Operation *op) {
    pending += mlir::isa<ac::TransactionMethodPendingOp, ac::PendingExprOp>(op);
    if (auto sum = mlir::dyn_cast<ac::BitsBinaryOp>(op)) {
      auto width = analysis.getPackedWidth(sum->getResult(0).getType(), {}, op);
      if (mlir::succeeded(width) && *width == 8)
        ++narrowSums;
    }
    if (auto call = mlir::dyn_cast<ac::TransactionMethodCallOp>(op)) {
      if (!mlir::isa<ac::ModuleRefType>(call.getReceiver().getType()))
        return;
      auto name = call.getCalleeAttr().getRootReference().getValue();
      if (name == "read") {
        ++reads;
        EXPECT_EQ(call.getNumResults(), 1u);
      }
      if (name == "echo") {
        ++echoes;
        ASSERT_EQ(call.getArguments().size(), 1u);
        auto width =
            analysis.getPackedWidth(call.getArguments()[0].getType(), {}, op);
        ASSERT_TRUE(mlir::succeeded(width));
        EXPECT_EQ(*width, 16u);
      }
      if (name == "pair") {
        ++pairs;
        EXPECT_EQ(call.getNumResults(), 2u);
      }
    }
  });
  EXPECT_EQ(reads, 2u);
  EXPECT_EQ(pairs, 2u);
  EXPECT_EQ(echoes, 1u);
  EXPECT_GE(narrowSums, 1u);
  EXPECT_EQ(pending, 0u);
}

} // namespace
} // namespace acir::compiler
