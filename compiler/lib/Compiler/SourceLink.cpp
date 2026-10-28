#include "SourceLink.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "SourceHeaderHelpers.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "pycircuit/Dialect/ACIR/TransactionalRouting.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/raw_ostream.h"

#include <functional>

using namespace mlir;

namespace acir::compiler {
namespace {

// Bind references to actual allocation occurrences before common analysis.
// Each clone remains a Module with its own allocations, Methods and child
// instances. Specialization changes static signatures, never storage identity.
class ReferenceLinker {
  struct Occurrence {
    ac::ModuleOp original;
    ac::InstanceOp allocation;
    unsigned parent;
    std::string name;
    llvm::StringMap<unsigned> children;
    ac::ModuleOp linked;
  };
  static constexpr unsigned NoParent = ~0u;
  ModuleOp package;
  SmallVector<Occurrence> instances;
  llvm::DenseSet<Operation *> expanding;
  llvm::DenseSet<std::pair<unsigned, Value>> resolving;

  Operation *lookup(StringRef name) {
    return SymbolTable::lookupSymbolIn(package, name);
  }
  LogicalResult expand(ac::ModuleOp module, ac::InstanceOp allocation,
                       unsigned parent) {
    if (instances.size() >= 4096 || expanding.size() >= 64 ||
        !expanding.insert(module).second)
      return module.emitOpError(
          "recursive or over-capacity Module instance closure");
    unsigned id = instances.size();
    std::string name = module.getSymName().str();
    if (parent != NoParent) {
      name += ".__instance_" + std::to_string(id);
      if (lookup(name))
        return module.emitOpError(
            "instance specialization name collides with a source declaration");
    }
    instances.push_back({module, allocation, parent, std::move(name), {}, {}});
    for (auto child : module.getBody().front().getOps<ac::InstanceOp>()) {
      auto target = dyn_cast_or_null<ac::ModuleOp>(lookup(child.getCallee()));
      if (!target)
        return child.emitOpError(
            "linked instance requires its explicit provider body");
      unsigned childID = instances.size();
      instances[id].children[child.getInstanceName()] = childID;
      if (failed(expand(target, child, id)))
        return failure();
    }
    expanding.erase(module);
    return success();
  }
  FailureOr<unsigned> reference(unsigned owner, Value value) {
    auto key = std::make_pair(owner, value);
    if (resolving.size() >= 128 || !resolving.insert(key).second) {
      emitError(value.getLoc(),
                "cyclic or over-depth Module reference binding");
      return failure();
    }
    auto cleanup = llvm::scope_exit([&] { resolving.erase(key); });
    if (auto arg = dyn_cast<BlockArgument>(value)) {
      auto *scope = arg.getOwner()->getParentOp();
      if (scope == instances[owner].original) {
        auto &instance = instances[owner];
        if (instance.parent == NoParent ||
            arg.getArgNumber() >= instance.allocation.getNumOperands()) {
          instance.original.emitOpError(
              "Module reference is not bound by an existing parent instance");
          return failure();
        }
        return reference(instance.parent,
                         instance.allocation.getOperand(arg.getArgNumber()));
      }
      if (isa<ac::TransactionRuleOp, ac::TransactionMethodOp>(scope)) {
        auto sig = cast<FunctionType>(
            scope->getAttrOfType<TypeAttr>("function_type").getValue());
        unsigned explicitCount = sig.getNumInputs();
        if (isa<ac::TransactionRuleOp>(scope))
          explicitCount += sig.getNumResults();
        if (arg.getArgNumber() >= explicitCount &&
            arg.getArgNumber() - explicitCount < scope->getNumOperands())
          return reference(
              owner, scope->getOperand(arg.getArgNumber() - explicitCount));
      }
    }
    if (auto ref = value.getDefiningOp<ac::InstanceRefOp>()) {
      unsigned target = owner;
      for (Attribute raw : ref.getInstancePath()) {
        auto at =
            instances[target].children.find(cast<StringAttr>(raw).getValue());
        if (at == instances[target].children.end()) {
          ref.emitOpError(
              "Module reference path does not identify an allocated instance");
          return failure();
        }
        target = at->second;
      }
      return target;
    }
    emitError(value.getLoc(),
              "Module reference requires an existing instance or bound formal");
    return failure();
  }

