#include "acir/CodeGen/EmitCxx.h"
#include "acir/Conversion/ACIRToACSim/ACIRToACSim.h"
#include "acir/Transforms/Passes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACSim/ACSimDialect.h"
#include "acir/Dialect/ACSim/ACSimOps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"

#include "gtest/gtest.h"

#include <fstream>
#include <iterator>
#include <optional>
#include <string>

namespace acir {
namespace {

llvm::StringRef kFrozen = R"mlir(
module attributes {ac.contract_epoch = "0.3"} {
  ac.system @soc root @Top as "root" tick 0 "cycle" workload @Top::@workload seed {kind = "fixed", value = 7 : i64} instrumentation [] results {format = "json", id = "default"} selected true
  ac.module @Child() parameters {} graph {
    ac.return
  }
  ac.module @Top() parameters {} graph {
    ac.instance @child of @Child() static {} id "child" path "child" : () -> ()
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}
)mlir";

class EmitCxxTest : public ::testing::Test {
protected:
  EmitCxxTest() {
    registry.insert<ac::ACIRDialect, acsim::ACSimDialect>();
    context.appendDialectRegistry(registry);
    context.loadAllAvailableDialects();
  }

  mlir::OwningOpRef<mlir::ModuleOp> lower() {
    auto module = mlir::parseSourceString<mlir::ModuleOp>(kFrozen, &context);
    if (!module)
      return nullptr;
    ACIRToACSimPassOptions options;
    options.profile = "fast";
    options.target = "x86_64-linux-gnu";
    mlir::PassManager manager(&context);
    manager.addPass(createFreezeTopologyPass());
    manager.addPass(createACIRToACSimPass(options));
    if (mlir::failed(manager.run(module.get())))
      return nullptr;
    return module;
  }

  mlir::DialectRegistry registry;
  mlir::MLIRContext context{mlir::MLIRContext::Threading::DISABLED};
};

TEST_F(EmitCxxTest, EmitsProcessThunksAndManifest) {
  auto module = lower();
  ASSERT_TRUE(module);

  llvm::SmallString<128> dir;
  ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("acsim-emit", dir));

  codegen::EmitCxxOptions options;
  options.outputDir = dir.str().str();
  options.profile = "fast";
  options.toolchainTarget = "x86_64-linux-gnu";
  auto manifest = codegen::emitCxxFile(module.get(), options);
  EXPECT_TRUE(mlir::succeeded(manifest));

  std::ifstream header((std::string(dir) + "/include/generated/model.h").c_str());
  std::ifstream source((std::string(dir) + "/src/generated/model.cpp").c_str());
  std::ifstream manifestFile((std::string(dir) + "/build-manifest.json").c_str());
  ASSERT_TRUE(header);
  ASSERT_TRUE(source);
  ASSERT_TRUE(manifestFile);
  std::string headerText((std::istreambuf_iterator<char>(header)), {});
  std::string sourceText((std::istreambuf_iterator<char>(source)), {});
  std::string manifestText((std::istreambuf_iterator<char>(manifestFile)), {});
  EXPECT_NE(headerText.find("struct Process"), std::string::npos);
  EXPECT_NE(headerText.find("thunkWork"), std::string::npos);
  EXPECT_NE(sourceText.find("setLegacyDispatchTable"), std::string::npos);
  EXPECT_NE(sourceText.find("scheduleWork"), std::string::npos);
  EXPECT_NE(manifestText.find("agentic-circuit-build-manifest"),
            std::string::npos);
  EXPECT_NE(manifestText.find("sha256:"), std::string::npos);

  llvm::sys::fs::remove_directories(dir);
}

TEST_F(EmitCxxTest, GeneratedSimulatorCompilesAndRuns) {
#ifndef ACIR_TEST_CXX_COMPILER
  GTEST_SKIP() << "no host C++ compiler configured";
#else
  auto module = lower();
  ASSERT_TRUE(module);

  llvm::SmallString<128> dir;
  ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("acsim-emit-run", dir));

  codegen::EmitCxxOptions options;
  options.outputDir = dir.str().str();
  options.profile = "fast";
  options.toolchainTarget = "x86_64-linux-gnu";
  ASSERT_TRUE(mlir::succeeded(codegen::emitCxxFile(module.get(), options)));

