#include "acir/Transforms/Passes.h"

#include "Dialect/ACIR/ProcessLowerability.h"
#include "acir/Bindings/Binding.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/ACIRResources.h"
#include "acir/Dialect/ACSim/ACSimOps.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"

#include <limits>

namespace acir {
namespace {

bool isValidPythonSourcePath(llvm::StringRef path) {
  if (path.empty() || llvm::sys::path::is_absolute(path) ||
      !path.ends_with(".py") || path.contains('\\') || path.contains("//") ||
      (path.size() >= 2 && llvm::isAlpha(path[0]) && path[1] == ':'))
    return false;
  llvm::SmallVector<llvm::StringRef> segments;
  path.split(segments, '/', /*MaxSplit=*/-1, /*KeepEmpty=*/true);
  for (llvm::StringRef segment : segments) {
    if (segment.empty() || segment == "." || segment == "..")
      return false;
    for (char character : segment)
      if (!llvm::isAlnum(character) && character != '.' && character != '_' &&
          character != '+' && character != '@' && character != '-')
        return false;
  }
  return true;
}

void appendFingerprintPart(llvm::SHA256 &sha, llvm::StringRef value) {
  sha.update(value);
  const uint8_t zero = 0;
  sha.update(llvm::ArrayRef<uint8_t>(&zero, 1));
}

std::string printType(mlir::Type type) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  type.print(stream);
  return stream.str();
}

std::string sha256String(llvm::StringRef value) {
  llvm::SHA256 sha;
  sha.update(value);
  return "sha256:" + llvm::toHex(sha.final(), /*LowerCase=*/true);
}

struct StaticConfigRoot {
  std::string root;
  llvm::json::Value schema;
  llvm::json::Value value;
};

bool validateStaticConfigValue(const llvm::json::Value &schema,
                               const llvm::json::Value &value) {
  const llvm::json::Object *schemaObject = schema.getAsObject();
  if (!schemaObject)
    return false;
  auto kind = schemaObject->getString("kind");
  auto version = schemaObject->getInteger("version");
  if (!kind || !version || *version != 1)
    return false;
  if (*kind == "scalar") {
    auto name = schemaObject->getString("name");
    if (!name)
      return false;
    if (*name == "int")
      return value.getAsInteger().has_value();
    if (*name == "bool")
      return value.getAsBoolean().has_value();
    if (*name == "float")
      return value.getAsNumber().has_value();
    if (*name == "str")
      return value.getAsString().has_value();
    return false;
  }
  if (*kind != "config" || !schemaObject->getString("name"))
    return false;
  const llvm::json::Array *fields = schemaObject->getArray("fields");
  const llvm::json::Object *valueObject = value.getAsObject();
  if (!fields || !valueObject || fields->size() != valueObject->size())
    return false;
  llvm::StringSet<> names;
  for (const llvm::json::Value &rawField : *fields) {
    const llvm::json::Object *field = rawField.getAsObject();
    auto name = field ? field->getString("name") : std::nullopt;
    const llvm::json::Value *fieldSchema =
        field ? field->get("type") : nullptr;
    const llvm::json::Value *fieldValue =
        name ? valueObject->get(*name) : nullptr;
    if (!field || field->size() != 2 || !name || name->empty() ||
        !names.insert(*name).second || !fieldSchema || !fieldValue ||
        !validateStaticConfigValue(*fieldSchema, *fieldValue))
      return false;
  }
  return true;
}

std::optional<int64_t>
projectStaticConfigInteger(const StaticConfigRoot &config,
                           llvm::StringRef path) {
  const llvm::json::Value *schema = &config.schema;
  const llvm::json::Value *value = &config.value;
  llvm::SmallVector<llvm::StringRef> fields;
  path.split(fields, '.');
  if (fields.empty())
    return std::nullopt;
  for (llvm::StringRef name : fields) {
    const llvm::json::Object *schemaObject = schema->getAsObject();
    const llvm::json::Object *valueObject = value->getAsObject();
    const llvm::json::Array *schemaFields =
        schemaObject ? schemaObject->getArray("fields") : nullptr;
    if (!schemaFields || !valueObject)
      return std::nullopt;
    const llvm::json::Value *nextSchema = nullptr;
    for (const llvm::json::Value &rawField : *schemaFields) {
      const llvm::json::Object *field = rawField.getAsObject();
      if (field && field->getString("name") == name) {
        nextSchema = field->get("type");
        break;
      }
    }
    const llvm::json::Value *nextValue = valueObject->get(name);
    if (!nextSchema || !nextValue)
      return std::nullopt;
    schema = nextSchema;
    value = nextValue;
  }
  const llvm::json::Object *leafSchema = schema->getAsObject();
  if (!leafSchema || leafSchema->getString("kind") != "scalar" ||
      leafSchema->getString("name") != "int")
    return std::nullopt;
  return value->getAsInteger();
}

std::string staticStructFingerprint(ac::StructOp structure,
                                    llvm::StringRef source,
                                    mlir::ArrayAttr identityBindings) {
  llvm::SHA256 sha;
  appendFingerprintPart(sha, "ac.struct-specialization-v1");
  appendFingerprintPart(sha, source);
  for (mlir::Attribute rawField : structure.getFields()) {
    auto field = mlir::cast<mlir::DictionaryAttr>(rawField);
    appendFingerprintPart(sha,
                          field.getAs<mlir::StringAttr>("name").getValue());
    appendFingerprintPart(
        sha, printType(field.getAs<mlir::TypeAttr>("type").getValue()));
  }
  for (mlir::Attribute rawBinding : identityBindings) {
    auto binding = mlir::cast<mlir::DictionaryAttr>(rawBinding);
    appendFingerprintPart(sha,
                          binding.getAs<mlir::StringAttr>("name").getValue());
    appendFingerprintPart(
        sha,
        std::to_string(binding.getAs<mlir::IntegerAttr>("value").getInt()));
  }
  return "sha256:" + llvm::toHex(sha.final(), /*LowerCase=*/true);
}

mlir::LogicalResult verifySourceProvenance(mlir::ModuleOp module) {
  mlir::WalkResult result = module.walk([&](mlir::Operation *operation) {
    mlir::Attribute raw = operation->getAttr("ac.source_provenance");
    if (!raw)
      return mlir::WalkResult::advance();
    auto origins = mlir::dyn_cast<mlir::ArrayAttr>(raw);
    if (!origins || origins.empty()) {
      operation->emitError("source provenance must be a non-empty origin array");
      return mlir::WalkResult::interrupt();
    }
    std::string previous;
    for (mlir::Attribute rawOrigin : origins) {
      auto origin = mlir::dyn_cast<mlir::DictionaryAttr>(rawOrigin);
      auto frames = origin ? origin.getAs<mlir::ArrayAttr>("frames")
                           : mlir::ArrayAttr();
      if (!origin || origin.size() != 1 || !frames || frames.empty()) {
        operation->emitError("source provenance origin is malformed");
        return mlir::WalkResult::interrupt();
      }
      std::string key;
      llvm::raw_string_ostream keyStream(key);
      unsigned previousKindRank = 0;
      bool firstFrame = true;
      for (mlir::Attribute rawFrame : frames) {
        auto frame = mlir::dyn_cast<mlir::DictionaryAttr>(rawFrame);
        auto file = frame ? frame.getAs<mlir::StringAttr>("file")
                          : mlir::StringAttr();
        auto kind = frame ? frame.getAs<mlir::StringAttr>("kind")
                          : mlir::StringAttr();
        auto line = frame ? frame.getAs<mlir::IntegerAttr>("line")
                          : mlir::IntegerAttr();
        auto column = frame ? frame.getAs<mlir::IntegerAttr>("column")
                            : mlir::IntegerAttr();
        auto symbol = frame ? frame.getAs<mlir::StringAttr>("symbol")
                            : mlir::StringAttr();
        unsigned kindRank =
            !kind || kind.getValue() == "statement" ||
                    kind.getValue() == "definition"
                ? 0
            : kind.getValue() == "inline_callsite" ? 1
            : kind.getValue() == "instance"        ? 2
                                                    : 3;
        if (!frame || (frame.size() != 4 && frame.size() != 5) || !file ||
            !kind || !line || !column ||
            (frame.size() == 5) != static_cast<bool>(symbol) ||
            !isValidPythonSourcePath(file.getValue()) ||
            line.getInt() <= 0 || column.getInt() <= 0 ||
            (kind.getValue() != "statement" &&
             kind.getValue() != "definition" &&
             kind.getValue() != "inline_callsite" &&
             kind.getValue() != "instance" &&
             kind.getValue() != "specialization") ||
            (symbol && symbol.getValue().empty()) ||
            (!firstFrame && kindRank < previousKindRank)) {
          operation->emitError("source provenance frame is malformed");
          return mlir::WalkResult::interrupt();
        }
        previousKindRank = kindRank;
        firstFrame = false;
        keyStream << kind.getValue() << '\0' << file.getValue() << '\0'
                  << line.getInt() << '\0' << column.getInt() << '\0'
                  << (symbol ? symbol.getValue() : llvm::StringRef()) << '\0';
      }
      keyStream.flush();
      if (!previous.empty() && previous >= key) {
        operation->emitError(
            "source provenance origins must be unique and canonical");
        return mlir::WalkResult::interrupt();
      }
      previous = std::move(key);
    }
    return mlir::WalkResult::advance();
  });
  return result.wasInterrupted() ? mlir::failure() : mlir::success();
}

mlir::LogicalResult verifyStaticTypeMetadata(mlir::ModuleOp module) {
  auto rawBindings =
      module->getAttrOfType<mlir::DictionaryAttr>("ac.static_type_bindings");
  auto checks = module->getAttrOfType<mlir::ArrayAttr>("ac.static_type_checks");
  auto identities =
      module->getAttrOfType<mlir::ArrayAttr>("ac.static_type_identities");
  mlir::Attribute rawConfigBindings =
      module->getAttr("ac.static_config_bindings");
  auto configBindings =
      mlir::dyn_cast_or_null<mlir::ArrayAttr>(rawConfigBindings);
  if (rawConfigBindings && !configBindings)
    return module.emitError("static config bindings must be an array");
  if (!rawBindings && !checks && !identities && !configBindings)
    return mlir::success();
  if (!rawBindings || !checks)
    return module.emitError(
        "static type bindings and checks must be provided together");

  llvm::StringMap<int64_t> bindings;
  for (mlir::NamedAttribute binding : rawBindings) {
    auto value = mlir::dyn_cast<mlir::IntegerAttr>(binding.getValue());
    if (!value || !value.getType().isSignlessInteger(64) ||
        !bindings.try_emplace(binding.getName(), value.getInt()).second)
      return module.emitError(
          "static type bindings require unique i64 integer values");
  }

  llvm::SmallVector<StaticConfigRoot> configs;
  llvm::StringSet<> configRoots;
  if (configBindings) {
    for (mlir::Attribute rawConfig : configBindings) {
      auto config = mlir::dyn_cast<mlir::DictionaryAttr>(rawConfig);
      auto root = config ? config.getAs<mlir::StringAttr>("root")
                         : mlir::StringAttr();
      auto type = config ? config.getAs<mlir::StringAttr>("type")
                         : mlir::StringAttr();
      auto schema = config ? config.getAs<mlir::StringAttr>("schema")
                           : mlir::StringAttr();
      auto schemaSha = config
                           ? config.getAs<mlir::StringAttr>("schema_sha256")
                           : mlir::StringAttr();
      auto value = config ? config.getAs<mlir::StringAttr>("value")
                          : mlir::StringAttr();
      if (!config || config.size() != 5 || !root || !type || !schema ||
          !schemaSha || !value || root.getValue().empty() ||
          type.getValue().empty() ||
          !configRoots.insert(root.getValue()).second ||
          schemaSha.getValue() != sha256String(schema.getValue()))
        return module.emitError("static config binding metadata is malformed");
      auto canonicalSchema =
          bindings::canonicalizeJsonText(schema.getValue());
      auto canonicalValue = bindings::canonicalizeJsonText(value.getValue());
      if (!canonicalSchema || !canonicalValue) {
        if (!canonicalSchema)
          llvm::consumeError(canonicalSchema.takeError());
        if (!canonicalValue)
          llvm::consumeError(canonicalValue.takeError());
        return module.emitError("static config schema or value is invalid");
      }
      if (*canonicalSchema != schema.getValue() ||
          *canonicalValue != value.getValue())
        return module.emitError(
            "static config schema and value must use canonical JSON");
      auto parsedSchema = llvm::json::parse(*canonicalSchema);
      auto parsedValue = llvm::json::parse(*canonicalValue);
      if (!parsedSchema || !parsedValue ||
          !validateStaticConfigValue(*parsedSchema, *parsedValue))
        return module.emitError("static config schema or value is invalid");
      const llvm::json::Object *schemaObject = parsedSchema->getAsObject();
      if (!schemaObject || schemaObject->getString("name") != type.getValue())
        return module.emitError("static config type disagrees with its schema");
      configs.push_back({root.getValue().str(), std::move(*parsedSchema),
                         std::move(*parsedValue)});
    }
  }
  llvm::StringSet<> usedConfigRoots;
  for (const auto &binding : bindings) {
    unsigned matchingRoots = 0;
    for (const StaticConfigRoot &config : configs) {
      llvm::StringRef parameter = binding.getKey();
      llvm::StringRef prefix = config.root;
      if (!parameter.consume_front(prefix) || !parameter.consume_front("."))
        continue;
      ++matchingRoots;
      auto projected = projectStaticConfigInteger(config, parameter);
      if (!projected || *projected != binding.getValue())
        return module.emitError()
               << "static config projection '" << binding.getKey()
               << "' disagrees with its root binding";
      usedConfigRoots.insert(config.root);
    }
    if (binding.getKey().contains('.') && matchingRoots != 1)
      return module.emitError()
             << "static config projection '" << binding.getKey()
             << "' must match exactly one config root";
  }
  for (llvm::StringRef root : configRoots.keys())
    if (!usedConfigRoots.contains(root))
      return module.emitError() << "static config root '" << root
                                << "' has no dependent type projection";

  ac::TypeScopeOp typeScope;
  for (ac::TypeScopeOp candidate : module.getBody()->getOps<ac::TypeScopeOp>())
    if (candidate.getSymName() == "types") {
      typeScope = candidate;
      break;
    }
  if (!typeScope) {
    if (identities)
      return module.emitError(
          "static type identities require ac.type_scope @types");
    for (mlir::Attribute rawCheck : checks) {
      auto check = mlir::dyn_cast<mlir::DictionaryAttr>(rawCheck);
      if (!check || !check.getAs<mlir::TypeAttr>("type"))
        return module.emitError(
            "struct static type checks require ac.type_scope @types");
    }
  }

  auto queueElementType = [](mlir::Type type) -> mlir::Type {
    if (auto queue = mlir::dyn_cast<ac::QueueType>(type))
      return queue.getElementType();
    return {};
  };
  auto resolveInterfaceType = [&](llvm::StringRef path) -> mlir::Type {
    llvm::SmallVector<llvm::StringRef> segments;
    path.split(segments, '.');
    if (segments.size() != 5 || segments[0] != "interface")
      return {};
    llvm::StringRef ownerKind = segments[1];
    llvm::StringRef ownerName = segments[2];
    llvm::StringRef direction = segments[3];
    llvm::StringRef endpoint = segments[4];
    if (ownerKind == "module") {
      auto definition = mlir::dyn_cast_or_null<ac::ModuleOp>(
          mlir::SymbolTable::lookupSymbolIn(module, ownerName));
      if (!definition)
        return {};
      if (direction == "input") {
        auto names = definition->getAttrOfType<mlir::ArrayAttr>(
            "ac.input_display_names");
        if (!names || names.size() != definition.getArgumentTypes().size())
          return {};
        for (auto [index, rawName] : llvm::enumerate(names))
          if (mlir::cast<mlir::StringAttr>(rawName).getValue() == endpoint)
            return queueElementType(definition.getArgumentTypes()[index]);
        return {};
      }
      unsigned ordinal = 0;
      if (direction != "output" || endpoint.getAsInteger(10, ordinal) ||
          ordinal >= definition.getResultTypes().size())
        return {};
      return queueElementType(definition.getResultTypes()[ordinal]);
    }
    if (ownerKind != "system")
      return {};
    auto systemName = module->getAttrOfType<mlir::StringAttr>("ac.system");
    bool matches = systemName && systemName.getValue() == ownerName;
    if (!matches)
      module.walk([&](ac::SystemOp system) {
        if (system.getSymName() == ownerName)
          matches = true;
      });
    if (!matches)
      return {};
    auto top = mlir::dyn_cast_or_null<ac::ModuleOp>(
        mlir::SymbolTable::lookupSymbolIn(module, "Top"));
    mlir::Operation *boundary =
        top ? top.getOperation() : module.getOperation();
    if (direction == "input") {
      mlir::Type result;
      boundary->walk([&](ac::SourceOp source) {
        auto name = source->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!result && name && name.getValue() == endpoint)
          result = source.getOutput().getType().getElementType();
      });
      return result;
    }
    unsigned ordinal = 0;
    if (direction != "output" || endpoint.getAsInteger(10, ordinal))
      return {};
    if (top && !top.getResultTypes().empty())
      return ordinal < top.getResultTypes().size()
                 ? queueElementType(top.getResultTypes()[ordinal])
                 : mlir::Type();
    llvm::SmallVector<mlir::Type> outputs;
    boundary->walk([&](ac::SinkOp sink) {
      outputs.push_back(sink.getInput().getType().getElementType());
    });
    return ordinal < outputs.size() ? outputs[ordinal] : mlir::Type();
  };

