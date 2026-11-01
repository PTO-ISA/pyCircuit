#include "pycircuit/Transforms/ExpandStaticFor.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/AttrTypeSubElements.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/Pass/Pass.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/xxhash.h"

using namespace mlir;
namespace acir {
namespace {
namespace ac = acir::ac;

struct Expander {
  ac::HardwareAnalysis &hardware;
  const ac::HardwareBindings &bindings;
  ac::TransactionalDebit debit;
  uint64_t remaining = ac::HardwareSourceCheckLimits{}.maxWorkUnits;

  LogicalResult charge(Operation *site, uint64_t units = 1) {
    if (units > remaining)
      return site->emitOpError("static for expansion budget exceeded");
    remaining -= units;
    return debit ? debit(units, site) : success();
  }
  FailureOr<llvm::APInt> bound(Value value) {
    if (auto constant = value.getDefiningOp<arith::ConstantIndexOp>())
      return llvm::APInt(64, constant.value(), true);
    auto bridge = value.getDefiningOp<ac::SourceStaticIndexOp>();
    if (!bridge)
      return emitError(value.getLoc(),
                       "static for requires a static Integer bound");
    if (failed(charge(bridge)))
      return failure();
    auto evaluated =
        hardware.evaluateStatic(bridge.getValue(), bindings, bridge);
    auto integer = succeeded(evaluated) ? dyn_cast<ac::MathIntAttr>(*evaluated)
                                        : ac::MathIntAttr();
    if (!integer)
      return bridge.emitOpError("static for bound must be an Integer");
    llvm::APSInt result(integer.getCanonicalValue());
    if (!result.isRepresentableByInt64())
      return bridge.emitOpError(
          "static for bound exceeds index representation");
    return llvm::APInt(64, result.getExtValue(), true);
  }
  ac::StaticExprAttr literal(ac::StaticExprAttr expression,
                             const llvm::APInt &value) {
    Builder builder(expression.getContext());
    NamedAttrList tree(expression.getTree());
    tree.set("kind", builder.getStringAttr("literal"));
    tree.erase("ref");
    tree.set(
        "value",
        builder.getDictionaryAttr(
            {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
             builder.getNamedAttr(
                 "value", ac::MathIntAttr::get(builder.getContext(),
                                               llvm::APSInt(value, false)))}));
    return ac::StaticExprAttr::get(builder.getContext(),
                                   tree.getDictionary(builder.getContext()));
  }
  DictionaryAttr frame(DictionaryAttr origin, uint64_t ordinal,
                       const llvm::APInt &value) {
    Builder builder(origin.getContext());
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("iteration")),
         builder.getNamedAttr("site", origin.getAs<DictionaryAttr>("site")),
         builder.getNamedAttr("ordinal", builder.getI64IntegerAttr(ordinal)),
         builder.getNamedAttr(
             "value", builder.getDictionaryAttr(
                          {builder.getNamedAttr(
                               "kind", builder.getStringAttr("integer")),
                           builder.getNamedAttr(
                               "value", ac::MathIntAttr::get(
                                            builder.getContext(),
                                            llvm::APSInt(value, false)))}))});
  }
  LogicalResult specialize(Operation *copy, DictionaryAttr origin,
                           uint64_t ordinal, const llvm::APInt &value) {
    // Substitute identities before adding this iteration to provenance. This
    // also rewrites StaticExprs embedded in logical types and nested bounds.
    AttrTypeReplacer substitution;
    substitution.addReplacement(
        [&](ac::StaticExprAttr expression) -> Attribute {
          auto tree = expression.getTree();
          auto reference = tree.getAs<DictionaryAttr>("ref");
          if (tree.getAs<StringAttr>("kind").getValue() == "reference" &&
              reference &&
              reference.getAs<StringAttr>("kind").getValue() == "induction" &&
              reference.getAs<DictionaryAttr>("loop") == origin)
            return literal(expression, value);
          return expression;
        });
    substitution.recursivelyReplaceElementsIn(copy, true, false, true);
    auto iteration = frame(origin, ordinal, value);
    unsigned prefix = origin.getAs<ArrayAttr>("expansion").size();
    AttrTypeReplacer provenance;
    provenance.addReplacement([&](DictionaryAttr dictionary) -> Attribute {
      auto expansion = dictionary.getAs<ArrayAttr>("expansion");
      if (!dictionary.getAs<DictionaryAttr>("site") || !expansion)
        return dictionary;
      SmallVector<Attribute> frames(expansion.begin(), expansion.end());
      if (prefix <= frames.size())
        frames.insert(frames.begin() + prefix, iteration);
      NamedAttrList fields(dictionary);
      fields.set("expansion", ArrayAttr::get(copy->getContext(), frames));
      return fields.getDictionary(copy->getContext());
    });
    provenance.recursivelyReplaceElementsIn(copy, true, false, true);
    return success();
  }
  LogicalResult materializeIndex(ac::SourceIndexOp bridge) {
    auto value = bound(bridge.getInput());
    if (failed(value))
      return failure();
    OpBuilder builder(bridge);
    OperationState state(bridge.getLoc(),
                         ac::PendingExprOp::getOperationName());
    state.addTypes(bridge.getResult().getType());
    state.addAttribute("kind", builder.getStringAttr("literal"));
    state.addAttribute("number",
                       ac::MathIntAttr::get(builder.getContext(),
                                            llvm::APSInt(*value, false)));
    if (auto origin = bridge->getAttr("ac.origin"))
      state.addAttribute("ac.origin", origin);
    Value replacement = builder.create(state)->getResult(0);
    bridge.getResult().replaceAllUsesWith(replacement);
    bridge.erase();
    return success();
  }
  LogicalResult loop(scf::ForOp operation) {
    if (!operation.getRegion().hasOneBlock() || operation.getUnsignedCmp())
      return operation.emitOpError(
          "static for requires one signed index block");
    auto origin = operation->getAttrOfType<DictionaryAttr>("occurrence");
    if (failed(ac::detail::verifyOccurrence(
            origin, [&] { return operation.emitOpError(); })))
      return failure();
    auto lower = bound(operation.getLowerBound());
    auto upper = bound(operation.getUpperBound());
    auto step = bound(operation.getStep());
    if (failed(lower) || failed(upper) || failed(step))
      return failure();
    if (step->isNegative() || step->isZero())
      return operation.emitOpError("static for requires a positive step");
    auto lo = lower->sext(128), hi = upper->sext(128),
         increment = step->sext(128);
    llvm::APInt count(128, 0);
    if (lo.slt(hi))
      count = (hi - lo + increment - 1).udiv(increment);
    if (count.getActiveBits() > 64 || count.getZExtValue() > remaining)
      return operation.emitOpError("static for expansion budget exceeded");
    uint64_t trips = count.getZExtValue(), size = 1;
    operation.getRegion().walk([&](Operation *) { ++size; });
    if (trips > remaining / size)
      return operation.emitOpError("static for expansion budget exceeded");
    SmallVector<Value> carried(operation.getInitArgs());
    Block &source = operation.getRegion().front();
    auto yield = dyn_cast<scf::YieldOp>(source.getTerminator());
    if (!yield)
      return operation.emitOpError("static for body requires scf.yield");
    std::string identity;
    llvm::raw_string_ostream(identity) << origin;
    std::string suffix = "__for_" + std::to_string(llvm::xxHash64(identity));
    for (uint64_t ordinal = 0; ordinal < trips; ++ordinal) {
      if (failed(charge(operation, size)))
        return failure();
      auto index = lo + increment * ordinal;
      if (!index.isSignedIntN(64))
        return operation.emitOpError(
            "static for induction exceeds index representation");
      OpBuilder builder(operation);
      auto constant = arith::ConstantIndexOp::create(
          builder, operation.getLoc(), index.getSExtValue());
      auto staged = scf::ExecuteRegionOp::create(builder, operation.getLoc(),
                                                 operation.getResultTypes());
      auto *block = new Block;
      staged.getRegion().push_back(block);
      OpBuilder inside = OpBuilder::atBlockEnd(block);
      IRMapping mapping;
      mapping.map(source.getArgument(0), constant.getResult());
      for (auto [formal, actual] :
           llvm::zip(source.getArguments().drop_front(), carried))
        mapping.map(formal, actual);
      llvm::StringMap<std::string> renamed;
      for (Operation &nested : source.without_terminator()) {
        Operation *copy = inside.clone(nested, mapping);
        // This conversion no longer joins a loop once its body is flattened.
        // Conversions within a surviving conditional still belong to its phi.
        copy->removeAttr("ac.join_transport");
        if (failed(specialize(copy, origin, ordinal, index.sextOrTrunc(64))))
          return failure();
        copy->walk([&](Operation *allocation) {
          if (isa<ac::InstanceOp, ac::ResourceAllocOp, ac::TableAllocOp,
                  ac::ResourceConnectionOp>(allocation)) {
            auto name = allocation->getAttrOfType<StringAttr>("instance_name");
            std::string unique =
                name.getValue().str() + suffix + "_" + std::to_string(ordinal);
            renamed[name.getValue()] = unique;
            allocation->setAttr("instance_name", inside.getStringAttr(unique));
          }
        });
      }
      block->walk([&](ac::InstanceRefOp reference) {
        SmallVector<Attribute> path(reference.getInstancePath().begin(),
                                    reference.getInstancePath().end());
        if (!path.empty()) {
          auto found = renamed.find(cast<StringAttr>(path.front()).getValue());
          if (found != renamed.end()) {
            path.front() = inside.getStringAttr(found->second);
            reference->setAttr("instance_path", inside.getArrayAttr(path));
          }
        }
      });
      SmallVector<Value> outputs;
      for (Value output : yield.getOperands())
        outputs.push_back(mapping.lookupOrDefault(output));
      scf::YieldOp::create(inside, operation.getLoc(), outputs);
      if (failed(region(staged.getRegion())))
        return failure();
      auto resultYield = cast<scf::YieldOp>(block->getTerminator());
      carried.assign(resultYield.getOperands().begin(),
                     resultYield.getOperands().end());
      for (Operation &nested :
           llvm::make_early_inc_range(block->without_terminator()))
        nested.moveBefore(operation);
      staged.erase();
    }
    operation.replaceAllUsesWith(carried);
    operation.erase();
    return success();
  }
  LogicalResult staticIf(scf::IfOp operation) {
    auto bridge =
        operation.getCondition().getDefiningOp<ac::SourceStaticPredicateOp>();
    auto evaluated =
        hardware.evaluateStatic(bridge.getValue(), bindings, bridge);
    auto condition =
        succeeded(evaluated) ? dyn_cast<BoolAttr>(*evaluated) : BoolAttr();
    if (!condition)
      return operation.emitOpError("static condition requires a Boolean value");
    Region &selected = condition.getValue() ? operation.getThenRegion()
                                            : operation.getElseRegion();
    if (selected.empty()) {
      if (operation.getNumResults())
        return operation.emitOpError(
            "static branch requires values on its selected path");
      operation.erase();
      return success();
    }
    // Non-selected static paths have no executed index accesses or effects.
    // Source admission has already checked their syntax, purity and structure.
    if (failed(region(selected)))
      return failure();
    auto yield = cast<scf::YieldOp>(selected.front().getTerminator());
    SmallVector<Value> results(yield.getOperands());
    for (Operation &nested :
         llvm::make_early_inc_range(selected.front().without_terminator())) {
      nested.removeAttr("ac.join_transport");
      nested.moveBefore(operation);
    }
    operation.replaceAllUsesWith(results);
    operation.erase();
    return success();
  }
  LogicalResult region(Region &region) {
    for (Block &block : region)
      for (Operation &nested : llvm::make_early_inc_range(block)) {
        if (failed(charge(&nested)))
          return failure();
        if (auto operation = dyn_cast<scf::ForOp>(nested)) {
          if (failed(loop(operation)))
            return failure();
        } else if (auto branch = dyn_cast<scf::IfOp>(nested);
                   branch &&
                   branch.getCondition()
                       .getDefiningOp<ac::SourceStaticPredicateOp>()) {
          if (failed(staticIf(branch)))
            return failure();
        } else if (auto bridge = dyn_cast<ac::SourceIndexOp>(nested)) {
          if (failed(materializeIndex(bridge)))
            return failure();
        } else {
          for (Region &inner : nested.getRegions())
            if (failed(this->region(inner)))
              return failure();
        }
      }
    return success();
  }
  LogicalResult run(Operation *scope) {
    for (Region &body : scope->getRegions())
      if (failed(region(body)))
        return failure();
    // Source loop slots may differ only in type provenance. Static resource
    // connections have no runtime conversion and must remain actual SSA edges.
    SmallVector<ac::PendingExprOp> connections;
    llvm::DenseSet<Operation *> queued;
    auto enqueue = [&](Operation *operation) {
      auto pending = dyn_cast<ac::PendingExprOp>(operation);
      if (pending && pending.getKind() == "convert" &&
          pending.getNumOperands() == 1 && queued.insert(operation).second)
        connections.push_back(pending);
    };
    scope->walk(enqueue);
    while (!connections.empty()) {
      auto connection = connections.pop_back_val();
      queued.erase(connection);
      if (failed(charge(connection)))
        return failure();
      Type source = connection.getOperand(0).getType();
      Type target = connection.getResult().getType();
      if (!isa<ac::StateType, ac::QueueType, ac::PrimitiveType,
               ac::ModuleRefType>(source))
        continue;
      if (!isa<ac::InferType>(target) &&
          !ac::transactionalTypesEqual(source, target))
        return connection.emitOpError(
            "loop-carried references require the same interface type");
      SmallVector<Operation *> users(connection.getResult().getUsers());
      connection.getResult().replaceAllUsesWith(connection.getOperand(0));
      for (Operation *user : users)
        enqueue(user);
      connection.erase();
    }
    // Drop unused source transport and exact induction literals. Their widths
    // require a use context; dead values must not invent a hardware width.
    // Payload-use observations and transaction effects retain their demands.
    auto eraseDead = [&](bool allPending) -> LogicalResult {
      SmallVector<ac::PendingExprOp> deadTransports;
      queued.clear();
      auto deadTransport = [&](Operation *operation) {
        auto pending = dyn_cast_or_null<ac::PendingExprOp>(operation);
        if (pending &&
            (allPending || pending.getKind() == "convert" ||
             pending.getKind() == "literal") &&
            pending->use_empty() && queued.insert(operation).second)
          deadTransports.push_back(pending);
      };
      scope->walk(deadTransport);
      while (!deadTransports.empty()) {
        auto transport = deadTransports.pop_back_val();
        queued.erase(transport);
        if (failed(charge(transport)))
          return failure();
        SmallVector<Operation *> inputs;
        for (Value input : transport.getOperands())
          if (auto *definition = input.getDefiningOp())
            inputs.push_back(definition);
        transport.erase();
        for (Operation *input : inputs)
          deadTransport(input);
      }
      return success();
    };
    if (failed(eraseDead(false)))
      return failure();
    // Capture includes source locals conservatively. An unused callable formal
    // does not read an unassigned local, and must not keep its placeholder
    // live.
    scope->walk<WalkOrder::PostOrder>([&](Operation *callable) {
      if (!isa<ac::TransactionRuleOp, ac::TransactionMethodOp>(callable))
        return;
      auto signature = cast<FunctionType>(
          callable->getAttrOfType<TypeAttr>("function_type").getValue());
      unsigned offset = signature.getNumInputs();
      if (isa<ac::TransactionRuleOp>(callable))
        offset += signature.getNumResults();
      for (unsigned index = callable->getNumOperands(); index-- > 0;) {
        bool unused = llvm::all_of(callable->getRegions(), [&](Region &region) {
          return region.hasOneBlock() &&
                 offset + index < region.front().getNumArguments() &&
                 region.front().getArgument(offset + index).use_empty();
        });
        if (!unused)
          continue;
        for (Region &region : callable->getRegions())
          region.front().eraseArgument(offset + index);
        callable->eraseOperand(index);
      }
    });
    if (failed(eraseDead(false)))
      return failure();
    SmallVector<ac::SourceUnboundOp> unassigned;
    scope->walk([&](ac::SourceUnboundOp operation) {
      unassigned.push_back(operation);
    });
    for (auto value : unassigned) {
      if (!value->use_empty())
        return value.emitOpError(
            "local is not definitely assigned after a zero-iteration loop");
      value.erase();
    }
    // Static aliases can leave unused pure arithmetic with no payload width
    // context. Check definite assignment first, then discard that dead SSA.
    if (failed(eraseDead(true)))
      return failure();
    // Bound bridges are dead after all enclosing loops have been expanded.
    SmallVector<Operation *> dead;
    scope->walk([&](Operation *operation) {
      if (isa<ac::SourceStaticIndexOp, ac::SourceStaticPredicateOp,
              arith::ConstantIndexOp>(operation) &&
          operation->use_empty())
        dead.push_back(operation);
    });
    for (Operation *operation : llvm::reverse(dead))
      operation->erase();
    return success();
  }
};

