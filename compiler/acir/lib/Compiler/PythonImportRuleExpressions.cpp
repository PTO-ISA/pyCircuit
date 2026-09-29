#include "PythonImportRuleExpressions.h"
#include "PythonImportContext.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/StringSwitch.h"

using namespace mlir;
namespace acir::compiler::detail {

RuleExpressionProducer::RuleExpressionProducer(
    OpBuilder &builder, ac::RuleOp rule, const ModuleModel &module,
    StringRef sourcePath, const llvm::StringMap<Value> &entries,
    const llvm::StringMap<DictionaryAttr> &types, ac::detail::EmitError error)
    : b(builder), rule(rule), module(module), sourcePath(sourcePath),
      entries(entries), types(types), error(error) {}

DictionaryAttr RuleExpressionProducer::origin(const AstNode &site) {
  return occurrence(b, module.symbol,
                    relativeToModule(module.declaration, site));
}
DictionaryAttr RuleExpressionProducer::identity(const AstNode &site,
                                                unsigned slot) {
  return b.getDictionaryAttr(
      {b.getNamedAttr("origin", origin(site)),
       b.getNamedAttr("slot", b.getI32IntegerAttr(slot))});
}
DictionaryAttr RuleExpressionProducer::reference(unsigned index,
                                                 StringRef kind) {
  return b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr(kind)),
       b.getNamedAttr("index", b.getI32IntegerAttr(index))});
}
Operation *RuleExpressionProducer::operation(StringRef name,
                                             const AstNode &site,
                                             ValueRange operands,
                                             TypeRange results,
                                             ArrayRef<NamedAttribute> attrs) {
  return createSourceOperation(b, site.location(b.getContext(), sourcePath),
                               name, operands, results, attrs);
}
unsigned RuleExpressionProducer::numericNode(DictionaryAttr id,
                                             StringRef opcode,
                                             ArrayRef<Attribute> operands,
                                             DictionaryAttr target) {
  if (!target)
    target =
        b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("none"))});
  unsigned index = nodes.size();
  nodes.push_back(
      b.getDictionaryAttr({b.getNamedAttr("id", id),
                           b.getNamedAttr("operator", b.getStringAttr(opcode)),
                           b.getNamedAttr("operands", b.getArrayAttr(operands)),
                           b.getNamedAttr("target", target)}));
  return index;
}
Value RuleExpressionProducer::boolean(bool value, const AstNode &site) {
  return arith::ConstantOp::create(b, site.location(b.getContext(), sourcePath),
                                   b.getBoolAttr(value));
}
Value RuleExpressionProducer::conjunction(Value lhs, Value rhs,
                                          const AstNode &site) {
  return arith::AndIOp::create(b, site.location(b.getContext(), sourcePath),
                               lhs, rhs);
}
Value RuleExpressionProducer::disjunction(Value lhs, Value rhs,
                                          const AstNode &site) {
  return arith::OrIOp::create(b, site.location(b.getContext(), sourcePath), lhs,
                              rhs);
}
Value RuleExpressionProducer::invert(Value value, const AstNode &site) {
  return arith::XOrIOp::create(b, site.location(b.getContext(), sourcePath),
                               value, boolean(true, site));
}
Value RuleExpressionProducer::continueAfter(Value live, Value path, Value valid,
                                            const AstNode &site) {
  return conjunction(live, disjunction(invert(path, site), valid, site), site);
}

FailureOr<RuleExpressionValue> RuleExpressionProducer::read(const AstNode &node,
                                                            Value path) {
  auto entry = entries.find(node.string("id"));
  auto type = types.find(node.string("id"));
  if (node.kind() != "Name" || entry == entries.end() || type == types.end())
    return error() << "rule expression requires a bound current value";
  auto value = entry->second;
  DictionaryAttr id;
  if (auto previous = value.getDefiningOp<ac::SourceReadOp>()) {
    id = b.getDictionaryAttr(
        {b.getNamedAttr("origin", previous->getAttr("ac.origin")),
         b.getNamedAttr("slot", b.getI32IntegerAttr(0))});
  } else {
    auto *op = operation(ac::SourceReadOp::getOperationName(), node, {value},
                         {value.getType()},
                         {b.getNamedAttr("ac.origin", origin(node))});
    value = op->getResult(0);
    id = identity(node);
  }
  return RuleExpressionValue{value, path, type->second, id, std::nullopt};
}

