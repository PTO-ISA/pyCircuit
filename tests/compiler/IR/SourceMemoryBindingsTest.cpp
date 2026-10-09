#include "Compiler/PythonImportRecords.h"
#include "Compiler/SourceMemoryBindings.h"
#include "Compiler/SourceRuleWrites.h"
#include "Compiler/SourceUnit.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Verifier.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {
using namespace mlir;
namespace compiler = acir::compiler;
namespace source = acir::compiler::detail;
namespace ac = acir::ac;

constexpr llvm::StringLiteral memorySource = R"py(import pycircuit as ac
@ac.struct
class Read:
    value: ac.u13
@ac.module
def Memory(ren: ac.u1, raddr: ac.u2, wvalid: ac.u1,
           waddr: ac.u2, wdata: ac.u13, wstrb: ac.u2) -> Read:
    value = ac.sync_mem[ac.u13](ren, raddr, wvalid, waddr, wdata, wstrb, depth=3)
    return Read(value=value)
)py";
constexpr llvm::StringLiteral consumerSource = R"py(import pycircuit as ac
from memory_cache.provider import Payload
@ac.module
def Memory(ren: ac.u1, raddr: ac.u2, wvalid: ac.u1,
           waddr: ac.u2, wdata: Payload, wstrb: ac.u2) -> Payload:
    value = ac.sync_mem[Payload](ren, raddr, wvalid, waddr, wdata, wstrb, depth=3)
    return value
)py";

class SourceMemoryBindingsTest : public ::testing::Test {
protected:
  MLIRContext context;
  llvm::SmallString<256> directory;
  std::string diagnostics;
  std::vector<Location> noteLocations;
  std::unique_ptr<ScopedDiagnosticHandler> handler;

  void SetUp() override {
    context.loadDialect<acir::ac::ACIRDialect>();
    ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("source-memory-bindings", directory));
    handler = std::make_unique<ScopedDiagnosticHandler>(&context, [&](Diagnostic &d) {
      llvm::raw_string_ostream out(diagnostics);
      d.print(out);
      for (const auto &note : d.getNotes())
        noteLocations.push_back(note.getLocation());
      return success();
    });
  }
  void TearDown() override {
    if (!directory.empty())
      llvm::sys::fs::remove_directories(directory);
  }
  std::string child(StringRef name) const {
    llvm::SmallString<256> path(directory);
    llvm::sys::path::append(path, name);
    return path.str().str();
  }
  DictionaryAttr owner(StringRef path = "memory.py", StringRef package = "memory_cache") {
    Builder b(&context);
    return b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr(package)),
                                b.getNamedAttr("path", b.getStringAttr(path))});
  }
  auto error() { return [&] { return emitError(UnknownLoc::get(&context)); }; }

  // Reuse the production capture APIs and the established ExecuteAndWait idiom
  // from EnumSourceUnitAuthorityTest. No handwritten AST or extra test runner.
  OwningOpRef<ModuleOp> capture(StringRef filename, StringRef text) {
    const auto path = child(filename), transport = path + ".capture.mlir";
    std::error_code ec;
    {
      llvm::raw_fd_ostream out(path, ec);
      EXPECT_FALSE(ec);
      if (ec)
        return {};
      out << text;
    }
    static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
repo, source, root, output = map(Path, sys.argv[1:])
sys.path.insert(0, str(repo / "python"))
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
output.write_text(_emit_source_transport(_capture_source_file(source, source_root=root)), encoding="utf-8")
)py";
    std::vector<std::string> owned{PYCIRCUIT_TEST_PYTHON, "-c", script.str(),
        PYCIRCUIT_TEST_REPO_ROOT, path, directory.str().str(), transport};
    SmallVector<StringRef> args;
    for (const auto &value : owned)
      args.push_back(value);
    const auto log = path + ".capture.log";
    const std::array<std::optional<StringRef>, 3> redirects{std::nullopt, log, log};
    int status = llvm::sys::ExecuteAndWait(PYCIRCUIT_TEST_PYTHON, args, std::nullopt, redirects);
    auto output = llvm::MemoryBuffer::getFile(log);
    EXPECT_EQ(status, 0) << (output ? output.get()->getBuffer().str() : log);
    if (status)
      return {};
    return parseSourceFile<ModuleOp>(transport, &context);
  }

  FailureOr<compiler::SourceUnitArtifacts> provider(unsigned width) {
    auto captured = capture("provider.py", "import pycircuit as ac\n@ac.struct\n"
        "class Payload:\n    value: ac.bits[" + std::to_string(width) + "]\n");
    if (!captured)
      return failure();
    auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
    if (failed(headers))
      return failure();
    return compiler::compilePythonSourceUnit(*captured, owner("provider.py"), *headers, error());
  }
};

TEST_F(SourceMemoryBindingsTest, PipelineRequiresTheSamePreparedPlan) {
  for (unsigned mode = 0; mode < 3; ++mode) {
    auto captured = capture("memory.py", memorySource);
    ASSERT_TRUE(captured);
    auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
    ASSERT_TRUE(succeeded(headers));
    diagnostics.clear();
    PassManager pipeline(&context);
    pipeline.addPass(source::createAnalyzeRuleWritesPass());
    if (mode != 1) {
      pipeline.addPass(source::createInferSourceBindingsPass(owner(), *headers));
      if (mode == 0)
        pipeline.addPass(source::createInferSourceBindingsPass(owner(), *headers));
    }
    pipeline.addPass(source::createLowerPythonSourcePass(
        owner("memory.py", mode == 2 ? "different_owner" : "memory_cache"), *headers));
    EXPECT_EQ(succeeded(pipeline.run(*captured)), mode == 0) << diagnostics;
    if (mode != 0)
      EXPECT_NE(diagnostics.find("binding"), std::string::npos) << diagnostics;
  }
}

