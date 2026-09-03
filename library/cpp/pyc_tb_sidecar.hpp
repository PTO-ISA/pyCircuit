#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <cpp/pyc_tb_sidecar_runtime.hpp>

namespace pyc::cpp {

constexpr std::uint16_t kSidecarSectionStringTable = 1;
constexpr std::uint16_t kSidecarSectionPortTable = 2;
constexpr std::uint16_t kSidecarSectionEventTable = 3;
constexpr std::uint16_t kSidecarSectionFrameTable = 4;
constexpr std::uint16_t kSidecarSectionPatternTable = 5;
constexpr std::uint32_t kSidecarNone = 0xffffffffu;

struct SidecarSectionInfo {
  std::uint16_t kind = 0;
  std::uint16_t flags = 0;
  std::uint64_t offset = 0;
  std::uint64_t size = 0;
  std::uint64_t count = 0;
};

struct SidecarPort {
  std::uint32_t id = 0;
  std::string name;
  std::uint8_t direction = 0;
  std::uint8_t role = 0;
  std::uint32_t bit_width = 0;
  std::uint32_t word_count = 0;
  std::string protocol;
};

struct SidecarFileEvent {
  std::uint64_t cycle = 0;
  std::uint8_t kind = 0;
  std::uint8_t phase = 0;
  std::uint32_t port_id = kSidecarNone;
  std::uint32_t nwords = 0;
  std::vector<std::uint64_t> words;
  std::string msg;
};

struct SidecarFileFrameItem {
  std::uint32_t port_id = kSidecarNone;
  std::uint32_t nwords = 0;
  std::vector<std::uint64_t> words;
  std::string msg;
};

struct SidecarFileFrame {
  std::uint64_t cycle = 0;
  std::uint8_t kind = 0;
  std::uint8_t phase = 0;
  std::vector<SidecarFileFrameItem> items;
};

struct SidecarPeriodicDrive {
  std::uint32_t port_id = kSidecarNone;
  std::uint64_t start_cycle = 0;
  std::uint64_t end_cycle = 0;
  std::uint64_t period = 0;
  std::uint64_t active_cycles = 0;
  std::uint64_t phase_cycle = 0;
  std::uint32_t active_nwords = 0;
  std::vector<std::uint64_t> active_words;
  std::uint32_t default_nwords = 0;
  std::vector<std::uint64_t> default_words;

