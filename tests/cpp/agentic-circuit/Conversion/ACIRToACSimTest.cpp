#include "acir/Conversion/ACIRToACSim/ACIRToACSim.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACSim/ACSimDialect.h"
#include "acir/Dialect/ACSim/ACSimOps.h"
#include "acir/Transforms/Passes.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"

#include "gtest/gtest.h"

namespace acir {
namespace {

llvm::StringRef kFrozenTwoRowModule = R"mlir(
module attributes {ac.contract_epoch = "0.4", ac.freeze_epoch = "0.4", ac.frozen_instrumentation = [], ac.frozen_owners = [{kind = "ac.system_root", owner = @Top, path = "root", stable_id = "root"}, {kind = "ac.instance", owner = @Top::@child, path = "root.child", stable_id = "root/child"}, {kind = "ac.process", owner = @Top::@workload, path = "root.workload", stable_id = "root/workload"}], ac.frozen_primary_workload = {path = "root.workload", reference = @Top::@workload, stable_id = "root/workload"}, ac.frozen_system = @soc, ac.topology_digest = "25e91b3b7250eab37ec78329957fda6232c5226b5599eaedd704727a214a7ea6", ac.topology_frozen = true} {
  ac.system @soc root @Top as "root" tick 0 "cycle" workload @Top::@workload seed {kind = "fixed", value = 7 : i64} instrumentation [] results {format = "json", id = "default"} selected true
  ac.module @Child() parameters {} graph {
    ac.return
  }
  ac.module @Top() parameters {} graph {
    ac.instance @child of @Child() static {} id "child" path "child" {ac.frozen_owners = [{kind = "ac.instance", owner = @Top::@child, path = "root.child", stable_id = "root/child"}]} : () -> ()
    ac.process @workload kind "workload" {
      ac.yield_sim
    } {ac.frozen_owners = [{kind = "ac.process", owner = @Top::@workload, path = "root.workload", stable_id = "root/workload"}], ac.frozen_process_skeleton = ["process/r0/b0/o0 ac.yield_sim{}props=<<NULL ATTRIBUTE>> operands= results= regions="]}
    ac.return
  }
}
)mlir";

llvm::StringRef kAdversarialModuleOrder = R"mlir(
module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @A as "root" tick 0 "cycle" workload @A::@workload seed {kind = "fixed", value = 7 : i64} instrumentation [] results {format = "json", id = "default"} selected true
  ac.module @A() parameters {} graph {
    ac.instance @child of @Z() static {} id "child" path "child" : () -> ()
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
  ac.module @Z() parameters {} graph { ac.return }
}
)mlir";

llvm::StringRef kCyclicModuleOrder = R"mlir(
module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @A as "root" tick 0 "cycle" workload @A::@workload seed {kind = "fixed", value = 7 : i64} instrumentation [] results {format = "json", id = "default"} selected true
  ac.module @A() parameters {} graph {
    ac.instance @b of @B() static {} id "b" path "b" : () -> ()
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
  ac.module @B() parameters {} graph {
    ac.instance @a of @A() static {} id "a" path "a" : () -> ()
    ac.return
  }
}
)mlir";

llvm::StringRef kAdversarialModuleOrderRenamedPlacement = R"mlir(
module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @A as "root" tick 0 "cycle" workload @A::@workload seed {kind = "fixed", value = 7 : i64} instrumentation [] results {format = "json", id = "default"} selected true
  ac.module @A() parameters {} graph {
    ac.instance @offspring of @Z() static {} id "offspring" path "offspring" : () -> ()
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
  ac.module @Z() parameters {} graph { ac.return }
}
)mlir";

llvm::StringRef kInvalidGeneratedModuleName = R"mlir(
module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @"bad-name" as "root" tick 0 "cycle"
      workload @"bad-name"::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {format = "json", id = "default"} selected true
  ac.module @"bad-name"() parameters {} graph {
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
}
)mlir";