TEST_F(SourceMemoryBindingsTest, SourceOwnerPathMustMatchCapture) {
  auto captured = capture("memory.py", memorySource);
  ASSERT_TRUE(captured);
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  source::SourceRuleWritesAnalysis writes(captured->getOperation());
  source::SourceMemoryBindingsAnalysis analysis(captured->getOperation());
  EXPECT_TRUE(failed(analysis.initialize(owner("other.py"), *headers, writes)));
  EXPECT_NE(diagnostics.find("captured source path"), std::string::npos) << diagnostics;
}

TEST_F(SourceMemoryBindingsTest, ChangedCaptureAndRepeatedConsumptionReject) {
  for (bool changeCapture : {false, true}) {
    auto captured = capture("memory.py", memorySource);
    ASSERT_TRUE(captured);
    auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
    ASSERT_TRUE(succeeded(headers));
    source::SourceRuleWritesAnalysis writes(captured->getOperation());
    source::SourceMemoryBindingsAnalysis analysis(captured->getOperation());
    ASSERT_TRUE(succeeded(analysis.initialize(owner(), *headers, writes))) << diagnostics;
    ASSERT_EQ(analysis.memoryCalls().size(), 1u);
    EXPECT_EQ(analysis.memoryCalls()[0].payloadWidth, 13u);
    EXPECT_EQ(analysis.memoryCalls()[0].addressWidth, 2u);
    EXPECT_TRUE(captured->getBody()->empty());
    if (changeCapture) {
      Builder b(&context);
      (*captured)->setAttr("ac.python_capture", b.getArrayAttr({}));
      EXPECT_TRUE(failed(analysis.initialize(owner(), *headers, writes)));
      EXPECT_TRUE(failed(analysis.lower(owner(), *headers)));
    } else {
      auto lowered = analysis.lower(owner(), *headers);
      ASSERT_TRUE(succeeded(lowered)) << diagnostics;
      EXPECT_TRUE(failed(analysis.lower(owner(), *headers)));
    }
  }
}

TEST_F(SourceMemoryBindingsTest, HeaderStorageOutlivesOriginalRegistryAndProvider) {
  auto captured = capture("memory.py", consumerSource);
  ASSERT_TRUE(captured);
  source::SourceRuleWritesAnalysis writes(captured->getOperation());
  source::SourceMemoryBindingsAnalysis analysis(captured->getOperation());
  std::string serialized;
  {
    auto original = provider(13);
    ASSERT_TRUE(succeeded(original)) << diagnostics;
    auto headers = compiler::SourceHeaderRegistry::create(&context, {*original->interface}, error());
    ASSERT_TRUE(succeeded(headers)) << diagnostics;
    ASSERT_TRUE(succeeded(analysis.initialize(owner(), *headers, writes))) << diagnostics;
    llvm::raw_string_ostream out(serialized);
    original->interface->print(out);
  }
  // Original operations and registry are destroyed; only the analysis's owned
  // declarations remain. An equivalent fresh registry is accepted by content.
  auto reloaded = parseSourceString<ModuleOp>(serialized, &context);
  ASSERT_TRUE(reloaded);
  (*reloaded)->setLoc(FileLineColLoc::get(&context, "relocated-interface.ac", 9, 3));
  auto replacement = compiler::SourceHeaderRegistry::create(&context, {*reloaded}, error());
  ASSERT_TRUE(succeeded(replacement)) << diagnostics;
  ASSERT_TRUE(succeeded(analysis.initialize(owner(), *replacement, writes))) << diagnostics;
  ASSERT_EQ(analysis.memoryCalls().size(), 1u);
  EXPECT_EQ(analysis.memoryCalls()[0].payloadWidth, 13u);
  auto lowered = analysis.lower(owner(), *replacement);
  EXPECT_TRUE(succeeded(lowered)) << diagnostics;
}

TEST_F(SourceMemoryBindingsTest, ChangedValidHeaderContentRejectsAtBothBoundaries) {
  auto narrow = provider(8), original = provider(13);
  ASSERT_TRUE(succeeded(narrow) && succeeded(original)) << diagnostics;
  auto first = compiler::SourceHeaderRegistry::create(&context, {*original->interface}, error());
  auto changed = compiler::SourceHeaderRegistry::create(&context, {*narrow->interface}, error());
  ASSERT_TRUE(succeeded(first) && succeeded(changed)) << diagnostics;
  auto captured = capture("memory.py", consumerSource);
  ASSERT_TRUE(captured);
  source::SourceRuleWritesAnalysis writes(captured->getOperation());
  source::SourceMemoryBindingsAnalysis analysis(captured->getOperation());
  ASSERT_TRUE(succeeded(analysis.initialize(owner(), *first, writes))) << diagnostics;
  EXPECT_TRUE(failed(analysis.initialize(owner(), *changed, writes)));
  EXPECT_NE(diagnostics.find("input identity changed"), std::string::npos) << diagnostics;
  diagnostics.clear();
  EXPECT_TRUE(failed(analysis.lower(owner(), *changed)));
  EXPECT_NE(diagnostics.find("unchanged binding plan"), std::string::npos) << diagnostics;
}

TEST_F(SourceMemoryBindingsTest, InferenceStopsBeforeRuntimeBodyLowering) {
  std::string text = memorySource.str();
  const auto position = text.find("    return Read");
  ASSERT_NE(position, std::string::npos);
  text.insert(position, "    unused = unsupported_runtime_expression(value)\n");
  auto captured = capture("memory.py", text);
  ASSERT_TRUE(captured);
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  source::SourceRuleWritesAnalysis writes(captured->getOperation());
  source::SourceMemoryBindingsAnalysis analysis(captured->getOperation());
  ASSERT_TRUE(succeeded(analysis.initialize(owner(), *headers, writes))) << diagnostics;
  ASSERT_EQ(analysis.memoryCalls().size(), 1u);
  EXPECT_EQ(analysis.memoryCalls()[0].depth, 3u);
  EXPECT_TRUE(captured->getBody()->empty());
  EXPECT_TRUE(failed(analysis.lower(owner(), *headers)));
}

