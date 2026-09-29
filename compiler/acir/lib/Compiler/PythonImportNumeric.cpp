#include "PythonImportNumeric.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/StringSwitch.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {

bool containsNumericAssignment(const AstNode &node) {
  if (!node)
    return false;
  if (node.kind() == "Assign" && (node.child("value").kind() == "BinOp" ||
                                  node.child("value").kind() == "Compare"))
    return true;
  for (NamedAttribute field : node.fields()) {
    if (auto child = dyn_cast<DictionaryAttr>(field.getValue());
        child && child.getAs<StringAttr>("kind")) {
      if (containsNumericAssignment(node.child(field.getName())))
        return true;
      continue;
    }
    auto array = dyn_cast<ArrayAttr>(field.getValue());
    if (!array)
      continue;
    for (size_t index = 0; index < array.size(); ++index)
      if (containsNumericAssignment(node.item(field.getName(), index)))
        return true;
  }
  return false;
}

StringAttr integerLiteralSpelling(const AstNode &node, bool &negative) {
  negative = false;
  AstNode constant = node;
  if (node.kind() == "UnaryOp" && node.child("op").kind() == "USub") {
    negative = true;
    constant = node.child("operand");
  }
  auto encoded = constant.kind() == "Constant"
                     ? dyn_cast_or_null<DictionaryAttr>(constant.get("value"))
                     : DictionaryAttr();
  return encoded && encoded.size() == 1 ? encoded.getAs<StringAttr>("integer")
                                        : StringAttr();
}

bool isIntegerLiteral(const AstNode &node) {
  bool negative = false;
  return static_cast<bool>(integerLiteralSpelling(node, negative));
}

StringRef binaryOpcode(const AstNode &operation) {
  return llvm::StringSwitch<StringRef>(operation.kind())
      .Case("Add", "add")
      .Case("Sub", "sub")
      .Default({});
}

StringRef compareOpcode(const AstNode &operation) {
  return llvm::StringSwitch<StringRef>(operation.kind())
      .Case("Eq", "eq")
      .Case("NotEq", "ne")
      .Case("Lt", "lt")
      .Case("LtE", "le")
      .Case("Gt", "gt")
      .Case("GtE", "ge")
      .Default({});
}

DictionaryAttr valueID(OpBuilder &builder, DictionaryAttr origin,
                       unsigned slot) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("origin", origin),
      builder.getNamedAttr("slot", builder.getI32IntegerAttr(slot)),
  });
}

DictionaryAttr numericRef(OpBuilder &builder, StringRef kind, unsigned index) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr(kind)),
      builder.getNamedAttr("index", builder.getI32IntegerAttr(index)),
  });
}

DictionaryAttr constantRef(OpBuilder &builder, ac::MathIntAttr value) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("constant")),
      builder.getNamedAttr("value", value),
  });
}

DictionaryAttr numericNode(OpBuilder &builder, DictionaryAttr id,
                           StringRef opcode, ArrayRef<Attribute> operands) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("id", id),
      builder.getNamedAttr("operator", builder.getStringAttr(opcode)),
      builder.getNamedAttr("operands", builder.getArrayAttr(operands)),
      builder.getNamedAttr("target",
                           builder.getDictionaryAttr({builder.getNamedAttr(
                               "kind", builder.getStringAttr("none"))})),
  });
}

FailureOr<ac::MathIntAttr> literalValue(const PythonImportNumericPlan &plan,
                                        OpBuilder &builder,
                                        ac::detail::EmitError emitError) {
  bool negative = false;
  auto spelling = integerLiteralSpelling(plan.literal, negative);
  if (!spelling)
    return emitError() << "numeric source literal must be an integer literal";
  std::string signedSpelling = negative
                                   ? (Twine("-") + spelling.getValue()).str()
                                   : spelling.getValue().str();
  return parseStaticInteger(builder, signedSpelling, emitError);
}

struct NumericIdentities {
  DictionaryAttr from;
  DictionaryAttr constant;
  DictionaryAttr result;
};

