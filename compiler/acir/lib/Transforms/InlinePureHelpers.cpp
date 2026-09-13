#include "acir/Transforms/Passes.h"

#include "Analysis/ModelAnalysisInternal.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Dominance.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Transforms/CSE.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/ADT/SmallVector.h"

using namespace mlir;

namespace acir {
namespace {

Location pythonSourceLocation(Location location) {
  if (auto file = dyn_cast<FileLineColLoc>(location))
    return file.getFilename().getValue().ends_with(".py")
               ? location
               : UnknownLoc::get(location.getContext());
  if (auto named = dyn_cast<NameLoc>(location)) {
    Location child = pythonSourceLocation(named.getChildLoc());
    return isa<UnknownLoc>(child)
               ? child
               : Location(NameLoc::get(named.getName(), child));
  }
  if (auto call = dyn_cast<CallSiteLoc>(location)) {
    Location callee = pythonSourceLocation(call.getCallee());
    Location caller = pythonSourceLocation(call.getCaller());
    if (isa<UnknownLoc>(callee))
      return caller;
    if (isa<UnknownLoc>(caller))
      return callee;
    return CallSiteLoc::get(callee, caller);
  }
  if (auto fused = dyn_cast<FusedLoc>(location)) {
    SmallVector<Location> children;
    for (Location child : fused.getLocations()) {
      child = pythonSourceLocation(child);
      if (!isa<UnknownLoc>(child) && !llvm::is_contained(children, child))
        children.push_back(child);
    }
    if (children.empty())
      return UnknownLoc::get(location.getContext());
    if (children.size() == 1)
      return children.front();
    return FusedLoc::get(location.getContext(), children);
  }
  return UnknownLoc::get(location.getContext());
}

Location mergeSourceLocations(Location retained, Location removed) {
  Location retainedSource = pythonSourceLocation(retained);
  Location removedSource = pythonSourceLocation(removed);
  if (isa<UnknownLoc>(removedSource))
    return retained;
  if (isa<UnknownLoc>(retainedSource))
    return removedSource;
  if (retainedSource == removedSource)
    return retainedSource;
  return FusedLoc::get(retained.getContext(),
                       {retainedSource, removedSource});
}

class SourceLocationCSEListener final : public RewriterBase::Listener {
public:
  void notifyOperationReplaced(Operation *operation,
                               ValueRange replacements) override {
    SmallVector<Operation *> producers;
    SmallVector<BlockArgument> arguments;
    for (Value replacement : replacements) {
      if (Operation *producer = replacement.getDefiningOp()) {
        if (!llvm::is_contained(producers, producer))
          producers.push_back(producer);
      } else if (auto argument = dyn_cast<BlockArgument>(replacement)) {
        if (!llvm::is_contained(arguments, argument))
          arguments.push_back(argument);
      }
    }
    for (Operation *producer : producers)
      producer->setLoc(
          mergeSourceLocations(producer->getLoc(), operation->getLoc()));
    for (BlockArgument argument : arguments)
      argument.setLoc(
          mergeSourceLocations(argument.getLoc(), operation->getLoc()));
  }
};

SourceLocationCSEListener &sharedSourceLocationRewriteListener() {
  static SourceLocationCSEListener listener;
  return listener;
}

void eliminateCommonSubExpressionsWithSourceLocations(ModuleOp module) {
  SourceLocationCSEListener listener;
  IRRewriter rewriter(module.getContext(), &listener);
  DominanceInfo dominance(module);
  eliminateCommonSubExpressions(rewriter, dominance, module);
}

class SourceAwareCSEPass final
    : public PassWrapper<SourceAwareCSEPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(SourceAwareCSEPass)

  StringRef getArgument() const override { return "ac-source-aware-cse"; }
  StringRef getDescription() const override {
    return "Eliminate common ACIR expressions while fusing source locations";
  }

