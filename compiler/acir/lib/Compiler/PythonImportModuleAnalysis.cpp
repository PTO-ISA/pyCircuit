#include "PythonImportModules.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::compiler::detail {
bool isModuleDocstring(const AstNode &node) {
  if (node.kind() != "Expr")
    return false;
  AstNode value = node.child("value");
  return value.kind() == "Constant" && isa<StringAttr>(value.get("value"));
}

bool isModuleSelfMember(const AstNode &node, StringRef *name) {
  if (node.kind() != "Attribute" || node.child("value").kind() != "Name" ||
      node.child("value").string("id") != "self")
    return false;
  if (name)
    *name = node.string("attr");
  return true;
}

bool usesLexicalName(const AstNode &node, StringRef name) {
  if (!node)
    return false;
  if (node.kind() == "Name" && node.string("id") == name)
    return true;
  if (node.kind() == "Nonlocal")
    for (Attribute rawName : node.array("names"))
      if (cast<StringAttr>(rawName).getValue() == name)
        return true;
  for (NamedAttribute field : node.fields()) {
    if (auto child = dyn_cast<DictionaryAttr>(field.getValue());
        child && child.getAs<StringAttr>("kind")) {
      if (usesLexicalName(node.child(field.getName()), name))
        return true;
      continue;
    }
    auto children = dyn_cast<ArrayAttr>(field.getValue());
    if (!children)
      continue;
    for (size_t index = 0; index < children.size(); ++index) {
      auto child = dyn_cast<DictionaryAttr>(children[index]);
      if (child && child.getAs<StringAttr>("kind") &&
          usesLexicalName(node.item(field.getName(), index), name))
        return true;
    }
  }
  return false;
}

bool RecordCompiler::isModuleDefinition(const AstNode &node) const {
  if (node.kind() != "ClassDef" && node.kind() != "FunctionDef")
    return false;
  for (size_t index = 0; index < node.array("decorator_list").size(); ++index) {
    AstNode decorator = node.item("decorator_list", index);
    if (decorator.kind() == "Name" &&
        (isCompilerDecorator(decorator.string("id"), "module") ||
         isCompilerDecorator(decorator.string("id"), "system")))
      return true;
  }
  return false;
}

LogicalResult RecordCompiler::emitModule(const AstNode &node) {
  return ModuleCompiler(*this, node).run();
}

ModuleCompiler::ModuleCompiler(RecordCompiler &sourceCompiler,
                               const AstNode &declaration)
    : sourceCompiler(sourceCompiler), declaration(declaration) {}

