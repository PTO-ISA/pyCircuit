#include "pycircuit/Dialect/ACIR/SourceHelperAnalysis.h"
#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

#include <algorithm>
#include <functional>

using namespace mlir;
namespace acir::ac {
namespace {
constexpr uint64_t maxWork = 1024 * 1024;
constexpr unsigned maxDepth = 128;

bool payload(Type type) {
  return isa<BitsType, StructType, EnumType, TableType>(type);
}

LogicalResult charge(Operation *site, uint64_t amount, uint64_t &work) {
  if (work > maxWork || amount > maxWork - work)
    return site->emitOpError() << "value helper verification budget exceeded";
  work += amount;
  return success();
}

// Raw fields and region count must be checked before generated typed getters.
// This preflight is also used for every callee before MLIR symbol-use checking.
LogicalResult preflightSignature(func::FuncOp helper, uint64_t &work) {
  if (!helper)
    return failure();
  auto name = helper->getAttrOfType<StringAttr>("sym_name");
  auto rawType = helper->getAttrOfType<TypeAttr>("function_type");
  auto signature = rawType ? dyn_cast<FunctionType>(rawType.getValue())
                           : FunctionType();
  if (helper->getNumRegions() != 1 || !name || name.getValue().empty() ||
      !signature)
    return helper.emitOpError() << "value helper requires a named function "
                                  "signature and one body region";
  if (failed(charge(helper, 1 + uint64_t(signature.getNumInputs()) +
                              signature.getNumResults(), work)))
    return failure();
  return helper.verifyInvariantsImpl();
}

LogicalResult preflightBody(func::FuncOp helper, SourceHelperLookup lookup,
                            uint64_t &work) {
  std::function<LogicalResult(Region &, unsigned)> visit =
      [&](Region &region, unsigned depth) -> LogicalResult {
    if (depth > maxDepth || !region.hasOneBlock() || region.front().empty())
      return helper.emitOpError() << "value helper region depth or shape invalid";
    if (failed(charge(helper, region.front().getNumArguments(), work)))
      return failure();
    for (Operation &operation : region.front()) {
      if (failed(charge(&operation, 1 + uint64_t(operation.getNumOperands()) +
                                       operation.getNumResults(), work)))
        return failure();
      for (Value operand : operation.getOperands()) {
        Region *source = operand ? operand.getParentRegion() : nullptr;
        Operation *scope = source ? source->getParentOp() : nullptr;
        if (!scope || (scope != helper && !helper->isProperAncestor(scope)))
          return operation.emitOpError() << "value helper reads outside its "
                                           "explicit inputs and captures";
      }
      if (!isa<func::CallOp, func::ReturnOp, YieldOp>(operation) &&
          !detail::isCombinationalHardwareOperation(&operation))
        return operation.emitOpError() << "unsupported operation in pure value "
                                         "helper body";
      if (operation.getNumRegions() &&
          !isa<TableMapOp, TableMatchOp>(operation))
        return operation.emitOpError() << "unsupported value helper region";
      if (isa<func::CallOp>(operation)) {
        auto callee = operation.getAttrOfType<FlatSymbolRefAttr>("callee");
        auto target = callee ? lookup(callee) : func::FuncOp();
        if (!target || failed(preflightSignature(target, work)))
          return operation.emitOpError() << "value helper call requires a "
                                           "well-formed helper signature";
      }
      for (Region &nested : operation.getRegions())
        if (failed(visit(nested, depth + 1)))
          return failure();
    }
    return success();
  };
  return visit(helper.getBody(), 0);
}

LogicalResult verifySignature(func::FuncOp helper, uint64_t &work) {
  if (failed(preflightSignature(helper, work)))
    return failure();
  auto error = [&] { return helper.emitOpError(); };
  auto kind = helper->getAttrOfType<StringAttr>("ac.helper_kind");
  auto owner = helper->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto origin = helper->getAttrOfType<DictionaryAttr>("ac.origin");
  auto role = helper->getAttrOfType<StringAttr>("ac.declaration_role");
  auto parameters = helper->getAttrOfType<ArrayAttr>("ac.parameters");
  auto constraints = helper->getAttrOfType<ArrayAttr>("ac.result_constraints");
  auto captures = helper->getAttrOfType<ArrayAttr>("ac.capture_types");
  auto form = helper->getAttrOfType<StringAttr>("ac.return_form");
  auto symbol = FlatSymbolRefAttr::get(helper.getContext(), helper.getSymName());
  auto package = helper->getParentOfType<mlir::ModuleOp>();
  if (!kind || kind.getValue() != "value" || !package ||
      helper->getParentOp() != package || !parameters || !constraints ||
      !captures || !form || helper->hasAttr("ac.check_templates") ||
      helper->hasAttr("ac.record") || helper->hasAttr("ac.evaluation_path") ||
      failed(detail::verifyDeclarationMetadata(helper, owner, origin, role)) ||
      failed(detail::verifyOriginDefinition(origin, symbol, "value helper", error)))
    return error() << "value helper requires the ordinary hardware contract";
  if (!detail::hasQualifiedDeclarationIdentityForOwner(helper, owner))
    return error() << "value helper symbol does not match its SourceOwner";
  if (parameters.size() > maxWork || captures.size() > maxWork)
    return error() << "value helper signature budget exceeded";
  FunctionType signature = helper.getFunctionType();
  if (signature.getNumInputs() != parameters.size() + captures.size() ||
      signature.getNumResults() != constraints.size() ||
      constraints.size() > 1 ||
      form.getValue() != (constraints.empty() ? "none" : "single"))
    return error() << "value helper signature must contain explicit inputs, "
                      "capture suffix, and zero or one ordinary result";
  auto lexicalOwner = helper->getAttrOfType<FlatSymbolRefAttr>("ac.lexical_owner");
  if (helper->hasAttr("ac.lexical_owner") && !lexicalOwner)
    return error() << "value helper lexical owner must be a FlatSymbolRefAttr";
  if (lexicalOwner) {
    auto visibility = helper->getAttrOfType<StringAttr>("sym_visibility");
    auto declaration = SymbolTable::lookupSymbolIn(package, lexicalOwner);
    if (!visibility || visibility.getValue() != "private" || !declaration ||
        !isa<ModuleOp, ModuleImportOp>(declaration) ||
        detail::declarationSourceOwner(declaration) != owner ||
        !helper.getSymName().starts_with(
            (Twine(lexicalOwner.getValue()) + ".").str()))
      return error() << "nested value helper requires its private lexical "
                        "module declaration";
  } else if (!captures.empty()) {
    return error() << "top-level value helper cannot capture runtime values";
  }
  HardwareAnalysis analysis(package);
  HardwareBindings bindings;
  bindings.owner = helper;
  llvm::StringSet<> names;
  for (auto [index, raw] : llvm::enumerate(parameters)) {
    auto parameter = dyn_cast<DictionaryAttr>(raw);
    auto name = parameter ? parameter.getAs<StringAttr>("name") : StringAttr();
    if (!name || name.getValue().empty() ||
        !names.insert(name.getValue()).second)
      return error() << "value helper parameter requires a unique nonempty name";
    if (failed(detail::verifyRequiredSourceParameter(
            raw, name, owner, symbol, helper, "value helper")) ||
        failed(detail::verifySourceCallHardwareConstraint(
            parameter.get("constraint"), signature.getInput(index), analysis,
            helper, helper, "value helper parameter")))
      return failure();
  }
  for (auto [index, raw] : llvm::enumerate(captures)) {
    auto type = dyn_cast<TypeAttr>(raw);
    Type physical = signature.getInput(parameters.size() + index);
    if (!type || type.getValue() != physical || !payload(physical) ||
        failed(detail::verifyHardwareTypeScope(physical, helper)))
      return error() << "value helper capture suffix must match hardware types";
    auto width = analysis.getPackedWidth(physical, bindings, helper);
    if (failed(width))
      return failure();
    if (!*width)
      return error() << "value helper capture must have positive packed width";
  }
  for (auto [index, raw] : llvm::enumerate(constraints))
    if (failed(detail::verifySourceCallHardwareConstraint(
            raw, signature.getResult(index), analysis, helper, helper,
            "value helper result")))
      return failure();
  return success();
}

LogicalResult verifyBody(func::FuncOp helper, SourceHelperLookup lookup,
                         uint64_t &work) {
  if (failed(preflightBody(helper, lookup, work)) ||
      failed(mlir::verify(helper)))
    return failure();
  auto returned = dyn_cast<func::ReturnOp>(helper.getBody().front().back());
  if (!returned ||
      !llvm::equal(returned.getOperandTypes(),
                   helper.getFunctionType().getResults()) ||
      !llvm::equal(helper.getBody().front().getArgumentTypes(),
                   helper.getFunctionType().getInputs()))
    return helper.emitOpError() << "value helper block and return must match "
                                  "its physical signature";
  HardwareAnalysis analysis(helper->getParentOfType<mlir::ModuleOp>());
  HardwareBindings bindings;
  bindings.owner = helper;
  std::function<LogicalResult(Region &, unsigned)> visit =
      [&](Region &region, unsigned depth) -> LogicalResult {
    if (depth > maxDepth || !region.hasOneBlock())
      return helper.emitOpError() << "value helper region depth or shape invalid";
    for (Operation &operation : region.front()) {
      if (++work > maxWork)
        return helper.emitOpError() << "value helper verification budget exceeded";
      for (Value operand : operation.getOperands()) {
        Operation *scope = operand.getParentRegion()->getParentOp();
        if (scope != helper && !helper->isProperAncestor(scope))
          return operation.emitOpError() << "value helper reads outside its "
                                           "explicit inputs and captures";
      }
      if (auto call = dyn_cast<func::CallOp>(operation)) {
        auto target = lookup(call.getCalleeAttr());
        auto kind = target ? target->getAttrOfType<StringAttr>("ac.helper_kind")
                           : StringAttr();
        if (!target || !kind || kind.getValue() != "value" ||
            failed(verifySignature(target, work)) ||
            target.getFunctionType() != call.getCalleeType())
          return call.emitOpError() << "value helper call requires a resolved "
                                      "ordinary helper with exact signature";
        auto lexical = target->getAttrOfType<FlatSymbolRefAttr>("ac.lexical_owner");
        if (lexical && lexical !=
                helper->getAttrOfType<FlatSymbolRefAttr>("ac.lexical_owner"))
          return call.emitOpError() << "private helper call crosses lexical owner";
        continue;
      }
      if (isa<func::ReturnOp>(operation)) {
        if (&operation != returned.getOperation())
          return operation.emitOpError() << "value helper cannot return early";
        continue;
      }
      if (isa<YieldOp>(operation)) {
        if (&region == &helper.getBody() ||
            &operation != &region.front().back())
          return operation.emitOpError() << "value helper region yield is invalid";
        continue;
      }
      bool hardware = detail::isCombinationalHardwareOperation(&operation);
      if (!hardware)
        return operation.emitOpError() << "unsupported operation in pure value "
                                         "helper body";
      if (operation.getNumRegions() &&
          !isa<TableMapOp, TableMatchOp>(operation))
        return operation.emitOpError() << "unsupported value helper region";
      for (Type type : operation.getOperandTypes())
        if (failed(detail::verifyHardwareTypeScope(type, helper)))
          return failure();
      for (Type type : operation.getResultTypes())
        if (failed(detail::verifyHardwareTypeScope(type, helper)))
          return failure();
      for (NamedAttribute attribute : operation.getAttrs())
        if (failed(detail::verifyHardwareAttributeScope(attribute.getValue(), helper)))
          return failure();
      if (hardware && failed(analysis.verifyResolvedOperation(&operation, bindings)))
        return failure();
      for (Region &nested : operation.getRegions())
        if (failed(visit(nested, depth + 1)))
          return failure();
    }
    return success();
  };
  return visit(helper.getBody(), 0);
}
} // namespace

LogicalResult verifySourceValueHelper(func::FuncOp helper,
                                      SourceHelperLookup lookup) {
  uint64_t work = 0;
  if (failed(verifySignature(helper, work)))
    return failure();
  return verifyBody(helper, lookup, work);
}

LogicalResult verifySourceValueHelperClosure(ArrayRef<func::FuncOp> helpers,
                                             SourceHelperLookup lookup) {
  uint64_t work = 0;
  llvm::DenseMap<Operation *, unsigned> marks, heights;
  std::function<LogicalResult(func::FuncOp, unsigned)> visit =
      [&](func::FuncOp helper, unsigned depth) -> LogicalResult {
    if (failed(preflightSignature(helper, work)))
      return failure();
    if (depth > maxDepth)
      return helper.emitOpError() << "value helper closure budget exceeded";
    unsigned mark = marks.lookup(helper);
    if (mark == 1)
      return helper.emitOpError() << "recursive value helper call graph";
    if (mark == 2)
      return success();
    if (failed(verifySignature(helper, work)) ||
        failed(verifyBody(helper, lookup, work)))
      return failure();
    marks[helper] = 1;
    unsigned height = 0;
    auto result = helper.walk([&](func::CallOp call) {
      auto target = lookup(call.getCalleeAttr());
      if (failed(visit(target, depth + 1)))
        return WalkResult::interrupt();
      height = std::max(height, heights.lookup(target) + 1);
      return WalkResult::advance();
    });
    if (result.wasInterrupted())
      return failure();
    if (height > maxDepth)
      return helper.emitOpError() << "value helper closure depth exceeded";
    heights[helper] = height;
    marks[helper] = 2;
    return success();
  };
  for (func::FuncOp helper : helpers)
    if (failed(visit(helper, 0)))
      return failure();
  return success();
}
} // namespace acir::ac
