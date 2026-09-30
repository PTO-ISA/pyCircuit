#ifndef ACIR_LIB_COMPILER_FINALCPPSOURCEPARTS_H
#define ACIR_LIB_COMPILER_FINALCPPSOURCEPARTS_H

#include "FinalProgram.h"
#include <string>
#include <vector>

namespace acir::compiler {

// Private in-memory backend result for verified source-owned final units.
// Header-only groups leave sourcePath/source empty. This is not the C3
// generated.json publication contract.
struct FinalCppSourceGroup {
  mlir::DictionaryAttr sourceOwner;
  std::string headerPath;
  std::string sourcePath;
  std::string header;
  std::string source;
};

struct FinalCppSourceParts {
  // Included as pycircuit_support.hpp and pycircuit_system.hpp respectively.
  std::string supportHeader;
  std::string systemHeader;
  std::vector<FinalCppSourceGroup> sourceGroups;
};

mlir::FailureOr<FinalCppSourceParts>
emitFinalCppSourceParts(const FinalProgram &program,
                        ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALCPPSOURCEPARTS_H
