#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Transforms/Passes.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir {
namespace {

class MaterializationBudget {
public:
  bool charge(Operation *site, uint64_t units = 1) {
    if (units > limit - used) {
      mlir::emitError(UnknownLoc::get(site->getContext()))
          << "transaction materialization work budget exceeded";
      return false;
    }
    used += units;
    return true;
  }

  bool product(Operation *site, uint64_t count, uint64_t units) {
    if (units && count > (limit - used) / units) {
      mlir::emitError(UnknownLoc::get(site->getContext()))
          << "transaction materialization expansion budget exceeded";
      return false;
    }
    return charge(site, count * units);
  }

private:
  static constexpr uint64_t limit = 1024 * 1024;
  uint64_t used = 0;
};

LogicalResult mergedEnvelope(mlir::ModuleOp package) {
  if (package->hasAttr("ac.stage"))
    return package.emitError()
           << "transaction lowering requires a merged package without ac.stage";
  for (StringRef name : {"ac.source_owner", "ac.unit_kind", "ac.interfaces",
                         "ac.exports", "ac.import_bindings"})
    if (package->hasAttr(name))
      return package.emitError()
             << "transaction lowering cannot consume a single-source envelope: "
             << name;
  if (!package->getAttrOfType<ArrayAttr>("ac.source_units"))
    return package.emitError()
           << "transaction lowering requires merged ac.source_units";
  if (failed(ac::collectFinalSourceUnits(package,
                                         [&] { return package.emitError(); })))
    return failure();
  unsigned systems = 0;
  for (Operation &op : package.getBody()->getOperations()) {
    if (!isa<ac::ModuleOp, ac::ModuleImportOp, ac::StructOp, ac::EnumOp,
             ac::SystemOp>(op))
      return op.emitOpError() << "transaction lowering requires closed merged "
                                 "hardware declarations";
    systems += isa<ac::SystemOp>(op);
    if (auto imported = dyn_cast<ac::ModuleImportOp>(op))
      if (imported.getSemanticKind() != "hardware" ||
          !imported->getAttrOfType<StringAttr>("primitive_kind"))
        return imported.emitOpError()
               << "transaction lowering requires resolved provider definitions";
  }
  if (systems != 1)
    return package.emitError()
           << "transaction lowering requires exactly one merged ac.system";
  return success();
}

LogicalResult prepayClone(Operation *root, MaterializationBudget &budget) {
  SmallVector<Operation *> pending;
  if (!budget.charge(root))
    return failure();
  pending.push_back(root);
  while (!pending.empty()) {
    Operation *op = pending.pop_back_val();
    if (!budget.charge(op, 3 + uint64_t(op->getNumOperands()) * 2 +
                               uint64_t(op->getNumResults()) * 2 +
                               op->getNumRegions() +
                               op->getRawDictionaryAttrs().size() +
                               op->getPropertiesStorageSize()))
      return failure();
    for (Region &region : op->getRegions())
      for (Block &block : region) {
        if (!budget.charge(op, 1 + uint64_t(block.getNumArguments()) * 2))
          return failure();
        for (Operation &nested : block) {
          if (!budget.charge(op))
            return failure();
          pending.push_back(&nested);
        }
      }
  }
  return success();
}

bool moduleValue(Value value, ac::ModuleOp module) {
  return value.getParentBlock() == &module.getBody().front();
}

struct TransactionPreparation {
  const ac::TransactionStatePlan *plan = nullptr;
  SmallVector<Value> liveIns;
  SmallVector<ac::OwnerVersionBinding> outputs;
};

/// Admission and growth accounting finish on the intact input before cloning.
/// The prepared records borrow the unique analysis; they own no alternate IR.
class LoweringPreparation {
public:
  LoweringPreparation(mlir::ModuleOp package,
                      const ac::SourceTransactionStateAnalysis &analysis,
                      MaterializationBudget &budget)
      : package(package), hardware(package), analysis(analysis),
        budget(budget) {}

