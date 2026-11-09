// Collection/table member resolution and verification (B2 step-2 design
// r9): the HardwareAnalysis members and their helper cluster. The staged
// entries — the source-unit semantic walk in its symbolic-tolerant mode and
// HardwareAnalysis::verifyResolvedOperation at the binding tiers — run these
// members after the IX reroute severed the native staged calls
// (CollectionLocalVerifies.cpp keeps only local grammar).
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "AnalysisResolver.h"
#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"

using namespace mlir;

namespace acir::ac {
namespace {

Attribute semantic(Attribute attr) {
  if (auto expr = dyn_cast_or_null<StaticExprAttr>(attr))
    return semantic(expr.getTree());
  if (auto dict = dyn_cast_or_null<DictionaryAttr>(attr)) {
    NamedAttrList fields;
    for (auto field : dict)
      if (field.getName() != "origin" && field.getName() != "location")
        fields.append(field.getName(), semantic(field.getValue()));
    return fields.getDictionary(attr.getContext());
  }
  if (auto array = dyn_cast_or_null<ArrayAttr>(attr)) {
    SmallVector<Attribute> out;
    for (auto item : array)
      out.push_back(semantic(item));
    return ArrayAttr::get(attr.getContext(), out);
  }
  return attr;
}
bool equivalentTypes(TypeRange lhs, TypeRange rhs) {
  if (lhs.size() != rhs.size())
    return false;
  for (auto [left, right] : llvm::zip(lhs, rhs))
    if (!areEquivalentHardwareTypes(left, right))
      return false;
  return true;
}
uint64_t indexWidth(uint64_t n) {
  return std::max<uint64_t>(1, llvm::APInt(64, n - 1).getActiveBits());
}
uint64_t countWidth(uint64_t n) {
  return std::max<uint64_t>(1, llvm::APInt(64, n).getActiveBits());
}
HardwareBindings scope(Operation *op) {
  HardwareBindings b;
  b.owner = op->getParentOfType<ModuleOp>();
  return b;
}
LogicalResult scalarRegion(Operation *op, TypeRange args, TypeRange results) {
  auto &region = op->getRegion(0);
  if (!region.hasOneBlock() || region.front().empty() ||
      !equivalentTypes(region.front().getArgumentTypes(), args) ||
      !isa<YieldOp>(region.front().back()))
    return op->emitOpError() << "requires isolated single block with matching "
                                "scalar arguments and ac.yield";
  if (!equivalentTypes(region.front().back().getOperandTypes(), results))
    return op->emitOpError()
           << "scalar region yield types do not match results";
  for (auto &nested : region.front().without_terminator())
    if (!isa<BitsConstantOp, BitsConstant4Op, BitsUnaryOp, BitsBinaryOp,
             BitsCompareOp, BitsSelectOp, BitsConcatOp, BitsExtractOp,
             BitsResizeOp, StructCreateOp, StructGetOp, EnumCreateOp,
             EnumToBitsOp, EnumFromBitsOp, ValueMergeOp>(nested) &&
        !(isa<TableMapOp>(op) &&
          isa<TableCreateOp, TableSplatOp, TableReadOp, TableGetOp, TableMapOp>(nested)))
      return nested.emitOpError()
             << "table region accepts only closed pure scalar operations";
  return success();
}
FailureOr<Type> fieldType(const HardwareAnalysis &a, Type type, ArrayAttr path,
                          const HardwareBindings &b, Operation *site) {
  auto resolved = a.resolveType(type, b, site);
  if (failed(resolved))
    return failure();
  type = *resolved;
  for (auto raw : path) {
    auto name = dyn_cast<StringAttr>(raw);
    auto record = dyn_cast<StructType>(type);
    if (!name || name.getValue().empty() || !record)
      return site->emitOpError()
             << "merge path requires existing struct fields";
    auto decl = a.lookupStruct(record);
    bool found = false;
    for (auto rawField : decl.getFields()) {
      auto field = cast<DictionaryAttr>(rawField);
      if (field.getAs<StringAttr>("name") == name) {
        type = field.getAs<TypeAttr>("type").getValue();
        found = true;
        break;
      }
    }
    if (!found)
      return site->emitOpError() << "merge path names unknown field";
  }
  return a.resolveType(type, b, site);
}
bool prefix(ArrayAttr a, ArrayAttr b) {
  return a.size() <= b.size() && std::equal(a.begin(), a.end(), b.begin());
}
FailureOr<llvm::APSInt> integerStatic(const HardwareAnalysis &a, Attribute raw,
                                      const HardwareBindings &b,
                                      Operation *site) {
  auto expr = dyn_cast_or_null<StaticExprAttr>(raw);
  if (!expr)
    return site->emitOpError() << "view parameter requires StaticExpr";
  auto value = a.evaluateStatic(expr, b, site);
  if (failed(value))
    return failure();
  auto integer = dyn_cast<MathIntAttr>(*value);
  if (!integer)
    return site->emitOpError() << "view parameter requires integer";
  return llvm::APSInt(integer.getCanonicalValue());
}
FailureOr<int64_t> signedStatic(const HardwareAnalysis &a, Attribute raw,
                                const HardwareBindings &b, Operation *site) {
  auto n = integerStatic(a, raw, b, site);
  if (failed(n))
    return failure();
  if (!n->isSignedIntN(64))
    return site->emitOpError() << "view parameter exceeds signed 64 bits";
  return n->getExtValue();
}
unsigned signedIntegerWidth(const llvm::APSInt &value) {
  // APInt signed remainder needs a zero sign bit for nonnegative APSInt
  // values whose highest stored bit is one.
  return value.getBitWidth() + unsigned(value.isUnsigned());
}
llvm::APInt extendInteger(const llvm::APSInt &value, unsigned bits) {
  return value.isUnsigned() ? value.zextOrTrunc(bits) : value.sextOrTrunc(bits);
}
FailureOr<uint64_t> rotateOffset(const HardwareAnalysis &a, Attribute raw,
                                 uint64_t extent, const HardwareBindings &b,
                                 Operation *site) {
  auto n = integerStatic(a, raw, b, site);
  if (failed(n))
    return failure();
  unsigned bits = std::max<unsigned>(65, signedIntegerWidth(*n));
  llvm::APInt modulus(bits, extent);
  auto remainder = extendInteger(*n, bits).srem(modulus);
  if (remainder.isNegative())
    remainder += modulus;
  return remainder.getZExtValue();
}
FailureOr<SmallVector<llvm::APSInt>> exactVector(const HardwareAnalysis &a,
                                                 Attribute raw,
                                                 const HardwareBindings &b,
                                                 Operation *site) {
  auto array = dyn_cast_or_null<ArrayAttr>(raw);
  if (!array)
    return site->emitOpError() << "view parameter requires StaticExpr array";
  SmallVector<llvm::APSInt> values;
  for (auto x : array) {
    auto n = integerStatic(a, x, b, site);
    if (failed(n))
      return failure();
    values.push_back(*n);
  }
  return values;
}
FailureOr<SmallVector<int64_t>> vectorStatic(const HardwareAnalysis &a,
                                             Attribute raw,
                                             const HardwareBindings &b,
                                             Operation *site) {
  auto arr = dyn_cast_or_null<ArrayAttr>(raw);
  if (!arr)
    return site->emitOpError() << "view parameter requires StaticExpr array";
  SmallVector<int64_t> values;
  for (auto x : arr) {
    auto n = signedStatic(a, x, b, site);
    if (failed(n))
      return failure();
    values.push_back(*n);
  }
  return values;
}
LogicalResult viewSchema(StringRef kind, DictionaryAttr p, unsigned rank,
                         Operation *site) {
  SmallVector<StringRef> names;
  if (kind == "reshape")
    names = {"shape"};
  else if (kind == "slice")
    names = {"offsets", "sizes", "strides"};
  else if (kind == "transpose")
    names = {"axes"};
  else if (kind == "rotate")
    names = {"axis", "offset"};
  else
    return site->emitOpError() << "unknown table view kind";
  if (p.size() != names.size())
    return site->emitOpError() << "view parameter inventory mismatch";
  for (auto name : names) {
    auto raw = p.get(name);
    if (!raw)
      return site->emitOpError() << "missing table view parameter " << name;
    if (kind == "rotate") {
      auto expr = dyn_cast<StaticExprAttr>(raw);
      if (!expr || failed(StaticExprAttr::verify(
                       [&] { return site->emitOpError(); }, expr.getTree())))
        return site->emitOpError() << "view parameter requires StaticExpr";
    } else {
      auto arr = dyn_cast<ArrayAttr>(raw);
      if (!arr || (kind != "reshape" && arr.size() != rank) || arr.empty())
        return site->emitOpError() << "view parameter rank mismatch";
      for (auto x : arr) {
        auto expr = dyn_cast<StaticExprAttr>(x);
        if (!expr || failed(StaticExprAttr::verify(
                         [&] { return site->emitOpError(); }, expr.getTree())))
          return site->emitOpError() << "view parameter requires StaticExpr";
      }
    }
  }
  return success();
}
FailureOr<SmallVector<uint64_t>>
viewShape(const HardwareAnalysis &a, StringRef kind, DictionaryAttr p,
          ArrayRef<uint64_t> in, const HardwareBindings &b, Operation *site) {
  SmallVector<uint64_t> out(in);
  if (kind == "reshape") {
    auto shape = a.resolveShape(cast<ArrayAttr>(p.get("shape")), b, site);
    if (failed(shape))
      return failure();
    if (detail::shapeProduct(*shape) != detail::shapeProduct(in))
      return site->emitOpError() << "reshape must preserve element count";
    return *shape;
  }
  if (kind == "slice") {
    auto offsets = exactVector(a, p.get("offsets"), b, site);
    auto sizes = exactVector(a, p.get("sizes"), b, site);
    auto strides = exactVector(a, p.get("strides"), b, site);
    if (failed(offsets) || failed(sizes) || failed(strides))
      return failure();
    for (unsigned i = 0; i < in.size(); ++i) {
      auto &offset = (*offsets)[i], &size = (*sizes)[i],
           &stride = (*strides)[i];
      if (offset.isNegative() || size.isNegative() || size.isZero() ||
          stride.isNegative() || stride.isZero())
        return site->emitOpError() << "slice requires nonnegative offsets and "
                                      "positive sizes/strides";
      unsigned bits = std::max({offset.getBitWidth(), size.getBitWidth(),
                                stride.getBitWidth(), 65u}) *
                          2 +
                      1;
      auto last = extendInteger(offset, bits) +
                  (extendInteger(size, bits) - llvm::APInt(bits, 1)) *
                      extendInteger(stride, bits);
      if (last.uge(llvm::APInt(bits, in[i])))
        return site->emitOpError() << "slice exceeds table extent";
      out[i] = size.getZExtValue();
    }
  } else if (kind == "transpose") {
    auto axes = vectorStatic(a, p.get("axes"), b, site);
    if (failed(axes))
      return failure();
    SmallVector<bool> seen(in.size(), false);
    for (unsigned i = 0; i < in.size(); ++i) {
      auto axis = (*axes)[i];
      if (axis < 0 || uint64_t(axis) >= in.size() || seen[axis])
        return site->emitOpError()
               << "transpose axes must be a rank permutation";
      seen[axis] = true;
      out[i] = in[axis];
    }
  } else if (kind == "rotate") {
    auto axis = signedStatic(a, p.get("axis"), b, site);
    if (failed(axis))
      return failure();
    if (*axis < 0 || uint64_t(*axis) >= in.size())
      return site->emitOpError() << "rotate axis out of range";
    if (failed(rotateOffset(a, p.get("offset"), in[*axis], b, site)))
      return failure();
  }
  return out;
}


} // namespace



FailureOr<SmallVector<uint64_t>> HardwareAnalysis::resolveShape(
    ArrayAttr shape, const HardwareBindings &bindings, Operation *site) const {
  return detail::resolveHardwareShape(AnalysisResolver(*this), shape, bindings,
                                      site);
}
FailureOr<SmallVector<uint64_t>> HardwareAnalysis::getTableShape(
    TableType table, const HardwareBindings &bindings, Operation *site) const {
  return detail::hardwareTableShape(AnalysisResolver(*this), table, bindings,
                                    site);
}
FailureOr<uint64_t> HardwareAnalysis::getTableSize(
    TableType table, const HardwareBindings &bindings, Operation *site) const {
  return detail::hardwareTableSize(AnalysisResolver(*this), table, bindings,
                                   site);
}
FailureOr<uint64_t>
HardwareAnalysis::getViewSourceOrdinal(TableViewOp view, uint64_t ordinal,
                                       const HardwareBindings &bindings) const {
  auto in = getTableShape(view.getInput().getType(), bindings, view);
  auto out = getTableShape(view.getResult().getType(), bindings, view);
  if (failed(in) || failed(out))
    return failure();
  if (ordinal >= detail::shapeProduct(*out))
    return view.emitOpError() << "view output ordinal out of range";
  SmallVector<uint64_t> coords(out->size()), source(in->size());
  for (size_t i = out->size(); i-- > 0;) {
    coords[i] = ordinal % (*out)[i];
    ordinal /= (*out)[i];
  }
  auto p = view.getParameters();
  auto kind = view.getKind();
  if (kind == "reshape") {
    uint64_t n = 0;
    for (unsigned i = 0; i < out->size(); ++i)
      n = n * (*out)[i] + coords[i];
    return n;
  }
  source = coords;
  if (kind == "slice") {
    auto offsets = exactVector(*this, p.get("offsets"), bindings, view);
    auto strides = exactVector(*this, p.get("strides"), bindings, view);
    if (failed(offsets) || failed(strides))
      return failure();
    for (unsigned i = 0; i < source.size(); ++i) {
      unsigned bits =
          std::max<unsigned>(65, std::max((*offsets)[i].getBitWidth(),
                                          (*strides)[i].getBitWidth()) +
                                     64);
      source[i] =
          (extendInteger((*offsets)[i], bits) +
           llvm::APInt(bits, coords[i]) * extendInteger((*strides)[i], bits))
              .getZExtValue();
    }
  } else if (kind == "transpose") {
    auto axes = vectorStatic(*this, p.get("axes"), bindings, view);
    if (failed(axes))
      return failure();
    for (unsigned i = 0; i < source.size(); ++i)
      source[(*axes)[i]] = coords[i];
  } else if (kind == "rotate") {
    auto axis = signedStatic(*this, p.get("axis"), bindings, view);
    if (failed(axis) || *axis < 0 || uint64_t(*axis) >= in->size())
      return failure();
    uint64_t extent = (*in)[*axis];
    auto shift = rotateOffset(*this, p.get("offset"), extent, bindings, view);
    if (failed(shift))
      return failure();
    source[*axis] = (coords[*axis] + *shift) % extent;
  }
  uint64_t n = 0;
  for (unsigned i = 0; i < in->size(); ++i)
    n = n * (*in)[i] + source[i];
  return n;
}
LogicalResult HardwareAnalysis::normalizeMergeOperands(
    Type baseType, SmallVectorImpl<ArrayAttr> &paths,
    SmallVectorImpl<Value> &guards, SmallVectorImpl<Value> &values,
    const HardwareBindings &bindings, Operation *site) const {
  if (paths.size() != guards.size() || paths.size() != values.size())
    return site->emitOpError()
           << "merge paths, guards and values must have equal lengths";
  auto leaves = getFieldPaths(baseType, bindings, site);
  if (failed(leaves))
    return failure();
  SmallVector<std::pair<size_t, size_t>> order;
  for (auto [i, path] : llvm::enumerate(paths)) {
    if (!path || failed(fieldType(*this, baseType, path, bindings, site)))
      return failure();
    for (size_t j = 0; j < i; ++j)
      if (prefix(path, paths[j]) || prefix(paths[j], path))
        return site->emitOpError() << "merge paths overlap";
    FieldPath names;
    for (auto raw : path)
      names.push_back(cast<StringAttr>(raw));
    size_t position = 0;
    while (
        position < leaves->size() &&
        !(names.size() <= (*leaves)[position].size() &&
          std::equal(names.begin(), names.end(), (*leaves)[position].begin())))
      ++position;
    order.push_back({position, i});
  }
  llvm::sort(order);
  SmallVector<ArrayAttr> sortedPaths;
  SmallVector<Value> sortedGuards, sortedValues;
  for (auto [position, i] : order) {
    sortedPaths.push_back(paths[i]);
    sortedGuards.push_back(guards[i]);
    sortedValues.push_back(values[i]);
  }
  paths.assign(sortedPaths.begin(), sortedPaths.end());
  guards.assign(sortedGuards.begin(), sortedGuards.end());
  values.assign(sortedValues.begin(), sortedValues.end());
  return success();
}
LogicalResult
HardwareAnalysis::verifyCollectionOperation(Operation *op,
                                            const HardwareBindings &bindings,
                                            bool requireResolved) const {
  auto error = [&] { return op->emitOpError(); };
  auto type = [&](Type t) { return resolveType(t, bindings, op); };
  auto equal = [&](Type a, Type b) -> LogicalResult {
    auto x = type(a), y = type(b);
    if (failed(x) || failed(y))
      return failure();
    return areEquivalentHardwareTypes(*x, *y)
               ? success()
               : LogicalResult(error() << "table payload type mismatch");
  };
  auto shape = [&](ArrayAttr s) -> LogicalResult {
    if (failed(detail::shapeSchema(s, error)))
      return failure();
    if (requireResolved || isStaticEvaluable(s, bindings))
      return success(succeeded(resolveShape(s, bindings, op)));
    return success();
  };
  auto sameShape = [&](ArrayAttr a, ArrayAttr b) -> LogicalResult {
    if (failed(shape(a)) || failed(shape(b)))
      return failure();
    if (isStaticEvaluable(a, bindings) && isStaticEvaluable(b, bindings)) {
      auto x = resolveShape(a, bindings, op), y = resolveShape(b, bindings, op);
      if (failed(x) || failed(y))
        return failure();
      if (*x == *y)
        return success();
    } else if (semantic(a) == semantic(b))
      return success();
    return error() << "table shapes must match";
  };
  auto width = [&](BitsType t, uint64_t expected) -> LogicalResult {
    if (!requireResolved && !isStaticEvaluable(t.getWidth(), bindings))
      return success();
    auto w = getPackedWidth(t, bindings, op);
    if (failed(w))
      return failure();
    return *w == expected
               ? success()
               : LogicalResult(error() << "table bit width mismatch: expected "
                                       << expected);
  };
  auto count = [&](TableType t) -> std::optional<uint64_t> {
    if (!requireResolved && !isStaticEvaluable(t.getShape(), bindings))
      return std::nullopt;
    auto n = getTableSize(t, bindings, op);
    if (failed(n))
      return std::nullopt;
    return *n;
  };
  for (Type t : op->getOperandTypes())
    if (auto table = dyn_cast<TableType>(t))
      if (failed(shape(table.getShape())))
        return failure();
  for (Type t : op->getResultTypes())
    if (auto table = dyn_cast<TableType>(t))
      if (failed(shape(table.getShape())))
        return failure();
  if (auto c = dyn_cast<CollectionOp>(op)) {
    if (failed(shape(c.getShape())))
      return failure();
    for (StringRef name : {"constrained", "banks", "bank_count", "one_hot",
                           "at_most_one", "no_reset", "allow_ram_mapping"})
      if (op->hasAttr(name))
        return error() << "unsupported collection layout or hint";
    if (auto layout = c.getLayoutAttr()) {
      ArrayAttr current = c.getShape();
      for (auto raw : layout) {
        auto step = dyn_cast<DictionaryAttr>(raw);
        auto kind = step ? step.getAs<StringAttr>("kind") : StringAttr();
        auto parameters =
            step ? step.getAs<DictionaryAttr>("parameters") : DictionaryAttr();
        if (!step || step.size() != 2 || !kind || !parameters ||
            !llvm::is_contained(
                ArrayRef<StringRef>{"reshape", "transpose", "rotate"},
                kind.getValue()))
          return error() << "physical layout accepts only reshape, transpose "
                            "and rotate steps";
        if (failed(viewSchema(kind.getValue(), parameters, current.size(), op)))
          return failure();
        if (requireResolved || (isStaticEvaluable(current, bindings) &&
                                isStaticEvaluable(parameters, bindings))) {
          auto in = resolveShape(current, bindings, op);
          if (failed(in))
            return failure();
          auto out =
              viewShape(*this, kind.getValue(), parameters, *in, bindings, op);
          if (failed(out))
            return failure();
        }
        // Later steps use the reshaped rank. Transpose keeps rank; dimensions
        // themselves are validated on the concrete layout walk below.
        if (kind.getValue() == "reshape")
          current = cast<ArrayAttr>(parameters.get("shape"));
      }
      if (requireResolved || isStaticEvaluable(layout, bindings)) {
        auto currentShape = resolveShape(c.getShape(), bindings, op);
        if (failed(currentShape))
          return failure();
        for (auto raw : layout) {
          auto step = cast<DictionaryAttr>(raw);
          auto out = viewShape(*this, step.getAs<StringAttr>("kind").getValue(),
                               step.getAs<DictionaryAttr>("parameters"),
                               *currentShape, bindings, op);
          if (failed(out))
            return failure();
          currentShape = *out;
        }
      }
    }
    return verifyInstance(c, bindings);
  }
  if (auto create = dyn_cast<TableCreateOp>(op)) {
    auto t = create.getResult().getType();
    if (auto n = count(t); n && *n != create.getInputs().size())
      return error() << "table.create input count must equal shape product";
    for (Value v : create.getInputs())
      if (failed(equal(v.getType(), t.getElementType())))
        return failure();
  } else if (auto splat = dyn_cast<TableSplatOp>(op)) {
    if (failed(shape(splat.getShape())))
      return failure();
    auto expected = TableType::get(op->getContext(), splat.getShape(),
                                   splat.getInput().getType());
    return equal(expected, splat.getResult().getType());
  } else if (auto map = dyn_cast<TableMapOp>(op)) {
    if (failed(shape(map.getShape())) || map.getResults().empty())
      return error() << "table.map requires shape and at least one result";
    for (Value v : map.getTables())
      if (failed(sameShape(cast<TableType>(v.getType()).getShape(),
                           map.getShape())))
        return failure();
    for (Value v : map.getResults())
      if (failed(sameShape(cast<TableType>(v.getType()).getShape(),
                           map.getShape())))
        return failure();
    if (!map.getBody().empty() &&
        !map.getBody().front().getArguments().empty()) {
      auto ordinal =
          dyn_cast<BitsType>(map.getBody().front().getArgument(0).getType());
      if (!ordinal)
        return error() << "table.map ordinal requires bits";
      if (requireResolved || isStaticEvaluable(map.getShape(), bindings)) {
        auto s = resolveShape(map.getShape(), bindings, op);
        if (failed(s) || failed(width(ordinal, indexWidth(detail::shapeProduct(*s)))))
          return failure();
      }
    }
  } else if (auto view = dyn_cast<TableViewOp>(op)) {
    auto in = view.getInput().getType(), out = view.getResult().getType();
    if (failed(equal(in.getElementType(), out.getElementType())) ||
        failed(viewSchema(view.getKind(), view.getParameters(),
                          in.getShape().size(), op)))
      return failure();
    if (requireResolved ||
        (isStaticEvaluable(in.getShape(), bindings) &&
         isStaticEvaluable(out.getShape(), bindings) &&
         isStaticEvaluable(view.getParameters(), bindings))) {
      auto s = getTableShape(in, bindings, op),
           r = getTableShape(out, bindings, op);
      if (failed(s) || failed(r))
        return failure();
      auto expected = viewShape(*this, view.getKind(), view.getParameters(), *s,
                                bindings, op);
      if (failed(expected))
        return failure();
      if (*expected != *r)
        return error() << "table.view result shape mismatch";
    }
  } else if (auto index = dyn_cast<TableIndexOp>(op)) {
    if (failed(shape(index.getShape())))
      return failure();
    if (index.getCoords().size() != index.getShape().size())
      return error() << "table.index coordinate rank mismatch";
    if (requireResolved || isStaticEvaluable(index.getShape(), bindings)) {
      auto s = resolveShape(index.getShape(), bindings, op);
      if (failed(s) ||
          failed(width(index.getResult().getType(), countWidth(detail::shapeProduct(*s)))))
        return failure();
    }
  } else if (auto read = dyn_cast<TableReadOp>(op)) {
    auto input = read.getInput().getType();
    if (input.getShape().size() != 1 ||
        !isa<BitsType, StructType, EnumType>(input.getElementType()))
      return error() << "checked Table read requires a rank-one Table of finite elements";
    if (failed(equal(input.getElementType(), read.getValue().getType())) ||
        failed(detail::verifySourceTemplateType(*this, input, bindings, op)) ||
        failed(detail::verifySourceTemplateType(*this, read.getIndex().getType(), bindings, op)))
      return failure();
    if (requireResolved ||
        (isStaticEvaluable(input.getShape(), bindings) &&
         isStaticEvaluable(read.getIndex().getType().getWidth(), bindings)))
      return verifySourceIndex(read.getIndex(), input, bindings, op);
  } else if (auto get = dyn_cast<TableGetOp>(op)) {
    if (failed(equal(get.getInput().getType().getElementType(),
                     get.getValue().getType())) ||
        failed(width(get.getInRange().getType(), 1)))
      return failure();
  } else if (auto match = dyn_cast<TableMatchOp>(op)) {
    if (auto n = count(match.getInput().getType())) {
      if (failed(width(match.getMask().getType(), *n)))
        return failure();
      // Same width-one rejection the native verifyRegions used before the
      // r3 residual split, evaluated with the current bindings. Only runs
      // once the native region grammar has established the yield shape.
      auto &body = match.getBody();
      if (!body.empty() && !body.front().empty() &&
          isa<YieldOp>(body.front().back()) &&
          body.front().back().getNumOperands() == 1) {
        auto yielded =
            dyn_cast<BitsType>(body.front().back().getOperand(0).getType());
        if (!yielded)
          return match.emitOpError()
                 << "table.match predicate must yield one bit";
        auto w = getPackedWidth(yielded, bindings, match);
        if (failed(w))
          return failure();
        if (*w != 1)
          return match.emitOpError()
                 << "table.match predicate must yield one bit";
      }
    }
  } else if (auto choose = dyn_cast<TableChooseOp>(op)) {
    if (choose.getPolicy() != "first" ||
        (choose.getOrder() != "low" && choose.getOrder() != "high"))
      return error() << "table.choose accepts first policy and low/high order";
    for (StringRef name : {"key", "cursor", "min", "max", "round_robin"})
      if (op->hasAttr(name))
        return error() << "unsupported table.choose policy parameter";
    int64_t c = choose.getCount();
    if (c <= 0 || uint64_t(c) > std::numeric_limits<unsigned>::max() / 2 ||
        choose.getResults().size() != uint64_t(c) * 2)
      return error()
             << "table.choose requires 2*count results and positive count";
    auto match = choose.getMask().getDefiningOp<TableMatchOp>();
    if (!match || match.getInput() != choose.getInput())
      return error() << "table.choose mask must come from match of the same "
                        "table SSA value";
    if (auto n = count(choose.getInput().getType())) {
      if (uint64_t(c) > *n)
        return error() << "table.choose count exceeds element count";
      if (failed(width(choose.getMask().getType(), *n)))
        return failure();
      for (auto [i, v] : llvm::enumerate(choose.getResults()))
        if (failed(width(cast<BitsType>(v.getType()),
                         i < uint64_t(c) ? indexWidth(*n) : 1)))
          return failure();
    }
  } else if (auto fold = dyn_cast<TableFoldOp>(op)) {
    if (!llvm::is_contained(
            ArrayRef<StringRef>{"add", "mul", "and", "or", "xor", "min", "max"},
            fold.getKind()))
      return error() << "unsupported closed table.fold kind";
    auto element = type(fold.getInput().getType().getElementType());
    if (failed(element))
      return failure();
    if (!isa<BitsType, TypeParamType>(*element))
      return error() << "table.fold requires unsigned bits elements";
    return equal(*element, fold.getResult().getType());
  } else if (auto merge = dyn_cast<ValueMergeOp>(op)) {
    if (merge.getPaths().size() != merge.getGuards().size() ||
        merge.getPaths().size() != merge.getValues().size())
      return error()
             << "merge paths, guards and values must have equal lengths";
    if (failed(equal(merge.getBase().getType(), merge.getNext().getType())) ||
        failed(width(merge.getEn().getType(), 1)))
      return failure();
    if (isa<TableType>(merge.getBase().getType()))
      for (Attribute raw : merge.getPaths()) {
        auto path = dyn_cast<ArrayAttr>(raw);
        if (!path || !path.empty())
          return error() << "whole-Table merge requires empty value paths";
      }
    SmallVector<ArrayAttr> paths;
    for (auto [i, raw] : llvm::enumerate(merge.getPaths())) {
      auto path = dyn_cast<ArrayAttr>(raw);
      if (!path)
        return error() << "merge requires static field paths";
      for (auto other : paths)
        if (prefix(path, other) || prefix(other, path))
          return error() << "merge paths overlap";
      auto target =
          fieldType(*this, merge.getBase().getType(), path, bindings, op);
      if (failed(target) ||
          failed(equal(*target, merge.getValues()[i].getType())) ||
          failed(width(cast<BitsType>(merge.getGuards()[i].getType()), 1)))
        return failure();
      paths.push_back(path);
    }
    auto leaves = getFieldPaths(merge.getBase().getType(), bindings, op);
    if (failed(leaves))
      return failure();
    std::optional<size_t> previous;
    for (auto path : paths) {
      size_t pos = 0;
      for (; pos < leaves->size(); ++pos) {
        FieldPath p;
        for (auto raw : path)
          p.push_back(cast<StringAttr>(raw));
        if (p.size() <= (*leaves)[pos].size() &&
            std::equal(p.begin(), p.end(), (*leaves)[pos].begin()))
          break;
      }
      if (previous && pos <= *previous)
        return error() << "merge paths must follow declaration field order";
      previous = pos;
    }
  }
  return success();
}

} // namespace acir::ac
