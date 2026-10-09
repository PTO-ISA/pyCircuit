#include "SourceStaticRecordConstructors.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/DenseMap.h"
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
  return error()
         << "source module static reset cannot physicalize this logical type";
}

} // namespace

FailureOr<func::FuncOp>
resolveSourceStaticRecordConstructor(FlatSymbolRefAttr callee, Operation *site,
                                     EmitError error) {
  if (!site || !callee)
    return error() << "static constructor requires canonical callee and fixed "
                      "source scope";
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
  auto signature = helper.getFunctionType();
  if (signature.getNumInputs() != metadata.size() + 1 ||
      signature.getNumResults() != 2 ||
      !signature.getInput(metadata.size()).isInteger(1) ||
      !signature.getResult(1).isInteger(1) ||
      helper.getBody().front().getNumArguments() != metadata.size() + 1)
    return error() << "static constructor physical signature is inconsistent";
  auto record = dyn_cast<StructType>(signature.getResult(0));
  if (!record || record.getName().getValue() != nominal.getValue() ||
      failed(resolveSourceRecord(nominal, site, error)))
    return error() << "static constructor result has wrong nominal declaration";
  for (auto [index, raw] : llvm::enumerate(metadata)) {
    auto parameter = dyn_cast<DictionaryAttr>(raw);
    auto constraint = parameter ? parameter.getAs<DictionaryAttr>("constraint")
                                : DictionaryAttr();
    auto type = constraint ? constraint.getAs<DictionaryAttr>("type")
                           : DictionaryAttr();
    auto kind =
        constraint ? constraint.getAs<StringAttr>("kind") : StringAttr();
    if (!parameter || !kind || kind.getValue() != "logical" || !type ||
        !parameter.getAs<DictionaryAttr>("default") ||
        !parameter.getAs<StringAttr>("name") ||
        !parameter.getAs<StringAttr>("binding"))
      return error() << "static constructor parameter contract is malformed";
    if (failed(verifyLogicalTypeStructure(type, error)) ||
        failed(verifyDefaultStructure(
            parameter.getAs<DictionaryAttr>("default"), error)))
      return failure();
    auto physical = physicalType(type, site->getContext(), error);
    if (failed(physical) || signature.getInput(index) != *physical)
      return error()
             << "static constructor physical argument type is inconsistent";
  }
  return helper;
}

FailureOr<SmallVector<unsigned>> bindStaticRecordArguments(func::FuncOp helper,
                                                           ArrayAttr arguments,
                                                           EmitError error) {
  auto metadata = helper->getAttrOfType<ArrayAttr>("ac.parameters");
  if (!metadata || !arguments)
    return error() << "static constructor requires parameter/argument arrays";
  SmallVector<unsigned> mapping;
  llvm::DenseSet<unsigned> bound;
  size_t nextPositional = 0;
  bool keywordSeen = false;
  for (Attribute raw : arguments) {
    auto argument = dyn_cast<DictionaryAttr>(raw);
    auto argKind = argument ? argument.getAs<StringAttr>("kind") : StringAttr();
    auto valueExpr =
        argument ? argument.getAs<StaticExprAttr>("value") : StaticExprAttr();
    if (!argKind || !valueExpr)
      return error() << "malformed static constructor argument";
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
    if (!bound.insert(static_cast<unsigned>(target)).second)
      return error() << "duplicate static constructor argument";
    mapping.push_back(static_cast<unsigned>(target));
  }

  for (auto [index, raw] : llvm::enumerate(metadata)) {
    if (bound.contains(static_cast<unsigned>(index)))
      continue;
    auto parameter = cast<DictionaryAttr>(raw);
    auto defaultValue = parameter.getAs<DictionaryAttr>("default");
    auto present =
        defaultValue ? defaultValue.getAs<BoolAttr>("present") : BoolAttr();
    if (!present || !present.getValue())
      return error() << "missing required static constructor argument";
  }
  return mapping;
}

FailureOr<DictionaryAttr> evaluateSourceStaticRecordConstructor(
    func::FuncOp helper, ArrayAttr arguments, Operation *site,
    llvm::DenseSet<Operation *> &activeConstructors,
    EvaluateStaticChild evaluateChild, EmitError error) {
  auto file = site->getParentOfType<mlir::ModuleOp>();
  auto nominal = helper->getAttrOfType<FlatSymbolRefAttr>("ac.record");
  auto metadata = helper->getAttrOfType<ArrayAttr>("ac.parameters");
  auto mapping = bindStaticRecordArguments(helper, arguments, error);
  if (failed(mapping))
    return failure();
  SmallVector<DictionaryAttr> bound(metadata.size());
  for (auto [index, raw] : llvm::enumerate(arguments)) {
    auto value =
        evaluateChild(cast<DictionaryAttr>(raw).getAs<StaticExprAttr>("value"));
    if (failed(value))
      return failure();
    bound[(*mapping)[index]] = *value;
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
  // Actual expressions and boundary checks finish before entering this body.
  // Nested calls in actuals are ordinary calls, not recursive body execution.
  if (!activeConstructors.insert(helper).second)
    return error() << "recursive static record constructor call";
  llvm::scope_exit removeActive([&] { activeConstructors.erase(helper); });
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
    return error()
           << "source module constant evaluator cannot execute this verified "
              "helper operation";
  }
  return error() << "static constructor has no return";
}

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

} // namespace acir::ac::detail
