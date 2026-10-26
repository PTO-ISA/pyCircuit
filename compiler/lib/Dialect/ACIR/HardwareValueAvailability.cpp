#include "HardwareValueAvailability.h"
#include "HardwareQueueResources.h"
#include "HardwareSourceChecks.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallString.h"
#include <map>
#include <set>

using namespace mlir;
namespace acir::ac::detail {
bool hasUniformTableAvailability(Value value, const HardwareAnalysis &analysis) {
  for (unsigned depth = 0; depth < 128; ++depth) {
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      auto *owner = argument.getOwner()->getParentOp();
      if (isa<ModuleOp>(owner)) return true;
      if (auto rule = dyn_cast<RuleOp>(owner)) {
        if (argument.getArgNumber() >= rule.getCaptures().size()) return false;
        value = rule.getCaptures()[argument.getArgNumber()];
        continue;
      }
      return false;
    }
    auto *operation = value.getDefiningOp();
    if (isa_and_nonnull<SourceStateOp, TableSplatOp, InstanceOp>(operation)) return true;
    if (auto collection = dyn_cast_or_null<CollectionOp>(operation))
      return !analysis.getPrimitiveKind(analysis.resolveCallee(collection)).empty();
    if (auto rule = dyn_cast_or_null<RuleOp>(operation)) {
      if (rule->hasAttr("ac.sampled_admission")) return true;
      if (!rule.getBody().hasOneBlock() || rule.getBody().front().empty()) return false;
      auto yield = dyn_cast<YieldOp>(rule.getBody().front().back());
      auto ordinal = cast<OpResult>(value).getResultNumber();
      if (!yield || ordinal >= yield.getNumOperands()) return false;
      value = yield.getOperand(ordinal);
      continue;
    }
    return false;
  }
  return false;
}
namespace {
// Availability proof preserves three statuses: known zero, known one, and
// unknown (X/Z). These identities do not assert raw four-state SSA equality or
// rewrite hardware predicates. Neither a | ~a nor an unknown condition becomes
// a known Boolean.
class AvailabilityProof {
  const HardwareAnalysis &analysis;
  std::function<LogicalResult(uint64_t, Operation *)> debit;
  bool charge(uint64_t units, Operation *site) {
    if (!debit || succeeded(debit(units, site))) return true;
    invalid = true;
    return false;
  }
  using Expr = unsigned;
  struct Node {
    std::string kind;
    SmallVector<Expr> children;
  };
  SmallVector<Node> nodes{{"zero", {}}, {"one", {}}};
  std::map<std::string, Expr> interned;
  DenseMap<Value, Expr> formulaCache;
  DenseSet<Value> formulaActive;
  std::map<std::string, Expr> availabilityCache;
  std::set<std::string> active;
  bool invalid = false;
  struct RowFrame {
    TableMapOp map;
    Expr ordinal;
    DenseMap<Value, Expr> raw, formulas;
    std::map<std::string, Expr> available, rows, rawRows, rowFormulas;
    std::set<std::string> active, activeRows, activeRawRows, activeRowFormulas;
  };
  RowFrame *rowFrame = nullptr;
  unsigned rowDepth = 0;
  std::map<std::string, Expr> rowCache, rawRowCache, formulaRowCache;
  std::set<std::string> rowActive, rawRowActive, formulaRowActive;
  static void appendKeyPart(std::string &key, StringRef part) {
    key += ":" + std::to_string(part.size()) + ":" + part.str();
  }
  bool inRow(Value value) {
    return rowFrame && value.getParentBlock() == &rowFrame->map.getBody().front();
  }
  std::string valueKey(Value value, ArrayRef<StringAttr> path = {}) {
    Operation *site = value.getParentBlock()->getParentOp();
    if (!charge(32, site)) return "invalid";
    std::string key = std::to_string(reinterpret_cast<uintptr_t>(value.getAsOpaquePointer()));
    for (auto field : path) {
      if (!charge(32 + field.getValue().size(), site)) return "invalid";
      appendKeyPart(key, field.getValue());
    }
    return key;
  }
  std::string attributeKey(Attribute attr, unsigned depth = 0) {
    Operation *site = analysis.getPackage();
    if (!charge(8, site)) return "invalid";
    if (depth > 128) {
      reject(site, "row predicate attribute depth exceeds 128");
      return "invalid";
    }
    if (auto expression = dyn_cast<StaticExprAttr>(attr)) {
      // Bound and prepay the evaluator's complete input before invoking it.
      // The temporary key is private and carries no semantic authority.
      (void)attributeKey(expression.getTree(), depth + 1);
      if (invalid) return "invalid";
      if (!analysis.isStaticEvaluable(expression, {}))
        return "static:" + attributeKey(expression.getTree(), depth + 1);
      auto evaluated = analysis.evaluateStatic(expression, {}, site);
      if (failed(evaluated)) { invalid = true; return "invalid"; }
      return attributeKey(*evaluated, depth + 1);
    }
    if (auto array = dyn_cast<ArrayAttr>(attr)) {
      if (!charge(8 + array.size(), site)) return "invalid";
      std::string key = "array";
      for (Attribute element : array) {
        auto nested = attributeKey(element, depth + 1);
        if (invalid || !charge(32 + nested.size(), site)) return "invalid";
        appendKeyPart(key, nested);
      }
      return key;
    }
    if (auto dictionary = dyn_cast<DictionaryAttr>(attr)) {
      if (!charge(16 + dictionary.size(), site)) return "invalid";
      std::string key = "dictionary";
      for (NamedAttribute element : dictionary) {
        StringRef name = element.getName().strref();
        if (name == "origin" || name == "location") continue;
        if (!charge(32 + name.size(), site)) return "invalid";
        auto nested = attributeKey(element.getValue(), depth + 1);
        if (invalid || !charge(32 + nested.size(), site)) return "invalid";
        appendKeyPart(key, name);
        appendKeyPart(key, nested);
      }
      return key;
    }
    if (auto text = dyn_cast<StringAttr>(attr)) {
      if (!charge(8 + text.getValue().size(), site)) return "invalid";
      return "string:" + text.getValue().str();
    }
    if (auto integer = dyn_cast<MathIntAttr>(attr)) {
      if (!charge(8 + integer.getCanonicalValue().size(), site)) return "invalid";
      return "math:" + integer.getCanonicalValue().str();
    }
    if (auto integer = dyn_cast<IntegerAttr>(attr)) {
      if (!charge(64 + integer.getValue().getBitWidth(), site)) return "invalid";
      llvm::SmallString<32> text;
      integer.getValue().toString(text, 10, false);
      std::string type = isa<IndexType>(integer.getType()) ? "index" :
          std::to_string(unsigned(cast<IntegerType>(integer.getType()).getSignedness()));
      return "integer:" + type + ":" + std::to_string(integer.getValue().getBitWidth()) + ":" + text.str().str();
    }
    if (auto values = dyn_cast<DenseI32ArrayAttr>(attr)) {
      if (!charge(16 + 32 * uint64_t(values.size()), site)) return "invalid";
      std::string key = "i32-array";
      for (int32_t value : values.asArrayRef()) key += ":" + std::to_string(value);
      return key;
    }
    if (isa<UnitAttr>(attr)) return "unit";
    if (auto symbol = dyn_cast<FlatSymbolRefAttr>(attr)) {
      if (!charge(8 + symbol.getValue().size(), site)) return "invalid";
      return "symbol:" + symbol.getValue().str();
    }
    reject(site, "unsupported row predicate semantic attribute");
    return "invalid";
  }
  std::string typeKey(Type type, Operation *site) {
    if (auto table = dyn_cast<TableType>(type)) {
      std::string key = "table";
      appendKeyPart(key, shapeKey(table, site));
      appendKeyPart(key, typeKey(table.getElementType(), site));
      return key;
    }
    if (auto bits = dyn_cast<BitsType>(type)) {
      auto logical = attributeKey(bits.getWidth());
      if (invalid) return "invalid";
      if (!analysis.isStaticEvaluable(bits.getWidth(), {})) return "bits-template:" + logical;
      auto width = analysis.getPackedWidth(type, {}, site);
      if (failed(width)) { invalid = true; return "invalid"; }
      return "bits:" + std::to_string(*width);
    }
    if (auto structure = dyn_cast<StructType>(type)) return "struct:" + attributeKey(structure.getName());
    if (auto enumeration = dyn_cast<EnumType>(type)) return "enum:" + attributeKey(enumeration.getName());
    reject(site, "unsupported row predicate hardware type");
    return "invalid";
  }
  std::string shapeKey(TableType type, Operation *site) {
    if (!charge(16 + 32 * uint64_t(type.getShape().size()), site)) return "invalid";
    auto logical = attributeKey(type.getShape());
    if (invalid) return "invalid";
    if (!analysis.isStaticEvaluable(type.getShape(), {})) return "template:" + logical;
    auto shape = analysis.getTableShape(type, {}, site);
    if (failed(shape)) { invalid = true; return "invalid"; }
    std::string key;
    for (auto extent : *shape) key += ":" + std::to_string(extent);
    return key;
  }
  Expr rowDomain(TableType type, Operation *site) {
    return node("ordinal" + shapeKey(type, site));
  }
  Expr viewDomain(TableViewOp view, Expr ordinal) {
    std::string key = "view";
    appendKeyPart(key, view.getKind());
    appendKeyPart(key, attributeKey(view.getParameters()));
    appendKeyPart(key, shapeKey(view.getInput().getType(), view));
    appendKeyPart(key, shapeKey(view.getResult().getType(), view));
    return node(key, {ordinal});
  }
  Expr rawExpression(Value value) {
    if (rowDepth >= 128) return reject(value.getParentBlock()->getParentOp(), "row predicate depth exceeds 128");
    ++rowDepth;
    llvm::scope_exit depth([&] { --rowDepth; });
    if (!charge(8, value.getParentBlock()->getParentOp())) return 0;
    value = wire(value);
    if (!inRow(value)) return node("raw:" + valueKey(value) + ":" +
        typeKey(value.getType(), value.getParentBlock()->getParentOp()));
    if (auto found = rowFrame->raw.find(value); found != rowFrame->raw.end()) return found->second;
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      unsigned ordinal = argument.getArgNumber();
      Expr result;
      if (!ordinal) result = node("row-ordinal:" + typeKey(value.getType(), rowFrame->map), {rowFrame->ordinal});
      else if (ordinal <= rowFrame->map.getTables().size())
        result = rowValue(rowFrame->map.getTables()[ordinal - 1], rowFrame->ordinal, false);
      else result = rawExpression(rowFrame->map.getCaptures()[ordinal - 1 - rowFrame->map.getTables().size()]);
      rowFrame->raw.try_emplace(value, result);
      return result;
    }
    auto *op = value.getDefiningOp();
    if (!op || op->getNumRegions()) return reject(op ? op : rowFrame->map.getOperation(), "unsupported nested row predicate");
    if (!charge(32 + op->getNumOperands() + op->getAttrs().size(), op)) return 0;
    SmallVector<Expr> operands;
    operands.reserve(op->getNumOperands());
    for (Value operand : op->getOperands()) operands.push_back(rawExpression(operand));
    std::string key = "row-op";
    appendKeyPart(key, op->getName().getStringRef());
    appendKeyPart(key, std::to_string(cast<OpResult>(value).getResultNumber()));
    appendKeyPart(key, typeKey(value.getType(), op));
    for (NamedAttribute attr : op->getAttrs()) {
      StringRef name = attr.getName().strref();
      if (!charge(32 + name.size(), op)) return 0;
      auto semantic = attributeKey(attr.getValue());
      if (invalid || !charge(16 + semantic.size(), op)) return 0;
      // Attribute payloads may contain arbitrary punctuation, including text
      // that resembles another attribute. Preserve both component boundaries.
      appendKeyPart(key, name);
      appendKeyPart(key, semantic);
    }
    Expr result = node(key, operands);
    rowFrame->raw.try_emplace(value, result);
    return result;
  }
  Expr rowValue(Value table, Expr ordinal, unsigned mode,
                ArrayRef<StringAttr> path = {}) {
    if (rowDepth >= 128) return reject(table.getParentBlock()->getParentOp(), "symbolic row depth exceeds 128");
    ++rowDepth;
    llvm::scope_exit depth([&] { --rowDepth; });
    table = wire(table);
    auto key = valueKey(table, path);
    appendKeyPart(key, std::to_string(ordinal));
    if (!charge(8 + key.size(), table.getParentBlock()->getParentOp())) return 0;
    bool bound = inRow(table);
    auto &cache = bound
        ? (mode == 1 ? rowFrame->rows : mode == 2 ? rowFrame->rowFormulas : rowFrame->rawRows)
        : (mode == 1 ? rowCache : mode == 2 ? formulaRowCache : rawRowCache);
    auto &activeRows = bound
        ? (mode == 1 ? rowFrame->activeRows : mode == 2 ? rowFrame->activeRowFormulas : rowFrame->activeRawRows)
        : (mode == 1 ? rowActive : mode == 2 ? formulaRowActive : rawRowActive);
    if (auto found = cache.find(key); found != cache.end()) return found->second;
    if (!activeRows.insert(key).second)
      return reject(table.getParentBlock()->getParentOp(), "cycle in symbolic row availability");
    llvm::scope_exit cleanup([&] { activeRows.erase(key); });
    Expr result;
    if (auto splat = table.getDefiningOp<TableSplatOp>())
      result = mode == 1 ? availability(splat.getInput(), path) : mode == 2 ? formula(splat.getInput()) : rawExpression(splat.getInput());
    else if (auto create = table.getDefiningOp<TableCreateOp>()) {
      if (!charge(8 + create.getInputs().size(), create)) return 0;
      SmallVector<Expr> elements{ordinal};
      elements.reserve(1 + create.getInputs().size());
      for (Value input : create.getInputs())
        elements.push_back(mode == 1 ? availability(input, path) : mode == 2 ? formula(input) : rawExpression(input));
      if (mode != 0 && llvm::all_of(ArrayRef<Expr>(elements).drop_front(), [](Expr e) { return e == 1; })) result = 1;
      else result = node("row-create:" + shapeKey(create.getResult().getType(), create), elements);
    } else if (auto view = table.getDefiningOp<TableViewOp>())
      result = rowValue(view.getInput(), viewDomain(view, ordinal), mode, path);
    else if (auto map = table.getDefiningOp<TableMapOp>()) {
      if (!charge(16 + map.getBody().front().getNumArguments(), map)) return 0;
      RowFrame frame;
      frame.map = map;
      frame.ordinal = ordinal;
      auto *outer = rowFrame;
      rowFrame = &frame;
      llvm::scope_exit restore([&] { rowFrame = outer; });
      Value yielded = cast<YieldOp>(map.getBody().front().getTerminator())
          .getValues()[cast<OpResult>(table).getResultNumber()];
      result = mode == 1 ? availability(yielded, path) : mode == 2 ? formula(yielded) : rawExpression(yielded);
    } else if (mode == 1) result = availability(table, path);
    else result = node("row-source:" + valueKey(table) + ":" +
        typeKey(cast<TableType>(table.getType()).getElementType(), table.getParentBlock()->getParentOp()), {ordinal});
    cache.emplace(std::move(key), result);
    return result;
  }
  Expr wholeRows(Value table, bool available, ArrayRef<StringAttr> path = {}) {
    auto type = cast<TableType>(table.getType());
    Operation *site = table.getParentBlock()->getParentOp();
    Expr domain = rowDomain(type, site);
    Expr row = rowValue(table, domain, available ? 1 : 2, path);
    if (row == 0 || row == 1) return row;
    // TableCreate already has an explicit operand for each row. Reducing its
    // complete domain visits those operands, without expanding a table extent.
    // A view/subset has a different ordinal expression and cannot use this.
    if (StringRef(nodes[row].kind).starts_with("row-create:") &&
        nodes[row].children.front() == domain)
      return all(ArrayRef<Expr>(nodes[row].children).drop_front());
    // Reuse the existing availability-conjunction normalization for a
    // broadcast over a nonempty shape. This does not rewrite hardware SSA.
    SmallVector<Expr> pending{row};
    DenseSet<Expr> seen;
    bool dependent = false;
    while (!pending.empty()) {
      Expr current = pending.pop_back_val();
      if (!charge(4 + nodes[current].children.size(), site)) return 0;
      if (current == domain) { dependent = true; break; }
      if (!seen.insert(current).second) continue;
      llvm::append_range(pending, nodes[current].children);
    }
    if (!dependent) return row;
    return node("fold-and" + shapeKey(type, site), {row});
  }

