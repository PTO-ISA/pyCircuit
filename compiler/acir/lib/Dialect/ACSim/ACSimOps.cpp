#include "acir/Dialect/ACSim/ACSimOps.h"
#include "ACSimOpsTestHooks.h"

#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/TypeSwitch.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <utility>

using namespace mlir;

namespace acir::acsim {
namespace {
thread_local detail::ModelVerificationWork *modelVerificationWorkCollector =
    nullptr;
thread_local const detail::ModelVerificationLimits *modelVerificationLimits =
    nullptr;

const detail::ModelVerificationLimits &currentModelVerificationLimits() {
  static constexpr detail::ModelVerificationLimits defaults;
  return modelVerificationLimits ? *modelVerificationLimits : defaults;
}
} // namespace

namespace detail {

ScopedModelVerificationWorkCollector::ScopedModelVerificationWorkCollector(
    ModelVerificationWork &work)
    : previous(modelVerificationWorkCollector) {
  modelVerificationWorkCollector = &work;
}

ScopedModelVerificationWorkCollector::~ScopedModelVerificationWorkCollector() {
  modelVerificationWorkCollector = previous;
}

ScopedModelVerificationLimits::ScopedModelVerificationLimits(
    const ModelVerificationLimits &limits)
    : previous(modelVerificationLimits) {
  modelVerificationLimits = &limits;
}

ScopedModelVerificationLimits::~ScopedModelVerificationLimits() {
  modelVerificationLimits = previous;
}

} // namespace detail

namespace {

constexpr StringLiteral kSourceMapAttrName = "acsim.source_map";

bool isSha256(StringRef value) {
  if (!value.consume_front("sha256:") || value.size() != 64)
    return false;
  return llvm::all_of(value, [](char c) {
    return std::isdigit(static_cast<unsigned char>(c)) ||
           (c >= 'a' && c <= 'f');
  });
}

LogicalResult verifyFingerprint(Operation *operation, StringAttr fingerprint,
                                StringRef label = "fingerprint") {
  if (fingerprint && isSha256(fingerprint.getValue()))
    return success();
  return operation->emitOpError() << label
                                  << " must be sha256: followed by 64 "
                                     "lowercase hexadecimal digits";
}

bool hasRawCppFragment(StringRef value) {
  return value.contains(';') || value.contains('{') || value.contains('}') ||
         value.contains('\n') || value.contains('#');
}

bool isCppQualifiedSymbol(StringRef value) {
  if (value.empty() || value.starts_with("::") || value.ends_with("::"))
    return false;
  while (!value.empty()) {
    auto [segment, remainder] = value.split("::");
    if (segment.empty() ||
        !(std::isalpha(static_cast<unsigned char>(segment.front())) ||
          segment.front() == '_') ||
        !llvm::all_of(segment.drop_front(), [](char character) {
          return std::isalnum(static_cast<unsigned char>(character)) ||
                 character == '_';
        }))
      return false;
    value = remainder;
  }
  return true;
}

bool isCanonicalIdentifier(StringRef value) {
  if (value.empty() ||
      !(std::isalpha(static_cast<unsigned char>(value.front())) ||
        value.front() == '_'))
    return false;
  return llvm::all_of(value.drop_front(), [](char character) {
    return std::isalnum(static_cast<unsigned char>(character)) ||
           character == '_';
  });
}

bool isCanonicalStaticData(Attribute root) {
  SmallVector<Attribute> stack{root};
  while (!stack.empty()) {
    Attribute attribute = stack.pop_back_val();
    if (isa<IntegerAttr, FloatAttr, BoolAttr, TypeAttr, SymbolRefAttr>(
            attribute))
      continue;
    if (auto string = dyn_cast<StringAttr>(attribute)) {
      if (hasRawCppFragment(string.getValue()) ||
          string.getValue().contains('(') || string.getValue().contains(')') ||
          string.getValue().contains('='))
        return false;
      continue;
    }
    if (auto array = dyn_cast<ArrayAttr>(attribute)) {
      stack.append(array.begin(), array.end());
      continue;
    }
    if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
      for (NamedAttribute named : dictionary) {
        if (!isCanonicalIdentifier(named.getName().getValue()))
          return false;
        stack.push_back(named.getValue());
      }
      continue;
    }
    return false;
  }
  return true;
}

bool hasExactKeys(DictionaryAttr dictionary, ArrayRef<StringLiteral> keys) {
  if (!dictionary || dictionary.size() != keys.size())
    return false;
  return llvm::all_of(keys,
                      [&](StringLiteral key) { return dictionary.get(key); });
}

LogicalResult verifyBindingLockShape(BindingOp binding) {
  constexpr std::array<StringLiteral, 20> topKeys = {
      "activation_sources",
      "availability",
      "binding",
      "binding_schema",
      "component_schema",
      "component_schema_fingerprint",
      "construction",
      "contract_epoch",
      "cpp",
      "cpp_type",
      "effect",
      "fingerprint",
      "implementation",
      "ownership",
      "parameters",
      "ports",
      "provider",
      "provider_implementation_fingerprint",
      "resources",
      "results",
  };
  DictionaryAttr record = binding.getRecord();
  if (!hasExactKeys(record, topKeys))
    return binding.emitOpError(
        "binding lock must contain exactly the acsim-binding-0.1 fields");
  auto identity = record.getAs<StringAttr>("binding");
  auto epoch = record.getAs<StringAttr>("contract_epoch");
  auto availability = record.getAs<StringAttr>("availability");
  if (!identity || identity.getValue() != binding.getSymName() ||
      binding.getBindingSchema() != "acsim-binding-0.1" || !epoch ||
      epoch.getValue() != "0.5" || !availability ||
      availability.getValue() != "available" ||
      (binding.getEffect() != "pure" && binding.getEffect() != "stateful") ||
      !binding.getCppTypeAttr() || !binding.getSchemaAttr() ||
      !binding.getProviderAttr() || !binding.getImplementationAttr())
    return binding.emitOpError(
        "binding lock identity, epoch, availability, and effect are invalid");
  for (StringLiteral field :
       {StringLiteral("component_schema_fingerprint"),
        StringLiteral("provider_implementation_fingerprint"),
        StringLiteral("fingerprint")}) {
    auto fingerprint = record.getAs<StringAttr>(field);
    if (!fingerprint || !isSha256(fingerprint.getValue()))
      return binding.emitOpError() << "binding lock field '" << field
                                   << "' requires an exact fingerprint";
  }

  constexpr std::array<StringLiteral, 5> cppKeys = {
      "concept", "entry_points", "header", "symbol", "target"};
  constexpr std::array<StringLiteral, 5> entryKeys = {
      "pure", "reset", "validate", "work", "xfer"};
  DictionaryAttr cpp = binding.getCppRecord();
  auto entries =
      cpp ? cpp.getAs<DictionaryAttr>("entry_points") : DictionaryAttr();
  if (!hasExactKeys(cpp, cppKeys) || !hasExactKeys(entries, entryKeys))
    return binding.emitOpError(
        "binding lock C++ record and entry points must be exact");
  for (StringLiteral field :
       {StringLiteral("concept"), StringLiteral("header"),
        StringLiteral("symbol"), StringLiteral("target")}) {
    auto value = cpp.getAs<StringAttr>(field);
    if (!value || value.getValue().empty() ||
        hasRawCppFragment(value.getValue()))
      return binding.emitOpError(
          "binding metadata cannot contain raw C++ or emitter behavior");
  }
  StringRef header = cpp.getAs<StringAttr>("header").getValue();
  if (header.starts_with('/') || header.contains('\\') ||
      header.starts_with("../") || header.contains("/../") ||
      header.ends_with("/..") || header == "..")
    return binding.emitOpError(
        "cpp.header must be a repository-relative header path");
  if (!isCppQualifiedSymbol(cpp.getAs<StringAttr>("concept").getValue()) ||
      !isCppQualifiedSymbol(cpp.getAs<StringAttr>("symbol").getValue()) ||
      !isCanonicalIdentifier(cpp.getAs<StringAttr>("target").getValue()))
    return binding.emitOpError(
        "C++ concept, symbol, and target must be declarative qualified names");
  for (StringLiteral field : entryKeys) {
    auto value = entries.getAs<StringAttr>(field);
    if (!value ||
        (!value.getValue().empty() && !isCppQualifiedSymbol(value.getValue())))
      return binding.emitOpError(
          "binding entry points must be empty or qualified C++ symbols");
  }
  if ((binding.getEffect() == "pure" &&
       (entries.getAs<StringAttr>("pure").getValue().empty() ||
        !entries.getAs<StringAttr>("reset").getValue().empty() ||
        !entries.getAs<StringAttr>("validate").getValue().empty() ||
        !entries.getAs<StringAttr>("work").getValue().empty() ||
        !entries.getAs<StringAttr>("xfer").getValue().empty())) ||
      (binding.getEffect() == "stateful" &&
       (!entries.getAs<StringAttr>("pure").getValue().empty() ||
        entries.getAs<StringAttr>("work").getValue().empty() ||
        entries.getAs<StringAttr>("xfer").getValue().empty())))
    return binding.emitOpError(
        "binding effect requires its exact executable entry points");

  constexpr std::array<StringLiteral, 2> constructionKeys = {"arguments",
                                                             "kind"};
  constexpr std::array<StringLiteral, 2> ownershipKeys = {"kind", "placement"};
  auto construction = record.getAs<DictionaryAttr>("construction");
  auto ownership = record.getAs<DictionaryAttr>("ownership");
  if (!hasExactKeys(construction, constructionKeys) ||
      !construction.getAs<ArrayAttr>("arguments") ||
      !construction.getAs<StringAttr>("kind") ||
      !hasExactKeys(ownership, ownershipKeys) ||
      !ownership.getAs<StringAttr>("kind") ||
      !ownership.getAs<StringAttr>("placement"))
    return binding.emitOpError(
        "binding construction and ownership records must be exact");
  auto constructionKind = construction.getAs<StringAttr>("kind");
  if (constructionKind.getValue() != "constructor")
    return binding.emitOpError(
        "construction kind must be exactly 'constructor'");
  for (Attribute argument : construction.getAs<ArrayAttr>("arguments"))
    if (!isCanonicalStaticData(argument))
      return binding.emitOpError(
          "construction arguments must be canonical static data");
  auto ownershipKind = ownership.getAs<StringAttr>("kind");
  auto ownershipPlacement = ownership.getAs<StringAttr>("placement");
  if (binding.getEffect() == "pure" &&
      (ownershipKind.getValue() != "none" ||
       ownershipPlacement.getValue() != "inline"))
    return binding.emitOpError(
        "pure binding ownership must be exactly none/inline");
  if (binding.getEffect() == "stateful" &&
      (ownershipKind.getValue() != "unique" ||
       !llvm::is_contained(
           {StringRef("member_or_array"), StringRef("root_or_process")},
           ownershipPlacement.getValue())))
    return binding.emitOpError(
        "stateful binding ownership must be exactly unique/member_or_array or "
        "unique/root_or_process");

  constexpr std::array<StringLiteral, 6> parameterKeys = {
      "acir_type", "cpp_type", "mapping", "name", "ordinal", "value"};
  auto parameters = record.getAs<ArrayAttr>("parameters");
  if (!parameters)
    return binding.emitOpError("binding parameters must be a static array");
  int64_t expectedOrdinal = 0;
  llvm::StringSet<> parameterNames;
  for (Attribute attribute : parameters) {
    auto parameter = dyn_cast<DictionaryAttr>(attribute);
    auto name = parameter ? parameter.getAs<StringAttr>("name") : StringAttr();
    auto ordinal =
        parameter ? parameter.getAs<IntegerAttr>("ordinal") : IntegerAttr();
    auto mapping =
        parameter ? parameter.getAs<StringAttr>("mapping") : StringAttr();
    if (!hasExactKeys(parameter, parameterKeys) || !name || !ordinal ||
        ordinal.getInt() != expectedOrdinal++ || !mapping ||
        !parameter.getAs<StringAttr>("acir_type") ||
        !parameter.getAs<StringAttr>("cpp_type") || !parameter.get("value") ||
        !parameterNames.insert(name.getValue()).second)
      return binding.emitOpError(
          "binding lock parameter must contain exact static fields");
    if (!isCanonicalIdentifier(name.getValue()))
      return binding.emitOpError(
          "parameter name must be a canonical identifier");
    if (parameter.getAs<StringAttr>("acir_type").getValue().empty() ||
        parameter.getAs<StringAttr>("cpp_type").getValue().empty() ||
        !isCanonicalStaticData(parameter.get("value")))
      return binding.emitOpError(
          "parameter types and value must be non-empty canonical static data");
    if (!llvm::is_contained({StringRef("template_argument"),
                             StringRef("constexpr_argument"),
                             StringRef("constructor_constant")},
                            mapping.getValue()))
      return binding.emitOpError("parameter mapping must be template_argument, "
                                 "constexpr_argument, or constructor_constant");
  }
  auto verifyRecordArray = [&](StringRef name,
                               ArrayRef<StringLiteral> keys) -> LogicalResult {
    auto records = record.getAs<ArrayAttr>(name);
    if (!records)
      return binding.emitOpError()
             << "binding " << name << " must be a static record array";
    for (Attribute attribute : records)
      if (!hasExactKeys(dyn_cast<DictionaryAttr>(attribute), keys))
        return binding.emitOpError()
               << "binding " << name
               << " records must have exact closed fields";
    return success();
  };
  constexpr std::array<StringLiteral, 10> portKeys = {
      "accessor",  "cardinality", "delegation", "direction", "interface",
      "ownership", "payload",     "protocol",   "role",      "time_domain"};
  constexpr std::array<StringLiteral, 7> resourceKeys = {
      "accessor", "delegation", "mode",       "ownership",
      "resource", "role",       "time_domain"};
  constexpr std::array<StringLiteral, 2> resultKeys = {"cpp_type", "name"};
  constexpr std::array<StringLiteral, 2> activationKeys = {"kind", "name"};
  if (failed(verifyRecordArray("ports", portKeys)) ||
      failed(verifyRecordArray("resources", resourceKeys)) ||
      failed(verifyRecordArray("results", resultKeys)) ||
      failed(verifyRecordArray("activation_sources", activationKeys)))
    return failure();
  if (binding.getEffect() == "pure" &&
      !record.getAs<ArrayAttr>("activation_sources").empty())
    return binding.emitOpError(
        "pure binding cannot contain activation or wakeup metadata");
  llvm::StringSet<> portAccessors;
  for (Attribute attribute : record.getAs<ArrayAttr>("ports")) {
    auto port = cast<DictionaryAttr>(attribute);
    auto direction = port.getAs<StringAttr>("direction");
    auto accessor = port.getAs<FlatSymbolRefAttr>("accessor");
    if (!accessor || !portAccessors.insert(accessor.getValue()).second ||
        !port.getAs<FlatSymbolRefAttr>("interface") ||
        !port.getAs<FlatSymbolRefAttr>("role") ||
        !port.getAs<FlatSymbolRefAttr>("payload") ||
        !port.getAs<FlatSymbolRefAttr>("protocol") || !direction ||
        !llvm::is_contained({StringRef("input"), StringRef("output")},
                            direction.getValue()) ||
        !port.get("cardinality") || !port.getAs<StringAttr>("delegation") ||
        !port.getAs<StringAttr>("ownership") || !port.get("time_domain"))
      return binding.emitOpError(
          "binding port records require exact typed endpoint metadata");
    if (!port.getAs<FlatSymbolRefAttr>("time_domain"))
      return binding.emitOpError("time_domain must be a flat symbol reference");
    auto cardinality = port.getAs<StringAttr>("cardinality");
    if (!cardinality ||
        !llvm::is_contained({StringRef("exclusive"), StringRef("shared")},
                            cardinality.getValue()))
      return binding.emitOpError(
          "cardinality must be exactly 'exclusive' or 'shared'");
    auto delegation = port.getAs<StringAttr>("delegation");
    if (!llvm::is_contained({StringRef("forbidden"), StringRef("allowed"),
                             StringRef("required")},
                            delegation.getValue()))
      return binding.emitOpError(
          "delegation must be forbidden, allowed, or required");
    auto endpointOwnership = port.getAs<StringAttr>("ownership");
    if (!llvm::is_contained(
            {StringRef("owned"), StringRef("borrowed"), StringRef("shared")},
            endpointOwnership.getValue()))
      return binding.emitOpError(
          "endpoint ownership must be owned, borrowed, or shared");
  }
  llvm::StringSet<> resourceAccessors;
  for (Attribute attribute : record.getAs<ArrayAttr>("resources")) {
    auto resource = cast<DictionaryAttr>(attribute);
    auto mode = resource.getAs<StringAttr>("mode");
    auto accessor = resource.getAs<FlatSymbolRefAttr>("accessor");
    if (!accessor || !resourceAccessors.insert(accessor.getValue()).second ||
        !resource.getAs<FlatSymbolRefAttr>("resource") ||
        !resource.getAs<FlatSymbolRefAttr>("role") || !mode ||
        !llvm::is_contained({StringRef("initiator"), StringRef("target")},
                            mode.getValue()) ||
        !resource.getAs<StringAttr>("delegation") ||
        !resource.getAs<StringAttr>("ownership") ||
        !resource.get("time_domain"))
      return binding.emitOpError(
          "binding resource records require exact typed endpoint metadata");
    if (!resource.getAs<FlatSymbolRefAttr>("time_domain"))
      return binding.emitOpError("time_domain must be a flat symbol reference");
    auto delegation = resource.getAs<StringAttr>("delegation");
    if (!llvm::is_contained({StringRef("forbidden"), StringRef("allowed"),
                             StringRef("required")},
                            delegation.getValue()))
      return binding.emitOpError(
          "delegation must be forbidden, allowed, or required");
    auto endpointOwnership = resource.getAs<StringAttr>("ownership");
    if (!llvm::is_contained(
            {StringRef("owned"), StringRef("borrowed"), StringRef("shared")},
            endpointOwnership.getValue()))
      return binding.emitOpError(
          "endpoint ownership must be owned, borrowed, or shared");
  }
  llvm::StringSet<> resultNames;
  for (Attribute attribute : record.getAs<ArrayAttr>("results")) {
    auto result = cast<DictionaryAttr>(attribute);
    auto name = result.getAs<StringAttr>("name");
    if (!result.getAs<FlatSymbolRefAttr>("cpp_type") || !name ||
        !isCanonicalIdentifier(name.getValue()) ||
        !resultNames.insert(name.getValue()).second)
      return binding.emitOpError(
          "result name must be a canonical identifier and result metadata "
          "must be exact");
  }
  llvm::StringSet<> activationNames;
  for (Attribute attribute : record.getAs<ArrayAttr>("activation_sources")) {
    auto source = cast<DictionaryAttr>(attribute);
    auto name = source.getAs<StringAttr>("name");
    if (!source.getAs<FlatSymbolRefAttr>("kind") || !name ||
        !isCanonicalIdentifier(name.getValue()) ||
        !activationNames.insert(name.getValue()).second)
      return binding.emitOpError(
          "activation-source name must be a canonical identifier and source "
          "metadata must be exact");
  }
  return success();
}

LogicalResult verifyModuleInterfaceShape(ModuleOp module) {
  constexpr std::array<StringLiteral, 3> interfaceKeys = {"ports", "resources",
                                                          "results"};
  constexpr std::array<StringLiteral, 11> portKeys = {
      "accessor",  "cardinality", "delegation", "direction",
      "interface", "name",        "ownership",  "payload",
      "protocol",  "role",        "time_domain"};
  constexpr std::array<StringLiteral, 8> resourceKeys = {
      "accessor",  "delegation", "mode", "name",
      "ownership", "resource",   "role", "time_domain"};
  constexpr std::array<StringLiteral, 2> resultKeys = {"cpp_type", "name"};
  DictionaryAttr interface = module.getInterface();
  if (!hasExactKeys(interface, interfaceKeys))
    return module.emitOpError(
        "interface must contain exactly ports, resources, and results");
  auto ports = interface.getAs<ArrayAttr>("ports");
  auto resources = interface.getAs<ArrayAttr>("resources");
  auto results = interface.getAs<ArrayAttr>("results");
  if (!ports || !resources || !results)
    return module.emitOpError("interface fields must be ordered record arrays");

  llvm::StringSet<> accessors;
  auto verifyNames = [&](ArrayAttr records, ArrayRef<StringLiteral> keys,
                         StringRef kind) -> LogicalResult {
    StringRef previous;
    for (Attribute attribute : records) {
      auto record = dyn_cast<DictionaryAttr>(attribute);
      auto name = record ? record.getAs<StringAttr>("name") : StringAttr();
      if (!hasExactKeys(record, keys) || !name ||
          !isCanonicalIdentifier(name.getValue()))
        return module.emitOpError()
               << kind << " interface records require exact closed fields and "
               << "canonical names";
      if (!previous.empty() && name.getValue() <= previous)
        return module.emitOpError()
               << kind << " interface records must be strictly name-sorted";
      previous = name.getValue();
    }
    return success();
  };
  if (failed(verifyNames(ports, portKeys, "port")) ||
      failed(verifyNames(resources, resourceKeys, "resource")) ||
      failed(verifyNames(results, resultKeys, "result")))
    return failure();

  for (Attribute attribute : ports) {
    auto record = cast<DictionaryAttr>(attribute);
    auto accessor = record.getAs<FlatSymbolRefAttr>("accessor");
    auto cardinality = record.getAs<StringAttr>("cardinality");
    auto delegation = record.getAs<StringAttr>("delegation");
    auto direction = record.getAs<StringAttr>("direction");
    auto ownership = record.getAs<StringAttr>("ownership");
    if (!accessor || !accessors.insert(accessor.getValue()).second ||
        !record.getAs<FlatSymbolRefAttr>("interface") ||
        !record.getAs<FlatSymbolRefAttr>("payload") ||
        !record.getAs<FlatSymbolRefAttr>("protocol") ||
        !record.getAs<FlatSymbolRefAttr>("role") ||
        !record.getAs<FlatSymbolRefAttr>("time_domain") || !cardinality ||
        !delegation || !direction || !ownership ||
        !llvm::is_contained({StringRef("exclusive"), StringRef("shared")},
                            cardinality.getValue()) ||
        !llvm::is_contained({StringRef("forbidden"), StringRef("allowed"),
                             StringRef("required")},
                            delegation.getValue()) ||
        !llvm::is_contained({StringRef("input"), StringRef("output")},
                            direction.getValue()) ||
        !llvm::is_contained(
            {StringRef("owned"), StringRef("borrowed"), StringRef("shared")},
            ownership.getValue()))
      return module.emitOpError(
          "port interface records require exact typed endpoint metadata and "
          "globally unique accessors");
  }
  for (Attribute attribute : resources) {
    auto record = cast<DictionaryAttr>(attribute);
    auto accessor = record.getAs<FlatSymbolRefAttr>("accessor");
    auto delegation = record.getAs<StringAttr>("delegation");
    auto mode = record.getAs<StringAttr>("mode");
    auto ownership = record.getAs<StringAttr>("ownership");
    if (!accessor || !accessors.insert(accessor.getValue()).second ||
        !record.getAs<FlatSymbolRefAttr>("resource") ||
        !record.getAs<FlatSymbolRefAttr>("role") ||
        !record.getAs<FlatSymbolRefAttr>("time_domain") || !delegation ||
        !mode || !ownership ||
        !llvm::is_contained({StringRef("forbidden"), StringRef("allowed"),
                             StringRef("required")},
                            delegation.getValue()) ||
        !llvm::is_contained({StringRef("initiator"), StringRef("target")},
                            mode.getValue()) ||
        !llvm::is_contained(
            {StringRef("owned"), StringRef("borrowed"), StringRef("shared")},
            ownership.getValue()))
      return module.emitOpError(
          "resource interface records require exact typed endpoint metadata "
          "and globally unique accessors");
  }
  for (Attribute attribute : results)
    if (!cast<DictionaryAttr>(attribute).getAs<FlatSymbolRefAttr>("cpp_type"))
      return module.emitOpError(
          "result interface records require an exact C++ type realization");
  return success();
}

std::string symbolKey(SymbolRefAttr reference) {
  std::string result = reference.getRootReference().getValue().str();
  for (FlatSymbolRefAttr nested : reference.getNestedReferences()) {
    result.append("::");
    result.append(nested.getValue());
  }
  return result;
}

StringAttr symbolName(Operation *operation) {
  return operation->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
}

ModuleOp enclosingConstructionModule(Operation *operation) {
  return operation->getParentOfType<ModuleOp>();
}

std::string definitionKey(Operation *operation) {
  StringAttr name = symbolName(operation);
  if (!name)
    return {};
  if (isa<TypeOp, BindingOp, ModuleOp>(operation))
    return name.getValue().str();
  if (ModuleOp module = enclosingConstructionModule(operation)) {
    std::string key = module.getSymName().str();
    key.append("::");
    key.append(name.getValue());
    return key;
  }
  return name.getValue().str();
}

uint64_t arrayVolume(ArrayRef<int64_t> shape) {
  uint64_t volume = 1;
  for (int64_t extent : shape) {
    if (extent == 0)
      return 0;
    volume *= static_cast<uint64_t>(extent);
  }
  return volume;
}

uint64_t arrayVolume(DenseI64ArrayAttr shape) {
  return arrayVolume(shape.asArrayRef());
}

SmallVector<int64_t> lexicographicIndices(ArrayRef<int64_t> shape,
                                          uint64_t ordinal) {
  SmallVector<int64_t> indices(shape.size(), 0);
  for (size_t index = shape.size(); index > 0; --index) {
    uint64_t extent = static_cast<uint64_t>(shape[index - 1]);
    if (!extent)
      return indices;
    indices[index - 1] = static_cast<int64_t>(ordinal % extent);
    ordinal /= extent;
  }
  return indices;
}

bool lexicographicallyLess(ArrayRef<int64_t> left, ArrayRef<int64_t> right) {
  return std::lexicographical_compare(left.begin(), left.end(), right.begin(),
                                      right.end());
}

struct ModelIndex {
  SmallVector<Operation *> ordered;
  llvm::StringMap<Operation *> definitions;
  llvm::DenseMap<Operation *, uint64_t> positions;
};

LogicalResult verifySourceMap(Operation *operation, Attribute attribute) {
  auto records = dyn_cast<ArrayAttr>(attribute);
  constexpr std::array<StringLiteral, 5> keys = {"column", "end_column",
                                                 "end_line", "file", "line"};
  if (!records)
    return operation->emitOpError(
        "acsim.source_map must be an array of exact source records");
  for (Attribute item : records) {
    auto record = dyn_cast<DictionaryAttr>(item);
    auto file = record ? record.getAs<StringAttr>("file") : StringAttr();
    auto line = record ? record.getAs<IntegerAttr>("line") : IntegerAttr();
    auto column = record ? record.getAs<IntegerAttr>("column") : IntegerAttr();
    auto endLine =
        record ? record.getAs<IntegerAttr>("end_line") : IntegerAttr();
    auto endColumn =
        record ? record.getAs<IntegerAttr>("end_column") : IntegerAttr();
    if (!hasExactKeys(record, keys) || !file || file.getValue().empty() ||
        !line || !column || !endLine || !endColumn || line.getInt() <= 0 ||
        column.getInt() <= 0 || endLine.getInt() < line.getInt() ||
        endColumn.getInt() <= 0 ||
        (endLine.getInt() == line.getInt() &&
         endColumn.getInt() < column.getInt()))
      return operation->emitOpError(
          "acsim.source_map records require exact ordered positive ranges");
  }
  return success();
}

LogicalResult preflightModel(ModelOp model) {
  struct Frame {
    Operation *operation;
    uint64_t depth;
  };
  SmallVector<Frame> stack{{model.getOperation(), 0}};
  uint64_t nodes = 0;
  uint64_t edges = 0;
  uint64_t totalArrayVolume = 0;
  uint64_t attributeElements = 0;
  uint64_t attributeStringBytes = 0;
  const detail::ModelVerificationLimits &limits =
      currentModelVerificationLimits();

  while (!stack.empty()) {
    Frame frame = stack.pop_back_val();
    if (modelVerificationWorkCollector)
      ++modelVerificationWorkCollector->preflightOperationVisits;
    if (++nodes > limits.maxNodes)
      return model.emitOpError() << "model node count exceeds ACSim "
                                    "capability "
                                 << limits.maxNodes;
    if (frame.depth > limits.maxRegionDepth)
      return frame.operation->emitOpError()
             << "region nesting exceeds ACSim capability "
             << limits.maxRegionDepth;
    SmallVector<Attribute> attributeStack;
    if (frame.operation->getAttrs().size() >
        limits.maxAttributeElements - attributeElements)
      return frame.operation->emitOpError(
          "attribute element count exceeds ACSim capability");
    for (NamedAttribute named : frame.operation->getAttrs()) {
      if (named.getName().size() >
          limits.maxAttributeStringBytes - attributeStringBytes)
        return frame.operation->emitOpError(
            "attribute string bytes exceed ACSim capability");
      attributeStringBytes += named.getName().size();
      attributeStack.push_back(named.getValue());
    }
    while (!attributeStack.empty()) {
      Attribute attribute = attributeStack.pop_back_val();
      if (++attributeElements > limits.maxAttributeElements)
        return frame.operation->emitOpError(
            "attribute element count exceeds ACSim capability");
      auto addString = [&](StringRef value) -> LogicalResult {
        if (value.size() >
            limits.maxAttributeStringBytes - attributeStringBytes)
          return frame.operation->emitOpError(
              "attribute string bytes exceed ACSim capability");
        attributeStringBytes += value.size();
        return success();
      };
      if (auto string = dyn_cast<StringAttr>(attribute)) {
        if (failed(addString(string.getValue())))
          return failure();
      } else if (auto symbol = dyn_cast<SymbolRefAttr>(attribute)) {
        if (failed(addString(symbolKey(symbol))))
          return failure();
      } else if (auto array = dyn_cast<ArrayAttr>(attribute)) {
        if (attributeStack.size() >
                limits.maxAttributeElements - attributeElements ||
            array.size() > limits.maxAttributeElements - attributeElements -
                               attributeStack.size())
          return frame.operation->emitOpError(
              "attribute element count exceeds ACSim capability");
        attributeStack.append(array.begin(), array.end());
      } else if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
        for (NamedAttribute named : dictionary) {
          if (failed(addString(named.getName().getValue())))
            return failure();
          if (attributeStack.size() >=
              limits.maxAttributeElements - attributeElements)
            return frame.operation->emitOpError(
                "attribute element count exceeds ACSim capability");
          attributeStack.push_back(named.getValue());
        }
      } else if (auto dense = dyn_cast<DenseArrayAttr>(attribute)) {
        uint64_t count = dense.size();
        if (count > limits.maxAttributeElements - attributeElements)
          return frame.operation->emitOpError(
              "attribute element count exceeds ACSim capability");
        attributeElements += count;
      } else if (auto dense = dyn_cast<DenseElementsAttr>(attribute)) {
        uint64_t count = dense.getNumElements();
        if (count > limits.maxAttributeElements - attributeElements)
          return frame.operation->emitOpError(
              "attribute element count exceeds ACSim capability");
        attributeElements += count;
      }
    }
    uint64_t operandEdges = frame.operation->getNumOperands();
    if (modelVerificationWorkCollector)
      modelVerificationWorkCollector->edgeVisits += operandEdges;
    if (operandEdges > limits.maxEdges ||
        edges > limits.maxEdges - operandEdges)
      return frame.operation->emitOpError()
             << "model edge count exceeds ACSim capability " << limits.maxEdges;
    edges += operandEdges;
    if (auto array = dyn_cast<ArrayOp>(frame.operation)) {
      auto type = dyn_cast<ArrayType>(array.getResult().getType());
      if (type) {
        uint64_t volume = 1;
        for (int64_t extent : type.getShape().asArrayRef()) {
          if (extent < 0 ||
              (extent != 0 && volume > limits.maxExpandedObjects /
                                           static_cast<uint64_t>(extent)))
            return array.emitOpError()
                   << "expanded array volume exceeds ACSim capability "
                   << limits.maxExpandedObjects;
          volume *= static_cast<uint64_t>(extent);
        }
        if (volume > limits.maxExpandedObjects ||
            totalArrayVolume > limits.maxExpandedObjects - volume)
          return array.emitOpError()
                 << "expanded array volume exceeds ACSim capability "
                 << limits.maxExpandedObjects;
        totalArrayVolume += volume;
      }
    }
    for (Region &region : frame.operation->getRegions()) {
      for (Block &block : region) {
        if (Operation *terminator = block.getTerminator()) {
          uint64_t successors = terminator->getNumSuccessors();
          if (modelVerificationWorkCollector)
            modelVerificationWorkCollector->edgeVisits += successors;
          if (successors > limits.maxEdges ||
              edges > limits.maxEdges - successors)
            return terminator->emitOpError()
                   << "model edge count exceeds ACSim capability "
                   << limits.maxEdges;
          edges += successors;
        }
        for (Operation &child : llvm::reverse(block)) {
          if (stack.size() >= limits.maxNodes - nodes)
            return frame.operation->emitOpError()
                   << "model node count exceeds ACSim capability "
                   << limits.maxNodes;
          stack.push_back({&child, frame.depth + 1});
        }
      }
    }
  }
  return success();
}

