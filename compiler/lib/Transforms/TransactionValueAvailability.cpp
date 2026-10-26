#include "TransactionValueAvailability.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "Dialect/ACIR/HardwareValueAvailability.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include <limits>

using namespace mlir;
namespace acir::detail {
namespace {
// A null bit is statically available. Dynamic bits are ordinary verified
// hardware SSA; no holding state or substitute payload is introduced.
class ValueAvailabilityMaterializer {
  // Canonical handles retain the complete ordered path, including the
  // distinction between nested fields and one field containing punctuation.
  using Key = std::pair<Value, ArrayAttr>;
  using AvailabilityMap = llvm::DenseMap<Key, Value>;
  // Keep inactive entries rather than erase them: hash-table growth remains
  // governed only by entry count, with no hidden tombstone-triggered rehash.
  using ActiveKeys = llvm::DenseMap<Key, bool>;
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
  AvailabilityMap cache;
  ActiveKeys active;
  DenseMap<Value, Value> inputAvailability;
  DenseMap<std::pair<Operation *, Value>, Value> ruleCaptures;
  struct RowFrame {
    ac::TableMapOp source, companion;
    IRMapping raw;
    AvailabilityMap cache, bindings, rows;
    ActiveKeys active, activeRows;
  };
  RowFrame *rowFrame = nullptr;
  unsigned rowDepth = 0;
  AvailabilityMap rowCache;
  ActiveKeys rowActive;
  FailureOr<Key> keyFor(Value value, ArrayRef<StringAttr> path,
                        Operation *site) {
    // Replace pointer/field serialization with typed handle uniquing. Pay path
    // traversal and temporary storage even for an already interned path.
    uint64_t work = ac::detail::SourceCheckBudget::add(
        2, ac::detail::SourceCheckBudget::multiply(path.size(),
            2 + ac::detail::SourceCheckBudget::bytes(sizeof(Attribute))));
    if (failed(budget.debit(work, site))) return failure();
    if (path.empty()) return Key{value, {}};
    SmallVector<Attribute> fields(path.begin(), path.end());
    return Key{value, ArrayAttr::get(value.getContext(), fields)};
  }
  LogicalResult keyLookup(Operation *site) {
    return budget.debit(2, site);
  }
  template <typename Map>
  LogicalResult reserveKey(Operation *site, Map &storage) {
    // Match the pinned DenseMap reserve policy: at least 64 power-of-two
    // buckets, with load strictly below 3/4. No cache or active map erases keys.
    using Bucket = typename Map::value_type;
    uint64_t count = ac::detail::SourceCheckBudget::add(storage.size(), 1);
    uint64_t oldCapacity = storage.getMemorySize() / sizeof(Bucket);
    uint64_t capacity = std::max(uint64_t(64), oldCapacity);
    while (ac::detail::SourceCheckBudget::multiply(count, 4) >=
           ac::detail::SourceCheckBudget::multiply(capacity, 3)) {
      if (failed(budget.debit(1, site))) return failure();
      capacity = ac::detail::SourceCheckBudget::multiply(capacity, 2);
    }
    if (count > std::numeric_limits<unsigned>::max() ||
        failed(budget.debit(2, site))) return failure();
    if (capacity > oldCapacity) {
      uint64_t growth = ac::detail::SourceCheckBudget::add(oldCapacity,
          ac::detail::SourceCheckBudget::bytes(
              ac::detail::SourceCheckBudget::multiply(capacity - oldCapacity,
                                                       sizeof(Bucket))));
      if (failed(budget.debit(growth, site))) return failure();
    }
    storage.reserve(count);
    return success();
  }
  FailureOr<bool> enterKey(ActiveKeys &storage, const Key &key,
                           Operation *site) {
    // Prepay this lookup and the cleanup lookup before making the key active.
    if (failed(budget.debit(4, site))) return failure();
    auto found = storage.find(key);
    if (found != storage.end()) {
      if (found->second) return false;
      found->second = true;
    } else {
      if (failed(reserveKey(site, storage))) return failure();
      storage.try_emplace(key, true);
    }
    return true;
  }
  bool inRow(Value value) {
    return rowFrame && value.getParentBlock() == &rowFrame->source.getBody().front();
  }
  FailureOr<Value> raw(Value value, Operation *site) {
    if (rowDepth >= 128)
      return (void)error(site, "row predicate depth exceeds 128"), failure();
    ++rowDepth;
    llvm::scope_exit depth([&] { --rowDepth; });
    if (failed(budget.debit(8, site))) return failure();
    if (!inRow(value)) return value;
    if (auto mapped = rowFrame->raw.lookupOrNull(value)) return mapped;
    auto *op = value.getDefiningOp();
    if (!op || op->getNumRegions() ||
        !isa<ac::BitsConstantOp, ac::BitsConstant4Op, ac::BitsUnaryOp,
             ac::BitsBinaryOp, ac::BitsCompareOp, ac::BitsSelectOp,
             ac::BitsConcatOp, ac::BitsExtractOp, ac::BitsResizeOp,
             ac::StructGetOp, ac::StructCreateOp, ac::ValueMergeOp,
             ac::EnumCreateOp>(op))
      return (void)error(site, "unsupported nested row predicate"), failure();
    if (failed(budget.debit(8 + op->getNumOperands(), site))) return failure();
    SmallVector<Value> operands;
    operands.reserve(op->getNumOperands());
    for (Value operand : op->getOperands()) {
      auto mapped = raw(operand, site);
      if (failed(mapped)) return failure();
      operands.push_back(*mapped);
    }
    auto *copy = create(site, op->getName().getStringRef(), operands,
                        op->getResultTypes(), op->getAttrs());
    if (!copy || failed(budget.debit(4 + 2 * op->getNumResults(), site))) return failure();
    for (auto [before, after] : llvm::zip(op->getResults(), copy->getResults()))
      rowFrame->raw.map(before, after);
    return rowFrame->raw.lookup(value);
  }
  FailureOr<Value> rowBinding(BlockArgument argument, ArrayRef<StringAttr> path,
                              Operation *site) {
    auto paidKey = keyFor(argument, path, site);
    if (failed(paidKey)) return failure();
    auto key = std::move(*paidKey);
    if (failed(keyLookup(site))) return failure();
    if (auto found = rowFrame->bindings.find(key); found != rowFrame->bindings.end())
      return found->second;
    unsigned ordinal = argument.getArgNumber();
    if (!ordinal) return Value();
    auto source = rowFrame->source;
    auto companion = rowFrame->companion;
    bool table = ordinal <= source.getTables().size();
    auto available = table
        ? rows(source.getTables()[ordinal - 1], path, companion)
        : availability(source.getCaptures()[ordinal - 1 - source.getTables().size()], path, companion);
    if (failed(available)) return failure();
    if (!*available) {
      if (failed(reserveKey(site, rowFrame->bindings))) return failure();
      rowFrame->bindings.try_emplace(std::move(key), Value());
      return Value();
    }
    if (failed(budget.debit(16 + path.size(), site))) return failure();
    unsigned input = table ? companion.getTables().size() : companion->getNumOperands();
    companion->insertOperands(input, *available);
    auto &body = companion.getBody().front();
    Value bound = body.insertArgument(input + 1,
        table ? bit : available->getType(), site->getLoc());
    Builder b(site->getContext());
    companion->setAttr("operandSegmentSizes", b.getDenseI32ArrayAttr({
        int32_t(companion.getTables().size() + table),
        int32_t(companion.getCaptures().size() + !table)}));
    if (failed(reserveKey(site, rowFrame->bindings))) return failure();
    rowFrame->bindings.try_emplace(std::move(key), bound);
    return bound;
  }
  FailureOr<Value> rows(Value value, ArrayRef<StringAttr> path, Operation *site) {
    if (rowDepth >= 128)
      return (void)error(site, "row availability depth exceeds 128"), failure();
    ++rowDepth;
    llvm::scope_exit depth([&] { --rowDepth; });
    auto paidKey = keyFor(value, path, site);
    if (failed(paidKey)) return failure();
    auto key = std::move(*paidKey);
    auto &localRows = inRow(value) ? rowFrame->rows : rowCache;
    auto &localActive = inRow(value) ? rowFrame->activeRows : rowActive;
    if (failed(keyLookup(site))) return failure();
    if (auto found = localRows.find(key); found != localRows.end()) return found->second;
    auto entered = enterKey(localActive, key, site);
    if (failed(entered)) return failure();
    if (!*entered) return (void)error(site, "cyclic row availability"), failure();
    llvm::scope_exit cleanup([&] { localActive.find(key)->second = false; });
    auto type = cast<ac::TableType>(value.getType());
    if (failed(budget.debit(16 + 4 * type.getShape().size(), site))) return failure();
    Type companionType = ac::TableType::get(site->getContext(), type.getShape(), bit);
    Builder b(site->getContext());
    Value result;
    if (auto argument = dyn_cast<BlockArgument>(value);
        argument && isa<ac::RuleOp>(argument.getOwner()->getParentOp())) {
      auto rule = cast<ac::RuleOp>(argument.getOwner()->getParentOp());
      auto module = rule->getParentOfType<ac::ModuleOp>();
      auto actual = rows(rule.getCaptures()[argument.getArgNumber()], path,
                         module.getBody().front().getTerminator());
      if (failed(actual)) return failure();
      auto bound = capture(rule, *actual, argument.getArgNumber(), site);
      if (failed(bound)) return failure();
      result = *bound;
    } else if (auto splat = value.getDefiningOp<ac::TableSplatOp>()) {
      auto input = availability(splat.getInput(), path, site);
      if (failed(input)) return failure();
      if (!*input) input = one(site);
      if (failed(input)) return failure();
      auto *op = create(site, ac::TableSplatOp::getOperationName(), {*input},
          {companionType}, {b.getNamedAttr("shape", type.getShape())});
      if (!op) return failure();
      result = op->getResult(0);
    } else if (auto values = value.getDefiningOp<ac::TableCreateOp>()) {
      SmallVector<Value> inputs;
      for (Value input : values.getInputs()) {
        auto available = availability(input, path, site);
        if (failed(available)) return failure();
        if (!*available) available = one(site);
        if (failed(available) || failed(budget.debit(2, site))) return failure();
        inputs.push_back(*available);
      }
      auto *op = create(site, ac::TableCreateOp::getOperationName(), inputs, {companionType});
      if (!op) return failure();
      result = op->getResult(0);
    } else if (auto view = value.getDefiningOp<ac::TableViewOp>()) {
      auto input = rows(view.getInput(), path, site);
      if (failed(input)) return failure();
      auto *op = create(site, ac::TableViewOp::getOperationName(), {*input},
                        {companionType}, view->getAttrs());
      if (!op) return failure();
      result = op->getResult(0);
    } else if (auto map = value.getDefiningOp<ac::TableMapOp>()) {
      auto *op = create(site, ac::TableMapOp::getOperationName(), map->getOperands(),
          {companionType}, {b.getNamedAttr("shape", map.getShape()),
            b.getNamedAttr("operandSegmentSizes", b.getDenseI32ArrayAttr({
              int32_t(map.getTables().size()), int32_t(map.getCaptures().size())}))}, 1);
      if (!op || failed(budget.debit(16 + 4 * map.getBody().front().getNumArguments(), site)))
        return failure();
      auto *body = new Block;
      op->getRegion(0).push_back(body);
      RowFrame frame;
      frame.source = map;
      frame.companion = cast<ac::TableMapOp>(op);
      for (auto argument : map.getBody().front().getArguments())
        frame.raw.map(argument, body->addArgument(argument.getType(), argument.getLoc()));
      auto *yield = create(op, ac::YieldOp::getOperationName(), {}, {}, {}, 0, body);
      if (!yield) return failure();
      auto *outer = rowFrame;
      rowFrame = &frame;
      llvm::scope_exit restore([&] { rowFrame = outer; });
      auto sourceYield = cast<ac::YieldOp>(map.getBody().front().getTerminator());
      auto available = availability(sourceYield.getValues()[cast<OpResult>(value).getResultNumber()], path, yield);
      if (failed(available)) return failure();
      if (!*available) available = one(yield);
      if (failed(available)) return failure();
      yield->setOperands({*available});
      // Raw rows exist only to clone actual predicates. Do not transport an
      // unrelated private control through an unused source row/capture.
      unsigned tables = frame.companion.getTables().size();
      unsigned captures = frame.companion.getCaptures().size();
      for (unsigned i = body->getNumArguments(); i-- > 1;) {
        if (failed(budget.debit(1, site))) return failure();
        if (!body->getArgument(i).use_empty()) continue;
        op->eraseOperand(i - 1);
        body->eraseArgument(i);
        if (i <= tables) --tables;
        else --captures;
      }
      op->setAttr("operandSegmentSizes", b.getDenseI32ArrayAttr({int32_t(tables), int32_t(captures)}));
      result = op->getResult(0);
    } else {
      auto available = availability(value, path, site);
      if (failed(available)) return failure();
      if (!*available) available = one(site);
      if (failed(available)) return failure();
      auto *op = create(site, ac::TableSplatOp::getOperationName(), {*available},
          {companionType}, {b.getNamedAttr("shape", type.getShape())});
      if (!op) return failure();
      result = op->getResult(0);
    }
    if (failed(reserveKey(site, localRows))) return failure();
    localRows.try_emplace(key, result);
    return result;
  }

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
                    TypeRange types, ArrayRef<NamedAttribute> attrs = {},
                    unsigned regions = 0, Block *block = nullptr) {
    if (failed(budget.debit(16 + operands.size() + attrs.size(), site)))
      return nullptr;
    OpBuilder builder(site);
    if (block) builder.setInsertionPointToEnd(block);
    OperationState state(site->getLoc(), name);
    state.addOperands(operands);
    state.addTypes(types);
    state.addAttributes(attrs);
    for (unsigned i = 0; i < regions; ++i) state.addRegion();
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
    if (auto map = table.getDefiningOp<ac::TableMapOp>()) {
      for (Value input : map->getOperands()) {
        auto available = availability(input, {}, site);
        if (failed(available)) return failure();
        if (*available)
          return (void)error(site, "selected read of a partial TableMap requires exact row availability"), failure();
      }
      return Value();
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
    Value argument = rule.getBody().front().addArgument(actual.getType(), site->getLoc());
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
      if (inRow(argument)) return rowBinding(argument, path, site);
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
    if (inRow(value) && operation->getNumRegions())
      return (void)error(site, "availability has unsupported nested row regions"), failure();
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
      auto predicate = raw(selected.getCondition(), site);
      if (failed(predicate)) return failure();
      auto joined = selection(site, *predicate, *yes, *no);
      if (failed(joined)) return failure();
      return conjunction(site, *condition, *joined);
    }
    if (auto merge = dyn_cast<ac::ValueMergeOp>(operation)) {
      if (cast<OpResult>(value).getResultNumber() == 1)
        return all(site, merge.getGuards());
      auto leaves = hardware.getFieldPaths(merge.getBase().getType(), {}, site,
          [&](uint64_t units, Operation *at) { return budget.debit(units, at); });
      if (failed(leaves)) return failure();
      Value result;
      for (const auto &leaf : *leaves) {
        if (path.size() > leaf.size() || !std::equal(path.begin(), path.end(), leaf.begin())) continue;
        auto available = availability(merge.getBase(), leaf, site);
        if (failed(available)) return failure();
        for (auto [i, rawPath] : llvm::enumerate(merge.getPaths())) {
          auto fields = cast<ArrayAttr>(rawPath);
          if (fields.size() > leaf.size() || !std::equal(fields.begin(), fields.end(), leaf.begin())) continue;
          auto guard = availability(merge.getGuards()[i], {}, site);
          auto replacement = availability(merge.getValues()[i], ArrayRef<StringAttr>(leaf).drop_front(fields.size()), site);
          auto predicate = raw(merge.getGuards()[i], site);
          if (failed(guard) || failed(replacement) || failed(predicate)) return failure();
          auto chosen = selection(site, *predicate, *replacement, *available);
          if (failed(chosen)) return failure();
          available = conjunction(site, *guard, *chosen);
          if (failed(available)) return failure();
        }
        auto joined = conjunction(site, result, *available);
        if (failed(joined)) return failure();
        result = *joined;
      }
      return result;
    }
    if (auto splat = dyn_cast<ac::TableSplatOp>(operation))
      return availability(splat.getInput(), path, site);
    if (auto values = dyn_cast<ac::TableCreateOp>(operation)) {
      Value result;
      for (Value input : values.getInputs()) {
        auto available = availability(input, path, site);
        if (failed(available)) return failure();
        auto joined = conjunction(site, result, *available);
        if (failed(joined)) return failure();
        result = *joined;
      }
      return result;
    }
    if (isa<ac::TableMapOp, ac::TableViewOp>(operation)) {
      bool totalInputs = true;
      for (Value input : operation->getOperands()) {
        auto available = availability(input, {}, site);
        if (failed(available)) return failure();
        totalInputs &= !*available;
      }
      if (totalInputs) return Value();
      auto companion = rows(value, path, site);
      if (failed(companion)) return failure();
      Builder b(site->getContext());
      auto *fold = create(site, ac::TableFoldOp::getOperationName(), {*companion}, {bit},
          {b.getNamedAttr("kind", b.getStringAttr("and"))});
      if (!fold) return failure();
      return fold->getResult(0);
    }
    if (auto read = dyn_cast<ac::TableReadOp>(operation)) {
      auto index = availability(read.getIndex(), {}, site);
      if (failed(index)) return failure();
      auto selected = ac::detail::hasUniformTableAvailability(read.getInput(), hardware)
                          ? tableAt(read.getInput(), 0, path, site)
                          : tableElement(read.getInput(), read.getIndex(), path, site);
      if (failed(selected)) return failure();
      return conjunction(site, *index, *selected);
    }
    if (auto get = dyn_cast<ac::TableGetOp>(operation)) {
      auto index = availability(get.getIndex(), {}, site);
      if (failed(index)) return failure();
      if (cast<OpResult>(value).getResultNumber() == 1) return index;
      auto selected = tableElement(get.getInput(), get.getIndex(), path, site);
      if (failed(selected)) return failure();
      return conjunction(site, *index, *selected);
    }
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
    bool bound = inRow(value);
    if (bound && rowDepth >= 128)
      return (void)error(site, "row expression depth exceeds 128"), failure();
    rowDepth += bound;
    llvm::scope_exit depth([&] { rowDepth -= bound; });
    auto paidKey = keyFor(value, path, site);
    if (failed(paidKey)) return failure();
    auto key = std::move(*paidKey);
    auto &localCache = inRow(value) ? rowFrame->cache : cache;
    auto &localActive = inRow(value) ? rowFrame->active : active;
    if (failed(keyLookup(site))) return failure();
    if (auto found = localCache.find(key); found != localCache.end()) return found->second;
    auto entered = enterKey(localActive, key, site);
    if (failed(entered)) return failure();
    if (!*entered) return (void)error(site, "cyclic required availability"), failure();
    llvm::scope_exit cleanup([&] { localActive.find(key)->second = false; });
    auto result = derive(value, path, site);
    if (succeeded(result)) {
      if (failed(reserveKey(site, localCache))) return failure();
      localCache.try_emplace(key, *result);
    }
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
    // Pin records are verifier locators outside the SSA use lists. Scan them
    // once, using the preserved result ordinal to select the replacement.
    auto remapPinValue = [&](Value &recorded) {
      auto result = dyn_cast_or_null<OpResult>(recorded);
      if (result && result.getOwner() == old)
        recorded = replacement->getResult(result.getResultNumber());
    };
    for (auto &pin : candidate.memoryPins) {
      if (failed(budget.debit(ac::detail::SourceCheckBudget::add(
              4, pin.selections.size()), old))) return failure();
      remapPinValue(pin.active);
      remapPinValue(pin.data);
      remapPinValue(pin.idle);
      for (auto &selection : pin.selections)
        remapPinValue(selection.value);
    }
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

  LogicalResult removeDeadAvailability() {
    // Queries and their caches are finished. Prune only our generated pure
    // operations; source clones and capture/port metadata remain authoritative.
    uint64_t count = candidate.availabilityOperations.size();
    uint64_t storage = ac::detail::SourceCheckBudget::add(
        ac::detail::SourceCheckBudget::multiply(count, 8 * sizeof(Operation *)),
        64 * sizeof(Operation *));
    if (failed(budget.debit(ac::detail::SourceCheckBudget::add(count,
            ac::detail::SourceCheckBudget::bytes(storage)),
            prepared.original)))
      return failure();
    llvm::DenseSet<Operation *> live;
    SmallVector<Operation *> pending;
    for (Operation *operation : candidate.availabilityOperations) {
      live.insert(operation);
      pending.push_back(operation);
    }
    while (!pending.empty()) {
      Operation *operation = pending.pop_back_val();
      if (failed(budget.debit(4, prepared.original))) return failure();
      // A dead region owner may already have erased this queued descendant.
      if (!live.contains(operation) || !isOpTriviallyDead(operation)) continue;
      SmallVector<Operation *> erased;
      auto walked = operation->walk([&](Operation *nested) {
        if (failed(budget.debit(8 +
                ac::detail::SourceCheckBudget::bytes(sizeof(Operation *)),
                prepared.original)))
          return WalkResult::interrupt();
        erased.push_back(nested);
        for (Value operand : nested->getOperands()) {
          if (failed(budget.debit(4 +
                  ac::detail::SourceCheckBudget::bytes(sizeof(Operation *)),
                  prepared.original)))
            return WalkResult::interrupt();
          Operation *producer = operand.getDefiningOp();
          if (producer && live.contains(producer)) pending.push_back(producer);
        }
        return WalkResult::advance();
      });
      if (walked.wasInterrupted()) return failure();
      for (Operation *nested : erased) live.erase(nested);
      operation->erase();
    }
    llvm::erase_if(candidate.availabilityOperations,
                   [&](Operation *operation) { return !live.contains(operation); });
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
    return removeDeadAvailability();
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
