#include "HardwareStateLowering.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Transforms/Passes.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir {
namespace {
using ac::detail::SourceCheckBudget;
using detail::knownTrue;

bool charge(SourceCheckBudget &budget, Operation *site, uint64_t units = 1) {
  return succeeded(budget.debit(units, site));
}
} // namespace

namespace detail {
bool prepayOperationHeader(Operation *op, SourceCheckBudget &budget) {
  return charge(budget, op, 3 + uint64_t(op->getNumOperands()) * 2 +
                               uint64_t(op->getNumResults()) * 2 + op->getNumRegions() +
                               op->getRawDictionaryAttrs().size() +
                               op->getPropertiesStorageSize());
}

LogicalResult prepayClone(Operation *root, SourceCheckBudget &budget) {
  SmallVector<Operation *> pending;
  if (!charge(budget, root))
    return failure();
  pending.push_back(root);
  while (!pending.empty()) {
    Operation *op = pending.pop_back_val();
    if (!prepayOperationHeader(op, budget))
      return failure();
    for (Region &region : op->getRegions())
      for (Block &block : region) {
        if (!charge(budget, op, 1 + uint64_t(block.getNumArguments()) * 2))
          return failure();
        for (Operation &nested : block) {
          if (!charge(budget, op))
            return failure();
          pending.push_back(&nested);
        }
      }
  }
  return success();
}

/// Prove an admitted Bits1 SSA value is exactly constant one. Only closed
/// constants and bitwise/compare/select combinations are folded; any unknown
/// operand or unsupported operation is false, never a guess. This decides the
/// plain DFF leaf from the actually materialized commit enable.
bool knownTrue(Value value, const ac::HardwareAnalysis &hardware,
               SourceCheckBudget &budget, unsigned depth) {
  Operation *site = value.getDefiningOp();
  if (!site || depth > 64 || !charge(budget, site)) return false;
  Type type = value.getType();
  if (!isa<ac::BitsType>(type)) return false;
  auto width = hardware.getPackedWidth(type, {}, site);
  if (failed(width) || *width != 1) return false;
  if (auto rule = value.getDefiningOp<ac::RuleOp>()) {
    auto *terminator = rule.getBody().front().getTerminator();
    auto yielded = dyn_cast_or_null<ac::YieldOp>(terminator);
    auto result = dyn_cast<OpResult>(value);
    if (!yielded || !result ||
        result.getResultNumber() >= yielded.getNumOperands())
      return false;
    return knownTrue(yielded.getOperand(result.getResultNumber()), hardware,
                     budget, depth + 1);
  }
  if (auto constant = value.getDefiningOp<ac::BitsConstantOp>()) {
    auto evaluated =
        hardware.evaluateStatic(constant.getValue(), {}, constant);
    if (failed(evaluated)) return false;
    auto integer = dyn_cast<ac::MathIntAttr>(*evaluated);
    if (!integer) return false;
    llvm::APSInt number(integer.getCanonicalValue());
    return number.getBitWidth() == 1 && number.getBoolValue();
  }
  if (auto binary = value.getDefiningOp<ac::BitsBinaryOp>()) {
    StringRef opcode = binary.getOpcode();
    if (opcode == "and")
      return knownTrue(binary.getLhs(), hardware, budget, depth + 1) &&
             knownTrue(binary.getRhs(), hardware, budget, depth + 1);
    if (opcode == "or") {
      bool lhs = knownTrue(binary.getLhs(), hardware, budget, depth + 1);
      if (lhs) return true;
      return knownTrue(binary.getRhs(), hardware, budget, depth + 1);
    }
    return false;
  }
  if (auto compare = value.getDefiningOp<ac::BitsCompareOp>()) {
    auto constant = [&](Value operand) -> std::optional<llvm::APSInt> {
      auto op = operand.getDefiningOp<ac::BitsConstantOp>();
      if (!op) return std::nullopt;
      auto evaluated = hardware.evaluateStatic(op.getValue(), {}, op);
      if (failed(evaluated)) return std::nullopt;
      auto integer = dyn_cast<ac::MathIntAttr>(*evaluated);
      if (!integer) return std::nullopt;
      return llvm::APSInt(integer.getCanonicalValue());
    };
    auto lhs = constant(compare.getLhs());
    auto rhs = constant(compare.getRhs());
    if (!lhs || !rhs) return false;
    StringRef predicate = compare.getPredicate();
    if (predicate == "eq" || predicate == "case_eq") return *lhs == *rhs;
    if (predicate == "ne") return *lhs != *rhs;
    if (predicate == "ult") return lhs->ult(*rhs);
    if (predicate == "ule") return lhs->ule(*rhs);
    if (predicate == "ugt") return lhs->ugt(*rhs);
    if (predicate == "uge") return lhs->uge(*rhs);
    if (predicate == "slt") return lhs->slt(*rhs);
    if (predicate == "sle") return lhs->sle(*rhs);
    if (predicate == "sgt") return lhs->sgt(*rhs);
    if (predicate == "sge") return lhs->sge(*rhs);
    return false;
  }
  if (auto select = value.getDefiningOp<ac::BitsSelectOp>()) {
    if (!knownTrue(select.getCondition(), hardware, budget, depth + 1))
      return false;
    return knownTrue(select.getTrueValue(), hardware, budget, depth + 1);
  }
  return false;
}

} // namespace detail

