#include "ACIRHardwareClosure.h"
#include "ACIRFinalDeclarations.h"

#include "ACIRHardwareClosureDetail.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"

#include <algorithm>

using namespace mlir;

namespace acir::ac::hardware_detail {

namespace {
int compareUTF8(StringRef left, StringRef right) {
  size_t count = std::min(left.size(), right.size());
  for (size_t index = 0; index < count; ++index) {
    auto lhs = static_cast<unsigned char>(left[index]);
    auto rhs = static_cast<unsigned char>(right[index]);
    if (lhs != rhs)
      return lhs < rhs ? -1 : 1;
  }
  return left.size() == right.size() ? 0 : left.size() < right.size() ? -1 : 1;
}

bool containsRecordPayload(Type type) {
  if (isa<StructType>(type))
    return true;
  if (auto reg = dyn_cast<RegType>(type))
    return containsRecordPayload(reg.getElementType());
  return false;
}
} // namespace

FailureOr<DictionaryAttr> specKey(Attribute raw, Operation *owner) {
  // `dyn_cast` requires a non-null attribute; a package without `ac.entry`
  // reaches here with none, which must be a diagnostic and not a crash.
  auto value = dyn_cast_or_null<DictionaryAttr>(raw);
  if (!value || failed(detail::verifySpecKey(
                    value, [&] { return owner->emitOpError(); })))
    return owner->emitOpError() << "final hardware SpecKey is malformed";
  return value;
}

DictionaryAttr ownedRef(Builder &builder, RegOp reg) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("owned")),
      builder.getNamedAttr("declaration", reg->getAttr("ac.declaration")),
      builder.getNamedAttr("element", reg->getAttr("ac.element")),
  });
}

DictionaryAttr formalRef(Builder &builder, DictionaryAttr port) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("formal")),
      builder.getNamedAttr("parameter", port.get("parameter")),
      builder.getNamedAttr("ordinal", port.get("ordinal")),
  });
}

DictionaryAttr stateID(Builder &builder, DictionaryAttr owner,
                       DictionaryAttr state) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("owner", owner),
      builder.getNamedAttr("declaration", state.get("declaration")),
      builder.getNamedAttr("element", state.get("element")),
  });
}

