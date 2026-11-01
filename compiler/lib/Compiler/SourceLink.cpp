#include "SourceLink.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "SourceHeaderHelpers.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/AttrTypeSubElements.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/TransactionalAnalysis.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "pycircuit/Dialect/ACIR/TransactionalRouting.h"
#include "pycircuit/Transforms/ExpandStaticFor.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/raw_ostream.h"

#include <functional>

using namespace mlir;

namespace acir::compiler {
namespace {

// Bind references to actual allocation occurrences before common analysis.
// Each clone remains a Module with its own allocations, Methods and child
// instances. Specialization changes static signatures, never storage identity.
class ReferenceLinker {
  struct Occurrence {
    ac::ModuleOp original;
    ac::InstanceOp allocation;
    unsigned parent;
    std::string name;
    llvm::StringMap<unsigned> children;
    ac::ModuleOp linked;
    ac::HardwareBindings bindings;
  };
  static constexpr unsigned NoParent = ~0u;
  ModuleOp package;
  ac::HardwareAnalysis hardware;
  SmallVector<Occurrence> instances;
  SmallVector<OwningOpRef<ac::ModuleOp>> working;
  uint64_t remaining = ac::HardwareSourceCheckLimits{}.maxWorkUnits;
  LogicalResult debit(uint64_t units, Operation *site) {
    if (units > remaining)
      return site->emitOpError("static instance expansion budget exceeded");
    remaining -= units;
    return success();
  }
  llvm::DenseSet<Operation *> expanding;
  llvm::DenseSet<Operation *> tableInstances;
  llvm::DenseSet<std::pair<unsigned, Value>> resolving;

