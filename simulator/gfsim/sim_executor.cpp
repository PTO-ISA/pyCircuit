#include "gfsim/SimExecutor.h"

#include "gfsim/model_input.h"

#include <algorithm>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace gfsim {
namespace {

constexpr std::string_view kFallbackError =
    R"({"code":"runtime_failure","message":"runtime failure","phase":"api","instance":null,"source":null,"check_id":null})";

class CallLock {
public:
  explicit CallLock(std::atomic_flag &lock) noexcept : lock_(lock) {
    while (lock_.test_and_set(std::memory_order_acquire)) {
    }
  }
  ~CallLock() { lock_.clear(std::memory_order_release); }

private:
  std::atomic_flag &lock_;
};

std::string escapeJson(std::string_view value) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  for (unsigned char character : value) {
    switch (character) {
    case '\\':
      output << "\\\\";
      break;
    case '"':
      output << "\\\"";
      break;
    case '\b':
      output << "\\b";
      break;
    case '\f':
      output << "\\f";
      break;
    case '\n':
      output << "\\n";
      break;
    case '\r':
      output << "\\r";
      break;
    case '\t':
      output << "\\t";
      break;
    default:
      if (character < 0x20) {
        constexpr char digits[] = "0123456789abcdef";
        output << "\\u00" << digits[character >> 4] << digits[character & 0xf];
      } else {
        output << static_cast<char>(character);
      }
    }
  }
  return output.str();
}

void appendString(std::ostringstream &output, std::string_view value) {
  output << '"' << escapeJson(value) << '"';
}

void appendStatistic(std::ostringstream &output, bool &first,
                     std::string_view objectPath, std::string_view name,
                     std::string_view kind, std::uint64_t value,
                     std::uint64_t lastUpdate) {
  if (!first)
    output << ',';
  first = false;
  output << "{\"buckets\":[],\"count\":0,\"kind\":";
  appendString(output, kind);
  output << ",\"last_update\":{\"delta\":0,\"time\":" << lastUpdate
         << "},\"maximum\":0,\"minimum\":0,\"name\":";
  appendString(output, name);
  output << ",\"object_path\":";
  appendString(output, objectPath);
  output << ",\"sum\":0,\"value\":" << value << '}';
}

std::string_view phaseName(SimFailurePhase phase) {
  switch (phase) {
  case SimFailurePhase::Build:
    return "api";
  case SimFailurePhase::Evaluate:
    return "evaluate";
  case SimFailurePhase::Check:
    return "check";
  case SimFailurePhase::Drive:
    return "drive";
  case SimFailurePhase::Xfer:
    return "xfer";
  case SimFailurePhase::None:
    return "api";
  }
  return "api";
}

} // namespace

SimExecutor::SimExecutor(
    SimSystem &system, ObservationSlots &observations,
    std::span<const ReportGaugeDescriptor> reports) noexcept
    : system_(system), observations_(observations) {
  try {
    reports_.assign(reports.begin(), reports.end());
    gaugeSnapshots_.assign(observations_.Gauges().begin(),
                           observations_.Gauges().end());
    lastError_.reserve(512);
    statistics_.reserve(512 + reports.size() * 192);
    if (!system_.HasObservations(observations_) || !reportsAreValid()) {
      setError("runtime_failure", "report gauge metadata is invalid", "api");
      return;
    }
    system_.Build();
    if (system_.state() != SimSystemState::Built) {
      setError("runtime_failure", "model Build failed", "api");
      return;
    }
    state_ = SimExecutorState::Created;
    clearError();
  } catch (...) {
    state_ = SimExecutorState::Failed;
    setError("runtime_failure", "model construction failed", "api");
  }
}

bool SimExecutor::created() const noexcept {
  CallLock lock(callLock_);
  return state_ == SimExecutorState::Created;
}

SimExecutorState SimExecutor::state() const noexcept {
  CallLock lock(callLock_);
  return state_;
}

std::uint64_t SimExecutor::cycles() const noexcept {
  CallLock lock(callLock_);
  return committedCycles_;
}

