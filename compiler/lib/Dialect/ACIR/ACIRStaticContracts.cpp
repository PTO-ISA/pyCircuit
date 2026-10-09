#include "pycircuit/Dialect/ACIR/ACIRTypes.h"

#include "ACIRSourceContracts.h"
#include "SourceSyntaxSiteContracts.h"
#include "SourceValueSemantics.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

using namespace mlir;

namespace acir::ac {
namespace {

struct StaticVerification {
  llvm::DenseMap<DictionaryAttr, bool> completed;
  llvm::DenseSet<DictionaryAttr> active;
};

LogicalResult verifyTree(DictionaryAttr tree, detail::EmitError error,
                         StaticVerification &verification);

LogicalResult verifyChild(Attribute value, detail::EmitError error,
                          StaticVerification &verification) {
  auto expression = dyn_cast_or_null<StaticExprAttr>(value);
  if (!expression)
    return error() << "StaticExpr child must be #ac.static_expr";
  return verifyTree(expression.getTree(), error, verification);
}

LogicalResult verifyLexicalUse(ArrayAttr name, DictionaryAttr site,
                               DictionaryAttr location,
                               detail::EmitError error) {
  if (failed(detail::verifyLexicalPath(name, error)) ||
      failed(detail::verifyNamespaceSite(site, error)))
    return failure();
  // The expression's captured location supplies a known declaring source file.
  // A callee/member span may be narrower, so only the source path must agree.
  if (!location || site.getAs<DictionaryAttr>("location").get("path") !=
                       location.get("path"))
    return error() << "lexical syntax site path must match enclosing "
                      "StaticExpr location";
  return success();
}

LogicalResult verifyReference(DictionaryAttr reference, DictionaryAttr location,
                              detail::EmitError error) {
  if (!reference)
    return error() << "StaticRef must be a DictionaryAttr";
  auto kind = reference.getAs<StringAttr>("kind");
  if (!kind)
    return error() << "StaticRef requires kind";
  if (kind.getValue() == "lexical") {
    if (reference.size() != 3)
      return error() << "lexical StaticRef requires exactly kind/name/site";
    return verifyLexicalUse(reference.getAs<ArrayAttr>("name"),
                            reference.getAs<DictionaryAttr>("site"), location,
                            error);
  }
  if (kind.getValue() == "parameter") {
    if (reference.size() != 3 || !reference.getAs<FlatSymbolRefAttr>("owner") ||
        !reference.getAs<StringAttr>("name"))
      return error() << "parameter StaticRef requires owner and name";
    return success();
  }
  if (reference.size() != 2)
    return error() << "StaticRef has extra or missing fields";
  if (kind.getValue() == "binding")
    return detail::verifyOccurrence(reference.getAs<DictionaryAttr>("id"),
                                    error);
  if (kind.getValue() == "induction")
    return detail::verifyOccurrence(reference.getAs<DictionaryAttr>("loop"),
                                    error);
  if (kind.getValue() == "export" &&
      reference.getAs<FlatSymbolRefAttr>("symbol"))
    return success();
  return error() << "StaticRef kind or payload is invalid";
}

LogicalResult verifyIterable(DictionaryAttr iterable, detail::EmitError error,
                             StaticVerification &verification) {
  if (!iterable)
    return error() << "StaticIterable must be a DictionaryAttr";
  auto kind = iterable.getAs<StringAttr>("kind");
  auto origin = iterable.getAs<DictionaryAttr>("origin");
  auto location = iterable.getAs<DictionaryAttr>("location");
  if (!kind || failed(detail::verifyOccurrence(origin, error)) ||
      failed(detail::verifySourceSpan(location, error)))
    return failure();
  if (kind.getValue() == "sequence") {
    if (iterable.size() != 4)
      return error() << "sequence StaticIterable has incorrect fields";
    return verifyChild(iterable.get("value"), error, verification);
  }
  if (kind.getValue() != "range" || iterable.size() != 6)
    return error() << "StaticIterable kind or fields are invalid";
  for (StringRef field : {"start", "stop", "step"})
    if (failed(verifyChild(iterable.get(field), error, verification)))
      return failure();
  return success();
}

LogicalResult verifyArgument(DictionaryAttr argument, detail::EmitError error,
                             StaticVerification &verification) {
  if (!argument)
    return error() << "StaticArgument must be a DictionaryAttr";
  auto kind = argument.getAs<StringAttr>("kind");
  if (!kind || failed(verifyChild(argument.get("value"), error, verification)))
    return failure();
  if (kind.getValue() == "positional" && argument.size() == 2)
    return success();
  if (kind.getValue() == "keyword" && argument.size() == 3 &&
      argument.getAs<StringAttr>("name"))
    return success();
  return error() << "StaticArgument kind or fields are invalid";
}

bool isUnary(StringRef op) {
  auto opcode = detail::parseValueOpcode(op);
  return opcode && detail::getValueOpcodeInfo(*opcode).arity == 1;
}

bool isBinary(StringRef op) {
  auto opcode = detail::parseValueOpcode(op);
  return opcode && detail::getValueOpcodeInfo(*opcode).arity == 2;
}

LogicalResult verifyTreeContents(DictionaryAttr tree, detail::EmitError error,
                                 StaticVerification &verification) {
  if (!tree)
    return error() << "StaticExpr tree must be a DictionaryAttr";
  auto kind = tree.getAs<StringAttr>("kind");
  auto origin = tree.getAs<DictionaryAttr>("origin");
  auto location = tree.getAs<DictionaryAttr>("location");
  if (!kind || failed(detail::verifyOccurrence(origin, error)) ||
      failed(detail::verifySourceSpan(location, error)))
    return failure();
  StringRef tag = kind.getValue();
  if (tag == "type_width") {
    auto type = tree.getAs<TypeAttr>("type");
    if (tree.size() != 4 || !type ||
        !isa<BitsType, StructType, TypeParamType, TableType>(type.getValue()))
      return error() << "type_width requires one hardware type";
    return success();
  }
  if (tag == "literal") {
    if (tree.size() != 4)
      return error() << "literal StaticExpr has incorrect fields";
    return detail::verifyStaticValueStructure(
        tree.getAs<DictionaryAttr>("value"), error);
  }
  if (tag == "reference") {
    if (tree.size() != 4)
      return error() << "reference StaticExpr has incorrect fields";
    return verifyReference(tree.getAs<DictionaryAttr>("ref"), location, error);
  }
  if (tag == "unary") {
    auto op = tree.getAs<StringAttr>("operator");
    if (tree.size() != 5 || !op || !isUnary(op.getValue()))
      return error() << "unary StaticExpr operator or fields are invalid";
    return verifyChild(tree.get("operand"), error, verification);
  }
  if (tag == "binary") {
    auto op = tree.getAs<StringAttr>("operator");
    if (tree.size() != 6 || !op || !isBinary(op.getValue()))
      return error() << "binary StaticExpr operator or fields are invalid";
    if (failed(verifyChild(tree.get("lhs"), error, verification)))
      return failure();
    return verifyChild(tree.get("rhs"), error, verification);
  }
  if (tag == "select") {
    if (tree.size() != 6)
      return error() << "select StaticExpr has incorrect fields";
    for (StringRef field : {"condition", "yes", "no"})
      if (failed(verifyChild(tree.get(field), error, verification)))
        return failure();
    return success();
  }
  if (tag == "list") {
    auto elements = tree.getAs<ArrayAttr>("elements");
    if (tree.size() != 4 || !elements)
      return error() << "list StaticExpr has incorrect fields";
    for (Attribute raw : elements)
      if (failed(verifyChild(raw, error, verification)))
        return failure();
    return success();
  }
  if (tag == "call") {
    auto arguments = tree.getAs<ArrayAttr>("arguments");
    if (!arguments)
      return error() << "call StaticExpr requires arguments";
    Attribute callee = tree.get("callee");
    if (auto lexical = dyn_cast_or_null<ArrayAttr>(callee)) {
      if (tree.size() != 6 ||
          failed(verifyLexicalUse(lexical,
                                  tree.getAs<DictionaryAttr>("callee_site"),
                                  location, error)))
        return error()
               << "lexical call requires callee_site and exact call fields";
    } else if (isa_and_nonnull<FlatSymbolRefAttr>(callee)) {
      if (tree.size() != 5 || tree.get("callee_site"))
        return error() << "canonical call forbids callee_site and requires "
                          "exact call fields";
    } else {
      return error()
             << "call callee requires lexical path or canonical flat symbol";
    }
    for (Attribute raw : arguments)
      if (failed(verifyArgument(dyn_cast<DictionaryAttr>(raw), error,
                                verification)))
        return failure();
    return success();
  }
  if (tag == "field" || tag == "element" || tag == "length") {
    if (tree.size() != (tag == "length" ? 4u : 5u) ||
        failed(verifyChild(tree.get("base"), error, verification)))
      return error() << tag << " StaticExpr has incorrect fields";
    if (tag == "field") {
      if (!tree.getAs<StringAttr>("name"))
        return error() << "field name is missing";
      return success();
    }
    if (tag == "element")
      return verifyChild(tree.get("index"), error, verification);
    return success();
  }
  if (tag == "comprehension") {
    if (tree.size() != 6 ||
        failed(detail::verifyOccurrence(tree.getAs<DictionaryAttr>("binder"),
                                        error)) ||
        failed(verifyIterable(tree.getAs<DictionaryAttr>("iterable"), error,
                              verification)))
      return failure();
    return verifyChild(tree.get("value"), error, verification);
  }
  return error() << "unknown StaticExpr kind '" << tag << "'";
}

LogicalResult verifyTree(DictionaryAttr tree, detail::EmitError error,
                         StaticVerification &verification) {
  if (!tree)
    return error() << "StaticExpr tree must be a DictionaryAttr";
  auto known = verification.completed.find(tree);
  if (known != verification.completed.end())
    return success(known->second);
  if (!verification.active.insert(tree).second)
    return error() << "cycle in StaticExpr grammar";
  LogicalResult result = verifyTreeContents(tree, error, verification);
  verification.active.erase(tree);
  verification.completed.try_emplace(tree, succeeded(result));
  return result;
}

} // namespace

LogicalResult
StaticExprAttr::verify(llvm::function_ref<InFlightDiagnostic()> emitError,
                       DictionaryAttr tree) {
  // Completed facts are intrinsic to this immutable grammar dictionary. The
  // cache lives for one call only; owning-source checks use their own context.
  StaticVerification verification;
  return verifyTree(tree, emitError, verification);
}

} // namespace acir::ac
