#include "acir/Dialect/ACIR/ACIRAttributes.h"

#include "ACIRSourceContracts.h"

using namespace mlir;

namespace acir::ac {
namespace {

LogicalResult verifyTree(DictionaryAttr tree, detail::EmitError error);

LogicalResult verifyChild(Attribute value, detail::EmitError error) {
  auto expression = dyn_cast_or_null<StaticExprAttr>(value);
  if (!expression)
    return error() << "StaticExpr child must be #ac.static_expr";
  return verifyTree(expression.getTree(), error);
}

LogicalResult verifyReference(DictionaryAttr reference,
                              detail::EmitError error) {
  if (!reference)
    return error() << "StaticRef must be a DictionaryAttr";
  auto kind = reference.getAs<StringAttr>("kind");
  if (!kind)
    return error() << "StaticRef requires kind";
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

LogicalResult verifyIterable(DictionaryAttr iterable, detail::EmitError error) {
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
    return verifyChild(iterable.get("value"), error);
  }
  if (kind.getValue() != "range" || iterable.size() != 6)
    return error() << "StaticIterable kind or fields are invalid";
  for (StringRef field : {"start", "stop", "step"})
    if (failed(verifyChild(iterable.get(field), error)))
      return failure();
  return success();
}

LogicalResult verifyArgument(DictionaryAttr argument, detail::EmitError error) {
  if (!argument)
    return error() << "StaticArgument must be a DictionaryAttr";
  auto kind = argument.getAs<StringAttr>("kind");
  if (!kind || failed(verifyChild(argument.get("value"), error)))
    return failure();
  if (kind.getValue() == "positional" && argument.size() == 2)
    return success();
  if (kind.getValue() == "keyword" && argument.size() == 3 &&
      argument.getAs<StringAttr>("name"))
    return success();
  return error() << "StaticArgument kind or fields are invalid";
}

bool isUnary(StringRef op) {
  return op == "neg" || op == "invert" || op == "not" || op == "to_int";
}

bool isBinary(StringRef op) {
  return op == "add" || op == "sub" || op == "mul" || op == "floordiv" ||
         op == "mod" || op == "and_bits" || op == "or_bits" ||
         op == "xor_bits" || op == "shl" || op == "shr" || op == "eq" ||
         op == "ne" || op == "lt" || op == "le" || op == "gt" || op == "ge" ||
         op == "and_bool" || op == "or_bool";
}

LogicalResult verifyTree(DictionaryAttr tree, detail::EmitError error) {
  if (!tree)
    return error() << "StaticExpr tree must be a DictionaryAttr";
  auto kind = tree.getAs<StringAttr>("kind");
  auto origin = tree.getAs<DictionaryAttr>("origin");
  auto location = tree.getAs<DictionaryAttr>("location");
  if (!kind || failed(detail::verifyOccurrence(origin, error)) ||
      failed(detail::verifySourceSpan(location, error)))
    return failure();
  StringRef tag = kind.getValue();
  if (tag == "literal") {
    if (tree.size() != 4)
      return error() << "literal StaticExpr has incorrect fields";
    return detail::verifyStaticValueStructure(
        tree.getAs<DictionaryAttr>("value"), error);
  }
  if (tag == "reference") {
    if (tree.size() != 4)
      return error() << "reference StaticExpr has incorrect fields";
    return verifyReference(tree.getAs<DictionaryAttr>("ref"), error);
  }
  if (tag == "unary") {
    auto op = tree.getAs<StringAttr>("operator");
    if (tree.size() != 5 || !op || !isUnary(op.getValue()))
      return error() << "unary StaticExpr operator or fields are invalid";
    return verifyChild(tree.get("operand"), error);
  }
  if (tag == "binary") {
    auto op = tree.getAs<StringAttr>("operator");
    if (tree.size() != 6 || !op || !isBinary(op.getValue()))
      return error() << "binary StaticExpr operator or fields are invalid";
    if (failed(verifyChild(tree.get("lhs"), error)))
      return failure();
    return verifyChild(tree.get("rhs"), error);
  }
  if (tag == "select") {
    if (tree.size() != 6)
      return error() << "select StaticExpr has incorrect fields";
    for (StringRef field : {"condition", "yes", "no"})
      if (failed(verifyChild(tree.get(field), error)))
        return failure();
    return success();
  }
  if (tag == "list" || tag == "call") {
    auto elements =
        tree.getAs<ArrayAttr>(tag == "list" ? "elements" : "arguments");
    if (tree.size() != (tag == "list" ? 4u : 5u) || !elements ||
        (tag == "call" && !tree.getAs<FlatSymbolRefAttr>("callee")))
      return error() << tag << " StaticExpr has incorrect fields";
    for (Attribute raw : elements)
      if (failed(tag == "list"
                     ? verifyChild(raw, error)
                     : verifyArgument(dyn_cast<DictionaryAttr>(raw), error)))
        return failure();
    return success();
  }
  if (tag == "field" || tag == "element" || tag == "length") {
    if (tree.size() != (tag == "length" ? 4u : 5u) ||
        failed(verifyChild(tree.get("base"), error)))
      return error() << tag << " StaticExpr has incorrect fields";
    if (tag == "field") {
      if (!tree.getAs<StringAttr>("name"))
        return error() << "field name is missing";
      return success();
    }
    if (tag == "element")
      return verifyChild(tree.get("index"), error);
    return success();
  }
  if (tag == "comprehension") {
    if (tree.size() != 6 ||
        failed(detail::verifyOccurrence(tree.getAs<DictionaryAttr>("binder"),
                                        error)) ||
        failed(verifyIterable(tree.getAs<DictionaryAttr>("iterable"), error)))
      return failure();
    return verifyChild(tree.get("value"), error);
  }
  return error() << "unknown StaticExpr kind '" << tag << "'";
}

} // namespace

LogicalResult
StaticExprAttr::verify(llvm::function_ref<InFlightDiagnostic()> emitError,
                       DictionaryAttr tree) {
  return verifyTree(tree, emitError);
}

} // namespace acir::ac
