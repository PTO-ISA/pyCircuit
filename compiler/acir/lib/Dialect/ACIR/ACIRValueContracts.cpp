#include "ACIRSourceContracts.h"

#include "mlir/IR/BuiltinTypes.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"

#include <optional>
#include <tuple>
#include <utility>

using namespace mlir;

namespace acir::ac::detail {
namespace {

FailureOr<StringRef> recordKind(DictionaryAttr value, StringRef recordName,
                                EmitError emitError) {
  if (!value)
    return emitError() << recordName << " must be a DictionaryAttr";
  auto kind = value.getAs<StringAttr>("kind");
  if (!kind)
    return emitError() << recordName << " field 'kind' must be a StringAttr";
  return kind.getValue();
}

LogicalResult requireExactFields(DictionaryAttr value, size_t count,
                                 StringRef recordName, EmitError emitError) {
  if (!value || value.size() != count)
    return emitError() << recordName << " must contain exactly " << count
                       << " fields";
  return success();
}

llvm::APSInt mathValue(MathIntAttr value) {
  return llvm::APSInt(value.getCanonicalValue());
}

int compareMath(const llvm::APSInt &left, const llvm::APSInt &right) {
  return llvm::APSInt::compareValues(left, right);
}

llvm::APSInt positivePowerOfTwo(unsigned exponent) {
  llvm::APInt bits(exponent + 2, 0);
  bits.setBit(exponent);
  return llvm::APSInt(std::move(bits), /*isUnsigned=*/true);
}

llvm::APSInt negativePowerOfTwo(unsigned exponent) {
  llvm::APInt bits(exponent + 2, 0);
  bits.setBit(exponent);
  bits = -bits;
  return llvm::APSInt(std::move(bits), /*isUnsigned=*/false);
}

FailureOr<unsigned> requiredLogicalWidth(const llvm::APSInt &lower,
                                         const llvm::APSInt &upper,
                                         StringRef interpretation,
                                         EmitError emitError) {
  const llvm::APSInt zero("0");
  bool nonNegative = compareMath(lower, zero) >= 0;
  StringRef requiredInterpretation = nonNegative ? "unsigned" : "signed";
  if (interpretation != requiredInterpretation)
    return emitError() << "LogicalType integer interpretation must be '"
                       << requiredInterpretation << "' for its bounds";

  for (unsigned width = 1; width <= 64; ++width) {
    llvm::APSInt limit = positivePowerOfTwo(nonNegative ? width : width - 1);
    if (compareMath(upper, limit) > 0)
      continue;
    if (!nonNegative) {
      llvm::APSInt minimum = negativePowerOfTwo(width - 1);
      if (compareMath(lower, minimum) < 0)
        continue;
    }
    return width;
  }
  return emitError()
         << "LogicalType integer bounds require storage wider than 64 bits";
}

LogicalResult verifyBounds(MathIntAttr lowerAttr, MathIntAttr upperAttr,
                           StringRef recordName, EmitError emitError) {
  if (!lowerAttr || !upperAttr)
    return emitError() << recordName
                       << " bounds must both be MathInt attributes";
  llvm::APSInt lower = mathValue(lowerAttr);
  llvm::APSInt upper = mathValue(upperAttr);
  if (compareMath(lower, upper) >= 0)
    return emitError() << recordName
                       << " requires lower < upper mathematical bounds";
  return success();
}

LogicalResult verifyLogicalInteger(DictionaryAttr value, EmitError emitError) {
  if (failed(requireExactFields(value, 5, "LogicalType integer", emitError)))
    return failure();
  auto storageAttr = value.getAs<TypeAttr>("storage");
  auto lowerAttr = value.getAs<MathIntAttr>("lower");
  auto upperAttr = value.getAs<MathIntAttr>("upper");
  auto interpretation = value.getAs<StringAttr>("interpretation");
  auto storage = storageAttr ? dyn_cast<IntegerType>(storageAttr.getValue())
                             : IntegerType();
  if (!storage || !storage.isSignless() || storage.getWidth() == 0 ||
      storage.getWidth() > 64)
    return emitError() << "LogicalType integer storage must be signless iN for "
                          "N in [1, 64]";
  if (failed(
          verifyBounds(lowerAttr, upperAttr, "LogicalType integer", emitError)))
    return failure();
  if (!interpretation)
    return emitError()
           << "LogicalType integer interpretation must be a StringAttr";
  auto required =
      requiredLogicalWidth(mathValue(lowerAttr), mathValue(upperAttr),
                           interpretation.getValue(), emitError);
  if (failed(required))
    return failure();
  if (storage.getWidth() != *required)
    return emitError() << "LogicalType integer storage width must be minimal: i"
                       << *required;
  return success();
}

LogicalResult verifyLogicalTypeStructureImpl(DictionaryAttr value,
                                             EmitError emitError) {
  auto kind = recordKind(value, "LogicalType", emitError);
  if (failed(kind))
    return failure();
  if (*kind == "bool") {
    if (failed(requireExactFields(value, 2, "LogicalType bool", emitError)))
      return failure();
    auto storageAttr = value.getAs<TypeAttr>("storage");
    auto storage = storageAttr ? dyn_cast<IntegerType>(storageAttr.getValue())
                               : IntegerType();
    if (!storage || !storage.isSignless() || storage.getWidth() != 1)
      return emitError() << "LogicalType bool storage must be signless i1";
    return success();
  }
  if (*kind == "integer")
    return verifyLogicalInteger(value, emitError);
  if (*kind == "record") {
    if (failed(requireExactFields(value, 2, "LogicalType record", emitError)))
      return failure();
    if (!value.getAs<FlatSymbolRefAttr>("symbol"))
      return emitError()
             << "LogicalType record symbol must be a FlatSymbolRefAttr";
    return success();
  }
  if (*kind == "list") {
    if (failed(requireExactFields(value, 3, "LogicalType list", emitError)))
      return failure();
    auto element = value.getAs<DictionaryAttr>("element");
    auto length = decodeU64(value.getAs<IntegerAttr>("length"),
                            "LogicalType list length", emitError);
    if (!element)
      return emitError() << "LogicalType list element must be a DictionaryAttr";
    if (failed(length) || *length == 0)
      return failed(length)
                 ? failure()
                 : emitError() << "LogicalType list length must be positive";
    if (failed(verifyLogicalTypeStructureImpl(element, emitError))) {
      emitError() << "LogicalType list element is invalid";
      return failure();
    }
    auto elementKind = element.getAs<StringAttr>("kind");
    if (elementKind && elementKind.getValue() == "list")
      return emitError() << "LogicalType list element cannot be another list";
    return success();
  }
  return emitError() << "LogicalType has unknown kind '" << *kind << "'";
}

LogicalResult verifyStaticTypeStructureImpl(DictionaryAttr value,
                                            EmitError emitError) {
  auto kind = recordKind(value, "StaticType", emitError);
  if (failed(kind))
    return failure();
  if (*kind == "bool")
    return requireExactFields(value, 1, "StaticType bool", emitError);
  if (*kind == "integer") {
    if (value.size() == 1)
      return success();
    if (failed(requireExactFields(value, 3, "StaticType integer", emitError)))
      return failure();
    return verifyBounds(value.getAs<MathIntAttr>("lower"),
                        value.getAs<MathIntAttr>("upper"), "StaticType integer",
                        emitError);
  }
  if (*kind == "record") {
    if (failed(requireExactFields(value, 2, "StaticType record", emitError)))
      return failure();
    if (!value.getAs<FlatSymbolRefAttr>("symbol"))
      return emitError()
             << "StaticType record symbol must be a FlatSymbolRefAttr";
    return success();
  }
  if (*kind == "list") {
    if (failed(requireExactFields(value, 3, "StaticType list", emitError)))
      return failure();
    auto element = value.getAs<DictionaryAttr>("element");
    auto length = decodeU64(value.getAs<IntegerAttr>("length"),
                            "StaticType list length", emitError);
    if (!element)
      return emitError() << "StaticType list element must be a DictionaryAttr";
    if (failed(length) || *length == 0)
      return failed(length)
                 ? failure()
                 : emitError() << "StaticType list length must be positive";
    if (failed(verifyStaticTypeStructureImpl(element, emitError))) {
      emitError() << "StaticType list element is invalid";
      return failure();
    }
    return success();
  }
  return emitError() << "StaticType has unknown kind '" << *kind << "'";
}

LogicalResult verifyStaticValueStructureImpl(DictionaryAttr value,
                                             EmitError emitError) {
  auto kind = recordKind(value, "StaticValue", emitError);
  if (failed(kind))
    return failure();
  if (*kind == "bool") {
    if (failed(requireExactFields(value, 2, "StaticValue bool", emitError)))
      return failure();
    return value.getAs<BoolAttr>("value")
               ? success()
               : emitError() << "StaticValue bool value must be a BoolAttr";
  }
  if (*kind == "integer") {
    if (failed(requireExactFields(value, 2, "StaticValue integer", emitError)))
      return failure();
    return value.getAs<MathIntAttr>("value")
               ? success()
               : emitError()
                     << "StaticValue integer value must be a MathIntAttr";
  }
  if (*kind == "record") {
    if (failed(requireExactFields(value, 3, "StaticValue record", emitError)))
      return failure();
    if (!value.getAs<FlatSymbolRefAttr>("symbol"))
      return emitError()
             << "StaticValue record symbol must be a FlatSymbolRefAttr";
    auto fields = value.getAs<ArrayAttr>("fields");
    if (!fields)
      return emitError() << "StaticValue record fields must be an ArrayAttr";
    for (auto [index, field] : llvm::enumerate(fields)) {
      auto record = dyn_cast<DictionaryAttr>(field);
      if (!record)
        return emitError() << "StaticValue record field[" << index
                           << "] must be a DictionaryAttr";
      if (failed(verifyStaticValueStructureImpl(record, emitError))) {
        emitError() << "StaticValue record field[" << index << "] is invalid";
        return failure();
      }
    }
    return success();
  }
  if (*kind == "list") {
    if (failed(requireExactFields(value, 2, "StaticValue list", emitError)))
      return failure();
    auto values = value.getAs<ArrayAttr>("values");
    if (!values || values.empty())
      return emitError()
             << "StaticValue list values must be a non-empty ArrayAttr";
    for (auto [index, element] : llvm::enumerate(values)) {
      auto record = dyn_cast<DictionaryAttr>(element);
      if (!record)
        return emitError() << "StaticValue list element[" << index
                           << "] must be a DictionaryAttr";
      if (failed(verifyStaticValueStructureImpl(record, emitError))) {
        emitError() << "StaticValue list element[" << index << "] is invalid";
        return failure();
      }
    }
    return success();
  }
  return emitError() << "StaticValue has unknown kind '" << *kind << "'";
}

LogicalResult verifyDefaultStructureImpl(DictionaryAttr value,
                                         EmitError emitError) {
  if (!value)
    return emitError() << "Default must be a DictionaryAttr";
  auto present = value.getAs<BoolAttr>("present");
  if (!present)
    return emitError() << "Default field 'present' must be a BoolAttr";
  if (!present.getValue())
    return requireExactFields(value, 1, "absent Default", emitError);
  if (failed(requireExactFields(value, 2, "present Default", emitError)))
    return failure();
  auto staticValue = value.getAs<DictionaryAttr>("value");
  if (!staticValue)
    return emitError() << "present Default value must be a DictionaryAttr";
  if (failed(verifyStaticValueStructureImpl(staticValue, emitError))) {
    emitError() << "present Default contains an invalid StaticValue";
    return failure();
  }
  return success();
}

struct ResolutionState {
  ResolutionState(RecordResolver resolver, EmitError emitError)
      : resolver(resolver), emitError(emitError) {}

