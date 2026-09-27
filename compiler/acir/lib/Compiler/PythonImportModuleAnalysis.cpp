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

bool RecordCompiler::isModuleDefinition(const AstNode &node) const {
  if (node.kind() != "ClassDef")
    return false;
  for (size_t index = 0; index < node.array("decorator_list").size(); ++index) {
    AstNode decorator = node.item("decorator_list", index);
    if (decorator.kind() == "Name" &&
        isCompilerDecorator(decorator.string("id"), "module"))
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
  if (!declaration.array("bases").empty() ||
      !declaration.array("keywords").empty() ||
      (declaration.array("type_params") &&
       !declaration.array("type_params").empty()))
    return sourceCompiler.emitError() << "@module classes reject inheritance, "
                                         "metaclasses and type parameters";
  ArrayAttr moduleDecorators = declaration.array("decorator_list");
  if (!moduleDecorators || moduleDecorators.size() != 1)
    return sourceCompiler.emitError()
           << "@module classes require exactly one imported @module decorator";
  AstNode moduleDecorator = declaration.item("decorator_list", 0);
  if (moduleDecorator.kind() != "Name" ||
      !sourceCompiler.isCompilerDecorator(moduleDecorator.string("id"),
                                          "module"))
    return sourceCompiler.emitError()
           << "module class decorator does not resolve to pycircuit.module";

  ModuleModel model;
  model.declaration = declaration;
  model.symbol = FlatSymbolRefAttr::get(
      sourceCompiler.builder.getContext(),
      sourceCompiler.qualifiedName(declaration.string("name")));

  llvm::StringMap<AstNode> ruleMethods;
  ArrayAttr classBody = declaration.array("body");
  for (size_t index = 0; index < classBody.size(); ++index) {
    AstNode member = declaration.item("body", index);
    if (isModuleDocstring(member))
      continue;
    if (member.kind() != "FunctionDef")
      return sourceCompiler.emitError()
             << "@module class body permits methods only";
    StringRef methodName = member.string("name");
    ArrayAttr decorators = member.array("decorator_list");
    if (methodName == "__init__") {
      if (model.constructor)
        return sourceCompiler.emitError()
               << "@module class defines more than one __init__";
      if (decorators && !decorators.empty())
        return sourceCompiler.emitError()
               << "module constructor cannot be decorated";
      model.constructor = member;
      continue;
    }
    if (!decorators || decorators.empty())
      continue;
    if (decorators.size() != 1)
      return sourceCompiler.emitError()
             << "module method supports only one @rule decorator";
    AstNode decorator = member.item("decorator_list", 0);
    if (decorator.kind() != "Name" ||
        !sourceCompiler.isCompilerDecorator(decorator.string("id"), "rule"))
      return sourceCompiler.emitError()
             << "module method decorator does not resolve to pycircuit.rule";
    if (!ruleMethods.try_emplace(methodName, member).second)
      return sourceCompiler.emitError() << "duplicate @rule method name";
  }
  if (!model.constructor)
    return sourceCompiler.emitError()
           << "@module class requires exactly one __init__ constructor";

  auto signature =
      parseFunctionSignature(model.constructor.child("args"), true,
                             sourceCompiler.emitError, "module constructor");
  if (failed(signature))
    return failure();
  llvm::StringMap<size_t> parameterByName;
  for (size_t index = 0; index < signature->parameters.size(); ++index) {
    const ParameterSyntax &syntax = signature->parameters[index];
    if (!syntax.parameter.child("annotation"))
      return sourceCompiler.emitError()
             << "module constructor parameters require explicit types";
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
    parameterByName[name] = index;
    model.parameters.push_back({syntax, name.str(),
                                static_cast<unsigned>(index), "connection",
                                *type, defaultValue});
  }

  llvm::StringSet<> memberNames;
  ArrayAttr constructorBody = model.constructor.array("body");
  for (size_t index = 0; index < constructorBody.size(); ++index) {
    AstNode statement = model.constructor.item("body", index);
    if (isModuleDocstring(statement))
      continue;
    if (statement.kind() != "Assign" && statement.kind() != "AnnAssign")
      return sourceCompiler.emitError()
             << "U02-A module constructor accepts member bindings and rule "
                "registrations";
    AstNode target;
    if (statement.kind() == "Assign") {
      ArrayAttr targets = statement.array("targets");
      if (!targets || targets.size() != 1)
        return sourceCompiler.emitError()
               << "module constructor assignment requires one member target";
      target = statement.item("targets", 0);
    } else {
      target = statement.child("target");
    }
    StringRef memberName;
    if (!isModuleSelfMember(target, &memberName))
      return sourceCompiler.emitError()
             << "module constructor assignments require self.member targets";
    if (statement.kind() == "AnnAssign") {
      if (!memberNames.insert(memberName).second)
        return sourceCompiler.emitError()
               << "module constructor member identity "
                  "is declared more than once";
      if (!statement.get("value") || isa<UnitAttr>(statement.get("value")))
        return sourceCompiler.emitError()
               << "owned state requires an initializer in this module subset";
      auto logical = sourceCompiler.annotation(statement.child("annotation"));
      if (failed(logical))
        return failure();
      AstNode initializer = statement.child("value");
      auto expression = staticExpression(model, initializer, *logical);
      if (failed(expression))
        return failure();
      ModuleMember state;
      state.kind = ModuleMember::Kind::OwnedState;
      state.name = memberName.str();
      state.declaration = statement;
      state.logicalType = *logical;
      state.initialValue = sourceCompiler.builder.getDictionaryAttr({
          sourceCompiler.builder.getNamedAttr(
              "kind", sourceCompiler.builder.getStringAttr("scalar")),
          sourceCompiler.builder.getNamedAttr("value", *expression),
      });
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
               << "module constructor member identity "
                  "is declared more than once";
      auto parameter = parameterByName.find(value.string("id"));
      if (parameter == parameterByName.end())
        return sourceCompiler.emitError()
               << "U02-A connection alias must name a "
                  "module constructor parameter";
      ModuleParameter &formal = model.parameters[parameter->second];
      ModuleMember connection;
      connection.kind = ModuleMember::Kind::Connection;
      connection.name = memberName.str();
      connection.parameter = formal.name;
      connection.declaration = statement;
      connection.logicalType = formal.type;
      connection.payloadType =
          physicalType(formal.type, sourceCompiler.builder.getContext());
      size_t memberIndex = model.members.size();
      model.members.push_back(std::move(connection));
      model.actions.push_back(
          {ModuleAction::Kind::Member, memberIndex, statement});
      continue;
    }
    if (value.kind() != "Call")
      return sourceCompiler.emitError()
             << "U02-A constructor member binding must be a connection, state, "
                "rule, or child module";

    AstNode callee = value.child("func");
    if (callee.kind() == "Attribute" &&
        callee.child("value").kind() == "Name" &&
        callee.child("value").string("id") == "self") {
      auto method = ruleMethods.find(callee.string("attr"));
      if (method == ruleMethods.end())
        return sourceCompiler.emitError()
               << "constructor call target is not a declared @rule method";
      AstNode outputTarget = target;
      size_t registrationIndex = model.registrations.size();
      model.registrations.push_back(
          {value, method->second, outputTarget, memberName.str()});
      model.actions.push_back(
          {ModuleAction::Kind::RuleRegistration, registrationIndex, statement});
      auto outputMember =
          llvm::find_if(model.members, [&](ModuleMember &entry) {
            return entry.name == memberName;
          });
      if (outputMember == model.members.end())
        return sourceCompiler.emitError()
               << "rule output target must be previously declared";
      outputMember->write = true;
      outputMember->writeSites.push_back(outputTarget);
      continue;
    }

    if (callee.kind() != "Name")
      return sourceCompiler.emitError()
             << "module child constructor must use an imported module name";
    if (!memberNames.insert(memberName).second)
      return sourceCompiler.emitError()
             << "module constructor member identity is declared more than once";
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
             << "module constructor parameter is not "
                "bound to a connection member";

  for (const auto &entry : ruleMethods)
    if (llvm::none_of(model.registrations, [&](const RuleRegistration &rule) {
          return rule.method.string("name") == entry.getKey();
        }))
      model.inactiveRuleMethods.push_back(entry.second);

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

  llvm::StringMap<size_t> memberByParameter;
  for (size_t index = 0; index < parent.members.size(); ++index) {
    const ModuleMember &candidate = parent.members[index];
    if (candidate.kind == ModuleMember::Kind::Connection)
      memberByParameter[candidate.parameter] = index;
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
    StringRef actualName;
    size_t parentMemberIndex = parent.members.size();
    if (isModuleSelfMember(actual, &actualName)) {
      for (size_t index = 0; index < parent.members.size(); ++index)
        if (parent.members[index].name == actualName) {
          parentMemberIndex = index;
          break;
        }
    } else if (actual.kind() == "Name") {
      auto found = memberByParameter.find(actual.string("id"));
      if (found != memberByParameter.end())
        parentMemberIndex = found->second;
    }
    if (parentMemberIndex == parent.members.size())
      return sourceCompiler.emitError()
             << "module child connection actual must name a previously "
                "declared self member or connection parameter";
    ModuleMember &parentMember = parent.members[parentMemberIndex];
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
