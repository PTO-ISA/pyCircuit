#include "PythonImportEnumSyntax.h"

#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringSwitch.h"

using namespace mlir;

namespace acir::compiler::detail {

std::optional<unsigned> fixedBitsAliasWidth(StringRef module,
                                           StringRef remoteName) {
  if (module != "pycircuit" || !remoteName.consume_front("u"))
    return std::nullopt;
  unsigned width = 0;
  if (remoteName.getAsInteger(10, width) || !width || width > 64 ||
      remoteName != Twine(width).str())
    return std::nullopt;
  return width;
}

MarkerKind classifyImportedMarker(StringRef module, StringRef remoteName) {
  if (module == "enum")
    return llvm::StringSwitch<MarkerKind>(remoteName)
        .Case("Enum", MarkerKind::Enum)
        .Case("auto", MarkerKind::Auto)
        .Default(MarkerKind::None);
  if (module != "pycircuit")
    return MarkerKind::None;
  return llvm::StringSwitch<MarkerKind>(remoteName)
      .Case("encoding", MarkerKind::Encoding)
      .Case("enum_to_bits", MarkerKind::EnumToBits)
      .Case("enum_from_bits", MarkerKind::EnumFromBits)
      .Case("module", MarkerKind::Module)
      .Case("system", MarkerKind::System)
      .Case("rule", MarkerKind::Rule)
      .Case("method", MarkerKind::Method)
      .Case("struct", MarkerKind::Struct)
      .Case("bits", MarkerKind::Bits)
      .Case("table", MarkerKind::Table)
      .Case("queue", MarkerKind::Queue)
      .Case("concat", MarkerKind::Concat)
      .Case("four_state", MarkerKind::FourState)
      .Case("case_eq", MarkerKind::CaseEq)
      .Case("is_known", MarkerKind::IsKnown)
      .Case("popcount", MarkerKind::Popcount)
      .Case("count_leading_zeros", MarkerKind::CountLeadingZeros)
      .Case("count_trailing_zeros", MarkerKind::CountTrailingZeros)
      .Case("priority_encode", MarkerKind::PriorityEncode)
      .Case("onehot_encode", MarkerKind::OnehotEncode)
      .Default(MarkerKind::None);
}

bool resolvesMarker(const AstNode &node, MarkerKind marker,
                    LexicalBindings bindings) {
  if (!node || !bindings || marker == MarkerKind::None)
    return false;
  auto resolved = bindings(node);
  return resolved && resolved->marker == marker && !resolved->nominalSymbol;
}

bool hasSourceDecorator(const AstNode &declaration, MarkerKind marker,
                        LexicalBindings bindings) {
  for (size_t i = 0; i < declaration.array("decorator_list").size(); ++i) {
    auto decorator = declaration.item("decorator_list", i);
    if (decorator.kind() == "Call" &&
        (marker == MarkerKind::Encoding || marker == MarkerKind::Rule ||
         marker == MarkerKind::Method || marker == MarkerKind::System))
      decorator = decorator.child("func");
    if (resolvesMarker(decorator, marker, bindings))
      return true;
  }
  return false;
}

FailureOr<CapturedCallableSyntax>
readCallableSyntax(const AstNode &declaration, LexicalBindings bindings,
                   ac::detail::EmitError emitError) {
  if (declaration.kind() != "FunctionDef" ||
      declaration.array("decorator_list").size() != 1)
    return emitError() << "Rule/Method requires one bound decorator";
  auto decorator = declaration.item("decorator_list", 0);
  auto target = decorator.kind() == "Call" ? decorator.child("func") : decorator;
  auto resolved = target && bindings ? bindings(target) : std::nullopt;
  auto marker = resolved && !resolved->nominalSymbol ? resolved->marker
                                                   : MarkerKind::None;
  if (marker != MarkerKind::Rule && marker != MarkerKind::Method)
    return emitError() << "callable decorator must resolve to pycircuit rule or method";
  // Dynamic default bindings belong only to a direct Module-local Rule.
  // Source-global templates and Methods retain their required-formal ABI.
  bool localRule = marker == MarkerKind::Rule && declaration.path.size() == 4 &&
                   declaration.path[0].field == "body" && !declaration.path[0].index &&
                   declaration.path[1].index &&
                   declaration.path[2].field == "body" && !declaration.path[2].index &&
                   declaration.path[3].index;
  auto formals = declaration.child("args");
  if (!localRule && !formals.array("defaults").empty())
    return emitError() << "only autonomous local Rules accept hardware default bindings";
  CapturedCallableSyntax result{marker, {}, {}};
  if (decorator.kind() != "Call")
    return result;
  if (!decorator.array("args").empty() || decorator.array("keywords").empty())
    return emitError() << "Rule/Method decorator accepts guard=lambda";
  for (size_t i = 0; i < decorator.array("keywords").size(); ++i) {
    auto keyword = decorator.item("keywords", i);
    StringRef name = keyword.string("arg");
    if (name == "guard" && !result.guard)
      result.guard = keyword.child("value");
    else if (name == "after")
      return emitError() << "source after is retired; independent Rules read current state";
    else
      return emitError() << "duplicate or unsupported Rule/Method decorator option";
  }
  if (!result.guard) return result;
  if (result.guard.kind() != "Lambda")
    return emitError() << "Rule/Method guard requires a lambda";
  auto required = [](const AstNode &arguments) {
    return arguments && arguments.array("posonlyargs").empty() &&
           arguments.array("kwonlyargs").empty() &&
           arguments.array("defaults").empty() &&
           !arguments.child("vararg") && !arguments.child("kwarg");
  };
  auto guardFormals = result.guard.child("args");
  bool boundFormals = localRule &&
      formals.array("posonlyargs").empty() && formals.array("kwonlyargs").empty() &&
      !formals.child("vararg") && !formals.child("kwarg") &&
      formals.array("defaults").size() == formals.array("args").size();
  if ((!required(formals) && !boundFormals) || !required(guardFormals) ||
      formals.array("args").size() != guardFormals.array("args").size())
    return emitError() << "guard lambda requires the callable's exact required positional parameters";
  for (size_t i = 0; i < formals.array("args").size(); ++i)
    if (formals.item("args", i).string("arg") !=
        guardFormals.item("args", i).string("arg"))
      return emitError() << "guard lambda parameter names and order must match the callable";
  return result;
}

FailureOr<FlatSymbolRefAttr>
resolveNominalType(const AstNode &node, LexicalBindings bindings,
                   ac::detail::EmitError emitError) {
  auto resolved = node && bindings ? bindings(node) : std::nullopt;
  if (!resolved || resolved->marker != MarkerKind::None ||
      !resolved->nominalSymbol)
    return emitError() << "type syntax requires a bound nominal declaration";
  return resolved->nominalSymbol;
}

FailureOr<CapturedEnumSyntax> readEnumSyntax(const AstNode &declaration,
                                             LexicalBindings bindings,
                                             ac::detail::EmitError emitError) {
  if (declaration.kind() != "ClassDef" ||
      failed(ac::detail::verifyPythonAstIdentifier(declaration.string("name"),
                                                   true, emitError)))
    return emitError() << "enum requires a named class declaration";
  auto typeParameters = declaration.array("type_params");
  if (declaration.array("bases").size() != 1 ||
      !resolvesMarker(declaration.item("bases", 0), MarkerKind::Enum,
                      bindings) ||
      !declaration.array("keywords").empty() ||
      (typeParameters && !typeParameters.empty()))
    return emitError() << "enum requires one bound Enum base, no mixins, "
                          "metaclass or type parameters";
  if (declaration.array("decorator_list").size() != 1)
    return emitError() << "enum requires exactly one encoding decorator";
  AstNode decorator = declaration.item("decorator_list", 0);
  if (decorator.kind() != "Call" ||
      !resolvesMarker(decorator.child("func"), MarkerKind::Encoding,
                      bindings) ||
      !decorator.array("args").empty())
    return emitError() << "enum requires a bound encoding decorator with "
                          "keyword arguments";

  CapturedEnumSyntax syntax{declaration, {}, "explicit", {}};
  llvm::StringSet<> keywords;
  for (size_t i = 0; i < decorator.array("keywords").size(); ++i) {
    AstNode keyword = decorator.item("keywords", i);
    StringRef name = keyword.string("arg");
    if (name.empty() || !keywords.insert(name).second)
      return emitError() << "encoding requires distinct named keywords";
    if (name == "width")
      syntax.width = keyword.child("value");
    else if (name == "kind") {
      AstNode value = keyword.child("value");
      auto kind = value.kind() == "Constant"
                      ? dyn_cast_or_null<StringAttr>(value.get("value"))
                      : StringAttr();
      if (!kind)
        return emitError() << "encoding kind requires a literal string";
      syntax.encoding = kind.getValue().str();
    } else
      return emitError() << "unsupported encoding keyword '" << name << "'";
  }
  if (!syntax.width)
    return emitError() << "encoding requires an explicit width";
  if (!llvm::is_contained(ArrayRef<StringRef>{"explicit", "binary_sequential",
                                              "binary_one_hot",
                                              "gray_sequential"},
                          StringRef(syntax.encoding)))
    return emitError() << "unknown enum encoding kind";

  llvm::StringSet<> members;
  for (size_t i = 0; i < declaration.array("body").size(); ++i) {
    AstNode statement = declaration.item("body", i);
    AstNode expression =
        statement.kind() == "Expr" ? statement.child("value") : AstNode();
    if (i == 0 && expression.kind() == "Constant" &&
        isa_and_nonnull<StringAttr>(expression.get("value")))
      continue;
    if (statement.kind() != "Assign" ||
        statement.array("targets").size() != 1 ||
        statement.item("targets", 0).kind() != "Name")
      return emitError()
             << "enum body requires single-name member assignments; "
                "methods and construction hooks are unsupported";
    StringRef name = statement.item("targets", 0).string("id");
    if (failed(ac::detail::verifyPythonAstIdentifier(name, true, emitError)) ||
        !members.insert(name).second)
      return emitError() << "enum member names must be valid and unique";
    // Sunder/dunder assignments customize Enum construction or metadata;
    // capture supports members only, rather than Python class hooks.
    if (name.size() > 2 && name.starts_with('_') && name.ends_with('_'))
      return emitError() << "enum construction hooks are unsupported";
    AstNode value = statement.child("value");
    bool automatic =
        value.kind() == "Call" &&
        resolvesMarker(value.child("func"), MarkerKind::Auto, bindings);
    if (automatic &&
        (!value.array("args").empty() || !value.array("keywords").empty()))
      return emitError() << "enum auto requires an argument-free call";
    if ((syntax.encoding == "explicit" && automatic) ||
        (syntax.encoding != "explicit" && !automatic))
      return emitError() << "explicit enum requires Integer expressions; "
                            "derived encoding requires uniformly bound auto()";
    syntax.members.push_back({statement, name.str(), value, automatic});
  }
  if (syntax.members.empty())
    return emitError() << "enum requires at least one member";
  return syntax;
}

} // namespace acir::compiler::detail
