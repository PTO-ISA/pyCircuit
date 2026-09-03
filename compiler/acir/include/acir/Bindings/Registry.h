#ifndef ACIR_BINDINGS_REGISTRY_H
#define ACIR_BINDINGS_REGISTRY_H

#include "acir/Bindings/Binding.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>
#include <vector>

namespace acir::bindings {

/// Candidate-only selection metadata is intentionally outside the exact lock
/// record and is never emitted into acsim.binding.
class BindingCandidate {
public:
  BindingCandidate(const BindingCandidate &) = default;
  BindingCandidate(BindingCandidate &&) noexcept = default;
  BindingCandidate &operator=(const BindingCandidate &) = default;
  BindingCandidate &operator=(BindingCandidate &&) noexcept = default;
  ~BindingCandidate();

  static llvm::Expected<BindingCandidate>
  parse(const llvm::json::Object &object,
        const JsonParseLimits &limits = JsonParseLimits());

  llvm::StringRef profile() const;
  llvm::StringRef target() const;
  bool available() const;
  const BindingRecord &record() const;
  llvm::Expected<std::string> deterministicKey() const;

private:
  struct Storage;
  explicit BindingCandidate(std::shared_ptr<const Storage> storage);
  std::shared_ptr<const Storage> storage;
};

struct ParameterRequirement {
  std::string acirType;
  std::string name;
  int64_t ordinal = 0;
  llvm::json::Value value = nullptr;

  bool operator==(const ParameterRequirement &) const = default;
};

struct PortRequirement {
  std::string cardinality;
  std::string delegation;
  std::string direction;
  std::string interface;
  std::string ownership;
  std::string payload;
  std::string protocol;
  std::string role;
  std::string timeDomain;

  bool operator==(const PortRequirement &) const = default;
};

struct ResourceRequirement {
  std::string delegation;
  std::string mode;
  std::string ownership;
  std::string resource;
  std::string role;
  std::string timeDomain;

  bool operator==(const ResourceRequirement &) const = default;
};

struct ResultRequirement {
  std::string acirType;
  std::string name;

  bool operator==(const ResultRequirement &) const = default;
};

struct BindingRequest {
  /// Independent frozen-architecture authority. Candidate-only C++ realization
  /// fields (accessors, mappings, construction, symbols, and entry points) are
  /// intentionally absent and must never be synthesized into this request.
  std::string resolutionKey;
  std::string functionType;
  std::string bindingSchema;
  std::string contractEpoch;
  std::string binding;
  std::string componentSchema;
  std::string componentSchemaFingerprint;
  std::string effect;
  std::string provider;
  std::string providerImplementationFingerprint;
  std::vector<ParameterRequirement> parameters;
  std::vector<PortRequirement> ports;
  std::vector<ResourceRequirement> resources;
  std::vector<ResultRequirement> results;
  std::vector<ActivationSourceBinding> activationSources;
};

struct BindingRegistryDocument {
  /// The two arrays have distinct provenance: candidates cannot author or
  /// supply defaults for requests.
  std::vector<BindingCandidate> candidates;
  std::vector<BindingRequest> requests;
};

llvm::Expected<BindingRegistryDocument>
parseBindingRegistry(llvm::StringRef input,
                     const JsonParseLimits &limits = JsonParseLimits());

class ResolvedBinding {
public:
  ResolvedBinding(std::string resolutionKey, BindingCandidate candidate);

  llvm::StringRef resolutionKey() const { return key; }
  const BindingRecord &record() const { return selected.record(); }
  const BindingCandidate &candidate() const { return selected; }

private:
  std::string key;
  BindingCandidate selected;
};

class BindingRegistry {
public:
  static llvm::Expected<BindingRegistry>
  create(std::vector<BindingCandidate> candidates);

  llvm::Expected<ResolvedBinding> resolve(const BindingRequest &request,
                                          llvm::StringRef profile,
                                          llvm::StringRef target) const;
  llvm::ArrayRef<BindingCandidate> candidates() const;

private:
  explicit BindingRegistry(std::vector<BindingCandidate> candidates);
  std::vector<BindingCandidate> orderedCandidates;
};

class BindingResolutionResult {
public:
  BindingResolutionResult(std::vector<ResolvedBinding> selections,
                          std::string canonicalLock,
                          std::string lockFingerprint);

  llvm::ArrayRef<ResolvedBinding> selections() const { return selected; }
  const ResolvedBinding *
  selectionForResolutionKey(llvm::StringRef resolutionKey) const;
  llvm::StringRef canonicalLock() const { return lock; }
  llvm::StringRef lockFingerprint() const { return fingerprint; }

private:
  std::vector<ResolvedBinding> selected;
  std::string lock;
  std::string fingerprint;
};

llvm::Expected<BindingResolutionResult>
resolveBindings(llvm::ArrayRef<BindingCandidate> candidates,
                llvm::ArrayRef<BindingRequest> requests,
                llvm::StringRef profile, llvm::StringRef target);

llvm::Error emitBindingLock(const BindingResolutionResult &result,
                            llvm::raw_ostream &output);
llvm::Error emitBindingLockAtomically(const BindingResolutionResult &result,
                                      llvm::StringRef outputPath);
llvm::Error
resolveAndWriteBindingLock(llvm::ArrayRef<BindingCandidate> candidates,
                           llvm::ArrayRef<BindingRequest> requests,
                           llvm::StringRef profile, llvm::StringRef target,
                           llvm::StringRef outputPath);

} // namespace acir::bindings

#endif // ACIR_BINDINGS_REGISTRY_H