// A plan-valid capture can contain obligations. Inference must retain them,
// and failed lowering must neither mutate the analysis nor publish a body.
TEST_F(SourceMemoryBindingsTest, PendingGrantPlanIsNotProofAndRemainsImmutable) {
  for (bool complementary : {false, true}) {
    std::string text = "import pycircuit as ac\n@ac.struct\nclass Out:\n    value: ac.u8\n"
        "@ac.rule\ndef put(value, data, enable):\n    if enable:\n        value = data\n"
        "@ac.module\ndef Top(p: ac.u1, q: ac.u1, data: ac.u8) -> Out:\n"
        "    value: ac.u8 = 7\n    put(value, data, p)\n    put(value, data, " +
        std::string(complementary ? "~p" : "q") + ")\n    return Out(value=value)\n";
    auto captured = capture("pending.py", text);
    ASSERT_TRUE(captured);
    source::SourceRuleWritesAnalysis writes(captured->getOperation());
    ASSERT_TRUE(writes.isPlanValid()) << diagnostics;
    ASSERT_EQ(writes.pendingPairs().size(), 1u);
    const auto pair = writes.pendingPairs()[0];
    EXPECT_NE(pair.firstCall.value.get("span"), pair.secondCall.value.get("span"));
    EXPECT_NE(pair.firstAssignment.value.get("span"), Attribute{});
    EXPECT_NE(pair.secondAssignment.value.get("span"), Attribute{});
    EXPECT_EQ(pair.firstCall.child("func").get("id"),
              pair.secondCall.child("func").get("id"));
    const auto spent = writes.spentWork();
    const auto captureAttr = (*captured)->getAttr("ac.python_capture");
    auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
    ASSERT_TRUE(succeeded(headers));
    source::SourceMemoryBindingsAnalysis bindings(captured->getOperation());
    ASSERT_TRUE(succeeded(bindings.initialize(owner("pending.py"), *headers, writes))) << diagnostics;
    noteLocations.clear();
    auto lowered = bindings.lower(owner("pending.py"), *headers);
    EXPECT_EQ(succeeded(lowered), complementary) << diagnostics;
    if (!complementary) {
      EXPECT_NE(std::find(noteLocations.begin(), noteLocations.end(),
                         pair.firstCall.location(&context, "pending.py")),
                noteLocations.end());
      EXPECT_NE(std::find(noteLocations.begin(), noteLocations.end(),
                         pair.secondCall.location(&context, "pending.py")),
                noteLocations.end());
    }
    EXPECT_EQ(writes.spentWork(), spent);
    EXPECT_EQ(writes.pendingPairs().size(), 1u);
    EXPECT_EQ(writes.pendingPairs()[0].firstCall.value.get("span"), pair.firstCall.value.get("span"));
    EXPECT_EQ((*captured)->getAttr("ac.python_capture"), captureAttr);
    EXPECT_TRUE(captured->getBody()->empty());
    EXPECT_TRUE(failed(bindings.lower(owner("pending.py"), *headers)));
  }
}

TEST_F(SourceMemoryBindingsTest, PendingEndpointsSurviveGrowingRegistrationStorage) {
  // Multiple owners and calls grow both storage vectors. Each pair must keep
  // the exact owner/call occurrences rather than borrow invalidated pointers.
  std::string text = "import pycircuit as ac\n@ac.struct\nclass Out:\n    value: ac.u8\n"
      "@ac.rule\ndef put(value, data, enable):\n    if enable:\n        value = data\n"
      "@ac.module\ndef Top(p: ac.u1, data: ac.u8) -> Out:\n";
  for (unsigned i = 0; i < 40; ++i)
    text += "    v" + std::to_string(i) + ": ac.u8 = 0\n";
  for (unsigned i = 0; i < 40; ++i)
    text += "    put(v" + std::to_string(i) + ", data, p)\n";
  for (unsigned i = 0; i < 40; ++i)
    text += "    put(v" + std::to_string(i) + ", data, ~p)\n";
  text += "    return Out(value=v39)\n";
  auto captured = capture("growing.py", text);
  ASSERT_TRUE(captured);
  source::SourceRuleWritesAnalysis writes(captured->getOperation());
  ASSERT_TRUE(writes.isPlanValid()) << diagnostics;
  ASSERT_EQ(writes.pendingPairs().size(), 40u);
  for (unsigned i = 0; i < 40; ++i) {
    const auto &pair = writes.pendingPairs()[i];
    EXPECT_EQ(pair.owner.child("target").get("id"),
              StringAttr::get(&context, "v" + std::to_string(i)));
    EXPECT_NE(pair.firstCall.value.get("span"), pair.secondCall.value.get("span"));
  }
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  source::SourceMemoryBindingsAnalysis bindings(captured->getOperation());
  ASSERT_TRUE(succeeded(bindings.initialize(owner("growing.py"), *headers, writes))) << diagnostics;
  EXPECT_TRUE(succeeded(bindings.lower(owner("growing.py"), *headers))) << diagnostics;
}

