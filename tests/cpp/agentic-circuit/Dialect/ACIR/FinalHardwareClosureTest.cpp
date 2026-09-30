#include "Compiler/FinalProgram.h"
#include "Dialect/ACIR/ACIRHardwareClosure.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/STLExtras.h"
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

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("final-hardware-closure", path));
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

class FinalHardwareClosureTest : public ::testing::Test {
protected:
  FinalHardwareClosureTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    writeFile(sourceRoot + "/probe.py", R"py(from typing import Annotated
from pycircuit import module, rule
Word = Annotated[int, range(256)]
@module
def Probe(source: Word, sink: Word):
    @rule
    def transfer():
        nonlocal sink
        sink = source
        return
    transfer()
)py");
    writeFile(sourceRoot + "/bridge.py", R"py(from typing import Annotated
from pycircuit import module
from .probe import Probe
Word = Annotated[int, range(256)]
@module
def Bridge(source: Word, sink: Word):
    leaf = Probe(source, sink)
)py");
    writeFile(sourceRoot + "/root.py", R"py(from typing import Annotated
from pycircuit import system
from .bridge import Bridge
Word = Annotated[int, range(256)]
@system
def Root():
    shared: Word = 7
    left: Word = 0
    right: Word = 0
    first = Bridge(shared, left)
    second = Bridge(shared, right)
)py");
    ASSERT_TRUE(compile("probe.py", {}, probeBody, probeHeader));
    ASSERT_TRUE(
        compile("bridge.py", {probeHeaderPath}, bridgeBody, bridgeHeader));
    ASSERT_TRUE(compile("root.py", {bridgeHeaderPath, probeHeaderPath},
                        rootBody, rootHeader));
  }

  bool compile(llvm::StringRef relative,
               std::initializer_list<std::string> headers,
               mlir::OwningOpRef<mlir::ModuleOp> &body,
               mlir::OwningOpRef<mlir::ModuleOp> &header) {
    std::string stem = relative.str();
    std::replace(stem.begin(), stem.end(), '.', '_');
    const std::string source = sourceRoot + "/" + relative.str();
    const std::string capture = temporary.child(stem + ".transport.mlir");
    const std::string bodyPath = temporary.child(stem + ".body.mlir");
    const std::string headerPath = temporary.child(stem + ".interface.mlir");
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
            {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT, source,
             sourceRoot, capture},
            temporary.child(stem + ".capture.log")) != 0)
      return false;
    std::vector<std::string> arguments = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                          "--capture",
                                          capture,
                                          "--package",
                                          "final_package",
                                          "--path",
                                          relative.str()};
    for (const std::string &dependency : headers) {
      arguments.push_back("--header");
      arguments.push_back(dependency);
    }
    arguments.insert(arguments.end(),
                     {"--body-out", bodyPath, "--interface-out", headerPath});
    const std::string log = temporary.child(stem + ".compile.log");
    if (run(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log) != 0) {
      ADD_FAILURE() << readFile(log);
      return false;
    }
    body = mlir::parseSourceFile<mlir::ModuleOp>(bodyPath, &context);
    header = mlir::parseSourceFile<mlir::ModuleOp>(headerPath, &context);
    if (relative == "probe.py")
      probeHeaderPath = headerPath;
    if (relative == "bridge.py")
      bridgeHeaderPath = headerPath;
    return body && header;
  }

  auto emitError() {
    return [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
  }

  std::string materializeText() {
    llvm::SmallVector<SourceLinkUnit> units = {
        {*rootBody, *rootHeader},
        {*bridgeBody, *bridgeHeader},
        {*probeBody, *probeHeader},
    };
    auto analysis = buildFinalProgram(units, emitError());
    EXPECT_TRUE(mlir::succeeded(analysis));
    if (mlir::failed(analysis))
      return {};
    auto ready = materializeFinalProgram(std::move(*analysis), emitError());
    EXPECT_TRUE(mlir::succeeded(ready));
    if (mlir::failed(ready) || !ready->hardware())
      return {};
    std::string text;
    llvm::raw_string_ostream stream(text);
    ready->hardware().print(stream, mlir::OpPrintingFlags().enableDebugInfo());
    return text;
  }

  bool verify(mlir::ModuleOp package) {
    return mlir::succeeded(ac::verifyFinalHardware(package)) &&
           mlir::succeeded(mlir::verify(package));
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot, probeHeaderPath, bridgeHeaderPath;
  mlir::OwningOpRef<mlir::ModuleOp> probeBody, probeHeader, bridgeBody,
      bridgeHeader, rootBody, rootHeader;
};

TEST_F(FinalHardwareClosureTest,
       FreshContextRebuildsHierarchyRepeatedInstancesAndAliases) {
  std::string text = materializeText();
  ASSERT_FALSE(text.empty());
  mlir::MLIRContext fresh;
  fresh.loadDialect<ac::ACIRDialect, mlir::arith::ArithDialect>();
  auto package = mlir::parseSourceString<mlir::ModuleOp>(text, &fresh);
  ASSERT_TRUE(package);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*package)));
  ASSERT_TRUE(mlir::succeeded(ac::verifyFinalHardware(*package)));
  EXPECT_EQ((*package)->getAttrs().size(), 3u);
  EXPECT_EQ((*package)->getAttrOfType<mlir::StringAttr>("ac.stage").getValue(),
            "final");
  EXPECT_FALSE((*package)->hasAttr("ac.unit_kind"));
  EXPECT_EQ(llvm::range_size(package->getBody()->getOps<ac::SystemOp>()), 1u);
  auto system = *package->getBody()->getOps<ac::SystemOp>().begin();
  EXPECT_EQ(system->getAttrs().size(), 4u);
  EXPECT_EQ(system.getEntryAttr(),
            (*package)->getAttrOfType<mlir::DictionaryAttr>("ac.entry"));
  EXPECT_EQ(system.getDomain(), "default");
  auto units = llvm::to_vector(package->getBody()->getOps<mlir::ModuleOp>());
  ASSERT_EQ(units.size(), 3u);
  std::vector<std::string> paths;
  llvm::SmallVector<ac::ModuleOp> definitions;
  for (mlir::ModuleOp unit : units) {
    auto owner = unit->getAttrOfType<mlir::DictionaryAttr>("ac.source_owner");
    paths.push_back(owner.getAs<mlir::StringAttr>("path").str());
    auto nested = llvm::to_vector(unit.getOps<ac::ModuleOp>());
    ASSERT_EQ(nested.size(), 1u);
    definitions.push_back(nested.front());
    EXPECT_EQ(unit->getAttrs().size(), 3u);
    EXPECT_EQ(unit->getAttrOfType<mlir::StringAttr>("ac.stage").getValue(),
              "final");
    EXPECT_EQ(unit->getAttrOfType<mlir::StringAttr>("ac.unit_kind").getValue(),
              "implementation");
    EXPECT_EQ(llvm::range_size(unit.getOps<ac::ModuleImportOp>()), 0u);
    EXPECT_EQ(llvm::range_size(unit.getOps<ac::TypeAliasOp>()), 1u)
        << "each implementation source retains its own Word alias";
  }
  EXPECT_TRUE(std::is_sorted(paths.begin(), paths.end()));
  auto rows =
      (*package)->getAttrOfType<mlir::ArrayAttr>("ac.instance_bindings");
  ASSERT_TRUE(rows);
  EXPECT_EQ(rows.size(), 5u);
  auto entry = (*package)->getAttrOfType<mlir::DictionaryAttr>("ac.entry");
  auto rootSymbol = entry.getAs<mlir::FlatSymbolRefAttr>("definition");
  ac::ModuleOp root;
  for (ac::ModuleOp definition : definitions) {
    auto targets =
        definition->getAttrOfType<mlir::ArrayAttr>("ac.commit_targets");
    auto yield =
        mlir::cast<ac::YieldOp>(definition.getBody().front().getTerminator());
    ASSERT_TRUE(targets);
    EXPECT_EQ(yield.getValues().size(), 2 * targets.size());
    if (definition.getSymName() == rootSymbol.getValue())
      root = definition;
  }
  ASSERT_TRUE(root);
  auto children =
      llvm::to_vector(root.getBody().front().getOps<ac::InstanceOp>());
  ASSERT_EQ(children.size(), 2u);
  EXPECT_EQ(children[0].getCalleeAttr(), children[1].getCalleeAttr());
  auto left = children[0]->getAttrOfType<mlir::ArrayAttr>("ac.port_bindings");
  auto right = children[1]->getAttrOfType<mlir::ArrayAttr>("ac.port_bindings");
  ASSERT_EQ(left.size(), 2u);
  ASSERT_EQ(right.size(), 2u);
  EXPECT_EQ(mlir::cast<mlir::DictionaryAttr>(left[0]).get("target"),
            mlir::cast<mlir::DictionaryAttr>(right[0]).get("target"));
  EXPECT_NE(mlir::cast<mlir::DictionaryAttr>(left[1]).get("target"),
            mlir::cast<mlir::DictionaryAttr>(right[1]).get("target"));
}

