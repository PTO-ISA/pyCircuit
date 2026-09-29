#ifndef ACIR_BACKEND_PUBLICATION_H
#define ACIR_BACKEND_PUBLICATION_H
#include "llvm/ADT/ArrayRef.h"
#include <string>
namespace acir::testing {
struct Publication {
  std::string path;
  std::string contents;
  std::string temporary;
  std::string backup;
  bool installed = false;
};
bool publishAtomically(llvm::MutableArrayRef<Publication> publications);
} // namespace acir::testing
#endif
