#ifndef ACIR_LIB_COMPILER_FINALSOURCEMAP_H
#define ACIR_LIB_COMPILER_FINALSOURCEMAP_H

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/IR/BuiltinOps.h"

#include <string>

namespace acir::compiler {

// Native files emitted for one final SourceOwner. Empty generatedFiles is
// valid for a final declaration unit with no RTL source file.
struct FinalSourceMembership {
  mlir::DictionaryAttr sourceOwner;
  llvm::SmallVector<std::string> generatedFiles;
};

struct FinalSourceMap {
  mlir::DictionaryAttr sourceOwner;
  std::string path;
  std::string text;
  llvm::SmallVector<std::string> generatedFiles;
};

// Derive one provenance inventory per verified final unit. Membership comes
// from the selected native emitter, never from generated text or filenames.
mlir::FailureOr<llvm::SmallVector<FinalSourceMap>>
emitFinalSourceMaps(mlir::ModuleOp package,
                    llvm::ArrayRef<FinalSourceMembership> membership,
                    ac::detail::EmitError emitError);

// Validate a saved map's exact native schema and canonical MLIR payloads.
// Historical source authenticity and external file ownership are out of scope.
mlir::LogicalResult
validateFinalSourceMapText(llvm::StringRef text, mlir::MLIRContext &context,
                           ac::detail::EmitError emitError,
                           std::string *canonicalPath = nullptr);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALSOURCEMAP_H
