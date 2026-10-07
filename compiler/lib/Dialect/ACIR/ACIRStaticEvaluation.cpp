#include "ACIRStaticEvaluation.h"

#include "SourceStaticRecordConstructors.h"
#include "SourceValueSemantics.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"

using namespace mlir;

namespace acir::ac::detail {
namespace {

enum class PackedKind { Boolean, Integer, Record, List };
struct Kind {
  PackedKind kind;
  FlatSymbolRefAttr record;
  bool operator==(const Kind &other) const {
    return kind == other.kind && record == other.record;
  }
};
using KindSet = SmallVector<Kind>;

Kind valueKind(DictionaryAttr value) {
  auto kind = value.getAs<StringAttr>("kind").getValue();
  if (kind == "bool")
    return {PackedKind::Boolean, {}};
  if (kind == "integer")
    return {PackedKind::Integer, {}};
  if (kind == "record")
    return {PackedKind::Record, value.getAs<FlatSymbolRefAttr>("symbol")};
  return {PackedKind::List, {}};
}

bool compatible(const KindSet &kinds, ValueKindConstraint constraint) {
  return llvm::any_of(kinds, [&](Kind kind) {
    return (kind.kind == PackedKind::Boolean &&
            constraint != ValueKindConstraint::Integer) ||
           (kind.kind == PackedKind::Integer &&
            constraint != ValueKindConstraint::Boolean);
  });
}

bool compatible(const KindSet &kinds, DictionaryAttr logical) {
  auto kind = logical.getAs<StringAttr>("kind").getValue();
  return llvm::any_of(kinds, [&](Kind candidate) {
    return (kind == "bool" && candidate.kind == PackedKind::Boolean) ||
           (kind == "integer" && candidate.kind == PackedKind::Integer) ||
           (kind == "record" && candidate.kind == PackedKind::Record &&
            candidate.record == logical.getAs<FlatSymbolRefAttr>("symbol"));
  });
}

DictionaryAttr wrap(Attribute value) {
  Builder builder(value.getContext());
  return builder.getDictionaryAttr(
      {builder.getNamedAttr(
           "kind",
           builder.getStringAttr(isa<BoolAttr>(value) ? "bool" : "integer")),
       builder.getNamedAttr("value", value)});
}

class Evaluation {
public:
  Evaluation(Operation *site, EmitError error) : site(site), error(error) {}
  FailureOr<KindSet> preflight(StaticExprAttr expression);
  FailureOr<DictionaryAttr> evaluate(StaticExprAttr expression);

private:
  FailureOr<KindSet> inspect(StaticExprAttr expression);
  FailureOr<DictionaryAttr> compute(StaticExprAttr expression);
  FailureOr<KindSet> constructorKinds(StaticExprAttr expression);
  Operation *site;
  EmitError error;
  llvm::DenseMap<Attribute, KindSet> kinds;
  llvm::DenseMap<Attribute, DictionaryAttr> values;
  llvm::DenseMap<Attribute, func::FuncOp> constructors;
  llvm::DenseSet<Attribute> activeKinds, activeValues;
  llvm::DenseSet<Operation *> activeConstructors;
};

FailureOr<KindSet> Evaluation::preflight(StaticExprAttr expression) {
  auto found = kinds.find(expression);
  if (found != kinds.end())
    return found->second;
  if (!activeKinds.insert(expression).second)
    return error() << "cycle in static expression kind analysis";
  llvm::scope_exit cleanup([&] { activeKinds.erase(expression); });
  auto result = inspect(expression);
  if (succeeded(result))
    kinds.try_emplace(expression, *result);
  return result;
}

FailureOr<KindSet> Evaluation::constructorKinds(StaticExprAttr expression) {
  auto tree = expression.getTree();
  auto callee = tree.getAs<FlatSymbolRefAttr>("callee");
  if (!callee)
    return error()
           << "lexical static calls require source authority resolution";
  auto helper = resolveSourceStaticRecordConstructor(callee, site, error);
  if (failed(helper))
    return failure();
  auto arguments = tree.getAs<ArrayAttr>("arguments");
  auto mapping = bindStaticRecordArguments(*helper, arguments, error);
  if (failed(mapping))
    return failure();
  auto parameters = (*helper)->getAttrOfType<ArrayAttr>("ac.parameters");
  for (auto [index, raw] : llvm::enumerate(arguments)) {
    auto argument = cast<DictionaryAttr>(raw).getAs<StaticExprAttr>("value");
    auto actualKinds = preflight(argument);
    if (failed(actualKinds))
      return failure();
    auto parameter = cast<DictionaryAttr>(parameters[(*mapping)[index]]);
    auto logical = parameter.getAs<DictionaryAttr>("constraint")
                       .getAs<DictionaryAttr>("type");
    if (!compatible(*actualKinds, logical))
      return error()
             << "static constructor argument has definitely incompatible kind";
  }
  llvm::DenseSet<unsigned> supplied;
  for (unsigned ordinal : *mapping)
    supplied.insert(ordinal);
  for (auto [ordinal, raw] : llvm::enumerate(parameters)) {
    if (supplied.contains(static_cast<unsigned>(ordinal)))
      continue;
    auto parameter = cast<DictionaryAttr>(raw);
    auto value =
        parameter.getAs<DictionaryAttr>("default").getAs<DictionaryAttr>(
            "value");
    auto logical = parameter.getAs<DictionaryAttr>("constraint")
                       .getAs<DictionaryAttr>("type");
    if (!value || !compatible(KindSet{valueKind(value)}, logical))
      return error()
             << "static constructor default has definitely incompatible kind";
  }
  constructors.try_emplace(expression, *helper);
  return KindSet{{PackedKind::Record,
                  (*helper)->getAttrOfType<FlatSymbolRefAttr>("ac.record")}};
}

FailureOr<KindSet> Evaluation::inspect(StaticExprAttr expression) {
  auto tree = expression.getTree();
  auto tag = tree.getAs<StringAttr>("kind").getValue();
  if (tag == "literal") {
    auto literal = tree.getAs<DictionaryAttr>("value");
    if (failed(verifyStaticValueStructure(literal, error)))
      return failure();
    return KindSet{valueKind(literal)};
  }
  if (tag == "call")
    return constructorKinds(expression);
  if (tag == "select") {
    auto condition = preflight(tree.getAs<StaticExprAttr>("condition"));
    if (failed(condition))
      return failure();
    auto yes = preflight(tree.getAs<StaticExprAttr>("yes"));
    if (failed(yes))
      return failure();
    auto no = preflight(tree.getAs<StaticExprAttr>("no"));
    if (failed(no))
      return failure();
    if (!compatible(*condition, ValueKindConstraint::Boolean))
      return error()
             << "static select condition has definitely nonboolean kind";
    auto conditionTree = tree.getAs<StaticExprAttr>("condition").getTree();
    if (conditionTree.getAs<StringAttr>("kind").getValue() == "literal") {
      auto literal = conditionTree.getAs<DictionaryAttr>("value");
      if (auto boolean = literal.getAs<BoolAttr>("value"))
        return boolean.getValue() ? *yes : *no;
    }
    KindSet joined = *yes;
    for (Kind kind : *no)
      if (!llvm::is_contained(joined, kind))
        joined.push_back(kind);
    return joined;
  }
  if (tag != "unary" && tag != "binary")
    return error() << "static expression category requires deferred "
                      "source/helper authority or semantics";
  auto opcode = parseValueOpcode(tree.getAs<StringAttr>("operator").getValue());
  if (!opcode)
    return error() << "unknown static value opcode";
  auto info = getValueOpcodeInfo(*opcode);
  if (info.arity != (tag == "unary" ? 1u : 2u))
    return error() << "static value opcode arity differs from expression shape";
  for (unsigned index = 0; index < info.arity; ++index) {
    auto child = tree.getAs<StaticExprAttr>(tag == "unary" ? "operand"
                                            : index == 0   ? "lhs"
                                                           : "rhs");
    auto childKinds = preflight(child);
    if (failed(childKinds))
      return failure();
    if (!compatible(*childKinds, info.operands[index]))
      return error() << "static value operand has definitely incompatible "
                        "bool/integer kind";
  }
  return KindSet{{info.result == ValueKind::Boolean ? PackedKind::Boolean
                                                    : PackedKind::Integer,
                  {}}};
}

FailureOr<DictionaryAttr> Evaluation::evaluate(StaticExprAttr expression) {
  auto found = values.find(expression);
  if (found != values.end())
    return found->second;
  if (!activeValues.insert(expression).second)
    return error() << "cycle in demanded static expression";
  llvm::scope_exit cleanup([&] { activeValues.erase(expression); });
  auto result = compute(expression);
  if (succeeded(result))
    values.try_emplace(expression, *result);
  return result;
}

FailureOr<DictionaryAttr> Evaluation::compute(StaticExprAttr expression) {
  auto tree = expression.getTree();
  auto tag = tree.getAs<StringAttr>("kind").getValue();
  if (tag == "literal")
    return tree.getAs<DictionaryAttr>("value");
  if (tag == "call")
    return evaluateSourceStaticRecordConstructor(
        constructors.lookup(expression), tree.getAs<ArrayAttr>("arguments"),
        site, activeConstructors,
        [&](StaticExprAttr child) { return evaluate(child); }, error);
  if (tag == "select") {
    auto condition = evaluate(tree.getAs<StaticExprAttr>("condition"));
    if (failed(condition))
      return failure();
    auto boolean = (*condition).getAs<BoolAttr>("value");
    if ((*condition).getAs<StringAttr>("kind").getValue() != "bool" || !boolean)
      return error() << "selected static condition must be bool, without "
                        "integer truthiness";
    return evaluate(
        tree.getAs<StaticExprAttr>(boolean.getValue() ? "yes" : "no"));
  }
  auto opcode =
      *parseValueOpcode(tree.getAs<StringAttr>("operator").getValue());
  auto info = getValueOpcodeInfo(opcode);
  auto lhs =
      evaluate(tree.getAs<StaticExprAttr>(tag == "unary" ? "operand" : "lhs"));
  if (failed(lhs))
    return failure();
  auto lhsKind = valueKind(*lhs);
  if (!compatible(KindSet{lhsKind}, info.operands[0]))
    return error()
           << "selected static operand has incompatible bool/integer kind";
  SmallVector<Attribute> operands{(*lhs).get("value")};
  if (info.evaluation != ValueEvaluation::Strict) {
    bool boolean = cast<BoolAttr>(operands.front()).getValue();
    if ((info.evaluation == ValueEvaluation::AndShortCircuit && !boolean) ||
        (info.evaluation == ValueEvaluation::OrShortCircuit && boolean))
      return *lhs;
  }
  if (info.arity == 2) {
    auto rhs = evaluate(tree.getAs<StaticExprAttr>("rhs"));
    if (failed(rhs))
      return failure();
    if (!compatible(KindSet{valueKind(*rhs)}, info.operands[1]))
      return error()
             << "selected static operand has incompatible bool/integer kind";
    operands.push_back((*rhs).get("value"));
  }
  auto value = evaluateValue(opcode, operands, error);
  if (failed(value))
    return failure();
  return wrap(*value);
}
} // namespace

FailureOr<DictionaryAttr> evaluateSourceStaticExpr(StaticExprAttr expression,
                                                   Operation *site,
                                                   EmitError error) {
  if (!expression ||
      failed(StaticExprAttr::verify(error, expression.getTree())))
    return error() << "static evaluation requires the complete valid "
                      "StaticExpr grammar";
  Evaluation evaluation(site, error);
  if (failed(evaluation.preflight(expression)))
    return failure();
  return evaluation.evaluate(expression);
}
} // namespace acir::ac::detail