  LogicalResult run() {
    if (failed(mergedEnvelope(package)) || failed(prepayClone(package, budget)))
      return failure();
    for (auto imported : package.getOps<ac::ModuleImportOp>()) {
      if (!budget.charge(imported) || failed(hardware.verifyImport(imported)))
        return failure();
      if (hardware.getPrimitiveKind(imported) == "dffe") {
        ++dffeCount;
        if (!dffe)
          dffe = imported;
      }
    }
    for (auto module : package.getOps<ac::ModuleOp>()) {
      ac::HardwareBindings bindings;
      bindings.owner = module;
      for (Operation &op : module.getBody().front()) {
        if (!budget.charge(&op))
          return failure();
        if (auto state = dyn_cast<ac::SourceStateOp>(op)) {
          if (!isa<ac::BitsType>(state.getQ().getType()))
            return state.emitOpError()
                   << "transaction lowering supports fixed Bits state only";
          auto width =
              hardware.getPackedWidth(state.getQ().getType(), bindings, state);
          if (failed(width))
            return failure();
          if (*width == 0)
            return state.emitOpError()
                   << "transaction state width must be positive";
          if (!budget.charge(state, 32))
            return failure();
          states.push_back(state);
        } else if (auto transaction = dyn_cast<ac::SourceTransactionOp>(op)) {
          auto *plan = analysis.lookup(transaction);
          if (!plan)
            return transaction.emitOpError()
                   << "transaction lowering requires the current complete "
                      "state plan";
          if (failed(admit(*plan)) || !budget.charge(transaction))
            return failure();
          transactions.push_back({plan, {}, {}});
        }
      }
    }
    if (!states.empty() && !dffe)
      return package.emitError() << "transaction state requires an existing "
                                    "verified DFFE primitive";
    if (!states.empty() && dffeCount != 1)
      return package.emitError()
             << "transaction state requires exactly one DFFE primitive";

    // Conflicts are collected before any operation or unused result disappears.
    for (const auto &prepared : transactions)
      for (const auto &write : prepared.plan->writes) {
        if (!budget.charge(write.operation))
          return failure();
        auto declaration = prepared.plan->owners[write.owner].declaration;
        auto transaction = prepared.plan->transaction;
        auto [prior, inserted] =
            writers.try_emplace(declaration, transaction.getOperation());
        if (!inserted && prior->second != prepared.plan->transaction)
          return declaration.emitOpError()
                 << "transaction lowering cannot admit multiple outer writers";
      }
    for (auto &prepared : transactions)
      if (failed(liveIns(prepared)) || failed(outputs(prepared)) ||
          failed(expansion(prepared)))
        return failure();
    return success();
  }

  SmallVector<TransactionPreparation, 0> transactions;
  SmallVector<ac::SourceStateOp> states;
  ac::ModuleImportOp dffe;

private:
  LogicalResult admit(const ac::TransactionStatePlan &plan) {
    auto transaction = plan.transaction;
    auto root = plan.frames[plan.rootFrame].anchor;
    for (const auto &frame : plan.frames) {
      if (!budget.charge(transaction))
        return failure();
      if (frame.anchor != root)
        return transaction.emitOpError()
               << "transaction lowering supports same-owner Method calls only";
    }
    for (const auto &guard : plan.guards) {
      if (!budget.charge(transaction))
        return failure();
      if (!guard.predicateFact || !guard.predicateFact.getValue())
        return guard.callable->emitOpError()
               << "transaction lowering requires proven-true active guards";
    }
    for (const auto &owner : plan.owners) {
      if (!budget.charge(transaction))
        return failure();
      if (owner.anchor != root ||
          owner.declaration->getParentOp() != plan.lexicalModule ||
          !isa<ac::BitsType>(owner.payloadType))
        return transaction.emitOpError()
               << "transaction lowering requires whole same-owner Bits state";
    }
    return success();
  }

