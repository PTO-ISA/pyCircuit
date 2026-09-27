#include "PythonImportRecords.h"
#include "PythonImportInternal.h"
#include "PythonImportSignature.h"

#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"

#include <optional>
#include <string>

using namespace mlir;

namespace acir::compiler::detail {
namespace {

bool isDocstring(const AstNode &node) {
  if (node.kind() != "Expr")
    return false;
  AstNode value = node.child("value");
  return value.kind() == "Constant" && isa<StringAttr>(value.get("value"));
}

DictionaryAttr logicalBool(OpBuilder &builder) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("bool")),
      builder.getNamedAttr("storage", TypeAttr::get(builder.getI1Type())),
  });
}

DictionaryAttr logicalInteger(OpBuilder &builder, ac::MathIntAttr lower,
                              ac::MathIntAttr upper, unsigned width) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("storage",
                           TypeAttr::get(builder.getIntegerType(width))),
      builder.getNamedAttr("lower", lower),
      builder.getNamedAttr("upper", upper),
      builder.getNamedAttr("interpretation", builder.getStringAttr("unsigned")),
  });
}

DictionaryAttr logicalRecord(OpBuilder &builder, FlatSymbolRefAttr symbol) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("record")),
      builder.getNamedAttr("symbol", symbol),
  });
}

FailureOr<ac::MathIntAttr> integerLiteral(OpBuilder &builder,
                                          const AstNode &node,
                                          ac::detail::EmitError emitError) {
  if (node.kind() != "Constant")
    return emitError() << "expected a static integer literal";
  auto encoded = dyn_cast_or_null<DictionaryAttr>(node.get("value"));
  auto spelling = encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
  if (!spelling)
    return emitError() << "expected a static integer literal";
  return parseStaticInteger(builder, spelling.getValue(), emitError);
}

DictionaryAttr fieldRecord(OpBuilder &builder, StringRef name,
                           DictionaryAttr type, FlatSymbolRefAttr definition,
                           const AstNode &node, StringRef sourcePath) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("name", builder.getStringAttr(name)),
      builder.getNamedAttr("type", type),
      builder.getNamedAttr("origin", occurrence(builder, definition, node)),
      builder.getNamedAttr("location", sourceSpan(builder, sourcePath, node)),
  });
}

OwningOpRef<ModuleOp> createUnit(MLIRContext *context, Location location,
                                 DictionaryAttr owner, StringRef kind,
                                 ArrayAttr interfaces) {
  OwningOpRef<ModuleOp> unit(ModuleOp::create(location));
  unit->getOperation()->setAttr("ac.source_owner", owner);
  unit->getOperation()->setAttr("ac.unit_kind", StringAttr::get(context, kind));
  unit->getOperation()->setAttr("ac.stage", StringAttr::get(context, "source"));
  unit->getOperation()->setAttr("ac.interfaces", interfaces);
  return unit;
}

Operation *createAlias(OpBuilder &builder, Location location, StringRef symbol,
                       DictionaryAttr owner, DictionaryAttr origin,
                       StringRef role, DictionaryAttr target) {
  OperationState state(location, ac::TypeAliasOp::getOperationName());
  state.addAttribute(SymbolTable::getSymbolAttrName(),
                     builder.getStringAttr(symbol));
  state.addAttribute("ac.source_owner", owner);
  state.addAttribute("ac.origin", origin);
  state.addAttribute("ac.declaration_role", builder.getStringAttr(role));
  state.addAttribute("target", target);
  return builder.create(state);
}

Operation *createConstant(OpBuilder &builder, Location location,
                          StringRef symbol, DictionaryAttr owner,
                          DictionaryAttr origin, StringRef role,
                          DictionaryAttr type, DictionaryAttr value) {
  OperationState state(location, ac::ConstantOp::getOperationName());
  state.addAttribute(SymbolTable::getSymbolAttrName(),
                     builder.getStringAttr(symbol));
  state.addAttribute("ac.source_owner", owner);
  state.addAttribute("ac.origin", origin);
  state.addAttribute("ac.declaration_role", builder.getStringAttr(role));
  state.addAttribute("type", type);
  state.addAttribute("value", value);
  return builder.create(state);
}