  llvm::SmallString<128> simPath = dir;
  llvm::sys::path::append(simPath, "sim");
  llvm::SmallString<128> modelCpp = dir;
  llvm::sys::path::append(modelCpp, "src", "generated", "model.cpp");
  llvm::SmallString<128> mainCpp = dir;
  llvm::sys::path::append(mainCpp, "src", "generated", "main.cpp");
  llvm::SmallString<128> includeDir = dir;
  llvm::sys::path::append(includeDir, "include");

  llvm::SmallVector<llvm::StringRef, 16> args = {
      ACIR_TEST_CXX_COMPILER,
      "-std=c++20",
      "-I",
      ACIR_GFSIM_INCLUDE,
      "-I",
      includeDir.c_str(),
      modelCpp.c_str(),
      mainCpp.c_str(),
      ACIR_GFSIM_LIBRARY,
      "-o",
      simPath.c_str(),
  };
  std::string error;
  int compile = llvm::sys::ExecuteAndWait(ACIR_TEST_CXX_COMPILER, args, {}, {},
                                          60, 0, &error);
  ASSERT_EQ(compile, 0) << error;

  llvm::SmallVector<llvm::StringRef, 2> runArgs = {simPath.c_str()};
  std::string runError;
  int run = llvm::sys::ExecuteAndWait(simPath, runArgs, {}, {}, 30, 0,
                                      &runError);
  EXPECT_EQ(run, 1) << "yield-only model should hit max_deltas and fail: "
                    << runError;

  llvm::sys::fs::remove_directories(dir);
#endif
}

llvm::StringRef kAdder = R"mlir(
builtin.module attributes {ac.contract_epoch = "0.3"} {
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload i32 action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
  }
  ac.module @Adder() parameters {} graph {
    ac.queue @op_a payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "op_a" path "op_a"
    ac.queue @op_b payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "op_b" path "op_b"
    ac.queue @result payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "result" path "result"
    ac.process @source kind "workload" {
      %two = arith.constant 2 : i32
      %three = arith.constant 3 : i32
      %ok_a = ac.try_send @op_a %two : i32
      %ok_b = ac.try_send @op_b %three : i32
      ac.yield_sim
    }
    ac.process @alu kind "control" {
      %a, %got_a = ac.try_recv @op_a : i32
      %b, %got_b = ac.try_recv @op_b : i32
      %both = arith.andi %got_a, %got_b : i1
      scf.if %both {
        %sum = arith.addi %a, %b : i32
        %ok = ac.try_send @result %sum : i32
      }
      ac.yield_sim
    }
    ac.process @sink kind "control" {
      %value, %got = ac.try_recv @result : i32
      scf.if %got {
        ac.assert %got, "sum"
      }
      ac.yield_sim
    }
    ac.return
  }
  ac.system @soc root @Adder as "root" tick 0 "cycle"
      workload @Adder::@source seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}
)mlir";

