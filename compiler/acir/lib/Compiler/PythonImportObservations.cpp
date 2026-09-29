#include "PythonImportObservations.h"

#include "PythonImportContext.h"
#include "PythonImportModules.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinTypes.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {

DictionaryAttr observationItem(OpBuilder &builder, StringRef text) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("literal")),
      builder.getNamedAttr("text", builder.getStringAttr(text)),
  });
}

DictionaryAttr observationItem(OpBuilder &builder, uint32_t ordinal) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("value")),
      builder.getNamedAttr("ordinal", builder.getI32IntegerAttr(ordinal)),
  });
}

bool isFiniteObservationType(DictionaryAttr logical) {
  auto kind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
  return kind && (kind.getValue() == "bool" || kind.getValue() == "integer");
}

bool isNonnegativeU64Integer(DictionaryAttr logical) {
  auto kind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
  auto interpretation =
      logical ? logical.getAs<StringAttr>("interpretation") : StringAttr();
  auto storage = logical ? logical.getAs<TypeAttr>("storage") : TypeAttr();
  auto integer =
      storage ? dyn_cast<IntegerType>(storage.getValue()) : IntegerType();
  auto lower =
      logical ? logical.getAs<ac::MathIntAttr>("lower") : ac::MathIntAttr();
  return kind && kind.getValue() == "integer" && interpretation &&
         interpretation.getValue() == "unsigned" && integer &&
         integer.getWidth() <= 64 && lower &&
         !llvm::APSInt(lower.getCanonicalValue()).isNegative();
}

} // namespace

PythonImportObservationProducer::PythonImportObservationProducer(
    OpBuilder &builder, StringRef sourcePath, FlatSymbolRefAttr moduleSymbol,
    const AstNode &moduleDeclaration, DictionaryAttr registration,
    ac::detail::EmitError emitError, const llvm::StringMap<Value> &localValues,
    const llvm::StringMap<Value> &entryValues,
    const llvm::StringMap<DictionaryAttr> &valueTypes)
    : builder(builder), sourcePath(sourcePath), moduleSymbol(moduleSymbol),
      moduleDeclaration(moduleDeclaration), registration(registration),
      emitError(emitError), localValues(localValues), entryValues(entryValues),
      valueTypes(valueTypes) {}

FailureOr<StringAttr>
PythonImportObservationProducer::staticString(const AstNode &node,
                                              StringRef role) {
  auto value = dyn_cast_or_null<StringAttr>(node.get("value"));
  if (node.kind() != "Constant" || !value)
    return emitError() << role
                       << " observation argument must be a static string";
  return value;
}

