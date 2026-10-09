#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Transforms/Passes.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/Dominance.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"

using namespace mlir;

namespace acir {
namespace {

using FieldOrdinals = llvm::StringMap<llvm::StringMap<unsigned>>;

// Only identity extracts created during this invocation enter the cache. The
// rewriter listener keeps its keys valid when forwarding changes an input or
// the bounded greedy driver removes an unused extraction.
class RecordForwardingState : public RewriterBase::Listener {
public:
  RecordForwardingState(mlir::ModuleOp input, const FieldOrdinals &fields)
      : fields(fields), dominance(input) {}

  LogicalResult rewrite(ac::StructGetOp get, PatternRewriter &rewriter) {
    if (failedRewrite)
      return failure();
    auto create = get.getValue().getDefiningOp<ac::StructCreateOp>();
    if (!create)
      return failure();
    auto nominal = create.getResult().getType();
    auto declaration = fields.find(nominal.getName().getValue());
    if (declaration == fields.end())
      return reject(get, "cannot resolve nominal struct declaration");
    auto ordinal = declaration->second.find(get.getField());
    if (ordinal == declaration->second.end() ||
        ordinal->second >= create.getValues().size())
      return reject(get, "cannot resolve declared struct field");
    Value selected = create.getValues()[ordinal->second];
    Type resultType = get.getResult().getType();
    if (!ac::areEquivalentHardwareTypes(selected.getType(), resultType))
      return reject(get, "record forwarding hardware type mismatch");
    if (!available(selected, get))
      return reject(get, "record field is unavailable at projection");

    Value replacement = selected;
    if (selected.getType() != resultType) {
      auto inputBits = dyn_cast<ac::BitsType>(selected.getType());
      auto outputBits = dyn_cast<ac::BitsType>(resultType);
      if (!inputBits || !outputBits)
        return reject(get,
                      "record forwarding requires exact nominal field types");
      replacement = findExtract(selected, resultType, get);
      if (!replacement) {
        auto width = outputBits.getWidth().getTree();
        auto origin = width.getAs<DictionaryAttr>("origin");
        auto location = width.getAs<DictionaryAttr>("location");
        if (!origin || !location)
          return reject(get,
                        "record forwarding requires declared width provenance");
        auto zero = ac::StaticExprAttr::get(
            get.getContext(),
            rewriter.getDictionaryAttr(
                {rewriter.getNamedAttr("kind",
                                       rewriter.getStringAttr("literal")),
                 rewriter.getNamedAttr(
                     "value",
                     rewriter.getDictionaryAttr(
                         {rewriter.getNamedAttr(
                              "kind", rewriter.getStringAttr("integer")),
                          rewriter.getNamedAttr(
                              "value", ac::MathIntAttr::get(
                                           get.getContext(),
                                           llvm::APSInt::getUnsigned(0)))})),
                 rewriter.getNamedAttr("origin", origin),
                 rewriter.getNamedAttr("location", location)}));
        rewriter.setInsertionPoint(get);
        OperationState state(get.getLoc(),
                             ac::BitsExtractOp::getOperationName());
        state.addOperands(selected);
        state.addTypes(resultType);
        state.addAttribute("low", zero);
        replacement = rewriter.create(state)->getResult(0);
      }
    }
    rewriter.replaceOp(get, replacement);
    return success();
  }

  bool hasFailed() const { return failedRewrite; }

  void notifyOperationInserted(Operation *operation,
                               OpBuilder::InsertPoint) override {
    if (isa<ac::BitsExtractOp>(operation))
      cacheExtract(operation);
  }

  void notifyOperationModified(Operation *operation) override {
    if (createdExtracts.contains(operation)) {
      uncacheExtract(operation);
      cacheExtract(operation);
    }
  }

  void notifyOperationErased(Operation *operation) override {
    uncacheExtract(operation);
  }

private:
  using ExtractKey = std::pair<Value, Type>;

  LogicalResult reject(ac::StructGetOp get, StringRef message) {
    get.emitOpError() << message;
    failedRewrite = true;
    return failure();
  }

  bool available(Value value, ac::StructGetOp get) {
    // DominanceInfo handles graph regions without imposing textual order, and
    // ordinary SSA regions with their actual ordering. Replacements must be
    // available to every existing user of the validated projection.
    return dominance.properlyDominates(value, get) &&
           llvm::all_of(get.getResult().getUsers(), [&](Operation *user) {
             return dominance.properlyDominates(value, user);
           });
  }

  Value findExtract(Value input, Type output, ac::StructGetOp get) {
    auto found = extracts.find({input, output});
    if (found == extracts.end())
      return {};
    for (Operation *candidate : found->second)
      if (candidate->getParentRegion() == get->getParentRegion() &&
          available(candidate->getResult(0), get))
        return candidate->getResult(0);
    return {};
  }

  void cacheExtract(Operation *operation) {
    ExtractKey key{operation->getOperand(0), operation->getResult(0).getType()};
    createdExtracts[operation] = key;
    extracts[key].push_back(operation);
  }