  RecordResolver resolver;
  EmitError emitError;
  llvm::DenseSet<Attribute> active;
  llvm::DenseMap<Attribute, ResolvedRecordView> resolved;

  FailureOr<const ResolvedRecordView *>
  resolveRecord(FlatSymbolRefAttr symbol) {
    if (auto found = resolved.find(symbol); found != resolved.end())
      return &found->second;
    if (!active.insert(symbol).second)
      return emitError() << "nominal record resolution cycle at " << symbol;
    llvm::scope_exit leave([&] { active.erase(symbol); });
    if (!resolver)
      return emitError() << "nominal record resolver is unavailable for "
                         << symbol;
    auto view = resolver(symbol);
    if (failed(view))
      return emitError() << "failed to resolve nominal record " << symbol;
    if (!view->symbol || view->symbol != symbol)
      return emitError() << "nominal resolver returned a mismatched symbol for "
                         << symbol;
    for (auto [index, fieldType] : llvm::enumerate(view->fieldLogicalTypes)) {
      if (failed(verifyLogicalTypeStructureImpl(fieldType, emitError)) ||
          failed(resolveType(fieldType, ExpectedTypeKind::Logical))) {
        emitError() << "nominal record " << symbol << " field[" << index
                    << "] has an invalid LogicalType";
        return failure();
      }
    }
    auto [position, inserted] = resolved.try_emplace(symbol, std::move(*view));
    (void)inserted;
    return &position->second;
  }

