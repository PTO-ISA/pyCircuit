#include "ComposedFixture.h"
#include "Compiler/ScalarNumericLowering.h"
#include "Compiler/SourceUnit.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include <array>

using namespace mlir;
namespace {
bool write(llvm::StringRef path, llvm::StringRef text) {
  std::error_code error;
  llvm::raw_fd_ostream out(path, error);
  if (error)
    return false;
  out << text;
  out.close();
  return !out.has_error();
}
constexpr llvm::StringLiteral types = R"py(from typing import Annotated
Word = Annotated[int, range(256)]
Phase = Annotated[int, range(8)]
)py";
constexpr llvm::StringLiteral increment =
    R"py(from pycircuit import module, rule
from .types import Word
@module
def Increment(enabled: bool, incoming: Word, outgoing: Word):
    @rule
    def update():
        nonlocal outgoing
        if enabled and incoming != 0:
            outgoing = (incoming + 1) & 255
    update()
)py";
constexpr llvm::StringLiteral pipelineSource = R"py(from pycircuit import module
from .types import Word
from .increment import Increment
@module
def Pipeline(enabled: bool, incoming: Word, outgoing: Word):
    middle: Word = 0
    first = Increment(enabled, incoming, middle)
    second = Increment(enabled, middle, outgoing)
)py";

std::string systemSource(bool pipeline) {
  std::string name = pipeline ? "Pipeline" : "Increment";
  std::string source =
      "from pycircuit import system, rule, log, report\n"
      "from .types import Word, Phase\nfrom ." +
      std::string(pipeline ? "pipeline" : "increment") + " import " + name +
      "\n@system\ndef Test" + name +
      "():\n"
      "    enabled: bool = True\n    incoming: Word = 1\n"
      "    outgoing: Word = 0\n    phase: Phase = 0\n    dut = " +
      name +
      "(enabled, incoming, outgoing)\n\n"
      "    @rule\n    def fixture():\n"
      "        nonlocal enabled, incoming, phase\n"
      "        if phase == 0:\n"
      "            assert outgoing == 0, \"reset output\"\n"
      "            incoming = 4\n"
      "        elif phase == 1:\n"
      "            assert outgoing == " +
      std::string(pipeline ? "0" : "2") +
      ", \"first input\"\n            incoming = 0\n"
      "        elif phase == 2:\n"
      "            assert outgoing == " +
      std::string(pipeline ? "3" : "5") +
      ", \"second input\"\n"
      "        elif phase == 3:\n"
      "            assert outgoing == " +
      std::string(pipeline ? "6" : "5") +
      ", \"disabled input holds\"\n            enabled = False\n"
      "        elif phase == 4:\n"
      "            assert outgoing == " +
      std::string(pipeline ? "6" : "5") +
      ", \"final output\"\n"
      "            print(\"" +
      std::string(pipeline ? "pipeline" : "increment") +
      " passed\", outgoing)\n"
      "            log(\"info\", \"test_complete\", outgoing)\n"
      "            report(\"completed\", 1)\n"
      "        if phase < 4:\n            phase = phase + 1\n\n"
      "    fixture()\n";
  return source;
}
} // namespace

FailureOr<acir::compiler::FinalProgram>
buildComposedFixture(MLIRContext &context, bool pipeline,
                     acir::ac::detail::EmitError error) {
  using namespace acir::compiler;
  llvm::SmallString<256> temporary;
  if (llvm::sys::fs::createUniqueDirectory("acir-composed", temporary))
    return error() << "cannot create source fixture workspace";
  auto cleanup =
      llvm::scope_exit([&] { llvm::sys::fs::remove_directories(temporary); });
  const std::string root = temporary.str().str();
  SmallVector<std::pair<std::string, std::string>> sources{
      {"types.py", types.str()}, {"increment.py", increment.str()}};
  if (pipeline)
    sources.push_back({"pipeline.py", pipelineSource.str()});
  sources.push_back({pipeline ? "test_pipeline.py" : "test_increment.py",
                     systemSource(pipeline)});
  static constexpr llvm::StringLiteral capture = R"py(
import sys
from pathlib import Path
repo = Path(sys.argv[1])
sys.path[:0] = [str(repo / "python/pycircuit/src")]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
root = Path(sys.argv[2])
Path(sys.argv[4]).write_text(_emit_source_transport(_capture_source_file(root / sys.argv[3], source_root=root)), encoding="utf-8")
)py";
  SmallVector<SourceUnitArtifacts> artifacts;
  SmallVector<ModuleOp> headers;
  SmallVector<SourceLinkUnit> units;
  Builder b(&context);
  for (const auto &[path, source] : sources) {
    std::string transportPath = root + "/" + path + ".capture.mlir";
    std::string log = root + "/capture.log";
    if (!write(root + "/" + path, source))
      return error() << "cannot write fixture source " << path;
    std::array<std::string, 7> owned{
        ACIR_BACKEND_PYTHON,    "-c", capture.str(),
        ACIR_BACKEND_REPO_ROOT, root, path,
        transportPath};
    SmallVector<llvm::StringRef> args(owned.begin(), owned.end());
    std::array<std::optional<llvm::StringRef>, 3> redirects{std::nullopt, log,
                                                            log};
    if (llvm::sys::ExecuteAndWait(args.front(), args, std::nullopt,
                                  redirects) != 0)
      return error() << "cannot capture fixture source " << path;
    auto transport = parseSourceFile<ModuleOp>(transportPath, &context);
    auto registry = SourceHeaderRegistry::create(headers, error);
    if (!transport || failed(registry))
      return failure();
    auto owner =
        b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr("w10")),
                             b.getNamedAttr("path", b.getStringAttr(path))});
    auto unit = compilePythonSourceUnit(*transport, owner, *registry, error);
    if (failed(unit))
      return failure();
    auto kind = (*unit->body)->getAttrOfType<StringAttr>("ac.unit_kind");
    if (kind && kind.getValue() == "implementation" &&
        failed(lowerExactInputAddTransactional(*unit->body)))
      return failure();
    headers.push_back(*unit->interface);
    units.push_back({*unit->body, *unit->interface});
    artifacts.push_back(std::move(*unit));
  }
  auto analysis = buildFinalProgram(units, error);
  if (failed(analysis))
    return failure();
  return materializeFinalProgram(std::move(*analysis), error);
}
