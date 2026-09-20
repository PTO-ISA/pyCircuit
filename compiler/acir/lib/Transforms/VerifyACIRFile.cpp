#include "acir/Transforms/Passes.h"

#include "Dialect/ACIR/ProcessLowerability.h"
#include "acir/Bindings/Binding.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/ACIRResources.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/JSON.h"
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

std::string printType(mlir::Type type) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  type.print(stream);
  return stream.str();
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
             kind.getValue() != "instance") ||
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

mlir::LogicalResult verifyRemovedStaticMetadataAbsentImpl(mlir::ModuleOp module) {
  for (mlir::NamedAttribute attribute : module->getAttrs())
    if (attribute.getName().strref().starts_with("ac.static_"))
      return module.emitError(
          "removed untyped static metadata is forbidden; use typed family attributes");
  mlir::WalkResult result = module.walk([&](mlir::Operation *operation) {
    for (mlir::NamedAttribute attribute : operation->getAttrs())
      if (attribute.getName().strref().starts_with("ac.static_type_")) {
        operation->emitError(
            "removed static type metadata is forbidden; use typed family attributes");
        return mlir::WalkResult::interrupt();
      }
    return mlir::WalkResult::advance();
  });
  return result.wasInterrupted() ? mlir::failure() : mlir::success();
}

class VerifyACIRFilePass final
    : public mlir::PassWrapper<VerifyACIRFilePass,
                               mlir::OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(VerifyACIRFilePass)

  llvm::StringRef getArgument() const override { return "verify-ac-file"; }
  llvm::StringRef getDescription() const override {
    return "Verify Agentic Circuit whole-file legality";
  }

  void runOnOperation() override {
    mlir::ModuleOp module = getOperation();
    if (mlir::failed(ac::preflightRawModelStructure(module))) {
      signalPassFailure();
      return;
    }
    if (mlir::failed(acir::verifyRemovedStaticMetadataAbsent(module))) {
      signalPassFailure();
      return;
    }
    if (mlir::failed(verifySourceProvenance(module))) {
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

mlir::LogicalResult verifyRemovedStaticMetadataAbsent(mlir::ModuleOp module) {
  return verifyRemovedStaticMetadataAbsentImpl(module);
}

std::unique_ptr<mlir::Pass> createVerifyACIRFilePass() {
  return std::make_unique<VerifyACIRFilePass>();
}

} // namespace acir