  bool activeAt(std::uint64_t cycle) const {
    if (period == 0 || cycle < start_cycle || cycle >= end_cycle) return false;
    return ((cycle - phase_cycle) % period) < active_cycles;
  }
};

struct SidecarSchedule {
  std::uint16_t major = 0;
  std::uint16_t minor = 0;
  std::uint16_t patch = 0;
  std::uint32_t max_words = 0;
  std::uint64_t max_cycle = 0;
  std::uint32_t reset_cycles = 0;
  std::vector<SidecarSectionInfo> sections;
  std::vector<std::string> strings;
  std::vector<SidecarPort> ports;
  std::vector<SidecarFileEvent> events;
  std::vector<SidecarFileFrame> frames;
  std::vector<SidecarPeriodicDrive> periodic_drives;
};

namespace detail {

inline bool fail(std::string* error, const std::string& msg) {
  if (error != nullptr) *error = msg;
  return false;
}

inline bool checkRange(std::size_t size, std::size_t offset, std::size_t bytes, std::string* error, const char* what) {
  if (offset > size || bytes > size - offset) {
    return fail(error, std::string("truncated sidecar ") + what);
  }
  return true;
}

inline bool readU8(const std::vector<std::uint8_t>& data, std::size_t offset, std::uint8_t* out, std::string* error, const char* what) {
  if (!checkRange(data.size(), offset, 1, error, what)) return false;
  *out = data[offset];
  return true;
}

inline bool readU16(const std::vector<std::uint8_t>& data, std::size_t offset, std::uint16_t* out, std::string* error, const char* what) {
  if (!checkRange(data.size(), offset, 2, error, what)) return false;
  *out = static_cast<std::uint16_t>(data[offset]) | (static_cast<std::uint16_t>(data[offset + 1]) << 8);
  return true;
}

inline bool readU32(const std::vector<std::uint8_t>& data, std::size_t offset, std::uint32_t* out, std::string* error, const char* what) {
  if (!checkRange(data.size(), offset, 4, error, what)) return false;
  *out = static_cast<std::uint32_t>(data[offset]) |
         (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
         (static_cast<std::uint32_t>(data[offset + 3]) << 24);
  return true;
}

inline bool readU64(const std::vector<std::uint8_t>& data, std::size_t offset, std::uint64_t* out, std::string* error, const char* what) {
  if (!checkRange(data.size(), offset, 8, error, what)) return false;
  std::uint64_t value = 0;
  for (int idx = 0; idx < 8; ++idx) {
    value |= static_cast<std::uint64_t>(data[offset + static_cast<std::size_t>(idx)]) << (idx * 8);
  }
  *out = value;
  return true;
}

inline bool readFile(const std::filesystem::path& path, std::vector<std::uint8_t>* out, std::string* error) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream.is_open()) return fail(error, "failed to open sidecar file: " + path.string());
  out->assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  return true;
}

inline const SidecarSectionInfo* findSection(const SidecarSchedule& schedule, std::uint16_t kind) {
  for (const auto& section : schedule.sections) {
    if (section.kind == kind) return &section;
  }
  return nullptr;
}

inline bool stringRef(const SidecarSchedule& schedule, std::uint32_t sid, std::string* out, std::string* error) {
  if (sid == kSidecarNone) {
    out->clear();
    return true;
  }
  if (sid >= schedule.strings.size()) return fail(error, "sidecar string reference out of range");
  *out = schedule.strings[sid];
  return true;
}

inline bool readWords(const std::vector<std::uint8_t>& data,
                      std::size_t offset,
                      std::uint32_t max_words,
                      std::uint32_t nwords,
                      std::vector<std::uint64_t>* out,
                      std::string* error,
                      const char* what) {
  if (nwords > max_words) return fail(error, std::string(what) + " nwords exceeds max_words");
  if (!checkRange(data.size(), offset, static_cast<std::size_t>(max_words) * 8u, error, what)) return false;
  out->clear();
  out->reserve(nwords);
  for (std::uint32_t idx = 0; idx < nwords; ++idx) {
    std::uint64_t word = 0;
    if (!readU64(data, offset + static_cast<std::size_t>(idx) * 8u, &word, error, what)) return false;
    out->push_back(word);
  }
  return true;
}

inline bool decodeStringTable(const std::vector<std::uint8_t>& data,
                              const SidecarSectionInfo& section,
                              SidecarSchedule* schedule,
                              std::string* error) {
  std::size_t cursor = static_cast<std::size_t>(section.offset);
  const std::size_t end = static_cast<std::size_t>(section.offset + section.size);
  std::uint32_t count = 0;
  if (!readU32(data, cursor, &count, error, "string_table count")) return false;
  cursor += 4;
  schedule->strings.clear();
  schedule->strings.reserve(count);
  for (std::uint32_t idx = 0; idx < count; ++idx) {
    std::uint32_t length = 0;
    if (!readU32(data, cursor, &length, error, "string length")) return false;
    cursor += 4;
    if (cursor > end || length > end - cursor) return fail(error, "sidecar string table entry exceeds section");
    schedule->strings.emplace_back(reinterpret_cast<const char*>(data.data() + cursor), length);
    cursor += length;
  }
  return true;
}

inline bool decodePortTable(const std::vector<std::uint8_t>& data,
                            const SidecarSectionInfo& section,
                            SidecarSchedule* schedule,
                            std::string* error) {
  constexpr std::size_t kRecordSize = 24;
  if (section.size < section.count * kRecordSize) return fail(error, "sidecar port_table size/count mismatch");
  schedule->ports.clear();
  schedule->ports.reserve(static_cast<std::size_t>(section.count));
  for (std::uint64_t idx = 0; idx < section.count; ++idx) {
    const std::size_t cursor = static_cast<std::size_t>(section.offset) + static_cast<std::size_t>(idx) * kRecordSize;
    SidecarPort port;
    std::uint32_t name_sid = kSidecarNone;
    std::uint32_t protocol_sid = kSidecarNone;
    std::uint16_t reserved = 0;
    if (!readU32(data, cursor + 0, &port.id, error, "port id") ||
        !readU32(data, cursor + 4, &name_sid, error, "port name") ||
        !readU8(data, cursor + 8, &port.direction, error, "port direction") ||
        !readU8(data, cursor + 9, &port.role, error, "port role") ||
        !readU16(data, cursor + 10, &reserved, error, "port reserved") ||
        !readU32(data, cursor + 12, &port.bit_width, error, "port width") ||
        !readU32(data, cursor + 16, &port.word_count, error, "port words") ||
        !readU32(data, cursor + 20, &protocol_sid, error, "port protocol")) {
      return false;
    }
    if (!stringRef(*schedule, name_sid, &port.name, error) || !stringRef(*schedule, protocol_sid, &port.protocol, error)) return false;
    schedule->ports.push_back(std::move(port));
  }
  return true;
}

inline bool decodeEventTable(const std::vector<std::uint8_t>& data,
                             const SidecarSectionInfo& section,
                             SidecarSchedule* schedule,
                             std::string* error) {
  const std::size_t record_size = 24u + static_cast<std::size_t>(schedule->max_words) * 16u;
  if (section.size < section.count * record_size) return fail(error, "sidecar event_table size/count mismatch");
  schedule->events.clear();
  schedule->events.reserve(static_cast<std::size_t>(section.count));
  for (std::uint64_t idx = 0; idx < section.count; ++idx) {
    const std::size_t cursor = static_cast<std::size_t>(section.offset) + static_cast<std::size_t>(idx) * record_size;
    SidecarFileEvent event;
    std::uint16_t reserved = 0;
    std::uint32_t msg_sid = kSidecarNone;
    if (!readU64(data, cursor + 0, &event.cycle, error, "event cycle") ||
        !readU8(data, cursor + 8, &event.kind, error, "event kind") ||
        !readU8(data, cursor + 9, &event.phase, error, "event phase") ||
        !readU16(data, cursor + 10, &reserved, error, "event reserved") ||
        !readU32(data, cursor + 12, &event.port_id, error, "event port") ||
        !readU32(data, cursor + 16, &event.nwords, error, "event nwords") ||
        !readU32(data, cursor + 20, &msg_sid, error, "event message")) {
      return false;
    }
    if (!readWords(data, cursor + 24, schedule->max_words, event.nwords, &event.words, error, "event words")) return false;
    if (!stringRef(*schedule, msg_sid, &event.msg, error)) return false;
    schedule->events.push_back(std::move(event));
  }
  return true;
}

inline bool decodeFrameTable(const std::vector<std::uint8_t>& data,
                             const SidecarSectionInfo& section,
                             SidecarSchedule* schedule,
                             std::string* error) {
  std::size_t cursor = static_cast<std::size_t>(section.offset);
  const std::size_t end = static_cast<std::size_t>(section.offset + section.size);
  schedule->frames.clear();
  schedule->frames.reserve(static_cast<std::size_t>(section.count));
  for (std::uint64_t idx = 0; idx < section.count; ++idx) {
    if (!checkRange(end, cursor, 16, error, "frame prefix")) return false;
    SidecarFileFrame frame;
    std::uint16_t reserved = 0;
    std::uint32_t item_count = 0;
    if (!readU64(data, cursor + 0, &frame.cycle, error, "frame cycle") ||
        !readU8(data, cursor + 8, &frame.kind, error, "frame kind") ||
        !readU8(data, cursor + 9, &frame.phase, error, "frame phase") ||
        !readU16(data, cursor + 10, &reserved, error, "frame reserved") ||
        !readU32(data, cursor + 12, &item_count, error, "frame item count")) {
      return false;
    }
    cursor += 16;
    frame.items.reserve(item_count);
    for (std::uint32_t item_idx = 0; item_idx < item_count; ++item_idx) {
      const std::size_t item_size = 16u + static_cast<std::size_t>(schedule->max_words) * 16u;
      if (cursor > end || item_size > end - cursor) return fail(error, "sidecar frame item exceeds section");
      SidecarFileFrameItem item;
      std::uint32_t msg_sid = kSidecarNone;
      std::uint32_t item_reserved = 0;
      if (!readU32(data, cursor + 0, &item.port_id, error, "frame item port") ||
          !readU32(data, cursor + 4, &item.nwords, error, "frame item nwords") ||
          !readU32(data, cursor + 8, &msg_sid, error, "frame item message") ||
          !readU32(data, cursor + 12, &item_reserved, error, "frame item reserved")) {
        return false;
      }
      if (!readWords(data, cursor + 16, schedule->max_words, item.nwords, &item.words, error, "frame item words")) return false;
      if (!stringRef(*schedule, msg_sid, &item.msg, error)) return false;
      frame.items.push_back(std::move(item));
      cursor += item_size;
    }
    schedule->frames.push_back(std::move(frame));
  }
  return true;
}

inline bool decodePatternTable(const std::vector<std::uint8_t>& data,
                               const SidecarSectionInfo& section,
                               SidecarSchedule* schedule,
                               std::string* error) {
  const std::size_t record_size = 56u + static_cast<std::size_t>(schedule->max_words) * 16u;
  if (section.size < section.count * record_size) return fail(error, "sidecar pattern_table size/count mismatch");
  schedule->periodic_drives.clear();
  schedule->periodic_drives.reserve(static_cast<std::size_t>(section.count));
  for (std::uint64_t idx = 0; idx < section.count; ++idx) {
    const std::size_t cursor = static_cast<std::size_t>(section.offset) + static_cast<std::size_t>(idx) * record_size;
    std::uint16_t kind = 0;
    std::uint16_t reserved = 0;
    SidecarPeriodicDrive pattern;
    if (!readU16(data, cursor + 0, &kind, error, "pattern kind") ||
        !readU16(data, cursor + 2, &reserved, error, "pattern reserved") ||
        !readU32(data, cursor + 4, &pattern.port_id, error, "pattern port") ||
        !readU64(data, cursor + 8, &pattern.start_cycle, error, "pattern start") ||
        !readU64(data, cursor + 16, &pattern.end_cycle, error, "pattern end") ||
        !readU64(data, cursor + 24, &pattern.period, error, "pattern period") ||
        !readU64(data, cursor + 32, &pattern.active_cycles, error, "pattern active_cycles") ||
        !readU64(data, cursor + 40, &pattern.phase_cycle, error, "pattern phase_cycle") ||
        !readU32(data, cursor + 48, &pattern.active_nwords, error, "pattern active_nwords") ||
        !readU32(data, cursor + 52, &pattern.default_nwords, error, "pattern default_nwords")) {
      return false;
    }
    if (kind != 1) continue;
    const std::size_t active_offset = cursor + 56;
    const std::size_t default_offset = active_offset + static_cast<std::size_t>(schedule->max_words) * 8u;
    if (!readWords(data, active_offset, schedule->max_words, pattern.active_nwords, &pattern.active_words, error, "pattern active words") ||
        !readWords(data, default_offset, schedule->max_words, pattern.default_nwords, &pattern.default_words, error, "pattern default words")) {
      return false;
    }
    pattern.active_words.resize(schedule->max_words, 0);
    pattern.default_words.resize(schedule->max_words, 0);
    schedule->periodic_drives.push_back(std::move(pattern));
  }
  return true;
}

inline int findDriveIndex(const std::vector<std::uint32_t>& drive_port_ids, std::uint32_t port_id) {
  const auto it = std::find(drive_port_ids.begin(), drive_port_ids.end(), port_id);
  if (it == drive_port_ids.end()) return -1;
  return static_cast<int>(std::distance(drive_port_ids.begin(), it));
}

}  // namespace detail