  LogicalResult specializeResources() {
    // Each registration specializes its template independently, including
    // source-global Rules shared by differently bound instances.
    unsigned ruleIndex = 0;
    for (auto &occurrence : instances) {
      auto module = occurrence.linked;
      llvm::DenseSet<Operation *> specialized;
      SmallVector<ac::TransactionRuleCallOp> calls;
      module.walk(
          [&](ac::TransactionRuleCallOp call) { calls.push_back(call); });
      for (auto call : calls) {
        Operation *target =
            SymbolTable::lookupSymbolIn(module, call.getCallee());
        if (!target)
          target = lookup(call.getCallee());
        auto rule = dyn_cast_or_null<ac::TransactionRuleOp>(target);
        if (!rule)
          return call.emitOpError(
              "Rule registration requires its provider body");
        auto clone = cast<ac::TransactionRuleOp>(rule->clone());
        std::string name;
        do {
          name = "__bound_rule_" + std::to_string(ruleIndex++);
        } while (SymbolTable::lookupSymbolIn(module, name));
        specialized.insert(clone);
        clone->setAttr("sym_name", StringAttr::get(package.getContext(), name));
        module.getBody().front().getOperations().insert(
            module.getBody().front().getTerminator()->getIterator(), clone);
        call->setAttr("callee",
                      FlatSymbolRefAttr::get(package.getContext(), name));
      }
      for (auto rule : llvm::make_early_inc_range(
               module.getBody().front().getOps<ac::TransactionRuleOp>()))
        if (!specialized.contains(rule))
          rule.erase();
    }
    for (auto rule :
         llvm::make_early_inc_range(package.getOps<ac::TransactionRuleOp>()))
      rule.erase();

    SmallVector<ac::ModuleOp> modules;
    for (auto &occurrence : instances)
      modules.push_back(occurrence.linked);
    return ac::resolveTransactionalResourceConnections(package, modules);
  }

public:
  explicit ReferenceLinker(ModuleOp package) : package(package) {}
  LogicalResult run(ac::ModuleOp root) {
    if (failed(expand(root, {}, NoParent)))
      return failure();
    // Clone all bodies before rewriting any provider or signature. The mapping
    // keeps captures and region arguments tied to the original occurrence.
    SmallVector<IRMapping> mappings(instances.size());
    for (unsigned index = 0; index < instances.size(); ++index) {
      auto &instance = instances[index];
      instance.linked =
          cast<ac::ModuleOp>(instance.original->clone(mappings[index]));
      instance.linked->setAttr(
          "sym_name", StringAttr::get(package.getContext(), instance.name));
      package.getBody()->push_back(instance.linked);
    }
    for (unsigned index = 0; index < instances.size(); ++index) {
      auto &instance = instances[index];
      auto &mapping = mappings[index];
      bool failedReference = false;
      auto rewrite = [&](Value original) {
        if (failedReference || !isa<ac::ModuleRefType>(original.getType()))
          return;
        auto target = reference(index, original);
        if (failed(target)) {
          failedReference = true;
          return;
        }
        mapping.lookup(original).setType(ac::ModuleRefType::get(
            package.getContext(),
            FlatSymbolRefAttr::get(package.getContext(),
                                   instances[*target].name)));
      };
      instance.original.walk([&](Operation *op) {
        for (Value result : op->getResults())
          rewrite(result);
        for (Region &region : op->getRegions())
          for (Block &block : region)
            for (BlockArgument argument : block.getArguments())
              rewrite(argument);
        if (auto allocation = dyn_cast<ac::InstanceOp>(op)) {
          auto child = instance.children.lookup(allocation.getInstanceName());
          mapping.lookup(op)->setAttr(
              "callee", FlatSymbolRefAttr::get(package.getContext(),
                                               instances[child].name));
        }
      });
      if (failedReference)
        return failure();
      SmallVector<Type> bodyInputs(
          instance.linked.getBody().front().getArgumentTypes());
      auto originalSignature = instance.original.getFunctionType();
      instance.linked->setAttr(
          "function_type",
          TypeAttr::get(FunctionType::get(
              package.getContext(),
              ArrayRef<Type>(bodyInputs)
                  .take_front(originalSignature.getNumInputs()),
              originalSignature.getResults())));
    }
    if (failed(specializeResources()))
      return failure();
    for (unsigned index = 0; index < instances.size(); ++index) {
      auto module = instances[index].original;
      for (BlockArgument argument : module.getBody().front().getArguments()) {
        auto ref = dyn_cast<ac::ModuleRefType>(argument.getType());
        if (!ref)
          continue;
        auto target = reference(index, argument);
        if (failed(target))
          return failure();
        auto *requirement = lookup(ref.getModule().getValue());
        if (ac::isTransactionalReferenceRequirement(requirement) &&
            failed(ac::verifyTransactionalReferenceBinding(
                requirement, instances[*target].linked,
                instances[index].linked)))
          return failure();
      }
    }
    llvm::DenseSet<Operation *> retained;
    for (auto &instance : instances)
      retained.insert(instance.linked);
    for (Operation &op : llvm::make_early_inc_range(*package.getBody()))
      if (isa<ac::ModuleOp, ac::ModuleImportOp>(op) && !retained.contains(&op))
        op.erase();
    return success();
  }
};

LogicalResult preflightSourceLinkPair(SourceLinkUnit unit, unsigned index,
                                      ac::detail::EmitError emitError) {
  for (auto artifact : {unit.header, unit.body}) {
    auto version = artifact->getAttrOfType<IntegerAttr>("ac.transactional");
    if (!version || !version.getType().isSignlessInteger(64) ||
        version.getInt() != 2)
      return emitError() << "source linking requires transactional version 2; "
                            "regenerate this artifact";
  }
  for (auto artifact : {unit.header, unit.body}) {
    auto contract = artifact->getAttrOfType<IntegerAttr>("ac.source_contract");
    if (!contract || !contract.getType().isSignlessInteger(64) ||
        contract.getInt() != 3)
      return emitError() << "source linking requires source contract 3; "
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
  for (StringRef name : {"ac.transactional", "ac.source_contract",
                         "ac.interfaces", "ac.exports", "ac.import_bindings"})
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
linkSourceUnits(ArrayRef<SourceLinkUnit> units, StringRef top,
                ac::detail::EmitError emitError, SourceLinkTarget target,
                SourceLinkStages *retainedStages,
                ArrayRef<SourceLinkParameterBinding> parameters,
                StringRef resetSemantics) {
  if (resetSemantics != "strict" && resetSemantics != "if_one")
    return emitError() << "reset semantics must be strict or if_one";
  auto admitted = admitSourceLinkUnits(units, emitError);
  if (failed(admitted))
    return failure();
  SmallVector<SourceLinkUnit> orderedUnits(units);
  llvm::sort(orderedUnits,
             [](const SourceLinkUnit &left, const SourceLinkUnit &right) {
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
          if (isa<ac::StructOp, ac::EnumOp, ac::ConstantOp,
                  ac::TransactionRuleOp>(operation) &&
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
  // declaration identity, but executable attributes and types may not refer to
  // it.
  llvm::DenseSet<Attribute> constantSymbols;
  for (auto constant : linked->getOps<ac::ConstantOp>())
    constantSymbols.insert(
        FlatSymbolRefAttr::get(context, constant.getSymName()));
  if (!constantSymbols.empty()) {
    llvm::DenseSet<Attribute> checkedAttributes;
    llvm::DenseSet<Type> checkedTypes;
    bool unresolvedConstant = false;
    std::function<void(Attribute)> checkAttribute;
    std::function<void(Type)> checkType;
    checkAttribute = [&](Attribute attribute) {
      if (!attribute || !checkedAttributes.insert(attribute).second)
        return;
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
      if (isa<ac::ConstantOp>(operation))
        return;
      for (NamedAttribute attribute : operation->getAttrs())
        if (attribute.getName() != "ac.origin")
          checkAttribute(attribute.getValue());
      for (Type type : operation->getOperandTypes())
        checkType(type);
      for (Type type : operation->getResultTypes())
        checkType(type);
      for (Region &region : operation->getRegions())
        for (Block &block : region)
          for (BlockArgument argument : block.getArguments())
            checkType(argument.getType());
    });
    if (unresolvedConstant)
      return emitError()
             << "source constant has a remaining semantic reference "
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

  // Requirements describe interfaces only. Close them against the actual
  // instance graph while preserving every Module and owner boundary.
  if (failed(ReferenceLinker(*linked).run(root)))
    return failure();
  // Instance/resource identities are now concrete. Close Method requirements
  // and payload constraints before admitting linked ACIR to common analysis.
  if (failed(ac::resolveTransactionalMethodConstraints(*linked, true)) ||
      failed(ac::solveTransactionalPayloadConstraints(*linked, true)))
    return failure();
  if (target == SourceLinkTarget::Linked) {
    if (failed(verify(*linked)) ||
        failed(ac::verifyTransactionalPackage(*linked)))
      return failure();
    return std::move(linked);
  }
  // Routing's public analysis entry verifies this same, unmodified linked IR.

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