Operation *createStruct(OpBuilder &builder, Location location, StringRef symbol,
                        DictionaryAttr owner, DictionaryAttr origin,
                        StringRef role, ArrayAttr fields,
                        FlatSymbolRefAttr constructor) {
  OperationState state(location, ac::StructOp::getOperationName());
  state.addAttribute(SymbolTable::getSymbolAttrName(),
                     builder.getStringAttr(symbol));
  state.addAttribute("ac.source_owner", owner);
  state.addAttribute("ac.origin", origin);
  state.addAttribute("ac.declaration_role", builder.getStringAttr(role));
  state.addAttribute("fields", fields);
  state.addAttribute("constructor", constructor);
  return builder.create(state);
}

} // namespace

RecordCompiler::RecordCompiler(const CapturedSource &source,
                               DictionaryAttr owner,
                               const SourceHeaderRegistry &headers,
                               ac::detail::EmitError emitError)
    : PythonImportContext(source, owner, headers, emitError) {}

LogicalResult RecordCompiler::scanImportsAndAliases() {
  ArrayAttr statements = source.module.array("body");
  for (size_t index = 0; index < statements.size(); ++index) {
    AstNode statement = source.module.item("body", index);
    if (statement.kind() == "Import")
      return emitError() << "plain import is outside the U01 source capability";
    if (statement.kind() == "ImportFrom") {
      StringRef importedModule = statement.string("module");
      auto level = dyn_cast_or_null<DictionaryAttr>(statement.get("level"));
      auto levelText =
          level ? level.getAs<StringAttr>("integer") : StringAttr();
      if (!levelText)
        return emitError() << "relative import level must be a static integer";
      std::string targetModule = importedModule.str();
      if (levelText.getValue() == "1") {
        StringRef prefix = StringRef(module).rsplit('.').first;
        targetModule = prefix.empty()
                           ? importedModule.str()
                           : (Twine(prefix) + "." + importedModule).str();
      } else if (levelText.getValue() != "0")
        return emitError()
               << "relative import levels greater than one are outside U01";
      if (targetModule == "typing")
        continue;
      if (targetModule == "pycircuit") {
        ArrayAttr names = statement.array("names");
        for (size_t nameIndex = 0; nameIndex < names.size(); ++nameIndex) {
          AstNode alias = statement.item("names", nameIndex);
          StringRef remote = alias.string("name");
          if (remote != "module" && remote != "rule")
            return emitError() << "pycircuit import is outside the U02-A "
                                  "module/rule capability";
          StringRef local =
              alias.get("asname") && !isa<UnitAttr>(alias.get("asname"))
                  ? alias.string("asname")
                  : remote;
          bindCompilerDecorator(local, remote);
        }
        continue;
      }
      DictionaryAttr provider = headers.ownerForModule(targetModule);
      ArrayAttr closure = headers.interfacesForModule(targetModule);
      if (!provider || !closure || closure.empty())
        return emitError() << "missing explicit interface for import module '"
                           << targetModule << "'";
      for (Attribute rawDependency : closure) {
        auto dependency = dyn_cast<DictionaryAttr>(rawDependency);
        if (dependency && !llvm::is_contained(dependencies, dependency))
          dependencies.push_back(dependency);
      }
      ArrayAttr names = statement.array("names");
      for (size_t nameIndex = 0; nameIndex < names.size(); ++nameIndex) {
        AstNode alias = statement.item("names", nameIndex);
        StringRef remote = alias.string("name");
        StringRef local =
            alias.get("asname") && !isa<UnitAttr>(alias.get("asname"))
                ? alias.string("asname")
                : remote;
        auto symbol = headers.lookupExport(targetModule, remote);
        if (!symbol)
          return emitError() << "interface module '" << targetModule
                             << "' does not export '" << remote << "'";
        bindNamespaceName(local, symbol, alias);
        recordNamespaceImport(provider, remote, symbol, alias);
      }
      continue;
    }
    if (statement.kind() == "ClassDef" || statement.kind() == "FunctionDef") {
      StringRef name = statement.string("name");
      bindNamespaceName(
          name,
          FlatSymbolRefAttr::get(builder.getContext(), qualifiedName(name)),
          statement);
      continue;
    }
    if (statement.kind() != "Assign")
      continue;
    ArrayAttr targets = statement.array("targets");
    if (!targets || targets.size() != 1)
      return emitError() << "type alias assignment requires one target";
    AstNode target = statement.item("targets", 0);
    if (target.kind() != "Name")
      continue;
    AstNode assigned = statement.child("value");
    DictionaryAttr value =
        staticValue(builder, assigned.get("value"), emitError);
    if (value) {
      StringRef name = target.string("id");
      std::string symbol = qualifiedName(name);
      auto flat = FlatSymbolRefAttr::get(builder.getContext(), symbol);
      AstNode declaration = statement;
      declaration.path.clear();
      StringRef kind = value.getAs<StringAttr>("kind").getValue();
      DictionaryAttr type = builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr(kind))});
      Operation *constant = createConstant(
          builder, statement.location(builder.getContext(), source.path),
          symbol, owner, occurrence(builder, flat, declaration), "definition",
          type, value);
      registerLocalDeclaration(flat, constant);
      bindNamespaceName(name, flat, statement);
      continue;
    }
    auto type = annotation(assigned);
    if (failed(type))
      return emitError()
             << "U01 assignment must be a supported source type alias";
    StringRef name = target.string("id");
    std::string symbol = qualifiedName(name);
    auto flat = FlatSymbolRefAttr::get(builder.getContext(), symbol);
    AstNode declaration = statement;
    declaration.path.clear();
    Operation *alias = createAlias(
        builder, statement.location(builder.getContext(), source.path), symbol,
        owner, occurrence(builder, flat, declaration), "definition", *type);
    registerLocalDeclaration(flat, alias);
    bindNamespaceName(name, flat, statement);
    continue;
  }
  llvm::sort(dependencies, [](DictionaryAttr left, DictionaryAttr right) {
    StringRef leftPackage = left.getAs<StringAttr>("package").getValue();
    StringRef rightPackage = right.getAs<StringAttr>("package").getValue();
    if (leftPackage != rightPackage)
      return leftPackage < rightPackage;
    return left.getAs<StringAttr>("path").getValue() <
           right.getAs<StringAttr>("path").getValue();
  });
  return success();
}

