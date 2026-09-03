#ifndef GFSIM_HARNESS_H
#define GFSIM_HARNESS_H

#include "gfsim/core.h"
#include "gfsim/trace.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/Error.h"

#include <concepts>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gfsim {

struct HarnessFileHash {
  std::string path;
  std::string sha256;
};

struct TraceIdentity {
  std::string path;
  std::string schema;
  std::string version;
  std::string sha256;
};

struct TerminationExpectation {
  std::string kind;
  std::optional<std::string> reason;
};

struct RunManifest {
  HarnessFileHash buildManifest;
  TraceIdentity trace;
  uint64_t seed = 0;
  std::string outputDirectory;
  RuntimeLimits limits;
  std::string statsFormat;
  std::string eventLog;
  TerminationExpectation expectation;

  // Cold-path resolution identity, not part of the public JSON document.
  std::string rootDirectory;
  std::string manifestSha256;
};

enum class RunStatus { Completed, Incomplete, Failed };

struct ValidationResult {
  std::string status;
  std::optional<std::string> reportSha256;
};

struct RunResultDocument {
  HarnessFileHash runManifest;
  RunStatus status = RunStatus::Failed;
  std::string terminationReason;
  uint64_t simulatedTicks = 0;
  std::map<std::string, uint64_t> domainCycles;
  uint64_t eventCount = 0;
  TracePosition tracePosition;
  std::vector<HarnessFileHash> outputs;
  ValidationResult validation;
};

llvm::Expected<RunManifest> loadRunManifest(llvm::StringRef bytes,
                                            llvm::StringRef rootDirectory);

llvm::Expected<PtoTraceDocument>
preflightRunManifest(const RunManifest &manifest,
                     llvm::StringRef buildFingerprint,
                     std::span<const TimeDomainRuntime> timeDomains,
                     llvm::StringRef resultStage);

RunResultDocument makeRunResult(const RunManifest &manifest,
                                const TerminationResult &termination);

llvm::Error publishRunResult(const RunManifest &manifest,
                             RunResultDocument &result,
                             std::span<const StatSnapshot> statistics,
                             std::span<const CommittedEvent> events,
                             llvm::StringRef resultStage);

template <typename Model>
llvm::Expected<RunResultDocument> runGeneratedModel(Model &model,
                                                    const RunManifest &manifest,
                                                    llvm::StringRef resultStage)
  requires requires(Model &value, const RuntimeLimits &limits) {
    { value.loadTrace(PtoTraceDocument{}) } -> std::same_as<bool>;
    value.configure(limits);
    { value.run() } -> std::same_as<TerminationResult>;
    { value.buildFingerprint() } -> std::convertible_to<std::string_view>;
    {
      value.timeDomains()
    } -> std::convertible_to<std::span<const TimeDomainRuntime>>;
    { value.statistics() } -> std::same_as<std::vector<StatSnapshot>>;
    {
      value.observations()
    } -> std::convertible_to<std::span<const CommittedEvent>>;
  }
{
  auto trace = preflightRunManifest(
      manifest, std::string_view(model.buildFingerprint()),
      std::span<const TimeDomainRuntime>(model.timeDomains()), resultStage);
  if (!trace)
    return trace.takeError();
  if (!model.loadTrace(std::move(*trace)))
    return llvm::createStringError(llvm::errc::invalid_argument,
                                   "ACRUN-PREFLIGHT-001: generated model "
                                   "rejected the validated trace document");
  model.configure(manifest.limits);
  RunResultDocument result = makeRunResult(manifest, model.run());
  std::vector<StatSnapshot> statistics = model.statistics();
  std::span<const CommittedEvent> events = model.observations();
  if (llvm::Error error =
          publishRunResult(manifest, result, statistics, events, resultStage))
    return std::move(error);
  return result;
}

} // namespace gfsim

#endif // GFSIM_HARNESS_H
