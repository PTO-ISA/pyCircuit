#include "pyc/Transforms/Passes.h"
#include "pyc/Support/Diagnostics.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "llvm/Support/JSON.h"

#include <string>

using namespace mlir;

namespace pyc {
namespace {

static constexpr int64_t kFunctionInlineCostCap = 1400;
static constexpr int64_t kFunctionRepeatPressureCap = 96;
static constexpr int64_t kModuleRepeatPressureCap = 192;

static int64_t jsonIntOr(const llvm::json::Object &obj, llvm::StringRef key, int64_t fallback = 0) {
  if (auto v = obj.getInteger(key))
    return static_cast<int64_t>(*v);
  return fallback;
}

static LogicalResult parseStructMetrics(func::FuncOp f, llvm::json::Object &out) {
  auto raw = f->getAttrOfType<StringAttr>("pyc.struct.metrics");
  if (!raw) {
    pyc::emitError(f, "PYC982") << "missing required `pyc.struct.metrics` for hierarchy discipline";
    return failure();
  }
  llvm::Expected<llvm::json::Value> parsed = llvm::json::parse(raw.getValue());
  if (!parsed || !parsed->getAsObject()) {
    pyc::emitError(f, "PYC983") << "invalid `pyc.struct.metrics` JSON for hierarchy discipline";
    return failure();
  }
  out = *parsed->getAsObject();
  return success();
}

static LogicalResult parseStructCollections(func::FuncOp f, llvm::json::Array &out) {
  auto raw = f->getAttrOfType<StringAttr>("pyc.struct.collections");
  if (!raw) {
    pyc::emitError(f, "PYC984") << "missing required `pyc.struct.collections` for hierarchy discipline";
    return failure();
  }
  llvm::Expected<llvm::json::Value> parsed = llvm::json::parse(raw.getValue());
  if (!parsed || !parsed->getAsArray()) {
    pyc::emitError(f, "PYC985") << "invalid `pyc.struct.collections` JSON for hierarchy discipline";
    return failure();
  }
  out = *parsed->getAsArray();
  return success();
}

static int64_t repeatedPressure(const llvm::json::Object &metrics) {
  auto *clusters = metrics.getArray("repeated_body_clusters");
  if (!clusters)
    return 0;

  int64_t pressure = 0;
  for (const llvm::json::Value &value : *clusters) {
    auto *obj = value.getAsObject();
    if (!obj)
      continue;
    int64_t count = jsonIntOr(*obj, "count", 0);
    int64_t loopExtent = jsonIntOr(*obj, "loop_extent_hint", 0);
    int64_t hardwareCalls = jsonIntOr(*obj, "hardware_calls", 0);
    int64_t moduleCalls = jsonIntOr(*obj, "module_calls", 0);
    int64_t stateCalls = jsonIntOr(*obj, "state_calls", 0);
    int64_t bodyWeight = hardwareCalls + (moduleCalls * 6) + (stateCalls * 4);
    if (bodyWeight <= 0)
      bodyWeight = 1;
    if (loopExtent <= 0)
      loopExtent = 1;
    if (count <= 0)
      count = 1;
    pressure += bodyWeight * loopExtent * count;
  }
  return pressure;
}

static int64_t repeatedHierarchyPressure(const llvm::json::Object &metrics) {
  auto *clusters = metrics.getArray("repeated_body_clusters");
  if (!clusters)
    return 0;

  int64_t pressure = 0;
  for (const llvm::json::Value &value : *clusters) {
    auto *obj = value.getAsObject();
    if (!obj)
      continue;
    int64_t count = jsonIntOr(*obj, "count", 0);
    int64_t loopExtent = jsonIntOr(*obj, "loop_extent_hint", 0);
    int64_t moduleCalls = jsonIntOr(*obj, "module_calls", 0);
    int64_t bodyWeight = moduleCalls * 6;
    if (bodyWeight <= 0)
      continue;
    if (loopExtent <= 0)
      loopExtent = 1;
    if (count <= 0)
      count = 1;
    pressure += bodyWeight * loopExtent * count;
  }
  return pressure;
}

static bool hasModuleFamilyCollection(const llvm::json::Array &collections) {
  for (const llvm::json::Value &value : collections) {
    auto *obj = value.getAsObject();
    if (!obj)
      continue;
    if (auto flag = obj->getBoolean("from_module_family"); flag.has_value() && *flag)
      return true;
  }
  return false;
}

class CheckHierarchyDisciplinePass : public PassWrapper<CheckHierarchyDisciplinePass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CheckHierarchyDisciplinePass)

