#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTRECORDS_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTRECORDS_H

#include "PythonImportAST.h"
#include "SourceUnit.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/FunctionExtras.h"
#include <memory>

namespace acir::compiler::detail {

struct SourceTemplateBinding {
  mlir::StringAttr name;
  ac::MathIntAttr value;
};
struct SourceTemplateRequest {
  mlir::FlatSymbolRefAttr target;
  llvm::SmallVector<SourceTemplateBinding> bindings;
};
using SourceTemplateCalleeResolver =
    llvm::function_ref<mlir::FailureOr<ModuleInterfaceView>(
        const SourceTemplateRequest &)>;

mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
compileSourceTemplates(mlir::ModuleOp transport, mlir::DictionaryAttr owner,
                       const SourceHeaderRegistry &headers,
                       ac::detail::EmitError emitError);
mlir::FailureOr<mlir::DictionaryAttr>
describeSourceTemplate(mlir::ArrayAttr capture, mlir::DictionaryAttr owner,
                       mlir::FlatSymbolRefAttr target,
                       const SourceHeaderRegistry &headers,
                       ac::detail::EmitError emitError);
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
lowerBoundSourceTemplate(mlir::ArrayAttr capture, mlir::DictionaryAttr owner,
                         mlir::FlatSymbolRefAttr target,
                         llvm::ArrayRef<SourceTemplateBinding> bindings,
                         const SourceHeaderRegistry &headers,
                         SourceTemplateCalleeResolver resolveCallee,
                         ac::detail::EmitError emitError);
mlir::StringAttr sourceTemplateCaptureDigest(mlir::DictionaryAttr owner,
                                             mlir::ArrayAttr capture);

std::unique_ptr<mlir::Pass>
createLowerPythonSourcePass(mlir::DictionaryAttr owner,
                            const SourceHeaderRegistry &headers);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTRECORDS_H
