#include "acir/Bindings/Registry.h"

#include "BindingInternal.h"
#include "BindingTestHooks.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <array>
#include <optional>
#include <tuple>

namespace acir::bindings {
namespace {

llvm::Error registryError(const llvm::Twine &message) {
  return llvm::createStringError(llvm::errc::invalid_argument,
                                 "ACLOWER-BINDING-REGISTRY: %s",
                                 message.str().c_str());
}

llvm::Error resolutionError(llvm::StringRef code, const BindingRequest &request,
                            const llvm::Twine &detail) {
  return llvm::createStringError(llvm::errc::invalid_argument,
                                 "%s: key=%s binding=%s %s", code.str().c_str(),
                                 request.resolutionKey.c_str(),
                                 request.binding.c_str(), detail.str().c_str());
}

llvm::Error outputError(const llvm::Twine &message) {
  return llvm::createStringError(llvm::errc::io_error,
                                 "ACLOWER-BINDING-OUTPUT: %s",
                                 message.str().c_str());
}

bool hasExactKeys(const llvm::json::Object &object,
                  llvm::ArrayRef<llvm::StringRef> keys) {
  return object.size() == keys.size() &&
         llvm::all_of(keys,
                      [&](llvm::StringRef key) { return object.get(key); });
}

llvm::Expected<std::string> requestString(const llvm::json::Object &object,
                                          llvm::StringRef key) {
  auto value = object.getString(key);
  if (!value)
    return registryError(llvm::Twine("request field '") + key +
                         "' must be a string");
  return value->str();
}

bool isSha256(llvm::StringRef value) {
  return value.starts_with("sha256:") && value.size() == 71 &&
         llvm::all_of(value.drop_front(7), [](char character) {
           return llvm::isDigit(character) ||
                  (character >= 'a' && character <= 'f');
         });
}

bool isName(llvm::StringRef value) {
  if (value.empty() || !(llvm::isAlpha(value.front()) || value.front() == '_'))
    return false;
  return llvm::all_of(value.drop_front(), [](char character) {
    return llvm::isAlnum(character) || character == '_';
  });
}

bool isIdentity(llvm::StringRef value) {
  if (value.empty())
    return false;
  while (true) {
    auto [segment, remainder] = value.split('.');
    if (!isName(segment))
      return false;
    if (remainder.empty())
      return true;
    value = remainder;
  }
}

bool isResolutionKey(llvm::StringRef value) {
  while (true) {
    if (!value.consume_front("@"))
      return false;
    auto [segment, remainder] = value.split("::");
    if (!isName(segment))
      return false;
    if (remainder.empty())
      return true;
    value = remainder;
  }
}

bool isSelectionToken(llvm::StringRef value) {
  return !value.empty() && llvm::all_of(value, [](char character) {
    return llvm::isAlnum(character) || character == '_' || character == '.' ||
           character == '+' || character == '-';
  });
}

template <typename Record, typename Parser>
llvm::Expected<std::vector<Record>>
parseRequestArray(const llvm::json::Object &object, llvm::StringRef key,
                  Parser parse) {
  const auto *array = object.getArray(key);
  if (!array)
    return registryError(llvm::Twine("request field '") + key +
                         "' must be an array");
  std::vector<Record> records;
  records.reserve(array->size());
  for (const llvm::json::Value &value : *array) {
    const auto *entry = value.getAsObject();
    if (!entry)
      return registryError(llvm::Twine("request field '") + key +
                           "' must contain records");
    auto record = parse(*entry);
    if (!record)
      return record.takeError();
    records.push_back(std::move(*record));
  }
  return records;
}

bool matchesParameters(const BindingRecord &record,
                       const BindingRequest &request) {
  if (record.parameters().size() != request.parameters.size())
    return false;
  for (auto [candidate, required] :
       llvm::zip_equal(record.parameters(), request.parameters))
    if (candidate.acirType != required.acirType ||
        candidate.name != required.name ||
        candidate.ordinal != required.ordinal ||
        candidate.value != required.value)
      return false;
  return true;
}

bool matchesResults(const BindingRecord &record,
                    const BindingRequest &request) {
  if (record.results().size() != request.results.size())
    return false;
  for (auto [candidate, required] :
       llvm::zip_equal(record.results(), request.results))
    if (candidate.name != required.name)
      return false;
  return true;
}

bool matchesPorts(const BindingRecord &record, const BindingRequest &request) {
  if (record.ports().size() != request.ports.size())
    return false;
  for (auto [candidate, required] :
       llvm::zip_equal(record.ports(), request.ports))
    if (candidate.cardinality != required.cardinality ||
        candidate.delegation != required.delegation ||
        candidate.direction != required.direction ||
        candidate.interface != required.interface ||
        candidate.ownership != required.ownership ||
        candidate.payload != required.payload ||
        candidate.protocol != required.protocol ||
        candidate.role != required.role ||
        candidate.timeDomain != required.timeDomain)
      return false;
  return true;
}

bool matchesResources(const BindingRecord &record,
                      const BindingRequest &request) {
  if (record.resources().size() != request.resources.size())
    return false;
  for (auto [candidate, required] :
       llvm::zip_equal(record.resources(), request.resources))
    if (candidate.delegation != required.delegation ||
        candidate.mode != required.mode ||
        candidate.ownership != required.ownership ||
        candidate.resource != required.resource ||
        candidate.role != required.role ||
        candidate.timeDomain != required.timeDomain)
      return false;
  return true;
}

bool matchesMetadata(const BindingRecord &record,
                     const BindingRequest &request) {
  return record.bindingSchema() == request.bindingSchema &&
         record.contractEpoch() == request.contractEpoch &&
         record.binding() == request.binding &&
         record.componentSchema() == request.componentSchema &&
         record.componentSchemaFingerprint() ==
             request.componentSchemaFingerprint &&
         record.effect() == request.effect &&
         record.provider() == request.provider &&
         record.providerImplementationFingerprint() ==
             request.providerImplementationFingerprint &&
         matchesParameters(record, request) && matchesPorts(record, request) &&
         matchesResources(record, request) && matchesResults(record, request) &&
         record.activationSources() ==
             llvm::ArrayRef<ActivationSourceBinding>(request.activationSources);
}

llvm::Error validateRequest(const BindingRequest &request,
                            llvm::StringRef profile, llvm::StringRef target) {
  if (request.contractEpoch != ContractEpoch)
    return resolutionError("ACLOWER-EPOCH-MISMATCH", request,
                           llvm::Twine("expected=") + ContractEpoch);
  if (request.bindingSchema != BindingSchema)
    return resolutionError("ACLOWER-SCHEMA-MISMATCH", request,
                           llvm::Twine("expected=") + BindingSchema);
  if (request.effect != "pure" && request.effect != "stateful")
    return resolutionError("ACLOWER-INLINE-EFFECT", request,
                           "effect must be pure or stateful");
  if (!isSha256(request.componentSchemaFingerprint) ||
      !isSha256(request.providerImplementationFingerprint))
    return resolutionError("ACLOWER-FINGERPRINT", request,
                           "request fingerprints are malformed");
  if (!isSelectionToken(profile) || !isSelectionToken(target))
    return resolutionError("ACLOWER-PROFILE", request,
                           "profile or target syntax is invalid");
  if (!isResolutionKey(request.resolutionKey) || !isName(request.binding) ||
      !isIdentity(request.componentSchema) || !isIdentity(request.provider) ||
      request.functionType.empty())
    return resolutionError("ACLOWER-BINDING-MISSING", request,
                           "request identity is malformed");

  llvm::StringSet<> parameterNames;
  for (size_t index = 0; index < request.parameters.size(); ++index) {
    const ParameterRequirement &parameter = request.parameters[index];
    if (parameter.ordinal != static_cast<int64_t>(index) ||
        !isName(parameter.name) || parameter.acirType.empty() ||
        !parameterNames.insert(parameter.name).second)
      return resolutionError("ACLOWER-PARAM-PHASE", request,
                             "request parameter metadata is malformed");
    if (auto canonical = canonicalizeJson(parameter.value); !canonical)
      return resolutionError("ACLOWER-PARAM-PHASE", request,
                             llvm::toString(canonical.takeError()));
  }

  llvm::StringSet<> resultNames;
  for (const ResultRequirement &result : request.results)
    if (result.acirType.empty() || !isName(result.name) ||
        !resultNames.insert(result.name).second)
      return resolutionError("ACLOWER-TYPE-MISMATCH", request,
                             "request result metadata is malformed");
  llvm::StringSet<> activationNames;
  for (const ActivationSourceBinding &activation : request.activationSources)
    if (!isIdentity(activation.kind) || !isName(activation.name) ||
        !activationNames.insert(activation.name).second)
      return resolutionError("ACLOWER-TYPE-MISMATCH", request,
                             "request activation metadata is malformed");
  return llvm::Error::success();
}

llvm::Expected<BindingRequest>
parseBindingRequest(const llvm::json::Object &object) {
  static constexpr std::array<llvm::StringRef, 15> Keys = {
      "activation_sources",
      "binding",
      "binding_schema",
      "component_schema",
      "component_schema_fingerprint",
      "contract_epoch",
      "effect",
      "function_type",
      "parameters",
      "ports",
      "provider",
      "provider_implementation_fingerprint",
      "resolution_key",
      "resources",
      "results",
  };
  if (!hasExactKeys(object, Keys))
    return registryError(
        "request must contain exactly the frozen architecture fields");
  BindingRequest request;
#define ACIR_REQUEST_STRING(member, key)                                       \
  do {                                                                         \
    auto value = requestString(object, key);                                   \
    if (!value)                                                                \
      return value.takeError();                                                \
    request.member = std::move(*value);                                        \
  } while (false)
  ACIR_REQUEST_STRING(resolutionKey, "resolution_key");
  ACIR_REQUEST_STRING(functionType, "function_type");
  ACIR_REQUEST_STRING(bindingSchema, "binding_schema");
  ACIR_REQUEST_STRING(contractEpoch, "contract_epoch");
  ACIR_REQUEST_STRING(binding, "binding");
  ACIR_REQUEST_STRING(componentSchema, "component_schema");
  ACIR_REQUEST_STRING(componentSchemaFingerprint,
                      "component_schema_fingerprint");
  ACIR_REQUEST_STRING(effect, "effect");
  ACIR_REQUEST_STRING(provider, "provider");
  ACIR_REQUEST_STRING(providerImplementationFingerprint,
                      "provider_implementation_fingerprint");
#undef ACIR_REQUEST_STRING
  if (!request.resolutionKey.starts_with('@') ||
      request.resolutionKey.size() == 1 || request.functionType.empty() ||
      request.binding.empty() || request.componentSchema.empty() ||
      request.provider.empty() ||
      (request.effect != "pure" && request.effect != "stateful") ||
      !isSha256(request.componentSchemaFingerprint) ||
      !isSha256(request.providerImplementationFingerprint))
    return registryError("request identity, fingerprint, or effect is invalid");

  auto parameters = parseRequestArray<ParameterRequirement>(
      object, "parameters",
      [](const llvm::json::Object &entry)
          -> llvm::Expected<ParameterRequirement> {
        static constexpr std::array<llvm::StringRef, 4> Keys = {
            "acir_type", "name", "ordinal", "value"};
        if (!hasExactKeys(entry, Keys))
          return registryError(
              "request parameter must contain exact architecture fields");
        auto acirType = requestString(entry, "acir_type");
        auto name = requestString(entry, "name");
        auto ordinal = entry.getInteger("ordinal");
        const llvm::json::Value *value = entry.get("value");
        if (!acirType || acirType->empty() || !name || name->empty() ||
            !ordinal || *ordinal < 0 || !value)
          return registryError("request parameter is invalid");
        return ParameterRequirement{std::move(*acirType), std::move(*name),
                                    *ordinal, *value};
      });
  if (!parameters)
    return parameters.takeError();
  for (size_t index = 0; index < parameters->size(); ++index)
    if ((*parameters)[index].ordinal != static_cast<int64_t>(index))
      return registryError("request parameter ordinals must be contiguous");
  request.parameters = std::move(*parameters);

  auto ports = parseRequestArray<PortRequirement>(
      object, "ports",
      [](const llvm::json::Object &entry) -> llvm::Expected<PortRequirement> {
        static constexpr std::array<llvm::StringRef, 9> Keys = {
            "cardinality", "delegation", "direction", "interface",  "ownership",
            "payload",     "protocol",   "role",      "time_domain"};
        if (!hasExactKeys(entry, Keys))
          return registryError(
              "request port must contain exact architecture fields");
        PortRequirement port;
#define ACIR_PORT_STRING(member, key)                                          \
  do {                                                                         \
    auto value = requestString(entry, key);                                    \
    if (!value)                                                                \
      return value.takeError();                                                \
    port.member = std::move(*value);                                           \
  } while (false)
        ACIR_PORT_STRING(cardinality, "cardinality");
        ACIR_PORT_STRING(delegation, "delegation");
        ACIR_PORT_STRING(direction, "direction");
        ACIR_PORT_STRING(interface, "interface");
        ACIR_PORT_STRING(ownership, "ownership");
        ACIR_PORT_STRING(payload, "payload");
        ACIR_PORT_STRING(protocol, "protocol");
        ACIR_PORT_STRING(role, "role");
        ACIR_PORT_STRING(timeDomain, "time_domain");
#undef ACIR_PORT_STRING
        return port;
      });
  if (!ports)
    return ports.takeError();
  request.ports = std::move(*ports);

  auto resources = parseRequestArray<ResourceRequirement>(
      object, "resources",
      [](const llvm::json::Object &entry)
          -> llvm::Expected<ResourceRequirement> {
        static constexpr std::array<llvm::StringRef, 6> Keys = {
            "delegation", "mode", "ownership",
            "resource",   "role", "time_domain"};
        if (!hasExactKeys(entry, Keys))
          return registryError(
              "request resource must contain exact architecture fields");
        ResourceRequirement resource;
#define ACIR_RESOURCE_STRING(member, key)                                      \
  do {                                                                         \
    auto value = requestString(entry, key);                                    \
    if (!value)                                                                \
      return value.takeError();                                                \
    resource.member = std::move(*value);                                       \
  } while (false)
        ACIR_RESOURCE_STRING(delegation, "delegation");
        ACIR_RESOURCE_STRING(mode, "mode");
        ACIR_RESOURCE_STRING(ownership, "ownership");
        ACIR_RESOURCE_STRING(resource, "resource");
        ACIR_RESOURCE_STRING(role, "role");
        ACIR_RESOURCE_STRING(timeDomain, "time_domain");
#undef ACIR_RESOURCE_STRING
        return resource;
      });
  if (!resources)
    return resources.takeError();
  request.resources = std::move(*resources);

  auto results = parseRequestArray<ResultRequirement>(
      object, "results",
      [](const llvm::json::Object &entry) -> llvm::Expected<ResultRequirement> {
        static constexpr std::array<llvm::StringRef, 2> Keys = {"acir_type",
                                                                "name"};
        if (!hasExactKeys(entry, Keys))
          return registryError(
              "request result must contain exact architecture fields");
        auto acirType = requestString(entry, "acir_type");
        auto name = requestString(entry, "name");
        if (!acirType || acirType->empty() || !name || name->empty())
          return registryError("request result is invalid");
        return ResultRequirement{std::move(*acirType), std::move(*name)};
      });
  if (!results)
    return results.takeError();
  request.results = std::move(*results);

  auto activations = parseRequestArray<ActivationSourceBinding>(
      object, "activation_sources",
      [](const llvm::json::Object &entry)
          -> llvm::Expected<ActivationSourceBinding> {
        static constexpr std::array<llvm::StringRef, 2> Keys = {"kind", "name"};
        if (!hasExactKeys(entry, Keys))
          return registryError(
              "request activation source must contain exact fields");
        auto kind = requestString(entry, "kind");
        auto name = requestString(entry, "name");
        if (!kind || kind->empty() || !name || name->empty())
          return registryError("request activation source is invalid");
        return ActivationSourceBinding{std::move(*kind), std::move(*name)};
      });
  if (!activations)
    return activations.takeError();
  request.activationSources = std::move(*activations);
  return request;
}

} // namespace

struct BindingCandidate::Storage {
  bool available = false;
  std::string profile;
  std::string target;
  BindingRecord record;

