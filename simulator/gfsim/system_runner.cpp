#include "gfsim/SystemRunner.h"

#include "gfsim/SimExecutor.h"
#include "gfsim/model_api.h"
#include "gfsim/model_input.h"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace gfsim {
namespace {

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
        output << "\\u00" << digits[character >> 4] << digits[character & 0x0f];
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

std::string signedDecimal(std::uint64_t bits) {
  return std::to_string(std::bit_cast<std::int64_t>(bits));
}

void appendValue(std::ostringstream &output, SlotValue value) {
  if (value.kind == SlotValueKind::Bool) {
    output << "{\"kind\":\"bool\",\"value\":"
           << (value.bits != 0 ? "true" : "false") << '}';
    return;
  }
  output << "{\"kind\":\"integer\",\"value\":";
  appendString(output, value.kind == SlotValueKind::Signed
                           ? signedDecimal(value.bits)
                           : std::to_string(value.bits));
  output << '}';
}

bool sameObservationGroup(const RunnerObservation &first,
                          const RunnerObservation &entry, std::size_t index) {
  return entry.groupIndex == index && entry.groupSize == first.groupSize &&
         entry.kind == first.kind && entry.instance == first.instance &&
         entry.registration == first.registration && entry.site == first.site &&
         entry.specJson == first.specJson &&
         entry.reportName == first.reportName;
}

bool metadataMatches(std::span<const RunnerObservation> metadata,
                     std::span<const ObservationDescriptor> descriptors,
                     std::vector<ReportGaugeDescriptor> &reports) {
  if (metadata.size() != descriptors.size())
    return false;
  reports.clear();
  reports.reserve(metadata.size());
  std::vector<bool> seen(metadata.size(), false);
  std::vector<const RunnerObservation *> ordered(metadata.size(), nullptr);
  for (const RunnerObservation &entry : metadata) {
    if (entry.instance.empty() || entry.registration.empty() ||
        entry.site.empty() || entry.specJson.empty())
      return false;
    auto descriptor = std::find_if(
        descriptors.begin(), descriptors.end(), [&](const auto &candidate) {
          return candidate.stableOrdinal == entry.stableOrdinal;
        });
    if (descriptor == descriptors.end())
      return false;
    const std::size_t index =
        static_cast<std::size_t>(descriptor - descriptors.begin());
    if (seen[index])
      return false;
    seen[index] = true;
    ordered[index] = &entry;
    const bool isReport = entry.kind == "report";
    if ((!isReport && entry.kind != "print" && entry.kind != "log") ||
        (isReport != (descriptor->kind == ObservationKind::Gauge)) ||
        (isReport && !entry.hasValue))
      return false;
    if (isReport) {
      if (entry.reportName.empty())
        return false;
      reports.push_back(
          {entry.stableOrdinal, entry.instance, entry.reportName});
    } else if (!entry.reportName.empty()) {
      return false;
    }
  }
  for (std::size_t begin = 0; begin < ordered.size();) {
    const RunnerObservation &first = *ordered[begin];
    if (first.groupIndex != 0 || first.groupSize == 0 ||
        first.groupSize > ordered.size() - begin ||
        (first.kind == "report" && first.groupSize != 1))
      return false;
    for (std::size_t index = 0; index < first.groupSize; ++index) {
      const RunnerObservation &entry = *ordered[begin + index];
      const ObservationDescriptor &descriptor = descriptors[begin + index];
      const ObservationDescriptor &groupDescriptor = descriptors[begin];
      if (!sameObservationGroup(first, entry, index) ||
          descriptor.ownerKey != groupDescriptor.ownerKey ||
          descriptor.registrationKey != groupDescriptor.registrationKey ||
          descriptor.siteKey != groupDescriptor.siteKey ||
          descriptor.kind != groupDescriptor.kind ||
          (first.groupSize > 1 && !entry.hasValue))
        return false;
    }
    begin += first.groupSize;
  }
  return std::all_of(seen.begin(), seen.end(),
                     [](bool value) { return value; });
}

struct OutputItem {
  std::size_t descriptorOrder;
  const RunnerObservation *metadata;
  const CommittedEventSlot *event;
  const GaugeSnapshot *gauge;
};

std::uint64_t observationEpoch(const OutputItem &item) {
  return item.event ? item.event->epoch : item.gauge->lastUpdate;
}

std::string eventRecord(std::span<const OutputItem> group) {
  const OutputItem &item = group.front();
  const RunnerObservation &metadata = *item.metadata;
  const std::uint64_t committedEpoch = observationEpoch(item);
  const std::uint64_t evaluationEpoch =
      committedEpoch == 0 ? 0 : committedEpoch - 1;

  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << "{\"kind\":";
  appendString(output, metadata.kind);
  output << ",\"instance\":";
  appendString(output, metadata.instance);
  output << ",\"registration\":";
  appendString(output, metadata.registration);
  output << ",\"site\":";
  appendString(output, metadata.site);
  output << ",\"evaluation_epoch\":";
  appendString(output, std::to_string(evaluationEpoch));
  output << ",\"commit_epoch\":";
  appendString(output, std::to_string(committedEpoch));
  output << ",\"spec\":" << metadata.specJson << ",\"values\":[";
  bool hasValue = false;
  for (const OutputItem &lane : group) {
    if (!lane.metadata->hasValue)
      continue;
    const SlotValue value = lane.event ? lane.event->value : lane.gauge->value;
    if (value.kind != SlotValueKind::Bool &&
        value.kind != SlotValueKind::Signed &&
        value.kind != SlotValueKind::Unsigned)
      throw std::runtime_error("observation has an unsupported value kind");
    if (hasValue)
      output << ',';
    appendValue(output, value);
    hasValue = true;
  }
  output << "]}\n";
  return output.str();
}

std::string resultRecord(std::string_view status, std::uint64_t epoch,
                         std::string_view statistics,
                         std::string_view errorJson) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << "{\"kind\":\"result\",\"status\":";
  appendString(output, status);
  output << ",\"epoch_time\":";
  appendString(output, std::to_string(epoch));
  output << ",\"statistics\":" << statistics << ",\"error\":";
  output << (errorJson.empty() ? "null" : std::string(errorJson));
  output << "}\n";
  return output.str();
}

