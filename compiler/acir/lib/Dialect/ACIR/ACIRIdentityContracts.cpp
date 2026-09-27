#include "ACIRSourceContracts.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Path.h"

#include <cstddef>

using namespace mlir;

namespace acir::ac::detail {
namespace {

LogicalResult requireExactFields(DictionaryAttr value, size_t count,
                                 StringRef recordName, EmitError emitError) {
  if (!value || value.size() != count)
    return emitError() << recordName << " must contain exactly " << count
                       << " fields";
  return success();
}

LogicalResult verifyDictionaryField(
    DictionaryAttr owner, StringRef field, StringRef ownerName,
    LogicalResult (*verifier)(DictionaryAttr, EmitError), EmitError emitError) {
  auto nested = owner.getAs<DictionaryAttr>(field);
  if (!nested)
    return emitError() << ownerName << " field '" << field
                       << "' must be a DictionaryAttr";
  if (failed(verifier(nested, emitError))) {
    emitError() << ownerName << " field '" << field << "' is invalid";
    return failure();
  }
  return success();
}

LogicalResult verifyOccurrenceArray(ArrayAttr values, StringRef description,
                                    EmitError emitError) {
  if (!values)
    return emitError() << description << " must be an ArrayAttr";
  for (auto [index, value] : llvm::enumerate(values)) {
    auto occurrence = dyn_cast<DictionaryAttr>(value);
    if (!occurrence)
      return emitError() << description << '[' << index
                         << "] must be an Occurrence DictionaryAttr";
    if (failed(verifyOccurrence(occurrence, emitError))) {
      emitError() << description << '[' << index << "] is invalid";
      return failure();
    }
  }
  return success();
}

LogicalResult verifyU64Array(ArrayAttr values, StringRef description,
                             EmitError emitError) {
  if (!values)
    return emitError() << description << " must be an ArrayAttr";
  for (auto [index, value] : llvm::enumerate(values))
    if (failed(decodeU64(dyn_cast<IntegerAttr>(value),
                         (Twine(description) + "[" + Twine(index) + "]").str(),
                         emitError)))
      return failure();
  return success();
}

LogicalResult verifyStaticValueArray(ArrayAttr values, StringRef description,
                                     EmitError emitError) {
  if (!values)
    return emitError() << description << " must be an ArrayAttr";
  for (auto [index, value] : llvm::enumerate(values)) {
    auto staticValue = dyn_cast<DictionaryAttr>(value);
    if (!staticValue)
      return emitError() << description << '[' << index
                         << "] must be a StaticValue DictionaryAttr";
    if (failed(verifyStaticValueStructure(staticValue, emitError))) {
      emitError() << description << '[' << index << "] is invalid";
      return failure();
    }
  }
  return success();
}

bool hasNul(StringRef value) { return value.find('\0') != StringRef::npos; }

LogicalResult verifyRelativePythonPath(StringRef path, EmitError emitError) {
  using llvm::sys::path::Style;
  if (path.empty() || llvm::sys::path::is_absolute(path, Style::posix) ||
      llvm::sys::path::has_root_name(path, Style::windows_slash) ||
      llvm::sys::path::has_root_name(path, Style::windows_backslash) ||
      path.contains('\\') || hasNul(path) || !path.ends_with(".py"))
    return emitError() << "SourceOwner path must be a relative POSIX .py path";
  SmallVector<StringRef> components;
  path.split(components, '/', /*MaxSplit=*/-1, /*KeepEmpty=*/true);
  for (StringRef component : components)
    if (component.empty() || component == "." || component == "..")
      return emitError()
             << "SourceOwner path contains an invalid POSIX component";
  return success();
}

LogicalResult verifyFormalOrdinal(Attribute value, EmitError emitError) {
  if (isa<UnitAttr>(value))
    return success();
  return succeeded(decodeU64(dyn_cast<IntegerAttr>(value),
                             "formal StateRef ordinal", emitError))
             ? success()
             : failure();
}

} // namespace

LogicalResult verifySourceOwner(DictionaryAttr value, EmitError emitError) {
  if (failed(requireExactFields(value, 2, "SourceOwner", emitError)))
    return failure();
  auto package = value.getAs<StringAttr>("package");
  auto path = value.getAs<StringAttr>("path");
  if (!package)
    return emitError() << "SourceOwner package must be a StringAttr";
  if (!path)
    return emitError() << "SourceOwner path must be a StringAttr";
  return verifyRelativePythonPath(path.getValue(), emitError);
}

LogicalResult verifyExpansionFrame(DictionaryAttr value, EmitError emitError) {
  if (!value)
    return emitError() << "ExpansionFrame must be a DictionaryAttr";
  auto kind = value.getAs<StringAttr>("kind");
  if (!kind)
    return emitError() << "ExpansionFrame kind must be a StringAttr";
  if (kind.getValue() == "call") {
    if (failed(requireExactFields(value, 3, "call ExpansionFrame", emitError)))
      return failure();
    if (failed(verifyDictionaryField(value, "site", "call ExpansionFrame",
                                     verifySite, emitError)))
      return failure();
    if (!value.getAs<FlatSymbolRefAttr>("callee"))
      return emitError()
             << "call ExpansionFrame callee must be a FlatSymbolRefAttr";
    return success();
  }
  if (kind.getValue() == "iteration") {
    if (failed(requireExactFields(value, 4, "iteration ExpansionFrame",
                                  emitError)))
      return failure();
    if (failed(verifyDictionaryField(value, "site", "iteration ExpansionFrame",
                                     verifySite, emitError)))
      return failure();
    if (failed(decodeU64(value.getAs<IntegerAttr>("ordinal"),
                         "iteration ExpansionFrame ordinal", emitError)))
      return failure();
    return verifyDictionaryField(value, "value", "iteration ExpansionFrame",
                                 verifyStaticValueStructure, emitError);
  }
  return emitError() << "ExpansionFrame kind must be 'call' or 'iteration'";
}

LogicalResult verifyOccurrence(DictionaryAttr value, EmitError emitError) {
  if (failed(requireExactFields(value, 2, "Occurrence", emitError)))
    return failure();
  if (failed(verifyDictionaryField(value, "site", "Occurrence", verifySite,
                                   emitError)))
    return failure();
  auto expansion = value.getAs<ArrayAttr>("expansion");
  if (!expansion)
    return emitError() << "Occurrence expansion must be an ArrayAttr";
  for (auto [index, frame] : llvm::enumerate(expansion)) {
    auto record = dyn_cast<DictionaryAttr>(frame);
    if (!record)
      return emitError() << "Occurrence expansion[" << index
                         << "] must be an ExpansionFrame DictionaryAttr";
    if (failed(verifyExpansionFrame(record, emitError))) {
      emitError() << "Occurrence expansion[" << index << "] is invalid";
      return failure();
    }
  }
  return success();
}

LogicalResult verifySpecKey(DictionaryAttr value, EmitError emitError) {
  if (failed(requireExactFields(value, 2, "SpecKey", emitError)))
    return failure();
  if (!value.getAs<FlatSymbolRefAttr>("definition"))
    return emitError() << "SpecKey definition must be a FlatSymbolRefAttr";
  return verifyStaticValueArray(value.getAs<ArrayAttr>("arguments"),
                                "SpecKey arguments", emitError);
}

LogicalResult verifyValueID(DictionaryAttr value, EmitError emitError) {
  if (failed(requireExactFields(value, 2, "ValueID", emitError)))
    return failure();
  if (failed(verifyDictionaryField(value, "origin", "ValueID", verifyOccurrence,
                                   emitError)))
    return failure();
  return succeeded(decodeU32(value.getAs<IntegerAttr>("slot"), "ValueID slot",
                             emitError))
             ? success()
             : failure();
}

LogicalResult verifyCheckID(DictionaryAttr value, EmitError emitError) {
  if (failed(requireExactFields(value, 3, "CheckID", emitError)))
    return failure();
  if (failed(verifyDictionaryField(value, "registration", "CheckID",
                                   verifyOccurrence, emitError)) ||
      failed(verifyDictionaryField(value, "check", "CheckID", verifyOccurrence,
                                   emitError)))
    return failure();
  return succeeded(decodeU64(value.getAs<IntegerAttr>("obligation"),
                             "CheckID obligation", emitError))
             ? success()
             : failure();
}

LogicalResult verifyProofScope(DictionaryAttr value, EmitError emitError) {
  if (failed(requireExactFields(value, 2, "ProofScope", emitError)))
    return failure();
  if (failed(verifyDictionaryField(value, "specialization", "ProofScope",
                                   verifySpecKey, emitError)) ||
      failed(verifyDictionaryField(value, "registration", "ProofScope",
                                   verifyOccurrence, emitError)))
    return failure();
  return success();
}

LogicalResult verifyOwnerRef(DictionaryAttr value, EmitError emitError) {
  if (failed(requireExactFields(value, 1, "OwnerRef", emitError)))
    return failure();
  return verifyOccurrenceArray(value.getAs<ArrayAttr>("instance_path"),
                               "OwnerRef instance_path", emitError);
}

LogicalResult verifyStateID(DictionaryAttr value, EmitError emitError) {
  if (failed(requireExactFields(value, 3, "StateID", emitError)))
    return failure();
  if (failed(verifyDictionaryField(value, "owner", "StateID", verifyOwnerRef,
                                   emitError)) ||
      failed(verifyDictionaryField(value, "declaration", "StateID",
                                   verifyOccurrence, emitError)))
    return failure();
  return verifyU64Array(value.getAs<ArrayAttr>("element"), "StateID element",
                        emitError);
}

LogicalResult verifyStateRef(DictionaryAttr value, EmitError emitError) {
  if (!value)
    return emitError() << "StateRef must be a DictionaryAttr";
  auto kind = value.getAs<StringAttr>("kind");
  if (!kind)
    return emitError() << "StateRef kind must be a StringAttr";
  if (kind.getValue() == "owned") {
    if (failed(requireExactFields(value, 3, "owned StateRef", emitError)))
      return failure();
    if (failed(verifyDictionaryField(value, "declaration", "owned StateRef",
                                     verifyOccurrence, emitError)))
      return failure();
    return verifyU64Array(value.getAs<ArrayAttr>("element"),
                          "owned StateRef element", emitError);
  }
  if (kind.getValue() == "formal") {
    if (failed(requireExactFields(value, 3, "formal StateRef", emitError)))
      return failure();
    if (!value.getAs<StringAttr>("parameter"))
      return emitError() << "formal StateRef parameter must be a StringAttr";
    return verifyFormalOrdinal(value.get("ordinal"), emitError);
  }
  return emitError() << "StateRef kind must be 'owned' or 'formal'";
}

} // namespace acir::ac::detail
