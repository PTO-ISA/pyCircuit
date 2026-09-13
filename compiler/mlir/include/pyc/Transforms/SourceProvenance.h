#pragma once

#include "mlir/IR/PatternMatch.h"

namespace pyc {

::mlir::Location mergeSourceLocations(::mlir::Location retained,
                                      ::mlir::Location removed);

/// Preserves every source origin when canonicalization or CSE replaces an
/// operation with an existing value.
class SourceLocationRewriteListener final
    : public ::mlir::RewriterBase::Listener {
public:
  void notifyOperationReplaced(
      ::mlir::Operation *operation,
      ::mlir::ValueRange replacements) override;
};

} // namespace pyc