SmallVector<Operation *> collectPreorder(ModelOp model) {
  SmallVector<Operation *> result;
  SmallVector<Operation *> stack{model.getOperation()};
  while (!stack.empty()) {
    Operation *operation = stack.pop_back_val();
    if (modelVerificationWorkCollector)
      ++modelVerificationWorkCollector->preorderOperationVisits;
    result.push_back(operation);
    for (Region &region : llvm::reverse(operation->getRegions()))
      for (Block &block : llvm::reverse(region))
        for (Operation &child : llvm::reverse(block))
          stack.push_back(&child);
  }
  return result;
}

bool isProcessOperation(Operation *operation) {
  return isa<InlineOp, LiveLoadOp, LiveStoreOp, InvokeOp, ContinueOp, SuspendOp,
             TerminateOp>(operation);
}

bool isModuleOperation(Operation *operation) {
  return isa<InstanceOp, ArrayOp, ElementOp, PortOp, ResourceOp, BindOp,
             InlineOp, ProcessOp, ExportOp, ReturnOp>(operation);
}

bool isModelOperation(Operation *operation) {
  return isa<TypeOp, BindingOp, ModuleOp, DispatchOp, ActivateOp>(operation);
}

LogicalResult verifyClosedLegality(ModelOp model,
                                   ArrayRef<Operation *> ordered) {
  for (Operation *operation : ordered) {
    if (modelVerificationWorkCollector)
      ++modelVerificationWorkCollector->closureOperationVisits;
    for (NamedAttribute attribute : operation->getDiscardableAttrs()) {
      StringRef name = attribute.getName().getValue();
      if (name != kSourceMapAttrName)
        return operation->emitOpError() << "unknown public attribute '" << name
                                        << "' is not legal in canonical ACSim";
      if (failed(verifySourceMap(operation, attribute.getValue())))
        return failure();
    }

    if (operation == model.getOperation())
      continue;

    if (ProcessOp process = operation->getParentOfType<ProcessOp>()) {
      (void)process;
      if (isProcessOperation(operation))
        continue;
      if (isa<cf::BranchOp, cf::CondBranchOp>(operation))
        continue;
      if (isa<UnrealizedConversionCastOp>(operation))
        return operation->emitOpError(
            "conversion placeholders are not legal in canonical ACSim");
      StringRef dialect = operation->getName().getDialectNamespace();
      if ((dialect == "builtin" || dialect == "arith" || dialect == "index" ||
           dialect == "cf") &&
          isMemoryEffectFree(operation) && operation->getNumRegions() == 0)
        continue;
      return operation->emitOpError()
             << "operation '" << operation->getName()
             << "' is not legal in an acsim.process body";
    }

    Operation *parent = operation->getParentOp();
    if (parent == model.getOperation() && isModelOperation(operation))
      continue;
    if (isa<ModuleOp>(parent) && isModuleOperation(operation))
      continue;
    return operation->emitOpError() << "operation '" << operation->getName()
                                    << "' is not legal in canonical ACSim";
  }
  return success();
}