TEST(EmitAdder, GeneratedSimulatorCompletesWithSum) {
#ifndef ACIR_TEST_CXX_COMPILER
  GTEST_SKIP() << "no host C++ compiler configured";
#else
  mlir::DialectRegistry registry;
  registry.insert<ac::ACIRDialect, acsim::ACSimDialect, mlir::arith::ArithDialect,
                  mlir::scf::SCFDialect, mlir::cf::ControlFlowDialect>();
  mlir::MLIRContext context(registry, mlir::MLIRContext::Threading::DISABLED);
  context.loadAllAvailableDialects();

  auto module = mlir::parseSourceString<mlir::ModuleOp>(kAdder, &context);
  ASSERT_TRUE(module);

  mlir::PassManager freeze(&context);
  freeze.enableVerifier(false);
  freeze.addPass(createFreezeTopologyPass());
  ASSERT_TRUE(mlir::succeeded(freeze.run(module.get())));

  ACIRToACSimPassOptions lowerOptions;
  lowerOptions.profile = "fast";
  lowerOptions.target = "x86_64-linux-gnu";
  mlir::PassManager lower(&context);
  lower.addPass(createACIRToACSimPass(lowerOptions));
  ASSERT_TRUE(mlir::succeeded(lower.run(module.get())));

  llvm::SmallString<128> dir;
  ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("acsim-adder", dir));

  codegen::EmitCxxOptions options;
  options.outputDir = dir.str().str();
  options.profile = "fast";
  options.toolchainTarget = "x86_64-linux-gnu";
  ASSERT_TRUE(mlir::succeeded(codegen::emitCxxFile(module.get(), options)));

  llvm::SmallString<128> simPath = dir;
  llvm::sys::path::append(simPath, "sim");
  llvm::SmallString<128> modelCpp = dir;
  llvm::sys::path::append(modelCpp, "src", "generated", "model.cpp");
  llvm::SmallString<128> mainCpp = dir;
  llvm::sys::path::append(mainCpp, "src", "generated", "main.cpp");
  llvm::SmallString<128> includeDir = dir;
  llvm::sys::path::append(includeDir, "include");
  llvm::SmallString<128> stdoutPath = dir;
  llvm::sys::path::append(stdoutPath, "sim.out");

  llvm::SmallVector<llvm::StringRef, 16> args = {
      ACIR_TEST_CXX_COMPILER,
      "-std=c++20",
      "-I",
      ACIR_GFSIM_INCLUDE,
      "-I",
      includeDir.c_str(),
      modelCpp.c_str(),
      mainCpp.c_str(),
      ACIR_GFSIM_LIBRARY,
      "-o",
      simPath.c_str(),
  };
  std::string error;
  int compile = llvm::sys::ExecuteAndWait(ACIR_TEST_CXX_COMPILER, args, {}, {},
                                          60, 0, &error);
  ASSERT_EQ(compile, 0) << error;

  llvm::SmallVector<llvm::StringRef, 2> runArgs = {simPath.c_str()};
  std::optional<llvm::StringRef> redirects[3] = {
      std::nullopt, llvm::StringRef(stdoutPath), std::nullopt};
  std::string runError;
  int run = llvm::sys::ExecuteAndWait(simPath, runArgs, {}, redirects, 30, 0,
                                      &runError);
  std::ifstream out(std::string(stdoutPath).c_str());
  std::string output((std::istreambuf_iterator<char>(out)), {});
  EXPECT_EQ(run, 0) << runError << '\n' << output;
  EXPECT_NE(output.find("\"classification\":\"completed\""), std::string::npos)
      << output;
  EXPECT_NE(output.find("\"diagnostic\":\"sum=5\""), std::string::npos)
      << output;

  llvm::sys::fs::remove_directories(dir);
#endif
}