bool exclusiveCreate(const std::string &path, std::FILE *&file) noexcept {
#if defined(_WIN32)
  const int descriptor =
      _open(path.c_str(), _O_CREAT | _O_EXCL | _O_WRONLY | _O_BINARY,
            _S_IREAD | _S_IWRITE);
  if (descriptor < 0)
    return false;
  const intptr_t native = _get_osfhandle(descriptor);
  BY_HANDLE_FILE_INFORMATION info{};
  if (native == -1 ||
      !GetFileInformationByHandle(reinterpret_cast<HANDLE>(native), &info) ||
      (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    _close(descriptor);
    DeleteFileA(path.c_str());
    return false;
  }
  file = _fdopen(descriptor, "wb");
  if (!file) {
    _close(descriptor);
    DeleteFileA(path.c_str());
    return false;
  }
#else
  const int descriptor = ::open(path.c_str(),
                                O_CREAT | O_EXCL | O_WRONLY
#ifdef O_CLOEXEC
                                    | O_CLOEXEC
#endif
                                ,
                                0666);
  if (descriptor < 0)
    return false;
  struct stat status{};
  if (::fstat(descriptor, &status) != 0 || !S_ISREG(status.st_mode)) {
    ::close(descriptor);
    ::unlink(path.c_str());
    return false;
  }
  file = ::fdopen(descriptor, "wb");
  if (!file) {
    ::close(descriptor);
    ::unlink(path.c_str());
    return false;
  }
#endif
  return true;
}

} // namespace

SystemRunner::SystemRunner(int argc, char **argv)
    : SystemRunner(argc, argv, {}) {}

