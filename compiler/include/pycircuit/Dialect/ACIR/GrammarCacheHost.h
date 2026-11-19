#ifndef PYCIRCUIT_DIALECT_ACIR_GRAMMARCACHEHOST_H
#define PYCIRCUIT_DIALECT_ACIR_GRAMMARCACHEHOST_H

#include "mlir/IR/DialectInterface.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "llvm/ADT/DenseMap.h"
#include <algorithm>
#include <cstdint>
#include <mutex>

namespace acir::ac {
namespace detail {

struct RawAttributeShape {
  static constexpr unsigned maxHeight = 128;
  static constexpr uint32_t maxExpandedWork = 1024 * 1024;

  // Zero means that grammar succeeded without a reusable bounded shape.
  uint32_t expandedWork = 0;
  unsigned height = 0;

  explicit operator bool() const { return expandedWork != 0; }

  static RawAttributeShape leaf(uint64_t storage = 0) {
    return storage < maxExpandedWork
               ? RawAttributeShape{uint32_t(1 + storage), 0}
               : RawAttributeShape{};
  }

  void include(RawAttributeShape child) {
    if (!*this || !child || child.height >= maxHeight ||
        child.expandedWork > maxExpandedWork - expandedWork) {
      *this = {};
      return;
    }
    expandedWork += child.expandedWork;
    height = std::max(height, child.height + 1);
  }
};

using VerifiedGrammarMemo = llvm::DenseMap<mlir::DictionaryAttr, RawAttributeShape>;

} // namespace detail

// Shared grammar-memo storage (B2 step-2 design r9 §GrammarCacheHost).
// Only grammar successes and optional immutable shapes are retained.
// Owner/source checks stay with callers. The mutex guards all three maps.
struct GrammarMemoCache {
  detail::VerifiedGrammarMemo verifiedOccurrenceGrammars;
  detail::VerifiedGrammarMemo verifiedStaticExpressionGrammars;
  detail::VerifiedGrammarMemo verifiedSourceSpanGrammars;
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