LogicalResult buildIndex(ModelOp model, ModelIndex &index) {
  index.ordered = collectPreorder(model);
  uint64_t position = 0;
  for (Operation *operation : index.ordered) {
    if (modelVerificationWorkCollector)
      ++modelVerificationWorkCollector->indexOperationVisits;
    index.positions[operation] = position++;
  }

  auto addDefinition = [&](Operation *operation) -> LogicalResult {
    std::string key = definitionKey(operation);
    if (key.empty())
      return success();
    auto [iterator, inserted] = index.definitions.try_emplace(key, operation);
    if (!inserted)
      return operation->emitOpError()
             << "duplicate canonical symbol or placement '" << key << "'";
    return success();
  };

  for (Operation *operation : index.ordered) {
    if (modelVerificationWorkCollector)
      ++modelVerificationWorkCollector->indexOperationVisits;
    if (isa<TypeOp, BindingOp, ModuleOp, InstanceOp, ArrayOp, ProcessOp>(
            operation) &&
        failed(addDefinition(operation)))
      return failure();
  }
  return success();
}

Operation *resolveReference(const ModelIndex &index, Operation *from,
                            SymbolRefAttr reference) {
  if (modelVerificationWorkCollector)
    ++modelVerificationWorkCollector->referenceLookups;
  std::string key = symbolKey(reference);
  if (!reference.getNestedReferences().empty())
    return index.definitions.lookup(key);
  if (ModuleOp module = enclosingConstructionModule(from)) {
    std::string local = module.getSymName().str();
    local.append("::");
    local.append(key);
    if (Operation *definition = index.definitions.lookup(local))
      return definition;
  }
  return index.definitions.lookup(key);
}

template <typename... Expected>
FailureOr<Operation *>
requireReference(const ModelIndex &index, Operation *from,
                 SymbolRefAttr reference, StringRef label,
                 bool requireEarlier = true) {
  Operation *definition = resolveReference(index, from, reference);
  if (!definition)
    return from->emitOpError()
           << label << " reference '" << reference << "' is unresolved";
  if (!isa<Expected...>(definition))
    return from->emitOpError() << label << " reference '" << reference
                               << "' resolves to incompatible operation '"
                               << definition->getName() << "'";
  if (requireEarlier && definition != from &&
      index.positions.lookup(definition) >= index.positions.lookup(from))
    return from->emitOpError() << label << " reference '" << reference
                               << "' appears before its construction";
  return definition;
}

FailureOr<Operation *> requireCallCallee(const ModelIndex &index,
                                         Operation *call,
                                         FlatSymbolRefAttr callee) {
  Operation *definition = resolveReference(index, call, callee);
  if (!definition)
    return call->emitOpError()
           << "callee reference '" << callee << "' is unresolved";
  if (!isa<BindingOp, TypeOp>(definition))
    return call->emitOpError() << "callee reference '" << callee
                               << "' resolves to incompatible operation '"
                               << definition->getName() << "'";
  if (definition != call &&
      index.positions.lookup(definition) >= index.positions.lookup(call))
    return call->emitOpError() << "callee reference '" << callee
                               << "' appears before its construction";
  return definition;
}

LogicalResult requireTypeKind(const ModelIndex &index, Operation *from,
                              SymbolRefAttr reference,
                              ArrayRef<StringRef> allowedKinds,
                              StringRef label) {
  FailureOr<Operation *> definition =
      requireReference<TypeOp>(index, from, reference, label);
  if (failed(definition))
    return failure();
  StringRef kind = cast<TypeOp>(*definition).getKind();
  if (llvm::is_contained(allowedKinds, kind))
    return success();
  return from->emitOpError()
         << label << " reference '" << reference
         << "' has incompatible acsim.type kind '" << kind << "'";
}

LogicalResult verifyCanonicalType(Type type, Operation *from,
                                  const ModelIndex &index) {
  return llvm::TypeSwitch<Type, LogicalResult>(type)
      .Case<ValueType, ExprType>([&](auto valueType) {
        const std::array<StringRef, 2> kinds = {"value", "packet"};
        return requireTypeKind(index, from, valueType.getSymbol(), kinds,
                               "C++ type");
      })
      .Case<OwnerType, RefType>([&](auto ownerType) -> LogicalResult {
        FailureOr<Operation *> definition =
            requireReference<BindingOp, ModuleOp>(
                index, from, ownerType.getRealization(), "realization");
        if (failed(definition))
          return failure();
        if (auto binding = dyn_cast<BindingOp>(*definition);
            binding && binding.getEffect() != "stateful")
          return from->emitOpError() << "owner/ref type requires a generated "
                                        "module or stateful binding";
        return success();
      })
      .Case<PortType>([&](PortType port) {
        const std::array<StringRef, 1> interfaceKinds = {"interface"};
        const std::array<StringRef, 1> roleKinds = {"role"};
        const std::array<StringRef, 2> payloadKinds = {"value", "packet"};
        const std::array<StringRef, 1> protocolKinds = {"protocol"};
        if (failed(requireTypeKind(index, from, port.getInterface(),
                                   interfaceKinds, "interface")) ||
            failed(requireTypeKind(index, from, port.getRole(), roleKinds,
                                   "role")) ||
            failed(requireTypeKind(index, from, port.getPayload(), payloadKinds,
                                   "payload")) ||
            failed(requireTypeKind(index, from, port.getProtocol(),
                                   protocolKinds, "protocol")))
          return failure();
        return success();
      })
      .Case<ResourceType>([&](ResourceType resource) {
        const std::array<StringRef, 1> resourceKinds = {"resource"};
        const std::array<StringRef, 1> roleKinds = {"role"};
        return success(
            succeeded(requireTypeKind(index, from, resource.getResource(),
                                      resourceKinds, "resource")) &&
            succeeded(requireTypeKind(index, from, resource.getRole(),
                                      roleKinds, "role")));
      })
      .Case<ArrayType>([&](ArrayType array) {
        return verifyCanonicalType(array.getElementType(), from, index);
      })
      .Case<PcType>([&](PcType pc) {
        return success(succeeded(requireReference<ProcessOp>(
            index, from, pc.getSymbol(), "process")));
      })
      .Case<WakeType>([&](WakeType wake) {
        const std::array<StringRef, 1> kinds = {"wake"};
        return requireTypeKind(index, from, wake.getSymbol(), kinds,
                               "wake kind");
      })
      .Case<ObjectIdType, ActivationIdType>([](auto) { return success(); })
      .Default([&](Type other) -> LogicalResult {
        if ((isa<ProcessOp>(from) || from->getParentOfType<ProcessOp>() ||
             isa<InlineOp>(from)) &&
            isa<IntegerType, FloatType, IndexType>(other))
          return success();
        return from->emitOpError()
               << "type '" << other << "' is not legal in canonical ACSim";
      });
}

LogicalResult verifyModelFingerprints(ModelOp model) {
  constexpr std::array<StringLiteral, 6> expected = {
      "binding_lock", "frozen_acir", "profile",
      "provider",     "schema_set",  "toolchain"};
  DictionaryAttr fingerprints = model.getFingerprints();
  if (!fingerprints || fingerprints.size() != expected.size())
    return model.emitOpError(
        "fingerprints must contain exactly frozen_acir, binding_lock, "
        "provider, profile, toolchain, and schema_set");
  for (StringLiteral name : expected) {
    auto value = fingerprints.getAs<StringAttr>(name);
    if (!value || !isSha256(value.getValue()))
      return model.emitOpError()
             << "fingerprint field '" << name
             << "' must be sha256: followed by 64 lowercase hexadecimal "
                "digits";
  }
  return success();
}

unsigned modelRank(Operation *operation) {
  return llvm::TypeSwitch<Operation *, unsigned>(operation)
      .Case<TypeOp>([](auto) { return 0; })
      .Case<BindingOp>([](auto) { return 1; })
      .Case<ModuleOp>([](auto) { return 2; })
      .Case<DispatchOp>([](auto) { return 3; })
      .Case<ActivateOp>([](auto) { return 4; })
      .Default([](auto) { return 5; });
}

unsigned moduleRank(Operation *operation) {
  if (auto bind = dyn_cast<BindOp>(operation)) {
    if (bind.getKind() == "pure_view")
      return 5;
    if (bind.getKind() == "export")
      return 7;
    return 3;
  }
  return llvm::TypeSwitch<Operation *, unsigned>(operation)
      .Case<InstanceOp, ArrayOp>([](auto) { return 0; })
      .Case<ElementOp>([](auto) { return 1; })
      .Case<PortOp, ResourceOp>([](auto) { return 2; })
      .Case<InlineOp>([](auto) { return 4; })
      .Case<ExportOp>([](auto) { return 6; })
      .Case<ProcessOp>([](auto) { return 8; })
      .Case<ReturnOp>([](auto) { return 100; })
      .Default([](auto) { return 99; });
}

LogicalResult verifyDeterministicOrder(ModelOp model) {
  unsigned previousRank = 0;
  std::string previousName;
  bool first = true;
  for (Operation &operation : model.getBody().front()) {
    if (modelVerificationWorkCollector)
      ++modelVerificationWorkCollector->orderingOperationVisits;
    unsigned rank = modelRank(&operation);
    if (!first && rank < previousRank)
      return operation.emitOpError(
          "model declarations are not in deterministic canonical order");
    StringAttr name = symbolName(&operation);
    if (!first && rank == previousRank && rank != 2 && name &&
        name.getValue() <= previousName)
      return operation.emitOpError(
          "same-kind model declarations must be strictly symbol-sorted");
    previousRank = rank;
    previousName = name ? name.getValue().str() : std::string();
    first = false;
  }

  SmallVector<ModuleOp> moduleOrder;
  llvm::StringMap<unsigned> moduleIndex;
  for (ModuleOp module : model.getOps<ModuleOp>()) {
    moduleIndex[module.getSymName()] = moduleOrder.size();
    moduleOrder.push_back(module);
  }
  SmallVector<uint32_t> dependencyCount(moduleOrder.size());
  SmallVector<SmallVector<unsigned, 2>> parentsByChild(moduleOrder.size());
  for (auto [ownerIndex, module] : llvm::enumerate(moduleOrder)) {
    llvm::SmallSet<unsigned, 8> dependencies;
    for (Operation &operation : module.getBody().front()) {
      SymbolRefAttr target;
      if (auto instance = dyn_cast<InstanceOp>(operation))
        target = instance.getTargetAttr();
      else if (auto array = dyn_cast<ArrayOp>(operation))
        target = array.getTargetAttr();
      if (!target)
        continue;
      auto child = moduleIndex.find(target.getRootReference().getValue());
      if (child != moduleIndex.end())
        dependencies.insert(child->second);
    }
    dependencyCount[ownerIndex] = dependencies.size();
    for (unsigned childIndex : dependencies)
      parentsByChild[childIndex].push_back(ownerIndex);
  }
  std::set<std::pair<std::string, unsigned>> readyModules;
  for (auto [index, module] : llvm::enumerate(moduleOrder))
    if (dependencyCount[index] == 0)
      readyModules.emplace(module.getSymName().str(), index);
  SmallVector<unsigned> expectedModuleOrder;
  while (!readyModules.empty()) {
    unsigned childIndex = readyModules.begin()->second;
    readyModules.erase(readyModules.begin());
    expectedModuleOrder.push_back(childIndex);
    for (unsigned parentIndex : parentsByChild[childIndex])
      if (--dependencyCount[parentIndex] == 0)
        readyModules.emplace(moduleOrder[parentIndex].getSymName().str(),
                             parentIndex);
  }
  if (expectedModuleOrder.size() != moduleOrder.size())
    return model.emitOpError(
        "module instantiation cycle has no canonical declaration order");
  for (auto [actualIndex, expectedIndex] : llvm::enumerate(expectedModuleOrder))
    if (actualIndex != expectedIndex)
      return moduleOrder[actualIndex].emitOpError(
          "module declarations must use child-before-parent topological "
          "order with symbol-sorted ties");

  for (Operation &operation : model.getBody().front()) {
    auto module = dyn_cast<ModuleOp>(operation);
    if (!module)
      continue;
    unsigned prior = 0;
    bool moduleFirst = true;
    std::string priorPlacement;
    std::string priorProcess;
    for (Operation &child : module.getBody().front()) {
      if (modelVerificationWorkCollector)
        ++modelVerificationWorkCollector->orderingOperationVisits;
      unsigned rank = moduleRank(&child);
      if (!moduleFirst && rank < prior)
        return child.emitOpError(
            "module construction is not in deterministic canonical order");
      if (rank == 0) {
        StringAttr name = symbolName(&child);
        if (!moduleFirst && prior == rank && name &&
            name.getValue() <= priorPlacement)
          return child.emitOpError(
              "owned placements must be strictly symbol-sorted");
        priorPlacement = name ? name.getValue().str() : std::string();
      } else if (rank == 8) {
        StringAttr name = symbolName(&child);
        if (!moduleFirst && prior == rank && name &&
            name.getValue() <= priorProcess)
          return child.emitOpError(
              "process declarations must be strictly symbol-sorted");
        priorProcess = name ? name.getValue().str() : std::string();
      }
      prior = rank;
      moduleFirst = false;
    }
  }
  return success();
}