class ExpandStaticForPass
    : public PassWrapper<ExpandStaticForPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(ExpandStaticForPass)
  StringRef getArgument() const final { return "ac-expand-static-for"; }
  StringRef getDescription() const final {
    return "Expand finite static source loops without changing transaction "
           "boundaries";
  }
  void getDependentDialects(DialectRegistry &registry) const final {
    registry.insert<arith::ArithDialect, scf::SCFDialect>();
  }
  void runOnOperation() final {
    ac::HardwareAnalysis hardware(getOperation());
    uint64_t remaining = ac::HardwareSourceCheckLimits{}.maxWorkUnits;
    auto debit = [&](uint64_t units, Operation *site) -> LogicalResult {
      if (units > remaining)
        return site->emitOpError("static for expansion budget exceeded");
      remaining -= units;
      return success();
    };
    for (Operation &scope : getOperation().getBody()->getOperations()) {
      ac::HardwareBindings bindings;
      if (auto module = dyn_cast<ac::ModuleOp>(scope)) {
        auto defaults =
            hardware.bindRoot(module, DictionaryAttr::get(&getContext()));
        if (failed(defaults)) {
          signalPassFailure();
          return;
        }
        bindings = std::move(*defaults);
      }
      if (failed(expandStaticFor(&scope, hardware, bindings, debit))) {
        signalPassFailure();
        return;
      }
    }
  }
};
} // namespace

LogicalResult expandStaticFor(Operation *scope, ac::HardwareAnalysis &hardware,
                              const ac::HardwareBindings &bindings,
                              ac::TransactionalDebit debit) {
  return Expander{hardware, bindings, std::move(debit)}.run(scope);
}
std::unique_ptr<Pass> createExpandStaticForPass() {
  return std::make_unique<ExpandStaticForPass>();
}
} // namespace acir