FailureOr<RuleExpressionValue> RuleExpressionProducer::emit(const AstNode &node,
                                                            Value path) {
  auto boolDomain = b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("bool")),
       b.getNamedAttr("storage", TypeAttr::get(b.getI1Type()))});
  Type math = ac::MathIntType::get(b.getContext());
  if (node.kind() == "Name") {
    auto result = read(node, path);
    if (failed(result))
      return failure();
    if (result->logical == boolDomain)
      return *result;
    if (result->logical.getAs<StringAttr>("kind").getValue() != "integer")
      return error()
             << "numeric expression requires integer or bool current value";
    auto *from = operation(ac::MathFromBitsOp::getOperationName(), node,
                           {result->value}, {math},
                           {b.getNamedAttr("ac.origin", origin(node)),
                            b.getNamedAttr("domain", result->logical)});
    result->value = from->getResult(0);
    result->id = identity(node, 1);
    result->numeric =
        numericNode(result->id, "from_bits", {reference(0, "input")});
    return *result;
  }
  if (node.kind() == "Constant") {
    if (auto value = dyn_cast_or_null<BoolAttr>(node.get("value")))
      return RuleExpressionValue{boolean(value.getValue(), node), path,
                                 boolDomain, identity(node), std::nullopt};
    auto encoded = dyn_cast_or_null<DictionaryAttr>(node.get("value"));
    auto spelling =
        encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
    if (!spelling)
      return error()
             << "numeric expression constant must be an integer or bool";
    auto value = parseStaticInteger(b, spelling.getValue(), error);
    if (failed(value))
      return failure();
    auto *constant =
        operation(ac::MathConstantOp::getOperationName(), node, {}, {math},
                  {b.getNamedAttr("ac.origin", origin(node)),
                   b.getNamedAttr("value", *value)});
    auto ref = b.getDictionaryAttr(
        {b.getNamedAttr("kind", b.getStringAttr("constant")),
         b.getNamedAttr("value", *value)});
    auto id = identity(node);
    return RuleExpressionValue{constant->getResult(0),
                               path,
                               {},
                               id,
                               numericNode(id, "constant", {ref})};
  }
  if (node.kind() == "BoolOp") {
    auto values = node.array("values");
    bool isAnd = node.child("op").kind() == "And";
    if (values.size() < 2 || (!isAnd && node.child("op").kind() != "Or"))
      return error() << "boolean expression requires and/or operands";
    auto lhs = emit(node.item("values", 0), path);
    if (failed(lhs) || lhs->logical != boolDomain)
      return error() << "short circuit condition requires source bool";
    for (size_t i = 1; i < values.size(); ++i) {
      Value demand = isAnd ? lhs->value : invert(lhs->value, node);
      Value rhsPath = conjunction(lhs->valid, demand, node);
      auto rhs = emit(node.item("values", i), rhsPath);
      if (failed(rhs) || rhs->logical != boolDomain)
        return error() << "short circuit condition requires source bool";
      Value valid = conjunction(
          lhs->valid, disjunction(invert(demand, node), rhs->valid, node),
          node);
      Value data = isAnd ? conjunction(lhs->value, rhs->value, node)
                         : disjunction(lhs->value, rhs->value, node);
      *lhs = RuleExpressionValue{data, valid, boolDomain, identity(node),
                                 std::nullopt};
    }
    return *lhs;
  }
  bool compare = node.kind() == "Compare";
  if (!compare && node.kind() != "BinOp")
    return error() << "unsupported numeric source expression";
  if (compare &&
      (node.array("ops").size() != 1 || node.array("comparators").size() != 1))
    return error() << "numeric comparison currently requires one comparator";
  StringRef opcode =
      llvm::StringSwitch<StringRef>(compare ? node.item("ops", 0).kind()
                                            : node.child("op").kind())
          .Case("Add", "add")
          .Case("Sub", "sub")
          .Case("BitAnd", "and_bits")
          .Case("Eq", "eq")
          .Case("NotEq", "ne")
          .Case("Lt", "lt")
          .Case("LtE", "le")
          .Case("Gt", "gt")
          .Case("GtE", "ge")
          .Default("");
  if (opcode.empty())
    return error() << "unsupported numeric operator";
  AstNode right = compare ? node.item("comparators", 0) : node.child("right");
  auto readsCurrent = [&](auto &&self, const AstNode &value) -> bool {
    if (value.kind() == "Name")
      return true;
    if (value.kind() == "BinOp")
      return self(self, value.child("left")) ||
             self(self, value.child("right"));
    return false;
  };
  if (readsCurrent(readsCurrent, right))
    return error()
           << "numeric composition currently requires a constant right operand";
  auto lhs = emit(node.child("left"), path);
  if (failed(lhs) || !lhs->numeric)
    return error() << "numeric operand requires mathematical integer";
  auto rhs = emit(compare ? node.item("comparators", 0) : node.child("right"),
                  lhs->valid);
  if (failed(rhs) || !rhs->numeric)
    return error() << "numeric operand requires mathematical integer";
  auto id = identity(node);
  auto *op =
      operation(compare ? ac::MathCompareOp::getOperationName()
                        : ac::MathBinaryOp::getOperationName(),
                node, {path, lhs->value, lhs->valid, rhs->value, rhs->valid},
                {compare ? b.getI1Type() : math, b.getI1Type()},
                {b.getNamedAttr("ac.origin", origin(node)),
                 b.getNamedAttr(compare ? "predicate" : "operator",
                                b.getStringAttr(opcode))});
  return RuleExpressionValue{op->getResult(0), op->getResult(1),
                             compare ? boolDomain : DictionaryAttr(), id,
                             numericNode(id, opcode,
                                         {reference(*lhs->numeric, "node"),
                                          reference(*rhs->numeric, "node")})};
}