struct ExpandedRuntimeRow {
  Operation *placement = nullptr;
  Operation *realization = nullptr;
  unsigned context = 0;
  std::string target;
  std::string path;
  SmallVector<int64_t> indices;
  int64_t objectId = -1;
  int64_t activationId = -1;
};

struct ExpandedOwnerRow {
  Operation *placement = nullptr;
  unsigned context = 0;
  std::string path;
  SmallVector<int64_t> indices;
};

struct ExpansionContext {
  ModuleOp module;
  std::string path;
  llvm::DenseMap<Operation *, SmallVector<int64_t>> objectIds;
  llvm::DenseMap<Operation *, SmallVector<unsigned>> childContexts;

  ExpansionContext(ModuleOp module, std::string path)
      : module(module), path(std::move(path)) {}
};

struct HierarchyExpansion {
  SmallVector<ExpandedOwnerRow> ownerRows;
  SmallVector<ExpandedRuntimeRow> runtimeRows;
  SmallVector<ExpansionContext> contexts;
  llvm::StringMap<unsigned> ownerByPath;
};

std::string expandedPath(StringRef parent, StringRef name,
                         ArrayRef<int64_t> indices = {}) {
  std::string path = parent.str();
  if (!path.empty())
    path.push_back('.');
  path.append(name);
  llvm::raw_string_ostream os(path);
  for (int64_t index : indices)
    os << '[' << index << ']';
  return path;
}

std::string moduleSpecializationKey(ModuleOp module) {
  std::string key = module.getSymName().str();
  key.push_back(':');
  key.append(module.getSpecializationFingerprint());
  key.push_back(':');
  llvm::raw_string_ostream(key) << module.getStaticParams();
  return key;
}

LogicalResult expandSelectedRootOwners(ModelOp model, const ModelIndex &index,
                                       HierarchyExpansion &expansion) {
  FailureOr<Operation *> root = requireReference<ModuleOp>(
      index, model, model.getRootAttr(), "root", false);
  if (failed(root))
    return failure();

  enum class ActionKind { Enter, Exit, Row };
  struct Action {
    ActionKind kind;
    ModuleOp module;
    Operation *placement = nullptr;
    unsigned context = 0;
    std::string path;
    SmallVector<int64_t> indices;
    std::string specializationKey;
  };
  SmallVector<Action> stack;
  ModuleOp rootModule = cast<ModuleOp>(*root);
  std::string rootPath = rootModule.getSymName().str();
  if (!model.getConstructionOrder().empty()) {
    auto firstPath =
        dyn_cast<StringAttr>(model.getConstructionOrder().getValue().front());
    if (!firstPath)
      return model.emitOpError(
          "construction order paths must be canonical strings");
    rootPath = firstPath.getValue().split('.').first.str();
    if (rootPath.empty())
      return model.emitOpError("construction order has an empty root path");
  }
  stack.push_back({ActionKind::Enter,
                   rootModule,
                   nullptr,
                   0,
                   rootPath,
                   {},
                   moduleSpecializationKey(rootModule)});
  llvm::StringSet<> activeSpecializations;
  const uint64_t expansionLimit =
      currentModelVerificationLimits().maxExpandedObjects;

  while (!stack.empty()) {
    Action action = std::move(stack.back());
    stack.pop_back();
    if (action.kind == ActionKind::Exit) {
      activeSpecializations.erase(action.specializationKey);
      continue;
    }
    if (action.kind == ActionKind::Row) {
      if (expansion.ownerRows.size() >= expansionLimit)
        return action.placement->emitOpError()
               << "expanded hierarchy exceeds ACSim capability "
               << expansionLimit;
      if (expansion.ownerByPath.contains(action.path))
        return action.placement->emitOpError()
               << "derived hierarchy path is not unique: '" << action.path
               << "'";
      SymbolRefAttr target;
      if (auto instance = dyn_cast<InstanceOp>(action.placement)) {
        target = instance.getTargetAttr();
      } else if (auto array = dyn_cast<ArrayOp>(action.placement)) {
        target = array.getTargetAttr();
      }
      Operation *targetDefinition =
          target ? index.definitions.lookup(symbolKey(target)) : nullptr;
      if (auto binding = dyn_cast_or_null<BindingOp>(targetDefinition);
          binding && binding.getEffect() != "stateful")
        return action.placement->emitOpError(
            "ownership expansion requires a generated module or stateful "
            "binding target");
      unsigned ownerOrdinal = expansion.ownerRows.size();
      expansion.ownerByPath.try_emplace(action.path, ownerOrdinal);
      expansion.ownerRows.push_back(
          {action.placement, action.context, action.path, action.indices});
      if (modelVerificationWorkCollector)
        ++modelVerificationWorkCollector->expandedOwnerRows;

      if (auto childModule = dyn_cast_or_null<ModuleOp>(targetDefinition))
        stack.push_back({ActionKind::Enter, childModule, action.placement,
                         action.context, action.path, action.indices,
                         moduleSpecializationKey(childModule)});
      continue;
    }

    if (!activeSpecializations.insert(action.specializationKey).second)
      return action.module.emitOpError(
          "active module specialization cycle in selected hierarchy");
    unsigned context = expansion.contexts.size();
    expansion.contexts.emplace_back(action.module, action.path);
    if (action.placement)
      expansion.contexts[action.context]
          .childContexts[action.placement]
          .push_back(context);
    stack.push_back({ActionKind::Exit,
                     action.module,
                     nullptr,
                     context,
                     {},
                     {},
                     action.specializationKey});
    SmallVector<Operation *> placements;
    for (Operation &operation : action.module.getBody().front())
      if (isa<InstanceOp, ArrayOp, ProcessOp>(operation))
        placements.push_back(&operation);
    for (Operation *placement : llvm::reverse(placements)) {
      if (auto array = dyn_cast<ArrayOp>(placement)) {
        uint64_t volume = arrayVolume(array.getShape());
        for (uint64_t ordinal = volume; ordinal > 0; --ordinal) {
          SmallVector<int64_t> indices =
              lexicographicIndices(array.getShape(), ordinal - 1);
          stack.push_back(
              {ActionKind::Row,
               {},
               placement,
               context,
               expandedPath(action.path, array.getSymName(), indices),
               std::move(indices),
               {}});
        }
      } else {
        StringRef name = cast<StringAttr>(symbolName(placement)).getValue();
        stack.push_back({ActionKind::Row,
                         {},
                         placement,
                         context,
                         expandedPath(action.path, name),
                         {},
                         {}});
      }
    }
  }
  return success();
}

LogicalResult expandRuntime(const ModelIndex &index,
                            HierarchyExpansion &expansion) {
  const uint64_t expansionLimit =
      currentModelVerificationLimits().maxExpandedObjects;
  for (const ExpandedOwnerRow &owner : expansion.ownerRows) {
    Operation *realization = nullptr;
    if (isa<ProcessOp>(owner.placement)) {
      realization = owner.placement;
    } else {
      SymbolRefAttr target =
          isa<InstanceOp>(owner.placement)
              ? cast<InstanceOp>(owner.placement).getTargetAttr()
              : cast<ArrayOp>(owner.placement).getTargetAttr();
      Operation *definition = index.definitions.lookup(symbolKey(target));
      if (isa_and_nonnull<BindingOp>(definition))
        realization = definition;
    }
    if (!realization)
      continue;
    if (expansion.runtimeRows.size() >= expansionLimit)
      return owner.placement->emitOpError()
             << "runtime expansion exceeds ACSim capability " << expansionLimit;
    int64_t id = static_cast<int64_t>(expansion.runtimeRows.size());
    expansion.runtimeRows.push_back(
        {owner.placement, realization, owner.context,
         definitionKey(owner.placement), owner.path, owner.indices, id, id});
    expansion.contexts[owner.context].objectIds[owner.placement].push_back(id);
    if (modelVerificationWorkCollector)
      ++modelVerificationWorkCollector->expandedRuntimeRows;
  }
  return success();
}

LogicalResult verifyConstructionOrder(ModelOp model,
                                      const HierarchyExpansion &expansion) {
  auto readOrder = [&](ArrayAttr order, StringRef label,
                       SmallVectorImpl<std::string> &paths) -> LogicalResult {
    llvm::StringSet<> unique;
    for (Attribute attribute : order) {
      auto path = dyn_cast<StringAttr>(attribute);
      if (!path || path.getValue().empty())
        return model.emitOpError()
               << label << " entries must be concrete hierarchy-path strings";
      if (!unique.insert(path.getValue()).second)
        return model.emitOpError()
               << label << " contains duplicate '" << path << "'";
      paths.push_back(path.getValue().str());
    }
    return success();
  };

  SmallVector<std::string> actual;
  actual.reserve(expansion.ownerRows.size());
  for (const ExpandedOwnerRow &row : expansion.ownerRows)
    actual.push_back(row.path);
  SmallVector<std::string> construction;
  SmallVector<std::string> destruction;
  if (failed(readOrder(model.getConstructionOrder(), "construction order",
                       construction)) ||
      failed(readOrder(model.getDestructionOrder(), "destruction order",
                       destruction)))
    return failure();
  if (construction != actual)
    return model.emitOpError(
        "construction order must equal canonical ownership preorder");
  SmallVector<std::string> reversed(actual.rbegin(), actual.rend());
  if (destruction != reversed)
    return model.emitOpError(
        "destruction order must be the exact reverse of construction order");
  return success();
}

LogicalResult verifyProcess(ProcessOp process, const ModelIndex &index) {
  if (process.getFairnessCap() <= 0 ||
      static_cast<uint64_t>(process.getFairnessCap()) > kMaxModelNodes)
    return process.emitOpError(
        "fairness cap must be a positive bounded static integer");
  SmallVector<std::string> pcs;
  llvm::StringSet<> pcSet;
  for (Attribute attribute : process.getPcs()) {
    auto reference = dyn_cast<FlatSymbolRefAttr>(attribute);
    if (!reference || !pcSet.insert(reference.getValue()).second)
      return process.emitOpError(
          "pcs must be a non-empty ordered list of unique flat symbols");
    pcs.push_back(reference.getValue().str());
  }
  if (pcs.empty() || !pcSet.contains(process.getEntryPc()))
    return process.emitOpError(
        "entry PC must occur exactly once in the closed PC list");
  if (process.getStates().size() != pcs.size())
    return process.emitOpError(
        "process requires exactly one ordered state region per PC");
  if (pcs.front() != process.getEntryPc())
    return process.emitOpError(
        "the first ordered state region must be the entry PC");

  if (process.getCaptureNames().size() != process.getCaptures().size())
    return process.emitOpError(
        "process captures must have one exact ordered name per operand");
  llvm::StringSet<> captureNames;
  for (Attribute attribute : process.getCaptureNames()) {
    auto name = dyn_cast<StringAttr>(attribute);
    if (!name || name.getValue().empty() ||
        !captureNames.insert(name.getValue()).second)
      return process.emitOpError(
          "process capture names must be unique non-empty strings");
  }

  llvm::StringMap<Type> slots;
  for (Attribute attribute : process.getLiveSlots()) {
    auto dictionary = dyn_cast<DictionaryAttr>(attribute);
    auto name =
        dictionary ? dictionary.getAs<StringAttr>("name") : StringAttr();
    auto type = dictionary ? dictionary.getAs<TypeAttr>("type") : TypeAttr();
    if (!dictionary || dictionary.size() != 2 || !name || !type ||
        !isa<ValueType>(type.getValue()) ||
        !slots.try_emplace(name.getValue(), type.getValue()).second)
      return process.emitOpError(
          "live slots require unique exact {name, type} value records");
    if (failed(verifyCanonicalType(type.getValue(), process, index)))
      return failure();
  }

  for (Region &state : process.getStates()) {
    if (state.empty())
      return process.emitOpError("every PC requires a non-empty state region");
    Block &entry = state.front();
    if (entry.getNumArguments() != process.getCaptures().size())
      return process.emitOpError(
          "process state arguments must exactly match declared typed captures");
    for (auto [argument, capture] :
         llvm::zip_equal(entry.getArguments(), process.getCaptures()))
      if (argument.getType() != capture.getType())
        return process.emitOpError("process state arguments must exactly match "
                                   "declared typed captures");
    llvm::DenseMap<Block *, unsigned> blockOrdinals;
    unsigned ordinal = 0;
    for (Block &block : state) {
      blockOrdinals[&block] = ordinal++;
      if (block.empty())
        return process.emitOpError("every process block requires operations");
      Operation *terminator = block.getTerminator();
      if (!terminator ||
          (!isa<ContinueOp, SuspendOp, TerminateOp>(terminator) &&
           terminator->getName().getDialectNamespace() != "cf"))
        return block.front().emitOpError(
            "every process path must continue, suspend, terminate, or use cf");
    }
    for (Block &block : state) {
      Operation *terminator = block.getTerminator();
      for (Block *successor : terminator->getSuccessors()) {
        if (successor->getParent() != &state)
          return terminator->emitOpError("ordinary cf edges cannot cross "
                                         "process PC suspension boundaries");
        if (blockOrdinals.lookup(successor) <= blockOrdinals.lookup(&block))
          return terminator->emitOpError(
              "intra-PC control flow must prove bounded acyclic progress");
      }
    }
  }

  auto verifyTarget = [&](Operation *operation,
                          FlatSymbolRefAttr target) -> LogicalResult {
    if (pcSet.contains(target.getValue()))
      return success();
    return operation->emitOpError()
           << "target PC '" << target << "' is not in the closed PC list";
  };
  for (Region &state : process.getStates()) {
    for (Block &block : state) {
      for (Operation &operation : block) {
        if (auto load = dyn_cast<LiveLoadOp>(operation)) {
          if (resolveReference(index, load, load.getProcessAttr()) != process ||
              !slots.contains(load.getSlot()) ||
              slots.lookup(load.getSlot()) != load.getResult().getType())
            return load.emitOpError("live load must resolve to an exact typed "
                                    "slot of this process");
        } else if (auto store = dyn_cast<LiveStoreOp>(operation)) {
          if (resolveReference(index, store, store.getProcessAttr()) !=
                  process ||
              !slots.contains(store.getSlot()) ||
              slots.lookup(store.getSlot()) != store.getValue().getType())
            return store.emitOpError("live store must resolve to an exact "
                                     "typed slot of this process");
        } else if (auto next = dyn_cast<ContinueOp>(operation)) {
          if (failed(verifyTarget(next, next.getTargetPcAttr())))
            return failure();
        } else if (auto suspend = dyn_cast<SuspendOp>(operation)) {
          if (!isa<WakeType>(suspend.getWake().getType()) ||
              failed(verifyTarget(suspend, suspend.getTargetPcAttr())))
            return suspend.emitOpError(
                "suspend requires one exact typed wake and a closed next PC");
        } else if (auto terminate = dyn_cast<TerminateOp>(operation)) {
          if (terminate.getStatus() != "success" &&
              terminate.getStatus() != "failure")
            return terminate.emitOpError(
                "terminal status must be exactly 'success' or 'failure'");
        }
      }
    }
  }

  llvm::StringMap<unsigned> pcOrdinals;
  for (auto [ordinal, pc] : llvm::enumerate(pcs))
    pcOrdinals[pc] = ordinal;
  const uint64_t dependencyLimit =
      currentModelVerificationLimits().maxDependencyNodes;
  if (pcs.size() > dependencyLimit)
    return process.emitOpError()
           << "dependency graph exceeds ACSim capability " << dependencyLimit;
  SmallVector<SmallVector<unsigned>> successors(pcs.size());
  SmallVector<unsigned> indegree(pcs.size(), 0);
  uint64_t dependencyNodes = pcs.size();
  for (auto [ordinal, state] : llvm::enumerate(process.getStates())) {
    llvm::SmallSet<unsigned, 4> unique;
    for (Block &block : state) {
      auto next = dyn_cast<ContinueOp>(block.getTerminator());
      if (!next)
        continue;
      auto found = pcOrdinals.find(next.getTargetPc());
      if (found == pcOrdinals.end())
        return next.emitOpError("target PC is not in the closed PC list");
      if (unique.insert(found->second).second) {
        if (++dependencyNodes > dependencyLimit)
          return process.emitOpError()
                 << "dependency graph exceeds ACSim capability "
                 << dependencyLimit;
        successors[ordinal].push_back(found->second);
        ++indegree[found->second];
      }
    }
  }
  std::set<unsigned> ready;
  for (auto [ordinal, degree] : llvm::enumerate(indegree))
    if (degree == 0)
      ready.insert(ordinal);
  SmallVector<unsigned> topological;
  while (!ready.empty()) {
    unsigned ordinal = *ready.begin();
    ready.erase(ready.begin());
    topological.push_back(ordinal);
    for (unsigned successor : successors[ordinal])
      if (--indegree[successor] == 0)
        ready.insert(successor);
  }
  if (topological.size() != pcs.size())
    return process.emitOpError(
        "process continue graph must prove bounded acyclic progress");

  SmallVector<uint64_t> memo(pcs.size(), 0);
  uint64_t maximumPath = 0;
  for (unsigned stateOrdinal : llvm::reverse(topological)) {
    Region &state = process.getStates()[stateOrdinal];
    llvm::DenseMap<Block *, uint64_t> blockCost;
    uint64_t stateMaximum = 0;
    for (Block &block : llvm::reverse(state)) {
      uint64_t suffix = 0;
      Operation *terminator = block.getTerminator();
      if (auto next = dyn_cast<ContinueOp>(terminator))
        suffix = memo[pcOrdinals.lookup(next.getTargetPc())];
      else
        for (Block *successor : terminator->getSuccessors())
          suffix = std::max(suffix, blockCost.lookup(successor));
      uint64_t own = block.getOperations().size();
      if (suffix > kMaxModelNodes - own)
        return process.emitOpError("process fairness work count overflows");
      blockCost[&block] = own + suffix;
      stateMaximum = std::max(stateMaximum, own + suffix);
    }
    memo[stateOrdinal] = stateMaximum;
    maximumPath = std::max(maximumPath, stateMaximum);
  }
  if (maximumPath > static_cast<uint64_t>(process.getFairnessCap()))
    return process.emitOpError()
           << "fairness cap " << process.getFairnessCap()
           << " is below maximum local execution path " << maximumPath;
  return success();
}