  LogicalResult liveIns(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    auto module = plan.lexicalModule;
    auto transaction = plan.transaction;
    llvm::DenseSet<Value> required;
    auto add = [&](Value value) -> LogicalResult {
      if (!budget.charge(transaction, 2))
        return failure();
      if (!moduleValue(value, module))
        return transaction.emitOpError() << "transaction live-in must be "
                                            "actual SSA of its lexical module";
      required.insert(value);
      return success();
    };
    for (Value capture : transaction.getCaptures())
      if (failed(add(capture)))
        return failure();
    for (const auto &owner : plan.owners) {
      auto declaration = owner.declaration;
      if (failed(add(declaration.getQ())))
        return failure();
    }
    for (const auto &frame : plan.frames) {
      auto method = frame.method;
      if (method)
        for (Value capture : method.getCaptures())
          if (failed(add(capture)))
            return failure();
      Operation *function =
          method ? method.getOperation() : transaction.getOperation();
      SmallVector<Operation *> pending;
      if (!budget.charge(function))
        return failure();
      for (Region &region : function->getRegions())
        for (Operation &op : region.front()) {
          if (!budget.charge(function))
            return failure();
          pending.push_back(&op);
        }
      while (!pending.empty()) {
        Operation *op = pending.pop_back_val();
        if (!budget.charge(op, op->getNumOperands()))
          return failure();
        for (Value operand : op->getOperands())
          if (moduleValue(operand, module) && failed(add(operand)))
            return failure();
        for (Region &region : op->getRegions())
          for (Block &block : region)
            for (Operation &nested : block) {
              if (!budget.charge(op))
                return failure();
              pending.push_back(&nested);
            }
      }
    }
    auto record = [&](Value value) {
      if (!budget.charge(transaction))
        return false;
      if (required.contains(value))
        prepared.liveIns.push_back(value);
      return true;
    };
    for (BlockArgument argument : module.getBody().front().getArguments())
      if (!record(argument))
        return failure();
    for (Operation &op : module.getBody().front())
      for (Value result : op.getResults())
        if (!record(result))
          return failure();
    if (prepared.liveIns.size() != required.size())
      return transaction.emitOpError()
             << "transaction live-in identity is unresolved";
    return success();
  }

  LogicalResult outputs(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    auto transaction = plan.transaction;
    llvm::DenseMap<Operation *, ac::OwnerVersionBinding> final;
    for (auto binding : plan.finalVersions) {
      if (!budget.charge(transaction))
        return failure();
      if (plan.versions[binding.version].kind != ac::StateVersionKind::Write)
        continue;
      auto declaration = plan.owners[binding.owner].declaration;
      final.try_emplace(declaration, binding);
    }
    // Internal D result order follows state declaration order, independent of
    // first-read discovery or ordinary return/capture names.
    auto module = plan.lexicalModule;
    for (auto state : module.getBody().front().getOps<ac::SourceStateOp>()) {
      if (!budget.charge(state))
        return failure();
      auto found = final.find(state);
      if (found != final.end())
        prepared.outputs.push_back(found->second);
    }
    return success();
  }

  LogicalResult expansion(const TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    auto transaction = plan.transaction;
    if (!budget.charge(transaction, plan.versions.size() + plan.reads.size() +
                                        plan.writes.size() + plan.calls.size() +
                                        plan.frames.size() +
                                        plan.results.size() +
                                        prepared.outputs.size()) ||
        !budget.product(transaction, plan.frames.size(),
                        2 * prepared.liveIns.size() + 1))
      return failure();
    for (const auto &frame : plan.frames) {
      auto method = frame.method;
      Operation *function =
          method ? method.getOperation() : transaction.getOperation();
      if (!budget.charge(function, frame.guardArguments.size() +
                                       frame.bodyArguments.size()))
        return failure();
      SmallVector<Operation *> pending;
      for (Region &region : function->getRegions())
        for (Operation &op : region.front()) {
          if (!budget.charge(function))
            return failure();
          pending.push_back(&op);
        }
      while (!pending.empty()) {
        Operation *op = pending.pop_back_val();
        if (!budget.charge(op, 3 + uint64_t(op->getNumOperands()) * 2 +
                                   uint64_t(op->getNumResults()) * 2 +
                                   op->getNumRegions() +
                                   op->getRawDictionaryAttrs().size() +
                                   op->getPropertiesStorageSize()))
          return failure();
        if (!isa<ac::SourceStateReadOp, ac::SourceStateWriteOp,
                 ac::SourceMethodCallOp, ac::YieldOp>(op) &&
            !ac::detail::isCombinationalHardwareOperation(op))
          return op->emitOpError()
                 << "transaction lowering cannot materialize this operation";
        for (Region &region : op->getRegions())
          for (Block &block : region) {
            if (!budget.charge(op, 1 + uint64_t(block.getNumArguments()) * 2))
              return failure();
            for (Operation &nested : block) {
              if (!budget.charge(op))
                return failure();
              pending.push_back(&nested);
            }
          }
      }
    }
    return success();
  }

