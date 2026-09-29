#include "PythonImportRuleControl.h"
#include "PythonImportContext.h"
#include "PythonImportObservations.h"
#include "mlir/Dialect/Arith/IR/Arith.h"

using namespace mlir;
namespace acir::compiler::detail {

bool needsComposedRule(const AstNode &method) {
  auto inspect = [&](auto &&self, const AstNode &node, bool nested) -> bool {
    if (!node)
      return false;
    if (node.kind() == "If" && !node.array("orelse").empty()) {
      llvm::StringSet<> thenTargets;
      for (size_t i = 0; i < node.array("body").size(); ++i) {
        auto assignment = node.item("body", i);
        if (assignment.kind() == "Assign" && assignment.array("targets").size() == 1)
          thenTargets.insert(assignment.item("targets", 0).string("id"));
      }
      for (size_t i = 0; i < node.array("orelse").size(); ++i) {
        auto assignment = node.item("orelse", i);
        if (assignment.kind() == "Assign" && assignment.array("targets").size() == 1 &&
            thenTargets.contains(assignment.item("targets", 0).string("id")))
          return true;
      }
    }
    if ((node.kind() == "Compare" || node.kind() == "BoolOp") && nested)
      return true;
    if (node.kind() == "BinOp" && nested)
      return true;
    for (NamedAttribute field : node.fields()) {
      if (auto child = dyn_cast<DictionaryAttr>(field.getValue());
          child && child.getAs<StringAttr>("kind")) {
        if (self(self, node.child(field.getName()),
                 nested || node.kind() == "If" || node.kind() == "Assert"))
          return true;
      } else if (auto array = dyn_cast<ArrayAttr>(field.getValue())) {
        for (size_t i = 0; i < array.size(); ++i)
          if (self(self, node.item(field.getName(), i),
                   nested || node.kind() == "If" || node.kind() == "Assert"))
            return true;
      }
    }
    return false;
  };
  return inspect(inspect, method, false);
}

LogicalResult emitComposedRule(
    OpBuilder &b, ac::RuleOp rule, const RulePlan &plan,
    const ModuleModel &module, const AstNode &method, StringRef sourcePath,
    const llvm::StringMap<Value> &entries,
    const llvm::StringMap<DictionaryAttr> &types,
    llvm::function_ref<StringRef(const AstNode &)> observationIntrinsic,
    ac::detail::EmitError error) {
  llvm::StringMap<Value> boundValues(entries);
  llvm::StringMap<DictionaryAttr> boundTypes(types);
  RuleExpressionProducer expressions(b, rule, module, sourcePath, boundValues,
                                     boundTypes, error);
  llvm::StringMap<Value> locals;
  PythonImportObservationProducer observations(
      b, sourcePath, module.symbol, module.declaration,
      rule.getRegistrationAttr(), error, locals, entries, types);
  SmallVector<Attribute> requiredObservations, requiredUses, yieldBindings;
  struct Contribution {
    Value data;
    Value enable;
    DictionaryAttr id;
  };
  SmallVector<SmallVector<Contribution>> contributions(plan.outputs.size());
  auto bindings = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto boolDomain =
      b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("bool")), b.getNamedAttr("storage", TypeAttr::get(b.getI1Type()))});
  auto assignment = [&](const AstNode &statement, Value &live,
                        unsigned depth) -> LogicalResult {
    auto targets = statement.array("targets");
    auto name = targets.size() == 1 ? statement.item("targets", 0) : AstNode();
    if (name && name.kind() == "Name" &&
        plan.localNames.contains(name.string("id"))) {
      if (depth || statement.child("value").kind() != "Name")
        return error() << "composed local snapshot requires an unconditional "
                          "current read";
      auto value = expressions.read(statement.child("value"), live);
      if (failed(value))
        return failure();
      boundValues[name.string("id")] = value->value;
      boundTypes[name.string("id")] = value->logical;
      locals[name.string("id")] = value->value;
      return success();
    }
    if (!name || name.kind() != "Name" ||
        !plan.nonlocalNames.contains(name.string("id")))
      return error()
             << "composed rule assignment requires a nonlocal scalar target";
    std::optional<size_t> ordinal;
    for (auto [i, member] : llvm::enumerate(plan.outputs))
      if (module.members[member].name == name.string("id"))
        ordinal = i;
    if (!ordinal)
      return error() << "composed rule target is absent from output bindings";
    const auto &member = module.members[plan.outputs[*ordinal]];
    AstNode rhs = statement.child("value");
    auto value = rhs.kind() == "Name" ? expressions.read(rhs, live)
                                      : expressions.emit(rhs, live);
    if (failed(value))
      return failure();
    Value evaluationPath = live;
    if (member.logicalType != boolDomain && value->numeric) {
      auto converted = expressions.boundary(rhs, statement, *value, live,
                                            member.logicalType);
      if (failed(converted))
        return failure();
      *value = *converted;
    } else if (value->logical != member.logicalType) {
      return error() << "next-state assignment requires an exact logical type";
    }
    live = expressions.continueAfter(live, evaluationPath, value->valid,
                                     statement);
    auto id = b.getDictionaryAttr(
        {b.getNamedAttr("origin",
                        expressions.identity(statement).get("origin")),
         b.getNamedAttr("role", b.getStringAttr("next")),
         b.getNamedAttr("slot", b.getI32IntegerAttr(0))});
    auto target = b.getDictionaryAttr(
        {b.getNamedAttr("kind", b.getStringAttr("next_scalar")),
         b.getNamedAttr("state", bindings[*ordinal])});
    auto *use = createSourceOperation(
        b, statement.location(b.getContext(), sourcePath),
        ac::SourceUseOp::getOperationName(),
        {value->value, value->valid, evaluationPath},
        {member.payloadType, b.getI1Type()},
        {b.getNamedAttr("id", id), b.getNamedAttr("source", value->id),
         b.getNamedAttr("target", target)});
    requiredUses.push_back(b.getDictionaryAttr(
        {b.getNamedAttr("id", id), b.getNamedAttr("value", value->id),
         b.getNamedAttr("target", target)}));
    contributions[*ordinal].push_back(
        {use->getResult(0), use->getResult(1), id});
    return success();
  };
  auto statements = [&](auto &&self, const AstNode &parent, StringRef field,
                        Value live, unsigned depth) -> FailureOr<Value> {
    auto body = parent.array(field);
    for (size_t i = 0; i < body.size(); ++i) {
      auto statement = parent.item(field, i);
      if (isModuleDocstring(statement) || statement.kind() == "Nonlocal")
        continue;
      if (statement.kind() == "Assign") {
        if (failed(assignment(statement, live, depth)))
          return failure();
      } else if (statement.kind() == "Assert") {
        if (failed(expressions.assertion(statement, live)))
          return failure();
      } else if (statement.kind() == "Expr") {
        auto emitted =
            observations.emit(statement, observationIntrinsic(statement), live,
                              requiredObservations);
        if (failed(emitted) || !*emitted)
          return error()
                 << "composed rule expression must be a standard observation";
      } else if (statement.kind() == "If") {
        auto condition = expressions.emit(statement.child("test"), live);
        if (failed(condition) || condition->logical != boolDomain)
          return error() << "if condition requires source bool";
        Value valid =
            expressions.conjunction(live, condition->valid, statement);
        Value yes = expressions.conjunction(valid, condition->value, statement);
        Value no = expressions.conjunction(
            valid, expressions.invert(condition->value, statement), statement);
        auto yesLive = self(self, statement, "body", yes, depth + 1);
        auto noLive = self(self, statement, "orelse", no, depth + 1);
        if (failed(yesLive) || failed(noLive))
          return failure();
        live = expressions.disjunction(*yesLive, *noLive, statement);
      } else if (statement.kind() == "Return" && !statement.child("value") &&
                 i + 1 == body.size()) {
        // A return from an arm must not execute statements after the branch.
        return expressions.boolean(false, statement);
      } else {
        return error() << "unsupported composed rule statement";
      }
    }
    return live;
  };
  if (failed(statements(statements, method, "body",
                        expressions.boolean(true, method), 0)))
    return failure();
  SmallVector<Value> yields;
  for (auto [ordinal, parts] : llvm::enumerate(contributions)) {
    if (parts.empty())
      return error() << "nonlocal target has no source assignment";
    auto type =
        cast<IntegerType>(module.members[plan.outputs[ordinal]].payloadType);
    auto loc = method.location(b.getContext(), sourcePath);
    Value data = arith::ConstantOp::create(b, loc, b.getIntegerAttr(type, 0));
    Value enable = expressions.boolean(false, method);
    SmallVector<Attribute> uses;
    for (auto part : parts) {
      data = arith::SelectOp::create(b, loc, part.enable, part.data, data);
      enable = expressions.disjunction(enable, part.enable, method);
      uses.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("use", part.id),
           b.getNamedAttr("selection_ordinal", b.getUnitAttr())}));
    }
    yields.push_back(data);
    yields.push_back(enable);
    yieldBindings.push_back(b.getDictionaryAttr(
        {b.getNamedAttr("data_operand", b.getI32IntegerAttr(2 * ordinal)),
         b.getNamedAttr("enable_operand", b.getI32IntegerAttr(2 * ordinal + 1)),
         b.getNamedAttr("target", bindings[ordinal]),
         b.getNamedAttr("contributions", b.getArrayAttr(uses))}));
  }
  expressions.finish();
  rule->setAttr("ac.required_uses", b.getArrayAttr(requiredUses));
  rule->setAttr("ac.yield_bindings", b.getArrayAttr(yieldBindings));
  if (!requiredObservations.empty())
    rule->setAttr("ac.required_observations",
                  b.getArrayAttr(requiredObservations));
  createSourceOperation(b, method.location(b.getContext(), sourcePath),
                        ac::YieldOp::getOperationName(), yields, {}, {});
  return success();
}
} // namespace acir::compiler::detail
