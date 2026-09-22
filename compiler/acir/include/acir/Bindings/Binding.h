#ifndef ACIR_BINDINGS_BINDING_H
#define ACIR_BINDINGS_BINDING_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/JSON.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace acir::bindings {

struct JsonParseLimits {
  size_t maxInputBytes = 1U << 20;
  // Canonicalization may expand a compact input (for example, a queue-graph
  // metadata DOM) while preserving the same structural limits.  Keep this
  // budget separate from the raw input cap so valid generated metadata is not
  // rejected solely because its canonical spelling is larger.
  size_t maxCanonicalBytes = 128U << 20;
  size_t maxDepth = 64;
  size_t maxStructuralWork = 10000000;
  size_t maxStringBytes = 1U << 18;
  // Large composed queue packages legitimately repeat long qualified type
  // names across their binding metadata. Keep the per-string guard tight,
  // while allowing the package-wide accounting budget to scale with a fused
  // bring-up graph.
  size_t maxTotalStringBytes = 128U << 20;
  size_t maxArrayElements = 65536;
  size_t maxObjectMembers = 4096;
};

/// Parses the accepted RFC 8785/I-JSON subset while retaining lexical checks
/// that ordinary JSON DOM parsers lose (duplicate keys and negative zero).
llvm::Expected<llvm::json::Value>
parseIJson(llvm::StringRef input,
           const JsonParseLimits &limits = JsonParseLimits());

/// Emits RFC 8785 canonical UTF-8 bytes. Object names are ordered by unsigned
/// UTF-16 code units recursively; arrays retain their input order. Constructed
/// DOMs are preflighted against the same deterministic resource limits as text.
llvm::Expected<std::string>
canonicalizeJson(const llvm::json::Value &value,
                 const JsonParseLimits &limits = JsonParseLimits());
llvm::Expected<std::string>
canonicalizeJsonText(llvm::StringRef input,
                     const JsonParseLimits &limits = JsonParseLimits());

} // namespace acir::bindings

#endif // ACIR_BINDINGS_BINDING_H