FailureOr<Value> PythonImportObservationProducer::dynamicValue(
    const AstNode &node, SmallVectorImpl<Attribute> &valueIDs,
    SmallVectorImpl<Attribute> &constraints) {
  if (node.kind() == "Constant") {
    auto encoded = dyn_cast_or_null<DictionaryAttr>(node.get("value"));
    auto spelling =
        encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
    auto boolean = dyn_cast_or_null<BoolAttr>(node.get("value"));
    if (!spelling && !boolean)
      return emitError()
             << "observation literal must be finite integer or bool";
    DictionaryAttr logical;
    IntegerAttr bits;
    if (boolean) {
      logical = builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr("bool")), builder.getNamedAttr("storage", TypeAttr::get(builder.getI1Type()))});
      bits = builder.getIntegerAttr(builder.getI1Type(), boolean.getValue());
    } else {
      auto parsed = parseStaticInteger(builder, spelling.getValue(), emitError);
      if (failed(parsed))
        return failure();
      llvm::APSInt value((*parsed).getCanonicalValue());
      if (value.isNegative() || value.getActiveBits() > 64)
        return emitError()
               << "observation literal requires a nonnegative u64 integer";
      unsigned width = std::max(1u, value.getActiveBits());
      auto type = builder.getIntegerType(width);
      llvm::APInt upper = value.zextOrTrunc(width + 1) + 1;
      logical = builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
           builder.getNamedAttr("storage", TypeAttr::get(type)),
           builder.getNamedAttr("lower", *parsed),
           builder.getNamedAttr(
               "upper", ac::MathIntAttr::get(builder.getContext(),
                                             llvm::APSInt(upper, true))),
           builder.getNamedAttr("interpretation",
                                builder.getStringAttr("unsigned"))});
      bits = builder.getIntegerAttr(type, value.zextOrTrunc(width));
    }
    auto origin = occurrence(builder, moduleSymbol,
                             relativeToModule(moduleDeclaration, node));
    auto value = arith::ConstantOp::create(
        builder, node.location(builder.getContext(), sourcePath), bits);
    value->setAttr("ac.origin", origin);
    valueIDs.push_back(builder.getDictionaryAttr(
        {builder.getNamedAttr("origin", origin),
         builder.getNamedAttr("slot", builder.getI32IntegerAttr(0))}));
    constraints.push_back(valueConstraint(builder, logical));
    return value.getResult();
  }
  if (node.kind() != "Name")
    return emitError()
           << "observation value must be a static string or direct persistent "
              "bool/integer name";
  StringRef name = node.string("id");
  if (localValues.contains(name))
    return emitError()
           << "observation value cannot use a local or derived value";
  auto entry = entryValues.find(name);
  auto logical = valueTypes.find(name);
  if (entry == entryValues.end() || logical == valueTypes.end() ||
      !isFiniteObservationType(logical->second))
    return emitError()
           << "observation value must be a direct persistent finite "
              "bool/integer read";

  DictionaryAttr origin = occurrence(builder, moduleSymbol,
                                     relativeToModule(moduleDeclaration, node));
  Operation *read = createSourceOperation(
      builder, node.location(builder.getContext(), sourcePath),
      ac::SourceReadOp::getOperationName(), ValueRange{entry->second},
      TypeRange{entry->second.getType()},
      {builder.getNamedAttr("ac.origin", origin)});
  valueIDs.push_back(builder.getDictionaryAttr({
      builder.getNamedAttr("origin", origin),
      builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
  }));
  constraints.push_back(valueConstraint(builder, logical->second));
  return read->getResult(0);
}