TEST(EmitRiscvMini, GeneratedSimulatorCompletesWithX3) {
#ifndef ACIR_TEST_CXX_COMPILER
  GTEST_SKIP() << "no host C++ compiler configured";
#else
  std::ifstream in(std::string(ACIR_EXAMPLES_DIR) + "/riscv-mini/model.mlir");
  ASSERT_TRUE(in) << "missing " << ACIR_EXAMPLES_DIR << "/riscv-mini/model.mlir";
  std::string source((std::istreambuf_iterator<char>(in)), {});

  mlir::DialectRegistry registry;
  registry.insert<ac::ACIRDialect, acsim::ACSimDialect, mlir::arith::ArithDialect,
                  mlir::scf::SCFDialect, mlir::cf::ControlFlowDialect>();
  mlir::MLIRContext context(registry, mlir::MLIRContext::Threading::DISABLED);
  context.loadAllAvailableDialects();

  auto module = mlir::parseSourceString<mlir::ModuleOp>(source, &context);
  ASSERT_TRUE(module);

  mlir::PassManager freeze(&context);
  freeze.enableVerifier(false);
  freeze.addPass(createFreezeTopologyPass());
  ASSERT_TRUE(mlir::succeeded(freeze.run(module.get())));

  ACIRToACSimPassOptions lowerOptions;
  lowerOptions.profile = "fast";
  lowerOptions.target = "x86_64-linux-gnu";
  mlir::PassManager lower(&context);
  lower.addPass(createACIRToACSimPass(lowerOptions));
  ASSERT_TRUE(mlir::succeeded(lower.run(module.get())));

  llvm::SmallString<128> dir;
  ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("acsim-riscv", dir));

  codegen::EmitCxxOptions options;
  options.outputDir = dir.str().str();
  options.profile = "fast";
  options.toolchainTarget = "x86_64-linux-gnu";
  ASSERT_TRUE(mlir::succeeded(codegen::emitCxxFile(module.get(), options)));

  llvm::SmallString<128> simPath = dir;
  llvm::sys::path::append(simPath, "sim");
  llvm::SmallString<128> modelCpp = dir;
  llvm::sys::path::append(modelCpp, "src", "generated", "model.cpp");
  llvm::SmallString<128> mainCpp = dir;
  llvm::sys::path::append(mainCpp, "src", "generated", "main.cpp");
  llvm::SmallString<128> includeDir = dir;
  llvm::sys::path::append(includeDir, "include");
  llvm::SmallString<128> stdoutPath = dir;
  llvm::sys::path::append(stdoutPath, "sim.out");

  llvm::SmallVector<llvm::StringRef, 16> args = {
      ACIR_TEST_CXX_COMPILER,
      "-std=c++20",
      "-I",
      ACIR_GFSIM_INCLUDE,
      "-I",
      includeDir.c_str(),
      modelCpp.c_str(),
      mainCpp.c_str(),
      ACIR_GFSIM_LIBRARY,
      "-o",
      simPath.c_str(),
  };
  std::string error;
  int compile = llvm::sys::ExecuteAndWait(ACIR_TEST_CXX_COMPILER, args, {}, {},
                                          60, 0, &error);
  ASSERT_EQ(compile, 0) << error;

  llvm::SmallVector<llvm::StringRef, 2> runArgs = {simPath.c_str()};
  std::optional<llvm::StringRef> redirects[3] = {
      std::nullopt, llvm::StringRef(stdoutPath), std::nullopt};
  std::string runError;
  int run = llvm::sys::ExecuteAndWait(simPath, runArgs, {}, redirects, 30, 0,
                                      &runError);
  std::ifstream out(std::string(stdoutPath).c_str());
  std::string output((std::istreambuf_iterator<char>(out)), {});
  EXPECT_EQ(run, 0) << runError << '\n' << output;
  EXPECT_NE(output.find("\"classification\":\"completed\""), std::string::npos)
      << output;
  EXPECT_NE(output.find("\"diagnostic\":\"x3=5\""), std::string::npos)
      << output;

  llvm::sys::fs::remove_directories(dir);
#endif
}

void expectGeneratedDiagnostic(const std::string &source,
                               const char *diagnostic,
                               const char *tracePath = nullptr,
                               unsigned runTimeoutSeconds = 30) {
#ifndef ACIR_TEST_CXX_COMPILER
  GTEST_SKIP() << "no host C++ compiler configured";
#else
  mlir::DialectRegistry registry;
  registry.insert<ac::ACIRDialect, acsim::ACSimDialect, mlir::arith::ArithDialect,
                  mlir::scf::SCFDialect, mlir::cf::ControlFlowDialect>();
  mlir::MLIRContext context(registry, mlir::MLIRContext::Threading::DISABLED);
  context.loadAllAvailableDialects();

  auto module = mlir::parseSourceString<mlir::ModuleOp>(source, &context);
  ASSERT_TRUE(module);

  mlir::PassManager freeze(&context);
  freeze.enableVerifier(false);
  freeze.addPass(createFreezeTopologyPass());
  ASSERT_TRUE(mlir::succeeded(freeze.run(module.get())));

  ACIRToACSimPassOptions lowerOptions;
  lowerOptions.profile = "fast";
  lowerOptions.target = "x86_64-linux-gnu";
  mlir::PassManager lower(&context);
  lower.addPass(createACIRToACSimPass(lowerOptions));
  ASSERT_TRUE(mlir::succeeded(lower.run(module.get())));

  llvm::SmallString<128> dir;
  ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("acsim-case", dir));

  codegen::EmitCxxOptions options;
  options.outputDir = dir.str().str();
  options.profile = "fast";
  options.toolchainTarget = "x86_64-linux-gnu";
  ASSERT_TRUE(mlir::succeeded(codegen::emitCxxFile(module.get(), options)));

  llvm::SmallString<128> simPath = dir;
  llvm::sys::path::append(simPath, "sim");
  llvm::SmallString<128> modelCpp = dir;
  llvm::sys::path::append(modelCpp, "src", "generated", "model.cpp");
  llvm::SmallString<128> mainCpp = dir;
  llvm::sys::path::append(mainCpp, "src", "generated", "main.cpp");
  llvm::SmallString<128> includeDir = dir;
  llvm::sys::path::append(includeDir, "include");
  llvm::SmallString<128> stdoutPath = dir;
  llvm::sys::path::append(stdoutPath, "sim.out");

  llvm::SmallVector<llvm::StringRef, 16> args = {
      ACIR_TEST_CXX_COMPILER,
      "-std=c++20",
      "-I",
      ACIR_GFSIM_INCLUDE,
      "-I",
      includeDir.c_str(),
      modelCpp.c_str(),
      mainCpp.c_str(),
      ACIR_GFSIM_LIBRARY,
      "-o",
      simPath.c_str(),
  };
  std::string error;
  int compile = llvm::sys::ExecuteAndWait(ACIR_TEST_CXX_COMPILER, args, {}, {},
                                          180, 0, &error);
  ASSERT_EQ(compile, 0) << error;

  llvm::SmallVector<llvm::StringRef, 4> runArgs = {simPath.c_str()};
  if (tracePath) {
    runArgs.push_back("--trace");
    runArgs.push_back(tracePath);
  }
  std::optional<llvm::StringRef> redirects[3] = {
      std::nullopt, llvm::StringRef(stdoutPath), std::nullopt};
  std::string runError;
  int run = llvm::sys::ExecuteAndWait(simPath, runArgs, {}, redirects,
                                      runTimeoutSeconds, 0, &runError);
  std::ifstream out(std::string(stdoutPath).c_str());
  std::string output((std::istreambuf_iterator<char>(out)), {});
  EXPECT_EQ(run, 0) << runError << '\n' << output;
  EXPECT_NE(output.find("\"classification\":\"completed\""), std::string::npos)
      << output;
  EXPECT_NE(output.find(diagnostic), std::string::npos) << output;

  llvm::sys::fs::remove_directories(dir);
