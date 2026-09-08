#ifndef GFSIM_TOOLING_TRACE_IO_H
#define GFSIM_TOOLING_TRACE_IO_H

#include "gfsim/trace.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gfsim {

struct TraceValidationLimits {
  size_t maxDocumentBytes = 1U << 20;
  size_t maxNestingDepth = 64;
  size_t maxStringBytes = 1U << 18;
  size_t maxRecordCount = 65536;
  size_t maxOperandsPerRecord = 1024;
  size_t maxDependenciesPerRecord = 1024;
  size_t maxAttributeMembers = 4096;
  size_t maxAggregateDecodedBytes = 1U << 24;
  size_t maxDiagnostics = 16;
};

struct TraceDiagnostic {
  std::string code;
  std::string jsonPointer;
  std::optional<uint64_t> sequenceId;
  std::string message;

  bool operator==(const TraceDiagnostic &) const = default;
};

struct TraceLoadResult {
  std::optional<PtoTraceDocument> document;
  std::vector<TraceDiagnostic> diagnostics;

  bool succeeded() const { return document.has_value() && diagnostics.empty(); }
  std::string primaryDiagnostic() const;
};

TraceLoadResult
parsePtoTrace(std::string_view input,
              const TraceValidationLimits &limits = TraceValidationLimits());

class PtoTraceStream {
public:
  explicit PtoTraceStream(
      TraceValidationLimits limits = TraceValidationLimits())
      : limits_(limits) {}

  bool append(std::string_view bytes);
  TraceLoadResult finish();

private:
  TraceValidationLimits limits_;
  std::string buffer_;
  bool finished_ = false;
  bool exceededByteLimit_ = false;
};

} // namespace gfsim

#endif // GFSIM_TOOLING_TRACE_IO_H