  Expr node(StringRef kind, ArrayRef<Expr> children = {}) {
    if (!charge(1 + kind.size() + children.size(), analysis.getPackage())) return 0;
    // Kind text contains semantic strings; it must not absorb child IDs.
    std::string key;
    appendKeyPart(key, kind);
    for (auto child : children)
      key += ":" + std::to_string(child);
    auto [entry, inserted] = interned.emplace(key, nodes.size());
    if (inserted)
      nodes.push_back({kind.str(), SmallVector<Expr>(children)});
    return entry->second;
  }
  Expr all(ArrayRef<Expr> inputs) {
    SmallVector<Expr> terms;
    for (auto input : inputs) {
      if (input == 0)
        return 0;
      if (input == 1)
        continue;
      if (nodes[input].kind == "and")
        llvm::append_range(terms, nodes[input].children);
      else
        terms.push_back(input);
    }
    llvm::sort(terms);
    terms.erase(std::unique(terms.begin(), terms.end()), terms.end());
    if (terms.empty())
      return 1;
    if (terms.size() == 1)
      return terms.front();
    return node("and", terms);
  }
  Expr select(Expr condition, Expr yes, Expr no) {
    if (condition == 1 || yes == no)
      return yes;
    if (condition == 0)
      return no;
    if (yes == 1 && no == 0)
      return condition;
    return node("select", {condition, yes, no});
  }
  Value wire(Value value) {
    DenseSet<Value> seen;
    while (seen.insert(value).second) {
      if (auto arg = dyn_cast<BlockArgument>(value)) {
        if (auto rule = dyn_cast<RuleOp>(arg.getOwner()->getParentOp())) {
          value = rule.getCaptures()[arg.getArgNumber()];
          continue;
        }
      } else if (auto rule = value.getDefiningOp<RuleOp>()) {
        value = cast<YieldOp>(rule.getBody().front().back())
                    .getValues()[cast<OpResult>(value).getResultNumber()];
        continue;
      }
      return value;
    }
    invalid = true;
    return value;
  }
  std::optional<std::string> constant(Value value) {
    auto op = wire(value).getDefiningOp<BitsConstantOp>();
    if (!op)
      return std::nullopt;
    auto tree = op.getValue().getTree();
    auto literal = tree.getAs<DictionaryAttr>("value");
    auto integer =
        literal ? literal.getAs<MathIntAttr>("value") : MathIntAttr();
    if (!integer)
      return std::nullopt;
    return integer.getCanonicalValue().str();
  }
  Expr scalar(Value value) {
    value = wire(value);
    if (auto number = constant(value))
      return node("constant:" + *number);
    return node("wire:" + std::to_string(reinterpret_cast<uintptr_t>(
                              value.getAsOpaquePointer())));
  }
  Expr equal(Expr left, Expr right) {
    if (left > right)
      std::swap(left, right);
    return node("eq", {left, right});
  }
  Expr formula(Value value) {
    if (!charge(1, value.getParentBlock()->getParentOp())) return 0;
    value = wire(value);
    bool bound = inRow(value);
    if (bound && rowDepth >= 128) return reject(value.getParentBlock()->getParentOp(), "row formula depth exceeds 128");
    rowDepth += bound;
    llvm::scope_exit depth([&] { rowDepth -= bound; });
    auto &localFormulas = inRow(value) ? rowFrame->formulas : formulaCache;
    if (auto found = localFormulas.find(value); found != localFormulas.end())
      return found->second;
    if (!formulaActive.insert(value).second) {
      invalid = true;
      return 0;
    }
    llvm::scope_exit cleanup([&] { formulaActive.erase(value); });
    Expr result;
    if (auto number = constant(value);
        number && (*number == "0" || *number == "1"))
      result = *number == "1" ? 1 : 0;
    else if (auto fold = value.getDefiningOp<TableFoldOp>(); fold && fold.getKind() == "and")
      result = wholeRows(fold.getInput(), false);
    else if (inRow(value) && isa<BlockArgument>(value)) {
      auto argument = cast<BlockArgument>(value);
      unsigned ordinal = argument.getArgNumber();
      if (!ordinal) result = rawExpression(value);
      else if (ordinal <= rowFrame->map.getTables().size())
        result = rowValue(rowFrame->map.getTables()[ordinal - 1], rowFrame->ordinal, 2);
      else result = formula(rowFrame->map.getCaptures()[ordinal - 1 - rowFrame->map.getTables().size()]);
    }
    else if (auto op = value.getDefiningOp<BitsBinaryOp>();
             op && op.getOpcode() == "and")
      result = all({formula(op->getOperand(0)), formula(op->getOperand(1))});
    else if (auto op = value.getDefiningOp<BitsSelectOp>())
      result = select(formula(op.getCondition()), formula(op.getTrueValue()),
                      formula(op.getFalseValue()));
    else if (auto op = value.getDefiningOp<ValueRequireOp>())
      result = select(formula(op.getPath()), formula(op.getAvailability()), 1);
    else if (inRow(value) && value.getDefiningOp<BitsCompareOp>())
      result = rawExpression(value);
    else if (auto op = value.getDefiningOp<BitsCompareOp>()) {
      auto left = constant(op->getOperand(0));
      auto right = constant(op->getOperand(1));
      StringRef predicate = op.getPredicate();
      if (left && right &&
          (predicate == "eq" || predicate == "case_eq" || predicate == "ne"))
        result = ((*left == *right) != (predicate == "ne")) ? 1 : 0;
      else if (predicate == "eq")
        result = equal(scalar(op->getOperand(0)), scalar(op->getOperand(1)));
      else
        result = scalar(value);
    } else
      result = inRow(value) ? rawExpression(value) : scalar(value);
    localFormulas.try_emplace(value, result);
    return result;
  }
  Expr reject(Operation *site, StringRef message) {
    site->emitOpError() << message;
    invalid = true;
    return 0;
  }
  Expr tableAt(Value table, uint64_t ordinal, ArrayRef<StringAttr> path) {
    if (auto arg = dyn_cast<BlockArgument>(table))
      if (auto rule = dyn_cast<RuleOp>(arg.getOwner()->getParentOp()))
        return tableAt(rule.getCaptures()[arg.getArgNumber()], ordinal, path);
    if (auto splat = table.getDefiningOp<TableSplatOp>())
      return availability(splat.getInput(), path);
    if (auto view = table.getDefiningOp<TableViewOp>()) {
      auto source = analysis.getViewSourceOrdinal(view, ordinal);
      if (failed(source)) {
        invalid = true;
        return 0;
      }
      return tableAt(view.getInput(), *source, path);
    }
    if (auto create = table.getDefiningOp<TableCreateOp>())
      return ordinal < create.getInputs().size()
                 ? availability(create.getInputs()[ordinal], path)
                 : 1;
    if (auto map = table.getDefiningOp<TableMapOp>()) {
      if (llvm::any_of(map->getOperands(), [&](Value input) { return availability(input) != 1; }))
        return reject(map, "selected read of a partial TableMap requires exact row availability");
      return 1;
    }
    return availability(table, path);
  }
  Expr tableElement(Value table, Value index, ArrayRef<StringAttr> path) {
    auto size = analysis.getTableSize(cast<TableType>(table.getType()), {},
                                      table.getParentRegion()->getParentOp());
    if (failed(size)) {
      invalid = true;
      return 0;
    }
    auto indexWidth = analysis.getPackedWidth(
        index.getType(), {}, index.getParentRegion()->getParentOp());
    if (failed(indexWidth)) {
      invalid = true;
      return 0;
    }
    // Ordinals outside the index carrier cannot be selected. In particular,
    // a Bits1 index into three rows never compares against a truncated two.
    if (*indexWidth < 64)
      *size = std::min(*size, uint64_t(1) << *indexWidth);
    if (auto number = constant(index)) {
      uint64_t ordinal;
      if (!StringRef(*number).getAsInteger(10, ordinal))
        return ordinal < *size ? tableAt(table, ordinal, path) : 1;
    }
    // A known out-of-range index preserves TableGet's existing payload and
    // membership semantics; availability does not introduce a new range guard.
    Expr selected = 1;
    for (uint64_t i = *size; i-- > 0;)
      selected =
          select(equal(scalar(index), node("constant:" + std::to_string(i))),
                 tableAt(table, i, path), selected);
    return selected;
  }
  Expr derive(Value value, ArrayRef<StringAttr> path) {
    if (auto arg = dyn_cast<BlockArgument>(value)) {
      auto *owner = arg.getOwner()->getParentOp();
      if (inRow(arg)) {
        unsigned ordinal = arg.getArgNumber();
        if (!ordinal) return 1;
        if (ordinal <= rowFrame->map.getTables().size())
          return rowValue(rowFrame->map.getTables()[ordinal - 1], rowFrame->ordinal, true, path);
        return availability(rowFrame->map.getCaptures()[ordinal - 1 - rowFrame->map.getTables().size()], path);
      }
      if (auto rule = dyn_cast<RuleOp>(owner))
        return availability(rule.getCaptures()[arg.getArgNumber()], path);
      if (auto module = dyn_cast<ModuleOp>(owner)) {
        auto ports = getModulePortLayout(module);
        if (failed(ports)) {
          invalid = true;
          return 0;
        }
        unsigned ordinal = arg.getArgNumber();
        if (ordinal < ports->inputValidity.size() &&
            ports->inputValidity[ordinal] >= 0)
          return formula(
              arg.getOwner()->getArgument(ports->inputValidity[ordinal]));
        // These internal lanes are authenticated against the actual producer
        // Rule and its entire fire by verifyTransactionReturns. Availability
        // follows the return companion across each hierarchy boundary.
        if (auto returns = module->getAttrOfType<DictionaryAttr>("ac.transaction_returns"))
          if (auto incoming = returns.getAs<ArrayAttr>("incoming"))
            for (Attribute raw : incoming) {
              if (!charge(1, module)) return 0;
              auto row = dyn_cast<DictionaryAttr>(raw);
              auto data = row ? row.getAs<IntegerAttr>("data") : IntegerAttr();
              auto enable = row ? row.getAs<IntegerAttr>("enable") : IntegerAttr();
              if (!data || !enable || data.getInt() < 0 || enable.getInt() < 0 ||
                  uint64_t(enable.getInt()) >= arg.getOwner()->getNumArguments())
                return reject(module, "invalid transaction return availability lane");
              if (uint64_t(data.getInt()) == ordinal)
                return formula(arg.getOwner()->getArgument(enable.getInt()));
            }
        return 1;
      }
      return reject(owner,
                    "availability has unsupported region argument provenance");
    }
    Operation *op = value.getDefiningOp();
    if (inRow(value) && op->getNumRegions())
      return reject(op, "availability has unsupported nested row regions");
    if (isa<BitsConstantOp, BitsConstant4Op, EnumCreateOp, SourceStateOp>(op))
      return 1;
    if (auto rule = dyn_cast<RuleOp>(op)) {
      auto yields = cast<YieldOp>(rule.getBody().front().back()).getValues();
      auto sampled =
          rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
      if (sampled) {
        auto fire = sampled.getAs<IntegerAttr>("fire_result");
        if (!fire || fire.getInt() < 0 ||
            uint64_t(fire.getInt()) >= yields.size())
          return reject(rule,
                        "availability requires a valid sampled fire result");
        uint64_t ordinary = fire.getInt();
        auto module = rule->getParentOfType<ModuleOp>();
        auto wiring =
            module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
        auto writers =
            wiring ? wiring.getAs<ArrayAttr>("writers") : ArrayAttr();
        if (writers)
          for (auto raw : writers) {
            auto writer = dyn_cast<DictionaryAttr>(raw);
            if (writer &&
                writer.get("registration") == rule->getAttr("occurrence")) {
              auto count = writer.getAs<IntegerAttr>("ordinary_results");
              if (!count || count.getInt() < 0)
                return reject(
                    rule, "availability requires valid ordinary result count");
              ordinary = count.getInt();
            }
          }
        // Candidate D/E and check witnesses have private routing
        // proofs. They are not authored payload outputs and must not acquire a
        // producer-fire prerequisite merely by being transported internally.
        if (cast<OpResult>(value).getResultNumber() >= ordinary)
          return 1;
        return formula(yields[fire.getInt()]);
      }
      return availability(yields[cast<OpResult>(value).getResultNumber()],
                          path);
    }
    if (auto instance = dyn_cast<InstanceOp>(op)) {
      Operation *target = analysis.lookupDefinition(instance.getCallee());
      if (!target)
        return reject(instance, "availability has an unresolved instance");
      if (!analysis.getPrimitiveKind(target).empty())
        return 1;
      if (isa<ModuleImportOp>(target) && target->hasAttr("ac.external_contract")) {
        // Availability includes every declared input, independently of payload
        // dependency_summary. Empty-input externs are total.
        SmallVector<Expr> inputs;
        for (Value input : instance.getInputs())
          inputs.push_back(availability(input));
        return all(inputs);
      }
      auto ports = getModulePortLayout(target);
      if (failed(ports)) {
        invalid = true;
        return 0;
      }
      auto ordinal = cast<OpResult>(value).getResultNumber();
      if (ordinal < ports->outputValidity.size() &&
          ports->outputValidity[ordinal] >= 0)
        return formula(instance->getResult(ports->outputValidity[ordinal]));
      for (auto [payload, companion] : ports->routedOutputValidity)
        if (payload == ordinal)
          return formula(instance->getResult(companion));
      return 1;
    }
    if (auto collection = dyn_cast<CollectionOp>(op)) {
      auto target = analysis.lookupDefinition(collection.getCallee());
      if (target && !analysis.getPrimitiveKind(target).empty())
        return 1;
      if (target) {
        auto ports = getModulePortLayout(target);
        if (succeeded(ports) &&
            llvm::all_of(ports->inputValidity,
                         [](int64_t i) { return i < 0; }) &&
            llvm::all_of(ports->outputValidity,
                         [](int64_t i) { return i < 0; }) &&
            llvm::all_of(collection->getOperands(),
                         [&](Value input) { return availability(input) == 1; }))
          return 1;
      }
      return reject(collection, "availability requires scalar instance routing "
                                "for ordinary Collections");
    }
    if (auto get = dyn_cast<StructGetOp>(op)) {
      SmallVector<StringAttr> nested{get.getFieldAttr()};
      llvm::append_range(nested, path);
      return availability(get.getValue(), nested);
    }
    if (auto create = dyn_cast<StructCreateOp>(op)) {
      if (!path.empty()) {
        auto declaration = analysis.lookupStruct(create.getResult().getType());
        for (auto [i, raw] : llvm::enumerate(declaration.getFields()))
          if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name") ==
              path.front())
            return availability(create.getValues()[i], path.drop_front());
        return reject(create,
                      "availability projection names an unknown Struct field");
      }
    }
    if (auto selectOp = dyn_cast<BitsSelectOp>(op))
      return all({availability(selectOp.getCondition()),
                  select(formula(selectOp.getCondition()),
                         availability(selectOp.getTrueValue(), path),
                         availability(selectOp.getFalseValue(), path))});
    if (auto merge = dyn_cast<ValueMergeOp>(op)) {
      if (cast<OpResult>(value).getResultNumber() == 1) {
        SmallVector<Expr> guards;
        for (Value guard : merge.getGuards()) guards.push_back(availability(guard));
        return all(guards);
      }
      auto leaves = analysis.getFieldPaths(merge.getBase().getType(), {}, merge, debit);
      if (failed(leaves)) { invalid = true; return 0; }
      SmallVector<Expr> fields;
      for (const auto &leaf : *leaves) {
        if (path.size() > leaf.size() || !std::equal(path.begin(), path.end(), leaf.begin())) continue;
        Expr available = availability(merge.getBase(), leaf);
        for (auto [i, raw] : llvm::enumerate(merge.getPaths())) {
          auto suffix = cast<ArrayAttr>(raw);
          if (suffix.size() > leaf.size() || !std::equal(suffix.begin(), suffix.end(), leaf.begin())) continue;
          available = all({availability(merge.getGuards()[i]),
              select(formula(merge.getGuards()[i]),
                availability(merge.getValues()[i], ArrayRef<StringAttr>(leaf).drop_front(suffix.size())), available)});
        }
        fields.push_back(available);
      }
      return all(fields);
    }
    if (auto splat = dyn_cast<TableSplatOp>(op)) return availability(splat.getInput(), path);
    if (auto create = dyn_cast<TableCreateOp>(op)) {
      SmallVector<Expr> inputs;
      for (Value input : create.getInputs()) inputs.push_back(availability(input, path));
      return all(inputs);
    }
    if (isa<TableMapOp, TableViewOp>(op)) {
      if (llvm::all_of(op->getOperands(), [&](Value input) { return availability(input) == 1; })) return 1;
      return wholeRows(value, true, path);
    }
    if (auto read = dyn_cast<TableReadOp>(op)) {
      Expr selected = hasUniformTableAvailability(read.getInput(), analysis)
                          ? tableAt(read.getInput(), 0, path)
                          : tableElement(read.getInput(), read.getIndex(), path);
      return all({availability(read.getIndex()), selected});
    }
    if (auto get = dyn_cast<TableGetOp>(op)) {
      Expr index = availability(get.getIndex());
      return cast<OpResult>(value).getResultNumber() == 1
                 ? index
                 : all({index,
                        tableElement(get.getInput(), get.getIndex(), path)});
    }
    if (auto require = dyn_cast<ValueRequireOp>(op))
      return all({availability(require.getAvailability()),
                  availability(require.getPath())});
    if (op->getNumRegions()) {
      bool total = llvm::all_of(op->getOperands(), [&](Value input) {
        return availability(input) == 1;
      });
      if (total && isa<TableMapOp, TableMatchOp>(op))
        return 1;
      return reject(op, "availability requires normalized region provenance");
    }
    SmallVector<Expr> operands;
    for (Value operand : op->getOperands())
      operands.push_back(availability(operand, path));
    return all(operands);
  }
  Expr availability(Value value, ArrayRef<StringAttr> path = {}) {
    if (!charge(1 + path.size(), value.getParentBlock()->getParentOp())) return 0;
    bool bound = inRow(value);
    if (bound && rowDepth >= 128) return reject(value.getParentBlock()->getParentOp(), "row availability depth exceeds 128");
    rowDepth += bound;
    llvm::scope_exit depth([&] { rowDepth -= bound; });
    auto key = valueKey(value, path);
    if (!charge(8 + key.size(), value.getParentBlock()->getParentOp())) return 0;
    auto &localCache = inRow(value) ? rowFrame->available : availabilityCache;
    auto &localActive = inRow(value) ? rowFrame->active : active;
    if (auto found = localCache.find(key); found != localCache.end()) return found->second;
    if (!localActive.insert(key).second)
      return reject(value.getDefiningOp() ? value.getDefiningOp() : value.getParentBlock()->getParentOp(),
                    "cycle in hardware value availability");
    llvm::scope_exit cleanup([&] { localActive.erase(key); });
    Expr result = derive(value, path);
    localCache.emplace(std::move(key), result);
    return result;
  }
  LogicalResult matches(Value payload, Value supplied, Operation *site) {
    if (availability(payload) != (supplied ? formula(supplied) : 1) || invalid)
      return site->emitOpError()
             << "availability differs from actual payload SSA";
    return success();
  }