FailureOr<DictionaryAttr> RecordCompiler::annotation(const AstNode &node) {
  if (node.kind() == "Name") {
    StringRef name = node.string("id");
    if (name == "bool")
      return logicalBool(builder);
    auto binding = namespaceBindings.find(name);
    if (binding == namespaceBindings.end())
      return emitError() << "unresolved source annotation '" << name << "'";
    Operation *declaration = lookupCanonicalDeclaration(binding->second.target);
    if (auto alias = dyn_cast_or_null<ac::TypeAliasOp>(declaration))
      return alias.getTarget();
    if (isa_and_nonnull<ac::StructOp>(declaration))
      return logicalRecord(builder, binding->second.target);
    return emitError() << "imported symbol is not a source type: "
                       << binding->second.target;
  }
  if (node.kind() != "Subscript" ||
      node.child("value").string("id") != "Annotated")
    return emitError() << "unsupported U01 source annotation";
  AstNode tuple = node.child("slice");
  if (tuple.kind() != "Tuple" || tuple.array("elts").size() != 2 ||
      tuple.item("elts", 0).string("id") != "int")
    return emitError() << "Annotated integer requires int and range bounds";
  AstNode range = tuple.item("elts", 1);
  if (range.kind() != "Call" || range.child("func").string("id") != "range" ||
      range.array("args").size() != 1 || !range.array("keywords").empty())
    return emitError() << "Annotated integer requires range(upper)";
  auto upper = integerLiteral(builder, range.item("args", 0), emitError);
  auto lower = parseStaticInteger(builder, "0", emitError);
  if (failed(lower) || failed(upper))
    return failure();
  llvm::APSInt upperValue((*upper).getCanonicalValue());
  unsigned width = 1;
  while (width < 64 &&
         llvm::APSInt::compareValues(
             upperValue,
             llvm::APSInt(llvm::APInt::getOneBitSet(width + 1, width), true)) >
             0)
    ++width;
  DictionaryAttr result = logicalInteger(builder, *lower, *upper, width);
  auto resolver = [&](FlatSymbolRefAttr symbol) {
    return headers.resolveRecord(symbol);
  };
  if (failed(ac::detail::verifyTypeResolved(
          result, ac::detail::ExpectedTypeKind::Logical, resolver, emitError)))
    return failure();
  return result;
}

