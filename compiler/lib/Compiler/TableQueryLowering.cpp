#include "TableQueryLowering.h"
#include "PythonImportContext.h"
#include "mlir/IR/IRMapping.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/Support/MathExtras.h"

using namespace mlir;
namespace acir::compiler::detail {
uint64_t TableQueryBudget::limit(TableQueryResource resource) {
  constexpr uint64_t limits[] = {128,    4096,    1048576, 65536,
                                 262144, 1048576, 1048576};
  return limits[static_cast<unsigned>(resource)];
}
uint64_t TableQueryBudget::used(TableQueryResource resource) const {
  return spent.amounts[static_cast<unsigned>(resource)];
}
bool TableQueryBudget::add(TableQueryCharge &charge,
                           TableQueryResource resource, uint64_t amount) {
  auto &value = charge.amounts[static_cast<unsigned>(resource)];
  if (value > limit(resource) || amount > limit(resource) - value)
    return false;
  value += amount;
  return true;
}
bool TableQueryBudget::addProduct(TableQueryCharge &charge,
                                  TableQueryResource resource, uint64_t a,
                                  uint64_t b) {
  uint64_t existing = charge.amounts[static_cast<unsigned>(resource)];
  if (existing > limit(resource))
    return false;
  uint64_t remaining = limit(resource) - existing;
  if (b && a > remaining / b)
    return false;
  return add(charge, resource, a * b);
}
bool TableQueryBudget::reserve(const TableQueryCharge &charge,
                               std::string &reason) {
  constexpr const char *names[] = {
      "nesting", "occurrences",   "work",          "operations",
      "slots",   "payload words", "constant bytes"};
  for (unsigned i = 0; i < spent.amounts.size(); ++i)
    if (charge.amounts[i] >
        limit(static_cast<TableQueryResource>(i)) - spent.amounts[i]) {
      reason = std::string("Table query ") + names[i] + " budget exhausted";
      return false;
    }
  for (unsigned i = 0; i < spent.amounts.size(); ++i)
    spent.amounts[i] += charge.amounts[i];
  return true;
}
bool TableQueryBudget::reserveProduct(TableQueryResource resource, uint64_t a,
                                      uint64_t b, std::string &reason) {
  TableQueryCharge charge;
  uint64_t remaining = limit(resource) - used(resource);
  if (b && a > remaining / b) {
    reason = "Table query resource product budget exhausted";
    return false;
  }
  charge.amounts[static_cast<unsigned>(resource)] = a * b;
  return reserve(charge, reason);
}
bool TableQueryBudget::enter(std::string &reason) {
  TableQueryCharge charge;
  charge.amounts[static_cast<unsigned>(TableQueryResource::Nesting)] = 1;
  return reserve(charge, reason);
}
void TableQueryBudget::leave() {
  auto &depth =
      spent.amounts[static_cast<unsigned>(TableQueryResource::Nesting)];
  assert(depth && "unbalanced Table query nesting");
  --depth;
}
} // namespace acir::compiler::detail