  llvm::StringSet<> referencedBindings;
  llvm::StringSet<> targets;
  for (mlir::Attribute rawCheck : checks) {
    auto check = mlir::dyn_cast<mlir::DictionaryAttr>(rawCheck);
    auto program =
        check ? check.getAs<mlir::ArrayAttr>("program") : mlir::ArrayAttr();
    auto result =
        check ? check.getAs<mlir::IntegerAttr>("result") : mlir::IntegerAttr();
    auto target =
        check ? check.getAs<mlir::StringAttr>("target") : mlir::StringAttr();
    auto concreteType =
        check ? check.getAs<mlir::TypeAttr>("type") : mlir::TypeAttr();
    if (!check || (check.size() != 3 && check.size() != 4) || !program ||
        program.empty() || !result || !result.getType().isSignlessInteger(64) ||
        !target || (check.size() == 4) != static_cast<bool>(concreteType))
      return module.emitError("static type checks require exact program, "
                              "result, and target fields");
    if (!targets.insert(target.getValue()).second)
      return module.emitError("static type check targets must be unique");

    llvm::SmallVector<int64_t> stack;
    for (mlir::Attribute rawToken : program) {
      auto tokenAttr = mlir::dyn_cast<mlir::StringAttr>(rawToken);
      if (!tokenAttr)
        return module.emitError(
            "static type check program tokens must be strings");
      llvm::StringRef token = tokenAttr.getValue();
      if (llvm::StringRef name =
              token.consume_front("param:") ? token : llvm::StringRef();
          !name.empty()) {
        auto binding = bindings.find(name);
        if (binding == bindings.end())
          return module.emitError()
                 << "unbound static type parameter '" << name << "'";
        referencedBindings.insert(name);
        stack.push_back(binding->getValue());
        continue;
      }
      if (token.consume_front("literal:")) {
        int64_t literal = 0;
        if (token.getAsInteger(10, literal))
          return module.emitError("invalid static type literal token");
        stack.push_back(literal);
        continue;
      }
      auto unary = [&](auto function) -> mlir::LogicalResult {
        if (stack.empty())
          return module.emitError("static type expression stack underflow");
        int64_t value = stack.pop_back_val();
        auto evaluated = function(value);
        if (!evaluated)
          return module.emitError("invalid static type unary expression");
        stack.push_back(*evaluated);
        return mlir::success();
      };
      auto binary = [&](auto function) -> mlir::LogicalResult {
        if (stack.size() < 2)
          return module.emitError("static type expression stack underflow");
        int64_t right = stack.pop_back_val();
        int64_t left = stack.pop_back_val();
        int64_t evaluated = 0;
        if (function(left, right, evaluated))
          return module.emitError("static type integer arithmetic overflow");
        stack.push_back(evaluated);
        return mlir::success();
      };
      if (token == "index_width") {
        if (mlir::failed(unary([](int64_t value) -> std::optional<int64_t> {
              if (value <= 0)
                return std::nullopt;
              return std::max<int64_t>(1, llvm::Log2_64_Ceil(value));
            })))
          return mlir::failure();
      } else if (token == "count_width") {
        if (mlir::failed(unary([](int64_t value) -> std::optional<int64_t> {
              if (value <= 0)
                return std::nullopt;
              return llvm::Log2_64(static_cast<uint64_t>(value)) + 1;
            })))
          return mlir::failure();
      } else if (token == "add") {
        if (mlir::failed(binary(llvm::AddOverflow<int64_t>)))
          return mlir::failure();
      } else if (token == "sub") {
        if (mlir::failed(binary(llvm::SubOverflow<int64_t>)))
          return mlir::failure();
      } else if (token == "mul") {
        if (mlir::failed(binary(llvm::MulOverflow<int64_t>)))
          return mlir::failure();
      } else {
        return module.emitError()
               << "unsupported static type token '" << token << "'";
      }
    }
    if (stack.size() != 1 || stack.front() != result.getInt())
      return module.emitError("static type expression result is inconsistent");

    auto [path, kind] = target.getValue().rsplit(':');
    mlir::Type fieldType;
    llvm::SmallVector<llvm::StringRef> segments;
    if (concreteType) {
      fieldType = resolveInterfaceType(path);
      if (!fieldType || fieldType != concreteType.getValue())
        return module.emitError(
            "static interface type check does not match the actual boundary");
    } else {
      path.split(segments, '.');
      if (segments.size() < 2)
        return module.emitError("static type check target is unresolved");
      llvm::StringRef structName = segments[0];
      llvm::StringRef fieldName = segments[1];
      auto declaration = mlir::dyn_cast_or_null<ac::StructOp>(
          mlir::SymbolTable::lookupSymbolIn(typeScope, structName));
      if (!declaration || fieldName.empty() || kind.empty())
        return module.emitError("static type check target is unresolved");
      for (mlir::Attribute rawField : declaration.getFields()) {
        auto field = mlir::cast<mlir::DictionaryAttr>(rawField);
        if (field.getAs<mlir::StringAttr>("name").getValue() == fieldName) {
          fieldType = field.getAs<mlir::TypeAttr>("type").getValue();
          break;
        }
      }
    }
    if (!fieldType)
      return module.emitError("static type check field is unresolved");
    for (llvm::StringRef segment :
         concreteType
             ? llvm::ArrayRef<llvm::StringRef>()
             : llvm::ArrayRef<llvm::StringRef>(segments).drop_front(2)) {
      if (segment == "array_element") {
        auto array = mlir::dyn_cast<ac::ValueArrayType>(fieldType);
        if (!array)
          return module.emitError(
              "static type check array-element path is unresolved");
        fieldType = array.getElementType();
        continue;
      }
      llvm::StringRef ordinal = segment;
      if (!ordinal.consume_front("tuple_"))
        return module.emitError(
            "static type check path component is unsupported");
      unsigned index = 0;
      auto tuple = mlir::dyn_cast<mlir::TupleType>(fieldType);
      if (!tuple || ordinal.getAsInteger(10, index) || index >= tuple.size())
        return module.emitError(
            "static type check tuple-element path is unresolved");
      fieldType = tuple.getType(index);
    }
    if (kind == "bits") {
      auto integer = mlir::dyn_cast<mlir::IntegerType>(fieldType);
      if (!integer || integer.getWidth() != result.getInt())
        return module.emitError(
            "static bits width does not match resolved result");
    } else if (kind == "array_length") {
      auto array = mlir::dyn_cast<ac::ValueArrayType>(fieldType);
      if (!array || array.getLength() != result.getInt())
        return module.emitError(
            "static value-array length does not match resolved result");
    } else {
      return module.emitError("static type check target kind is unsupported");
    }
  }
  for (const auto &binding : bindings)
    if (!referencedBindings.contains(binding.getKey()))
      return module.emitError() << "static type parameter '" << binding.getKey()
                                << "' is not referenced by any type check";

