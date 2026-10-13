#include "Dialect.h"
#include "llvm/ADT/StringSet.h"
#include "Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
using namespace mlir;
namespace {
Operation *make(OpBuilder &b, Location loc, StringRef name, ValueRange operands = {}, TypeRange types = {}, ArrayRef<NamedAttribute> attrs = {}) {
  OperationState state(loc, name); state.addOperands(operands); state.addTypes(types); state.addAttributes(attrs); return b.create(state);
}
struct Lower : PassWrapper<Lower, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(Lower)
  StringRef getArgument() const final { return "acir-lower-analysis"; }
  StringRef getDescription() const final { return "Lower native circuit regions, returns and component contracts to analysis functions/CFG"; }
  void getDependentDialects(DialectRegistry &r) const override { r.insert<func::FuncDialect, arith::ArithDialect, cf::ControlFlowDialect, scf::SCFDialect, acir::ACIRDialect>(); }
  void runOnOperation() override {
    auto module = getOperation(); OpBuilder b(module.getContext());
    NamedAttrList model(module->getAttrOfType<DictionaryAttr>("acir.model"));
    SmallVector<Attribute> instances, contracts;
    for (auto scope : module.getOps<acir::ModuleOp>()) {
      if (auto value = scope->getAttr("acir.instance")) instances.push_back(value);
      else instances.push_back(b.getDictionaryAttr({b.getNamedAttr("name", scope.getSymNameAttr()), b.getNamedAttr("parent", b.getUnitAttr()), b.getNamedAttr("members", b.getDictionaryAttr({})), b.getNamedAttr("inputs", b.getDictionaryAttr({})), b.getNamedAttr("outputs", b.getUnitAttr()), b.getNamedAttr("methods", b.getArrayAttr({}))}));
      SmallVector<Attribute> methods, relations;
      for (auto interface : scope.getDeclarations().front().getOps<acir::InterfaceOp>()) {
        methods.push_back(interface.getSymNameAttr());
        for (auto order : interface.getOrders()) {
          NamedAttrList relation(cast<DictionaryAttr>(order));
          relation.set("before", cast<SymbolRefAttr>(relation.get("before")).getLeafReference());
          relation.set("after", interface.getSymNameAttr());
          relations.push_back(relation.getDictionary(b.getContext()));
        }
      }
      contracts.push_back(b.getDictionaryAttr({b.getNamedAttr("id", b.getStringAttr("Module:" + scope.getSymName().str())), b.getNamedAttr("methods", b.getArrayAttr(methods)), b.getNamedAttr("relations", b.getArrayAttr(relations))}));
    }
    model.set("instances", b.getArrayAttr(instances)); model.set("contracts", b.getArrayAttr(contracts));
    module->setAttr("acir.model", model.getDictionary(b.getContext()));
    DenseMap<Operation *, ArrayAttr> bindings;
    bool invalid = false;
    module.walk([&](acir::RuleCallOp call) {
      auto *target = SymbolTable::lookupSymbolIn(module, call.getTarget());
      if (bindings.count(target) && bindings[target] != call.getOutputBindings()) { call.emitOpError("Rule requires fixed output bindings across calls"); invalid = true; }
      bindings[target] = call.getOutputBindings();
    });
    if (invalid) { signalPassFailure(); return; }
    SmallVector<Operation *> definitions;
    module.walk([&](Operation *op) { if (isa<acir::RuleOp, acir::MethodOp, acir::WorkOp>(op)) definitions.push_back(op); });
    for (auto *definition : definitions) {
      auto meta = definition->getAttrOfType<DictionaryAttr>("acir.function");
      bool work = isa<acir::WorkOp>(definition), rule = isa<acir::RuleOp>(definition);
      NamedAttrList metadata(meta);
      auto scope = definition->getParentOfType<acir::ModuleOp>();
      auto name = work ? (meta ? meta.getAs<StringAttr>("name") : StringAttr()) : definition->getAttrOfType<StringAttr>("sym_name");
      if (!name) name = b.getStringAttr(scope.getSymName().str() + "_Work");
      metadata.set("name", name);
      metadata.set("kind", b.getStringAttr(work ? "work" : rule ? "rule" : "method"));
      if (work || rule) metadata.set("owner", scope.getSymNameAttr());
      if (rule) {
        auto identity = name.getValue(); auto prefix = scope.getSymName().str() + ".";
        if (identity.starts_with(prefix)) identity = identity.drop_front(prefix.size());
        metadata.set("rule", b.getStringAttr(identity));
      }
      if (!work && !rule) {
        if (auto interface = definition->getAttrOfType<SymbolRefAttr>("interface_ref")) {
          auto method = interface.getLeafReference();
          metadata.set("instance", scope.getSymNameAttr()); metadata.set("method", method);
          metadata.set("contract", b.getStringAttr("Module:" + scope.getSymName().str()));
          metadata.set("interface", b.getStringAttr("Module:" + scope.getSymName().str() + ":" + method.getValue().str()));
        }
      }
      meta = metadata.getDictionary(b.getContext());
      auto signature = work ? b.getFunctionType({}, {}) : cast<FunctionType>(cast<TypeAttr>(definition->getAttr("function_type")).getValue());
      auto outputTypes = definition->getAttrOfType<ArrayAttr>("output_types");
      // Returns refer to actual payloads only. Omitted slots create no SSA.
      if (rule) {
        SmallVector<acir::ReturnOp> returns; definition->getRegion(1).walk([&](acir::ReturnOp op) { returns.push_back(op); });
        for (auto ret : returns) {
          b.setInsertionPoint(ret);
          auto slots = ret->getAttrOfType<ArrayAttr>("slots");
          for (auto [index, slot] : llvm::enumerate(slots)) {
            auto number = cast<IntegerAttr>(slot).getInt();
            if (!bindings.count(definition)) continue; // Unselected Rule has no effects.
            auto target = cast<SymbolRefAttr>(bindings[definition][number]);
            auto refType = cast<TypeAttr>(outputTypes[number]).getValue();
            auto ref = make(b, ret.getLoc(), "acir.ref", {}, refType, {b.getNamedAttr("target", target)})->getResult(0);
            auto operation = isa<acir::QueueType>(refType) ? "acir.queue.push" : isa<acir::CellType>(refType) ? "acir.cell.write" : "acir.signal.set";
            auto *effect = make(b, ret.getLoc(), operation, {ref, ret.getValues()[index]});
            if (auto id = ret->getAttrOfType<StringAttr>("acir.id")) effect->setAttr("acir.id", b.getStringAttr(id.getValue().str() + ".out" + std::to_string(number)));
          }
          func::ReturnOp::create(b, ret.getLoc()); ret.erase();
        }
      } else {
        SmallVector<Operation *> ends;
        definition->walk([&](Operation *op) { if (isa<acir::ReturnOp, acir::WorkEndOp>(op)) ends.push_back(op); });
        for (auto *op : ends) { b.setInsertionPoint(op); func::ReturnOp::create(b, op->getLoc(), op->getOperands()); op->erase(); }
      }
      b.setInsertionPointToEnd(module.getBody());
      auto fn = func::FuncOp::create(b, definition->getLoc(), name.getValue(), signature);
      fn->setAttr("acir.function", meta);
      auto &body = definition->getRegion(work ? 0 : 1);
      fn.getBody().takeBody(body);
      bool trivialGuard = !work && definition->getRegion(0).empty();
      if (!work && definition->getRegion(0).hasOneBlock()) {
        auto &block = definition->getRegion(0).front();
        if (block.getOperations().size() == 2)
          if (auto constant = dyn_cast<arith::ConstantOp>(block.front()))
            if (auto value = dyn_cast<IntegerAttr>(constant.getValue())) trivialGuard = value.getValue().isOne();
      }
      if (!work && !trivialGuard) {
        auto guardName = name.getValue().str() + ".guard";
        auto guard = func::FuncOp::create(b, definition->getLoc(), guardName, b.getFunctionType(signature.getInputs(), b.getI1Type()));
        NamedAttrList guardMeta(meta);
        for (auto key : {"method", "interface", "contract", "instance", "summary", "owner", "rule"}) guardMeta.erase(key);
        guardMeta.set("kind", b.getStringAttr("guard")); guardMeta.set("name", b.getStringAttr(guardName)); guardMeta.set("result", b.getStringAttr("bool"));
        guard->setAttr("acir.function", guardMeta.getDictionary(b.getContext()));
        guard.getBody().takeBody(definition->getRegion(0));
        SmallVector<acir::GuardYieldOp> yields; guard.walk([&](acir::GuardYieldOp op) { yields.push_back(op); });
        for (auto yield : yields) { b.setInsertionPoint(yield); func::ReturnOp::create(b, yield.getLoc(), ValueRange{yield.getCondition()}); yield.erase(); }
        b.setInsertionPointToStart(&fn.getBody().front());
        auto check = func::CallOp::create(b, fn.getLoc(), guardName, TypeRange{b.getI1Type()}, fn.getArguments());
        auto info = meta.getAs<DictionaryAttr>("guard_info");
        SmallVector<NamedAttribute> attrs{b.getNamedAttr("name", info ? info.get("name") : name)};
        if (info && info.get("source")) attrs.push_back(b.getNamedAttr("source", info.get("source")));
        make(b, guard.getBody().front().back().getLoc(), "acir.guard", check.getResults(), {}, attrs);
      }
      definition->erase();
    }
    // Flatten the elaborated instance scopes. Retain each global helper once.
    llvm::StringSet<> seen;
    for (auto f : module.getOps<func::FuncOp>()) seen.insert(f.getSymName());
    SmallVector<acir::ModuleOp> scopes(module.getOps<acir::ModuleOp>());
    for (auto scope : scopes) {
      for (auto &op : llvm::make_early_inc_range(scope.getDeclarations().front())) {
        if (isa<acir::InterfaceOp, acir::InstanceOp>(op)) { op.erase(); continue; }
        if (auto fn = dyn_cast<func::FuncOp>(&op)) {
          if (!seen.insert(fn.getSymName()).second) { fn.erase(); continue; }
        }
        op.moveBefore(module.getBody(), module.getBody()->end());
      }
      scope.erase();
    }
    // Component expansion is deliberately performed on the analysis copy.
    SmallVector<Operation *> operations; module.walk([&](Operation *op) { operations.push_back(op); });
    DenseMap<Block *, DenseMap<Value, Operation *>> consumed;
    for (auto *op : operations) {
      b.setInsertionPoint(op);
      if (auto ref = dyn_cast<acir::GetOp>(op)) { ref.setTargetAttr(FlatSymbolRefAttr::get(b.getContext(), ref.getTarget().getLeafReference().getValue())); }
      if (auto call = dyn_cast<acir::MethodCallOp>(op)) {
        auto replacement = func::CallOp::create(b, call.getLoc(), call.getTarget().getLeafReference().getValue(), call->getResultTypes(), call.getInputs());
        for (auto attr : call->getAttrs()) if (attr.getName() != "target") replacement->setAttr(attr.getName(), attr.getValue());
        call->replaceAllUsesWith(replacement.getResults()); call.erase(); continue;
      }
      if (auto call = dyn_cast<acir::RuleCallOp>(op)) {
        auto *invoke = make(b, call.getLoc(), "acir.analysis.invoke", call.getInputs(), {}, {b.getNamedAttr("name", call.getTarget().getLeafReference()), b.getNamedAttr("callee", b.getStringAttr(call.getTarget().getLeafReference().getValue()))});
        if (auto id = call->getAttr("acir.id")) invoke->setAttr("acir.id", id);
        call.erase(); continue;
      }
      if (auto full = dyn_cast<acir::QueueFullOp>(op)) {
        auto *read = make(b, full.getLoc(), "acir.analysis.capacity.read", full.getResource(), b.getI1Type(), full->getAttrs());
        full->replaceAllUsesWith(read->getResults()); full.erase(); continue;
      }
      if (auto peek = dyn_cast<acir::QueuePeekOp>(op)) {
        auto role = peek->getAttrOfType<StringAttr>("role");
        if (role && role.getValue() == "consume") {
          auto &requests = consumed[peek->getBlock()];
          if (auto *existing = requests.lookup(peek.getResource())) {
            SmallVector<Attribute> locations(existing->getAttrOfType<ArrayAttr>("acir.consume_locs").getValue());
            locations.push_back(peek.getLoc()); existing->setAttr("acir.consume_locs", b.getArrayAttr(locations));
          } else {
            b.setInsertionPointAfter(peek);
            auto *pop = make(b, peek.getLoc(), "acir.queue.pop", peek.getResource());
            requests[peek.getResource()] = pop;
            pop->setAttr("acir.consume_locs", b.getArrayAttr({peek.getLoc()}));
            if (auto id = peek->getAttrOfType<StringAttr>("acir.id")) pop->setAttr("acir.id", b.getStringAttr(id.getValue().str() + ".pop"));
          }
        }
      }
      if (auto push = dyn_cast<acir::PushOp>(op)) {
        auto full = make(b, push.getLoc(), "acir.analysis.capacity.read", push.getResource(), b.getI1Type())->getResult(0);
        auto yes = arith::ConstantIntOp::create(b, push.getLoc(), 1, 1);
        auto ready = arith::XOrIOp::create(b, push.getLoc(), full, yes);
        make(b, push.getLoc(), "acir.guard", ready.getResult(), {}, {b.getNamedAttr("name", b.getStringAttr("Queue.push capacity")), b.getNamedAttr("implicit", b.getBoolAttr(true))});
      }
    }
    SmallVector<acir::PopOp> pops;
    module.walk([&](acir::PopOp pop) { pops.push_back(pop); });
    for (auto pop : pops) {
      b.setInsertionPointAfter(pop);
      auto available = arith::ConstantIntOp::create(b, pop.getLoc(), 0, 1);
      make(b, pop.getLoc(), "acir.analysis.capacity.write", {pop.getResource(), available});
    }
    PassManager pm(b.getContext()); pm.addPass(createSCFToControlFlowPass());
    if (failed(pm.run(module))) signalPassFailure();
  }
};
}
std::unique_ptr<Pass> acir::createLowerAnalysisPass() { return std::make_unique<Lower>(); }
void acir::registerACIRPasses() { PassRegistration<Lower>(); }