NumericIdentities identities(const PythonImportNumericPlan &plan,
                             FlatSymbolRefAttr moduleSymbol,
                             const AstNode &moduleDeclaration,
                             OpBuilder &builder) {
  auto origin = [&](const AstNode &node) {
    return occurrence(builder, moduleSymbol,
                      relativeToModule(moduleDeclaration, node));
  };
  // The SourceRead owns slot zero at the current-name occurrence. The lift is
  // a distinct value produced from the same syntax occurrence, so it owns the
  // next explicit producer slot. Constant and result producers each own slot
  // zero at their own source occurrence.
  return {valueID(builder, origin(plan.current), 1),
          valueID(builder, origin(plan.literal), 0),
          valueID(builder, origin(plan.result), 0)};
}

} // namespace

FailureOr<std::optional<PythonImportNumericPlan>>
analyzePythonImportNumericRule(const RulePlan &rule, const ModuleModel &module,
                               const AstNode &method,
                               ac::detail::EmitError emitError) {
  if (!containsNumericAssignment(method))
    return std::optional<PythonImportNumericPlan>();
  if (!rule.outputs.empty() || rule.inputs.size() != 1 ||
      rule.arguments.size() != 1)
    return emitError() << "numeric source rule requires exactly one current "
                          "integer input and no next-state target";

  const BoundRuleArgument &argument = rule.arguments[rule.inputs.front()];
  const ModuleMember &member = module.members[argument.memberIndex];
  auto kind = member.logicalType.getAs<StringAttr>("kind");
  if (!kind || kind.getValue() != "integer")
    return emitError()
           << "numeric source rule current input must have an exact "
              "integer LogicalType";

  AstNode assignment;
  ArrayAttr statements = method.array("body");
  bool returned = false;
  for (size_t index = 0; index < statements.size(); ++index) {
    AstNode statement = method.item("body", index);
    if (isModuleDocstring(statement))
      continue;
    if (statement.kind() == "Return" && !statement.child("value") &&
        !returned) {
      returned = true;
      continue;
    }
    if (returned)
      return emitError()
             << "numeric source rule has reachable syntax after return";
    if (statement.kind() != "Assign" || assignment)
      return emitError() << "numeric source rule admits one unused local "
                            "assignment and an optional bare return";
    assignment = statement;
  }
  if (!assignment)
    return emitError() << "numeric source rule has no numeric assignment";
  ArrayAttr targets = assignment.array("targets");
  AstNode target = targets && targets.size() == 1
                       ? assignment.item("targets", 0)
                       : AstNode();
  if (!target || target.kind() != "Name" ||
      !rule.localNames.contains(target.string("id")))
    return emitError() << "numeric source result must be one local name";

  PythonImportNumericPlan plan;
  plan.assignment = assignment;
  plan.result = assignment.child("value");
  plan.inputName = argument.localName;
  plan.targetName = target.string("id").str();
  plan.inputType = member.logicalType;
  if (plan.targetName == plan.inputName)
    return emitError() << "numeric source temporary cannot replace its current "
                          "input binding";

  if (plan.result.kind() == "BinOp") {
    plan.current = plan.result.child("left");
    plan.literal = plan.result.child("right");
    plan.opcode = binaryOpcode(plan.result.child("op")).str();
  } else if (plan.result.kind() == "Compare") {
    ArrayAttr operations = plan.result.array("ops");
    ArrayAttr comparators = plan.result.array("comparators");
    if (!operations || operations.size() != 1 || !comparators ||
        comparators.size() != 1)
      return emitError() << "numeric source comparison must have one predicate "
                            "and one literal comparator";
    plan.current = plan.result.child("left");
    plan.literal = plan.result.item("comparators", 0);
    plan.opcode = compareOpcode(plan.result.item("ops", 0)).str();
    plan.comparison = true;
  } else {
    return emitError()
           << "numeric source assignment requires current + literal, "
              "current - literal, or one scalar comparison";
  }
  if (plan.current.kind() != "Name" ||
      plan.current.string("id") != plan.inputName ||
      !isIntegerLiteral(plan.literal) || plan.opcode.empty())
    return emitError() << "numeric source expression must use its sole current "
                          "input on the left and one supported integer literal "
                          "on the right";
  return std::optional<PythonImportNumericPlan>(std::move(plan));
}