  LogicalResult resolveType(DictionaryAttr type, ExpectedTypeKind kind) {
    LogicalResult structure =
        kind == ExpectedTypeKind::Logical
            ? verifyLogicalTypeStructureImpl(type, emitError)
            : verifyStaticTypeStructureImpl(type, emitError);
    if (failed(structure))
      return failure();
    StringRef typeKind = type.getAs<StringAttr>("kind").getValue();
    if (typeKind == "record")
      return succeeded(resolveRecord(type.getAs<FlatSymbolRefAttr>("symbol")))
                 ? success()
                 : failure();
    if (typeKind == "list") {
      auto element = type.getAs<DictionaryAttr>("element");
      if (failed(resolveType(element, kind))) {
        emitError() << (kind == ExpectedTypeKind::Logical ? "LogicalType"
                                                          : "StaticType")
                    << " list element failed nominal resolution";
        return failure();
      }
    }
    return success();
  }
};

struct MatchState {
  MatchState(RecordResolver resolver, EmitError emitError)
      : resolution(resolver, emitError) {}

  ResolutionState resolution;

  LogicalResult matchInteger(MathIntAttr value, DictionaryAttr type,
                             ExpectedTypeKind kind) {
    if (kind == ExpectedTypeKind::Static && type.size() == 1)
      return success();
    llvm::APSInt number = mathValue(value);
    llvm::APSInt lower = mathValue(type.getAs<MathIntAttr>("lower"));
    llvm::APSInt upper = mathValue(type.getAs<MathIntAttr>("upper"));
    if (compareMath(number, lower) < 0 || compareMath(number, upper) >= 0)
      return resolution.emitError()
             << "StaticValue integer is outside expected [lower, upper) bounds";
    return success();
  }

