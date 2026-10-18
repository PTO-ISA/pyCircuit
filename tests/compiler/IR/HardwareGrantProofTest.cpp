#include "Dialect/ACIR/HardwareGrantProof.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "mlir/IR/MLIRContext.h"
#include "llvm/ADT/APSInt.h"
#include "gtest/gtest.h"
#include <array>

namespace {
using acir::ac::detail::HardwareGrantProof;
using Node = HardwareGrantProof::Node;
using View = HardwareGrantProof::NodeView;
class HardwareGrantProofTest : public ::testing::Test {
protected:
  mlir::MLIRContext context;
  llvm::SmallVector<View> nodes;
  void SetUp() override { context.loadDialect<acir::ac::ACIRDialect>(); }
  Node raw(uint64_t width = 1) {
    View v; v.width = width; nodes.push_back(v); return nodes.size() - 1;
  }
  Node constant(unsigned value, unsigned width = 1) {
    View v; v.width = width;
    v.fullyKnown = HardwareGrantProof::Constant{
        acir::ac::MathIntAttr::get(&context, llvm::APSInt(llvm::APInt(width, value), true)), width};
    nodes.push_back(v); return nodes.size() - 1;
  }
  Node op(View::Kind kind, std::initializer_list<Node> args) {
    View v; v.kind = kind; v.operands.assign(args); v.width = 1;
    nodes.push_back(v); return nodes.size() - 1;
  }
  std::unique_ptr<HardwareGrantProof> proof(uint64_t allowance = 100000) {
    return std::make_unique<HardwareGrantProof>(
        [this](Node node, unsigned) -> mlir::FailureOr<Node> {
          if (node >= nodes.size()) return mlir::failure();
          return node;
        },
        [this](Node node) -> mlir::FailureOr<View> {
          if (node >= nodes.size()) return mlir::failure();
          return nodes[node];
        },
        [left = allowance](uint64_t units) mutable {
          if (units > left) return false;
          left -= units; return true;
        });
  }
};
TEST_F(HardwareGrantProofTest, BoundNodeIdentityIncludesZeroAndDistinctInvocations) {
  Node first = raw(), second = raw();
  Node notFirst = op(View::Not, {first}), notSecond = op(View::Not, {second});
  auto p = proof();
  auto a = p->disjoint(first, notFirst), b = p->disjoint(first, notSecond);
  ASSERT_TRUE(mlir::succeeded(a)); ASSERT_TRUE(mlir::succeeded(b));
  EXPECT_TRUE(*a); EXPECT_FALSE(*b);
}
TEST_F(HardwareGrantProofTest, EqualSelectArmsDoNotEstablishSelectorKnownness) {
  Node selector = raw(), one = constant(1), zero = constant(0);
  Node always = op(View::Select, {selector, one, one});
  Node inverse = op(View::Not, {selector});
  Node gated = op(View::Select, {selector, one, zero});
  auto p = proof();
  auto equalArms = p->disjoint(always, inverse);
  auto complementary = p->disjoint(gated, inverse);
  ASSERT_TRUE(mlir::succeeded(equalArms)); ASSERT_TRUE(mlir::succeeded(complementary));
  EXPECT_FALSE(*equalArms); EXPECT_TRUE(*complementary);
}
TEST_F(HardwareGrantProofTest, PossibleAddressCollisionRetainsUnknownEquality) {
  // Index Node zero is legal and must not be confused with 'no collision'.
  Node i = raw(2), j = raw(2), a = raw(), b = raw();
  Node unequal = op(View::Ne, {i, j});
  Node equal = op(View::Eq, {i, j});
  Node notA = op(View::Not, {a});
  Node grant = op(View::And, {b, op(View::Or, {notA, unequal})});
  Node invalid = op(View::And, {b, op(View::Or, {notA, equal})});
  auto p = proof();
  auto separated = p->disjointAtAddress(a, grant, i, j);
  auto unscoped = p->disjoint(a, grant);
  auto overlapping = p->disjointAtAddress(a, invalid, i, j);
  ASSERT_TRUE(mlir::succeeded(separated)); ASSERT_TRUE(mlir::succeeded(unscoped));
  ASSERT_TRUE(mlir::succeeded(overlapping));
  EXPECT_TRUE(*separated); EXPECT_FALSE(*unscoped); EXPECT_FALSE(*overlapping);
  // Independent four-state oracle: a possible collision is any equality not
  // known false. X and Z never count as a known-one enable.
  constexpr int X = -1, Z = -2;
  auto conjunction = [](int x, int y) { return x == 0 || y == 0 ? 0 : x == 1 && y == 1 ? 1 : X; };
  auto disjunction = [](int x, int y) { return x == 1 || y == 1 ? 1 : x == 0 && y == 0 ? 0 : X; };
  for (int x : {0, 1, X, Z}) for (int y : {0, 1, X, Z})
    for (int left : {0, 1, 2, 3, X, Z}) for (int right : {0, 1, 2, 3, X, Z}) {
      int eq = left < 0 || right < 0 ? X : int(left == right);
      int ne = eq < 0 ? X : 1 - eq;
      int inv = x < 0 ? X : 1 - x;
      int enabled = conjunction(y, disjunction(inv, ne));
      if (eq != 0) EXPECT_FALSE(x == 1 && enabled == 1);
    }
}
TEST_F(HardwareGrantProofTest, ReservedAdapterNodeIdentitiesFailClosed) {
  auto p = proof();
  Node first = raw();
  EXPECT_TRUE(mlir::failed(p->disjoint(first, UINT64_MAX)));
  EXPECT_FALSE(p->failureReason().empty());
}
TEST_F(HardwareGrantProofTest, WidthAndConstantAuthorityRemainExact) {
  Node a = raw(), b = raw(), i = raw(4), j = raw(8);
  Node small = constant(3, 4), same = constant(3, 8), other = constant(7, 8);
  auto p = proof();
  auto dynamic = p->disjointAtAddress(a, b, i, j);
  auto equal = p->disjointAtAddress(a, b, small, same);
  auto different = p->disjointAtAddress(a, b, small, other);
  ASSERT_TRUE(mlir::succeeded(dynamic)); ASSERT_TRUE(mlir::succeeded(equal));
  ASSERT_TRUE(mlir::succeeded(different));
  EXPECT_FALSE(*dynamic); EXPECT_FALSE(*equal); EXPECT_TRUE(*different);
  auto exhausted = proof(1);
  EXPECT_TRUE(mlir::failed(exhausted->disjoint(a, b)));
}
} // namespace