SystemRunner::SystemRunner(int argc, char **argv,
                           std::string_view embeddedConfig) {
  std::string configPath;
  std::string eventPath;
  bool hasConfig = false;
  bool hasEvents = false;
  bool hasWorkers = false;
  if (!argv || argc < 1) {
    fail("usage: pycircuit_system --config PATH [--events PATH| -] [--workers "
         "N]");
    return;
  }
  for (int index = 1; index < argc;) {
    if (!argv[index]) {
      fail("invalid command-line argument");
      return;
    }
    const std::string_view option(argv[index++]);
    const bool configOption = option == "--config";
    const bool eventsOption = option == "--events";
    const bool workersOption = option == "--workers";
    if (!configOption && !eventsOption && !workersOption) {
      fail("unknown option or positional argument");
      return;
    }
    if ((configOption && hasConfig) || (eventsOption && hasEvents) ||
        (workersOption && hasWorkers)) {
      fail("repeated command-line option");
      return;
    }
    if (index >= argc || !argv[index] || argv[index][0] == '\0' ||
        std::string_view(argv[index]) == "--config" ||
        std::string_view(argv[index]) == "--events" ||
        std::string_view(argv[index]) == "--workers") {
      fail(configOption    ? "expected --config PATH"
           : workersOption ? "expected --workers N"
                           : "expected --events PATH or --events -");
      return;
    }
    if (configOption) {
      configPath = argv[index++];
      hasConfig = true;
    } else if (eventsOption) {
      eventPath = argv[index++];
      hasEvents = true;
    } else {
      const std::string_view value(argv[index++]);
      const auto parsed =
          std::from_chars(value.data(), value.data() + value.size(), workers_);
      if (!std::all_of(value.begin(), value.end(),
                       [](char c) { return c >= '0' && c <= '9'; }) ||
          parsed.ec != std::errc() ||
          parsed.ptr != value.data() + value.size() || workers_ == 0) {
        fail("workers must be a positive decimal integer");
        return;
      }
      hasWorkers = true;
    }
  }
  if (!hasConfig && embeddedConfig.empty()) {
    fail("expected exactly one --config PATH");
    return;
  }
  if (hasConfig) {
    std::ifstream config(configPath, std::ios::binary);
    if (!config) {
      fail("cannot open config file");
      return;
    }
    configJson_.assign(std::istreambuf_iterator<char>(config),
                       std::istreambuf_iterator<char>());
    if (config.bad()) {
      fail("cannot read config file");
      return;
    }
  } else {
    configJson_.assign(embeddedConfig);
  }
  std::string_view configView(configJson_);
  if (configView.ends_with('\n'))
    configView.remove_suffix(1);
  RuntimeLimits limits;
  std::string parseError;
  if (!parseModelConfigJson(configView, limits, parseError)) {
    fail(parseError.empty() ? "invalid model config" : parseError);
    return;
  }
  if (limits.deadlockWindow) {
    fail("deadlock_window is unavailable in the foundation runtime");
    return;
  }
  const auto domainLimit = limits.maxDomainCycles.find("default");
  hasDomainLimit_ = !limits.maxDomainCycles.empty();
  const bool hasDefaultDomainLimit =
      domainLimit != limits.maxDomainCycles.end();
  if ((!limits.maxTicks || *limits.maxTicks == 0) &&
      (!hasDefaultDomainLimit || domainLimit->second == 0)) {
    fail("config requires a finite positive max_ticks or default domain limit");
    return;
  }
  if (hasDefaultDomainLimit && domainLimit->second == 0) {
    fail("default domain limit must be positive");
    return;
  }
  for (const auto &[name, value] : limits.maxDomainCycles) {
    if (name != "default" || value == 0) {
      fail("runtime limits are outside the foundation profile");
      return;
    }
  }
  if (limits.maxTicks && *limits.maxTicks == 0) {
    fail("max_ticks must be positive");
    return;
  }

  if (!eventPath.empty()) {
    if (eventPath == "-") {
      eventFile_ = stdout;
    } else if (!openEventSink(eventPath)) {
      return;
    }
  }
  ready_ = true;
}

SystemRunner::~SystemRunner() {
  if (ownsEventFile_ && eventFile_)
    std::fclose(eventFile_);
}

