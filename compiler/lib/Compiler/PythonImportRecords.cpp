#include "PythonImportRecords.h"
#include "SourceRuleWrites.h"
#include "SourceDeclarationContext.h"

#include "FixedUnsignedDivRemLowering.h"
#include "NumericLowering.h"
#include "PythonImportCaseSyntax.h"
#include "PythonImportContext.h"
#include "PythonImportEnumSyntax.h"
#include "TableQueryLowering.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/Support/MathExtras.h"

#include <limits>
#include <memory>

using namespace mlir;

namespace acir::compiler::detail {
namespace {

struct Parameter {
  std::string name;
  AstNode node;
  std::optional<AstNode> value;
};
struct Port {
  std::string name;
  Type type;
  AstNode node;
  std::optional<ac::detail::ValueKind> sourceKind;
  bool fixedBits = false;
};
enum class ReturnShape { Mapping, Struct };
struct ModuleDecl {
  AstNode node;
  FlatSymbolRefAttr symbol;
  SmallVector<Parameter> parameters;
  SmallVector<Port> inputs, outputs;
  FunctionType type;
  bool persistent = false;
  bool queueDomain = false;
  SmallVector<NamedAttribute> sourceCallAttrs;
  bool behavioral = false;
  bool needsDomain = false;
  bool system = false;
  size_t sourceInputCount = 0;
  ReturnShape returnShape = ReturnShape::Mapping;
};
struct CallableDecl {
  FlatSymbolRefAttr symbol;
  SmallVector<Port> inputs;
  Port result;
  bool needsDomain = false;
};
struct StateOwner {
  std::string name;
  Type type;
  Value init;
  Operation *op;
  AstNode node;
};
enum class BindingCategory { Module, Struct, Enum, Marker, Namespace, Other };
struct Import {
  FlatSymbolRefAttr symbol;
  Operation *declaration = nullptr;
  AstNode site;
  DictionaryAttr provider;
  std::string remote;
  bool builtin = false;
  BindingCategory category = BindingCategory::Module;
  std::string importedModule;
};
struct Instance {
  std::string name;
  AstNode node;
  FlatSymbolRefAttr callee;
  Operation *declaration = nullptr;
  SmallVector<Attribute> parameters;
  SmallVector<Attribute> typeArguments;
  SmallVector<Port> inputs, outputs;
  Operation *op = nullptr;
  bool bound = false;
  std::optional<unsigned> clockInput, resetInput;
};
struct SliceShape {
  Type type;
  ac::StaticExprAttr low;
};
struct FixedCountShape {
  uint64_t width;
  Type resultType;
};
struct DirectCallSite {
  Operation *op;
  bool bound = false;
};
struct ForwardCallBinding {
  DictionaryAttr site;
  bool eligible = false;
  unsigned ordinal = 0;
};
enum class IntegerBoundaryMode { Inferred, Representation, Range };
struct BindingBoundary {
  Port contract;
  bool declared = false;
  IntegerBoundaryMode integerMode = IntegerBoundaryMode::Range;
};
// Slots are planned before lowering and never resized during assignments.
struct AddressWitness {
  size_t ownerIndex, pathIndex;
  std::string formal;
  AstNode assignment, occurrence;
  Type maskType;
  Value index, mask;
};
struct OwnerProposal {
  size_t ownerIndex;
  Value next, enable;
  const SourceOwnerWrite *write;
  const SourceRuleCallPlan *callPlan;
  SmallVector<AddressWitness, 0> domains;
};
struct BranchEnvironment {
  llvm::StringMap<Value> values;
  llvm::StringMap<Value> writes;
  llvm::StringMap<BindingBoundary> boundaries;
  Value ambient, continuation;
};
// One entry of the rule result suffix that a source instrumentation site
// contributes. A source assertion contributes exactly one entry; a source
// observation contributes one entry per hardware value operand, or a single
// value-less placeholder entry when a `log` carries only literal items. The
// publisher rebuilds one `ac.observe` per consecutive run of entries that share
// the same source site.
struct SourceCheckCapture {
  AstNode statement;
  Value condition, path;
  StringAttr message;
  // Non-null only for a source observation. `observationKind` is `log` or
  // `report`, `observationSpec` is the same closed static spec the pure-module
  // instrumentation pass emits, and `observationRule` is the rule definition
  // that carries the site so the two paths derive the same observation
  // identity.
  StringAttr observationKind;
  DictionaryAttr observationSpec;
  AstNode observationRule;
  unsigned observationValues = 0;
  bool observationCarriesValue = false;
};
struct RuleEvaluation {
  SmallVector<AddressWitness, 0> domains;
  SmallVector<SourceCheckCapture> checks;
  // Source observations a structural rule body deferred to the module-level
  // instrumentation pass. That pass must publish every recorded site, because
  // docs/reference/language.md:842 forbids silently dropping a source
  // observation. The entry is an (rule definition, statement) pair.
  SmallVector<std::pair<AstNode, AstNode>, 0> deferredObservations;
  // Number of leading `checks` entries that belong to source observations.
  // Source assertion obligations must occupy the trailing results of the rule
  // suffix, because `HardwareSourceChecks.cpp` locates them from the end of the
  // result list, so observation entries are kept in front of them.
  unsigned observationEntries = 0;
  llvm::StringMap<Instance *> *instances = nullptr;
  llvm::StringMap<SmallVector<Value>> *outputs = nullptr;
  Instance *target = nullptr;
  AstNode binding;
  SmallVector<Value> bindingValues;
  SmallVector<Instance *> structuralTargets;
  unsigned nextStructuralTarget = 0;
  bool structural = false;
};
struct ClosedSourceConstant {
  Attribute value;
};
struct SourceChoice {
  Value value;
  std::optional<ClosedSourceConstant> constant;
};
struct EnumMemberRef {
  ac::EnumType type;
  StringAttr member;
};

// A canonical key view borrowed from existing literal/member authorities.
struct MatchPatternKey {
  AstNode site;
  Attribute value;
  ac::EnumType enumeration;
};

bool sameSourceSite(const AstNode &a, const AstNode &c) {
  return a.value == c.value && a.path.size() == c.path.size() &&
         llvm::all_of(llvm::zip(a.path, c.path), [](const auto &steps) {
           return std::get<0>(steps).field == std::get<1>(steps).field &&
                  std::get<0>(steps).index == std::get<1>(steps).index;
         });
}

Operation *createOp(OpBuilder &b, Location loc, StringRef name,
                    ValueRange operands, TypeRange results,
                    ArrayRef<NamedAttribute> attrs, unsigned regions = 0) {
  OperationState state(loc, name);
  state.addOperands(operands);
  state.addTypes(results);
  state.addAttributes(attrs);
  while (regions--)
    state.addRegion();
  return b.create(state);
}

bool docstring(const AstNode &node) {
  AstNode value = node.kind() == "Expr" ? node.child("value") : AstNode();
  return value.kind() == "Constant" && isa<StringAttr>(value.get("value"));
}

std::string moduleName(DictionaryAttr owner) {
  std::string result = owner.getAs<StringAttr>("package").getValue().str();
  StringRef path = owner.getAs<StringAttr>("path").getValue();
  if (path.ends_with(".py"))
    path = path.drop_back(3);
  SmallVector<StringRef> parts;
  path.split(parts, '/');
  if (!parts.empty() && parts.back() == "__init__")
    parts.pop_back();
  for (StringRef part : parts) {
    if (!result.empty())
      result.push_back('.');
    result.append(part);
  }
  return result;
}

OwningOpRef<ModuleOp> unit(MLIRContext *ctx, Location loc, DictionaryAttr owner,
                           StringRef kind, ArrayAttr interfaces) {
  OwningOpRef<ModuleOp> result(ModuleOp::create(loc));
  result->getOperation()->setAttr("ac.source_owner", owner);
  result->getOperation()->setAttr("ac.unit_kind", StringAttr::get(ctx, kind));
  result->getOperation()->setAttr("ac.stage", StringAttr::get(ctx, "source"));
  result->getOperation()->setAttr("ac.interfaces", interfaces);
  return result;
}

// Admission-only analysis: it never rewrites the four-state enable graph.
class OwnerEnableProof {
public:
  explicit OwnerEnableProof(ModuleOp body, uint64_t spent)
      : analysis(body), work(spent) {}
  FailureOr<bool> disjoint(Value first, Value second) {
    if (!charge())
      return failure();
    auto left = required(first, true, 0);
    auto right = required(second, true, 0);
    auto together = conjunction(left, right);
    if (together.kind == Result::Failure)
      return failure();
    return together.kind == Result::Impossible;
  }
  FailureOr<bool> disjointAtAddress(Value first, Value second,
                                    Value firstIndex, Value secondIndex) {
    auto left = canonical(firstIndex, 0), right = canonical(secondIndex, 0);
    if (failed(left) || failed(right))
      return failure();
    auto leftConstant = constant(*left, 0), rightConstant = constant(*right, 0);
    auto leftWidth = width(*left), rightWidth = width(*right);
    if (failed(leftConstant) || failed(rightConstant) || failed(leftWidth) ||
        failed(rightWidth))
      return failure();
    if (!*leftWidth || !*rightWidth) {
      fail("address proof requires typed Bits indices");
      return failure();
    }
    if (*leftConstant && *rightConstant)
      return !equalConstants(**leftConstant, **rightConstant);
    // No widening equivalence is inferred for two dynamic physical indices.
    if (!*leftConstant && !*rightConstant && **leftWidth != **rightWidth)
      return false;
    collision = {*left, *right};
    auto result = disjoint(first, second);
    collision = {};
    return result;
  }
  bool chargeProduct(uint64_t first, uint64_t second) {
    if (first && second > 1048576 / first)
      return charge(1048577);
    return charge(first * second);
  }
  StringRef failureReason() const { return reason; }
  bool charge(uint64_t amount = 1) {
    if (!reason.empty())
      return false;
    if (work > 1048576 || amount > 1048576 - work) {
      reason = "owner-enable proof work budget exhausted";
      return false;
    }
    work += amount;
    return true;
  }

private:
  struct Fact {
    Value value;
    bool polarity;
    // Null constant is a canonical SSA truth fact. Otherwise this is an
    // equality/disequality with an exact, fully known typed bit pattern.
    Attribute constant;
    uint64_t width = 0;
  };
  struct Result {
    enum Kind { Facts, Impossible, Failure } kind = Facts;
    SmallVector<Fact> facts;
  };
  Result fail(StringRef message) {
    if (reason.empty())
      reason = message.str();
    return {Result::Failure, {}};
  }
  bool scanConstantAttribute(Attribute attr, unsigned depth = 0) {
    if (depth >= 128) {
      reason = "owner-enable proof recursion budget exhausted";
      return false;
    }
    if (!charge())
      return false;
    if (auto integer = dyn_cast<ac::MathIntAttr>(attr))
      return charge(integer.getCanonicalValue().size());
    if (auto expression = dyn_cast<ac::StaticExprAttr>(attr))
      return scanConstantAttribute(expression.getTree(), depth + 1);
    if (auto dictionary = dyn_cast<DictionaryAttr>(attr)) {
      for (auto field : dictionary)
        if (!scanConstantAttribute(field.getValue(), depth + 1))
          return false;
    } else if (auto array = dyn_cast<ArrayAttr>(attr)) {
      for (auto element : array)
        if (!scanConstantAttribute(element, depth + 1))
          return false;
    }
    return true;
  }
  FailureOr<std::optional<llvm::APSInt>> integer(ac::StaticExprAttr expr,
                                                 Operation *site) {
    if (!scanConstantAttribute(expr))
      return failure();
    if (!analysis.isStaticEvaluable(expr, {}))
      return std::optional<llvm::APSInt>();
    auto result = analysis.evaluateStatic(expr, {}, site);
    if (failed(result))
      return failure();
    auto value = dyn_cast<ac::MathIntAttr>(*result);
    if (!value)
      return std::optional<llvm::APSInt>();
    return std::optional<llvm::APSInt>(llvm::APSInt(value.getCanonicalValue()));
  }
  FailureOr<std::optional<uint64_t>> width(Value value) {
    auto type = dyn_cast<ac::BitsType>(value.getType());
    if (!type)
      return std::optional<uint64_t>();
    auto result = integer(type.getWidth(), value.getDefiningOp());
    if (failed(result))
      return failure();
    if (!*result || (**result).isNegative() || (**result).isZero() ||
        (**result).getActiveBits() > 64)
      return std::optional<uint64_t>();
    return std::optional<uint64_t>((**result).getZExtValue());
  }
  FailureOr<Value> canonical(Value value, unsigned depth) {
    if (!charge())
      return failure();
    if (depth >= 128) {
      fail("owner-enable proof recursion budget exhausted");
      return failure();
    }
    auto cached = normalized.find(value);
    if (cached != normalized.end())
      return cached->second;
    if (!normalizing.insert(value).second) {
      fail("cycle in owner-enable proof normalization");
      return failure();
    }
    Value next;
    if (auto result = dyn_cast<OpResult>(value)) {
      if (auto rule = dyn_cast<ac::RuleOp>(result.getOwner())) {
        if (!rule->getRegion(0).hasOneBlock() ||
            rule->getRegion(0).front().empty()) {
          fail("missing owner-enable rule yield endpoint");
          return failure();
        }
        auto yield = dyn_cast<ac::YieldOp>(rule->getRegion(0).front().back());
        if (!yield || result.getResultNumber() >= yield->getNumOperands()) {
          fail("missing owner-enable rule yield endpoint");
          return failure();
        }
        next = yield->getOperand(result.getResultNumber());
      }
    } else if (auto argument = dyn_cast<BlockArgument>(value)) {
      auto rule =
          dyn_cast_or_null<ac::RuleOp>(argument.getOwner()->getParentOp());
      if (rule) {
        if (argument.getArgNumber() >= rule->getNumOperands()) {
          fail("missing owner-enable actual capture endpoint");
          return failure();
        }
        next = rule->getOperand(argument.getArgNumber());
      }
    }
    if (auto extract = value.getDefiningOp<ac::BitsExtractOp>()) {
      auto low = integer(extract.getLow(), extract);
      auto from = width(extract.getInput()), to = width(value);
      if (failed(low) || failed(from) || failed(to))
        return failure();
      if (*low && (**low).isZero() && *from && *to && **from == **to)
        next = extract.getInput();
    } else if (auto resize = value.getDefiningOp<ac::BitsResizeOp>()) {
      auto from = width(resize.getInput()), to = width(value);
      if (failed(from) || failed(to))
        return failure();
      if (*from && *to && **from == **to)
        next = resize.getInput();
    }
    if (next) {
      auto resolved = canonical(next, depth + 1);
      if (failed(resolved))
        return failure();
      next = *resolved;
    } else
      next = value;
    normalizing.erase(value);
    normalized.try_emplace(value, next);
    return next;
  }
  struct Constant {
    Attribute value;
    uint64_t width;
  };
  FailureOr<std::optional<Constant>> constant(Value value, unsigned depth) {
    auto resolved = canonical(value, depth);
    if (failed(resolved))
      return failure();
    auto op = resolved->getDefiningOp<ac::BitsConstantOp>();
    if (!op)
      return std::optional<Constant>();
    auto count = width(*resolved);
    auto code = integer(op.getValue(), op);
    if (failed(count) || failed(code))
      return failure();
    if (!*count || !*code || (**code).isNegative() ||
        (**code).getActiveBits() > **count)
      return std::optional<Constant>();
    // Charge the entire padded pattern before allocating it.
    if (!charge(**count / 64 + (**count % 64 != 0)) ||
        **count > std::numeric_limits<unsigned>::max())
      return failure();
    llvm::APSInt pattern((**code).zextOrTrunc(**count), true);
    return std::optional<Constant>(
        Constant{ac::MathIntAttr::get(value.getContext(), pattern), **count});
  }
  bool equalConstants(const Constant &a, const Constant &c) {
    // Fully known patterns compare as mathematical unsigned values, allowing
    // lossless padding while never truncating an unknown or a high bit.
    return llvm::APSInt::compareValues(
               llvm::APSInt(cast<ac::MathIntAttr>(a.value).getCanonicalValue()),
               llvm::APSInt(cast<ac::MathIntAttr>(c.value).getCanonicalValue())) == 0;
  }
  FailureOr<bool> sameAddress(Value value, Value endpoint, unsigned depth) {
    auto resolved = canonical(value, depth);
    if (failed(resolved))
      return failure();
    if (*resolved == endpoint)
      return true;
    auto a = constant(*resolved, depth), c = constant(endpoint, depth);
    if (failed(a) || failed(c))
      return failure();
    return *a && *c && equalConstants(**a, **c);
  }
  FailureOr<bool> collisionComparison(ac::BitsCompareOp compare,
                                      unsigned depth) {
    if (!collision.first)
      return false;
    auto a = sameAddress(compare.getLhs(), collision.first, depth);
    auto c = sameAddress(compare.getRhs(), collision.second, depth);
    if (failed(a) || failed(c))
      return failure();
    if (*a && *c)
      return true;
    a = sameAddress(compare.getLhs(), collision.second, depth);
    c = sameAddress(compare.getRhs(), collision.first, depth);
    if (failed(a) || failed(c))
      return failure();
    return *a && *c;
  }
  bool same(const Fact &a, const Fact &c) {
    return a.value == c.value && a.polarity == c.polarity &&
           a.constant == c.constant && a.width == c.width;
  }
  bool contradictory(const Fact &a, const Fact &c) {
    if (a.value != c.value)
      return false;
    if (!a.constant || !c.constant)
      return !a.constant && !c.constant && a.polarity != c.polarity;
    if (a.width != c.width)
      return false;
    if (a.constant == c.constant)
      return a.polarity != c.polarity;
    return a.polarity && c.polarity;
  }
  Result append(Result result, const Fact &fact) {
    if (result.kind != Result::Facts)
      return result;
    for (const auto &prior : result.facts) {
      if (!charge())
        return {Result::Failure, {}};
      if (same(prior, fact))
        return result;
      if (contradictory(prior, fact))
        return {Result::Impossible, {}};
    }
    if (!charge())
      return {Result::Failure, {}};
    if (result.facts.size() == 256)
      return fail("owner-enable proof fact budget exhausted");
    result.facts.push_back(fact);
    return result;
  }
  Result conjunction(const Result &a, const Result &c) {
    if (!charge())
      return {Result::Failure, {}};
    if (a.kind == Result::Failure || c.kind == Result::Failure)
      return {Result::Failure, {}};
    if (a.kind == Result::Impossible || c.kind == Result::Impossible)
      return {Result::Impossible, {}};
    if (!charge(a.facts.size()))
      return {Result::Failure, {}};
    Result result = a;
    for (const auto &fact : c.facts) {
      result = append(std::move(result), fact);
      if (result.kind != Result::Facts)
        return result;
    }
    return result;
  }
  Result alternatives(const Result &a, const Result &c) {
    if (!charge())
      return {Result::Failure, {}};
    if (a.kind == Result::Failure || c.kind == Result::Failure)
      return {Result::Failure, {}};
    if (a.kind == Result::Impossible) {
      if (!charge(c.facts.size()))
        return {Result::Failure, {}};
      return c;
    }
    if (c.kind == Result::Impossible) {
      if (!charge(a.facts.size()))
        return {Result::Failure, {}};
      return a;
    }
    Result result;
    for (const auto &left : a.facts)
      for (const auto &right : c.facts) {
        if (!charge())
          return {Result::Failure, {}};
        if (same(left, right)) {
          if (!charge())
            return {Result::Failure, {}};
          result.facts.push_back(left);
          break;
        }
      }
    return result;
  }
  Result required(Value original, bool polarity, unsigned depth) {
    if (!charge())
      return {Result::Failure, {}};
    if (depth >= 128)
      return fail("owner-enable proof recursion budget exhausted");
    auto resolved = canonical(original, depth);
    if (failed(resolved))
      return fail("owner-enable proof normalization failed");
    Value value = *resolved;
    auto key = std::make_pair(collision, std::make_pair(value, unsigned(polarity)));
    auto found = cache.find(key);
    if (found != cache.end()) {
      if (!charge(found->second.facts.size()))
        return {Result::Failure, {}};
      return found->second;
    }
    if (!visiting.insert(key).second)
      return fail("cycle in owner-enable proof queries");
    if (cache.size() + visiting.size() > 4096)
      return fail("owner-enable proof query budget exhausted");
    auto count = width(value);
    if (failed(count))
      return fail("owner-enable proof width analysis failed");
    if (!*count || **count != 1)
      return fail("owner-enable proof requires one-bit enable");
    Result result;
    auto code = constant(value, depth);
    if (failed(code))
      return fail("owner-enable proof constant analysis failed");
    if (*code) {
      bool truth =
          !llvm::APSInt(
               cast<ac::MathIntAttr>((**code).value).getCanonicalValue())
               .isZero();
      if (truth != polarity)
        result.kind = Result::Impossible;
    } else if (auto unary = value.getDefiningOp<ac::BitsUnaryOp>();
               unary && unary.getOpcode() == "not") {
      result = required(unary.getInput(), !polarity, depth + 1);
    } else if (auto binary = value.getDefiningOp<ac::BitsBinaryOp>();
               binary &&
               (binary.getOpcode() == "and" || binary.getOpcode() == "or")) {
      auto left = required(binary.getLhs(), polarity, depth + 1);
      auto right = required(binary.getRhs(), polarity, depth + 1);
      bool both = (binary.getOpcode() == "and") == polarity;
      result = both ? conjunction(left, right) : alternatives(left, right);
    } else if (auto select = value.getDefiningOp<ac::BitsSelectOp>()) {
      auto yes = canonical(select.getTrueValue(), depth + 1);
      auto no = canonical(select.getFalseValue(), depth + 1);
      if (failed(yes) || failed(no))
        return fail("owner-enable proof select analysis failed");
      if (*yes == *no)
        result = required(*yes, polarity, depth + 1);
      else {
        auto yesConstant = constant(*yes, depth + 1);
        auto noConstant = constant(*no, depth + 1);
        if (failed(yesConstant) || failed(noConstant))
          return fail("owner-enable proof select constant analysis failed");
        auto truth = [](const Constant &c) {
          return !llvm::APSInt(
                      cast<ac::MathIntAttr>(c.value).getCanonicalValue())
                      .isZero();
        };
        if (*yesConstant && *noConstant &&
            truth(**yesConstant) == truth(**noConstant)) {
          if (truth(**yesConstant) != polarity)
            result.kind = Result::Impossible;
        } else if (*yesConstant && *noConstant) {
          result = required(select.getCondition(),
                            truth(**yesConstant) == polarity, depth + 1);
        } else if (*noConstant) {
          bool noTruth = truth(**noConstant);
          auto guard =
              required(select.getCondition(), polarity != noTruth, depth + 1);
          auto branch = required(*yes, polarity, depth + 1);
          result = polarity != noTruth ? conjunction(guard, branch)
                                       : alternatives(guard, branch);
        } else if (*yesConstant) {
          bool yesTruth = truth(**yesConstant);
          auto guard =
              required(select.getCondition(), polarity == yesTruth, depth + 1);
          auto branch = required(*no, polarity, depth + 1);
          result = polarity != yesTruth ? conjunction(guard, branch)
                                        : alternatives(guard, branch);
        } else {
          auto guardTrue = required(select.getCondition(), true, depth + 1);
          auto guardFalse = required(select.getCondition(), false, depth + 1);
          auto yesFacts = required(*yes, polarity, depth + 1);
          auto noFacts = required(*no, polarity, depth + 1);
          auto selectedTrue = conjunction(guardTrue, yesFacts);
          auto selectedFalse = conjunction(guardFalse, noFacts);
          // An X/Z selector also produces a known result when both arms are
          // known and equal. Keep this alternative, including its failures.
          auto common = conjunction(yesFacts, noFacts);
          result = alternatives(alternatives(selectedTrue, selectedFalse), common);
        }
      }
    } else if (auto compare = value.getDefiningOp<ac::BitsCompareOp>();
               compare && (compare.getPredicate() == "eq" ||
                           compare.getPredicate() == "ne")) {
      auto inCollision = collisionComparison(compare, depth + 1);
      if (failed(inCollision))
        return fail("address proof equality endpoint analysis failed");
      bool equal = (compare.getPredicate() == "eq") == polarity;
      if (*inCollision && !equal) {
        // Possible-address collision forbids known inequality. It does not
        // imply known equality: both equality predicates may still be X.
        result.kind = Result::Impossible;
      }
      auto left = constant(compare.getLhs(), depth + 1);
      auto right = constant(compare.getRhs(), depth + 1);
      if (failed(left) || failed(right))
        return fail("owner-enable proof equality constant analysis failed");
      if (*left && *right) {
        bool sameCode = (**left).width == (**right).width &&
                        (**left).value == (**right).value;
        if (sameCode != equal)
          result.kind = Result::Impossible;
      } else if (*left || *right) {
        const auto &closed = *left ? **left : **right;
        auto selector =
            canonical(*left ? compare.getRhs() : compare.getLhs(), depth + 1);
        if (failed(selector))
          return fail("owner-enable proof equality selector analysis failed");
        result = append(std::move(result),
                        {*selector, equal, closed.value, closed.width});
      }
    }
    result = append(std::move(result), {value, polarity, {}, 0});
    visiting.erase(key);
    if (result.kind != Result::Failure) {
      if (!charge(result.facts.size()))
        return {Result::Failure, {}};
      cache.try_emplace(key, result);
    }
    return result;
  }
  ac::HardwareAnalysis analysis;
  uint64_t work;
  std::string reason;
  llvm::DenseMap<Value, Value> normalized;
  llvm::DenseSet<Value> normalizing;
  using QueryKey = std::pair<std::pair<Value, Value>, std::pair<Value, unsigned>>;
  std::pair<Value, Value> collision;
  llvm::DenseMap<QueryKey, Result> cache;
  llvm::DenseSet<QueryKey> visiting;
};

class Importer final : public SourceDeclarationContext::Implementation {
public:
  Importer(const CapturedSource &source, DictionaryAttr owner,
           const SourceHeaderRegistry &headers,
           const SourceRuleWritesAnalysis &ruleWrites,
           std::function<InFlightDiagnostic()> error)
      : source(source), owner(owner), headers(headers), ruleWrites(ruleWrites),
        error([this, error = std::move(error)] { diagnosticFailed = true; return error(); }), b(owner.getContext()), prefix(moduleName(owner)) {}
  LogicalResult prepare() override;
  FailureOr<OwningOpRef<ModuleOp>> lower() override;
  ArrayRef<MemoryCallPlan> memoryCalls() const override { return memoryPlans; }
  ArrayRef<ModuleDomainPlan> moduleDomains() const override { return domainPlans; }

private:
  LogicalResult scanImports();
  LogicalResult scanNominals();
  LogicalResult stageImports();
  LogicalResult stageEnums();
  bool lexicallyShadowed(const AstNode &node, StringRef name) const;
  std::optional<Import> lookupBinding(const AstNode &node) const;
  std::optional<ResolvedSourceBinding>
  resolveBinding(const AstNode &node) const;
  bool nominalAnnotation(const AstNode &node) const;
  FailureOr<std::optional<EnumMemberRef>>
  resolveEnumMember(const AstNode &node) const;
  FailureOr<Value> enumToBits(Value value, const AstNode &node, OpBuilder &at,
                              FlatSymbolRefAttr symbol);
  FailureOr<SmallVector<Value>>
  enumFromBits(const AstNode &call, OpBuilder &at, FlatSymbolRefAttr symbol,
               llvm::StringMap<Value> &values,
               llvm::StringMap<Instance *> &instances,
               llvm::StringMap<SmallVector<Value>> *captures = nullptr);
  LogicalResult scanModules();
  bool intrinsic(const AstNode &node, StringRef name) const;
  bool fixedAnnotation(const AstNode &node) const;
  bool hasDecorator(const AstNode &node, StringRef name) const;
  LogicalResult scanStructs();
  LogicalResult validateDefaults();
  bool staticInitializer(const AstNode &node) const;
  FailureOr<Value>
  constructStruct(const AstNode &node, OpBuilder &at, FlatSymbolRefAttr symbol,
                  llvm::StringMap<Value> &values,
                  llvm::StringMap<Instance *> &instances,
                  std::optional<Type> expected,
                  llvm::StringMap<SmallVector<Value>> *captures);
  FailureOr<Value> replaceField(Value value, StringRef field, Value replacement,
                                const AstNode &node, OpBuilder &at,
                                FlatSymbolRefAttr symbol);
  StringRef assignmentRoot(const AstNode &target) const;
  AstNode indexedFieldTarget(const AstNode &target) const;
  LogicalResult updateAssignment(const AstNode &target, Value value,
                                 const AstNode &node, OpBuilder &at,
                                 FlatSymbolRefAttr symbol,
                                 llvm::StringMap<Value> &values);
  FailureOr<std::optional<SmallVector<StringRef, 2>>>
  fixedResultTargets(const AstNode &assignment) const;
  StringRef fixedResultCall(const AstNode &node) const;
  FailureOr<bool> bindFixedResults(const AstNode &assignment, OpBuilder &at,
                                   FlatSymbolRefAttr symbol,
                                   llvm::StringMap<Value> &values,
                                   llvm::StringMap<Instance *> &instances,
                                   llvm::StringMap<BindingBoundary> *boundaries,
                                   const llvm::StringSet<> &forbidden,
                                   bool immutable);
  FailureOr<SmallVector<Value>> tableQueryResults(
      const AstNode &call, OpBuilder &at, FlatSymbolRefAttr symbol,
      llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances);
  LogicalResult preflightQueryCallback(const AstNode &callback,
      const llvm::StringMap<Value> &values, SmallVectorImpl<StringRef> &captures);
  bool queueCall(const AstNode &node) const;
  bool moduleCall(const AstNode &node) const;
  FailureOr<SmallVector<StringRef, 3>>
  queueTargets(const AstNode &assignment) const;
  LogicalResult declareQueue(const AstNode &assignment, ModuleDecl &decl,
                             OpBuilder &at, StringRef instanceName);
  FailureOr<bool> bindQueueResults(const AstNode &assignment, OpBuilder &at,
                                   FlatSymbolRefAttr symbol,
                                   llvm::StringMap<Value> &values,
                                   llvm::StringMap<Instance *> &instances);
  FailureOr<CallableDecl> callable(const AstNode &function);
  FailureOr<DictionaryAttr> sourceConstraint(const Port &port);
  LogicalResult serializeSourceCall(ModuleDecl &decl);
  LogicalResult inferModuleGraph();
  StringRef memoryReference(const AstNode &node) const;
  StringRef memoryKind(const AstNode &call) const;
  LogicalResult prepareMemoryBindings();
  const MemoryCallPlan *memoryPlan(const AstNode &call, FlatSymbolRefAttr symbol);
  LogicalResult bindMemoryInputs(const MemoryCallPlan &plan, Operation *instance,
      OpBuilder &at, FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
      llvm::StringMap<Instance *> &instances);
  FailureOr<bool> bindMemoryResults(const AstNode &statement, OpBuilder &at,
      FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
      llvm::StringMap<Instance *> &instances);
  llvm::StringMap<unsigned> sourceNameWrites(const AstNode &module) const;
  LogicalResult predeclareDirectCalls(ModuleDecl &decl, OpBuilder &at);
  FailureOr<Value> forwardModuleValue(StringRef name, FlatSymbolRefAttr symbol);
  FailureOr<Value> directModuleCall(const AstNode &call, OpBuilder &at,
                                    FlatSymbolRefAttr symbol,
                                    llvm::StringMap<Value> &values,
                                    llvm::StringMap<Instance *> &instances);
  FailureOr<SmallVector<Value>>
  lowerReturn(const ModuleDecl &decl, const AstNode &node, OpBuilder &at,
              llvm::StringMap<Value> &values,
              llvm::StringMap<Instance *> &instances);
  FailureOr<SmallVector<Value>>
  behavioralCall(ModuleDecl &decl, const AstNode &call, OpBuilder &at,
                 llvm::StringMap<Value> &values, ArrayRef<StateOwner> states,
                 bool statementCall = false);
  LogicalResult bindProposals(ArrayRef<StateOwner> states, OpBuilder &at,
                               llvm::StringMap<Value> &values,
                               FlatSymbolRefAttr symbol);
  LogicalResult preflightProposals(const AstNode &module);
  void bindState(const StateOwner &state, Value next, Value en, OpBuilder &at,
                 llvm::StringMap<Value> &values);
  LogicalResult emitBehavioral(ModuleDecl &decl);
  ac::StaticExprAttr literal(uint64_t value, const AstNode &node,
                             FlatSymbolRefAttr owner);
  Type bits(unsigned width, const AstNode &node, FlatSymbolRefAttr owner);
  FailureOr<Value> constant(Type type, unsigned value, const AstNode &node,
                            OpBuilder &at, FlatSymbolRefAttr owner);
  FailureOr<Value> select(Value guard, Value yes, Value no, const AstNode &node,
                          OpBuilder &at, FlatSymbolRefAttr owner);
  FailureOr<NumericValue> predicateView(Value value, const AstNode &node,
                                        FlatSymbolRefAttr owner);
  FailureOr<SourceChoice> sourceChoice(Value value, const AstNode &node);
  FailureOr<Value> joinSourceValues(const NumericValue &condition,
                                    SourceChoice yes, SourceChoice no,
                                    const AstNode &node, OpBuilder &at,
                                    FlatSymbolRefAttr owner);
  FailureOr<BranchEnvironment>
  joinBranches(const NumericValue &condition, const BranchEnvironment &incoming,
               const BranchEnvironment &yes, const BranchEnvironment &no,
               const AstNode &node, OpBuilder &at, FlatSymbolRefAttr owner);
  LogicalResult propagateClosed(Value result, ac::detail::ValueOpcode opcode,
                                ArrayRef<Value> operands, const AstNode &node);
  LogicalResult recordClosed(Value result, Attribute constant,
                             const AstNode &node);
  BindingBoundary inferredBoundary(StringRef name, Value value,
                                   const AstNode &node, bool declared = false);
  BindingBoundary ownerBoundary(const StateOwner &state);
  bool sameBindingBoundary(const BindingBoundary &a, const BindingBoundary &c);
  bool sameBindingContract(const BindingBoundary &a, const BindingBoundary &c);
  FailureOr<Value> applyBindingBoundary(Value value,
                                        const BindingBoundary &binding,
                                        const AstNode &node, OpBuilder &at,
                                        FlatSymbolRefAttr owner);
  FailureOr<Value> tableUpdate(Value table, Value index, Value replacement,
                               ArrayRef<Attribute> fieldPath,
                               const AstNode &node, OpBuilder &at,
                               FlatSymbolRefAttr owner,
                               std::optional<size_t> domainSlot = std::nullopt);
  FailureOr<std::optional<size_t>> addressDomainSlot(
      StringRef formal, const AstNode &assignment, const AstNode &index);
  LogicalResult proveIndex(Value table, Value index, const AstNode &node);
  FailureOr<Value> unsignedBoundary(Value value, Type target,
                                    const AstNode &node, OpBuilder &at,
                                    FlatSymbolRefAttr symbol,
                                    bool allowFixedWidening = true);
  FailureOr<Value> fixedUnsignedShift(Value input, const llvm::APSInt &count,
                                      StringRef opcode, const AstNode &node,
                                      OpBuilder &at, FlatSymbolRefAttr symbol);
  FailureOr<Value> compareSourceValues(Value lhs, Value rhs,
                                       StringRef predicate,
                                       const AstNode &leftSite,
                                       const AstNode &rightSite,
                                       const AstNode &site, OpBuilder &at,
                                       FlatSymbolRefAttr owner);
  FailureOr<MatchPatternKey> matchPatternKey(const AstNode &atom);
  FailureOr<Value> materializeMatchKey(const MatchPatternKey &key, Type type,
                                       OpBuilder &at, FlatSymbolRefAttr owner,
                                       bool fixedBits);
  LogicalResult matchStatement(const AstNode &statement, OpBuilder &at,
                               FlatSymbolRefAttr owner,
                               BranchEnvironment &environment,
                               llvm::StringSet<> &owners);
  LogicalResult statements(const AstNode &parent, StringRef field,
                           OpBuilder &at, FlatSymbolRefAttr owner,
                           BranchEnvironment &environment,
                           llvm::StringSet<> &owners, AstNode &returned);
  // Source assertion sites only. A structural rule publishes its observations
  // through the module-level instrumentation pass, so the rule result suffix it
  // reserves must not include them.
  size_t countSourceAssertions(const AstNode &parent, StringRef field);
  // Capture groups the rule result suffix must carry: source assertion sites
  // plus one group per hardware value operand of a rule-body `log`/`report`
  // (or a single value-less group for a purely literal `log`). Both the
  // behavioral rule lowering that reserves the suffix and the walker that fills
  // it must agree, so both derive the count here.
  size_t countSourceChecks(const AstNode &parent, StringRef field);
  // The hardware value operands of one `log`/`report` call in source order.
  // Literal string items stay in the static spec; every other item is a value.
  // Returns false when the call is not a well-formed observation, leaving the
  // diagnostic to the caller that accepts the site.
  bool observationValueItems(const AstNode &call, StringRef intrinsic,
                             SmallVectorImpl<AstNode> &items);
  // Validate one `log`/`report` call and build its closed static spec together
  // with its ordered hardware value operands. Shared by the pure-module
  // instrumentation pass and the behavioral rule-body capture so both accept
  // exactly the same source forms and emit the same diagnostics.
  LogicalResult buildObservation(const AstNode &statement,
                                 SmallVectorImpl<AstNode> &valueItems,
                                 DictionaryAttr &spec, StringRef &intrinsic);
  FailureOr<Value> checkAnd(Value lhs, Value rhs, const AstNode &node,
                            OpBuilder &at, FlatSymbolRefAttr owner);
  FailureOr<Value> checkNot(Value value, const AstNode &node, OpBuilder &at,
                            FlatSymbolRefAttr owner);
  LogicalResult enterCheckBranch(BranchEnvironment &environment, Value demand,
                                 const AstNode &node, OpBuilder &at,
                                 FlatSymbolRefAttr owner);
  LogicalResult captureSourceCheck(const AstNode &statement, OpBuilder &at,
                                   FlatSymbolRefAttr owner,
                                   BranchEnvironment &environment);
  // Capture one rule-body `log`/`report` site onto the rule result suffix. The
  // observation is published as `ac.observe` at module scope by
  // publishSourceChecks once the rule operation exists, exactly like a source
  // assertion, because `ac.observe` requires module placement.
  LogicalResult captureRuleObservation(const AstNode &rule,
                                       const AstNode &statement, OpBuilder &at,
                                       FlatSymbolRefAttr owner,
                                       BranchEnvironment &environment);
  LogicalResult publishSourceChecks(Operation *rule, RuleEvaluation &evaluation,
                                    OpBuilder &at, FlatSymbolRefAttr owner);
  LogicalResult structuralBinding(const AstNode &call, OpBuilder &at,
                                  FlatSymbolRefAttr owner,
                                  BranchEnvironment &environment);

