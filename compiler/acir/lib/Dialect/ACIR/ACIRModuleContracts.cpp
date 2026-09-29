#include "acir/Dialect/ACIR/ACIROps.h"

#include "ACIRSourceContracts.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::ac {
namespace {
LogicalResult verifyElementEffect(DictionaryAttr effect, bool isList,
                                  uint64_t expectedOrdinal,
                                  Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!effect || effect.size() != 5)
    return error() << "connection element effect must have exactly five fields";
  Attribute ordinal = effect.get("ordinal");
  if (isList) {
    auto decoded = detail::decodeU64(dyn_cast<IntegerAttr>(ordinal),
                                     "connection element ordinal", error);
    if (failed(decoded) || *decoded != expectedOrdinal)
      return error() << "connection element ordinal is out of order";
  } else if (!isa_and_nonnull<UnitAttr>(ordinal)) {
    return error() << "scalar connection element ordinal must be UnitAttr";
  }
  if (!effect.getAs<BoolAttr>("read") || !effect.getAs<BoolAttr>("write"))
    return error() << "connection element read/write must be BoolAttr";
  auto precision = effect.getAs<StringAttr>("precision");
  if (!precision || (precision.getValue() != "exact" &&
                     precision.getValue() != "conservative"))
    return error() << "connection element precision is invalid";
  auto origins = effect.getAs<ArrayAttr>("origins");
  if (!origins)
    return error() << "connection element origins must be ArrayAttr";
  llvm::DenseSet<Attribute> seen;
  Attribute previous;
  for (Attribute raw : origins) {
    auto origin = dyn_cast<DictionaryAttr>(raw);
    if (!origin || failed(detail::verifyOccurrence(origin, error)) ||
        !seen.insert(origin).second)
      return error() << "connection element has invalid or repeated origin";
    if (previous && detail::compareClosedSourceStructure(previous, origin) >= 0)
      return error()
             << "connection effect origins must be structurally ordered";
    previous = origin;
  }
  return success();
}

LogicalResult verifyParameter(DictionaryAttr parameter, DictionaryAttr owner,
                              Operation *operation, StringRef &name,
                              bool &isConnection, unsigned &bindingRank) {
  auto error = [&] { return operation->emitOpError(); };
  if (!parameter || parameter.size() != 7)
    return error() << "module parameter must have exactly seven fields";
  auto nameAttr = parameter.getAs<StringAttr>("name");
  auto binding = parameter.getAs<StringAttr>("binding");
  auto category = parameter.getAs<StringAttr>("category");
  auto type = parameter.getAs<DictionaryAttr>("type");
  auto defaultValue = parameter.getAs<DictionaryAttr>("default");
  auto origin = parameter.getAs<DictionaryAttr>("origin");
  auto location = parameter.getAs<DictionaryAttr>("location");
  if (!nameAttr || nameAttr.getValue().empty() || !binding || !category ||
      !type || !defaultValue || !origin || !location)
    return error() << "module parameter fields have incorrect types";
  name = nameAttr.getValue();
  if (binding.getValue() == "positional_only")
    bindingRank = 0;
  else if (binding.getValue() == "positional_or_keyword")
    bindingRank = 1;
  else if (binding.getValue() == "keyword_only")
    bindingRank = 2;
  else
    return error() << "module parameter binding is invalid";
  isConnection = category.getValue() == "connection";
  if (!isConnection && category.getValue() != "static")
    return error() << "module parameter category is invalid";
  if (failed(isConnection ? detail::verifyLogicalTypeStructure(type, error)
                          : detail::verifyStaticTypeStructure(type, error)) ||
      failed(detail::verifyDefaultStructure(defaultValue, error)) ||
      failed(detail::verifyOccurrence(origin, error)) ||
      failed(detail::verifySourceSpan(location, error)))
    return failure();
  auto path = location.getAs<StringAttr>("path");
  if (!path || path != owner.getAs<StringAttr>("path"))
    return error() << "module parameter location does not match original owner";
  return success();
}