  StringRef getArgument() const override { return "pyc-check-hierarchy-discipline"; }
  StringRef getDescription() const override {
    return "Reject oversized inline helpers and repeated naked hierarchy replication";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    bool ok = true;

    module.walk([&](func::FuncOp f) {
      auto kindAttr = f->getAttrOfType<StringAttr>("pyc.kind");
      if (!kindAttr)
        return;

      llvm::json::Object metrics;
      llvm::json::Array collections;
      if (failed(parseStructMetrics(f, metrics)) || failed(parseStructCollections(f, collections))) {
        ok = false;
        return;
      }

      const std::string kind = kindAttr.getValue().str();
      const int64_t inlineCost = jsonIntOr(metrics, "estimated_inline_cost", 0);
      const int64_t instanceCount = jsonIntOr(metrics, "instance_count", 0);
      const int64_t stateAllocCount = jsonIntOr(metrics, "state_alloc_count", 0);
      const int64_t collectionCount = jsonIntOr(metrics, "collection_count", 0);
      const int64_t moduleCallCount = jsonIntOr(metrics, "module_call_count", 0);
      const int64_t stateCallCount = jsonIntOr(metrics, "state_call_count", 0);
      const int64_t repeat = repeatedPressure(metrics);
      const int64_t hierarchyRepeat = repeatedHierarchyPressure(metrics);
      const bool hasFamilyCollection = hasModuleFamilyCollection(collections);

      if (kind == "function") {
        if (instanceCount > 0 || collectionCount > 0 || moduleCallCount > 0) {
          pyc::emitError(f, "PYC986") << "`@function` must not instantiate modules or collections "
                        << "(instance_count=" << instanceCount
                        << ", collection_count=" << collectionCount
                        << ", module_call_count=" << moduleCallCount
                        << "; hint: promote the repeated hierarchy to `@module` and instantiate via `array(...)`)";
          ok = false;
        }
        if (stateAllocCount > 0 || stateCallCount > 0) {
          pyc::emitError(f, "PYC987") << "`@function` must not allocate state "
                        << "(state_alloc_count=" << stateAllocCount
                        << ", state_call_count=" << stateCallCount
                        << "; hint: move stateful logic behind a `@module` boundary)";
          ok = false;
        }
        if (inlineCost > kFunctionInlineCostCap) {
          pyc::emitError(f, "PYC988") << "`@function` exceeds inline complexity cap: estimated_inline_cost="
                        << inlineCost << " > " << kFunctionInlineCostCap
                        << " (hint: split the helper or promote it to `@module`)";
          ok = false;
        }
        if (repeat > kFunctionRepeatPressureCap) {
          pyc::emitError(f, "PYC989") << "`@function` contains repeated inline hardware pressure=" << repeat
                        << " > " << kFunctionRepeatPressureCap
                        << " (hint: use `spec.module_family(...)` + `array(...)` for repeated structure)";
          ok = false;
        }
        return;
      }

      if (kind == "module") {
        if (hierarchyRepeat > kModuleRepeatPressureCap && !hasFamilyCollection) {
          pyc::emitError(f, "PYC990") << "repeated hierarchy pressure=" << hierarchyRepeat
                        << " exceeds module discipline cap " << kModuleRepeatPressureCap
                        << " without `ModuleFamilySpec` collections"
                        << " (hint: model repeated same-shape hierarchy with `spec.module_family(...)` "
                           "and `array(...)` instead of naked loop replication)";
          ok = false;
        }
      }
    });

    if (!ok)
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<::mlir::Pass> createCheckHierarchyDisciplinePass() {
  return std::make_unique<CheckHierarchyDisciplinePass>();
}

static PassRegistration<CheckHierarchyDisciplinePass> pass;

} // namespace pyc