public:
  explicit AvailabilityProof(
      const HardwareAnalysis &analysis,
      const std::function<LogicalResult(uint64_t, Operation *)> &debit = {})
      : analysis(analysis), debit(debit) {}
  LogicalResult run() {
    std::optional<DenseSet<OpOperand *>> queueUses;
    for (auto module : analysis.getDefinitions()) {
      auto ports = getModulePortLayout(module);
      if (failed(ports))
        return failure();
      auto yield = cast<YieldOp>(module.getBody().front().back());
      auto yielded = yield.getValues();
      DenseMap<unsigned, unsigned> companions;
      DenseSet<unsigned> controls;
      for (auto [payload, companion] : llvm::enumerate(ports->outputValidity))
        if (companion >= 0) {
          companions.try_emplace(payload, companion);
          controls.insert(companion);
        }
      for (auto [payload, companion] : ports->routedOutputValidity) {
        companions.try_emplace(payload, companion);
        controls.insert(companion);
      }
      DenseSet<unsigned> queueSummaries;
      auto queueWiring = module->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
      if (queueWiring && queueWiring.get("pop_summaries")) {
        // Queue summaries are private control outputs, even when their exact
        // subtree union is an authored public input. Authenticate the actual
        // yield edge, not the shared SSA value. This structural collector never
        // calls sampled-admission or availability proof, so there is no proof
        // recursion through Rule fire/private-route validation.
        if (!queueUses) {
          auto verified = getVerifiedQueueResourceUses(analysis);
          if (failed(verified)) return failure();
          queueUses = std::move(*verified);
        }
        auto rows = queueWiring.getAs<ArrayAttr>("pop_summaries");
        if (!rows || rows.empty()) return failure();
        for (Attribute raw : rows) {
          auto row = dyn_cast<DictionaryAttr>(raw);
          auto output = row ? row.getAs<IntegerAttr>("output") : IntegerAttr();
          if (!row || row.size() != 2 || !row.getAs<DictionaryAttr>("queue") ||
              !output || !output.getType().isSignlessInteger(64) || output.getInt() < 0 ||
              uint64_t(output.getInt()) < ports->sourceOutputCount ||
              uint64_t(output.getInt()) >= yielded.size())
            return module->emitOpError() << "private Queue summary requires its authenticated output ordinal";
          unsigned ordinal = output.getInt();
          if (controls.contains(ordinal) || companions.count(ordinal) ||
              !queueSummaries.insert(ordinal).second ||
              !queueUses->contains(&yield->getOpOperand(ordinal)))
            return module->emitOpError() << "private Queue summary cannot replace public or routed payload availability";
        }
      }
      // Private lexical capture exports also require actual yielded-SSA proof.
      // An unmapped payload is permitted only when that proof establishes
      // totality. Only separately authenticated private Queue summary edges
      // bypass this payload check; other uses of the same value remain checked.
      for (unsigned i = 0; i < yielded.size(); ++i) {
        if (controls.contains(i) || queueSummaries.contains(i))
          continue;
        auto companion = companions.find(i);
        Value supplied = companion == companions.end()
                             ? Value()
                             : yielded[companion->second];
        if (failed(matches(yielded[i], supplied, module)))
          return failure();
      }
      auto result = module.walk([&](Operation *operation) {
        if (auto require = dyn_cast<ValueRequireOp>(operation))
          return failed(matches(require.getPayload(), require.getAvailability(),
                                require))
                     ? WalkResult::interrupt()
                     : WalkResult::advance();
        if (auto collection = dyn_cast<CollectionOp>(operation)) {
          auto *target = analysis.resolveCallee(collection);
          if (target && isa<ModuleImportOp>(target) &&
              target->hasAttr("ac.external_contract")) {
            for (Value output : collection.getOutputs()) availability(output);
            // Result-free extern Collections still cannot bypass the current
            // total-input admission limitation.
            for (Value input : collection.getInputs())
              if (availability(input) != 1) {
                collection.emitOpError()
                    << "availability requires scalar instance routing for ordinary Collections";
                return WalkResult::interrupt();
              }
            if (invalid) return WalkResult::interrupt();
          }
        }
        auto instance = dyn_cast<InstanceOp>(operation);
        if (!instance)
          return WalkResult::advance();
        auto *target = analysis.lookupDefinition(instance.getCallee());
        if (!target)
          return WalkResult::interrupt();
        if (!analysis.getPrimitiveKind(target).empty())
          return WalkResult::advance();
        if (isa<ModuleImportOp>(target) && target->hasAttr("ac.external_contract")) {
          // An extern accepts partial inputs. Its outputs derive their own
          // all-input availability, including dead outputs and cycles.
          for (Value output : instance.getOutputs())
            availability(output);
          return invalid ? WalkResult::interrupt() : WalkResult::advance();
        }
        auto layout = getModulePortLayout(target);
        if (failed(layout))
          return WalkResult::interrupt();
        for (unsigned i = 0; i < layout->sourceInputCount; ++i) {
          int64_t companion = layout->inputValidity[i];
          if (failed(matches(instance->getOperand(i),
                             companion < 0 ? Value()
                                           : instance->getOperand(companion),
                             instance)))
            return WalkResult::interrupt();
        }
        return WalkResult::advance();
      });
      if (result.wasInterrupted())
        return failure();
    }
    return failure(invalid);
  }
};
// Called only after verifyTransactionReturns authenticates the complete graph.
static SmallVector<uint64_t> transactionReturnCompanions(Operation *module) {
  SmallVector<uint64_t> result;
  if (auto metadata = module->getAttrOfType<DictionaryAttr>("ac.transaction_returns"))
    for (Attribute raw : metadata.getAs<ArrayAttr>("incoming"))
      result.push_back(cast<DictionaryAttr>(raw).getAs<IntegerAttr>("enable").getInt());
  return result;
}

