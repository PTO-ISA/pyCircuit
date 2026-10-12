#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "SourceTransactionSummary.h"

#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/SmallPtrSet.h"

using namespace mlir;

namespace acir::ac {
namespace {

enum class PredicateFact { Unknown, False, True };

struct Totality {
  bool total = false;
  Operation *guard = nullptr;
  Operation *call = nullptr;
};

class SummaryBuilder {
public:
  SummaryBuilder(mlir::ModuleOp package,
                 llvm::DenseMap<Operation *, SourceMethodSummary> &methods,
                 llvm::DenseMap<Operation *, bool> &transactions)
      : package(package), hardware(package), methods(methods),
        transactions(transactions) {}

  LogicalResult run() {
    if (failed(mlir::verify(package)))
      return failure();
    SmallVector<SourceMethodOp> declarations;
    SmallVector<SourceTransactionOp> registrations;
    auto scanned = package.walk([&](Operation *operation) {
      if (!charge(operation,
                  1 + operation->getNumOperands() + operation->getNumResults()))
        return WalkResult::interrupt();
      for (Region &region : operation->getRegions())
        for (Block &block : region)
          if (!charge(operation, block.getNumArguments()))
            return WalkResult::interrupt();
      if (auto method = dyn_cast<SourceMethodOp>(operation))
        declarations.push_back(method);
      if (auto transaction = dyn_cast<SourceTransactionOp>(operation))
        registrations.push_back(transaction);
      return WalkResult::advance();
    });
    if (scanned.wasInterrupted())
      return failure();

    for (auto method : declarations) {
      auto &summary = methods[method];
      llvm::SmallPtrSet<Operation *, 8> seen;
      for (auto call : method.getBody().front().getOps<SourceMethodCallOp>()) {
        if (!charge(call))
          return failure();
        auto callee = resolve(call);
        if (!callee)
          return failure();
        if (seen.insert(callee).second)
          summary.callees.push_back(callee);
      }
    }

    // Recursion is a source legality property, including unused definitions
    // and calls on known-false paths. Do not hide it with a totality shortcut.
    llvm::DenseMap<Operation *, unsigned> marks;
    SmallVector<SourceMethodOp> order;
    struct Frame {
      SourceMethodOp method;
      size_t next = 0;
    };
    SmallVector<Frame> stack;
    for (auto root : declarations) {
      if (marks.lookup(root))
        continue;
      marks[root] = 1;
      stack.push_back({root});
      while (!stack.empty()) {
        auto &frame = stack.back();
        if (!charge(frame.method))
          return failure();
        const auto &callees = methods.find(frame.method)->second.callees;
        if (frame.next == callees.size()) {
          marks[frame.method] = 2;
          order.push_back(frame.method);
          stack.pop_back();
          continue;
        }
        auto callee = callees[frame.next++];
        if (marks.lookup(callee) == 1)
          return callee.emitOpError()
                 << "recursive Method call graph is unsupported";
        if (!marks.lookup(callee)) {
          marks[callee] = 1;
          stack.push_back({callee});
        }
      }
    }

    for (auto method : order) {
      SmallVector<PredicateFact> unknown(
          method.getFunctionType().getNumInputs(), PredicateFact::Unknown);
      auto proof = prove(method, unknown, 0);
      if (exhausted)
        return failure();
      methods.find(method)->second.mayBlock = !proof.total;
    }
    for (auto transaction : registrations) {
      auto proof = prove(transaction, {}, 0);
      if (exhausted)
        return failure();
      transactions[transaction] = proof.total;
      if (transaction.getNumResults() && !proof.total) {
        auto diagnostic = transaction.emitOpError();
        diagnostic << "ordinary transaction results require a total "
                      "guard and required Method calls; use -> None "
                      "for a potentially guarded Rule";
        if (proof.call)
          diagnostic.attachNote(proof.call->getLoc())
              << "this required Method call may block";
        if (proof.guard)
          diagnostic.attachNote(proof.guard->getLoc())
              << "guard is not proven statically true";
        return failure();
      }
    }
    return success();
  }

private:
  bool charge(Operation *site, uint64_t units = 1) {
    if (exhausted)
      return false;
    if (units > maxWork - work) {
      site->emitOpError()
          << "source Method summary analysis work budget exceeded";
      exhausted = true;
      return false;
    }
    work += units;
    return true;
  }

  SourceMethodOp resolve(SourceMethodCallOp call) {
    auto method = dyn_cast_or_null<SourceMethodOp>(
        symbols.lookupSymbolIn(package, call.getCalleeAttr()));
    if (!method) {
      call.emitOpError() << "source Method summary requires a resolved Method";
      exhausted = true;
    }
    return method;
  }