Operation *realizationForBase(Value value, const ModelIndex &index) {
  SymbolRefAttr symbol;
  if (auto owner = dyn_cast<OwnerType>(value.getType()))
    symbol = owner.getRealization();
  else if (auto reference = dyn_cast<RefType>(value.getType()))
    symbol = reference.getRealization();
  if (!symbol)
    return nullptr;
  Operation *definition = index.definitions.lookup(symbolKey(symbol));
  return isa_and_nonnull<BindingOp, ModuleOp>(definition) ? definition
                                                          : nullptr;
}

ArrayAttr realizationRecords(Operation *realization, StringRef field) {
  if (auto binding = dyn_cast_or_null<BindingOp>(realization))
    return binding.getRecord().getAs<ArrayAttr>(field);
  if (auto module = dyn_cast_or_null<ModuleOp>(realization))
    return module.getInterface().getAs<ArrayAttr>(field);
  return {};
}

DictionaryAttr findEndpoint(Operation *realization, StringRef field,
                            FlatSymbolRefAttr accessor) {
  auto records = realizationRecords(realization, field);
  if (!records)
    return {};
  for (Attribute attribute : records) {
    auto record = dyn_cast<DictionaryAttr>(attribute);
    if (record && record.getAs<FlatSymbolRefAttr>("accessor") == accessor)
      return record;
  }
  return {};
}

DictionaryAttr findProjectedEndpoint(Value value, const ModelIndex &index) {
  if (auto port = value.getDefiningOp<PortOp>())
    return findEndpoint(realizationForBase(port.getBase(), index), "ports",
                        port.getAccessorAttr());
  if (auto resource = value.getDefiningOp<ResourceOp>())
    return findEndpoint(realizationForBase(resource.getBase(), index),
                        "resources", resource.getAccessorAttr());
  return {};
}

ExportOp findModuleEndpointExport(ModuleOp module, StringRef field,
                                  FlatSymbolRefAttr accessor) {
  SmallVector<ExportOp> exports;
  for (Operation &operation : module.getBody().front())
    if (auto exportOp = dyn_cast<ExportOp>(operation))
      exports.push_back(exportOp);

  unsigned ordinal = 0;
  for (StringRef candidateField :
       {StringRef("ports"), StringRef("resources"), StringRef("results")}) {
    for (Attribute attribute :
         module.getInterface().getAs<ArrayAttr>(candidateField)) {
      auto record = cast<DictionaryAttr>(attribute);
      if (candidateField == field &&
          record.getAs<FlatSymbolRefAttr>("accessor") == accessor)
        return ordinal < exports.size() ? exports[ordinal] : ExportOp();
      ++ordinal;
    }
  }
  return {};
}

LogicalResult verifyBindingReferenceFingerprint(BindingOp binding,
                                                const ModelIndex &index,
                                                FlatSymbolRefAttr reference,
                                                StringRef fingerprintField,
                                                StringRef label) {
  Operation *definition = resolveReference(index, binding, reference);
  auto type = dyn_cast_or_null<TypeOp>(definition);
  auto fingerprint = binding.getRecord().getAs<StringAttr>(fingerprintField);
  if (!type || !fingerprint || fingerprint != type.getFingerprintAttr())
    return binding.emitOpError()
           << label << " fingerprint must exactly match referenced acsim.type";
  return success();
}

ArrayAttr bindingStaticValues(BindingOp binding) {
  SmallVector<Attribute> values;
  auto parameters = binding.getRecord().getAs<ArrayAttr>("parameters");
  if (!parameters)
    return {};
  for (Attribute attribute : parameters) {
    auto record = dyn_cast<DictionaryAttr>(attribute);
    if (!record || !record.get("value"))
      return {};
    values.push_back(record.get("value"));
  }
  return ArrayAttr::get(binding.getContext(), values);
}

LogicalResult verifyPlacementTarget(Operation *operation,
                                    SymbolRefAttr targetReference,
                                    ArrayAttr staticArguments,
                                    StringAttr specialization,
                                    const ModelIndex &index) {
  FailureOr<Operation *> targetDefinition =
      requireReference<BindingOp, ModuleOp>(index, operation, targetReference,
                                            "realization target");
  if (failed(targetDefinition))
    return failure();
  Operation *target = *targetDefinition;
  if (auto targetBinding = dyn_cast<BindingOp>(target)) {
    if (targetBinding.getEffect() != "stateful")
      return operation->emitOpError(
          "placement target binding must be stateful");
    if (bindingStaticValues(targetBinding) != staticArguments)
      return operation->emitOpError("static arguments must exactly match "
                                    "ordered binding-lock parameters");
  } else {
    auto targetModule = cast<ModuleOp>(target);
    if (targetModule.getStaticParams() != staticArguments ||
        targetModule.getSpecializationFingerprintAttr() != specialization)
      return operation->emitOpError(
          "generated module target requires exact static arguments and "
          "specialization fingerprint");
  }
  return success();
}

FailureOr<Operation *> capturedPlacement(Value value, Operation *reporter) {
  llvm::SmallPtrSet<void *, 16> visited;
  uint64_t nodes = 0;
  const uint64_t limit = currentModelVerificationLimits().maxDependencyNodes;
  while (value) {
    if (++nodes > limit)
      return reporter->emitOpError()
             << "dependency graph exceeds ACSim capability " << limit;
    void *key = value.getAsOpaquePointer();
    if (!visited.insert(key).second)
      return reporter->emitOpError(
          "typed SSA dependency graph contains a cycle");
    Operation *definition = value.getDefiningOp();
    if (!definition)
      return static_cast<Operation *>(nullptr);
    if (isa<InstanceOp, ArrayOp>(definition))
      return definition;
    if (auto element = dyn_cast<ElementOp>(definition)) {
      value = element.getArray();
      continue;
    }
    if (auto port = dyn_cast<PortOp>(definition)) {
      value = port.getBase();
      continue;
    }
    if (auto resource = dyn_cast<ResourceOp>(definition)) {
      value = resource.getBase();
      continue;
    }
    return static_cast<Operation *>(nullptr);
  }
  return static_cast<Operation *>(nullptr);
}

LogicalResult verifyCaptureBoundary(ProcessOp process, Value capture,
                                    const ModelIndex &index) {
  if (!isa<OwnerType, RefType>(capture.getType()))
    return success();
  FailureOr<Operation *> placement = capturedPlacement(capture, process);
  if (failed(placement))
    return failure();
  if (*placement && (*placement)->getParentOfType<ModuleOp>() ==
                        process->getParentOfType<ModuleOp>())
    return success();
  return process.emitOpError("captured owner/ref must remain within the "
                             "lexically enclosing module boundary");
}