  FailureOr<ModuleDecl> signature(const AstNode &node);
  FailureOr<Type> portType(const AstNode &node, FlatSymbolRefAttr owner);
  FailureOr<ac::StaticExprAttr> staticExpr(const AstNode &node,
                                           FlatSymbolRefAttr owner);
  FailureOr<SliceShape> resolveSlice(const AstNode &slice, Type base,
                                     FlatSymbolRefAttr owner);
  bool concatCall(const AstNode &node) const;
  LogicalResult validateConcatCall(const AstNode &node);
  FailureOr<Type> resolveConcatType(const AstNode &node,
                                    ArrayRef<Type> operands,
                                    FlatSymbolRefAttr owner);
  StringRef fixedCountCall(const AstNode &node) const;
  LogicalResult validateFixedCountCall(const AstNode &node, StringRef name);
  FailureOr<FixedCountShape> resolveFixedCountShape(const AstNode &node,
                                                    Type operand,
                                                    FlatSymbolRefAttr owner,
                                                    StringRef name);
  FailureOr<FixedCountShape> resolveFixedInputShape(Value input,
                                                    const AstNode &node,
                                                    FlatSymbolRefAttr owner,
                                                    StringRef name);
  SmallVector<std::pair<uint64_t, Value>, 64>
  lowerFixedMasks(uint64_t width, const AstNode &node, OpBuilder &at,
                  FlatSymbolRefAttr owner);
  Value lowerPopcountValue(Value input, const FixedCountShape &shape,
                           const AstNode &node, OpBuilder &at,
                           FlatSymbolRefAttr owner);
  FailureOr<bool> validateFixedEncodingCall(const AstNode &node,
                                            StringRef name);
  SmallVector<Value> lowerFixedEncodingValue(Value input,
                                             const FixedCountShape &shape,
                                             bool high, bool onehot,
                                             const AstNode &node, OpBuilder &at,
                                             FlatSymbolRefAttr owner);
  FailureOr<SmallVector<Value>>
  fixedEncodingResults(const AstNode &node, OpBuilder &at,
                       FlatSymbolRefAttr owner, llvm::StringMap<Value> &values,
                       llvm::StringMap<Instance *> &instances);
  FailureOr<Value> lowerFixedCount(Value input, const AstNode &node,
                                   OpBuilder &at, FlatSymbolRefAttr owner,
                                   StringRef name);
  LogicalResult emitModule(ModuleDecl &decl);
  FailureOr<SmallVector<Attribute>> bindParameters(Operation *callee,
                                                   const AstNode &call,
                                                   FlatSymbolRefAttr parent);
  FailureOr<SmallVector<Attribute>> bindTypeArguments(Operation *callee,
                                                      const AstNode &call,
                                                      FlatSymbolRefAttr parent);
  FailureOr<Type> substitute(Type type, Operation *callee,
                             ArrayRef<Attribute> actuals,
                             ArrayRef<Attribute> typeActuals);
  FailureOr<Value>
  expression(const AstNode &node, OpBuilder &at, FlatSymbolRefAttr owner,
             llvm::StringMap<Value> &values,
             llvm::StringMap<Instance *> &instances,
             std::optional<Type> expected = std::nullopt,
             llvm::StringMap<SmallVector<Value>> *captures = nullptr);
  FailureOr<Type> expressionType(const AstNode &node,
                                 llvm::StringMap<Value> &values,
                                 llvm::StringMap<Instance *> &instances,
                                 FlatSymbolRefAttr owner);
  FailureOr<Value> trueValue(const AstNode &node, OpBuilder &at,
                             FlatSymbolRefAttr owner);
  LogicalResult emitInstrumentation(const ModuleDecl &module,
                                    const AstNode &rule,
                                    const AstNode &statement, unsigned ordinal,
                                    OpBuilder &at,
                                    llvm::StringMap<Value> &moduleValues,
                                    llvm::StringMap<Instance *> &instances);
  DictionaryAttr namespaceSite(const AstNode &node);
  NumericLoweringSite numericSite(const AstNode &node,
                                  FlatSymbolRefAttr ownerSymbol);
  std::optional<ac::detail::ValueKind> annotationKind(const AstNode &node);
  NumericValue valueInfo(Value value);
  void remember(Value value, std::optional<ac::detail::ValueKind> kind,
                std::optional<IntegerInterval> interval = std::nullopt,
                Attribute closed = {});
  Value captureValue(Block *block, Value original, Location location);
  FailureOr<bool> equivalentBoundaryTypes(Type actual, Type declared,
                                          Operation *site);
  Value boundaryBitsIdentity(Value value, Type target, const AstNode &node,
                             OpBuilder &at, FlatSymbolRefAttr ownerSymbol);
  FailureOr<Value> boundary(Value value, const Port &destination,
                            const AstNode &node, OpBuilder &at,
                            FlatSymbolRefAttr ownerSymbol);
  std::string qualify(StringRef name) const {
    return prefix.empty() ? name.str() : (Twine(prefix) + "." + name).str();
  }
  ArrayAttr parameters(Operation *op) const {
    return op ? op->getAttrOfType<ArrayAttr>("parameters") : ArrayAttr();
  }

