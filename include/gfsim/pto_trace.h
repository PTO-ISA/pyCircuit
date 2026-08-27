#ifndef GFSIM_PTO_TRACE_H
#define GFSIM_PTO_TRACE_H

#include "gfsim/core.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace gfsim {

/// Compact scheduling metadata returned by trace.decode.
///
/// Queues carry only an opaque record handle.  The generated model decodes a
/// handle when it needs scheduling metadata:
///   [7:0]   sequence id
///   [9:8]   engine (scalar/vector/cube/tma)
///   [19:10] latency
///   [27:20] dependency 0 sequence id
///   [35:28] dependency 1 sequence id
///   [43:36] dependency 2 sequence id
///   [46:44] dependency-valid mask
///   [49:47] opcode class
struct PtoScheduleDescriptor {
  static constexpr unsigned kSequenceShift = 0;
  static constexpr unsigned kEngineShift = 8;
  static constexpr unsigned kLatencyShift = 10;
  static constexpr unsigned kDependency0Shift = 20;
  static constexpr unsigned kDependency1Shift = 28;
  static constexpr unsigned kDependency2Shift = 36;
  static constexpr unsigned kDependencyValidShift = 44;
  static constexpr unsigned kOpcodeShift = 47;
};

/// Clean-room PTO JSONL reader used by generated timing models.
///
/// This intentionally exposes records as integer handles.  It does not expose
/// or copy an upstream simulator's instruction representation.
class PtoTraceProvider {
public:
  void load(std::string source, const std::string &path);

  size_t open(std::string_view source) const;
  TraceNextResult next(std::string_view source, size_t cursor) const;
  bool eof(std::string_view source, size_t cursor) const;
  size_t position(std::string_view source, size_t cursor) const;
  uint64_t decode(uint64_t handle) const;
  size_t recordCount(std::string_view source) const;

private:
  struct Source {
    std::vector<uint64_t> descriptors;
  };

  const Source &requireSource(std::string_view source) const;

  std::map<std::string, Source, std::less<>> sources_;
  std::string activeSource_;
};

} // namespace gfsim

#endif // GFSIM_PTO_TRACE_H
