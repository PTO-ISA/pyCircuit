#include "PythonImportAST.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/Location.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {

LogicalResult verifyCapturedSpan(DictionaryAttr span,
                                 ac::detail::EmitError emitError) {
  if (!span || span.size() != 6)
    return emitError() << "captured AST span must contain exactly six fields";
  constexpr StringLiteral names[] = {
      "start_line", "start_byte_column", "start_codepoint_column",
      "end_line",   "end_byte_column",   "end_codepoint_column"};
  uint64_t values[6];
  for (auto [index, name] : llvm::enumerate(names)) {
    auto value =
        ac::detail::decodeU64(span.getAs<IntegerAttr>(name), name, emitError);
    if (failed(value) || *value == 0)
      return failed(value)
                 ? failure()
                 : emitError() << "captured AST span coordinates are one-based";
    values[index] = *value;
  }
  if (values[3] < values[0] ||
      (values[3] == values[0] &&
       (values[4] < values[1] || values[5] < values[2])))
    return emitError() << "captured AST span ends before it starts";
  return success();
}

LogicalResult verifyCapturedNode(Attribute raw,
                                 ac::detail::EmitError emitError);

LogicalResult verifyCapturedField(Attribute value,
                                  ac::detail::EmitError emitError) {
  if (isa<StringAttr, BoolAttr, UnitAttr>(value))
    return success();
  if (auto values = dyn_cast<ArrayAttr>(value)) {
    for (auto [index, element] : llvm::enumerate(values))
      if (failed(verifyCapturedField(element, emitError))) {
        emitError() << "captured AST array element[" << index << "] is invalid";
        return failure();
      }
    return success();
  }
  auto dictionary = dyn_cast<DictionaryAttr>(value);
  if (!dictionary)
    return emitError()
           << "captured AST field has an unsupported attribute kind";
  if (dictionary.size() == 1 && dictionary.getAs<StringAttr>("integer"))
    return success();
  return verifyCapturedNode(dictionary, emitError);
}

LogicalResult verifyRequiredNode(DictionaryAttr fields, StringRef name,
                                 StringRef kind,
                                 ac::detail::EmitError emitError) {
  auto value = fields.getAs<DictionaryAttr>(name);
  if (!value || !value.getAs<StringAttr>("kind"))
    return emitError() << "captured " << kind << " field '" << name
                       << "' must be an AST node";
  return success();
}

LogicalResult verifyRequiredArray(DictionaryAttr fields, StringRef name,
                                  StringRef kind,
                                  ac::detail::EmitError emitError) {
  if (!fields.getAs<ArrayAttr>(name))
    return emitError() << "captured " << kind << " field '" << name
                       << "' must be an ArrayAttr";
  return success();
}

LogicalResult verifyNodeArray(DictionaryAttr fields, StringRef name,
                              StringRef kind, StringRef elementKind,
                              bool nonempty, ac::detail::EmitError emitError) {
  if (failed(verifyRequiredArray(fields, name, kind, emitError)))
    return failure();
  auto elements = fields.getAs<ArrayAttr>(name);
  if (nonempty && elements.empty())
    return emitError() << "captured " << kind << " field '" << name
                       << "' must not be empty";
  for (Attribute element : elements) {
    auto node = dyn_cast<DictionaryAttr>(element);
    auto form = node ? node.getAs<StringAttr>("kind") : StringAttr();
    if (!form || (!elementKind.empty() && form.getValue() != elementKind))
      return emitError() << "captured " << kind << " field '" << name
                         << "' must contain "
                         << (elementKind.empty() ? "AST nodes" : elementKind);
  }
  return success();
}

LogicalResult verifyOptionalArray(DictionaryAttr fields, StringRef name,
                                  StringRef kind,
                                  ac::detail::EmitError emitError) {
  Attribute value = fields.get(name);
  if (!value || isa<ArrayAttr>(value))
    return success();
  return emitError() << "captured " << kind << " field '" << name
                     << "' must be an ArrayAttr when present";
}

