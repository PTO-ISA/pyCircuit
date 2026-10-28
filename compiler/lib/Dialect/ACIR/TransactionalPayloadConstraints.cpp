#include "mlir/IR/Builders.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include <deque>

using namespace mlir;
namespace acir::ac {
namespace {
bool unknown(Type type) { return isa<InferType>(type); }

// One description of the operand relation is shared by propagation and solving.
struct OperationRule {
  unsigned start;
  bool paired;
  bool shift;
  bool comparison;
};
OperationRule operationRule(PendingExprOp expr) {
  auto kind = expr.getKind();
  bool binary = kind == "binary";
  bool comparison = kind == "compare";
  return {kind == "select" ? 1u : 0u, binary || comparison || kind == "select",
          binary && expr.getOpcodeAttr() &&
              (expr.getOpcodeAttr().getValue() == "shl" ||
               expr.getOpcodeAttr().getValue() == "lshr"),
          comparison};
}

BitsType constantWidth(Builder &builder, unsigned width,
                       StaticExprAttr provenance) {
  auto tree = provenance.getTree();
  auto number = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
       builder.getNamedAttr(
           "value",
           MathIntAttr::get(builder.getContext(),
                            llvm::APSInt(llvm::APInt(64, width), true)))});
  return BitsType::get(
      builder.getContext(),
      StaticExprAttr::get(
          builder.getContext(),
          builder.getDictionaryAttr(
              {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
               builder.getNamedAttr("value", number),
               builder.getNamedAttr("origin", tree.get("origin")),
               builder.getNamedAttr("location", tree.get("location"))})));
}

// Facts belong to Values. Requests describe conversion contexts, not equality
// with an operation's result: a wider return never widens a typed addition.
class PayloadConstraints {
  mlir::ModuleOp package;
  HardwareAnalysis hardware;
  struct Context {
    Type type;
    Operation *source;
  };
  llvm::DenseMap<Value, Type> facts, observedAnchors;
  llvm::DenseSet<Value> anchoring;
  llvm::DenseSet<Operation *> excludedRelations;
  llvm::DenseMap<Value, SmallVector<Context>> requests;
  llvm::DenseMap<Value, SmallVector<Value>> aliases;
  std::deque<Value> work;
  SmallVector<PendingExprOp> expressions;
  // Rewrite consumes this solved plan; it never chooses operation types.
  struct OperationTypes {
    Type execution;
    Type result;
  };
  llvm::DenseMap<Operation *, OperationTypes> operationTypes;
  bool invalid = false;
  unsigned budget = 1048576;