  LogicalResult match(DictionaryAttr value, DictionaryAttr type,
                      ExpectedTypeKind kind) {
    if (failed(resolution.resolveType(type, kind)))
      return failure();
    StringRef valueKind = value.getAs<StringAttr>("kind").getValue();
    StringRef typeKind = type.getAs<StringAttr>("kind").getValue();
    if (valueKind != typeKind)
      return resolution.emitError()
             << "StaticValue kind '" << valueKind
             << "' does not match expected type kind '" << typeKind << "'";
    if (typeKind == "bool")
      return success();
    if (typeKind == "integer")
      return matchInteger(value.getAs<MathIntAttr>("value"), type, kind);
    if (typeKind == "record") {
      auto expectedSymbol = type.getAs<FlatSymbolRefAttr>("symbol");
      auto valueSymbol = value.getAs<FlatSymbolRefAttr>("symbol");
      if (valueSymbol != expectedSymbol)
        return resolution.emitError()
               << "StaticValue record nominal symbol does not match expected "
               << expectedSymbol;
      auto view = resolution.resolveRecord(expectedSymbol);
      if (failed(view))
        return failure();
      auto fields = value.getAs<ArrayAttr>("fields");
      if (fields.size() != (*view)->fieldLogicalTypes.size())
        return resolution.emitError()
               << "StaticValue record field arity does not match nominal "
               << expectedSymbol;
      for (auto [index, pair] :
           llvm::enumerate(llvm::zip(fields, (*view)->fieldLogicalTypes))) {
        auto fieldValue = cast<DictionaryAttr>(std::get<0>(pair));
        DictionaryAttr fieldType = std::get<1>(pair);
        if (failed(match(fieldValue, fieldType, ExpectedTypeKind::Logical))) {
          resolution.emitError() << "StaticValue record field[" << index
                                 << "] does not match its ordered LogicalType";
          return failure();
        }
      }
      return success();
    }
    auto values = value.getAs<ArrayAttr>("values");
    auto expectedLength =
        decodeU64(type.getAs<IntegerAttr>("length"), "expected list length",
                  resolution.emitError);
    if (failed(expectedLength))
      return failure();
    if (*expectedLength != values.size())
      return resolution.emitError()
             << "StaticValue list length does not match expected type length";
    DictionaryAttr elementType = type.getAs<DictionaryAttr>("element");
    for (auto [index, element] : llvm::enumerate(values)) {
      if (failed(match(cast<DictionaryAttr>(element), elementType, kind))) {
        resolution.emitError() << "StaticValue list element[" << index
                               << "] does not match its expected element type";
        return failure();
      }
    }
    return success();
  }
};

} // namespace

LogicalResult verifyLogicalTypeStructure(DictionaryAttr value,
                                         EmitError emitError) {
  return verifyLogicalTypeStructureImpl(value, emitError);
}

LogicalResult verifyStaticTypeStructure(DictionaryAttr value,
                                        EmitError emitError) {
  return verifyStaticTypeStructureImpl(value, emitError);
}

LogicalResult verifyStaticValueStructure(DictionaryAttr value,
                                         EmitError emitError) {
  return verifyStaticValueStructureImpl(value, emitError);
}

LogicalResult verifyDefaultStructure(DictionaryAttr value,
                                     EmitError emitError) {
  return verifyDefaultStructureImpl(value, emitError);
}

LogicalResult verifyTypeResolved(DictionaryAttr type, ExpectedTypeKind kind,
                                 RecordResolver resolver, EmitError emitError) {
  ResolutionState state(resolver, emitError);
  return state.resolveType(type, kind);
}

LogicalResult verifyStaticValueMatchesType(DictionaryAttr value,
                                           DictionaryAttr expectedType,
                                           ExpectedTypeKind kind,
                                           RecordResolver resolver,
                                           EmitError emitError) {
  if (failed(verifyStaticValueStructureImpl(value, emitError)))
    return failure();
  MatchState state(resolver, emitError);
  return state.match(value, expectedType, kind);
}

LogicalResult verifyDefaultMatchesType(DictionaryAttr defaultValue,
                                       DictionaryAttr expectedType,
                                       ExpectedTypeKind kind,
                                       RecordResolver resolver,
                                       EmitError emitError) {
  if (failed(verifyDefaultStructureImpl(defaultValue, emitError)))
    return failure();
  MatchState state(resolver, emitError);
  if (failed(state.resolution.resolveType(expectedType, kind)))
    return failure();
  auto present = defaultValue.getAs<BoolAttr>("present");
  if (!present.getValue())
    return success();
  return state.match(defaultValue.getAs<DictionaryAttr>("value"), expectedType,
                     kind);
}

} // namespace acir::ac::detail
