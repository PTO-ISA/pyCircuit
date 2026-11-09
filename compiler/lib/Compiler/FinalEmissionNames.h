#ifndef ACIR_LIB_COMPILER_FINALEMISSIONNAMES_H
#define ACIR_LIB_COMPILER_FINALEMISSIONNAMES_H

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/JSON.h"

#include <string>

namespace acir::compiler {

struct CppOwnerComponents {
  std::string moduleName;
  llvm::SmallVector<std::string> namespaces;
  llvm::SmallVector<std::string> filePath;
};

mlir::FailureOr<CppOwnerComponents>
sourceOwnerComponents(mlir::DictionaryAttr owner,
                      ac::detail::EmitError emitError);
mlir::FailureOr<std::string>
legalizeIdentifier(llvm::StringRef source, ac::detail::EmitError emitError);
mlir::FailureOr<std::string>
sourceDefinitionName(mlir::FlatSymbolRefAttr definition,
                     llvm::StringRef moduleName,
                     ac::detail::EmitError emitError);
mlir::FailureOr<std::string>
namespaceCppName(llvm::ArrayRef<std::string> components,
                 ac::detail::EmitError emitError);
mlir::FailureOr<std::string>
sourcePathName(llvm::ArrayRef<std::string> components,
               llvm::StringRef extension, ac::detail::EmitError emitError);
std::string join(llvm::ArrayRef<std::string> parts, llvm::StringRef separator);
mlir::FailureOr<std::string> emissionMetadataJson(mlir::Attribute value,
                                                  mlir::Operation *site);
mlir::FailureOr<std::string>
observationSiteMetadataJson(ac::SourceObserveOp observe);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALEMISSIONNAMES_H
