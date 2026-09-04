#ifndef ACIR_UNITTESTS_ANALYSIS_PROCESSSTATEPLANTESTSUPPORT_H
#define ACIR_UNITTESTS_ANALYSIS_PROCESSSTATEPLANTESTSUPPORT_H

#include "Analysis/ProcessStatePlanInternal.h"
#include "Analysis/ProcessStatePlanTestHooks.h"
#include "acir/InitAllDialects.h"
#include "acir/Transforms/Passes.h"

#include "mlir/Bytecode/BytecodeWriter.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace acir::test {

inline mlir::OwningOpRef<mlir::ModuleOp>
parseAndFreezeYieldOnly(mlir::MLIRContext &context) {
  constexpr llvm::StringLiteral source = R"mlir(
    builtin.module attributes {ac.contract_epoch = "0.5"} {
      ac.system @soc root @Top as "root" tick 0 "cycle"
          workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
          instrumentation [] results {id = "default", format = "json"}
          selected true
      ac.module @Top() parameters {} graph {
        ac.process @workload kind "workload" { ac.yield_sim }
        ac.return
      }
    }
  )mlir";
  auto module = mlir::parseSourceString<mlir::ModuleOp>(source, &context);
  if (!module)
    return {};
  mlir::PassManager manager(&context);
  manager.addPass(createFreezeTopologyPass());
  if (mlir::failed(manager.run(*module)))
    return {};
  return module;
}

inline mlir::OwningOpRef<mlir::ModuleOp>
parseAndFreezeLoopActions(mlir::MLIRContext &context) {
  constexpr llvm::StringLiteral source = R"mlir(
    builtin.module attributes {ac.contract_epoch = "0.5"} {
      ac.system @soc root @Top as "root" tick 0 "cycle"
          workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
          instrumentation [] results {id = "default", format = "json"}
          selected true
      ac.module @Top() parameters {} graph {
        ac.process @workload kind "workload" {
          %lb = arith.constant 0 : index
          %ub = arith.constant 4 : index
          %step = arith.constant 1 : index
          scf.for %i = %lb to %ub step %step {
            scf.yield
          }
          scf.for %i = %lb to %ub step %step {
            scf.yield
          }
          ac.yield_sim
        }
        ac.return
      }
    }
  )mlir";
  auto module = mlir::parseSourceString<mlir::ModuleOp>(source, &context);
  if (!module)
    return {};
  mlir::PassManager manager(&context);
  manager.addPass(createFreezeTopologyPass());
  if (mlir::failed(manager.run(*module)))
    return {};
  return module;
}

inline std::string moduleText(mlir::ModuleOp module) {
  std::string storage;
  llvm::raw_string_ostream stream(storage);
  module.print(stream);
  return storage;
}

inline std::string moduleBytecode(mlir::ModuleOp module) {
  std::string storage;
  llvm::raw_string_ostream stream(storage);
  if (mlir::failed(mlir::writeBytecodeToFile(module, stream)))
    return {};
  return storage;
}

inline std::string moduleFreezeSeal(mlir::ModuleOp module) {
  mlir::Attribute digest = module->getAttr("ac.topology_digest");
  if (!digest)
    return {};
  std::string storage;
  llvm::raw_string_ostream stream(storage);
  digest.print(stream);
  return storage;
}

inline mlir::OwningOpRef<mlir::ModuleOp>
parseAndFreezeYieldPermutation(mlir::MLIRContext &context,
                               bool reverseDeclarations) {
  constexpr llvm::StringLiteral alphaFirst = R"mlir(
    builtin.module attributes {ac.contract_epoch = "0.5"} {
      ac.system @soc root @Top as "root" tick 0 "cycle"
          workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
          instrumentation [] results {id = "default", format = "json"}
          selected true
      ac.module @Top() parameters {} graph {
        ac.process @alpha kind "control" { ac.yield_sim }
        ac.process @workload kind "workload" { ac.yield_sim }
        ac.return
      }
    }
  )mlir";
  constexpr llvm::StringLiteral workloadFirst = R"mlir(
    builtin.module attributes {ac.contract_epoch = "0.5"} {
      ac.system @soc root @Top as "root" tick 0 "cycle"
          workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
          instrumentation [] results {id = "default", format = "json"}
          selected true
      ac.module @Top() parameters {} graph {
        ac.process @workload kind "workload" { ac.yield_sim }
        ac.process @alpha kind "control" { ac.yield_sim }
        ac.return
      }
    }
  )mlir";
  auto module = mlir::parseSourceString<mlir::ModuleOp>(
      reverseDeclarations ? workloadFirst : alphaFirst, &context);
  if (!module)
    return {};
  mlir::PassManager manager(&context);
  manager.addPass(createFreezeTopologyPass());
  if (mlir::failed(manager.run(*module)))
    return {};
  return module;
}

inline mlir::OwningOpRef<mlir::ModuleOp>
parseEmptyModel(mlir::MLIRContext &context) {
  return mlir::parseSourceString<mlir::ModuleOp>(
      "builtin.module attributes {ac.contract_epoch = \"0.5\"} {}", &context);
}

inline std::string takeError(llvm::Error error) {
  std::string message;
  llvm::handleAllErrors(std::move(error), [&](const llvm::ErrorInfoBase &info) {
    message = info.message();
  });
  return message;
}

inline std::string verifyDiagnostic(const ProcessStatePlanSet &plans) {
  std::string diagnostic;
  mlir::MLIRContext *context = nullptr;
  if (!plans.processes().empty())
    context = plans.processes().front().process().getContext();
  if (!context)
    return mlir::succeeded(verifyProcessStatePlan(plans))
               ? std::string()
               : "verification failed";
  mlir::ScopedDiagnosticHandler handler(context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream(diagnostic) << value;
    return mlir::success();
  });
  if (mlir::succeeded(verifyProcessStatePlan(plans)))
    return {};
  return diagnostic;
}

} // namespace acir::test

#endif