class ACIRToACSimTest : public ::testing::Test {
protected:
  ACIRToACSimTest() {
    registry.insert<ac::ACIRDialect, acsim::ACSimDialect>();
    context.appendDialectRegistry(registry);
    context.loadAllAvailableDialects();
  }

  mlir::OwningOpRef<mlir::ModuleOp> parseFrozen() {
    return mlir::parseSourceString<mlir::ModuleOp>(kFrozenTwoRowModule,
                                                   &context);
  }

  mlir::DialectRegistry registry;
  mlir::MLIRContext context{mlir::MLIRContext::Threading::DISABLED};
};

TEST_F(ACIRToACSimTest, DefaultBoundLowersTwoRowModel) {
  auto module = parseFrozen();
  ASSERT_TRUE(module);
  ACIRToACSimPassOptions options;
  options.profile = "fast";
  options.target = "arm64-apple-darwin";
  mlir::PassManager manager(&context);
  manager.addPass(createACIRToACSimPass(options));
  EXPECT_TRUE(mlir::succeeded(manager.run(module.get())));
  auto model = mlir::dyn_cast<acsim::ModelOp>(module->getBody()->front());
  ASSERT_TRUE(model);
  ASSERT_EQ(model.getConstructionOrder().size(), 2u);
  EXPECT_EQ(
      mlir::cast<mlir::StringAttr>(model.getConstructionOrder()[0]).getValue(),
      "root.child");
  EXPECT_EQ(
      mlir::cast<mlir::StringAttr>(model.getConstructionOrder()[1]).getValue(),
      "root.workload");
  for (acsim::DispatchOp dispatch : model.getOps<acsim::DispatchOp>())
    EXPECT_TRUE(dispatch.getPath().starts_with("root."));
}

TEST_F(ACIRToACSimTest, CapabilityBoundOverflowIsAtomicDispatchFailure) {
  auto module = parseFrozen();
  ASSERT_TRUE(module);
  ACIRToACSimPassOptions options;
  options.profile = "fast";
  options.target = "arm64-apple-darwin";
  // The frozen model expands to two construction rows; a one-row bound must
  // trip the ACLOWER-DISPATCH capability diagnostic before any emission.
  options.maxExpandedRows = 1;

  std::string diagnostic;
  mlir::ScopedDiagnosticHandler handler(&context, [&](mlir::Diagnostic &diag) {
    if (diag.getSeverity() == mlir::DiagnosticSeverity::Error) {
      diagnostic += diag.str();
      diagnostic += '\n';
    }
    return mlir::success();
  });
  mlir::PassManager manager(&context);
  manager.addPass(createACIRToACSimPass(options));
  EXPECT_TRUE(mlir::failed(manager.run(module.get())));
  EXPECT_NE(diagnostic.find("ACLOWER-DISPATCH"), std::string::npos)
      << diagnostic;
  // Atomicity: the frozen ACIR is untouched, no partial ACSim leaked out.
  EXPECT_TRUE(mlir::isa<ac::SystemOp>(module->getBody()->front()));
  EXPECT_TRUE(module->getBody()->getOps<acsim::ModelOp>().empty());
}

TEST_F(ACIRToACSimTest, CanonicalVerificationFailureDoesNotPublishACSim) {
  auto module = mlir::parseSourceString<mlir::ModuleOp>(
      kInvalidGeneratedModuleName, &context);
  ASSERT_TRUE(module);
  mlir::PassManager freezer(&context);
  freezer.addPass(createFreezeTopologyPass());
  ASSERT_TRUE(mlir::succeeded(freezer.run(module.get())));

  ACIRToACSimPassOptions options;
  options.profile = "fast";
  options.target = "arm64-apple-darwin";
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler handler(&context, [&](mlir::Diagnostic &diag) {
    if (diag.getSeverity() == mlir::DiagnosticSeverity::Error)
      diagnostic += diag.str();
    return mlir::success();
  });
  mlir::PassManager lowerer(&context);
  lowerer.addPass(createACIRToACSimPass(options));
  EXPECT_TRUE(mlir::failed(lowerer.run(module.get())));
  EXPECT_NE(diagnostic.find("canonical C++ identifier"), std::string::npos)
      << diagnostic;
  EXPECT_FALSE(module->getBody()->getOps<ac::SystemOp>().empty());
  EXPECT_TRUE(module->getBody()->getOps<acsim::ModelOp>().empty());
  EXPECT_TRUE(module->getOperation()->hasAttr("ac.topology_frozen"));
}

