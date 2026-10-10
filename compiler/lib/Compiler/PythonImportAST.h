#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTAST_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTAST_H

#include "Dialect/ACIR/ACIRSourceContracts.h"

#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace acir::compiler::detail {

struct AstStep {
  std::string field;
  std::optional<uint64_t> index;
};

struct AstNode {
  mlir::DictionaryAttr value;
  llvm::SmallVector<AstStep> path;

  explicit operator bool() const { return static_cast<bool>(value); }
  llvm::StringRef kind() const;
  mlir::DictionaryAttr fields() const;
  mlir::Attribute get(llvm::StringRef name) const;
  AstNode child(llvm::StringRef name) const;
  mlir::ArrayAttr array(llvm::StringRef name) const;
  AstNode item(llvm::StringRef name, size_t index) const;
  llvm::StringRef string(llvm::StringRef name) const;
  mlir::Location location(mlir::MLIRContext *context,
                          llvm::StringRef sourcePath) const;
};

struct CapturedSource {
  std::string path;
  AstNode module;
  // Scope-local scans depend only on the immutable AST and an owned name key.
  mutable llvm::DenseMap<mlir::DictionaryAttr, llvm::StringMap<bool>>
      scopeBindingCache{};
};

bool sourceBindingShadowed(const CapturedSource &source, const AstNode &node,
                           llvm::StringRef name);

mlir::LogicalResult validateTableQueryLambda(const AstNode &node,
                                             ac::detail::EmitError emitError,
                                             std::optional<size_t> arity = 1);

mlir::FailureOr<CapturedSource>
readSingleCapture(mlir::ModuleOp transport, ac::detail::EmitError emitError);

struct LiteralForSyntax {
  AstNode target;
  AstNode bound;
  llvm::StringRef spelling;
};
mlir::FailureOr<LiteralForSyntax>
readLiteralForSyntax(const AstNode &node, ac::detail::EmitError emitError);

mlir::DictionaryAttr sourceSpan(mlir::OpBuilder &builder,
                                llvm::StringRef sourcePath,
                                const AstNode &node);
mlir::DictionaryAttr
occurrence(mlir::OpBuilder &builder, mlir::FlatSymbolRefAttr definition,
           const AstNode &node,
           llvm::ArrayRef<mlir::DictionaryAttr> expansion = {});

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTAST_H
