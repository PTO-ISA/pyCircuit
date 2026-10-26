// Typed Module results remain ordinary values when canonical storage leaves
// are allocated and bound through Module-scope structural wiring.
#include "Compiler/FinalCppSourceParts.h"
#include "Compiler/FinalVerilogSourceParts.h"
#include "Compiler/SourceLink.h"
#include "Compiler/SourceUnit.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {
namespace ac = acir::ac;
namespace compiler = acir::compiler;
using namespace mlir;

class SourcePrimitiveValueResultsTest : public ::testing::Test {
protected:
  MLIRContext context;
  Builder builder{&context};
  llvm::SmallString<256> directory;
  std::string diagnostics;
  std::unique_ptr<ScopedDiagnosticHandler> handler;

  void SetUp() override {
    context.loadDialect<ac::ACIRDialect>();
    ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("primitive-value-results",
                                                      directory));
    handler =
        std::make_unique<ScopedDiagnosticHandler>(&context, [&](Diagnostic &d) {
          llvm::raw_string_ostream out(diagnostics);
          d.print(out);
          diagnostics += '\n';
          return success();
        });
  }

  void TearDown() override { llvm::sys::fs::remove_directories(directory); }

  std::string path(StringRef name) {
    llvm::SmallString<256> result(directory);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }

  auto error() {
    return [&] { return emitError(builder.getUnknownLoc()); };
  }

  FailureOr<compiler::SourceUnitArtifacts>
  compile(StringRef source, StringRef name = "leafs.py") {
    std::error_code ec;
    {
      llvm::raw_fd_ostream output(path(name), ec);
      if (ec)
        return failure();
      output << source;
    }
    static constexpr llvm::StringLiteral capture = R"py(
import sys
from pathlib import Path
repo, source, root, output = map(Path, sys.argv[1:])
sys.path.insert(0, str(repo / "python"))
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
output.write_text(_emit_source_transport(_capture_source_file(source, source_root=root)), encoding="utf-8")
)py";
    std::vector<std::string> owned{
        PYCIRCUIT_TEST_PYTHON,    "-c",       capture.str(),
        PYCIRCUIT_TEST_REPO_ROOT, path(name), directory.str().str(),
        path("capture.mlir")};
    SmallVector<StringRef> arguments;
    for (auto &argument : owned)
      arguments.push_back(argument);
    auto log = path("capture.log");
    std::array<std::optional<StringRef>, 3> redirects{std::nullopt, log, log};
    int status =
        llvm::sys::ExecuteAndWait(owned[0], arguments, std::nullopt, redirects);
    auto output = llvm::MemoryBuffer::getFile(log);
    EXPECT_EQ(status, 0) << (output ? output.get()->getBuffer().str() : log);
    if (status)
      return failure();
    auto captured = parseSourceFile<ModuleOp>(path("capture.mlir"), &context);
    if (!captured)
      return failure();
    auto registry =
        compiler::SourceHeaderRegistry::create(&context, {}, error());
    if (failed(registry))
      return failure();
    auto owner = builder.getDictionaryAttr(
        {builder.getNamedAttr("package", builder.getStringAttr("")),
         builder.getNamedAttr("path", builder.getStringAttr(name))});
    return compiler::compilePythonSourceUnit(*captured, owner, *registry,
                                             error());
  }

  ac::ModuleImportOp declaration(ModuleOp file, StringRef symbol) {
    return dyn_cast_or_null<ac::ModuleImportOp>(
        SymbolTable::lookupSymbolIn(file, symbol));
  }

  ac::ModuleOp definition(ModuleOp file, StringRef symbol) {
    return dyn_cast_or_null<ac::ModuleOp>(
        SymbolTable::lookupSymbolIn(file, symbol));
  }

  void dualEmit(ModuleOp file) {
    ac::HardwareAnalysis analysis(file);
    ASSERT_TRUE(succeeded(analysis.verify())) << diagnostics;
    auto cpp = compiler::emitCppSourceParts(file, analysis);
    ASSERT_TRUE(succeeded(cpp)) << diagnostics;
    EXPECT_FALSE(cpp->sourceGroups.empty());
    EXPECT_FALSE(cpp->systemHeader.empty());
    auto rtl = compiler::emitVerilogSourceParts(file, analysis);
    ASSERT_TRUE(succeeded(rtl)) << diagnostics;
    EXPECT_FALSE(rtl->sourceGroups.empty());
    EXPECT_FALSE(rtl->core.empty());
  }

  void expectResultKinds(ac::ModuleImportOp declaration,
                         ArrayRef<StringRef> expectedKinds,
                         StringRef returnForm) {
    ASSERT_TRUE(declaration);
    auto form = declaration->getAttrOfType<StringAttr>("ac.return_form");
    ASSERT_TRUE(form);
    EXPECT_EQ(form.getValue(), returnForm);
    auto constraints =
        declaration->getAttrOfType<ArrayAttr>("ac.result_constraints");
    ASSERT_TRUE(constraints);
    ASSERT_EQ(constraints.size(), expectedKinds.size());
    for (auto [constraint, expected] : llvm::zip(constraints, expectedKinds)) {
      auto kind =
          cast<DictionaryAttr>(constraint).getAs<StringAttr>("source_kind");
      ASSERT_TRUE(kind);
      EXPECT_EQ(kind.getValue(), expected);
    }
  }

  void expectLeafWiring(ac::ModuleOp module, StringRef instanceName,
                        StringRef primitive,
                        ArrayRef<unsigned> moduleArguments) {
    ac::InstanceOp selected;
    for (auto instance : module.getBody().front().getOps<ac::InstanceOp>())
      if (instance.getInstanceName() == instanceName)
        selected = instance;
    ASSERT_TRUE(selected) << instanceName.str();
    auto callee = selected->getAttrOfType<FlatSymbolRefAttr>("callee");
    ASSERT_TRUE(callee);
    std::string expectedCallee = "pycircuit.__builtins__.";
    expectedCallee += primitive.str();
    EXPECT_EQ(callee.getValue(), StringRef(expectedCallee));
    ASSERT_EQ(selected.getInputs().size(), moduleArguments.size());

    for (auto [index, moduleArgument] : llvm::enumerate(moduleArguments)) {
      ASSERT_LT(moduleArgument, module.getBody().front().getNumArguments());
      EXPECT_EQ(selected.getInputs()[index],
                module.getBody().front().getArgument(moduleArgument));
    }
  }
};

