#include "pyc/Transforms/Passes.h"
#include "pyc/Transforms/SourceProvenance.h"

#include "mlir/IR/Dominance.h"
#include "mlir/IR/Location.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/CSE.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"

using namespace mlir;

namespace pyc {
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

Location mergeSourceLocationsImpl(Location retained, Location removed) {
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

class SourceAwareCSEPass final
    : public PassWrapper<SourceAwareCSEPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(SourceAwareCSEPass)

  StringRef getArgument() const override { return "pyc-source-aware-cse"; }
  StringRef getDescription() const override {
    return "Eliminate common PYC expressions while fusing source locations";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    SourceLocationRewriteListener listener;
    IRRewriter rewriter(module.getContext(), &listener);
    DominanceInfo dominance(module);
    eliminateCommonSubExpressions(rewriter, dominance, module);
  }
};

} // namespace

Location mergeSourceLocations(Location retained, Location removed) {
  return mergeSourceLocationsImpl(retained, removed);
}

void SourceLocationRewriteListener::notifyOperationReplaced(
    Operation *operation, ValueRange replacements) {
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

std::unique_ptr<Pass> createSourceAwareCSEPass() {
  return std::make_unique<SourceAwareCSEPass>();
}

static PassRegistration<SourceAwareCSEPass> sourceAwareCSEPass;

} // namespace pyc
