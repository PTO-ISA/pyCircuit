#include "PythonImportCaseSyntax.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {

bool integerLiteral(const AstNode &node) {
  if (node.kind() != "Constant")
    return false;
  auto value = dyn_cast_or_null<DictionaryAttr>(node.get("value"));
  return value && value.size() == 1 && value.getAs<StringAttr>("integer");
}

bool qualifiedAttribute(const AstNode &node) {
  if (node.kind() != "Attribute")
    return false;
  auto base = node.child("value");
  return base.kind() == "Name" || qualifiedAttribute(base);
}

LogicalResult flattenPattern(const AstNode &pattern,
                             SmallVectorImpl<AstNode> &atoms,
                             MatchSyntaxError emitError) {
  StringRef form = pattern.kind();
  if (form == "MatchOr") {
    auto alternatives = pattern.array("patterns");
    if (alternatives.size() < 2)
      return emitError(pattern)
             << "match OR pattern requires at least two alternatives";
    for (size_t i = 0; i < alternatives.size(); ++i)
      if (failed(flattenPattern(pattern.item("patterns", i), atoms, emitError)))
        return failure();
    return success();
  }
  if (form == "MatchValue") {
    auto value = pattern.child("value");
    bool negativeInteger = value.kind() == "UnaryOp" &&
                           value.child("op").kind() == "USub" &&
                           integerLiteral(value.child("operand"));
    if (integerLiteral(value) || negativeInteger || qualifiedAttribute(value)) {
      atoms.push_back(pattern);
      return success();
    }
    return emitError(pattern)
           << "match key requires an Integer literal or qualified Enum member";
  }
  if (form == "MatchSingleton") {
    if (!isa_and_nonnull<BoolAttr>(pattern.get("value")))
      return emitError(pattern) << "match singleton key requires True or False";
    atoms.push_back(pattern);
    return success();
  }
  if (form == "MatchAs") {
    if (isa_and_nonnull<UnitAttr>(pattern.get("pattern")) &&
        isa_and_nonnull<UnitAttr>(pattern.get("name")))
      return emitError(pattern)
             << "match catch-all must be a final standalone arm outside OR";
    return emitError(pattern)
           << "match capture/as patterns are unsupported; use a qualified member";
  }
  return emitError(pattern) << "unsupported match pattern '" << form << "'";
}

} // namespace

FailureOr<SmallVector<CapturedMatchArmSyntax>>
readMatchSyntax(const AstNode &statement, MatchSyntaxError emitError) {
  if (statement.kind() != "Match")
    return emitError(statement) << "match syntax requires a Match statement";
  SmallVector<CapturedMatchArmSyntax> arms;
  auto cases = statement.array("cases");
  for (size_t i = 0; i < cases.size(); ++i) {
    auto arm = statement.item("cases", i);
    auto guard = arm.get("guard");
    if (!isa<UnitAttr>(guard))
      return emitError(arm.child("guard")) << "match guards are unsupported";
    auto pattern = arm.child("pattern");
    bool catchAll = pattern.kind() == "MatchAs" &&
                    isa_and_nonnull<UnitAttr>(pattern.get("pattern")) &&
                    isa_and_nonnull<UnitAttr>(pattern.get("name"));
    CapturedMatchArmSyntax syntax{arm, {}, catchAll};
    if (catchAll) {
      if (i + 1 != cases.size())
        return emitError(pattern) << "match catch-all must be the final arm";
    } else if (failed(flattenPattern(pattern, syntax.atoms, emitError)))
      return failure();
    arms.push_back(std::move(syntax));
  }
  return arms;
}

} // namespace acir::compiler::detail