  void runOnOperation() override {
    eliminateCommonSubExpressionsWithSourceLocations(getOperation());
  }
};
#define GEN_PASS_DEF_INLINEPUREHELPERSPASS
#include "acir/Transforms/Passes.h.inc"

struct InlinePureHelpersPass
    : impl::InlinePureHelpersPassBase<InlinePureHelpersPass> {
  void runOnOperation() override {
    ModuleOp module = getOperation();
    if (failed(detail::validatePureQueueCallGraph(module))) {
      signalPassFailure();
      return;
    }
    SymbolTable symbols(module);
    bool changed = true;
    while (changed) {
      changed = false;
      SmallVector<func::CallOp> calls;
      module.walk([&](func::CallOp call) { calls.push_back(call); });
      for (func::CallOp call : calls) {
        auto callee = symbols.lookup<func::FuncOp>(call.getCallee());
        if (!callee || !callee->hasAttrOfType<BoolAttr>("ac.inline") ||
            !callee->getAttrOfType<BoolAttr>("ac.inline").getValue())
          continue;
        if (callee.isExternal() || !callee.getBody().hasOneBlock()) {
          call.emitOpError(
              "ac.inline helper must resolve to one non-external block");
          signalPassFailure();
          return;
        }
        Block &body = callee.getBody().front();
        auto returned = dyn_cast<func::ReturnOp>(body.getTerminator());
        if (!returned || body.getNumArguments() != call.getNumOperands() ||
            returned.getNumOperands() != call.getNumResults()) {
          call.emitOpError("ac.inline helper signature is malformed");
          signalPassFailure();
          return;
        }
        IRMapping mapping;
        for (auto [argument, operand] :
             llvm::zip_equal(body.getArguments(), call.getOperands()))
          mapping.map(argument, operand);
        OpBuilder builder(call);
        for (Operation &operation : body.without_terminator()) {
          Operation *cloned = builder.clone(operation, mapping);
          cloned->walk([&](Operation *nested) {
            nested->setLoc(
                CallSiteLoc::get(nested->getLoc(), call.getLoc()));
            for (Region &region : nested->getRegions())
              for (Block &block : region)
                for (BlockArgument argument : block.getArguments())
                  argument.setLoc(
                      CallSiteLoc::get(argument.getLoc(), call.getLoc()));
          });
        }
        Location helperResultLocation =
            mergeSourceLocations(
                CallSiteLoc::get(callee.getLoc(), call.getLoc()), call.getLoc());
        for (auto [result, value] :
             llvm::zip_equal(call.getResults(), returned.getOperands())) {
          Value replacement = mapping.lookupOrDefault(value);
          SmallVector<Operation *> users;
          for (OpOperand &use : result.getUses())
            if (!llvm::is_contained(users, use.getOwner()))
              users.push_back(use.getOwner());
          for (Operation *user : users)
            user->setLoc(
                mergeSourceLocations(user->getLoc(), helperResultLocation));
          if (auto argument = dyn_cast<BlockArgument>(replacement))
            argument.setLoc(mergeSourceLocations(argument.getLoc(),
                                                  helperResultLocation));
          result.replaceAllUsesWith(replacement);
        }
        call.erase();
        changed = true;
      }
    }

    SmallVector<func::FuncOp> dead;
    for (func::FuncOp function : module.getOps<func::FuncOp>())
      if (auto marker = function->getAttrOfType<BoolAttr>("ac.inline");
          marker && marker.getValue() && function->use_empty())
        dead.push_back(function);
    for (func::FuncOp function : dead)
      function.erase();
    eliminateCommonSubExpressionsWithSourceLocations(module);
  }
};
} // namespace

std::unique_ptr<Pass> createInlinePureHelpersPass() {
  return std::make_unique<InlinePureHelpersPass>();
}

std::unique_ptr<Pass> createSourceAwareCSEPass() {
  return std::make_unique<SourceAwareCSEPass>();
}

std::unique_ptr<Pass> createSourceAwareCanonicalizerPass() {
  GreedyRewriteConfig config;
  config.setListener(&sharedSourceLocationRewriteListener());
  return createCanonicalizerPass(config);
}

static PassRegistration<SourceAwareCSEPass> sourceAwareCSEPass;

} // namespace acir
