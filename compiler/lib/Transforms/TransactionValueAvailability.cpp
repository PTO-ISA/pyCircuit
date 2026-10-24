#include "TransactionValueAvailability.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/ScopeExit.h"
#include <map>
#include <set>

using namespace mlir;
namespace acir::detail {
namespace {
// A null bit is statically available. Dynamic bits are ordinary verified
// hardware SSA; no holding state or substitute payload is introduced.
class ValueAvailabilityMaterializer {
  struct Ports {
    uint64_t inputs = 0, outputs = 0, routedEnd = 0;
    SmallVector<int64_t> input, output;
    SmallVector<std::pair<int64_t, int64_t>> routed;
  };
  const PreparedTransactionLowering &prepared;
  MaterializedTransactionLowering &candidate;
  ac::detail::SourceCheckBudget &budget;
  ac::HardwareAnalysis hardware;
  Type bit;
  DictionaryAttr origin;
  DenseMap<Operation *, Ports> ports;
  DenseMap<Operation *, unsigned> colors;
  DenseMap<Operation *, MaterializedTransaction *> records;
  std::map<std::string, Value> cache;
  std::set<std::string> active;
  DenseMap<Value, Value> inputAvailability;
  DenseMap<std::pair<Operation *, Value>, Value> ruleCaptures;

