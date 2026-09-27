#include "ACIRStaticEvaluation.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"

using namespace mlir;

namespace acir::ac::detail {
namespace {

FailureOr<Type> physicalType(DictionaryAttr logical, MLIRContext *context,
                             EmitError error) {
  auto kind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
  if (!kind)
    return error() << "static evaluator requires a resolved logical type";
  if (kind.getValue() == "bool")
    return IntegerType::get(context, 1);
  if (kind.getValue() == "integer") {
    auto storage = logical.getAs<TypeAttr>("storage");
    if (storage)
      return storage.getValue();
  }
  if (kind.getValue() == "record") {
    auto symbol = logical.getAs<FlatSymbolRefAttr>("symbol");
    if (symbol)
      return StructType::get(context,
                             StringAttr::get(context, symbol.getValue()));
  }
  return error() << "U02-A static reset cannot physicalize this logical type";
}

FailureOr<DictionaryAttr> evaluateImpl(StaticExprAttr expression,
                                       Operation *site,
                                       llvm::DenseSet<Attribute> &active,
                                       EmitError error);

FailureOr<DictionaryAttr> evaluateConstructor(FlatSymbolRefAttr callee,
                                              ArrayAttr arguments,
                                              Operation *site,
                                              llvm::DenseSet<Attribute> &active,
                                              EmitError error) {
  auto file = site->getParentOfType<mlir::ModuleOp>();
  auto helper = file ? dyn_cast_or_null<func::FuncOp>(
                           SymbolTable::lookupSymbolIn(file, callee))
                     : func::FuncOp();
  auto kind = helper ? helper->getAttrOfType<StringAttr>("ac.helper_kind")
                     : StringAttr();
  auto nominal = helper ? helper->getAttrOfType<FlatSymbolRefAttr>("ac.record")
                        : FlatSymbolRefAttr();
  auto metadata =
      helper ? helper->getAttrOfType<ArrayAttr>("ac.parameters") : ArrayAttr();
  if (!helper || !kind || kind.getValue() != "record_constructor" || !nominal ||
      !metadata || helper.isExternal() ||
      helper.getBody().getBlocks().size() != 1)
    return error() << "static initializer requires an explicit verified "
                      "header-owned record constructor";
  if (!active.insert(callee).second)
    return error() << "recursive static record constructor call";
  auto removeActive = llvm::make_scope_exit([&] { active.erase(callee); });

  SmallVector<DictionaryAttr> bound(metadata.size());
  size_t nextPositional = 0;
  bool keywordSeen = false;
  for (Attribute raw : arguments) {
    auto argument = dyn_cast<DictionaryAttr>(raw);
    auto argKind = argument ? argument.getAs<StringAttr>("kind") : StringAttr();
    auto valueExpr =
        argument ? argument.getAs<StaticExprAttr>("value") : StaticExprAttr();
    if (!argKind || !valueExpr)
      return error() << "malformed static constructor argument";
    auto value = evaluateImpl(valueExpr, site, active, error);
    if (failed(value))
      return failure();
    size_t target = metadata.size();
    if (argKind.getValue() == "positional") {
      if (keywordSeen || nextPositional >= metadata.size())
        return error() << "too many or late positional constructor arguments";
      target = nextPositional++;
      auto parameter = cast<DictionaryAttr>(metadata[target]);
      auto binding = parameter.getAs<StringAttr>("binding");
      if (!binding || binding.getValue() == "keyword_only")
        return error()
               << "keyword-only constructor parameter passed positionally";
    } else if (argKind.getValue() == "keyword") {
      keywordSeen = true;
      auto name = argument.getAs<StringAttr>("name");
      if (!name)
        return error() << "constructor keyword requires a name";
      for (size_t index = 0; index < metadata.size(); ++index) {
        auto parameter = cast<DictionaryAttr>(metadata[index]);
        if (parameter.getAs<StringAttr>("name") == name) {
          target = index;
          auto binding = parameter.getAs<StringAttr>("binding");
          if (!binding || binding.getValue() == "positional_only")
            return error()
                   << "positional-only constructor parameter passed by keyword";
          break;
        }
      }
      if (target == metadata.size())
        return error() << "unknown constructor keyword";
    } else {
      return error() << "unsupported static constructor argument kind";
    }
    if (bound[target])
      return error() << "duplicate static constructor argument";
    bound[target] = *value;
  }

  auto resolver = [&](FlatSymbolRefAttr symbol) {
    return resolveSourceRecord(symbol, site, error);
  };
  FunctionType signature = helper.getFunctionType();
  if (signature.getNumInputs() != metadata.size() + 1 ||
      signature.getNumResults() != 2 ||
      !signature.getInput(metadata.size()).isInteger(1) ||
      !signature.getResult(1).isInteger(1))
    return error() << "static constructor physical signature is inconsistent";
  for (auto [index, raw] : llvm::enumerate(metadata)) {
    auto parameter = dyn_cast<DictionaryAttr>(raw);
    auto constraint = parameter ? parameter.getAs<DictionaryAttr>("constraint")
                                : DictionaryAttr();
    auto type = constraint ? constraint.getAs<DictionaryAttr>("type")
                           : DictionaryAttr();
    auto defaultValue = parameter ? parameter.getAs<DictionaryAttr>("default")
                                  : DictionaryAttr();
    auto constraintKind =
        constraint ? constraint.getAs<StringAttr>("kind") : StringAttr();
    if (!constraintKind || constraintKind.getValue() != "logical" || !type ||
        !defaultValue)
      return error() << "static constructor parameter contract is malformed";
    if (!bound[index]) {
      auto present = defaultValue.getAs<BoolAttr>("present");
      if (!present || !present.getValue())
        return error() << "missing required static constructor argument";
      bound[index] = defaultValue.getAs<DictionaryAttr>("value");
      if (!bound[index])
        return error() << "static constructor default is malformed";
    }
    auto physical = physicalType(type, site->getContext(), error);
    if (failed(physical) || signature.getInput(index) != *physical ||
        failed(verifyStaticValueMatchesType(
            bound[index], type, ExpectedTypeKind::Logical, resolver, error)))
      return error() << "static constructor argument type or range mismatch";
  }

  auto resultType = signature.getResult(0);
  auto recordType = dyn_cast<StructType>(resultType);
  if (!recordType || recordType.getName().getValue() != nominal.getValue())
    return error() << "static constructor result has wrong nominal type";
  auto declaredRecord = resolveSourceRecord(nominal, site, error);
  if (failed(declaredRecord))
    return failure();

  Block &block = helper.getBody().front();
  if (block.getNumArguments() != metadata.size() + 1)
    return error() << "static constructor body parameter count differs";
  llvm::DenseMap<Value, DictionaryAttr> values;
  for (size_t index = 0; index < metadata.size(); ++index)
    values.try_emplace(block.getArgument(index), bound[index]);
  Builder builder(site->getContext());
  auto trueValue = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("bool")),
      builder.getNamedAttr("value", builder.getBoolAttr(true)),
  });
  values.try_emplace(block.getArgument(metadata.size()), trueValue);

  for (Operation &operation : block) {
    if (auto create = dyn_cast<StructCreateOp>(operation)) {
      auto createdType = create.getResult().getType();
      auto createdRecord = dyn_cast<StructType>(createdType);
      if (!createdRecord)
        return error() << "static constructor creates a non-record";
      SmallVector<Attribute> fields;
      for (Value operand : create.getValues()) {
        DictionaryAttr value = values.lookup(operand);
        if (!value)
          return error() << "static constructor uses a nonconstant field";
        fields.push_back(value);
      }
      auto symbol = FlatSymbolRefAttr::get(site->getContext(),
                                           createdRecord.getName().getValue());
      auto created = builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("record")),
          builder.getNamedAttr("symbol", symbol),
          builder.getNamedAttr("fields", builder.getArrayAttr(fields)),
      });
      auto logical = builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("record")),
          builder.getNamedAttr("symbol", symbol),
      });
      if (failed(verifyStaticValueMatchesType(
              created, logical, ExpectedTypeKind::Logical, resolver, error)))
        return failure();
      values.try_emplace(create.getResult(), created);
      continue;
    }
    if (auto get = dyn_cast<StructGetOp>(operation)) {
      DictionaryAttr base = values.lookup(get.getValue());
      auto symbol =
          base ? base.getAs<FlatSymbolRefAttr>("symbol") : FlatSymbolRefAttr();
      auto fields = base ? base.getAs<ArrayAttr>("fields") : ArrayAttr();
      auto record = symbol ? resolveSourceRecord(symbol, site, error)
                           : FailureOr<ResolvedRecordView>(failure());
      if (failed(record) || !fields)
        return error() << "static record field source is unavailable";
      auto declaration =
          cast<StructOp>(SymbolTable::lookupSymbolIn(file, symbol));
      auto declarations = declaration.getFields();
      bool found = false;
      for (size_t index = 0; index < declarations.size(); ++index) {
        auto field = cast<DictionaryAttr>(declarations[index]);
        if (field.getAs<StringAttr>("name") != get.getFieldAttr())
          continue;
        values.try_emplace(get.getResult(),
                           cast<DictionaryAttr>(fields[index]));
        found = true;
        break;
      }
      if (!found)
        return error() << "static record field does not exist";
      continue;
    }
    if (auto ret = dyn_cast<func::ReturnOp>(operation)) {
      if (ret.getNumOperands() != 2)
        return error() << "static constructor must return data and validity";
      DictionaryAttr data = values.lookup(ret.getOperand(0));
      DictionaryAttr valid = values.lookup(ret.getOperand(1));
      auto kindAttr = valid ? valid.getAs<StringAttr>("kind") : StringAttr();
      auto bit = valid ? valid.getAs<BoolAttr>("value") : BoolAttr();
      if (!data || !kindAttr || kindAttr.getValue() != "bool" || !bit ||
          !bit.getValue())
        return error() << "static constructor did not return valid data";
      if (data.getAs<FlatSymbolRefAttr>("symbol") != nominal)
        return error() << "static constructor returned the wrong record";
      return data;
    }
    return error() << "U02-A constant evaluator cannot execute this verified "
                      "helper operation";
  }
  return error() << "static constructor has no return";
}