inline bool loadSidecarSchedule(const std::filesystem::path& path, SidecarSchedule* schedule, std::string* error) {
  if (schedule == nullptr) return detail::fail(error, "null sidecar schedule output");
  std::vector<std::uint8_t> data;
  if (!detail::readFile(path, &data, error)) return false;
  if (data.size() < 49) return detail::fail(error, "sidecar file too small");
  static constexpr char kMagic[8] = {'S', 'I', 'D', 'E', 'C', 'A', 'R', '\n'};
  if (std::memcmp(data.data(), kMagic, 8) != 0) return detail::fail(error, "invalid sidecar magic");

  std::uint8_t endian = 0;
  std::uint16_t header_size = 0;
  std::uint64_t flags = 0;
  std::uint32_t section_count = 0;
  std::uint32_t reserved32 = 0;
  if (!detail::readU8(data, 8, &endian, error, "header endian") ||
      !detail::readU16(data, 9, &header_size, error, "header size") ||
      !detail::readU16(data, 11, &schedule->major, error, "header major") ||
      !detail::readU16(data, 13, &schedule->minor, error, "header minor") ||
      !detail::readU16(data, 15, &schedule->patch, error, "header patch") ||
      !detail::readU64(data, 17, &flags, error, "header flags") ||
      !detail::readU32(data, 25, &section_count, error, "header section_count") ||
      !detail::readU32(data, 29, &schedule->max_words, error, "header max_words") ||
      !detail::readU64(data, 33, &schedule->max_cycle, error, "header max_cycle") ||
      !detail::readU32(data, 41, &schedule->reset_cycles, error, "header reset_cycles") ||
      !detail::readU32(data, 45, &reserved32, error, "header reserved")) {
    return false;
  }
  if (endian != 1) return detail::fail(error, "unsupported sidecar endian marker");
  if (header_size < 49) return detail::fail(error, "invalid sidecar header size");
  const std::size_t directory_offset = header_size;
  const std::size_t directory_size = static_cast<std::size_t>(section_count) * 32u;
  if (!detail::checkRange(data.size(), directory_offset, directory_size, error, "section directory")) return false;

  *schedule = SidecarSchedule{schedule->major, schedule->minor, schedule->patch, schedule->max_words, schedule->max_cycle, schedule->reset_cycles};
  schedule->sections.reserve(section_count);
  for (std::uint32_t idx = 0; idx < section_count; ++idx) {
    const std::size_t cursor = directory_offset + static_cast<std::size_t>(idx) * 32u;
    SidecarSectionInfo section;
    std::uint32_t reserved = 0;
    if (!detail::readU16(data, cursor + 0, &section.kind, error, "section kind") ||
        !detail::readU16(data, cursor + 2, &section.flags, error, "section flags") ||
        !detail::readU32(data, cursor + 4, &reserved, error, "section reserved") ||
        !detail::readU64(data, cursor + 8, &section.offset, error, "section offset") ||
        !detail::readU64(data, cursor + 16, &section.size, error, "section size") ||
        !detail::readU64(data, cursor + 24, &section.count, error, "section count")) {
      return false;
    }
    if (section.offset > data.size() || section.size > data.size() - section.offset) {
      return detail::fail(error, "sidecar section extends beyond file");
    }
    schedule->sections.push_back(section);
  }

  const auto* string_section = detail::findSection(*schedule, kSidecarSectionStringTable);
  const auto* port_section = detail::findSection(*schedule, kSidecarSectionPortTable);
  const auto* event_section = detail::findSection(*schedule, kSidecarSectionEventTable);
  const auto* frame_section = detail::findSection(*schedule, kSidecarSectionFrameTable);
  const auto* pattern_section = detail::findSection(*schedule, kSidecarSectionPatternTable);
  if (string_section == nullptr || port_section == nullptr || event_section == nullptr || frame_section == nullptr) {
    return detail::fail(error, "sidecar file is missing a required section");
  }
  if (!detail::decodeStringTable(data, *string_section, schedule, error) ||
      !detail::decodePortTable(data, *port_section, schedule, error) ||
      !detail::decodeEventTable(data, *event_section, schedule, error) ||
      !detail::decodeFrameTable(data, *frame_section, schedule, error)) {
    return false;
  }
  if (pattern_section != nullptr && !detail::decodePatternTable(data, *pattern_section, schedule, error)) return false;
  return true;
}