  CapturedSource source;
  DictionaryAttr owner;
  const SourceHeaderRegistry &headers;
  const SourceRuleWritesAnalysis &ruleWrites;
  bool diagnosticFailed = false;
  std::function<InFlightDiagnostic()> error;
  OpBuilder b;
  std::string prefix;
  OwningOpRef<ModuleOp> body;
  llvm::StringMap<Import> names;
  llvm::StringMap<AstNode> structs, behavioralRules;
  llvm::StringMap<CapturedEnumSyntax> enums;
  llvm::StringMap<ArrayAttr> structFields;
  llvm::StringMap<SmallVector<AstNode>> structDefaults;
  SmallVector<OwnerProposal> ownerProposals;
  std::unique_ptr<OwnerEnableProof> grantProof;
  TableQueryBudget queryBudget;
  std::string queryArgument;
  llvm::DenseSet<size_t> dischargedPairs;
  SmallVector<MemoryCallPlan, 0> memoryPlans;
  SmallVector<ModuleDomainPlan, 0> domainPlans;
  llvm::StringSet<> memoryResultNames;
  unsigned directCallOrdinal = 0;
  llvm::DenseMap<DictionaryAttr, DirectCallSite> directCallSites;
  llvm::DenseSet<DictionaryAttr> structuralCallSites;
  llvm::StringMap<ForwardCallBinding> forwardCallBindings;
  llvm::StringSet<> queueResultNames;
  ModuleDecl *activeBehavioralDecl = nullptr;
  SmallVector<StateOwner> *activeStates = nullptr;
  RuleEvaluation *activeRule = nullptr;
  llvm::StringMap<size_t> local;
  SmallVector<ModuleDecl, 0> modules;
  SmallVector<DictionaryAttr> dependencies;
  llvm::DenseMap<Value, NumericValue> numericValues;
  llvm::DenseSet<Value> arithmeticValues, fixedValues;
};

ArrayAttr portNames(OpBuilder &b, ArrayRef<Port> ports);

#include "PythonImportAggregates.inc"
#include "PythonImportAssignments.inc"
#include "PythonImportBehavior.inc"
#include "PythonImportComposition.inc"
#include "PythonImportMemoryPreparation.inc"
#include "PythonImportMemoryCalls.inc"
#include "PythonImportDefaults.inc"
#include "PythonImportQueues.inc"
#include "PythonImportRuleCall.inc"
#include "PythonImportSourceCalls.inc"

std::optional<ac::detail::ValueKind>
Importer::annotationKind(const AstNode &node) {
  if (nominalAnnotation(node))
    return std::nullopt;
  if (node.kind() == "Name" && node.string("id") == "bool")
    return ac::detail::ValueKind::Boolean;
  if (node.kind() == "Subscript" && node.child("value").kind() == "Name" &&
      node.child("value").string("id") == "Annotated")
    return ac::detail::ValueKind::Integer;
  return std::nullopt;
}
NumericLoweringSite Importer::numericSite(const AstNode &node,
                                          FlatSymbolRefAttr ownerSymbol) {
  return {node.location(b.getContext(), source.path),
          occurrence(b, ownerSymbol, node), sourceSpan(b, source.path, node)};
}
NumericValue Importer::valueInfo(Value value) {
  auto found = numericValues.find(value);
  return found == numericValues.end() ? NumericValue{value, {}, {}}
                                      : found->second;
}
void Importer::remember(Value value, std::optional<ac::detail::ValueKind> kind,
                        std::optional<IntegerInterval> interval,
                        Attribute closed) {
  if (isa<ac::EnumType>(value.getType())) {
    numericValues.erase(value);
    return;
  }
  if (!interval && kind == ac::detail::ValueKind::Integer) {
    auto bits = dyn_cast<ac::BitsType>(value.getType());
    Operation *site = value.getDefiningOp();
    if (auto argument = dyn_cast<BlockArgument>(value))
      site = argument.getOwner()->getParentOp();
    ac::HardwareAnalysis analysis(*body);
    if (bits && analysis.isStaticEvaluable(bits.getWidth(), {})) {
      auto width = analysis.evaluateStatic(bits.getWidth(), {}, site);
      auto integer = succeeded(width) ? dyn_cast<ac::MathIntAttr>(*width)
                                      : ac::MathIntAttr();
      if (integer) {
        llvm::APSInt amount(integer.getCanonicalValue());
        if (!amount.isNegative() && !amount.isZero() &&
            amount.getActiveBits() <= std::numeric_limits<unsigned>::digits &&
            amount.getZExtValue() < std::numeric_limits<unsigned>::max()) {
          unsigned count = static_cast<unsigned>(amount.getZExtValue());
          llvm::APInt upper(count + 1, 0);
          upper.setBit(count);
          interval = IntegerInterval{llvm::APSInt::getUnsigned(0),
                                     llvm::APSInt(std::move(upper), true)};
        }
      }
    }
  }
  numericValues[value] = {value, kind, std::move(interval), closed};
}
Value Importer::captureValue(Block *block, Value original, Location location) {
  Value argument = block->addArgument(original.getType(), location);
  // A capture preserves the producer's facts; its physical type alone does
  // not establish a new interval, source kind or mathematical interpretation.
  NumericValue info = valueInfo(original);
  info.value = argument;
  if (!isa<ac::EnumType>(original.getType()))
    numericValues[argument] = std::move(info);
  if (fixedValues.contains(original))
    fixedValues.insert(argument);
  if (arithmeticValues.contains(original))
    arithmeticValues.insert(argument);
  return argument;
}
FailureOr<bool> Importer::equivalentBoundaryTypes(Type actual, Type declared,
                                                  Operation *site) {
  if (ac::areEquivalentHardwareTypes(actual, declared))
    return true;
  ac::HardwareAnalysis analysis(*body);
  auto resolvedActual = analysis.resolveType(actual, {}, site);
  if (failed(resolvedActual))
    return failure();
  auto resolvedDeclared = analysis.resolveType(declared, {}, site);
  if (failed(resolvedDeclared))
    return failure();
  return ac::areEquivalentHardwareTypes(*resolvedActual, *resolvedDeclared);
}
Value Importer::boundaryBitsIdentity(Value value, Type target,
                                     const AstNode &node, OpBuilder &at,
                                     FlatSymbolRefAttr ownerSymbol) {
  auto result = createOp(at, node.location(b.getContext(), source.path),
                         ac::BitsExtractOp::getOperationName(), {value},
                         TypeRange{target},
                         {at.getNamedAttr("low", literal(0, node, ownerSymbol))})
                    ->getResult(0);
  auto info = valueInfo(value);
  info.value = result;
  numericValues[result] = std::move(info);
  if (fixedValues.contains(value))
    fixedValues.insert(result);
  if (arithmeticValues.contains(value))
    arithmeticValues.insert(result);
  return result;
}
FailureOr<Value> Importer::boundary(Value value, const Port &destination,
                                    const AstNode &node, OpBuilder &at,
                                    FlatSymbolRefAttr ownerSymbol) {
  auto info = valueInfo(value);
  if (destination.sourceKind && info.sourceKind &&
      destination.sourceKind != info.sourceKind)
    return error()
           << "source Boolean and Integer kinds cannot be implicitly converted";
  auto equivalent = equivalentBoundaryTypes(value.getType(), destination.type,
                                            at.getBlock()->getParentOp());
  if (failed(equivalent))
    return failure();
  if (isa<ac::EnumType>(value.getType()) ||
      isa<ac::EnumType>(destination.type)) {
    if (!*equivalent)
      return error() << "nominal enum boundary hardware type mismatch";
    return value;
  }
  bool negative = info.interval && info.interval->lower.isNegative();
  if (arithmeticValues.contains(value) || negative || !*equivalent) {
    if (destination.sourceKind != ac::detail::ValueKind::Integer ||
        info.sourceKind != ac::detail::ValueKind::Integer)
      return error() << "mathematical boundary requires known Integer source "
                        "and destination kinds";
    if (*equivalent && isa<ac::BitsType>(destination.type) &&
        !ac::areEquivalentHardwareTypes(value.getType(), destination.type)) {
      value = boundaryBitsIdentity(value, destination.type, node, at,
                                   ownerSymbol);
      info = valueInfo(value);
    }
    auto converted =
        lowerExactIntegerBoundary(at, numericSite(node, ownerSymbol), info,
                                  dyn_cast<ac::BitsType>(destination.type));
    if (failed(converted))
      return failure();
    remember(*converted, info.sourceKind, info.interval,
             info.closedSourceConstant);
    if (arithmeticValues.contains(value))
      arithmeticValues.insert(*converted);
    return *converted;
  }
  if (isa<ac::BitsType>(value.getType()) &&
      isa<ac::BitsType>(destination.type) &&
      !ac::areEquivalentHardwareTypes(value.getType(), destination.type))
    return boundaryBitsIdentity(value, destination.type, node, at, ownerSymbol);
  return value;
}

FailureOr<NumericValue> Importer::predicateView(Value value,
                                                const AstNode &node,
                                                FlatSymbolRefAttr ownerSymbol) {
  using ac::detail::ValueKind;
  auto diagnostic = [&] {
    return mlir::emitError(node.location(b.getContext(), source.path));
  };
  auto info = valueInfo(value);
  if (info.sourceKind == ValueKind::Integer)
    return diagnostic()
           << "conditional requires a Boolean source condition, not Integer";
  if (!ac::areEquivalentHardwareTypes(value.getType(),
                                      bits(1, node, ownerSymbol)) ||
      (info.sourceKind != ValueKind::Boolean && !fixedValues.contains(value)))
    return diagnostic()
           << "conditional requires authoritative Boolean or bits[1]";
  // Authority is local to this predicate use, never assigned to its producer.
  info.sourceKind = ValueKind::Boolean;
  return info;
}

FailureOr<SourceChoice> Importer::sourceChoice(Value value,
                                               const AstNode &node) {
  auto info = valueInfo(value);
  SourceChoice choice{value, std::nullopt};
  if (!info.closedSourceConstant)
    return choice;
  using ac::detail::ValueKind;
  if ((info.sourceKind == ValueKind::Integer &&
       isa<ac::MathIntAttr>(info.closedSourceConstant)) ||
      (info.sourceKind == ValueKind::Boolean &&
       isa<BoolAttr>(info.closedSourceConstant))) {
    choice.constant = ClosedSourceConstant{info.closedSourceConstant};
    return choice;
  }
  return mlir::emitError(node.location(b.getContext(), source.path))
         << "closed source constant disagrees with source kind";
}

LogicalResult Importer::propagateClosed(Value result,
                                        ac::detail::ValueOpcode opcode,
                                        ArrayRef<Value> operands,
                                        const AstNode &node) {
  if (fixedValues.contains(result) || !valueInfo(result).sourceKind)
    return success();
  auto contract = ac::detail::getValueOpcodeInfo(opcode);
  if (valueInfo(result).sourceKind != contract.result)
    return success();
  SmallVector<Attribute> constants;
  for (auto [index, operand] : llvm::enumerate(operands)) {
    auto choice = sourceChoice(operand, node);
    if (failed(choice))
      return failure();
    if (!choice->constant)
      return success();
    auto constraint = contract.operands[index];
    Attribute constant = choice->constant->value;
    // The admitted one-bit Boolean XOR/comparisons use the existing exact
    // comparison evaluator on Boolean-to-Integer constants only. Producer
    // source facts remain Boolean.
    if (constraint == ac::detail::ValueKindConstraint::Integer &&
        contract.result == ac::detail::ValueKind::Boolean &&
        isa<BoolAttr>(constant)) {
      auto converted = ac::detail::evaluateValue(
          ac::detail::ValueOpcode::ToInt, {constant}, [&] {
            return mlir::emitError(node.location(b.getContext(), source.path));
          });
      if (failed(converted))
        return failure();
      constant = *converted;
    }
    if ((constraint == ac::detail::ValueKindConstraint::Integer &&
         !isa<ac::MathIntAttr>(constant)) ||
        (constraint == ac::detail::ValueKindConstraint::Boolean &&
         !isa<BoolAttr>(constant)))
      return success();
    constants.push_back(constant);
  }
  auto evaluated = ac::detail::evaluateValue(opcode, constants, [&] {
    return mlir::emitError(node.location(b.getContext(), source.path));
  });
  if (failed(evaluated))
    return failure();
  return recordClosed(result, *evaluated, node);
}

LogicalResult Importer::recordClosed(Value result, Attribute constant,
                                     const AstNode &node) {
  auto &info = numericValues[result];
  info.closedSourceConstant = constant;
  if (auto integer = dyn_cast<ac::MathIntAttr>(constant)) {
    auto upper = ac::detail::evaluateValue(
        ac::detail::ValueOpcode::Add,
        {constant,
         ac::MathIntAttr::get(b.getContext(), llvm::APSInt::getUnsigned(1))},
        [&] {
          return mlir::emitError(node.location(b.getContext(), source.path));
        });
    if (failed(upper))
      return failure();
    info.interval = IntegerInterval{
        llvm::APSInt(integer.getCanonicalValue()),
        llvm::APSInt(cast<ac::MathIntAttr>(*upper).getCanonicalValue())};
  }
  return success();
}

FailureOr<Value> Importer::joinSourceValues(const NumericValue &condition,
                                            SourceChoice yes, SourceChoice no,
                                            const AstNode &node, OpBuilder &at,
                                            FlatSymbolRefAttr ownerSymbol) {
  using ac::detail::ValueKind;
  if (yes.value == no.value)
    return yes.value;
  if (!isa<ac::BitsType>(yes.value.getType()) ||
      !isa<ac::BitsType>(no.value.getType()))
    return select(condition.value, yes.value, no.value, node, at, ownerSymbol);
  bool yesFixed = fixedValues.contains(yes.value);
  bool noFixed = fixedValues.contains(no.value);
  if (yesFixed != noFixed) {
    auto &constant = yesFixed ? no : yes;
    Type target = (yesFixed ? yes : no).value.getType();
    if (!constant.constant)
      return mlir::emitError(node.location(b.getContext(), source.path))
             << "fixed branch peer requires a closed source Integer or Boolean "
                "constant";
    if (isa<BoolAttr>(constant.constant->value) &&
        !ac::areEquivalentHardwareTypes(target, bits(1, node, ownerSymbol)))
      return mlir::emitError(node.location(b.getContext(), source.path))
             << "closed Boolean branch requires a bits[1] peer";
    auto converted =
        unsignedBoundary(constant.value, target, node, at, ownerSymbol);
    if (failed(converted))
      return failure();
    constant.value = *converted;
    yesFixed = noFixed = true;
  }
  if (yesFixed && noFixed)
    return select(condition.value, yes.value, no.value, node, at, ownerSymbol);

  auto a = valueInfo(yes.value), c = valueInfo(no.value);
  if (a.sourceKind && c.sourceKind && a.sourceKind != c.sourceKind)
    return mlir::emitError(node.location(b.getContext(), source.path))
           << "conditional branches have different known Boolean and Integer "
              "kinds";
  bool mathematical = arithmeticValues.contains(yes.value) ||
                      arithmeticValues.contains(no.value) ||
                      (a.interval && a.interval->lower.isNegative()) ||
                      (c.interval && c.interval->lower.isNegative());
  auto closeSelection = [&](Value result) -> FailureOr<Value> {
    auto guard = dyn_cast_or_null<BoolAttr>(condition.closedSourceConstant);
    if (guard && yes.constant && no.constant &&
        failed(recordClosed(
            result, guard.getValue() ? yes.constant->value : no.constant->value,
            node)))
      return failure();
    return result;
  };
  if (mathematical || !ac::areEquivalentHardwareTypes(yes.value.getType(),
                                                      no.value.getType())) {
    auto result = lowerExactIntegerSelect(at, numericSite(node, ownerSymbol),
                                          condition, a, c);
    if (failed(result))
      return failure();
    numericValues[result->value] = *result;
    arithmeticValues.insert(result->value);
    return closeSelection(result->value);
  }
  auto result =
      select(condition.value, yes.value, no.value, node, at, ownerSymbol);
  if (failed(result))
    return failure();
  std::optional<IntegerInterval> interval;
  if (a.interval && c.interval)
    interval = IntegerInterval{
        llvm::APSInt::compareValues(a.interval->lower, c.interval->lower) < 0
            ? a.interval->lower
            : c.interval->lower,
        llvm::APSInt::compareValues(a.interval->upper, c.interval->upper) > 0
            ? a.interval->upper
            : c.interval->upper};
  remember(*result, a.sourceKind == c.sourceKind ? a.sourceKind : std::nullopt,
           interval);
  return closeSelection(*result);
}

BindingBoundary Importer::inferredBoundary(StringRef name, Value value,
                                           const AstNode &node, bool declared) {
  auto info = valueInfo(value);
  Type type = !declared && info.sourceKind == ac::detail::ValueKind::Integer
                  ? Type()
                  : value.getType();
  return {
      {name.str(), type, node, info.sourceKind, fixedValues.contains(value)},
      declared,
      declared ? IntegerBoundaryMode::Representation
               : IntegerBoundaryMode::Inferred};
}

BindingBoundary Importer::ownerBoundary(const StateOwner &state) {
  auto annotation = state.node.kind() == "AnnAssign"
                        ? state.node.child("annotation")
                        : state.node.child("value").child("func");
  return {{state.name, state.type, annotation, annotationKind(annotation),
           fixedAnnotation(annotation)},
          true};
}

bool Importer::sameBindingBoundary(const BindingBoundary &a,
                                   const BindingBoundary &c) {
  return sameBindingContract(a, c) &&
         (a.contract.sourceKind != ac::detail::ValueKind::Integer ||
          a.integerMode == c.integerMode);
}

bool Importer::sameBindingContract(const BindingBoundary &a,
                                   const BindingBoundary &c) {
  return a.contract.sourceKind == c.contract.sourceKind &&
         a.contract.fixedBits == c.contract.fixedBits &&
         ((!a.contract.type && !c.contract.type) ||
          (a.contract.type && c.contract.type &&
           ac::areEquivalentHardwareTypes(a.contract.type, c.contract.type)));
}

FailureOr<Value> Importer::applyBindingBoundary(Value value,
                                                const BindingBoundary &binding,
                                                const AstNode &node,
                                                OpBuilder &at,
                                                FlatSymbolRefAttr ownerSymbol) {
  using ac::detail::ValueKind;
  const Port &contract = binding.contract;
  auto info = valueInfo(value);
  if (contract.sourceKind &&
      (info.sourceKind != contract.sourceKind || fixedValues.contains(value)))
    return mlir::emitError(node.location(b.getContext(), source.path))
           << "binding boundary requires declared "
           << (contract.sourceKind == ValueKind::Boolean ? "Boolean"
                                                         : "Integer")
           << " source kind";
  if (contract.fixedBits)
    return unsignedBoundary(value, contract.type, node, at, ownerSymbol);
  if (!contract.type)
    return value;
  if (contract.sourceKind == ValueKind::Integer) {
    if (binding.integerMode == IntegerBoundaryMode::Inferred)
      return value;
    if (binding.integerMode == IntegerBoundaryMode::Representation) {
      if (!ac::areEquivalentHardwareTypes(value.getType(), contract.type))
        return mlir::emitError(node.location(b.getContext(), source.path))
               << "Integer representation binding requires original hardware "
                  "type";
      return value;
    }
    auto converted =
        lowerExactIntegerBoundary(at, numericSite(node, ownerSymbol), info,
                                  dyn_cast<ac::BitsType>(contract.type));
    if (failed(converted))
      return failure();
    if (*converted != value) {
      remember(*converted, info.sourceKind, info.interval,
               info.closedSourceConstant);
      if (arithmeticValues.contains(value))
        arithmeticValues.insert(*converted);
    }
    return *converted;
  }
  if (!ac::areEquivalentHardwareTypes(value.getType(), contract.type))
    return mlir::emitError(node.location(b.getContext(), source.path))
           << "binding boundary hardware type mismatch";
  if (isa<ac::BitsType>(contract.type) && !contract.sourceKind)
    return mlir::emitError(node.location(b.getContext(), source.path))
           << "binding boundary requires authoritative source kind";
  return value;
}

FailureOr<BranchEnvironment> Importer::joinBranches(
    const NumericValue &condition, const BranchEnvironment &incoming,
    const BranchEnvironment &yes, const BranchEnvironment &no,
    const AstNode &node, OpBuilder &at, FlatSymbolRefAttr ownerSymbol) {
  BranchEnvironment joined;
  joined.boundaries = incoming.boundaries;
  joined.ambient = incoming.ambient;
  if (incoming.continuation) {
    auto selected = select(condition.value, yes.continuation, no.continuation,
                           node, at, ownerSymbol);
    if (failed(selected))
      return failure();
    auto continued =
        checkAnd(incoming.continuation, *selected, node, at, ownerSymbol);
    if (failed(continued))
      return failure();
    joined.continuation = *continued;
  }
  // Declarations constrain every retained path, including a path without a
  // value. Inferred partial bindings do not survive the join.
  for (const auto *branch : {&yes, &no})
    for (const auto &item : branch->boundaries) {
      if (!item.second.declared)
        continue;
      auto existing = joined.boundaries.find(item.first());
      if (existing != joined.boundaries.end() && existing->second.declared &&
          !sameBindingBoundary(existing->second, item.second))
        return mlir::emitError(node.location(b.getContext(), source.path))
               << "branch declarations have incompatible binding boundaries";
      joined.boundaries[item.first()] = item.second;
    }
  for (const auto &item : yes.values) {
    auto opposite = no.values.find(item.first());
    if (opposite == no.values.end())
      continue;
    Value yesValue = item.second, noValue = opposite->second;
    auto binding = joined.boundaries.find(item.first());
    if (binding != joined.boundaries.end()) {
      auto a = applyBindingBoundary(yesValue, binding->second, node, at,
                                    ownerSymbol);
      auto c =
          applyBindingBoundary(noValue, binding->second, node, at, ownerSymbol);
      if (failed(a) || failed(c))
        return failure();
      yesValue = *a;
      noValue = *c;
    }
    auto a = sourceChoice(yesValue, node);
    auto c = sourceChoice(noValue, node);
    if (failed(a) || failed(c))
      return failure();
    auto value = joinSourceValues(condition, *a, *c, node, at, ownerSymbol);
    if (failed(value))
      return failure();
    if (binding != joined.boundaries.end()) {
      auto converted =
          applyBindingBoundary(*value, binding->second, node, at, ownerSymbol);
      if (failed(converted))
        return failure();
      value = *converted;
    } else {
      joined.boundaries[item.first()] =
          inferredBoundary(item.first(), *value, node);
    }
    joined.values[item.first()] = *value;
  }
  for (const auto &item : incoming.writes) {
    auto value = select(condition.value, yes.writes.lookup(item.first()),
                        no.writes.lookup(item.first()), node, at, ownerSymbol);
    if (failed(value))
      return failure();
    joined.writes[item.first()] = *value;
  }
  return joined;
}

DictionaryAttr Importer::namespaceSite(const AstNode &node) {
  SmallVector<Attribute> path;
  for (const AstStep &step : node.path) {
    if (step.index)
      path.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("kind", b.getStringAttr("index")),
           b.getNamedAttr("value", b.getI64IntegerAttr(*step.index))}));
    else
      path.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("kind", b.getStringAttr("field")),
           b.getNamedAttr("name", b.getStringAttr(step.field))}));
  }
  return b.getDictionaryAttr(
      {b.getNamedAttr("ast_path", b.getArrayAttr(path)),
       b.getNamedAttr("location", sourceSpan(b, source.path, node))});
}

LogicalResult Importer::scanImports() {
  llvm::DenseSet<Attribute> seenDependencies;
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode stmt = source.module.item("body", i);
    if (stmt.kind() == "Import") {
      for (size_t j = 0; j < stmt.array("names").size(); ++j) {
        auto alias = stmt.item("names", j);
        StringRef remote = alias.string("name");
        if (remote != "pycircuit" && remote != "enum")
          return error()
                 << "only pycircuit and enum namespace imports are supported";
        auto rename = dyn_cast_or_null<StringAttr>(alias.get("asname"));
        StringRef name = rename ? rename.getValue() : remote;
        if (names.contains(name))
          return error() << "duplicate or conflicting import binding '" << name
                         << "'";
        names[name] = {{},
                       nullptr,
                       alias,
                       {},
                       remote.str(),
                       false,
                       BindingCategory::Namespace,
                       remote.str()};
      }
      continue;
    }
    if (stmt.kind() != "ImportFrom")
      continue;
    auto module = dyn_cast_or_null<StringAttr>(stmt.get("module"));
    auto encodedLevel = dyn_cast_or_null<DictionaryAttr>(stmt.get("level"));
    auto level =
        encodedLevel ? encodedLevel.getAs<StringAttr>("integer") : StringAttr();
    if (!module || !level || level.getValue() != "0")
      return error() << "relative imports are unsupported";
    for (size_t j = 0; j < stmt.array("names").size(); ++j) {
      AstNode alias = stmt.item("names", j);
      StringRef remote = alias.string("name");
      auto rename = dyn_cast_or_null<StringAttr>(alias.get("asname"));
      StringRef name = rename ? rename.getValue() : remote;
      if (names.contains(name))
        return error() << "duplicate or conflicting import binding '" << name
                       << "'";
      if ((module.getValue() == "__future__" && remote == "annotations") ||
          (module.getValue() == "typing" && remote == "Annotated")) {
        names[name] = {{},
                       nullptr,
                       alias,
                       {},
                       remote.str(),
                       false,
                       BindingCategory::Other,
                       module.getValue().str()};
        continue;
      }
      if (classifyImportedMarker(module.getValue(), remote) !=
              MarkerKind::None ||
          (module.getValue() == "pycircuit" &&
           llvm::is_contained(ArrayRef<StringRef>{"system", "log", "report"},
                              remote))) {
        names[name] = {{},
                       nullptr,
                       alias,
                       {},
                       remote.str(),
                       false,
                       BindingCategory::Marker,
                       module.getValue().str()};
        continue;
      }
      if (module.getValue() == "pycircuit" &&
          llvm::is_contained(ArrayRef<StringRef>{"dff", "dffe", "sync_mem",
                                                 "sync_mem_dp", "byte_mem"},
                             remote)) {
        auto builtin = headers.lookupBuiltin(remote);
        if (!builtin || !headers.isTrustedBuiltin(builtin.declaration))
          return error() << "trusted builtin '" << remote << "' is unavailable";
        names[name] = {
            builtin.symbol, builtin.declaration, alias, {}, remote.str(), true};
        continue;
      }
      auto symbol = headers.lookupExport(module.getValue(), remote);
      auto provider = headers.ownerForModule(module.getValue());
      Operation *declaration =
          symbol ? headers.lookupDeclaration(symbol) : nullptr;
      if (!symbol || !provider ||
          !isa_and_nonnull<ac::ModuleImportOp, ac::StructOp, ac::EnumOp>(
              declaration))
        return error() << "imported declaration '" << module.getValue() << "."
                       << remote << "' is absent from explicit headers";
      auto category = isa<ac::EnumOp>(declaration) ? BindingCategory::Enum
                      : isa<ac::StructOp>(declaration)
                          ? BindingCategory::Struct
                          : BindingCategory::Module;
      names[name] = {symbol,       declaration, alias,   provider,
                     remote.str(), false,       category};
      auto closure = headers.interfacesForModule(module.getValue());
      if (!closure)
        return error()
               << "imported provider lacks its admitted interface closure";
      if (seenDependencies.insert(provider).second)
        dependencies.push_back(provider);
      for (Attribute raw : closure) {
        auto dependency = cast<DictionaryAttr>(raw);
        if (seenDependencies.insert(dependency).second)
          dependencies.push_back(dependency);
      }
    }
  }
  llvm::sort(dependencies, [](DictionaryAttr a, DictionaryAttr c) {
    auto packageA = a.getAs<StringAttr>("package").getValue();
    auto packageC = c.getAs<StringAttr>("package").getValue();
    return packageA != packageC ? packageA < packageC
                                : a.getAs<StringAttr>("path").getValue() <
                                      c.getAs<StringAttr>("path").getValue();
  });
  auto rejectShadow = [&](auto &&self, const AstNode &target) -> LogicalResult {
    if (target.kind() == "Name" && names.contains(target.string("id")))
      return error() << "source binding is shadowed at top level: '"
                     << target.string("id") << "'";
    if (target.kind() == "Tuple" || target.kind() == "List")
      for (size_t j = 0; j < target.array("elts").size(); ++j)
        if (failed(self(self, target.item("elts", j))))
          return failure();
    return success();
  };
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto stmt = source.module.item("body", i);
    if (stmt.kind() == "Assign") {
      for (size_t j = 0; j < stmt.array("targets").size(); ++j)
        if (failed(rejectShadow(rejectShadow, stmt.item("targets", j))))
          return failure();
    } else if (stmt.kind() == "AnnAssign" || stmt.kind() == "AugAssign") {
      if (failed(rejectShadow(rejectShadow, stmt.child("target"))))
        return failure();
    }
  }
  return success();
}

LogicalResult Importer::scanNominals() {
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto node = source.module.item("body", i);
    if (node.kind() != "ClassDef")
      continue;
    bool enumeration = hasDecorator(node, "encoding");
    for (size_t j = 0; j < node.array("bases").size(); ++j)
      enumeration |=
          resolvesMarker(node.item("bases", j), MarkerKind::Enum,
                         [&](const AstNode &n) { return resolveBinding(n); });
    bool record = hasDecorator(node, "struct");
    if (!enumeration && !record)
      continue;
    StringRef name = node.string("name");
    if (names.contains(name))
      return error() << "nominal declaration shadows an existing binding '"
                     << name << "'";
    auto symbol = FlatSymbolRefAttr::get(b.getContext(), qualify(name));
    if (enumeration) {
      auto syntax = readEnumSyntax(
          node, [&](const AstNode &n) { return resolveBinding(n); }, error);
      if (failed(syntax))
        return failure();
      enums[name] = std::move(*syntax);
    } else {
      if (!node.array("bases").empty() || !node.array("keywords").empty())
        return error() << "struct requires a unique name and no inheritance";
      structs[name] = node;
    }
    names[name] = {symbol,
                   nullptr,
                   node,
                   {},
                   name.str(),
                   false,
                   enumeration ? BindingCategory::Enum
                               : BindingCategory::Struct};
  }
  return success();
}