  llvm::StringSet<> identitySymbols;
  llvm::StringSet<> identityTargets;
  if (identities) {
    for (mlir::Attribute rawIdentity : identities) {
      auto identity = mlir::dyn_cast<mlir::DictionaryAttr>(rawIdentity);
      auto source = identity ? identity.getAs<mlir::StringAttr>("source")
                             : mlir::StringAttr();
      auto symbol = identity ? identity.getAs<mlir::StringAttr>("symbol")
                             : mlir::StringAttr();
      auto fingerprint = identity
                             ? identity.getAs<mlir::StringAttr>("fingerprint")
                             : mlir::StringAttr();
      auto identityBindings = identity
                                  ? identity.getAs<mlir::ArrayAttr>("bindings")
                                  : mlir::ArrayAttr();
      auto identityCheckTargets =
          identity ? identity.getAs<mlir::ArrayAttr>("targets")
                   : mlir::ArrayAttr();
      if (!identity || identity.size() != 5 || !source || !symbol ||
          !fingerprint || !identityBindings || !identityCheckTargets ||
          source.getValue().empty() || symbol.getValue().empty() ||
          !identitySymbols.insert(symbol.getValue()).second)
        return module.emitError("static type identity metadata is malformed");
      auto structure = mlir::dyn_cast_or_null<ac::StructOp>(
          mlir::SymbolTable::lookupSymbolIn(typeScope, symbol.getValue()));
      if (!structure)
        return module.emitError("static type identity symbol is unresolved");

      llvm::StringSet<> identityBindingNames;
      llvm::StringRef previousName;
      for (mlir::Attribute rawBinding : identityBindings) {
        auto binding = mlir::dyn_cast<mlir::DictionaryAttr>(rawBinding);
        auto name = binding ? binding.getAs<mlir::StringAttr>("name")
                            : mlir::StringAttr();
        auto parameter = binding ? binding.getAs<mlir::StringAttr>("parameter")
                                 : mlir::StringAttr();
        auto value = binding ? binding.getAs<mlir::IntegerAttr>("value")
                             : mlir::IntegerAttr();
        if (!binding || binding.size() != 3 || !name || !parameter || !value ||
            !value.getType().isSignlessInteger(64) || name.getValue().empty() ||
            parameter.getValue().empty() ||
            !identityBindingNames.insert(name.getValue()).second ||
            (!previousName.empty() && previousName >= name.getValue()))
          return module.emitError(
              "static type identity bindings must be canonical");
        previousName = name.getValue();
        auto global = bindings.find(parameter.getValue());
        if (global == bindings.end() || global->getValue() != value.getInt())
          return module.emitError(
              "static type identity binding disagrees with global bindings");
      }
      const std::string targetPrefix = symbol.getValue().str() + ".";
      for (mlir::Attribute rawTarget : identityCheckTargets) {
        auto target = mlir::dyn_cast<mlir::StringAttr>(rawTarget);
        if (!target || !targets.contains(target.getValue()) ||
            !identityTargets.insert(target.getValue()).second ||
            !target.getValue().starts_with(targetPrefix))
          return module.emitError(
              "static type identity targets must exactly reference checks");
      }

      const std::string expected = staticStructFingerprint(
          structure, source.getValue(), identityBindings);
      if (fingerprint.getValue() != expected)
        return module.emitError(
            "static type identity fingerprint is inconsistent");
      const std::string expectedSymbol =
          source.getValue().str() + "__p" + expected.substr(7, 12);
      if (symbol.getValue() != expectedSymbol)
        return module.emitError("static type identity symbol is inconsistent");
    }
  }

