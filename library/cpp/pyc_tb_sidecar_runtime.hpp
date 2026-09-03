#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace pyc::cpp {

template <std::size_t MaxWords>
struct SidecarEvent {
  std::uint64_t cycle = 0;
  std::uint32_t port_id = 0;
  std::uint32_t nwords = 0;
  std::array<std::uint64_t, MaxWords> words{};
  std::string msg;
};

template <std::size_t MaxWords, std::size_t MaxDrivePorts>
struct SidecarDriveFrame {
  static constexpr std::size_t kMaskWords = (MaxDrivePorts + 63u) / 64u;

  std::uint64_t cycle = 0;
  std::array<std::uint64_t, kMaskWords> port_mask{};
  std::array<std::array<std::uint64_t, MaxWords>, MaxDrivePorts> words{};
};

template <std::size_t MaxWords, std::size_t MaxDrivePorts>
struct SidecarRunnerSchedule {
  std::vector<SidecarDriveFrame<MaxWords, MaxDrivePorts>> drive_frames;
  std::vector<SidecarEvent<MaxWords>> pre_expect_events;
  std::vector<SidecarEvent<MaxWords>> post_expect_events;
};

}  // namespace pyc::cpp