TEST_F(ACIRToACSimTest, ModuleReferencesDoNotDependOnSymbolOrder) {
  auto module = mlir::parseSourceString<mlir::ModuleOp>(kAdversarialModuleOrder,
                                                        &context);
  ASSERT_TRUE(module);
  ACIRToACSimPassOptions options;
  options.profile = "fast";
  options.target = "arm64-apple-darwin";
  mlir::PassManager freezer(&context);
  freezer.addPass(createFreezeTopologyPass());
  ASSERT_TRUE(mlir::succeeded(freezer.run(module.get())));
  mlir::PassManager manager(&context);
  manager.addPass(createACIRToACSimPass(options));
  ASSERT_TRUE(mlir::succeeded(manager.run(module.get())));
  auto model = mlir::cast<acsim::ModelOp>(module->getBody()->front());
  llvm::SmallVector<llvm::StringRef> names;
  for (acsim::ModuleOp realized : model.getOps<acsim::ModuleOp>())
    names.push_back(realized.getSymName());
  EXPECT_EQ(names, (llvm::SmallVector<llvm::StringRef>{"Z", "A"}));
}

TEST_F(ACIRToACSimTest, ModuleInstantiationCycleHasOwnershipDiagnostic) {
  auto module =
      mlir::parseSourceString<mlir::ModuleOp>(kCyclicModuleOrder, &context);
  ASSERT_TRUE(module);
  ACIRToACSimPassOptions options;
  options.profile = "fast";
  options.target = "arm64-apple-darwin";
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler handler(&context, [&](mlir::Diagnostic &diag) {
    if (diag.getSeverity() == mlir::DiagnosticSeverity::Error)
      diagnostic += diag.str();
    return mlir::success();
  });
  mlir::PassManager manager(&context);
  manager.addPass(createFreezeTopologyPass());
  manager.addPass(createACIRToACSimPass(options));
  EXPECT_TRUE(mlir::failed(manager.run(module.get())));
  EXPECT_NE(diagnostic.find("cycle"), std::string::npos) << diagnostic;
}

TEST_F(ACIRToACSimTest, ModuleFingerprintIncludesDefiningTopology) {
  auto fingerprint = [&](llvm::StringRef source) {
    auto module = mlir::parseSourceString<mlir::ModuleOp>(source, &context);
    EXPECT_TRUE(module);
    mlir::PassManager freezer(&context);
    freezer.addPass(createFreezeTopologyPass());
    EXPECT_TRUE(mlir::succeeded(freezer.run(module.get())));
    ACIRToACSimPassOptions options;
    options.profile = "fast";
    options.target = "arm64-apple-darwin";
    mlir::PassManager lowerer(&context);
    lowerer.addPass(createACIRToACSimPass(options));
    EXPECT_TRUE(mlir::succeeded(lowerer.run(module.get())));
    auto model = mlir::cast<acsim::ModelOp>(module->getBody()->front());
    for (acsim::ModuleOp realized : model.getOps<acsim::ModuleOp>())
      if (realized.getSymName() == "A")
        return realized.getSpecializationFingerprint().str();
    return std::string();
  };
  std::string child = fingerprint(kAdversarialModuleOrder);
  std::string offspring = fingerprint(kAdversarialModuleOrderRenamedPlacement);
  ASSERT_FALSE(child.empty());
  ASSERT_FALSE(offspring.empty());
  EXPECT_NE(child, offspring);
}

} // namespace
} // namespace acir
