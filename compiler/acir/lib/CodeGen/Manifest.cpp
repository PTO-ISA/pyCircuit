#include "acir/CodeGen/Manifest.h"

#include "acir/Bindings/Binding.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Twine.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <string_view>
#include <system_error>
#include <utility>

namespace acir::codegen {
namespace {

llvm::Error manifestError(const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument),
      "ACLOWER-FINGERPRINT: " + message);
}

bool isNormalizedRelativePath(llvm::StringRef path) {
  if (path.empty() || path.starts_with('/') || path.ends_with('/') ||
      path.contains('\\') || path.contains('\0'))
    return false;

  while (!path.empty()) {
    auto [component, remainder] = path.split('/');
    if (component.empty() || component == "." || component == "..")
      return false;
    path = remainder;
  }
  return true;
}

template <typename Range, typename Key>
llvm::Error validateUniqueKeys(const Range &range, Key key,
                               llvm::StringRef field) {
  std::set<std::string> seen;
  for (const auto &value : range) {
    llvm::StringRef current = key(value);
    if (current.empty())
      return manifestError(field + " contains an empty stable key");
    if (!seen.insert(current.str()).second)
      return manifestError(field + " contains duplicate key '" + current + "'");
  }
  return llvm::Error::success();
}

template <typename T, typename Key>
std::vector<const T *> sortedPointers(const std::vector<T> &values, Key key) {
  std::vector<const T *> sorted;
  sorted.reserve(values.size());
  for (const auto &value : values)
    sorted.push_back(&value);
  std::sort(sorted.begin(), sorted.end(),
            [&](const T *lhs, const T *rhs) { return key(*lhs) < key(*rhs); });
  return sorted;
}

llvm::StringRef artifactKindName(ArtifactKind kind) {
  switch (kind) {
  case ArtifactKind::Acpy:
    return "acpy";
  case ArtifactKind::Acir:
    return "acir";
  case ArtifactKind::Acsim:
    return "acsim";
  case ArtifactKind::CppSource:
    return "cpp_source";
  case ArtifactKind::CppHeader:
    return "cpp_header";
  case ArtifactKind::Executable:
    return "executable";
  case ArtifactKind::Report:
    return "report";
  }
  llvm_unreachable("closed ArtifactKind is exhaustive");
}

llvm::StringRef validationStatusName(ValidationStatus status) {
  switch (status) {
  case ValidationStatus::Passed:
    return "passed";
  case ValidationStatus::Failed:
    return "failed";
  }
  llvm_unreachable("closed ValidationStatus is exhaustive");
}

llvm::json::Object identityJson(const Identity &identity) {
  return llvm::json::Object{{"name", identity.name},
                            {"identity", identity.identity}};
}

