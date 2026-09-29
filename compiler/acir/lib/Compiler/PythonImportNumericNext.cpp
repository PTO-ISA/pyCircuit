#include "PythonImportNumericNext.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"

using namespace mlir;
namespace acir::compiler::detail {
namespace {
bool literalIs(const AstNode &node, StringRef value) {
  auto encoded = node.kind() == "Constant"
                     ? dyn_cast_or_null<DictionaryAttr>(node.get("value"))
                     : DictionaryAttr();
  auto integer = encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
  return integer && integer.getValue() == value;
}
DictionaryAttr id(OpBuilder &b, DictionaryAttr origin, unsigned slot = 0) {
  return b.getDictionaryAttr(
      {b.getNamedAttr("origin", origin),
       b.getNamedAttr("slot", b.getI32IntegerAttr(slot))});
}
DictionaryAttr ref(OpBuilder &b, StringRef kind, unsigned index) {
  return b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr(kind)),
       b.getNamedAttr("index", b.getI32IntegerAttr(index))});
}
} // namespace

FailureOr<std::optional<PythonNumericNextPlan>>
analyzePythonNumericNext(const RulePlan &rule, const ModuleModel &module,
                         const AstNode &method, ac::detail::EmitError error) {
  if (rule.outputs.empty())
    return std::optional<PythonNumericNextPlan>();
  AstNode assignment;
  bool returned = false, candidate = false, unsupported = false;
  auto statements = method.array("body");
  for (size_t n = 0; n < statements.size(); ++n) {
    auto stmt = method.item("body", n);
    if (isModuleDocstring(stmt) || stmt.kind() == "Nonlocal")
      continue;
    if (stmt.kind() == "Return" && !stmt.child("value") && !returned) {
      returned = true;
      continue;
    }
    if (stmt.kind() != "Assign" || assignment || returned)
      unsupported = true;
    if (stmt.kind() == "Assign") {
      auto targets = stmt.array("targets");
      auto target =
          targets && targets.size() == 1 ? stmt.item("targets", 0) : AstNode();
      candidate |= target && target.kind() == "Name" &&
                   rule.nonlocalNames.contains(target.string("id")) &&
                   stmt.child("value").kind() == "BinOp";
      assignment = stmt;
    }
  }
  if (!candidate)
    return std::optional<PythonNumericNextPlan>();
  if (unsupported || !assignment || rule.outputs.size() != 1 ||
      rule.inputs.size() != 1)
    return error()
           << "numeric next requires one unconditional masked assignment";
  auto targets = assignment.array("targets");
  auto target = targets && targets.size() == 1 ? assignment.item("targets", 0)
                                               : AstNode();
  const auto &member = module.members[rule.outputs.front()];
  const auto &argument = rule.arguments[rule.inputs.front()];
  if (!target || target.kind() != "Name" ||
      target.string("id") != member.name ||
      !rule.nonlocalNames.contains(member.name) ||
      member.kind != ModuleMember::Kind::OwnedState ||
      argument.memberIndex != rule.outputs.front())
    return error() << "numeric next requires one owned current/next register";
  auto domain = member.logicalType;
  auto lower = domain.getAs<ac::MathIntAttr>("lower");
  auto upper = domain.getAs<ac::MathIntAttr>("upper");
  if (!lower || !upper || lower.getCanonicalValue() != "0" ||
      upper.getCanonicalValue() != "256" || !member.payloadType.isInteger(8))
    return error() << "numeric next currently requires range(256) storage";
  PythonNumericNextPlan plan;
  plan.assignment = assignment;
  plan.result = assignment.child("value");
  plan.add = plan.result.child("left");
  plan.mask = plan.result.child("right");
  plan.current = plan.add.child("left");
  plan.increment = plan.add.child("right");
  plan.name = argument.localName;
  plan.domain = domain;
  if (plan.result.kind() != "BinOp" ||
      plan.result.child("op").kind() != "BitAnd" ||
      plan.add.kind() != "BinOp" || plan.add.child("op").kind() != "Add" ||
      plan.current.kind() != "Name" || plan.current.string("id") != plan.name ||
      !literalIs(plan.increment, "1") || !literalIs(plan.mask, "255"))
    return error() << "numeric next requires explicit (state + 1) & 255";
  return std::optional<PythonNumericNextPlan>(std::move(plan));
}