TEST_F(SourceMemoryBindingsTest, PendingPairCapAdmitsExactBoundaryAndRejectsNextPair) {
  for (bool overflow : {false, true}) {
    std::string text = "import pycircuit as ac\n@ac.struct\nclass Out:\n    value: ac.u8\n"
        "@ac.rule\ndef put(value, data, enable):\n    if enable:\n        value = data\n"
        "@ac.module\ndef Top(p: ac.u1, data: ac.u8) -> Out:\n";
    for (unsigned ownerIndex = 0; ownerIndex < 12; ++ownerIndex)
      text += "    v" + std::to_string(ownerIndex) + ": ac.u8 = 0\n";
    // C(128,2)+C(11,2)+9*C(2,2) = 8192 exact obligations.
    for (unsigned ownerIndex = 0; ownerIndex < (overflow ? 12u : 11u); ++ownerIndex) {
      unsigned registrations = ownerIndex == 0 ? 128 : ownerIndex == 1 ? 11 : 2;
      for (unsigned call = 0; call < registrations; ++call)
        text += "    put(v" + std::to_string(ownerIndex) + ", data, p)\n";
    }
    text += "    return Out(value=v0)\n";
    diagnostics.clear();
    auto captured = capture("pairs.py", text);
    ASSERT_TRUE(captured);
    source::SourceRuleWritesAnalysis writes(captured->getOperation());
    EXPECT_EQ(writes.isPlanValid(), !overflow) << diagnostics;
    EXPECT_EQ(writes.pendingPairs().size(), 8192u);
    if (overflow)
      EXPECT_NE(diagnostics.find("pending pair budget exhausted"), std::string::npos) << diagnostics;
  }
}

TEST_F(SourceMemoryBindingsTest, SourceWorkCapIsSharedAcrossUnusedModules) {
  for (unsigned modules : {1u, 2u}) {
    std::string text = "import pycircuit as ac\n@ac.struct\nclass Out:\n    value: ac.u8\n"
        "@ac.rule\ndef put(value, data):\n    value = data\n";
    for (unsigned moduleIndex = 0; moduleIndex < modules; ++moduleIndex) {
      text += "@ac.module\ndef Top" + std::to_string(moduleIndex) + "(data: ac.u8) -> Out:\n";
      for (unsigned i = 0; i < 1000; ++i)
        text += "    v" + std::to_string(i) + ": ac.u8 = 0\n";
      for (unsigned i = 0; i < 1000; ++i)
        text += "    put(v" + std::to_string(i) + ", data)\n";
      text += "    return Out(value=v999)\n";
    }
    diagnostics.clear();
    auto captured = capture("work.py", text);
    ASSERT_TRUE(captured);
    source::SourceRuleWritesAnalysis writes(captured->getOperation());
    EXPECT_EQ(writes.isPlanValid(), modules == 1) << diagnostics;
    EXPECT_TRUE(writes.pendingPairs().empty());
    if (modules == 1)
      EXPECT_LT(writes.spentWork(), 1048576u);
    else
      EXPECT_NE(diagnostics.find("work budget exhausted"), std::string::npos) << diagnostics;
  }
}

TEST_F(SourceMemoryBindingsTest, LoweringWorkCapChargesPaddedConstantsAcrossModules) {
  for (unsigned modules : {1u, 2u}) {
    std::string text = "import pycircuit as ac\n@ac.struct\nclass Out:\n    value: ac.u8\n"
        "@ac.rule\ndef put(value, data, enable):\n    if enable:\n        value = data\n";
    for (unsigned moduleIndex = 0; moduleIndex < modules; ++moduleIndex) {
      text += "@ac.module\ndef Top" + std::to_string(moduleIndex) +
          "(p: ac.bits[1000000], data: ac.u8) -> Out:\n";
      for (unsigned i = 0; i < (modules == 1 ? 32u : 20u); ++i)
        text += "    v" + std::to_string(i) + ": ac.u8 = 0\n"
            "    put(v" + std::to_string(i) + ", data, p == 0)\n"
            "    put(v" + std::to_string(i) + ", data, p == 1)\n";
      text += "    return Out(value=v0)\n";
    }
    diagnostics.clear();
    auto captured = capture("padded.py", text);
    ASSERT_TRUE(captured);
    const auto original = (*captured)->getAttr("ac.python_capture");
    source::SourceRuleWritesAnalysis writes(captured->getOperation());
    ASSERT_TRUE(writes.isPlanValid()) << diagnostics;
    const auto spent = writes.spentWork();
    auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
    ASSERT_TRUE(succeeded(headers));
    source::SourceMemoryBindingsAnalysis bindings(captured->getOperation());
    ASSERT_TRUE(succeeded(bindings.initialize(owner("padded.py"), *headers, writes))) << diagnostics;
    auto lowered = bindings.lower(owner("padded.py"), *headers);
    EXPECT_EQ(succeeded(lowered), modules == 1) << diagnostics;
    EXPECT_EQ(writes.spentWork(), spent);
    EXPECT_EQ((*captured)->getAttr("ac.python_capture"), original);
    EXPECT_TRUE(captured->getBody()->empty());
    if (modules == 2)
      EXPECT_NE(diagnostics.find("work budget exhausted"), std::string::npos) << diagnostics;
  }
}

TEST_F(SourceMemoryBindingsTest, CyclicGrantProducerFailsWholeSourceVerification) {
  // Grants may be complements of a shared opaque producer, but that does not
  // exempt the producer graph from ordinary whole-source cycle verification.
  auto captured = capture("cycle.py", R"py(import pycircuit as ac
@ac.struct
class Flag:
    value: ac.u1
@ac.struct
class Out:
    value: ac.u8
@ac.rule
def put(value, data, enable):
    if enable:
        value = data
@ac.module
def Flip(p: ac.u1) -> Flag:
    return Flag(value=~p)
@ac.module
def Top(data: ac.u8) -> Out:
    value: ac.u8 = 7
    a = Flip(b.value)
    b = Flip(a.value)
    grant = a.value
    put(value, data, grant)
    put(value, data, ~grant)
    return Out(value=value)
)py");
  ASSERT_TRUE(captured);
  source::SourceRuleWritesAnalysis writes(captured->getOperation());
  ASSERT_TRUE(writes.isPlanValid()) << diagnostics;
  ASSERT_EQ(writes.pendingPairs().size(), 1u);
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  EXPECT_TRUE(failed(compiler::compilePythonSourceUnit(
      *captured, owner("cycle.py"), *headers, error())));
  EXPECT_NE(diagnostics.find("combinational cycle"), std::string::npos) << diagnostics;
}