LogicalResult RecordCompiler::emitRecord(const AstNode &node) {
  if (!node.array("bases").empty() || !node.array("keywords").empty() ||
      !node.array("decorator_list").empty() ||
      (node.array("type_params") && !node.array("type_params").empty()))
    return emitError() << "U01 records reject inheritance, metaclasses, "
                          "decorators and type parameters";
  StringRef name = node.string("name");
  std::string symbolText = qualifiedName(name);
  auto symbol = FlatSymbolRefAttr::get(builder.getContext(), symbolText);
  AstNode constructor;
  SmallVector<std::pair<AstNode, DictionaryAttr>> fields;
  llvm::StringSet<> fieldNames;
  ArrayAttr bodyNodes = node.array("body");
  for (size_t index = 0; index < bodyNodes.size(); ++index) {
    AstNode member = node.item("body", index);
    member.path = {{"body", std::nullopt}, {{}, static_cast<uint64_t>(index)}};
    if (isDocstring(member))
      continue;
    if (member.kind() == "AnnAssign") {
      AstNode target = member.child("target");
      if (target.kind() != "Name")
        return emitError() << "record field declaration requires a name";
      if (member.get("value") && !isa<UnitAttr>(member.get("value")))
        return emitError()
               << "record field declarations cannot contain initializers";
      auto type = annotation(member.child("annotation"));
      if (failed(type))
        return failure();
      if (!fieldNames.insert(target.string("id")).second)
        return emitError() << "duplicate record field declaration '"
                           << target.string("id") << "'";
      fields.push_back({member, *type});
      continue;
    }
    if (member.kind() == "FunctionDef" && member.string("name") == "__init__") {
      if (!member.array("decorator_list").empty() ||
          (member.array("type_params") && !member.array("type_params").empty()))
        return emitError()
               << "record constructors reject decorators and type parameters";
      if (constructor)
        return emitError() << "record defines more than one __init__";
      constructor = member;
      continue;
    }
    return emitError() << "U01 record body supports fields and __init__ only";
  }
  if (!constructor)
    return emitError() << "record requires one __init__ constructor";
  constructor.path.clear();

  std::string constructorText = symbolText + ".__init__";
  auto constructorSymbol =
      FlatSymbolRefAttr::get(builder.getContext(), constructorText);
  SmallVector<Attribute> fieldAttrs;
  for (const auto &[field, type] : fields)
    fieldAttrs.push_back(fieldRecord(builder,
                                     field.child("target").string("id"), type,
                                     symbol, field, source.path));
  Operation *record = createStruct(
      builder, node.location(builder.getContext(), source.path), symbolText,
      owner, occurrence(builder, symbol, {}), "definition",
      builder.getArrayAttr(fieldAttrs), constructorSymbol);
  registerLocalDeclaration(symbol, record);

  AstNode arguments = constructor.child("args");
  auto signature = parseFunctionSignature(arguments, true, emitError);
  if (failed(signature))
    return failure();

  SmallVector<Type> inputTypes;
  SmallVector<Attribute> parameters;
  llvm::StringMap<size_t> parameterIndices;
  llvm::StringMap<DictionaryAttr> parameterTypes;
  for (size_t index = 0; index < signature->parameters.size(); ++index) {
    const ParameterSyntax &syntax = signature->parameters[index];
    AstNode formal = syntax.parameter;
    auto type = annotation(formal.child("annotation"));
    if (failed(type))
      return failure();
    StringRef parameterName = formal.string("arg");
    parameterIndices[parameterName] = index;
    parameterTypes[parameterName] = *type;
    inputTypes.push_back(physicalType(*type, builder.getContext()));
    DictionaryAttr defaultValue = absentDefault(builder);
    if (syntax.defaultValue) {
      AstNode defaultNode = *syntax.defaultValue;
      DictionaryAttr value =
          staticValue(builder, defaultNode.get("value"), emitError);
      if (!value)
        return emitError() << "U01 defaults must be bool or integer literals";
      defaultValue = builder.getDictionaryAttr({
          builder.getNamedAttr("present", builder.getBoolAttr(true)),
          builder.getNamedAttr("value", value),
      });
      auto resolver = [&](FlatSymbolRefAttr requested) {
        if (requested == symbol) {
          ac::detail::ResolvedRecordView local;
          local.symbol = symbol;
          for (const auto &field : fields)
            local.fieldLogicalTypes.push_back(field.second);
          return FailureOr<ac::detail::ResolvedRecordView>(std::move(local));
        }
        return headers.resolveRecord(requested);
      };
      if (failed(ac::detail::verifyDefaultMatchesType(
              defaultValue, *type, ac::detail::ExpectedTypeKind::Logical,
              resolver, emitError)))
        return failure();
    }
    parameters.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("name", builder.getStringAttr(parameterName)),
        builder.getNamedAttr("binding", builder.getStringAttr(syntax.binding)),
        builder.getNamedAttr("constraint", valueConstraint(builder, *type)),
        builder.getNamedAttr("default", defaultValue),
        builder.getNamedAttr("origin",
                             occurrence(builder, constructorSymbol, formal)),
        builder.getNamedAttr("location",
                             sourceSpan(builder, source.path, formal)),
    }));
  }
  inputTypes.push_back(builder.getI1Type());
  auto recordType = ac::StructType::get(builder.getContext(),
                                        builder.getStringAttr(symbolText));
  auto functionType = builder.getFunctionType(
      inputTypes, TypeRange{recordType, builder.getI1Type()});
  auto function = func::FuncOp::create(
      constructor.location(builder.getContext(), source.path), constructorText,
      functionType);
  function->setAttr("ac.source_owner", owner);
  function->setAttr("ac.origin", occurrence(builder, constructorSymbol, {}));
  function->setAttr("ac.declaration_role", builder.getStringAttr("definition"));
  function->setAttr("ac.helper_kind",
                    builder.getStringAttr("record_constructor"));
  function->setAttr("ac.parameters", builder.getArrayAttr(parameters));
  function->setAttr("ac.return_form", builder.getStringAttr("single"));
  function->setAttr("ac.result_constraints",
                    builder.getArrayAttr({valueConstraint(
                        builder, logicalRecord(builder, symbol))}));
  function->setAttr("ac.check_templates", builder.getArrayAttr({}));
  function->setAttr("ac.record", symbol);
  registerLocalDeclaration(constructorSymbol, function);
  Block *entry = function.addEntryBlock();
  OpBuilder at = OpBuilder::atBlockEnd(entry);
  llvm::StringMap<size_t> fieldArguments;
  ArrayAttr constructorBody = constructor.array("body");
  for (size_t index = 0; index < constructorBody.size(); ++index) {
    AstNode statement = constructor.item("body", index);
    if (isDocstring(statement))
      continue;
    if (statement.kind() != "Assign" || statement.array("targets").size() != 1)
      return emitError()
             << "U01 record constructor supports direct field assignments";
    AstNode target = statement.item("targets", 0);
    AstNode base = target.child("value");
    AstNode assigned = statement.child("value");
    if (target.kind() != "Attribute" || base.kind() != "Name" ||
        base.string("id") != "self" || assigned.kind() != "Name")
      return emitError() << "U01 record constructor assignment must be "
                            "self.field = parameter";
    if (!fieldNames.contains(target.string("attr")))
      return emitError() << "constructor assigns undeclared field '"
                         << target.string("attr") << "'";
    auto parameter = parameterIndices.find(assigned.string("id"));
    if (parameter == parameterIndices.end())
      return emitError() << "constructor assignment uses an unknown parameter";
    if (!fieldArguments.try_emplace(target.string("attr"), parameter->second)
             .second)
      return emitError() << "record field is initialized more than once";
  }
  SmallVector<Value> values;
  for (const auto &[field, fieldType] : fields) {
    StringRef fieldName = field.child("target").string("id");
    auto argument = fieldArguments.find(fieldName);
    if (argument == fieldArguments.end())
      return emitError() << "record constructor does not initialize field '"
                         << fieldName << "'";
    StringRef parameterName = cast<DictionaryAttr>(parameters[argument->second])
                                  .getAs<StringAttr>("name")
                                  .getValue();
    if (parameterTypes.lookup(parameterName) != fieldType)
      return emitError() << "constructor assignment type does not match field '"
                         << fieldName << "'";
    values.push_back(entry->getArgument(argument->second));
  }
  auto created = at.create<ac::StructCreateOp>(function.getLoc(), recordType,
                                               ValueRange(values));
  at.create<func::ReturnOp>(
      function.getLoc(),
      ValueRange{created.getResult(), entry->getArguments().back()});
  interface->getBody()->push_back(function);
  return success();
}