namespace {
// Physical refinement is a separate transformation of independently verified
// routed IR. None of the source preparation's borrowed records enter this proof.
struct HardwareStatePrimitives {
  ac::ModuleImportOp dff, dffe;
};

FailureOr<HardwareStatePrimitives>
hardwareStatePrimitives(mlir::ModuleOp routed, SourceCheckBudget &budget) {
  ac::HardwareAnalysis analysis(routed);
  HardwareStatePrimitives result;
  unsigned dffeCount = 0;
  for (auto imported : routed.getOps<ac::ModuleImportOp>()) {
    if (!charge(budget, imported) || failed(analysis.verifyImport(imported)))
      return failure();
    StringRef kind = analysis.getPrimitiveKind(imported);
    if (kind.empty())
      return imported.emitOpError()
             << "hardware refinement requires an implementation for an ordinary external module";
    if (kind == "dff" && !result.dff)
      result.dff = imported;
    if (kind == "dffe") {
      ++dffeCount;
      result.dffe = imported;
    }
  }
  bool hasState = false;
  for (auto module : routed.getOps<ac::ModuleOp>())
    for (auto state : module.getBody().front().getOps<ac::SourceStateOp>()) {
      if (!charge(budget, state)) return failure();
      hasState = true;
    }
  if (hasState && (!result.dffe || dffeCount != 1))
    return routed.emitError()
           << "hardware state refinement requires exactly one verified DFFE primitive";
  return result;
}

FailureOr<ac::StateUpdateOp> logicalUpdate(ac::SourceStateOp state,
                                         SourceCheckBudget &budget) {
  ac::StateUpdateOp result;
  for (auto *user : state.getQ().getUsers()) {
    if (!charge(budget, state)) return failure();
    auto update = dyn_cast<ac::StateUpdateOp>(user);
    if (!update || update.getState() != state.getQ()) continue;
    // One operation may use Q in both the state and candidate positions.
    if (result && result != update)
      return state.emitOpError() << "logical state has multiple update bindings";
    result = update;
  }
  if (!result)
    return state.emitOpError() << "logical state has no update binding";
  return result;
}

bool unconditionalScalarState(ac::SourceStateOp state, ac::StateUpdateOp update,
                              const ac::HardwareAnalysis &analysis,
                              SourceCheckBudget &budget) {
  if (isa<ac::ArrayType>(state.getQ().getType())) return false;
  auto rule = update.getIntent().getDefiningOp<ac::RuleOp>();
  return rule && rule->getParentOp() == state->getParentOp() &&
         knownTrue(update.getIntent(), analysis, budget);
}

// Derive lexical correspondence from the two live graphs. A materializer map is
// not accepted: every inserted operation must occupy the exact storage slot,
// and all unaffected operations, regions and SSA edges remain exact clones.
class HardwareStateProof {
public:
  HardwareStateProof(mlir::ModuleOp routed, mlir::ModuleOp candidate,
                     SourceCheckBudget &budget,
                     const HardwareStatePrimitives &primitives)
      : routed(routed), candidate(candidate), budget(budget),
        analysis(routed), primitives(primitives) {}

  LogicalResult run() {
    if (failed(index(routed, candidate))) return failure();
    for (auto [source, target] : operations)
      if (failed(exact(source, target))) return failure();
    for (const auto &pair : states)
      if (failed(stateProof(pair))) return failure();
    return success();
  }

private:
  struct StatePair {
    ac::SourceStateOp source;
    Operation *target;
    SmallVector<ac::ArraySplatOp, 3> splats;
  };
  LogicalResult reject(StringRef message) {
    return routed.emitError() << "hardware state refinement proof: " << message;
  }