TEST_F(SourceMemoryBindingsTest, OptionalSourceImportIsTheVerifiedUnsimplifiedBody) {
  auto captured = capture("projection.py", R"py(import pycircuit as ac
@ac.struct
class Payload:
    value: ac.u13
@ac.module
def Projection(value: ac.u13) -> Payload:
    local = Payload(value=value)
    return Payload(value=local.value)
)py");
  ASSERT_TRUE(captured);
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  auto print = [](ModuleOp module) {
    std::string text;
    llvm::raw_string_ostream out(text);
    module.print(out, OpPrintingFlags().enableDebugInfo());
    out << '\n';
    return text;
  };
  const auto before = print(*captured);
  auto ordinary = compiler::compilePythonSourceUnit(
      *captured, owner("projection.py"), *headers, error());
  ASSERT_TRUE(succeeded(ordinary)) << diagnostics;
  EXPECT_FALSE(ordinary->sourceImport.has_value());
  auto retained = compiler::compilePythonSourceUnit(
      *captured, owner("projection.py"), *headers, error(), true);
  ASSERT_TRUE(succeeded(retained)) << diagnostics;
  ASSERT_TRUE(retained->sourceImport.has_value());
  EXPECT_EQ(print(*ordinary->body), print(*retained->body));
  EXPECT_EQ(print(*ordinary->interface), print(*retained->interface));
  EXPECT_EQ(print(*captured), before);

  // Parsing one complete ModuleOp rejects concatenated boundary dumps. The
  // create/projection relation identifies the actual pre-Simplify stage.
  const auto &text = *retained->sourceImport;
  ASSERT_FALSE(text.empty());
  EXPECT_EQ(text.back(), '\n');
  auto imported = parseSourceString<ModuleOp>(text, &context);
  ASSERT_TRUE(imported) << diagnostics;
  EXPECT_TRUE(succeeded(verify(*imported))) << diagnostics;
  EXPECT_EQ((*imported)->getAttr("ac.source_owner"), owner("projection.py"));
  unsigned forwardingRelations = 0, importedModules = 0, importedCreates = 0;
  imported->walk([&](ac::StructGetOp get) {
    forwardingRelations +=
        static_cast<bool>(get.getValue().getDefiningOp<ac::StructCreateOp>());
  });
  imported->walk([&](ModuleOp) { ++importedModules; });
  imported->walk([&](ac::StructCreateOp) { ++importedCreates; });
  EXPECT_GT(forwardingRelations, 0u);
  EXPECT_EQ(importedModules, 1u);
  unsigned remainingCreates = 0, remainingGets = 0;
  retained->body->walk([&](ac::StructCreateOp) { ++remainingCreates; });
  retained->body->walk([&](ac::StructGetOp) { ++remainingGets; });
  EXPECT_LT(remainingCreates, importedCreates);
  EXPECT_GT(remainingCreates, 0u);
  EXPECT_EQ(remainingGets, 0u);
  EXPECT_TRUE(succeeded(verify(*retained->body))) << diagnostics;
  EXPECT_TRUE(succeeded(verify(*retained->interface))) << diagnostics;
}

TEST_F(SourceMemoryBindingsTest,
       RequestedSourceImportDoesNotAdmitInvalidSource) {
  auto captured = capture("invalid.py", R"py(import pycircuit as ac
@ac.struct
class Payload:
    value: ac.u13
@ac.module
def Invalid(value: ac.u13) -> Payload:
    return Payload(value=missing)
)py");
  ASSERT_TRUE(captured);
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  EXPECT_TRUE(failed(compiler::compilePythonSourceUnit(
      *captured, owner("invalid.py"), *headers, error(), true)));
  EXPECT_NE(diagnostics.find("unknown hardware value 'missing'"),
            std::string::npos)
      << diagnostics;
}

// --- General dependent-width and type-authority regressions -------------
// Resolving a declared dependent width before comparison must not be specific
// to the memory strobe, and it must not merge distinct logical or nominal
// types that merely share a physical width.
class SourceDependentWidthTest : public SourceMemoryBindingsTest {
protected:
  // Reuse the production capture plus the real analyze/infer/lower pipeline.
  // The lowered transport is returned so a test can read the widths the
  // compiler actually committed to instead of only accept/reject.
  OwningOpRef<ModuleOp> lowered(StringRef name, StringRef text) {
    auto captured = capture(name, text);
    if (!captured)
      return {};
    auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
    if (failed(headers))
      return {};
    diagnostics.clear();
    PassManager pipeline(&context);
    pipeline.addPass(source::createAnalyzeRuleWritesPass());
    pipeline.addPass(
        source::createInferSourceBindingsPass(owner(name), *headers));
    pipeline.addPass(
        source::createLowerPythonSourcePass(owner(name), *headers));
    if (failed(pipeline.run(*captured)))
      return {};
    return captured;
  }
  bool lower(StringRef name, StringRef text) {
    auto module = lowered(name, text);
    return module.get() != nullptr;
  }
  // Resolve one instance result type through the same analysis the importer
  // uses, so the assertion reads the resolved payload width.
  std::optional<uint64_t> instanceWidth(ModuleOp module, StringRef name) {
    ac::HardwareAnalysis analysis(module);
    std::optional<uint64_t> width;
    module.walk([&](ac::InstanceOp instance) {
      if (instance.getInstanceName() != name || instance.getNumResults() != 1)
        return;
      auto resolved = analysis.getPackedWidth(instance.getResult(0).getType(),
                                              {}, instance.getOperation());
      if (succeeded(resolved))
        width = *resolved;
    });
    return width;
  }
  static std::string leafSource(StringRef typeText) {
    return ("from typing import Annotated\n"
            "import pycircuit as ac\n"
            "from pycircuit import dff\n"
            "@ac.module\n"
            "def Holder(clk: bool, rst: bool, d: " + typeText.str() +
            ") -> {\"q\": " + typeText.str() + "}:\n"
            "    state = dff(T=" + typeText.str() + ")\n"
            "    @ac.rule\n"
            "    def bind():\n"
            "        state(clk=clk, rst=rst, d=d, init=0)\n"
            "    bind()\n"
            "    return {\"q\": d}\n");
  }
};