LogicalResult verifyStringArray(DictionaryAttr fields, StringRef name,
                                StringRef kind,
                                ac::detail::EmitError emitError) {
  auto values = fields.getAs<ArrayAttr>(name);
  if (!values)
    return emitError() << "captured " << kind << " field '" << name
                       << "' must be an ArrayAttr";
  for (Attribute value : values)
    if (!isa<StringAttr>(value))
      return emitError() << "captured " << kind << " field '" << name
                         << "' must contain strings";
  return success();
}

LogicalResult verifyOptionalNode(DictionaryAttr fields, StringRef name,
                                 StringRef kind,
                                 ac::detail::EmitError emitError) {
  Attribute value = fields.get(name);
  if (!value)
    return emitError() << "captured " << kind << " field '" << name
                       << "' is missing";
  if (isa<UnitAttr>(value))
    return success();
  auto node = dyn_cast<DictionaryAttr>(value);
  if (!node || !node.getAs<StringAttr>("kind"))
    return emitError() << "captured " << kind << " field '" << name
                       << "' must be an AST node or UnitAttr";
  return success();
}

LogicalResult verifyStringOrUnit(DictionaryAttr fields, StringRef name,
                                 StringRef kind,
                                 ac::detail::EmitError emitError) {
  Attribute value = fields.get(name);
  if (!value || (!isa<StringAttr>(value) && !isa<UnitAttr>(value)))
    return emitError() << "captured " << kind << " field '" << name
                       << "' must be a StringAttr or UnitAttr";
  return success();
}

LogicalResult verifyEncodedInteger(DictionaryAttr fields, StringRef name,
                                   StringRef kind,
                                   ac::detail::EmitError emitError) {
  auto value = fields.getAs<DictionaryAttr>(name);
  if (!value || value.size() != 1 || !value.getAs<StringAttr>("integer"))
    return emitError() << "captured " << kind << " field '" << name
                       << "' must be an encoded integer";
  return success();
}