llvm::json::Object manifestJson(const BuildManifest &manifest,
                                llvm::StringRef buildFingerprint) {
  llvm::json::Array sources;
  for (const FileHash *source :
       sortedPointers(manifest.sourceFiles, [](const FileHash &value) {
         return llvm::StringRef(value.path);
       }))
    sources.push_back(
        llvm::json::Object{{"path", source->path}, {"sha256", source->sha256}});

  llvm::json::Object compiler{
      {"name", manifest.compiler.name},
      {"build_id", manifest.compiler.buildId},
      {"toolchain_target", manifest.compiler.toolchainTarget}};

  llvm::json::Array pipeline;
  for (const auto &pass : manifest.passPipeline)
    pipeline.push_back(pass);

  llvm::json::Array providers;
  for (const ProviderIdentity *provider :
       sortedPointers(manifest.providers, [](const ProviderIdentity &value) {
         return llvm::StringRef(value.nameSpace);
       })) {
    providers.push_back(llvm::json::Object{
        {"namespace", provider->nameSpace},
        {"schema_fingerprint", provider->schemaFingerprint},
        {"implementation_fingerprint", provider->implementationFingerprint}});
  }

  llvm::json::Array specializations;
  for (const ComponentSpecialization *specialization :
       sortedPointers(manifest.componentSpecializations,
                      [](const ComponentSpecialization &value) {
                        return llvm::StringRef(value.canonicalName);
                      })) {
    specializations.push_back(llvm::json::Object{
        {"canonical_name", specialization->canonicalName},
        {"schema_fingerprint", specialization->schemaFingerprint},
        {"specialization_fingerprint",
         specialization->specializationFingerprint}});
  }

  llvm::json::Array protocols;
  for (const NamedFingerprint *protocol : sortedPointers(
           manifest.protocolIdentities, [](const NamedFingerprint &value) {
             return llvm::StringRef(value.name);
           })) {
    protocols.push_back(llvm::json::Object{
        {"name", protocol->name}, {"fingerprint", protocol->fingerprint}});
  }

  llvm::json::Array artifacts;
  for (const Artifact *artifact :
       sortedPointers(manifest.artifacts, [](const Artifact &value) {
         return llvm::StringRef(value.path);
       })) {
    artifacts.push_back(
        llvm::json::Object{{"path", artifact->path},
                           {"kind", artifactKindName(artifact->kind)},
                           {"sha256", artifact->sha256}});
  }

  llvm::json::Array gates;
  for (const ValidationGate *gate : sortedPointers(
           manifest.validationGates, [](const ValidationGate &value) {
             return llvm::StringRef(value.name);
           })) {
    llvm::json::Object object{{"name", gate->name},
                              {"status", validationStatusName(gate->status)}};
    if (gate->reportSha256)
      object["report_sha256"] = *gate->reportSha256;
    else
      object["report_sha256"] = nullptr;
    gates.push_back(std::move(object));
  }

  std::vector<std::string> layers = manifest.instrumentationLayers;
  std::sort(layers.begin(), layers.end());
  llvm::json::Array instrumentation;
  for (const auto &layer : layers)
    instrumentation.push_back(layer);

  llvm::json::Array specializationInputs;
  for (const SpecializationInput *input : sortedPointers(
           manifest.specializationInputs, [](const SpecializationInput &value) {
             return llvm::StringRef(value.name);
           })) {
    specializationInputs.push_back(
        llvm::json::Object{{"name", input->name},
                           {"acir_type", input->acirType},
                           {"canonical_value", input->canonicalValue}});
  }

  return llvm::json::Object{
      {"schema", manifest.schema},
      {"version", manifest.version},
      {"contract_epoch", manifest.contractEpoch},
      {"project", identityJson(manifest.project)},
      {"system", identityJson(manifest.system)},
      {"source_files", std::move(sources)},
      {"normalized_acir_sha256", manifest.normalizedAcirSha256},
      {"compiler", std::move(compiler)},
      {"pass_pipeline", std::move(pipeline)},
      {"providers", std::move(providers)},
      {"component_specializations", std::move(specializations)},
      {"protocol_identities", std::move(protocols)},
      {"artifacts", std::move(artifacts)},
      {"validation_gates", std::move(gates)},
      {"build_profile", manifest.buildProfile},
      {"instrumentation_layers", std::move(instrumentation)},
      {"specialization_inputs", std::move(specializationInputs)},
      {"build_fingerprint", buildFingerprint},
  };
}