// A computed declared width binds on a non-memory leaf.
TEST_F(SourceDependentWidthTest, ComputedDependentWidthBindsOutsideMemory) {
  EXPECT_TRUE(lower("computed.py",
                    leafSource("Annotated[int, range(1 << (4 + 4))]")))
      << diagnostics;
}

// A computed width and its literal spelling are the same resolved width.
TEST_F(SourceDependentWidthTest, LiteralAndComputedWidthsAgree) {
  EXPECT_TRUE(lower("computed.py",
                    leafSource("Annotated[int, range(1 << (4 + 4))]")))
      << diagnostics;
  EXPECT_TRUE(
      lower("literal.py", leafSource("Annotated[int, range(1 << 8)]")))
      << diagnostics;
}

// A real width disagreement is still rejected, so resolution did not become a
// blanket acceptance of any actual type.
TEST_F(SourceDependentWidthTest, DependentWidthMismatchStillRejects) {
  std::string text = ("from typing import Annotated\n"
                      "import pycircuit as ac\n"
                      "from pycircuit import dff\n"
                      "@ac.module\n"
                      "def Holder(clk: bool, rst: bool, "
                      "d: Annotated[int, range(1 << 9)]) -> "
                      "{\"q\": Annotated[int, range(1 << 9)]}:\n"
                      "    state = dff(T=Annotated[int, range(1 << 8)])\n"
                      "    @ac.rule\n"
                      "    def bind():\n"
                      "        state(clk=clk, rst=rst, d=d, init=0)\n"
                      "    bind()\n"
                      "    return {\"q\": d}\n");
  diagnostics.clear();
  EXPECT_FALSE(lower("mismatch.py", text));
  EXPECT_FALSE(diagnostics.empty()) << "mismatch must be diagnosed";
}

// Equal packed width does not merge nominal identity or the Boolean kind.
TEST_F(SourceDependentWidthTest, NominalAndBooleanIdentitySurvivesResolution) {
  std::string nominal = ("import pycircuit as ac\n"
                         "@ac.struct\n"
                         "class Left:\n    value: ac.u8\n"
                         "@ac.struct\n"
                         "class Right:\n    value: ac.u8\n"
                         "@ac.module\n"
                         "def Take(x: Left) -> Left:\n"
                         "    return x\n");
  diagnostics.clear();
  EXPECT_TRUE(lower("nominal.py", nominal)) << diagnostics;

  // A Boolean actual must not satisfy a one-bit bits destination.
  std::string boolean = ("from typing import Annotated\n"
                         "import pycircuit as ac\n"
                         "from pycircuit import dff\n"
                         "@ac.module\n"
                         "def Holder(clk: bool, rst: bool, d: bool) -> "
                         "{\"q\": Annotated[int, range(1 << 1)]}:\n"
                         "    state = dff(T=Annotated[int, range(1 << 1)])\n"
                         "    @ac.rule\n"
                         "    def bind():\n"
                         "        state(clk=clk, rst=rst, d=d, init=0)\n"
                         "    bind()\n"
                         "    return {\"q\": d}\n");
  diagnostics.clear();
  EXPECT_FALSE(lower("boolean.py", boolean));
  EXPECT_FALSE(diagnostics.empty())
      << "Boolean must not be silently accepted as a bits destination";
}

// --- Dependent-width negative coverage ------------------------------------
// Resolving a declared dependent width before comparison must stay a bounded
// equivalence check over the actual SSA value. Missing, wrong, unclosable,
// illegally spelled and nominally different actuals must all still be rejected,
// and two standard leaves bound with different type arguments must keep their
// own resolved widths instead of collapsing into one shared acceptance.

namespace {
// A single standard leaf whose declared payload width is exactly `leafWidth`,
// with a concrete module payload port. Acceptance requires the resolved leaf
// width and the port type to agree. `returned` selects whether the module
// output carries the port value or the leaf's own output.
std::string leafWidthSource(StringRef leafWidth, StringRef payload,
                            StringRef returned = "d") {
  return ("from typing import Annotated\n"
          "import pycircuit as ac\n"
          "from pycircuit import dff\n"
          "@ac.module\n"
          "def Holder(clk: bool, rst: bool, d: " + payload.str() +
          ") -> {\"q\": " + payload.str() + "}:\n"
          "    state = dff(T=" + leafWidth.str() + ")\n"
          "    @ac.rule\n"
          "    def bind():\n"
          "        state(clk=clk, rst=rst, d=d, init=0)\n"
          "    bind()\n"
          "    return {\"q\": " + returned.str() + "}\n");
}
} // namespace

// A dependent-width leaf input with no actual is still a missing binding.
TEST_F(SourceDependentWidthTest, MissingActualOnDependentWidthPortRejects) {
  std::string text = ("from typing import Annotated\n"
                      "import pycircuit as ac\n"
                      "from pycircuit import dff\n"
                      "@ac.module\n"
                      "def Holder(clk: bool, rst: bool, "
                      "d: Annotated[int, range(1 << (4 + 4))]) -> "
                      "{\"q\": Annotated[int, range(1 << (4 + 4))]}:\n"
                      "    state = dff(T=Annotated[int, range(1 << (4 + 4))])\n"
                      "    @ac.rule\n"
                      "    def bind():\n"
                      "        state(clk=clk, rst=rst, init=0)\n"
                      "    bind()\n"
                      "    return {\"q\": d}\n");
  diagnostics.clear();
  EXPECT_FALSE(lower("missing-actual.py", text));
  EXPECT_NE(diagnostics.find("missing instance input 'd'"), std::string::npos)
      << diagnostics;
}

