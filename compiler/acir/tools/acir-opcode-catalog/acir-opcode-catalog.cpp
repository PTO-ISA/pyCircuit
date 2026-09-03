#include "acir/CodeGen/QueueBlockContract.h"

#include "llvm/Support/raw_ostream.h"

#include <cstdlib>

int main() {
  auto catalog = acir::codegen::canonicalQueueBlockCatalogJson();
  if (!catalog) {
    llvm::errs() << llvm::toString(catalog.takeError()) << '\n';
    return EXIT_FAILURE;
  }
  llvm::outs() << *catalog << '\n';
  return EXIT_SUCCESS;
}