FailureOr<SourceUnitArtifacts> RecordCompiler::run() {
  if (source.path != owner.getAs<StringAttr>("path").getValue())
    return emitError() << "capture path does not match SourceOwner path";
  Location location = source.module.location(builder.getContext(), source.path);
  SmallVector<Attribute> interfaces{owner};
  for (DictionaryAttr dependency : dependencies)
    interfaces.push_back(dependency);
  body = createUnit(builder.getContext(), location, owner, "declarations",
                    builder.getArrayAttr(interfaces));
  interface = createUnit(builder.getContext(), location, owner, "interface",
                         builder.getArrayAttr(interfaces));
  builder.setInsertionPointToEnd(interface->getBody());
  if (failed(scanImportsAndAliases()))
    return failure();
  unsigned publicModules = 0;
  ArrayAttr statements = source.module.array("body");
  for (size_t index = 0; index < statements.size(); ++index)
    if (isModuleDefinition(source.module.item("body", index)))
      ++publicModules;
  if (publicModules > 1)
    return emitError()
           << "one source unit may define at most one @module class";
  if (publicModules == 1)
    (*body)->setAttr("ac.unit_kind", builder.getStringAttr("implementation"));
  interfaces.assign({owner});
  llvm::append_range(interfaces, dependencies);
  (*body)->setAttr("ac.interfaces", builder.getArrayAttr(interfaces));
  (*interface)->setAttr("ac.interfaces", builder.getArrayAttr(interfaces));
  if (failed(cloneImportedDeclarations(publicModules == 1)))
    return failure();

  for (size_t index = 0; index < statements.size(); ++index) {
    AstNode statement = source.module.item("body", index);
    if (isDocstring(statement) || statement.kind() == "Import" ||
        statement.kind() == "ImportFrom" || statement.kind() == "Assign")
      continue;
    if (statement.kind() == "ClassDef") {
      LogicalResult result = isModuleDefinition(statement)
                                 ? emitModule(statement)
                                 : emitRecord(statement);
      if (failed(result))
        return failure();
      continue;
    }
    if (statement.kind() == "FunctionDef") {
      if (failed(emitValueHelper(statement)))
        return failure();
      continue;
    }
    return emitError() << "unsupported U01 top-level source syntax '"
                       << statement.kind() << "'";
  }

  if (failed(attachNamespaceMetadata()))
    return failure();

  SmallVector<ModuleOp> validationHeaders(headers.suppliedHeaders());
  validationHeaders.push_back(*interface);
  auto validated = SourceHeaderRegistry::create(validationHeaders, emitError);
  if (failed(validated))
    return failure();
  return SourceUnitArtifacts{std::move(body), std::move(interface)};
}

FailureOr<SourceUnitArtifacts>
lowerRecordSourceUnit(const CapturedSource &source, DictionaryAttr owner,
                      const SourceHeaderRegistry &headers,
                      ac::detail::EmitError emitError) {
  return RecordCompiler(source, owner, headers, emitError).run();
}

} // namespace acir::compiler::detail