LogicalResult verifyControlPorts(DictionaryAttr controls,
                                 Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!controls || controls.size() != 2)
    return error() << "ac.control_ports requires exactly clock and reset";
  auto clockAttr = controls.getAs<IntegerAttr>("clock");
  auto resetAttr = controls.getAs<IntegerAttr>("reset");
  if (!clockAttr || !resetAttr || !clockAttr.getType().isInteger(32) ||
      !resetAttr.getType().isInteger(32))
    return error() << "ac.control_ports indices must be i32 attributes";
  auto clock = detail::decodeU64(clockAttr, "clock control index", error);
  auto reset = detail::decodeU64(resetAttr, "reset control index", error);
  if (failed(clock) || failed(reset) || *clock != 0 || *reset != 1)
    return error() << "ac.control_ports requires clock=0 and reset=1";
  return success();
}

} // namespace

LogicalResult ModuleImportOp::verify() {
  auto owner = (*this)->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto origin = (*this)->getAttrOfType<DictionaryAttr>("ac.origin");
  auto role = (*this)->getAttrOfType<StringAttr>("ac.declaration_role");
  if (failed(detail::verifyDeclarationMetadata(*this, owner, origin, role)))
    return failure();
  auto contract = (*this)->getAttrOfType<DictionaryAttr>("ac.contract");
  auto error = [&] { return emitOpError(); };
  if (auto rootKind = (*this)->getAttrOfType<StringAttr>("ac.root_kind");
      rootKind && rootKind.getValue() != "system")
    return error() << "ac.root_kind must be system when present";
  if (failed(verifyControlPorts(
          (*this)->getAttrOfType<DictionaryAttr>("ac.control_ports"), *this)))
    return failure();
  if (!contract || contract.size() != 2)
    return error()
           << "ac.contract must have exactly parameters and connections";
  auto parameters = contract.getAs<ArrayAttr>("parameters");
  auto connections = contract.getAs<ArrayAttr>("connections");
  if (!parameters || !connections)
    return error() << "ac.contract parameters/connections must be ArrayAttr";

  llvm::StringSet<> names;
  SmallVector<std::pair<StringRef, DictionaryAttr>> connectionParameters;
  unsigned previousBindingRank = 0;
  bool havePrevious = false;
  bool positionalDefaultSeen = false;
  for (Attribute raw : parameters) {
    StringRef name;
    bool isConnection = false;
    unsigned bindingRank = 0;
    if (failed(verifyParameter(dyn_cast<DictionaryAttr>(raw), owner, *this,
                               name, isConnection, bindingRank)))
      return failure();
    if (!names.insert(name).second ||
        (havePrevious && bindingRank < previousBindingRank))
      return error()
             << "module parameter names or Python binding order are invalid";
    auto parameter = cast<DictionaryAttr>(raw);
    auto defaultValue = parameter.getAs<DictionaryAttr>("default");
    bool hasDefault = defaultValue.getAs<BoolAttr>("present").getValue();
    if (bindingRank < 2 && positionalDefaultSeen && !hasDefault)
      return error() << "required positional parameter follows a default";
    if (bindingRank < 2 && hasDefault)
      positionalDefaultSeen = true;
    previousBindingRank = bindingRank;
    havePrevious = true;
    if (isConnection)
      connectionParameters.emplace_back(
          name, parameter.getAs<DictionaryAttr>("type"));
  }
  if (connections.size() != connectionParameters.size())
    return error()
           << "ac.contract connections must match connection parameters";
  for (auto [index, raw] : llvm::enumerate(connections)) {
    auto connection = dyn_cast<DictionaryAttr>(raw);
    if (!connection || connection.size() != 2)
      return error() << "connection must have exactly parameter and elements";
    auto parameter = connection.getAs<StringAttr>("parameter");
    auto elements = connection.getAs<ArrayAttr>("elements");
    if (!parameter ||
        parameter.getValue() != connectionParameters[index].first || !elements)
      return error() << "connection parameter/order/elements are invalid";
    DictionaryAttr type = connectionParameters[index].second;
    auto kind = type.getAs<StringAttr>("kind");
    if (!kind)
      return error() << "connection parameter lacks logical kind";
    uint64_t count = 1;
    bool list = kind.getValue() == "list";
    if (list) {
      auto decoded = detail::decodeU64(type.getAs<IntegerAttr>("length"),
                                       "connection list length", error);
      if (failed(decoded))
        return failure();
      count = *decoded;
    }
    if (elements.size() != count)
      return error() << "connection element count does not match logical shape";
    for (auto [ordinal, effect] : llvm::enumerate(elements)) {
      if (failed(verifyElementEffect(dyn_cast<DictionaryAttr>(effect), list,
                                     ordinal, *this)))
        return failure();
    }
  }
  return success();
}

} // namespace acir::ac