FailureOr<ModuleModel> ModuleCompiler::classify() {
  if (declaration.kind() != "FunctionDef")
    return sourceCompiler.emitError()
           << "@module and @system require function definitions; class/self "
              "authoring has been retired";
  if (declaration.array("type_params") &&
      !declaration.array("type_params").empty())
    return sourceCompiler.emitError()
           << "module/system functions reject type parameters";
  ArrayAttr moduleDecorators = declaration.array("decorator_list");
  if (!moduleDecorators || moduleDecorators.size() != 1)
    return sourceCompiler.emitError()
           << "module/system functions require exactly one compiler decorator";
  AstNode moduleDecorator = declaration.item("decorator_list", 0);
  bool isModule = moduleDecorator.kind() == "Name" &&
                  sourceCompiler.isCompilerDecorator(
                      moduleDecorator.string("id"), "module");
  bool isSystem = moduleDecorator.kind() == "Name" &&
                  sourceCompiler.isCompilerDecorator(
                      moduleDecorator.string("id"), "system");
  if (!isModule && !isSystem)
    return sourceCompiler.emitError()
           << "decorator does not resolve to pycircuit.module or system";

  ModuleModel model;
  model.declaration = declaration;
  model.constructor = declaration;
  model.isSystem = isSystem;
  model.symbol = FlatSymbolRefAttr::get(
      sourceCompiler.builder.getContext(),
      sourceCompiler.qualifiedName(declaration.string("name")));

  llvm::StringMap<AstNode> ruleMethods;
  ArrayAttr structureBody = declaration.array("body");
  for (size_t index = 0; index < structureBody.size(); ++index) {
    AstNode member = declaration.item("body", index);
    if (isModuleDocstring(member))
      continue;
    if (member.kind() != "FunctionDef")
      continue;
    StringRef methodName = member.string("name");
    ArrayAttr decorators = member.array("decorator_list");
    if (!decorators || decorators.size() != 1)
      return sourceCompiler.emitError()
             << "nested module functions require exactly one @rule decorator";
    AstNode decorator = member.item("decorator_list", 0);
    if (decorator.kind() != "Name" ||
        !sourceCompiler.isCompilerDecorator(decorator.string("id"), "rule"))
      return sourceCompiler.emitError()
             << "nested module function does not resolve to pycircuit.rule";
    if (!ruleMethods.try_emplace(methodName, member).second)
      return sourceCompiler.emitError() << "duplicate lexical @rule name";
  }

  auto signature =
      parseFunctionSignature(model.declaration.child("args"), false,
                             sourceCompiler.emitError, "module function");
  if (failed(signature))
    return failure();
  for (size_t index = 0; index < signature->parameters.size(); ++index) {
    const ParameterSyntax &syntax = signature->parameters[index];
    if (!syntax.parameter.child("annotation"))
      return sourceCompiler.emitError()
             << "module function parameters require explicit types";
    auto type = sourceCompiler.annotation(syntax.parameter.child("annotation"));
    if (failed(type))
      return failure();
    DictionaryAttr defaultValue = absentDefault(sourceCompiler.builder);
    if (syntax.defaultValue) {
      Attribute raw = syntax.defaultValue->get("value");
      DictionaryAttr value =
          staticValue(sourceCompiler.builder, raw, sourceCompiler.emitError);
      if (!value)
        return sourceCompiler.emitError() << "U02-A module parameter default "
                                             "must be a bool/integer literal";
      defaultValue = sourceCompiler.builder.getDictionaryAttr({
          sourceCompiler.builder.getNamedAttr(
              "present", sourceCompiler.builder.getBoolAttr(true)),
          sourceCompiler.builder.getNamedAttr("value", value),
      });
      auto resolver = [&](FlatSymbolRefAttr symbol) {
        return sourceCompiler.headers.resolveRecord(symbol);
      };
      if (failed(ac::detail::verifyDefaultMatchesType(
              defaultValue, *type, ac::detail::ExpectedTypeKind::Logical,
              resolver, sourceCompiler.emitError)))
        return failure();
    }
    StringRef name = syntax.parameter.string("arg");
    model.parameters.push_back({syntax, name.str(),
                                static_cast<unsigned>(index), "connection",
                                *type, defaultValue});
    ModuleMember connection;
    connection.kind = ModuleMember::Kind::Connection;
    connection.name = name.str();
    connection.parameter = name.str();
    connection.declaration = syntax.parameter;
    connection.logicalType = *type;
    connection.payloadType =
        physicalType(*type, sourceCompiler.builder.getContext());
    model.members.push_back(std::move(connection));
  }

  llvm::StringSet<> memberNames;
  for (const ModuleParameter &parameter : model.parameters)
    memberNames.insert(parameter.name);
  for (size_t index = 0; index < structureBody.size(); ++index) {
    AstNode statement = model.declaration.item("body", index);
    if (isModuleDocstring(statement))
      continue;
    if (statement.kind() == "FunctionDef")
      continue;
    if (statement.kind() == "Expr") {
      AstNode call = statement.child("value");
      if (call.kind() != "Call" || call.child("func").kind() != "Name")
        return sourceCompiler.emitError()
               << "module structure expression must register a lexical rule";
      StringRef ruleName = call.child("func").string("id");
      auto method = ruleMethods.find(ruleName);
      if (method == ruleMethods.end())
        return sourceCompiler.emitError()
               << "module structure call is not a nested @rule";
      if (!call.array("args").empty() || !call.array("keywords").empty())
        return sourceCompiler.emitError()
               << "lexical rule registration takes no arguments";
      size_t registrationIndex = model.registrations.size();
      model.registrations.push_back({call, method->second, {}, {}});
      model.actions.push_back(
          {ModuleAction::Kind::RuleRegistration, registrationIndex, statement});
      continue;
    }
    if (statement.kind() != "Assign" && statement.kind() != "AnnAssign")
      return sourceCompiler.emitError()
             << "module structure accepts declarations, aliases, child "
                "instances and lexical rule registrations";
    AstNode target;
    if (statement.kind() == "Assign") {
      ArrayAttr targets = statement.array("targets");
      if (!targets || targets.size() != 1)
        return sourceCompiler.emitError()
               << "module structure assignment requires one name target";
      target = statement.item("targets", 0);
    } else {
      target = statement.child("target");
    }
    if (target.kind() != "Name")
      return sourceCompiler.emitError()
             << "module structure assignments require lexical name targets";
    StringRef memberName = target.string("id");
    if (statement.kind() == "AnnAssign") {
      if (!memberNames.insert(memberName).second)
        return sourceCompiler.emitError()
               << "module lexical identity is declared more than once";
      if (!statement.get("value") || isa<UnitAttr>(statement.get("value")))
        return sourceCompiler.emitError()
               << "owned state requires an initializer in this module subset";
      AstNode annotation = statement.child("annotation");
      bool isCollection = annotation.kind() == "Subscript" &&
                          annotation.child("value").kind() == "Name" &&
                          annotation.child("value").string("id") == "list";
      AstNode elementAnnotation =
          isCollection ? annotation.child("slice") : annotation;
      auto logical = sourceCompiler.annotation(elementAnnotation);
      if (failed(logical))
        return failure();
      AstNode initializer = statement.child("value");
      ModuleMember state;
      state.kind = ModuleMember::Kind::OwnedState;
      state.name = memberName.str();
      state.declaration = statement;
      state.logicalType = *logical;
      if (isCollection) {
        auto kind = (*logical).getAs<StringAttr>("kind");
        if (!kind ||
            (kind.getValue() != "bool" && kind.getValue() != "integer" &&
             kind.getValue() != "record"))
          return sourceCompiler.emitError()
                 << "concrete reg collection element type must be a supported "
                    "finite logical type";
        if (initializer.kind() != "List" || initializer.array("elts").empty())
          return sourceCompiler.emitError()
                 << "concrete reg collection requires a non-empty list literal "
                    "initializer";
        SmallVector<Attribute> initialElements;
        for (size_t element = 0; element < initializer.array("elts").size();
             ++element) {
          auto expression = staticExpression(
              model, initializer.item("elts", element), *logical);
          if (failed(expression))
            return failure();
          initialElements.push_back(*expression);
        }
        auto count = parseStaticInteger(sourceCompiler.builder,
                                        Twine(initialElements.size()).str(),
                                        sourceCompiler.emitError);
        if (failed(count))
          return failure();
        DictionaryAttr countValue = sourceCompiler.builder.getDictionaryAttr({
            sourceCompiler.builder.getNamedAttr(
                "kind", sourceCompiler.builder.getStringAttr("integer")),
            sourceCompiler.builder.getNamedAttr("value", *count),
        });
        auto shapeExpression = ac::StaticExprAttr::get(
            sourceCompiler.builder.getContext(),
            sourceCompiler.builder.getDictionaryAttr({
                sourceCompiler.builder.getNamedAttr(
                    "kind", sourceCompiler.builder.getStringAttr("literal")),
                sourceCompiler.builder.getNamedAttr("value", countValue),
                sourceCompiler.builder.getNamedAttr(
                    "origin", occurrence(sourceCompiler.builder, model.symbol,
                                         relativeToModule(model.declaration,
                                                          initializer))),
                sourceCompiler.builder.getNamedAttr(
                    "location",
                    sourceSpan(sourceCompiler.builder,
                               sourceCompiler.source.path, initializer)),
            }));
        state.shape = sourceCompiler.builder.getArrayAttr({shapeExpression});
        state.initialValue = sourceCompiler.builder.getDictionaryAttr({
            sourceCompiler.builder.getNamedAttr(
                "kind", sourceCompiler.builder.getStringAttr("elements")),
            sourceCompiler.builder.getNamedAttr(
                "values", sourceCompiler.builder.getArrayAttr(initialElements)),
        });
      } else {
        auto expression = staticExpression(model, initializer, *logical);
        if (failed(expression))
          return failure();
        state.initialValue = sourceCompiler.builder.getDictionaryAttr({
            sourceCompiler.builder.getNamedAttr(
                "kind", sourceCompiler.builder.getStringAttr("scalar")),
            sourceCompiler.builder.getNamedAttr("value", *expression),
        });
      }
      state.payloadType =
          physicalType(*logical, sourceCompiler.builder.getContext());
      size_t memberIndex = model.members.size();
      model.members.push_back(std::move(state));
      model.actions.push_back(
          {ModuleAction::Kind::Member, memberIndex, statement});
      continue;
    }

    AstNode value = statement.child("value");
    if (value.kind() == "Name") {
      if (!memberNames.insert(memberName).second)
        return sourceCompiler.emitError()
               << "module lexical identity is declared more than once";
      auto sourceMember =
          llvm::find_if(model.members, [&](const ModuleMember &entry) {
            return entry.name == value.string("id");
          });
      if (sourceMember == model.members.end())
        return sourceCompiler.emitError()
               << "module alias must name a previously declared reg or "
                  "connection";
      if (sourceMember->kind == ModuleMember::Kind::ChildInstance)
        return sourceCompiler.emitError()
               << "module alias cannot name a child instance";
      size_t sourceIndex =
          static_cast<size_t>(sourceMember - model.members.begin());
      size_t canonicalIndex =
          sourceMember->canonicalMemberIndex.value_or(sourceIndex);
      if (canonicalIndex >= model.members.size() ||
          model.members[canonicalIndex].canonicalMemberIndex)
        return sourceCompiler.emitError()
               << "module alias has a dangling or cyclic canonical identity";
      const ModuleMember &canonical = model.members[canonicalIndex];
      if (canonical.isCollection())
        return sourceCompiler.emitError()
               << "concrete reg collection '" << canonical.name
               << "' cannot be aliased before exact element lowering is "
                  "available";
      ModuleMember alias;
      alias.kind = canonical.kind;
      alias.name = memberName.str();
      alias.declaration = statement;
      alias.parameter = canonical.parameter;
      alias.logicalType = canonical.logicalType;
      alias.canonicalMemberIndex = canonicalIndex;
      alias.payloadType = canonical.payloadType;
      model.members.push_back(std::move(alias));
      continue;
    }
    if (value.kind() != "Call")
      return sourceCompiler.emitError()
             << "module assignment must be an alias or child instance";

    AstNode callee = value.child("func");
    if (callee.kind() != "Name")
      return sourceCompiler.emitError()
             << "module child constructor must use an imported module name";
    if (!memberNames.insert(memberName).second)
      return sourceCompiler.emitError()
             << "module lexical identity is declared more than once";
    auto binding = sourceCompiler.namespaceBindings.find(callee.string("id"));
    if (binding == sourceCompiler.namespaceBindings.end())
      return sourceCompiler.emitError()
             << "module child constructor name is not imported from a header";
    Operation *childHeader =
        sourceCompiler.lookupCanonicalDeclaration(binding->second.target);
    if (!childHeader ||
        childHeader->getName().getStringRef() != "ac.module.import")
      return sourceCompiler.emitError()
             << "module child constructor does not resolve to ac.module.import";
    ModuleMember child;
    child.kind = ModuleMember::Kind::ChildInstance;
    child.name = memberName.str();
    child.declaration = statement;
    child.child = binding->second.target;
    child.childHeader = childHeader;
    if (failed(classifyInstanceArguments(model, child)))
      return failure();
    size_t memberIndex = model.members.size();
    model.members.push_back(std::move(child));
    model.actions.push_back(
        {ModuleAction::Kind::Member, memberIndex, statement});
  }

  for (const ModuleParameter &parameter : model.parameters)
    if (llvm::none_of(model.members, [&](const ModuleMember &member) {
          return member.kind == ModuleMember::Kind::Connection &&
                 member.parameter == parameter.name;
        }))
      return sourceCompiler.emitError()
             << "module parameter is not bound to a connection";

  for (const auto &entry : ruleMethods)
    if (llvm::none_of(model.registrations, [&](const RuleRegistration &rule) {
          return rule.method.string("name") == entry.getKey();
        }))
      model.inactiveRuleMethods.push_back(entry.second);

  for (const ModuleMember &member : model.members) {
    if (!member.isCollection())
      continue;
    for (const auto &entry : ruleMethods)
      if (usesLexicalName(entry.second, member.name))
        return sourceCompiler.emitError()
               << "concrete reg collection '" << member.name
               << "' cannot be captured by a rule before exact element "
                  "lowering is available";
  }

  return model;
}