// Only companion destinations and required availability operands authorize
// transport of a private fire. Shared ordinary-data consumers invalidate it.
class AvailabilityUses {
  const HardwareAnalysis &analysis;
  DenseSet<OpOperand *> roots, backward;
  DenseMap<OpOperand *, bool> closed;
  DenseSet<OpOperand *> active;
  SmallVector<OpOperand *> pending;
  void add(OpOperand &use, bool root = false) {
    if (root)
      roots.insert(&use);
    if (backward.insert(&use).second)
      pending.push_back(&use);
  }
  bool allUsers(Value value) {
    return llvm::all_of(value.getUses(),
                        [&](OpOperand &use) { return permits(use); });
  }
  bool permits(OpOperand &use) {
    if (roots.contains(&use))
      return true;
    if (!backward.contains(&use)) {
      auto *owner = use.getOwner();
      // A retained, dead availability select/AND has no consumer to expose
      // the control to. This does not authorize an ordinary live data route.
      bool transfer = isa<BitsSelectOp>(owner);
      if (auto binary = dyn_cast<BitsBinaryOp>(owner))
        transfer |= binary.getOpcode() == "and";
      return transfer && llvm::all_of(owner->getResults(), [](Value value) {
               return value.use_empty();
             });
    }
    if (auto known = closed.find(&use); known != closed.end())
      return known->second;
    if (!active.insert(&use).second)
      return false;
    llvm::scope_exit cleanup([&] { active.erase(&use); });
    Operation *owner = use.getOwner();
    bool allowed = false;
    if (auto rule = dyn_cast<RuleOp>(owner))
      allowed =
          allUsers(rule.getBody().front().getArgument(use.getOperandNumber()));
    else if (auto yield = dyn_cast<YieldOp>(owner)) {
      if (auto rule = dyn_cast<RuleOp>(yield->getParentOp()))
        allowed = allUsers(rule->getResult(use.getOperandNumber()));
      else if (auto map = dyn_cast<TableMapOp>(yield->getParentOp()))
        allowed = allUsers(map->getResult(use.getOperandNumber()));
    } else if (auto map = dyn_cast<TableMapOp>(owner)) {
      allowed = allUsers(map.getBody().front().getArgument(use.getOperandNumber() + 1));
    } else if (isa<BitsBinaryOp, BitsSelectOp, TableCreateOp, TableSplatOp,
                   TableViewOp>(owner))
      allowed = llvm::all_of(owner->getResults(),
                             [&](Value value) { return allUsers(value); });
    else if (auto fold = dyn_cast<TableFoldOp>(owner))
      allowed = fold.getKind() == "and" && allUsers(fold.getResult());
    closed.try_emplace(&use, allowed);
    return allowed;
  }

public:
  explicit AvailabilityUses(const HardwareAnalysis &analysis)
      : analysis(analysis) {}
  FailureOr<DenseSet<OpOperand *>> collect() {
    for (auto module : analysis.getDefinitions()) {
      auto layout = getModulePortLayout(module);
      if (failed(layout))
        return failure();
      auto yield = cast<YieldOp>(module.getBody().front().back());
      for (int64_t companion : layout->outputValidity)
        if (companion >= 0)
          add(yield->getOpOperand(companion), true);
      for (auto [payload, companion] : layout->routedOutputValidity)
        add(yield->getOpOperand(companion), true);
      auto walked = module.walk([&](Operation *operation) {
        if (auto require = dyn_cast<ValueRequireOp>(operation))
          add(require->getOpOperand(1), true);
        if (auto instance = dyn_cast<InstanceOp>(operation)) {
          auto *callee = analysis.resolveCallee(instance);
          if (!callee)
            return WalkResult::interrupt();
          auto ports = getModulePortLayout(callee);
          if (failed(ports))
            return WalkResult::interrupt();
          for (int64_t companion : ports->inputValidity)
            if (companion >= 0)
              add(instance->getOpOperand(companion), true);
          for (uint64_t companion : transactionReturnCompanions(callee))
            add(instance->getOpOperand(companion), true);
        }
        return WalkResult::advance();
      });
      if (walked.wasInterrupted())
        return failure();
    }
    while (!pending.empty()) {
      Value value = pending.pop_back_val()->get();
      if (auto argument = dyn_cast<BlockArgument>(value)) {
        if (auto rule = dyn_cast<RuleOp>(argument.getOwner()->getParentOp()))
          add(rule->getOpOperand(argument.getArgNumber()));
        else if (auto map = dyn_cast<TableMapOp>(argument.getOwner()->getParentOp())) {
          if (argument.getArgNumber()) add(map->getOpOperand(argument.getArgNumber() - 1));
        }
        continue;
      }
      Operation *producer = value.getDefiningOp();
      if (auto op = dyn_cast<BitsBinaryOp>(producer);
          op && op.getOpcode() == "and") {
        add(op->getOpOperand(0));
        add(op->getOpOperand(1));
      } else if (auto op = dyn_cast<BitsSelectOp>(producer)) {
        // The selector is a raw predicate reference, not an availability route.
        add(op->getOpOperand(1));
        add(op->getOpOperand(2));
      } else if (auto fold = dyn_cast<TableFoldOp>(producer); fold && fold.getKind() == "and") {
        add(fold->getOpOperand(0));
      } else if (auto map = dyn_cast<TableMapOp>(producer)) {
        add(map.getBody().front().getTerminator()->getOpOperand(cast<OpResult>(value).getResultNumber()));
      } else if (isa<TableSplatOp, TableViewOp, TableCreateOp>(producer)) {
        for (OpOperand &operand : producer->getOpOperands()) add(operand);
      } else if (auto rule = dyn_cast<RuleOp>(producer)) {
        auto sampled =
            rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
        auto fire =
            sampled ? sampled.getAs<IntegerAttr>("fire_result") : IntegerAttr();
        unsigned index = cast<OpResult>(value).getResultNumber();
        if (!fire || fire.getInt() != index)
          add(rule.getBody().front().back().getOpOperand(index));
      }
    }
    DenseSet<OpOperand *> result;
    for (OpOperand *use : backward)
      if (permits(*use))
        result.insert(use);
    // A physical companion cannot be repurposed as public payload or a raw
    // branch predicate inside another definition.
    for (auto module : analysis.getDefinitions()) {
      auto layout = getModulePortLayout(module);
      if (failed(layout))
        return failure();
      for (int64_t companion : layout->inputValidity)
        if (companion >= 0 &&
            !allUsers(module.getBody().front().getArgument(companion)))
          return module.emitOpError()
                 << "availability companion has a data use";
      for (uint64_t companion : transactionReturnCompanions(module))
        if (!allUsers(module.getBody().front().getArgument(companion)))
          return module.emitOpError()
                 << "transaction return availability has a data use";
      auto walked = module.walk([&](InstanceOp instance) {
        auto *callee = analysis.resolveCallee(instance);
        if (!callee)
          return WalkResult::interrupt();
        auto ports = getModulePortLayout(callee);
        if (failed(ports))
          return WalkResult::interrupt();
        for (int64_t companion : ports->outputValidity)
          if (companion >= 0 && !allUsers(instance->getResult(companion))) {
            instance.emitOpError() << "availability companion has a data use";
            return WalkResult::interrupt();
          }
        for (auto [payload, companion] : ports->routedOutputValidity)
          if (!allUsers(instance->getResult(companion))) {
            instance.emitOpError()
                << "routed availability companion has a data use";
            return WalkResult::interrupt();
          }
        return WalkResult::advance();
      });
      if (walked.wasInterrupted())
        return failure();
    }
    return result;
  }
};
bool hasAvailabilityTransport(const HardwareAnalysis &analysis) {
  bool found = false;
  analysis.getPackage().walk([&](Operation *operation) {
    auto sampled =
        operation->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    found |= isa<ValueRequireOp>(operation) ||
             operation->hasAttr("ac.external_contract") ||
             operation->hasAttr("ac.input_validity") ||
             operation->hasAttr("ac.output_validity") ||
             operation->hasAttr("ac.routed_output_validity") ||
             operation->hasAttr("ac.transaction_returns") ||
             (sampled && sampled.get("requirements"));
  });
  return found;
}
} // namespace
LogicalResult
verifyHardwareValueAvailability(
    const HardwareAnalysis &analysis,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  // Transport-free packages retain independently checked legacy totality,
  // including literal predicates routed through ordinary instances.
  if (!hasAvailabilityTransport(analysis))
    return success();
  if (failed(verifyTransactionReturns(analysis, debit)))
    return failure();
  if (failed(AvailabilityProof(analysis, debit).run()))
    return failure();
  return success(succeeded(AvailabilityUses(analysis).collect()));
}
FailureOr<DenseSet<OpOperand *>>
getVerifiedValueAvailabilityUses(const HardwareAnalysis &analysis) {
  if (!hasAvailabilityTransport(analysis))
    return DenseSet<OpOperand *>{};
  if (failed(verifyTransactionReturns(analysis, {})))
    return failure();
  if (failed(AvailabilityProof(analysis).run()))
    return failure();
  return AvailabilityUses(analysis).collect();
}
} // namespace acir::ac::detail