LogicalResult Importer::stageImports() {
  llvm::DenseSet<Attribute> cloned;
  auto retain = [&](Operation *declaration) {
    auto name = SymbolTable::getSymbolName(declaration);
    auto symbol = FlatSymbolRefAttr::get(b.getContext(), name.getValue());
    if (!cloned.insert(symbol).second)
      return;
    Operation *canonical = headers.lookupDeclaration(symbol);
    Operation *copy = (canonical ? canonical : declaration)->clone();
    copy->setAttr("ac.declaration_role", b.getStringAttr("import_snapshot"));
    body->getBody()->push_back(copy);
  };
  for (auto &entry : names)
    if (entry.second.declaration &&
        (entry.second.provider || entry.second.builtin))
      retain(entry.second.declaration);
  for (auto header : headers.suppliedHeaders()) {
    auto provider = header->getAttrOfType<DictionaryAttr>("ac.source_owner");
    if (!llvm::is_contained(dependencies, provider))
      continue;
    for (Operation &declaration : header.getBody()->getOperations())
      if (isa<ac::ModuleImportOp, ac::StructOp, ac::EnumOp>(declaration))
        retain(&declaration);
  }
  return success();
}

LogicalResult Importer::stageEnums() {
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto node = source.module.item("body", i);
    auto found = enums.find(node.string("name"));
    if (node.kind() != "ClassDef" || found == enums.end())
      continue;
    const auto &syntax = found->second;
    auto symbol = names[found->first()].symbol;
    auto integer = [&](const AstNode &value) -> FailureOr<ac::MathIntAttr> {
      auto expression = staticExpr(value, symbol);
      if (failed(expression))
        return failure();
      ac::HardwareAnalysis analysis(*body);
      if (!analysis.isStaticEvaluable(*expression, {}))
        return error()
               << "enum width and codes require closed Integer expressions";
      auto raw = analysis.evaluateStatic(*expression, {}, body->getOperation());
      if (failed(raw))
        return failure();
      auto result = dyn_cast<ac::MathIntAttr>(*raw);
      if (!result)
        return error() << "enum width and codes require mathematical Integer, "
                          "never Boolean";
      return result;
    };
    auto width = integer(syntax.width);
    if (failed(width))
      return failure();
    SmallVector<Attribute> members;
    for (auto [ordinal, member] : llvm::enumerate(syntax.members)) {
      ac::MathIntAttr code;
      if (!member.automatic) {
        auto value = integer(member.value);
        if (failed(value))
          return failure();
        code = *value;
      } else {
        llvm::APInt value(65, ordinal);
        if (syntax.encoding == "gray_sequential")
          value ^= value.lshr(1);
        if (syntax.encoding == "binary_one_hot") {
          if (ordinal >= std::numeric_limits<unsigned>::max() - 1)
            return error() << "enum derived code exceeds common "
                              "arbitrary-precision capacity";
          value =
              llvm::APInt(static_cast<unsigned>(ordinal) + 2, 1).shl(ordinal);
        }
        code = ac::MathIntAttr::get(b.getContext(), llvm::APSInt(value, true));
      }
      members.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("name", b.getStringAttr(member.name)),
           b.getNamedAttr("code", code)}));
    }
    b.setInsertionPointToEnd(body->getBody());
    auto declaration = createOp(
        b, node.location(b.getContext(), source.path),
        ac::EnumOp::getOperationName(), {}, {},
        {b.getNamedAttr("sym_name", b.getStringAttr(symbol.getValue())),
         b.getNamedAttr("width", *width),
         b.getNamedAttr("encoding", b.getStringAttr(syntax.encoding)),
         b.getNamedAttr("members", b.getArrayAttr(members)),
         b.getNamedAttr("ac.source_owner", owner),
         b.getNamedAttr("ac.origin", occurrence(b, symbol, node)),
         b.getNamedAttr("ac.declaration_role", b.getStringAttr("definition"))});
    if (failed(verify(declaration)))
      return failure();
    names[found->first()].declaration = declaration;
  }
  return success();
}

FailureOr<ac::StaticExprAttr>
Importer::staticExpr(const AstNode &node, FlatSymbolRefAttr ownerSymbol) {
  auto wrap = [&](StringRef kind, ArrayRef<NamedAttribute> payload) {
    SmallVector<NamedAttribute> fields{
        b.getNamedAttr("kind", b.getStringAttr(kind))};
    llvm::append_range(fields, payload);
    fields.push_back(
        b.getNamedAttr("origin", occurrence(b, ownerSymbol, node)));
    fields.push_back(
        b.getNamedAttr("location", sourceSpan(b, source.path, node)));
    return ac::StaticExprAttr::get(b.getContext(), b.getDictionaryAttr(fields));
  };
  if (node.kind() == "Constant") {
    DictionaryAttr value = staticValue(b, node.get("value"), error);
    if (!value)
      return error() << "static literal must be bool or integer";
    NamedAttribute field = b.getNamedAttr("value", value);
    return wrap("literal", ArrayRef<NamedAttribute>{field});
  }
  if (node.kind() == "Name") {
    DictionaryAttr ref = b.getDictionaryAttr(
        {b.getNamedAttr("kind", b.getStringAttr("parameter")),
         b.getNamedAttr("owner", ownerSymbol),
         b.getNamedAttr("name", b.getStringAttr(node.string("id")))});
    NamedAttribute field = b.getNamedAttr("ref", ref);
    return wrap("reference", ArrayRef<NamedAttribute>{field});
  }
  if (node.kind() == "BinOp") {
    StringRef form = node.child("op").kind();
    StringRef opcode = form == "Add"      ? "add"
                       : form == "Sub"    ? "sub"
                       : form == "Mult"   ? "mul"
                       : form == "LShift" ? "shl"
                       : form == "RShift" ? "shr"
                       : form == "BitAnd" ? "and_bits"
                       : form == "BitOr"  ? "or_bits"
                       : form == "BitXor" ? "xor_bits"
                                          : StringRef();
    if (opcode.empty())
      return error() << "unsupported static binary operator";
    auto lhs = staticExpr(node.child("left"), ownerSymbol);
    auto rhs = staticExpr(node.child("right"), ownerSymbol);
    if (failed(lhs) || failed(rhs))
      return failure();
    SmallVector<NamedAttribute> fields{
        b.getNamedAttr("operator", b.getStringAttr(opcode)),
        b.getNamedAttr("lhs", *lhs), b.getNamedAttr("rhs", *rhs)};
    return wrap("binary", fields);
  }
  return error() << "unsupported static expression '" << node.kind() << "'";
}

FailureOr<Type> Importer::portType(const AstNode &node,
                                   FlatSymbolRefAttr ownerSymbol) {
  auto binding = lookupBinding(node);
  if (binding && (binding->category == BindingCategory::Enum ||
                  binding->category == BindingCategory::Struct)) {
    auto nominal = resolveNominalType(
        node, [&](const AstNode &n) { return resolveBinding(n); }, error);
    if (failed(nominal))
      return failure();
    auto name = b.getStringAttr(nominal->getValue());
    return binding->category == BindingCategory::Enum
               ? Type(ac::EnumType::get(b.getContext(), name))
               : Type(ac::StructType::get(b.getContext(), name));
  }
  if (node.kind() == "Attribute" && binding &&
      binding->category == BindingCategory::Marker &&
      binding->importedModule == "pycircuit") {
    StringRef name = binding->remote;
    unsigned width = 0;
    if (name.consume_front("u") && !name.getAsInteger(10, width) && width &&
        width <= 64 && name == Twine(width).str())
      return bits(width, node, ownerSymbol);
  }
  if (node.kind() == "Subscript" && intrinsic(node.child("value"), "bits")) {
    auto width = staticExpr(node.child("slice"), ownerSymbol);
    if (failed(width))
      return failure();
    return Type(ac::BitsType::get(b.getContext(), *width));
  }
  if (node.kind() == "Subscript" && intrinsic(node.child("value"), "table")) {
    auto tuple = node.child("slice");
    if (tuple.kind() != "Tuple" || tuple.array("elts").size() != 2)
      return error() << "table requires one extent and a scalar element type";
    auto elementNode = tuple.item("elts", 1);
    if (!fixedAnnotation(elementNode) && !nominalAnnotation(elementNode))
      return error() << "table element requires explicitly unsigned bits or "
                        "nominal struct/enum";
    auto extent = staticExpr(tuple.item("elts", 0), ownerSymbol);
    auto element = portType(tuple.item("elts", 1), ownerSymbol);
    if (failed(extent) || failed(element))
      return failure();
    if (!isa<ac::BitsType, ac::StructType, ac::EnumType>(*element))
      return error() << "table element must be finite bits, struct or enum";
    return Type(ac::TableType::get(b.getContext(), b.getArrayAttr({*extent}),
                                   *element));
  }
  if (node.kind() == "Name" && node.string("id") == "bool") {
    auto one = parseStaticInteger(b, "1", error);
    if (failed(one))
      return failure();
    DictionaryAttr value =
        b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("integer")),
                             b.getNamedAttr("value", *one)});
    auto width = ac::StaticExprAttr::get(
        b.getContext(),
        b.getDictionaryAttr(
            {b.getNamedAttr("kind", b.getStringAttr("literal")),
             b.getNamedAttr("value", value),
             b.getNamedAttr("origin", occurrence(b, ownerSymbol, node)),
             b.getNamedAttr("location", sourceSpan(b, source.path, node))}));
    return Type(ac::BitsType::get(b.getContext(), width));
  }
  if (node.kind() != "Subscript" || node.child("value").kind() != "Name" ||
      node.child("value").string("id") != "Annotated")
    return error() << "ports require bool or Annotated[int, range(1 << WIDTH)]";
  AstNode tuple = node.child("slice");
  if (tuple.kind() != "Tuple" || tuple.array("elts").size() != 2 ||
      tuple.item("elts", 0).kind() != "Name" ||
      tuple.item("elts", 0).string("id") != "int")
    return error() << "Annotated port requires int and one range";
  AstNode range = tuple.item("elts", 1);
  if (range.kind() != "Call" || range.child("func").kind() != "Name" ||
      range.child("func").string("id") != "range" ||
      range.array("args").size() != 1)
    return error() << "integer port requires range(1 << WIDTH)";
  AstNode bound = range.item("args", 0);
  if (bound.kind() != "BinOp" || bound.child("op").kind() != "LShift")
    return error() << "integer port range must be a power of two";
  auto lhs = dyn_cast_or_null<DictionaryAttr>(bound.child("left").get("value"));
  if (bound.child("left").kind() != "Constant" || !lhs ||
      lhs.getAs<StringAttr>("integer").getValue() != "1")
    return error() << "integer port range must start with one";
  auto width = staticExpr(bound.child("right"), ownerSymbol);
  if (failed(width))
    return failure();
  return Type(ac::BitsType::get(b.getContext(), *width));
}

FailureOr<ModuleDecl> Importer::signature(const AstNode &node) {
  ModuleDecl result;
  result.system = hasDecorator(node, "system");
  result.node = node;
  result.symbol =
      FlatSymbolRefAttr::get(b.getContext(), qualify(node.string("name")));
  AstNode args = node.child("args");
  if (result.system &&
      (!args.array("posonlyargs").empty() || !args.array("args").empty() ||
       !args.array("kwonlyargs").empty() || args.child("vararg") ||
       args.child("kwarg") || !args.array("defaults").empty() ||
       !args.array("kw_defaults").empty()))
    return error() << "@system is closed and cannot declare authored inputs or "
                      "static parameters";
  if (!args.array("posonlyargs").empty() || args.child("vararg") ||
      args.child("kwarg") || !args.array("defaults").empty())
    return error() << "module inputs are required ordinary arguments";
  llvm::StringSet<> seen;
  for (size_t i = 0; i < args.array("args").size(); ++i) {
    AstNode arg = args.item("args", i);
    if (names.contains(arg.string("arg")) ||
        behavioralRules.contains(arg.string("arg")))
      return error()
             << "module input shadows a source declaration or namespace";
    if (!seen.insert(arg.string("arg")).second || !arg.child("annotation"))
      return error() << "module input requires a unique name and annotation";
    auto type = portType(arg.child("annotation"), result.symbol);
    if (failed(type))
      return failure();
    result.inputs.push_back({arg.string("arg").str(), *type, arg,
                             annotationKind(arg.child("annotation")),
                             fixedAnnotation(arg.child("annotation"))});
  }
  if (args.array("kwonlyargs").size() != args.array("kw_defaults").size())
    return error() << "malformed keyword-only static parameters";
  for (size_t i = 0; i < args.array("kwonlyargs").size(); ++i) {
    AstNode arg = args.item("kwonlyargs", i);
    AstNode type = arg.child("annotation");
    if (names.contains(arg.string("arg")))
      return error()
             << "static parameter shadows a source declaration or namespace";
    if (!seen.insert(arg.string("arg")).second || type.kind() != "Name" ||
        type.string("id") != "int")
      return error() << "static parameters require unique keyword-only int "
                        "declarations";
    AstNode value = args.item("kw_defaults", i);
    result.parameters.push_back(
        {arg.string("arg").str(), arg,
         value ? std::optional<AstNode>(value) : std::nullopt});
  }
  AstNode returns = node.child("returns");
  if (result.system && returns)
    return error() << "@system cannot declare a result";
  if (!result.system && !returns)
    return error() << "module requires a return annotation";
  std::optional<Import> returnBinding;
  if (returns)
    returnBinding = lookupBinding(returns);
  if (result.system) {
    result.returnShape = ReturnShape::Mapping;
  } else if (returnBinding &&
             returnBinding->category == BindingCategory::Struct) {
    auto type = portType(returns, result.symbol);
    if (failed(type))
      return failure();
    std::string resultName = "result";
    unsigned ordinal = 1;
    while (seen.contains(resultName))
      resultName = "result_" + std::to_string(ordinal++);
    seen.insert(resultName);
    result.returnShape = ReturnShape::Struct;
    result.outputs.push_back({resultName, *type, returns, {}, false});
  } else {
    if (returns.kind() != "Dict" ||
        returns.array("keys").size() != returns.array("values").size())
      return error() << "module return annotation requires a named mapping or "
                        "nominal struct/enum";
    for (size_t i = 0; i < returns.array("keys").size(); ++i) {
      AstNode key = returns.item("keys", i);
      auto name = key.kind() == "Constant"
                      ? dyn_cast_or_null<StringAttr>(key.get("value"))
                      : StringAttr();
      if (!name || !seen.insert(name.getValue()).second)
        return error() << "output names must be unique strings";
      auto type = portType(returns.item("values", i), result.symbol);
      if (failed(type))
        return failure();
      result.outputs.push_back({name.getValue().str(), *type,
                                returns.item("values", i),
                                annotationKind(returns.item("values", i)),
                                fixedAnnotation(returns.item("values", i))});
    }
  }
  for (size_t i = 0; i < node.array("body").size(); ++i) {
    auto stmt = node.item("body", i);
    if (stmt.kind() == "AnnAssign" ||
        (stmt.kind() == "Assign" && stmt.child("value").kind() == "Call" &&
         stmt.child("value").child("func").kind() == "Subscript" &&
         intrinsic(stmt.child("value").child("func").child("value"), "table")))
      result.persistent = true;
  }
  result.sourceInputCount = result.inputs.size();
  result.behavioral =
      result.persistent || result.returnShape == ReturnShape::Struct;
  for (size_t i = 0; i < node.array("body").size(); ++i) {
    auto stmt = node.item("body", i);
    auto call = stmt.child("value");
    if (stmt.kind() == "Return" && call.kind() == "Call" &&
        call.child("func").kind() == "Name" &&
        behavioralRules.contains(call.child("func").string("id")))
      result.behavioral = true;
  }
  SmallVector<Type> inputs, outputs;
  for (const Port &port : result.inputs)
    inputs.push_back(port.type);
  for (const Port &port : result.outputs)
    outputs.push_back(port.type);
  result.type = FunctionType::get(b.getContext(), inputs, outputs);
  return result;
}

LogicalResult Importer::scanModules() {
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto stmt = source.module.item("body", i);
    if (stmt.kind() == "FunctionDef" && hasDecorator(stmt, "rule")) {
      if (behavioralRules.contains(stmt.string("name")) ||
          names.contains(stmt.string("name")) ||
          structs.contains(stmt.string("name")) ||
          names.contains(stmt.string("name")))
        return error() << "duplicate rule name";
      behavioralRules[stmt.string("name")] = stmt;
    }
  }
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode stmt = source.module.item("body", i);
    if (docstring(stmt) || stmt.kind() == "ImportFrom" ||
        stmt.kind() == "Import" ||
        (stmt.kind() == "ClassDef" && (structs.contains(stmt.string("name")) ||
                                       enums.contains(stmt.string("name")))))
      continue;
    if (stmt.kind() == "FunctionDef" && hasDecorator(stmt, "rule")) {
      continue;
    }
    if (stmt.kind() != "FunctionDef" ||
        (!hasDecorator(stmt, "module") && !hasDecorator(stmt, "system")))
      return error()
             << "source top level supports imports, @module, and @system "
                "definitions";
    auto parsed = signature(stmt);
    if (failed(parsed))
      return failure();
    StringRef name = stmt.string("name");
    if (names.contains(name) || local.contains(name) ||
        structs.contains(name) || behavioralRules.contains(name) ||
        names.contains(name))
      return error() << "module name is duplicated";
    local[name] = modules.size();
    names[name] = {parsed->symbol, nullptr, stmt, {}, name.str(), false};
    modules.push_back(std::move(*parsed));
  }
  return success();
}

ArrayAttr portNames(OpBuilder &b, ArrayRef<Port> ports) {
  SmallVector<Attribute> result;
  for (const Port &port : ports)
    result.push_back(b.getStringAttr(port.name));
  return b.getArrayAttr(result);
}

FailureOr<SmallVector<Attribute>>
Importer::bindParameters(Operation *callee, const AstNode &call,
                         FlatSymbolRefAttr parent) {
  ArrayAttr formals = parameters(callee);
  SmallVector<std::optional<Attribute>> values(formals.size());
  if (!call.array("args").empty())
    return error() << "instance parameters are keyword-only";
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    AstNode kw = call.item("keywords", i);
    auto name = dyn_cast_or_null<StringAttr>(kw.get("arg"));
    auto typeFormals = callee->getAttrOfType<ArrayAttr>("type_parameters");
    if (name && typeFormals && llvm::is_contained(typeFormals, name))
      continue;
    size_t ordinal = formals.size();
    for (size_t j = 0; name && j < formals.size(); ++j)
      if (cast<DictionaryAttr>(formals[j])
              .getAs<StringAttr>("name")
              .getValue() == name.getValue())
        ordinal = j;
    if (ordinal == formals.size() || values[ordinal])
      return error() << "unknown or repeated instance parameter";
    auto value = staticExpr(kw.child("value"), parent);
    if (failed(value))
      return failure();
    values[ordinal] = *value;
  }
  SmallVector<Attribute> result;
  for (size_t i = 0; i < formals.size(); ++i) {
    if (values[i])
      result.push_back(*values[i]);
    else {
      auto value =
          cast<DictionaryAttr>(formals[i]).getAs<ac::StaticExprAttr>("default");
      if (!value)
        return error() << "missing required instance parameter";
      result.push_back(value);
    }
  }
  return result;
}

FailureOr<SmallVector<Attribute>>
Importer::bindTypeArguments(Operation *callee, const AstNode &call,
                            FlatSymbolRefAttr parent) {
  auto formals = callee->getAttrOfType<ArrayAttr>("type_parameters");
  if (!formals)
    return error() << "callee has no type parameter declaration";
  SmallVector<Attribute> values(formals.size());
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    AstNode keyword = call.item("keywords", i);
    auto name = dyn_cast_or_null<StringAttr>(keyword.get("arg"));
    for (size_t j = 0; name && j < formals.size(); ++j) {
      if (formals[j] != name)
        continue;
      if (values[j])
        return error() << "repeated instance type argument";
      auto type = portType(keyword.child("value"), parent);
      if (failed(type))
        return failure();
      values[j] = TypeAttr::get(*type);
    }
  }
  for (Attribute value : values)
    if (!value)
      return error() << "missing required instance type argument";
  return values;
}

FailureOr<Type> Importer::substitute(Type type, Operation *callee,
                                     ArrayRef<Attribute> actuals,
                                     ArrayRef<Attribute> typeActuals) {
  if (auto parameter = dyn_cast<ac::TypeParamType>(type)) {
    auto formals = callee->getAttrOfType<ArrayAttr>("type_parameters");
    for (auto [index, formal] : llvm::enumerate(formals))
      if (formal == parameter.getName() && index < typeActuals.size())
        return cast<TypeAttr>(typeActuals[index]).getValue();
    return error() << "unbound instance type parameter";
  }
  auto bits = dyn_cast<ac::BitsType>(type);
  if (!bits)
    return type;
  ArrayAttr formals = parameters(callee);
  llvm::StringMap<ac::StaticExprAttr> replacements;
  for (size_t i = 0; i < formals.size(); ++i)
    replacements[cast<DictionaryAttr>(formals[i])
                     .getAs<StringAttr>("name")
                     .getValue()] = cast<ac::StaticExprAttr>(actuals[i]);
  auto rewrite = [&](auto &&self, Attribute value) -> Attribute {
    if (auto expression = dyn_cast<ac::StaticExprAttr>(value)) {
      DictionaryAttr tree = expression.getTree();
      auto kind = tree.getAs<StringAttr>("kind");
      auto ref = tree.getAs<DictionaryAttr>("ref");
      if (kind && kind.getValue() == "reference" && ref &&
          ref.getAs<StringAttr>("kind") &&
          ref.getAs<StringAttr>("kind").getValue() == "parameter") {
        auto found =
            replacements.find(ref.getAs<StringAttr>("name").getValue());
        if (found != replacements.end())
          return found->second;
      }
      SmallVector<NamedAttribute> fields;
      for (NamedAttribute field : tree)
        fields.push_back(
            b.getNamedAttr(field.getName(), self(self, field.getValue())));
      return ac::StaticExprAttr::get(b.getContext(),
                                     b.getDictionaryAttr(fields));
    }
    if (auto typeAttr = dyn_cast<TypeAttr>(value)) {
      if (auto parameter = dyn_cast<ac::TypeParamType>(typeAttr.getValue())) {
        auto formals = callee->getAttrOfType<ArrayAttr>("type_parameters");
        for (auto [index, formal] : llvm::enumerate(formals))
          if (formal == parameter.getName() && index < typeActuals.size())
            return typeActuals[index];
      }
      return value;
    }
    if (auto dictionary = dyn_cast<DictionaryAttr>(value)) {
      SmallVector<NamedAttribute> fields;
      for (NamedAttribute field : dictionary)
        fields.push_back(
            b.getNamedAttr(field.getName(), self(self, field.getValue())));
      return b.getDictionaryAttr(fields);
    }
    if (auto array = dyn_cast<ArrayAttr>(value)) {
      SmallVector<Attribute> elements;
      for (Attribute element : array)
        elements.push_back(self(self, element));
      return b.getArrayAttr(elements);
    }
    return value;
  };
  return Type(ac::BitsType::get(
      b.getContext(),
      cast<ac::StaticExprAttr>(rewrite(rewrite, bits.getWidth()))));
}

FailureOr<Value>
Importer::fixedUnsignedShift(Value input, const llvm::APSInt &count,
                             StringRef opcode, const AstNode &node,
                             OpBuilder &at, FlatSymbolRefAttr symbol) {
  auto loc = node.location(b.getContext(), source.path);
  if (!isa<ac::BitsType>(input.getType()) || !fixedValues.contains(input) ||
      valueInfo(input).sourceKind == ac::detail::ValueKind::Boolean)
    return mlir::emitError(loc)
           << "unsigned shift requires explicitly unsigned fixed bits";

  ac::HardwareAnalysis analysis(*body);
  auto width = analysis.getPackedWidth(input.getType(), {},
                                       at.getInsertionBlock()->getParentOp());
  if (failed(width))
    return failure();
  if (!*width)
    return mlir::emitError(loc)
           << "unsigned shift requires a positive resolved input width";

  // Bound the full static count before converting or encoding it. Keeping the
  // original input in the binary operation preserves validation and dependency
  // checking even when every bit is shifted out.
  uint64_t amount =
      llvm::APSInt::compareValues(count, llvm::APSInt::getUnsigned(*width)) >= 0
          ? *width
          : count.getZExtValue();
  auto knownCount =
      createOp(at, loc, ac::BitsConstantOp::getOperationName(), {},
               TypeRange{input.getType()},
               {at.getNamedAttr("value", literal(amount, node, symbol))})
          ->getResult(0);
  auto result = createOp(at, loc, ac::BitsBinaryOp::getOperationName(),
                         {input, knownCount}, TypeRange{input.getType()},
                         {at.getNamedAttr("opcode", at.getStringAttr(opcode))})
                    ->getResult(0);
  fixedValues.insert(result);
  return result;
}

