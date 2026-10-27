#include "SourceLink.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "SourceHeaderHelpers.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/TransactionalRouting.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <functional>

using namespace mlir;

namespace acir::compiler {
namespace {

LogicalResult preflightSourceLinkPair(SourceLinkUnit unit, unsigned index,
                                      ac::detail::EmitError emitError) {
  for (auto artifact : {unit.header, unit.body}) {
    auto version = artifact->getAttrOfType<IntegerAttr>("ac.transactional");
    if (!version || !version.getType().isSignlessInteger(64) ||
        version.getInt() != 2)
      return emitError() << "source linking requires transactional version 2; "
                            "regenerate this artifact";
  }
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
      headerStage.getValue() != "unlinked")
    return emitError() << "source link unit[" << index
                       << "] owning header stage/unit_kind is invalid";
  if (!bodyKind ||
      (bodyKind.getValue() != "implementation" &&
       bodyKind.getValue() != "declarations") ||
      !bodyStage || bodyStage.getValue() != "unlinked")
    return emitError() << "source link unit[" << index
                       << "] body stage/unit_kind is invalid";
  for (StringRef name : {"ac.transactional", "ac.interfaces", "ac.exports",
                         "ac.import_bindings"})
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

FailureOr<OwningOpRef<ModuleOp>> linkSourceUnits(
    ArrayRef<SourceLinkUnit> units, StringRef top,
    ac::detail::EmitError emitError, SourceLinkTarget target,
    SourceLinkStages *retainedStages,
    ArrayRef<SourceLinkParameterBinding> parameters, StringRef resetSemantics) {
  if (resetSemantics != "strict" && resetSemantics != "if_one")
    return emitError() << "reset semantics must be strict or if_one";
  auto admitted = admitSourceLinkUnits(units, emitError);
  if (failed(admitted))
    return failure();
  SmallVector<SourceLinkUnit> orderedUnits(units);
  llvm::sort(orderedUnits, [](const SourceLinkUnit &left,
                             const SourceLinkUnit &right) {
    return ac::detail::compareClosedSourceStructure(
               left.header->getAttr("ac.source_owner"),
               right.header->getAttr("ac.source_owner")) < 0;
  });
  units = orderedUnits;
  MLIRContext *context = units.front().body->getContext();
  OwningOpRef<ModuleOp> linked(ModuleOp::create(UnknownLoc::get(context)));
  Builder builder(context);
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
        return operation.emitOpError(
            "source-unit top-level operation must be a declaration");
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
          if (isa<ac::StructOp, ac::EnumOp, ac::ConstantOp>(operation) &&
              role.getValue() == "definition" &&
              priorRole.getValue() == "import_snapshot") {
            prior->second->erase();
            symbols.erase(prior);
          } else {
            continue;
          }
        } else {
          return operation.emitOpError("conflicting linked declaration: ")
                 << name;
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
    return emitError() << "link top is not a supplied module definition: @"
                       << top;
  if (!parameters.empty() || !root.getParameters().empty() ||
      !root.getTypeParameters().empty() ||
      !root.getFunctionType().getInputs().empty())
    return root.emitOpError(
        "transactional link requires a closed concrete root");
  (*linked)->setAttr("ac.transactional", builder.getI64IntegerAttr(2));
  (*linked)->setAttr("ac.stage", builder.getStringAttr("linked"));
  SmallVector<Attribute> arguments;
  OperationState state(root.getLoc(), ac::SystemOp::getOperationName());
  state.addAttribute(
      "entry",
      builder.getDictionaryAttr(
          {builder.getNamedAttr("callee", FlatSymbolRefAttr::get(context, top)),
           builder.getNamedAttr("parameters", builder.getArrayAttr(arguments)),
           builder.getNamedAttr("type_arguments", builder.getArrayAttr({}))}));
  state.addAttribute("domain", builder.getStringAttr("default"));
  state.addAttribute("ac.execution_policy",
                     builder.getDictionaryAttr({builder.getNamedAttr(
                         "reset", builder.getStringAttr(resetSemantics))}));
  (*linked).getBody()->push_back(Operation::create(state));
  // Constants carry source authority only. Admission authenticated the exact
  // owning bodies and every imported snapshot before we made this clone. Uses
  // must already be ordinary literal payloads; provenance may retain the source
  // declaration identity, but executable attributes and types may not refer to it.
  llvm::DenseSet<Attribute> constantSymbols;
  for (auto constant : linked->getOps<ac::ConstantOp>())
    constantSymbols.insert(FlatSymbolRefAttr::get(context, constant.getSymName()));
  if (!constantSymbols.empty()) {
    llvm::DenseSet<Attribute> checkedAttributes;
    llvm::DenseSet<Type> checkedTypes;
    bool unresolvedConstant = false;
    std::function<void(Attribute)> checkAttribute;
    std::function<void(Type)> checkType;
    checkAttribute = [&](Attribute attribute) {
      if (!attribute || !checkedAttributes.insert(attribute).second) return;
      if (auto symbol = dyn_cast<SymbolRefAttr>(attribute)) {
        if (constantSymbols.contains(FlatSymbolRefAttr::get(
                context, symbol.getRootReference().getValue())))
          unresolvedConstant = true;
        return;
      }
      if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
        for (NamedAttribute field : dictionary)
          if (field.getName() != "origin" && field.getName() != "location" &&
              field.getName() != "ac.origin")
            checkAttribute(field.getValue());
        return;
      }
      attribute.walkImmediateSubElements(checkAttribute, checkType);
    };
    checkType = [&](Type type) {
      if (type && checkedTypes.insert(type).second)
        type.walkImmediateSubElements(checkAttribute, checkType);
    };
    linked->walk([&](Operation *operation) {
      if (isa<ac::ConstantOp>(operation)) return;
      for (NamedAttribute attribute : operation->getAttrs())
        if (attribute.getName() != "ac.origin")
          checkAttribute(attribute.getValue());
      for (Type type : operation->getOperandTypes()) checkType(type);
      for (Type type : operation->getResultTypes()) checkType(type);
      for (Region &region : operation->getRegions())
        for (Block &block : region)
          for (BlockArgument argument : block.getArguments())
            checkType(argument.getType());
    });
    if (unresolvedConstant)
      return emitError() << "source constant has a remaining semantic reference "
                           "after literal materialization";
  }
  for (auto constant :
       llvm::make_early_inc_range(linked->getOps<ac::ConstantOp>()))
    constant.erase();
  // Native transformation checks IR semantics; canonical provider authority
  // stays with the admitted source registry, including after definition merge.
  for (auto imported : linked->getOps<ac::ModuleImportOp>())
    if (failed(admitted->verifyModuleImport(imported, emitError)))
      return failure();

  SourceLinkStages stages;
  auto capture = [](ModuleOp module, std::string &text) {
    llvm::raw_string_ostream output(text);
    module->print(output, OpPrintingFlags().enableDebugInfo());
    output << '\n';
  };
  capture(*linked, stages.beforeTransactions);
  ac::HardwareSourceCheckLimits limits;
  limits.maxWorkUnits = 1024 * 1024;
  ac::detail::SourceCheckBudget budget(limits);
  auto routed = ac::routeTransactionalPackage(
      *linked, top, [&](uint64_t units, Operation *site) {
        return budget.debit(units, site);
      });
  if (failed(routed))
    return failure();
  linked = std::move(*routed);
  capture(*linked, stages.routedTransactions);
  if (target == SourceLinkTarget::Hardware) {
    PassManager lowering(context);
    lowering.addPass(acir::createLowerTransactionsPass());
    if (failed(lowering.run(*linked)))
      return failure();
    capture(*linked, stages.afterTransactions);
  }
  if (retainedStages)
    *retainedStages = std::move(stages);
  return std::move(linked);
}

} // namespace acir::compiler
