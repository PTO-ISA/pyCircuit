#ifndef GFSIM_SIMEXECUTOR_H
#define GFSIM_SIMEXECUTOR_H

#include "gfsim/ObservationSlot.h"
#include "gfsim/SimSystem.h"
#include "gfsim/model_api.h"

#include <atomic>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gfsim {

struct ReportGaugeDescriptor {
  std::uint32_t stableOrdinal = 0;
  std::string objectPath;
  std::string name;
};

enum class SimExecutorState { Created, Configured, Ready, Completed, Failed };

class SimExecutor {
public:
  SimExecutor(SimSystem &system, ObservationSlots &observations,
              std::span<const ReportGaugeDescriptor> reports) noexcept;

  SimExecutor(const SimExecutor &) = delete;
  SimExecutor &operator=(const SimExecutor &) = delete;
  SimExecutor(SimExecutor &&) = delete;
  SimExecutor &operator=(SimExecutor &&) = delete;

  bool created() const noexcept;
  SimExecutorState state() const noexcept;
  std::uint64_t cycles() const noexcept;

  PycircuitModelStatusV1 ConfigureJson(const std::uint8_t *data,
                                     std::uint64_t size) noexcept;
  PycircuitModelStatusV1 Reset() noexcept;
  PycircuitModelStatusV1 Step(PycircuitModelStepResultV1 *result) noexcept;
  PycircuitModelStatusV1 StatisticsJson(PycircuitModelBufferV1 *result) noexcept;
  PycircuitModelStatusV1 LastError(PycircuitModelBufferV1 *result) noexcept;

private:
  enum class StopReason : std::uint64_t {
    None = 0,
    MaxTicks = 1,
    MaxDomainCycles = 2,
  };

  void clearError() noexcept;
  void setError(std::string_view code, std::string_view message,
                std::string_view phase, std::string_view instance = {},
                std::string_view sourceJson = {},
                std::string_view checkIdJson = {},
                bool replaceExecutionFailure = false) noexcept;
  void setBuffer(PycircuitModelBufferV1 &result,
                 const std::string &storage) const noexcept;
  bool reportsAreValid() const noexcept;
  bool buildStatistics();

  SimSystem &system_;
  ObservationSlots &observations_;
  std::vector<ReportGaugeDescriptor> reports_;
  std::vector<GaugeSnapshot> gaugeSnapshots_;
  mutable std::atomic_flag callLock_ = ATOMIC_FLAG_INIT;
  SimExecutorState state_ = SimExecutorState::Failed;
  std::optional<std::uint64_t> maxTicks_;
  std::optional<std::uint64_t> maxDomainCycles_;
  StopReason stopReason_ = StopReason::None;
  std::uint64_t stopReasonUpdate_ = 0;
  std::uint64_t committedCycles_ = 0;
  std::string statistics_;
  std::string lastError_;
  bool fallbackError_ = false;
  bool executionFailureLatched_ = false;
};

} // namespace gfsim

#endif // GFSIM_SIMEXECUTOR_H
