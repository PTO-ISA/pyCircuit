#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceEffects.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/OpImplementation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/TypeSwitch.h"
#include <functional>
#include <optional>

using namespace mlir;
namespace acir::ac {
namespace {
mlir::ModuleOp packageOf(Operation *op) {
  for (auto *at = op; at; at = at->getParentOp())
    if (auto package = dyn_cast<mlir::ModuleOp>(at))
      if (package->hasAttr("ac.transactional")) return package;
  return {};
}
FunctionType signature(Operation *op) {
  auto attr = op ? op->getAttrOfType<TypeAttr>("function_type") : TypeAttr();
  return attr ? dyn_cast<FunctionType>(attr.getValue()) : FunctionType();
}
bool resource(Type type) { return isa<PrimitiveType, StateType, QueueType>(type); }
Type payload(Type type) {
  if (auto t = dyn_cast<PrimitiveType>(type)) return t.getPayload();
  if (auto t = dyn_cast<StateType>(type)) return t.getPayload();
  if (auto t = dyn_cast<QueueType>(type)) return t.getPayload();
  return type;
}
bool sameTypes(TypeRange a, TypeRange b) {
  if (a.size() != b.size()) return false;
  for (auto [x, y] : llvm::zip(a, b))
    if (!transactionalTypesEqual(x, y)) return false;
  return true;
}
bool purePayload(Operation *op) {
  return detail::isCombinationalHardwareOperation(op) &&
         !isa<ValueMergeOp, ValueRequireOp>(op);
}
Operation *lookup(Operation *site, StringRef name) {
  auto package = packageOf(site);
  if (!package) return nullptr;
  for (Operation &op : *package.getBody())
    if (auto symbol = op.getAttrOfType<StringAttr>("sym_name"))
      if (symbol.getValue() == name) return &op;
  return nullptr;
}
Operation *resolvePath(Operation *site, ArrayAttr path) {
  Operation *current = site->getParentOfType<ModuleOp>();
  if (!path || path.empty() || !current) return nullptr;
  for (auto raw : path) {
    auto name = dyn_cast<StringAttr>(raw);
    auto module = dyn_cast<ModuleOp>(current);
    if (!name || name.getValue().empty() || !module ||
        !module.getBody().hasOneBlock()) return nullptr;
    Operation *next = nullptr;
    for (auto instance : module.getBody().front().getOps<InstanceOp>())
      if (instance.getInstanceNameAttr() == name)
        next = lookup(site, instance.getCallee());
    if (!next) return nullptr;
    current = next;
  }
  return current;
}
// Reused from the prototype: attribute callbacks run before child verification.
// Check shape before accessing generated ODS operand/property accessors.
template <template <typename> class Trait>
LogicalResult shapeTrait(Operation *op) {
  return op->hasTrait<Trait>() ? Trait<ModuleOp>::verifyTrait(op) : success();
}
LogicalResult preflight(Operation *op) {
  if (failed(shapeTrait<OpTrait::AttrSizedOperandSegments>(op)) ||
      failed(shapeTrait<OpTrait::ZeroOperands>(op)) ||
      failed(shapeTrait<OpTrait::OneOperand>(op)) ||
      failed(shapeTrait<OpTrait::NOperands<2>::Impl>(op)) ||
      failed(shapeTrait<OpTrait::NOperands<3>::Impl>(op)) ||
      failed(shapeTrait<OpTrait::AtLeastNOperands<1>::Impl>(op)) ||
      failed(shapeTrait<OpTrait::AtLeastNOperands<2>::Impl>(op)) ||
      failed(shapeTrait<OpTrait::ZeroResults>(op)) ||
      failed(shapeTrait<OpTrait::OneResult>(op)) ||
      failed(shapeTrait<OpTrait::NResults<2>::Impl>(op)) ||
      failed(shapeTrait<OpTrait::ZeroRegions>(op)) ||
      failed(shapeTrait<OpTrait::OneRegion>(op))) return failure();
  if (isa<ModuleOp>(op) &&
      (!op->getRegion(0).hasOneBlock() || op->getRegion(0).front().empty()))
    return op->emitOpError("transactional module requires one nonempty block");
  return llvm::TypeSwitch<Operation *, LogicalResult>(op)
      .Case<ModuleOp, ModuleImportOp, StructOp, EnumOp, ResourceAllocOp,
            InstanceOp, InstanceRefOp, YieldOp, BitsConstantOp, BitsConstant4Op,
            BitsUnaryOp, BitsBinaryOp, BitsCompareOp, BitsSelectOp,
            BitsConcatOp, BitsExtractOp, BitsResizeOp, StructCreateOp,
            StructGetOp, EnumCreateOp, EnumToBitsOp, EnumFromBitsOp,
            TableCreateOp, TableSplatOp, TableViewOp, TableIndexOp,
            TableReadOp, TableGetOp, TableMapOp, TableMatchOp, TableChooseOp, TableFoldOp>(
          [](auto typed) { return typed.verifyInvariantsImpl(); })
      .Default([](Operation *) { return success(); });
}
LogicalResult ownedName(Operation *op) {
  auto parent = dyn_cast_or_null<ModuleOp>(op->getParentOp());
  auto name = op->getAttrOfType<StringAttr>("instance_name");
  if (!parent || !name || name.getValue().empty() || name.getValue().contains('\0'))
    return op->emitOpError("requires a nonempty name and direct module ownership");
  for (Operation &other : parent.getBody().front())
    if (&other != op && other.getAttrOfType<StringAttr>("instance_name") == name)
      return op->emitOpError("resource/instance name must be unique in its owner");
  return success();
}
FailureOr<SmallVector<Type>> bodyInputs(Operation *op) {
  auto sig = signature(op);
  auto kinds = op ? op->getAttrOfType<ArrayAttr>("output_kinds") : ArrayAttr();
  if (!sig || !kinds || kinds.size() != sig.getNumResults()) {
    op->emitOpError("output_kinds must match the complete output signature");
    return failure();
  }
  SmallVector<Type> result(sig.getInputs());
  for (auto [i, raw] : llvm::enumerate(kinds)) {
    auto kind = dyn_cast<StringAttr>(raw);
    if (!kind || (kind.getValue() != "bound" && kind.getValue() != "export")) {
      op->emitOpError("output_kinds entries must be bound or export");
      return failure();
    }
    if (!resource(sig.getResult(i))) {
      op->emitOpError("transactional outputs require resource references");
      return failure();
    }
    if (kind.getValue() == "bound") result.push_back(sig.getResult(i));
  }
  return result;
}
LogicalResult moduleSignature(Operation *op) {
  auto sig = signature(op);
  if (!isa_and_nonnull<mlir::ModuleOp>(op->getParentOp()) || !sig)
    return op->emitOpError("transactional module requires package placement and a signature");
  if (failed(bodyInputs(op))) return failure();
  llvm::StringSet<> names;
  for (auto [key, count] : {std::pair<StringRef, unsigned>{"input_names", sig.getNumInputs()},
                            {"output_names", sig.getNumResults()}}) {
    auto list = op->getAttrOfType<ArrayAttr>(key);
    if (!list || list.size() != count)
      return op->emitOpError() << key << " must match signature arity";
    for (auto raw : list) {
      auto name = dyn_cast<StringAttr>(raw);
      if (!name || name.getValue().empty() || name.getValue().contains('\0') ||
          !names.insert(name.getValue()).second)
        return op->emitOpError("port names must be nonempty and unique");
    }
  }
  auto parameters = op->getAttrOfType<ArrayAttr>("parameters");
  auto types = op->getAttrOfType<ArrayAttr>("type_parameters");
  if (!parameters || !types || !parameters.empty() || !types.empty())
    return op->emitOpError("transactional resource contract requires concrete module signatures");
  return detail::verifySourceOwner(op->getAttrOfType<DictionaryAttr>("source_owner"),
                                   [&] { return op->emitOpError(); });
}
} // namespace

bool isTransactionalPackage(Operation *op) { return bool(packageOf(op)); }
bool isLinkedTransactionalPackage(Operation *op) {
  auto package = packageOf(op);
  auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  return stage && stage.getValue() == "linked";
}
bool transactionalTypesEqual(Type a, Type b) {
  if (resource(a) || resource(b))
    return a.getTypeID() == b.getTypeID() &&
           areEquivalentHardwareTypes(payload(a), payload(b));
  if (isa<ModuleRefType>(a) || isa<ModuleRefType>(b)) return a == b;
  return areEquivalentHardwareTypes(a, b);
}
ParseResult InstanceOp::parse(OpAsmParser &parser, OperationState &result) {
  // Legacy normal printing retains the generic body after the bare op name.
  if (succeeded(parser.parseOptionalLParen())) {
    SmallVector<OpAsmParser::UnresolvedOperand> operands;
    if (parser.parseOperandList(operands) || parser.parseRParen()) return failure();
    return parser.parseGenericOperationAfterOpName(result, ArrayRef(operands));
  }
  StringAttr name;
  FlatSymbolRefAttr callee;
  SmallVector<OpAsmParser::UnresolvedOperand> inputs, bound;
  Type rawType;
  if (parser.parseAttribute(name, "instance_name", result.attributes) ||
      parser.parseAttribute(callee, "callee", result.attributes) ||
      parser.parseKeyword("inputs") ||
      parser.parseOperandList(inputs, OpAsmParser::Delimiter::Paren) ||
      parser.parseKeyword("bound") ||
      parser.parseOperandList(bound, OpAsmParser::Delimiter::Paren) ||
      parser.parseOptionalAttrDict(result.attributes) || parser.parseColonType(rawType))
    return failure();
  auto type = dyn_cast<FunctionType>(rawType);
  if (!type || type.getNumInputs() != inputs.size() + bound.size())
    return parser.emitError(parser.getCurrentLocation(), "instance type must cover inputs and bound operand groups");
  if (result.attributes.get("bound_count"))
    return parser.emitError(parser.getCurrentLocation(), "bound_count is represented by the bound operand group");
  result.addAttribute("bound_count", parser.getBuilder().getI64IntegerAttr(bound.size()));
  inputs.append(bound);
  if (parser.resolveOperands(inputs, type.getInputs(), parser.getCurrentLocation(), result.operands))
    return failure();
  result.addTypes(type.getResults());
  return success();
}
void InstanceOp::print(OpAsmPrinter &printer) {
  if (!isTransactionalPackage(*this)) {
    printer.printGenericOp(*this, false);
    return;
  }
  auto *target = lookup(*this, getCallee());
  auto sig = signature(target);
  unsigned count = sig ? std::min<unsigned>(sig.getNumInputs(), getNumOperands()) : getNumOperands();
  printer << " ";
  printer.printAttributeWithoutType(getInstanceNameAttr());
  printer << " ";
  printer.printAttributeWithoutType(getCalleeAttr());
  printer << " inputs(";
  printer.printOperands(getOperands().take_front(count));
  printer << ") bound(";
  printer.printOperands(getOperands().drop_front(count));
  printer << ")";
  printer.printOptionalAttrDict((*this)->getAttrs(), {"instance_name", "callee", "bound_count"});
  printer << " : ";
  printer.printFunctionalType(getOperandTypes(), getResultTypes());
}

LogicalResult ResourceAllocOp::verify() {
  if (!isTransactionalPackage(*this))
    return emitOpError("requires the transactional version 2 contract");
  return verifyTransactionalOperation(*this);
}
void ResourceAllocOp::getEffects(SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(MemoryEffects::Allocate::get(), SourceStateResource::get());
}
LogicalResult InstanceRefOp::verify() {
  if (!isTransactionalPackage(*this))
    return emitOpError("requires the transactional version 2 contract");
  return verifyTransactionalOperation(*this);
}
LogicalResult verifyTransactionalOperation(Operation *op) {
  if (isa<ModuleOp, ModuleImportOp>(op)) {
    if (failed(moduleSignature(op))) return failure();
    if (auto imported = dyn_cast<ModuleImportOp>(op)) {
      if (isLinkedTransactionalPackage(op))
        return op->emitOpError("linked transactional package cannot retain imports");
      if (imported.getSemanticKind() != "resources" ||
          imported.getDependencySummaryAttr() || imported.getTransactionSummaryAttr() ||
          imported.getPrimitiveKindAttr())
        return op->emitOpError("resource import requires semantic_kind resources without legacy summaries");
    }
    return success();
  }
  if (auto allocation = dyn_cast<ResourceAllocOp>(op)) {
    if (failed(ownedName(op))) return failure();
    Type type = allocation.getResource().getType();
    if (isa<PrimitiveType>(type))
      return op->emitOpError("primitive resources cannot be allocated");
    auto depth = allocation.getDepthAttr();
    if (isa<QueueType>(type)) {
      if (!depth || !depth.getType().isSignlessInteger(64) ||
          depth.getValue().isNegative() || depth.getValue().isZero())
        return op->emitOpError("Queue allocation requires an explicit positive i64 depth");
      if (!allocation.getInitial().empty())
        return op->emitOpError("Queue allocation cannot have initial contents");
    } else {
      if (depth) return op->emitOpError("State allocation cannot have Queue depth");
      if (allocation.getInitial().size() > 1)
        return op->emitOpError("State allocation accepts at most one initializer");
      if (!allocation.getInitial().empty()) {
        Value initial = allocation.getInitial().front();
        if (!transactionalTypesEqual(initial.getType(), payload(type)))
          return op->emitOpError("State initializer must match its logical payload type");
        llvm::DenseSet<Value> active, proven;
        llvm::DenseSet<Block *> localBlocks;
        std::function<bool(Value)> closed = [&](Value value) {
          if (auto arg = dyn_cast<BlockArgument>(value))
            return localBlocks.contains(arg.getOwner());
          if (proven.contains(value)) return true;
          if (!active.insert(value).second) return false;
          auto *def = value.getDefiningOp();
          if (!def || !purePayload(def) ||
              def->getParentOfType<ModuleOp>() != op->getParentOfType<ModuleOp>() ||
              !llvm::all_of(def->getOperands(), closed)) return false;
          for (Region &region : def->getRegions()) {
            for (Block &block : region) localBlocks.insert(&block);
            for (Block &block : region)
              for (Operation &child : block)
                if ((!purePayload(&child) && !isa<YieldOp>(child)) ||
                    !llvm::all_of(child.getOperands(), closed)) return false;
            for (Block &block : region) localBlocks.erase(&block);
          }
          active.erase(value); proven.insert(value); return true;
        };
        if (!closed(initial)) return op->emitOpError("State initializer must be a closed pure expression");
      }
    }
    return success();
  }
  if (auto instance = dyn_cast<InstanceOp>(op)) {
    if (failed(ownedName(op))) return failure();
    if (!instance.getParameters().empty() || !instance.getTypeArguments().empty())
      return op->emitOpError("transactional instances require concrete specialization");
    auto *target = lookup(op, instance.getCallee());
    if (!isa_and_nonnull<ModuleOp, ModuleImportOp>(target))
      return op->emitOpError("instance requires an explicit module definition or import");
    auto inputs = bodyInputs(target);
    if (failed(inputs)) return failure();
    if (auto count = instance.getBoundCountAttr())
      if (!count.getType().isSignlessInteger(64) || count.getValue().isNegative() ||
          count.getValue().getZExtValue() != inputs->size() - signature(target).getNumInputs())
        return op->emitOpError("printed bound operand group must match the callee bound output signature");
    if (!sameTypes(instance.getOperandTypes(), *inputs) ||
        !sameTypes(instance.getResultTypes(), signature(target).getResults()))
      return op->emitOpError("instance inputs, bound outputs or result types do not match the interface");
    if (!instance.getOccurrence().empty() &&
        failed(detail::verifyOccurrence(instance.getOccurrence(), [&] { return op->emitOpError(); })))
      return failure();
    return success();
  }
  if (auto ref = dyn_cast<InstanceRefOp>(op)) {
    if (!isa_and_nonnull<ModuleOp>(op->getParentOp()))
      return op->emitOpError("instance reference requires direct module placement");
    auto *target = resolvePath(op, ref.getInstancePath());
    auto symbol = target ? target->getAttrOfType<StringAttr>("sym_name") : StringAttr();
    if (!symbol || symbol.getValue() != ref.getReference().getType().getModule().getValue())
      return op->emitOpError("instance reference path must resolve to an existing instance of the declared module");
    return success();
  }
  if (isa<YieldOp>(op)) {
    if (!isa_and_nonnull<ModuleOp, TableMapOp, TableMatchOp>(op->getParentOp()))
      return op->emitOpError("resource contract yield must terminate a module or pure table region");
    return success();
  }
  return success();
}
LogicalResult verifyTransactionalRegions(Operation *op) {
  auto expected = bodyInputs(op);
  if (failed(expected)) return failure();
  auto &body = op->getRegion(0);
  if (!body.hasOneBlock() || body.front().empty() ||
      !sameTypes(body.front().getArgumentTypes(), *expected) ||
      !isa<YieldOp>(body.front().back()))
    return op->emitOpError("module body requires inputs then bound output arguments and final ac.yield");
  auto &end = body.front().back();
  auto sig = signature(op);
  if (!sameTypes(end.getOperandTypes(), sig.getResults()))
    return end.emitOpError("yield types must match the complete output signature");
  unsigned argument = sig.getNumInputs();
  auto kinds = op->getAttrOfType<ArrayAttr>("output_kinds");
  for (auto [i, raw] : llvm::enumerate(kinds))
    if (cast<StringAttr>(raw).getValue() == "bound")
      if (end.getOperand(i) != body.front().getArgument(argument++))
        return end.emitOpError("bound output must yield its original block argument");
  for (Operation &nested : body.front()) {
    if (&nested != &end && isa<YieldOp>(nested))
      return nested.emitOpError("early yield is invalid");
    for (Value value : nested.getOperands()) {
      if (auto arg = dyn_cast<BlockArgument>(value)) {
        if (arg.getOwner() != &body.front())
          return nested.emitOpError("operand must belong to this module scope");
      } else if (!value.getDefiningOp() || value.getDefiningOp()->getParentOp() != op)
        return nested.emitOpError("operand must belong to this module scope");
    }
  }
  return success();
}

LogicalResult verifyTransactionalPackage(mlir::ModuleOp package) {
  auto version = package->getAttrOfType<IntegerAttr>("ac.transactional");
  auto stage = package->getAttrOfType<StringAttr>("ac.stage");
  if (!version || !version.getType().isSignlessInteger(64) || version.getInt() != 2)
    return package.emitOpError("ac.transactional requires version 2 : i64; unsupported contracts never fall back");
  if (!stage || (stage.getValue() != "unlinked" && stage.getValue() != "linked"))
    return package.emitOpError("transactional version 2 requires unlinked or linked stage");
  bool linked = stage.getValue() == "linked";
  auto fail = package.walk([&](Operation *op) {
    if (failed(preflight(op))) return WalkResult::interrupt();
    return WalkResult::advance();
  });
  if (fail.wasInterrupted()) return failure();
  llvm::StringSet<> symbols;
  for (Operation &op : *package.getBody()) {
    if (!isa<ModuleOp, ModuleImportOp, StructOp, EnumOp>(op))
      return op.emitOpError("unsupported declaration in transactional resource contract");
    auto name = op.getAttrOfType<StringAttr>("sym_name");
    if (!name || name.getValue().empty() || !symbols.insert(name.getValue()).second)
      return op.emitOpError("package symbols must be nonempty and unique");
  }
  HardwareAnalysis analysis(package);
  std::function<LogicalResult(Type, Operation *)> checkType = [&](Type type, Operation *site) -> LogicalResult {
    if (linked && isa<PrimitiveType>(type))
      return site->emitOpError("linked package cannot retain primitive resources");
    if (auto ref = dyn_cast<ModuleRefType>(type)) {
      if (!isa_and_nonnull<ModuleOp, ModuleImportOp>(lookup(site, ref.getModule().getValue())))
        return site->emitOpError("module reference type requires a declared module");
      return success();
    }
    type = payload(type);
    if (!isa<BitsType, EnumType, StructType, TableType>(type))
      return site->emitOpError("transactional resource contract requires concrete hardware payloads");
    HardwareBindings bindings;
    bindings.owner = isa<ModuleOp, ModuleImportOp>(site) ? site : site->getParentOfType<ModuleOp>().getOperation();
    if (bindings.owner && failed(detail::verifyHardwareTypeScope(type, bindings.owner))) return failure();
    return success(succeeded(analysis.getPackedWidth(type, bindings, site)));
  };
  fail = package.walk([&](Operation *op) {
    bool root = op == package.getOperation();
    for (auto attr : op->getAttrs()) {
      auto name = attr.getName().getValue();
      bool schema = root
          ? (name == "ac.transactional" || name == "ac.stage" ||
             name == "sym_name" || name == "sym_visibility")
          : llvm::is_contained(op->getName().getAttributeNames(), attr.getName());
      bool provenance = isa<StructOp, EnumOp>(op) &&
                        (name == "ac.source_owner" || name == "ac.origin");
      bool visibility = isa<ModuleOp, ModuleImportOp, StructOp, EnumOp>(op) &&
                        name == "sym_visibility";
      if (!schema && !provenance && !visibility) {
        op->emitOpError() << "unsupported transactional contract field " << name;
        return WalkResult::interrupt();
      }
    }
    if (root) return WalkResult::advance();
    if ((!purePayload(op) && !isa<ModuleOp, ModuleImportOp, StructOp, EnumOp,
         ResourceAllocOp, InstanceOp, InstanceRefOp, YieldOp>(op)) ||
        (purePayload(op) && !op->getParentOfType<ModuleOp>())) {
      op->emitOpError("operation is outside the transactional resource-only contract; Methods and transactions are not supported");
      return WalkResult::interrupt();
    }
    if (op->hasAttr("primitive_kind") || op->hasAttr("transaction_summary") || op->hasAttr("dependency_summary")) {
      op->emitOpError("legacy or Method contract fields are not supported by transactional resources");
      return WalkResult::interrupt();
    }
    for (Type type : op->getOperandTypes()) if (failed(checkType(type, op))) return WalkResult::interrupt();
    for (Type type : op->getResultTypes()) if (failed(checkType(type, op))) return WalkResult::interrupt();
    if (auto sig = signature(op)) {
      for (Type type : sig.getInputs()) if (failed(checkType(type, op))) return WalkResult::interrupt();
      for (Type type : sig.getResults()) if (failed(checkType(type, op))) return WalkResult::interrupt();
    }
    if (failed(verifyTransactionalOperation(op))) return WalkResult::interrupt();
    if (isa<ModuleOp>(op) && failed(verifyTransactionalRegions(op))) return WalkResult::interrupt();
    return WalkResult::advance();
  });
  if (fail.wasInterrupted()) return failure();
  llvm::DenseMap<Operation *, unsigned> colors;
  std::function<LogicalResult(ModuleOp)> visit = [&](ModuleOp module) -> LogicalResult {
    auto color = colors.lookup(module);
    if (color == 1) return module.emitOpError("recursive module allocation is forbidden");
    if (color == 2) return success();
    colors[module] = 1;
    for (auto instance : module.getBody().front().getOps<InstanceOp>())
      if (auto target = dyn_cast_or_null<ModuleOp>(lookup(instance, instance.getCallee())))
        if (failed(visit(target))) return failure();
    colors[module] = 2;
    return success();
  };
  for (auto module : package.getOps<ModuleOp>()) if (failed(visit(module))) return failure();
  // Summaries distinguish a forwarded formal from a locally owned descendant.
  // They do not collapse identities: a descendant root is instantiated afresh
  // at each ac.instance. Following only formal aliases in the caller also
  // avoids confusing two occurrences of the same shared module definition.
  using Origin = std::optional<unsigned>;
  llvm::DenseMap<Operation *, SmallVector<Origin>> origins;
  std::function<LogicalResult(Operation *)> summarize = [&](Operation *declaration) -> LogicalResult {
    if (origins.count(declaration)) return success();
    auto sig = signature(declaration);
    SmallVector<Origin> outputs;
    if (isa<ModuleImportOp>(declaration)) {
      unsigned bound = sig.getNumInputs();
      for (auto kind : declaration->getAttrOfType<ArrayAttr>("output_kinds"))
        outputs.push_back(cast<StringAttr>(kind).getValue() == "bound" ? Origin(bound++) : std::nullopt);
    } else {
      auto module = cast<ModuleOp>(declaration);
      llvm::DenseMap<Value, Origin> known;
      llvm::DenseSet<Value> active;
      std::function<FailureOr<Origin>(Value)> resolve = [&](Value value) -> FailureOr<Origin> {
        if (auto found = known.find(value); found != known.end()) return found->second;
        if (!active.insert(value).second) {
          declaration->emitOpError("resource alias cycle has no existing allocation or formal owner");
          return failure();
        }
        Origin origin;
        if (auto arg = dyn_cast<BlockArgument>(value)) {
          if (arg.getOwner() != &module.getBody().front()) return failure();
          origin = arg.getArgNumber();
        } else if (auto instance = value.getDefiningOp<InstanceOp>()) {
          auto *target = lookup(instance, instance.getCallee());
          if (failed(summarize(target))) return failure();
          Origin targetOrigin = origins[target][cast<OpResult>(value).getResultNumber()];
          if (targetOrigin) {
            auto result = resolve(instance.getOperand(*targetOrigin));
            if (failed(result)) return failure();
            origin = *result;
          }
        } else if (!value.getDefiningOp<ResourceAllocOp>()) {
          declaration->emitOpError("resource reference requires an existing allocation, instance export or formal");
          return failure();
        }
        active.erase(value);
        known[value] = origin;
        return origin;
      };
      // Check unused results and bound inputs too: no ownerless graph survives
      // merely because the module does not export or consume it yet.
      for (Operation &nested : module.getBody().front()) {
        for (Value value : nested.getResults())
          if (resource(value.getType()) && failed(resolve(value))) return failure();
        for (Value value : nested.getOperands())
          if (resource(value.getType()) && failed(resolve(value))) return failure();
      }
      for (Value value : module.getBody().front().back().getOperands()) {
        auto result = resolve(value);
        if (failed(result)) return failure();
        outputs.push_back(*result);
      }
    }
    origins[declaration] = std::move(outputs);
    return success();
  };
  for (Operation &declaration : *package.getBody())
    if (isa<ModuleOp, ModuleImportOp>(declaration) && failed(summarize(&declaration))) return failure();
  return success();
}
} // namespace acir::ac
