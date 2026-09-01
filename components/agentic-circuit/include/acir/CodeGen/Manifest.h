#ifndef ACIR_CODEGEN_MANIFEST_H
#define ACIR_CODEGEN_MANIFEST_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/JSON.h"

#include <optional>
#include <string>
#include <vector>

namespace acir::codegen {

/// A normative SHA-256 fingerprint: `sha256:` followed by 64 lowercase hex
/// digits.
using Fingerprint = std::string;

bool isValidFingerprint(llvm::StringRef value);
Fingerprint computeFingerprint(llvm::StringRef content);
llvm::Expected<Fingerprint>
fingerprintCanonicalJson(const llvm::json::Value &value);

/// Transitional source record used by the low-level emitter. Structured source
/// bundles replace it at the ModelPlan boundary.
struct SourceFile {
  std::string relativePath;
  std::string content;
  Fingerprint fingerprint;
};

struct Identity {
  std::string name;
  std::string identity;
};

struct FileHash {
  std::string path;
  Fingerprint sha256;
};

struct CompilerIdentity {
  std::string name;
  std::string buildId;
  std::string toolchainTarget;
};

struct ProviderIdentity {
  std::string nameSpace;
  Fingerprint schemaFingerprint;
  Fingerprint implementationFingerprint;
};

struct ComponentSpecialization {
  std::string canonicalName;
  Fingerprint schemaFingerprint;
  Fingerprint specializationFingerprint;
};

struct NamedFingerprint {
  std::string name;
  Fingerprint fingerprint;
};

enum class ArtifactKind {
  Acpy,
  Acir,
  Acsim,
  CppSource,
  CppHeader,
  Executable,
  Report,
};

struct Artifact {
  std::string path;
  ArtifactKind kind;
  Fingerprint sha256;
};

enum class ValidationStatus { Passed, Failed };

struct ValidationGate {
  std::string name;
  ValidationStatus status;
  std::optional<Fingerprint> reportSha256;
};

struct SpecializationInput {
  std::string name;
  std::string acirType;
  llvm::json::Value canonicalValue = nullptr;
};

/// Typed representation of schemas/build-manifest.schema.json.
struct BuildManifest {
  std::string schema = "agentic-circuit-build-manifest";
  std::string version = "0.1";
  std::string contractEpoch = "0.4";
  Identity project;
  Identity system;
  std::vector<FileHash> sourceFiles;
  Fingerprint normalizedAcirSha256;
  CompilerIdentity compiler;
  std::vector<std::string> passPipeline;
  std::vector<ProviderIdentity> providers;
  std::vector<ComponentSpecialization> componentSpecializations;
  std::vector<NamedFingerprint> protocolIdentities;
  std::vector<Artifact> artifacts;
  std::vector<ValidationGate> validationGates;
  std::string buildProfile;
  std::vector<std::string> instrumentationLayers;
  std::vector<SpecializationInput> specializationInputs;
  Fingerprint buildFingerprint;

  llvm::Error validate() const;
  llvm::Expected<std::string> canonicalJson() const;

  /// Fingerprint the closed versioned build preimage and store the result in
  /// buildFingerprint.
  llvm::Error finalizeBuildFingerprint();
};

} // namespace acir::codegen

#endif // ACIR_CODEGEN_MANIFEST_H