FailureOr<Value> Importer::expression(
    const AstNode &node, OpBuilder &at, FlatSymbolRefAttr ownerSymbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    std::optional<Type> expected,
    llvm::StringMap<SmallVector<Value>> *captures) {
  if (!captures && activeRule && activeRule->outputs)
    captures = activeRule->outputs;
  using ac::detail::ValueKind;
  using ac::detail::ValueOpcode;
  auto error = [&] {
    return mlir::emitError(node.location(b.getContext(), source.path));
  };
  auto staticOnly = [&](auto &&self, const AstNode &candidate) -> bool {
    if (candidate.kind() == "Constant")
      return static_cast<bool>(
          dyn_cast_or_null<DictionaryAttr>(candidate.get("value")));
    if (candidate.kind() == "Name") {
      for (const ModuleDecl &module : modules)
        if (module.symbol == ownerSymbol)
          return llvm::any_of(module.parameters,
                              [&](const Parameter &parameter) {
                                return parameter.name == candidate.string("id");
                              });
      return false;
    }
    return candidate.kind() == "BinOp" && self(self, candidate.child("left")) &&
           self(self, candidate.child("right"));
  };
  auto makeWidth = [&](unsigned width) {
    auto literal = at.getDictionaryAttr(
        {at.getNamedAttr("kind", at.getStringAttr("integer")),
         at.getNamedAttr(
             "value", ac::MathIntAttr::get(at.getContext(),
                                           llvm::APSInt::getUnsigned(width)))});
    auto site = numericSite(node, ownerSymbol);
    return ac::BitsType::get(
        at.getContext(),
        ac::StaticExprAttr::get(
            at.getContext(),
            at.getDictionaryAttr(
                {at.getNamedAttr("kind", at.getStringAttr("literal")),
                 at.getNamedAttr("value", literal),
                 at.getNamedAttr("origin", site.origin),
                 at.getNamedAttr("location", site.sourceSpan)})));
  };
  auto track = [&](Value value, std::optional<ValueKind> kind,
                   std::optional<IntegerInterval> interval =
                       std::nullopt) -> Value {
    remember(value, kind, std::move(interval));
    return value;
  };
  auto needsMath = [&](Value value) {
    auto info = valueInfo(value);
    return arithmeticValues.contains(value) ||
           (info.interval && info.interval->lower.isNegative());
  };
  auto literalWidth = [&](Type type) {
    auto bits = dyn_cast<ac::BitsType>(type);
    auto tag = bits ? bits.getWidth().getTree().getAs<StringAttr>("kind")
                    : StringAttr();
    return tag && tag.getValue() == "literal";
  };
  auto provenStatic =
      [&](const AstNode &candidate) -> std::optional<llvm::APSInt> {
    if (!staticOnly(staticOnly, candidate))
      return std::nullopt;
    auto expression = staticExpr(candidate, ownerSymbol);
    if (failed(expression))
      return std::nullopt;
    ac::HardwareAnalysis analysis(*body);
    if (!analysis.isStaticEvaluable(*expression, {}))
      return std::nullopt;
    auto value = analysis.evaluateStatic(*expression, {},
                                         at.getInsertionBlock()->getParentOp());
    if (failed(value))
      return std::nullopt;
    auto integer = dyn_cast<ac::MathIntAttr>(*value);
    return integer ? std::optional<llvm::APSInt>(
                         llvm::APSInt(integer.getCanonicalValue()))
                   : std::nullopt;
  };
  auto provenMask = [&](Value value) {
    auto info = valueInfo(value);
    auto constant = value.getDefiningOp<ac::BitsConstantOp>();
    if (!constant || info.sourceKind != ValueKind::Integer || !info.interval ||
        info.interval->lower.isNegative())
      return false;
    ac::HardwareAnalysis analysis(*body);
    if (!analysis.isStaticEvaluable(constant.getValue(), {}))
      return false;
    auto result = analysis.evaluateStatic(constant.getValue(), {}, constant);
    auto integer = succeeded(result) ? dyn_cast<ac::MathIntAttr>(*result)
                                     : ac::MathIntAttr();
    return integer && !llvm::APSInt(integer.getCanonicalValue()).isNegative();
  };
  auto binary = [&](StringRef opcode, Value left, Value right) -> Value {
    return createOp(at, node.location(b.getContext(), source.path),
                    ac::BitsBinaryOp::getOperationName(), {left, right},
                    TypeRange{left.getType()},
                    {at.getNamedAttr("opcode", at.getStringAttr(opcode))})
        ->getResult(0);
  };
  if (queueCall(node))
    return error() << "queue allocation requires a direct module-body "
                      "three-name Tuple assignment";
  if (moduleCall(node))
    return directModuleCall(node, at, ownerSymbol, values, instances);
#include "PythonImportExpressions.inc"
  if (staticOnly(staticOnly, node) &&
      (!expected || isa<ac::BitsType>(*expected))) {
    auto expression = staticExpr(node, ownerSymbol);
    if (failed(expression))
      return failure();
    ac::HardwareAnalysis analysis(*body);
    std::optional<llvm::APSInt> known;
    if (analysis.isStaticEvaluable(*expression, {})) {
      auto evaluated = analysis.evaluateStatic(
          *expression, {}, at.getInsertionBlock()->getParentOp());
      if (failed(evaluated))
        return failure();
      if (auto integer = dyn_cast<ac::MathIntAttr>(*evaluated))
        known = llvm::APSInt(integer.getCanonicalValue());
    }
    Type type;
    if (expected && !known)
      type = *expected;
    if (expected && known && !known->isNegative()) {
      auto hint = cast<ac::BitsType>(*expected);
      if (!analysis.isStaticEvaluable(hint.getWidth(), {})) {
        // An unresolved parameter remains contextual; its default is not a
        // proof for all bindings. Mathematical consumers require later proof.
        type = *expected;
      } else {
        auto resolved = analysis.evaluateStatic(
            hint.getWidth(), {}, at.getInsertionBlock()->getParentOp());
        if (failed(resolved))
          return failure();
        auto width = dyn_cast<ac::MathIntAttr>(*resolved);
        if (!width)
          return error() << "integer literal hint requires an integer width";
        llvm::APSInt amount(width.getCanonicalValue());
        if (amount.isNegative() || amount.isZero() ||
            amount.getActiveBits() > std::numeric_limits<unsigned>::digits)
          return error()
                 << "integer literal hint requires a legal finite width";
        if (known->getActiveBits() <= amount.getZExtValue())
          type = *expected;
      }
    }
    if (!type && known) {
      uint64_t width = known->isNegative()
                           ? known->getSignificantBits()
                           : std::max(1u, known->getActiveBits());
      if (width > std::numeric_limits<unsigned>::max())
        return error()
               << "literal width exceeds compiler representation capacity";
      type = makeWidth(static_cast<unsigned>(width));
    } else if (!type)
      return error()
             << "static integer width is unproved without concrete bindings";
    if (known && known->isNegative()) {
      auto width = llvm::APSInt(cast<ac::BitsType>(type)
                                    .getWidth()
                                    .getTree()
                                    .getAs<DictionaryAttr>("value")
                                    .getAs<ac::MathIntAttr>("value")
                                    .getCanonicalValue())
                       .getZExtValue();
      auto encoded =
          llvm::APSInt(known->sextOrTrunc(static_cast<unsigned>(width)), true);
      auto site = numericSite(node, ownerSymbol);
      expression = ac::StaticExprAttr::get(
          at.getContext(),
          at.getDictionaryAttr(
              {at.getNamedAttr("kind", at.getStringAttr("literal")),
               at.getNamedAttr(
                   "value",
                   at.getDictionaryAttr(
                       {at.getNamedAttr("kind", at.getStringAttr("integer")),
                        at.getNamedAttr(
                            "value",
                            ac::MathIntAttr::get(at.getContext(), encoded))})),
               at.getNamedAttr("origin", site.origin),
               at.getNamedAttr("location", site.sourceSpan)}));
    }
    Value value =
        createOp(at, node.location(b.getContext(), source.path),
                 ac::BitsConstantOp::getOperationName(), {}, TypeRange{type},
                 {at.getNamedAttr("value", *expression)})
            ->getResult(0);
    std::optional<IntegerInterval> interval;
    if (known) {
      auto upper = ac::detail::evaluateValue(
          ValueOpcode::Add,
          {ac::MathIntAttr::get(at.getContext(), *known),
           ac::MathIntAttr::get(at.getContext(), llvm::APSInt::getUnsigned(1))},
          error);
      if (failed(upper))
        return failure();
      interval = IntegerInterval{
          *known,
          llvm::APSInt(cast<ac::MathIntAttr>(*upper).getCanonicalValue())};
    }
    if (!known) {
      numericValues[value] = {value, ValueKind::Integer, std::nullopt};
      return value;
    }
    remember(value, ValueKind::Integer, interval,
             ac::MathIntAttr::get(at.getContext(), *known));
    return value;
  }
  if (node.kind() == "Name") {
    if (node.string("id") == "pyc_clk" || node.string("id") == "pyc_rst")
      for (auto &decl : modules)
        if (decl.symbol == ownerSymbol && decl.needsDomain)
          return error() << "generated physical domain pin '"
                         << node.string("id") << "' is not a source value";
    auto found = values.find(node.string("id"));
    if (found == values.end() && activeBehavioralDecl &&
        activeBehavioralDecl->symbol == ownerSymbol &&
        isa<ac::ModuleOp>(at.getInsertionBlock()->getParentOp()) &&
        forwardCallBindings.contains(node.string("id")))
      return forwardModuleValue(node.string("id"), ownerSymbol);
    if (found == values.end())
      return error() << "unknown hardware value '" << node.string("id") << "'";
    return found->second;
  }
  if (node.kind() == "Attribute" && node.child("value").kind() == "Name") {
    StringRef name = node.child("value").string("id");
    auto found = instances.find(name);
    if (found == instances.end())
      return error() << "unknown instance output";
    for (size_t i = 0; i < found->second->outputs.size(); ++i)
      if (found->second->outputs[i].name == node.string("attr"))
        return captures ? (*captures)[name][i]
                        : found->second->op->getResult(i);
    return error() << "unknown instance output '" << node.string("attr") << "'";
  }
  if (node.kind() == "Constant") {
    if (isa<BoolAttr>(node.get("value")))
      expected = makeWidth(1);
    if (!expected || !isa<ac::BitsType>(*expected))
      return error() << "hardware literal width is ambiguous";
    auto expression = staticExpr(node, ownerSymbol);
    if (failed(expression))
      return failure();
    if (auto boolean = dyn_cast_or_null<BoolAttr>(node.get("value"))) {
      auto integer =
          parseStaticInteger(b, boolean.getValue() ? "1" : "0", error);
      if (failed(integer))
        return failure();
      NamedAttrList tree(expression->getTree());
      tree.set("value", b.getDictionaryAttr(
                            {b.getNamedAttr("kind", b.getStringAttr("integer")),
                             b.getNamedAttr("value", *integer)}));
      expression = ac::StaticExprAttr::get(b.getContext(),
                                           tree.getDictionary(b.getContext()));
    }
    Value value =
        createOp(at, node.location(b.getContext(), source.path),
                 ac::BitsConstantOp::getOperationName(), {},
                 TypeRange{*expected}, {at.getNamedAttr("value", *expression)})
            ->getResult(0);
    remember(value, ValueKind::Boolean, {}, node.get("value"));
    return value;
  }
  if (node.kind() == "BinOp") {
    StringRef form = node.child("op").kind();
    if (form == "FloorDiv" || form == "Mod") {
      auto input = expression(node.child("left"), at, ownerSymbol, values,
                              instances, {}, captures);
      if (failed(input))
        return failure();
      if (!fixedValues.contains(*input))
        return error() << "source operator requires supported exact integer or "
                          "width-preserving bitwise lowering";
      auto divisor = provenStatic(node.child("right"));
      ac::HardwareAnalysis analysis(*body);
      if (!divisor) {
        auto rhs = expression(node.child("right"), at, ownerSymbol, values,
                              instances, {}, captures);
        if (failed(rhs))
          return failure();
        if (!fixedValues.contains(*rhs) ||
            !isa<ac::BitsType>(input->getType()) ||
            !isa<ac::BitsType>(rhs->getType()))
          return error() << "runtime unsigned division/remainder requires "
                            "authoritative fixed Bits operands";
        Operation *site = at.getBlock()->getParentOp();
        auto inputWidth = analysis.getPackedWidth(input->getType(), {}, site);
        auto rhsWidth = analysis.getPackedWidth(rhs->getType(), {}, site);
        if (failed(inputWidth) || failed(rhsWidth))
          return failure();
        if (!*inputWidth || *inputWidth != *rhsWidth)
          return error() << "runtime unsigned division/remainder requires "
                            "the same positive operand width";
        auto equivalent =
            equivalentBoundaryTypes(rhs->getType(), input->getType(), site);
        if (failed(equivalent))
          return failure();
        if (!*equivalent)
          return error() << "runtime unsigned division/remainder operand "
                            "hardware types differ";
        Value rhsValue = *rhs;
        if (!ac::areEquivalentHardwareTypes(rhsValue.getType(), input->getType()))
          rhsValue = boundaryBitsIdentity(rhsValue, input->getType(),
                                          node.child("right"), at, ownerSymbol);
        Value result =
            createOp(at, node.location(b.getContext(), source.path),
                     ac::BitsBinaryOp::getOperationName(), {*input, rhsValue},
                     TypeRange{input->getType()},
                     {at.getNamedAttr("opcode", at.getStringAttr(
                         form == "FloorDiv" ? "udiv" : "urem"))})
                ->getResult(0);
        fixedValues.insert(result);
        return result;
      }
      auto result = lowerFixedUnsignedDivRem(
          at, analysis, numericSite(node, ownerSymbol), valueInfo(*input),
          fixedValues.contains(*input), *divisor,
          form == "FloorDiv" ? FixedUnsignedDivRemResult::Quotient
                             : FixedUnsignedDivRemResult::Remainder);
      if (failed(result))
        return failure();
      fixedValues.insert(*result);
      if (form == "Mod")
        remember(*result, std::nullopt,
                 IntegerInterval{llvm::APSInt::getUnsigned(0), *divisor});
      return *result;
    }
    if (form == "RShift" || form == "LShift") {
      FailureOr<Value> input = failure();
      if (form == "LShift") {
        input = expression(node.child("left"), at, ownerSymbol, values,
                           instances, {}, captures);
        if (failed(input))
          return failure();
        if (!fixedValues.contains(*input))
          return error()
                 << "source operator requires supported exact integer or "
                    "width-preserving bitwise lowering";
      }
      auto count = provenStatic(node.child("right"));
      if (!count)
        return error()
               << (form == "RShift"
                       ? "integer right shift requires a proven static Integer "
                         "count"
                       : "unsigned shift requires a proven static Integer "
                         "count");
      if (count->isNegative())
        return error() << (form == "RShift"
                               ? "integer right-shift count must be nonnegative"
                               : "unsigned shift count must be nonnegative");
      if (form == "RShift")
        input = expression(node.child("left"), at, ownerSymbol, values,
                           instances, {}, captures);
      if (failed(input))
        return failure();
      if (fixedValues.contains(*input))
        return fixedUnsignedShift(*input, *count,
                                  form == "LShift" ? "shl" : "lshr", node, at,
                                  ownerSymbol);
      auto result = lowerExactIntegerShiftRight(
          at, numericSite(node, ownerSymbol), valueInfo(*input), *count);
      if (failed(result))
        return failure();
      numericValues[result->value] = *result;
      arithmeticValues.insert(result->value);
      // The count has its own existing static proof; do not give aliases new
      // admission in the count position.
      if (auto closed = valueInfo(*input).closedSourceConstant) {
        auto evaluated = ac::detail::evaluateValue(
            ValueOpcode::Shr,
            {closed, ac::MathIntAttr::get(at.getContext(), *count)}, error);
        if (failed(evaluated))
          return failure();
        if (failed(recordClosed(result->value, *evaluated, node)))
          return failure();
      }
      return result->value;
    }
    std::optional<ValueOpcode> arithmetic;
    if (form == "Add")
      arithmetic = ValueOpcode::Add;
    if (form == "Sub")
      arithmetic = ValueOpcode::Sub;
    if (form == "Mult")
      arithmetic = ValueOpcode::Mul;
    if (arithmetic) {
      for (const AstNode &operand : {node.child("left"), node.child("right")})
        if (operand.kind() == "Constant" && isa<BoolAttr>(operand.get("value")))
          return error()
                 << "mathematical arithmetic requires Integer operands; "
                    "Boolean literal cannot be implicitly converted";
      if (node.child("left").kind() == "Constant" &&
          node.child("right").kind() != "Constant") {
        auto right = expression(node.child("right"), at, ownerSymbol, values,
                                instances, {}, captures);
        if (failed(right))
          return failure();
        if (fixedValues.contains(*right)) {
          auto left = expression(node.child("left"), at, ownerSymbol, values,
                                 instances, right->getType(), captures);
          if (failed(left) || !ac::areEquivalentHardwareTypes(left->getType(),
                                                              right->getType()))
            return error() << "unsigned arithmetic operand width mismatch";
          auto result = binary(form == "Add"   ? "add"
                               : form == "Sub" ? "sub"
                                               : "mul",
                               *left, *right);
          fixedValues.insert(result);
          return result;
        }
      }
      auto lhs = expression(node.child("left"), at, ownerSymbol, values,
                            instances, {}, captures);
      if (succeeded(lhs) && fixedValues.contains(*lhs) &&
          isa<ac::BitsType>(lhs->getType())) {
        auto rhs = expression(node.child("right"), at, ownerSymbol, values,
                              instances, lhs->getType(), captures);
        if (failed(rhs))
          return failure();
        if (!ac::areEquivalentHardwareTypes(lhs->getType(), rhs->getType()))
          return error() << "unsigned arithmetic operands require equal widths";
        if (!fixedValues.contains(*rhs) &&
            node.child("right").kind() != "Constant")
          return error() << "unsigned arithmetic requires explicitly typed "
                            "bits operands";
        auto result = binary(form == "Add"   ? "add"
                             : form == "Sub" ? "sub"
                                             : "mul",
                             *lhs, *rhs);
        fixedValues.insert(result);
        return result;
      }
      auto rhs = expression(node.child("right"), at, ownerSymbol, values,
                            instances, {}, captures);
      if (failed(lhs) || failed(rhs))
        return failure();
      if (isa<ac::EnumType>(lhs->getType()) ||
          isa<ac::EnumType>(rhs->getType()))
        return error() << "Enum arithmetic requires explicit enum_to_bits";
      auto result = lowerExactIntegerBinary(at, numericSite(node, ownerSymbol),
                                            *arithmetic, valueInfo(*lhs),
                                            valueInfo(*rhs));
      if (failed(result))
        return failure();
      numericValues[result->value] = *result;
      arithmeticValues.insert(result->value);
      if (failed(
              propagateClosed(result->value, *arithmetic, {*lhs, *rhs}, node)))
        return failure();
      return result->value;
    }
    StringRef opcode = form == "BitAnd"   ? "and"
                       : form == "BitOr"  ? "or"
                       : form == "BitXor" ? "xor"
                                          : StringRef();
    if (opcode.empty())
      return error() << "source operator requires supported exact integer or "
                        "width-preserving bitwise lowering";
    FailureOr<Value> lhs = failure(), rhs = failure();
    auto leftMask = form == "BitAnd" ? provenStatic(node.child("left"))
                                     : std::optional<llvm::APSInt>();
    if ((leftMask && !leftMask->isNegative()) ||
        (node.child("left").kind() == "Constant" && !expected)) {
      rhs = expression(node.child("right"), at, ownerSymbol, values, instances,
                       leftMask ? expected : std::optional<Type>(), captures);
      if (failed(rhs))
        return failure();
      lhs =
          expression(node.child("left"), at, ownerSymbol, values, instances,
                     form == "BitAnd" &&
                             valueInfo(*rhs).sourceKind == ValueKind::Integer &&
                             literalWidth(rhs->getType()) && leftMask &&
                             !leftMask->isNegative()
                         ? std::optional<Type>()
                         : std::optional<Type>(rhs->getType()),
                     captures);
    } else {
      lhs = expression(node.child("left"), at, ownerSymbol, values, instances,
                       expected, captures);
      if (failed(lhs))
        return failure();
      std::optional<Type> hint = lhs->getType();
      auto mask = form == "BitAnd" ? provenStatic(node.child("right"))
                                   : std::optional<llvm::APSInt>();
      if (form == "BitAnd" &&
          valueInfo(*lhs).sourceKind == ValueKind::Integer &&
          literalWidth(lhs->getType()) && mask && !mask->isNegative())
        hint.reset();
      rhs = expression(node.child("right"), at, ownerSymbol, values, instances,
                       hint, captures);
    }
    if (failed(lhs) || failed(rhs))
      return failure();
    if (isa<ac::EnumType>(lhs->getType()) || isa<ac::EnumType>(rhs->getType()))
      return error() << "Enum bitwise operations require explicit enum_to_bits";
    bool mathematical = needsMath(*lhs) || needsMath(*rhs);
    auto leftInfo = valueInfo(*lhs), rightInfo = valueInfo(*rhs);
    if (leftInfo.sourceKind && rightInfo.sourceKind &&
        leftInfo.sourceKind != rightInfo.sourceKind)
      return error() << "bitwise operands have different known Boolean and "
                        "Integer kinds";
    bool constantMask = provenMask(*lhs) || provenMask(*rhs);
    bool representations =
        literalWidth(lhs->getType()) && literalWidth(rhs->getType());
    if (form == "BitAnd" && leftInfo.sourceKind == ValueKind::Integer &&
        rightInfo.sourceKind == ValueKind::Integer &&
        (mathematical || (constantMask && representations &&
                          leftInfo.interval && rightInfo.interval))) {
      auto result =
          lowerExactIntegerBinary(at, numericSite(node, ownerSymbol),
                                  ValueOpcode::AndBits, leftInfo, rightInfo);
      if (failed(result))
        return failure();
      numericValues[result->value] = *result;
      if (mathematical)
        arithmeticValues.insert(result->value);
      if (failed(propagateClosed(result->value, ValueOpcode::AndBits,
                                 {*lhs, *rhs}, node)))
        return failure();
      return result->value;
    }
    if (mathematical)
      return error() << "mathematical bitwise composition requires a proven "
                        "constant mask";
    if (!ac::areEquivalentHardwareTypes(rhs->getType(), lhs->getType()))
      return error() << "binary operands require equal bits types";
    auto kind = leftInfo.sourceKind == rightInfo.sourceKind
                    ? leftInfo.sourceKind
                    : std::nullopt;
    auto result = track(binary(opcode, *lhs, *rhs), kind);
    bool mixedOneBit =
        ac::areEquivalentHardwareTypes(lhs->getType(), makeWidth(1)) &&
        (fixedValues.contains(*lhs) ||
         leftInfo.sourceKind == ValueKind::Boolean) &&
        (fixedValues.contains(*rhs) ||
         rightInfo.sourceKind == ValueKind::Boolean) &&
        (fixedValues.contains(*lhs) || fixedValues.contains(*rhs));
    if (mixedOneBit ||
        ((fixedValues.contains(*lhs) ||
          node.child("left").kind() == "Constant") &&
         (fixedValues.contains(*rhs) ||
          node.child("right").kind() == "Constant") &&
         (fixedValues.contains(*lhs) || fixedValues.contains(*rhs)))) {
      fixedValues.insert(result);
      numericValues.erase(result);
    }
    bool boolean = valueInfo(result).sourceKind == ValueKind::Boolean;
    auto sourceOpcode =
        form == "BitAnd"
            ? (boolean ? ValueOpcode::AndBool : ValueOpcode::AndBits)
        : form == "BitOr"
            ? (boolean ? ValueOpcode::OrBool : ValueOpcode::OrBits)
            : (boolean ? ValueOpcode::Ne : ValueOpcode::XorBits);
    if (failed(propagateClosed(result, sourceOpcode, {*lhs, *rhs}, node)))
      return failure();
    return result;
  }
  if (node.kind() == "Compare") {
    if (node.array("ops").size() != 1 || node.array("comparators").size() != 1)
      return error() << "hardware comparisons do not support chaining";
    StringRef form = node.item("ops", 0).kind();
    StringRef predicate = form == "Eq"      ? "eq"
                          : form == "NotEq" ? "ne"
                          : form == "Lt"    ? "ult"
                          : form == "LtE"   ? "ule"
                          : form == "Gt"    ? "ugt"
                          : form == "GtE"   ? "uge"
                                            : StringRef();
    if (predicate.empty())
      return error() << "unsupported hardware comparison predicate";
    AstNode left = node.child("left"), right = node.item("comparators", 0);
    FailureOr<Value> lhs = failure(), rhs = failure();
    if (left.kind() == "Constant") {
      rhs = expression(right, at, ownerSymbol, values, instances, {}, captures);
      if (failed(rhs))
        return failure();
      lhs = expression(left, at, ownerSymbol, values, instances, rhs->getType(),
                       captures);
    } else {
      lhs = expression(left, at, ownerSymbol, values, instances, {}, captures);
      if (failed(lhs))
        return failure();
      rhs = expression(right, at, ownerSymbol, values, instances,
                       lhs->getType(), captures);
    }
    if (failed(lhs) || failed(rhs))
      return failure();
    return compareSourceValues(*lhs, *rhs, predicate, left, right, node, at,
                               ownerSymbol);
  }
  if (node.kind() == "IfExp") {
    auto cond = expression(node.child("test"), at, ownerSymbol, values,
                           instances, {}, captures);
    if (failed(cond))
      return failure();
    auto condition = predicateView(*cond, node, ownerSymbol);
    if (failed(condition))
      return failure();
    auto yes = expression(node.child("body"), at, ownerSymbol, values,
                          instances, expected, captures);
    if (failed(yes))
      return failure();
    // Scalar branches get the same outer context. Aggregate contextual zero
    // keeps its established peer-type hint without creating scalar authority.
    auto noExpected = isa<ac::BitsType>(yes->getType())
                          ? expected
                          : std::optional<Type>(yes->getType());
    auto no = expression(node.child("orelse"), at, ownerSymbol, values,
                         instances, noExpected, captures);
    if (failed(no))
      return failure();
    auto a = sourceChoice(*yes, node.child("body"));
    auto c = sourceChoice(*no, node.child("orelse"));
    if (failed(a) || failed(c))
      return failure();
    return joinSourceValues(*condition, *a, *c, node, at, ownerSymbol);
  }
  return error() << "unsupported hardware expression '" << node.kind() << "'";
}

