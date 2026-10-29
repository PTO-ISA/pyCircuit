#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "ACIRSourceContracts.h"
#include "HardwareSourceCheckPreflight.h"
#include "SourceTransactionSummary.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/PointerUnion.h"
#include <functional>

using namespace mlir;
namespace acir::ac {
namespace {
enum class PredicateFact { Unknown, False, True };
struct Totality {
  bool total = false;
  Operation *guard = nullptr;
  Operation *call = nullptr;
  Operation *selection = nullptr;
  Operation *resource = nullptr;
};
struct Callable {
  Operation *module = nullptr;
  Operation *definition = nullptr;
  DictionaryAttr row;
  SymbolRefAttr symbol;
};
struct Call {
  Operation *definition = nullptr;
  DictionaryAttr row;
  ResolvedSourceMethod resolved;
};
class SummaryBuilder {
public:
  SummaryBuilder(llvm::DenseMap<Attribute, SourceMethodSummary> &methods,
                 llvm::DenseMap<Operation *, bool> &transactions)
      : methods(methods), transactions(transactions) {}
  LogicalResult run(ArrayRef<Operation *> modules, SourceModuleLookup lookup,
                    mlir::ModuleOp package = {}) {
    for (Operation *module : modules) {
      if (!charge(module))
        return failure();
      if (failed(verifySourceReferenceParameters(module))) return failure();
      if (auto references = module->getAttrOfType<ArrayAttr>("ac.reference_parameters"))
        for (Attribute raw : references) {
          if (!charge(module)) return failure();
          auto callee = cast<DictionaryAttr>(raw).getAs<FlatSymbolRefAttr>("callee");
          Operation *target = lookup(callee);
          if (!isa_and_nonnull<ModuleOp, ModuleImportOp>(target))
            return module->emitOpError() << "Module reference formal provider is absent from explicit source closure";
          auto envelope = module->getParentOfType<mlir::ModuleOp>();
          auto interfaces = envelope->getAttrOfType<ArrayAttr>("ac.interfaces");
          auto owner = target->getAttrOfType<DictionaryAttr>("source_owner");
          if (interfaces && owner && !llvm::is_contained(interfaces, owner))
            return module->emitOpError() << "Module reference provider is absent from caller source ac.interfaces closure";
        }
      HardwareBindings bindings;
      bindings.owner = module;
      if (auto body = dyn_cast<ModuleOp>(module)) {
        if (!body.getBody().hasOneBlock())
          return body.emitOpError()
                 << "source Method analysis requires one module body block";
        auto scanned = body.walk([&](Operation *op) {
          if (!charge(op, 1 + op->getNumOperands() + op->getNumResults()))
            return WalkResult::interrupt();
          for (Region &region : op->getRegions())
            for (Block &block : region)
              if (!charge(op, block.getNumArguments()))
                return WalkResult::interrupt();
          return WalkResult::advance();
        });
        if (scanned.wasInterrupted())
          return failure();
        for (Operation &op : body.getBody().front()) {
          if (auto method = dyn_cast<SourceMethodOp>(op)) {
            SourceMethodView view{module, method, {}};
            Callable callable{module, method, {}, view.getSymbol()};
            if (failed(collect(callable, bindings, lookup)))
              return failure();
            declarations.push_back(callable);
          } else if (auto transaction = dyn_cast<SourceTransactionOp>(op)) {
            Callable callable{module, transaction, {}, {}};
            if (failed(collect(callable, bindings, lookup)))
              return failure();
            registrations.push_back(callable);
          }
        }
      } else if (auto summary = module->getAttrOfType<DictionaryAttr>(
                     "transaction_summary")) {
        if (failed(verifySourceTransactionSummary(module, summary)))
          return failure();
        for (Attribute raw : summary.getAs<ArrayAttr>("methods")) {
          SourceMethodView view{module, {}, cast<DictionaryAttr>(raw)};
          Callable callable{module, nullptr, view.row, view.getSymbol()};
          if (failed(collect(callable, bindings, lookup)))
            return failure();
          declarations.push_back(callable);
        }
        for (Attribute raw : summary.getAs<ArrayAttr>("autonomous")) {
          Callable callable{module, nullptr, cast<DictionaryAttr>(raw), {}};
          if (failed(collect(callable, bindings, lookup)))
            return failure();
          registrations.push_back(callable);
        }
      }
    }
    for (Operation *module : modules) {
      HardwareBindings bindings;
      bindings.owner = module;
      if (failed(instances(module, lookup, bindings, 0, false)))
        return failure();
    }
    if (package && !package.getOps<SystemOp>().empty()) {
      HardwareAnalysis hardware(package);
      auto root = detail::resolveHardwareRootBindings(hardware);
      if (failed(root)) return failure();
      // Template proofs cannot discharge actual-bound body checks. Unallocated
      // generic templates do not acquire widths from declaration defaults.
      completedInstances.clear();
      if (failed(instances(root->owner, lookup, *root, 0, true)))
        return failure();
      // Emit also retains concrete scalar wrappers outside the selected root.
      // Validate those actual closures before unused Methods are erased.
      for (Operation *module : modules) {
        if (!charge(module)) return failure();
        auto definition = dyn_cast<ModuleOp>(module);
        if (!definition || !definition.getParameters().empty() ||
            !definition.getTypeParameters().empty() ||
            definition->hasAttr("ac.reference_parameters"))
          continue;
        HardwareBindings bindings;
        bindings.owner = module;
        if (failed(instances(module, lookup, bindings, 0, true)))
          return failure();
      }
    }
    for (const Callable &callable : declarations)
      nodes[callable.symbol] = callable;
    // All edges participate, including unreachable and known-false calls.
    llvm::DenseMap<Attribute, unsigned> marks;
    SmallVector<Attribute> order;
    struct Frame {
      Attribute symbol;
      size_t next = 0;
    };
    SmallVector<Frame> stack;
    for (const Callable &root : declarations) {
      if (marks.lookup(root.symbol))
        continue;
      marks[root.symbol] = 1;
      stack.push_back({root.symbol});
      while (!stack.empty()) {
        auto &frame = stack.back();
        auto node = nodes.find(frame.symbol);
        if (node == nodes.end())
          return root.module->emitOpError()
                 << "Method closure contains an unresolved Method";
        if (!charge(node->second.module))
          return failure();
        const auto &callees = methods.find(frame.symbol)->second.callees;
        if (frame.next == callees.size()) {
          marks[frame.symbol] = 2;
          order.push_back(frame.symbol);
          stack.pop_back();
          continue;
        }
        Attribute callee = callees[frame.next++];
        if (marks.lookup(callee) == 1)
          return node->second.module->emitOpError()
                 << "recursive Method call graph is unsupported";
        if (!marks.lookup(callee)) {
          marks[callee] = 1;
          stack.push_back({callee});
        }
      }
    }
    for (Attribute symbol : order) {
      const Callable &callable = nodes.find(symbol)->second;
      unsigned arity =
          callable.definition
              ? cast<SourceMethodOp>(callable.definition)
                    .getFunctionType()
                    .getNumInputs()
              : cast<FunctionType>(
                    callable.row.getAs<TypeAttr>("function_type").getValue())
                    .getNumInputs();
      SmallVector<PredicateFact> unknown(arity, PredicateFact::Unknown);
      auto proof = prove(callable, unknown, 0);
      if (exhausted)
        return failure();
      methods.find(symbol)->second.mayBlock = !proof.total;
    }
    for (const Callable &callable : registrations) {
      auto proof = prove(callable, {}, 0);
      if (exhausted)
        return failure();
      bool valued =
          callable.definition
              ? callable.definition->getNumResults() != 0
              : !callable.row.getAs<ArrayAttr>("result_types").empty();
      if (callable.definition)
        transactions[callable.definition] = proof.total;
      if (valued && !proof.total &&
          !callable.module->getAttrOfType<ArrayAttr>("ac.result_availability")) {
        auto diagnostic =
            (callable.definition ? callable.definition : callable.module)
                ->emitOpError();
        diagnostic << "ordinary transaction results require a total guard and "
                      "required Method calls, branch selections and queue resources; use -> "
                      "None for a potentially "
                      "guarded Rule";
        if (proof.resource)
          diagnostic.attachNote(proof.resource->getLoc())
              << "this required queue resource may block";
        if (proof.call)
          diagnostic.attachNote(proof.call->getLoc())
              << "this required Method call may block";
        if (proof.selection)
          diagnostic.attachNote(proof.selection->getLoc())
              << "effectful branch selector is not proven statically known";
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
  bool bindingCost(const HardwareBindings &bindings, Operation *site) {
    using Budget = detail::SourceCheckBudget;
    // StringMap copies retain bucket capacity as well as entries and keys.
    uint64_t buckets = Budget::add(bindings.integers.getNumBuckets(),
                                   bindings.types.getNumBuckets());
    return charge(site, Budget::add(Budget::bindings(bindings), Budget::add(
        buckets, Budget::bytes(Budget::add(
            Budget::multiply(buckets, sizeof(void *) + sizeof(unsigned)),
            2 * sizeof(void *))))));
  }
  LogicalResult boundCost(llvm::PointerUnion<Attribute, Type> node,
                          Operation *site, unsigned depth = 0) {
    if (!node) return success();
    if (depth > maxDepth)
      return site->emitOpError() << "bound Method static expression depth exceeded";
    if (!charge(site)) return failure();
    bool valid = true;
    auto attribute = [&](Attribute value) {
      if (valid && failed(boundCost(value, site, depth + 1))) valid = false;
    };
    auto type = [&](Type value) {
      if (valid && failed(boundCost(value, site, depth + 1))) valid = false;
    };
    if (auto value = dyn_cast<Attribute>(node)) {
      if (auto text = dyn_cast<StringAttr>(value))
        if (!charge(site, text.getValue().size())) return failure();
      if (auto integer = dyn_cast<MathIntAttr>(value))
        if (!charge(site, integer.getCanonicalValue().size())) return failure();
      value.walkImmediateSubElements(attribute, type);
    } else {
      cast<Type>(node).walkImmediateSubElements(attribute, type);
    }
    return success(valid);
  }
  static bool hardwareType(Type type) {
    return isa<BitsType, StructType, EnumType, TableType, TypeParamType>(type);
  }
  LogicalResult boundType(Type type, const HardwareBindings &bindings,
                          const HardwareAnalysis &hardware, Operation *site) {
    // Arithmetic/index transport keeps its existing source verifier.
    if (!hardwareType(type)) return success();
    if (!bindingCost(bindings, site) || failed(boundCost(type, site)))
      return failure();
    auto width = hardware.getPackedWidth(type, bindings, site);
    if (failed(width)) return failure();
    if (!*width)
      return site->emitOpError() << "bound Method payload requires positive width";
    return success();
  }
  LogicalResult boundMethodBody(Operation *operation,
                                const HardwareBindings &bindings,
                                const HardwareAnalysis &hardware,
                                unsigned depth = 0) {
    if (depth > maxDepth)
      return operation->emitOpError() << "bound Method region depth exceeded";
    if (!charge(operation, 1 + operation->getNumOperands() +
                               operation->getNumResults()) ||
        failed(boundCost(operation->getAttrDictionary(), operation)) ||
        failed(boundCost(operation->getPropertiesAsAttribute(), operation)))
      return failure();
    bool hardwareOperands = true;
    for (TypeRange types : {TypeRange(operation->getOperandTypes()),
                           TypeRange(operation->getResultTypes())})
      for (Type type : types) {
        hardwareOperands &= hardwareType(type);
        if (failed(boundType(type, bindings, hardware, operation)))
          return failure();
      }
    if (auto method = dyn_cast<SourceMethodOp>(operation))
      for (TypeRange types : {method.getFunctionType().getInputs(),
                             method.getFunctionType().getResults()})
        for (Type type : types)
          if (failed(boundType(type, bindings, hardware, operation)))
            return failure();
    // Reuse constant-fit/extract/resize/width semantics; mixed transport
    // operations retain their existing verifier instead of imposing Bits.
    if (hardwareOperands &&
        failed(hardware.verifyResolvedOperation(operation, bindings)))
      return failure();
    for (Region &region : operation->getRegions())
      for (Block &block : region) {
        if (!charge(operation, block.getNumArguments())) return failure();
        for (Type type : block.getArgumentTypes())
          if (failed(boundType(type, bindings, hardware, operation)))
            return failure();
        for (Operation &nested : block)
          if (failed(boundMethodBody(&nested, bindings, hardware, depth + 1)))
            return failure();
      }
    return success();
  }
  template <typename T, unsigned N>
  bool append(SmallVector<T, N> &values, T value, Operation *site) {
    if (values.size() == values.capacity()) {
      uint64_t capacity = values.capacity();
      if (capacity > (maxWork - 1) / 2)
        return charge(site, maxWork + 1);
      uint64_t next = capacity * 2 + 1;
      uint64_t units = 1 + sizeof(T) / 8 + (sizeof(T) % 8 != 0);
      if (next > values.max_size() ||
          next - capacity > (maxWork - work) / units)
        return charge(site, maxWork + 1);
      if (!charge(site, (next - capacity) * units) ||
          !charge(site, values.size()))
        return false;
      values.reserve(next);
    }
    if (!charge(site))
      return false;
    values.push_back(std::move(value));
    return true;
  }
  template <typename T>
  bool contains(ArrayRef<T> values, T value, Operation *site) {
    for (const T &candidate : values) {
      if (!charge(site))
        return false;
      if (candidate == value)
        return true;
    }
    return false;
  }
  LogicalResult collect(const Callable &callable,
                        const HardwareBindings &bindings,
                        SourceModuleLookup lookup) {
    auto key = callable.definition ? Attribute() : Attribute(callable.row);
    auto &calls =
        callable.definition ? nativeCalls[callable.definition] : rowCalls[key];
    auto add = [&](Operation *op, DictionaryAttr row) -> LogicalResult {
      if (!charge(callable.module))
        return failure();
      auto workStorage = [&](Operation *site, uint64_t units,
                             uint64_t bytes) -> LogicalResult {
        return success(charge(site, units) &&
                       charge(site, bytes / 8 + (bytes % 8 != 0)));
      };
      SourceReceiverProofFees fees{{}, {}, workStorage, {}};
      auto resolved =
          op ? resolveSourceMethodCall(cast<SourceMethodCallOp>(op), bindings,
                                       lookup, nullptr, &fees)
             : resolveSourceMethodCall(callable.module, row, bindings, lookup,
                                       callable.module, &fees);
      if (failed(resolved))
        return failure();
      auto symbol = resolved->method.getSymbol();
      bool terminal = resolved->method.isNativeTerminal();
      if (!append(calls, Call{op, row, std::move(*resolved)}, callable.module))
        return failure();
      if (callable.symbol && !terminal) {
        auto &summary = methods[callable.symbol];
        if (!contains<SymbolRefAttr>(summary.callees, symbol,
                                     callable.module)) {
          if (exhausted || !append(summary.callees, symbol, callable.module))
            return failure();
        }
      }
      return success();
    };
    if (callable.symbol)
      methods.try_emplace(callable.symbol);
    if (callable.definition) {
      // Classification is structural: every call counts, including empty or
      // read-only Methods and effects nested below a statically dead selector.
      auto visit = [&](auto &&visit, Operation *operation,
                       unsigned depth) -> FailureOr<bool> {
        if (!charge(operation))
          return failure();
        if (depth > maxDepth)
          return operation->emitOpError()
                 << "source Method callable traversal depth exceeded";
        if (auto call = dyn_cast<SourceMethodCallOp>(operation)) {
          Operation *parent = operation->getParentOp();
          while (isa_and_nonnull<SourceBranchOp>(parent)) {
            if (!charge(operation))
              return failure();
            parent = parent->getParentOp();
          }
          if (parent != callable.definition)
            return call.emitOpError() << "Method calls require callable body "
                                         "or branch arm placement";
          if (failed(add(call, {})))
            return failure();
          return true;
        }
        bool effectful = isa<SourceExpectOp, SourceObserveOp>(operation);
        for (Region &region : operation->getRegions())
          for (Block &block : region)
            for (Operation &child : block) {
              auto nested = visit(visit, &child, depth + 1);
              if (failed(nested))
                return failure();
              effectful |= *nested;
            }
        if (effectful && isa<SourceBranchOp>(operation)) {
          if (!append(nativeEffectfulBranches, operation, operation))
            return failure();
        }
        return effectful;
      };
      for (unsigned region : {0u, 1u})
        for (Operation &operation :
             callable.definition->getRegion(region).front())
          if (failed(visit(visit, &operation, 0)))
            return failure();
    } else {
      auto visit = [&](auto &&visit, ArrayAttr rows,
                       unsigned depth) -> FailureOr<bool> {
        if (!charge(callable.module))
          return failure();
        if (depth > maxDepth)
          return callable.module->emitOpError()
                 << "source Method summary traversal depth exceeded";
        bool effectful = false;
        for (Attribute raw : rows) {
          if (!charge(callable.module))
            return failure();
          auto row = cast<DictionaryAttr>(raw);
          auto kind = row.getAs<StringAttr>("kind").getValue();
          if (kind == "call") {
            if (failed(add(nullptr, row)))
              return failure();
            effectful = true;
          } else if (kind == "branch") {
            bool branchEffectful = false;
            for (StringRef arm : {"then_effects", "else_effects"}) {
              auto nested = visit(visit, row.getAs<ArrayAttr>(arm), depth + 1);
              if (failed(nested))
                return failure();
              branchEffectful |= *nested;
            }
            if (branchEffectful) {
              if (!append(rowEffectfulBranches, Attribute(row),
                          callable.module))
                return failure();
            }
            effectful |= branchEffectful;
          } else if (kind == "check" || kind == "observe")
            effectful = true;
        }
        return effectful;
      };
      for (StringRef field : {"guard_effects", "body_effects"})
        if (failed(visit(visit, callable.row.getAs<ArrayAttr>(field), 0)))
          return failure();
    }
    return success();
  }
  LogicalResult instances(Operation *module, SourceModuleLookup lookup,
                          const HardwareBindings &parent, unsigned depth,
                          bool verifyBodies) {
    if (!charge(module) || !bindingCost(parent, module))
      return failure();
    if (depth > maxDepth)
      return module->emitOpError()
             << "source Method instance closure depth exceeded";
    if (instanceMarks.lookup(module) == 1)
      return module->emitOpError()
             << "recursive instance graph cannot be published";
    if (auto found = completedInstances.find(module);
        found != completedInstances.end())
      for (const HardwareBindings &prior : found->second) {
        if (!bindingCost(prior, module)) return failure();
        if (detail::sameHardwareBindings(prior, parent)) return success();
      }
    instanceMarks[module] = 1;
    HardwareAnalysis hardware(module->getParentOfType<mlir::ModuleOp>());
    auto edge = [&](FlatSymbolRefAttr callee, ArrayAttr parameters,
                    ArrayAttr types, TypeRange inputs, TypeRange outputs,
                    Operation *site, Attribute references) -> LogicalResult {
      if (!charge(site))
        return failure();
      Operation *target = lookup(callee);
      if (failed(verifySourceReferenceBindings(module, target, references, lookup, site)))
        return failure();
      // The registry's authority may live in another header ModuleOp. Bind
      // through its authenticated local snapshot so TypeParam ownership agrees
      // with this HardwareAnalysis symbol table; retain registry authority for
      // closure traversal and the expected signature below.
      Operation *bindingTarget = hardware.lookupDefinition(callee.getValue());
      if (!bindingTarget) bindingTarget = target;
      if (bindingTarget != target) {
        if (!target || !isa<ModuleImportOp>(bindingTarget))
          return site->emitOpError() << "instance provider snapshot is invalid";
        for (StringRef field : {"source_owner", "function_type", "parameters",
                                "type_parameters", "primitive_kind", "ac.reference_parameters"})
          if (bindingTarget->getAttr(field) != target->getAttr(field))
            return site->emitOpError()
                   << "instance provider snapshot differs from authority: " << field;
      }
      auto bound = hardware.bindCallee(bindingTarget, parameters, types, parent, site);
      if (failed(bound))
        return failure();
      auto envelope = module->getParentOfType<mlir::ModuleOp>();
      auto interfaces = envelope->getAttrOfType<ArrayAttr>("ac.interfaces");
      auto owner = target->getAttrOfType<DictionaryAttr>("source_owner");
      // The interface constructor receives canonical primitive authorities;
      // registry snapshot checks precede this synchronous analysis.
      bool primitive =
          isa<ModuleImportOp>(target) && target->hasAttr("primitive_kind");
      if (interfaces && owner && !primitive &&
          !llvm::is_contained(interfaces, owner))
        return site->emitOpError() << "instance provider is absent from caller "
                                      "source ac.interfaces closure";
      auto signature = cast<FunctionType>(
          target->getAttrOfType<TypeAttr>("function_type").getValue());
      auto check = [&](TypeRange actual, TypeRange formal,
                        StringRef direction) -> LogicalResult {
        if (actual.size() != formal.size())
          return site->emitOpError() << "instance signature arity mismatch";
        for (auto [ordinal, pair] : llvm::enumerate(llvm::zip(actual, formal))) {
          auto [a, f] = pair;
          auto at = hardware.resolveType(a, parent, site),
               ft = hardware.resolveType(f, *bound, site);
          if (failed(at) || failed(ft))
            return failure();
          if (auto collection = dyn_cast<CollectionOp>(site)) {
            ft = hardware.resolveType(
                TableType::get(site->getContext(), collection.getShape(), *ft),
                parent, site);
            if (failed(ft))
              return failure();
          }
          if (!areEquivalentHardwareTypes(*at, *ft))
            return site->emitOpError()
                   << "instance payload type mismatch for " << direction << " "
                   << ordinal << " of " << callee << ": actual " << *at
                   << ", expected " << *ft;
        }
        return success();
      };
      if (failed(check(inputs, signature.getInputs(), "input")) ||
          failed(check(outputs, signature.getResults(), "output")))
        return failure();
      bound->owner = target;
      return instances(target, lookup, *bound, depth + 1, verifyBodies);
    };
    if (auto body = dyn_cast<ModuleOp>(module)) {
      for (Operation &op : body.getBody().front()) {
        // Include unused Methods and both arms: routing later removes them,
        // but their declarations must be valid for each actual occurrence.
        if (verifyBodies && isa<SourceMethodOp>(op) &&
            failed(boundMethodBody(&op, parent, hardware)))
          return failure();
        if (isa<InstanceOp, CollectionOp>(op) &&
            failed(edge(op.getAttrOfType<FlatSymbolRefAttr>("callee"),
                        op.getAttrOfType<ArrayAttr>("parameters"),
                        op.getAttrOfType<ArrayAttr>("type_arguments"),
                        op.getOperandTypes(), op.getResultTypes(), &op, op.getAttr("ac.reference_bindings"))))
          return failure();
      }
    } else if (auto summary = module->getAttrOfType<DictionaryAttr>(
                   "transaction_summary")) {
      for (Attribute raw : summary.getAs<ArrayAttr>("owners")) {
        auto row = cast<DictionaryAttr>(raw);
        if (row.getAs<StringAttr>("kind").getValue() != "instance")
          continue;
        SmallVector<Type> inputs, outputs;
        for (Attribute raw : row.getAs<ArrayAttr>("input_types"))
          inputs.push_back(cast<TypeAttr>(raw).getValue());
        for (Attribute raw : row.getAs<ArrayAttr>("result_types"))
          outputs.push_back(cast<TypeAttr>(raw).getValue());
        if (failed(edge(row.getAs<FlatSymbolRefAttr>("callee"),
                        row.getAs<ArrayAttr>("parameters"),
                        row.getAs<ArrayAttr>("type_arguments"), inputs, outputs,
                        module, row.get("reference_bindings"))))
          return failure();
      }
    }
    if (!completedInstances.contains(module)) {
      using Budget = detail::SourceCheckBudget;
      using Bucket = decltype(completedInstances)::value_type;
      uint64_t previous = completedInstances.getMemorySize() / sizeof(Bucket);
      uint64_t capacity = std::max(uint64_t(64), previous);
      while (Budget::multiply(completedInstances.size() + 1, 4) >=
             Budget::multiply(capacity, 3)) {
        if (!charge(module)) return failure();
        capacity = Budget::multiply(capacity, 2);
      }
      if (capacity > previous &&
          !charge(module, Budget::add(completedInstances.size(), Budget::bytes(
              Budget::multiply(capacity, sizeof(Bucket))))))
        return failure();
    }
    if (!bindingCost(parent, module) ||
        !append(completedInstances[module], parent, module))
      return failure();
    instanceMarks[module] = 0;
    return success();
  }
  PredicateFact rowFact(DictionaryAttr fact,
                        ArrayRef<PredicateFact> arguments) {
    auto kind = fact.getAs<StringAttr>("kind").getValue();
    if (kind == "constant")
      return fact.getAs<BoolAttr>("value").getValue() ? PredicateFact::True
                                                      : PredicateFact::False;
    if (kind == "argument") {
      uint64_t index = fact.getAs<IntegerAttr>("ordinal").getInt();
      if (index < arguments.size())
        return arguments[index];
    }
    return PredicateFact::Unknown;
  }
  PredicateFact fact(Value value, const Callable &callable,
                     ArrayRef<PredicateFact> arguments, unsigned depth) {
    if (!charge(callable.module))
      return PredicateFact::Unknown;
    if (depth > maxDepth) {
      callable.module->emitOpError()
          << "source Method summary proof depth exceeded";
      exhausted = true;
      return PredicateFact::Unknown;
    }
    HardwareAnalysis hardware(
        callable.module->getParentOfType<mlir::ModuleOp>());
    auto input = detail::getSourcePredicateIdentityInput(value, hardware);
    if (failed(input)) {
      exhausted = true;
      return PredicateFact::Unknown;
    }
    if (*input)
      return fact(*input, callable, arguments, depth + 1);
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      if (argument.getOwner()->getParentOp() != callable.definition)
        return PredicateFact::Unknown;
      unsigned index = argument.getArgNumber(), formals = 0;
      ValueRange captures;
      if (auto method = dyn_cast_or_null<SourceMethodOp>(callable.definition)) {
        formals = method.getFunctionType().getNumInputs();
        captures = method.getCaptures();
      } else if (auto transaction =
                     dyn_cast_or_null<SourceTransactionOp>(callable.definition))
        captures = transaction.getCaptures();
      if (index < formals)
        return index < arguments.size() ? arguments[index]
                                        : PredicateFact::Unknown;
      if (index - formals < captures.size())
        return fact(captures[index - formals],
                    {callable.module, nullptr, {}, {}}, {}, depth + 1);
      return PredicateFact::Unknown;
    }
    auto literal = detail::getSourceLiteralPredicate(value, hardware);
    if (failed(literal)) {
      exhausted = true;
      return PredicateFact::Unknown;
    }
    return *literal ? ((*literal).getValue() ? PredicateFact::True
                                             : PredicateFact::False)
                    : PredicateFact::Unknown;
  }
  Totality prove(const Callable &callable, ArrayRef<PredicateFact> arguments,
                 unsigned depth) {
    if (!charge(callable.module))
      return {};
    if (depth > maxDepth) {
      callable.module->emitOpError()
          << "source Method summary proof depth exceeded";
      exhausted = true;
      return {};
    }
    Operation *guard = nullptr;
    PredicateFact guardFact;
    if (callable.definition) {
      guard = &callable.definition->getRegion(0).front().back();
      guardFact = fact(cast<YieldOp>(guard).getValues().front(), callable,
                       arguments, 0);
    } else
      guardFact = rowFact(
          callable.row.getAs<DictionaryAttr>("guard").getAs<DictionaryAttr>(
              "fact"),
          arguments);
    if (guardFact != PredicateFact::True)
      return {false, guard, nullptr};
    auto &calls = callable.definition ? nativeCalls[callable.definition]
                                      : rowCalls[callable.row];
    size_t callIndex = 0;
    auto callProof = [&](bool reachable) -> Totality {
      if (!charge(callable.module) || callIndex >= calls.size()) {
        exhausted = true;
        return {};
      }
      const Call &call = calls[callIndex++];
      // Closure, signatures and recursion have already checked every call.
      // Only controls whose structural path can execute contribute totality.
      if (!reachable)
        return {true, nullptr, nullptr};
      auto native = dyn_cast_or_null<SourceMethodCallOp>(call.definition);
      PredicateFact path =
          native ? fact(native.getPath(), callable, arguments, 0)
                 : rowFact(call.row.getAs<DictionaryAttr>("path"), arguments);
      if (path == PredicateFact::False)
        return {true, nullptr, nullptr};
      if (call.resolved.method.isNativeTerminal())
        return {false, nullptr, nullptr, nullptr,
                call.definition ? call.definition : callable.module};
      auto symbol = call.resolved.method.getSymbol();
      auto summary = methods.find(symbol);
      if (summary != methods.end() && !summary->second.mayBlock)
        return {true, nullptr, nullptr};
      SmallVector<PredicateFact, 0> actuals;
      uint64_t count = native ? native.getArguments().size()
                              : call.row.getAs<ArrayAttr>("arguments").size();
      if (count > actuals.max_size() ||
          count > (maxWork - work) / (1 + sizeof(PredicateFact))) {
        (void)charge(callable.module, maxWork + 1);
        return {};
      }
      if (!charge(callable.module, count * (1 + sizeof(PredicateFact))))
        return {};
      actuals.reserve(count);
      if (native)
        for (Value value : native.getArguments())
          actuals.push_back(fact(value, callable, arguments, 0));
      else
        for (Attribute raw : call.row.getAs<ArrayAttr>("arguments"))
          actuals.push_back(
              rowFact(cast<DictionaryAttr>(raw).getAs<DictionaryAttr>("fact"),
                      arguments));
      auto target = nodes.find(symbol);
      if (target == nodes.end()) {
        callable.module->emitOpError()
            << "Method totality requires a closed callee graph";
        exhausted = true;
        return {};
      }
      auto proof = prove(target->second, actuals, depth + 1);
      if (!proof.total && !proof.call)
        proof.call = call.definition ? call.definition : callable.module;
      return proof;
    };
    if (callable.definition) {
      auto visit = [&](auto &&visit, Region &region, bool reachable,
                       unsigned nesting) -> Totality {
        if (!charge(callable.module))
          return {};
        if (nesting > maxDepth) {
          callable.module->emitOpError()
              << "source Method summary proof depth exceeded";
          exhausted = true;
          return {};
        }
        for (Operation &operation : region.front().without_terminator()) {
          if (!charge(&operation))
            return {};
          if (isa<SourceMethodCallOp>(operation)) {
            auto proof = callProof(reachable);
            if (!proof.total)
              return proof;
          } else if (auto branch = dyn_cast<SourceBranchOp>(operation)) {
            if (!contains<Operation *>(nativeEffectfulBranches, branch,
                                       callable.module)) {
              if (exhausted)
                return {};
              continue;
            }
            auto selected =
                reachable ? fact(branch.getCondition(), callable, arguments, 0)
                          : PredicateFact::Unknown;
            if (reachable && selected == PredicateFact::Unknown)
              return {false, nullptr, nullptr, branch};
            for (auto [arm, whenTrue] :
                 {std::make_pair(&branch.getThenRegion(), true),
                  std::make_pair(&branch.getElseRegion(), false)}) {
              auto proof = visit(
                  visit, *arm,
                  reachable && (selected == PredicateFact::True) == whenTrue,
                  nesting + 1);
              if (!proof.total)
                return proof;
            }
          }
        }
        return {true, nullptr, nullptr};
      };
      return visit(visit, callable.definition->getRegion(1), true, 0);
    }
    auto visit = [&](auto &&visit, ArrayAttr rows, bool reachable,
                     unsigned nesting) -> Totality {
      if (!charge(callable.module))
        return {};
      if (nesting > maxDepth) {
        callable.module->emitOpError()
            << "source Method summary proof depth exceeded";
        exhausted = true;
        return {};
      }
      for (Attribute raw : rows) {
        if (!charge(callable.module))
          return {};
        auto row = cast<DictionaryAttr>(raw);
        auto kind = row.getAs<StringAttr>("kind").getValue();
        if (kind == "call") {
          auto proof = callProof(reachable);
          if (!proof.total)
            return proof;
        } else if (kind == "branch") {
          if (!contains<Attribute>(rowEffectfulBranches, row,
                                   callable.module)) {
            if (exhausted)
              return {};
            continue;
          }
          auto selected = reachable
                              ? rowFact(row.getAs<DictionaryAttr>("condition")
                                            .getAs<DictionaryAttr>("fact"),
                                        arguments)
                              : PredicateFact::Unknown;
          if (reachable && selected == PredicateFact::Unknown)
            return {false, nullptr, nullptr, callable.module};
          for (auto [arm, whenTrue] :
               {std::make_pair(StringRef("then_effects"), true),
                std::make_pair(StringRef("else_effects"), false)}) {
            auto proof = visit(visit, row.getAs<ArrayAttr>(arm),
                               reachable && (selected == PredicateFact::True) ==
                                                whenTrue,
                               nesting + 1);
            if (!proof.total)
              return proof;
          }
        }
      }
      return {true, nullptr, nullptr};
    };
    return visit(visit, callable.row.getAs<ArrayAttr>("body_effects"), true, 0);
  }
  static constexpr uint64_t maxWork = 1024 * 1024;
  static constexpr unsigned maxDepth = 128;
  llvm::DenseMap<Attribute, SourceMethodSummary> &methods;
  llvm::DenseMap<Operation *, bool> &transactions;
  llvm::DenseMap<Attribute, Callable> nodes;
  llvm::DenseMap<Operation *, SmallVector<Call, 0>> nativeCalls;
  llvm::DenseMap<Attribute, SmallVector<Call, 0>> rowCalls;
  llvm::DenseMap<Operation *, unsigned> instanceMarks;
  llvm::DenseMap<Operation *, SmallVector<HardwareBindings, 0>> completedInstances;
  SmallVector<Operation *, 0> nativeEffectfulBranches;
  SmallVector<Attribute, 0> rowEffectfulBranches;
  SmallVector<Callable> declarations, registrations;
  uint64_t work = 0;
  bool exhausted = false;
};
} // namespace
SourceMethodSummariesAnalysis::SourceMethodSummariesAnalysis(
    Operation *operation)
    : package(operation) {
  auto unit = dyn_cast_or_null<mlir::ModuleOp>(operation);
  if (!unit) {
    if (operation)
      operation->emitOpError() << "source Method summaries require a package";
    return;
  }
  if (failed(mlir::verify(unit)))
    return;
  SmallVector<Operation *> declarations;
  for (Operation &op : unit.getBody()->getOperations())
    if (isa<ModuleOp, ModuleImportOp>(op))
      declarations.push_back(&op);
  SymbolTableCollection symbols;
  auto lookup = [&](FlatSymbolRefAttr symbol) -> Operation * {
    return symbols.lookupSymbolIn(unit, symbol);
  };
  valid = succeeded(
      SummaryBuilder(methods, transactions).run(declarations, lookup, unit));
}
SourceMethodSummariesAnalysis::SourceMethodSummariesAnalysis(
    ArrayRef<Operation *> modules, SourceModuleLookup lookup) {
  valid = succeeded(SummaryBuilder(methods, transactions).run(modules, lookup));
}
const SourceMethodSummary *
SourceMethodSummariesAnalysis::lookup(SourceMethodOp method) const {
  if (!method)
    return nullptr;
  auto module = method->getParentOfType<ModuleOp>();
  return lookup(SourceMethodView{module, method, {}}.getSymbol());
}
const SourceMethodSummary *
SourceMethodSummariesAnalysis::lookup(SymbolRefAttr symbol) const {
  auto found = methods.find(symbol);
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
  if (!valid || definition->getParentOp() != package)
    definition.emitOpError()
        << "publication summary requires a valid current package analysis";
  else {
    auto derived = detail::deriveSourceTransactionSummary(definition);
    if (succeeded(derived))
      summary = *derived;
  }
  publicationSummaries[definition] = summary;
  return summary ? FailureOr<DictionaryAttr>(summary)
                 : FailureOr<DictionaryAttr>(failure());
}
FailureOr<bool> SourceMethodSummariesAnalysis::requiresTransactionPublication(
    ModuleOp definition) {
  if (!valid || !definition || definition->getParentOp() != package) {
    if (definition)
      definition.emitOpError() << "transaction publication classification "
                                  "requires a valid current package analysis";
    return failure();
  }
  HardwareAnalysis hardware(cast<mlir::ModuleOp>(package));
  llvm::DenseMap<Operation *, unsigned> marks;
  llvm::DenseMap<Operation *, bool> required;
  uint64_t work = 0;
  std::function<FailureOr<bool>(Operation *, unsigned)> visit =
      [&](Operation *module, unsigned depth) -> FailureOr<bool> {
    if (++work > 1024 * 1024 || depth > 128)
      return definition.emitOpError()
             << "transaction publication instance closure budget exceeded";
    if (marks.lookup(module) == 1)
      return definition.emitOpError()
             << "recursive instance graph cannot be published";
    if (marks.lookup(module) == 2)
      return required.lookup(module);
    marks[module] = 1;
    bool result = module->hasAttr("ac.reference_parameters");
    if (auto imported = dyn_cast<ModuleImportOp>(module))
      result = bool(imported.getTransactionSummaryAttr());
    else if (auto body = dyn_cast<ModuleOp>(module)) {
      for (Operation &op : body.getBody().front()) {
        if (++work > 1024 * 1024)
          return definition.emitOpError()
                 << "transaction publication instance closure budget exceeded";
        if (isa<SourceStateOp, SourceMethodOp, SourceTransactionOp>(op) ||
            (isa<InstanceOp>(op) &&
             (op.hasAttr("ac.source_unbound_push") ||
              op.hasAttr("ac.source_unbound_pop") ||
              op.hasAttr("ac.payload_constraint") ||
              op.hasAttr("ac.reference_bindings"))))
          result = true;
        if (isa<InstanceOp, CollectionOp>(op)) {
          auto symbol = op.getAttrOfType<FlatSymbolRefAttr>("callee");
          Operation *target =
              symbol ? hardware.lookupDefinition(symbol.getValue()) : nullptr;
          if (!target)
            return op.emitOpError() << "transaction publication requires a "
                                       "resolved instance callee";
          auto child = visit(target, depth + 1);
          if (failed(child))
            return failure();
          result |= *child;
        }
      }
    } else
      return definition.emitOpError()
             << "transaction publication requires module instance closure";
    marks[module] = 2;
    required[module] = result;
    return result;
  };
  return visit(definition, 0);
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