DictionaryAttr RuleExpressionProducer::check(const AstNode &site,
                                             StringRef kind, Value condition,
                                             Value path) {
  auto id = b.getDictionaryAttr(
      {b.getNamedAttr("registration", rule.getRegistrationAttr()),
       b.getNamedAttr("check", origin(site)),
       b.getNamedAttr("obligation", b.getI64IntegerAttr(checks.size()))});
  auto location = sourceSpan(b, sourcePath, site);
  operation(ac::SourceExpectOp::getOperationName(), site, {condition, path}, {},
            {b.getNamedAttr("kind", b.getStringAttr(kind)),
             b.getNamedAttr("ac.check_id", id),
             b.getNamedAttr("location", location)});
  checks.push_back(b.getDictionaryAttr(
      {b.getNamedAttr("id", id), b.getNamedAttr("kind", b.getStringAttr(kind)),
       b.getNamedAttr("location", location)}));
  return b.getDictionaryAttr(
      {b.getNamedAttr("leaf", origin(site).get("site")),
       b.getNamedAttr("kind", b.getStringAttr(kind)),
       b.getNamedAttr("obligation", id.get("obligation")),
       b.getNamedAttr("location", location)});
}
FailureOr<RuleExpressionValue>
RuleExpressionProducer::boundary(const AstNode &expression, const AstNode &site,
                                 RuleExpressionValue value, Value path,
                                 DictionaryAttr domain) {
  if (!value.numeric)
    return error() << "integer boundary requires mathematical integer";
  auto storage = domain.getAs<TypeAttr>("storage");
  if (!storage)
    return error() << "integer boundary requires a logical integer domain";
  auto *op = operation(ac::MathToBitsOp::getOperationName(), expression,
                       {path, value.value, value.valid},
                       {storage.getValue(), b.getI1Type()},
                       {b.getNamedAttr("domain", domain),
                        b.getNamedAttr("ac.origin", origin(expression))});
  auto shape = check(site, "range", op->getResult(1),
                     conjunction(path, value.valid, site));
  op->setAttr("ac.check_template", shape);
  unsigned slot = 1;
  if (value.id && value.id.get("origin") == origin(expression))
    slot = value.id.getAs<IntegerAttr>("slot").getInt() + 1;
  auto id = identity(expression, slot);
  auto target = b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("integer_boundary")),
       b.getNamedAttr("domain", domain)});
  return RuleExpressionValue{
      op->getResult(0), op->getResult(1), domain, id,
      numericNode(id, "to_bits", {reference(*value.numeric, "node")}, target)};
}
LogicalResult RuleExpressionProducer::assertion(const AstNode &statement,
                                                Value &live) {
  AstNode message = statement.child("msg");
  if (message && (message.kind() != "Constant" ||
                  !isa_and_nonnull<StringAttr>(message.get("value"))))
    return error() << "assert message must be a static string";
  auto value = emit(statement.child("test"), live);
  if (failed(value) || !value->logical ||
      value->logical.getAs<StringAttr>("kind").getValue() != "bool")
    return error() << "assert requires source bool";
  Value path = conjunction(live, value->valid, statement);
  check(statement, "assert", value->value, path);
  live = conjunction(path, value->value, statement);
  return success();
}
void RuleExpressionProducer::finish() {
  if (!nodes.empty()) {
    auto spec =
        b.getDictionaryAttr({b.getNamedAttr("definition", module.symbol),
                             b.getNamedAttr("arguments", b.getArrayAttr({}))});
    rule->setAttr(
        "ac.proof_scope",
        b.getDictionaryAttr(
            {b.getNamedAttr("specialization", spec),
             b.getNamedAttr("registration", rule.getRegistrationAttr())}));
    rule->setAttr("ac.required_numeric", b.getArrayAttr(nodes));
  }
  rule->setAttr("ac.required_checks", b.getArrayAttr(checks));
}

} // namespace acir::compiler::detail