  mlir::ModuleOp package;
  ac::HardwareAnalysis hardware;
  const ac::SourceTransactionStateAnalysis &analysis;
  MaterializationBudget &budget;
  llvm::DenseMap<Operation *, Operation *> writers;
  unsigned dffeCount = 0;
};

struct StateStorage {
  ac::SourceStateOp declaration;
  ac::InstanceOp instance;
};

class RuleMaterializer {
public:
  RuleMaterializer(const TransactionPreparation &prepared,
                   IRMapping &packageMap)
      : prepared(prepared), plan(*prepared.plan), packageMap(packageMap),
        frames(plan.frames.size()), versions(plan.versions.size()) {}

  FailureOr<ac::RuleOp> run() {
    auto transaction = plan.transaction;
    auto staged = cast<ac::SourceTransactionOp>(
        packageMap.lookup(transaction.getOperation()));
    SmallVector<Value> captures;
    SmallVector<Type> types(transaction->getResultTypes());
    for (Value liveIn : prepared.liveIns) {
      Value capture = packageMap.lookupOrNull(liveIn);
      if (!capture)
        return transaction.emitOpError()
               << "transaction live-in mapping is missing";
      captures.push_back(capture);
    }
    for (auto output : prepared.outputs)
      types.push_back(plan.owners[output.owner].payloadType);
    OperationState state(staged.getLoc(), ac::RuleOp::getOperationName());
    state.addOperands(captures);
    state.addTypes(types);
    state.addAttributes(transaction->getAttrs());
    state.addRegion();
    auto rule = cast<ac::RuleOp>(Operation::create(state));
    staged->getBlock()->getOperations().insert(staged->getIterator(), rule);
    auto *block = new Block();
    rule.getBody().push_back(block);
    for (Value liveIn : prepared.liveIns)
      base.map(liveIn, block->addArgument(liveIn.getType(), liveIn.getLoc()));
    for (auto &mapping : frames)
      mapping = base;
    for (auto [id, version] : llvm::enumerate(plan.versions))
      if (version.kind == ac::StateVersionKind::OldQ) {
        auto value = bound(version.value);
        if (failed(value))
          return failure();
        versions[id] = *value;
      }
    for (const auto &read : plan.reads)
      reads.try_emplace(Key{read.frame, read.operation}, &read);
    for (const auto &write : plan.writes)
      writes.try_emplace(Key{write.frame, write.operation}, &write);
    for (const auto &call : plan.calls)
      calls.try_emplace(Key{call.caller, call.operation}, &call);
    OpBuilder builder(rule.getContext());
    builder.setInsertionPointToEnd(block);
    if (failed(invocation(plan.rootFrame, builder)))
      return failure();
    SmallVector<Value> results;
    for (const auto &result : plan.results) {
      auto value = bound(result.yielded);
      if (failed(value))
        return failure();
      results.push_back(*value);
    }
    for (auto output : prepared.outputs) {
      if (!versions[output.version])
        return transaction.emitOpError()
               << "transaction final candidate is unmapped";
      results.push_back(versions[output.version]);
    }
    OperationState yield(staged.getLoc(), ac::YieldOp::getOperationName());
    yield.addOperands(results);
    builder.create(yield);
    return rule;
  }

private:
  using Key = std::pair<ac::InvocationFrameId, Operation *>;

  FailureOr<Value> bound(ac::BoundValueId id) {
    const auto &ref = plan.values[id];
    Value result = frames[ref.frame].lookupOrNull(ref.value);
    if (!result)
      return plan.transaction->emitOpError()
             << "transaction invocation SSA mapping is incomplete";
    return result;
  }

