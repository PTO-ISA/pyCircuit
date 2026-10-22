#include "gfsim/SimSystem.h"
#include "gfsim/WorkExecutor.h"
#include "pycircuit_system.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <source_location>
#include <string_view>

namespace {

void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "sampled admission oracle failed at " << at.line() << '\n';
    std::abort();
  }
}

gfsim::wire<gfsim::Bits<1>> bit(char value) {
  using State = gfsim::FourState<1>;
  switch (value) {
  case '0':
    return gfsim::wire<gfsim::Bits<1>>::fromPacked(
        State::known(gfsim::Bits<1>{0}));
  case '1':
    return gfsim::wire<gfsim::Bits<1>>::fromPacked(
        State::known(gfsim::Bits<1>{1}));
  case 'x':
    return gfsim::wire<gfsim::Bits<1>>::fromPacked(State::unknown());
  case 'z':
    return gfsim::wire<gfsim::Bits<1>>::fromPacked(State::highImpedance());
  default:
    std::abort();
  }
}

unsigned observed(const gfsim::wire<gfsim::Bits<1>> &value) {
  require(value.isFullyKnown());
  return value.value() == gfsim::Bits<1>{1};
}

class CheckedSystem final : public gfsim::SimSystem {
public:
  explicit CheckedSystem(pyc_root &root) : root_(root) {}

private:
  bool Precheck() noexcept override {
    failure_ = {};
    return root_.pyc_validate_checks(failure_);
  }
  gfsim::SimFailureInfo
  DescribeFailure(gfsim::SimFailurePhase phase) const noexcept override {
    if (phase == gfsim::SimFailurePhase::Check && !failure_.code.empty())
      return failure_;
    return gfsim::SimSystem::DescribeFailure(phase);
  }
  pyc_root &root_;
  gfsim::SimFailureInfo failure_{};
};

struct Harness {
  gfsim::WorkExecutor executor;
  std::shared_ptr<pyc_root> root;
  CheckedSystem system;

  explicit Harness(unsigned workers)
      : executor(workers), root(std::make_shared<pyc_root>("root", &executor)),
        system(*root) {
    require(system.AddModule(*root));
    system.Build();
    require(system.state() == gfsim::SimSystemState::Built);
    drive('1', '1', '1', '1', '0', '0', '0', '0');
    system.Reset();
    require(system.state() == gfsim::SimSystemState::Ready);
    refresh();
  }

  void drive(char leftFirst, char leftSecond, char rightFirst, char rightSecond,
             char rightClock, char rightReset, char rootClock, char rootReset) {
    driveSeparate(leftFirst, leftSecond, rightFirst, rightSecond, rootClock,
                  rootReset, rightClock, rightReset, rootClock, rootReset);
  }

  void driveSeparate(char leftFirst, char leftSecond, char rightFirst,
                     char rightSecond, char leftClock, char leftReset,
                     char rightClock, char rightReset, char rootClock,
                     char rootReset) {
    root->left_first = bit(leftFirst);
    root->left_second = bit(leftSecond);
    root->right_first = bit(rightFirst);
    root->right_second = bit(rightSecond);
    root->left_clk = bit(leftClock);
    root->left_rst = bit(leftReset);
    root->right_clk = bit(rightClock);
    root->right_rst = bit(rightReset);
    root->sample_clk = bit(rootClock);
    root->sample_rst = bit(rootReset);
    root->selector = bit('0');
    root->then_guard = bit('1');
    root->else_guard = bit('1');
    root->asserted = bit('1');
    root->conditional_clk = bit('0');
    root->conditional_rst = bit('0');
  }

  std::string_view step(bool accepted) {
    const auto before = system.cycle();
    const auto result = system.Step();
    if (accepted) {
      require(result == gfsim::SimStepResult::Running);
      require(system.cycle() == before + 1);
      refresh();
      return "ok";
    }
    require(result == gfsim::SimStepResult::Failed);
    require(system.cycle() == before);
    const auto failure = system.failureInfo();
    if (failure.code == "source_check_failed")
      return "guard";
    require(failure.code == "runtime_failure");
    require(failure.message == "sampled clock is unknown" ||
            failure.message == "sampled reset is unknown");
    return "physical";
  }

  void refresh() {
    root->Work();
    root->DiscardNext();
  }
};

void row(std::string_view name, std::string_view status, const pyc_root &root,
         unsigned expectedLeft, unsigned expectedRight) {
  const auto left = observed(root.left_q);
  const auto right = observed(root.right_q);
  require(left == expectedLeft && right == expectedRight);
  std::cout << "ROW " << name << ' ' << status << ' ' << left << ' ' << right
            << '\n';
}