template <std::size_t MaxWords, std::size_t MaxDrivePorts>
bool convertSidecarToRunnerSchedule(const SidecarSchedule& src,
                                          const std::array<std::uint32_t, MaxDrivePorts>& drive_port_ids,
                                          SidecarRunnerSchedule<MaxWords, MaxDrivePorts>* out,
                                          std::string* error) {
  if (out == nullptr) return detail::fail(error, "null sidecar schedule output");
  if (src.max_words > MaxWords) return detail::fail(error, "sidecar max_words exceeds generated sidecar capacity");

  std::vector<std::uint32_t> drive_ids(drive_port_ids.begin(), drive_port_ids.end());
  out->drive_frames.clear();
  out->pre_expect_events.clear();
  out->post_expect_events.clear();

  for (const auto& frame : src.frames) {
    if (frame.kind != 0) continue;
    SidecarDriveFrame<MaxWords, MaxDrivePorts> dst;
    dst.cycle = frame.cycle;
    for (auto& mask_word : dst.port_mask) mask_word = 0;
    for (auto& port_words : dst.words) {
      for (auto& word : port_words) word = 0;
    }
    for (const auto& item : frame.items) {
      const int slot = detail::findDriveIndex(drive_ids, item.port_id);
      if (slot < 0) return detail::fail(error, "sidecar frame targets a non-drive port");
      if (item.nwords > MaxWords) return detail::fail(error, "sidecar frame item nwords exceeds generated capacity");
      dst.port_mask[static_cast<std::size_t>(slot) / 64u] |= (std::uint64_t{1} << (static_cast<std::size_t>(slot) % 64u));
      for (std::uint32_t word = 0; word < item.nwords; ++word) {
        dst.words[static_cast<std::size_t>(slot)][word] = item.words[word];
      }
    }
    out->drive_frames.push_back(std::move(dst));
  }

  for (const auto& event : src.events) {
    if (event.kind != 1) continue;
    if (event.nwords > MaxWords) return detail::fail(error, "sidecar event nwords exceeds generated capacity");
    SidecarEvent<MaxWords> dst;
    dst.cycle = event.cycle;
    dst.port_id = event.port_id;
    dst.nwords = event.nwords;
    dst.msg = event.msg;
    dst.words.fill(0);
    for (std::uint32_t word = 0; word < event.nwords; ++word) {
      dst.words[word] = event.words[word];
    }
    if (event.phase == 0) {
      out->pre_expect_events.push_back(std::move(dst));
    } else if (event.phase == 1) {
      out->post_expect_events.push_back(std::move(dst));
    } else {
      return detail::fail(error, "sidecar expect event has unsupported phase");
    }
  }

  for (const auto& pattern : src.periodic_drives) {
    if (pattern.active_nwords > MaxWords || pattern.default_nwords > MaxWords) {
      return detail::fail(error, "sidecar pattern nwords exceeds generated capacity");
    }
    if (detail::findDriveIndex(drive_ids, pattern.port_id) < 0) {
      return detail::fail(error, "sidecar pattern targets a non-drive port");
    }
  }
  return true;
}

}  // namespace pyc::cpp