// A dependent width that cannot be closed must not ride the resolved
// equivalence path into acceptance: the concrete 8-bit payload port would be
// accepted if the unbound reference were treated as an opaque match.
TEST_F(SourceDependentWidthTest, UnclosedDependentReferenceIsNotAccepted) {
  EXPECT_TRUE(lower("closed.py", leafWidthSource("ac.u8", "ac.u8")))
      << diagnostics;
  diagnostics.clear();
  EXPECT_FALSE(lower("unclosed.py",
                     leafWidthSource("Annotated[int, range(1 << W)]",
                                     "ac.u8")));
  EXPECT_FALSE(diagnostics.empty())
      << "an unclosable dependent width must be diagnosed";
}

// The same leaf bound with a different type argument resolves to that
// argument's own width; resolution is per binding, not per leaf kind.
// `range(1 << N)` declares an N-bit payload, so the two leaves below are 8 and
// 12 bits wide through two different type arguments.
TEST_F(SourceDependentWidthTest, TypeArgumentSelectsItsOwnResolvedWidth) {
  std::string text =
      ("from typing import Annotated\n"
       "import pycircuit as ac\n"
       "from pycircuit import dff\n"
       "@ac.module\n"
       "def Holder(clk: bool, rst: bool, "
       "d: Annotated[int, range(1 << (4 + 4))], "
       "e: Annotated[int, range(1 << 12)]) -> "
       "{\"q\": Annotated[int, range(1 << (4 + 4))], "
       "\"r\": Annotated[int, range(1 << 12)]}:\n"
       "    narrow = dff(T=Annotated[int, range(1 << (4 + 4))])\n"
       "    wide = dff(T=Annotated[int, range(1 << 12)])\n"
       "    @ac.rule\n"
       "    def bindnarrow():\n"
       "        narrow(clk=clk, rst=rst, d=d, init=0)\n"
       "    @ac.rule\n"
       "    def bindwide():\n"
       "        wide(clk=clk, rst=rst, d=e, init=0)\n"
       "    bindnarrow()\n"
       "    bindwide()\n"
       "    return {\"q\": d, \"r\": e}\n");
  auto module = lowered("type-arguments.py", text);
  ASSERT_TRUE(module) << diagnostics;
  EXPECT_EQ(instanceWidth(module.get(), "narrow"), std::optional<uint64_t>(8));
  EXPECT_EQ(instanceWidth(module.get(), "wide"), std::optional<uint64_t>(12));
  // The two type arguments must not collapse into one shared width.
  EXPECT_NE(instanceWidth(module.get(), "narrow"),
            instanceWidth(module.get(), "wide"));
}

// A width that disagrees with the type argument's resolved width still rejects.
TEST_F(SourceDependentWidthTest, WrongActualForTypeArgumentWidthRejects) {
  std::string text = ("from typing import Annotated\n"
                      "import pycircuit as ac\n"
                      "from pycircuit import dff\n"
                      "@ac.module\n"
                      "def Holder(clk: bool, rst: bool, "
                      "d: Annotated[int, range(1 << 12)]) -> "
                      "{\"q\": Annotated[int, range(1 << 12)]}:\n"
                      "    narrow = dff(T=Annotated[int, range(1 << (4 + 4))])\n"
                      "    @ac.rule\n"
                      "    def bindnarrow():\n"
                      "        narrow(clk=clk, rst=rst, d=d, init=0)\n"
                      "    bindnarrow()\n"
                      "    return {\"q\": d}\n");
  diagnostics.clear();
  EXPECT_FALSE(lower("type-argument-mismatch.py", text));
  EXPECT_FALSE(diagnostics.empty()) << "a real width disagreement must reject";
}

// A computed dependent width is resolved in the output direction too, and a
// narrowing result is still refused.
TEST_F(SourceDependentWidthTest, ComputedDependentWidthResolvesOnOutputPort) {
  EXPECT_TRUE(lower("output.py",
                    leafWidthSource("Annotated[int, range(1 << (4 + 4))]",
                                    "Annotated[int, range(1 << (4 + 4))]",
                                    "state.q")))
      << diagnostics;
  diagnostics.clear();
  EXPECT_FALSE(lower("output-narrow.py",
                     leafWidthSource("Annotated[int, range(1 << 9)]",
                                     "Annotated[int, range(1 << (4 + 4))]",
                                     "state.q")));
  EXPECT_FALSE(diagnostics.empty())
      << "a narrowing dependent-width output must reject";
}

// An illegal width literal must not normalise into equivalence with a legal
// one: `1 << True` is not the integer width one, and a zero width is not any
// legal width.
TEST_F(SourceDependentWidthTest, IllegalWidthLiteralIsNotALegalWidth) {
  EXPECT_TRUE(lower("legal-one-bit.py",
                    leafWidthSource("Annotated[int, range(1 << (2 - 1))]",
                                    "ac.u1")))
      << diagnostics;
  diagnostics.clear();
  EXPECT_FALSE(lower("boolean-width.py",
                     leafWidthSource("Annotated[int, range(1 << True)]",
                                     "ac.u1")));
  EXPECT_NE(diagnostics.find("must be a mathematical integer"),
            std::string::npos)
      << diagnostics;
  diagnostics.clear();
  EXPECT_FALSE(lower("zero-width.py", leafWidthSource("ac.bits[0]", "ac.u1")));
  EXPECT_NE(diagnostics.find("must be positive"), std::string::npos)
      << diagnostics;
}

