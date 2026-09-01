#ifndef ACIR_BINDINGS_BINDING_H
#define ACIR_BINDINGS_BINDING_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/JSON.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace acir::bindings {

inline constexpr llvm::StringLiteral BindingSchema = "acsim-binding-0.1";
inline constexpr llvm::StringLiteral ContractEpoch = "0.4";

struct JsonParseLimits {
  size_t maxInputBytes = 1U << 20;
  size_t maxDepth = 64;
  size_t maxStructuralWork = 100000;
  size_t maxStringBytes = 1U << 18;
  size_t maxTotalStringBytes = 1U << 20;
  size_t maxArrayElements = 65536;
  size_t maxObjectMembers = 4096;
};

/// Parses the accepted RFC 8785/I-JSON subset while retaining lexical checks
/// that ordinary JSON DOM parsers lose (duplicate keys and negative zero).
llvm::Expected<llvm::json::Value>
parseIJson(llvm::StringRef input,
           const JsonParseLimits &limits = JsonParseLimits());

/// Emits RFC 8785 canonical UTF-8 bytes. Object names are ordered by unsigned
/// UTF-16 code units recursively; arrays retain their input order. Constructed
/// DOMs are preflighted against the same deterministic resource limits as text.
llvm::Expected<std::string>
canonicalizeJson(const llvm::json::Value &value,
                 const JsonParseLimits &limits = JsonParseLimits());
llvm::Expected<std::string>
canonicalizeJsonText(llvm::StringRef input,
                     const JsonParseLimits &limits = JsonParseLimits());

std::string sha256Fingerprint(llvm::StringRef canonicalBytes);

struct CppEntryPoints {
  std::string pure;
  std::string reset;
  std::string validate;
  std::string work;
  std::string xfer;

  bool operator==(const CppEntryPoints &) const = default;
};

struct CppBinding {
  std::string conceptName;
  CppEntryPoints entryPoints;
  std::string header;
  std::string symbol;
  std::string target;

  bool operator==(const CppBinding &) const = default;
};

struct ConstructionBinding {
  std::vector<llvm::json::Value> arguments;
  std::string kind;

  bool operator==(const ConstructionBinding &) const = default;
};

struct OwnershipBinding {
  std::string kind;
  std::string placement;

  bool operator==(const OwnershipBinding &) const = default;
};

struct ParameterBinding {
  std::string acirType;
  std::string cppType;
  std::string mapping;
  std::string name;
  int64_t ordinal = 0;
  llvm::json::Value value = nullptr;

  bool operator==(const ParameterBinding &) const = default;
};

struct PortBinding {
  std::string accessor;
  std::string cardinality;
  std::string delegation;
  std::string direction;
  std::string interface;
  std::string ownership;
  std::string payload;
  std::string protocol;
  std::string role;
  std::string timeDomain;

  bool operator==(const PortBinding &) const = default;
};

struct ResourceBinding {
  std::string accessor;
  std::string delegation;
  std::string mode;
  std::string ownership;
  std::string resource;
  std::string role;
  std::string timeDomain;

  bool operator==(const ResourceBinding &) const = default;
};

struct ResultBinding {
  std::string cppType;
  std::string name;

  bool operator==(const ResultBinding &) const = default;
};

struct ActivationSourceBinding {
  std::string kind;
  std::string name;

  bool operator==(const ActivationSourceBinding &) const = default;
};

/// Immutable, typed form of the exact 20-field acsim.binding consumer record.
class BindingRecord {
public:
  BindingRecord(const BindingRecord &) = default;
  BindingRecord(BindingRecord &&) noexcept = default;
  BindingRecord &operator=(const BindingRecord &) = default;
  BindingRecord &operator=(BindingRecord &&) noexcept = default;
  ~BindingRecord();

  static llvm::Expected<BindingRecord>
  parse(const llvm::json::Object &object,
        const JsonParseLimits &limits = JsonParseLimits());

  llvm::StringRef bindingSchema() const;
  llvm::StringRef contractEpoch() const;
  llvm::StringRef binding() const;
  llvm::StringRef componentSchema() const;
  llvm::StringRef componentSchemaFingerprint() const;
  llvm::StringRef availability() const;
  llvm::StringRef effect() const;
  llvm::StringRef cppType() const;
  llvm::StringRef implementation() const;
  llvm::StringRef provider() const;
  llvm::StringRef providerImplementationFingerprint() const;
  llvm::StringRef fingerprint() const;
  const CppBinding &cpp() const;
  const ConstructionBinding &construction() const;
  const OwnershipBinding &ownership() const;
  llvm::ArrayRef<ParameterBinding> parameters() const;
  llvm::ArrayRef<PortBinding> ports() const;
  llvm::ArrayRef<ResourceBinding> resources() const;
  llvm::ArrayRef<ResultBinding> results() const;
  llvm::ArrayRef<ActivationSourceBinding> activationSources() const;
  const llvm::json::Object &json() const;

  llvm::Expected<std::string> canonicalJson() const;
  llvm::Expected<std::string> canonicalJsonForFingerprint() const;
  llvm::Error validateFingerprint() const;

private:
  struct Storage;
  explicit BindingRecord(std::shared_ptr<const Storage> storage);
  std::shared_ptr<const Storage> storage;
};

llvm::Expected<std::string>
computeBindingRecordFingerprint(const BindingRecord &record);

} // namespace acir::bindings

#endif // ACIR_BINDINGS_BINDING_H
