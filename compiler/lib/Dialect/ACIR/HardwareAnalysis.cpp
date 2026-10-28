#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "HardwareSourceCheckPreflight.h"
#include "SourceValueSemantics.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "ACIRSourceContracts.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/IR/ConstantRange.h"
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
    // Provenance is immutable source evidence, not part of the static
    // computation. Literal and type-width nodes have no expression children
    // to substitute; keep their complete authored metadata by identity.
    if (!kind || kind.getValue() == "literal" ||
        kind.getValue() == "type_width" || kind.getValue() == "reference")
      return expr;
    SmallVector<StringRef, 3> children;
    if (kind.getValue() == "unary") children.push_back("operand");
    else if (kind.getValue() == "binary") children.append({"lhs", "rhs"});
    else if (kind.getValue() == "select") children.append({"condition", "yes", "no"});
    else return expr; // Existing grammar/evaluation checks reject unknown kinds.
    SmallVector<std::pair<StringRef, Attribute>, 3> changed;
    for (StringRef name : children) {
      Attribute child = tree.get(name);
      Attribute rewritten = substitute(child, bindings);
      if (rewritten != child) changed.push_back({name, rewritten});
    }
    if (changed.empty()) return expr;
    NamedAttrList rewritten(tree);
    for (auto [name, value] : changed) rewritten.set(name, value);
    return StaticExprAttr::get(attr.getContext(),
                               rewritten.getDictionary(attr.getContext()));
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
Attribute HardwareAnalysis::substituteAttribute(
    Attribute attribute, const HardwareBindings &bindings) const {
  return substitute(attribute, bindings);
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
      if (!isa<BitsType, StructType, EnumType, TableType, TypeParamType>(found->second))
        return site->emitOpError()
               << "hardware type parameter requires a finite hardware binding";
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
  return bindCallee(callee, actualParameters, actualTypes, parent, instance);
}
FailureOr<HardwareBindings> HardwareAnalysis::bindCallee(
    Operation *callee, ArrayAttr actualParameters, ArrayAttr actualTypes,
    const HardwareBindings &parent, Operation *site) const {
  if (!isa_and_nonnull<ModuleOp, ModuleImportOp>(callee))
    return site->emitOpError() << "callee must resolve to hardware module";
  if (callee->getAttrOfType<StringAttr>("ac.root_kind"))
    return site->emitOpError()
           << "source system is root-only and cannot be an instance callee";
  auto parameters = callee->getAttrOfType<ArrayAttr>("parameters");
  auto names = callee->getAttrOfType<ArrayAttr>("type_parameters");
  if (!actualParameters || !actualTypes || !parameters || !names ||
      actualParameters.size() > parameters.size() ||
      actualTypes.size() != names.size())
    return site->emitOpError() << "instance parameter arity mismatch";
  HardwareBindings result;
  result.owner = callee;
  for (auto [name, raw] : llvm::zip(names, actualTypes)) {
    auto actual = dyn_cast<TypeAttr>(raw);
    if (!actual || !isa<StringAttr>(name))
      return site->emitOpError() << "invalid instance type arguments";
    auto resolved = resolveType(actual.getValue(), parent, site);
    if (failed(resolved))
      return failure();
    result.types[cast<StringAttr>(name).getValue()] = *resolved;
  }
  for (auto [index, raw] : llvm::enumerate(parameters)) {
    auto parameter = dyn_cast<DictionaryAttr>(raw);
    if (!parameter || !parameter.getAs<StringAttr>("name"))
      return site->emitOpError() << "invalid instance static parameter declaration";
    StaticExprAttr expr;
    bool supplied = index < actualParameters.size();
    expr = supplied ? dyn_cast<StaticExprAttr>(actualParameters[index])
                    : parameter.getAs<StaticExprAttr>("default");
    if (!expr)
      return site->emitOpError() << "missing required static parameter";
    auto substituted =
        cast<StaticExprAttr>(substitute(expr, supplied ? parent : result));
    Attribute value = substituted;
    if (evaluable(substituted, supplied ? parent : result, this)) {
      auto evaluated =
          evaluateStatic(substituted, supplied ? parent : result, site);
      if (failed(evaluated))
        return failure();
      if (!isa<MathIntAttr>(*evaluated))
        return site->emitOpError()
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
  if (failed(verifyOccurrence(instance, parent)))
    return failure();
  auto target = lookupDefinition(instance.getCallee());
  if (failed(verifySourceReferenceBindings(instance->getParentOp(), target,
          instance->getAttr("ac.reference_bindings"),
          [&](FlatSymbolRefAttr symbol) { return lookupDefinition(symbol.getValue()); }, instance)))
    return failure();
  if (isQueueInstance(instance))
    return verifyQueueOperation(instance, parent);
  if (instance->hasAttr("ac.source_memory")) {
    if (!isa<UnitAttr>(instance->getAttr("ac.source_memory")))
      return instance.emitOpError() << "ac.source_memory requires a unit marker";
    auto layout = getMemoryPrimitiveLayout(instance, parent);
    if (failed(layout)) return failure();
    auto constraint = instance->getAttr("ac.payload_constraint");
    if (!constraint || failed(detail::verifySourceCallHardwareConstraint(
            constraint, layout->payloadType, *this, instance->getParentOp(),
            instance, "source Memory payload")))
      return failure();
    for (StringRef flag : {"ac.source_unbound_push", "ac.source_unbound_pop"})
      if (instance->hasAttr(flag))
        return instance.emitOpError() << "Queue binding flag is invalid on Memory";
    return success();
  }
  for (StringRef name : {"ac.payload_constraint", "ac.source_unbound_push",
                         "ac.source_unbound_pop"})
    if (instance->hasAttr(name))
      return instance.emitOpError()
             << name << " requires a canonical FIFO instance";
  return success();
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
  auto primitive = getPrimitiveKind(bindings->owner);
  if (!primitive.empty() && primitive != "fifo_local" &&
      primitive != "fifo_downstream_pop" &&
      isa_and_nonnull<TableType>(bindings->types.lookup("T")))
    return instance->emitOpError()
           << "storage and memory primitives require a scalar payload binding";
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
LogicalResult verifyRoutedPackage(mlir::ModuleOp package,
                                  HardwareSourceCheckLimits limits) {
  if (!package)
    return failure();
  HardwareAnalysis analysis(package);
  return success(succeeded(detail::buildSourceCheckPlan(
      analysis, limits, detail::PackageVerificationProfile::Routed)));
}
} // namespace acir::ac
namespace acir::ac {
LogicalResult HardwareAnalysis::verifySourceIndex(
    Value index, TableType table, const HardwareBindings &bindings,
    Operation *site) const {
  using Range = llvm::ConstantRange;
  using Pred = llvm::CmpInst::Predicate;
  auto extent = getTableSize(table, bindings, site);
  if (failed(extent) || table.getShape().size() != 1 || !isa<BitsType>(index.getType()))
    return failure();
  detail::SourceCheckBudget budget(HardwareSourceCheckLimits{});
  budget.setPhase("checked Table index ranges");
  SmallVector<std::unique_ptr<HardwareBindings>, 4> contexts;
  auto context = [&](const HardwareBindings &actual) -> FailureOr<unsigned> {
    for (auto [ordinal, prior] : llvm::enumerate(contexts)) {
      if (failed(budget.debit(detail::SourceCheckBudget::bindings(actual), site))) return failure();
      if (detail::sameHardwareBindings(*prior, actual)) return unsigned(ordinal);
    }
    if (failed(budget.debit(detail::SourceCheckBudget::add(
            detail::SourceCheckBudget::bindings(actual),
            detail::SourceCheckBudget::bytes(sizeof(HardwareBindings))), site))) return failure();
    contexts.push_back(std::make_unique<HardwareBindings>(actual));
    return unsigned(contexts.size() - 1);
  };
  struct Node { Value value; unsigned context; };
  auto validate = [&](Operation *operation, unsigned scope) -> LogicalResult {
    if (failed(budget.debit(16 + operation->getNumOperands() + operation->getNumResults(), site))) return failure();
    return verifyResolvedOperation(operation, *contexts[scope]);
  };
  // Only value-preserving routes are identities. In particular Module inputs
  // never follow caller samples, and ordinary Method formals are not captures.
  auto alias = [&](Node node) -> FailureOr<std::optional<Node>> {
    Value value = node.value, next;
    unsigned scope = node.context, nextScope = scope;
    auto &actual = *contexts[scope];
    if (auto extract = value.getDefiningOp<BitsExtractOp>()) {
      if (failed(extract.verifyInvariantsImpl()) || failed(extract.verify()) || failed(validate(extract, scope))) return failure();
      auto low = evaluateStatic(extract.getLow(), actual, site);
      auto input = getPackedWidth(extract.getInput().getType(), actual, site);
      auto output = getPackedWidth(value.getType(), actual, site);
      auto integer = succeeded(low) ? dyn_cast<MathIntAttr>(*low) : MathIntAttr();
      if (failed(input) || failed(output) || !integer) return failure();
      if (integer.getCanonicalValue() == "0" && *input == *output) next = extract.getInput();
    } else if (auto resize = value.getDefiningOp<BitsResizeOp>(); resize && resize.getMode() == "zext") {
      if (failed(resize.verifyInvariantsImpl()) || failed(resize.verify()) || failed(validate(resize, scope))) return failure();
      next = resize.getInput();
    } else if (auto argument = dyn_cast<BlockArgument>(value)) {
      Operation *owner = argument.getOwner()->getParentOp();
      unsigned ordinal = argument.getArgNumber();
      if (auto rule = dyn_cast<RuleOp>(owner)) {
        if (argument.getOwner()->getNumArguments() != rule.getCaptures().size() || ordinal >= rule.getCaptures().size()) return failure();
        next = rule.getCaptures()[ordinal];
      } else if (auto transaction = dyn_cast<SourceTransactionOp>(owner)) {
        if (argument.getOwner()->getNumArguments() != transaction.getCaptures().size() || ordinal >= transaction.getCaptures().size()) return failure();
        next = transaction.getCaptures()[ordinal];
      } else if (auto method = dyn_cast<SourceMethodOp>(owner)) {
        auto signature = method.getFunctionType();
        if (argument.getOwner()->getNumArguments() != signature.getNumInputs() + method.getCaptures().size()) return failure();
        if (ordinal >= signature.getNumInputs()) next = method.getCaptures()[ordinal - signature.getNumInputs()];
        else {
          auto module = method->getParentOfType<ModuleOp>();
          for (auto transaction : module.getBody().front().getOps<SourceTransactionOp>()) {
            if (failed(budget.debit(1, site))) return failure();
            if (transaction.getOccurrence() != transaction.getDeclaration() || transaction.getDeclaration() != method.getOccurrence()) continue;
            auto entry = getSourceRegistrationEntry(transaction);
            if (failed(entry) || !*entry || (**entry).method != method || ordinal >= (**entry).argumentCaptureOrdinals.size()) return failure();
            unsigned capture = (**entry).argumentCaptureOrdinals[ordinal];
            if (next || capture >= transaction.getCaptures().size()) return failure();
            next = transaction.getCaptures()[capture];
          }
        }
      }
      if (next) {
        auto formal = resolveType(value.getType(), actual, site), supplied = resolveType(next.getType(), actual, site);
        if (failed(formal) || failed(supplied) || !areEquivalentHardwareTypes(*formal, *supplied)) return failure();
      }
    } else if (auto result = dyn_cast<OpResult>(value)) {
      if (auto rule = dyn_cast<RuleOp>(result.getOwner())) {
        if (!rule.getBody().hasOneBlock() || rule.getBody().front().empty()) return failure();
        auto yield = dyn_cast<YieldOp>(rule.getBody().front().back());
        if (!yield || result.getResultNumber() >= yield.getNumOperands()) return failure();
        next = yield.getOperand(result.getResultNumber());
      } else if (auto instance = dyn_cast<InstanceOp>(result.getOwner())) {
        auto child = dyn_cast_or_null<ModuleOp>(resolveCallee(instance));
        if (child && getPrimitiveKind(child).empty()) {
          auto bound = bindInstance(instance, actual);
          if (failed(bound) || !child.getBody().hasOneBlock() || child.getBody().front().empty()) return failure();
          auto yield = dyn_cast<YieldOp>(child.getBody().front().back());
          if (!yield || result.getResultNumber() >= yield.getNumOperands()) return failure();
          next = yield.getOperand(result.getResultNumber());
          auto formal = resolveType(value.getType(), actual, site), supplied = resolveType(next.getType(), *bound, site);
          if (failed(formal) || failed(supplied) || !areEquivalentHardwareTypes(*formal, *supplied)) return failure();
          auto id = context(*bound); if (failed(id)) return failure(); nextScope = *id;
        }
      }
    }
    if (!next) return std::optional<Node>();
    return std::optional<Node>(Node{next, nextScope});
  };
  auto canonical = [&](Node node) -> FailureOr<Node> {
    for (unsigned depth = 0; depth < 128; ++depth) {
      if (failed(budget.debit(1, site))) return failure();
      auto next = alias(node); if (failed(next)) return failure();
      if (!*next) return node;
      node = **next;
    }
    return site->emitOpError() << "checked index identity depth exceeded";
  };
  auto predicate = [](StringRef spelling) -> std::optional<Pred> {
    return llvm::StringSwitch<std::optional<Pred>>(spelling)
      .Case("eq", llvm::CmpInst::ICMP_EQ).Case("case_eq", llvm::CmpInst::ICMP_EQ)
      .Case("ne", llvm::CmpInst::ICMP_NE).Case("ult", llvm::CmpInst::ICMP_ULT)
      .Case("ule", llvm::CmpInst::ICMP_ULE).Case("ugt", llvm::CmpInst::ICMP_UGT)
      .Case("uge", llvm::CmpInst::ICMP_UGE).Case("slt", llvm::CmpInst::ICMP_SLT)
      .Case("sle", llvm::CmpInst::ICMP_SLE).Case("sgt", llvm::CmpInst::ICMP_SGT)
      .Case("sge", llvm::CmpInst::ICMP_SGE).Default(std::nullopt);
  };
  using Key = std::pair<Value, std::pair<unsigned, uint64_t>>;
  struct Memo { bool complete = false; std::optional<Range> range; };
  llvm::DenseMap<Key, Memo> memo;
  std::function<FailureOr<Range>(Node, unsigned)> infer;
  // Branch refinement is local. Cached ranges are unconditional, so a narrow
  // arm fact can never be reused for a broader value/refinement context.
  auto refine = [&](Node arm, Node condition, bool whenTrue, Range range, unsigned depth) -> FailureOr<Range> {
    auto control = canonical(condition); if (failed(control)) return failure();
    auto compare = control->value.getDefiningOp<BitsCompareOp>();
    if (!compare) return range;
    auto pred = predicate(compare.getPredicate()); if (!pred) return range;
    if (!whenTrue) *pred = llvm::CmpInst::getInversePredicate(*pred);
    auto target = canonical(arm); if (failed(target)) return failure();
    for (unsigned side = 0; side < 2; ++side) {
      auto variable = canonical(Node{compare->getOperand(side), control->context});
      if (failed(variable)) return failure();
      if (target->value != variable->value || target->context != variable->context) continue;
      auto other = infer(Node{compare->getOperand(1 - side), control->context}, depth + 1);
      if (failed(other)) return failure();
      if (!other->isSingleElement()) continue;
      auto selected = side ? llvm::CmpInst::getSwappedPredicate(*pred) : *pred;
      Range allowed = Range::makeExactICmpRegion(selected, *other->getSingleElement());
      return range.intersectWith(allowed.zextOrTrunc(range.getBitWidth()));
    }
    return range;
  };
  // Prove the unsigned quotient relation from actual arithmetic SSA. No
  // importer recipe tag or claimed interval participates in this proof.
  auto remainder = [&](BitsBinaryOp subtract, unsigned scope, unsigned depth) -> FailureOr<std::optional<Range>> {
    if (subtract.getOpcode() != "sub") return std::optional<Range>();
    auto scaled = subtract.getRhs().getDefiningOp<BitsBinaryOp>();
    if (!scaled || scaled.getOpcode() != "mul") return std::optional<Range>();
    for (unsigned side = 0; side < 2; ++side) {
      auto quotient = scaled->getOperand(side).getDefiningOp<BitsExtractOp>();
      if (!quotient) continue;
      auto product = quotient.getInput().getDefiningOp<BitsBinaryOp>();
      if (!product || product.getOpcode() != "mul") continue;
      auto divisorRange = infer(Node{scaled->getOperand(1 - side), scope}, depth + 1);
      if (failed(divisorRange)) return failure();
      auto divisor = divisorRange->getSingleElement();
      if (!divisor || divisor->isZero()) continue;
      for (unsigned factor = 0; factor < 2; ++factor) {
        auto wide = product->getOperand(factor).getDefiningOp<BitsResizeOp>();
        if (!wide || wide.getMode() != "zext" || wide.getInput() != subtract.getLhs()) continue;
        auto multiplierRange = infer(Node{product->getOperand(1 - factor), scope}, depth + 1);
        if (failed(multiplierRange)) return failure();
        auto multiplier = multiplierRange->getSingleElement();
        if (!multiplier || multiplier->isZero()) continue;
        auto &actual = *contexts[scope];
        for (Operation *operation : {scaled.getOperation(), quotient.getOperation(), product.getOperation(), wide.getOperation()})
          if (failed(validate(operation, scope))) return failure();
        auto sourceWidth = getPackedWidth(subtract.getLhs().getType(), actual, site);
        auto productWidth = getPackedWidth(product.getResult().getType(), actual, site);
        auto lowValue = evaluateStatic(quotient.getLow(), actual, site);
        auto low = succeeded(lowValue) ? dyn_cast<MathIntAttr>(*lowValue) : MathIntAttr();
        if (failed(sourceWidth) || failed(productWidth) || !low) return failure();
        llvm::APSInt shiftValue(low.getCanonicalValue());
        if (shiftValue.isNegative() || shiftValue.getActiveBits() > 32) continue;
        uint64_t shift = shiftValue.getZExtValue();
        uint64_t precision = std::max({*productWidth + 2, shift + 2,
            *sourceWidth + multiplier->getActiveBits() + divisor->getActiveBits() + 4});
        if (precision > std::numeric_limits<unsigned>::max() ||
            failed(budget.debit(detail::SourceCheckBudget::multiply(
                16, detail::SourceCheckBudget::bytes((precision + 7) / 8)), site))) return failure();
        unsigned bits = unsigned(precision), width = unsigned(*sourceWidth);
        llvm::APInt power(bits, 0), modulus(bits, 0);
        power.setBit(unsigned(shift)); modulus.setBit(unsigned(*productWidth));
        auto d = divisor->zextOrTrunc(bits), m = multiplier->zextOrTrunc(bits);
        auto maximum = llvm::APInt::getLowBitsSet(bits, width);
        auto scaledPower = m * d;
        if (scaledPower.ult(power) || (scaledPower - power).uge(d) || (maximum * m).uge(modulus)) continue;
        auto error = scaledPower - power, q = maximum.udiv(d), r = maximum.urem(d);
        auto tail = q * error + r * m;
        auto full = q.isZero() ? llvm::APInt(bits, 0) : (q - 1) * error + (d - 1) * m;
        if (tail.uge(power) || full.uge(power)) continue;
        return std::optional<Range>(Range(llvm::APInt(width, 0), divisor->zextOrTrunc(width)));
      }
    }
    return std::optional<Range>();
  };
  infer = [&](Node node, unsigned depth) -> FailureOr<Range> {
    if (depth > 128) return site->emitOpError() << "checked index range depth exceeded";
    if (failed(budget.debit(16, site))) return failure();
    auto &actual = *contexts[node.context];
    auto boundWidth = getPackedWidth(node.value.getType(), actual, site);
    if (failed(boundWidth) || !isa<BitsType>(node.value.getType()) ||
        *boundWidth > std::numeric_limits<unsigned>::max()) return failure();
    unsigned width = unsigned(*boundWidth);
    uint64_t words = (uint64_t(width) + 63) / 64;
    // Cache hits copy arbitrary-precision interval endpoints too.
    if (failed(budget.debit(detail::SourceCheckBudget::multiply(4, words), site))) return failure();
    Key key{node.value, {node.context, *boundWidth}};
    if (auto found = memo.find(key); found != memo.end()) {
      if (!found->second.complete) return site->emitOpError() << "cyclic checked index range";
      return *found->second.range;
    }
    if (failed(budget.debit(detail::SourceCheckBudget::add(64,
        detail::SourceCheckBudget::multiply(16, words)), site))) return failure();
    memo.try_emplace(key);
    auto compute = [&]() -> FailureOr<Range> {
      auto next = alias(node); if (failed(next)) return failure();
      if (*next) {
        auto range = infer(**next, depth + 1);
        if (failed(range)) return failure();
        return range->zextOrTrunc(width);
      }
      auto *operation = node.value.getDefiningOp();
      if (auto constant = dyn_cast_or_null<BitsConstantOp>(operation)) {
        if (failed(constant.verifyInvariantsImpl()) || failed(constant.verify()) || failed(validate(constant, node.context))) return failure();
        auto evaluated = evaluateStatic(constant.getValue(), actual, site);
        auto integer = succeeded(evaluated) ? dyn_cast<MathIntAttr>(*evaluated) : MathIntAttr();
        if (!integer) return failure();
        llvm::APSInt number(integer.getCanonicalValue());
        if (number.isNegative() || number.getActiveBits() > width) return failure();
        return Range(number.zextOrTrunc(width));
      }
      if (auto extract = dyn_cast_or_null<BitsExtractOp>(operation)) {
        auto input = infer(Node{extract.getInput(), node.context}, depth + 1);
        auto low = evaluateStatic(extract.getLow(), actual, site);
        auto integer = succeeded(low) ? dyn_cast<MathIntAttr>(*low) : MathIntAttr();
        if (failed(input) || !integer || failed(validate(extract, node.context))) return failure();
        llvm::APSInt amount(integer.getCanonicalValue());
        if (amount.isNegative() || amount.getActiveBits() > 32) return failure();
        return input->lshr(Range(llvm::APInt(input->getBitWidth(), amount.getZExtValue()))).zextOrTrunc(width);
      }
      if (auto resize = dyn_cast_or_null<BitsResizeOp>(operation)) {
        auto input = infer(Node{resize.getInput(), node.context}, depth + 1);
        if (failed(input) || failed(validate(resize, node.context))) return failure();
        return resize.getMode() == "sext" ? input->sextOrTrunc(width) : input->zextOrTrunc(width);
      }
      auto selectRange = [&](Value condition, Value yes, Value no) -> FailureOr<Range> {
        auto a = infer(Node{yes, node.context}, depth + 1), b = infer(Node{no, node.context}, depth + 1);
        if (failed(a) || failed(b)) return failure();
        auto left = refine(Node{yes, node.context}, Node{condition, node.context}, true, *a, depth + 1);
        auto right = refine(Node{no, node.context}, Node{condition, node.context}, false, *b, depth + 1);
        if (failed(left) || failed(right)) return failure();
        return left->unionWith(*right);
      };
      if (auto select = dyn_cast_or_null<BitsSelectOp>(operation)) {
        if (failed(select.verifyInvariantsImpl()) || failed(select.verify()) || failed(validate(select, node.context))) return failure();
        return selectRange(select.getCondition(), select.getTrueValue(), select.getFalseValue());
      }
      if (auto branch = dyn_cast_or_null<SourceBranchOp>(operation)) {
        if (!branch.getThenRegion().hasOneBlock() || !branch.getElseRegion().hasOneBlock() ||
            branch.getThenRegion().front().empty() || branch.getElseRegion().front().empty()) return failure();
        auto yes = dyn_cast<YieldOp>(branch.getThenRegion().front().back()), no = dyn_cast<YieldOp>(branch.getElseRegion().front().back());
        unsigned ordinal = cast<OpResult>(node.value).getResultNumber();
        if (!yes || !no || ordinal >= yes.getNumOperands() || ordinal >= no.getNumOperands()) return failure();
        return selectRange(branch.getCondition(), yes.getOperand(ordinal), no.getOperand(ordinal));
      }
      if (auto binary = dyn_cast_or_null<BitsBinaryOp>(operation)) {
        if (failed(binary.verifyInvariantsImpl()) || failed(binary.verify()) || failed(validate(binary, node.context))) return failure();
        auto residual = remainder(binary, node.context, depth + 1);
        if (failed(residual)) return failure();
        if (*residual) return **residual;
        auto lhs = infer(Node{binary.getLhs(), node.context}, depth + 1), rhs = infer(Node{binary.getRhs(), node.context}, depth + 1);
        if (failed(lhs) || failed(rhs)) return failure();
        StringRef op = binary.getOpcode();
        if ((op == "udiv" || op == "urem") && rhs->contains(llvm::APInt(width, 0))) return Range::getFull(width);
        if ((op == "shl" || op == "lshr" || op == "ashr") && rhs->getUnsignedMax().uge(width)) return Range::getFull(width);
        auto opcode = llvm::StringSwitch<std::optional<llvm::Instruction::BinaryOps>>(op)
            .Case("add", llvm::Instruction::Add).Case("sub", llvm::Instruction::Sub).Case("mul", llvm::Instruction::Mul)
            .Case("and", llvm::Instruction::And).Case("or", llvm::Instruction::Or).Case("xor", llvm::Instruction::Xor)
            .Case("udiv", llvm::Instruction::UDiv).Case("urem", llvm::Instruction::URem)
            .Case("shl", llvm::Instruction::Shl).Case("lshr", llvm::Instruction::LShr).Case("ashr", llvm::Instruction::AShr)
            .Default(std::nullopt);
        if (opcode) return lhs->binaryOp(*opcode, *rhs);
      }
      if (auto constant = dyn_cast_or_null<BitsConstant4Op>(operation))
        if (failed(constant.verifyInvariantsImpl()) || failed(constant.verify()) || failed(validate(constant, node.context))) return failure();
      return Range::getFull(width);
    };
    auto result = compute();
    if (failed(result)) return failure();
    auto &entry = memo.find(key)->second;
    entry.complete = true; entry.range = *result;
    return *result;
  };
  auto root = context(bindings); if (failed(root)) return failure();
  auto result = infer(Node{index, *root}, 0);
  if (failed(result)) return failure();
  if (result->isEmptySet()) return site->emitOpError() << "checked Table index has no admitted unsigned value";
  auto maximum = result->getUnsignedMax();
  if (maximum.getActiveBits() <= 64 && maximum.getZExtValue() < *extent) return success();
  return site->emitOpError() << "checked Table index complete unsigned range is not proven within bound extent";
}

LogicalResult HardwareAnalysis::verifyResolvedOperation(
    Operation *op, const HardwareBindings &bindings) const {
  if (isa<QueueOp>(op))
    return op->emitOpError() << "ac.queue is retired; use a canonical FIFO instance";
  if (auto read = dyn_cast<TableReadOp>(op)) {
    if (op->getNumOperands() != 2 || op->getNumResults() != 1 || op->getNumRegions() ||
        failed(read.verifyInvariantsImpl()))
      return op->emitOpError() << "checked Table read has malformed operands or results";
  }
  if (auto instance = dyn_cast<InstanceOp>(op); isQueueInstance(instance))
    return verifyQueueOperation(instance, bindings, true);
  if (isa<EnumCreateOp, EnumToBitsOp, EnumFromBitsOp>(op))
    return verifyEnumValueOperation(op, bindings, true);
  if (isa<CollectionOp, TableCreateOp, TableSplatOp, TableMapOp, TableViewOp,
          TableIndexOp, TableReadOp, TableGetOp, TableMatchOp, TableChooseOp, TableFoldOp,
          ValueMergeOp>(op) &&
      failed(verifyCollectionOperation(op, bindings, true)))
    return failure();
  if (auto write = dyn_cast<SourceStateWriteOp>(op); write && !write.getIndices().empty()) {
    auto owner = write->getParentOfType<ModuleOp>();
    SourceStateOp state;
    for (auto candidate : owner.getBody().front().getOps<SourceStateOp>())
      if (candidate.getInstanceNameAttr() == write.getStateAttr()) state = candidate;
    auto table = state ? dyn_cast<TableType>(state.getQ().getType()) : TableType();
    if (table) {
      if (write.getIndices().size() != 1 ||
          failed(verifySourceIndex(write.getIndices().front(), table, bindings, write)))
        return failure();
    }
  }
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
  if (auto observe = dyn_cast<SourceObserveOp>(op))
    if (observe.getKind() == "report" && inputs.size() == 2 && inputs[1] > 64)
      return op->emitOpError() << "numeric report requires at most 64 bits";
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
  if (auto constant = dyn_cast<BitsConstant4Op>(op))
    if (constant.getPattern().size() != outputs[0])
      return op->emitOpError()
             << "four-state pattern does not match bound width";
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
  if (isa<ValueRequireOp>(op) &&
      (inputs[1] != 1 || inputs[2] != 1 || outputs[0] != 1))
    return op->emitOpError()
           << "bound required availability and path require width-one result";
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
bool isMemoryInstance(InstanceOp instance) {
  if (!instance) return false;
  auto callee = instance.getCallee();
  return callee == "pycircuit.__builtins__.sync_mem" ||
         callee == "pycircuit.__builtins__.sync_mem_dp" ||
         callee == "pycircuit.__builtins__.byte_mem";
}

FailureOr<MemoryPrimitiveLayout>
getMemoryPrimitiveLayout(InstanceOp instance,
                         const HardwareBindings &parentBindings) {
  if (!isMemoryInstance(instance))
    return instance.emitOpError() << "Memory requires an actual canonical primitive allocation";
  auto package = instance->getParentOfType<mlir::ModuleOp>();
  HardwareAnalysis analysis(package);
  auto bound = analysis.bindInstance(instance, parentBindings);
  if (failed(bound)) return failure();
  auto primitive = dyn_cast_or_null<ModuleImportOp>(bound->owner);
  StringRef kind = analysis.getPrimitiveKind(primitive);
  if (!primitive || (kind != "sync_mem" && kind != "sync_mem_dp" && kind != "byte_mem") ||
      failed(analysis.verifyImport(primitive)))
    return instance.emitOpError() << "Memory requires its checked canonical primitive declaration";
  MemoryPrimitiveLayout result;
  result.allocation = instance;
  result.primitive = primitive;
  result.kind = kind;
  result.bindings = *bound;
  result.readPorts = kind == "sync_mem_dp" ? 2 : 1;
  result.readAddressOperands = kind == "byte_mem" ? SmallVector<unsigned, 2>{2}
                              : kind == "sync_mem_dp" ? SmallVector<unsigned, 2>{3, 5}
                                                       : SmallVector<unsigned, 2>{3};
  if (kind != "byte_mem")
    result.readEnableOperands = kind == "sync_mem_dp" ? SmallVector<unsigned, 2>{2, 4}
                                                       : SmallVector<unsigned, 2>{2};
  result.writeEnableOperand = kind == "byte_mem" ? 3 : kind == "sync_mem_dp" ? 6 : 4;
  result.writeAddressOperand = result.writeEnableOperand + 1;
  result.writeDataOperand = result.writeEnableOperand + 2;
  result.writeMaskOperand = result.writeEnableOperand + 3;
  unsigned outputs = kind == "byte_mem" ? 1 : 2 * result.readPorts;
  if (instance.getNumOperands() != result.writeMaskOperand + 1 ||
      instance.getNumResults() != outputs)
    return instance.emitOpError() << "Memory pin layout differs from canonical signature";
  auto resolve = [&](Type type) { return analysis.resolveType(type, parentBindings, instance); };
  auto payload = resolve(instance.getResult(0).getType());
  auto address = resolve(instance.getOperand(result.readAddressOperands[0]).getType());
  auto mask = resolve(instance.getOperand(result.writeMaskOperand).getType());
  auto bit = resolve(instance.getOperand(0).getType());
  if (failed(payload) || failed(address) || failed(mask) || failed(bit)) return failure();
  result.payloadType = *payload;
  result.addressType = *address;
  result.maskType = *mask;
  result.predicateType = *bit;
  return result;
}
} // namespace acir::ac