  LogicalResult index(Operation *source, Operation *target) {
    if (!charge(budget, routed, 16 + 4 * source->getNumResults()) ||
        source->getNumResults() != target->getNumResults() ||
        source->getNumRegions() != target->getNumRegions())
      return reject("operation/result/region inventory differs");
    operations.push_back({source, target});
    mapping.map(source, target);
    for (auto [from, to] : llvm::zip(source->getResults(), target->getResults()))
      mapping.map(from, to);
    for (auto [from, to] : llvm::zip(source->getRegions(), target->getRegions())) {
      if (from.getBlocks().size() != to.getBlocks().size())
        return reject("region block inventory differs");
      for (auto [left, right] : llvm::zip(from, to)) {
        if (!charge(budget, routed, 4 + 4 * left.getNumArguments()) ||
            left.getNumArguments() != right.getNumArguments())
          return reject("block argument inventory differs");
        mapping.map(&left, &right);
        for (auto [a, b] : llvm::zip(left.getArguments(), right.getArguments())) {
          if (a.getType() != b.getType() || a.getLoc() != b.getLoc())
            return reject("block argument type/location differs");
          mapping.map(a, b);
        }
        auto next = right.begin();
        for (Operation &op : left) {
          if (!charge(budget, &op)) return failure();
          if (isa<ac::StateUpdateOp>(op)) continue;
          if (auto state = dyn_cast<ac::SourceStateOp>(op)) {
            StatePair pair{state, nullptr, {}};
            if (isa<ac::ArrayType>(state.getQ().getType()))
              for (unsigned i = 0; i < 3; ++i) {
                if (next == right.end()) return reject("missing Table control splat");
                auto splat = dyn_cast<ac::ArraySplatOp>(&*next++);
                if (!splat) return reject("Table state control is not a splat");
                pair.splats.push_back(splat);
              }
            if (next == right.end()) return reject("missing physical state");
            pair.target = &*next++;
            if (pair.target->getNumResults() != 1 ||
                pair.target->getNumRegions() != 0 ||
                pair.target->getNumSuccessors() != 0)
              return reject("physical state shape differs");
            if (!charge(budget, state, 24)) return failure();
            mapping.map(state.getOperation(), pair.target);
            mapping.map(state.getQ(), pair.target->getResult(0));
            states.push_back(std::move(pair));
          } else {
            if (next == right.end()) return reject("missing unchanged operation");
            if (failed(index(&op, &*next++))) return failure();
          }
        }
        if (next != right.end()) return reject("extra physical operation");
      }
    }
    return success();
  }

  LogicalResult exact(Operation *source, Operation *target) {
    if (!charge(budget, source, 8 + 2 * source->getNumOperands() +
                                  source->getAttrs().size()))
      return failure();
    NamedAttrList expected(source->getAttrs());
    if (source == routed.getOperation())
      expected.set("ac.stage", StringAttr::get(source->getContext(), "final"));
    if (source->getName() != target->getName() ||
        expected.getAttrs() != target->getAttrs() ||
        source->getPropertiesAsAttribute() != target->getPropertiesAsAttribute() ||
        source->getLoc() != target->getLoc() ||
        source->getResultTypes() != target->getResultTypes() ||
        source->getNumOperands() != target->getNumOperands() ||
        source->getNumSuccessors() != target->getNumSuccessors())
      return reject("unchanged operation shape/attributes/properties differ");
    for (auto [a, b] : llvm::zip(source->getOperands(), target->getOperands()))
      if (mapping.lookupOrNull(a) != b)
        return reject("unchanged SSA edge differs");
    for (auto [a, b] : llvm::zip(source->getSuccessors(), target->getSuccessors()))
      if (mapping.lookupOrNull(a) != b)
        return reject("unchanged successor differs");
    return success();
  }