LogicalResult verifyCapturedNode(Attribute raw,
                                 ac::detail::EmitError emitError) {
  auto node = dyn_cast<DictionaryAttr>(raw);
  if (!node || node.size() != 3)
    return emitError()
           << "captured AST node must contain kind, fields and span";
  auto kind = node.getAs<StringAttr>("kind");
  auto fields = node.getAs<DictionaryAttr>("fields");
  auto span = node.getAs<DictionaryAttr>("span");
  if (!kind || kind.getValue().empty() || !fields ||
      failed(verifyCapturedSpan(span, emitError)))
    return emitError() << "captured AST node header is malformed";
  for (NamedAttribute field : fields)
    if (failed(verifyCapturedField(field.getValue(), emitError)))
      return failure();

  StringRef form = kind.getValue();
  if (form == "Module")
    return verifyRequiredArray(fields, "body", form, emitError);
  if (form == "ClassDef" || form == "FunctionDef") {
    if (!fields.getAs<StringAttr>("name") ||
        failed(verifyRequiredArray(fields, "body", form, emitError)) ||
        failed(
            verifyRequiredArray(fields, "decorator_list", form, emitError)) ||
        failed(verifyOptionalArray(fields, "type_params", form, emitError)))
      return emitError() << "captured " << form << " shape is malformed";
    if (form == "ClassDef") {
      if (failed(verifyRequiredArray(fields, "bases", form, emitError)) ||
          failed(verifyRequiredArray(fields, "keywords", form, emitError)))
        return failure();
    } else if (failed(verifyRequiredNode(fields, "args", form, emitError)) ||
               failed(verifyOptionalNode(fields, "returns", form, emitError)))
      return failure();
  } else if (form == "Lambda") {
    if (fields.size() != 2 ||
        failed(verifyRequiredNode(fields, "args", form, emitError)) ||
        failed(verifyRequiredNode(fields, "body", form, emitError)))
      return emitError() << "captured Lambda shape is malformed";
    auto args = fields.getAs<DictionaryAttr>("args");
    if (args.getAs<StringAttr>("kind").getValue() != "arguments")
      return emitError() << "captured Lambda args must be arguments";
    auto body = fields.getAs<DictionaryAttr>("body");
    StringRef bodyKind = body.getAs<StringAttr>("kind").getValue();
    if (!llvm::is_contained(
            ArrayRef<StringRef>{"BoolOp", "NamedExpr", "BinOp", "UnaryOp",
                "Lambda", "IfExp", "Dict", "Set", "ListComp", "SetComp",
                "DictComp", "GeneratorExp", "Await", "Yield", "YieldFrom",
                "Compare", "Call", "FormattedValue", "JoinedStr", "Constant",
                "Attribute", "Subscript", "Starred", "Name", "List", "Tuple"},
            bodyKind))
      return emitError() << "captured Lambda body must be an expression";
    if (failed(validateTableQueryLambda(AstNode{node, {}}, emitError,
                                        std::nullopt)))
      return failure();
  } else if (form == "arguments") {
    if (failed(verifyRequiredArray(fields, "posonlyargs", form, emitError)) ||
        failed(verifyRequiredArray(fields, "args", form, emitError)) ||
        failed(verifyRequiredArray(fields, "kwonlyargs", form, emitError)) ||
        failed(verifyRequiredArray(fields, "kw_defaults", form, emitError)) ||
        failed(verifyRequiredArray(fields, "defaults", form, emitError)) ||
        failed(verifyOptionalNode(fields, "vararg", form, emitError)) ||
        failed(verifyOptionalNode(fields, "kwarg", form, emitError)))
      return failure();
  } else if (form == "ImportFrom") {
    if (failed(verifyStringOrUnit(fields, "module", form, emitError)) ||
        failed(verifyEncodedInteger(fields, "level", form, emitError)) ||
        failed(verifyRequiredArray(fields, "names", form, emitError)))
      return failure();
  } else if (form == "alias") {
    if (!fields.getAs<StringAttr>("name") ||
        failed(verifyStringOrUnit(fields, "asname", form, emitError)))
      return emitError() << "captured alias shape is malformed";
  } else if (form == "Assign") {
    if (failed(verifyRequiredArray(fields, "targets", form, emitError)) ||
        failed(verifyRequiredNode(fields, "value", form, emitError)))
      return failure();
  } else if (form == "AnnAssign") {
    if (failed(verifyRequiredNode(fields, "target", form, emitError)) ||
        failed(verifyRequiredNode(fields, "annotation", form, emitError)) ||
        failed(verifyOptionalNode(fields, "value", form, emitError)) ||
        failed(verifyEncodedInteger(fields, "simple", form, emitError)))
      return failure();
  } else if (form == "Call") {
    if (failed(verifyRequiredNode(fields, "func", form, emitError)) ||
        failed(verifyRequiredArray(fields, "args", form, emitError)) ||
        failed(verifyRequiredArray(fields, "keywords", form, emitError)))
      return failure();
  } else if (form == "keyword") {
    if (failed(verifyStringOrUnit(fields, "arg", form, emitError)) ||
        failed(verifyRequiredNode(fields, "value", form, emitError)))
      return emitError() << "captured keyword shape is malformed";
  } else if (form == "Attribute") {
    if (!fields.getAs<StringAttr>("attr") ||
        failed(verifyRequiredNode(fields, "value", form, emitError)))
      return emitError() << "captured Attribute shape is malformed";
  } else if (form == "Subscript") {
    if (failed(verifyRequiredNode(fields, "value", form, emitError)) ||
        failed(verifyRequiredNode(fields, "slice", form, emitError)))
      return failure();
  } else if (form == "Tuple") {
    if (failed(verifyRequiredArray(fields, "elts", form, emitError)))
      return failure();
  } else if (form == "Return") {
    if (failed(verifyOptionalNode(fields, "value", form, emitError)))
      return failure();
  } else if (form == "Expr") {
    if (failed(verifyRequiredNode(fields, "value", form, emitError)))
      return failure();
  } else if (form == "Nonlocal") {
    if (failed(verifyStringArray(fields, "names", form, emitError)))
      return failure();
  } else if (form == "If") {
    if (failed(verifyRequiredNode(fields, "test", form, emitError)) ||
        failed(verifyRequiredArray(fields, "body", form, emitError)) ||
        failed(verifyRequiredArray(fields, "orelse", form, emitError)))
      return failure();
  } else if (form == "UnaryOp") {
    if (failed(verifyRequiredNode(fields, "op", form, emitError)) ||
        failed(verifyRequiredNode(fields, "operand", form, emitError)))
      return failure();
  } else if (form == "Match") {
    if (fields.size() != 2 ||
        failed(verifyRequiredNode(fields, "subject", form, emitError)) ||
        failed(verifyNodeArray(fields, "cases", form, "match_case", true,
                               emitError)))
      return emitError() << "captured Match shape is malformed";
  } else if (form == "match_case") {
    if (fields.size() != 3 ||
        failed(verifyRequiredNode(fields, "pattern", form, emitError)) ||
        failed(verifyOptionalNode(fields, "guard", form, emitError)) ||
        failed(verifyNodeArray(fields, "body", form, {}, true, emitError)))
      return emitError() << "captured match_case shape is malformed";
  } else if (form == "MatchValue") {
    if (fields.size() != 1 ||
        failed(verifyRequiredNode(fields, "value", form, emitError)))
      return emitError() << "captured MatchValue shape is malformed";
  } else if (form == "MatchSingleton") {
    if (fields.size() != 1 ||
        !isa_and_nonnull<BoolAttr, UnitAttr>(fields.get("value")))
      return emitError() << "captured MatchSingleton shape is malformed";
  } else if (form == "MatchOr") {
    if (fields.size() != 1 ||
        failed(verifyNodeArray(fields, "patterns", form, {}, false, emitError)))
      return emitError() << "captured MatchOr shape is malformed";
  } else if (form == "MatchAs") {
    if (fields.size() != 2 ||
        failed(verifyOptionalNode(fields, "pattern", form, emitError)) ||
        failed(verifyStringOrUnit(fields, "name", form, emitError)))
      return emitError() << "captured MatchAs shape is malformed";
  } else if (form == "Name") {
    if (!fields.getAs<StringAttr>("id"))
      return emitError() << "captured Name id must be a StringAttr";
  } else if (form == "arg") {
    if (!fields.getAs<StringAttr>("arg") ||
        failed(verifyOptionalNode(fields, "annotation", form, emitError)))
      return emitError() << "captured arg shape is malformed";
  }
  return success();
}

} // namespace