llvm::Error validateManifest(const BuildManifest &manifest,
                             bool requireBuildFingerprint) {
  if (manifest.schema != "agentic-circuit-build-manifest")
    return manifestError(
        "manifest schema must be agentic-circuit-build-manifest");
  if (manifest.version != "0.1" || manifest.contractEpoch != "0.4")
    return manifestError(
        "manifest version must be 0.1 and contract epoch must be 0.4");
  if (manifest.project.name.empty() || manifest.project.identity.empty() ||
      manifest.system.name.empty() || manifest.system.identity.empty())
    return manifestError("project and system identities must be non-empty");
  if (!isValidFingerprint(manifest.normalizedAcirSha256))
    return manifestError("normalized_acir_sha256 is invalid");
  if (manifest.compiler.name.empty() || manifest.compiler.buildId.empty() ||
      manifest.compiler.toolchainTarget.empty())
    return manifestError("compiler identity must be complete");
  if (manifest.buildProfile != "fast" && manifest.buildProfile != "validated" &&
      manifest.buildProfile != "custom")
    return manifestError("build_profile is not a closed v0.1 value");
  if (requireBuildFingerprint && !isValidFingerprint(manifest.buildFingerprint))
    return manifestError("build_fingerprint is invalid");

  if (auto error = validateUniqueKeys(
          manifest.sourceFiles,
          [](const FileHash &value) { return llvm::StringRef(value.path); },
          "source_files"))
    return error;
  for (const auto &source : manifest.sourceFiles) {
    if (!isNormalizedRelativePath(source.path))
      return manifestError("source_files contains a non-normalized path");
    if (!isValidFingerprint(source.sha256))
      return manifestError("source_files contains an invalid fingerprint");
  }

  for (const auto &pass : manifest.passPipeline)
    if (pass.empty())
      return manifestError("pass_pipeline contains an empty pass name");

  if (auto error = validateUniqueKeys(
          manifest.providers,
          [](const ProviderIdentity &value) {
            return llvm::StringRef(value.nameSpace);
          },
          "providers"))
    return error;
  for (const auto &provider : manifest.providers) {
    if (!isValidFingerprint(provider.schemaFingerprint) ||
        !isValidFingerprint(provider.implementationFingerprint))
      return manifestError("providers contains an invalid fingerprint");
  }

  if (auto error = validateUniqueKeys(
          manifest.componentSpecializations,
          [](const ComponentSpecialization &value) {
            return llvm::StringRef(value.canonicalName);
          },
          "component_specializations"))
    return error;
  for (const auto &specialization : manifest.componentSpecializations) {
    if (!isValidFingerprint(specialization.schemaFingerprint) ||
        !isValidFingerprint(specialization.specializationFingerprint))
      return manifestError(
          "component_specializations contains an invalid fingerprint");
  }

  if (auto error = validateUniqueKeys(
          manifest.protocolIdentities,
          [](const NamedFingerprint &value) {
            return llvm::StringRef(value.name);
          },
          "protocol_identities"))
    return error;
  for (const auto &protocol : manifest.protocolIdentities)
    if (!isValidFingerprint(protocol.fingerprint))
      return manifestError(
          "protocol_identities contains an invalid fingerprint");

  if (auto error = validateUniqueKeys(
          manifest.artifacts,
          [](const Artifact &value) { return llvm::StringRef(value.path); },
          "artifacts"))
    return error;
  for (const auto &artifact : manifest.artifacts) {
    if (!isNormalizedRelativePath(artifact.path))
      return manifestError("artifacts contains a non-normalized path");
    if (!isValidFingerprint(artifact.sha256))
      return manifestError("artifacts contains an invalid fingerprint");
  }

  if (auto error = validateUniqueKeys(
          manifest.validationGates,
          [](const ValidationGate &value) {
            return llvm::StringRef(value.name);
          },
          "validation_gates"))
    return error;
  for (const auto &gate : manifest.validationGates)
    if (gate.reportSha256 && !isValidFingerprint(*gate.reportSha256))
      return manifestError("validation_gates contains an invalid report hash");

  if (auto error = validateUniqueKeys(
          manifest.instrumentationLayers,
          [](const std::string &value) { return llvm::StringRef(value); },
          "instrumentation_layers"))
    return error;

  if (auto error = validateUniqueKeys(
          manifest.specializationInputs,
          [](const SpecializationInput &value) {
            return llvm::StringRef(value.name);
          },
          "specialization_inputs"))
    return error;
  for (const auto &input : manifest.specializationInputs) {
    if (input.acirType.empty())
      return manifestError("specialization_inputs contains an empty ACIR type");
    auto canonical = bindings::canonicalizeJson(input.canonicalValue);
    if (!canonical)
      return canonical.takeError();
  }

  return llvm::Error::success();
}

} // namespace

bool isValidFingerprint(llvm::StringRef value) {
  constexpr llvm::StringLiteral Prefix = "sha256:";
  if (!value.consume_front(Prefix) || value.size() != 64)
    return false;
  return std::all_of(value.begin(), value.end(), [](char character) {
    return (character >= '0' && character <= '9') ||
           (character >= 'a' && character <= 'f');
  });
}

Fingerprint computeFingerprint(llvm::StringRef content) {
  return bindings::sha256Fingerprint(content);
}

llvm::Expected<Fingerprint>
fingerprintCanonicalJson(const llvm::json::Value &value) {
  auto canonical = bindings::canonicalizeJson(value);
  if (!canonical)
    return canonical.takeError();
  return computeFingerprint(*canonical);
}

llvm::Error BuildManifest::validate() const {
  return validateManifest(*this, true);
}

llvm::Expected<std::string> BuildManifest::canonicalJson() const {
  if (auto error = validate())
    return std::move(error);
  return bindings::canonicalizeJson(
      llvm::json::Value(manifestJson(*this, buildFingerprint)));
}

llvm::Error BuildManifest::finalizeBuildFingerprint() {
  if (auto error = validateManifest(*this, false))
    return error;

  llvm::json::Object preimage{
      {"domain", "agentic-circuit-build-0.1"},
      {"manifest", manifestJson(*this, "")},
  };
  auto fingerprint =
      fingerprintCanonicalJson(llvm::json::Value(std::move(preimage)));
  if (!fingerprint)
    return fingerprint.takeError();
  buildFingerprint = std::move(*fingerprint);
  return validate();
}

} // namespace acir::codegen