  LogicalResult error(Operation *site, StringRef message) {
    return site->emitOpError() << "value availability: " << message;
  }
  StringAttr portName(ac::ModuleOp module, ArrayRef<Attribute> local,
                      StringRef prefix, uint64_t ordinal) {
    std::string name = prefix.str() + std::to_string(ordinal);
    auto used = [&](StringRef candidate) {
      auto contains = [&](ArrayRef<Attribute> names) {
        return llvm::any_of(names, [&](Attribute value) {
          return cast<StringAttr>(value).getValue() == candidate;
        });
      };
      return contains(local) || contains(module.getInputNames().getValue()) ||
             contains(module.getOutputNames().getValue());
    };
    while (used(name)) name += "_";
    return StringAttr::get(module.getContext(), name);
  }
  Operation *create(Operation *site, StringRef name, ValueRange operands,
                    TypeRange types, ArrayRef<NamedAttribute> attrs = {}) {
    if (failed(budget.debit(16 + operands.size() + attrs.size(), site)))
      return nullptr;
    OpBuilder builder(site);
    OperationState state(site->getLoc(), name);
    state.addOperands(operands);
    state.addTypes(types);
    state.addAttributes(attrs);
    Operation *operation = builder.create(state);
    candidate.availabilityOperations.push_back(operation);
    return operation;
  }
  FailureOr<Value> integer(Operation *site, Type type, uint64_t number) {
    Builder b(site->getContext());
    auto width = cast<ac::BitsType>(type).getWidth().getTree();
    auto expression = b.getDictionaryAttr({
        b.getNamedAttr("kind", b.getStringAttr("literal")),
        b.getNamedAttr("value", b.getDictionaryAttr({
            b.getNamedAttr("kind", b.getStringAttr("integer")),
            b.getNamedAttr("value", ac::MathIntAttr::get(site->getContext(),
                llvm::APSInt(llvm::APInt(64, number), true)))})),
        b.getNamedAttr("origin", origin),
        b.getNamedAttr("location", width.get("location"))});
    auto *operation = create(site, ac::BitsConstantOp::getOperationName(), {},
        {type}, {b.getNamedAttr("value", ac::StaticExprAttr::get(site->getContext(), expression))});
    if (!operation) return failure();
    return operation->getResult(0);
  }
  FailureOr<Value> one(Operation *site) { return integer(site, bit, 1); }
  FailureOr<Value> conjunction(Operation *site, Value left, Value right) {
    if (!left) return right;
    if (!right || left == right) return left;
    Builder b(site->getContext());
    auto *operation = create(site, ac::BitsBinaryOp::getOperationName(),
        {left, right}, {bit}, {b.getNamedAttr("opcode", b.getStringAttr("and"))});
    if (!operation) return failure();
    return operation->getResult(0);
  }
  FailureOr<Value> selection(Operation *site, Value condition, Value yes, Value no) {
    if (yes == no) return yes;
    if (!yes) { auto value = one(site); if (failed(value)) return failure(); yes = *value; }
    if (!no) { auto value = one(site); if (failed(value)) return failure(); no = *value; }
    auto *operation = create(site, ac::BitsSelectOp::getOperationName(),
                             {condition, yes, no}, {bit});
    if (!operation) return failure();
    return operation->getResult(0);
  }
  FailureOr<Value> all(Operation *site, ValueRange inputs) {
    Value result;
    for (Value input : inputs) {
      auto next = availability(input, {}, site);
      if (failed(next)) return failure();
      auto joined = conjunction(site, result, *next);
      if (failed(joined)) return failure();
      result = *joined;
    }
    return result;
  }
  FailureOr<Value> tableAt(Value table, uint64_t ordinal,
                           ArrayRef<StringAttr> path, Operation *site) {
    if (auto argument = dyn_cast<BlockArgument>(table))
      if (auto rule = dyn_cast<ac::RuleOp>(argument.getOwner()->getParentOp())) {
        // Project before crossing a capture, so a locally reconstructed Table
        // does not acquire requirements from an unselected element.
        Value actual = rule.getCaptures()[argument.getArgNumber()];
        auto module = rule->getParentOfType<ac::ModuleOp>();
        auto result = tableAt(actual, ordinal, path,
                             module.getBody().front().getTerminator());
        if (failed(result) || !*result) return result;
        return capture(rule, *result, argument.getArgNumber(), site);
      }
    if (auto splat = table.getDefiningOp<ac::TableSplatOp>())
      return availability(splat.getInput(), path, site);
    if (auto view = table.getDefiningOp<ac::TableViewOp>()) {
      auto source = hardware.getViewSourceOrdinal(view, ordinal);
      if (failed(source)) return failure();
      return tableAt(view.getInput(), *source, path, site);
    }
    if (auto values = table.getDefiningOp<ac::TableCreateOp>()) {
      if (ordinal >= values.getInputs().size()) return Value();
      return availability(values.getInputs()[ordinal], path, site);
    }
    return availability(table, path, site);
  }
  FailureOr<Value> tableElement(Value table, Value index,
                                ArrayRef<StringAttr> path, Operation *site) {
    auto size = hardware.getTableSize(cast<ac::TableType>(table.getType()), {}, site);
    if (failed(size)) return failure();
    if (auto constant = index.getDefiningOp<ac::BitsConstantOp>()) {
      auto value = hardware.evaluateStatic(constant.getValue(), {}, constant);
      if (failed(value)) return failure();
      if (auto integer = dyn_cast<ac::MathIntAttr>(*value)) {
        uint64_t ordinal;
        if (!integer.getCanonicalValue().getAsInteger(10, ordinal))
          return ordinal < *size ? tableAt(table, ordinal, path, site)
                                 : FailureOr<Value>(Value());
      }
    }
    Value selected;
    Builder b(site->getContext());
    auto width = hardware.getPackedWidth(index.getType(), {}, site);
    if (failed(width)) return failure();
    uint64_t reachable = *width < 64 ? std::min(*size, uint64_t(1) << *width) : *size;
    for (uint64_t ordinal = reachable; ordinal-- > 0;) {
      auto element = tableAt(table, ordinal, path, site);
      if (failed(element)) return failure();
      if (*element == selected) continue;
      auto literal = integer(site, index.getType(), ordinal);
      if (failed(literal)) return failure();
      auto *equal = create(site, ac::BitsCompareOp::getOperationName(),
          {index, *literal}, {bit}, {b.getNamedAttr("predicate", b.getStringAttr("eq"))});
      if (!equal) return failure();
      auto next = selection(site, equal->getResult(0), *element, selected);
      if (failed(next)) return failure();
      selected = *next;
    }
    return selected;
  }
  FailureOr<Value> capture(ac::RuleOp rule, Value actual, unsigned payload,
                           Operation *site) {
    auto key = std::make_pair(rule.getOperation(), actual);
    auto found = ruleCaptures.find(key);
    if (found != ruleCaptures.end()) return found->second;
    uint64_t ordinal = rule->getNumOperands();
    rule->insertOperands(ordinal, actual);
    Value argument = rule.getBody().front().addArgument(bit, site->getLoc());
    ruleCaptures.try_emplace(key, argument);
    if (auto record = records.lookup(rule)) record->validityCaptures.push_back({payload, ordinal});
    Builder b(rule.getContext());
    SmallVector<Attribute> pairs;
    if (auto prior = rule->getAttrOfType<ArrayAttr>("ac.value_availability_captures"))
      llvm::append_range(pairs, prior);
    pairs.push_back(b.getDictionaryAttr({
        b.getNamedAttr("payload", b.getI64IntegerAttr(payload)),
        b.getNamedAttr("validity", b.getI64IntegerAttr(ordinal))}));
    rule->setAttr("ac.value_availability_captures", b.getArrayAttr(pairs));
    return argument;
  }
  FailureOr<Value> derive(Value value, ArrayRef<StringAttr> path, Operation *site) {
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      if (auto rule = dyn_cast<ac::RuleOp>(argument.getOwner()->getParentOp())) {
        auto module = rule->getParentOfType<ac::ModuleOp>();
        auto result = availability(rule.getCaptures()[argument.getArgNumber()], path,
                                  module.getBody().front().getTerminator());
        if (failed(result) || !*result) return result;
        return capture(rule, *result, argument.getArgNumber(), site);
      }
      if (auto module = dyn_cast<ac::ModuleOp>(argument.getOwner()->getParentOp())) {
        if (auto returns = module->getAttrOfType<DictionaryAttr>("ac.transaction_returns"))
          for (auto raw : returns.getAs<ArrayAttr>("incoming")) {
            auto row = cast<DictionaryAttr>(raw);
            if (row.getAs<IntegerAttr>("data").getInt() == argument.getArgNumber())
              return argument.getOwner()->getArgument(row.getAs<IntegerAttr>("enable").getInt());
          }
        return inputAvailability.lookup(argument);
      }
      return (void)error(site, "unexpected block argument"), failure();
    }
    Operation *operation = value.getDefiningOp();
    if (!operation) return failure();
    if (isa<ac::BitsConstantOp, ac::BitsConstant4Op, ac::EnumCreateOp,
            ac::SourceStateOp>(
            operation))
      return Value();
    if (auto rule = dyn_cast<ac::RuleOp>(operation)) {
      auto sampled = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
      auto yields = cast<ac::YieldOp>(rule.getBody().front().back()).getValues();
      if (!sampled) {
        auto result = availability(yields[cast<OpResult>(value).getResultNumber()], path,
                                   rule.getBody().front().getTerminator());
        if (failed(result) || !*result) return result;
        return (void)error(site, "ordinary helper Rule requires an availability result route"), failure();
      }
      uint64_t ordinary = 0;
      auto record = records.lookup(rule);
      if (!record) return failure();
      auto index = record - candidate.transactions.data();
      ordinary = prepared.transactions[index].plan->transaction->getNumResults();
      if (cast<OpResult>(value).getResultNumber() >= ordinary) return Value();
      return rule->getResult(sampled.getAs<IntegerAttr>("fire_result").getInt());
    }
    if (auto instance = dyn_cast<ac::InstanceOp>(operation)) {
      auto target = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(instance));
      if (!target) return Value();
      auto &layout = ports[target];
      auto ordinal = cast<OpResult>(value).getResultNumber();
      if (ordinal < layout.output.size() && layout.output[ordinal] >= 0)
        return instance->getResult(layout.output[ordinal]);
      for (auto [payload, validity] : layout.routed)
        if (payload == ordinal) return instance->getResult(validity);
      return Value();
    }
    if (auto collection = dyn_cast<ac::CollectionOp>(operation)) {
      if (!hardware.getPrimitiveKind(hardware.resolveCallee(collection)).empty()) return Value();
      return (void)error(site, "ordinary Collection requires element availability routing"), failure();
    }
    if (auto get = dyn_cast<ac::StructGetOp>(operation)) {
      SmallVector<StringAttr> projection{get.getFieldAttr()};
      llvm::append_range(projection, path);
      return availability(get->getOperand(0), projection, site);
    }
    if (auto structure = dyn_cast<ac::StructCreateOp>(operation)) {
      if (!path.empty()) {
        auto fields = hardware.lookupStruct(cast<ac::StructType>(value.getType())).getFields();
        for (auto [ordinal, raw] : llvm::enumerate(fields))
          if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name") == path.front())
            return availability(operation->getOperand(ordinal), path.drop_front(), site);
        return (void)error(site, "unresolved Struct projection"), failure();
      }
      return all(site, operation->getOperands());
    }
    if (auto selected = dyn_cast<ac::BitsSelectOp>(operation)) {
      auto condition = availability(selected.getCondition(), {}, site);
      auto yes = availability(selected.getTrueValue(), path, site);
      auto no = availability(selected.getFalseValue(), path, site);
      if (failed(condition) || failed(yes) || failed(no)) return failure();
      auto joined = selection(site, selected.getCondition(), *yes, *no);
      if (failed(joined)) return failure();
      return conjunction(site, *condition, *joined);
    }
    if (auto get = dyn_cast<ac::TableGetOp>(operation)) {
      auto index = availability(get.getIndex(), {}, site);
      if (failed(index)) return failure();
      if (cast<OpResult>(value).getResultNumber() == 1) return index;
      auto selected = tableElement(get.getInput(), get.getIndex(), path, site);
      if (failed(selected)) return failure();
      return conjunction(site, *index, *selected);
    }
    if (auto view = dyn_cast<ac::TableViewOp>(operation))
      return availability(view.getInput(), path, site);
    if (operation->getNumRegions()) {
      auto inputs = all(site, operation->getOperands());
      if (failed(inputs)) return failure();
      if (!*inputs && isa<ac::TableMapOp, ac::TableMatchOp>(operation)) return Value();
      return (void)error(site, "availability requires normalized region provenance"), failure();
    }
    return all(site, operation->getOperands());
  }
  FailureOr<Value> availability(Value value, ArrayRef<StringAttr> path,
                                Operation *site) {
    if (failed(budget.debit(4 + path.size(), site))) return failure();
    std::string key = std::to_string(reinterpret_cast<uintptr_t>(value.getAsOpaquePointer()));
    for (auto field : path) key += "." + field.getValue().str();
    if (auto found = cache.find(key); found != cache.end()) return found->second;
    if (!active.insert(key).second)
      return (void)error(site, "cyclic required availability"), failure();
    llvm::scope_exit cleanup([&] { active.erase(key); });
    auto result = derive(value, path, site);
    if (succeeded(result)) cache.emplace(key, *result);
    return result;
  }