StringRef AstNode::kind() const {
  auto kind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  return kind ? kind.getValue() : StringRef();
}

DictionaryAttr AstNode::fields() const {
  return value ? value.getAs<DictionaryAttr>("fields") : DictionaryAttr();
}

Attribute AstNode::get(StringRef name) const {
  DictionaryAttr data = fields();
  return data ? data.get(name) : Attribute();
}

AstNode AstNode::child(StringRef name) const {
  AstNode result{dyn_cast_or_null<DictionaryAttr>(get(name)), path};
  result.path.push_back({name.str(), std::nullopt});
  return result;
}

ArrayAttr AstNode::array(StringRef name) const {
  return dyn_cast_or_null<ArrayAttr>(get(name));
}

AstNode AstNode::item(StringRef name, size_t index) const {
  ArrayAttr values = array(name);
  if (!values || index >= values.size())
    return {};
  AstNode result{dyn_cast<DictionaryAttr>(values[index]), path};
  result.path.push_back({name.str(), std::nullopt});
  result.path.push_back({{}, static_cast<uint64_t>(index)});
  return result;
}

StringRef AstNode::string(StringRef name) const {
  auto value = dyn_cast_or_null<StringAttr>(get(name));
  return value ? value.getValue() : StringRef();
}

Location AstNode::location(MLIRContext *context, StringRef sourcePath) const {
  auto span = value ? value.getAs<DictionaryAttr>("span") : DictionaryAttr();
  auto line = span ? span.getAs<IntegerAttr>("start_line") : IntegerAttr();
  auto column =
      span ? span.getAs<IntegerAttr>("start_codepoint_column") : IntegerAttr();
  return FileLineColLoc::get(context, sourcePath,
                             line ? line.getValue().getZExtValue() : 1,
                             column ? column.getValue().getZExtValue() : 1);
}