FailureOr<SmallVector<Value, 2>> emitPythonNumericNext(
    const PythonNumericNextPlan &p, Value current, DictionaryAttr state,
    Operation *rule, FlatSymbolRefAttr symbol, const AstNode &moduleDeclaration,
    StringRef path, OpBuilder &b) {
  auto origin = [&](const AstNode &n) {
    return occurrence(b, symbol, relativeToModule(moduleDeclaration, n));
  };
  auto loc = [&](const AstNode &n) { return n.location(b.getContext(), path); };
  auto make = [&](StringRef name, const AstNode &n, ValueRange inputs,
                  TypeRange outputs, ArrayRef<NamedAttribute> attrs) {
    return createSourceOperation(b, loc(n), name, inputs, outputs, attrs);
  };
  auto reg = rule->getAttrOfType<DictionaryAttr>("registration");
  auto spec =
      b.getDictionaryAttr({b.getNamedAttr("definition", symbol),
                           b.getNamedAttr("arguments", b.getArrayAttr({}))});
  rule->setAttr("ac.proof_scope",
                b.getDictionaryAttr({b.getNamedAttr("specialization", spec),
                                     b.getNamedAttr("registration", reg)}));
  Type math = ac::MathIntType::get(b.getContext());
  Value truth = arith::ConstantOp::create(b, loc(p.assignment), b.getI1Type(),
                                          b.getBoolAttr(true));
  auto *read = make(ac::SourceReadOp::getOperationName(), p.current, {current},
                    {current.getType()},
                    {b.getNamedAttr("ac.origin", origin(p.current))});
  auto *from = make(ac::MathFromBitsOp::getOperationName(), p.current,
                    {read->getResult(0)}, {math},
                    {b.getNamedAttr("ac.origin", origin(p.current)),
                     b.getNamedAttr("domain", p.domain)});
  auto one = ac::MathIntAttr::get(b.getContext(), llvm::APSInt("1"));
  auto mask = ac::MathIntAttr::get(b.getContext(), llvm::APSInt("255"));
  auto *k =
      make(ac::MathConstantOp::getOperationName(), p.increment, {}, {math},
           {b.getNamedAttr("ac.origin", origin(p.increment)),
            b.getNamedAttr("value", one)});
  auto *add = make(ac::MathBinaryOp::getOperationName(), p.add,
                   {truth, from->getResult(0), truth, k->getResult(0), truth},
                   {math, b.getI1Type()},
                   {b.getNamedAttr("ac.origin", origin(p.add)),
                    b.getNamedAttr("operator", b.getStringAttr("add"))});
  auto *m = make(ac::MathConstantOp::getOperationName(), p.mask, {}, {math},
                 {b.getNamedAttr("ac.origin", origin(p.mask)),
                  b.getNamedAttr("value", mask)});
  auto *bits = make(
      ac::MathBinaryOp::getOperationName(), p.result,
      {truth, add->getResult(0), add->getResult(1), m->getResult(0), truth},
      {math, b.getI1Type()},
      {b.getNamedAttr("ac.origin", origin(p.result)),
       b.getNamedAttr("operator", b.getStringAttr("and_bits"))});
  auto checkOrigin = origin(p.assignment);
  auto span = sourceSpan(b, path, p.assignment);
  auto checkID = b.getDictionaryAttr(
      {b.getNamedAttr("registration", reg),
       b.getNamedAttr("check", checkOrigin),
       b.getNamedAttr("obligation", b.getI64IntegerAttr(0))});
  auto checkTemplate =
      b.getDictionaryAttr({b.getNamedAttr("leaf", checkOrigin.get("site")),
                           b.getNamedAttr("kind", b.getStringAttr("range")),
                           b.getNamedAttr("obligation", b.getI64IntegerAttr(0)),
                           b.getNamedAttr("location", span)});
  auto *boundary = make(ac::MathToBitsOp::getOperationName(), p.result,
                        {truth, bits->getResult(0), bits->getResult(1)},
                        {b.getI8Type(), b.getI1Type()},
                        {b.getNamedAttr("domain", p.domain),
                         b.getNamedAttr("ac.origin", origin(p.result)),
                         b.getNamedAttr("ac.check_template", checkTemplate)});
  make(ac::SourceExpectOp::getOperationName(), p.assignment,
       {boundary->getResult(1), truth}, {},
       {b.getNamedAttr("kind", b.getStringAttr("range")),
        b.getNamedAttr("ac.check_id", checkID),
        b.getNamedAttr("location", span)});
  rule->setAttr("ac.required_checks",
                b.getArrayAttr({b.getDictionaryAttr(
                    {b.getNamedAttr("id", checkID),
                     b.getNamedAttr("kind", b.getStringAttr("range")),
                     b.getNamedAttr("location", span)})}));
  auto fID = id(b, origin(p.current), 1), kID = id(b, origin(p.increment));
  auto aID = id(b, origin(p.add)), mID = id(b, origin(p.mask));
  auto maskedID = id(b, origin(p.result)),
       boundaryID = id(b, origin(p.result), 1);
  auto none =
      b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("none"))});
  auto integerTarget = b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("integer_boundary")),
       b.getNamedAttr("domain", p.domain)});
  auto node = [&](DictionaryAttr valueID, StringRef op,
                  ArrayRef<Attribute> args, DictionaryAttr target) {
    return b.getDictionaryAttr(
        {b.getNamedAttr("id", valueID),
         b.getNamedAttr("operator", b.getStringAttr(op)),
         b.getNamedAttr("operands", b.getArrayAttr(args)),
         b.getNamedAttr("target", target)});
  };
  auto constantRef = [&](ac::MathIntAttr v) {
    return b.getDictionaryAttr(
        {b.getNamedAttr("kind", b.getStringAttr("constant")),
         b.getNamedAttr("value", v)});
  };
  rule->setAttr(
      "ac.required_numeric",
      b.getArrayAttr(
          {node(fID, "from_bits", {ref(b, "input", 0)}, none),
           node(kID, "constant", {constantRef(one)}, none),
           node(aID, "add", {ref(b, "node", 0), ref(b, "node", 1)}, none),
           node(mID, "constant", {constantRef(mask)}, none),
           node(maskedID, "and_bits", {ref(b, "node", 2), ref(b, "node", 3)},
                none),
           node(boundaryID, "to_bits", {ref(b, "node", 4)}, integerTarget)}));
  auto useID =
      b.getDictionaryAttr({b.getNamedAttr("origin", origin(p.assignment)),
                           b.getNamedAttr("role", b.getStringAttr("next")),
                           b.getNamedAttr("slot", b.getI32IntegerAttr(0))});
  auto target = b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("next_scalar")),
       b.getNamedAttr("state", state)});
  auto *use =
      make(ac::SourceUseOp::getOperationName(), p.assignment,
           {boundary->getResult(0), boundary->getResult(1), truth},
           {b.getI8Type(), b.getI1Type()},
           {b.getNamedAttr("id", useID), b.getNamedAttr("source", boundaryID),
            b.getNamedAttr("target", target)});
  rule->setAttr("ac.required_uses", b.getArrayAttr({b.getDictionaryAttr(
                                        {b.getNamedAttr("id", useID),
                                         b.getNamedAttr("value", boundaryID),
                                         b.getNamedAttr("target", target)})}));
  rule->setAttr(
      "ac.yield_bindings",
      b.getArrayAttr({b.getDictionaryAttr(
          {b.getNamedAttr("data_operand", b.getI32IntegerAttr(0)),
           b.getNamedAttr("enable_operand", b.getI32IntegerAttr(1)),
           b.getNamedAttr("target", state),
           b.getNamedAttr("contributions",
                          b.getArrayAttr({b.getDictionaryAttr(
                              {b.getNamedAttr("use", useID),
                               b.getNamedAttr("selection_ordinal",
                                              b.getUnitAttr())})}))})}));
  return SmallVector<Value, 2>{use->getResult(0), use->getResult(1)};
}
} // namespace acir::compiler::detail