  LogicalResult refresh(ac::ModuleOp module) {
    auto &layout = ports[module];
    Builder b(module.getContext());
    auto oldType = module.getFunctionType();
    SmallVector<Type> inputs(module.getBody().front().getArgumentTypes());
    auto yield = cast<ac::YieldOp>(module.getBody().front().getTerminator());
    // Preserve source/interface-owned type spellings in the existing prefix;
    // the yielded SSA may carry an equivalent statement-local width origin.
    SmallVector<Type> outputs(oldType.getResults());
    while (outputs.size() < yield.getNumOperands())
      outputs.push_back(yield->getOperand(outputs.size()).getType());
    module->setAttr("function_type", TypeAttr::get(FunctionType::get(module.getContext(), inputs, outputs)));
    auto indices = [&](ArrayRef<int64_t> values) {
      SmallVector<Attribute> result;
      for (int64_t value : values) result.push_back(b.getI64IntegerAttr(value));
      return b.getArrayAttr(result);
    };
    bool input = llvm::any_of(layout.input, [](int64_t i) { return i >= 0; });
    bool output = llvm::any_of(layout.output, [](int64_t i) { return i >= 0; });
    if (input) module->setAttr("ac.input_validity", indices(layout.input));
    if (output) module->setAttr("ac.output_validity", indices(layout.output));
    if (!layout.routed.empty()) {
      SmallVector<Attribute> rows;
      for (auto [payload, validity] : layout.routed)
        rows.push_back(b.getDictionaryAttr({
            b.getNamedAttr("payload", b.getI64IntegerAttr(payload)),
            b.getNamedAttr("validity", b.getI64IntegerAttr(validity))}));
      module->setAttr("ac.routed_output_validity", b.getArrayAttr(rows));
    }
    if (inputs.size() > layout.inputs || outputs.size() > layout.outputs) {
      SmallVector<Attribute> defaults;
      for (uint64_t i = layout.inputs; i < inputs.size(); ++i)
        defaults.push_back(b.getI64IntegerAttr(llvm::is_contained(layout.input, i) ? 1 : 0));
      module->setAttr("ac.internal_ports", b.getDictionaryAttr({
          b.getNamedAttr("source_inputs", b.getI64IntegerAttr(layout.inputs)),
          b.getNamedAttr("source_outputs", b.getI64IntegerAttr(layout.outputs)),
          b.getNamedAttr("root_inputs", b.getArrayAttr(defaults))}));
    }
    return success();
  }
  LogicalResult replaceInstance(ac::InstanceOp old, ac::ModuleOp target) {
    const auto &child = ports[target];
    SmallVector<Type> types(old.getResultTypes());
    for (int64_t output : child.output) if (output >= 0) types.push_back(bit);
    types.append(child.routed.size(), bit);
    OperationState state(old.getLoc(), ac::InstanceOp::getOperationName());
    state.addOperands(old.getOperands());
    state.addTypes(types);
    state.addAttributes(old->getAttrs());
    OpBuilder builder(old);
    auto replacement = cast<ac::InstanceOp>(builder.create(state));
    SmallVector<std::pair<Value, Value>> remaps;
    for (auto [source, mapped] : candidate.packageMap.getValueMap())
      for (auto [ordinal, value] : llvm::enumerate(old.getResults()))
        if (mapped == value) remaps.push_back({source, replacement->getResult(ordinal)});
    SmallVector<Operation *> operationRemaps;
    for (auto [source, mapped] : candidate.packageMap.getOperationMap())
      if (mapped == old) operationRemaps.push_back(source);
    for (auto [source, value] : remaps) candidate.packageMap.map(source, value);
    for (auto source : operationRemaps) candidate.packageMap.map(source, replacement.operator->());
    for (auto [ordinal, value] : llvm::enumerate(old.getResults()))
      value.replaceAllUsesWith(replacement->getResult(ordinal));
    old.erase();
    return success();
  }
  LogicalResult process(ac::ModuleOp module) {
    if (colors.lookup(module) == 2) return success();
    if (colors.lookup(module) == 1) return error(module, "recursive Module route");
    colors[module] = 1;
    SmallVector<ac::InstanceOp> instances;
    for (auto instance : module.getBody().front().getOps<ac::InstanceOp>())
      if (auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(instance))) {
        if (failed(process(child))) return failure();
        instances.push_back(instance);
      }
    for (auto instance : instances)
      if (failed(replaceInstance(instance, cast<ac::ModuleOp>(hardware.resolveCallee(instance)))))
        return failure();
    for (auto instance : module.getBody().front().getOps<ac::InstanceOp>()) {
      auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(instance));
      if (!child) continue;
      const auto &layout = ports[child];
      SmallVector<Value> inputs(instance.getOperands());
      for (uint64_t i = 0; i < layout.inputs; ++i) {
        if (layout.input[i] < 0) continue;
        auto value = availability(inputs[i], {}, instance);
        if (failed(value)) return failure();
        if (!*value) { value = one(instance); if (failed(value)) return failure(); }
        inputs.push_back(*value);
      }
      instance->setOperands(inputs);
    }
    for (auto &record : candidate.transactions) {
      if (record.rule->getParentOp() != module) continue;
      for (auto requirement : record.requirements) {
        Value payload = requirement->getOperand(0), path = requirement->getOperand(1);
        auto value = availability(payload, {}, requirement);
        if (failed(value)) return failure();
        if (!*value) { value = one(requirement); if (failed(value)) return failure(); }
        requirement->setOperands({payload, *value, path});
      }
    }
    auto &layout = ports[module];
    auto yield = cast<ac::YieldOp>(module.getBody().front().getTerminator());
    SmallVector<Value> outputs(yield.getValues());
    SmallVector<Attribute> names(module.getOutputNames().getValue());
    Builder b(module.getContext());
    for (uint64_t i = 0; i < layout.outputs; ++i) {
      auto value = availability(outputs[i], {}, yield);
      if (failed(value)) return failure();
      if (!*value) continue;
      layout.output[i] = outputs.size();
      outputs.push_back(*value);
      names.push_back(portName(module, names, "__pyc_valid_out_", i));
    }
    // Pop summaries are private controls, outside the lexical payload export
    // range. They never acquire payload-availability companions.
    for (uint64_t i = layout.outputs; i < layout.routedEnd; ++i) {
      auto value = availability(outputs[i], {}, yield);
      if (failed(value)) return failure();
      if (!*value) continue;
      layout.routed.push_back({i, outputs.size()});
      outputs.push_back(*value);
      names.push_back(portName(module, names, "__pyc_valid_capture_", i));
    }
    yield->setOperands(outputs);
    module->setAttr("output_names", b.getArrayAttr(names));
    if (failed(refresh(module))) return failure();
    colors[module] = 2;
    return success();
  }