LogicalResult verifyModulesAndTypedGraph(ModelOp model,
                                         const ModelIndex &index) {
  llvm::DenseMap<Value, SmallVector<int64_t>> lastProjection;
  llvm::SmallSet<std::pair<void *, void *>, 16> bindingPairs;
  llvm::StringMap<StringAttr> specializationByKey;
  llvm::StringMap<std::string> keyBySpecialization;
  llvm::StringMap<StringRef> generatedCallEffects;
  auto recordSpecialization = [&](Operation *placement, SymbolRefAttr target,
                                  ArrayAttr arguments,
                                  StringAttr fingerprint) -> LogicalResult {
    std::string key = symbolKey(target);
    std::string printedArguments;
    llvm::raw_string_ostream(printedArguments) << arguments;
    key.push_back(':');
    key.append(printedArguments);
    auto [sameKey, inserted] =
        specializationByKey.try_emplace(key, fingerprint);
    if (!inserted && sameKey->second != fingerprint)
      return placement->emitOpError("identical target and static arguments "
                                    "require one specialization fingerprint");
    auto [sameFingerprint, unique] =
        keyBySpecialization.try_emplace(fingerprint.getValue(), key);
    if (!unique && sameFingerprint->second != key)
      return placement->emitOpError(
          "different specialization inputs require distinct fingerprints");
    return success();
  };

  for (Operation *operation : index.ordered) {
    if (modelVerificationWorkCollector)
      ++modelVerificationWorkCollector->semanticOperationVisits;
    for (Type type : operation->getOperandTypes())
      if (failed(verifyCanonicalType(type, operation, index)))
        return failure();
    for (Type type : operation->getResultTypes())
      if (failed(verifyCanonicalType(type, operation, index)))
        return failure();
    for (Region &region : operation->getRegions())
      for (Block &block : region)
        for (BlockArgument argument : block.getArguments())
          if (failed(verifyCanonicalType(argument.getType(), operation, index)))
            return failure();

    if (auto binding = dyn_cast<BindingOp>(operation)) {
      // The field projections below assume the exact lock shape; malformed
      // records must be rejected before any of them are dereferenced.
      if (failed(verifyBindingLockShape(binding)))
        return failure();
      const std::array<StringRef, 2> cppKinds = {"value", "packet"};
      const std::array<StringRef, 1> schemaKinds = {"schema"};
      const std::array<StringRef, 1> providerKinds = {"provider"};
      const std::array<StringRef, 1> implementationKinds = {"implementation"};
      if (failed(requireTypeKind(index, binding, binding.getCppTypeAttr(),
                                 cppKinds, "C++ type")) ||
          failed(requireTypeKind(index, binding, binding.getSchemaAttr(),
                                 schemaKinds, "schema")) ||
          failed(requireTypeKind(index, binding, binding.getProviderAttr(),
                                 providerKinds, "provider")) ||
          failed(requireTypeKind(index, binding,
                                 binding.getImplementationAttr(),
                                 implementationKinds, "implementation")))
        return failure();
      if (failed(verifyBindingReferenceFingerprint(
              binding, index, binding.getSchemaAttr(),
              "component_schema_fingerprint", "component schema")) ||
          failed(verifyBindingReferenceFingerprint(
              binding, index, binding.getImplementationAttr(),
              "provider_implementation_fingerprint", "implementation")))
        return failure();
      for (StringRef field : {StringRef("ports"), StringRef("resources")})
        for (Attribute item : binding.getRecord().getAs<ArrayAttr>(field)) {
          auto endpoint = cast<DictionaryAttr>(item);
          const std::array<StringRef, 1> accessorKinds = {"accessor"};
          const std::array<StringRef, 1> roleKinds = {"role"};
          const std::array<StringRef, 1> timeDomainKinds = {"time_domain"};
          if (failed(requireTypeKind(
                  index, binding, endpoint.getAs<FlatSymbolRefAttr>("accessor"),
                  accessorKinds, "endpoint accessor")) ||
              failed(requireTypeKind(index, binding,
                                     endpoint.getAs<FlatSymbolRefAttr>("role"),
                                     roleKinds, "endpoint role")) ||
              failed(requireTypeKind(
                  index, binding,
                  endpoint.getAs<FlatSymbolRefAttr>("time_domain"),
                  timeDomainKinds, "time-domain")))
            return failure();
        }
      for (Attribute item : binding.getRecord().getAs<ArrayAttr>("ports")) {
        auto endpoint = cast<DictionaryAttr>(item);
        const std::array<StringRef, 1> interfaceKinds = {"interface"};
        const std::array<StringRef, 2> payloadKinds = {"value", "packet"};
        const std::array<StringRef, 1> protocolKinds = {"protocol"};
        if (failed(requireTypeKind(
                index, binding, endpoint.getAs<FlatSymbolRefAttr>("interface"),
                interfaceKinds, "endpoint interface")) ||
            failed(requireTypeKind(index, binding,
                                   endpoint.getAs<FlatSymbolRefAttr>("payload"),
                                   payloadKinds, "endpoint payload")) ||
            failed(requireTypeKind(
                index, binding, endpoint.getAs<FlatSymbolRefAttr>("protocol"),
                protocolKinds, "endpoint protocol")))
          return failure();
      }
      for (Attribute item : binding.getRecord().getAs<ArrayAttr>("resources")) {
        auto endpoint = cast<DictionaryAttr>(item);
        const std::array<StringRef, 1> resourceKinds = {"resource"};
        if (failed(requireTypeKind(
                index, binding, endpoint.getAs<FlatSymbolRefAttr>("resource"),
                resourceKinds, "endpoint resource")))
          return failure();
      }
      for (Attribute item : binding.getRecord().getAs<ArrayAttr>("results")) {
        auto result = cast<DictionaryAttr>(item);
        const std::array<StringRef, 2> resultKinds = {"value", "packet"};
        if (failed(requireTypeKind(index, binding,
                                   result.getAs<FlatSymbolRefAttr>("cpp_type"),
                                   resultKinds, "result C++ type")))
          return failure();
      }
      for (Attribute item :
           binding.getRecord().getAs<ArrayAttr>("activation_sources")) {
        auto source = cast<DictionaryAttr>(item);
        const std::array<StringRef, 1> wakeKinds = {"wake"};
        if (failed(requireTypeKind(index, binding,
                                   source.getAs<FlatSymbolRefAttr>("kind"),
                                   wakeKinds, "activation source kind")))
          return failure();
      }
    } else if (auto module = dyn_cast<ModuleOp>(operation)) {
      // As with binding locks, the interface projections below assume the
      // exact module interface shape.
      if (failed(verifyModuleInterfaceShape(module)))
        return failure();
      for (StringRef field : {StringRef("ports"), StringRef("resources")})
        for (Attribute item : module.getInterface().getAs<ArrayAttr>(field)) {
          auto endpoint = cast<DictionaryAttr>(item);
          const std::array<StringRef, 1> accessorKinds = {"accessor"};
          const std::array<StringRef, 1> roleKinds = {"role"};
          const std::array<StringRef, 1> timeDomainKinds = {"time_domain"};
          if (failed(requireTypeKind(
                  index, module, endpoint.getAs<FlatSymbolRefAttr>("accessor"),
                  accessorKinds, "interface accessor")) ||
              failed(requireTypeKind(index, module,
                                     endpoint.getAs<FlatSymbolRefAttr>("role"),
                                     roleKinds, "interface role")) ||
              failed(requireTypeKind(
                  index, module,
                  endpoint.getAs<FlatSymbolRefAttr>("time_domain"),
                  timeDomainKinds, "interface time-domain")))
            return failure();
        }
      for (Attribute item : module.getInterface().getAs<ArrayAttr>("ports")) {
        auto endpoint = cast<DictionaryAttr>(item);
        const std::array<StringRef, 1> interfaceKinds = {"interface"};
        const std::array<StringRef, 2> payloadKinds = {"value", "packet"};
        const std::array<StringRef, 1> protocolKinds = {"protocol"};
        if (failed(requireTypeKind(
                index, module, endpoint.getAs<FlatSymbolRefAttr>("interface"),
                interfaceKinds, "interface kind")) ||
            failed(requireTypeKind(index, module,
                                   endpoint.getAs<FlatSymbolRefAttr>("payload"),
                                   payloadKinds, "interface payload")) ||
            failed(requireTypeKind(
                index, module, endpoint.getAs<FlatSymbolRefAttr>("protocol"),
                protocolKinds, "interface protocol")))
          return failure();
      }
      for (Attribute item :
           module.getInterface().getAs<ArrayAttr>("resources")) {
        const std::array<StringRef, 1> resourceKinds = {"resource"};
        if (failed(requireTypeKind(
                index, module,
                cast<DictionaryAttr>(item).getAs<FlatSymbolRefAttr>("resource"),
                resourceKinds, "interface resource")))
          return failure();
      }
      for (Attribute item : module.getInterface().getAs<ArrayAttr>("results")) {
        const std::array<StringRef, 2> resultKinds = {"value", "packet"};
        if (failed(requireTypeKind(
                index, module,
                cast<DictionaryAttr>(item).getAs<FlatSymbolRefAttr>("cpp_type"),
                resultKinds, "interface result C++ type")))
          return failure();
      }
    } else if (auto instance = dyn_cast<InstanceOp>(operation)) {
      auto ownerType = dyn_cast<OwnerType>(instance.getResult().getType());
      if (failed(verifyPlacementTarget(
              instance, instance.getTargetAttr(), instance.getStaticArgs(),
              instance.getSpecializationFingerprintAttr(), index)) ||
          failed(recordSpecialization(
              instance, instance.getTargetAttr(), instance.getStaticArgs(),
              instance.getSpecializationFingerprintAttr())))
        return failure();
      if (!ownerType || ownerType.getRealization() != instance.getTargetAttr())
        return instance.emitOpError(
            "instance result must own its exact realization target");
    } else if (auto array = dyn_cast<ArrayOp>(operation)) {
      if (failed(verifyPlacementTarget(
              array, array.getTargetAttr(), array.getStaticArgs(),
              array.getSpecializationFingerprintAttr(), index)) ||
          failed(recordSpecialization(
              array, array.getTargetAttr(), array.getStaticArgs(),
              array.getSpecializationFingerprintAttr())))
        return failure();
      auto type = dyn_cast<ArrayType>(array.getResult().getType());
      auto element =
          type ? dyn_cast<OwnerType>(type.getElementType()) : OwnerType();
      if (!type || type.getShape().asArrayRef() != array.getShape() ||
          !element || element.getRealization() != array.getTargetAttr())
        return array.emitOpError(
            "array result shape and owning element realization must be exact");
    } else if (auto element = dyn_cast<ElementOp>(operation)) {
      auto arrayType = dyn_cast<ArrayType>(element.getArray().getType());
      auto owner = arrayType ? dyn_cast<OwnerType>(arrayType.getElementType())
                             : OwnerType();
      auto reference = dyn_cast<RefType>(element.getResult().getType());
      if (!arrayType || !owner || !reference ||
          owner.getRealization() != reference.getRealization() ||
          element.getIndices().size() != arrayType.getShape().size())
        return element.emitOpError(
            "element must be a constant typed reference projection");
      for (auto [indexValue, extent] : llvm::zip_equal(
               element.getIndices(), arrayType.getShape().asArrayRef()))
        if (indexValue < 0 || indexValue >= extent)
          return element.emitOpError("element index is out of static bounds");
      SmallVector<int64_t> current(element.getIndices());
      auto found = lastProjection.find(element.getArray());
      if (found != lastProjection.end() &&
          !lexicographicallyLess(found->second, current))
        return element.emitOpError(
            "array element projections must be strictly lexicographic");
      lastProjection[element.getArray()] = std::move(current);
    } else if (auto port = dyn_cast<PortOp>(operation)) {
      if (!isa<OwnerType, RefType>(port.getBase().getType()) ||
          !isa<PortType>(port.getResult().getType()))
        return port.emitOpError(
            "port projection requires a typed owner/ref and typed port result");
      const std::array<StringRef, 1> kinds = {"accessor"};
      if (failed(requireTypeKind(index, port, port.getAccessorAttr(), kinds,
                                 "port accessor")))
        return failure();
      Operation *realization = realizationForBase(port.getBase(), index);
      DictionaryAttr endpoint =
          findEndpoint(realization, "ports", port.getAccessorAttr());
      PortType type = cast<PortType>(port.getResult().getType());
      if (!endpoint ||
          endpoint.getAs<FlatSymbolRefAttr>("interface") !=
              type.getInterface() ||
          endpoint.getAs<FlatSymbolRefAttr>("role") != type.getRole() ||
          endpoint.getAs<FlatSymbolRefAttr>("payload") != type.getPayload() ||
          endpoint.getAs<FlatSymbolRefAttr>("protocol") != type.getProtocol())
        return port.emitOpError("port projection must exactly match its "
                                "binding-lock endpoint record");
    } else if (auto resource = dyn_cast<ResourceOp>(operation)) {
      if (!isa<OwnerType, RefType>(resource.getBase().getType()) ||
          !isa<ResourceType>(resource.getResult().getType()))
        return resource.emitOpError("resource projection requires a typed "
                                    "owner/ref and resource result");
      const std::array<StringRef, 1> kinds = {"accessor"};
      if (failed(requireTypeKind(index, resource, resource.getAccessorAttr(),
                                 kinds, "resource accessor")))
        return failure();
      Operation *realization = realizationForBase(resource.getBase(), index);
      DictionaryAttr endpoint =
          findEndpoint(realization, "resources", resource.getAccessorAttr());
      ResourceType type = cast<ResourceType>(resource.getResult().getType());
      if (!endpoint ||
          endpoint.getAs<FlatSymbolRefAttr>("resource") != type.getResource() ||
          endpoint.getAs<FlatSymbolRefAttr>("role") != type.getRole())
        return resource.emitOpError("resource projection must exactly match "
                                    "its binding-lock endpoint record");
    } else if (auto bind = dyn_cast<BindOp>(operation)) {
      if (!llvm::is_contained({StringRef("port"), StringRef("resource"),
                               StringRef("export"), StringRef("pure_view")},
                              bind.getKind()))
        return bind.emitOpError("unknown closed typed binding kind");
      if (bind.getKind() == "port") {
        auto source = bind.getSource().getDefiningOp<PortOp>();
        auto target = bind.getTarget().getDefiningOp<PortOp>();
        DictionaryAttr sourceRecord =
            source ? findEndpoint(realizationForBase(source.getBase(), index),
                                  "ports", source.getAccessorAttr())
                   : DictionaryAttr();
        DictionaryAttr targetRecord =
            target ? findEndpoint(realizationForBase(target.getBase(), index),
                                  "ports", target.getAccessorAttr())
                   : DictionaryAttr();
        if (!sourceRecord || !targetRecord ||
            sourceRecord.getAs<StringAttr>("direction").getValue() !=
                "output" ||
            targetRecord.getAs<StringAttr>("direction").getValue() != "input")
          return bind.emitOpError("port binding must connect exact output and "
                                  "input endpoint records");
        if (sourceRecord.get("interface") != targetRecord.get("interface") ||
            sourceRecord.get("payload") != targetRecord.get("payload") ||
            sourceRecord.get("protocol") != targetRecord.get("protocol") ||
            sourceRecord.get("cardinality") !=
                targetRecord.get("cardinality") ||
            sourceRecord.get("delegation") != targetRecord.get("delegation") ||
            sourceRecord.get("ownership") != targetRecord.get("ownership") ||
            sourceRecord.get("time_domain") != targetRecord.get("time_domain"))
          return bind.emitOpError(
              "port bind endpoints must have identical interface, payload, "
              "protocol, cardinality, delegation, ownership, and time domain");
      } else if (bind.getKind() == "resource") {
        auto source = bind.getSource().getDefiningOp<ResourceOp>();
        auto target = bind.getTarget().getDefiningOp<ResourceOp>();
        DictionaryAttr sourceRecord =
            source ? findEndpoint(realizationForBase(source.getBase(), index),
                                  "resources", source.getAccessorAttr())
                   : DictionaryAttr();
        DictionaryAttr targetRecord =
            target ? findEndpoint(realizationForBase(target.getBase(), index),
                                  "resources", target.getAccessorAttr())
                   : DictionaryAttr();
        if (!sourceRecord || !targetRecord ||
            sourceRecord.getAs<StringAttr>("mode").getValue() != "initiator" ||
            targetRecord.getAs<StringAttr>("mode").getValue() != "target")
          return bind.emitOpError("resource binding must connect exact "
                                  "initiator and target endpoint records");
        if (sourceRecord.get("resource") != targetRecord.get("resource") ||
            sourceRecord.get("delegation") != targetRecord.get("delegation") ||
            sourceRecord.get("ownership") != targetRecord.get("ownership") ||
            sourceRecord.get("time_domain") != targetRecord.get("time_domain"))
          return bind.emitOpError(
              "resource bind endpoints must have identical resource kind, "
              "delegation, ownership, and time domain");
      } else if (bind.getKind() == "pure_view") {
        auto target = bind.getTarget().getDefiningOp<InlineOp>();
        if (!target || !llvm::is_contained(target.getArgs(), bind.getSource()))
          return bind.emitOpError(
              "pure_view target must directly consume the source expression");
      } else {
        auto target = bind.getTarget().getDefiningOp<ExportOp>();
        if (!target || bind.getTarget() != target.getResult())
          return bind.emitOpError(
              "export bind target must be the exact result of acsim.export");
        if (bind.getSource() != target.getValue())
          return bind.emitOpError(
              "export bind source must be the exact input of its target "
              "acsim.export");
        if (bind.getSource().getType() != bind.getTarget().getType())
          return bind.emitOpError(
              "export binding endpoints must have exactly equal types");
      }
      std::pair<void *, void *> key{bind.getSource().getAsOpaquePointer(),
                                    bind.getTarget().getAsOpaquePointer()};
      if (!bindingPairs.insert(key).second)
        return bind.emitOpError(
            "each typed construction relation must lower exactly once");
    } else if (auto inlineOp = dyn_cast<InlineOp>(operation)) {
      Type result = inlineOp.getResult().getType();
      if (inlineOp->getParentOfType<ProcessOp>()) {
        if (!isa<IntegerType, FloatType, IndexType, ValueType>(result))
          return inlineOp.emitOpError(
              "process inline result must be an integer, float, index, or "
              "!acsim.value");
      } else if (!isa<ExprType>(result)) {
        return inlineOp.emitOpError(
            "module inline result must be exactly !acsim.expr");
      }
      FailureOr<Operation *> definition =
          requireCallCallee(index, inlineOp, inlineOp.getCalleeAttr());
      if (failed(definition))
        return failure();
      if (auto binding = dyn_cast<BindingOp>(*definition)) {
        if (binding.getEffect() != "pure")
          return inlineOp.emitOpError()
                 << "inline callee '" << inlineOp.getCalleeAttr()
                 << "' requires effect 'pure'";
      } else {
        auto type = cast<TypeOp>(*definition);
        if (type.getKind() != "implementation")
          return inlineOp.emitOpError()
                 << "callee reference '" << inlineOp.getCalleeAttr()
                 << "' resolves to non-implementation acsim.type";
        std::string key = definitionKey(type);
        auto [entry, inserted] =
            generatedCallEffects.try_emplace(key, "inline");
        if (!inserted && entry->second != "inline")
          return inlineOp.emitOpError()
                 << "generated implementation callee '"
                 << inlineOp.getCalleeAttr()
                 << "' cannot be used by both acsim.inline and acsim.invoke";
      }
      if (!isMemoryEffectFree(inlineOp))
        return inlineOp.emitOpError("inline must remain effect-free");
    } else if (auto invoke = dyn_cast<InvokeOp>(operation)) {
      FailureOr<Operation *> definition =
          requireCallCallee(index, invoke, invoke.getCalleeAttr());
      if (failed(definition))
        return failure();
      if (auto binding = dyn_cast<BindingOp>(*definition)) {
        if (binding.getEffect() != "stateful")
          return invoke.emitOpError()
                 << "invoke callee '" << invoke.getCalleeAttr()
                 << "' requires effect 'stateful'";
      } else {
        auto type = cast<TypeOp>(*definition);
        if (type.getKind() != "implementation")
          return invoke.emitOpError()
                 << "callee reference '" << invoke.getCalleeAttr()
                 << "' resolves to non-implementation acsim.type";
        std::string key = definitionKey(type);
        auto [entry, inserted] =
            generatedCallEffects.try_emplace(key, "invoke");
        if (!inserted && entry->second != "invoke")
          return invoke.emitOpError()
                 << "generated implementation callee '"
                 << invoke.getCalleeAttr()
                 << "' cannot be used by both acsim.inline and acsim.invoke";
      }
      for (Type type : invoke.getResultTypes())
        if (!isa<ValueType, WakeType, IntegerType>(type))
          return invoke.emitOpError(
              "invoke results must be exact !acsim.value, !acsim.wake, or "
              "integer types");
    } else if (auto exportOp = dyn_cast<ExportOp>(operation)) {
      if (exportOp.getValue().getType() != exportOp.getResult().getType())
        return exportOp.emitOpError(
            "export result type must exactly preserve its internal value");
      const std::array<StringRef, 1> kinds = {"role"};
      if (failed(requireTypeKind(index, exportOp, exportOp.getRoleAttr(), kinds,
                                 "export role")))
        return failure();
    } else if (auto process = dyn_cast<ProcessOp>(operation)) {
      for (Value capture : process.getCaptures())
        if (failed(verifyCaptureBoundary(process, capture, index)))
          return failure();
      if (failed(verifyProcess(process, index)))
        return failure();
    }
  }

  for (Operation &operation : model.getBody().front()) {
    auto module = dyn_cast<ModuleOp>(operation);
    if (!module)
      continue;
    if (module.getBody().empty() || module.getBody().front().empty() ||
        !isa<ReturnOp>(module.getBody().front().back()))
      return module.emitOpError("module must end in one acsim.return");
    SmallVector<ExportOp> exports;
    for (Operation &child : module.getBody().front())
      if (auto exportOp = dyn_cast<ExportOp>(child))
        exports.push_back(exportOp);
    SmallVector<std::pair<StringRef, DictionaryAttr>> interfaceRecords;
    for (StringRef field :
         {StringRef("ports"), StringRef("resources"), StringRef("results")})
      for (Attribute attribute : module.getInterface().getAs<ArrayAttr>(field))
        interfaceRecords.emplace_back(field, cast<DictionaryAttr>(attribute));
    if (module.getExports().size() != exports.size() ||
        exports.size() != interfaceRecords.size())
      return module.emitOpError(
          "module exports must exactly cover its ordered interface records");
    for (auto [attribute, exportOp, interfaceRecord] :
         llvm::zip_equal(module.getExports(), exports, interfaceRecords)) {
      auto reference = dyn_cast<FlatSymbolRefAttr>(attribute);
      DictionaryAttr record = interfaceRecord.second;
      StringRef exportName = exportOp.getSymName();
      if (!reference || reference.getValue() != exportName ||
          record.getAs<StringAttr>("name").getValue() != exportName)
        return module.emitOpError(
            "module export names must match ordered interface records and "
            "acsim.export declarations");
      if (interfaceRecord.first == "ports") {
        auto projection = exportOp.getValue().getDefiningOp<PortOp>();
        auto type = dyn_cast<PortType>(exportOp.getValue().getType());
        DictionaryAttr endpoint =
            findProjectedEndpoint(exportOp.getValue(), index);
        if (!projection || !type || !endpoint ||
            projection.getAccessorAttr() !=
                record.getAs<FlatSymbolRefAttr>("accessor") ||
            endpoint.getAs<StringAttr>("delegation") !=
                record.getAs<StringAttr>("delegation") ||
            exportOp.getRoleAttr() != record.getAs<FlatSymbolRefAttr>("role") ||
            type.getInterface() !=
                record.getAs<FlatSymbolRefAttr>("interface") ||
            type.getRole() != record.getAs<FlatSymbolRefAttr>("role") ||
            type.getPayload() != record.getAs<FlatSymbolRefAttr>("payload") ||
            type.getProtocol() != record.getAs<FlatSymbolRefAttr>("protocol"))
          return exportOp.emitOpError(
              "port export must exactly match its module interface record");
      } else if (interfaceRecord.first == "resources") {
        auto projection = exportOp.getValue().getDefiningOp<ResourceOp>();
        auto type = dyn_cast<ResourceType>(exportOp.getValue().getType());
        DictionaryAttr endpoint =
            findProjectedEndpoint(exportOp.getValue(), index);
        if (!projection || !type || !endpoint ||
            projection.getAccessorAttr() !=
                record.getAs<FlatSymbolRefAttr>("accessor") ||
            endpoint.getAs<StringAttr>("delegation") !=
                record.getAs<StringAttr>("delegation") ||
            exportOp.getRoleAttr() != record.getAs<FlatSymbolRefAttr>("role") ||
            type.getResource() != record.getAs<FlatSymbolRefAttr>("resource") ||
            type.getRole() != record.getAs<FlatSymbolRefAttr>("role"))
          return exportOp.emitOpError(
              "resource export must exactly match its module interface record");
      } else {
        SymbolRefAttr cppType;
        if (auto value = dyn_cast<ValueType>(exportOp.getValue().getType()))
          cppType = value.getSymbol();
        else if (auto expr = dyn_cast<ExprType>(exportOp.getValue().getType()))
          cppType = expr.getSymbol();
        if (!cppType || cppType != record.getAs<FlatSymbolRefAttr>("cpp_type"))
          return exportOp.emitOpError(
              "result export must exactly match its module interface record");
      }
    }
    auto returnOp = cast<ReturnOp>(module.getBody().front().back());
    if (returnOp.getValues().size() != exports.size())
      return returnOp.emitOpError(
          "return values must exactly match ordered module exports");
    for (auto [value, exportOp] :
         llvm::zip_equal(returnOp.getValues(), exports))
      if (value != exportOp.getResult())
        return returnOp.emitOpError(
            "return values must be the exact ordered export results");
  }
  return success();
}