  LogicalResult invocation(ac::InvocationFrameId frame, OpBuilder &builder) {
    const auto &binding = plan.frames[frame];
    for (ArrayRef<ac::TransactionValueBinding> arguments :
         {ArrayRef<ac::TransactionValueBinding>(binding.guardArguments),
          ArrayRef<ac::TransactionValueBinding>(binding.bodyArguments)})
      for (const auto &argument : arguments) {
        auto actual = bound(argument.actual);
        if (failed(actual))
          return failure();
        frames[frame].map(argument.formal, *actual);
      }
    auto method = binding.method;
    auto transaction = plan.transaction;
    Operation *function =
        method ? method.getOperation() : transaction.getOperation();
    for (Region &region : function->getRegions())
      for (Operation &op : region.front().without_terminator()) {
        if (auto access = dyn_cast<ac::SourceStateReadOp>(op)) {
          auto found = reads.find({frame, access});
          if (found == reads.end() || !versions[found->second->version])
            return access.emitOpError()
                   << "transaction read version is unmapped";
          frames[frame].map(access.getValue(),
                            versions[found->second->version]);
        } else if (auto intent = dyn_cast<ac::SourceStateWriteOp>(op)) {
          auto found = writes.find({frame, intent});
          if (found == writes.end())
            return intent.emitOpError() << "transaction write plan is missing";
          auto rhs = bound(found->second->value);
          if (failed(rhs))
            return failure();
          versions[found->second->version] = *rhs;
        } else if (auto call = dyn_cast<ac::SourceMethodCallOp>(op)) {
          auto found = calls.find({frame, call});
          if (found == calls.end())
            return call.emitOpError() << "transaction call plan is missing";
          const auto &callee = *found->second;
          if (failed(invocation(callee.callee, builder)))
            return failure();
          for (const auto &result : callee.results) {
            auto value = bound(result.yielded);
            if (failed(value))
              return failure();
            frames[frame].map(plan.values[result.result].value, *value);
          }
        } else {
          for (Value operand : op.getOperands())
            if (!frames[frame].contains(operand))
              return op.emitOpError() << "transaction pure operand is unmapped";
          builder.clone(op, frames[frame]);
        }
      }
    return success();
  }

  const TransactionPreparation &prepared;
  const ac::TransactionStatePlan &plan;
  IRMapping &packageMap;
  IRMapping base;
  SmallVector<IRMapping, 0> frames;
  SmallVector<Value, 0> versions;
  llvm::DenseMap<Key, const ac::StateReadBinding *> reads;
  llvm::DenseMap<Key, const ac::StateWriteIntent *> writes;
  llvm::DenseMap<Key, const ac::TransactionCallBinding *> calls;
};

Value enableConstant(ac::SourceStateOp original, bool enabled,
                     OpBuilder &builder) {
  auto type = cast<ac::BitsType>(original.getClk().getType());
  auto value =
      ac::MathIntAttr::get(builder.getContext(),
                           llvm::APSInt(llvm::APInt(1, enabled ? 1 : 0), true));
  auto tree = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
       builder.getNamedAttr(
           "value",
           builder.getDictionaryAttr(
               {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
                builder.getNamedAttr("value", value)})),
       builder.getNamedAttr("origin", original.getOccurrence()),
       builder.getNamedAttr("location",
                            type.getWidth().getTree().get("location"))});
  OperationState constant(original.getLoc(),
                          ac::BitsConstantOp::getOperationName());
  constant.addTypes(type);
  constant.addAttribute("value",
                        ac::StaticExprAttr::get(builder.getContext(), tree));
  return builder.create(constant)->getResult(0);
}

