// gfir_core_only SDK consumer (B2 step-2 design r9 §4): links exactly
// pycircuit::GFIRDialect and proves symbol-registration direction. Typed
// !ac.* construction is NOT claimed here: loading the ACIR dialect for
// typed parsing requires the operations layer by design (r9 concession).
#include "pycircuit/Dialect/GFIR/GFIRDialect.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"
#include <cstdio>

int main() {
  mlir::DialectRegistry registry;
  registry.insert<acir::gfir::GFIRDialect>();
  mlir::MLIRContext context(registry);
  auto *dialect = context.getOrLoadDialect<acir::gfir::GFIRDialect>();
  if (!dialect || dialect->getNamespace() != "gfir")
    return 1;
  std::puts("gfir_core_only OK");
  return 0;
}