FailureOr<DictionaryAttr> evaluateImpl(StaticExprAttr expression,
                                       Operation *site,
                                       llvm::DenseSet<Attribute> &active,
                                       EmitError error) {
  if (!expression)
    return error() << "missing StaticExpr initializer";
  DictionaryAttr tree = expression.getTree();
  auto kind = tree ? tree.getAs<StringAttr>("kind") : StringAttr();
  if (!kind)
    return error() << "StaticExpr initializer has no kind";
  if (kind.getValue() == "literal") {
    auto literal = tree.getAs<DictionaryAttr>("value");
    if (!literal || failed(verifyStaticValueStructure(literal, error)))
      return failure();
    return literal;
  }
  if (kind.getValue() == "call") {
    auto callee = tree.getAs<FlatSymbolRefAttr>("callee");
    auto arguments = tree.getAs<ArrayAttr>("arguments");
    if (!callee || !arguments)
      return error() << "StaticExpr call has no callee or arguments";
    return evaluateConstructor(callee, arguments, site, active, error);
  }
  return error() << "U02-A source reset supports literal or verified "
                    "record-constructor call";
}

} // namespace

FailureOr<ResolvedRecordView> resolveSourceRecord(FlatSymbolRefAttr symbol,
                                                  Operation *site,
                                                  EmitError error) {
  auto file = site->getParentOfType<mlir::ModuleOp>();
  auto record = file ? dyn_cast_or_null<StructOp>(
                           SymbolTable::lookupSymbolIn(file, symbol))
                     : StructOp();
  if (!record)
    return error() << "static nominal record has no unit declaration snapshot";
  ResolvedRecordView view;
  view.symbol = symbol;
  for (Attribute raw : record.getFields()) {
    auto field = dyn_cast<DictionaryAttr>(raw);
    auto logical =
        field ? field.getAs<DictionaryAttr>("type") : DictionaryAttr();
    if (!logical)
      return error() << "static nominal record field is malformed";
    view.fieldLogicalTypes.push_back(logical);
  }
  return view;
}

FailureOr<DictionaryAttr> evaluateSourceStaticExpr(StaticExprAttr expression,
                                                   Operation *site,
                                                   EmitError error) {
  llvm::DenseSet<Attribute> active;
  return evaluateImpl(expression, site, active, error);
}

} // namespace acir::ac::detail