  Operation *lookup(StringRef name) {
    return SymbolTable::lookupSymbolIn(package, name);
  }
  FailureOr<uint64_t> tableCapacity(ac::TableType type, Operation *site,
                                    const ac::HardwareBindings &bindings) {
    auto evaluated = hardware.evaluateStatic(type.getCapacity(), bindings, site);
    auto integer = succeeded(evaluated) ? dyn_cast<ac::MathIntAttr>(*evaluated)
                                        : ac::MathIntAttr();
    if (!integer) return site->emitOpError("Table capacity requires a static Integer");
    llvm::APSInt value(integer.getCanonicalValue());
    if (value.isNegative() || value.isZero() || value.getActiveBits() > 63)
      return site->emitOpError("Table capacity requires a positive i64");
    return value.getZExtValue();
  }
  FailureOr<SmallVector<Value>> tableMembers(unsigned owner, Value value,
                                            unsigned depth = 0) {
    if (depth >= 128) return failure();
    if (auto create = value.getDefiningOp<ac::TableCreateOp>())
      return SmallVector<Value>(create.getMembers());
    auto arg = dyn_cast<BlockArgument>(value);
    if (!arg) return failure();
    auto *scope = arg.getOwner()->getParentOp();
    if (scope == instances[owner].linked) {
      auto &instance = instances[owner];
      if (instance.parent == NoParent ||
          arg.getArgNumber() >= instance.allocation.getNumOperands())
        return failure();
      return tableMembers(instance.parent,
          instance.allocation.getOperand(arg.getArgNumber()), depth + 1);
    }
    if (isa<ac::TransactionRuleOp, ac::TransactionMethodOp>(scope)) {
      auto sig = cast<FunctionType>(scope->getAttrOfType<TypeAttr>("function_type").getValue());
      unsigned offset = sig.getNumInputs();
      if (isa<ac::TransactionRuleOp>(scope)) offset += sig.getNumResults();
      if (arg.getArgNumber() >= offset && arg.getArgNumber() - offset < scope->getNumOperands())
        return tableMembers(owner, scope->getOperand(arg.getArgNumber() - offset), depth + 1);
    }
    return failure();
  }
  std::optional<llvm::APSInt> constantTableIndex(Value value, unsigned owner,
                                                Operation *site, unsigned depth = 0) {
    if (depth >= 128) return std::nullopt;
    if (auto constant = value.getDefiningOp<ac::BitsConstantOp>()) {
      auto evaluated = hardware.evaluateStatic(constant.getValue(), instances[owner].bindings, site);
      auto number = succeeded(evaluated) ? dyn_cast<ac::MathIntAttr>(*evaluated) : ac::MathIntAttr();
      if (number) return llvm::APSInt(number.getCanonicalValue());
    }
    auto pending = value.getDefiningOp<ac::PendingExprOp>();
    if (!pending) return std::nullopt;
    if (pending.getKind() == "literal") {
      auto number = pending->getAttrOfType<ac::MathIntAttr>("number");
      if (number) return llvm::APSInt(number.getCanonicalValue());
      if (auto expression = pending->getAttrOfType<ac::StaticExprAttr>("number")) {
        auto evaluated = hardware.evaluateStatic(expression, instances[owner].bindings, site);
        if (succeeded(evaluated))
          if (auto integer = dyn_cast<ac::MathIntAttr>(*evaluated))
            return llvm::APSInt(integer.getCanonicalValue());
      }
    }
    if (pending.getKind() == "convert" && pending.getNumOperands() == 1) {
      auto number = constantTableIndex(pending.getOperand(0), owner, site, depth + 1);
      if (!number) return std::nullopt;
      if (isa<ac::InferType>(pending.getResult().getType())) return number;
      if (!isa<ac::BitsType>(pending.getResult().getType())) return std::nullopt;
      auto width = hardware.getPackedWidth(pending.getResult().getType(), instances[owner].bindings, site);
      if (succeeded(width) && !number->isNegative() && number->getActiveBits() <= *width)
        return number;
    }
    return std::nullopt;
  }
  LogicalResult expandTables(unsigned owner) {
    auto module = instances[owner].linked;
    SmallVector<ac::TableAllocOp> allocations;
    module.walk([&](ac::TableAllocOp op) { allocations.push_back(op); });
    llvm::StringSet<> names;
    for (Operation &operation : module.getBody().front())
      if (auto name = operation.getAttrOfType<StringAttr>("instance_name"))
        names.insert(name.getValue());
    auto uniqueName = [&](std::string base) {
      std::string name = base;
      for (uint64_t suffix = 1; !names.insert(name).second; ++suffix)
        name = base + "__" + std::to_string(suffix);
      return name;
    };
    for (auto allocation : allocations) {
      auto type = allocation.getResult().getType();
      auto count = tableCapacity(type, allocation, instances[owner].bindings);
      if (failed(count) || failed(debit(succeeded(count) ? *count : 0, allocation)))
        return failure();
      if ((allocation.getInitialImage() || allocation.getInitial().size() > 1) &&
          allocation.getInitial().size() != *count)
        return allocation.emitOpError("Table initializer image must cover its capacity");
      OpBuilder b(allocation);
      SmallVector<Value> members;
      for (uint64_t i = 0; i < *count; ++i) {
        std::string name = uniqueName(allocation.getInstanceName().str() +
                                      "__table_" + std::to_string(i));
        auto origin = allocation->getAttrOfType<DictionaryAttr>("ac.origin");
        if (!origin) origin = allocation->getAttrOfType<DictionaryAttr>("occurrence");
        DictionaryAttr memberOrigin;
        if (origin) {
          SmallVector<Attribute> frames(origin.getAs<ArrayAttr>("expansion").getValue());
          frames.push_back(b.getDictionaryAttr({
              b.getNamedAttr("kind", b.getStringAttr("iteration")),
              b.getNamedAttr("site", origin.getAs<DictionaryAttr>("site")),
              b.getNamedAttr("ordinal", b.getI64IntegerAttr(i)),
              b.getNamedAttr("value", b.getDictionaryAttr({
                  b.getNamedAttr("kind", b.getStringAttr("integer")),
                  b.getNamedAttr("value", ac::MathIntAttr::get(package.getContext(),
                      llvm::APSInt(llvm::APInt(64, i), true)))}))}));
          memberOrigin = b.getDictionaryAttr({
              b.getNamedAttr("site", origin.getAs<DictionaryAttr>("site")),
              b.getNamedAttr("expansion", b.getArrayAttr(frames))});
        }
        if (auto state = dyn_cast<ac::StateType>(type.getElementType())) {
          OperationState member(allocation.getLoc(), "ac.resource.alloc");
          member.addTypes(state);
          member.addAttribute("instance_name", b.getStringAttr(name));
          if (memberOrigin) member.addAttribute("ac.origin", memberOrigin);
          if (!allocation.getInitial().empty())
            member.addOperands(allocation.getInitial()[allocation.getInitial().size() == 1 ? 0 : i]);
          members.push_back(b.create(member)->getResult(0));
        } else {
          auto reference = cast<ac::ModuleRefType>(type.getElementType());
          auto provider = dyn_cast_or_null<ac::ModuleOp>(lookup(reference.getModule().getValue()));
          if (!provider || provider.getFunctionType().getNumInputs())
            return allocation.emitOpError("Module Table allocation requires a provider without runtime inputs");
          SmallVector<Value> boundOutputs;
          auto outputKinds = provider.getOutputKindsAttr();
          for (auto [output, raw] : llvm::enumerate(outputKinds)) {
            if (cast<StringAttr>(raw).getValue() != "bound") continue;
            Type type = provider.getFunctionType().getResult(output);
            Type payload = isa<ac::PrimitiveType>(type) ? cast<ac::PrimitiveType>(type).getPayload()
                           : isa<ac::StateType>(type) ? cast<ac::StateType>(type).getPayload()
                                                     : cast<ac::QueueType>(type).getPayload();
            OperationState connection(allocation.getLoc(), "ac.resource.connection");
            connection.addTypes(ac::PrimitiveType::get(package.getContext(), payload));
            connection.addAttribute("instance_name", b.getStringAttr(uniqueName(name + "__out_" + std::to_string(output))));
            if (memberOrigin) connection.addAttribute("ac.origin", memberOrigin);
            boundOutputs.push_back(b.create(connection)->getResult(0));
          }
          OperationState member(allocation.getLoc(), "ac.instance");
          member.addTypes(provider.getFunctionType().getResults());
          member.addOperands(boundOutputs);
          member.addAttribute("bound_count", b.getI64IntegerAttr(boundOutputs.size()));
          member.addAttribute("instance_name", b.getStringAttr(name));
          member.addAttribute("callee", reference.getModule());
          member.addAttribute("parameters", b.getArrayAttr({}));
          member.addAttribute("type_arguments", b.getArrayAttr({}));
          member.addAttribute("occurrence", memberOrigin ? memberOrigin : b.getDictionaryAttr({}));
          if (memberOrigin) member.addAttribute("ac.origin", memberOrigin);
          tableInstances.insert(b.create(member));
          OperationState ref(allocation.getLoc(), "ac.instance.ref");
          ref.addTypes(reference);
          if (memberOrigin) ref.addAttribute("ac.origin", memberOrigin);
          ref.addAttribute("instance_path", b.getStrArrayAttr({name}));
          members.push_back(b.create(ref)->getResult(0));
        }
      }
      OperationState create(allocation.getLoc(), "ac.table.create");
      create.addTypes(type);
      create.addOperands(members);
      if (auto origin = allocation->getAttr("ac.origin")) create.addAttribute("ac.origin", origin);
      allocation.getResult().replaceAllUsesWith(b.create(create)->getResult(0));
      allocation.erase();
    }
    SmallVector<ac::TableSelectOp> selections;
    module.walk([&](ac::TableSelectOp op) { selections.push_back(op); });
    for (auto select : selections) {
      auto number = constantTableIndex(select.getIndex(), owner, select);
      if (!number) continue;
      auto members = tableMembers(owner, select.getTable());
      if (failed(members)) continue;
      const auto &integer = *number;
      if (integer.isNegative() || integer.getActiveBits() > 63 || integer.getZExtValue() >= members->size())
        return select.emitOpError("Table index is outside its capacity");
      Value member = (*members)[integer.getZExtValue()];
      if (member.getParentRegion()->getParentOfType<ac::ModuleOp>() != module)
        continue; // Cross-Module bindings must retain their local Table formal.
      Operation *callable = select->getParentOp();
      while (callable && !isa<ac::TransactionRuleOp, ac::TransactionMethodOp, ac::ModuleOp>(callable))
        callable = callable->getParentOp();
      if (callable && isa<ac::TransactionRuleOp, ac::TransactionMethodOp>(callable)) {
        auto sig = cast<FunctionType>(callable->getAttrOfType<TypeAttr>("function_type").getValue());
        unsigned offset = sig.getNumInputs();
        if (isa<ac::TransactionRuleOp>(callable)) offset += sig.getNumResults();
        unsigned capture = 0;
        while (capture < callable->getNumOperands() && callable->getOperand(capture) != member)
          ++capture;
        if (capture == callable->getNumOperands()) {
          callable->insertOperands(capture, member);
          for (Region &region : callable->getRegions())
            region.front().addArgument(member.getType(), select.getLoc());
        }
        Region *region = select->getParentRegion();
        while (region->getParentOp() != callable) region = region->getParentRegion();
        member = region->front().getArgument(offset + capture);
      }
      Value index = select.getIndex();
      select.getResult().replaceAllUsesWith(member);
      select.erase();
      while (auto pending = index.getDefiningOp<ac::PendingExprOp>()) {
        if (pending.getKind() != "literal" && pending.getKind() != "convert") break;
        // constantTableIndex proved this complete chain has no resource demand.
        // Its evaluation markers must not keep an otherwise untyped literal alive.
        for (Operation *user : llvm::make_early_inc_range(index.getUsers()))
          if (isa<ac::PayloadUseOp>(user)) user->erase();
        if (!index.use_empty()) break;
        Value prior = pending.getNumOperands() == 1 ? pending.getOperand(0) : Value();
        pending.erase();
        if (!prior) break;
        index = prior;
      }
    }
    return success();
  }
  LogicalResult expand(ac::ModuleOp module, ac::InstanceOp allocation,
                       unsigned parent, ac::HardwareBindings bindings,
                       bool validationRoot = false) {
    if (instances.size() >= 4096 || expanding.size() >= 64 ||
        !expanding.insert(module).second)
      return module.emitOpError(
          "recursive or over-capacity Module instance closure");
    unsigned id = instances.size();
    std::string name = module.getSymName().str();
    if (parent != NoParent || validationRoot) {
      name += ".__instance_" + std::to_string(id);
      if (lookup(name))
        return module.emitOpError(
            "instance specialization name collides with a source declaration");
    }
    instances.push_back({module,
                         allocation,
                         parent,
                         std::move(name),
                         {},
                         {},
                         std::move(bindings)});
    uint64_t bodySize = 0;
    module.walk([&](Operation *) { ++bodySize; });
    if (failed(debit(bodySize, module)))
      return failure();
    working.push_back(
        OwningOpRef<ac::ModuleOp>(cast<ac::ModuleOp>(module->clone())));
    instances[id].linked = *working.back();
    Builder builder(package.getContext());
    // Qualified static expressions still name the authored template here.
    // Rebind every type/attribute, including unused Method bodies and capture
    // arguments, before changing the occurrence's symbol identity.
    AttrTypeReplacer replace;
    bool failedType = false;
    replace.addReplacement([&](ac::StaticExprAttr expr) -> Attribute {
      return hardware.substituteAttribute(expr, instances[id].bindings);
    });
    replace.recursivelyReplaceElementsIn(instances[id].linked, true, false,
                                         true);
    if (failed(expandStaticFor(instances[id].linked, hardware,
                               instances[id].bindings,
                               [&](uint64_t units, Operation *site) {
                                 return debit(units, site);
                               })))
      return failure();
    AttrTypeReplacer resolve;
    resolve.addReplacement([&](Type type) -> std::optional<Type> {
      if (auto table = dyn_cast<ac::TableType>(type)) {
        auto value = hardware.evaluateStatic(table.getCapacity(), instances[id].bindings,
                                             instances[id].original);
        if (failed(value)) { failedType = true; return type; }
        NamedAttrList tree;
        tree.set("origin", table.getCapacity().getTree().get("origin"));
        tree.set("location", table.getCapacity().getTree().get("location"));
        tree.set("kind", builder.getStringAttr("literal"));
        tree.set("value", builder.getDictionaryAttr({
            builder.getNamedAttr("kind", builder.getStringAttr("integer")),
            builder.getNamedAttr("value", *value)}));
        return ac::TableType::get(package.getContext(),
            ac::StaticExprAttr::get(package.getContext(), tree.getDictionary(package.getContext())),
            table.getElementType());
      }
      if (!isa<ac::BitsType, ac::ArrayType, ac::TypeParamType>(type))
        return std::nullopt;
      auto resolved = hardware.resolveType(type, instances[id].bindings,
                                           instances[id].original);
      if (failed(resolved)) {
        failedType = true;
        return type;
      }
      return *resolved;
    });
    resolve.recursivelyReplaceElementsIn(instances[id].linked, true, false,
                                         true);
    if (failedType || failed(expandTables(id)))
      return failure();
    for (auto allocation :
         instances[id].linked.getBody().front().getOps<ac::ResourceAllocOp>()) {
      auto depth = allocation->getAttrOfType<ac::StaticExprAttr>("depth");
      if (!depth)
        continue;
      auto evaluated = hardware.evaluateStatic(depth, instances[id].bindings,
                                               instances[id].original);
      auto integer = succeeded(evaluated)
                         ? dyn_cast<ac::MathIntAttr>(*evaluated)
                         : ac::MathIntAttr();
      if (!integer)
        return allocation.emitOpError("Queue depth requires a static Integer");
      llvm::APSInt value(integer.getCanonicalValue());
      if (value.isNegative() || value.isZero() || value.getActiveBits() > 63)
        return allocation.emitOpError("Queue depth requires a positive i64");
      allocation->setAttr("depth",
                          builder.getI64IntegerAttr(value.getZExtValue()));
    }
    instances[id].linked->setAttr("parameters", builder.getArrayAttr({}));
    instances[id].linked->setAttr("type_parameters", builder.getArrayAttr({}));
    instances[id].linked->setAttr(
        "sym_name", StringAttr::get(package.getContext(), instances[id].name));
    for (auto child :
         instances[id].linked.getBody().front().getOps<ac::InstanceOp>()) {
      auto target = dyn_cast_or_null<ac::ModuleOp>(lookup(child.getCallee()));
      if (!target)
        return child.emitOpError(
            "linked instance requires its explicit provider body");
      unsigned childID = instances.size();
      instances[id].children[child.getInstanceName()] = childID;
      auto childBindings = hardware.bindInstance(child, instances[id].bindings);
      if (failed(childBindings) ||
          failed(expand(target, child, id, *childBindings)))
        return failure();
      if (tableInstances.contains(child)) {
        auto specialized = instances[childID].linked.getFunctionType();
        unsigned bound = 0;
        for (auto [output, result] : llvm::enumerate(child.getResults())) {
          Type type = specialized.getResult(output);
          result.setType(type);
          if (cast<StringAttr>(target.getOutputKindsAttr()[output]).getValue() != "bound") continue;
          Type payload = isa<ac::PrimitiveType>(type) ? cast<ac::PrimitiveType>(type).getPayload()
                         : isa<ac::StateType>(type) ? cast<ac::StateType>(type).getPayload()
                                                   : cast<ac::QueueType>(type).getPayload();
          child.getOperand(bound++).setType(ac::PrimitiveType::get(package.getContext(), payload));
        }
      }
    }
    expanding.erase(module);
    return success();
  }
  FailureOr<unsigned> reference(unsigned owner, Value value) {
    auto key = std::make_pair(owner, value);
    if (resolving.size() >= 128 || !resolving.insert(key).second) {
      emitError(value.getLoc(),
                "cyclic or over-depth Module reference binding");
      return failure();
    }
    auto cleanup = llvm::scope_exit([&] { resolving.erase(key); });
    if (auto arg = dyn_cast<BlockArgument>(value)) {
      auto *scope = arg.getOwner()->getParentOp();
      if (scope == instances[owner].linked) {
        auto &instance = instances[owner];
        if (instance.parent == NoParent ||
            arg.getArgNumber() >= instance.allocation.getNumOperands()) {
          instance.original.emitOpError(
              "Module reference is not bound by an existing parent instance");
          return failure();
        }
        return reference(instance.parent,
                         instance.allocation.getOperand(arg.getArgNumber()));
      }
      if (isa<ac::TransactionRuleOp, ac::TransactionMethodOp>(scope)) {
        auto sig = cast<FunctionType>(
            scope->getAttrOfType<TypeAttr>("function_type").getValue());
        unsigned explicitCount = sig.getNumInputs();
        if (isa<ac::TransactionRuleOp>(scope))
          explicitCount += sig.getNumResults();
        if (arg.getArgNumber() >= explicitCount &&
            arg.getArgNumber() - explicitCount < scope->getNumOperands())
          return reference(
              owner, scope->getOperand(arg.getArgNumber() - explicitCount));
      }
    }
    if (auto selected = value.getDefiningOp<ac::TableSelectOp>()) {
      auto members = tableMembers(owner, selected.getTable());
      if (failed(members) || members->empty())
        return selected.emitOpError("Table selection has no bound member closure");
      // This lookup supplies the common provider interface only. The selection
      // remains in Linked ACIR; common analysis retains every actual receiver.
      Value representative = members->front();
      auto declaration = representative.getParentRegion()->getParentOfType<ac::ModuleOp>();
      for (auto [memberOwner, occurrence] : llvm::enumerate(instances))
        if (occurrence.linked == declaration)
          return reference(memberOwner, representative);
      return selected.emitOpError("Table member has no actual Module owner");
    }
    if (auto ref = value.getDefiningOp<ac::InstanceRefOp>()) {
      unsigned target = owner;
      for (Attribute raw : ref.getInstancePath()) {
        auto at =
            instances[target].children.find(cast<StringAttr>(raw).getValue());
        if (at == instances[target].children.end()) {
          ref.emitOpError(
              "Module reference path does not identify an allocated instance");
          return failure();
        }
        target = at->second;
      }
      return target;
    }
    emitError(value.getLoc(),
              "Module reference requires an existing instance or bound formal");
    return failure();
  }

public:
  explicit ReferenceLinker(ModuleOp package)
      : package(package), hardware(package) {}
  LogicalResult run(ac::ModuleOp root,
                    ArrayRef<SourceLinkParameterBinding> parameters) {
    Builder builder(package.getContext());
    NamedAttrList overrides;
    for (const auto &binding : parameters) {
      if (binding.name.empty() || overrides.get(binding.name))
        return root.emitOpError(
            "root parameter binding names must be nonempty and unique");
      auto value =
          ac::detail::parseMathIntAttr(package.getContext(), binding.value,
                                       [&] { return root.emitOpError(); });
      if (failed(value))
        return failure();
      overrides.append(binding.name, *value);
    }
    auto rootBindings =
        hardware.bindRoot(root, overrides.getDictionary(package.getContext()));
    if (failed(rootBindings))
      return failure();
    for (Operation &definition : package.getBody()->getOperations())
      if (isa<func::FuncOp, ac::TransactionRuleOp>(definition) &&
          failed(expandStaticFor(&definition, hardware, {},
                                 [&](uint64_t units, Operation *site) {
                                   return debit(units, site);
                                 })))
        return failure();
    if (failed(expand(root, {}, NoParent, *rootBindings)))
      return failure();
    // A selected entry does not excuse invalid default-bound declarations or
    // disconnected concrete instances. Retain their closed proof occurrences
    // in Linked IR, without selecting additional execution roots. Templates
    // needing actual arguments/resources remain checked at their real uses.
    for (auto module : package.getOps<ac::ModuleOp>()) {
      if (module.getBody().front().getNumArguments() ||
          !module.getTypeParameters().empty() ||
          !llvm::all_of(module.getParameters(), [](Attribute raw) {
            return bool(
                cast<DictionaryAttr>(raw).getAs<ac::StaticExprAttr>("default"));
          }))
        continue;
      auto defaults = hardware.bindRoot(module, builder.getDictionaryAttr({}));
      if (failed(defaults))
        return failure();
      bool alreadyBound = llvm::any_of(instances, [&](const Occurrence &prior) {
        if (prior.original != module || !prior.bindings.types.empty() ||
            prior.bindings.integers.size() != defaults->integers.size())
          return false;
        return llvm::all_of(defaults->integers, [&](const auto &binding) {
          auto found = prior.bindings.integers.find(binding.getKey());
          return found != prior.bindings.integers.end() &&
                 found->second == binding.getValue();
        });
      });
      if (!alreadyBound &&
          failed(expand(module, {}, NoParent, *defaults, true)))
        return failure();
    }
    for (auto &body : working) {
      package.getBody()->push_back(*body);
      body.release();
    }
    for (unsigned index = 0; index < instances.size(); ++index) {
      auto &instance = instances[index];
      bool failedReference = false;
      auto rewrite = [&](Value original) {
        if (failedReference || !isa<ac::ModuleRefType>(original.getType()))
          return;
        auto target = reference(index, original);
        if (failed(target)) {
          failedReference = true;
          return;
        }
        original.setType(ac::ModuleRefType::get(
            package.getContext(),
            FlatSymbolRefAttr::get(package.getContext(),
                                   instances[*target].name)));
      };
      instance.linked.walk([&](Operation *op) {
        for (Value result : op->getResults())
          rewrite(result);
        for (Region &region : op->getRegions())
          for (Block &block : region)
            for (BlockArgument argument : block.getArguments())
              rewrite(argument);
        if (auto allocation = dyn_cast<ac::InstanceOp>(op)) {
          auto child = instance.children.lookup(allocation.getInstanceName());
          op->setAttr("callee", FlatSymbolRefAttr::get(package.getContext(),
                                                       instances[child].name));
          op->setAttr("parameters", builder.getArrayAttr({}));
          op->setAttr("type_arguments", builder.getArrayAttr({}));
        }
      });
      if (failedReference)
        return failure();
      instance.linked.walk([&](Operation *op) {
        auto rewriteTable = [&](Value value) {
          auto table = dyn_cast<ac::TableType>(value.getType());
          if (!table) return;
          auto members = tableMembers(index, value);
          if (failed(members) || members->empty()) { failedReference = true; return; }
          value.setType(ac::TableType::get(package.getContext(), table.getCapacity(),
                                           members->front().getType()));
        };
        for (Value value : op->getResults()) rewriteTable(value);
        for (Region &region : op->getRegions())
          for (Block &block : region)
            for (BlockArgument argument : block.getArguments()) rewriteTable(argument);
      });
      if (failedReference)
        return instance.linked.emitOpError("Table requires a statically bound member closure");
      SmallVector<Type> bodyInputs(
          instance.linked.getBody().front().getArgumentTypes());
      auto resolvedSignature = instance.linked.getFunctionType();
      instance.linked->setAttr(
          "function_type",
          TypeAttr::get(FunctionType::get(
              package.getContext(),
              ArrayRef<Type>(bodyInputs)
                  .take_front(resolvedSignature.getNumInputs()),
              resolvedSignature.getResults())));
    }
    SmallVector<ac::ModuleOp> modules;
    for (auto &occurrence : instances)
      modules.push_back(occurrence.linked);
    if (failed(ac::resolveTransactionalResourceConnections(package, modules)))
      return failure();
    for (unsigned index = 0; index < instances.size(); ++index) {
      auto module = instances[index].linked;
      auto original = instances[index].original;
      for (auto [declaration, argument] :
           llvm::zip(original.getBody().front().getArguments(),
                     module.getBody().front().getArguments())) {
        auto ref = dyn_cast<ac::ModuleRefType>(declaration.getType());
        if (!ref)
          continue;
        auto target = reference(index, argument);
        if (failed(target))
          return failure();
        auto *requirement = lookup(ref.getModule().getValue());
        if (ac::isTransactionalReferenceRequirement(requirement) &&
            failed(ac::verifyTransactionalReferenceBinding(
                requirement, instances[*target].linked,
                instances[index].linked)))
          return failure();
      }
    }
    llvm::DenseSet<Operation *> retained;
    for (auto &instance : instances)
      retained.insert(instance.linked);
    for (Operation &op : llvm::make_early_inc_range(*package.getBody()))
      if (isa<ac::ModuleOp, ac::ModuleImportOp>(op) && !retained.contains(&op))
        op.erase();
    return success();
  }
};

LogicalResult preflightSourceLinkPair(SourceLinkUnit unit, unsigned index,
                                      ac::detail::EmitError emitError) {
  for (auto artifact : {unit.header, unit.body}) {
    auto version = artifact->getAttrOfType<IntegerAttr>("ac.transactional");
    if (!version || !version.getType().isSignlessInteger(64) ||
        version.getInt() != 2)
      return emitError() << "source linking requires transactional version 2; "
                            "regenerate this artifact";
  }
  for (auto artifact : {unit.header, unit.body}) {
    auto contract = artifact->getAttrOfType<IntegerAttr>("ac.source_contract");
    if (!contract || !contract.getType().isSignlessInteger(64) ||
        contract.getInt() != 5)
      return emitError() << "source linking requires source contract 5; "
                            "regenerate this artifact";
  }
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
      headerStage.getValue() != "unlinked")
    return emitError() << "source link unit[" << index
                       << "] owning header stage/unit_kind is invalid";
  if (!bodyKind ||
      (bodyKind.getValue() != "implementation" &&
       bodyKind.getValue() != "declarations") ||
      !bodyStage || bodyStage.getValue() != "unlinked")
    return emitError() << "source link unit[" << index
                       << "] body stage/unit_kind is invalid";
  for (StringRef name : {"ac.transactional", "ac.source_contract",
                         "ac.interfaces", "ac.exports", "ac.import_bindings"})
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

FailureOr<OwningOpRef<ModuleOp>>
linkSourceUnits(ArrayRef<SourceLinkUnit> units, StringRef top,
                ac::detail::EmitError emitError, SourceLinkTarget target,
                SourceLinkStages *retainedStages,
                ArrayRef<SourceLinkParameterBinding> parameters,
                StringRef resetSemantics) {
  if (resetSemantics != "strict" && resetSemantics != "if_one")
    return emitError() << "reset semantics must be strict or if_one";
  auto admitted = admitSourceLinkUnits(units, emitError);
  if (failed(admitted))
    return failure();
  SmallVector<SourceLinkUnit> orderedUnits(units);
  llvm::sort(orderedUnits,
             [](const SourceLinkUnit &left, const SourceLinkUnit &right) {
               return ac::detail::compareClosedSourceStructure(
                          left.header->getAttr("ac.source_owner"),
                          right.header->getAttr("ac.source_owner")) < 0;
             });
  units = orderedUnits;
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
          if (isa<ac::StructOp, ac::EnumOp, ac::ConstantOp,
                  ac::TransactionRuleOp, func::FuncOp>(operation) &&
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
  if (!root.getTypeParameters().empty() ||
      !root.getFunctionType().getInputs().empty())
    return root.emitOpError(
        "transactional link requires a closed concrete root");
  (*linked)->setAttr("ac.transactional", builder.getI64IntegerAttr(2));
  (*linked)->setAttr("ac.stage", builder.getStringAttr("linked"));
  SmallVector<Attribute> arguments;
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
  // Constants carry source authority only. Admission authenticated the exact
  // owning bodies and every imported snapshot before we made this clone. Uses
  // must already be ordinary literal payloads; provenance may retain the source
  // declaration identity, but executable attributes and types may not refer to
  // it.
  llvm::DenseSet<Attribute> constantSymbols;
  for (auto constant : linked->getOps<ac::ConstantOp>())
    constantSymbols.insert(
        FlatSymbolRefAttr::get(context, constant.getSymName()));
  if (!constantSymbols.empty()) {
    llvm::DenseSet<Attribute> checkedAttributes;
    llvm::DenseSet<Type> checkedTypes;
    bool unresolvedConstant = false;
    std::function<void(Attribute)> checkAttribute;
    std::function<void(Type)> checkType;
    checkAttribute = [&](Attribute attribute) {
      if (!attribute || !checkedAttributes.insert(attribute).second)
        return;
      if (auto symbol = dyn_cast<SymbolRefAttr>(attribute)) {
        if (constantSymbols.contains(FlatSymbolRefAttr::get(
                context, symbol.getRootReference().getValue())))
          unresolvedConstant = true;
        return;
      }
      if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
        for (NamedAttribute field : dictionary)
          if (field.getName() != "origin" && field.getName() != "location" &&
              field.getName() != "ac.origin")
            checkAttribute(field.getValue());
        return;
      }
      attribute.walkImmediateSubElements(checkAttribute, checkType);
    };
    checkType = [&](Type type) {
      if (type && checkedTypes.insert(type).second)
        type.walkImmediateSubElements(checkAttribute, checkType);
    };
    linked->walk([&](Operation *operation) {
      if (isa<ac::ConstantOp>(operation))
        return;
      for (NamedAttribute attribute : operation->getAttrs())
        if (attribute.getName() != "ac.origin")
          checkAttribute(attribute.getValue());
      for (Type type : operation->getOperandTypes())
        checkType(type);
      for (Type type : operation->getResultTypes())
        checkType(type);
      for (Region &region : operation->getRegions())
        for (Block &block : region)
          for (BlockArgument argument : block.getArguments())
            checkType(argument.getType());
    });
    if (unresolvedConstant)
      return emitError()
             << "source constant has a remaining semantic reference "
                "after literal materialization";
  }
  for (auto constant :
       llvm::make_early_inc_range(linked->getOps<ac::ConstantOp>()))
    constant.erase();
  // Native transformation checks IR semantics; canonical provider authority
  // stays with the admitted source registry, including after definition merge.
  for (auto imported : linked->getOps<ac::ModuleImportOp>())
    if (failed(admitted->verifyModuleImport(imported, emitError)))
      return failure();

  // Requirements describe interfaces only. Close them against the actual
  // instance graph while preserving every Module and owner boundary.
  if (failed(ReferenceLinker(*linked).run(root, parameters)))
    return failure();
  // Instance/resource identities are now concrete. Close Method requirements
  // and payload constraints before admitting linked ACIR to common analysis.
  if (failed(ac::resolveTransactionalMethodConstraints(*linked, true)) ||
      failed(ac::solveTransactionalPayloadConstraints(*linked, true)))
    return failure();
  return continueLinkedSource(*linked, emitError, target, retainedStages);
}

FailureOr<std::unique_ptr<ac::TransactionalPreparation>>
analyzeLinkedSource(ModuleOp source, ac::detail::EmitError emitError,
                    ac::TransactionalDebit debit) {
  auto owners = source->getAttrOfType<ArrayAttr>("ac.source_units");
  if (!owners || owners.empty())
    return emitError() << "linked artifact requires source owners";
  llvm::DenseSet<Attribute> sourceOwners;
  for (Attribute raw : owners) {
    auto owner = dyn_cast<DictionaryAttr>(raw);
    if (!owner || failed(ac::detail::verifySourceOwner(owner, emitError)) ||
        !sourceOwners.insert(owner).second)
      return emitError() << "linked artifact source owner inventory is invalid";
  }
  for (Operation &operation : source.getBody()->getOperations())
    if (!isa<ac::SystemOp>(operation)) {
      auto owner = operation.getAttrOfType<DictionaryAttr>("source_owner");
      if (!owner)
        owner = operation.getAttrOfType<DictionaryAttr>("ac.source_owner");
      if (!owner || !sourceOwners.contains(owner))
        return operation.emitOpError(
            "source owner is absent from linked inventory");
    }
  auto systems = source.getOps<ac::SystemOp>();
  if (!llvm::hasSingleElement(systems))
    return emitError() << "linked artifact requires one System entry";
  auto entry = (*systems.begin())->getAttrOfType<DictionaryAttr>("entry");
  auto top =
      entry ? entry.getAs<FlatSymbolRefAttr>("callee") : FlatSymbolRefAttr();
  if (!top)
    return emitError() << "linked artifact requires a selected root";
  return ac::TransactionalPreparation::create(source, top.getValue(), debit);
}

FailureOr<OwningOpRef<ModuleOp>>
continueLinkedSource(ModuleOp source, ac::detail::EmitError emitError,
                     SourceLinkTarget target,
                     SourceLinkStages *retainedStages) {
  ac::HardwareSourceCheckLimits limits;
  limits.maxWorkUnits = 16 * 1024 * 1024;
  ac::detail::SourceCheckBudget budget(limits);
  auto debit = [&](uint64_t units, Operation *site) {
    return budget.debit(units, site);
  };
  auto plan = analyzeLinkedSource(source, emitError, debit);
  if (failed(plan))
    return failure();
  if (target == SourceLinkTarget::Linked)
    return OwningOpRef<ModuleOp>(cast<ModuleOp>(source->clone()));
  SourceLinkStages stages;
  auto capture = [](ModuleOp module, std::string &text) {
    llvm::raw_string_ostream output(text);
    module->print(output, OpPrintingFlags().enableDebugInfo());
    output << '\n';
  };
  if (retainedStages)
    capture(source, stages.beforeTransactions);
  auto routed = ac::routeTransactionalPackage(**plan, debit);
  if (failed(routed))
    return failure();
  auto linked = std::move(*routed);
  if (retainedStages)
    capture(*linked, stages.routedTransactions);
  if (target == SourceLinkTarget::Hardware) {
    PassManager lowering(source.getContext());
    lowering.addPass(acir::createLowerTransactionsPass());
    if (failed(lowering.run(*linked)))
      return failure();
    if (retainedStages)
      capture(*linked, stages.afterTransactions);
  }
  if (retainedStages)
    *retainedStages = std::move(stages);
  return std::move(linked);
}

} // namespace acir::compiler