void SystemRunner::fail(std::string_view message) noexcept {
  try {
    std::cerr << "pycircuit_system: " << message << '\n';
  } catch (...) {
  }
  ready_ = false;
}

bool SystemRunner::openEventSink(const std::string &path) noexcept {
  if (!exclusiveCreate(path, eventFile_)) {
    fail("events path must be a new regular file");
    return false;
  }
  ownsEventFile_ = true;
  return true;
}

bool SystemRunner::writeRecord(std::string_view record) noexcept {
  if (!eventFile_)
    return true;
  const std::size_t written =
      std::fwrite(record.data(), 1, record.size(), eventFile_);
  return written == record.size() && std::fflush(eventFile_) == 0;
}

int SystemRunner::Run(SimSystem &system, ObservationSlots &observations,
                      std::span<const RunnerObservation> metadata,
                      RunnerCallbacks callbacks) {
  if (!ready_)
    return 2;
  if (callbacks.drive && hasDomainLimit_) {
    fail("driven runner requires max_ticks in sampling epochs; "
         "max_domain_cycles needs a clock-domain driver");
    return 2;
  }
  try {
    std::vector<ReportGaugeDescriptor> reports;
    if (!metadataMatches(metadata, observations.Descriptors(), reports)) {
      fail("observation metadata does not match configured slots");
      return 2;
    }

    SimExecutor executor(system, observations, reports);
    if (!executor.created()) {
      fail("model Build failed");
      return 2;
    }
    const auto *configBytes =
        reinterpret_cast<const std::uint8_t *>(configJson_.data());
    if (executor.ConfigureJson(configBytes, configJson_.size()) !=
        PYCIRCUIT_MODEL_STATUS_V1_OK) {
      fail("model configuration or reset failed");
      return 2;
    }
    if (callbacks.initialize)
      callbacks.initialize(callbacks.context);
    if (executor.Reset() != PYCIRCUIT_MODEL_STATUS_V1_OK) {
      fail("model configuration or reset failed");
      return 2;
    }

    std::vector<OutputItem> outputItems;
    const auto descriptors = observations.Descriptors();
    auto descriptorOrder = [&](std::uint32_t ordinal) {
      auto found = std::find_if(descriptors.begin(), descriptors.end(),
                                [&](const auto &descriptor) {
                                  return descriptor.stableOrdinal == ordinal;
                                });
      return found == descriptors.end()
                 ? descriptors.size()
                 : static_cast<std::size_t>(found - descriptors.begin());
    };
    auto findMetadata =
        [&](std::uint32_t ordinal) -> const RunnerObservation * {
      for (std::size_t index = 0; index < metadata.size(); ++index) {
        if (metadata[index].stableOrdinal == ordinal)
          return &metadata[index];
      }
      return nullptr;
    };

    std::string_view finalStatus;
    int resultCode = 0;
    while (true) {
      if (callbacks.drive &&
          !callbacks.drive(callbacks.context, executor.cycles())) {
        finalStatus = "TERMINATED";
        break;
      }
      PycircuitModelStepResultV1 step{sizeof(PycircuitModelStepResultV1)};
      const PycircuitModelStatusV1 status = executor.Step(&step);
      if (status != PYCIRCUIT_MODEL_STATUS_V1_OK ||
          step.state == PYCIRCUIT_MODEL_STEP_V1_FAILED) {
        finalStatus = "FAILED";
        resultCode = 1;
        break;
      }

      const std::uint64_t committedEpoch = executor.cycles();
      if (callbacks.sample && step.state != PYCIRCUIT_MODEL_STEP_V1_QUIESCENT)
        callbacks.sample(callbacks.context, committedEpoch);
      outputItems.clear();
      if (step.state != PYCIRCUIT_MODEL_STEP_V1_QUIESCENT) {
        for (const CommittedEventSlot &event : observations.Events()) {
          const RunnerObservation *entry =
              findMetadata(event.descriptor.stableOrdinal);
          if (!entry || entry->kind == "report") {
            fail("event slot has no matching event metadata");
            return 2;
          }
          const std::size_t order =
              descriptorOrder(event.descriptor.stableOrdinal);
          if (order == descriptors.size()) {
            fail("event slot has no matching observation descriptor");
            return 2;
          }
          outputItems.push_back({order, entry, &event, nullptr});
        }
        for (const GaugeSnapshot &gauge : observations.Gauges()) {
          if (gauge.lastUpdate != committedEpoch)
            continue;
          const RunnerObservation *entry = findMetadata(gauge.stableOrdinal);
          if (!entry || entry->kind != "report") {
            fail("active gauge has no matching report metadata");
            return 2;
          }
          const std::size_t order = descriptorOrder(gauge.stableOrdinal);
          if (order == descriptors.size()) {
            fail("active gauge has no matching observation descriptor");
            return 2;
          }
          outputItems.push_back({order, entry, nullptr, &gauge});
        }
        std::sort(outputItems.begin(), outputItems.end(),
                  [](const OutputItem &left, const OutputItem &right) {
                    return left.descriptorOrder < right.descriptorOrder;
                  });
        // Validate and serialize every active group before publishing any
        // event from this epoch. An incomplete later group cannot leak an
        // earlier scalar event. Entirely inactive groups produce no slots.
        std::vector<std::string> records;
        for (std::size_t begin = 0; begin < outputItems.size();) {
          const OutputItem &first = outputItems[begin];
          const std::size_t count = first.metadata->groupSize;
          if (first.metadata->groupIndex != 0 || count == 0 ||
              count > outputItems.size() - begin) {
            fail("observation group is incomplete");
            return 2;
          }
          for (std::size_t index = 0; index < count; ++index) {
            const OutputItem &entry = outputItems[begin + index];
            if (!sameObservationGroup(*first.metadata, *entry.metadata,
                                      index) ||
                entry.descriptorOrder != first.descriptorOrder + index ||
                observationEpoch(entry) != committedEpoch) {
              fail("observation group is incomplete or inconsistent");
              return 2;
            }
          }
          records.push_back(eventRecord(
              std::span<const OutputItem>(outputItems).subspan(begin, count)));
          begin += count;
        }
        for (const std::string &record : records)
          if (!writeRecord(record)) {
            fail("events sink write failed");
            return 1;
          }
      }

      if (step.state == PYCIRCUIT_MODEL_STEP_V1_TERMINATED) {
        finalStatus = "TERMINATED";
        break;
      }
      if (step.state == PYCIRCUIT_MODEL_STEP_V1_QUIESCENT) {
        finalStatus = "QUIESCENT";
        break;
      }
      if (step.state != PYCIRCUIT_MODEL_STEP_V1_RUNNING) {
        finalStatus = "FAILED";
        resultCode = 1;
        break;
      }
    }

    PycircuitModelBufferV1 statistics{};
    if (executor.StatisticsJson(&statistics) != PYCIRCUIT_MODEL_STATUS_V1_OK ||
        !statistics.data || statistics.size == 0) {
      fail("cannot obtain final statistics");
      return 1;
    }
    std::string statisticsJson(reinterpret_cast<const char *>(statistics.data),
                               static_cast<std::size_t>(statistics.size));
    if (statisticsJson.ends_with('\n'))
      statisticsJson.pop_back();
    PycircuitModelBufferV1 lastError{};
    if (executor.LastError(&lastError) != PYCIRCUIT_MODEL_STATUS_V1_OK) {
      fail("cannot obtain final error status");
      return 1;
    }
    const std::string errorJson =
        lastError.data && lastError.size
            ? std::string(reinterpret_cast<const char *>(lastError.data),
                          static_cast<std::size_t>(lastError.size))
            : std::string();

    const std::string record =
        resultRecord(finalStatus, executor.cycles(), statisticsJson, errorJson);
    if (eventFile_ && !writeRecord(record)) {
      fail("events sink write failed");
      return 1;
    }
    return resultCode;
  } catch (const std::exception &error) {
    fail(error.what());
    return 1;
  } catch (...) {
    fail("runtime runner failure");
    return 1;
  }
}

} // namespace gfsim