PycircuitModelStatusV1 SimExecutor::ConfigureJson(const std::uint8_t *data,
                                                std::uint64_t size) noexcept {
  CallLock lock(callLock_);
  if (state_ != SimExecutorState::Created) {
    setError("invalid_state", "configure requires Created state", "configure");
    return PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE;
  }
  if (!data || size == 0 || size > std::numeric_limits<std::size_t>::max()) {
    setError("invalid_argument", "model config buffer is invalid", "configure");
    return PYCIRCUIT_MODEL_STATUS_V1_INVALID_ARGUMENT;
  }
  try {
    std::string_view json(reinterpret_cast<const char *>(data),
                          static_cast<std::size_t>(size));
    if (json.ends_with('\n'))
      json.remove_suffix(1);
    RuntimeLimits candidate;
    std::string error;
    if (!parseModelConfigJson(json, candidate, error)) {
      setError("invalid_argument", error, "configure");
      return PYCIRCUIT_MODEL_STATUS_V1_INVALID_ARGUMENT;
    }
    if (candidate.deadlockWindow) {
      setError("invalid_argument",
               "deadlock_window is unavailable in the foundation runtime",
               "configure");
      return PYCIRCUIT_MODEL_STATUS_V1_INVALID_ARGUMENT;
    }
    if ((candidate.maxTicks && *candidate.maxTicks == 0) ||
        candidate.maxDomainCycles.size() > 1 ||
        (!candidate.maxDomainCycles.empty() &&
         (candidate.maxDomainCycles.begin()->first != "default" ||
          candidate.maxDomainCycles.begin()->second == 0))) {
      setError("invalid_argument",
               "runtime limits are outside the foundation profile",
               "configure");
      return PYCIRCUIT_MODEL_STATUS_V1_INVALID_ARGUMENT;
    }
    maxTicks_ = candidate.maxTicks;
    maxDomainCycles_ = candidate.maxDomainCycles.empty()
                           ? std::optional<std::uint64_t>()
                           : std::optional<std::uint64_t>(
                                 candidate.maxDomainCycles.begin()->second);
    state_ = SimExecutorState::Configured;
    clearError();
    return PYCIRCUIT_MODEL_STATUS_V1_OK;
  } catch (...) {
    setError("runtime_failure", "model configuration failed", "configure");
    return PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
}

PycircuitModelStatusV1 SimExecutor::Reset() noexcept {
  CallLock lock(callLock_);
  if (state_ != SimExecutorState::Configured &&
      state_ != SimExecutorState::Ready &&
      state_ != SimExecutorState::Completed &&
      state_ != SimExecutorState::Failed) {
    setError("invalid_state", "reset requires a configured model", "reset");
    return PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE;
  }
  system_.Reset();
  if (system_.state() != SimSystemState::Ready) {
    state_ = SimExecutorState::Failed;
    setError("reset_failed", "model reset failed", "reset", {}, {}, {}, true);
    executionFailureLatched_ = true;
    return PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
  stopReason_ = StopReason::None;
  stopReasonUpdate_ = 0;
  committedCycles_ = 0;
  std::copy(observations_.Gauges().begin(), observations_.Gauges().end(),
            gaugeSnapshots_.begin());
  state_ = SimExecutorState::Ready;
  executionFailureLatched_ = false;
  clearError();
  return PYCIRCUIT_MODEL_STATUS_V1_OK;
}

PycircuitModelStatusV1
SimExecutor::Step(PycircuitModelStepResultV1 *result) noexcept {
  CallLock lock(callLock_);
  if (!result) {
    setError("invalid_argument", "step result is null", "api");
    return PYCIRCUIT_MODEL_STATUS_V1_INVALID_ARGUMENT;
  }
  if (result->struct_size != sizeof(PycircuitModelStepResultV1)) {
    setError("abi_mismatch", "step result layout is incompatible", "api");
    return PYCIRCUIT_MODEL_STATUS_V1_ABI_MISMATCH;
  }
  if (state_ != SimExecutorState::Ready) {
    if (state_ != SimExecutorState::Failed)
      setError("invalid_state", "step requires Ready state", "api");
    return PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE;
  }
  if (system_.cycle() == std::numeric_limits<std::uint64_t>::max()) {
    state_ = SimExecutorState::Failed;
    setError("counter_overflow", "cycle counter would overflow", "evaluate", {},
             {}, {}, true);
    executionFailureLatched_ = true;
    *result = {sizeof(PycircuitModelStepResultV1), PYCIRCUIT_MODEL_STEP_V1_FAILED,
               system_.cycle(), 0, 0};
    return PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
  SimStepResult step = system_.Step();
  if (step == SimStepResult::Quiescent) {
    clearError();
    *result = {sizeof(PycircuitModelStepResultV1),
               PYCIRCUIT_MODEL_STEP_V1_QUIESCENT, system_.cycle(), 0, 0};
    return PYCIRCUIT_MODEL_STATUS_V1_OK;
  }
  if (step != SimStepResult::Running) {
    state_ = SimExecutorState::Failed;
    SimFailureInfo failure = system_.failureInfo();
    setError(failure.code.empty() ? std::string_view("runtime_failure")
                                  : failure.code,
             failure.message.empty() ? std::string_view("model step failed")
                                     : failure.message,
             phaseName(failure.phase), failure.instance, failure.sourceJson,
             failure.checkIdJson, true);
    executionFailureLatched_ = true;
    *result = {sizeof(PycircuitModelStepResultV1), PYCIRCUIT_MODEL_STEP_V1_FAILED,
               committedCycles_, 0, 0};
    return PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }

  committedCycles_ = system_.cycle();
  std::copy(observations_.Gauges().begin(), observations_.Gauges().end(),
            gaugeSnapshots_.begin());
  bool ticksReached = maxTicks_ && committedCycles_ >= *maxTicks_;
  bool domainReached =
      maxDomainCycles_ && committedCycles_ >= *maxDomainCycles_;
  PycircuitModelStepStateV1 state = PYCIRCUIT_MODEL_STEP_V1_RUNNING;
  if (ticksReached || domainReached) {
    stopReason_ =
        ticksReached ? StopReason::MaxTicks : StopReason::MaxDomainCycles;
    stopReasonUpdate_ = committedCycles_;
    state_ = SimExecutorState::Completed;
    state = PYCIRCUIT_MODEL_STEP_V1_TERMINATED;
  }
  clearError();
  *result = {sizeof(PycircuitModelStepResultV1), state, committedCycles_, 0, 0};
  return PYCIRCUIT_MODEL_STATUS_V1_OK;
}

PycircuitModelStatusV1
SimExecutor::StatisticsJson(PycircuitModelBufferV1 *result) noexcept {
  CallLock lock(callLock_);
  if (!result) {
    setError("invalid_argument", "statistics result is null", "statistics");
    return PYCIRCUIT_MODEL_STATUS_V1_INVALID_ARGUMENT;
  }
  if (state_ != SimExecutorState::Ready &&
      state_ != SimExecutorState::Completed &&
      state_ != SimExecutorState::Failed) {
    setError("invalid_state", "statistics require an initialized model",
             "statistics");
    return PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE;
  }
  try {
    if (!buildStatistics())
      throw std::runtime_error("report gauge metadata changed");
    setBuffer(*result, statistics_);
    return PYCIRCUIT_MODEL_STATUS_V1_OK;
  } catch (...) {
    setError("statistics_failure", "statistics snapshot failed", "statistics");
    return PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
}

PycircuitModelStatusV1
SimExecutor::LastError(PycircuitModelBufferV1 *result) noexcept {
  CallLock lock(callLock_);
  if (!result) {
    setError("invalid_argument", "last_error result is null", "api");
    return PYCIRCUIT_MODEL_STATUS_V1_INVALID_ARGUMENT;
  }
  if (fallbackError_) {
    result->data =
        reinterpret_cast<const std::uint8_t *>(kFallbackError.data());
    result->size = kFallbackError.size();
  } else {
    setBuffer(*result, lastError_);
  }
  return PYCIRCUIT_MODEL_STATUS_V1_OK;
}

void SimExecutor::clearError() noexcept {
  fallbackError_ = false;
  lastError_.clear();
}

void SimExecutor::setError(std::string_view code, std::string_view message,
                           std::string_view phase, std::string_view instance,
                           std::string_view sourceJson,
                           std::string_view checkIdJson,
                           bool replaceExecutionFailure) noexcept {
  if (executionFailureLatched_ && !replaceExecutionFailure)
    return;
  try {
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << "{\"code\":";
    appendString(output, code);
    output << ",\"message\":";
    appendString(output, message);
    output << ",\"phase\":";
    appendString(output, phase);
    output << ",\"instance\":";
    if (instance.empty())
      output << "null";
    else
      appendString(output, instance);
    output << ",\"source\":" << (sourceJson.empty() ? "null" : sourceJson)
           << ",\"check_id\":" << (checkIdJson.empty() ? "null" : checkIdJson)
           << '}';
    lastError_ = output.str();
    fallbackError_ = false;
  } catch (...) {
    fallbackError_ = true;
    lastError_.clear();
  }
}

void SimExecutor::setBuffer(PycircuitModelBufferV1 &result,
                            const std::string &storage) const noexcept {
  result.data = storage.empty()
                    ? nullptr
                    : reinterpret_cast<const std::uint8_t *>(storage.data());
  result.size = storage.size();
}

bool SimExecutor::reportsAreValid() const noexcept {
  if (reports_.size() != gaugeSnapshots_.size())
    return false;
  for (std::size_t index = 0; index < reports_.size(); ++index) {
    const ReportGaugeDescriptor &report = reports_[index];
    if (report.objectPath.empty() || report.name.empty())
      return false;
    for (std::size_t previous = 0; previous < index; ++previous)
      if (reports_[previous].stableOrdinal == report.stableOrdinal ||
          (reports_[previous].objectPath == report.objectPath &&
           reports_[previous].name == report.name))
        return false;
    bool found = false;
    for (const GaugeSnapshot &gauge : gaugeSnapshots_)
      found |= gauge.stableOrdinal == report.stableOrdinal;
    if (!found)
      return false;
  }
  return true;
}

bool SimExecutor::buildStatistics() {
  if (!reportsAreValid())
    return false;
  struct Row {
    std::string_view objectPath;
    std::string_view name;
    std::string_view kind;
    std::uint64_t value;
    std::uint64_t lastUpdate;
  };
  std::vector<Row> rows{
      {"@runtime", "cycles", "counter", committedCycles_, committedCycles_},
      {"@runtime", "stop_reason", "gauge",
       static_cast<std::uint64_t>(stopReason_), stopReasonUpdate_},
  };
  rows.reserve(2 + reports_.size());
  for (const ReportGaugeDescriptor &report : reports_) {
    auto gauge =
        std::find_if(gaugeSnapshots_.begin(), gaugeSnapshots_.end(),
                     [&](const GaugeSnapshot &candidate) {
                       return candidate.stableOrdinal == report.stableOrdinal;
                     });
    if (gauge == gaugeSnapshots_.end() ||
        gauge->value.kind != SlotValueKind::Unsigned)
      return false;
    rows.push_back({report.objectPath, report.name, "gauge", gauge->value.bits,
                    gauge->lastUpdate});
  }
  std::sort(rows.begin(), rows.end(), [](const Row &left, const Row &right) {
    return std::tie(left.objectPath, left.name) <
           std::tie(right.objectPath, right.name);
  });

  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << '[';
  bool first = true;
  for (const Row &row : rows)
    appendStatistic(output, first, row.objectPath, row.name, row.kind,
                    row.value, row.lastUpdate);
  output << "]\n";
  statistics_ = output.str();
  return true;
}

} // namespace gfsim