static constexpr llvm::StringLiteral typedLeaves = R"py(import pycircuit as ac
from enum import Enum
from typing import Annotated
from pycircuit import dff, dffe

@ac.encoding(width=2)
class Mode(Enum):
    IDLE = 0
    RUN = 1

@ac.struct
class Packet:
    data: ac.u8
    tag: ac.u4

@ac.module
def BoolDFFE(clk: bool, rst: bool, en: bool, data: bool, init: bool) -> bool:
    state = dffe(T=bool)
    state(clk=clk, rst=rst, en=en, d=data, init=init)
    return state.q

@ac.module
def IntegerPairDFF(clk: bool, rst: bool,
                   data0: Annotated[int, range(1 << 4)],
                   init0: Annotated[int, range(1 << 4)],
                   data1: Annotated[int, range(1 << 4)],
                   init1: Annotated[int, range(1 << 4)]) -> tuple[Annotated[int, range(1 << 4)], Annotated[int, range(1 << 4)]]:
    first = dff(T=Annotated[int, range(1 << 4)])
    second = dff(T=Annotated[int, range(1 << 4)])
    first(clk=clk, rst=rst, d=data0, init=init0)
    second(clk=clk, rst=rst, d=data1, init=init1)
    return first.q, second.q

@ac.module
def EnumDFF(clk: bool, rst: bool, data: Mode, init: Mode) -> Mode:
    state = dff(T=Mode)
    state(clk=clk, rst=rst, d=data, init=init)
    return state.q

@ac.module
def WordDFFE(clk: bool, rst: bool, en: bool, data: ac.u8, init: ac.u8) -> ac.u8:
    state = dffe(T=ac.u8)
    state(clk=clk, rst=rst, en=en, d=data, init=init)
    return state.q

@ac.module
def PacketDFFE(clk: bool, rst: bool, en: bool, data: Packet, init: Packet) -> Packet:
    state = dffe(T=Packet)
    state(clk=clk, rst=rst, en=en, d=data, init=init)
    return state.q
)py";

