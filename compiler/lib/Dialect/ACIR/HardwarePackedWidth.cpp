// The shared packed-width core (r8 review): the static-expression
// substitution/evaluable/evaluation machinery, the hardware type resolution,
// the table shape grammar and the packed-layout walk moved verbatim out of
// HardwareAnalysis.cpp and CollectionAnalysis.cpp behind the
// PackedWidthResolver interface. HardwareAnalysis delegates its binding-aware
// members here, and the transactional closed-payload verifier (a dialect TU)
// runs the same core under an empty binding context — one semantic authority
// for nominal widths, recursion cycle detection and the overflow texts.
#include "pycircuit/Dialect/ACIR/HardwarePackedWidth.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "Dialect/ACIR/SourceValueSemantics.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/ScopeExit.h"
#include <algorithm>
#include <limits>
#include <thread>

using namespace mlir;

namespace acir::ac {
namespace detail {
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
               const PackedWidthResolver &resolver);
bool evaluableWidth(Type type, const HardwareBindings &bindings,
                    SmallVector<Attribute> &active,
                    const PackedWidthResolver &resolver) {
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
           evaluableWidth(found->second, bindings, active, resolver);
  }
  if (auto bits = dyn_cast<BitsType>(type))
    return evaluable(substitute(bits.getWidth(), bindings), bindings, active,
                     resolver);
  if (auto table = dyn_cast<TableType>(type))
    return evaluable(substitute(table.getShape(), bindings), bindings, active,
                     resolver) &&
           evaluableWidth(table.getElementType(), bindings, active, resolver);
  return isa<StructType, EnumType>(type);
}
bool evaluable(Attribute attr, const HardwareBindings &bindings,
               SmallVector<Attribute> &active,
               const PackedWidthResolver &resolver) {
  if (llvm::is_contained(active, attr))
    return true; // Invalid closed expressions must not look merely unbound.
  active.push_back(attr);
  llvm::scope_exit cleanup([&] { active.pop_back(); });
  if (auto expression = dyn_cast_or_null<StaticExprAttr>(attr))
    return evaluable(expression.getTree(), bindings, active, resolver);
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
                       resolver);
    }
    if (kind && kind.getValue() == "type_width") {
      auto attribute = dictionary.getAs<TypeAttr>("type");
      if (!attribute)
        return false;
      return evaluableWidth(attribute.getValue(), bindings, active, resolver);
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
    if (select || demand != detail::ValueEvaluation::Strict) {
      auto selector = dictionary.getAs<StaticExprAttr>(select ? "condition" : "lhs");
      if (!selector || !evaluable(selector, bindings, active, resolver))
        return false;
      Operation *site = bindings.owner ? bindings.owner : resolver.getPackage();
      if (!site)
        return false;
      FailureOr<Attribute> value = failure();
      {
        const auto probingThread = std::this_thread::get_id();
        ScopedDiagnosticHandler quiet(site->getContext(), [&](Diagnostic &) {
          return success(std::this_thread::get_id() == probingThread);
        });
        value = resolver.evaluateStatic(selector, bindings, site);
      }
      if (failed(value))
        return true;
      auto boolean = dyn_cast<BoolAttr>(*value);
      if (!boolean)
        return true;
      if (select)
        return evaluable(dictionary.getAs<StaticExprAttr>(
                             boolean.getValue() ? "yes" : "no"),
                         bindings, active, resolver);
      if ((demand == detail::ValueEvaluation::AndShortCircuit && !boolean.getValue()) ||
          (demand == detail::ValueEvaluation::OrShortCircuit && boolean.getValue()))
        return true;
      return evaluable(dictionary.getAs<StaticExprAttr>("rhs"), bindings, active,
                       resolver);
    }
    for (auto field : dictionary)
      if (!evaluable(field.getValue(), bindings, active, resolver))
        return false;
  }
  if (auto array = dyn_cast_or_null<ArrayAttr>(attr))
    for (auto value : array)
      if (!evaluable(value, bindings, active, resolver))
        return false;
  return true;
}