LogicalResult inspectEnvelope(Closure &closure,
                              ac::detail::EmitError emitError) {
  mlir::ModuleOp package = closure.package;
  auto stage = package->getAttrOfType<StringAttr>("ac.stage");
  auto entry = specKey(package->getAttr("ac.entry"), package);
  auto rows = package->getAttrOfType<ArrayAttr>("ac.instance_bindings");
  if (!stage || stage.getValue() != "final" || failed(entry) || !rows ||
      package->getAttrs().size() != 3 || package->hasAttr("ac.unit_kind") ||
      package.getBody()->empty())
    return emitError() << "final hardware package envelope is not canonical";
  closure.entry = *entry;
  closure.rows = rows;

  Builder builder(package.getContext());
  DictionaryAttr previousOwner;
  DenseSet<Attribute> caseFoldedOwners;
  llvm::StringMap<DictionaryAttr> ownerByImportModule;
  DenseSet<Attribute> canonicalSymbols;
  DenseSet<Operation *> recordDeclarations;
  for (Operation &nested : package.getBody()->getOperations()) {
    if (nested.getName().getStringRef() == "ac.system") {
      if (closure.system)
        return emitError() << "final hardware package repeats ac.system";
      closure.system = &nested;
      continue;
    }
    auto unit = dyn_cast<mlir::ModuleOp>(nested);
    auto unitStage =
        unit ? unit->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
    auto kind =
        unit ? unit->getAttrOfType<StringAttr>("ac.unit_kind") : StringAttr();
    auto owner = unit ? unit->getAttrOfType<DictionaryAttr>("ac.source_owner")
                      : DictionaryAttr();
    if (!unit || unit->getNumRegions() != 1 ||
        !unit.getBodyRegion().hasOneBlock() ||
        unit.getBody()->getNumArguments() != 0 || !unitStage ||
        unitStage.getValue() != "final" || !kind ||
        (kind.getValue() != "implementation" &&
         kind.getValue() != "declarations") ||
        failed(detail::verifySourceOwner(owner, emitError)) ||
        unit->getAttrs().size() != 3 ||
        (previousOwner &&
         detail::compareClosedSourceStructure(previousOwner, owner) >= 0))
      return emitError()
             << "final package child is not a canonical source-owned unit";
    auto caseFoldedOwner =
        final_detail::sourceOwnerCaseFoldIdentity(owner, emitError);
    if (failed(caseFoldedOwner))
      return failure();
    if (!caseFoldedOwners.insert(*caseFoldedOwner).second)
      return emitError() << "final package repeats a case-folded SourceOwner";
    auto importModule =
        final_detail::sourceImportModuleIdentity(owner, emitError);
    if (failed(importModule))
      return failure();
    if (!ownerByImportModule.try_emplace(*importModule, owner).second)
      return emitError()
             << "final package repeats a case-folded import-module identity";
    previousOwner = owner;
    bool implementation = kind.getValue() == "implementation";
    bool sawModule = false;
    size_t moduleCount = 0;
    StringRef previousDeclaration;
    for (Operation &child : unit.getBody()->getOperations()) {
      if (isa<TypeAliasOp, ConstantOp, StructOp>(child)) {
        if (sawModule)
          return emitError()
                 << "final declarations must precede their module definition";
        auto symbol = isa<StructOp>(child)
                          ? final_detail::verifyFinalRecordDeclaration(
                                &child, owner, emitError)
                          : final_detail::verifyFinalScalarDeclaration(
                                &child, owner, emitError);
        if (failed(symbol))
          return failure();
        if (isa<StructOp>(child))
          recordDeclarations.insert(&child);
        StringRef name = symbol->getValue();
        if (!previousDeclaration.empty()) {
          int order = compareUTF8(previousDeclaration, name);
          if (order == 0)
            return emitError()
                   << "final package repeats declaration symbol " << *symbol;
          if (order > 0)
            return emitError() << "final declarations are not sorted by symbol";
        }
        previousDeclaration = name;
        if (!canonicalSymbols.insert(*symbol).second)
          return emitError()
                 << "final package repeats canonical symbol " << *symbol;
        continue;
      }
      auto module = dyn_cast<ModuleOp>(child);
      if (!module || !implementation || sawModule)
        return emitError()
               << "final source unit contains an unsupported direct child";
      sawModule = true;
      ++moduleCount;
      if (failed(final_detail::verifyFinalQualifiedSymbol(
              owner, module.getSymName(), emitError)))
        return failure();
      FlatSymbolRefAttr definition =
          FlatSymbolRefAttr::get(package.getContext(), module.getSymName());
      if (!canonicalSymbols.insert(definition).second)
        return emitError() << "final package repeats canonical symbol "
                           << definition;
      auto key = builder.getDictionaryAttr({
          builder.getNamedAttr("definition", definition),
          builder.getNamedAttr("arguments", builder.getArrayAttr({})),
      });
      if (!closure.definitions.try_emplace(key, Definition{module, key}).second)
        return emitError() << "final package repeats a module definition";
    }
    if (implementation != (moduleCount == 1))
      return emitError()
             << (implementation
                     ? "final implementation unit must own one module"
                     : "final declarations unit cannot contain a module");
  }
  if (!closure.system || closure.definitions.empty())
    return emitError() << "final package lacks system or definitions";
  LogicalResult recordClosure = success();
  package.walk([&](Operation *operation) {
    if (failed(recordClosure))
      return;
    if (isa<StructOp>(operation)) {
      if (!recordDeclarations.contains(operation))
        recordClosure = operation->emitError()
                        << "ac.struct must be a direct final declaration";
      return;
    }
    if (isa<StructCreateOp, StructGetOp>(operation)) {
      recordClosure = operation->emitError()
                      << "record value operations are not admitted in S1";
      return;
    }
    for (Type type : operation->getOperandTypes())
      if (containsRecordPayload(type)) {
        recordClosure = operation->emitError()
                        << "record payloads are not admitted in S1";
        return;
      }
    for (Type type : operation->getResultTypes())
      if (containsRecordPayload(type)) {
        recordClosure = operation->emitError()
                        << "record payloads are not admitted in S1";
        return;
      }
    for (Region &region : operation->getRegions())
      for (Block &block : region)
        for (BlockArgument argument : block.getArguments())
          if (containsRecordPayload(argument.getType())) {
            recordClosure = operation->emitError()
                            << "record payloads are not admitted in S1";
            return;
          }
  });
  if (failed(recordClosure))
    return failure();
  Operation *system = closure.system;
  auto systemEntry = specKey(system->getAttr("entry"), system);
  auto owner = system->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto origin = system->getAttrOfType<DictionaryAttr>("ac.origin");
  auto domain = system->getAttrOfType<StringAttr>("domain");
  if (system->getNumOperands() || system->getNumResults() ||
      system->getNumRegions() || system->getAttrs().size() != 4 ||
      failed(systemEntry) || *systemEntry != closure.entry ||
      failed(detail::verifySourceOwner(owner, emitError)) ||
      failed(detail::verifyOccurrence(origin, emitError)) || !domain ||
      domain.getValue() != "default")
    return emitError() << "ac.system descriptor is not canonical";
  return success();
}

} // namespace acir::ac::hardware_detail

namespace acir::ac {

LogicalResult verifyFinalHardware(mlir::ModuleOp package) {
  if (!package)
    return failure();
  auto emit = [&] { return package.emitError(); };
  hardware_detail::Closure closure;
  closure.package = package;
  if (failed(hardware_detail::inspectEnvelope(closure, emit)) ||
      failed(hardware_detail::rebuildInstances(closure, emit)) ||
      failed(hardware_detail::verifyInstanceRows(closure, emit)) ||
      failed(hardware_detail::verifyCommits(closure, emit)))
    return failure();
  return success();
}

} // namespace acir::ac
