#ifndef ACIR_LIB_COMPILER_FINALVERILOGSOURCEPARTS_H
#define ACIR_LIB_COMPILER_FINALVERILOGSOURCEPARTS_H

#include "FinalProgram.h"

#include <string>

namespace acir::compiler {

struct FinalVerilogSourceGroup {
  mlir::DictionaryAttr sourceOwner;
  llvm::SmallVector<mlir::FlatSymbolRefAttr> definitions;
  std::string path;
  std::string moduleName;
  std::string text;
};

struct FinalVerilogSourceParts {
  llvm::SmallVector<FinalVerilogSourceGroup, 0> sourceGroups;
  std::string core;
  std::string runtimeGlue;
};

mlir::FailureOr<FinalVerilogSourceParts>
emitFinalVerilogSourceParts(const FinalProgram &program,
                            ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALVERILOGSOURCEPARTS_H
