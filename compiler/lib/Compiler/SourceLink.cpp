#include "SourceLink.h"
#include "LiteralPayloadWires.h"
#include "SourceHeaderHelpers.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassInstrumentation.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace acir::compiler {
namespace {

// Source pairs and native IR are authenticated before this pass sees the
// linked clone. Keep compile-time boundary wires in published source units;
// compact only redundant literal payload inputs before transaction planning.
class CompactLiteralPayloadWiresPass final
    : public PassWrapper<CompactLiteralPayloadWiresPass,
                         OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CompactLiteralPayloadWiresPass)

  void runOnOperation() final {
    for (auto module : getOperation().getOps<ac::ModuleOp>()) {
      auto &block = module.getBody().front();
      for (auto extract :
           llvm::make_early_inc_range(block.getOps<ac::BitsExtractOp>())) {
        Value input = detail::literalPayloadReplacement(extract, block);
        if (!input) continue;
        extract.getResult().replaceAllUsesWith(input);
        extract.erase();
      }
      // This standard link pass operates on the already admitted clone. Keep
      // the existing hardware/transaction verification limits at their owners.
      auto plan = detail::planLiteralPayloadConstants(
          block, [](uint64_t, uint64_t) { return success(); });
      if (failed(plan)) {
        module.emitError("literal payload plan exceeds container capacity");
        signalPassFailure();
        return;
      }
      for (auto [constant, representative] : *plan) {
        constant.getResult().replaceAllUsesWith(representative);
        constant.erase();
      }
    }
  }
};

struct TransactionStageCapture {
  unsigned beforeCount = 0;
  unsigned routedCount = 0;
  unsigned afterCount = 0;
  SourceLinkStages stages;
};

class TransactionStageInstrumentation final : public PassInstrumentation {
public:
  TransactionStageInstrumentation(Pass *route, Pass *lower, Operation *package,
                                  TransactionStageCapture &capture)
      : route(route), lower(lower), package(package), capture(capture) {}

  void runBeforePass(Pass *pass, Operation *operation) override {
    if (pass == route && operation == package && ++capture.beforeCount == 1)
      print(operation, capture.stages.beforeTransactions);
  }
  void runAfterPass(Pass *pass, Operation *operation) override {
    if (pass == route && operation == package && ++capture.routedCount == 1)
      print(operation, capture.stages.routedTransactions);
    if (pass == lower && operation == package && ++capture.afterCount == 1)
      print(operation, capture.stages.afterTransactions);
  }

private:
  static void print(Operation *operation, std::string &text) {
    llvm::raw_string_ostream output(text);
    operation->print(output, OpPrintingFlags().enableDebugInfo());
    output << '\n';
  }
  Pass *route;
  Pass *lower;
  Operation *package;
  TransactionStageCapture &capture;
};

LogicalResult preflightSourceLinkPair(SourceLinkUnit unit, unsigned index,
                                      ac::detail::EmitError emitError) {
  auto headerOwner =
      unit.header->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto bodyOwner = unit.body->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto headerKind = unit.header->getAttrOfType<StringAttr>("ac.unit_kind");
  auto bodyKind = unit.body->getAttrOfType<StringAttr>("ac.unit_kind");
  auto headerStage = unit.header->getAttrOfType<StringAttr>("ac.stage");
  auto bodyStage = unit.body->getAttrOfType<StringAttr>("ac.stage");
  if (failed(ac::detail::verifySourceOwner(headerOwner, emitError)) ||
      failed(ac::detail::verifySourceOwner(bodyOwner, emitError)) ||
      headerOwner != bodyOwner)
    return emitError() << "source link unit[" << index
                       << "] body/header SourceOwner mismatch";
  if (!headerKind || headerKind.getValue() != "interface" || !headerStage ||
      headerStage.getValue() != "source")
    return emitError() << "source link unit[" << index
                       << "] owning header stage/unit_kind is invalid";
  if (!bodyKind ||
      (bodyKind.getValue() != "implementation" &&
       bodyKind.getValue() != "declarations") ||
      !bodyStage || bodyStage.getValue() != "source")
    return emitError() << "source link unit[" << index
                       << "] body stage/unit_kind is invalid";
  for (StringRef name : {"ac.interfaces", "ac.exports", "ac.import_bindings"})
    if (!unit.header->getAttr(name) ||
        unit.header->getAttr(name) != unit.body->getAttr(name))
      return emitError() << "source link unit[" << index
                         << "] body/header metadata differs for " << name;
  return success();
}

} // namespace

FailureOr<SourceHeaderRegistry>
admitSourceLinkUnits(llvm::ArrayRef<SourceLinkUnit> units,
                     ac::detail::EmitError emitError) {
  if (units.empty())
    return emitError() << "source link requires at least one complete unit";

  SmallVector<ModuleOp> headers;
  headers.reserve(units.size());
  llvm::DenseMap<Attribute, unsigned> ownerIndices;
  for (auto [index, unit] : llvm::enumerate(units)) {
    if (!unit.body || !unit.header)
      return emitError() << "source link unit[" << index
                         << "] requires both body and owning header";
    if (failed(preflightSourceLinkPair(unit, index, emitError)))
      return failure();
    auto owner = unit.header->getAttrOfType<DictionaryAttr>("ac.source_owner");
    auto inserted = ownerIndices.try_emplace(owner, index);
    if (!inserted.second)
      return emitError() << "duplicate source link SourceOwner in unit["
                         << inserted.first->second << "] and unit[" << index
                         << ']';
    headers.push_back(unit.header);
  }

  // Build one authority registry for the complete link input. This closes the
  // provider, namespace, declaration, and helper snapshots before any body is
  // passed to the native verifier or a later evaluator.
  auto registry = SourceHeaderRegistry::create(headers, emitError);
  if (failed(registry))
    return failure();

  for (auto [index, unit] : llvm::enumerate(units)) {
    if (failed(
            registry->verifyBodySnapshots(unit.body, unit.header, emitError)))
      return emitError() << "source link body/header admission failed for "
                            "unit["
                         << index << ']';
    if (failed(verify(unit.body)))
      return emitError() << "source link body failed native verification for "
                            "unit["
                         << index << ']';
  }
  return std::move(*registry);
}

FailureOr<OwningOpRef<ModuleOp>> linkSourceUnits(
    ArrayRef<SourceLinkUnit> units, StringRef top,
    ac::detail::EmitError emitError, SourceLinkTarget target,
    SourceLinkStages *retainedStages,
    ArrayRef<SourceLinkParameterBinding> parameters, StringRef resetSemantics) {
  if (resetSemantics != "strict" && resetSemantics != "if_one")
    return emitError() << "reset semantics must be strict or if_one";
  auto admitted = admitSourceLinkUnits(units, emitError);
  if (failed(admitted))
    return failure();
  MLIRContext *context = units.front().body->getContext();
  OwningOpRef<ModuleOp> linked(ModuleOp::create(UnknownLoc::get(context)));
  Builder builder(context);
  SmallVector<Attribute> owners;
  for (SourceLinkUnit unit : units)
    owners.push_back(unit.header->getAttr("ac.source_owner"));
  llvm::sort(owners, [](Attribute left, Attribute right) {
    return ac::detail::compareClosedSourceStructure(left, right) < 0;
  });
  (*linked)->setAttr("ac.source_units", builder.getArrayAttr(owners));
  llvm::StringMap<Operation *> symbols;
  for (SourceLinkUnit unit : units) {
    for (Operation &operation : unit.body.getBody()->getOperations()) {
      auto name = SymbolTable::getSymbolName(&operation);
      if (!name)
        return operation.emitOpError(
            "source-unit top-level operation must be a declaration");
      auto prior = symbols.find(name.getValue());
      if (prior != symbols.end()) {
        bool oldDefinition = isa<ac::ModuleOp>(prior->second);
        bool newDefinition = isa<ac::ModuleOp>(operation);
        if (oldDefinition && isa<ac::ModuleImportOp>(operation))
          continue;
        if (newDefinition && isa<ac::ModuleImportOp>(prior->second)) {
          prior->second->erase();
          symbols.erase(prior);
        } else if (detail::sameDeclaration(prior->second, &operation) &&
                   !newDefinition) {
          auto role =
              operation.getAttrOfType<StringAttr>("ac.declaration_role");
          auto priorRole =
              prior->second->getAttrOfType<StringAttr>("ac.declaration_role");
          // Every pair already passed owning-body authority. Prefer that real
          // nominal definition over an equivalent consumer snapshot, preserving
          // provider metadata and location independently of link input order.
          if (isa<ac::StructOp, ac::EnumOp>(operation) &&
              role.getValue() == "definition" &&
              priorRole.getValue() == "import_snapshot") {
            prior->second->erase();
            symbols.erase(prior);
          } else {
            continue;
          }
        } else {
          return operation.emitOpError("conflicting linked declaration: ")
                 << name;
        }
      }
      Operation *copy = operation.clone();
      (*linked).getBody()->push_back(copy);
      symbols[name.getValue()] = copy;
    }
  }
  auto selected = symbols.find(top);
  auto root = selected == symbols.end()
                  ? ac::ModuleOp()
                  : dyn_cast<ac::ModuleOp>(selected->second);
  if (!root)
    return emitError() << "link top is not a supplied module definition: @"
                       << top;
  if (!root.getTypeParameters().empty())
    return root.emitOpError(
        "source link requires concrete root type arguments");
  llvm::StringMap<ac::MathIntAttr> overrides;
  for (const SourceLinkParameterBinding &binding : parameters) {
    if (binding.name.empty() || overrides.contains(binding.name))
      return root.emitOpError(
          "root parameter binding names must be nonempty and unique");
    if (!llvm::any_of(root.getParameters(), [&](Attribute raw) {
          return cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue() ==
                 binding.name;
        }))
      return root.emitOpError() << "unknown root integer parameter: "
                                << binding.name;
    auto value = ac::detail::parseMathIntAttr(context, binding.value, emitError);
    if (failed(value))
      return failure();
    overrides[binding.name] = *value;
  }
  DictionaryAttr origin, span;
  if (!overrides.empty()) {
    origin = root->getAttrOfType<DictionaryAttr>("ac.origin");
    auto location = dyn_cast<FileLineColLoc>(root.getLoc());
    if (!origin || !location)
      return root.emitOpError(
          "root parameter bindings require the source declaration location");
    // The binding belongs to the selected declaration. Preserve that actual
    // occurrence and source point instead of inventing a Python AST site.
    span = builder.getDictionaryAttr({
        builder.getNamedAttr("path", location.getFilename()),
        builder.getNamedAttr("line", builder.getI64IntegerAttr(location.getLine())),
        builder.getNamedAttr("column", builder.getI64IntegerAttr(location.getColumn())),
        builder.getNamedAttr("end_line", builder.getI64IntegerAttr(location.getLine())),
        builder.getNamedAttr("end_column", builder.getI64IntegerAttr(location.getColumn()))});
  }
  SmallVector<Attribute> arguments;
  for (Attribute raw : root.getParameters()) {
    auto parameter = cast<DictionaryAttr>(raw);
    StringRef name = parameter.getAs<StringAttr>("name").getValue();
    auto value = parameter.getAs<ac::StaticExprAttr>("default");
    if (auto found = overrides.find(name); found != overrides.end())
      value = ac::StaticExprAttr::get(
          context, builder.getDictionaryAttr({
              builder.getNamedAttr("kind", builder.getStringAttr("literal")),
              builder.getNamedAttr("value", builder.getDictionaryAttr({
                  builder.getNamedAttr("kind", builder.getStringAttr("integer")),
                  builder.getNamedAttr("value", found->second)})),
              builder.getNamedAttr("origin", origin),
              builder.getNamedAttr("location", span)}));
    if (!value)
      return root.emitOpError() << "source link root integer parameter requires "
                                  "a value or default: " << name;
    arguments.push_back(value);
  }
  OperationState state(root.getLoc(), ac::SystemOp::getOperationName());
  state.addAttribute(
      "entry",
      builder.getDictionaryAttr(
          {builder.getNamedAttr("callee", FlatSymbolRefAttr::get(context, top)),
           builder.getNamedAttr("parameters", builder.getArrayAttr(arguments)),
           builder.getNamedAttr("type_arguments", builder.getArrayAttr({}))}));
  state.addAttribute("domain", builder.getStringAttr("default"));
  state.addAttribute("ac.execution_policy",
                     builder.getDictionaryAttr({builder.getNamedAttr(
                         "reset", builder.getStringAttr(resetSemantics))}));
  (*linked).getBody()->push_back(Operation::create(state));
  auto residualCall = linked->walk([&](func::CallOp call) {
    if (call->getParentOfType<func::FuncOp>())
      return WalkResult::advance();
    call.emitOpError() << "source value helper calls must be expanded before "
                          "hardware publication";
    return WalkResult::interrupt();
  });
  if (residualCall.wasInterrupted())
    return failure();
  // Admission above checked all helper bodies and their source authority.
  // Hardware expressions already contain their expanded payload operations.
  for (auto helper :
       llvm::make_early_inc_range(linked->getOps<func::FuncOp>())) {
    auto kind = helper->getAttrOfType<StringAttr>("ac.helper_kind");
    if (kind && kind.getValue() == "value")
      helper.erase();
  }
  // Native transformation checks IR semantics; canonical provider authority
  // stays with the admitted source registry, including after definition merge.
  for (auto imported : linked->getOps<ac::ModuleImportOp>())
    if (failed(admitted->verifyModuleImport(imported, emitError)))
      return failure();

  // Capture storage outlives instrumentation. Validate before any diagnostic
  // printing, using existing hardware limits for packages with no transactions.
  TransactionStageCapture capture;
  PassManager pipeline(context);
  pipeline.addPass(acir::createNormalizeSourceControlFlowPass());
  pipeline.addPass(std::make_unique<CompactLiteralPayloadWiresPass>());
  if (ac::detail::findUnloweredTransaction(linked->getOperation()))
    pipeline.addPass(acir::createAnalyzeTransactionStateSSAPass());
  auto route = acir::createRouteTransactionsPass();
  auto lower = target == SourceLinkTarget::Hardware
                   ? acir::createLowerTransactionsPass()
                   : std::unique_ptr<Pass>();
  if (retainedStages)
    pipeline.addInstrumentation(
        std::make_unique<TransactionStageInstrumentation>(
            route.get(), lower.get(), linked->getOperation(), capture));
  pipeline.addPass(std::move(route));
  if (lower)
    pipeline.addPass(std::move(lower));
  if (failed(pipeline.run(*linked)))
    return failure();
  if (retainedStages) {
    if (capture.beforeCount != 1 || capture.routedCount != 1 ||
        capture.afterCount != (target == SourceLinkTarget::Hardware ? 1u : 0u))
      return emitError()
             << "transaction link stages were not captured exactly once";
    *retainedStages = std::move(capture.stages);
  }
  return std::move(linked);
}

} // namespace acir::compiler
