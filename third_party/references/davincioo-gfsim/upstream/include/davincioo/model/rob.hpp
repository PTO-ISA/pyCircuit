#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "davincioo/model/config.hpp"
#include "davincioo/model/pto_inst.hpp"

namespace davincioo {

enum class ROBEntryStatus {
  Free,
  Alloc,
  Issued,
  Completed,
  Resolved,
  Exception,
};

struct ROBEntry {
  std::uint64_t rid = 0;
  ROBEntryStatus status = ROBEntryStatus::Free;
  PTOInstRef inst;
};

constexpr std::string_view ToString(ROBEntryStatus status) {
  switch (status) {
    case ROBEntryStatus::Free:
      return "free";
    case ROBEntryStatus::Alloc:
      return "alloc";
    case ROBEntryStatus::Issued:
      return "issued";
    case ROBEntryStatus::Completed:
      return "completed";
    case ROBEntryStatus::Resolved:
      return "resolved";
    case ROBEntryStatus::Exception:
      return "exception";
    default:
      return "unknown";
  }
}

}  // namespace davincioo
