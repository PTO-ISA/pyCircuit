#include "pycircuit/Transforms/Passes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceHelperAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "Compiler/TableQueryLowering.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/ScopeExit.h"
#include <functional>

using namespace mlir;
namespace acir {
namespace {
using compiler::detail::TableQueryBudget;
using compiler::detail::TableQueryHooks;
using compiler::detail::TableQueryLowering;

bool imported(func::FuncOp helper, mlir::ModuleOp package) {
  return helper->getAttr("ac.source_owner") != package->getAttr("ac.source_owner");
}
bool importedBody(Operation *op, mlir::ModuleOp package) {
  auto helper = op->getParentOfType<func::FuncOp>();
  return helper && imported(helper, package);
}
Operation *make(OpBuilder &builder, Location loc, StringRef name,
                ValueRange inputs, TypeRange results,
                ArrayRef<NamedAttribute> attrs = {}) {
  OperationState state(loc, name);
  state.addOperands(inputs); state.addTypes(results); state.addAttributes(attrs);
  return builder.create(state);
}
ac::StaticExprAttr literal(OpBuilder &builder, uint64_t number, Operation *site) {
  ac::StaticExprAttr prototype;
  for (Type type : site->getResultTypes()) {
    if (auto bits = dyn_cast<ac::BitsType>(type)) prototype = bits.getWidth();
    if (auto table = dyn_cast<ac::TableType>(type))
      if (!table.getShape().empty()) prototype = dyn_cast<ac::StaticExprAttr>(table.getShape()[0]);
    if (prototype) break;
  }
  assert(prototype && "scalar/query result supplies source provenance");
  auto tree = prototype.getTree();
  Attribute origin = site->getAttr("ac.origin"), location = site->getAttr("ac.location");
  if (!origin || !location) {
    origin = tree.get("origin"); location = tree.get("location");
  }
  auto value = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("value", ac::MathIntAttr::get(builder.getContext(), llvm::APSInt::getUnsigned(number)))});
  return ac::StaticExprAttr::get(builder.getContext(), builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("literal")),
      builder.getNamedAttr("value", value),
      builder.getNamedAttr("origin", origin),
      builder.getNamedAttr("location", location)}));
}
LogicalResult requireACIR(mlir::ModuleOp package) {
  if (!ac::isACIRCapture(package)) return package.emitOpError("requires ac.stage = acir");
  return success();
}

class VerifyACIRPass : public PassWrapper<VerifyACIRPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(VerifyACIRPass)
  StringRef getArgument() const final { return "ac-verify-acir"; }
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<ac::ACIRDialect, func::FuncDialect, arith::ArithDialect, scf::SCFDialect>();
  }
  void runOnOperation() final {
    auto package = getOperation();
    if (failed(requireACIR(package)) ||
        failed(ac::verifyACIRBodyStructure(package, [&] { return package.emitOpError(); }))) {
      signalPassFailure(); return;
    }
    markAllAnalysesPreserved();
  }
};