std::string generatedProcessThunk(ProcessOp process, StringRef kind) {
  ModuleOp module = process->getParentOfType<ModuleOp>();
  std::string result = "acsim_generated::";
  result.append(module.getSymName());
  result.append("::s");
  result.append(module.getSpecializationFingerprint().drop_front(7));
  result.append("::");
  result.append(process.getSymName());
  result.append("::p");
  result.append(process.getSpecializationFingerprint().drop_front(7));
  result.append("::");
  result.append(kind);
  return result;
}

LogicalResult verifyDispatchAndActivation(ModelOp model,
                                          const ModelIndex &index,
                                          HierarchyExpansion &expansion) {
  llvm::DenseMap<int64_t, DispatchOp> dispatchByObject;
  for (Operation &operation : model.getBody().front()) {
    if (modelVerificationWorkCollector)
      ++modelVerificationWorkCollector->runtimeOperationVisits;
    auto dispatch = dyn_cast<DispatchOp>(operation);
    if (!dispatch)
      continue;
    int64_t id = dispatch.getObjectId();
    if (id < 0 || static_cast<uint64_t>(id) >= expansion.runtimeRows.size())
      return dispatch.emitOpError(
          "dispatch object ID has no expanded runtime object");
    const ExpandedRuntimeRow &row = expansion.runtimeRows[id];
    FailureOr<Operation *> targetDefinition =
        requireReference<InstanceOp, ArrayOp, ProcessOp>(
            index, dispatch, dispatch.getTargetAttr(), "dispatch target");
    if (failed(targetDefinition))
      return failure();
    std::string target = symbolKey(dispatch.getTargetAttr());
    if (*targetDefinition != row.placement || target != row.target ||
        dispatch.getPath() != row.path ||
        dispatch.getIndices() != ArrayRef<int64_t>(row.indices) ||
        dispatch.getActivationId() != row.activationId)
      return dispatch.emitOpError(
          "dispatch target, path, indices, and IDs must jointly match one "
          "derived expanded row");
    if (!dispatchByObject.try_emplace(id, dispatch).second)
      return dispatch.emitOpError(
          "runtime object has more than one dispatch row");
    if (auto binding = dyn_cast<BindingOp>(row.realization)) {
      auto entryPoints =
          binding.getCppRecord().getAs<DictionaryAttr>("entry_points");
      if (dispatch.getWorkAttr() != entryPoints.getAs<StringAttr>("work") ||
          dispatch.getXferAttr() != entryPoints.getAs<StringAttr>("xfer") ||
          dispatch.getResetAttr() != entryPoints.getAs<StringAttr>("reset") ||
          dispatch.getValidateAttr() !=
              entryPoints.getAs<StringAttr>("validate"))
        return dispatch.emitOpError(
            "dispatch thunks must exactly match the placement binding lock");
    } else {
      auto process = cast<ProcessOp>(row.realization);
      if (dispatch.getWork() != generatedProcessThunk(process, "work") ||
          dispatch.getXfer() != generatedProcessThunk(process, "xfer") ||
          dispatch.getReset() != generatedProcessThunk(process, "reset") ||
          dispatch.getValidate() != generatedProcessThunk(process, "validate"))
        return dispatch.emitOpError(
            "dispatch thunks must exactly match the generated process "
            "realization");
    }
  }
  if (dispatchByObject.size() != expansion.runtimeRows.size())
    return model.emitOpError(
        "every runtime object requires exactly one typed dispatch row");

  using DependencyKey = std::pair<unsigned, void *>;
  std::map<DependencyKey, std::set<int64_t>> dependencyMemo;
  uint64_t dependencyNodes = 0;
  auto collectIds = [&](Value rootValue, unsigned context,
                        Operation *reporter) -> FailureOr<std::set<int64_t>> {
    struct Dependency {
      unsigned context;
      Value value;
    };
    struct Frame {
      DependencyKey key;
      Value value;
      SmallVector<Dependency> dependencies;
      size_t next = 0;
      std::set<int64_t> result;
      bool initialized = false;
    };
    DependencyKey rootKey{context, rootValue.getAsOpaquePointer()};
    if (auto found = dependencyMemo.find(rootKey);
        found != dependencyMemo.end())
      return found->second;
    SmallVector<Frame> stack{{rootKey, rootValue}};
    std::set<DependencyKey> active;
    const uint64_t limit = currentModelVerificationLimits().maxDependencyNodes;
    while (!stack.empty()) {
      Frame &frame = stack.back();
      if (!frame.initialized) {
        if (!active.insert(frame.key).second)
          return reporter->emitOpError(
              "typed SSA dependency graph contains a cycle");
        if (++dependencyNodes > limit)
          return reporter->emitOpError()
                 << "dependency graph exceeds ACSim capability " << limit;
        frame.initialized = true;
        if (auto argument = dyn_cast<BlockArgument>(frame.value)) {
          if (auto process =
                  dyn_cast<ProcessOp>(argument.getOwner()->getParentOp()))
            if (argument.getArgNumber() < process.getCaptures().size())
              frame.dependencies.push_back(
                  {frame.key.first,
                   process.getCaptures()[argument.getArgNumber()]});
        } else if (Operation *definition = frame.value.getDefiningOp()) {
          unsigned currentContext = frame.key.first;
          auto appendGeneratedEndpoint = [&](Value base, StringRef field,
                                             FlatSymbolRefAttr accessor) {
            Operation *wrapper = base.getDefiningOp();
            std::optional<uint64_t> elementOrdinal;
            if (auto element = dyn_cast_or_null<ElementOp>(wrapper)) {
              auto array = element.getArray().getDefiningOp<ArrayOp>();
              wrapper = array;
              if (array) {
                uint64_t ordinal = 0;
                for (auto [indexValue, extent] :
                     llvm::zip_equal(element.getIndices(), array.getShape()))
                  ordinal = ordinal * static_cast<uint64_t>(extent) +
                            static_cast<uint64_t>(indexValue);
                elementOrdinal = ordinal;
              }
            }
            auto children =
                expansion.contexts[currentContext].childContexts.find(wrapper);
            if (children ==
                expansion.contexts[currentContext].childContexts.end())
              return false;
            uint64_t ordinal = elementOrdinal.value_or(0);
            if (ordinal >= children->second.size())
              return true;
            unsigned childContext = children->second[ordinal];
            ExportOp exportOp = findModuleEndpointExport(
                expansion.contexts[childContext].module, field, accessor);
            if (exportOp)
              frame.dependencies.push_back({childContext, exportOp.getValue()});
            return true;
          };
          auto found =
              expansion.contexts[currentContext].objectIds.find(definition);
          if (isa<InstanceOp, ArrayOp>(definition) &&
              found != expansion.contexts[currentContext].objectIds.end()) {
            frame.result.insert(found->second.begin(), found->second.end());
          } else if (auto element = dyn_cast<ElementOp>(definition)) {
            auto array = element.getArray().getDefiningOp<ArrayOp>();
            auto rows =
                array ? expansion.contexts[currentContext].objectIds.find(array)
                      : expansion.contexts[currentContext].objectIds.end();
            if (rows != expansion.contexts[currentContext].objectIds.end()) {
              uint64_t ordinal = 0;
              for (auto [indexValue, extent] :
                   llvm::zip_equal(element.getIndices(), array.getShape()))
                ordinal = ordinal * static_cast<uint64_t>(extent) +
                          static_cast<uint64_t>(indexValue);
              if (ordinal < rows->second.size())
                frame.result.insert(rows->second[ordinal]);
            }
          } else if (auto port = dyn_cast<PortOp>(definition)) {
            if (!appendGeneratedEndpoint(port.getBase(), "ports",
                                         port.getAccessorAttr()))
              frame.dependencies.push_back({currentContext, port.getBase()});
          } else if (auto resource = dyn_cast<ResourceOp>(definition)) {
            if (!appendGeneratedEndpoint(resource.getBase(), "resources",
                                         resource.getAccessorAttr()))
              frame.dependencies.push_back(
                  {currentContext, resource.getBase()});
          } else {
            for (Value operand : definition->getOperands())
              frame.dependencies.push_back({currentContext, operand});
          }
        }
      }
      if (frame.next < frame.dependencies.size()) {
        Dependency dependency = frame.dependencies[frame.next];
        DependencyKey key{dependency.context,
                          dependency.value.getAsOpaquePointer()};
        if (auto found = dependencyMemo.find(key);
            found != dependencyMemo.end()) {
          frame.result.insert(found->second.begin(), found->second.end());
          ++frame.next;
          continue;
        }
        if (active.contains(key))
          return reporter->emitOpError(
              "typed SSA dependency graph contains a cycle");
        stack.push_back({key, dependency.value});
        continue;
      }
      dependencyMemo[frame.key] = frame.result;
      active.erase(frame.key);
      stack.pop_back();
    }
    return dependencyMemo.find(rootKey)->second;
  };

  std::set<std::pair<int64_t, int64_t>> expected;
  for (const ExpandedRuntimeRow &row : expansion.runtimeRows)
    expected.insert({row.activationId, row.objectId});
  for (auto [contextOrdinal, expansionContext] :
       llvm::enumerate(expansion.contexts)) {
    for (Operation &operation : expansionContext.module.getBody().front()) {
      if (auto bind = dyn_cast<BindOp>(operation)) {
        FailureOr<std::set<int64_t>> sources =
            collectIds(bind.getSource(), contextOrdinal, bind);
        FailureOr<std::set<int64_t>> targets =
            collectIds(bind.getTarget(), contextOrdinal, bind);
        if (failed(sources) || failed(targets))
          return failure();
        for (int64_t source : *sources)
          for (int64_t target : *targets)
            expected.insert(
                {expansion.runtimeRows[source].activationId, target});
      } else if (auto process = dyn_cast<ProcessOp>(operation)) {
        auto rows = expansionContext.objectIds.find(process);
        if (rows == expansionContext.objectIds.end() ||
            rows->second.size() != 1)
          return process.emitOpError(
              "process must instantiate once per concrete module context");
        int64_t processId = rows->second.front();
        for (Value capture : process.getCaptures()) {
          FailureOr<std::set<int64_t>> sources =
              collectIds(capture, contextOrdinal, process);
          if (failed(sources))
            return failure();
          for (int64_t source : *sources)
            expected.insert(
                {expansion.runtimeRows[source].activationId, processId});
        }
        LogicalResult invokeStatus = success();
        process.walk([&](InvokeOp invoke) -> WalkResult {
          for (Value argument : invoke.getArgs()) {
            FailureOr<std::set<int64_t>> sources =
                collectIds(argument, contextOrdinal, invoke);
            if (failed(sources)) {
              invokeStatus = failure();
              return WalkResult::interrupt();
            }
            for (int64_t source : *sources)
              expected.insert(
                  {expansion.runtimeRows[source].activationId, processId});
          }
          return WalkResult::advance();
        });
        if (failed(invokeStatus))
          return failure();
      }
    }
  }

  std::pair<int64_t, int64_t> previous{-1, -1};
  std::set<std::pair<int64_t, int64_t>> actual;
  for (Operation &operation : model.getBody().front()) {
    if (modelVerificationWorkCollector)
      ++modelVerificationWorkCollector->runtimeOperationVisits;
    auto activate = dyn_cast<ActivateOp>(operation);
    if (!activate)
      continue;
    auto sourceDispatch = activate.getSource().getDefiningOp<DispatchOp>();
    auto targetDispatch = activate.getTarget().getDefiningOp<DispatchOp>();
    if (!sourceDispatch || !targetDispatch ||
        activate.getSource() != sourceDispatch.getActivation() ||
        activate.getTarget() != targetDispatch.getObject())
      return activate.emitOpError(
          "activation edge must consume dispatch-produced typed IDs");
    std::pair<int64_t, int64_t> edge{sourceDispatch.getActivationId(),
                                     targetDispatch.getObjectId()};
    if (edge <= previous)
      return activate.emitOpError(
          "activation edges must be deduplicated and sorted by source,target");
    previous = edge;
    actual.insert(edge);
  }
  if (actual != expected)
    return model.emitOpError(
        "activation edges must exactly equal computed static dependencies");
  return success();
}

} // namespace

ParseResult ProcessOp::parse(OpAsmParser &parser, OperationState &result) {
  Builder &builder = parser.getBuilder();
  StringAttr name;
  SmallVector<OpAsmParser::UnresolvedOperand> captures;
  SmallVector<Type> captureTypes;
  ArrayAttr captureNames;
  FlatSymbolRefAttr entryPc;
  ArrayAttr pcs;
  ArrayAttr liveSlots;
  int64_t fairnessCap;
  StringAttr specializationFingerprint;

  if (parser.parseSymbolName(name, SymbolTable::getSymbolAttrName(),
                             result.attributes) ||
      parser.parseKeyword("captures") || parser.parseLParen())
    return failure();
  if (failed(parser.parseOptionalRParen())) {
    do {
      captures.emplace_back();
      Type type;
      if (parser.parseOperand(captures.back()) || parser.parseColonType(type))
        return failure();
      captureTypes.push_back(type);
    } while (succeeded(parser.parseOptionalComma()));
    if (parser.parseRParen())
      return failure();
  }
  if (parser.resolveOperands(captures, captureTypes,
                             parser.getCurrentLocation(), result.operands) ||
      parser.parseKeyword("names") || parser.parseAttribute(captureNames) ||
      parser.parseKeyword("entry") || parser.parseAttribute(entryPc) ||
      parser.parseKeyword("pcs") || parser.parseAttribute(pcs) ||
      parser.parseKeyword("live") || parser.parseAttribute(liveSlots) ||
      parser.parseKeyword("fairness") || parser.parseInteger(fairnessCap) ||
      parser.parseKeyword("specialization") ||
      parser.parseAttribute(specializationFingerprint))
    return failure();

  result.addAttribute("capture_names", captureNames);
  result.addAttribute("entry_pc", entryPc);
  result.addAttribute("pcs", pcs);
  result.addAttribute("live_slots", liveSlots);
  result.addAttribute("fairness_cap", builder.getI64IntegerAttr(fairnessCap));
  result.addAttribute("specialization_fingerprint", specializationFingerprint);
  if (parser.parseOptionalAttrDictWithKeyword(result.attributes) ||
      parser.parseLBrace())
    return failure();

  for (Attribute attribute : pcs) {
    auto expected = dyn_cast<FlatSymbolRefAttr>(attribute);
    FlatSymbolRefAttr label;
    if (!expected || parser.parseKeyword("state") ||
        parser.parseAttribute(label))
      return parser.emitError(parser.getCurrentLocation(),
                              "expected one flat state label per PC");
    if (label != expected)
      return parser.emitError(parser.getCurrentLocation())
             << "state label " << label << " does not match ordered PC "
             << expected;
    Region *state = result.addRegion();
    if (parser.parseRegion(*state))
      return failure();
  }
  return parser.parseRBrace();
}

void ProcessOp::print(OpAsmPrinter &printer) {
  printer << ' ';
  printer.printSymbolName(getSymName());
  printer << " captures(";
  llvm::interleaveComma(getCaptures(), printer, [&](Value capture) {
    printer << capture << " : " << capture.getType();
  });
  printer << ") names " << getCaptureNamesAttr() << " entry "
          << getEntryPcAttr() << " pcs " << getPcsAttr() << " live "
          << getLiveSlotsAttr() << " fairness " << getFairnessCap()
          << " specialization " << getSpecializationFingerprintAttr();
  printer.printOptionalAttrDictWithKeyword(
      (*this)->getAttrs(),
      {SymbolTable::getSymbolAttrName(), "capture_names", "entry_pc", "pcs",
       "live_slots", "fairness_cap", "specialization_fingerprint"});
  printer << " {";
  for (auto [pc, state] : llvm::zip(getPcs(), getStates())) {
    printer << "\nstate " << pc << ' ';
    printer.printRegion(state, /*printEntryBlockArgs=*/true,
                        /*printBlockTerminators=*/true,
                        /*printEmptyBlock=*/true);
  }
  printer << "\n}";
}

