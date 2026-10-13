#pragma once
#include "llvm/Support/JSON.h"
#include "mlir/IR/Operation.h"
namespace acir {
llvm::json::Array componentContracts();
// Shared semantic interface for both lowering and exported analysis operations.
struct ComponentMethod {
  const char *operation;
  const char *interface;
  const char *view;
  const char *readiness;
};
ComponentMethod componentMethod(mlir::Operation *op);
}
