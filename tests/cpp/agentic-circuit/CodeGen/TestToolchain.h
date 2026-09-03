#ifndef ACIR_UNITTESTS_CODEGEN_TESTTOOLCHAIN_H
#define ACIR_UNITTESTS_CODEGEN_TESTTOOLCHAIN_H

#include "llvm/ADT/StringRef.h"

#include <string>
#include <vector>

namespace acir::codegen::test {

inline std::vector<std::string> splitToolchainList(llvm::StringRef value) {
  std::vector<std::string> result;
  while (!value.empty()) {
    auto [item, remainder] = value.split('|');
    if (!item.empty())
      result.push_back(item.str());
    value = remainder;
  }
  return result;
}

inline std::vector<std::string> llvmIncludeDirectories() {
  return splitToolchainList(ACIR_TEST_LLVM_INCLUDE_DIRS);
}

inline std::vector<std::string> llvmLinkerFlags() {
  return splitToolchainList(ACIR_TEST_LLVM_LINK_FLAGS);
}

} // namespace acir::codegen::test

#endif // ACIR_UNITTESTS_CODEGEN_TESTTOOLCHAIN_H