bool sourceBindingShadowed(const CapturedSource &source, const AstNode &node,
                           StringRef name) {
  auto binds = [&](auto &&self, const AstNode &scope) -> bool {
    if (scope.kind() == "Name")
      return scope.string("id") == name;
    if (scope.kind() == "FunctionDef" || scope.kind() == "ClassDef")
      return scope.string("name") == name;
    if (scope.kind() == "Assign") {
      for (size_t i = 0; i < scope.array("targets").size(); ++i)
        if (self(self, scope.item("targets", i)))
          return true;
      return false;
    }
    if (scope.kind() == "AnnAssign" || scope.kind() == "AugAssign" ||
        scope.kind() == "NamedExpr" || scope.kind() == "For")
      if (self(self, scope.child("target")))
        return true;
    if (scope.kind() == "Tuple" || scope.kind() == "List") {
      for (size_t i = 0; i < scope.array("elts").size(); ++i)
        if (self(self, scope.item("elts", i)))
          return true;
      return false;
    }
    if (scope.kind() == "Match")
      for (size_t i = 0; i < scope.array("cases").size(); ++i)
        if (self(self, scope.item("cases", i)))
          return true;
    for (NamedAttribute field : scope.fields()) {
      if (field.getName() != "body" && field.getName() != "orelse")
        continue;
      auto children = dyn_cast<ArrayAttr>(field.getValue());
      for (size_t i = 0; children && i < children.size(); ++i)
        if (self(self, scope.item(field.getName().getValue(), i)))
          return true;
    }
    return false;
  };
  auto bindsLocally = [&](const AstNode &scope) -> bool {
    auto args = scope.child("args");
    for (StringRef field : {"args", "posonlyargs", "kwonlyargs"}) {
      auto arguments = args.array(field);
      for (size_t j = 0; arguments && j < arguments.size(); ++j)
        if (args.item(field, j).string("arg") == name)
          return true;
    }
    for (StringRef field : {"vararg", "kwarg"})
      if (args.child(field).string("arg") == name)
        return true;
    auto statements = scope.array("body");
    for (size_t j = 0; statements && j < statements.size(); ++j)
      if (binds(binds, scope.item("body", j)))
        return true;
    return false;
  };
  auto scopeBinds = [&](const AstNode &scope) -> bool {
    auto &bindings = source.scopeBindingCache[scope.value];
    auto cached = bindings.find(name);
    if (cached != bindings.end())
      return cached->second;
    bool result = bindsLocally(scope);
    bindings.try_emplace(name, result);
    return result;
  };
  AstNode ancestor = source.module;
  for (size_t i = 0; i < node.path.size(); ++i) {
    const AstStep &step = node.path[i];
    if (step.index)
      return false;
    if ((ancestor.kind() == "FunctionDef" || ancestor.kind() == "ClassDef" ||
         ancestor.kind() == "Lambda") && step.field == "body") {
      if (scopeBinds(ancestor))
        return true;
    }
    if (i + 1 < node.path.size() && node.path[i + 1].index) {
      ancestor = ancestor.item(step.field, *node.path[++i].index);
    } else
      ancestor = ancestor.child(step.field);
    if (!ancestor)
      return false;
  }
  return false;
}