LogicalResult ModelOp::verify() {
  if (getContractEpoch() != "0.5")
    return emitOpError("contract epoch must be exactly \"0.5\"");
  auto parentModule = dyn_cast_or_null<mlir::ModuleOp>((*this)->getParentOp());
  if (!parentModule)
    return emitOpError("acsim.model must be directly inside builtin.module");
  unsigned models = 0;
  for (Operation &operation : *parentModule.getBody())
    models += isa<ModelOp>(operation);
  if (models != 1)
    return emitOpError("canonical ACSim requires exactly one acsim.model");
  if (parentModule.getBody()->getOperations().size() != 1)
    return emitOpError(
        "acsim.model must be the sole operation in the canonical file");
  if (failed(verifyModelFingerprints(*this)))
    return failure();
  if (getBody().empty())
    return emitOpError("model requires one closed body block");

  if (failed(preflightModel(*this)))
    return failure();
  ModelIndex index;
  HierarchyExpansion expansion;
  if (failed(buildIndex(*this, index)) ||
      failed(verifyClosedLegality(*this, index.ordered)) ||
      failed(verifyDeterministicOrder(*this)) ||
      failed(expandSelectedRootOwners(*this, index, expansion)) ||
      failed(expandRuntime(index, expansion)) ||
      failed(verifyConstructionOrder(*this, expansion)) ||
      failed(verifyModulesAndTypedGraph(*this, index)) ||
      failed(verifyDispatchAndActivation(*this, index, expansion)))
    return failure();
  return success();
}

LogicalResult TypeOp::verify() {
  constexpr std::array<StringLiteral, 14> kinds = {
      "accessor",    "implementation", "interface", "packet", "policy",
      "protocol",    "provider",       "resource",  "role",   "schema",
      "time_domain", "value",          "wake",      "payload"};
  if (!llvm::is_contained(kinds, getKind()))
    return emitOpError("kind is not a closed ACSim C++ realization kind");
  if (getCppName().empty() || hasRawCppFragment(getCppName()))
    return emitOpError(
        "C++ spelling must be a non-empty declarative symbol/type spelling");
  if (failed(verifyFingerprint(*this, getFingerprintAttr())))
    return failure();

  IntegerAttr period = getPeriodAttr();
  IntegerAttr phase = getPhaseAttr();
  IntegerAttr tickScale = getTickScaleAttr();
  FlatSymbolRefAttr parent = getParentAttr();
  DictionaryAttr bridge = getBridgeAttr();
  const bool hasRuntimeMetadata =
      period || phase || tickScale || parent || bridge;
  if (!hasRuntimeMetadata)
    return success();
  if (getKind() != "time_domain")
    return emitOpError("runtime domain metadata is legal only for time_domain");
  if (!period || !phase || !tickScale ||
      !period.getType().isSignlessInteger(64) ||
      !phase.getType().isSignlessInteger(64) ||
      !tickScale.getType().isSignlessInteger(64) || period.getInt() <= 0 ||
      phase.getInt() < 0 || tickScale.getInt() <= 0)
    return emitOpError(
        "time_domain requires exact positive period/tick_scale and "
        "non-negative phase i64 metadata");
  if (static_cast<bool>(parent) != static_cast<bool>(bridge))
    return emitOpError("time_domain parent requires exact bridge metadata");
  if (bridge) {
    auto kind = bridge.getAs<StringAttr>("kind");
    auto owner = bridge.getAs<FlatSymbolRefAttr>("owner");
    if (!hasExactKeys(bridge, {"kind", "owner"}) || !kind ||
        kind.getValue() != "explicit" || !owner)
      return emitOpError(
          "time_domain bridge requires exact explicit kind and owner");
  }
  return success();
}

LogicalResult BindingOp::verify() { return verifyBindingLockShape(*this); }

LogicalResult ModuleOp::verify() {
  if (!isCanonicalIdentifier(getSymName()))
    return emitOpError(
        "generated module symbol must be a canonical C++ identifier");
  if (failed(verifyFingerprint(*this, getSpecializationFingerprintAttr(),
                               "specialization fingerprint")) ||
      failed(verifyModuleInterfaceShape(*this)))
    return failure();
  if (getBody().empty() || getBody().front().empty() ||
      !isa<ReturnOp>(getBody().front().back()))
    return emitOpError("module must end in acsim.return");
  return success();
}

LogicalResult InstanceOp::verify() {
  if (failed(verifyFingerprint(*this, getSpecializationFingerprintAttr(),
                               "specialization fingerprint")))
    return failure();
  auto owner = dyn_cast<OwnerType>(getResult().getType());
  if (!owner || owner.getRealization() != getTargetAttr())
    return emitOpError("result must own the exact realization target");
  return success();
}

LogicalResult ArrayOp::verify() {
  if (failed(verifyFingerprint(*this, getSpecializationFingerprintAttr(),
                               "specialization fingerprint")))
    return failure();
  auto type = dyn_cast<ArrayType>(getResult().getType());
  auto owner = type ? dyn_cast<OwnerType>(type.getElementType()) : OwnerType();
  if (!type || type.getShape().asArrayRef() != getShape() || !owner ||
      owner.getRealization() != getTargetAttr())
    return emitOpError(
        "result array type must exactly match shape and realization target");
  return success();
}

LogicalResult ElementOp::verify() {
  auto array = dyn_cast<ArrayType>(getArray().getType());
  if (!array || getIndices().size() != array.getShape().size())
    return emitOpError("indices must have the exact static array rank");
  return success();
}

LogicalResult PortOp::verify() {
  if (isa<OwnerType, RefType>(getBase().getType()) &&
      isa<PortType>(getResult().getType()))
    return success();
  return emitOpError("requires owner/ref input and typed port result");
}

LogicalResult ResourceOp::verify() {
  if (isa<OwnerType, RefType>(getBase().getType()) &&
      isa<ResourceType>(getResult().getType()))
    return success();
  return emitOpError("requires owner/ref input and resource result");
}

LogicalResult BindOp::verify() {
  auto findRealization = [&](Value base) -> Operation * {
    SymbolRefAttr reference;
    if (auto owner = dyn_cast<OwnerType>(base.getType()))
      reference = owner.getRealization();
    else if (auto ref = dyn_cast<RefType>(base.getType()))
      reference = ref.getRealization();
    if (!reference)
      return nullptr;
    ModelOp model = (*this)->getParentOfType<ModelOp>();
    if (!model)
      return nullptr;
    for (Operation &operation : model.getBody().front())
      if (isa<BindingOp, ModuleOp>(operation) &&
          symbolName(&operation).getValue() ==
              reference.getRootReference().getValue())
        return &operation;
    return nullptr;
  };
  if (getKind() == "port") {
    auto sourceOp = getSource().getDefiningOp<PortOp>();
    auto targetOp = getTarget().getDefiningOp<PortOp>();
    auto source = dyn_cast<PortType>(getSource().getType());
    auto target = dyn_cast<PortType>(getTarget().getType());
    DictionaryAttr sourceRecord =
        sourceOp ? findEndpoint(findRealization(sourceOp.getBase()), "ports",
                                sourceOp.getAccessorAttr())
                 : DictionaryAttr();
    DictionaryAttr targetRecord =
        targetOp ? findEndpoint(findRealization(targetOp.getBase()), "ports",
                                targetOp.getAccessorAttr())
                 : DictionaryAttr();
    if (!source || !target || !sourceRecord || !targetRecord ||
        sourceRecord.getAs<StringAttr>("direction").getValue() != "output" ||
        targetRecord.getAs<StringAttr>("direction").getValue() != "input" ||
        sourceRecord.getAs<FlatSymbolRefAttr>("interface") !=
            source.getInterface() ||
        sourceRecord.getAs<FlatSymbolRefAttr>("role") != source.getRole() ||
        sourceRecord.getAs<FlatSymbolRefAttr>("payload") !=
            source.getPayload() ||
        sourceRecord.getAs<FlatSymbolRefAttr>("protocol") !=
            source.getProtocol() ||
        targetRecord.getAs<FlatSymbolRefAttr>("interface") !=
            target.getInterface() ||
        targetRecord.getAs<FlatSymbolRefAttr>("role") != target.getRole() ||
        targetRecord.getAs<FlatSymbolRefAttr>("payload") !=
            target.getPayload() ||
        targetRecord.getAs<FlatSymbolRefAttr>("protocol") !=
            target.getProtocol())
      return emitOpError("port bind endpoints must match exact output/input "
                         "binding-lock records");
    if (sourceRecord.get("interface") != targetRecord.get("interface") ||
        sourceRecord.get("payload") != targetRecord.get("payload") ||
        sourceRecord.get("protocol") != targetRecord.get("protocol") ||
        sourceRecord.get("cardinality") != targetRecord.get("cardinality") ||
        sourceRecord.get("delegation") != targetRecord.get("delegation") ||
        sourceRecord.get("ownership") != targetRecord.get("ownership") ||
        sourceRecord.get("time_domain") != targetRecord.get("time_domain"))
      return emitOpError(
          "port bind endpoints must have identical interface, payload, "
          "protocol, cardinality, delegation, ownership, and time domain");
    return success();
  } else if (getKind() == "resource") {
    auto sourceOp = getSource().getDefiningOp<ResourceOp>();
    auto targetOp = getTarget().getDefiningOp<ResourceOp>();
    auto source = dyn_cast<ResourceType>(getSource().getType());
    auto target = dyn_cast<ResourceType>(getTarget().getType());
    DictionaryAttr sourceRecord =
        sourceOp ? findEndpoint(findRealization(sourceOp.getBase()),
                                "resources", sourceOp.getAccessorAttr())
                 : DictionaryAttr();
    DictionaryAttr targetRecord =
        targetOp ? findEndpoint(findRealization(targetOp.getBase()),
                                "resources", targetOp.getAccessorAttr())
                 : DictionaryAttr();
    if (!source || !target || !sourceRecord || !targetRecord ||
        sourceRecord.getAs<StringAttr>("mode").getValue() != "initiator" ||
        targetRecord.getAs<StringAttr>("mode").getValue() != "target" ||
        sourceRecord.getAs<FlatSymbolRefAttr>("resource") !=
            source.getResource() ||
        sourceRecord.getAs<FlatSymbolRefAttr>("role") != source.getRole() ||
        targetRecord.getAs<FlatSymbolRefAttr>("resource") !=
            target.getResource() ||
        targetRecord.getAs<FlatSymbolRefAttr>("role") != target.getRole())
      return emitOpError("resource bind endpoints must match exact "
                         "initiator/target binding-lock records");
    if (sourceRecord.get("resource") != targetRecord.get("resource") ||
        sourceRecord.get("delegation") != targetRecord.get("delegation") ||
        sourceRecord.get("ownership") != targetRecord.get("ownership") ||
        sourceRecord.get("time_domain") != targetRecord.get("time_domain"))
      return emitOpError(
          "resource bind endpoints must have identical resource kind, "
          "delegation, ownership, and time domain");
    return success();
  } else if (getKind() == "pure_view") {
    if (getSource() != getTarget() &&
        getSource().getType() == getTarget().getType() &&
        isa<ExprType>(getSource().getType()))
      return success();
  } else if (getKind() == "export") {
    auto target = getTarget().getDefiningOp<ExportOp>();
    if (!target || getTarget() != target.getResult())
      return emitOpError(
          "export bind target must be the exact result of acsim.export");
    if (getSource() != target.getValue())
      return emitOpError(
          "export bind source must be the exact input of its target "
          "acsim.export");
    if (getSource().getType() == getTarget().getType())
      return success();
  }
  return emitOpError("typed binding endpoints are not exactly compatible");
}

LogicalResult InlineOp::verify() {
  if (!isMemoryEffectFree(*this))
    return emitOpError("inline must remain effect-free");
  Type result = getResult().getType();
  if (getOperation()->getParentOfType<ProcessOp>()) {
    if (!isa<IntegerType, FloatType, IndexType, ValueType>(result))
      return emitOpError(
          "process inline result must be an integer, float, index, or "
          "!acsim.value");
  } else if (!isa<ExprType>(result)) {
    return emitOpError("module inline result must be exactly !acsim.expr");
  }
  return success();
}

LogicalResult ProcessOp::verify() {
  if (!isCanonicalIdentifier(getSymName()))
    return emitOpError(
        "generated process symbol must be a canonical C++ identifier");
  return verifyFingerprint(*this, getSpecializationFingerprintAttr(),
                           "specialization fingerprint");
}

LogicalResult LiveLoadOp::verify() {
  return isa<ValueType>(getResult().getType())
             ? success()
             : emitOpError("live load must produce a typed value");
}

LogicalResult LiveStoreOp::verify() {
  return isa<ValueType>(getValue().getType())
             ? success()
             : emitOpError("live store requires a typed value");
}

LogicalResult InvokeOp::verify() {
  for (Type type : getResultTypes())
    if (!isa<ValueType, WakeType, IntegerType>(type))
      return emitOpError(
          "invoke results must be exact !acsim.value, !acsim.wake, or integer "
          "types");
  return success();
}

LogicalResult ContinueOp::verify() { return success(); }

LogicalResult SuspendOp::verify() {
  return isa<WakeType>(getWake().getType())
             ? success()
             : emitOpError("requires an exact typed wake");
}

LogicalResult TerminateOp::verify() {
  if (getStatus() != "success" && getStatus() != "failure")
    return emitOpError("status must be exactly 'success' or 'failure'");
  return success();
}

LogicalResult ExportOp::verify() {
  if (getValue().getType() != getResult().getType())
    return emitOpError("must preserve the exact exported type");
  if (isa<OwnerType, ObjectIdType, ActivationIdType, PcType, WakeType>(
          getResult().getType()))
    return emitOpError("owners, IDs, PCs, and wakes cannot escape a module");
  return success();
}

LogicalResult DispatchOp::verify() {
  if (getObjectId() < 0 || getActivationId() < 0 || getPath().empty())
    return emitOpError(
        "object and activation IDs must be non-negative and path non-empty");
  for (StringRef thunk : {getWork(), getXfer(), getReset(), getValidate()})
    if (!isCppQualifiedSymbol(thunk))
      return emitOpError(
          "dispatch thunks must be non-empty declarative C++ symbols");
  return success();
}

LogicalResult ActivateOp::verify() { return success(); }

LogicalResult ReturnOp::verify() {
  if (!isa_and_nonnull<ModuleOp>((*this)->getParentOp()))
    return emitOpError("must terminate an acsim.module body");
  return success();
}

LogicalResult verifyCanonicalACSimFile(mlir::ModuleOp module) {
  unsigned models = 0;
  bool hasACSimOperation = false;
  ModelOp model;
  SmallVector<Operation *> stack;
  for (Operation &operation : llvm::reverse(*module.getBody()))
    stack.push_back(&operation);
  while (!stack.empty()) {
    Operation *operation = stack.pop_back_val();
    if (operation->getName().getDialectNamespace() == "acsim")
      hasACSimOperation = true;
    if (auto candidate = dyn_cast<ModelOp>(operation)) {
      ++models;
      model = candidate;
    }
    for (Region &region : llvm::reverse(operation->getRegions()))
      for (Block &block : llvm::reverse(region))
        for (Operation &child : llvm::reverse(block))
          stack.push_back(&child);
  }
  if (!hasACSimOperation)
    return success();
  if (models != 1)
    return module.emitError("canonical ACSim requires exactly one acsim.model");
  if (model->getParentOp() != module)
    return model.emitOpError(
        "canonical acsim.model must be directly inside the file module");
  auto epoch = module->getAttrOfType<StringAttr>("ac.contract_epoch");
  auto discardable = module->getDiscardableAttrs();
  if (!epoch || epoch.getValue() != "0.5" ||
      std::distance(discardable.begin(), discardable.end()) != 1)
    return module.emitError("canonical ACSim file attributes must be exactly "
                            "ac.contract_epoch = \"0.5\"");
  return success();
}

} // namespace acir::acsim

#define GET_OP_CLASSES
#include "acir/Dialect/ACSim/ACSimOps.cpp.inc"