FailureOr<Value>
Importer::compareSourceValues(Value lhs, Value rhs, StringRef predicate,
                              const AstNode &leftSite, const AstNode &rightSite,
                              const AstNode &site, OpBuilder &at,
                              FlatSymbolRefAttr ownerSymbol) {
  using ac::detail::ValueKind;
  using ac::detail::ValueOpcode;
  auto diagnostic = [&] {
    return mlir::emitError(site.location(b.getContext(), source.path));
  };
  Value left = lhs, right = rhs;
  bool nominal =
      isa<ac::EnumType>(lhs.getType()) || isa<ac::EnumType>(rhs.getType());
  if (nominal) {
    if ((predicate != "eq" && predicate != "ne") ||
        !ac::areEquivalentHardwareTypes(lhs.getType(), rhs.getType()))
      return diagnostic()
             << "Enum comparison requires eq/ne and the same nominal type";
    auto a = enumToBits(lhs, leftSite, at, ownerSymbol);
    auto c = enumToBits(rhs, rightSite, at, ownerSymbol);
    if (failed(a) || failed(c))
      return failure();
    left = *a;
    right = *c;
  } else {
    auto a = valueInfo(lhs), c = valueInfo(rhs);
    if (a.sourceKind && c.sourceKind && a.sourceKind != c.sourceKind)
      return diagnostic() << "comparison operands have different known Boolean "
                             "and Integer kinds";
    if (arithmeticValues.contains(lhs) || arithmeticValues.contains(rhs) ||
        (a.interval && a.interval->lower.isNegative()) ||
        (c.interval && c.interval->lower.isNegative()))
      return diagnostic()
             << "comparison of mathematical intermediates requires full-width "
                "signed type unification";
    if (!isa<ac::BitsType>(lhs.getType()) ||
        !ac::areEquivalentHardwareTypes(rhs.getType(), lhs.getType()))
      return diagnostic() << "comparison operands require equal bits types";
  }
  auto result =
      createOp(at, site.location(b.getContext(), source.path),
               ac::BitsCompareOp::getOperationName(), {left, right},
               TypeRange{bits(1, site, ownerSymbol)},
               {at.getNamedAttr("predicate", at.getStringAttr(predicate))})
          ->getResult(0);
  remember(result, ValueKind::Boolean);
  if (!nominal) {
    auto opcode = predicate == "eq"    ? ValueOpcode::Eq
                  : predicate == "ne"  ? ValueOpcode::Ne
                  : predicate == "ult" ? ValueOpcode::Lt
                  : predicate == "ule" ? ValueOpcode::Le
                  : predicate == "ugt" ? ValueOpcode::Gt
                                       : ValueOpcode::Ge;
    if (failed(propagateClosed(result, opcode, {lhs, rhs}, site)))
      return failure();
  }
  return result;
}
FailureOr<Type> Importer::expressionType(const AstNode &node,
                                         llvm::StringMap<Value> &values,
                                         llvm::StringMap<Instance *> &instances,
                                         FlatSymbolRefAttr ownerSymbol) {
  auto literalWidth = [&](unsigned width) -> Type {
    auto value = parseStaticInteger(b, Twine(width).str(), error);
    if (failed(value))
      return {};
    DictionaryAttr literal =
        b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("integer")),
                             b.getNamedAttr("value", *value)});
    auto expression = ac::StaticExprAttr::get(
        b.getContext(),
        b.getDictionaryAttr(
            {b.getNamedAttr("kind", b.getStringAttr("literal")),
             b.getNamedAttr("value", literal),
             b.getNamedAttr("origin", occurrence(b, ownerSymbol, node)),
             b.getNamedAttr("location", sourceSpan(b, source.path, node))}));
    return ac::BitsType::get(b.getContext(), expression);
  };
  auto member = resolveEnumMember(node);
  if (failed(member))
    return failure();
  if (*member)
    return Type((*member)->type);
  if (auto producer = fixedResultCall(node); !producer.empty())
    return mlir::emitError(node.location(b.getContext(), source.path))
           << producer
           << " produces fixed results; unpack first into ordinary "
              "local Names";
  if (auto name = fixedCountCall(node); !name.empty()) {
    if (failed(validateFixedCountCall(node, name)))
      return failure();
    auto operand =
        expressionType(node.item("args", 0), values, instances, ownerSymbol);
    if (failed(operand))
      return failure();
    auto shape = resolveFixedCountShape(node, *operand, ownerSymbol, name);
    if (failed(shape))
      return failure();
    return shape->resultType;
  }
  if (concatCall(node)) {
    if (failed(validateConcatCall(node)))
      return failure();
    SmallVector<Type> operands;
    for (size_t i = 0; i < node.array("args").size(); ++i) {
      auto type =
          expressionType(node.item("args", i), values, instances, ownerSymbol);
      if (failed(type))
        return failure();
      operands.push_back(*type);
    }
    return resolveConcatType(node, operands, ownerSymbol);
  }
  if (node.kind() == "Call" && intrinsic(node.child("func"), "enum_to_bits")) {
    if (node.array("args").size() != 1 || !node.array("keywords").empty())
      return error()
             << "enum_to_bits requires exactly one positional Enum value";
    auto input =
        expressionType(node.item("args", 0), values, instances, ownerSymbol);
    if (failed(input))
      return failure();
    auto enumeration = dyn_cast<ac::EnumType>(*input);
    if (!enumeration)
      return error() << "enum_to_bits requires a nominal Enum value";
    auto definition = ac::HardwareAnalysis(*body).resolveEnum(
        enumeration, body->getOperation());
    if (failed(definition))
      return failure();
    return Type(ac::BitsType::get(
        b.getContext(), literal(definition->width, node, ownerSymbol)));
  }
  if (node.kind() == "Assert" || node.kind() == "Expr")
    return literalWidth(1);
  if (node.kind() == "Name") {
    auto found = values.find(node.string("id"));
    if (found == values.end() && activeBehavioralDecl &&
        activeBehavioralDecl->symbol == ownerSymbol &&
        forwardCallBindings.contains(node.string("id"))) {
      auto value = forwardModuleValue(node.string("id"), ownerSymbol);
      if (failed(value))
        return failure();
      return value->getType();
    }
    if (found == values.end())
      return error() << "unknown hardware value '" << node.string("id") << "'";
    return found->second.getType();
  }
  if (node.kind() == "Attribute") {
    if (node.child("value").kind() == "Name" &&
        node.child("value").string("id") != queryArgument) {
      auto found = instances.find(node.child("value").string("id"));
      if (found != instances.end()) {
        for (const Port &port : found->second->outputs)
          if (port.name == node.string("attr"))
            return port.type;
        return error() << "unknown instance output '" << node.string("attr")
                       << "'";
      }
    }
    auto base =
        expressionType(node.child("value"), values, instances, ownerSymbol);
    if (failed(base))
      return failure();
    auto record = dyn_cast<ac::StructType>(*base);
    ac::HardwareAnalysis analysis(*body);
    auto declaration = record ? analysis.lookupStruct(record) : ac::StructOp();
    if (!declaration)
      return error() << "field projection requires a struct";
    for (Attribute raw : declaration.getFields()) {
      auto field = cast<DictionaryAttr>(raw);
      if (field.getAs<StringAttr>("name").getValue() == node.string("attr"))
        return field.getAs<TypeAttr>("type").getValue();
    }
    return error() << "unknown struct field";
  }
  if (node.kind() == "Compare")
    return literalWidth(1);
  if (node.kind() == "Constant") {
    if (isa<BoolAttr>(node.get("value")))
      return literalWidth(1);
    auto encoded = dyn_cast_or_null<DictionaryAttr>(node.get("value"));
    auto spelling =
        encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
    if (!spelling)
      return error() << "observation literal must be bool or integer";
    auto value = parseStaticInteger(b, spelling.getValue(), error);
    if (failed(value))
      return failure();
    llvm::APSInt integer(value->getCanonicalValue());
    if (integer.isNegative())
      return error() << "observation integer literal must be nonnegative";
    return literalWidth(std::max(1u, integer.getActiveBits()));
  }
  if (node.kind() == "BinOp") {
    auto lhs =
        expressionType(node.child("left"), values, instances, ownerSymbol);
    if (succeeded(lhs))
      return lhs;
    return expressionType(node.child("right"), values, instances, ownerSymbol);
  }
  if (node.kind() == "Subscript") {
    auto base =
        expressionType(node.child("value"), values, instances, ownerSymbol);
    if (failed(base))
      return failure();
    if (node.child("slice").kind() == "Slice") {
      auto shape = resolveSlice(node.child("slice"), *base, ownerSymbol);
      if (failed(shape))
        return failure();
      return shape->type;
    }
    auto table = dyn_cast<ac::TableType>(*base);
    if (!table)
      return error() << "subscript requires a table";
    return table.getElementType();
  }
  if (node.kind() == "IfExp")
    return expressionType(node.child("body"), values, instances, ownerSymbol);
  return error() << "cannot infer finite hardware expression type";
}

FailureOr<Value> Importer::trueValue(const AstNode &node, OpBuilder &at,
                                     FlatSymbolRefAttr ownerSymbol) {
  auto one = parseStaticInteger(b, "1", error);
  if (failed(one))
    return failure();
  DictionaryAttr integer =
      b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("integer")),
                           b.getNamedAttr("value", *one)});
  auto expression = ac::StaticExprAttr::get(
      b.getContext(),
      b.getDictionaryAttr(
          {b.getNamedAttr("kind", b.getStringAttr("literal")),
           b.getNamedAttr("value", integer),
           b.getNamedAttr("origin", occurrence(b, ownerSymbol, node)),
           b.getNamedAttr("location", sourceSpan(b, source.path, node))}));
  Type type = ac::BitsType::get(b.getContext(), expression);
  return createOp(at, node.location(b.getContext(), source.path),
                  ac::BitsConstantOp::getOperationName(), {}, TypeRange{type},
                  {at.getNamedAttr("value", expression)})
      ->getResult(0);
}

size_t Importer::countSourceAssertions(const AstNode &parent, StringRef field) {
  size_t count = 0;
  for (size_t i = 0; i < parent.array(field).size(); ++i) {
    AstNode statement = parent.item(field, i);
    if (statement.kind() == "Assert")
      ++count;
    else if (statement.kind() == "If")
      count += countSourceAssertions(statement, "body") +
               countSourceAssertions(statement, "orelse");
    else if (statement.kind() == "Match")
      for (size_t arm = 0; arm < statement.array("cases").size(); ++arm)
        count += countSourceAssertions(statement.item("cases", arm), "body");
  }
  return count;
}

bool Importer::observationValueItems(const AstNode &call,
                                     StringRef observationKind,
                                     SmallVectorImpl<AstNode> &items) {
  if (call.kind() != "Call" || !call.array("keywords").empty())
    return false;
  size_t first = observationKind == "log" ? 2 : 1;
  if (observationKind == "report" && call.array("args").size() != 2)
    return false;
  for (size_t i = first; i < call.array("args").size(); ++i) {
    AstNode item = call.item("args", i);
    auto text = item.kind() == "Constant"
                    ? dyn_cast_or_null<StringAttr>(item.get("value"))
                    : StringAttr();
    if (!text)
      items.push_back(item);
  }
  return true;
}

size_t Importer::countSourceChecks(const AstNode &parent, StringRef field) {
  // Source assertions are admitted at any depth; a source observation is only
  // admitted as an unconditional rule statement, so only the rule body's own
  // top level contributes observation groups.
  size_t count = countSourceAssertions(parent, field);
  for (size_t i = 0; i < parent.array(field).size(); ++i) {
    AstNode statement = parent.item(field, i);
    if (statement.kind() != "Expr")
      continue;
    AstNode call = statement.child("value");
    StringRef observationKind;
    if (call.kind() == "Call" && intrinsic(call.child("func"), "log"))
      observationKind = "log";
    else if (call.kind() == "Call" &&
             intrinsic(call.child("func"), "report"))
      observationKind = "report";
    if (observationKind.empty())
      continue;
    SmallVector<AstNode, 4> items;
    if (!observationValueItems(call, observationKind, items))
      continue;
    count += std::max<size_t>(1, items.size());
  }
  return count;
}

FailureOr<Value> Importer::checkAnd(Value lhs, Value rhs, const AstNode &node,
                                    OpBuilder &at, FlatSymbolRefAttr symbol) {
  Value result = createOp(at, node.location(b.getContext(), source.path),
                          ac::BitsBinaryOp::getOperationName(), {lhs, rhs},
                          TypeRange{bits(1, node, symbol)},
                          {at.getNamedAttr("opcode", at.getStringAttr("and"))})
                     ->getResult(0);
  remember(result, ac::detail::ValueKind::Boolean);
  return result;
}

FailureOr<Value> Importer::checkNot(Value value, const AstNode &node,
                                    OpBuilder &at, FlatSymbolRefAttr symbol) {
  Value result = createOp(at, node.location(b.getContext(), source.path),
                          ac::BitsUnaryOp::getOperationName(), {value},
                          TypeRange{bits(1, node, symbol)},
                          {at.getNamedAttr("opcode", at.getStringAttr("not"))})
                     ->getResult(0);
  remember(result, ac::detail::ValueKind::Boolean);
  return result;
}

LogicalResult Importer::enterCheckBranch(BranchEnvironment &environment,
                                         Value demand, const AstNode &node,
                                         OpBuilder &at,
                                         FlatSymbolRefAttr symbol) {
  if (!environment.continuation)
    return success();
  auto path =
      checkAnd(environment.ambient, environment.continuation, node, at, symbol);
  if (failed(path))
    return failure();
  path = checkAnd(*path, demand, node, at, symbol);
  auto one = constant(bits(1, node, symbol), 1, node, at, symbol);
  if (failed(path) || failed(one))
    return failure();
  environment.ambient = *path;
  environment.continuation = *one;
  return success();
}

LogicalResult Importer::captureSourceCheck(const AstNode &statement,
                                           OpBuilder &at,
                                           FlatSymbolRefAttr symbol,
                                           BranchEnvironment &environment) {
  llvm::StringMap<Instance *> noInstances;
  auto &instances =
      activeRule->instances ? *activeRule->instances : noInstances;
  auto condition = expression(statement.child("test"), at, symbol,
                              environment.values, instances);
  if (failed(condition) ||
      failed(predicateView(*condition, statement.child("test"), symbol)))
    return failure();
  StringAttr message;
  if (AstNode node = statement.child("msg")) {
    message = node.kind() == "Constant"
                  ? dyn_cast_or_null<StringAttr>(node.get("value"))
                  : StringAttr();
    if (!message)
      return mlir::emitError(node.location(b.getContext(), source.path))
             << "assert message must be a static string";
  }
  auto path = checkAnd(environment.ambient, environment.continuation, statement,
                       at, symbol);
  auto continued =
      checkAnd(environment.continuation, *condition, statement, at, symbol);
  if (failed(path) || failed(continued))
    return failure();
  activeRule->checks.push_back({statement, *condition, *path, message});
  environment.continuation = *continued;
  return success();
}

LogicalResult Importer::captureRuleObservation(const AstNode &rule,
                                               const AstNode &statement,
                                               OpBuilder &at,
                                               FlatSymbolRefAttr symbol,
                                               BranchEnvironment &environment) {
  SmallVector<AstNode, 4> valueItems;
  DictionaryAttr spec;
  StringRef intrinsic;
  if (failed(buildObservation(statement, valueItems, spec, intrinsic)))
    return failure();
  llvm::StringMap<Instance *> noInstances;
  auto &instances =
      activeRule->instances ? *activeRule->instances : noInstances;
  auto path = trueValue(statement, at, symbol);
  if (failed(path))
    return failure();
  // One group per hardware value operand; a `log` whose items are all literal
  // still needs one placeholder group so the site is published at all.
  unsigned groups = std::max<size_t>(1, valueItems.size());
  auto &checks = activeRule->checks;
  SmallVector<Value> values;
  for (const AstNode &item : valueItems) {
    // Resolve the declared type first so a value this path cannot type is
    // rejected with the same diagnostic the pure-module instrumentation pass
    // gives for the identical statement.
    auto type = expressionType(item, environment.values, instances, symbol);
    if (failed(type))
      return failure();
    auto value =
        expression(item, at, symbol, environment.values, instances, *type);
    if (failed(value))
      return failure();
    values.push_back(*value);
  }
  for (unsigned index = 0; index < groups; ++index) {
    SourceCheckCapture capture;
    capture.statement = statement;
    capture.path = *path;
    capture.observationKind = b.getStringAttr(intrinsic);
    capture.observationSpec = spec;
    capture.observationRule = rule;
    capture.observationValues = groups;
    capture.observationCarriesValue = index < values.size();
    capture.condition =
        capture.observationCarriesValue ? values[index] : *path;
    checks.insert(checks.begin() + activeRule->observationEntries++, capture);
  }
  return success();
}

LogicalResult Importer::publishSourceChecks(Operation *rule,
                                            RuleEvaluation &evaluation,
                                            OpBuilder &at,
                                            FlatSymbolRefAttr symbol) {
  if (evaluation.checks.empty())
    return success();
  SmallVector<Attribute> required;
  unsigned first = rule->getNumResults() - 2 * evaluation.checks.size();
  // The rule result suffix is declared with one-bit slots because a source
  // assertion yields Boolean condition/path values, which the declared slot
  // type already accepts. A source observation yields a hardware value of any
  // type, so only its value slot is republished with the resolved type. The
  // structural lowering retypes every result from its yielded value, which
  // makes this a no-op there; the behavioral lowering has no such pass.
  auto retypeValue = [&](unsigned index, const SourceCheckCapture &capture) {
    rule->getResult(first + 2 * index).setType(capture.condition.getType());
  };
  unsigned observationOrdinal = 0;
  for (size_t index = 0; index < evaluation.checks.size();) {
    const SourceCheckCapture &check = evaluation.checks[index];
    if (!check.observationKind) {
      auto identity = at.getDictionaryAttr(
          {at.getNamedAttr("registration", rule->getAttr("occurrence")),
           at.getNamedAttr("check", occurrence(at, symbol, check.statement)),
           at.getNamedAttr("obligation", at.getI64IntegerAttr(0))});
      auto kind = at.getStringAttr("assert");
      auto location = sourceSpan(at, source.path, check.statement);
      required.push_back(at.getDictionaryAttr(
          {at.getNamedAttr("id", identity), at.getNamedAttr("kind", kind),
           at.getNamedAttr("location", location)}));
      SmallVector<NamedAttribute> attrs{at.getNamedAttr("kind", kind),
                                        at.getNamedAttr("location", location),
                                        at.getNamedAttr("ac.check_id", identity)};
      if (check.message)
        attrs.push_back(at.getNamedAttr("ac.message", check.message));
      createOp(at, check.statement.location(b.getContext(), source.path),
               ac::SourceExpectOp::getOperationName(),
               ValueRange{rule->getResult(first + 2 * index),
                          rule->getResult(first + 2 * index + 1)},
               {}, attrs);
      ++index;
      continue;
    }
    // One `ac.observe` per source site: collect the consecutive entries the
    // site contributed, then publish with the shared true path plus every
    // hardware value operand. This is the same op shape the pure-module
    // instrumentation pass emits for the same source statement.
    size_t end = index + 1;
    while (end < evaluation.checks.size() &&
           sameSourceSite(evaluation.checks[end].statement, check.statement))
      ++end;
    SmallVector<Value> operands{rule->getResult(first + 2 * index + 1)};
    for (size_t item = index; item < end; ++item) {
      if (!evaluation.checks[item].observationCarriesValue)
        continue;
      operands.push_back(rule->getResult(first + 2 * item));
      retypeValue(item, evaluation.checks[item]);
    }
    auto identity = at.getDictionaryAttr(
        {at.getNamedAttr("registration",
                         occurrence(at, symbol, check.observationRule)),
         at.getNamedAttr("site", occurrence(at, symbol, check.statement)),
         at.getNamedAttr("ordinal",
                         at.getI64IntegerAttr(observationOrdinal++))});
    createOp(at, check.statement.location(b.getContext(), source.path),
             ac::SourceObserveOp::getOperationName(), operands, {},
             {at.getNamedAttr("kind", check.observationKind),
              at.getNamedAttr("spec", check.observationSpec),
              at.getNamedAttr("ac.observation_id", identity),
              at.getNamedAttr("ac.location",
                              sourceSpan(at, source.path, check.statement))});
    index = end;
  }
  rule->setAttr("ac.required_checks", at.getArrayAttr(required));
  return success();
}

LogicalResult Importer::structuralBinding(const AstNode &call, OpBuilder &at,
                                          FlatSymbolRefAttr symbol,
                                          BranchEnvironment &environment) {
  auto &evaluation = *activeRule;
  Instance *resolved = evaluation.target;
  auto function = call.child("func");
  if (function.kind() == "Name") {
    auto found = evaluation.instances->find(function.string("id"));
    if (found != evaluation.instances->end())
      resolved = found->second;
  }
  if (!resolved)
    return error() << "structural binding has no instance target";
  auto &target = *resolved;
  if (!evaluation.structuralTargets.empty()) {
    if (evaluation.nextStructuralTarget >=
            evaluation.structuralTargets.size() ||
        evaluation.structuralTargets[evaluation.nextStructuralTarget] !=
            &target)
      return error() << "structural instance binding order changed during "
                        "lowering";
    ++evaluation.nextStructuralTarget;
  }
  if (!call.array("args").empty())
    return error() << "each instance requires one complete binding rule";
  llvm::StringMap<AstNode> actuals;
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    AstNode keyword = call.item("keywords", i);
    auto name = dyn_cast_or_null<StringAttr>(keyword.get("arg"));
    if (!name || actuals.contains(name.getValue()))
      return error() << "duplicate instance input";
    actuals[name.getValue()] = keyword.child("value");
  }
  for (auto [ordinal, input] : llvm::enumerate(target.inputs)) {
    if (target.clockInput == ordinal || target.resetInput == ordinal) {
      StringRef pin = target.clockInput == ordinal ? "pyc_clk" : "pyc_rst";
      auto value = environment.values.find(pin);
      if (value == environment.values.end())
        return error() << "instance binding lacks inferred physical domain";
      evaluation.bindingValues.push_back(value->second);
      continue;
    }
    auto found = actuals.find(input.name);
    if (found == actuals.end())
      return error() << "missing instance input '" << input.name << "'";
    auto value = expression(found->second, at, symbol, environment.values,
                            *evaluation.instances, input.type);
    if (failed(value))
      return failure();
    auto converted = boundary(*value, input, found->second, at, symbol);
    if (failed(converted))
      return failure();
    evaluation.bindingValues.push_back(*converted);
  }
  size_t inferred = unsigned(target.clockInput.has_value()) +
                    unsigned(target.resetInput.has_value());
  if (actuals.size() + inferred != target.inputs.size())
    return error() << "unknown instance input";
  return success();
}

