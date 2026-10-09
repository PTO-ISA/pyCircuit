#include "SourceSyntaxSiteContracts.h"

#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "llvm/ADT/DenseSet.h"

using namespace mlir;

namespace acir::ac::detail {

LogicalResult verifyLexicalPath(ArrayAttr path, EmitError error) {
  if (!path || path.empty())
    return error()
           << "source lexical path requires nonempty ArrayAttr<StringAttr>";
  for (Attribute component : path) {
    auto identifier = dyn_cast<StringAttr>(component);
    if (!identifier)
      return error() << "source lexical path components require StringAttr";
    if (failed(verifyPythonAstIdentifier(identifier.getValue(),
                                         /*bindingName=*/false, error)))
      return failure();
  }
  return success();
}

LogicalResult verifyNamespaceSite(DictionaryAttr site, EmitError error,
                                  DictionaryAttr expectedOwner) {
  auto path = site ? site.getAs<ArrayAttr>("ast_path") : ArrayAttr();
  auto location =
      site ? site.getAs<DictionaryAttr>("location") : DictionaryAttr();
  if (!site || site.size() != 2 || !path ||
      failed(verifySourceSpan(location, error)))
    return error()
           << "NamespaceSite requires exactly ast_path and SourceSpan location";
  for (Attribute component : path)
    if (failed(verifyPathComponent(dyn_cast<DictionaryAttr>(component), error)))
      return failure();
  if (expectedOwner) {
    if (failed(verifySourceOwner(expectedOwner, error)))
      return failure();
    if (location.get("path") != expectedOwner.get("path"))
      return error() << "NamespaceSite path must match expected SourceOwner";
  }
  return success();
}

namespace {

// The existing StaticExpr verifier establishes the one closed grammar first.
// This walk checks its captured syntax locations, not expression evaluation or
// correspondence to an unavailable AST. Memoization bounds shared attributes.
LogicalResult capturedLocations(Attribute value, DictionaryAttr owner,
                                EmitError error,
                                llvm::DenseSet<Attribute> &visited) {
  if (!value || !visited.insert(value).second)
    return success();
  if (auto expression = dyn_cast<StaticExprAttr>(value))
    return capturedLocations(expression.getTree(), owner, error, visited);
  if (auto fields = dyn_cast<DictionaryAttr>(value)) {
    if (auto location = fields.getAs<DictionaryAttr>("location")) {
      if (fields.getAs<ArrayAttr>("ast_path")) {
        if (failed(verifyNamespaceSite(fields, error, owner)))
          return failure();
      } else if (failed(verifySourceSpan(location, error)) ||
                 location.get("path") != owner.get("path")) {
        return error() << "captured static syntax location must match "
                          "declaring SourceOwner";
      }
    }
    for (NamedAttribute field : fields)
      if (failed(capturedLocations(field.getValue(), owner, error, visited)))
        return failure();
  } else if (auto values = dyn_cast<ArrayAttr>(value)) {
    for (Attribute element : values)
      if (failed(capturedLocations(element, owner, error, visited)))
        return failure();
  }
  return success();
}

} // namespace

LogicalResult verifyStaticExprOwner(StaticExprAttr expression,
                                    DictionaryAttr expectedOwner,
                                    EmitError error) {
  if (failed(verifySourceOwner(expectedOwner, error)))
    return failure();
  if (!expression ||
      failed(StaticExprAttr::verify(error, expression.getTree())))
    return error() << "owning static syntax requires a verified StaticExpr";
  llvm::DenseSet<Attribute> visited;
  return capturedLocations(expression, expectedOwner, error, visited);
}

LogicalResult verifySourceTypeExprOwner(SourceTypeExprAttr expression,
                                        DictionaryAttr expectedOwner,
                                        EmitError error) {
  if (failed(verifySourceOwner(expectedOwner, error)))
    return failure();
  if (!expression ||
      failed(SourceTypeExprAttr::verify(error, expression.getValue())))
    return error() << "owning annotation requires a verified SourceTypeExpr";
  DictionaryAttr fields = expression.getValue();
  StringRef kind = fields.getAs<StringAttr>("kind").getValue();
  if (kind == "alias")
    return verifyNamespaceSite(fields.getAs<DictionaryAttr>("site"), error,
                               expectedOwner);
  if (kind == "integer") {
    if (failed(verifyStaticExprOwner(fields.getAs<StaticExprAttr>("lower"),
                                     expectedOwner, error)) ||
        failed(verifyStaticExprOwner(fields.getAs<StaticExprAttr>("upper"),
                                     expectedOwner, error)))
      return failure();
  }
  return success();
}

} // namespace acir::ac::detail