// Each clone is charged when materialized. The path stack rejects recursion;
// repeated call edges spend separately, without an importer-side DAG replay.
class HelperInliner {
  mlir::ModuleOp package;
  SmallVector<func::FuncOp> stack;
  llvm::StringMap<func::FuncOp> templates;
  SmallVector<OwningOpRef<func::FuncOp>> ownedTemplates;
  uint64_t operations = 0, slots = 0, attributeWork = 0;
  bool attributeExhausted = false;
  LogicalResult charge(Operation *op) {
    if (++operations > 65536 || (slots += 1 + op->getNumOperands() + op->getNumResults()) > 1048576)
      return op->emitOpError("helper expansion budget exceeded");
    return success();
  }
  Attribute expand(Attribute attr, ArrayAttr frames, Builder &builder, unsigned depth = 0) {
    if (attributeExhausted) return attr;
    uint64_t cost = 1;
    if (auto text = dyn_cast<StringAttr>(attr)) cost += text.getValue().size();
    if (auto number = dyn_cast<ac::MathIntAttr>(attr)) cost += number.getCanonicalValue().size();
    if (auto fields = dyn_cast<DictionaryAttr>(attr)) cost += fields.size();
    if (auto array = dyn_cast<ArrayAttr>(attr)) cost += array.size();
    if (depth > 128 || cost > 16 * 1024 * 1024 - attributeWork) {
      attributeExhausted = true;
      return attr;
    }
    attributeWork += cost;
    if (auto expression = dyn_cast<ac::StaticExprAttr>(attr))
      return ac::StaticExprAttr::get(builder.getContext(), cast<DictionaryAttr>(expand(expression.getTree(), frames, builder, depth + 1)));
    if (auto dictionary = dyn_cast<DictionaryAttr>(attr)) {
      if (auto old = dictionary.getAs<ArrayAttr>("expansion"); old && dictionary.getAs<DictionaryAttr>("site")) {
        SmallVector<Attribute> merged(old.begin(), old.end());
        if (frames.size() > (16 * 1024 * 1024 - attributeWork) / 32) {
          attributeExhausted = true; return attr;
        }
        attributeWork += frames.size() * 32;
        llvm::append_range(merged, frames);
        NamedAttrList fields(dictionary); fields.set("expansion", builder.getArrayAttr(merged));
        return fields.getDictionary(builder.getContext());
      }
      NamedAttrList fields;
      for (NamedAttribute field : dictionary) fields.append(field.getName(), expand(field.getValue(), frames, builder, depth + 1));
      return fields.getDictionary(builder.getContext());
    }
    if (auto array = dyn_cast<ArrayAttr>(attr)) {
      SmallVector<Attribute> items;
      for (Attribute item : array) items.push_back(expand(item, frames, builder, depth + 1));
      return builder.getArrayAttr(items);
    }
    return attr;
  }
public:
  explicit HelperInliner(mlir::ModuleOp package) : package(package) {}
  LogicalResult inlineCall(Operation *call, SmallVectorImpl<Value> *replacements = nullptr) {
    auto callee = call->getAttrOfType<FlatSymbolRefAttr>("callee");
    auto function = callee ? templates.lookup(callee.getValue()) : func::FuncOp();
    if (!function || !function.getBody().hasOneBlock() || function.getBody().front().empty())
      return call->emitOpError("helper implementation is missing");
    if (stack.size() >= 128 || llvm::is_contained(stack, function))
      return call->emitOpError("recursive helper call or expansion depth exceeded");
    auto returned = dyn_cast<func::ReturnOp>(function.getBody().front().back());
    if (!returned || !llvm::equal(function.getArgumentTypes(), call->getOperandTypes()) ||
        !llvm::equal(function.getResultTypes(), call->getResultTypes()))
      return call->emitOpError("helper call signature mismatch");
    stack.push_back(function);
    auto leave = llvm::scope_exit([&] { stack.pop_back(); });
    OpBuilder builder(call);
    SmallVector<Attribute> frames;
    if (auto origin = call->getAttrOfType<DictionaryAttr>("ac.origin")) {
      if (auto saved = origin.getAs<ArrayAttr>("expansion")) llvm::append_range(frames, saved);
      if (auto site = origin.getAs<DictionaryAttr>("site"))
        frames.push_back(builder.getDictionaryAttr({
            builder.getNamedAttr("kind", builder.getStringAttr("call")),
            builder.getNamedAttr("site", site), builder.getNamedAttr("callee", callee)}));
    }
    IRMapping mapping;
    for (auto [formal, actual] : llvm::zip(function.getArguments(), call->getOperands())) mapping.map(formal, actual);
    for (Operation &operation : function.getBody().front().without_terminator()) {
      auto prepaid = operation.walk([&](Operation *nested) {
        return failed(charge(nested)) ? WalkResult::interrupt() : WalkResult::advance();
      });
      if (prepaid.wasInterrupted()) return failure();
      auto *copy = builder.clone(operation, mapping);
      copy->walk([&](Operation *nested) {
        nested->setAttrs(cast<DictionaryAttr>(expand(nested->getAttrDictionary(), builder.getArrayAttr(frames), builder)));
      });
      if (attributeExhausted) return call->emitOpError("helper expansion attribute budget exceeded");
      SmallVector<Operation *> calls;
      copy->walk([&](Operation *nested) { if (isa<ac::CallOp, func::CallOp>(nested)) calls.push_back(nested); });
      for (Operation *nested : calls) {
        SmallVector<Value> old(nested->getResults());
        SmallVector<Value> values;
        if (failed(inlineCall(nested, &values))) return failure();
        for (auto [before, after] : llvm::zip(old, values)) {
          SmallVector<Value> keys;
          for (auto entry : mapping.getValueMap()) if (entry.second == before) keys.push_back(entry.first);
          for (Value key : keys) mapping.map(key, after);
        }
      }
    }
    for (auto [result, value] : llvm::zip(call->getResults(), returned.getOperands())) {
      Value replacement = mapping.lookupOrDefault(value);
      if (replacements) replacements->push_back(replacement);
      result.replaceAllUsesWith(replacement);
    }
    call->erase();
    return success();
  }
  LogicalResult run() {
    // Source definitions stay immutable while expanding every call site. This
    // keeps nested expansion frames in caller-to-callee order regardless of
    // declaration order, including while producing retained local templates.
    for (auto helper : package.getOps<func::FuncOp>()) {
      auto charged = helper.walk([&](Operation *operation) {
        return failed(charge(operation)) ? WalkResult::interrupt() : WalkResult::advance();
      });
      if (charged.wasInterrupted()) return failure();
      auto copy = cast<func::FuncOp>(helper->clone());
      templates.try_emplace(copy.getSymName(), copy);
      ownedTemplates.emplace_back(copy);
    }
    SmallVector<Operation *> calls;
    package.walk([&](ac::CallOp call) { if (!importedBody(call, package)) calls.push_back(call); });
    for (Operation *call : calls) if (failed(inlineCall(call))) return failure();
    // Private lexical templates have no remaining callers or public interface.
    // Their capture-dependent obligations are proved on each expanded actual.
    for (auto helper : llvm::make_early_inc_range(package.getOps<func::FuncOp>()))
      if (!imported(helper, package) && helper->hasAttr("ac.lexical_owner")) helper.erase();
    return success();
  }
};
class InlineHelpersPass : public PassWrapper<InlineHelpersPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(InlineHelpersPass)
  StringRef getArgument() const final { return "ac-inline-helpers"; }
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<ac::ACIRDialect, func::FuncDialect, arith::ArithDialect, scf::SCFDialect>();
  }
  void runOnOperation() final {
    if (failed(requireACIR(getOperation())) || failed(HelperInliner(getOperation()).run())) signalPassFailure();
  }
};