LogicalResult Importer::buildObservation(const AstNode &statement,
                                         SmallVectorImpl<AstNode> &valueItems,
                                         DictionaryAttr &spec,
                                         StringRef &observationKind) {
  AstNode call =
      statement.kind() == "Expr" ? statement.child("value") : AstNode();
  observationKind = {};
  if (call.kind() == "Call" && intrinsic(call.child("func"), "log"))
    observationKind = "log";
  else if (call.kind() == "Call" &&
           intrinsic(call.child("func"), "report"))
    observationKind = "report";
  if (observationKind.empty())
    return error() << "unsupported rule instrumentation statement";
  SmallVector<Attribute> items;
  if (!call.array("keywords").empty())
    return error() << observationKind
                   << " observation does not accept keywords";
  if (observationKind == "log") {
    if (call.array("args").size() < 2)
      return error() << "log requires level, event and optional items";
    auto level =
        dyn_cast_or_null<StringAttr>(call.item("args", 0).get("value"));
    auto event =
        dyn_cast_or_null<StringAttr>(call.item("args", 1).get("value"));
    if (call.item("args", 0).kind() != "Constant" ||
        call.item("args", 1).kind() != "Constant" || !level || !event ||
        !llvm::is_contained(
            ArrayRef<StringRef>{"debug", "info", "warning", "error"},
            level.getValue()) ||
        event.getValue().empty())
      return error() << "log requires a valid static level and event";
    for (size_t i = 2; i < call.array("args").size(); ++i) {
      AstNode item = call.item("args", i);
      auto text = item.kind() == "Constant"
                      ? dyn_cast_or_null<StringAttr>(item.get("value"))
                      : StringAttr();
      if (text) {
        items.push_back(b.getDictionaryAttr(
            {b.getNamedAttr("kind", b.getStringAttr("literal")),
             b.getNamedAttr("text", text)}));
      } else {
        unsigned valueOrdinal = valueItems.size();
        valueItems.push_back(item);
        items.push_back(b.getDictionaryAttr(
            {b.getNamedAttr("kind", b.getStringAttr("value")),
             b.getNamedAttr("ordinal", b.getI32IntegerAttr(valueOrdinal))}));
      }
    }
    spec = b.getDictionaryAttr(
        {b.getNamedAttr("level", level), b.getNamedAttr("event", event),
         b.getNamedAttr("items", b.getArrayAttr(items))});
    return success();
  }
  if (call.array("args").size() != 2)
    return error() << "report requires a static name and one value";
  auto name = dyn_cast_or_null<StringAttr>(call.item("args", 0).get("value"));
  if (call.item("args", 0).kind() != "Constant" || !name ||
      name.getValue().empty())
    return error() << "report name must be a nonempty static string";
  valueItems.push_back(call.item("args", 1));
  spec = b.getDictionaryAttr({b.getNamedAttr("name", name)});
  return success();
}

LogicalResult Importer::emitInstrumentation(
    const ModuleDecl &module, const AstNode &rule, const AstNode &statement,
    unsigned ordinal, OpBuilder &at, llvm::StringMap<Value> &moduleValues,
    llvm::StringMap<Instance *> &instances) {
  SmallVector<AstNode, 4> dynamicValues;
  DictionaryAttr spec;
  StringRef intrinsic;
  if (failed(buildObservation(statement, dynamicValues, spec, intrinsic)))
    return failure();

  SmallVector<Value> captures;
  for (const auto &entry : moduleValues)
    captures.push_back(entry.second);
  for (const auto &entry : instances)
    for (Value output : entry.second->op->getResults())
      captures.push_back(output);

  SmallVector<Type> resultTypes;
  auto oneBit =
      expressionType(statement, moduleValues, instances, module.symbol);
  if (failed(oneBit))
    return failure();
  {
    resultTypes.push_back(*oneBit);
    for (const AstNode &value : dynamicValues) {
      auto type = expressionType(value, moduleValues, instances, module.symbol);
      if (failed(type))
        return failure();
      resultTypes.push_back(*type);
    }
  }

  std::string name =
      (Twine(rule.string("name")) + "." + "observe." + Twine(ordinal)).str();
  Operation *ruleOp = createOp(
      at, statement.location(b.getContext(), source.path),
      ac::RuleOp::getOperationName(), captures, resultTypes,
      {at.getNamedAttr("name", at.getStringAttr(name)),
       at.getNamedAttr("occurrence", occurrence(at, module.symbol, statement))},
      1);
  Block *block = new Block();
  ruleOp->getRegion(0).push_back(block);
  llvm::StringMap<Value> values;
  unsigned captureOrdinal = 0;
  for (const auto &entry : moduleValues)
    values[entry.first()] =
        captureValue(block, captures[captureOrdinal++], ruleOp->getLoc());
  llvm::StringMap<SmallVector<Value>> capturedOutputs;
  for (const auto &entry : instances)
    for (const Port &port : entry.second->outputs) {
      (void)port;
      capturedOutputs[entry.first()].push_back(
          captureValue(block, captures[captureOrdinal++], ruleOp->getLoc()));
    }

  OpBuilder inside = OpBuilder::atBlockEnd(block);
  SmallVector<Value> yields;
  {
    auto path = trueValue(statement, inside, module.symbol);
    if (failed(path))
      return failure();
    yields.push_back(*path);
    for (auto [index, valueNode] : llvm::enumerate(dynamicValues)) {
      auto value =
          expression(valueNode, inside, module.symbol, values, instances,
                     resultTypes[index + 1], &capturedOutputs);
      if (failed(value))
        return failure();
      yields.push_back(*value);
    }
  }
  createOp(inside, statement.location(b.getContext(), source.path),
           ac::YieldOp::getOperationName(), yields, {}, {});

  DictionaryAttr identity = b.getDictionaryAttr(
      {b.getNamedAttr("registration", occurrence(b, module.symbol, rule)),
       b.getNamedAttr("site", occurrence(b, module.symbol, statement)),
       b.getNamedAttr("ordinal", b.getI64IntegerAttr(ordinal))});
  {
    SmallVector<Value> operands{ruleOp->getResult(0)};
    for (unsigned i = 1; i < ruleOp->getNumResults(); ++i)
      operands.push_back(ruleOp->getResult(i));
    createOp(at, statement.location(b.getContext(), source.path),
             ac::SourceObserveOp::getOperationName(), operands, {},
             {at.getNamedAttr("kind", at.getStringAttr(intrinsic)),
              at.getNamedAttr("spec", spec),
              at.getNamedAttr("ac.observation_id", identity),
              at.getNamedAttr("ac.location",
                              sourceSpan(at, source.path, statement))});
  }
  return success();
}

LogicalResult Importer::emitModule(ModuleDecl &decl) {
  if (decl.behavioral)
    return emitBehavioral(decl);
  SmallVector<Attribute> params;
  for (const Parameter &p : decl.parameters) {
    ac::StaticExprAttr defaultValue;
    if (p.value) {
      auto value = staticExpr(*p.value, decl.symbol);
      if (failed(value))
        return failure();
      defaultValue = *value;
    }
    params.push_back(ac::getStaticParameterAttr(b, p.name, defaultValue));
  }
  b.setInsertionPointToEnd(body->getBody());
  Operation *module = createOp(
      b, decl.node.location(b.getContext(), source.path),
      ac::ModuleOp::getOperationName(), {}, {},
      {b.getNamedAttr(SymbolTable::getSymbolAttrName(),
                      b.getStringAttr(decl.symbol.getValue())),
       b.getNamedAttr("source_owner", owner),
       b.getNamedAttr("parameters", b.getArrayAttr(params)),
       b.getNamedAttr("function_type", TypeAttr::get(decl.type)),
       b.getNamedAttr("type_parameters", b.getArrayAttr({})),
       b.getNamedAttr("input_names", portNames(b, decl.inputs)),
       b.getNamedAttr("output_names", portNames(b, decl.outputs)),
       b.getNamedAttr("ac.declaration_role", b.getStringAttr("definition")),
       b.getNamedAttr("ac.origin", occurrence(b, decl.symbol, decl.node))},
      1);
  if (decl.system)
    module->setAttr("ac.root_kind", b.getStringAttr("system"));
  for (NamedAttribute attr : decl.sourceCallAttrs)
    module->setAttr(attr.getName(), attr.getValue());
  if (failed(ac::detail::verifyModuleSourceCallContract(module)))
    return failure();
  Block *graph = new Block();
  module->getRegion(0).push_back(graph);
  llvm::StringMap<Value> values;
  for (const Port &input : decl.inputs) {
    Value argument = graph->addArgument(input.type, module->getLoc());
    values[input.name] = argument;
    remember(argument, input.sourceKind);
    if (input.fixedBits)
      fixedValues.insert(argument);
  }
  OpBuilder at = OpBuilder::atBlockEnd(graph);
  SmallVector<Instance, 0> storage;
  size_t instanceCount = 0;
  llvm::StringSet<> ruleNames, parameterNames, localBindings;
  for (const Parameter &parameter : decl.parameters)
    parameterNames.insert(parameter.name);
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    AstNode statement = decl.node.item("body", i);
    if (statement.kind() == "Assign" &&
        statement.child("value").kind() == "Call")
      ++instanceCount;
    if (statement.kind() == "FunctionDef")
      ruleNames.insert(statement.string("name"));
  }
  storage.reserve(instanceCount);
  llvm::StringMap<Instance *> instances;
  llvm::StringMap<AstNode> rules;
  llvm::StringSet<> tupleForbidden;
  for (const Port &input : decl.inputs)
    tupleForbidden.insert(input.name);
  for (const auto &name : parameterNames)
    tupleForbidden.insert(name.getKey());
  for (const auto &name : ruleNames)
    tupleForbidden.insert(name.getKey());
  SmallVector<AstNode> registrations;
  AstNode returned;

  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    AstNode stmt = decl.node.item("body", i);
    if (docstring(stmt))
      continue;
    if (stmt.kind() == "FunctionDef") {
      if (!hasDecorator(stmt, "rule") || rules.contains(stmt.string("name")))
        return error() << "nested functions require unique @rule names";
      rules[stmt.string("name")] = stmt;
      continue;
    }
    if (stmt.kind() == "Assign") {
      auto declarationError = [&] {
        return mlir::emitError(stmt.location(b.getContext(), source.path));
      };
      auto fixedTargets = fixedResultTargets(stmt);
      if (failed(fixedTargets))
        return failure();
      if (*fixedTargets) {
        if (returned)
          return declarationError()
                 << "pure module local declaration after Return is unsupported";
        auto bound = bindFixedResults(stmt, at, decl.symbol, values, instances,
                                      nullptr, tupleForbidden, true);
        if (failed(bound))
          return failure();
        for (StringRef name : **fixedTargets)
          localBindings.insert(name);
        continue;
      }
      if (stmt.array("targets").size() != 1 ||
          stmt.item("targets", 0).kind() != "Name")
        return declarationError()
               << "module assignment requires one immutable Name target";
      StringRef instanceName = stmt.item("targets", 0).string("id");
      if (localBindings.contains(instanceName))
        return declarationError()
               << "module local cannot be rebound or shadowed: '"
               << instanceName << "'";
      AstNode rhs = stmt.child("value");
      StringRef calleeName =
          rhs.kind() == "Call" && rhs.child("func").kind() == "Name"
              ? rhs.child("func").string("id")
              : StringRef();
      auto binding = names.find(calleeName);
      if (binding != names.end() &&
          binding->second.category == BindingCategory::Enum)
        return declarationError()
               << "Enum declarations do not have source constructors";
      if (binding == names.end() ||
          binding->second.category != BindingCategory::Module) {
        if (returned)
          return declarationError()
                 << "pure module local declaration after Return is unsupported";
        bool intrinsic = llvm::is_contained(
            ArrayRef<StringRef>{"module", "rule", "system", "log", "report",
                                "bool", "int", "range", "Annotated"},
            instanceName);
        if (values.contains(instanceName) || instances.contains(instanceName) ||
            parameterNames.contains(instanceName) ||
            ruleNames.contains(instanceName) || names.contains(instanceName) ||
            names.contains(instanceName) || intrinsic)
          return declarationError()
                 << "module local shadows an existing binding: '"
                 << instanceName << "'";
        auto value = expression(rhs, at, decl.symbol, values, instances);
        if (failed(value))
          return failure();
        // Naming a wire neither changes its SSA identity/facts nor creates a
        // declaration boundary at which to narrow the value.
        values[instanceName] = *value;
        localBindings.insert(instanceName);
        continue;
      }
      if (values.contains(calleeName) || parameterNames.contains(calleeName) ||
          instances.contains(calleeName) || ruleNames.contains(calleeName))
        return declarationError()
               << "module constructor is shadowed by a lexical binding: '"
               << calleeName << "'";
      if (names.contains(instanceName))
        return declarationError()
               << "module instance shadows a source binding: '" << instanceName
               << "'";
      if (instances.contains(instanceName))
        return error() << "unknown callee or duplicate instance";
      Operation *callee = binding->second.declaration;
      bool systemCallee =
          callee ? bool(callee->getAttrOfType<StringAttr>("ac.root_kind"))
                 : modules[local[calleeName]].system;
      if (systemCallee)
        return declarationError()
               << "@system is root-only and cannot be instantiated as a child";
      std::unique_ptr<Operation, void (*)(Operation *)> temporary(
          nullptr, [](Operation *op) {
            if (op)
              op->destroy();
          });
      if (!callee) {
        ModuleDecl &target = modules[local[calleeName]];
        if (target.persistent || target.behavioral)
          return error() << "persistent child instantiation is unsupported";
        OperationState state(module->getLoc(),
                             ac::ModuleImportOp::getOperationName());
        SmallVector<Attribute> targetParams;
        for (const Parameter &p : target.parameters) {
          ac::StaticExprAttr defaultValue;
          if (p.value) {
            auto value = staticExpr(*p.value, target.symbol);
            if (failed(value))
              return failure();
            defaultValue = *value;
          }
          targetParams.push_back(
              ac::getStaticParameterAttr(b, p.name, defaultValue));
        }
        state.addAttribute("parameters", b.getArrayAttr(targetParams));
        state.addAttribute("type_parameters", b.getArrayAttr({}));
        state.addAttribute("input_names", portNames(b, target.inputs));
        state.addAttribute("output_names", portNames(b, target.outputs));
        state.addAttribute("function_type", TypeAttr::get(target.type));
        for (NamedAttribute attr : target.sourceCallAttrs)
          state.addAttribute(attr.getName(), attr.getValue());
        callee = Operation::create(state);
        temporary.reset(callee);
      }
      auto actuals = bindParameters(callee, stmt.child("value"), decl.symbol);
      auto typeActuals =
          bindTypeArguments(callee, stmt.child("value"), decl.symbol);
      if (failed(actuals) || failed(typeActuals))
        return failure();
      Instance instance;
      instance.name = instanceName.str();
      instance.node = stmt;
      instance.callee = binding->second.symbol;
      instance.declaration = binding->second.declaration;
      instance.parameters = std::move(*actuals);
      instance.typeArguments = std::move(*typeActuals);
      if (auto domain =
              callee->getAttrOfType<DictionaryAttr>("ac.domain_inputs")) {
        auto clock = domain.getAs<IntegerAttr>("clock");
        auto reset = domain.getAs<IntegerAttr>("reset");
        if (!clock || !reset || clock.getValue().isNegative() ||
            reset.getValue().isNegative() ||
            clock.getValue().getActiveBits() > 32 ||
            reset.getValue().getActiveBits() > 32)
          return error() << "callee has invalid inferred domain metadata";
        instance.clockInput = clock.getValue().getZExtValue();
        instance.resetInput = reset.getValue().getZExtValue();
      }
      auto signature = cast<FunctionType>(
          callee->getAttrOfType<TypeAttr>("function_type").getValue());
      auto appendPorts = [&](StringRef namesAttribute, TypeRange types,
                             SmallVectorImpl<Port> &ports) -> LogicalResult {
        auto names = callee->getAttrOfType<ArrayAttr>(namesAttribute);
        if (!names || names.size() != types.size())
          return error() << "callee port names do not match signature";
        for (auto [index, pair] : llvm::enumerate(llvm::zip(names, types))) {
          auto [name, formalType] = pair;
          auto type = substitute(formalType, callee, instance.parameters,
                                 instance.typeArguments);
          if (failed(type))
            return failure();
          std::optional<ac::detail::ValueKind> kind;
          if (!binding->second.declaration) {
            const auto &target = modules[local[calleeName]];
            const auto &declared = namesAttribute == "input_names"
                                       ? target.inputs
                                       : target.outputs;
            kind = declared[index].sourceKind;
          } else if (binding->second.builtin &&
                     headers.isTrustedBuiltin(dyn_cast<ac::ModuleImportOp>(
                         binding->second.declaration)) &&
                     (binding->second.remote == "dff" ||
                      binding->second.remote == "dffe")) {
            if (auto formal = dyn_cast<ac::TypeParamType>(formalType)) {
              const auto call = stmt.child("value");
              for (size_t i = 0; i < call.array("keywords").size(); ++i) {
                auto keyword = call.item("keywords", i);
                auto actualName =
                    dyn_cast_or_null<StringAttr>(keyword.get("arg"));
                if (actualName == formal.getName())
                  kind = annotationKind(keyword.child("value"));
              }
            } else {
              // The trusted DFF/DFFE catalog declares only Boolean controls
              // beside its T payload. This is a leaf contract, not width
              // inference.
              kind = ac::detail::ValueKind::Boolean;
            }
          }
          ports.push_back(
              {cast<StringAttr>(name).getValue().str(), *type, stmt, kind});
        }
        return success();
      };
      if (failed(appendPorts("input_names", signature.getInputs(),
                             instance.inputs)) ||
          failed(appendPorts("output_names", signature.getResults(),
                             instance.outputs)))
        return failure();
      SmallVector<Type> resultTypes;
      for (const Port &p : instance.outputs)
        resultTypes.push_back(p.type);
      instance.op = createOp(
          at, stmt.location(b.getContext(), source.path),
          ac::InstanceOp::getOperationName(), {}, resultTypes,
          {at.getNamedAttr("instance_name", at.getStringAttr(instance.name)),
           at.getNamedAttr("callee", instance.callee),
           at.getNamedAttr("parameters", at.getArrayAttr(instance.parameters)),
           at.getNamedAttr("type_arguments",
                           at.getArrayAttr(instance.typeArguments)),
           at.getNamedAttr("occurrence", occurrence(at, decl.symbol, stmt))});
      for (auto [output, port] :
           llvm::zip(instance.op->getResults(), instance.outputs))
        remember(output, port.sourceKind);
      storage.push_back(std::move(instance));
      instances[instanceName] = &storage.back();
      continue;
    }
    if (stmt.kind() == "Expr") {
      AstNode call = stmt.child("value");
      if (call.kind() != "Call" || call.child("func").kind() != "Name" ||
          !call.array("args").empty() || !call.array("keywords").empty() ||
          !rules.contains(call.child("func").string("id")))
        return error() << "module expression must register @rule";
      registrations.push_back(call);
      continue;
    }
    if (stmt.kind() == "Return") {
      if (failed(fixedResultTargets(stmt)))
        return failure();
      if (returned)
        return error() << "multiple module returns";
      returned = stmt;
      continue;
    }
    if (stmt.kind() == "Nonlocal")
      return error() << "nonlocal persistent state is forbidden";
    return error() << "unsupported structural module statement '" << stmt.kind()
                   << "'";
  }

  for (const AstNode &registration : registrations) {
    StringRef ruleName = registration.child("func").string("id");
    AstNode rule = rules[ruleName];
    Instance *target = nullptr;
    AstNode call;
    unsigned observationCount = 0;
    // Structural rules publish their observations through the module-level
    // instrumentation pass below, so the rule result suffix reserves slots for
    // source assertions only.
    size_t checkCount = countSourceAssertions(rule, "body");
    for (size_t i = 0; i < rule.array("body").size(); ++i) {
      AstNode stmt = rule.item("body", i);
      if (docstring(stmt))
        continue;
      if (stmt.kind() == "Nonlocal")
        return error() << "nonlocal writes are forbidden";
      if (stmt.kind() == "Return" && !stmt.child("value"))
        continue;
      if (stmt.kind() == "Assert" ||
          (checkCount && (stmt.kind() == "If" || stmt.kind() == "Match")))
        continue;
      if (stmt.kind() != "Expr")
        return error()
               << "pure rules support instance binding, assert, log, and "
                  "report statements";
      AstNode candidate = stmt.child("value");
      if (candidate.kind() == "Call" &&
          candidate.child("func").kind() == "Name") {
        StringRef callee = candidate.child("func").string("id");
        if (callee == "log" || callee == "report") {
          ++observationCount;
          continue;
        }
        auto found = instances.find(callee);
        if (found != instances.end()) {
          if (target)
            return error() << "one rule binds one instance";
          target = found->second;
          call = candidate;
          continue;
        }
      }
      return error() << "unsupported expression statement in pure rule";
    }
    if (!target && !checkCount && !observationCount)
      return error() << "registered rule has no hardware behavior";
    if (target && target->bound)
      return error() << "each instance requires one complete binding rule";
    // Declared before the binding block because the module-level instrumentation
    // pass below must still see which observation sites the rule-body walker
    // deferred to it.
    RuleEvaluation evaluation;
    if (target || checkCount) {
      SmallVector<Value> captures;
      for (const auto &item : values)
        captures.push_back(item.second);
      for (const Instance &item : storage)
        llvm::append_range(captures, item.op->getResults());
      SmallVector<Type> resultTypes;
      if (target)
        for (const Port &port : target->inputs)
          resultTypes.push_back(port.type);
      for (size_t i = 0; i < 2 * checkCount; ++i)
        resultTypes.push_back(bits(1, rule, decl.symbol));
      Operation *ruleOp =
          createOp(at, rule.location(b.getContext(), source.path),
                   ac::RuleOp::getOperationName(), captures, resultTypes,
                   {at.getNamedAttr("name", at.getStringAttr(ruleName)),
                    at.getNamedAttr("occurrence",
                                    occurrence(at, decl.symbol, registration))},
                   1);
      Block *block = new Block();
      ruleOp->getRegion(0).push_back(block);
      BranchEnvironment environment;
      unsigned ordinal = 0;
      auto captureArgument = [&]() {
        return captureValue(block, captures[ordinal++], ruleOp->getLoc());
      };
      for (const auto &item : values)
        environment.values[item.first()] = captureArgument();
      llvm::StringMap<SmallVector<Value>> capturedOutputs;
      for (const Instance &item : storage)
        for (const Port &port : item.outputs) {
          (void)port;
          capturedOutputs[item.name].push_back(captureArgument());
        }
      OpBuilder inRule = OpBuilder::atBlockEnd(block);
      evaluation.structural = true;
      evaluation.instances = &instances;
      evaluation.outputs = &capturedOutputs;
      evaluation.target = target;
      evaluation.binding = call;
      activeRule = &evaluation;
      if (checkCount) {
        auto one =
            constant(bits(1, rule, decl.symbol), 1, rule, inRule, decl.symbol);
        if (failed(one))
          return failure();
        environment.ambient = environment.continuation = *one;
      }
      llvm::StringSet<> noOwners;
      AstNode ruleReturn;
      if (failed(statements(rule, "body", inRule, decl.symbol, environment,
                            noOwners, ruleReturn)))
        return failure();
      SmallVector<Value> yields = evaluation.bindingValues;
      // The declared dependent width may still be a closed expression while the
      // bound value carries the concrete instantiated type. Republish the
      // resolved binding prefix on the rule results so the yield signature
      // matches, leaving the source-check suffix positions and types untouched.
      for (size_t i = 0; i < yields.size(); ++i)
        ruleOp->getResult(i).setType(yields[i].getType());
      for (const auto &check : evaluation.checks) {
        yields.push_back(check.condition);
        yields.push_back(check.path);
      }
      if (evaluation.checks.size() != checkCount)
        return error()
               << "source assertion capture count differs from rule suffix";
      createOp(inRule, rule.location(b.getContext(), source.path),
               ac::YieldOp::getOperationName(), yields, {}, {});
      activeRule = nullptr;
      if (failed(publishSourceChecks(ruleOp, evaluation, at, decl.symbol)))
        return failure();
      if (target) {
        target->op->setOperands(
            ruleOp->getResults().take_front(target->inputs.size()));
        target->bound = true;
      }
    }
    unsigned instrumentationOrdinal = 0;
    // Every observation site the rule-body walker deferred must be published
    // here. The walker and this pass are two independent readings of the same
    // rule body; docs/reference/language.md:842 requires source observations to
    // diagnose rather than disappear, so a divergence between the two must be a
    // named error and never a silent drop.
    SmallVector<std::pair<AstNode, AstNode>, 0> deferred =
        std::move(evaluation.deferredObservations);
    for (size_t i = 0; i < rule.array("body").size(); ++i) {
      AstNode statement = rule.item("body", i);
      AstNode value =
          statement.kind() == "Expr" ? statement.child("value") : AstNode();
      StringRef callee =
          value.kind() == "Call" && value.child("func").kind() == "Name"
              ? value.child("func").string("id")
              : StringRef();
      if (statement.kind() == "Assert") {
        ++instrumentationOrdinal;
        continue;
      }
      if (callee != "log" && callee != "report")
        continue;
      if (failed(emitInstrumentation(decl, rule, statement,
                                     instrumentationOrdinal++, at, values,
                                     instances)))
        return failure();
      llvm::erase_if(deferred, [&](const std::pair<AstNode, AstNode> &site) {
        return sameSourceSite(site.second, statement);
      });
    }
    if (!deferred.empty())
      return mlir::emitError(
                 deferred.front().second.location(b.getContext(), source.path))
             << "'log'/'report' observation in a pure rule was not published";
  }
  for (const Instance &item : storage)
    if (!item.bound)
      return error() << "unbound instance '" << item.name << "'";
  if (decl.system) {
    if (returned)
      return error() << "@system cannot return a value";
    createOp(at, module->getLoc(), ac::YieldOp::getOperationName(), {}, {}, {});
    return success();
  }
  if (!returned || returned.child("value").kind() != "Dict")
    return error() << "module requires named output mapping";
  AstNode mapping = returned.child("value");
  if (mapping.array("keys").size() != decl.outputs.size())
    return error() << "module output arity mismatch";
  SmallVector<Value> outputs;
  for (size_t i = 0; i < decl.outputs.size(); ++i) {
    auto key =
        dyn_cast_or_null<StringAttr>(mapping.item("keys", i).get("value"));
    if (!key || key.getValue() != decl.outputs[i].name)
      return error() << "module output key/order mismatch";
    AstNode outputExpression = mapping.item("values", i);
    auto value = expression(outputExpression, at, decl.symbol, values,
                            instances, decl.outputs[i].type);
    if (failed(value))
      return failure();
    auto converted =
        boundary(*value, decl.outputs[i], outputExpression, at, decl.symbol);
    if (failed(converted))
      return failure();
    outputs.push_back(*converted);
  }
  createOp(at, returned.location(b.getContext(), source.path),
           ac::YieldOp::getOperationName(), outputs, {}, {});
  return success();
}

