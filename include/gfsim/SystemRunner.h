#ifndef GFSIM_SYSTEMRUNNER_H
#define GFSIM_SYSTEMRUNNER_H

#include "gfsim/ObservationSlot.h"
#include "gfsim/SimSystem.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>

namespace gfsim {

struct RunnerObservation {
  std::uint32_t stableOrdinal = 0;
  std::string kind;
  std::string instance;
  std::string registration;
  std::string site;
  std::string specJson;
  bool hasValue = false;
  // Populated for report observations from the validated spec.name field.
  std::string reportName;
  // One source site may use several scalar slots. Defaults preserve ordinary
  // scalar callers; grouping is explicit, never inferred from display names.
  std::uint32_t groupIndex = 0;
  std::uint32_t groupSize = 1;
};

// Host callbacks borrow their context for Run. Initial drive precedes Reset;
// drive returns false when the host has finished, before starting another Step.
// sample observes only successful committed Steps, with their committed epoch.
struct RunnerCallbacks {
  void *context = nullptr;
  void (*initialize)(void *) = nullptr;
  bool (*drive)(void *, std::uint64_t epoch) = nullptr;
  void (*sample)(void *, std::uint64_t committedEpoch) = nullptr;
};

class SystemRunner {
public:
  SystemRunner(int argc, char **argv);
  SystemRunner(int argc, char **argv, std::string_view embeddedConfig);
  ~SystemRunner();

  SystemRunner(const SystemRunner &) = delete;
  SystemRunner &operator=(const SystemRunner &) = delete;
  SystemRunner(SystemRunner &&) = delete;
  SystemRunner &operator=(SystemRunner &&) = delete;

  bool ready() const noexcept { return ready_; }
  std::size_t workers() const noexcept { return workers_; }
  int Run(SimSystem &system, ObservationSlots &observations,
          std::span<const RunnerObservation> metadata,
          RunnerCallbacks callbacks = {});

private:
  void fail(std::string_view message) noexcept;
  bool openEventSink(const std::string &path) noexcept;
  bool writeRecord(std::string_view record) noexcept;

  std::string configJson_;
  std::FILE *eventFile_ = nullptr;
  bool ownsEventFile_ = false;
  bool ready_ = false;
  bool hasDomainLimit_ = false;
  std::size_t workers_ = 1;
};

} // namespace gfsim

#endif // GFSIM_SYSTEMRUNNER_H