  LogicalResult stateProof(const StatePair &pair) {
    auto state = pair.source;
    auto update = logicalUpdate(state, budget);
    if (failed(update)) return failure();
    auto table = dyn_cast<ac::ArrayType>(state.getQ().getType());
    // Re-derive the restricted optimization from actual routed SSA. Neither a
    // producer flag nor the candidate primitive's missing enable is authority.
    bool dff = unconditionalScalarState(state, *update, analysis, budget);
    auto primitive = dff ? primitives.dff : primitives.dffe;
    if (!primitive) return reject("required physical primitive is absent");
    auto *target = pair.target;
    Builder b(routed.getContext());
    NamedAttrList attributes;
    attributes.append("instance_name", state.getInstanceNameAttr());
    attributes.append("occurrence", state.getOccurrence());
    attributes.append("callee", FlatSymbolRefAttr::get(primitive.getSymNameAttr()));
    attributes.append("parameters", b.getArrayAttr({}));
    attributes.append("type_arguments", b.getArrayAttr({TypeAttr::get(
        table ? table.getElementType() : state.getQ().getType())}));
    if (table) attributes.append("shape", table.getShape());
    SmallVector<Value> inputs{mapping.lookup(state.getClk()),
                              mapping.lookup(state.getRst())};
    if (!dff) inputs.push_back(mapping.lookup(update->getIntent()));
    inputs.push_back(mapping.lookup(update->getCandidate()));
    inputs.push_back(mapping.lookup(state.getInitial()));
    if (table) {
      if (pair.splats.size() != 3) return reject("Table splat inventory differs");
      for (unsigned i = 0; i < 3; ++i) {
        auto splat = pair.splats[i];
        auto type = ac::ArrayType::get(state.getContext(), table.getShape(),
                                       inputs[i].getType());
        if (!charge(budget, state, 16) || splat.getInput() != inputs[i] ||
            splat->getNumOperands() != 1 || splat->getNumResults() != 1 ||
            splat.getResult().getType() != type ||
            splat.getShape() != table.getShape() ||
            splat->getAttrs() != b.getDictionaryAttr(
                {b.getNamedAttr("shape", table.getShape())}).getValue() ||
            splat.getLoc() != state.getLoc())
          return reject("Table control splat differs");
        inputs[i] = splat.getResult();
      }
    }
    if (target->getName().getStringRef() !=
            (table ? ac::CollectionOp::getOperationName()
                   : ac::InstanceOp::getOperationName()) ||
        target->getAttrDictionary() != attributes.getDictionary(state.getContext()))
      return reject("physical allocation kind/identity/type arguments differ");
    if (target->getResult(0).getType() != state.getQ().getType() ||
        target->getLoc() != state.getLoc())
      return reject("physical allocation payload type/location differs");
    if (!llvm::equal(target->getOperands(), inputs))
      return reject("physical allocation domain/initial/candidate/intent differs");
    return success();
  }

  mlir::ModuleOp routed, candidate;
  SourceCheckBudget &budget;
  ac::HardwareAnalysis analysis;
  const HardwareStatePrimitives &primitives;
  IRMapping mapping;
  SmallVector<std::pair<Operation *, Operation *>, 0> operations;
  SmallVector<StatePair, 0> states;
};

} // namespace