  if (typeScope)
    for (ac::StructOp structure :
         typeScope.getBody().front().getOps<ac::StructOp>())
      if (structure.getSymName().contains("__p") &&
          !identitySymbols.contains(structure.getSymName()))
        return module.emitError(
            "specialized struct declaration requires static type identity");
  for (llvm::StringRef target : targets.keys()) {
    auto [path, kind] = target.rsplit(':');
    (void)kind;
    auto [symbol, rest] = path.split('.');
    if (rest.empty())
      continue;
    auto structure =
        typeScope ? mlir::dyn_cast_or_null<ac::StructOp>(
                        mlir::SymbolTable::lookupSymbolIn(typeScope, symbol))
                  : ac::StructOp();
    if (structure && structure.getSymName().contains("__p") &&
        !identityTargets.contains(target))
      return module.emitError(
          "specialized struct check is missing from static type identity");
  }
  return mlir::success();
}

class VerifyACIRFilePass final
    : public mlir::PassWrapper<VerifyACIRFilePass,
                               mlir::OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(VerifyACIRFilePass)

  llvm::StringRef getArgument() const override { return "verify-ac-file"; }
  llvm::StringRef getDescription() const override {
    return "Verify the Agentic Circuit epoch and whole-file legality";
  }

  void runOnOperation() override {
    mlir::ModuleOp module = getOperation();
    if (mlir::failed(ac::preflightRawModelStructure(module))) {
      signalPassFailure();
      return;
    }
    auto epoch = module->getAttrOfType<mlir::StringAttr>("ac.contract_epoch");
    if (!epoch || epoch.getValue() != "0.5") {
      module.emitError(
          "expected top-level 'ac.contract_epoch' string attribute equal to "
          "\"0.5\"");
      signalPassFailure();
      return;
    }
    if (mlir::failed(verifyStaticTypeMetadata(module))) {
      signalPassFailure();
      return;
    }
    if (mlir::failed(verifySourceProvenance(module))) {
      signalPassFailure();
      return;
    }
    if (mlir::failed(acsim::verifyCanonicalACSimFile(module))) {
      signalPassFailure();
      return;
    }

    mlir::WalkResult result = module.walk([&](mlir::Operation *operation) {
      if (mlir::failed(ac::verifyTopologyTypeUses(operation)))
        return mlir::WalkResult::interrupt();
      auto rejectChannel = [&](mlir::Attribute attribute) {
        return attribute && attribute
                                .walk([](ac::ChannelType) {
                                  return mlir::WalkResult::interrupt();
                                })
                                .wasInterrupted();
      };
      for (mlir::NamedAttribute attribute : operation->getAttrs()) {
        if (mlir::isa<ac::PortOp>(operation) && attribute.getName() == "type")
          continue;
        if (rejectChannel(attribute.getValue())) {
          operation->emitError("channel type is only permitted in an "
                               "ac.interface channel declaration");
          return mlir::WalkResult::interrupt();
        }
      }
      if ((!mlir::isa<ac::PortOp>(operation) &&
           rejectChannel(operation->getPropertiesAsAttribute())) ||
          rejectChannel(mlir::LocationAttr(operation->getLoc()))) {
        operation->emitError("channel type is only permitted in an "
                             "ac.interface channel declaration");
        return mlir::WalkResult::interrupt();
      }
      for (mlir::Type type : operation->getOperandTypes())
        if (ac::containsChannelType(type)) {
          operation->emitError("channel type is only permitted in an "
                               "ac.interface channel declaration");
          return mlir::WalkResult::interrupt();
        }
      for (mlir::Type type : operation->getResultTypes())
        if (ac::containsChannelType(type)) {
          operation->emitError("channel type is only permitted in an "
                               "ac.interface channel declaration");
          return mlir::WalkResult::interrupt();
        }
      for (mlir::Region &region : operation->getRegions())
        for (mlir::Block &block : region)
          for (mlir::BlockArgument argument : block.getArguments())
            if (ac::containsChannelType(argument.getType())) {
              operation->emitError("channel type is only permitted in an "
                                   "ac.interface channel declaration");
              return mlir::WalkResult::interrupt();
            }
      return mlir::WalkResult::advance();
    });
    if (result.wasInterrupted())
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createVerifyACIRFilePass() {
  return std::make_unique<VerifyACIRFilePass>();
}

} // namespace acir
