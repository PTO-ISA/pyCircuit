// Installed-tree consumer for the production ProcessStatePlan API.
#include "acir/Analysis/ProcessStatePlan.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"

int main() {
  mlir::DialectRegistry registry;
  registry.insert<acir::ac::ACIRDialect>();
  mlir::MLIRContext context(registry);
  context.loadAllAvailableDialects();

  auto module = mlir::parseSourceString<mlir::ModuleOp>(R"mlir(
module attributes {ac.contract_epoch = "0.4", ac.freeze_epoch = "0.4",
    ac.frozen_instrumentation = [],
    ac.frozen_owners = [
      {kind = "ac.system_root", owner = @Top, path = "root", stable_id = "root"},
      {kind = "ac.process", owner = @Top::@workload,
       path = "root.workload", stable_id = "root/workload"}],
    ac.frozen_primary_workload = {path = "root.workload",
      reference = @Top::@workload, stable_id = "root/workload"},
    ac.frozen_system = @soc,
    ac.topology_digest = "0000000000000000000000000000000000000000000000000000000000000000",
    ac.topology_frozen = true} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {format = "json", id = "default"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      ac.yield_sim
    } {ac.frozen_process_skeleton = ["process/r0/b0/o0 ac.yield_sim{}props=<<NULL ATTRIBUTE>> operands= results= regions="]}
    ac.return
  }
}
)mlir",
                                                        &context);
  if (!module)
    return 1;

  auto plans = acir::planProcessState(*module);
  if (mlir::failed(plans) || mlir::failed(acir::verifyProcessStatePlan(*plans)))
    return 2;
  auto serialized = acir::serializeProcessStatePlan(*plans);
  if (!serialized || serialized->empty())
    return 3;
  return plans->processes().size() == 1 ? 0 : 4;
}
