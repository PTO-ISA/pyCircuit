#include "gfsim/harness.h"

#include "acir/Bindings/Binding.h"
#include "gfsim/trace.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <limits>
#include <set>
#include <string>
#include <system_error>
#include <tuple>
#include <utility>
#include <variant>

namespace gfsim {
namespace {

llvm::Error harnessError(const llvm::Twine &message) {
  return llvm::createStringError(llvm::errc::invalid_argument,
                                 "ACRUN-PREFLIGHT-001: " + message);
}

bool isFingerprint(llvm::StringRef value) {
  if (!value.consume_front("sha256:") || value.size() != 64)
    return false;
  return llvm::all_of(value, [](char character) {
    return (character >= '0' && character <= '9') ||
           (character >= 'a' && character <= 'f');
  });
}

bool isNormalizedRelativePath(llvm::StringRef value) {
  if (value.empty() || value.starts_with('/') || value.ends_with('/') ||
      value.contains('\\'))
    return false;
  while (!value.empty()) {
    auto [component, rest] = value.split('/');
    if (component.empty() || component == "." || component == "..")
      return false;
    value = rest;
  }
  return true;
}

bool isDomainName(llvm::StringRef value) {
  if (value.empty() ||
      !(std::isalpha(static_cast<unsigned char>(value.front())) ||
        value.front() == '_'))
    return false;
  return llvm::all_of(value.drop_front(), [](char character) {
    return std::isalnum(static_cast<unsigned char>(character)) ||
           character == '_' || character == '.' || character == '-';
  });
}

bool hasExactKeys(const llvm::json::Object &object,
                  llvm::ArrayRef<llvm::StringRef> keys) {
  if (object.size() != keys.size())
    return false;
  return llvm::all_of(keys,
                      [&](llvm::StringRef key) { return object.get(key); });
}

llvm::Expected<std::string> requiredString(const llvm::json::Object &object,
                                           llvm::StringRef key) {
  auto value = object.getString(key);
  if (!value || value->empty())
    return harnessError("field '" + key + "' must be a non-empty string");
  return value->str();
}

llvm::Expected<uint64_t> requiredUInt64(const llvm::json::Object &object,
                                        llvm::StringRef key,
                                        bool allowZero = true) {
  const llvm::json::Value *value = object.get(key);
  auto integer = value ? value->getAsUINT64() : std::nullopt;
  if (!integer || (!allowZero && *integer == 0))
    return harnessError("field '" + key + "' must be an unsigned integer");
  return *integer;
}

llvm::Expected<std::optional<uint64_t>>
optionalPositiveUInt64(const llvm::json::Object &object, llvm::StringRef key) {
  const llvm::json::Value *value = object.get(key);
  if (!value)
    return harnessError("field '" + key + "' is required");
  if (value->kind() == llvm::json::Value::Null)
    return std::optional<uint64_t>{};
  auto integer = value->getAsUINT64();
  if (!integer || *integer == 0)
    return harnessError("field '" + key + "' must be null or positive");
  return integer;
}

llvm::Expected<HarnessFileHash> parseFileHash(const llvm::json::Value *value,
                                              llvm::StringRef label) {
  const llvm::json::Object *object = value ? value->getAsObject() : nullptr;
  constexpr std::array<llvm::StringRef, 2> keys{"path", "sha256"};
  if (!object || !hasExactKeys(*object, keys))
    return harnessError(label + " must be an exact file-hash object");
  auto path = requiredString(*object, "path");
  auto hash = requiredString(*object, "sha256");
  if (!path || !hash)
    return path ? hash.takeError() : path.takeError();
  if (!isNormalizedRelativePath(*path) || !isFingerprint(*hash))
    return harnessError(label + " path or fingerprint is invalid");
  return HarnessFileHash{std::move(*path), std::move(*hash)};
}

llvm::Expected<TraceIdentity>
parseTraceIdentity(const llvm::json::Value *value) {
  const llvm::json::Object *object = value ? value->getAsObject() : nullptr;
  constexpr std::array<llvm::StringRef, 4> keys{"path", "schema", "version",
                                                "sha256"};
  if (!object || !hasExactKeys(*object, keys))
    return harnessError("trace must be an exact identity object");
  auto path = requiredString(*object, "path");
  auto schema = requiredString(*object, "schema");
  auto version = requiredString(*object, "version");
  auto hash = requiredString(*object, "sha256");
  if (!path || !schema || !version || !hash) {
    if (!path)
      return path.takeError();
    if (!schema)
      return schema.takeError();
    if (!version)
      return version.takeError();
    return hash.takeError();
  }
  if (!isNormalizedRelativePath(*path) || *schema != "pto-trace" ||
      *version != "0.1" || !isFingerprint(*hash))
    return harnessError("trace identity is invalid");
  return TraceIdentity{std::move(*path), std::move(*schema),
                       std::move(*version), std::move(*hash)};
}

llvm::Expected<std::string> readFile(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  if (!buffer)
    return llvm::createStringError(buffer.getError(),
                                   "ACRUN-PREFLIGHT-001: cannot read '%s'",
                                   path.str().c_str());
  return buffer.get()->getBuffer().str();
}

llvm::Expected<std::string> resolvedInputPath(const RunManifest &manifest,
                                              llvm::StringRef relative) {
  if (!isNormalizedRelativePath(relative))
    return harnessError("input path is not normalized and relative");
  std::filesystem::path root =
      std::filesystem::weakly_canonical(manifest.rootDirectory);
  std::filesystem::path candidate =
      std::filesystem::weakly_canonical(root / relative.str());
  auto [rootEnd, candidateEnd] = std::mismatch(
      root.begin(), root.end(), candidate.begin(), candidate.end());
  if (rootEnd != root.end())
    return harnessError("input path escapes the run-manifest root");
  return candidate.string();
}

llvm::Error writeExclusive(llvm::StringRef root, llvm::StringRef relative,
                           llvm::StringRef bytes) {
  llvm::SmallString<256> path(root);
  llvm::sys::path::append(path, relative);
  llvm::SmallString<256> parent(path);
  llvm::sys::path::remove_filename(parent);
  if (std::error_code error = llvm::sys::fs::create_directories(parent))
    return llvm::createStringError(error, "cannot create result directory");
  int descriptor = -1;
  if (std::error_code error = llvm::sys::fs::openFileForWrite(
          path, descriptor, llvm::sys::fs::CD_CreateNew,
          llvm::sys::fs::OF_None))
    return llvm::createStringError(error, "cannot create result artifact");
  llvm::raw_fd_ostream output(descriptor, true);
  output << bytes;
  output.flush();
  if (output.has_error())
    return llvm::createStringError(output.error(),
                                   "cannot write result artifact");
  return llvm::Error::success();
}

llvm::StringRef statusName(RunStatus status) {
  switch (status) {
  case RunStatus::Completed:
    return "completed";
  case RunStatus::Incomplete:
    return "incomplete";
  case RunStatus::Failed:
    return "failed";
  }
  return "failed";
}

bool expectationMatches(const TerminationExpectation &expectation,
                        const RunResultDocument &result) {
  const bool kindMatches = expectation.kind == "any" ||
                           (expectation.kind == "complete" &&
                            result.status == RunStatus::Completed) ||
                           (expectation.kind == "incomplete" &&
                            result.status == RunStatus::Incomplete);
  return kindMatches && (!expectation.reason ||
                         *expectation.reason == result.terminationReason);
}

llvm::Error validateResultDocument(const RunResultDocument &result) {
  constexpr std::array<llvm::StringRef, 2> completedReasons{
      "trace_drained", "declared_model_termination"};
  constexpr std::array<llvm::StringRef, 7> incompleteReasons{
      "max_ticks",           "max_domain_cycles", "max_events",
      "max_deltas_per_tick", "max_trace_records", "max_validation_work",
      "interrupted"};
  constexpr std::array<llvm::StringRef, 4> failedReasons{
      "deadlock", "invariant_violation", "trace_error", "runtime_error"};
  llvm::ArrayRef<llvm::StringRef> allowed;
  switch (result.status) {
  case RunStatus::Completed:
    allowed = completedReasons;
    break;
  case RunStatus::Incomplete:
    allowed = incompleteReasons;
    break;
  case RunStatus::Failed:
    allowed = failedReasons;
    break;
  }
  if (!llvm::is_contained(allowed, result.terminationReason))
    return harnessError("run result status and termination reason disagree");
  if (!isNormalizedRelativePath(result.runManifest.path) ||
      !isFingerprint(result.runManifest.sha256))
    return harnessError("run result manifest identity is invalid");
  std::string previous;
  for (const auto &[name, count] : result.domainCycles)
    if (!isDomainName(name))
      return harnessError("run result contains an invalid domain name");
  for (const HarnessFileHash &output : result.outputs) {
    if (!isNormalizedRelativePath(output.path) ||
        !isFingerprint(output.sha256) ||
        (!previous.empty() && previous >= output.path))
      return harnessError("run result outputs are not canonical");
    previous = output.path;
  }
  if (result.validation.status != "passed" &&
      result.validation.status != "failed" &&
      result.validation.status != "not_run")
    return harnessError("run result validation status is invalid");
  if ((result.validation.status == "not_run") ==
      result.validation.reportSha256.has_value())
    return harnessError("run result validation report identity is invalid");
  if (result.validation.reportSha256 &&
      !isFingerprint(*result.validation.reportSha256))
    return harnessError("run result validation hash is invalid");
  return llvm::Error::success();
}

llvm::Expected<std::string> canonicalResult(const RunResultDocument &result) {
  llvm::json::Object cycles;
  for (const auto &[name, count] : result.domainCycles)
    cycles[name] = count;
  llvm::json::Array outputs;
  for (const HarnessFileHash &output : result.outputs)
    outputs.push_back(
        llvm::json::Object{{"path", output.path}, {"sha256", output.sha256}});
  llvm::json::Object document{
      {"schema", "agentic-circuit-run-result"},
      {"version", "0.1"},
      {"contract_epoch", "0.5"},
      {"run_manifest",
       llvm::json::Object{{"path", result.runManifest.path},
                          {"sha256", result.runManifest.sha256}}},
      {"status", statusName(result.status)},
      {"termination_reason", result.terminationReason},
      {"simulated_ticks", result.simulatedTicks},
      {"domain_cycles", std::move(cycles)},
      {"event_count", result.eventCount},
      {"trace_position",
       llvm::json::Object{
           {"next_record_index", result.tracePosition.nextRecordIndex},
           {"last_committed_sequence_id",
            result.tracePosition.lastCommittedSequenceId
                ? llvm::json::Value(
                      *result.tracePosition.lastCommittedSequenceId)
                : llvm::json::Value(nullptr)}}},
      {"outputs", std::move(outputs)},
      {"validation",
       llvm::json::Object{
           {"status", result.validation.status},
           {"report_sha256",
            result.validation.reportSha256
                ? llvm::json::Value(*result.validation.reportSha256)
                : llvm::json::Value(nullptr)}}}};
  return acir::bindings::canonicalizeJson(
      llvm::json::Value(std::move(document)));
}

llvm::StringRef statisticKindName(StatisticKind kind) {
  switch (kind) {
  case StatisticKind::Counter:
    return "counter";
  case StatisticKind::Gauge:
    return "gauge";
  case StatisticKind::Histogram:
    return "histogram";
  }
  return "counter";
}

llvm::Expected<std::string>
canonicalStatistics(std::span<const StatSnapshot> statistics) {
  llvm::json::Array output;
  std::pair<std::string_view, std::string_view> previous;
  bool hasPrevious = false;
  for (const StatSnapshot &snapshot : statistics) {
    const auto key = std::pair(std::string_view(snapshot.objectPath),
                               std::string_view(snapshot.name));
    if (snapshot.objectPath.empty() || snapshot.name.empty() ||
        (hasPrevious && previous >= key))
      return harnessError("statistics are not in canonical path/name order");
    previous = key;
    hasPrevious = true;
    llvm::json::Array buckets;
    uint64_t previousBound = 0;
    bool hasBound = false;
    for (const HistogramBucket &bucket : snapshot.buckets) {
      if (hasBound && previousBound >= bucket.upperBound)
        return harnessError("histogram bounds are not strictly increasing");
      previousBound = bucket.upperBound;
      hasBound = true;
      buckets.push_back(llvm::json::Object{{"upper_bound", bucket.upperBound},
                                           {"count", bucket.count}});
    }
    output.push_back(llvm::json::Object{
        {"name", snapshot.name},
        {"object_path", snapshot.objectPath},
        {"kind", statisticKindName(snapshot.kind)},
        {"value", snapshot.value},
        {"count", snapshot.count},
        {"sum", snapshot.sum},
        {"minimum", snapshot.minimum},
        {"maximum", snapshot.maximum},
        {"buckets", std::move(buckets)},
        {"last_update",
         llvm::json::Object{{"time", snapshot.lastUpdate.time},
                            {"delta", snapshot.lastUpdate.delta}}}});
  }
  auto bytes =
      acir::bindings::canonicalizeJson(llvm::json::Value(std::move(output)));
  if (bytes)
    bytes->push_back('\n');
  return bytes;
}

llvm::StringRef tracePhaseName(TraceEventPhase phase) {
  switch (phase) {
  case TraceEventPhase::Instant:
    return "i";
  case TraceEventPhase::Complete:
    return "X";
  case TraceEventPhase::Counter:
    return "C";
  case TraceEventPhase::FlowStart:
    return "s";
  case TraceEventPhase::FlowEnd:
    return "f";
  }
  return "i";
}

llvm::json::Value observationValue(const ObservationValue &value) {
  return std::visit([](const auto &item) -> llvm::json::Value { return item; },
                    value);
}

llvm::Expected<std::string>
canonicalEvents(std::span<const CommittedEvent> events) {
  std::string output;
  std::optional<std::tuple<Epoch, ObjectId, uint64_t>> previous;
  for (const CommittedEvent &event : events) {
    const auto key =
        std::tuple(event.epoch, event.ownerId, event.localCommittedIndex);
    if (event.ownerId == kInvalidObjectId ||
        event.epoch.delta >= kMaxDeltasPerTick ||
        (previous && *previous >= key))
      return harnessError("events are not in canonical committed order");
    previous = key;
    if (event.epoch.time >
        (std::numeric_limits<uint64_t>::max() - event.epoch.delta) /
            kMaxDeltasPerTick)
      return harnessError("event presentation timestamp overflowed");
    const uint64_t timestamp =
        event.epoch.time * kMaxDeltasPerTick + event.epoch.delta;
    llvm::json::Object arguments{
        {"gfsim_epoch_time", event.epoch.time},
        {"gfsim_epoch_delta", event.epoch.delta},
        {"gfsim_object_id", event.ownerId},
        {"gfsim_local_committed_index", event.localCommittedIndex},
    };
    if (event.rootSequenceId)
      arguments["gfsim_root_sequence_id"] = *event.rootSequenceId;
    for (const ObservationArgument &argument : event.arguments) {
      if (llvm::StringRef(argument.name).starts_with("gfsim_") ||
          arguments.get(argument.name))
        return harnessError("event argument collides with runtime metadata");
      arguments[argument.name] = observationValue(argument.value);
    }
    llvm::json::Object encoded{{"name", event.name},
                               {"cat", event.category},
                               {"ph", tracePhaseName(event.phase)},
                               {"ts", timestamp},
                               {"pid", 0},
                               {"tid", event.ownerId},
                               {"args", std::move(arguments)}};
    if (event.phase == TraceEventPhase::Instant)
      encoded["s"] = "t";
    if (event.duration)
      encoded["dur"] = *event.duration;
    if (event.flowId)
      encoded["id"] = *event.flowId;
    if (event.phase == TraceEventPhase::FlowEnd)
      encoded["bp"] = "e";
    auto line =
        acir::bindings::canonicalizeJson(llvm::json::Value(std::move(encoded)));
    if (!line)
      return line.takeError();
    output.append(*line).push_back('\n');
  }
  return output;
}

} // namespace

llvm::Expected<RunManifest> loadRunManifest(llvm::StringRef bytes,
                                            llvm::StringRef rootDirectory) {
  auto parsed = acir::bindings::parseIJson(bytes);
  if (!parsed)
    return parsed.takeError();
  const llvm::json::Object *object = parsed->getAsObject();
  constexpr std::array<llvm::StringRef, 13> keys{"schema",
                                                 "version",
                                                 "contract_epoch",
                                                 "build_manifest",
                                                 "trace",
                                                 "seed",
                                                 "output_directory",
                                                 "deadlock_window",
                                                 "max_ticks",
                                                 "max_domain_cycles",
                                                 "stats_format",
                                                 "event_log",
                                                 "termination_expectation"};
  if (!object || !hasExactKeys(*object, keys) ||
      object->getString("schema") != "agentic-circuit-run-manifest" ||
      object->getString("version") != "0.1" ||
      object->getString("contract_epoch") != "0.5")
    return harnessError("run manifest has an invalid closed envelope");

  RunManifest manifest;
  auto build = parseFileHash(object->get("build_manifest"), "build_manifest");
  auto trace = parseTraceIdentity(object->get("trace"));
  auto seed = requiredUInt64(*object, "seed");
  auto output = requiredString(*object, "output_directory");
  auto deadlock = optionalPositiveUInt64(*object, "deadlock_window");
  auto maxTicks = optionalPositiveUInt64(*object, "max_ticks");
  auto stats = requiredString(*object, "stats_format");
  auto eventLog = requiredString(*object, "event_log");
  if (!build || !trace || !seed || !output || !deadlock || !maxTicks ||
      !stats || !eventLog) {
    if (!build)
      return build.takeError();
    if (!trace)
      return trace.takeError();
    if (!seed)
      return seed.takeError();
    if (!output)
      return output.takeError();
    if (!deadlock)
      return deadlock.takeError();
    if (!maxTicks)
      return maxTicks.takeError();
    if (!stats)
      return stats.takeError();
    return eventLog.takeError();
  }
  if (!isNormalizedRelativePath(*output) || *stats != "json" ||
      (*eventLog != "disabled" && *eventLog != "jsonl"))
    return harnessError("run output configuration is invalid");

  const llvm::json::Object *domainLimits =
      object->getObject("max_domain_cycles");
  if (!domainLimits)
    return harnessError("max_domain_cycles must be an object");
  for (const auto &[name, value] : *domainLimits) {
    auto maximum = value.getAsUINT64();
    if (!isDomainName(name) || !maximum || *maximum == 0)
      return harnessError("max_domain_cycles contains an invalid bound");
    manifest.limits.maxDomainCycles.emplace(name.str(), *maximum);
  }

  const llvm::json::Object *expectation =
      object->getObject("termination_expectation");
  constexpr std::array<llvm::StringRef, 2> expectationKeys{"kind", "reason"};
  if (!expectation || !hasExactKeys(*expectation, expectationKeys))
    return harnessError("termination_expectation is invalid");
  auto kind = expectation->getString("kind");
  const llvm::json::Value *reasonValue = expectation->get("reason");
  if (!kind ||
      (*kind != "complete" && *kind != "incomplete" && *kind != "any") ||
      !reasonValue)
    return harnessError("termination expectation kind is invalid");
  std::optional<std::string> reason;
  if (reasonValue->kind() != llvm::json::Value::Null) {
    auto value = reasonValue->getAsString();
    if (!value || (*value != "trace_drained" && *value != "max_ticks" &&
                   *value != "max_domain_cycles" &&
                   *value != "declared_model_termination"))
      return harnessError("termination expectation reason is invalid");
    reason = value->str();
  }
  if (reason && ((*kind == "complete" && *reason != "trace_drained" &&
                  *reason != "declared_model_termination") ||
                 (*kind == "incomplete" && *reason != "max_ticks" &&
                  *reason != "max_domain_cycles")))
    return harnessError(
        "termination expectation kind and reason are incompatible");

  manifest.buildManifest = std::move(*build);
  manifest.trace = std::move(*trace);
  manifest.seed = *seed;
  manifest.outputDirectory = std::move(*output);
  manifest.limits.deadlockWindow = *deadlock;
  manifest.limits.maxTicks = *maxTicks;
  manifest.statsFormat = std::move(*stats);
  manifest.eventLog = std::move(*eventLog);
  manifest.expectation = {kind->str(), std::move(reason)};
  manifest.rootDirectory =
      std::filesystem::weakly_canonical(rootDirectory.str()).string();
  manifest.manifestSha256 = acir::bindings::sha256Fingerprint(bytes);
  return manifest;
}

llvm::Expected<PtoTraceDocument>
preflightRunManifest(const RunManifest &manifest,
                     llvm::StringRef buildFingerprint,
                     std::span<const TimeDomainRuntime> timeDomains,
                     llvm::StringRef resultStage) {
  if (!isFingerprint(buildFingerprint))
    return harnessError("generated build fingerprint is invalid");
  auto buildPath = resolvedInputPath(manifest, manifest.buildManifest.path);
  if (!buildPath)
    return buildPath.takeError();
  auto buildBytes = readFile(*buildPath);
  if (!buildBytes)
    return buildBytes.takeError();
  if (acir::bindings::sha256Fingerprint(*buildBytes) !=
      manifest.buildManifest.sha256)
    return harnessError("build manifest hash does not match");
  auto buildDocument = acir::bindings::parseIJson(*buildBytes);
  const llvm::json::Object *build =
      buildDocument ? buildDocument->getAsObject() : nullptr;
  if (!buildDocument)
    return buildDocument.takeError();
  if (!build ||
      !hasExactKeys(*build,
                    {"schema", "version", "contract_epoch", "project", "system",
                     "source_files", "normalized_acir_sha256", "compiler",
                     "pass_pipeline", "providers", "component_specializations",
                     "protocol_identities", "artifacts", "validation_gates",
                     "build_profile", "instrumentation_layers",
                     "specialization_inputs", "build_fingerprint"}) ||
      build->getString("schema") != "agentic-circuit-build-manifest" ||
      build->getString("version") != "0.1" ||
      build->getString("contract_epoch") != "0.5" ||
      build->getString("build_fingerprint") != buildFingerprint)
    return harnessError("build manifest identity does not match the model");

  std::set<llvm::StringRef> knownDomains;
  for (const TimeDomainRuntime &domain : timeDomains) {
    if (!isDomainName(domain.name) || domain.period == 0 ||
        domain.tickScale == 0 || !knownDomains.insert(domain.name).second)
      return harnessError("generated time-domain metadata is invalid");
  }
  for (const auto &[name, maximum] : manifest.limits.maxDomainCycles)
    if (!knownDomains.contains(name))
      return harnessError("max_domain_cycles names unknown domain '" + name +
                          "'");

  auto tracePath = resolvedInputPath(manifest, manifest.trace.path);
  if (!tracePath)
    return tracePath.takeError();
  auto traceBytes = readFile(*tracePath);
  if (!traceBytes)
    return traceBytes.takeError();
  if (acir::bindings::sha256Fingerprint(*traceBytes) != manifest.trace.sha256)
    return harnessError("trace hash does not match");
  TraceLoadResult trace = parsePtoTrace(*traceBytes);
  if (!trace.succeeded())
    return harnessError("trace document failed exact preflight");

  std::filesystem::path root =
      std::filesystem::weakly_canonical(manifest.rootDirectory);
  auto outputPath = resolvedInputPath(manifest, manifest.outputDirectory);
  if (!outputPath)
    return outputPath.takeError();
  std::filesystem::path stage =
      std::filesystem::weakly_canonical(resultStage.str());
  auto [rootEnd, stageEnd] =
      std::mismatch(root.begin(), root.end(), stage.begin(), stage.end());
  if (rootEnd != root.end() || stage == root)
    return harnessError("result stage escapes the run-manifest root");
  return std::move(*trace.document);
}

RunResultDocument makeRunResult(const RunManifest &manifest,
                                const TerminationResult &termination) {
  RunResultDocument result;
  result.runManifest = {"run-manifest.json", manifest.manifestSha256};
  result.simulatedTicks = termination.finalEpoch.time;
  result.domainCycles = termination.domainCycles;
  result.eventCount = termination.committedEventCount;
  result.tracePosition = {termination.tracePosition,
                          termination.traceLastCommittedSequenceId};
  switch (termination.classification) {
  case TerminationClass::Completed:
    result.status = RunStatus::Completed;
    result.terminationReason =
        termination.diagnosticCode == "declared_model_termination"
            ? "declared_model_termination"
            : "trace_drained";
    break;
  case TerminationClass::Incomplete:
    result.status = RunStatus::Incomplete;
    if (termination.diagnosticCode == "max_ticks_reached")
      result.terminationReason = "max_ticks";
    else if (termination.diagnosticCode == "max_domain_cycles_reached")
      result.terminationReason = "max_domain_cycles";
    else if (termination.diagnosticCode == "max_events_reached")
      result.terminationReason = "max_events";
    else if (termination.diagnosticCode == "max_trace_records_reached")
      result.terminationReason = "max_trace_records";
    else
      result.terminationReason = "interrupted";
    break;
  case TerminationClass::Failed:
    result.status = RunStatus::Failed;
    result.terminationReason =
        termination.diagnosticCode == "no_progress" ||
                termination.diagnosticCode == "deadlock_window_reached"
            ? "deadlock"
            : "invariant_violation";
    break;
  }
  result.validation = {"not_run", std::nullopt};
  return result;
}

llvm::Error publishRunResult(const RunManifest &manifest,
                             RunResultDocument &result,
                             std::span<const StatSnapshot> statistics,
                             std::span<const CommittedEvent> events,
                             llvm::StringRef resultStage) {
  auto stats = canonicalStatistics(statistics);
  if (!stats)
    return stats.takeError();
  llvm::Expected<std::string> eventBytes = std::string();
  if (manifest.eventLog == "jsonl") {
    eventBytes = canonicalEvents(events);
    if (!eventBytes)
      return eventBytes.takeError();
  }
  if (llvm::sys::fs::exists(resultStage))
    return harnessError("result stage already exists");
  if (std::error_code error = llvm::sys::fs::create_directories(resultStage))
    return llvm::createStringError(error, "cannot create result stage");
  struct Cleanup {
    llvm::StringRef path;
    bool keep = false;
    ~Cleanup() {
      if (!keep)
        llvm::sys::fs::remove_directories(path);
    }
  } cleanup{resultStage};

  if (llvm::Error error = writeExclusive(resultStage, "stats.json", *stats))
    return error;
  result.outputs.push_back(
      {"stats.json", acir::bindings::sha256Fingerprint(*stats)});
  if (manifest.eventLog == "jsonl") {
    if (llvm::Error error =
            writeExclusive(resultStage, "events.jsonl", *eventBytes))
      return error;
    result.outputs.push_back(
        {"events.jsonl", acir::bindings::sha256Fingerprint(*eventBytes)});
  }
  const bool expectationPassed =
      expectationMatches(manifest.expectation, result);
  std::string validation =
      expectationPassed ? "{\"status\":\"passed\"}\n"
                        : "{\"diagnostic_code\":\"ACRUN-EXPECTATION-001\","
                          "\"status\":\"failed\"}\n";
  if (llvm::Error error =
          writeExclusive(resultStage, "validation-report.json", validation))
    return error;
  result.validation = {expectationPassed ? "passed" : "failed",
                       acir::bindings::sha256Fingerprint(validation)};
  if (!expectationPassed) {
    result.status = RunStatus::Failed;
    result.terminationReason = "invariant_violation";
  }
  result.outputs.push_back(
      {"validation-report.json", *result.validation.reportSha256});
  std::sort(result.outputs.begin(), result.outputs.end(),
            [](const HarnessFileHash &left, const HarnessFileHash &right) {
              return left.path < right.path;
            });
  if (llvm::Error error = validateResultDocument(result))
    return error;
  auto bytes = canonicalResult(result);
  if (!bytes)
    return bytes.takeError();
  bytes->push_back('\n');
  if (llvm::Error error =
          writeExclusive(resultStage, "run-result.json", *bytes))
    return error;
  cleanup.keep = true;
  return llvm::Error::success();
}

} // namespace gfsim