  Storage(bool available, std::string profile, std::string target,
          BindingRecord record)
      : available(available), profile(std::move(profile)),
        target(std::move(target)), record(std::move(record)) {}
};

BindingCandidate::BindingCandidate(std::shared_ptr<const Storage> storage)
    : storage(std::move(storage)) {}

BindingCandidate::~BindingCandidate() = default;

llvm::Expected<BindingCandidate>
BindingCandidate::parse(const llvm::json::Object &object,
                        const JsonParseLimits &limits) {
  auto canonicalSize = detail::preflightConstructedJson(object, limits);
  if (!canonicalSize)
    return canonicalSize.takeError();
  static constexpr std::array<llvm::StringRef, 4> Keys = {
      "available", "profile", "record", "target"};
  if (object.size() != Keys.size() ||
      !llvm::all_of(Keys, [&](llvm::StringRef key) { return object.get(key); }))
    return registryError("candidate must contain exactly available, profile, "
                         "record, and target");
  auto available = object.getBoolean("available");
  auto profile = object.getString("profile");
  auto target = object.getString("target");
  const auto *recordObject = object.getObject("record");
  if (!available || !profile || profile->empty() || !target ||
      target->empty() || !recordObject)
    return registryError("candidate selection metadata has invalid types");
  auto record = BindingRecord::parse(*recordObject, limits);
  if (!record)
    return record.takeError();
  return BindingCandidate(std::make_shared<Storage>(
      *available, profile->str(), target->str(), std::move(*record)));
}

llvm::StringRef BindingCandidate::profile() const { return storage->profile; }
llvm::StringRef BindingCandidate::target() const { return storage->target; }
bool BindingCandidate::available() const { return storage->available; }
const BindingRecord &BindingCandidate::record() const {
  return storage->record;
}

llvm::Expected<std::string> BindingCandidate::deterministicKey() const {
  auto canonical = record().canonicalJson();
  if (!canonical)
    return canonical.takeError();
  return (llvm::Twine(profile()) + "\n" + target() + "\n" +
          (available() ? "1\n" : "0\n") + *canonical)
      .str();
}

llvm::Expected<BindingRegistryDocument>
parseBindingRegistry(llvm::StringRef input, const JsonParseLimits &limits) {
  auto parsed = parseIJson(input, limits);
  if (!parsed)
    return parsed.takeError();
  const auto *document = parsed->getAsObject();
  static constexpr std::array<llvm::StringRef, 2> Keys = {"candidates",
                                                          "requests"};
  if (!document || !hasExactKeys(*document, Keys))
    return registryError(
        "registry must contain exactly candidates and requests arrays");
  const auto *candidateArray = document->getArray("candidates");
  const auto *requestArray = document->getArray("requests");
  if (!candidateArray || !requestArray)
    return registryError("registry candidates and requests must be arrays");
  BindingRegistryDocument result;
  result.candidates.reserve(candidateArray->size());
  for (const llvm::json::Value &value : *candidateArray) {
    const auto *object = value.getAsObject();
    if (!object)
      return registryError("registry candidate must be an object");
    auto candidate = BindingCandidate::parse(*object, limits);
    if (!candidate)
      return candidate.takeError();
    result.candidates.push_back(std::move(*candidate));
  }
  result.requests.reserve(requestArray->size());
  for (const llvm::json::Value &value : *requestArray) {
    const auto *object = value.getAsObject();
    if (!object)
      return registryError("registry request must be an object");
    auto request = parseBindingRequest(*object);
    if (!request)
      return request.takeError();
    result.requests.push_back(std::move(*request));
  }
  return result;
}

ResolvedBinding::ResolvedBinding(std::string resolutionKey,
                                 BindingCandidate candidate)
    : key(std::move(resolutionKey)), selected(std::move(candidate)) {}

BindingRegistry::BindingRegistry(std::vector<BindingCandidate> candidates)
    : orderedCandidates(std::move(candidates)) {}

llvm::Expected<BindingRegistry>
BindingRegistry::create(std::vector<BindingCandidate> candidates) {
  struct KeyedCandidate {
    std::string key;
    BindingCandidate candidate;
  };
  std::vector<KeyedCandidate> keyed;
  keyed.reserve(candidates.size());
  for (BindingCandidate &candidate : candidates) {
    if (llvm::Error error = candidate.record().validateFingerprint())
      return std::move(error);
    auto key = candidate.deterministicKey();
    if (!key)
      return key.takeError();
    keyed.push_back({std::move(*key), std::move(candidate)});
  }
  llvm::sort(keyed,
             [](const KeyedCandidate &left, const KeyedCandidate &right) {
               return left.key < right.key;
             });
  candidates.clear();
  candidates.reserve(keyed.size());
  for (KeyedCandidate &entry : keyed)
    candidates.push_back(std::move(entry.candidate));
  return BindingRegistry(std::move(candidates));
}

llvm::Expected<ResolvedBinding>
BindingRegistry::resolve(const BindingRequest &request, llvm::StringRef profile,
                         llvm::StringRef target) const {
  if (llvm::Error error = validateRequest(request, profile, target))
    return std::move(error);

  std::vector<const BindingCandidate *> exactMatches;
  for (const BindingCandidate &candidate : orderedCandidates)
    if (candidate.profile() == profile && candidate.target() == target &&
        candidate.available() && matchesMetadata(candidate.record(), request))
      exactMatches.push_back(&candidate);
  if (exactMatches.size() == 1)
    return ResolvedBinding(request.resolutionKey, *exactMatches.front());
  if (exactMatches.size() > 1) {
    std::string candidates;
    for (const BindingCandidate *candidate : exactMatches) {
      if (!candidates.empty())
        candidates.push_back(',');
      candidates.append(candidate->record().fingerprint());
    }
    return resolutionError("ACLOWER-BINDING-AMBIGUOUS", request,
                           llvm::Twine("candidates=") + candidates);
  }

  std::vector<const BindingCandidate *> diagnosticCandidates;
  for (const BindingCandidate &candidate : orderedCandidates)
    if (candidate.record().binding() == request.binding)
      diagnosticCandidates.push_back(&candidate);
  if (diagnosticCandidates.empty())
    return resolutionError("ACLOWER-BINDING-MISSING", request,
                           "no candidate has the exact binding identity");

  auto reasonIfEmpty = [&](llvm::StringRef reason,
                           auto predicate) -> std::optional<std::string> {
    std::vector<const BindingCandidate *> matches;
    for (const BindingCandidate *candidate : diagnosticCandidates)
      if (predicate(*candidate))
        matches.push_back(candidate);
    diagnosticCandidates = std::move(matches);
    if (diagnosticCandidates.empty())
      return (llvm::Twine("reason=") + reason).str();
    return std::nullopt;
  };

#define ACIR_RETURN_MISSING_REASON(code, predicate)                            \
  do {                                                                         \
    if (auto reason = reasonIfEmpty(code, predicate))                          \
      return resolutionError("ACLOWER-BINDING-MISSING", request, *reason);     \
  } while (false)
  ACIR_RETURN_MISSING_REASON(
      "ACLOWER-EPOCH-MISMATCH", [&](const BindingCandidate &candidate) {
        return candidate.record().contractEpoch() == request.contractEpoch;
      });
  ACIR_RETURN_MISSING_REASON(
      "ACLOWER-SCHEMA-MISMATCH", [&](const BindingCandidate &candidate) {
        return candidate.record().bindingSchema() == request.bindingSchema &&
               candidate.record().componentSchema() ==
                   request.componentSchema &&
               candidate.record().componentSchemaFingerprint() ==
                   request.componentSchemaFingerprint;
      });
  ACIR_RETURN_MISSING_REASON(
      "ACLOWER-INLINE-EFFECT", [&](const BindingCandidate &candidate) {
        return candidate.record().effect() == request.effect;
      });
  ACIR_RETURN_MISSING_REASON(
      "ACLOWER-PROFILE", [&](const BindingCandidate &candidate) {
        return candidate.profile() == profile && candidate.target() == target;
      });
  ACIR_RETURN_MISSING_REASON(
      "ACLOWER-PARAM-PHASE", [&](const BindingCandidate &candidate) {
        return matchesParameters(candidate.record(), request);
      });
  ACIR_RETURN_MISSING_REASON(
      "ACLOWER-TYPE-MISMATCH", [&](const BindingCandidate &candidate) {
        return matchesPorts(candidate.record(), request) &&
               matchesResources(candidate.record(), request) &&
               matchesResults(candidate.record(), request) &&
               candidate.record().activationSources() ==
                   llvm::ArrayRef<ActivationSourceBinding>(
                       request.activationSources);
      });
  ACIR_RETURN_MISSING_REASON(
      "ACLOWER-FINGERPRINT", [&](const BindingCandidate &candidate) {
        return candidate.record().provider() == request.provider &&
               candidate.record().providerImplementationFingerprint() ==
                   request.providerImplementationFingerprint;
      });
#undef ACIR_RETURN_MISSING_REASON
  return resolutionError("ACLOWER-BINDING-MISSING", request,
                         "exact candidate is unavailable");
}

llvm::ArrayRef<BindingCandidate> BindingRegistry::candidates() const {
  return orderedCandidates;
}

BindingResolutionResult::BindingResolutionResult(
    std::vector<ResolvedBinding> selections, std::string canonicalLock,
    std::string lockFingerprint)
    : selected(std::move(selections)), lock(std::move(canonicalLock)),
      fingerprint(std::move(lockFingerprint)) {}

const ResolvedBinding *BindingResolutionResult::selectionForResolutionKey(
    llvm::StringRef resolutionKey) const {
  for (const ResolvedBinding &selection : selected)
    if (selection.resolutionKey() == resolutionKey)
      return &selection;
  return nullptr;
}

llvm::Expected<BindingResolutionResult>
resolveBindings(llvm::ArrayRef<BindingCandidate> candidates,
                llvm::ArrayRef<BindingRequest> requests,
                llvm::StringRef profile, llvm::StringRef target) {
  std::vector<BindingCandidate> ownedCandidates(candidates.begin(),
                                                candidates.end());
  auto registry = BindingRegistry::create(std::move(ownedCandidates));
  if (!registry)
    return registry.takeError();

  std::vector<BindingRequest> orderedRequests(requests.begin(), requests.end());
  llvm::sort(orderedRequests,
             [](const BindingRequest &left, const BindingRequest &right) {
               return std::tie(left.resolutionKey, left.binding) <
                      std::tie(right.resolutionKey, right.binding);
             });
  for (size_t index = 1; index < orderedRequests.size(); ++index)
    if (orderedRequests[index - 1].resolutionKey ==
        orderedRequests[index].resolutionKey)
      return resolutionError("ACLOWER-BINDING-AMBIGUOUS",
                             orderedRequests[index],
                             "duplicate attempted resolution key");

  std::vector<ResolvedBinding> selections;
  selections.reserve(orderedRequests.size());
  for (const BindingRequest &request : orderedRequests) {
    auto selected = registry->resolve(request, profile, target);
    if (!selected)
      return selected.takeError();
    selections.push_back(std::move(*selected));
  }

  struct LockRecord {
    std::string binding;
    std::string canonical;
    const BindingRecord *record;
  };
  std::vector<LockRecord> lockRecords;
  llvm::StringMap<std::string> byBinding;
  for (const ResolvedBinding &selection : selections) {
    auto canonical = selection.record().canonicalJson();
    if (!canonical)
      return canonical.takeError();
    auto [iterator, inserted] =
        byBinding.try_emplace(selection.record().binding(), *canonical);
    if (!inserted && iterator->second != *canonical)
      return llvm::createStringError(
          llvm::errc::invalid_argument,
          "ACLOWER-BINDING-AMBIGUOUS: key=%s binding=%s one binding identity "
          "selected distinct records",
          selection.resolutionKey().str().c_str(),
          selection.record().binding().str().c_str());
    if (inserted)
      lockRecords.push_back({selection.record().binding().str(),
                             std::move(*canonical), &selection.record()});
  }
  llvm::sort(lockRecords, [](const LockRecord &left, const LockRecord &right) {
    return std::tie(left.binding, left.canonical) <
           std::tie(right.binding, right.canonical);
  });
  llvm::json::Array lockArray;
  for (const LockRecord &entry : lockRecords)
    lockArray.push_back(llvm::json::Object(entry.record->json()));
  auto canonicalLock =
      canonicalizeJson(llvm::json::Value(std::move(lockArray)));
  if (!canonicalLock)
    return canonicalLock.takeError();
  std::string fingerprint = sha256Fingerprint(*canonicalLock);
  return BindingResolutionResult(
      std::move(selections), std::move(*canonicalLock), std::move(fingerprint));
}

llvm::Error emitBindingLock(const BindingResolutionResult &result,
                            llvm::raw_ostream &output) {
  output << result.canonicalLock();
  return llvm::Error::success();
}

llvm::Error emitBindingLockAtomically(const BindingResolutionResult &result,
                                      llvm::StringRef outputPath) {
  if (outputPath.empty())
    return outputError("output path must be non-empty");
  auto temporary =
      llvm::sys::fs::TempFile::create(llvm::Twine(outputPath) + ".tmp-%%%%%%");
  if (!temporary)
    return outputError(llvm::Twine("cannot create temporary lock: ") +
                       llvm::toString(temporary.takeError()));
  {
    llvm::raw_fd_ostream output(temporary->FD, false);
    if (llvm::Error error = emitBindingLock(result, output)) {
      llvm::consumeError(temporary->discard());
      return error;
    }
    output.flush();
    if (output.has_error()) {
      llvm::consumeError(temporary->discard());
      return outputError("failed to flush canonical binding lock bytes");
    }
  }
  if (detail::shouldFailBindingPublish()) {
    llvm::consumeError(temporary->discard());
    return outputError("binding lock publication failed");
  }
  if (llvm::Error error = temporary->keep(outputPath))
    return outputError(llvm::Twine("cannot publish binding lock: ") +
                       llvm::toString(std::move(error)));
  return llvm::Error::success();
}

llvm::Error
resolveAndWriteBindingLock(llvm::ArrayRef<BindingCandidate> candidates,
                           llvm::ArrayRef<BindingRequest> requests,
                           llvm::StringRef profile, llvm::StringRef target,
                           llvm::StringRef outputPath) {
  auto result = resolveBindings(candidates, requests, profile, target);
  if (!result)
    return result.takeError();
  return emitBindingLockAtomically(*result, outputPath);
}

} // namespace acir::bindings