FailureOr<bool> PythonImportObservationProducer::emit(
    const AstNode &statement, StringRef intrinsic, Value path,
    SmallVectorImpl<Attribute> &requiredObservations) {
  if (intrinsic.empty())
    return false;
  AstNode call = statement.child("value");
  if (statement.kind() != "Expr" || call.kind() != "Call" || !path)
    return emitError()
           << "print/log/report observation must be an unconditional call";
  if (call.child("func").kind() != "Name")
    return emitError()
           << "print/log/report observation callee must be a direct name";

  ArrayAttr arguments = call.array("args");
  ArrayAttr keywords = call.array("keywords");
  SmallVector<Attribute> items;
  SmallVector<Value> values;
  SmallVector<Attribute> valueIDs;
  SmallVector<Attribute> constraints;
  DictionaryAttr spec;

  auto appendItem = [&](const AstNode &item) -> LogicalResult {
    if (auto text = dyn_cast_or_null<StringAttr>(item.get("value"));
        item.kind() == "Constant" && text) {
      items.push_back(observationItem(builder, text.getValue()));
      return success();
    }
    auto value = dynamicValue(item, valueIDs, constraints);
    if (failed(value))
      return failure();
    items.push_back(
        observationItem(builder, static_cast<uint32_t>(values.size())));
    values.push_back(*value);
    return success();
  };

  if (intrinsic == "print") {
    StringAttr separator = builder.getStringAttr(" ");
    StringAttr ending = builder.getStringAttr("\n");
    llvm::StringSet<> seenKeywords;
    for (size_t index = 0; index < keywords.size(); ++index) {
      AstNode keyword = call.item("keywords", index);
      Attribute rawName = keyword.get("arg");
      auto name = dyn_cast_or_null<StringAttr>(rawName);
      if (!name || (name.getValue() != "sep" && name.getValue() != "end") ||
          !seenKeywords.insert(name.getValue()).second)
        return emitError()
               << "print observation permits unique static sep/end keywords; "
                  "file, flush and **kwargs are unsupported";
      auto value = staticString(keyword.child("value"),
                                (Twine("print ") + name.getValue()).str());
      if (failed(value))
        return failure();
      if (name.getValue() == "sep")
        separator = *value;
      else
        ending = *value;
    }
    for (size_t index = 0; index < arguments.size(); ++index)
      if (failed(appendItem(call.item("args", index))))
        return failure();
    spec = builder.getDictionaryAttr({
        builder.getNamedAttr("items", builder.getArrayAttr(items)),
        builder.getNamedAttr("sep", separator),
        builder.getNamedAttr("end", ending),
    });
  } else if (intrinsic == "log") {
    if (!keywords.empty() || arguments.size() < 2)
      return emitError()
             << "log observation requires level, event and optional items "
                "without keywords";
    auto level = staticString(call.item("args", 0), "log level");
    auto event = staticString(call.item("args", 1), "log event");
    if (failed(level) || failed(event))
      return failure();
    if ((*event).getValue().empty() ||
        !llvm::is_contained(
            ArrayRef<StringRef>{"debug", "info", "warning", "error"},
            (*level).getValue()))
      return emitError()
             << "log observation requires debug/info/warning/error level and "
                "a nonempty static event";
    for (size_t index = 2; index < arguments.size(); ++index)
      if (failed(appendItem(call.item("args", index))))
        return failure();
    spec = builder.getDictionaryAttr({
        builder.getNamedAttr("level", *level),
        builder.getNamedAttr("event", *event),
        builder.getNamedAttr("items", builder.getArrayAttr(items)),
    });
  } else if (intrinsic == "report") {
    if (!keywords.empty() || arguments.size() != 2)
      return emitError()
             << "report observation requires exactly static name and value";
    auto name = staticString(call.item("args", 0), "report name");
    if (failed(name))
      return failure();
    if ((*name).getValue().empty())
      return emitError() << "report observation name must be nonempty";
    AstNode valueNode = call.item("args", 1);
    auto value = dynamicValue(valueNode, valueIDs, constraints);
    if (failed(value))
      return failure();
    auto logical =
        cast<DictionaryAttr>(constraints.back()).getAs<DictionaryAttr>("type");
    if (!isNonnegativeU64Integer(logical))
      return emitError() << "report observation value must be an exact "
                            "nonnegative u64-capable logical integer";
    values.push_back(*value);
    spec = builder.getDictionaryAttr({builder.getNamedAttr("name", *name)});
  } else {
    return emitError() << "observation intrinsic must be print, log or report";
  }

  DictionaryAttr site = occurrence(builder, moduleSymbol,
                                   relativeToModule(moduleDeclaration, call));
  DictionaryAttr identity = builder.getDictionaryAttr({
      builder.getNamedAttr("registration", registration),
      builder.getNamedAttr("site", site),
  });
  SmallVector<Value> operands{path};
  llvm::append_range(operands, values);
  createSourceOperation(
      builder, call.location(builder.getContext(), sourcePath),
      ac::SourceObserveOp::getOperationName(), operands, {},
      {builder.getNamedAttr("kind", builder.getStringAttr(intrinsic)),
       builder.getNamedAttr("ac.observation_id", identity),
       builder.getNamedAttr("ac.value_ids", builder.getArrayAttr(valueIDs)),
       builder.getNamedAttr("ac.value_constraints",
                            builder.getArrayAttr(constraints)),
       builder.getNamedAttr("spec", spec)});
  requiredObservations.push_back(builder.getDictionaryAttr({
      builder.getNamedAttr("id", identity),
      builder.getNamedAttr("kind", builder.getStringAttr(intrinsic)),
      builder.getNamedAttr("spec", spec),
      builder.getNamedAttr("values", builder.getArrayAttr(valueIDs)),
  }));
  return true;
}

} // namespace acir::compiler::detail