FailureOr<ac::StaticExprAttr>
ModuleCompiler::staticExpression(const ModuleModel &model,
                                 const AstNode &expression,
                                 DictionaryAttr expected) {
  OpBuilder &builder = sourceCompiler.builder;
  auto resolver = [&](FlatSymbolRefAttr symbol) {
    return sourceCompiler.headers.resolveRecord(symbol);
  };
  auto makeExpression = [&](StringRef kind, ArrayRef<NamedAttribute> fields) {
    SmallVector<NamedAttribute> attributes;
    attributes.push_back(
        builder.getNamedAttr("kind", builder.getStringAttr(kind)));
    llvm::append_range(attributes, fields);
    attributes.push_back(builder.getNamedAttr(
        "origin", occurrence(builder, model.symbol,
                             relativeToModule(model.declaration, expression))));
    attributes.push_back(builder.getNamedAttr(
        "location",
        sourceSpan(builder, sourceCompiler.source.path, expression)));
    return ac::StaticExprAttr::get(builder.getContext(),
                                   builder.getDictionaryAttr(attributes));
  };

  if (expression.kind() == "Constant") {
    DictionaryAttr value =
        staticValue(builder, expression.get("value"), sourceCompiler.emitError);
    if (!value)
      return sourceCompiler.emitError()
             << "module initializer literal must be bool or integer";
    if (failed(ac::detail::verifyStaticValueMatchesType(
            value, expected, ac::detail::ExpectedTypeKind::Logical, resolver,
            sourceCompiler.emitError)))
      return failure();
    NamedAttribute field = builder.getNamedAttr("value", value);
    return makeExpression("literal", ArrayRef<NamedAttribute>(field));
  }

  if (expression.kind() != "Call" || expression.child("func").kind() != "Name")
    return sourceCompiler.emitError() << "U02-A initializer supports static "
                                         "literals and record constructors";
  StringRef localName = expression.child("func").string("id");
  auto binding = sourceCompiler.namespaceBindings.find(localName);
  if (binding == sourceCompiler.namespaceBindings.end())
    return sourceCompiler.emitError()
           << "module initializer record constructor must resolve to a "
              "canonical declaration";
  auto record = dyn_cast_or_null<ac::StructOp>(
      sourceCompiler.lookupCanonicalDeclaration(binding->second.target));
  auto expectedKind = expected.getAs<StringAttr>("kind");
  auto expectedRecord = expected.getAs<FlatSymbolRefAttr>("symbol");
  if (!record || !expectedKind || expectedKind.getValue() != "record" ||
      !expectedRecord || expectedRecord != binding->second.target)
    return sourceCompiler.emitError() << "record initializer constructor does "
                                         "not match the declared state type";
  auto constructor = dyn_cast_or_null<func::FuncOp>(
      sourceCompiler.lookupCanonicalDeclaration(record.getConstructorAttr()));
  if (!constructor)
    return sourceCompiler.emitError()
           << "record initializer lacks its verified header constructor";
  ArrayAttr parameters = constructor->getAttrOfType<ArrayAttr>("ac.parameters");
  ArrayAttr positional = expression.array("args");
  ArrayAttr keywords = expression.array("keywords");
  SmallVector<bool> bound(parameters.size(), false);
  SmallVector<Attribute> arguments;
  if (positional.size() > parameters.size())
    return sourceCompiler.emitError()
           << "too many record initializer positional arguments";
  for (size_t index = 0; index < positional.size(); ++index) {
    auto parameter = cast<DictionaryAttr>(parameters[index]);
    if (parameter.getAs<StringAttr>("binding").getValue() == "keyword_only")
      return sourceCompiler.emitError()
             << "keyword-only record initializer argument passed positionally";
    DictionaryAttr constraint = parameter.getAs<DictionaryAttr>("constraint");
    DictionaryAttr type = constraint.getAs<DictionaryAttr>("type");
    auto value = staticExpression(model, expression.item("args", index), type);
    if (failed(value))
      return failure();
    bound[index] = true;
    arguments.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("positional")),
        builder.getNamedAttr("value", *value),
    }));
  }
  for (size_t index = 0; index < keywords.size(); ++index) {
    AstNode keyword = expression.item("keywords", index);
    StringRef name = keyword.string("arg");
    if (name.empty())
      return sourceCompiler.emitError()
             << "record initializer keyword unpacking is unsupported";
    size_t parameterIndex = 0;
    while (parameterIndex < parameters.size() &&
           cast<DictionaryAttr>(parameters[parameterIndex])
                   .getAs<StringAttr>("name")
                   .getValue() != name)
      ++parameterIndex;
    if (parameterIndex == parameters.size())
      return sourceCompiler.emitError()
             << "unknown record initializer keyword '" << name << "'";
    auto parameter = cast<DictionaryAttr>(parameters[parameterIndex]);
    if (parameter.getAs<StringAttr>("binding").getValue() == "positional_only")
      return sourceCompiler.emitError() << "positional-only record initializer "
                                           "parameter passed by keyword";
    if (bound[parameterIndex])
      return sourceCompiler.emitError()
             << "duplicate record initializer argument '" << name << "'";
    DictionaryAttr constraint = parameter.getAs<DictionaryAttr>("constraint");
    DictionaryAttr type = constraint.getAs<DictionaryAttr>("type");
    auto value = staticExpression(model, keyword.child("value"), type);
    if (failed(value))
      return failure();
    bound[parameterIndex] = true;
    arguments.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("keyword")),
        builder.getNamedAttr("name", builder.getStringAttr(name)),
        builder.getNamedAttr("value", *value),
    }));
  }
  for (size_t index = 0; index < parameters.size(); ++index) {
    if (bound[index])
      continue;
    auto parameter = cast<DictionaryAttr>(parameters[index]);
    auto defaultValue = parameter.getAs<DictionaryAttr>("default");
    if (!defaultValue.getAs<BoolAttr>("present").getValue())
      return sourceCompiler.emitError()
             << "missing record initializer argument '"
             << parameter.getAs<StringAttr>("name").getValue() << "'";
  }
  SmallVector<NamedAttribute> fields{
      builder.getNamedAttr("callee", record.getConstructorAttr()),
      builder.getNamedAttr("arguments", builder.getArrayAttr(arguments)),
  };
  return makeExpression("call", fields);
}