// Equal packed width must not merge nominal identity or the Boolean kind when
// the destination is reached through a resolved computed width.
TEST_F(SourceDependentWidthTest, NominalAndBooleanKindSurviveCrossBinding) {
  std::string cross = ("import pycircuit as ac\n"
                       "from pycircuit import dff\n"
                       "@ac.struct\n"
                       "class Left:\n    value: ac.u8\n"
                       "@ac.struct\n"
                       "class Right:\n    value: ac.u8\n"
                       "@ac.module\n"
                       "def Holder(clk: bool, rst: bool, r: Right) -> "
                       "{\"q\": Left}:\n"
                       "    state = dff(T=Left)\n"
                       "    @ac.rule\n"
                       "    def bind():\n"
                       "        state(clk=clk, rst=rst, d=r, init=0)\n"
                       "    bind()\n"
                       "    return {\"q\": state.q}\n");
  diagnostics.clear();
  EXPECT_FALSE(lower("nominal-cross.py", cross));
  EXPECT_FALSE(diagnostics.empty())
      << "a same-width nominal mismatch must reject";
  std::string same = ("import pycircuit as ac\n"
                      "from pycircuit import dff\n"
                      "@ac.struct\n"
                      "class Left:\n    value: ac.u8\n"
                      "@ac.struct\n"
                      "class Right:\n    value: ac.u8\n"
                      "@ac.module\n"
                      "def Holder(clk: bool, rst: bool, l: Left) -> "
                      "{\"q\": Left}:\n"
                      "    state = dff(T=Left)\n"
                      "    @ac.rule\n"
                      "    def bind():\n"
                      "        state(clk=clk, rst=rst, d=l, init=0)\n"
                      "    bind()\n"
                      "    return {\"q\": state.q}\n");
  diagnostics.clear();
  EXPECT_TRUE(lower("nominal-same.py", same)) << diagnostics;

  // A computed one-bit destination is still not a Boolean destination.
  std::string booleanComputed =
      ("from typing import Annotated\n"
       "import pycircuit as ac\n"
       "from pycircuit import dff\n"
       "@ac.module\n"
       "def Holder(clk: bool, rst: bool, d: bool) -> "
       "{\"q\": Annotated[int, range(1 << (2 - 1))]}:\n"
       "    state = dff(T=Annotated[int, range(1 << (2 - 1))])\n"
       "    @ac.rule\n"
       "    def bind():\n"
       "        state(clk=clk, rst=rst, d=d, init=0)\n"
       "    bind()\n"
       "    return {\"q\": state.q}\n");
  diagnostics.clear();
  EXPECT_FALSE(lower("boolean-computed.py", booleanComputed));
  EXPECT_NE(diagnostics.find("Boolean and Integer kinds"), std::string::npos)
      << diagnostics;
}
} // namespace


// Cross-spelling output tests must return the actual leaf result, not the input
// port: the latter never visits the structural output boundary in question.
TEST_F(SourceDependentWidthTest, ClosedStructuralResultUsesDeclaredWidth) {
  for (auto [leaf, port] : {std::pair<StringRef, StringRef>{"ac.bits[4 + 4]", "ac.u8"},
                           {"ac.u8", "ac.bits[4 + 4]"}}) {
    auto module = lowered("closed_output.py", leafWidthSource(leaf, port, "state.q"));
    ASSERT_TRUE(module) << diagnostics;
    EXPECT_TRUE(succeeded(mlir::verify(*module))) << diagnostics;
  }
}

TEST_F(SourceDependentWidthTest, ParameterDefaultIsNotAWidthEqualityProof) {
  for (StringRef expression : {"W", "W - 4"}) {
    auto text = leafWidthSource("ac.bits[" + expression.str() + "]", "ac.u8", "state.q");
    auto signature = text.find("d: ac.u8)");
    ASSERT_NE(signature, std::string::npos);
    text.replace(signature, std::string("d: ac.u8)").size(),
                 expression == "W" ? "d: ac.u8, *, W: int = 8)" :
                                     "d: ac.u8, *, W: int = 12)");
    EXPECT_FALSE(lower("generic_default.py", text));
    EXPECT_FALSE(diagnostics.empty());
  }
}


TEST_F(SourceDependentWidthTest, IntegerToComputedFixedPreservesBoundaryProof) {
  for (StringRef spelling : {"ac.u8", "ac.bits[4 + 4]"}) {
    std::string text = "from typing import Annotated\nimport pycircuit as ac\n"
        "@ac.struct\nclass Out:\n    value: " + spelling.str() + "\n"
        "@ac.module\ndef Top(value: Annotated[int, range(1 << 8)]) -> Out:\n"
        "    return Out(value=value)\n";
    EXPECT_TRUE(lower("integer_to_fixed.py", text)) << diagnostics;
  }
}


TEST_F(SourceDependentWidthTest, ExplicitComputedBitsDoesNotRetypeBooleanAlias) {
  for (StringRef spelling : {"ac.u1", "ac.bits[2 - 1]"}) {
    std::string text = "import pycircuit as ac\n@ac.struct\nclass Out:\n    value: ac.u1\n"
        "@ac.rule\ndef convert(flag) -> Out:\n    bound: " + spelling.str() + " = flag\n"
        "    return Out(value=ac.popcount(bound))\n"
        "@ac.module\ndef Top(flag: bool) -> Out:\n    return convert(flag)\n";
    EXPECT_TRUE(lower("converted_flag.py", text)) << diagnostics;
    auto at = text.find("ac.popcount(bound)");
    ASSERT_NE(at, std::string::npos);
    text.replace(at, std::string("ac.popcount(bound)").size(), "ac.popcount(flag)");
    EXPECT_FALSE(lower("original_flag.py", text));
    EXPECT_NE(diagnostics.find("explicitly unsigned fixed bits"), std::string::npos)
        << diagnostics;
  }
}
