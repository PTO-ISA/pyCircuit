#include "SourceLink.h"
#include "SourceHeaderHelpers.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::compiler {
namespace {

LogicalResult preflightSourceLinkPair(SourceLinkUnit unit, unsigned index,
                                      ac::detail::EmitError emitError) {
  auto headerOwner =
      unit.header->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto bodyOwner = unit.body->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto headerKind = unit.header->getAttrOfType<StringAttr>("ac.unit_kind");
  auto bodyKind = unit.body->getAttrOfType<StringAttr>("ac.unit_kind");
  auto headerStage = unit.header->getAttrOfType<StringAttr>("ac.stage");
  auto bodyStage = unit.body->getAttrOfType<StringAttr>("ac.stage");
  if (failed(ac::detail::verifySourceOwner(headerOwner, emitError)) ||
      failed(ac::detail::verifySourceOwner(bodyOwner, emitError)) ||
      headerOwner != bodyOwner)
    return emitError() << "source link unit[" << index
                       << "] body/header SourceOwner mismatch";
  if (!headerKind || headerKind.getValue() != "interface" || !headerStage ||
      headerStage.getValue() != "source")
    return emitError() << "source link unit[" << index
                       << "] owning header stage/unit_kind is invalid";
  if (!bodyKind ||
      (bodyKind.getValue() != "implementation" &&
       bodyKind.getValue() != "declarations") ||
      !bodyStage || bodyStage.getValue() != "source")
    return emitError() << "source link unit[" << index
                       << "] body stage/unit_kind is invalid";
  for (StringRef name : {"ac.interfaces", "ac.exports", "ac.import_bindings"})
    if (!unit.header->getAttr(name) ||
        unit.header->getAttr(name) != unit.body->getAttr(name))
      return emitError() << "source link unit[" << index
                         << "] body/header metadata differs for " << name;
  return success();
}

} // namespace

FailureOr<SourceHeaderRegistry>
admitSourceLinkUnits(llvm::ArrayRef<SourceLinkUnit> units,
                     ac::detail::EmitError emitError) {
  if (units.empty())
    return emitError() << "source link requires at least one complete unit";

  SmallVector<ModuleOp> headers;
  headers.reserve(units.size());
  llvm::DenseMap<Attribute, unsigned> ownerIndices;
  for (auto [index, unit] : llvm::enumerate(units)) {
    if (!unit.body || !unit.header)
      return emitError() << "source link unit[" << index
                         << "] requires both body and owning header";
    if (failed(preflightSourceLinkPair(unit, index, emitError)))
      return failure();
    auto owner = unit.header->getAttrOfType<DictionaryAttr>("ac.source_owner");
    auto inserted = ownerIndices.try_emplace(owner, index);
    if (!inserted.second)
      return emitError() << "duplicate source link SourceOwner in unit["
                         << inserted.first->second << "] and unit[" << index
                         << ']';
    headers.push_back(unit.header);
  }

  // Build one authority registry for the complete link input. This closes the
  // provider, namespace, declaration, and helper snapshots before any body is
  // passed to the native verifier or a later evaluator.
  auto registry = SourceHeaderRegistry::create(headers, emitError);
  if (failed(registry))
    return failure();

  for (auto [index, unit] : llvm::enumerate(units)) {
    if (failed(
            registry->verifyBodySnapshots(unit.body, unit.header, emitError)))
      return emitError() << "source link body/header admission failed for "
                            "unit["
                         << index << ']';
    if (failed(verify(unit.body)))
      return emitError() << "source link body failed native verification for "
                            "unit["
                         << index << ']';
  }
  return std::move(*registry);
}

FailureOr<OwningOpRef<ModuleOp>>
linkHardwareUnits(ArrayRef<SourceLinkUnit> units, StringRef top,
                   ac::detail::EmitError emitError) {
  auto admitted = admitSourceLinkUnits(units, emitError);
  if (failed(admitted))
    return failure();
  MLIRContext *context = units.front().body->getContext();
  OwningOpRef<ModuleOp> linked(ModuleOp::create(UnknownLoc::get(context)));
  Builder builder(context);
  (*linked)->setAttr("ac.stage", builder.getStringAttr("final"));
  SmallVector<Attribute> owners;
  for (SourceLinkUnit unit : units)
    owners.push_back(unit.header->getAttr("ac.source_owner"));
  llvm::sort(owners, [](Attribute left, Attribute right) {
    return ac::detail::compareClosedSourceStructure(left, right) < 0;
  });
  (*linked)->setAttr("ac.source_units", builder.getArrayAttr(owners));
  llvm::StringMap<Operation *> symbols;
  for (SourceLinkUnit unit : units) {
    for (Operation &operation : unit.body.getBody()->getOperations()) {
      auto name = SymbolTable::getSymbolName(&operation);
      if (!name)
        return operation.emitOpError("source-unit top-level operation must be a declaration");
      auto prior = symbols.find(name.getValue());
      if (prior != symbols.end()) {
        bool oldDefinition = isa<ac::ModuleOp>(prior->second);
        bool newDefinition = isa<ac::ModuleOp>(operation);
        if (oldDefinition && isa<ac::ModuleImportOp>(operation))
          continue;
        if (newDefinition && isa<ac::ModuleImportOp>(prior->second)) {
          prior->second->erase();
          symbols.erase(prior);
        } else if (detail::sameDeclaration(prior->second, &operation) &&
                   !newDefinition) {
          auto role =
              operation.getAttrOfType<StringAttr>("ac.declaration_role");
          auto priorRole =
              prior->second->getAttrOfType<StringAttr>("ac.declaration_role");
          // Every pair already passed owning-body authority. Prefer that real
          // nominal definition over an equivalent consumer snapshot, preserving
          // provider metadata and location independently of link input order.
          if (isa<ac::StructOp, ac::EnumOp>(operation) &&
              role.getValue() == "definition" &&
              priorRole.getValue() == "import_snapshot") {
            prior->second->erase();
            symbols.erase(prior);
          } else {
            continue;
          }
        } else {
          return operation.emitOpError("conflicting linked declaration: ") << name;
        }
      }
      Operation *copy = operation.clone();
      (*linked).getBody()->push_back(copy);
      symbols[name.getValue()] = copy;
    }
  }
  auto selected = symbols.find(top);
  auto root = selected == symbols.end()
                  ? ac::ModuleOp()
                  : dyn_cast<ac::ModuleOp>(selected->second);
  if (!root)
    return emitError() << "link top is not a supplied module definition: @" << top;
  if (!root.getTypeParameters().empty())
    return root.emitOpError("source link requires concrete root type arguments");
  SmallVector<Attribute> arguments;
  for (Attribute raw : root.getParameters()) {
    auto parameter = cast<DictionaryAttr>(raw);
    auto value = parameter.getAs<ac::StaticExprAttr>("default");
    if (!value)
      return root.emitOpError("source link root integer parameter requires a default");
    arguments.push_back(value);
  }
  OperationState state(root.getLoc(), ac::SystemOp::getOperationName());
  state.addAttribute("entry", builder.getDictionaryAttr({
      builder.getNamedAttr("callee", FlatSymbolRefAttr::get(context, top)),
      builder.getNamedAttr("parameters", builder.getArrayAttr(arguments)),
      builder.getNamedAttr("type_arguments", builder.getArrayAttr({}))}));
  state.addAttribute("domain", builder.getStringAttr("default"));
  (*linked).getBody()->push_back(Operation::create(state));
  if (failed(ac::verifyHardwarePackage(*linked)))
    return failure();
  return std::move(linked);
}

} // namespace acir::compiler