public:
  ValueAvailabilityMaterializer(const PreparedTransactionLowering &prepared,
      MaterializedTransactionLowering &candidate, ac::detail::SourceCheckBudget &budget)
      : prepared(prepared), candidate(candidate), budget(budget), hardware(*candidate.module) {
    auto &transaction = prepared.transactions.front();
    auto source = transaction.plan->transaction;
    bit = source.getClk().getType();
    origin = source.getOccurrence();
  }
  LogicalResult run() {
    for (auto &record : candidate.transactions) records[record.rule] = &record;
    for (const auto &definition : prepared.definitions) {
      auto module = cast<ac::ModuleOp>(candidate.packageMap.lookup(definition.module.operator->()));
      if (failed(budget.debit(1 + ac::detail::SourceCheckBudget::bytes(
              sizeof(Operation *) + sizeof(Ports)), module))) return failure();
      ports[module].routedEnd = definition.sourceOutputs + definition.upward.size();
    }
    for (auto module : hardware.getDefinitions()) {
      auto current = ac::getModulePortLayout(module);
      if (failed(current)) return failure();
      auto &layout = ports[module];
      layout.inputs = current->sourceInputCount;
      layout.outputs = current->sourceOutputCount;
      layout.input.assign(layout.inputs, -1);
      layout.output.assign(layout.outputs, -1);
      SmallVector<Attribute> names(module.getInputNames().getValue());
      Builder b(module.getContext());
      auto domain = module->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
      for (uint64_t i = 0; i < layout.inputs; ++i) {
        bool clock = false;
        if (domain)
          for (StringRef pin : {"clock", "reset"})
            if (auto index = domain.getAs<IntegerAttr>(pin); index && index.getInt() == i)
              clock = true;
        if (clock) continue;
        layout.input[i] = module.getBody().front().getNumArguments();
        auto argument = module.getBody().front().addArgument(bit, module.getLoc());
        inputAvailability[module.getBody().front().getArgument(i)] = argument;
        names.push_back(portName(module, names, "__pyc_valid_in_", i));
      }
      module->setAttr("input_names", b.getArrayAttr(names));
      if (failed(refresh(module))) return failure();
    }
    for (auto module : hardware.getDefinitions()) if (failed(process(module))) return failure();
    return success();
  }
};
} // namespace
LogicalResult materializeTransactionValueAvailability(
    const PreparedTransactionLowering &prepared,
    MaterializedTransactionLowering &candidate,
    ac::detail::SourceCheckBudget &budget) {
  if (!prepared.valueAvailability) return success();
  return ValueAvailabilityMaterializer(prepared, candidate, budget).run();
}
} // namespace acir::detail
