#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#if __has_include("Compiler/FinalProgram.h")
#include "Compiler/FinalProgram.h"
#define ACIR_HAS_FINAL_PROGRAM 1
#include <concepts>
#include <string>

namespace acir::compiler {
template <typename Program>
concept HasFinalCppEntry = requires(const Program &program,
                                    ac::detail::EmitError emitError) {
  { emitFinalCpp(program, emitError) } ->
      std::same_as<mlir::FailureOr<std::string>>;
};

template <typename Program>
concept HasFinalVerilogEntry = requires(const Program &program,
                                        ac::detail::EmitError emitError) {
  { emitFinalVerilog(program, emitError) } ->
      std::same_as<mlir::FailureOr<std::string>>;
};
} // namespace acir::compiler
#else
#define ACIR_HAS_FINAL_PROGRAM 0
#endif

#include <array>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

namespace {

constexpr llvm::StringLiteral kBackendApi = "pycircuit-w10-backend-closure-v1";

struct TemporaryDirectory {
  llvm::SmallString<256> path;

  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-backend-closure", path));
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
  return buffer ? buffer.get()->getBuffer().str() : std::string{};
}

struct ProcessResult {
  int status = -1;
  std::string output;
};

std::optional<std::string> backendHarness() {
  const char *configured = std::getenv("ACIR_BACKEND_CLOSURE_HARNESS");
  if (!configured || !*configured)
    return std::nullopt;
  llvm::sys::fs::file_status status;
  if (llvm::sys::fs::status(configured, status) ||
      !llvm::sys::fs::is_regular_file(status) ||
      !llvm::sys::fs::can_execute(configured))
    return std::nullopt;
  return std::string(configured);
}

ProcessResult runHarness(llvm::StringRef program,
                         const std::vector<std::string> &arguments,
                         llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> refs;
  refs.push_back(program);
  for (const std::string &argument : arguments)
    refs.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  int status =
      llvm::sys::ExecuteAndWait(program, refs, std::nullopt, redirects);
  return {status, readFile(log)};
}

class BackendClosureTest : public ::testing::Test {
protected:
  std::string requireHarness() {
    auto harness = backendHarness();
    if (!harness) {
      ADD_FAILURE()
          << "W10 RED: set ACIR_BACKEND_CLOSURE_HARNESS to the private "
             "verified-final materializer and dual-backend test harness; the "
             "legacy QueueGraph/PYC emitter is not an admissible substitute";
      return {};
    }
    ProcessResult api =
        runHarness(*harness, {"--api"}, temporary.child("api.log"));
    EXPECT_EQ(api.status, 0) << api.output;
    EXPECT_EQ(api.output, (kBackendApi + "\n").str());
    return *harness;
  }

  void expectRejectedWithoutPublication(llvm::StringRef mutation) {
    std::string harness = requireHarness();
    ASSERT_FALSE(harness.empty());
    std::string cpp = temporary.child((mutation + ".cpp").str());
    std::string verilog = temporary.child((mutation + ".sv").str());
    writeFile(cpp, "cpp-sentinel\n");
    writeFile(verilog, "verilog-sentinel\n");
    ProcessResult result = runHarness(
        harness,
        {"--fixture", "minimal-final", "--backend", "both", "--mutation",
         mutation.str(), "--cpp-out", cpp, "--verilog-out", verilog},
        temporary.child((mutation + ".log").str()));
    EXPECT_NE(result.status, 0) << mutation.str();
    EXPECT_NE(result.output.find("final verification rejected"),
              std::string::npos)
        << result.output;
    EXPECT_EQ(readFile(cpp), "cpp-sentinel\n");
    EXPECT_EQ(readFile(verilog), "verilog-sentinel\n");
  }

  TemporaryDirectory temporary;
};

TEST_F(BackendClosureTest, V31_OneVerifiedFinalGraphFeedsBothBackends) {
  std::string harness = requireHarness();
  ASSERT_FALSE(harness.empty());
  ProcessResult result =
      runHarness(harness,
                 {"--fixture", "minimal-final", "--backend", "both",
                  "--cpp-out", temporary.child("minimal.cpp"), "--verilog-out",
                  temporary.child("minimal.sv"), "--manifest-out",
                  temporary.child("minimal.json")},
                 temporary.child("minimal.log"));
  EXPECT_EQ(result.status, 0) << result.output;
  EXPECT_FALSE(readFile(temporary.child("minimal.cpp")).empty());
  EXPECT_FALSE(readFile(temporary.child("minimal.sv")).empty());
  std::string manifest = readFile(temporary.child("minimal.json"));
  EXPECT_NE(manifest.find("\"input_identity\""), std::string::npos);
  EXPECT_NE(manifest.find("\"cpp\""), std::string::npos);
  EXPECT_NE(manifest.find("\"verilog\""), std::string::npos);
}

TEST_F(BackendClosureTest, V31_ResidualSourceFactsRejectBeforePublication) {
  for (llvm::StringRef mutation :
       {"residual-source-read", "residual-source-use",
        "residual-source-observe"}) {
    SCOPED_TRACE(mutation.str());
    expectRejectedWithoutPublication(mutation);
  }
}

TEST_F(BackendClosureTest, V31_ForgedStageProofAndTargetSwapReject) {
  for (llvm::StringRef mutation :
       {"forged-final-stage", "forged-proof", "same-type-target-swap"}) {
    SCOPED_TRACE(mutation.str());
    expectRejectedWithoutPublication(mutation);
  }
}

TEST_F(BackendClosureTest, V29_PermitProjectionMustReachEveryCommit) {
  for (llvm::StringRef mutation :
       {"permit-disconnected", "permit-inverted", "commit-enable-bypass"}) {
    SCOPED_TRACE(mutation.str());
    expectRejectedWithoutPublication(mutation);
  }
}

TEST_F(BackendClosureTest, V30_ErrorCannotDependOnPostGatedEnable) {
  expectRejectedWithoutPublication("error-from-gated-enable");
}

#if ACIR_HAS_FINAL_PROGRAM
TEST(FinalBackendEntryApiAvailability, EmitsOnlyFromFinalProgram) {
  EXPECT_TRUE((acir::compiler::HasFinalCppEntry<
               acir::compiler::FinalProgram>))
      << "W10 RED: missing emitFinalCpp(const FinalProgram&, EmitError) -> "
         "FailureOr<string>";
  EXPECT_TRUE((acir::compiler::HasFinalVerilogEntry<
               acir::compiler::FinalProgram>))
      << "W10 RED: missing emitFinalVerilog(const FinalProgram&, EmitError) "
         "-> FailureOr<string>";
}
#else
TEST(FinalBackendEntryApiAvailability, EmitsOnlyFromFinalProgram) {
  FAIL() << "W10 RED: private Compiler/FinalProgram.h and its verified "
            "C++/Verilog backend entry points are missing; legacy QueueGraph "
            "materialization is not an admissible substitute";
}
#endif

} // namespace