  unsigned width(Type type, Operation *site) {
    auto result = hardware.getPackedWidth(type, {}, site);
    return succeeded(result) ? *result : 0;
  }
  Type common(Type a, Type b, Operation *site) {
    if (!a)
      return b;
    if (!b)
      return a;
    if (transactionalTypesEqual(a, b))
      return a;
    if (isa<BitsType>(a) && isa<BitsType>(b))
      return width(a, site) >= width(b, site) ? a : b;
    site->emitOpError("conflicting nominal payload constraints");
    invalid = true;
    return {};
  }
  Type contextBound(Type a, Type b, Operation *site) {
    if (!a)
      return b;
    if (!b || transactionalTypesEqual(a, b))
      return a;
    if (isa<BitsType>(a) && isa<BitsType>(b))
      return width(a, site) <= width(b, site) ? a : b;
    site->emitOpError("conflicting nominal payload contexts");
    invalid = true;
    return {};
  }
  Type fact(Value value) {
    if (!unknown(value.getType()))
      return value.getType();
    return facts.lookup(value);
  }
  Type anchor(Value value) {
    if (auto type = fact(value))
      return type;
    if (!anchoring.insert(value).second)
      return {};
    auto relation = value.getDefiningOp<PendingExprOp>();
    bool inserted = relation && excludedRelations.insert(relation).second;
    auto restoreExclusion = llvm::scope_exit([&] {
      if (inserted)
        excludedRelations.erase(relation);
    });
    Type result;
    if (auto expr = relation) {
      auto kind = expr.getKind();
      if (kind == "binary" || kind == "select") {
        auto rule = operationRule(expr);
        unsigned start = rule.start;
        result = anchor(expr.getOperand(start));
        if (!rule.shift)
          result = common(result, anchor(expr.getOperand(start + 1)), expr);
      } else if (kind == "unary")
        result = anchor(expr.getOperand(0));
    }
    // Capture aliases supply conversion bounds, never a wider replacement for
    // the producer's natural operation type.
    if (!result)
      for (Value other : aliases[value])
        result = contextBound(result, anchor(other),
                              value.getParentBlock()->getParentOp());
    // A relation's own propagated context is not an independent type anchor.
    // Preserve this exclusion through nested expressions and capture aliases.
    if (!result) {
      auto found = requests.find(value);
      if (found != requests.end())
        for (const auto &context : found->second) {
          if (excludedRelations.contains(context.source))
            continue;
          result = contextBound(result, context.type,
                                value.getParentBlock()->getParentOp());
        }
    }
    anchoring.erase(value);
    return result;
  }
  bool waiting(Value value, llvm::DenseSet<Value> &active) {
    if (fact(value) || !active.insert(value).second)
      return false;
    if (value.getDefiningOp<TransactionMethodPendingOp>())
      return true;
    if (auto expr = value.getDefiningOp<PendingExprOp>())
      for (Value operand : expr.getInputs())
        if (waiting(operand, active))
          return true;
    for (Value other : aliases[value])
      if (waiting(other, active))
        return true;
    return false;
  }
  void request(Value value, Type type, Operation *source) {
    if (!type || unknown(type))
      return;
    auto &types = requests[value];
    if (llvm::any_of(types, [&](const Context &prior) {
          return source == prior.source &&
                 transactionalTypesEqual(type, prior.type);
        }))
      return;
    types.push_back({type, source});
    work.push_back(value);
  }
  void connect(Value a, Value b) {
    aliases[a].push_back(b);
    aliases[b].push_back(a);
  }
  void propagate(PendingExprOp expr, Type context = {}) {
    auto kind = expr.getKind();
    excludedRelations.insert(expr);
    auto restoreExclusion =
        llvm::scope_exit([&] { excludedRelations.erase(expr); });
    if (kind == "convert") {
      request(expr.getOperand(0), expr.getResult().getType(), expr);
    } else if (kind == "binary" || kind == "compare" || kind == "select") {
      llvm::DenseSet<Value> active;
      for (Value operand : expr.getInputs())
        if (waiting(operand, active))
          return;

      auto rule = operationRule(expr);
      unsigned start = rule.start;
      Type a = anchor(expr.getOperand(start));
      Type b = anchor(expr.getOperand(start + 1));
      bool shift = rule.shift;
      Type type = shift ? (a ? a : context) : common(a, b, expr);
      if (!type && kind != "compare")
        type = context;
      request(expr.getOperand(start), type, expr);
      request(expr.getOperand(start + 1), shift && b ? b : type, expr);
      // Conversion contexts remain bounds even while another provisional
      // context supplies an anchor. Do not freeze that earlier choice.
      if (kind != "compare") {
        request(expr.getOperand(start), context, expr);
        if (!shift)
          request(expr.getOperand(start + 1), context, expr);
      }
    } else if (kind == "unary") {
      Type type = anchor(expr.getOperand(0));
      request(expr.getOperand(0), type ? type : context, expr);
      request(expr.getOperand(0), context, expr);
    }
  }
  LogicalResult setFact(Value value, Type type) {
    if (!type)
      return success();
    auto existing = fact(value);
    if (existing && !transactionalTypesEqual(existing, type))
      return emitError(value.getLoc(), "conflicting exact SSA payload types");
    if (!existing) {
      facts[value] = type;
      work.push_back(value);
    }
    return success();
  }
  LogicalResult infer(PendingExprOp expr) {
    auto kind = expr.getKind();
    if (kind == "literal" || kind == "convert")
      return success();
    for (Value input : expr.getInputs())
      if (!fact(input))
        return success();
    auto rule = operationRule(expr);
    Type execution = fact(expr.getOperand(rule.start));
    if (rule.paired)
      execution =
          common(execution, fact(expr.getOperand(rule.start + 1)), expr);
    if (!execution || invalid)
      return failure();
    Type result = rule.comparison ? fact(expr.getResult())
                  : rule.shift    ? fact(expr.getOperand(0))
                                  : execution;
    operationTypes[expr] = {execution, result};
    return setFact(expr.getResult(), result);
  }

