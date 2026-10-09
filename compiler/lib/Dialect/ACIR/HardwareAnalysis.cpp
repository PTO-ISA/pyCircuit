#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "SourceValueSemantics.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringSet.h"
#include <algorithm>
#include <limits>
#include <optional>
#include <thread>
using namespace mlir;
namespace acir::ac {
namespace {
Attribute substitute(Attribute attr, const HardwareBindings &bindings) {
  if (auto expr = dyn_cast_or_null<StaticExprAttr>(attr)) {
    auto tree = expr.getTree();
    auto kind = tree.getAs<StringAttr>("kind");
    if (kind && kind.getValue() == "reference") {
      auto ref = tree.getAs<DictionaryAttr>("ref");
      if (ref && bindings.owner && ref.getAs<FlatSymbolRefAttr>("owner") &&
          ref.getAs<FlatSymbolRefAttr>("owner").getValue() ==
              SymbolTable::getSymbolName(bindings.owner).getValue()) {
        auto name = ref.getAs<StringAttr>("name");
        auto found = name ? bindings.integers.find(name.getValue())
                          : bindings.integers.end();
        if (found != bindings.integers.end()) {
          if (isa<StaticExprAttr>(found->second))
            return found->second;
          Builder b(attr.getContext());
          NamedAttrList literal(tree);
          literal.set("kind", b.getStringAttr("literal"));
          literal.erase("ref");
          literal.set("value",
                      b.getDictionaryAttr(
                          {b.getNamedAttr("kind", b.getStringAttr("integer")),
                           b.getNamedAttr("value", found->second)}));
          return StaticExprAttr::get(attr.getContext(),
                                     literal.getDictionary(attr.getContext()));
        }
      }
    }
    return StaticExprAttr::get(
        attr.getContext(), cast<DictionaryAttr>(substitute(tree, bindings)));
  }
  if (auto dict = dyn_cast_or_null<DictionaryAttr>(attr)) {
    NamedAttrList fields;
    for (NamedAttribute field : dict)
      fields.append(field.getName(), substitute(field.getValue(), bindings));
    return fields.getDictionary(attr.getContext());
  }
  if (auto array = dyn_cast_or_null<ArrayAttr>(attr)) {
    SmallVector<Attribute> values;
    for (Attribute value : array)
      values.push_back(substitute(value, bindings));
    return ArrayAttr::get(attr.getContext(), values);
  }
  return attr;
}
bool evaluable(Attribute attr, const HardwareBindings &bindings,
               SmallVector<Attribute> &active,
               const HardwareAnalysis *analysis);
bool evaluableWidth(Type type, const HardwareBindings &bindings,
                    SmallVector<Attribute> &active,
                    const HardwareAnalysis *analysis) {
  Attribute key = TypeAttr::get(type);
  if (llvm::is_contained(active, key))
    return true; // Closed binding cycle: let the owning evaluator diagnose it.
  active.push_back(key);
  llvm::scope_exit cleanup([&] { active.pop_back(); });
  if (auto formal = dyn_cast<TypeParamType>(type)) {
    if (!bindings.owner || formal.getOwner().getValue() !=
                               SymbolTable::getSymbolName(bindings.owner).getValue())
      return false;
    auto found = bindings.types.find(formal.getName().getValue());
    return found != bindings.types.end() &&
           evaluableWidth(found->second, bindings, active, analysis);
  }
  if (auto bits = dyn_cast<BitsType>(type))
    return evaluable(substitute(bits.getWidth(), bindings), bindings, active,
                     analysis);
  if (auto table = dyn_cast<TableType>(type))
    return evaluable(substitute(table.getShape(), bindings), bindings, active,
                     analysis) &&
           evaluableWidth(table.getElementType(), bindings, active, analysis);
  return isa<StructType, EnumType>(type);
}
bool evaluable(Attribute attr, const HardwareBindings &bindings,
               SmallVector<Attribute> &active,
               const HardwareAnalysis *analysis) {
  if (llvm::is_contained(active, attr))
    return true; // Invalid closed expressions must not look merely unbound.
  active.push_back(attr);
  llvm::scope_exit cleanup([&] { active.pop_back(); });
  if (auto expression = dyn_cast_or_null<StaticExprAttr>(attr))
    return evaluable(expression.getTree(), bindings, active, analysis);
  if (auto dictionary = dyn_cast_or_null<DictionaryAttr>(attr)) {
    auto kind = dictionary.getAs<StringAttr>("kind");
    if (kind && kind.getValue() == "reference") {
      auto ref = dictionary.getAs<DictionaryAttr>("ref");
      auto owner = ref ? ref.getAs<FlatSymbolRefAttr>("owner") : FlatSymbolRefAttr();
      auto name = ref ? ref.getAs<StringAttr>("name") : StringAttr();
      if (!bindings.owner || !owner || !name ||
          owner.getValue() != SymbolTable::getSymbolName(bindings.owner).getValue())
        return false;
      auto found = bindings.integers.find(name.getValue());
      return found != bindings.integers.end() &&
             evaluable(substitute(found->second, bindings), bindings, active,
                       analysis);
    }
    if (kind && kind.getValue() == "type_width") {
      auto attribute = dictionary.getAs<TypeAttr>("type");
      if (!attribute)
        return false;
      return evaluableWidth(attribute.getValue(), bindings, active, analysis);
    }
    // Demand follows the existing evaluator. The quiet probe only chooses an
    // already-closed branch; invalid closed selectors remain evaluable so the
    // owning evaluateStatic call emits their diagnostic at the real use site.
    auto demand = detail::ValueEvaluation::Strict;
    if (kind && kind.getValue() == "binary") {
      auto name = dictionary.getAs<StringAttr>("operator");
      auto opcode = name ? detail::parseValueOpcode(name.getValue()) : std::nullopt;
      if (opcode)
        demand = detail::getValueOpcodeInfo(*opcode).evaluation;
    }
    bool select = kind && kind.getValue() == "select";
    if (analysis && (select || demand != detail::ValueEvaluation::Strict)) {
      auto selector = dictionary.getAs<StaticExprAttr>(select ? "condition" : "lhs");
      if (!selector || !evaluable(selector, bindings, active, analysis))
        return false;
      Operation *site = bindings.owner ? bindings.owner : analysis->getPackage();
      if (!site)
        return false;
      FailureOr<Attribute> value = failure();
      {
        const auto probingThread = std::this_thread::get_id();
        ScopedDiagnosticHandler quiet(site->getContext(), [&](Diagnostic &) {
          return success(std::this_thread::get_id() == probingThread);
        });
        value = analysis->evaluateStatic(selector, bindings, site);
      }
      if (failed(value))
        return true;
      auto boolean = dyn_cast<BoolAttr>(*value);
      if (!boolean)
        return true;
      if (select)
        return evaluable(dictionary.getAs<StaticExprAttr>(
                             boolean.getValue() ? "yes" : "no"),
                         bindings, active, analysis);
      if ((demand == detail::ValueEvaluation::AndShortCircuit && !boolean.getValue()) ||
          (demand == detail::ValueEvaluation::OrShortCircuit && boolean.getValue()))
        return true;
      return evaluable(dictionary.getAs<StaticExprAttr>("rhs"), bindings, active,
                       analysis);
    }
    for (auto field : dictionary)
      if (!evaluable(field.getValue(), bindings, active, analysis))
        return false;
  }
  if (auto array = dyn_cast_or_null<ArrayAttr>(attr))
    for (auto value : array)
      if (!evaluable(value, bindings, active, analysis))
        return false;
  return true;
}
bool evaluable(Attribute attr, const HardwareBindings &bindings,
               const HardwareAnalysis *analysis) {
  SmallVector<Attribute> active;
  return evaluable(attr, bindings, active, analysis);
}
Attribute semantic(Attribute attr) {
  if (auto expr = dyn_cast_or_null<StaticExprAttr>(attr))
    return semantic(expr.getTree());
  if (auto dict = dyn_cast_or_null<DictionaryAttr>(attr)) {
    NamedAttrList fields;
    for (NamedAttribute field : dict)
      if (field.getName() != "origin" && field.getName() != "location")
        fields.append(field.getName(), semantic(field.getValue()));
    return fields.getDictionary(attr.getContext());
  }
  if (auto array = dyn_cast_or_null<ArrayAttr>(attr)) {
    SmallVector<Attribute> fields;
    for (auto value : array)
      fields.push_back(semantic(value));
    return ArrayAttr::get(attr.getContext(), fields);
  }
  return attr;
}
FailureOr<uint64_t> natural(Attribute attr, Operation *site, bool positive) {
  auto integer = dyn_cast_or_null<MathIntAttr>(attr);
  if (!integer)
    return site->emitOpError()
           << "hardware static value must be a mathematical integer";
  llvm::APSInt value(integer.getCanonicalValue());
  if (value.isNegative() || (positive && value.isZero()) ||
      value.getActiveBits() > 64)
    return site->emitOpError()
           << "hardware static value must be "
           << (positive ? "positive" : "nonnegative") << " and fit u64";
  return value.getZExtValue();
}

// Tables occupy one compact leaf. Their element traversal checks finite width
// with the same active stack as surrounding records, without expanding lanes.
LogicalResult visitPackedLayout(const HardwareAnalysis &analysis, Type type,
                                const HardwareBindings &bindings,
                                Operation *site, llvm::DenseSet<Type> &active,
                                SmallVector<PackedLeaf> *leaves, FieldPath path,
                                uint64_t &low) {
  auto resolved = analysis.resolveType(type, bindings, site);
  if (failed(resolved))
    return failure();
  type = *resolved;
  if (!active.insert(type).second)
    return site->emitOpError() << "recursive packed struct layout";
  llvm::scope_exit cleanup([&] { active.erase(type); });

  uint64_t width;
  if (auto bits = dyn_cast<BitsType>(type)) {
    auto value = analysis.evaluateStatic(bits.getWidth(), bindings, site);
    if (failed(value))
      return failure();
    auto naturalWidth = natural(*value, site, true);
    if (failed(naturalWidth))
      return failure();
    width = *naturalWidth;
  } else if (auto enumeration = dyn_cast<EnumType>(type)) {
    auto definition = analysis.resolveEnum(enumeration, site);
    if (failed(definition))
      return failure();
    width = definition->width;
  } else if (auto table = dyn_cast<TableType>(type)) {
    auto count = analysis.getTableSize(table, bindings, site);
    if (failed(count))
      return failure();
    uint64_t elementWidth = 0;
    if (failed(visitPackedLayout(analysis, table.getElementType(), bindings,
                               site, active, nullptr, {}, elementWidth)))
      return failure();
    if (elementWidth > std::numeric_limits<uint64_t>::max() / *count)
      return site->emitOpError() << "table packed width overflow";
    width = *count * elementWidth;
  } else if (auto record = dyn_cast<StructType>(type)) {
    auto declaration = analysis.lookupStruct(record);
    if (!declaration)
      return failure();
    // Walking backward gives field zero the most significant packed position.
    for (Attribute raw : llvm::reverse(declaration.getFields())) {
      auto field = dyn_cast<DictionaryAttr>(raw);
      if (!field || !field.getAs<TypeAttr>("type") ||
          !field.getAs<StringAttr>("name"))
        return site->emitOpError() << "invalid struct field";
      auto next = path;
      next.push_back(field.getAs<StringAttr>("name"));
      if (failed(visitPackedLayout(analysis,
                                 field.getAs<TypeAttr>("type").getValue(),
                                 bindings, site, active, leaves, next, low)))
        return failure();
    }
    return success();
  } else {
    return site->emitOpError()
           << "unresolved type parameter in finite hardware layout";
  }
  if (low > std::numeric_limits<uint64_t>::max() - width)
    return site->emitOpError() << "packed width overflow";
  if (leaves)
    leaves->push_back({path, type, low, width});
  low += width;
  return success();
}
} // namespace

bool areEquivalentHardwareTypes(Type lhs, Type rhs) {
  if (lhs == rhs)
    return true;
  auto lt = dyn_cast<TableType>(lhs), rt = dyn_cast<TableType>(rhs);
  if (lt && rt)
    return semantic(lt.getShape()) == semantic(rt.getShape()) &&
           areEquivalentHardwareTypes(lt.getElementType(), rt.getElementType());
  auto l = dyn_cast<BitsType>(lhs), r = dyn_cast<BitsType>(rhs);
  return l && r && semantic(l.getWidth()) == semantic(r.getWidth());
}
HardwareAnalysis::HardwareAnalysis(mlir::ModuleOp package) : package(package) {
  if (!package || package->getNumRegions() != 1 ||
      !package->getRegion(0).hasOneBlock())
    return;
  for (Operation &op : package.getBody()->getOperations()) {
    if (auto name = op.getAttrOfType<StringAttr>("sym_name"))
      symbols.try_emplace(name.getValue(), &op);
    if (auto module = dyn_cast<ModuleOp>(op))
      definitions.push_back(module);
    if (auto record = dyn_cast<StructOp>(op))
      structures.push_back(record);
  }
}
SmallVector<ModuleOp> HardwareAnalysis::getDefinitions() const {
  return definitions;
}
SmallVector<StructOp> HardwareAnalysis::getStructs() const {
  return structures;
}
Operation *HardwareAnalysis::lookupDefinition(StringRef name) const {
  auto found = symbols.find(name);
  return found == symbols.end() ? nullptr : found->second;
}
StructOp HardwareAnalysis::lookupStruct(StructType type) const {
  return dyn_cast_or_null<StructOp>(
      lookupDefinition(type.getName().getValue()));
}
Operation *HardwareAnalysis::resolveCallee(InstanceOp instance) const {
  return lookupDefinition(instance.getCallee());
}
Operation *HardwareAnalysis::resolveCallee(CollectionOp collection) const {
  return lookupDefinition(collection.getCallee());
}
bool HardwareAnalysis::isStaticEvaluable(
    Attribute expression, const HardwareBindings &bindings) const {
  return evaluable(substitute(expression, bindings), bindings, this);
}
StringRef HardwareAnalysis::getPrimitiveKind(Operation *definition) const {
  if (!isa_and_nonnull<ModuleImportOp>(definition))
    return {};
  auto attr = definition->getAttrOfType<StringAttr>("primitive_kind");
  return attr ? attr.getValue() : StringRef();
}
FailureOr<Attribute>
HardwareAnalysis::evaluateStatic(StaticExprAttr expression,
                                 const HardwareBindings &bindings,
                                 Operation *site) const {
  expression = cast<StaticExprAttr>(substitute(expression, bindings));
  auto key = std::make_pair(Attribute(expression), bindings.owner);
  if (llvm::is_contained(activeStatic, key))
    return site->emitOpError() << "cyclic demanded hardware static expression";
  activeStatic.push_back(key);
  llvm::scope_exit cleanup([&] { activeStatic.pop_back(); });
  auto tree = expression.getTree();
  auto kind = tree.getAs<StringAttr>("kind");
  if (!kind)
    return site->emitOpError() << "static expression requires kind";
  StringRef tag = kind.getValue();
  if (tag == "reference") {
    auto ref = tree.getAs<DictionaryAttr>("ref");
    auto owner = ref ? ref.getAs<FlatSymbolRefAttr>("owner") : FlatSymbolRefAttr();
    auto name = ref ? ref.getAs<StringAttr>("name") : StringAttr();
    if (!bindings.owner || !owner || !name ||
        owner.getValue() != SymbolTable::getSymbolName(bindings.owner).getValue() ||
        !bindings.integers.contains(name.getValue()))
      return site->emitOpError() << "unresolved hardware static expression";
    // Follow only explicit bindings. The active expression guard rejects cycles;
    // no declaration default or source-local alias becomes a static fact.
    return evaluateStatic(expression, bindings, site);
  }
  if (tag == "literal") {
    auto value = tree.getAs<DictionaryAttr>("value");
    auto result = value ? value.get("value") : Attribute();
    if (isa_and_nonnull<MathIntAttr, BoolAttr>(result))
      return result;
    return site->emitOpError()
           << "hardware static literal requires integer or bool";
  }
  if (tag == "type_width") {
    auto type = tree.getAs<TypeAttr>("type");
    if (!type)
      return site->emitOpError() << "type_width requires hardware type";
    auto width = getPackedWidth(type.getValue(), bindings, site);
    if (failed(width))
      return failure();
    return Attribute(MathIntAttr::get(
        site->getContext(), llvm::APSInt(llvm::APInt(65, *width), false)));
  }
  if (tag == "select") {
    auto condition =
        evaluateStatic(tree.getAs<StaticExprAttr>("condition"), bindings, site);
    if (failed(condition))
      return failure();
    auto boolean = dyn_cast<BoolAttr>(*condition);
    if (!boolean)
      return site->emitOpError() << "static select requires bool condition";
    return evaluateStatic(
        tree.getAs<StaticExprAttr>(boolean.getValue() ? "yes" : "no"), bindings,
        site);
  }
  if (tag != "unary" && tag != "binary")
    return site->emitOpError() << "unresolved hardware static expression";
  auto opcode =
      detail::parseValueOpcode(tree.getAs<StringAttr>("operator").getValue());
  if (!opcode)
    return site->emitOpError() << "unknown static operator";
  auto info = detail::getValueOpcodeInfo(*opcode);
  auto lhs = evaluateStatic(
      tree.getAs<StaticExprAttr>(tag == "unary" ? "operand" : "lhs"), bindings,
      site);
  if (failed(lhs))
    return failure();
  if (info.evaluation != detail::ValueEvaluation::Strict) {
    auto boolean = dyn_cast<BoolAttr>(*lhs);
    if (!boolean)
      return site->emitOpError() << "static boolean operator requires bool";
    if ((info.evaluation == detail::ValueEvaluation::AndShortCircuit &&
         !boolean.getValue()) ||
        (info.evaluation == detail::ValueEvaluation::OrShortCircuit &&
         boolean.getValue()))
      return *lhs;
  }
  SmallVector<Attribute> operands{*lhs};
  if (tag == "binary") {
    auto rhs =
        evaluateStatic(tree.getAs<StaticExprAttr>("rhs"), bindings, site);
    if (failed(rhs))
      return failure();
    operands.push_back(*rhs);
  }
  return detail::evaluateValue(*opcode, operands,
                               [&] { return site->emitOpError(); });
}
FailureOr<Type> HardwareAnalysis::resolveType(Type type,
                                              const HardwareBindings &bindings,
                                              Operation *site) const {
  if (auto table = dyn_cast<TableType>(type)) {
    SmallVector<Attribute> shape;
    for (auto raw : table.getShape()) {
      auto expr = dyn_cast<StaticExprAttr>(raw);
      if (!expr)
        return site->emitOpError() << "table extent requires StaticExpr";
      expr = cast<StaticExprAttr>(substitute(expr, bindings));
      if (evaluable(expr, bindings, this)) {
        auto value = evaluateStatic(expr, bindings, site);
        if (failed(value) || failed(natural(*value, site, true)))
          return failure();
        NamedAttrList tree;
        Builder b(type.getContext());
        tree.append("kind", b.getStringAttr("literal"));
        tree.append("origin", expr.getTree().get("origin"));
        tree.append("location", expr.getTree().get("location"));
        tree.append("value",
                    b.getDictionaryAttr(
                        {b.getNamedAttr("kind", b.getStringAttr("integer")),
                         b.getNamedAttr("value", *value)}));
        expr = StaticExprAttr::get(type.getContext(),
                                   tree.getDictionary(type.getContext()));
      }
      shape.push_back(expr);
    }
    auto element = resolveType(table.getElementType(), bindings, site);
    if (failed(element))
      return failure();
    return Type(TableType::get(
        type.getContext(), ArrayAttr::get(type.getContext(), shape), *element));
  }
  if (isa<TypeParamType>(type)) {
    SmallVector<Type> aliases;
    while (auto formal = dyn_cast<TypeParamType>(type)) {
      auto owner = lookupDefinition(formal.getOwner().getValue());
      auto names = owner ? owner->getAttrOfType<ArrayAttr>("type_parameters")
                         : ArrayAttr();
      if (!names || !llvm::is_contained(names, Attribute(formal.getName())))
        return site->emitOpError() << "unknown hardware type parameter";
      if (owner != bindings.owner)
        return type;
      if (llvm::is_contained(aliases, type))
        return site->emitOpError() << "cyclic type parameter binding";
      aliases.push_back(type);
      auto found = bindings.types.find(formal.getName().getValue());
      if (found == bindings.types.end())
        return type;
      if (!isa<BitsType, StructType, EnumType, TypeParamType>(found->second))
        return site->emitOpError()
               << "hardware type parameter requires a scalar hardware binding";
      type = found->second;
    }
    if (auto enumeration = dyn_cast<EnumType>(type))
      if (failed(resolveEnum(enumeration, site)))
        return failure();
    return type;
  }
  if (auto bits = dyn_cast<BitsType>(type)) {
    auto width = cast<StaticExprAttr>(substitute(bits.getWidth(), bindings));
    if (evaluable(width, bindings, this)) {
      auto value = evaluateStatic(width, bindings, site);
      if (failed(value) || failed(natural(*value, site, true)))
        return failure();
      NamedAttrList tree(width.getTree());
      Builder b(type.getContext());
      tree.clear();
      tree.append("kind", b.getStringAttr("literal"));
      tree.append("origin", width.getTree().get("origin"));
      tree.append("location", width.getTree().get("location"));
      tree.append("value",
                  b.getDictionaryAttr(
                      {b.getNamedAttr("kind", b.getStringAttr("integer")),
                       b.getNamedAttr("value", *value)}));
      width = StaticExprAttr::get(type.getContext(),
                                  tree.getDictionary(type.getContext()));
    }
    return Type(BitsType::get(type.getContext(), width));
  }
  if (auto record = dyn_cast<StructType>(type)) {
    if (!lookupStruct(record))
      return site->emitOpError() << "unresolved nominal hardware struct";
    return type;
  }
  if (auto enumeration = dyn_cast<EnumType>(type)) {
    if (failed(resolveEnum(enumeration, site)))
      return failure();
    return type;
  }
  return site->emitOpError()
         << "expected bits, packed struct, enum or declared type parameter";
}
FailureOr<SmallVector<PackedLeaf>>
HardwareAnalysis::getLeaves(Type type, const HardwareBindings &bindings,
                            Operation *site) const {
  auto rootType = resolveType(type, bindings, site);
  if (failed(rootType))
    return failure();
  type = *rootType;
  // Nominal fields may depend on occurrence bindings despite a resolved root.
  // Cache only layouts established without an occurrence-dependent context.
  bool cacheable = !bindings.owner && bindings.integers.empty() &&
                   bindings.types.empty();
  if (cacheable) {
    auto cached = layouts.find(type);
    if (cached != layouts.end())
      return cached->second;
  }
  SmallVector<PackedLeaf> leaves;
  llvm::DenseSet<Type> active;
  uint64_t width = 0;
  if (failed(visitPackedLayout(*this, type, bindings, site, active, &leaves, {},
                             width)))
    return failure();
  std::reverse(leaves.begin(), leaves.end());
  if (cacheable)
    layouts.try_emplace(type, leaves);
  return leaves;
}
FailureOr<uint64_t>
HardwareAnalysis::getPackedWidth(Type type, const HardwareBindings &bindings,
                                 Operation *site) const {
  auto leaves = getLeaves(type, bindings, site);
  if (failed(leaves))
    return failure();
  uint64_t width = 0;
  for (const auto &leaf : *leaves) {
    if (width > std::numeric_limits<uint64_t>::max() - leaf.width)
      return site->emitOpError() << "packed width overflow";
    width += leaf.width;
  }
  return width;
}
FailureOr<HardwareBindings>
HardwareAnalysis::bindInstance(InstanceOp instance,
                               const HardwareBindings &parent) const {
  return bindOccurrence(instance, parent);
}
FailureOr<HardwareBindings>
HardwareAnalysis::bindInstance(CollectionOp collection,
                               const HardwareBindings &parent) const {
  return bindOccurrence(collection, parent);
}
FailureOr<HardwareBindings>
HardwareAnalysis::bindOccurrence(Operation *instance,
                                 const HardwareBindings &parent) const {
  Operation *callee = lookupDefinition(
      instance->getAttrOfType<FlatSymbolRefAttr>("callee").getValue());
  auto actualParameters = instance->getAttrOfType<ArrayAttr>("parameters");
  auto actualTypes = instance->getAttrOfType<ArrayAttr>("type_arguments");
  if (!isa_and_nonnull<ModuleOp, ModuleImportOp>(callee))
    return instance->emitOpError() << "callee must resolve to hardware module";
  if (callee->getAttrOfType<StringAttr>("ac.root_kind"))
    return instance->emitOpError()
           << "source system is root-only and cannot be an instance callee";
  auto parameters = callee->getAttrOfType<ArrayAttr>("parameters");
  auto names = callee->getAttrOfType<ArrayAttr>("type_parameters");
  if (actualParameters.size() > parameters.size() ||
      actualTypes.size() != names.size())
    return instance->emitOpError() << "instance parameter arity mismatch";
  HardwareBindings result;
  result.owner = callee;
  for (auto [name, raw] : llvm::zip(names, actualTypes)) {
    auto resolved =
        resolveType(cast<TypeAttr>(raw).getValue(), parent, instance);
    if (failed(resolved))
      return failure();
    result.types[cast<StringAttr>(name).getValue()] = *resolved;
  }
  for (auto [index, raw] : llvm::enumerate(parameters)) {
    auto parameter = cast<DictionaryAttr>(raw);
    StaticExprAttr expr;
    bool supplied = index < actualParameters.size();
    expr = supplied ? cast<StaticExprAttr>(actualParameters[index])
                    : parameter.getAs<StaticExprAttr>("default");
    if (!expr)
      return instance->emitOpError() << "missing required static parameter";
    auto substituted =
        cast<StaticExprAttr>(substitute(expr, supplied ? parent : result));
    Attribute value = substituted;
    if (evaluable(substituted, supplied ? parent : result, this)) {
      auto evaluated =
          evaluateStatic(substituted, supplied ? parent : result, instance);
      if (failed(evaluated))
        return failure();
      if (!isa<MathIntAttr>(*evaluated))
        return instance->emitOpError()
               << "integer formal requires mathematical integer";
      value = *evaluated;
    }
    result.integers[parameter.getAs<StringAttr>("name").getValue()] = value;
  }
  return result;
}
LogicalResult
HardwareAnalysis::verifyInstance(InstanceOp instance,
                                 const HardwareBindings &parent) const {
  return verifyOccurrence(instance, parent);
}
LogicalResult
HardwareAnalysis::verifyInstance(CollectionOp collection,
                                 const HardwareBindings &parent) const {
  return verifyOccurrence(collection, parent);
}
LogicalResult
HardwareAnalysis::verifyOccurrence(Operation *instance,
                                   const HardwareBindings &parent) const {
  auto bindings = bindOccurrence(instance, parent);
  if (failed(bindings))
    return failure();
  auto sig = cast<FunctionType>(
      bindings->owner->getAttrOfType<TypeAttr>("function_type").getValue());
  if (instance->getNumOperands() != sig.getNumInputs() ||
      instance->getNumResults() != sig.getNumResults())
    return instance->emitOpError() << "instance signature arity mismatch";
  auto check = [&](Type actual, Type expected) {
    auto a = resolveType(actual, parent, instance),
         e = resolveType(expected, *bindings, instance);
    if (failed(a) || failed(e))
      return failure();
    if (auto collection = dyn_cast<CollectionOp>(instance)) {
      auto table = resolveType(
          TableType::get(instance->getContext(), collection.getShape(), *e),
          parent, instance);
      if (failed(table))
        return failure();
      e = *table;
    }
    if (!areEquivalentHardwareTypes(*a, *e))
      return LogicalResult(
          instance->emitOpError()
          << "instance wire type does not match bound callee signature");
    return success();
  };
  for (auto [a, e] : llvm::zip(instance->getOperandTypes(), sig.getInputs()))
    if (failed(check(a, e)))
      return failure();
  for (auto [a, e] : llvm::zip(instance->getResultTypes(), sig.getResults()))
    if (failed(check(a, e)))
      return failure();
  return success();
}
LogicalResult verifyHardwarePackage(mlir::ModuleOp package) {
  if (!package)
    return failure();
  return HardwareAnalysis(package).verify();
}
} // namespace acir::ac
namespace acir::ac {
LogicalResult HardwareAnalysis::verifyResolvedOperation(
    Operation *op, const HardwareBindings &bindings) const {
  if (auto queue = dyn_cast<QueueOp>(op))
    return verifyQueueOperation(queue, bindings, true);
  if (isa<EnumCreateOp, EnumToBitsOp, EnumFromBitsOp>(op))
    return verifyEnumValueOperation(op, bindings, true);
  if (isa<CollectionOp, TableCreateOp, TableSplatOp, TableMapOp, TableViewOp,
          TableIndexOp, TableGetOp, TableMatchOp, TableChooseOp, TableFoldOp,
          ValueMergeOp>(op) &&
      failed(verifyCollectionOperation(op, bindings, true)))
    return failure();
  SmallVector<uint64_t> inputs, outputs;
  for (Type type : op->getOperandTypes()) {
    auto width = getPackedWidth(type, bindings, op);
    if (failed(width))
      return failure();
    inputs.push_back(*width);
  }
  for (Type type : op->getResultTypes()) {
    auto width = getPackedWidth(type, bindings, op);
    if (failed(width))
      return failure();
    outputs.push_back(*width);
  }
  if (auto constant = dyn_cast<BitsConstantOp>(op)) {
    auto value = evaluateStatic(constant.getValue(), bindings, op);
    if (failed(value))
      return failure();
    auto integer = dyn_cast<MathIntAttr>(*value);
    if (!integer)
      return op->emitOpError() << "bits constant requires integer";
    llvm::APSInt number(integer.getCanonicalValue());
    if (number.isNegative() || number.getActiveBits() > outputs[0])
      return op->emitOpError()
             << "bits constant does not fit bound result width";
  }
  if (isa<BitsUnaryOp, BitsBinaryOp, BitsSelectOp>(op)) {
    unsigned first = isa<BitsSelectOp>(op) ? 1 : 0;
    if (first && inputs[0] != 1)
      return op->emitOpError() << "select condition must have width one";
    for (unsigned index = first; index < inputs.size(); ++index)
      if (inputs[index] != outputs[0])
        return op->emitOpError()
               << "bound bit operands and result must have equal widths";
  }
  if (isa<BitsCompareOp>(op) && (inputs[0] != inputs[1] || outputs[0] != 1))
    return op->emitOpError()
           << "bound comparison requires equal operands and width-one result";
  if (isa<BitsConcatOp>(op)) {
    uint64_t total = 0;
    for (auto width : inputs) {
      if (total > std::numeric_limits<uint64_t>::max() - width)
        return op->emitOpError() << "concat width overflow";
      total += width;
    }
    if (total != outputs[0])
      return op->emitOpError()
             << "bound concat result must equal input width sum";
  }
  if (auto extract = dyn_cast<BitsExtractOp>(op)) {
    auto value = evaluateStatic(extract.getLow(), bindings, op);
    if (failed(value))
      return failure();
    auto low = natural(*value, op, false);
    if (failed(low))
      return failure();
    if (*low > inputs[0] || outputs[0] > inputs[0] - *low)
      return op->emitOpError() << "bound extract slice exceeds input width";
  }
  if (auto resize = dyn_cast<BitsResizeOp>(op)) {
    if ((resize.getMode() == "trunc" && outputs[0] >= inputs[0]) ||
        (resize.getMode() != "trunc" && outputs[0] <= inputs[0]))
      return op->emitOpError() << "bound resize direction disagrees with mode";
  }
  if (isa<InstanceOp, CollectionOp>(op)) {
    auto child = isa<InstanceOp>(op)
                     ? bindInstance(cast<InstanceOp>(op), bindings)
                     : bindInstance(cast<CollectionOp>(op), bindings);
    if (failed(child))
      return failure();
    auto kind = getPrimitiveKind(child->owner);
    if (kind == "sync_mem" || kind == "sync_mem_dp" || kind == "byte_mem") {
      auto payload = child->types.find("T");
      if (payload == child->types.end())
        return op->emitOpError() << "memory missing payload binding";
      auto width = getPackedWidth(payload->second, *child, op);
      if (failed(width))
        return failure();
      if (kind == "byte_mem" && *width % 8)
        return op->emitOpError()
               << "byte_mem payload width must be byte aligned";
      auto imported = cast<ModuleImportOp>(child->owner);
      unsigned strobe = imported.getInputNames().size() - 1;
      uint64_t expected =
          *width / 8 + ((kind != "byte_mem" && *width % 8) ? 1 : 0);
      if (auto collection = dyn_cast<CollectionOp>(op)) {
        auto table = cast<TableType>(collection.getInputs()[strobe].getType());
        auto actual = getPackedWidth(table.getElementType(), bindings, op);
        if (failed(actual))
          return failure();
        inputs[strobe] = *actual;
      }
      if (inputs[strobe] != expected)
        return op->emitOpError()
               << "memory strobe width must derive from packed payload bytes";
      for (StringRef name : {"ADDR_WIDTH", "DEPTH"}) {
        auto found = child->integers.find(name);
        if (found == child->integers.end())
          return op->emitOpError() << "memory missing integer binding";
        auto attr = found->second;
        if (auto expr = dyn_cast<StaticExprAttr>(attr)) {
          auto value = evaluateStatic(expr, *child, op);
          if (failed(value))
            return failure();
          attr = *value;
        }
        if (failed(natural(attr, op, true)))
          return failure();
      }
    }
  }
  return success();
}
} // namespace acir::ac