LogicalResult validateTableQueryLambda(const AstNode &node,
                                       ac::detail::EmitError emitError,
                                       std::optional<size_t> arity) {
  if (node.kind() != "Lambda")
    return emitError() << "Table query callback requires an expression Lambda";
  auto args = node.child("args");
  if (args.kind() != "arguments" || !node.child("body"))
    return emitError() << "captured Lambda shape is malformed";
  auto positional = args.array("args"),
       positionalOnly = args.array("posonlyargs");
  auto keywords = args.array("kwonlyargs"), defaults = args.array("defaults");
  auto keywordDefaults = args.array("kw_defaults");
  if (!positional || !positionalOnly || !keywords || !defaults ||
      !keywordDefaults || !args.get("vararg") || !args.get("kwarg") ||
      (arity && positional.size() + positionalOnly.size() != *arity) ||
      !keywords.empty() || !defaults.empty() || !keywordDefaults.empty() ||
      args.child("vararg") || args.child("kwarg") ||
      !isa<UnitAttr>(args.get("vararg")) || !isa<UnitAttr>(args.get("kwarg")))
    return emitError()
           << (arity && *arity == 1
                   ? "Table query Lambda requires exactly one positional "
                     "argument without defaults or variadic/keyword arguments"
                   : "Table map Lambda requires one positional argument per "
                     "input without defaults or variadic/keyword arguments");
  llvm::StringSet<> names;
  for (StringRef field : {"posonlyargs", "args"})
    for (size_t i = 0; i < args.array(field).size(); ++i) {
      auto argument = args.item(field, i);
      if (argument.kind() != "arg" || argument.string("arg").empty() ||
          !isa_and_nonnull<UnitAttr>(argument.get("annotation")))
        return emitError() << "Table query Lambda argument must be unannotated";
      if (!names.insert(argument.string("arg")).second)
        return emitError() << "Table query Lambda arguments must be distinct";
    }
  return success();
}

FailureOr<CapturedSource> readSingleCapture(ModuleOp transport,
                                            ac::detail::EmitError emitError) {
  if (!transport.getBody()->empty())
    return emitError() << "source transport module body must be empty";
  auto files = transport->getAttrOfType<ArrayAttr>("ac.python_capture");
  if (!files || files.size() != 1)
    return emitError()
           << "ac.python_capture must contain exactly one source file";
  auto file = dyn_cast<DictionaryAttr>(files[0]);
  auto path = file ? file.getAs<StringAttr>("path") : StringAttr();
  auto body = file ? file.getAs<ArrayAttr>("body") : ArrayAttr();
  if (!file || file.size() != 2 || !path || !body || body.size() != 1)
    return emitError() << "captured source file record is malformed";
  auto root = dyn_cast<DictionaryAttr>(body[0]);
  if (!root || !root.getAs<StringAttr>("kind") ||
      root.getAs<StringAttr>("kind").getValue() != "Module")
    return emitError() << "captured source root must be a Python Module node";
  if (failed(verifyCapturedNode(root, emitError)))
    return failure();
  return CapturedSource{path.getValue().str(), AstNode{root, {}}};
}

DictionaryAttr sourceSpan(OpBuilder &builder, StringRef sourcePath,
                          const AstNode &node) {
  auto captured = node.value.getAs<DictionaryAttr>("span");
  auto coordinate = [&](StringRef name) {
    return captured.getAs<IntegerAttr>(name);
  };
  return builder.getDictionaryAttr({
      builder.getNamedAttr("path", builder.getStringAttr(sourcePath)),
      builder.getNamedAttr("line", coordinate("start_line")),
      builder.getNamedAttr("column", coordinate("start_codepoint_column")),
      builder.getNamedAttr("end_line", coordinate("end_line")),
      builder.getNamedAttr("end_column", coordinate("end_codepoint_column")),
  });
}

DictionaryAttr occurrence(OpBuilder &builder, FlatSymbolRefAttr definition,
                          const AstNode &node) {
  SmallVector<Attribute> components;
  for (const AstStep &step : node.path) {
    if (step.index) {
      components.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("index")),
          builder.getNamedAttr("value", builder.getI64IntegerAttr(*step.index)),
      }));
    } else {
      components.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("field")),
          builder.getNamedAttr("name", builder.getStringAttr(step.field)),
      }));
    }
  }
  DictionaryAttr site = builder.getDictionaryAttr({
      builder.getNamedAttr("definition", definition),
      builder.getNamedAttr("ast_path", builder.getArrayAttr(components)),
  });
  return builder.getDictionaryAttr({
      builder.getNamedAttr("site", site),
      builder.getNamedAttr("expansion", builder.getArrayAttr({})),
  });
}

} // namespace acir::compiler::detail