TEST_F(SourcePrimitiveValueResultsTest,
       CanonicalLeavesPreserveTypedResultsWiringInstancesAndFinalEmission) {
  auto unit = compile(typedLeaves);
  ASSERT_TRUE(succeeded(unit)) << diagnostics;

  expectResultKinds(declaration(*unit->interface, "leafs.BoolDFFE"),
                    {"boolean"}, "single");
  expectResultKinds(declaration(*unit->interface, "leafs.IntegerPairDFF"),
                    {"integer", "integer"}, "tuple");
  expectResultKinds(declaration(*unit->interface, "leafs.EnumDFF"), {"nominal"},
                    "single");
  expectResultKinds(declaration(*unit->interface, "leafs.WordDFFE"),
                    {"fixed_bits"}, "single");
  expectResultKinds(declaration(*unit->interface, "leafs.PacketDFFE"),
                    {"nominal"}, "single");

  auto boolean = definition(*unit->body, "leafs.BoolDFFE");
  auto integers = definition(*unit->body, "leafs.IntegerPairDFF");
  auto enumeration = definition(*unit->body, "leafs.EnumDFF");
  auto word = definition(*unit->body, "leafs.WordDFFE");
  auto packet = definition(*unit->body, "leafs.PacketDFFE");
  ASSERT_TRUE(boolean && integers && enumeration && word && packet);
  expectLeafWiring(boolean, "state", "dffe", {0, 1, 2, 3, 4});
  expectLeafWiring(integers, "first", "dff", {0, 1, 2, 3});
  expectLeafWiring(integers, "second", "dff", {0, 1, 4, 5});
  expectLeafWiring(enumeration, "state", "dff", {0, 1, 2, 3});
  expectLeafWiring(word, "state", "dffe", {0, 1, 2, 3, 4});
  expectLeafWiring(packet, "state", "dffe", {0, 1, 2, 3, 4});

  SmallVector<ac::InstanceOp> integerLeaves;
  for (auto instance : integers.getBody().front().getOps<ac::InstanceOp>())
    integerLeaves.push_back(instance);
  ASSERT_EQ(integerLeaves.size(), 2u);
  EXPECT_NE(integerLeaves[0].getOperation(), integerLeaves[1].getOperation());
  EXPECT_NE(integerLeaves[0].getResult(0), integerLeaves[1].getResult(0));

  for (StringRef root :
       {"leafs.BoolDFFE", "leafs.IntegerPairDFF", "leafs.EnumDFF",
        "leafs.WordDFFE", "leafs.PacketDFFE"}) {
    SCOPED_TRACE(root.str());
    diagnostics.clear();
    auto final = compiler::linkSourceUnits({{*unit->body, *unit->interface}},
                                             root, error());
    ASSERT_TRUE(succeeded(final)) << diagnostics;
    dualEmit(**final);
  }
}

TEST_F(SourcePrimitiveValueResultsTest,
       CanonicalLeafStillRequiresExactlyOneCompleteBinding) {
  for (auto [name, body] : {std::pair{"unbound.py", R"py(import pycircuit as ac
from pycircuit import dff
@ac.module
def Top(clk: bool, rst: bool, data: ac.u8, init: ac.u8) -> ac.u8:
    state = dff(T=ac.u8)
    return state.q
)py"},
                            std::pair{"double.py", R"py(import pycircuit as ac
from pycircuit import dff
@ac.module
def Top(clk: bool, rst: bool, data: ac.u8, init: ac.u8) -> ac.u8:
    state = dff(T=ac.u8)
    state(clk=clk, rst=rst, d=data, init=init)
    state(clk=clk, rst=rst, d=data, init=init)
    return state.q
)py"}}) {
    SCOPED_TRACE(name);
    diagnostics.clear();
    auto unit = compile(body, name);
    EXPECT_TRUE(failed(unit));
    EXPECT_NE(diagnostics.find(StringRef(name) == "unbound.py" ? "unbound"
                                                               : "binding"),
              std::string::npos)
        << diagnostics;
  }
}

TEST_F(SourcePrimitiveValueResultsTest,
       OrdinaryUserModuleCannotUseAllocateThenBindLeafSyntax) {
  auto unit = compile(R"py(import pycircuit as ac
@ac.module
def UserLeaf(clk: bool, rst: bool, data: ac.u8, init: ac.u8) -> ac.u8:
    return data
@ac.module
def Top(clk: bool, rst: bool, data: ac.u8, init: ac.u8) -> ac.u8:
    state = UserLeaf()
    state(clk=clk, rst=rst, data=data, init=init)
    return state
)py",
                      "user_leaf.py");
  EXPECT_TRUE(failed(unit));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(SourcePrimitiveValueResultsTest,
       EqualWidthLogicalKindsRemainDistinctAtCanonicalLeafPorts) {
  for (auto [name, type, actual] :
       {std::tuple{"bool_to_integer.py", "Annotated[int, range(1 << 1)]",
                   "bool"},
        std::tuple{"integer_to_bool.py", "bool",
                   "Annotated[int, range(1 << 1)]"}}) {
    SCOPED_TRACE(name);
    diagnostics.clear();
    std::string source =
        "import pycircuit as ac\nfrom typing import Annotated\n"
        "from pycircuit import dff\n@ac.module\n"
        "def Top(clk: bool, rst: bool, data: " +
        std::string(actual) + ", init: " + std::string(actual) + ") -> " +
        std::string(type) + ":\n    state = dff(T=" + std::string(type) +
        ")\n    state(clk=clk, rst=rst, "
        "d=data, init=init)\n    return state.q\n";
    auto unit = compile(source, name);
    EXPECT_TRUE(failed(unit));
    EXPECT_NE(diagnostics.find("kind"), std::string::npos) << diagnostics;
  }
}

} // namespace