namespace detail {
FailureOr<OwningOpRef<mlir::ModuleOp>> materializeHardwareState(
    mlir::ModuleOp routed, SourceCheckBudget &budget) {
  budget.setPhase("routed hardware state preparation");
  if (failed(ac::verifyRoutedPackage(routed)) ||
      failed(prepayClone(routed, budget)))
    return failure();
  auto primitives = hardwareStatePrimitives(routed, budget);
  if (failed(primitives)) return failure();
  // Pay every inferred allocation and all three Table control splats before
  // cloning or constructing physical state. Width/layout is already verified.
  for (auto module : routed.getOps<ac::ModuleOp>())
    for (auto state : module.getBody().front().getOps<ac::SourceStateOp>())
      if (!charge(budget, state, 128)) return failure();
  OwningOpRef<mlir::ModuleOp> staged = cast<mlir::ModuleOp>(routed->clone());
  ac::HardwareAnalysis analysis(*staged);
  Builder attributes(routed.getContext());
  for (auto module : staged->getOps<ac::ModuleOp>())
    for (auto state : llvm::make_early_inc_range(
             module.getBody().front().getOps<ac::SourceStateOp>())) {
      auto update = logicalUpdate(state, budget);
      if (failed(update)) return failure();
      bool dff = unconditionalScalarState(state, *update, analysis, budget);
      auto primitive = dff ? primitives->dff : primitives->dffe;
      if (!primitive)
        return state.emitOpError()
               << "unconditional hardware state requires the verified DFF primitive";
      auto table = dyn_cast<ac::ArrayType>(state.getQ().getType());
      OperationState leaf(state.getLoc(), table ? ac::CollectionOp::getOperationName()
                                                : ac::InstanceOp::getOperationName());
      leaf.addTypes(state.getQ().getType());
      leaf.addAttribute("instance_name", state.getInstanceNameAttr());
      leaf.addAttribute("occurrence", state.getOccurrence());
      leaf.addAttribute("callee", FlatSymbolRefAttr::get(primitive.getSymNameAttr()));
      leaf.addAttribute("parameters", attributes.getArrayAttr({}));
      leaf.addAttribute("type_arguments", attributes.getArrayAttr({TypeAttr::get(
          table ? table.getElementType() : state.getQ().getType())}));
      if (table) leaf.addAttribute("shape", table.getShape());
      OpBuilder builder(state);
      // Establish Q first so a hold candidate or forward dependency is rebound
      // through ordinary SSA use replacement, without dangling copied Values.
      auto *storage = builder.create(leaf);
      state.getQ().replaceAllUsesWith(storage->getResult(0));
      SmallVector<Value> inputs{state.getClk(), state.getRst()};
      if (!dff) inputs.push_back(update->getIntent());
      inputs.push_back(update->getCandidate());
      inputs.push_back(state.getInitial());
      if (table)
        for (unsigned i = 0; i < 3; ++i) {
          OperationState splat(state.getLoc(), ac::ArraySplatOp::getOperationName());
          splat.addOperands(inputs[i]);
          splat.addTypes(ac::ArrayType::get(state.getContext(), table.getShape(),
                                            inputs[i].getType()));
          splat.addAttribute("shape", table.getShape());
          OpBuilder before(storage);
          inputs[i] = before.create(splat)->getResult(0);
        }
      storage->setOperands(inputs);
      update->erase();
      state.erase();
    }
  staged->getOperation()->setAttr("ac.stage", attributes.getStringAttr("final"));
  return std::move(staged);
}

LogicalResult verifyHardwareStateMaterialization(
    mlir::ModuleOp routed, mlir::ModuleOp candidate, SourceCheckBudget &budget) {
  budget.setPhase("routed hardware state correspondence proof");
  if (!candidate || failed(ac::verifyRoutedPackage(routed))) return failure();
  auto primitives = hardwareStatePrimitives(routed, budget);
  if (failed(primitives) ||
      failed(HardwareStateProof(routed, candidate, budget, *primitives).run()))
    return failure();
  return ac::verifyHardwarePackage(candidate);
}
} // namespace detail

namespace {
class LowerTransactionsPass
    : public PassWrapper<LowerTransactionsPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerTransactionsPass)
  StringRef getArgument() const final { return "ac-lower-transactions"; }
  StringRef getDescription() const final {
    return "Refine verified routed logical state to final hardware storage";
  }
  void runOnOperation() final {
    auto input = getOperation();
    auto stage = input->getAttrOfType<StringAttr>("ac.stage");
    if (stage && stage.getValue() == "final") {
      if (failed(ac::verifyHardwarePackage(input))) signalPassFailure();
      return;
    }
    if (!stage || stage.getValue() != "routed") {
      input.emitError() << "hardware state lowering requires routed ACIR; run ac-route-transactions first";
      signalPassFailure();
      return;
    }
    ac::HardwareSourceCheckLimits limits;
    // In valid routed IR the only unlowered operations are logical state and
    // its update. State-free graphs retain ordinary hardware limits through
    // clone, exact correspondence proof and final verification in this phase,
    // just as they do through the routing envelope transition above.
    if (ac::detail::findUnloweredTransaction(input))
      limits.maxWorkUnits = 1024 * 1024;
    SourceCheckBudget budget(limits);
    auto candidate = detail::materializeHardwareState(input, budget);
    if (failed(candidate) || failed(detail::verifyHardwareStateMaterialization(
                                 input, **candidate, budget))) {
      signalPassFailure();
      return;
    }
    // All verification finishes before the first input mutation.
    input->setAttrs((**candidate)->getAttrs());
    input->getRegion(0).takeBody((**candidate)->getRegion(0));
  }
};

} // namespace

std::unique_ptr<Pass> createLowerTransactionsPass() {
  return std::make_unique<LowerTransactionsPass>();
}

} // namespace acir