  PredicateFact fact(Value value, Operation *callable,
                     ArrayRef<PredicateFact> arguments, unsigned depth) {
    if (!charge(callable ? callable : package.getOperation()))
      return PredicateFact::Unknown;
    if (depth > maxDepth) {
      package.emitOpError() << "source Method summary proof depth exceeded";
      exhausted = true;
      return PredicateFact::Unknown;
    }
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      if (argument.getOwner()->getParentOp() != callable)
        return PredicateFact::Unknown;
      unsigned index = argument.getArgNumber();
      if (auto method = dyn_cast_or_null<SourceMethodOp>(callable)) {
        unsigned formals = method.getFunctionType().getNumInputs();
        if (index < formals)
          return index < arguments.size() ? arguments[index]
                                          : PredicateFact::Unknown;
        return fact(method.getCaptures()[index - formals], nullptr, {},
                    depth + 1);
      }
      if (auto transaction = dyn_cast_or_null<SourceTransactionOp>(callable))
        return fact(transaction.getCaptures()[index], nullptr, {}, depth + 1);
      return PredicateFact::Unknown;
    }
    auto literal = detail::getSourceLiteralPredicate(value, hardware);
    if (failed(literal)) {
      exhausted = true;
      return PredicateFact::Unknown;
    }
    if (*literal)
      return literal->getValue() ? PredicateFact::True : PredicateFact::False;
    return PredicateFact::Unknown;
  }

  Totality prove(Operation *callable, ArrayRef<PredicateFact> arguments,
                 unsigned depth) {
    if (!charge(callable))
      return {};
    if (depth > maxDepth) {
      callable->emitOpError() << "source Method summary proof depth exceeded";
      exhausted = true;
      return {};
    }
    auto guard = cast<YieldOp>(callable->getRegion(0).front().back());
    if (fact(guard.getValues().front(), callable, arguments, 0) !=
        PredicateFact::True)
      return {false, guard, nullptr};
    for (auto call :
         callable->getRegion(1).front().getOps<SourceMethodCallOp>()) {
      if (fact(call.getPath(), callable, arguments, 0) == PredicateFact::False)
        continue;
      auto callee = resolve(call);
      if (!callee)
        return {};
      auto summary = methods.find(callee);
      if (summary != methods.end() && !summary->second.mayBlock)
        continue;
      SmallVector<PredicateFact> actuals;
      for (Value value : call.getArguments())
        actuals.push_back(fact(value, callable, arguments, 0));
      auto proof = prove(callee, actuals, depth + 1);
      if (!proof.total) {
        if (!proof.call)
          proof.call = call;
        return proof;
      }
    }
    return {true, nullptr, nullptr};
  }

  static constexpr uint64_t maxWork = 1024 * 1024;
  static constexpr unsigned maxDepth = 128;
  mlir::ModuleOp package;
  HardwareAnalysis hardware;
  SymbolTableCollection symbols;
  llvm::DenseMap<Operation *, SourceMethodSummary> &methods;
  llvm::DenseMap<Operation *, bool> &transactions;
  uint64_t work = 0;
  bool exhausted = false;
};

} // namespace

SourceMethodSummariesAnalysis::SourceMethodSummariesAnalysis(
    Operation *operation)
    : package(operation) {
  auto package = dyn_cast_or_null<mlir::ModuleOp>(operation);
  if (!package) {
    if (operation)
      operation->emitOpError() << "source Method summaries require a package";
    return;
  }
  valid = succeeded(SummaryBuilder(package, methods, transactions).run());
}

const SourceMethodSummary *
SourceMethodSummariesAnalysis::lookup(SourceMethodOp method) const {
  auto found = methods.find(method);
  return valid && found != methods.end() ? &found->second : nullptr;
}

bool SourceMethodSummariesAnalysis::isTotal(
    SourceTransactionOp transaction) const {
  return valid && transactions.lookup(transaction);
}

FailureOr<DictionaryAttr>
SourceMethodSummariesAnalysis::getPublicationSummary(ModuleOp definition) {
  if (!definition) {
    if (package)
      package->emitOpError()
          << "publication summary requires a module definition";
    return failure();
  }
  auto found = publicationSummaries.find(definition);
  if (found != publicationSummaries.end())
    return found->second ? FailureOr<DictionaryAttr>(found->second)
                         : FailureOr<DictionaryAttr>(failure());
  DictionaryAttr summary;
  if (!valid || definition->getParentOp() != package) {
    definition.emitOpError()
        << "publication summary requires a valid current package analysis";
  } else {
    auto derived = detail::deriveSourceTransactionSummary(definition);
    if (succeeded(derived))
      summary = *derived;
  }
  publicationSummaries[definition] = summary;
  return summary ? FailureOr<DictionaryAttr>(summary)
                 : FailureOr<DictionaryAttr>(failure());
}

LogicalResult
verifyPublishedTransactionSummary(ModuleOp definition, DictionaryAttr published,
                                  SourceMethodSummariesAnalysis &analysis) {
  if (failed(verifySourceTransactionSummary(definition, published)))
    return failure();
  auto derived = analysis.getPublicationSummary(definition);
  if (failed(derived))
    return failure();
  if (*derived != published)
    return definition.emitOpError()
           << "published transaction summary disagrees with provider body";
  return success();
}

} // namespace acir::ac