void attachPythonImportNumericContract(const PythonImportNumericPlan &plan,
                                       Operation *rule,
                                       FlatSymbolRefAttr moduleSymbol,
                                       OpBuilder &builder) {
  auto registration = rule->getAttrOfType<DictionaryAttr>("registration");
  auto specialization = builder.getDictionaryAttr({
      builder.getNamedAttr("definition", moduleSymbol),
      builder.getNamedAttr("arguments", builder.getArrayAttr({})),
  });
  rule->setAttr("ac.proof_scope",
                builder.getDictionaryAttr({
                    builder.getNamedAttr("specialization", specialization),
                    builder.getNamedAttr("registration", registration),
                }));
  (void)plan;
}

LogicalResult emitPythonImportNumericRecipe(
    const PythonImportNumericPlan &plan, Value current,
    FlatSymbolRefAttr moduleSymbol, const AstNode &moduleDeclaration,
    StringRef sourcePath, OpBuilder &builder, ac::detail::EmitError emitError) {
  auto value = literalValue(plan, builder, emitError);
  if (failed(value))
    return failure();
  auto origin = [&](const AstNode &node) {
    return occurrence(builder, moduleSymbol,
                      relativeToModule(moduleDeclaration, node));
  };
  Location currentLocation =
      plan.current.location(builder.getContext(), sourcePath);
  Operation *read = createSourceOperation(
      builder, currentLocation, ac::SourceReadOp::getOperationName(),
      ValueRange{current}, TypeRange{current.getType()},
      {builder.getNamedAttr("ac.origin", origin(plan.current))});
  Operation *from = createSourceOperation(
      builder, currentLocation, ac::MathFromBitsOp::getOperationName(),
      ValueRange{read->getResult(0)},
      TypeRange{ac::MathIntType::get(builder.getContext())},
      {builder.getNamedAttr("domain", plan.inputType),
       builder.getNamedAttr("ac.origin", origin(plan.current))});
  Operation *constant = createSourceOperation(
      builder, plan.literal.location(builder.getContext(), sourcePath),
      ac::MathConstantOp::getOperationName(), {},
      TypeRange{ac::MathIntType::get(builder.getContext())},
      {builder.getNamedAttr("value", *value),
       builder.getNamedAttr("ac.origin", origin(plan.literal))});
  Value truth = arith::ConstantOp::create(
      builder, plan.result.location(builder.getContext(), sourcePath),
      builder.getI1Type(), builder.getBoolAttr(true));
  Operation *operation = createSourceOperation(
      builder, plan.result.location(builder.getContext(), sourcePath),
      plan.comparison ? ac::MathCompareOp::getOperationName()
                      : ac::MathBinaryOp::getOperationName(),
      ValueRange{truth, from->getResult(0), truth, constant->getResult(0),
                 truth},
      plan.comparison ? TypeRange{builder.getI1Type(), builder.getI1Type()}
                      : TypeRange{ac::MathIntType::get(builder.getContext()),
                                  builder.getI1Type()},
      {builder.getNamedAttr(plan.comparison ? "predicate" : "operator",
                            builder.getStringAttr(plan.opcode)),
       builder.getNamedAttr("ac.origin", origin(plan.result))});
  (void)operation;

  NumericIdentities ids =
      identities(plan, moduleSymbol, moduleDeclaration, builder);
  auto fromNode = numericNode(builder, ids.from, "from_bits",
                              {numericRef(builder, "input", 0)});
  auto constantNode = numericNode(builder, ids.constant, "constant",
                                  {constantRef(builder, *value)});
  auto resultNode = numericNode(
      builder, ids.result, plan.opcode,
      {numericRef(builder, "node", 0), numericRef(builder, "node", 1)});
  Operation *rule = builder.getInsertionBlock()->getParentOp();
  rule->setAttr("ac.required_numeric",
                builder.getArrayAttr({fromNode, constantNode, resultNode}));
  return success();
}

} // namespace acir::compiler::detail