LogicalResult Importer::preflightProposals(const AstNode &module) {
  // Index exact registration/owner occurrences once. A large shared grant DAG
  // must not pay a full proposal scan for every pair.
  llvm::DenseMap<DictionaryAttr,
                 llvm::DenseMap<DictionaryAttr, const OwnerProposal *>>
      endpoints;
  auto site = [&](const AstNode &node) -> DictionaryAttr {
    if (!grantProof->charge(1 + node.path.size()))
      return {};
    return occurrence(b, activeBehavioralDecl->symbol, node);
  };
  auto witnessFor = [&](const OwnerProposal &proposal,
                        size_t pathIndex) -> const AddressWitness * {
    const AddressWitness *found = nullptr;
    const auto &path = proposal.write->paths[pathIndex];
    for (const auto &witness : proposal.domains) {
      if (!grantProof->charge(1 + witness.assignment.path.size() +
                              witness.occurrence.path.size()))
        return nullptr;
      if (witness.pathIndex != pathIndex)
        continue;
      if (found || witness.ownerIndex != proposal.ownerIndex ||
          witness.formal != proposal.write->formal || !witness.index ||
          !witness.mask || !isa<ac::BitsType>(witness.index.getType()) ||
          witness.mask.getType() != witness.maskType ||
          !sameSourceSite(witness.assignment, path.assignment) ||
          path.selectors.empty() ||
          path.selectors.front().kind != SourceWriteSelector::Kind::Index ||
          !sameSourceSite(witness.occurrence, path.selectors.front().occurrence))
        return nullptr;
      found = &witness;
    }
    return found;
  };
  for (const auto &proposal : ownerProposals) {
    size_t indexedPaths = 0;
    for (auto [pathIndex, path] : llvm::enumerate(proposal.write->paths)) {
      if (!grantProof->charge())
        return error() << grantProof->failureReason();
      if (!path.selectors.empty() &&
          path.selectors.front().kind == SourceWriteSelector::Kind::Index) {
        ++indexedPaths;
        if (!witnessFor(proposal, pathIndex)) {
          if (!grantProof->failureReason().empty())
            return error() << grantProof->failureReason();
          return error() << "missing or invalid exact assignment-time address witness";
        }
      }
    }
    if (proposal.domains.size() != indexedPaths)
      return error() << "address witness count differs from retained indexed paths";
    auto call = site(proposal.callPlan->call),
         owner = site(proposal.write->owner);
    if (!call || !owner)
      return error() << grantProof->failureReason();
    if (!grantProof->charge())
      return error() << grantProof->failureReason();
    if (!endpoints[call].try_emplace(owner, &proposal).second)
      return error()
             << "owner-enable proof has duplicate exact proposal endpoints";
  }
  for (auto [index, pair] : llvm::enumerate(ruleWrites.pendingPairs())) {
    if (!grantProof->charge(1 + pair.module.path.size() + module.path.size()))
      return error() << grantProof->failureReason();
    if (!sameSourceSite(pair.module, module))
      continue;
    auto endpoint = [&](const AstNode &call) -> const OwnerProposal * {
      auto callSite = site(call), ownerSite = site(pair.owner);
      if (!callSite || !ownerSite || !grantProof->charge())
        return nullptr;
      auto foundCall = endpoints.find(callSite);
      if (foundCall == endpoints.end())
        return nullptr;
      if (!grantProof->charge())
        return nullptr;
      auto foundOwner = foundCall->second.find(ownerSite);
      return foundOwner == foundCall->second.end() ? nullptr
                                                   : foundOwner->second;
    };
    auto first = endpoint(pair.firstCall), second = endpoint(pair.secondCall);
    if (!first || !second) {
      if (!grantProof->failureReason().empty())
        return error() << grantProof->failureReason();
      return error() << "pending owner-enable proof is missing an exact "
                        "proposal endpoint";
    }
    auto proof = grantProof->disjoint(first->enable, second->enable);
    if (failed(proof))
      return error() << grantProof->failureReason();
    if (!*proof) {
      // Source Phase A already removed structurally/static-disjoint owner pairs.
      // Recheck its path relation here for each pair, including all later writes.
      if (!grantProof->chargeProduct(first->write->paths.size(),
                                     second->write->paths.size()))
        return error() << grantProof->failureReason();
      for (auto [firstIndex, left] : llvm::enumerate(first->write->paths))
        for (auto [secondIndex, right] : llvm::enumerate(second->write->paths)) {
          if (!grantProof->charge(left.selectors.size()) ||
              !grantProof->charge(right.selectors.size()))
            return error() << grantProof->failureReason();
          bool overlap = true;
          for (auto [a, c] : llvm::zip(left.selectors, right.selectors)) {
            if (a.kind != c.kind)
              break;
            if ((a.kind == SourceWriteSelector::Kind::Field &&
                 a.field != c.field) ||
                (a.kind == SourceWriteSelector::Kind::Index &&
                 a.directConstantInteger && c.directConstantInteger &&
                 a.directConstantInteger != c.directConstantInteger)) {
              overlap = false;
              break;
            }
          }
          if (!overlap)
            continue;
          const auto *a = witnessFor(*first, firstIndex);
          const auto *c = witnessFor(*second, secondIndex);
          bool separated = false;
          if (a && c) {
            auto addressProof = grantProof->disjointAtAddress(
                first->enable, second->enable, a->index, c->index);
            if (failed(addressProof))
              return error() << grantProof->failureReason();
            separated = *addressProof;
          } else if (!grantProof->failureReason().empty())
            return error() << grantProof->failureReason();
          if (separated)
            continue;
          auto diagnostic = mlir::emitError(
              right.assignment.location(b.getContext(), source.path));
          diagnostic << "overlapping state writes across rule registrations: owner "
                        "enables are not proven mutually exclusive and addresses "
                        "are not proven separated";
          diagnostic.attachNote(
              left.assignment.location(b.getContext(), source.path))
              << "previous overlapping write is here";
          diagnostic.attachNote(
              pair.firstCall.location(b.getContext(), source.path))
              << "previous rule registration is here";
          diagnostic.attachNote(
              pair.secondCall.location(b.getContext(), source.path))
              << "conflicting rule registration is here";
          return failure();
        }
    }
    if (!grantProof->charge())
      return error() << grantProof->failureReason();
    if (!dischargedPairs.insert(index).second)
      return error()
             << "pending owner-enable proof was discharged more than once";
  }
  return success();
}

LogicalResult
Importer::preflightQueryCallback(const AstNode &callback,
                                 const llvm::StringMap<Value> &values,
                                 SmallVectorImpl<StringRef> &captures) {
  auto diagnostic = [&] {
    return mlir::emitError(callback.location(b.getContext(), source.path));
  };
  if (failed(validateTableQueryLambda(callback, diagnostic)))
    return failure();
  auto args = callback.child("args");
  auto parameter = args.array("args").empty() ? args.item("posonlyargs", 0)
                                              : args.item("args", 0);
  auto savedArgument =
      std::exchange(queryArgument, parameter.string("arg").str());
  auto restore = llvm::scope_exit([&] { queryArgument = savedArgument; });
  llvm::StringSet<> captured;
  auto visit = [&](auto &&self, const AstNode &node) -> LogicalResult {
    std::string reason;
    if (!queryBudget.enter(reason))
      return diagnostic() << reason;
    auto leave = llvm::scope_exit([&] { queryBudget.leave(); });
    TableQueryCharge charge;
    TableQueryBudget::add(charge, TableQueryResource::Work, 1);
    StringRef name = node.kind() == "Name" ? node.string("id") : StringRef();
    bool capture = !name.empty() && name != queryArgument &&
                   values.contains(name) && !captured.contains(name);
    if (capture) {
      TableQueryBudget::add(charge, TableQueryResource::Work, 1);
      TableQueryBudget::add(charge, TableQueryResource::Slots, 1);
    }
    if (node.kind() == "Constant") {
      auto encoded = dyn_cast_or_null<DictionaryAttr>(node.get("value"));
      auto spelling =
          encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
      if (spelling &&
          !TableQueryBudget::add(charge, TableQueryResource::ConstantBytes,
                                 spelling.getValue().size()))
        return diagnostic() << "Table query constant bytes budget exhausted";
    }
    if (!queryBudget.reserve(charge, reason))
      return diagnostic() << reason;
    if (capture) {
      captured.insert(name);
      captures.push_back(name);
    }
    if (llvm::is_contained(ArrayRef<StringRef>{"NamedExpr", "Lambda", "Await",
                                               "Yield", "YieldFrom", "ListComp",
                                               "SetComp", "DictComp",
                                               "GeneratorExp", "List", "Set",
                                               "Dict", "Tuple", "Starred"},
                           node.kind()))
      return mlir::emitError(node.location(b.getContext(), source.path))
             << "Table query callbacks require closed pure scalar expressions";
    if (node.kind() == "Call") {
      auto function = node.child("func");
      bool pure = false;
      for (StringRef name : {"concat", "popcount", "count_leading_zeros",
                             "count_trailing_zeros", "enum_to_bits"})
        pure |= intrinsic(function, name);
      auto binding = lookupBinding(function);
      pure |= binding && binding->category == BindingCategory::Struct &&
              structs.contains(function.string("id"));
      if (!pure)
        return mlir::emitError(node.location(b.getContext(), source.path))
               << "Table query callback call must resolve to a pure scalar "
                  "intrinsic";
    }
    for (NamedAttribute field : node.fields()) {
      auto name = field.getName().getValue();
      if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
        if (child.getAs<StringAttr>("kind") &&
            failed(self(self, node.child(name))))
          return failure();
      } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
        for (size_t i = 0; i < children.size(); ++i)
          if (auto child = dyn_cast<DictionaryAttr>(children[i]);
              child && child.getAs<StringAttr>("kind") &&
              failed(self(self, node.item(name, i))))
            return failure();
    }
    return success();
  };
  return visit(visit, callback.child("body"));
}

FailureOr<SmallVector<Value>> Importer::tableQueryResults(
    const AstNode &call, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances) {
  auto diagnostic = [&] {
    return mlir::emitError(call.location(b.getContext(), source.path));
  };
  auto function = call.child("func");
  StringRef method = function.string("attr");
  if (function.kind() != "Attribute" ||
      (method != "first" && method != "argmin") || !call.array("args").empty())
    return diagnostic()
           << "Table query requires keyword-only where/key callbacks";
  AstNode predicate, key;
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    auto keyword = call.item("keywords", i);
    if (keyword.string("arg") == "where" && !predicate)
      predicate = keyword.child("value");
    else if (method == "argmin" && keyword.string("arg") == "key" && !key)
      key = keyword.child("value");
    else
      return diagnostic() << "Table query has an invalid or duplicate keyword";
  }
  if (!predicate || (method == "argmin" && !key))
    return diagnostic() << "Table query requires where and argmin requires key";
  auto receiver =
      expression(function.child("value"), at, symbol, values, instances);
  if (failed(receiver))
    return failure();
  auto type = dyn_cast<ac::TableType>(receiver->getType());
  ac::HardwareAnalysis analysis(*body);
  auto extent = type ? analysis.getTableShape(type, {}, body->getOperation())
                     : FailureOr<SmallVector<uint64_t>>(failure());
  if (failed(extent) || extent->size() != 1 || !extent->front())
    return diagnostic()
           << "Table query requires a positive closed one-dimensional Table";
  TableQueryCharge occurrenceCharge;
  TableQueryBudget::add(occurrenceCharge, TableQueryResource::Occurrences, 1);
  std::string reason;
  if (!queryBudget.reserve(occurrenceCharge, reason))
    return diagnostic() << reason;
  TableQueryHooks hooks{
      [&](uint64_t value) { return literal(value, call, symbol); },
      [&](uint64_t width) { return bits(width, call, symbol); },
      [&](Value guard, Value yes, Value no, OpBuilder &inside) {
        return select(guard, yes, no, call, inside, symbol);
      },
      [&](Value value, Type type, OpBuilder &inside) {
        return boundaryBitsIdentity(value, type, call, inside, symbol);
      },
      [&](Value original, Value mapped) {
        auto info = valueInfo(original);
        remember(mapped, info.sourceKind, info.interval,
                 info.closedSourceConstant);
        if (fixedValues.contains(original))
          fixedValues.insert(mapped);
        if (arithmeticValues.contains(original))
          arithmeticValues.insert(mapped);
      },
      [&](Value value) {
        numericValues.erase(value);
        fixedValues.erase(value);
        arithmeticValues.erase(value);
      }};
  TableQueryLowering lowering(at, call.location(b.getContext(), source.path),
                              *body, queryBudget, hooks);
  SmallVector<StringRef> predicateCaptures, keyCaptures;
  if (failed(lowering.capture(*receiver)) ||
      failed(preflightQueryCallback(predicate, values, predicateCaptures)) ||
      (key && failed(preflightQueryCallback(key, values, keyCaptures))))
    return failure();
  auto callback = [&](const AstNode &lambda, bool isKey,
                      ArrayRef<StringRef> captures) -> FailureOr<Value> {
    auto args = lambda.child("args");
    auto parameter = args.array("args").empty() ? args.item("posonlyargs", 0)
                                                : args.item("args", 0);
    TableQueryCharge binding;
    TableQueryBudget::add(binding, TableQueryResource::Work, 1);
    TableQueryBudget::add(binding, TableQueryResource::Slots, 1);
    if (!queryBudget.reserve(binding, reason))
      return diagnostic() << reason;
    auto loc = lambda.location(b.getContext(), source.path);
    auto temporary =
        createOp(at, loc, ac::TableMapOp::getOperationName(), {*receiver}, {},
                 {at.getNamedAttr("shape", type.getShape()),
                  at.getNamedAttr("operandSegmentSizes",
                                  at.getDenseI32ArrayAttr({1, 0}))},
                 1);
    auto block = new Block();
    temporary->getRegion(0).push_back(block);
    auto row = block->addArgument(type.getElementType(), loc);
    if (isa<ac::BitsType>(row.getType()))
      fixedValues.insert(row);
    auto cleanup = llvm::scope_exit([&] {
      for (Value value : block->getArguments()) {
        numericValues.erase(value);
        fixedValues.erase(value);
        arithmeticValues.erase(value);
      }
      for (Operation &operation : *block)
        for (Value value : operation.getResults()) {
          numericValues.erase(value);
          fixedValues.erase(value);
          arithmeticValues.erase(value);
        }
      temporary->erase();
    });
    llvm::StringMap<Value> environment;
    for (StringRef name : captures)
      environment.try_emplace(name, values.lookup(name));
    environment.try_emplace(parameter.string("arg"), row);
    auto savedArgument =
        std::exchange(queryArgument, parameter.string("arg").str());
    auto restore = llvm::scope_exit([&] { queryArgument = savedArgument; });
    auto inside = OpBuilder::atBlockEnd(block);
    auto result = expression(lambda.child("body"), inside, symbol, environment,
                             instances);
    if (failed(result))
      return failure();
    if (isKey) {
      if (!isa<ac::BitsType>(result->getType()) ||
          !fixedValues.contains(*result))
        return diagnostic()
               << "Table argmin key requires authoritative unsigned fixed bits";
    } else if (failed(predicateView(*result, lambda.child("body"), symbol)))
      return failure();
    auto staged = lowering.stage(*block, row, *receiver, {*result});
    if (failed(staged))
      return failure();
    return staged->front();
  };
  auto predicates = callback(predicate, false, predicateCaptures);
  if (failed(predicates))
    return failure();
  Value keys;
  if (key) {
    auto result = callback(key, true, keyCaptures);
    if (failed(result))
      return failure();
    keys = *result;
  }
  auto results = lowering.choose(*predicates, keys);
  if (failed(results))
    return failure();
  for (Value result : *results)
    fixedValues.insert(result);
  remember(results->front(), std::nullopt,
           IntegerInterval{llvm::APSInt::getUnsigned(0),
                           llvm::APSInt::getUnsigned(extent->front())});
  return results;
}

LogicalResult Importer::prepare() {
  if (failed(scanImports()) || failed(scanNominals()))
    return failure();
  SmallVector<Attribute> interfaces{owner};
  for (auto dependency : dependencies)
    if (dependency != owner)
      interfaces.push_back(dependency);
  Location loc = source.module.location(b.getContext(), source.path);
  body = unit(b.getContext(), loc, owner, "declarations",
              b.getArrayAttr(interfaces));
  if (failed(stageImports()) || failed(stageEnums()) || failed(scanStructs()))
    return failure();
  for (auto &item : structs) {
    b.setInsertionPointToEnd(body->getBody());
    createOp(
        b, item.second.location(b.getContext(), source.path),
        ac::StructOp::getOperationName(), {}, {},
        {b.getNamedAttr("sym_name", b.getStringAttr(qualify(item.first()))),
         b.getNamedAttr("fields", structFields[item.first()]),
         b.getNamedAttr("ac.source_owner", owner),
         b.getNamedAttr("ac.origin",
                        occurrence(b,
                                   FlatSymbolRefAttr::get(
                                       b.getContext(), qualify(item.first())),
                                   item.second)),
         b.getNamedAttr("ac.declaration_role", b.getStringAttr("definition"))});
  }
  if (failed(scanModules()) || failed(prepareMemoryBindings()) ||
      failed(inferModuleGraph()))
    return failure();
  (*body)->setAttr(
      "ac.unit_kind",
      b.getStringAttr(modules.empty() ? "declarations" : "implementation"));
  llvm::DenseSet<Attribute> cloned;
  for (Operation &declaration : body->getBody()->getOperations())
    if (auto name = SymbolTable::getSymbolName(&declaration))
      cloned.insert(FlatSymbolRefAttr::get(b.getContext(), name.getValue()));
  if (llvm::any_of(modules,
                   [](const ModuleDecl &decl) { return decl.persistent; })) {
    auto builtin = headers.lookupBuiltin("dffe");
    if (!builtin || !headers.isTrustedBuiltin(builtin.declaration))
      return error() << "trusted dffe unavailable";
    if (cloned.insert(builtin.symbol).second) {
      auto *copy = builtin.declaration->clone();
      copy->setAttr("ac.declaration_role", b.getStringAttr("import_snapshot"));
      body->getBody()->push_back(copy);
    }
  }
  ac::HardwareAnalysis declarationAnalysis(*body);
  for (auto &item : structs)
    if (failed(declarationAnalysis.getPackedWidth(
            ac::StructType::get(b.getContext(),
                                b.getStringAttr(qualify(item.first()))),
            {}, body->getOperation())))
      return failure();
  if (failed(validateDefaults()))
    return failure();
  for (const ModuleDecl &decl : modules)
    domainPlans.push_back({decl.node, decl.symbol, decl.type,
                          decl.sourceCallAttrs, decl.needsDomain});
  if (failed(verify(body->getOperation()))) return failure();
  return success(!diagnosticFailed);
}
FailureOr<OwningOpRef<ModuleOp>> Importer::lower() {
  grantProof = std::make_unique<OwnerEnableProof>(*body, ruleWrites.spentWork());
  for (ModuleDecl &decl : modules)
    if (failed(emitModule(decl)))
      return failure();
  if (dischargedPairs.size() != ruleWrites.pendingPairs().size())
    return error() << "pending owner-enable proof ledger is incomplete";
  SmallVector<StringRef> ordered;
  for (const auto &item : names)
    ordered.push_back(item.first());
  llvm::sort(ordered);
  SmallVector<Attribute> exports, imports;
  for (StringRef name : ordered) {
    const Import &item = names[name];
    if (item.category != BindingCategory::Module &&
        item.category != BindingCategory::Struct &&
        item.category != BindingCategory::Enum)
      continue;
    exports.push_back(b.getDictionaryAttr(
        {b.getNamedAttr("name", b.getStringAttr(name)),
         b.getNamedAttr("target", item.symbol),
         b.getNamedAttr("site", namespaceSite(item.site))}));
    if (item.declaration && item.provider && !item.builtin)
      imports.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("source", item.provider),
           b.getNamedAttr("name", b.getStringAttr(item.remote)),
           b.getNamedAttr("target", item.symbol),
           b.getNamedAttr("site", namespaceSite(item.site))}));
  }
  llvm::sort(exports, [](Attribute a, Attribute c) {
    return cast<DictionaryAttr>(a).getAs<StringAttr>("name").getValue() <
           cast<DictionaryAttr>(c).getAs<StringAttr>("name").getValue();
  });
  llvm::sort(imports, [](Attribute a, Attribute c) {
    auto left = cast<DictionaryAttr>(a), right = cast<DictionaryAttr>(c);
    for (StringRef field : {"source", "name"}) {
      int order = ac::detail::compareClosedSourceStructure(left.get(field),
                                                           right.get(field));
      if (order)
        return order < 0;
    }
    auto leftSite = left.getAs<DictionaryAttr>("site");
    auto rightSite = right.getAs<DictionaryAttr>("site");
    int path = ac::detail::compareClosedSourceStructure(
        leftSite.get("ast_path"), rightSite.get("ast_path"));
    if (path)
      return path < 0;
    auto leftLocation = leftSite.getAs<DictionaryAttr>("location");
    auto rightLocation = rightSite.getAs<DictionaryAttr>("location");
    int file = ac::detail::compareClosedSourceStructure(
        leftLocation.get("path"), rightLocation.get("path"));
    if (file)
      return file < 0;
    for (StringRef coordinate : {"line", "column", "end_line", "end_column"}) {
      int order = ac::detail::compareClosedSourceStructure(
          leftLocation.get(coordinate), rightLocation.get(coordinate));
      if (order)
        return order < 0;
    }
    return ac::detail::compareClosedSourceStructure(left.get("target"),
                                                    right.get("target")) < 0;
  });
  (*body)->setAttr("ac.exports", b.getArrayAttr(exports));
  (*body)->setAttr("ac.import_bindings", b.getArrayAttr(imports));
  return std::move(body);
}

} // namespace

std::unique_ptr<SourceDeclarationContext> createSourceDeclarationContext(
    const CapturedSource &source, const SourceCompilationInputs &inputs,
    const SourceRuleWritesAnalysis &writes, Operation *transport) {
  return std::make_unique<SourceDeclarationContext>(std::make_unique<Importer>(
      source, inputs.owner, *inputs.headers, writes,
      [transport] { return transport->emitError(); }));
}

namespace {
class LowerPythonSourcePass
    : public PassWrapper<LowerPythonSourcePass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerPythonSourcePass)
  LowerPythonSourcePass(DictionaryAttr owner,
                        const SourceHeaderRegistry &headers)
      : owner(owner), headers(&headers) {}
  LowerPythonSourcePass(const LowerPythonSourcePass &other)
      : PassWrapper(other), owner(other.owner), headers(other.headers) {}
  StringRef getArgument() const final {
    return "ac-lower-python-source";
  }
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<ac::ACIRDialect>();
  }
  void runOnOperation() override {
    auto transport = getOperation();
    auto &bindings = getAnalysis<SourceMemoryBindingsAnalysis>();
    auto result = bindings.lower(owner, *headers);
    if (failed(result) || failed(verify(**result))) {
      signalPassFailure();
      return;
    }
    transport->setAttrs((*result)->getOperation()->getAttrs());
    transport->setLoc((*result)->getLoc());
    transport.getBody()->getOperations().clear();
    transport.getBody()->getOperations().splice(
        transport.getBody()->end(), (*result)->getBody()->getOperations());
  }

private:
  DictionaryAttr owner;
  const SourceHeaderRegistry *headers;
};
} // namespace
std::unique_ptr<Pass>
createLowerPythonSourcePass(DictionaryAttr owner,
                            const SourceHeaderRegistry &headers) {
  return std::make_unique<LowerPythonSourcePass>(owner, headers);
}

} // namespace acir::compiler::detail
