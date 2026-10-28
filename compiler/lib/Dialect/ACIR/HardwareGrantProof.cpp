#include "HardwareGrantProof.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include <string>
#include <limits>

using namespace mlir;
namespace acir::ac::detail {
struct HardwareGrantProof::Impl {
  Impl(Canonicalize canonicalize, Inspect inspect, Charge debit)
      : canonicalize(std::move(canonicalize)), inspect(std::move(inspect)),
        debit(std::move(debit)) {}
  bool charge(uint64_t amount = 1) {
    if (!reason.empty()) return false;
    if (!debit || !debit(amount)) {
      reason = "owner-enable proof work budget exhausted";
      return false;
    }
    return true;
  }
  struct Fact {
    Node value;
    bool polarity;
    Attribute constant;
    uint64_t width = 0;
  };
  struct Result {
    enum Kind { Facts, Impossible, Failure } kind = Facts;
    SmallVector<Fact> facts;
  };
  Result fail(StringRef message) {
    if (reason.empty()) reason = message.str();
    return {Result::Failure, {}};
  }
  FailureOr<Node> canonical(Node node, unsigned depth) {
    if (!charge()) return failure();
    if (depth >= 128 || !canonicalize) {
      fail("owner-enable proof recursion or adapter contract failed");
      return failure();
    }
    if (node >= std::numeric_limits<Node>::max() - 1) {
      fail("owner-enable proof adapter uses a reserved node identity");
      return failure();
    }
    auto canonical = canonicalize(node, depth);
    if (failed(canonical)) return failure();
    if (*canonical >= std::numeric_limits<Node>::max() - 1) {
      fail("owner-enable proof adapter returns a reserved node identity");
      return failure();
    }
    return *canonical;
  }
  FailureOr<const NodeView *> describe(Node node) {
    if (!charge()) return failure();
    if (node >= std::numeric_limits<Node>::max() - 1) {
      fail("owner-enable proof adapter uses a reserved node identity");
      return failure();
    }
    if (auto found = views.find(node); found != views.end()) return &found->second;
    if (!inspect) return failure();
    auto view = inspect(node);
    if (failed(view)) return failure();
    unsigned count = view->kind == NodeView::Raw ? 0 : view->kind == NodeView::Not ? 1 :
                     view->kind == NodeView::Select ? 3 : 2;
    if (view->operands.size() != count) {
      fail("owner-enable proof adapter has invalid node arity");
      return failure();
    }
    if (view->fullyKnown && (!view->width || !*view->width ||
          view->fullyKnown->width != *view->width || !view->fullyKnown->value)) {
      fail("owner-enable proof adapter has invalid known pattern");
      return failure();
    }
    if (view->fullyKnown) {
      llvm::APSInt bits(view->fullyKnown->value.getCanonicalValue());
      if (bits.isNegative() || bits.getActiveBits() > *view->width) {
        fail("owner-enable proof known pattern exceeds its width");
        return failure();
      }
    }
    if (!charge(1 + view->operands.size())) return failure();
    return &views.try_emplace(node, std::move(*view)).first->second;
  }
  FailureOr<std::optional<uint64_t>> width(Node value) {
    auto view = describe(value);
    if (failed(view)) return failure();
    return (*view)->width;
  }
  FailureOr<std::optional<Constant>> constant(Node value, unsigned depth) {
    auto resolved = canonical(value, depth);
    if (failed(resolved)) return failure();
    auto view = describe(*resolved);
    if (failed(view)) return failure();
    return (*view)->fullyKnown;
  }
  FailureOr<bool> disjoint(Node first, Node second) {
    if (!charge()) return failure();
    auto left = required(first, true, 0);
    auto right = required(second, true, 0);
    auto together = conjunction(left, right);
    if (together.kind == Result::Failure) return failure();
    return together.kind == Result::Impossible;
  }
  FailureOr<bool> disjointAtAddress(Node first, Node second, Node firstIndex, Node secondIndex) {
    auto left = canonical(firstIndex, 0), right = canonical(secondIndex, 0);
    if (failed(left) || failed(right)) return failure();
    auto leftConstant = constant(*left, 0), rightConstant = constant(*right, 0);
    auto leftWidth = width(*left), rightWidth = width(*right);
    if (failed(leftConstant) || failed(rightConstant) || failed(leftWidth) || failed(rightWidth))
      return failure();
    if (!*leftWidth || !*rightWidth) {
      fail("address proof requires typed Bits indices");
      return failure();
    }
    // Different known addresses cannot collide. Equal addresses still permit
    // disjoint writers when their actual enables are mutually exclusive.
    if (*leftConstant && *rightConstant &&
        !equalConstants(**leftConstant, **rightConstant)) return true;
    if (!*leftConstant && !*rightConstant && **leftWidth != **rightWidth) return false;
    collision = std::pair<Node, Node>{*left, *right};
    auto result = disjoint(first, second);
    collision.reset();
    return result;
  }
  bool equalConstants(const Constant &a, const Constant &c) {
    // Fully known patterns compare as mathematical unsigned values, allowing
    // lossless padding while never truncating an unknown or a high bit.
    return llvm::APSInt::compareValues(
               llvm::APSInt(cast<MathIntAttr>(a.value).getCanonicalValue()),
               llvm::APSInt(
                   cast<MathIntAttr>(c.value).getCanonicalValue())) == 0;
  }
  FailureOr<bool> sameAddress(Node value, Node endpoint, unsigned depth) {
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
  FailureOr<bool> collisionComparison(Node lhs, Node rhs,
                                      unsigned depth) {
    if (!collision)
      return false;
    auto a = sameAddress(lhs, collision->first, depth);
    auto c = sameAddress(rhs, collision->second, depth);
    if (failed(a) || failed(c))
      return failure();
    if (*a && *c)
      return true;
    a = sameAddress(lhs, collision->second, depth);
    c = sameAddress(rhs, collision->first, depth);
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
  Result required(Node original, bool polarity, unsigned depth) {
    if (!charge())
      return {Result::Failure, {}};
    if (depth >= 128)
      return fail("owner-enable proof recursion budget exhausted");
    auto resolved = canonical(original, depth);
    if (failed(resolved))
      return fail("owner-enable proof normalization failed");
    Node value = *resolved;
    auto key = std::make_pair(unsigned(collision.has_value()),
        std::make_pair(collision.value_or(std::pair<Node, Node>{0, 0}),
                       std::make_pair(value, unsigned(polarity))));
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
    auto inspected = describe(value);
    if (failed(inspected)) return fail("owner-enable proof node inspection failed");
    NodeView view = **inspected;
    if (!charge(view.operands.size())) return {Result::Failure, {}};
    auto code = constant(value, depth);
    if (failed(code))
      return fail("owner-enable proof constant analysis failed");
    if (*code) {
      bool truth =
          !llvm::APSInt(
               cast<MathIntAttr>((**code).value).getCanonicalValue())
               .isZero();
      if (truth != polarity)
        result.kind = Result::Impossible;
    } else if (view.kind == NodeView::Not) {
      result = required(view.operands[0], !polarity, depth + 1);
    } else if (view.kind == NodeView::And || view.kind == NodeView::Or) {
      auto left = required(view.operands[0], polarity, depth + 1);
      auto right = required(view.operands[1], polarity, depth + 1);
      bool both = (view.kind == NodeView::And) == polarity;
      result = both ? conjunction(left, right) : alternatives(left, right);
    } else if (view.kind == NodeView::Select) {
      auto yes = canonical(view.operands[1], depth + 1);
      auto no = canonical(view.operands[2], depth + 1);
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
                      cast<MathIntAttr>(c.value).getCanonicalValue())
                      .isZero();
        };
        if (*yesConstant && *noConstant &&
            truth(**yesConstant) == truth(**noConstant)) {
          if (truth(**yesConstant) != polarity)
            result.kind = Result::Impossible;
        } else if (*yesConstant && *noConstant) {
          result = required(view.operands[0],
                            truth(**yesConstant) == polarity, depth + 1);
        } else if (*noConstant) {
          bool noTruth = truth(**noConstant);
          auto guard =
              required(view.operands[0], polarity != noTruth, depth + 1);
          auto branch = required(*yes, polarity, depth + 1);
          result = polarity != noTruth ? conjunction(guard, branch)
                                       : alternatives(guard, branch);
        } else if (*yesConstant) {
          bool yesTruth = truth(**yesConstant);
          auto guard =
              required(view.operands[0], polarity == yesTruth, depth + 1);
          auto branch = required(*no, polarity, depth + 1);
          result = polarity != yesTruth ? conjunction(guard, branch)
                                        : alternatives(guard, branch);
        } else {
          auto guardTrue = required(view.operands[0], true, depth + 1);
          auto guardFalse = required(view.operands[0], false, depth + 1);
          auto yesFacts = required(*yes, polarity, depth + 1);
          auto noFacts = required(*no, polarity, depth + 1);
          auto selectedTrue = conjunction(guardTrue, yesFacts);
          auto selectedFalse = conjunction(guardFalse, noFacts);
          // An X/Z selector also produces a known result when both arms are
          // known and equal. Keep this alternative, including its failures.
          auto common = conjunction(yesFacts, noFacts);
          result =
              alternatives(alternatives(selectedTrue, selectedFalse), common);
        }
      }
    } else if (view.kind == NodeView::Eq || view.kind == NodeView::Ne) {
      auto inCollision = collisionComparison(view.operands[0], view.operands[1], depth + 1);
      if (failed(inCollision))
        return fail("address proof equality endpoint analysis failed");
      bool equal = (view.kind == NodeView::Eq) == polarity;
      if (*inCollision && !equal) {
        // Possible-address collision forbids known inequality. It does not
        // imply known equality: both equality predicates may still be X.
        result.kind = Result::Impossible;
      }
      auto left = constant(view.operands[0], depth + 1);
      auto right = constant(view.operands[1], depth + 1);
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
            canonical(*left ? view.operands[1] : view.operands[0], depth + 1);
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
  Canonicalize canonicalize;
  Inspect inspect;
  Charge debit;
  std::string reason;
  std::optional<std::pair<Node, Node>> collision;
  using QueryKey = std::pair<unsigned, std::pair<std::pair<Node, Node>, std::pair<Node, unsigned>>>;
  llvm::DenseMap<QueryKey, Result> cache;
  llvm::DenseSet<QueryKey> visiting;
  llvm::DenseMap<Node, NodeView> views;
};
HardwareGrantProof::HardwareGrantProof(Canonicalize canonicalize, Inspect inspect, Charge charge)
    : impl(std::make_unique<Impl>(std::move(canonicalize), std::move(inspect), std::move(charge))) {}
HardwareGrantProof::~HardwareGrantProof() = default;
FailureOr<bool> HardwareGrantProof::disjoint(Node a, Node b) { return impl->disjoint(a,b); }
FailureOr<bool> HardwareGrantProof::disjointAtAddress(Node a, Node b, Node i, Node j) {
  return impl->disjointAtAddress(a,b,i,j);
}
StringRef HardwareGrantProof::failureReason() const { return impl->reason; }
} // namespace acir::ac::detail