LogicalResult materialize(LoweringPreparation &prepared, mlir::ModuleOp staged,
                          IRMapping &mapping) {
  Builder attributes(staged.getContext());
  SmallVector<StateStorage> storage;
  llvm::DenseMap<Operation *, Value> nextValues;
  // Allocate one actual storage leaf per source declaration, including owners
  // that are only read or never touched by any transaction.
  for (auto original : prepared.states) {
    auto state =
        cast<ac::SourceStateOp>(mapping.lookup(original.getOperation()));
    OperationState leaf(state.getLoc(), ac::InstanceOp::getOperationName());
    leaf.addTypes(original.getQ().getType());
    leaf.addAttribute("instance_name", original.getInstanceNameAttr());
    leaf.addAttribute("occurrence", original.getOccurrence());
    leaf.addAttribute("callee",
                      FlatSymbolRefAttr::get(staged.getContext(),
                                             prepared.dffe.getSymName()));
    leaf.addAttribute("parameters", attributes.getArrayAttr({}));
    leaf.addAttribute(
        "type_arguments",
        attributes.getArrayAttr({TypeAttr::get(original.getQ().getType())}));
    auto instance = cast<ac::InstanceOp>(Operation::create(leaf));
    state->getBlock()->getOperations().insert(state->getIterator(), instance);
    state.getQ().replaceAllUsesWith(instance.getResult(0));
    mapping.map(original.getQ(), instance.getResult(0));
    storage.push_back({original, instance});
    state.erase();
  }
  struct Replacement {
    ac::SourceTransactionOp original;
    ac::RuleOp rule;
  };
  SmallVector<Replacement> replacements;
  for (const auto &transaction : prepared.transactions) {
    RuleMaterializer builder(transaction, mapping);
    auto rule = builder.run();
    if (failed(rule))
      return failure();
    auto original = transaction.plan->transaction;
    replacements.push_back({original, *rule});
    unsigned ordinary = original.getNumResults();
    for (auto [index, binding] : llvm::enumerate(transaction.outputs))
      nextValues.try_emplace(
          transaction.plan->owners[binding.owner].declaration,
          rule->getResult(ordinary + index));
  }
  // All Rules exist before replacing any ordinary transaction result. Thus a
  // live-in from another transaction never depends on eager erasure order.
  for (auto replacement : replacements) {
    auto original = replacement.original;
    auto transaction =
        cast<ac::SourceTransactionOp>(mapping.lookup(original.getOperation()));
    for (auto [source, old, result] :
         llvm::zip(original.getResults(), transaction.getResults(),
                   replacement.rule.getResults().take_front(
                       original.getNumResults()))) {
      old.replaceAllUsesWith(result);
      mapping.map(source, result);
    }
  }
  for (auto row : storage) {
    auto original = row.declaration;
    OpBuilder builder(row.instance);
    auto found = nextValues.find(original);
    Value enable = enableConstant(original, found != nextValues.end(), builder);
    Value d =
        found == nextValues.end() ? row.instance.getResult(0) : found->second;
    row.instance->setOperands({mapping.lookup(original.getClk()),
                               mapping.lookup(original.getRst()), enable, d,
                               mapping.lookup(original.getInitial())});
  }
  for (auto replacement : replacements) {
    auto original = replacement.original;
    cast<ac::SourceTransactionOp>(mapping.lookup(original.getOperation()))
        .erase();
  }
  // Native verification/summary closure admitted these declarations before
  // construction, including uncalled guarded Methods. No execution occurrence
  // remains; only now may their source bodies disappear.
  for (auto module : staged.getOps<ac::ModuleOp>())
    for (auto method : llvm::make_early_inc_range(
             module.getBody().front().getOps<ac::SourceMethodOp>()))
      method.erase();
  staged->setAttr("ac.stage", attributes.getStringAttr("final"));
  if (failed(mlir::verify(staged)) ||
      failed(ac::detail::verifyNoUnloweredTransactions(staged)) ||
      failed(ac::verifyHardwarePackage(staged)))
    return failure();
  return success();
}

class LowerTransactionsPass
    : public PassWrapper<LowerTransactionsPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerTransactionsPass)
  StringRef getArgument() const final { return "ac-lower-transactions"; }
  StringRef getDescription() const final {
    return "Lower same-owner total transactions to pure Rules and DFFE storage";
  }
  void runOnOperation() final {
    auto input = getOperation();
    if (!ac::detail::findUnloweredTransaction(input)) {
      // Existing ordinary hardware retains its own admission and budgets. No
      // transaction analysis, invocation accounting or clone is introduced.
      if (failed(getAnalysis<ac::HardwareAnalysis>().verify()) ||
          failed(mergedEnvelope(input))) {
        signalPassFailure();
        return;
      }
      input->setAttr("ac.stage", StringAttr::get(input.getContext(), "final"));
      return;
    }
    const auto &analysis = getAnalysis<ac::SourceTransactionStateAnalysis>();
    if (!analysis.isValid()) {
      signalPassFailure();
      return;
    }
    MaterializationBudget budget;
    LoweringPreparation prepared(input, analysis, budget);
    if (failed(prepared.run())) {
      signalPassFailure();
      return;
    }
    IRMapping mapping;
    OwningOpRef<mlir::ModuleOp> staged(
        cast<mlir::ModuleOp>(input->clone(mapping)));
    if (failed(materialize(prepared, *staged, mapping))) {
      signalPassFailure();
      return;
    }
    // Verification is complete. Preserve the pass root and replace contents
    // atomically, with no fallible step after this first original mutation.
    input->setAttrs((*staged)->getAttrs());
    input->getRegion(0).takeBody((*staged)->getRegion(0));
  }
};

} // namespace

std::unique_ptr<Pass> createLowerTransactionsPass() {
  return std::make_unique<LowerTransactionsPass>();
}

} // namespace acir
