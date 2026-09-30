#ifndef GFSIM_SYSTEMRUNNER_H
#define GFSIM_SYSTEMRUNNER_H

#include "gfsim/ObservationSlot.h"
#include "gfsim/SimSystem.h"

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
};

class SystemRunner {
public:
  SystemRunner(int argc, char **argv);
  ~SystemRunner();

  SystemRunner(const SystemRunner &) = delete;
  SystemRunner &operator=(const SystemRunner &) = delete;
  SystemRunner(SystemRunner &&) = delete;
  SystemRunner &operator=(SystemRunner &&) = delete;

  bool ready() const noexcept { return ready_; }
  int Run(SimSystem &system, ObservationSlots &observations,
          std::span<const RunnerObservation> metadata);

private:
  void fail(std::string_view message) noexcept;
  bool openEventSink(const std::string &path) noexcept;
  bool writeRecord(std::string_view record) noexcept;

  std::string configJson_;
  std::FILE *eventFile_ = nullptr;
  bool ownsEventFile_ = false;
  bool ready_ = false;
};

} // namespace gfsim

#endif // GFSIM_SYSTEMRUNNER_H
