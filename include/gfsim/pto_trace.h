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

/// Compact trace metadata returned by trace.decode.
///
/// Queues carry only an opaque record handle. The provider records trace facts
/// only; engine routing and timing are intentionally owned by the ACIR model:
///   [7:0]   sequence id
///   [10:8]  opcode class
///   [18:11] dependency 0 sequence id
///   [26:19] dependency 1 sequence id
///   [34:27] dependency 2 sequence id
///   [37:35] dependency-valid mask
///   [63:38] raw workload (bytes or MAC operations)
struct PtoScheduleDescriptor {
  static constexpr unsigned kSequenceShift = 0;
  static constexpr unsigned kOpcodeShift = 8;
  static constexpr unsigned kDependency0Shift = 11;
  static constexpr unsigned kDependency1Shift = 19;
  static constexpr unsigned kDependency2Shift = 27;
  static constexpr unsigned kDependencyValidShift = 35;
  static constexpr unsigned kWorkloadShift = 38;
  static constexpr uint64_t kMaxWorkload = (uint64_t{1} << 26) - 1;
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