void committedScenarios(unsigned workers) {
  Harness h(workers);
  row("RESET", "ok", *h.root, 0, 0);

  h.drive('1', '1', '1', '1', '0', '0', '0', '0');
  row("BASE", h.step(true), *h.root, 0, 0);

  h.drive('1', '0', '1', '1', '1', '0', '1', '0');
  row("LOCAL_HOLD", h.step(true), *h.root, 0, 1);

  h.drive('x', '1', '0', 'x', '1', 'x', '1', 'x');
  row("HELD_MASK", h.step(true), *h.root, 0, 1);

  h.drive('1', '1', '1', '1', '0', 'z', '0', 'z');
  row("FALL", h.step(true), *h.root, 0, 1);

  h.drive('1', '1', '1', '1', '0', '0', '1', '0');
  row("INDEPENDENT_LEFT", h.step(true), *h.root, 1, 1);

  h.drive('1', '1', '1', '1', '1', '0', '1', '0');
  row("INDEPENDENT_RIGHT", h.step(true), *h.root, 1, 0);

  h.drive('1', '1', '1', '1', '0', '0', '0', '0');
  require(h.step(true) == "ok");
  h.drive('1', '0', '0', 'x', '1', '0', '1', '0');
  row("MASKED_RISING", h.step(true), *h.root, 1, 0);

  h.drive('1', '1', '1', '1', '0', '0', '0', '0');
  require(h.step(true) == "ok");
  h.drive('x', '0', '1', '1', '1', '0', '1', '0');
  row("GUARD_ROLLBACK", h.step(false), *h.root, 1, 0);
}

void physicalFault(unsigned workers, char clock, std::string_view name) {
  Harness h(workers);
  h.driveSeparate('1', '1', '1', '1', '0', '0', '0', '0', clock, '0');
  row(name, h.step(false), *h.root, 0, 0);
}

void intentUnknownRollback(unsigned workers) {
  Harness h(workers);
  h.drive('z', '0', '1', '1', '1', '0', '1', '0');
  row("INTENT_Z_ROLLBACK", h.step(false), *h.root, 0, 0);
}

void resetScenarios(unsigned workers, char unknown, std::string_view suffix) {
  Harness rising(workers);
  rising.drive('1', '1', '1', '1', '0', '0', '0', '0');
  require(rising.step(true) == "ok");
  rising.driveSeparate('1', '1', '1', '1', '1', '0', '1', '0', '1', unknown);
  row(suffix == "X" ? "RESET_RISING_X" : "RESET_RISING_Z", rising.step(false),
      *rising.root, 0, 0);

  Harness held(workers);
  held.drive('0', 'x', '0', 'x', '0', '0', '0', '0');
  require(held.step(true) == "ok");
  held.drive('0', 'x', '0', 'x', '1', '0', '1', '0');
  require(held.step(true) == "ok");
  held.drive('x', 'x', 'x', 'x', '1', unknown, '1', unknown);
  row(suffix == "X" ? "RESET_HELD_X" : "RESET_HELD_Z", held.step(true),
      *held.root, 0, 0);
  held.drive('x', 'x', 'x', 'x', '0', unknown, '0', unknown);
  row(suffix == "X" ? "RESET_FALL_X" : "RESET_FALL_Z", held.step(true),
      *held.root, 0, 0);
}

void resetActiveAndExplicitReset(unsigned workers) {
  Harness active(workers);
  active.drive('1', '1', '1', '1', '0', '0', '0', '0');
  require(active.step(true) == "ok");
  active.drive('x', 'x', 'x', 'x', '1', '1', '1', '1');
  row("RESET_ACTIVE", active.step(true), *active.root, 0, 0);

  Harness explicitReset(workers);
  explicitReset.drive('x', 'x', 'x', 'x', 'x', 'x', 'x', 'x');
  explicitReset.system.Reset();
  require(explicitReset.system.state() == gfsim::SimSystemState::Ready);
  explicitReset.drive('1', '1', '1', '1', '0', '0', '0', '0');
  explicitReset.refresh();
  row("EXPLICIT_RESET", "ok", *explicitReset.root, 0, 0);
}

void directLifecycle(unsigned workers) {
  Harness frozen(workers);
  frozen.drive('1', '1', '1', '1', '0', '0', '0', '0');
  frozen.root->Work();
  gfsim::SimFailureInfo failure;
  require(frozen.root->pyc_validate_checks(failure));
  frozen.root->Xfer();
  frozen.drive('1', '1', '1', '1', '1', '0', '1', '0');
  frozen.root->Work();
  frozen.drive('x', 'x', 'x', 'x', 'x', 'x', 'x', 'x');
  require(frozen.root->pyc_validate_checks(failure));
  frozen.root->Xfer();
  frozen.drive('1', '1', '1', '1', '1', '0', '1', '0');
  frozen.refresh();
  row("FROZEN_AFTER_WORK", "ok", *frozen.root, 1, 1);

  Harness retry(workers);
  retry.drive('1', '1', '1', '1', '0', '0', '0', '0');
  retry.root->Work();
  require(retry.root->pyc_validate_checks(failure));
  retry.root->Xfer();
  retry.refresh();
  retry.drive('1', '1', '1', '1', '1', '0', '1', '0');
  retry.root->Work();
  require(retry.root->pyc_validate_checks(failure));
  retry.root->DiscardNext();
  retry.root->Xfer();
  row("DISCARDED", "ok", *retry.root, 0, 0);
  retry.root->Work();
  require(retry.root->pyc_validate_checks(failure));
  retry.root->Xfer();
  retry.refresh();
  row("DISCARD_RETRY", "ok", *retry.root, 1, 1);
}

