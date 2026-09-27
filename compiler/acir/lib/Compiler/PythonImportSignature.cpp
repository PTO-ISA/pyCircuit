#include "PythonImportSignature.h"

#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::compiler::detail {

FailureOr<FunctionSignatureSyntax>
parseFunctionSignature(const AstNode &arguments, bool hasImplicitReceiver,
                       ac::detail::EmitError emitError) {
  ArrayAttr positionalOnly = arguments.array("posonlyargs");
  ArrayAttr ordinary = arguments.array("args");
  ArrayAttr keywordOnly = arguments.array("kwonlyargs");
  ArrayAttr defaults = arguments.array("defaults");
  ArrayAttr keywordDefaults = arguments.array("kw_defaults");
  if (!positionalOnly || !ordinary || !keywordOnly || !defaults ||
      !keywordDefaults || keywordDefaults.size() != keywordOnly.size())
    return emitError() << "record constructor signature arrays are malformed";

  FunctionSignatureSyntax result;
  Attribute rawVararg = arguments.get("vararg");
  Attribute rawKwarg = arguments.get("kwarg");
  result.hasVararg = rawVararg && !isa<UnitAttr>(rawVararg);
  result.hasKwarg = rawKwarg && !isa<UnitAttr>(rawKwarg);
  if (hasImplicitReceiver && (result.hasVararg || result.hasKwarg))
    return emitError() << "record constructors reject *args and **kwargs";

  SmallVector<AstNode> positional;
  for (size_t index = 0; index < positionalOnly.size(); ++index)
    positional.push_back(arguments.item("posonlyargs", index));
  for (size_t index = 0; index < ordinary.size(); ++index)
    positional.push_back(arguments.item("args", index));

  if (hasImplicitReceiver &&
      (positional.empty() || positional.front().string("arg") != "self"))
    return emitError()
           << "record constructor first positional parameter must be self";
  size_t firstDefault = 0;
  if (defaults.size() > positional.size()) {
    if (hasImplicitReceiver)
      return emitError()
             << "record constructor defaults exceed positional parameters";
    return emitError() << "function defaults exceed positional parameters";
  }
  firstDefault = positional.size() - defaults.size();

  size_t firstParameter = 0;
  llvm::StringSet<> parameterNames;
  if (hasImplicitReceiver) {
    Attribute annotation = positional.front().get("annotation");
    if (!annotation || !isa<UnitAttr>(annotation))
      return emitError()
             << "U01 does not yet support an annotated constructor receiver";
    if (firstDefault == 0)
      return emitError()
             << "U01 does not yet support a defaulted constructor receiver";
    parameterNames.insert("self");
    firstParameter = 1;
  }

  for (size_t index = firstParameter; index < positional.size(); ++index) {
    StringRef name = positional[index].string("arg");
    if (!parameterNames.insert(name).second)
      return emitError() << "duplicate record constructor parameter '" << name
                         << "'";
    std::optional<AstNode> defaultValue;
    if (index >= firstDefault)
      defaultValue =
          AstNode{dyn_cast<DictionaryAttr>(defaults[index - firstDefault]), {}};
    result.parameters.push_back({positional[index],
                                 index < positionalOnly.size()
                                     ? StringRef("positional_only")
                                     : StringRef("positional_or_keyword"),
                                 defaultValue});
  }
  for (size_t index = 0; index < keywordOnly.size(); ++index) {
    StringRef name = arguments.item("kwonlyargs", index).string("arg");
    if (!parameterNames.insert(name).second)
      return emitError() << "duplicate record constructor parameter '" << name
                         << "'";
    Attribute rawDefault = keywordDefaults[index];
    std::optional<AstNode> defaultValue;
    if (!isa<UnitAttr>(rawDefault))
      defaultValue = AstNode{dyn_cast<DictionaryAttr>(rawDefault), {}};
    result.parameters.push_back({arguments.item("kwonlyargs", index),
                                 StringRef("keyword_only"), defaultValue});
  }
  return result;
}

} // namespace acir::compiler::detail
