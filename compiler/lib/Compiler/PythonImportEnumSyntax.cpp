#include "PythonImportEnumSyntax.h"

#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringSwitch.h"

using namespace mlir;

namespace acir::compiler::detail {

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
      .Case("struct", MarkerKind::Struct)
      .Case("bits", MarkerKind::Bits)
      .Case("table", MarkerKind::Table)
      .Case("queue", MarkerKind::Queue)
      .Case("concat", MarkerKind::Concat)
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