LogicalResult ModuleCompiler::classifyInstanceArguments(ModuleModel &parent,
                                                        ModuleMember &member) {
  if (auto rootKind =
          member.childHeader->getAttrOfType<StringAttr>("ac.root_kind"))
    return sourceCompiler.emitError()
           << "system definition '" << member.child.getValue()
           << "' cannot be instantiated as a child module";
  auto contract =
      member.childHeader->getAttrOfType<DictionaryAttr>("ac.contract");
  auto parameters =
      contract ? contract.getAs<ArrayAttr>("parameters") : ArrayAttr();
  auto connections =
      contract ? contract.getAs<ArrayAttr>("connections") : ArrayAttr();
  if (!parameters || !connections)
    return sourceCompiler.emitError()
           << "module instance header has no verified ac.contract";
  if (failed(validateChildParameterCategories(parameters,
                                              sourceCompiler.emitError)))
    return failure();
  AstNode call = member.declaration.child("value");
  ArrayAttr positional = call.array("args");
  ArrayAttr keywords = call.array("keywords");
  if (positional.size() > parameters.size())
    return sourceCompiler.emitError()
           << "too many module instance positional arguments";
  SmallVector<AstNode> actuals(parameters.size());
  SmallVector<bool> explicitlyBound(parameters.size(), false);
  for (size_t index = 0; index < positional.size(); ++index) {
    auto parameter = cast<DictionaryAttr>(parameters[index]);
    if (parameter.getAs<StringAttr>("binding").getValue() == "keyword_only")
      return sourceCompiler.emitError()
             << "keyword-only module parameter passed positionally";
    actuals[index] = call.item("args", index);
    explicitlyBound[index] = true;
  }
  for (size_t index = 0; index < keywords.size(); ++index) {
    AstNode keyword = call.item("keywords", index);
    StringRef name = keyword.string("arg");
    if (name.empty())
      return sourceCompiler.emitError()
             << "module instance keyword unpacking is unsupported";
    size_t parameterIndex = 0;
    while (parameterIndex < parameters.size() &&
           cast<DictionaryAttr>(parameters[parameterIndex])
                   .getAs<StringAttr>("name")
                   .getValue() != name)
      ++parameterIndex;
    if (parameterIndex == parameters.size())
      return sourceCompiler.emitError()
             << "unknown module instance keyword '" << name << "'";
    auto parameter = cast<DictionaryAttr>(parameters[parameterIndex]);
    if (parameter.getAs<StringAttr>("binding").getValue() == "positional_only")
      return sourceCompiler.emitError()
             << "positional-only module parameter passed by keyword";
    if (explicitlyBound[parameterIndex])
      return sourceCompiler.emitError()
             << "duplicate module instance argument '" << name << "'";
    actuals[parameterIndex] = keyword.child("value");
    explicitlyBound[parameterIndex] = true;
  }

  llvm::StringMap<size_t> memberByName;
  for (size_t index = 0; index < parent.members.size(); ++index) {
    const ModuleMember &candidate = parent.members[index];
    if (candidate.kind != ModuleMember::Kind::ChildInstance)
      memberByName[candidate.name] = index;
  }
  for (Attribute rawConnection : connections) {
    auto connection = dyn_cast<DictionaryAttr>(rawConnection);
    auto parameterName =
        connection ? connection.getAs<StringAttr>("parameter") : StringAttr();
    auto elements =
        connection ? connection.getAs<ArrayAttr>("elements") : ArrayAttr();
    if (!parameterName || !elements || elements.size() != 1)
      return sourceCompiler.emitError()
             << "U02-A module instance supports scalar/record connections";
    size_t parameterIndex = 0;
    while (parameterIndex < parameters.size() &&
           cast<DictionaryAttr>(parameters[parameterIndex])
                   .getAs<StringAttr>("name")
                   .getValue() != parameterName.getValue())
      ++parameterIndex;
    if (parameterIndex == parameters.size())
      return sourceCompiler.emitError()
             << "module connection has no matching constructor parameter";
    auto parameter = cast<DictionaryAttr>(parameters[parameterIndex]);
    auto category = parameter.getAs<StringAttr>("category");
    auto expectedType = parameter.getAs<DictionaryAttr>("type");
    if (!category || category.getValue() != "connection" || !expectedType)
      return sourceCompiler.emitError()
             << "U02-A module child static parameters are not supported";
    if (!explicitlyBound[parameterIndex])
      return sourceCompiler.emitError()
             << "module connection requires an explicit handle argument; value "
                "defaults do not allocate state";
    AstNode actual = actuals[parameterIndex];
    size_t parentMemberIndex = parent.members.size();
    if (actual.kind() == "Name") {
      auto found = memberByName.find(actual.string("id"));
      if (found != memberByName.end())
        parentMemberIndex = found->second;
    }
    if (parentMemberIndex == parent.members.size())
      return sourceCompiler.emitError()
             << "module child connection actual must name a previously "
                "declared reg or connection alias through an approved lexical "
                "Name or projection";
    parentMemberIndex =
        parent.members[parentMemberIndex].canonicalMemberIndex.value_or(
            parentMemberIndex);
    if (parentMemberIndex >= parent.members.size() ||
        parent.members[parentMemberIndex].canonicalMemberIndex)
      return sourceCompiler.emitError()
             << "module child connection actual has a dangling or cyclic "
                "canonical alias";
    ModuleMember &parentMember = parent.members[parentMemberIndex];
    if (parentMember.isCollection())
      return sourceCompiler.emitError()
             << "concrete reg collection '" << parentMember.name
             << "' cannot bind a child connection before exact element "
                "lowering is available";
    if (!parentMember.logicalType || parentMember.logicalType != expectedType)
      return sourceCompiler.emitError()
             << "module child connection logical type "
                "does not match its parent actual";
    auto element = cast<DictionaryAttr>(elements[0]);
    auto read = element.getAs<BoolAttr>("read");
    auto write = element.getAs<BoolAttr>("write");
    auto precision = element.getAs<StringAttr>("precision");
    if (!read || !write || !precision)
      return sourceCompiler.emitError()
             << "module child connection effect is malformed";
    parentMember.read |= read.getValue();
    parentMember.write |= write.getValue();
    if (precision.getValue() == "conservative")
      parentMember.precision = "conservative";
    if (read.getValue()) {
      parentMember.readSites.push_back(call);
      member.childInputMembers.push_back(parentMemberIndex);
    }
    if (write.getValue()) {
      parentMember.writeSites.push_back(call);
      member.childOutputMembers.push_back(parentMemberIndex);
    }
  }
  return success();
}

} // namespace acir::compiler::detail