#endif
}

std::string readExample(const char *relative) {
  std::ifstream in(std::string(ACIR_EXAMPLES_DIR) + "/" + relative);
  EXPECT_TRUE(in) << "missing " << ACIR_EXAMPLES_DIR << "/" << relative;
  return std::string((std::istreambuf_iterator<char>(in)), {});
}

TEST(EmitHandshake, CompletesAfterPcSwitch) {
  expectGeneratedDiagnostic(readExample("handshake/model.mlir"),
                            "\"diagnostic\":\"token=7\"");
}

TEST(EmitNestedParentQueue, ChildProcessesShareParentFifo) {
  expectGeneratedDiagnostic(readExample("nested-parent-queue/model.mlir"),
                            "\"diagnostic\":\"token=7\"");
}

TEST(EmitDavinciooMini, RetiresBoundedTraceInOrder) {
  expectGeneratedDiagnostic(readExample("davincioo-mini/model.mlir"),
                            "\"diagnostic\":\"retired=6\"");
}

TEST(EmitDavinciooMatmul, RetiresSyntheticTraceWithDependencies) {
  std::string trace =
      std::string(ACIR_EXAMPLES_DIR) + "/davincioo-matmul/synthetic.pto.trace";
  expectGeneratedDiagnostic(readExample("davincioo-matmul/model.mlir"),
                            "\"diagnostic\":\"retired=12\"", trace.c_str());
}

TEST(EmitDavinciooFa2, RetiresFlashAttentionTrace) {
  std::string trace =
      std::string(ACIR_EXAMPLES_DIR) + "/davincioo-fa2/fa2-b1-h1-s128-d64.pto.trace";
  expectGeneratedDiagnostic(readExample("davincioo-matmul/model.mlir"),
                            "\"diagnostic\":\"retired=192\"", trace.c_str(),
                            /*runTimeoutSeconds=*/180);
}

TEST(EmitQueueI64, CompletesWithWideSum) {
  expectGeneratedDiagnostic(readExample("queue-i64/model.mlir"),
                            "\"diagnostic\":\"sum=5\"");
}

TEST(EmitMulLatency, CompletesWithProduct) {
  expectGeneratedDiagnostic(readExample("mul-latency/model.mlir"),
                            "\"diagnostic\":\"product=21\"");
}

} // namespace
} // namespace acir