TEST_F(FinalHardwareClosureTest,
       RejectsEnvelopeTreeRowsPortsAndCommitTampering) {
  std::string text = materializeText();
  ASSERT_FALSE(text.empty());
  mlir::MLIRContext fresh;
  fresh.loadDialect<ac::ACIRDialect, mlir::arith::ArithDialect>();
  auto original = mlir::parseSourceString<mlir::ModuleOp>(text, &fresh);
  ASSERT_TRUE(original);
  ASSERT_TRUE(verify(*original));
  auto expectRejected = [&](llvm::StringRef label, auto mutate) {
    SCOPED_TRACE(label.str());
    auto copy = mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>((*original)->clone()));
    mutate(*copy);
    EXPECT_FALSE(verify(*copy));
  };
  expectRejected("missing system", [&](mlir::ModuleOp package) {
    auto system = *package.getBody()->getOps<ac::SystemOp>().begin();
    system.erase();
  });
  expectRejected("duplicate system", [&](mlir::ModuleOp package) {
    auto system = *package.getBody()->getOps<ac::SystemOp>().begin();
    package.getBody()->push_back(system->clone());
  });
  expectRejected("stage spoof", [&](mlir::ModuleOp package) {
    package->setAttr("ac.stage", mlir::StringAttr::get(&fresh, "source"));
  });
  expectRejected("wrong entry", [&](mlir::ModuleOp package) {
    ac::InstanceOp child;
    package.walk([&](ac::InstanceOp op) {
      if (!child)
        child = op;
    });
    package->setAttr("ac.entry", child.getCalleeAttr());
  });
  for (llvm::StringRef change :
       {"missing row", "duplicate row", "wrong owner", "wrong port mapping"}) {
    expectRejected(change, [&](mlir::ModuleOp package) {
      mlir::Builder builder(&fresh);
      auto rows =
          package->getAttrOfType<mlir::ArrayAttr>("ac.instance_bindings");
      llvm::SmallVector<mlir::Attribute> values(rows.begin(), rows.end());
      if (change == "missing row")
        values.pop_back();
      else if (change == "duplicate row")
        values.push_back(values.front());
      else if (change == "wrong owner") {
        auto row = mlir::cast<mlir::DictionaryAttr>(values[0]);
        auto other = mlir::cast<mlir::DictionaryAttr>(values[1]);
        values[0] = replaceField(builder, row, "owner", other.get("owner"));
      } else {
        for (mlir::Attribute &raw : values) {
          auto row = mlir::cast<mlir::DictionaryAttr>(raw);
          auto ports = row.getAs<mlir::ArrayAttr>("ports");
          if (ports.size() < 2)
            continue;
          auto first = mlir::cast<mlir::DictionaryAttr>(ports[0]);
          auto second = mlir::cast<mlir::DictionaryAttr>(ports[1]);
          llvm::SmallVector<mlir::Attribute> changed(ports.begin(),
                                                     ports.end());
          changed[0] =
              replaceField(builder, first, "state", second.get("state"));
          raw = replaceField(builder, row, "ports",
                             builder.getArrayAttr(changed));
          break;
        }
      }
      package->setAttr("ac.instance_bindings", builder.getArrayAttr(values));
    });
  }
  expectRejected("callee", [&](mlir::ModuleOp package) {
    ac::InstanceOp child;
    package.walk([&](ac::InstanceOp op) {
      if (!child)
        child = op;
    });
    child->setAttr("callee", package->getAttr("ac.entry"));
  });
  expectRejected("child operand", [&](mlir::ModuleOp package) {
    ac::InstanceOp child;
    package.walk([&](ac::InstanceOp op) {
      if (!child && !op.getInputs().empty() && !op.getTargets().empty())
        child = op;
    });
    child.getTargetsMutable()[0].set(child.getInputs()[0]);
  });
  for (llvm::StringRef change :
       {"missing target", "reordered targets", "redirected data",
        "redirected enable", "missing enable"}) {
    expectRejected(change, [&](mlir::ModuleOp package) {
      ac::ModuleOp root;
      auto entry = package->getAttrOfType<mlir::DictionaryAttr>("ac.entry");
      auto symbol = entry.getAs<mlir::FlatSymbolRefAttr>("definition");
      package.walk([&](ac::ModuleOp module) {
        if (module.getSymName() == symbol.getValue())
          root = module;
      });
      auto targets = root->getAttrOfType<mlir::ArrayAttr>("ac.commit_targets");
      auto yield =
          mlir::cast<ac::YieldOp>(root.getBody().front().getTerminator());
      mlir::Builder builder(&fresh);
      if (change == "missing target") {
        llvm::SmallVector<mlir::Attribute> values(targets.begin(),
                                                  targets.end());
        values.pop_back();
        root->setAttr("ac.commit_targets", builder.getArrayAttr(values));
      } else if (change == "reordered targets") {
        llvm::SmallVector<mlir::Attribute> values(targets.begin(),
                                                  targets.end());
        std::swap(values[0], values[1]);
        root->setAttr("ac.commit_targets", builder.getArrayAttr(values));
      } else if (change == "redirected data")
        yield->setOperand(0, yield->getOperand(2));
      else if (change == "redirected enable")
        yield->setOperand(1, yield->getOperand(3));
      else
        yield->eraseOperand(yield->getNumOperands() - 1);
    });
  }
  expectRejected("orphan unit", [&](mlir::ModuleOp package) {
    auto unit = *package.getBody()->getOps<mlir::ModuleOp>().begin();
    package.getBody()->push_back(unit->clone());
  });
}

} // namespace
} // namespace acir::compiler
