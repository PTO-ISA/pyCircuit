#ifndef PYCIRCUIT_DIALECT_ACIR_GRAMMARCACHEHOST_H
#define PYCIRCUIT_DIALECT_ACIR_GRAMMARCACHEHOST_H

#include "mlir/IR/DialectInterface.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "llvm/ADT/DenseSet.h"
#include <mutex>

namespace acir::ac {

// Shared grammar-memo storage (B2 step-2 design r9 §GrammarCacheHost).
// The three sets retain only grammar successes; owner/source checks stay
// with callers. The mutex guards all three sets.
struct GrammarMemoCache {
  llvm::DenseSet<mlir::DictionaryAttr> verifiedOccurrenceGrammars;
  llvm::DenseSet<mlir::DictionaryAttr> verifiedStaticExpressionGrammars;
  llvm::DenseSet<mlir::DictionaryAttr> verifiedSourceSpanGrammars;
  std::mutex mutex;
};

// Dialect-interface host for the grammar memos, replacing the retired
// Dialect-class members. The interface identity is explicitly emitted in
// this header's home target (core); the ACIR dialect registers the model
// that owns the storage, so the cache keeps exactly the dialect's (=
// context's) lifetime. Consumers reach it by dialect name without any
// typed dialect lookup:
//   dialect->getRegisteredInterface<GrammarCacheHost>()->cache()
struct GrammarCacheHost
    : public mlir::DialectInterface::Base<GrammarCacheHost> {
  using Base::Base;
  virtual GrammarMemoCache &cache() = 0;
};

} // namespace acir::ac

MLIR_DECLARE_EXPLICIT_TYPE_ID(::acir::ac::GrammarCacheHost)

#endif // PYCIRCUIT_DIALECT_ACIR_GRAMMARCACHEHOST_H