  Value resize(OpBuilder &builder, Value value, Type type) {
    if (transactionalTypesEqual(value.getType(), type))
      return value;
    OperationState state(value.getLoc(), "ac.bits.resize");
    state.addOperands(value);
    state.addTypes(type);
    state.addAttribute("mode",
                       builder.getStringAttr(
                           width(type, value.getParentBlock()->getParentOp()) <
                                   width(value.getType(),
                                         value.getParentBlock()->getParentOp())
                               ? "trunc"
                               : "zext"));
    return builder.create(state)->getResult(0);
  }
  LogicalResult rewrite(PendingExprOp expr) {
    Type result = fact(expr.getResult());
    if (!result)
      return success();
    for (Value input : expr.getInputs())
      if (!fact(input))
        return success();
    auto kind = expr.getKind();
    OpBuilder builder(expr);
    OperationState state(expr.getLoc(), "ac.bits.constant4");
    SmallVector<Value> inputs(expr.getInputs());
    NamedAttrList attrs;
    if (auto origin = expr->getAttr("ac.origin"))
      attrs.set("ac.origin", origin);
    Type output = result;
    if (kind == "literal") {
      auto number = llvm::APSInt(
          cast<MathIntAttr>(expr.getNumberAttr()).getCanonicalValue());
      unsigned size = width(result, expr);
      if (!isa<BitsType>(result) || !size ||
          (!number.isNegative() && number.getActiveBits() > size) ||
          (number.isNegative() && number.getSignificantBits() > size))
        return expr.emitOpError(
            "integer literal does not fit its payload constraint");
      auto value = number.isSigned() ? number.sextOrTrunc(size)
                                     : number.zextOrTrunc(size);
      std::string pattern(size, '0');
      for (unsigned i = 0; i < size; ++i)
        if (value[i])
          pattern[size - i - 1] = '1';
      attrs.set("pattern", builder.getStringAttr(pattern));
    } else if (kind == "convert") {
      if (transactionalTypesEqual(inputs[0].getType(), result)) {
        expr.getResult().replaceAllUsesWith(inputs[0]);
        expr.erase();
        return success();
      }
      if (!isa<BitsType>(inputs[0].getType()) || !isa<BitsType>(result))
        return expr.emitOpError(
            "nominal payload conversion requires exact types");
      if (width(result, expr) < width(inputs[0].getType(), expr))
        return expr.emitOpError("Bits narrowing requires an explicit slice");
      state.name = OperationName("ac.bits.resize", package.getContext());
      attrs.set("mode", builder.getStringAttr("zext"));
    } else {
      auto rule = operationRule(expr);
      bool compare = rule.comparison;
      unsigned start = rule.start;
      auto solved = operationTypes.find(expr);
      if (solved == operationTypes.end())
        return expr.emitOpError("operation payload constraints are not solved");
      Type type = solved->second.execution;
      if (isa<EnumType>(type) && compare) {
        auto carrier = hardware.getPackedWidth(type, {}, expr);
        if (failed(carrier))
          return failure();
        // Enum comparisons retain the declared nominal boundary before carrier
        // lowering.
        for (Value &input : inputs) {
          OperationState castState(expr.getLoc(), "ac.enum.to_bits");
          castState.addOperands(input);
          auto bits = constantWidth(builder, *carrier,
                                    cast<BitsType>(result).getWidth());
          castState.addTypes(bits);
          input = builder.create(castState)->getResult(0);
        }
        type = inputs.front().getType();
      }
      if (!isa<BitsType>(type))
        return expr.emitOpError("operation requires Bits payloads");
      if (rule.paired)
        for (unsigned i = start; i < inputs.size(); ++i)
          inputs[i] = resize(builder, inputs[i], type);
      StringRef name = kind == "binary"  ? "ac.bits.binary"
                       : compare         ? "ac.bits.compare"
                       : kind == "unary" ? "ac.bits.unary"
                                         : "ac.bits.select";
      state.name = OperationName(name, package.getContext());
      if (kind == "binary")
        output = type;
      if (expr.getOpcodeAttr())
        attrs.set(compare ? "predicate" : "opcode", expr.getOpcodeAttr());
      if (kind == "unary" && expr.getOpcodeAttr().getValue() == "neg") {
        OperationState zero(expr.getLoc(), "ac.bits.constant4");
        zero.addTypes(type);
        zero.addAttribute("pattern", builder.getStringAttr(
                                         std::string(width(type, expr), '0')));
        inputs.insert(inputs.begin(), builder.create(zero)->getResult(0));
        state.name = OperationName("ac.bits.binary", package.getContext());
        attrs.set("opcode", builder.getStringAttr("sub"));
      }
      if (compare && expr.getOpcodeAttr().getValue() == "case_eq")
        attrs.set("ac.verification_observer", builder.getStringAttr("case_eq"));
    }
    state.addOperands(inputs);
    state.addTypes(output);
    state.addAttributes(attrs);
    Value replacement = builder.create(state)->getResult(0);
    replacement = resize(builder, replacement, result);
    expr.getResult().replaceAllUsesWith(replacement);
    expr.erase();
    return success();
  }

public:
  explicit PayloadConstraints(mlir::ModuleOp package)
      : package(package), hardware(package) {}
  LogicalResult run(bool requireClosed) {
    package.walk([&](PendingExprOp expr) { expressions.push_back(expr); });
    package.walk([&](Operation *op) {
      if (!isa<TransactionRuleOp, TransactionMethodOp>(op))
        return;
      auto sig = cast<FunctionType>(
          op->getAttrOfType<TypeAttr>("function_type").getValue());
      unsigned offset = sig.getNumInputs() +
                        (isa<TransactionRuleOp>(op) ? sig.getNumResults() : 0);
      for (Region &region : op->getRegions())
        if (!region.empty())
          for (auto [capture, arg] :
               llvm::zip(op->getOperands(),
                         region.front().getArguments().drop_front(offset)))
            connect(capture, arg);
    });
    for (auto &[value, others] : aliases)
      if (fact(value))
        work.push_back(value);
    for (auto expr : expressions)
      propagate(expr);
    // Context facts are collected before choosing literal representations. The
    // dependency queue terminates when no new constraint is added.
    while (!work.empty()) {
      if (!budget--)
        return package.emitOpError("payload analysis budget exhausted");
      Value value = work.front();
      work.pop_front();
      for (Value other : aliases[value])
        if (failed(setFact(other, fact(value))))
          return failure();
      auto contexts = requests[value];
      if (auto expr = value.getDefiningOp<PendingExprOp>())
        propagate(expr);
      for (const auto &context : contexts) {
        for (Value other : aliases[value])
          request(other, context.type, context.source);
        if (auto expr = value.getDefiningOp<PendingExprOp>())
          propagate(expr, context.type);
      }
      Type current = anchor(value);
      Type prior = observedAnchors.lookup(value);
      if (current && (!prior || !transactionalTypesEqual(current, prior))) {
        observedAnchors[value] = current;
        for (Value other : aliases[value])
          work.push_back(other);
        for (Operation *user : value.getUsers())
          if (auto expr = dyn_cast<PendingExprOp>(user))
            work.push_back(expr.getResult());
      }
    }
    if (invalid)
      return failure();
    for (auto &[value, contexts] : requests) {
      if (fact(value) || contexts.empty())
        continue;
      auto expr = value.getDefiningOp<PendingExprOp>();
      if (expr && expr.getKind() != "literal")
        continue;
      if (!expr)
        continue;
      Type chosen;
      for (const auto &context : contexts)
        chosen = contextBound(chosen, context.type,
                              value.getParentBlock()->getParentOp());
      if (invalid)
        return failure();
      if (failed(setFact(value, chosen)))
        return failure();
    }
    for (auto expr : expressions)
      if (failed(infer(expr)))
        return failure();
    while (!work.empty()) {
      if (!budget--)
        return package.emitOpError("payload analysis budget exhausted");
      Value value = work.front();
      work.pop_front();
      for (Value other : aliases[value])
        if (failed(setFact(other, fact(value))))
          return failure();
      for (Operation *user : value.getUsers())
        if (auto expr = dyn_cast<PendingExprOp>(user))
          if (failed(infer(expr)))
            return failure();
    }
    for (auto &[value, type] : facts)
      value.setType(type);
    // Source order is SSA definition order. Rewrites only consume solved facts.
    for (auto expr : expressions)
      if (failed(rewrite(expr)))
        return failure();
    if (requireClosed) {
      bool unresolved = false;
      package.walk([&](Operation *op) {
        if (isa<PendingExprOp>(op))
          unresolved = true;
        for (Type type : op->getResultTypes())
          if (unknown(type))
            unresolved = true;
      });
      if (unresolved)
        return package.emitOpError(
            "payload constraints await a provider or actual binding");
    }
    return success();
  }
};
} // namespace
LogicalResult PendingExprOp::verify() {
  if (!isTransactionalPackage(*this) || isLinkedTransactionalPackage(*this))
    return emitOpError(
        "pending payload expressions require unlinked transactional ACIR");
  auto kind = getKind();
  unsigned count = kind == "literal"                       ? 0
                   : kind == "convert" || kind == "unary"  ? 1
                   : kind == "binary" || kind == "compare" ? 2
                   : kind == "select"                      ? 3
                                                           : ~0u;
  if (getNumOperands() != count ||
      (kind == "literal" && !isa_and_nonnull<MathIntAttr>(getNumberAttr())) ||
      ((kind == "binary" || kind == "compare" || kind == "unary") &&
       !getOpcodeAttr()))
    return emitOpError("malformed SSA payload constraint");
  return success();
}
LogicalResult solveTransactionalPayloadConstraints(mlir::ModuleOp package,
                                                   bool requireClosed) {
  return PayloadConstraints(package).run(requireClosed);
}
} // namespace acir::ac