namespace acir::compiler::detail {
TableQueryLowering::TableQueryLowering(OpBuilder &builder, Location location,
                                       ModuleOp package,
                                       TableQueryBudget &budget,
                                       TableQueryHooks hooks)
    : at(builder), loc(location), analysis(package), package(package),
      budget(budget), hooks(std::move(hooks)) {}
ArrayAttr TableQueryLowering::shape(ArrayRef<uint64_t> sizes) {
  SmallVector<Attribute> values;
  for (uint64_t size : sizes)
    values.push_back(hooks.literal(size));
  return at.getArrayAttr(values);
}
FailureOr<uint64_t> TableQueryLowering::words(Type type, unsigned depth,
                                              bool debitWork) {
  std::string reason;
  if (debitWork &&
      !budget.reserveProduct(TableQueryResource::Work, 1, 1, reason))
    return emitError(loc) << reason;
  if (depth >= TableQueryBudget::limit(TableQueryResource::Nesting))
    return emitError(loc) << "Table query type nesting budget exhausted";
  if (auto table = dyn_cast<ac::TableType>(type)) {
    auto size = analysis.getTableSize(table, {}, package);
    if (failed(size))
      return failure();
    auto element = words(table.getElementType(), depth + 1, debitWork);
    if (failed(element) ||
        (*element &&
         *size > TableQueryBudget::limit(TableQueryResource::PayloadWords) /
                     *element))
      return failure();
    return *size * *element;
  }
  uint64_t weight = 0;
  if (auto record = dyn_cast<ac::StructType>(type)) {
    auto declaration = analysis.lookupStruct(record);
    if (!declaration)
      return failure();
    for (Attribute raw : declaration.getFields()) {
      auto field =
          words(cast<DictionaryAttr>(raw).getAs<TypeAttr>("type").getValue(),
                depth + 1, debitWork);
      if (failed(field) ||
          *field > TableQueryBudget::limit(TableQueryResource::PayloadWords) -
                       weight)
        return failure();
      weight += *field;
    }
  } else {
    auto width = analysis.getPackedWidth(type, {}, package);
    if (failed(width))
      return failure();
    weight = *width / 64 + (*width % 64 != 0);
  }
  if (weight > TableQueryBudget::limit(TableQueryResource::PayloadWords))
    return failure();
  return weight;
}
LogicalResult TableQueryLowering::capture(Value value) {
  auto weight = words(value.getType());
  TableQueryCharge charge;
  bool fits =
      succeeded(weight) &&
      TableQueryBudget::addProduct(charge, TableQueryResource::PayloadWords,
                                   succeeded(weight) ? *weight : 0, 3) &&
      TableQueryBudget::add(charge, TableQueryResource::Slots, 1) &&
      TableQueryBudget::add(charge, TableQueryResource::Work, 1);
  std::string reason;
  if (!fits || !budget.reserve(charge, reason))
    return emitError(loc)
           << (fits ? reason : "Table query capture storage budget exhausted");
  return success();
}
bool TableQueryLowering::describe(Operation *operation, uint64_t rows,
                                  TableQueryCharge &charge, bool debitWork) {
  if (!rows)
    return false;
  bool fits =
      TableQueryBudget::add(charge, TableQueryResource::Operations, 1) &&
      TableQueryBudget::add(charge, TableQueryResource::Slots,
                            operation->getNumOperands() +
                                operation->getNumResults()) &&
      TableQueryBudget::addProduct(charge, TableQueryResource::Work, rows, 1);
  for (Type type : operation->getResultTypes()) {
    auto weight = words(type, 0, debitWork);
    fits &= succeeded(weight) && TableQueryBudget::addProduct(
                                     charge, TableQueryResource::PayloadWords,
                                     succeeded(weight) ? *weight : 0, 3);
    // A Table projected from a scalar Struct lane is a lane-local temporary,
    // so its nested extent is replicated just like every other payload.
    fits &= succeeded(weight) &&
            TableQueryBudget::addProduct(
                charge, TableQueryResource::PayloadWords,
                succeeded(weight) ? *weight : 0, 3 * (rows - 1));
    if (isa<ac::BitsConstantOp>(operation))
      fits &= succeeded(weight) &&
              TableQueryBudget::addProduct(charge,
                                           TableQueryResource::ConstantBytes,
                                           succeeded(weight) ? *weight : 0, 8);
  }
  return fits;
}
bool TableQueryLowering::describeWithoutReservation(Operation *operation,
                                                    uint64_t rows,
                                                    TableQueryCharge &charge) {
  if (!operation || !rows ||
      rows > TableQueryBudget::limit(TableQueryResource::Work) ||
      operation->getNumRegions() || operation->getNumSuccessors() ||
      operation->getNumResults() != 1 || operation->getNumOperands() > 3 ||
      !isa<ac::BitsConstantOp, ac::BitsUnaryOp, ac::BitsBinaryOp,
           ac::BitsCompareOp, ac::BitsSelectOp, ac::BitsExtractOp,
           ac::BitsResizeOp>(operation))
    return false;
  // This descriptor is for the importer's precharged flat scalar template.
  // Do not introduce another aggregate or static-expression type walk here.
  auto fixedLiteral = [](Type type) {
    auto bits = dyn_cast<ac::BitsType>(type);
    auto tree = bits ? bits.getWidth().getTree() : DictionaryAttr();
    auto tag = tree ? tree.getAs<StringAttr>("kind") : StringAttr();
    auto raw = tree ? tree.getAs<DictionaryAttr>("value") : DictionaryAttr();
    auto width = raw ? raw.getAs<ac::MathIntAttr>("value") : ac::MathIntAttr();
    if (!tag || tag.getValue() != "literal" || !width)
      return false;
    StringRef spelling = width.getCanonicalValue();
    uint64_t ceiling =
        64 * TableQueryBudget::limit(TableQueryResource::PayloadWords);
    std::string maximum = std::to_string(ceiling);
    if (spelling.empty() || spelling.front() == '0' ||
        llvm::any_of(spelling, [](char c) { return c < '0' || c > '9'; }) ||
        spelling.size() > maximum.size() ||
        (spelling.size() == maximum.size() && spelling.compare(maximum) > 0))
      return false;
    uint64_t integer = 0;
    return !spelling.getAsInteger(10, integer) && integer && integer <= ceiling;
  };
  if (!llvm::all_of(operation->getOperandTypes(), fixedLiteral) ||
      !llvm::all_of(operation->getResultTypes(), fixedLiteral))
    return false;
  return describe(operation, rows, charge, false);
}
LogicalResult TableQueryLowering::charge(Operation *operation, uint64_t rows) {
  TableQueryCharge charge;
  std::string reason;
  bool fits = describe(operation, rows, charge);
  if (!fits || !budget.reserve(charge, reason))
    return emitError(loc)
           << (fits ? reason : "Table query typed storage budget exhausted");
  return success();
}
Operation *TableQueryLowering::op(OpBuilder &builder, StringRef name,
                                  ValueRange inputs, TypeRange outputs,
                                  ArrayRef<NamedAttribute> attrs,
                                  unsigned regions) {
  // Build a detached description to calculate all result-plane weights before
  // placing it in the source graph. No replicated value allocation happens
  // here.
  OperationState state(loc, name);
  state.addOperands(inputs);
  state.addTypes(outputs);
  state.addAttributes(attrs);
  for (unsigned i = 0; i < regions; ++i)
    state.addRegion();
  auto operation = Operation::create(state);
  if (!scalarTemplate && failed(charge(operation))) {
    failedCharge = true;
    operation->destroy();
    return nullptr;
  }
  builder.insert(operation);
  return operation;
}
Value TableQueryLowering::constant(OpBuilder &builder, uint64_t value,
                                   Type type) {
  auto operation =
      op(builder, ac::BitsConstantOp::getOperationName(), {}, {type},
         {builder.getNamedAttr("value", hooks.literal(value))});
  return operation ? operation->getResult(0) : Value();
}
Value TableQueryLowering::binary(OpBuilder &builder, StringRef code, Value a,
                                 Value b) {
  if (!a || !b)
    return {};
  auto operation =
      op(builder, ac::BitsBinaryOp::getOperationName(), {a, b}, {a.getType()},
         {builder.getNamedAttr("opcode", builder.getStringAttr(code))});
  return operation ? operation->getResult(0) : Value();
}
Value TableQueryLowering::compare(OpBuilder &builder, StringRef code, Value a,
                                  Value b) {
  if (!a || !b)
    return {};
  auto operation = op(
      builder, ac::BitsCompareOp::getOperationName(), {a, b}, {hooks.bits(1)},
      {builder.getNamedAttr("predicate", builder.getStringAttr(code))});
  return operation ? operation->getResult(0) : Value();
}
Value TableQueryLowering::widen(OpBuilder &builder, Value value, Type type) {
  if (!value)
    return {};
  if (ac::areEquivalentHardwareTypes(value.getType(), type))
    return value;
  auto actual = analysis.getPackedWidth(value.getType(), {}, package);
  auto target = analysis.getPackedWidth(type, {}, package);
  if (failed(actual) || failed(target)) {
    failedCharge = true;
    return {};
  }
  if (*actual == *target)
    return hooks.identity(value, type, builder);
  if (*actual > *target) {
    emitError(loc) << "Table query index widening cannot discard high bits";
    failedCharge = true;
    return {};
  }
  auto operation =
      op(builder, ac::BitsResizeOp::getOperationName(), {value}, {type},
         {builder.getNamedAttr("mode", builder.getStringAttr("zext"))});
  return operation ? operation->getResult(0) : Value();
}
FailureOr<Value> TableQueryLowering::fold(Value table, StringRef kind) {
  auto type = cast<ac::TableType>(table.getType());
  auto size = analysis.getTableSize(type, {}, package);
  if (failed(size) || !*size)
    return failure();
  OperationState state(loc, ac::TableFoldOp::getOperationName());
  state.addOperands(table);
  state.addTypes(type.getElementType());
  state.addAttribute("kind", at.getStringAttr(kind));
  auto operation = Operation::create(state);
  TableQueryCharge reservation;
  auto treeWords = words(type);
  // C++ materializes a full N-element V/K/Z tree; RTL has N-1 combine
  // intermediates. The separately described scalar result is not this tree.
  bool fits =
      succeeded(treeWords) &&
      TableQueryBudget::addProduct(reservation,
                                   TableQueryResource::PayloadWords,
                                   succeeded(treeWords) ? *treeWords : 0, 3) &&
      TableQueryBudget::add(reservation, TableQueryResource::Work, *size - 1) &&
      describe(operation, 1, reservation);
  std::string reason;
  if (!fits || !budget.reserve(reservation, reason)) {
    operation->destroy();
    return emitError(loc)
           << (fits ? reason
                    : "Table query fold tree/storage budget exhausted");
  }
  at.insert(operation);
  return operation->getResult(0);
}
FailureOr<Value> TableQueryLowering::count(Value table, Type result) {
  auto type = cast<ac::TableType>(table.getType());
  auto sizes = analysis.getTableShape(type, {}, package);
  if (failed(sizes))
    return failure();
  auto widened =
      map({table}, {}, *sizes, TypeRange{result},
          [&](OpBuilder &inside,
              ValueRange arguments) -> FailureOr<SmallVector<Value>> {
            auto value = widen(inside, arguments[1], result);
            if (!value)
              return failure();
            return SmallVector<Value>{value};
          });
  if (failed(widened))
    return failure();
  return fold(widened->front(), "add");
}
FailureOr<Value> TableQueryLowering::splat(Value input,
                                           ArrayRef<uint64_t> sizes) {
  auto domain = shape(sizes);
  auto operation =
      op(at, ac::TableSplatOp::getOperationName(), {input},
         {ac::TableType::get(at.getContext(), domain, input.getType())},
         {at.getNamedAttr("shape", domain)});
  if (!operation)
    return failure();
  return operation->getResult(0);
}
FailureOr<Value> TableQueryLowering::view(Value input, StringRef kind,
                                          ArrayRef<uint64_t> sizes,
                                          ArrayRef<NamedAttribute> parameters) {
  auto operation =
      op(at, ac::TableViewOp::getOperationName(), {input},
         {ac::TableType::get(
             at.getContext(), shape(sizes),
             cast<ac::TableType>(input.getType()).getElementType())},
         {at.getNamedAttr("kind", at.getStringAttr(kind)),
          at.getNamedAttr("parameters", at.getDictionaryAttr(parameters))});
  if (!operation)
    return failure();
  return operation->getResult(0);
}
FailureOr<SmallVector<Value>>
TableQueryLowering::combine(ValueRange left, ValueRange right, bool argmin) {
  auto type = cast<ac::TableType>(left.front().getType());
  auto sizes = analysis.getTableShape(type, {}, package);
  if (failed(sizes))
    return failure();
  SmallVector<Value> inputs(left);
  llvm::append_range(inputs, right);
  SmallVector<Type> elements;
  for (Value value : left)
    elements.push_back(cast<ac::TableType>(value.getType()).getElementType());
  return map(
      inputs, {}, *sizes, elements,
      [&](OpBuilder &nested, ValueRange args) -> FailureOr<SmallVector<Value>> {
        unsigned count = left.size();
        auto a = args.slice(1, count), c = args.slice(1 + count, count);
        Value valid = binary(nested, "or", a[0], c[0]);
        Value take = a[0];
        if (argmin) {
          auto inverse =
              op(nested, ac::BitsUnaryOp::getOperationName(), {a[0]},
                 {a[0].getType()},
                 {nested.getNamedAttr("opcode", nested.getStringAttr("not"))});
          if (!inverse)
            return failure();
          take = binary(nested, "and", c[0],
                        binary(nested, "or", inverse->getResult(0),
                               compare(nested, "ult", c[1], a[1])));
        }
        if (!valid || !take)
          return failure();
        SmallVector<Value> results{valid};
        for (unsigned i = 1; i < count; ++i) {
          auto value = hooks.select(take, argmin ? c[i] : a[i],
                                    argmin ? a[i] : c[i], nested);
          if (failed(value))
            return failure();
          // The shared nominal selector owns these scalar operations and
          // semantics.
          results.push_back(*value);
        }
        return results;
      });
}
FailureOr<SmallVector<Value>> TableQueryLowering::reduce(ValueRange tables,
                                                         uint64_t outer,
                                                         uint64_t extent,
                                                         bool argmin) {
  SmallVector<SmallVector<Value>> roots;
  uint64_t offset = 0;
  for (uint64_t chunk = uint64_t(1) << llvm::Log2_64(extent); chunk;
       chunk >>= 1) {
    if (!(extent & chunk))
      continue;
    SmallVector<Value> current;
    for (Value table : tables) {
      auto sliced = view(table, "slice", {outer, chunk},
                         {at.getNamedAttr("offsets", shape({0, offset})),
                          at.getNamedAttr("sizes", shape({outer, chunk})),
                          at.getNamedAttr("strides", shape({1, 1}))});
      if (failed(sliced))
        return failure();
      current.push_back(*sliced);
    }
    for (uint64_t width = chunk; width > 1; width >>= 1) {
      SmallVector<Value> left, right;
      for (Value table : current)
        for (unsigned side = 0; side < 2; ++side) {
          auto sliced =
              view(table, "slice", {outer, width / 2},
                   {at.getNamedAttr("offsets", shape({0, side})),
                    at.getNamedAttr("sizes", shape({outer, width / 2})),
                    at.getNamedAttr("strides", shape({1, 2}))});
          if (failed(sliced))
            return failure();
          (side ? right : left).push_back(*sliced);
        }
      auto combined = combine(left, right, argmin);
      if (failed(combined))
        return failure();
      current = std::move(*combined);
    }
    SmallVector<Value> root;
    for (Value table : current) {
      auto reshaped = view(table, "reshape", {outer},
                           {at.getNamedAttr("shape", shape({outer}))});
      if (failed(reshaped))
        return failure();
      root.push_back(*reshaped);
    }
    roots.push_back(std::move(root));
    offset += chunk;
  }
  SmallVector<Value> result = std::move(roots.back());
  for (size_t i = roots.size() - 1; i; --i) {
    auto combined = combine(roots[i - 1], result, argmin);
    if (failed(combined))
      return failure();
    result = std::move(*combined);
  }
  return result;
}
FailureOr<Value> TableQueryLowering::gather(Value table, Value indices) {
  auto sourceType = cast<ac::TableType>(table.getType());
  auto indexType = cast<ac::TableType>(indices.getType());
  auto m = analysis.getTableSize(sourceType, {}, package);
  auto n = analysis.getTableSize(indexType, {}, package);
  auto width = analysis.getPackedWidth(indexType.getElementType(), {}, package);
  if (failed(m) || failed(n) || failed(width))
    return failure();
  std::string reason;
  TableQueryCharge logical;
  auto capturedWords = words(table.getType());
  bool fits =
      succeeded(capturedWords) &&
      TableQueryBudget::addProduct(
          logical, TableQueryResource::PayloadWords,
          succeeded(capturedWords) ? *capturedWords : 0, 3) &&
      TableQueryBudget::addProduct(logical, TableQueryResource::Work, *n, *m) &&
      TableQueryBudget::addProduct(logical, TableQueryResource::Work, *n,
                                   *m - 1);
  if (!fits || !budget.reserve(logical, reason))
    return emitError(loc)
           << (fits ? reason
                    : "Table query Cartesian work/storage budget exhausted");
  auto payloads = splat(table, {*n}), repeated = splat(indices, {*m});
  if (failed(payloads) || failed(repeated))
    return failure();
  auto indexGrid = view(*repeated, "transpose", {*n, *m},
                        {at.getNamedAttr("axes", shape({1, 0}))});
  if (failed(indexGrid))
    return failure();
  auto ordinalType = hooks.bits(std::max(1u, llvm::Log2_64_Ceil(*m)));
  auto ordinals =
      map({}, {}, {*m}, {ordinalType},
          [](OpBuilder &, ValueRange args) -> FailureOr<SmallVector<Value>> {
            return SmallVector<Value>{args[0]};
          });
  if (failed(ordinals))
    return failure();
  auto ordinalGrid = splat(ordinals->front(), {*n});
  if (failed(ordinalGrid))
    return failure();
  auto compareType = hooks.bits(
      std::max<uint64_t>(*width, std::max(1u, llvm::Log2_64_Ceil(*m))));
  auto leaves = map(
      {*indexGrid, *ordinalGrid, *payloads}, {}, {*n, *m},
      {hooks.bits(1), sourceType.getElementType()},
      [&](OpBuilder &nested, ValueRange args) -> FailureOr<SmallVector<Value>> {
        Value valid = compare(nested, "eq", widen(nested, args[1], compareType),
                              widen(nested, args[2], compareType));
        if (!valid)
          return failure();
        return SmallVector<Value>{valid, args[3]};
      });
  if (failed(leaves))
    return failure();
  auto reduced = reduce(*leaves, *n, *m, false);
  if (failed(reduced))
    return failure();
  auto guardType =
      hooks.bits(std::max<uint64_t>(*width, llvm::Log2_64(*m) + 1));
  Value sentinel = constant(at, *m, guardType);
  if (!sentinel)
    return failure();
  auto poison = op(at, ac::TableGetOp::getOperationName(), {table, sentinel},
                   {sourceType.getElementType(), hooks.bits(1)});
  if (!poison)
    return failure();
  auto result = map(
      {indices, (*reduced)[1]}, {poison->getResult(0)}, {*n},
      {sourceType.getElementType()},
      [&](OpBuilder &nested, ValueRange args) -> FailureOr<SmallVector<Value>> {
        Value bound = constant(nested, *m, guardType);
        Value inRange =
            compare(nested, "ult", widen(nested, args[1], guardType), bound);
        if (!inRange)
          return failure();
        auto selected = hooks.select(inRange, args[2], args[3], nested);
        if (failed(selected))
          return failure();
        return SmallVector<Value>{*selected};
      });
  if (failed(result))
    return failure();
  return result->front();
}
FailureOr<SmallVector<Value>> TableQueryLowering::choose(Value predicates,
                                                         Value keys) {
  auto type = cast<ac::TableType>(predicates.getType());
  auto size = analysis.getTableSize(type, {}, package);
  if (failed(size))
    return failure();
  auto indexType = hooks.bits(std::max(1u, llvm::Log2_64_Ceil(*size)));
  std::string reason;
  TableQueryCharge logical;
  TableQueryBudget::add(logical, TableQueryResource::Work, *size - 1);
  if (!keys) {
    bool fits =
        TableQueryBudget::add(logical, TableQueryResource::Work, *size) &&
        TableQueryBudget::add(logical, TableQueryResource::Slots, 1);
    if (!fits || !budget.reserve(logical, reason))
      return emitError(loc)
             << (fits ? reason : "Table query first work budget exhausted");
    auto match = op(at, ac::TableMatchOp::getOperationName(), {predicates},
                    {hooks.bits(*size)}, {}, 1);
    if (!match)
      return failure();
    auto block = new Block();
    match->getRegion(0).push_back(block);
    Value predicate = block->addArgument(type.getElementType(), loc);
    auto nested = OpBuilder::atBlockEnd(block);
    if (!op(nested, ac::YieldOp::getOperationName(), {predicate}, {}))
      return failure();
    auto choice =
        op(at, ac::TableChooseOp::getOperationName(),
           {predicates, match->getResult(0)}, {indexType, hooks.bits(1)},
           {at.getNamedAttr("count", at.getI64IntegerAttr(1)),
            at.getNamedAttr("policy", at.getStringAttr("first")),
            at.getNamedAttr("order", at.getStringAttr("low"))});
    if (!choice)
      return failure();
    return SmallVector<Value>(choice->getResults());
  }
  if (!budget.reserve(logical, reason))
    return emitError(loc) << reason;
  auto keyType = cast<ac::TableType>(keys.getType()).getElementType();
  auto leaves = map(
      {predicates, keys}, {}, {*size}, {hooks.bits(1), keyType, indexType},
      [&](OpBuilder &nested, ValueRange args) -> FailureOr<SmallVector<Value>> {
        Value zero = constant(nested, 0, args[1].getType());
        Value valid = binary(nested, "or", zero, args[1]);
        if (!valid)
          return failure();
        return SmallVector<Value>{valid, args[2], args[0]};
      });
  if (failed(leaves))
    return failure();
  SmallVector<Value> grid;
  for (Value leaf : *leaves) {
    auto reshaped = view(leaf, "reshape", {1, *size},
                         {at.getNamedAttr("shape", shape({1, *size}))});
    if (failed(reshaped))
      return failure();
    grid.push_back(*reshaped);
  }
  auto reduced = reduce(grid, 1, *size, true);
  if (failed(reduced))
    return failure();
  Value zero = constant(at, 0, hooks.bits(1));
  if (!zero)
    return failure();
  SmallVector<Value> scalars;
  for (Value result : *reduced) {
    auto get = op(at, ac::TableGetOp::getOperationName(), {result, zero},
                  {cast<ac::TableType>(result.getType()).getElementType(),
                   hooks.bits(1)});
    if (!get)
      return failure();
    scalars.push_back(get->getResult(0));
  }
  return SmallVector<Value>{scalars[2], scalars[0]};
}

} // namespace acir::compiler::detail