class LowerValueBoundariesPass : public PassWrapper<LowerValueBoundariesPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerValueBoundariesPass)
  StringRef getArgument() const final { return "ac-lower-value-boundaries"; }
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<ac::ACIRDialect, func::FuncDialect, arith::ArithDialect, scf::SCFDialect>();
  }
  void runOnOperation() final {
    auto package = getOperation();
    if (failed(requireACIR(package))) { signalPassFailure(); return; }
    ac::HardwareAnalysis analysis(package);
    SmallVector<Operation *> boundaries;
    package.walk([&](Operation *operation) {
      if (isa<ac::BoundaryOp, ac::IndexCheckOp>(operation) && !importedBody(operation, package)) boundaries.push_back(operation);
    });
    for (Operation *operation : boundaries) {
      Value input = operation->getOperand(0);
      ac::HardwareBindings bindings;
      bindings.owner = operation->getParentOfType<ac::ModuleOp>();
      if (!bindings.owner) bindings.owner = operation->getParentOfType<func::FuncOp>();
      auto range = analysis.inferUnsignedValueRange(input, bindings, operation);
      if (failed(range) || range->isEmptySet()) { signalPassFailure(); return; }
      bool isSigned = operation->getAttrOfType<BoolAttr>("is_signed").getValue();
      if (auto check = dyn_cast<ac::IndexCheckOp>(operation)) {
        auto maximum = range->getUnsignedMax();
        if ((isSigned && range->getSignedMin().isNegative()) || maximum.getActiveBits() > 64 ||
            maximum.getZExtValue() >= static_cast<uint64_t>(check.getExtent())) {
          check.emitOpError("original complete index range is not proven within extent"); signalPassFailure(); return;
        }
        check.getResult().replaceAllUsesWith(input); check.erase(); continue;
      }
      auto boundary = cast<ac::BoundaryOp>(operation);
      auto width = analysis.getPackedWidth(boundary.getResult().getType(), bindings, boundary);
      if (failed(width)) { signalPassFailure(); return; }
      if ((isSigned && range->getSignedMin().isNegative()) || range->getUnsignedMax().getActiveBits() > *width) {
        boundary.emitOpError("original value is not proven to fit destination contract"); signalPassFailure(); return;
      }
      auto constraint = boundary.getConstraint();
      if (auto domain = constraint.getAs<ac::SourceDomainAttr>("domain")) {
        auto data = domain.getValue();
        auto lower = data.getAs<ac::MathIntAttr>("lower"), upper = data.getAs<ac::MathIntAttr>("upper");
        if (lower && upper &&
            (llvm::APSInt::compareValues(llvm::APSInt(range->getUnsignedMin(), true), llvm::APSInt(lower.getCanonicalValue())) < 0 ||
             llvm::APSInt::compareValues(llvm::APSInt(range->getUnsignedMax(), true), llvm::APSInt(upper.getCanonicalValue())) >= 0)) {
          boundary.emitOpError("original value is outside declared destination domain"); signalPassFailure(); return;
        }
      }
      Value result = input;
      if (input.getType() != boundary.getResult().getType()) {
        OpBuilder builder(boundary);
        auto sourceWidth = analysis.getPackedWidth(input.getType(), bindings, boundary);
        if (failed(sourceWidth)) { signalPassFailure(); return; }
        if (*width <= *sourceWidth)
          result = make(builder, boundary.getLoc(), ac::BitsExtractOp::getOperationName(), input,
              boundary.getResult().getType(), {builder.getNamedAttr("low", literal(builder, 0, operation))})->getResult(0);
        else
          result = make(builder, boundary.getLoc(), ac::BitsResizeOp::getOperationName(), input,
              boundary.getResult().getType(), {builder.getNamedAttr("mode", builder.getStringAttr(isSigned ? "sext" : "zext"))})->getResult(0);
      }
      boundary.getResult().replaceAllUsesWith(result); boundary.erase();
    }
  }
};

