#include "TransactionalRepresentation.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
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
  SmallVector<scf::IfOp> branches;
  llvm::DenseMap<Value, SmallVector<Value, 2>> joins;
  llvm::DenseMap<Value, SmallVector<Value, 2>> joinConversions;
  llvm::DenseMap<Value, SmallVector<Value>> joinedUses;
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
  Type fieldType(Type type, PendingExprOp expr) {
    if (!type)
      return {};
    if (auto structure = dyn_cast<StructType>(type))
      if (auto declaration = hardware.lookupStruct(structure))
        for (Attribute raw : declaration.getFields()) {
          auto field = cast<DictionaryAttr>(raw);
          if (field.getAs<StringAttr>("name") == expr.getOpcodeAttr())
            return field.getAs<TypeAttr>("type").getValue();
        }
    expr.emitOpError("field access requires a declared Struct field");
    invalid = true;
    return {};
  }
  Type anchor(Value value) {
    if (auto type = fact(value))
      return type;
    if (!anchoring.insert(value).second)
      return {};
    Operation *relation = value.getDefiningOp<PendingExprOp>();
    if (joins.contains(value))
      relation = value.getDefiningOp();
    bool inserted = relation && excludedRelations.insert(relation).second;
    auto restoreExclusion = llvm::scope_exit([&] {
      if (inserted)
        excludedRelations.erase(relation);
    });
    Type result;
    if (auto expr = dyn_cast_or_null<PendingExprOp>(relation)) {
      auto kind = expr.getKind();
      if (kind == "binary" || kind == "select") {
        auto rule = operationRule(expr);
        unsigned start = rule.start;
        result = anchor(expr.getOperand(start));
        if (!rule.shift)
          result = common(result, anchor(expr.getOperand(start + 1)), expr);
      } else if (kind == "unary")
        result = anchor(expr.getOperand(0));
      else if (kind == "field")
        result = fieldType(anchor(expr.getOperand(0)), expr);
    }
    if (auto found = joins.find(value); found != joins.end())
      for (Value arm : found->second)
        result = common(result, anchor(arm), relation);
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
    if (auto found = joins.find(value); found != joins.end())
      for (Value arm : found->second)
        if (waiting(arm, active))
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
  void propagateJoin(Value result, Type context = {}) {
    auto found = joins.find(result);
    if (found == joins.end())
      return;
    Operation *branch = result.getDefiningOp();
    bool inserted = excludedRelations.insert(branch).second;
    auto restore = llvm::scope_exit([&] {
      if (inserted)
        excludedRelations.erase(branch);
    });
    llvm::DenseSet<Value> active;
    for (Value arm : found->second)
      if (waiting(arm, active))
        return;
    Type type;
    for (Value arm : found->second)
      type = common(type, anchor(arm), branch);
    if (!type)
      type = context;
    for (Value arm : found->second) {
      request(arm, type, branch);
      request(arm, context, branch);
    }
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
    if (kind == "field") {
      Type type = fieldType(fact(expr.getOperand(0)), expr);
      return invalid ? failure() : setFact(expr.getResult(), type);
    }
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
  LogicalResult inferJoin(Value result) {
    auto found = joins.find(result);
    if (found == joins.end())
      return success();
    Type type;
    for (Value arm : found->second) {
      if (!fact(arm))
        return success();
      type = common(type, fact(arm), result.getDefiningOp());
    }
    if (invalid)
      return failure();
    if (failed(setFact(result, type)))
      return failure();
    for (Value conversion : joinConversions[result])
      if (failed(setFact(conversion, type)))
        return failure();
    return success();
  }

  Value resize(OpBuilder &builder, Value value, Type type, bool exact = false) {
    if (exact ? value.getType() == type
              : transactionalTypesEqual(value.getType(), type))
      return value;
    OperationState state(value.getLoc(), "ac.bits.resize");
    state.addOperands(value);
    state.addTypes(type);
    if (exact)
      state.addAttribute("ac.join_transport", builder.getUnitAttr());
    unsigned inputWidth =
        width(value.getType(), value.getParentBlock()->getParentOp());
    unsigned resultWidth = width(type, value.getParentBlock()->getParentOp());
    if (inputWidth == resultWidth) {
      // SCF requires exact types, including source provenance. A full-width
      // extraction retains the payload while carrying the chosen result type.
      state.name = OperationName("ac.bits.extract", package.getContext());
      state.addAttribute(
          "low", detail::transactionalInteger(builder, 0, cast<BitsType>(type).getWidth()));
      return builder.create(state)->getResult(0);
    }
    state.addAttribute(
        "mode",
        builder.getStringAttr(resultWidth < inputWidth ? "trunc" : "zext"));
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
    if (auto transport = expr->getAttr("ac.join_transport"))
      attrs.set("ac.join_transport", transport);
    Type output = result;
    if (kind == "field") {
      state.name = OperationName("ac.struct.get", package.getContext());
      attrs.set("field", expr.getOpcodeAttr());
    } else if (kind == "literal") {
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
      output = solved->second.result;
      if (isa<EnumType>(type) && compare) {
        auto carrier = hardware.getPackedWidth(type, {}, expr);
        if (failed(carrier))
          return failure();
        // Enum comparisons retain the declared nominal boundary before carrier
        // lowering.
        for (Value &input : inputs) {
          OperationState castState(expr.getLoc(), "ac.enum.to_bits");
          castState.addOperands(input);
          auto bits = BitsType::get(
              builder.getContext(),
              detail::transactionalInteger(builder, *carrier,
                             cast<BitsType>(result).getWidth()));
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
    auto collected = package.walk([&](scf::IfOp branch) {
      if (!branch.getNumResults())
        return WalkResult::advance();
      for (Region &region : branch->getRegions()) {
        if (!region.hasOneBlock() || region.front().empty()) {
          branch.emitOpError("payload branch requires both yield arms");
          return WalkResult::interrupt();
        }
        auto yield = dyn_cast<scf::YieldOp>(region.front().back());
        if (!yield || yield.getNumOperands() != branch.getNumResults()) {
          branch.emitOpError("payload branch yield arity differs");
          return WalkResult::interrupt();
        }
        for (auto [result, arm] :
             llvm::zip(branch.getResults(), yield.getOperands())) {
          // Unlinked generic calls may leave one arm unresolved. Its temporary
          // conversion keeps SCF's exact yield types without choosing the join
          // type before the provider is bound.
          Value source = arm;
          if (auto conversion = arm.getDefiningOp<PendingExprOp>();
              conversion && conversion.getKind() == "convert" &&
              unknown(arm.getType())) {
            source = conversion.getOperand(0);
            joinConversions[result].push_back(arm);
          }
          joins[result].push_back(source);
          joinedUses[source].push_back(result);
        }
      }
      branches.push_back(branch);
      return WalkResult::advance();
    });
    if (collected.wasInterrupted())
      return failure();
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
    for (const auto &[result, arms] : joins)
      propagateJoin(result);
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
      propagateJoin(value);
      for (const auto &context : contexts) {
        for (Value other : aliases[value])
          request(other, context.type, context.source);
        if (auto expr = value.getDefiningOp<PendingExprOp>())
          propagate(expr, context.type);
        propagateJoin(value, context.type);
      }
      Type current = anchor(value);
      Type prior = observedAnchors.lookup(value);
      if (current && (!prior || !transactionalTypesEqual(current, prior))) {
        observedAnchors[value] = current;
        for (Value other : aliases[value])
          work.push_back(other);
        for (Value result : joinedUses[value])
          work.push_back(result);
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
    for (const auto &[result, arms] : joins)
      if (failed(inferJoin(result)))
        return failure();
    while (!work.empty()) {
      if (!budget--)
        return package.emitOpError("payload analysis budget exhausted");
      Value value = work.front();
      work.pop_front();
      for (Value other : aliases[value])
        if (failed(setFact(other, fact(value))))
          return failure();
      for (Value result : joinedUses[value])
        if (failed(inferJoin(result)))
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
    for (auto branch : branches)
      for (Region &region : branch->getRegions()) {
        auto yield = cast<scf::YieldOp>(region.front().getTerminator());
        OpBuilder builder(yield);
        for (auto [index, result] : llvm::enumerate(branch.getResults())) {
          Value arm = yield.getOperand(index);
          if (arm.getType() == result.getType())
            continue;
          if (unknown(result.getType()) || unknown(arm.getType())) {
            OperationState state(yield.getLoc(), "ac.payload.pending");
            state.addOperands(arm);
            state.addTypes(result.getType());
            state.addAttribute("kind", builder.getStringAttr("convert"));
            state.addAttribute("ac.join_transport", builder.getUnitAttr());
            yield.setOperand(index, builder.create(state)->getResult(0));
            continue;
          }
          if (!isa<BitsType>(arm.getType()) ||
              !isa<BitsType>(result.getType()))
            return branch.emitOpError(
                "nominal branch payloads require exact types");
          yield.setOperand(index, resize(builder, arm, result.getType(), true));
        }
      }
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
namespace {
// Constant proofs follow the same finite-width payload relations as solving.
// A missing width/value is unknown, rather than a source-spelling heuristic.
class ConstantPayload {
  HardwareAnalysis hardware;
  const std::function<bool()> &debit;
  unsigned depth = 0;

  unsigned width(Value value) {
    if (auto type = dyn_cast<BitsType>(value.getType())) {
      if (!hardware.isStaticEvaluable(type.getWidth(), {}))
        return 0;
      auto width = hardware.getPackedWidth(type, {}, value.getDefiningOp());
      return succeeded(width) && *width <= 1048576 ? unsigned(*width) : 0;
    }
    auto expr = value.getDefiningOp<PendingExprOp>();
    if (!expr || !isa<InferType>(value.getType()) || !debit() || depth > 256)
      return 0;
    ++depth;
    auto restore = llvm::scope_exit([&] { --depth; });
    if (expr.getKind() == "unary")
      return width(expr.getOperand(0));
    if (expr.getKind() == "binary") {
      unsigned a = width(expr.getOperand(0));
      unsigned b = width(expr.getOperand(1));
      return expr.getOpcodeAttr().getValue() == "shl" ||
                     expr.getOpcodeAttr().getValue() == "lshr"
                 ? a
                 : std::max(a, b);
    }
    return 0;
  }

public:
  ConstantPayload(mlir::ModuleOp package, const std::function<bool()> &debit)
      : hardware(package), debit(debit) {}
  std::optional<llvm::APInt> evaluate(Value value, unsigned context = 0) {
    if (!debit() || depth > 256)
      return std::nullopt;
    ++depth;
    auto restore = llvm::scope_exit([&] { --depth; });
    unsigned bits = width(value);
    if (!bits)
      bits = context;
    if (!bits)
      return std::nullopt;
    if (auto constant = value.getDefiningOp<BitsConstant4Op>()) {
      auto pattern = constant.getPattern();
      if (pattern.size() != bits ||
          pattern.find_first_not_of("01") != StringRef::npos)
        return std::nullopt;
      return llvm::APInt(bits, pattern, 2);
    }
    auto expr = value.getDefiningOp<PendingExprOp>();
    if (!expr)
      return std::nullopt;
    auto kind = expr.getKind();
    if (kind == "literal") {
      auto number = expr->getAttrOfType<MathIntAttr>("number");
      if (!number)
        return std::nullopt;
      llvm::APSInt n(number.getCanonicalValue());
      if (n.isNegative() || n.getActiveBits() > bits)
        return std::nullopt;
      return n.zextOrTrunc(bits);
    }
    if (kind == "convert") {
      unsigned inputWidth = width(expr.getOperand(0));
      if (inputWidth > bits)
        return std::nullopt; // A narrowing conversion must still reject.
      auto input = evaluate(expr.getOperand(0), bits);
      return input ? std::optional<llvm::APInt>(input->zextOrTrunc(bits))
                   : std::nullopt;
    }
    auto opcode = expr.getOpcodeAttr();
    if (!opcode)
      return std::nullopt;
    if (kind == "unary") {
      auto input = evaluate(expr.getOperand(0), bits);
      if (!input)
        return std::nullopt;
      if (opcode.getValue() == "not")
        return ~*input;
      if (opcode.getValue() == "neg")
        return -*input;
      return std::nullopt;
    }
    if (kind != "binary" && kind != "compare")
      return std::nullopt;
    unsigned lhsWidth = width(expr.getOperand(0));
    unsigned rhsWidth = width(expr.getOperand(1));
    // Keep mixed-width shifts for payload solving, which owns the exact
    // operand conversions. Reachability must not guess a shift-amount width.
    if ((opcode.getValue() == "shl" || opcode.getValue() == "lshr") &&
        lhsWidth && rhsWidth && lhsWidth != rhsWidth)
      return std::nullopt;
    unsigned operandBits = std::max(lhsWidth, rhsWidth);
    if (!operandBits && kind == "binary")
      operandBits = bits;
    if (!operandBits)
      return std::nullopt;
    auto lhs = evaluate(expr.getOperand(0), operandBits);
    auto rhs = evaluate(expr.getOperand(1), operandBits);
    if (!lhs || !rhs)
      return std::nullopt;
    *lhs = lhs->zextOrTrunc(operandBits);
    *rhs = rhs->zextOrTrunc(operandBits);
    auto op = opcode.getValue();
    if (kind == "compare") {
      bool result;
      if (op == "eq")
        result = *lhs == *rhs;
      else if (op == "ne")
        result = *lhs != *rhs;
      else if (op == "ult")
        result = lhs->ult(*rhs);
      else if (op == "ule")
        result = lhs->ule(*rhs);
      else if (op == "ugt")
        result = lhs->ugt(*rhs);
      else if (op == "uge")
        result = lhs->uge(*rhs);
      else
        return std::nullopt;
      return llvm::APInt(1, result);
    }
    std::optional<llvm::APInt> result;
    if (op == "add") result = *lhs + *rhs;
    else if (op == "sub") result = *lhs - *rhs;
    else if (op == "mul") result = *lhs * *rhs;
    else if (op == "and") result = *lhs & *rhs;
    else if (op == "or") result = *lhs | *rhs;
    else if (op == "xor") result = *lhs ^ *rhs;
    else if (op == "shl") result = lhs->shl(rhs->getLimitedValue(operandBits));
    else if (op == "lshr") result = lhs->lshr(rhs->getLimitedValue(operandBits));
    // Shift execution may widen to the shift operand, but its natural result
    // retains the left operand's width before any surrounding conversion.
    if (result)
      return result->zextOrTrunc(bits);
    return std::nullopt;
  }
};
} // namespace
std::optional<bool>
evaluateTransactionalPredicate(Value value,
                               const std::function<bool()> &debit) {
  auto *operation = value.getDefiningOp();
  if (!operation)
    return std::nullopt;
  auto package = operation->getParentOfType<mlir::ModuleOp>();
  auto result = ConstantPayload(package, debit).evaluate(value);
  if (!result || result->getBitWidth() != 1)
    return std::nullopt;
  return !result->isZero();
}
LogicalResult PendingExprOp::verify() {
  if (!isTransactionalPackage(*this) || isLinkedTransactionalPackage(*this))
    return emitOpError(
        "pending payload expressions require unlinked transactional ACIR");
  auto kind = getKind();
  unsigned count = kind == "literal"                                         ? 0
                   : kind == "convert" || kind == "unary" || kind == "field" ? 1
                   : kind == "binary" || kind == "compare"                   ? 2
                   : kind == "select" ? 3
                                      : ~0u;
  if (getNumOperands() != count ||
      (kind == "literal" && !isa_and_nonnull<MathIntAttr>(getNumberAttr())) ||
      ((kind == "binary" || kind == "compare" || kind == "unary" ||
        kind == "field") &&
       !getOpcodeAttr()))
    return emitOpError("malformed SSA payload constraint");
  return success();
}
LogicalResult solveTransactionalPayloadConstraints(mlir::ModuleOp package,
                                                   bool requireClosed) {
  return PayloadConstraints(package).run(requireClosed);
}
} // namespace acir::ac