// Tables occupy one compact leaf. Their element traversal checks finite width
// with the same active stack as surrounding records, without expanding lanes.
LogicalResult visitPackedLayout(const PackedWidthResolver &resolver, Type type,
                                const HardwareBindings &bindings,
                                Operation *site, llvm::DenseSet<Type> &active,
                                SmallVector<PackedLeaf> *leaves, FieldPath path,
                                uint64_t &low) {
  auto resolved = resolver.resolveType(type, bindings, site);
  if (failed(resolved))
    return failure();
  type = *resolved;
  if (!active.insert(type).second)
    return site->emitOpError() << "recursive packed struct layout";
  llvm::scope_exit cleanup([&] { active.erase(type); });

  uint64_t width;
  if (auto bits = dyn_cast<BitsType>(type)) {
    auto value = resolver.evaluateStatic(bits.getWidth(), bindings, site);
    if (failed(value))
      return failure();
    auto naturalWidth = naturalHardwareStatic(*value, site, true);
    if (failed(naturalWidth))
      return failure();
    width = *naturalWidth;
  } else if (auto enumeration = dyn_cast<EnumType>(type)) {
    auto definition = resolver.resolveEnum(enumeration, site);
    if (failed(definition))
      return failure();
    width = definition->width;
  } else if (auto table = dyn_cast<TableType>(type)) {
    auto count = hardwareTableSize(resolver, table, bindings, site);
    if (failed(count))
      return failure();
    uint64_t elementWidth = 0;
    if (failed(visitPackedLayout(resolver, table.getElementType(), bindings,
                               site, active, nullptr, {}, elementWidth)))
      return failure();
    if (elementWidth > std::numeric_limits<uint64_t>::max() / *count)
      return site->emitOpError() << "table packed width overflow";
    width = *count * elementWidth;
  } else if (auto record = dyn_cast<StructType>(type)) {
    auto declaration = resolver.lookupStruct(record);
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
      if (failed(visitPackedLayout(resolver,
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

Attribute substituteHardwareAttribute(Attribute attribute,
                                      const HardwareBindings &bindings) {
  return substitute(attribute, bindings);
}
FailureOr<uint64_t> naturalHardwareStatic(Attribute attribute,
                                          Operation *site, bool positive) {
  auto integer = dyn_cast_or_null<MathIntAttr>(attribute);
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
bool hardwareStaticEvaluable(Attribute attribute,
                             const HardwareBindings &bindings,
                             const PackedWidthResolver &resolver) {
  SmallVector<Attribute> active;
  return evaluable(attribute, bindings, active, resolver);
}
FailureOr<Attribute>
evaluateHardwareStatic(const PackedWidthResolver &resolver,
                       StaticExprAttr expression,
                       const HardwareBindings &bindings, Operation *site) {
  expression = cast<StaticExprAttr>(substitute(expression, bindings));
  auto key = std::make_pair(Attribute(expression), bindings.owner);
  auto &active = resolver.activeStatic();
  if (llvm::is_contained(active, key))
    return site->emitOpError() << "cyclic demanded hardware static expression";
  active.push_back(key);
  llvm::scope_exit cleanup([&] { active.pop_back(); });
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
    return evaluateHardwareStatic(resolver, expression, bindings, site);
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
    auto width = resolver.getPackedWidth(type.getValue(), bindings, site);
    if (failed(width))
      return failure();
    return Attribute(MathIntAttr::get(
        site->getContext(), llvm::APSInt(llvm::APInt(65, *width), false)));
  }
  if (tag == "select") {
    auto condition =
        evaluateHardwareStatic(resolver, tree.getAs<StaticExprAttr>("condition"),
                               bindings, site);
    if (failed(condition))
      return failure();
    auto boolean = dyn_cast<BoolAttr>(*condition);
    if (!boolean)
      return site->emitOpError() << "static select requires bool condition";
    return evaluateHardwareStatic(
        resolver, tree.getAs<StaticExprAttr>(boolean.getValue() ? "yes" : "no"),
        bindings, site);
  }
  if (tag != "unary" && tag != "binary")
    return site->emitOpError() << "unresolved hardware static expression";
  auto opcode =
      detail::parseValueOpcode(tree.getAs<StringAttr>("operator").getValue());
  if (!opcode)
    return site->emitOpError() << "unknown static operator";
  auto info = detail::getValueOpcodeInfo(*opcode);
  auto lhs = evaluateHardwareStatic(
      resolver, tree.getAs<StaticExprAttr>(tag == "unary" ? "operand" : "lhs"),
      bindings, site);
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
    auto rhs = evaluateHardwareStatic(
        resolver, tree.getAs<StaticExprAttr>("rhs"), bindings, site);
    if (failed(rhs))
      return failure();
    operands.push_back(*rhs);
  }
  return detail::evaluateValue(*opcode, operands,
                               [&] { return site->emitOpError(); });
}
FailureOr<EnumDefinitionView>
resolveNominalEnum(EnumType type, Operation *site,
                   llvm::function_ref<Operation *(StringRef)> lookupDefinition) {
  auto declaration =
      dyn_cast_or_null<EnumOp>(lookupDefinition(type.getName().getValue()));
  if (!declaration)
    return site->emitOpError() << "cannot resolve nominal enum declaration '"
                               << type.getName().getValue() << "'";
  return validateEnumDeclaration(declaration, site);
}
FailureOr<Type> resolveHardwareType(const PackedWidthResolver &resolver,
                                    Type type, const HardwareBindings &bindings,
                                    Operation *site) {
  if (auto table = dyn_cast<TableType>(type)) {
    SmallVector<Attribute> shape;
    for (auto raw : table.getShape()) {
      auto expr = dyn_cast<StaticExprAttr>(raw);
      if (!expr)
        return site->emitOpError() << "table extent requires StaticExpr";
      expr = cast<StaticExprAttr>(substitute(expr, bindings));
      if (hardwareStaticEvaluable(expr, bindings, resolver)) {
        auto value = resolver.evaluateStatic(expr, bindings, site);
        if (failed(value) || failed(naturalHardwareStatic(*value, site, true)))
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
    auto element = resolveHardwareType(resolver, table.getElementType(),
                                       bindings, site);
    if (failed(element))
      return failure();
    return Type(TableType::get(
        type.getContext(), ArrayAttr::get(type.getContext(), shape), *element));
  }
  if (isa<TypeParamType>(type)) {
    SmallVector<Type> aliases;
    while (auto formal = dyn_cast<TypeParamType>(type)) {
      auto owner = resolver.lookupDefinition(formal.getOwner().getValue());
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
      if (failed(resolver.resolveEnum(enumeration, site)))
        return failure();
    return type;
  }
  if (auto bits = dyn_cast<BitsType>(type)) {
    auto width = cast<StaticExprAttr>(substitute(bits.getWidth(), bindings));
    if (hardwareStaticEvaluable(width, bindings, resolver)) {
      auto value = resolver.evaluateStatic(width, bindings, site);
      if (failed(value) || failed(naturalHardwareStatic(*value, site, true)))
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
    if (!resolver.lookupStruct(record))
      return site->emitOpError() << "unresolved nominal hardware struct";
    return type;
  }
  if (auto enumeration = dyn_cast<EnumType>(type)) {
    if (failed(resolver.resolveEnum(enumeration, site)))
      return failure();
    return type;
  }
  return site->emitOpError()
         << "expected bits, packed struct, enum or declared type parameter";
}
FailureOr<SmallVector<uint64_t>>
resolveHardwareShape(const PackedWidthResolver &resolver, ArrayAttr shape,
                     const HardwareBindings &bindings, Operation *site) {
  if (failed(detail::shapeSchema(shape, [&] { return site->emitOpError(); })))
    return failure();
  SmallVector<uint64_t> result;
  uint64_t n = 1;
  for (auto raw : shape) {
    auto value = resolver.evaluateStatic(cast<StaticExprAttr>(raw), bindings, site);
    if (failed(value))
      return failure();
    auto integer = dyn_cast<MathIntAttr>(*value);
    if (!integer)
      return site->emitOpError() << "table extent requires integer";
    llvm::APSInt v(integer.getCanonicalValue());
    if (v.isNegative() || v.isZero() || v.getActiveBits() > 63)
      return site->emitOpError()
             << "table extent must be positive and fit signed 64 bits";
    uint64_t extent = v.getZExtValue();
    if (n > uint64_t(std::numeric_limits<int64_t>::max()) / extent)
      return site->emitOpError()
             << "table element count exceeds signed 64 bits";
    n *= extent;
    result.push_back(extent);
  }
  return result;
}
uint64_t shapeProduct(ArrayRef<uint64_t> shape) {
  uint64_t n = 1;
  for (auto x : shape)
    n *= x;
  return n;
}
FailureOr<SmallVector<uint64_t>>
hardwareTableShape(const PackedWidthResolver &resolver, TableType table,
                   const HardwareBindings &bindings, Operation *site) {
  auto resolved = resolveHardwareType(resolver, table, bindings, site);
  if (failed(resolved))
    return failure();
  return resolveHardwareShape(resolver, cast<TableType>(*resolved).getShape(),
                              bindings, site);
}
FailureOr<uint64_t> hardwareTableSize(const PackedWidthResolver &resolver,
                                      TableType table,
                                      const HardwareBindings &bindings,
                                      Operation *site) {
  auto shape = hardwareTableShape(resolver, table, bindings, site);
  if (failed(shape))
    return failure();
  return shapeProduct(*shape);
}
FailureOr<SmallVector<PackedLeaf>>
collectPackedLeaves(const PackedWidthResolver &resolver, Type type,
                    const HardwareBindings &bindings, Operation *site) {
  auto rootType = resolveHardwareType(resolver, type, bindings, site);
  if (failed(rootType))
    return failure();
  SmallVector<PackedLeaf> leaves;
  llvm::DenseSet<Type> active;
  uint64_t width = 0;
  if (failed(visitPackedLayout(resolver, *rootType, bindings, site, active,
                               &leaves, {}, width)))
    return failure();
  std::reverse(leaves.begin(), leaves.end());
  return leaves;
}
FailureOr<uint64_t> sumPackedLeaves(Operation *site,
                                    ArrayRef<PackedLeaf> leaves) {
  uint64_t width = 0;
  for (const auto &leaf : leaves) {
    if (width > std::numeric_limits<uint64_t>::max() - leaf.width)
      return site->emitOpError() << "packed width overflow";
    width += leaf.width;
  }
  return width;
}
FailureOr<uint64_t> packedHardwareWidth(const PackedWidthResolver &resolver,
                                        Type type,
                                        const HardwareBindings &bindings,
                                        Operation *site) {
  auto leaves = collectPackedLeaves(resolver, type, bindings, site);
  if (failed(leaves))
    return failure();
  return sumPackedLeaves(site, *leaves);
}
} // namespace detail
} // namespace acir::ac