class LowerQueriesPass : public PassWrapper<LowerQueriesPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerQueriesPass)
  StringRef getArgument() const final { return "ac-lower-queries"; }
  void getDependentDialects(DialectRegistry &registry) const override { registry.insert<ac::ACIRDialect>(); }
  void runOnOperation() final {
    auto package = getOperation();
    if (failed(requireACIR(package))) { signalPassFailure(); return; }
    TableQueryBudget budget;
    SmallVector<Operation *> queries;
    package.walk<WalkOrder::PostOrder>([&](Operation *operation) {
      if (isa<ac::QueryMapOp, ac::QueryChooseOp>(operation) && !importedBody(operation, package)) queries.push_back(operation);
    });
    for (Operation *operation : queries) {
      OpBuilder builder(operation);
      auto loc = operation->getLoc();
      TableQueryHooks hooks{
        [&](uint64_t value) { return literal(builder, value, operation); },
        [&](Value guard, Value yes, Value no, OpBuilder &at) -> FailureOr<Value> {
          return make(at, loc, ac::BitsSelectOp::getOperationName(), {guard, yes, no}, yes.getType())->getResult(0);
        },
        [&](Value value, Type type, OpBuilder &at) {
          return make(at, loc, ac::BitsExtractOp::getOperationName(), value, type,
              {at.getNamedAttr("low", literal(at, 0, operation))})->getResult(0);
        }};
      TableQueryLowering lowering(builder, loc, package, budget, hooks);
      FailureOr<SmallVector<Value>> results = failure();
      if (auto query = dyn_cast<ac::QueryMapOp>(operation)) {
        Block &block = query.getBody().front();
        unsigned count = query.getTableCount();
        SmallVector<Value> rows(block.getArguments().take_front(count));
        SmallVector<Value> tables(query.getOperands().take_front(count));
        for (Value table : tables) if (failed(lowering.capture(table))) { signalPassFailure(); return; }
        for (auto [argument, capture] : llvm::zip(block.getArguments().drop_front(count), query.getOperands().drop_front(count)))
          argument.replaceAllUsesWith(capture);
        SmallVector<Value> outputs(block.back().getOperands());
        block.back().erase();
        results = lowering.stage(block, rows, tables, outputs, query.getAggregates());
      } else {
        auto choose = cast<ac::QueryChooseOp>(operation);
        results = lowering.choose(choose.getOperand(0), choose.getNumOperands() == 2 ? choose.getOperand(1) : Value());
      }
      if (failed(results)) { signalPassFailure(); return; }
      for (auto [old, replacement] : llvm::zip(operation->getResults(), *results)) old.replaceAllUsesWith(replacement);
      operation->erase();
    }
  }
};
class FinalizeSourcePass : public PassWrapper<FinalizeSourcePass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(FinalizeSourcePass)
  StringRef getArgument() const final { return "ac-finalize-source"; }
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<ac::ACIRDialect, func::FuncDialect, arith::ArithDialect, scf::SCFDialect>();
  }
  void runOnOperation() final {
    auto package = getOperation();
    if (failed(requireACIR(package))) { signalPassFailure(); return; }
    auto result = package.walk([&](Operation *operation) {
      if (ac::isACIRCaptureOperation(operation)) {
        operation->emitOpError("ACIR operation remains at Source boundary"); return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    if (result.wasInterrupted()) { signalPassFailure(); return; }
    package->setAttr("ac.stage", StringAttr::get(&getContext(), "source"));
    if (failed(ac::verifySourceBodyStructure(package, [&] { return package.emitOpError(); })))
      signalPassFailure();
  }
};
} // namespace
std::unique_ptr<Pass> createVerifyACIRPass() { return std::make_unique<VerifyACIRPass>(); }
std::unique_ptr<Pass> createInlineHelpersPass() { return std::make_unique<InlineHelpersPass>(); }
std::unique_ptr<Pass> createLowerValueBoundariesPass() { return std::make_unique<LowerValueBoundariesPass>(); }
std::unique_ptr<Pass> createLowerQueriesPass() { return std::make_unique<LowerQueriesPass>(); }
std::unique_ptr<Pass> createFinalizeSourcePass() { return std::make_unique<FinalizeSourcePass>(); }
} // namespace acir