  void uncacheExtract(Operation *operation) {
    auto saved = createdExtracts.find(operation);
    if (saved == createdExtracts.end())
      return;
    // Modified notifications occur after RAUW, so the old key must be retained
    // separately rather than reconstructed from the operation's new operand.
    auto found = extracts.find(saved->second);
    llvm::erase(found->second, operation);
    if (found->second.empty())
      extracts.erase(found);
    createdExtracts.erase(saved);
  }

  const FieldOrdinals &fields;
  DominanceInfo dominance;
  bool failedRewrite = false;
  llvm::DenseMap<Operation *, ExtractKey> createdExtracts;
  llvm::DenseMap<ExtractKey, SmallVector<Operation *>> extracts;
};

class ForwardRecordFieldPattern : public OpRewritePattern<ac::StructGetOp> {
public:
  ForwardRecordFieldPattern(MLIRContext *context, RecordForwardingState &state)
      : OpRewritePattern(context, 1), state(state) {}

  LogicalResult matchAndRewrite(ac::StructGetOp get,
                                PatternRewriter &rewriter) const override {
    return state.rewrite(get, rewriter);
  }

private:
  RecordForwardingState &state;
};

class SimplifyRecordWiresPass
    : public PassWrapper<SimplifyRecordWiresPass,
                         OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(SimplifyRecordWiresPass)
  StringRef getArgument() const final {
    return "ac-simplify-record-wires";
  }
  StringRef getDescription() const final {
    return "Forward constructed record fields and remove unused record wires";
  }

  void runOnOperation() final {
    mlir::ModuleOp input = getOperation();
    auto emitError = [&] { return input.emitError(); };
    if (failed(ac::verifySourceBodyStructure(input, emitError))) {
      signalPassFailure();
      return;
    }

    llvm::StringMap<ArrayAttr> summaries;
    FieldOrdinals fields;
    SmallVector<ac::ModuleOp> definitions;
    {
      // Finish every original definition's analysis before the first rewrite.
      // In particular, removable record transport must not hide dead cycles.
      ac::HardwareAnalysis original(input);
      definitions = original.getDefinitions();
      for (ac::ModuleOp definition : definitions) {
        ac::HardwareBindings bindings;
        bindings.owner = definition;
        auto facts = original.analyzeModule(definition, bindings);
        if (failed(facts)) {
          signalPassFailure();
          return;
        }
        summaries[definition.getSymName()] = ac::serializeOutputDependencies(
            input.getContext(), facts->dependencies);
      }
      for (ac::StructOp declaration : original.getStructs()) {
        auto &ordinals = fields[declaration.getSymName()];
        for (auto [index, raw] : llvm::enumerate(declaration.getFields()))
          ordinals
              [cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue()] =
                  index;
      }
    }

    SmallVector<Operation *> seeds;
    for (ac::ModuleOp definition : definitions)
      definition.walk([&](Operation *operation) {
        if (isa<ac::StructGetOp, ac::StructCreateOp>(operation))
          seeds.push_back(operation);
      });
    RecordForwardingState state(input, fields);
    RewritePatternSet patterns(input.getContext());
    patterns.add<ForwardRecordFieldPattern>(input.getContext(), state);
    GreedyRewriteConfig config;
    config.setScope(&input->getRegion(0))
        .setStrictness(GreedyRewriteStrictness::ExistingAndNewOps)
        .setListener(&state)
        .enableFolding(false)
        .enableConstantCSE(false)
        .setRegionSimplificationLevel(GreedySimplifyRegionLevel::Disabled)
        .setMaxNumRewrites(GreedyRewriteConfig::kNoLimit);
    // The multi-op entry point processes only these seeds and newly created
    // identity extracts. Its dead-op cleanup cannot reach original scalars,
    // rules, effects or instances, and it does not simplify regions or blocks.
    if (failed(applyOpPatternsGreedily(seeds, std::move(patterns), config)) ||
        state.hasFailed() ||
        failed(ac::verifySourceBodyStructure(input, emitError))) {
      signalPassFailure();
      return;
    }

    // No SSA-dependent analysis from the original body survives mutation.
    ac::HardwareAnalysis rewritten(input);
    for (ac::ModuleOp definition : rewritten.getDefinitions()) {
      ac::HardwareBindings bindings;
      bindings.owner = definition;
      auto facts = rewritten.analyzeModule(definition, bindings);
      if (failed(facts)) {
        signalPassFailure();
        return;
      }
      if (ac::serializeOutputDependencies(input.getContext(),
                                          facts->dependencies) !=
          summaries.lookup(definition.getSymName())) {
        definition.emitOpError()
            << "record simplification changed canonical output dependencies";
        signalPassFailure();
        return;
      }
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createSimplifyRecordWiresPass() {
  return std::make_unique<SimplifyRecordWiresPass>();
}

} // namespace acir