void conditionalScenarios(unsigned workers) {
  struct Case {
    std::string_view name;
    char selector, thenGuard, elseGuard, asserted;
    bool accepted;
    unsigned expected;
  };
  // Expected state and status are hand-derived from the selected guard/check.
  // Independent owners rise concurrently, exposing whole-system rollback.
  const Case cases[] = {
      {"SELECT_ZERO", '0', 'x', '1', '1', true, 1},
      {"SELECT_ONE", '1', '1', 'z', '1', true, 1},
      {"SKIP_THEN_FALSE", '0', '0', '1', '1', true, 1},
      {"SKIP_THEN_X", '0', 'x', '1', '1', true, 1},
      {"SKIP_THEN_Z", '0', 'z', '1', '1', true, 1},
      {"SKIP_ELSE_FALSE", '1', '1', '0', '1', true, 1},
      {"SKIP_ELSE_X", '1', '1', 'x', '1', true, 1},
      {"SKIP_ELSE_Z", '1', '1', 'z', '1', true, 1},
      {"THEN_FALSE_HOLD", '1', '0', 'x', 'x', true, 0},
      {"ELSE_FALSE_HOLD", '0', 'x', '0', 'x', true, 0},
      {"THEN_X_ROLLBACK", '1', 'x', '0', '1', false, 0},
      {"THEN_Z_ROLLBACK", '1', 'z', '0', '1', false, 0},
      {"ELSE_X_ROLLBACK", '0', '0', 'x', '1', false, 0},
      {"ELSE_Z_ROLLBACK", '0', '0', 'z', '1', false, 0},
      {"SELECT_X_LATE_FALSE", 'x', '0', '0', '1', false, 0},
      {"SELECT_Z_LATE_FALSE", 'z', '0', '0', '1', false, 0},
      {"THEN_ASSERT_FALSE", '1', '1', 'x', '0', false, 0},
      {"ELSE_ASSERT_FALSE", '0', 'x', '1', '0', false, 0},
      {"THEN_ASSERT_X", '1', '1', 'x', 'x', false, 0},
      {"ELSE_ASSERT_Z", '0', 'x', '1', 'z', false, 0},
  };
  for (const auto &test : cases) {
    Harness h(workers);
    require(h.step(true) == "ok");
    h.drive('1', '1', '1', '1', '1', '0', '1', '0');
    h.root->selector = bit(test.selector);
    h.root->then_guard = bit(test.thenGuard);
    h.root->else_guard = bit(test.elseGuard);
    h.root->asserted = bit(test.asserted);
    h.root->conditional_clk = bit('1');
    const auto status = h.step(test.accepted);
    const auto q = observed(h.root->conditional_q);
    const auto left = observed(h.root->left_q);
    const auto right = observed(h.root->right_q);
    require(q == test.expected);
    require(left == unsigned(test.accepted) &&
            right == unsigned(test.accepted));
    std::cout << "CROW " << test.name << ' ' << status << ' ' << q << ' '
              << left << ' ' << right << '\n';
  }
  Harness h(workers);
  require(h.step(true) == "ok");
  h.root->selector = bit('1');
  h.root->conditional_clk = bit('1');
  require(h.step(true) == "ok");
  require(observed(h.root->conditional_q) == 1);
  h.root->selector = bit('x');
  h.root->then_guard = bit('x');
  h.root->else_guard = bit('z');
  h.root->asserted = bit('x');
  require(h.step(true) == "ok");
  require(observed(h.root->conditional_q) == 1);
  std::cout << "CROW HELD_UNKNOWN ok 1 0 0\n";
  h.root->conditional_clk = bit('0');
  require(h.step(true) == "ok");
  require(observed(h.root->conditional_q) == 1);
  std::cout << "CROW FALLING_UNKNOWN ok 1 0 0\n";
  h.root->conditional_clk = bit('1');
  h.root->conditional_rst = bit('1');
  require(h.step(true) == "ok");
  require(observed(h.root->conditional_q) == 0);
  std::cout << "CROW RESET_UNKNOWN ok 0 0 0\n";
}

} // namespace

int main(int argc, char **argv) {
  require(argc == 3 && std::string_view(argv[1]) == "--workers");
  const auto workers =
      static_cast<unsigned>(std::strtoul(argv[2], nullptr, 10));
  require(workers == 1 || workers == 2);
  committedScenarios(workers);
  intentUnknownRollback(workers);
  physicalFault(workers, 'x', "CLOCK_X");
  physicalFault(workers, 'z', "CLOCK_Z");
  resetScenarios(workers, 'x', "X");
  resetScenarios(workers, 'z', "Z");
  resetActiveAndExplicitReset(workers);
  directLifecycle(workers);
  conditionalScenarios(workers);
  std::cout << "PASS sampled admission lifecycle\n";
  return 0;
}
