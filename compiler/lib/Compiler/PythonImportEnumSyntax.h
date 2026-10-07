#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTENUMSYNTAX_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTENUMSYNTAX_H

#include "PythonImportAST.h"
#include "llvm/ADT/STLFunctionalExtras.h"

namespace acir::compiler::detail {

enum class MarkerKind {
  None,
  Enum,
  Auto,
  Encoding,
  EnumToBits,
  EnumFromBits,
  Module,
  System,
  Rule,
  Struct,
  Bits,
  Table,
  Queue,
  Concat,
  Popcount,
  CountLeadingZeros,
  CountTrailingZeros,
  PriorityEncode,
  OnehotEncode
};

/// Classify an actual import identity, never a local spelling or suffix.
MarkerKind classifyImportedMarker(llvm::StringRef module,
                                  llvm::StringRef remoteName);

struct ResolvedSourceBinding {
  MarkerKind marker = MarkerKind::None;
  mlir::FlatSymbolRefAttr nominalSymbol;
};

/// The importer owns lexical resolution, including namespaces and shadows.
/// This borrowed query carries no duplicate namespace or declaration state.
using LexicalBindings =
    llvm::function_ref<std::optional<ResolvedSourceBinding>(const AstNode &)>;

bool resolvesMarker(const AstNode &node, MarkerKind marker,
                    LexicalBindings bindings);
mlir::FailureOr<mlir::FlatSymbolRefAttr>
resolveNominalType(const AstNode &node, LexicalBindings bindings,
                   ac::detail::EmitError emitError);

struct CapturedEnumMemberSyntax {
  AstNode declaration;
  std::string name;
  AstNode value;
  bool automatic = false;
};

/// Syntax only: static evaluation and semantic authority remain in common IR.
struct CapturedEnumSyntax {
  AstNode declaration;
  AstNode width;
  std::string encoding;
  llvm::SmallVector<CapturedEnumMemberSyntax> members;
};

mlir::FailureOr<CapturedEnumSyntax>
readEnumSyntax(const AstNode &declaration, LexicalBindings bindings,
               ac::detail::EmitError emitError);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTENUMSYNTAX_H
