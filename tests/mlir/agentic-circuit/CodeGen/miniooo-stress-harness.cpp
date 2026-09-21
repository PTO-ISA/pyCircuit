#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

using Model = pyc::gen::miniooo;

void clock_one(Model &model) {
  model.clk = pyc::cpp::Wire<1>{0};
  model.step();
  model.clk = pyc::cpp::Wire<1>{1};
  model.step();
}

std::uint64_t next_random(std::uint64_t &state) {
  state += 0x9e3779b97f4a7c15ULL;
  std::uint64_t z = state;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}

// The dispatch payload mirrors the fixture's @Dispatch field order. It is wider
// than one machine word, so it is assembled in the generated wire type. `free`
// is the availability bitmap and `valid` the dispatch bitmap: the ALU allocator
// only hands an entry to a lane while the bitmap still has an entry left after
// the same-cycle prefix walk, so the stress keeps two dispatch lanes against
// four free entries.
pyc::cpp::Wire<84> pack_dispatch(std::uint64_t identity, std::uint64_t alias,
                                 std::uint64_t disjoint,
                                 std::uint64_t data_ready,
                                 std::uint64_t executed,
                                 std::uint64_t killed) {
  pyc::cpp::Wire<84> value{0};
  auto append = [&](std::uint64_t field, unsigned width) {
    value = pyc::cpp::shl(value, width) | pyc::cpp::Wire<84>{field};
  };
  append(3, 4);             // valid: lanes 0 and 1 dispatched
  append(15, 4);            // r0
  append(15, 4);            // r1
  append(15, 4);            // r2
  append(15, 4);            // free: four entries available
  append(0, 4);             // release
  append(15, 4);            // candidates
  append(0, 3);             // age0
  append(1, 3);             // age1
  append(2, 3);             // age2
  append(3, 3);             // age3
  append(15, 4);            // current
  append(0, 4);             // resolve
  append(0, 4);             // kill
  append(identity, 4);      // identity match
  append(15, 4);            // effects
  append(15, 4);            // terminal
  append(alias, 4);         // alias
  append(disjoint, 4);      // disjoint
  append(data_ready, 4);    // data ready
  append(executed, 4);      // already executed
  append(killed, 4);        // flush invalidated
  return value;
}

// slot i2, generation i2, recovery_epoch i3, attempt i2, payload i8. The
// reference has to name the window version the commit rule is about to write,
// so `attempt` is the window version counter and not a constant.
pyc::cpp::Wire<17> pack_completion(std::uint64_t payload, std::uint64_t attempt) {
  pyc::cpp::Wire<17> value{0};
  auto append = [&](std::uint64_t field, unsigned width) {
    value = pyc::cpp::shl(value, width) | pyc::cpp::Wire<17>{field};
  };
  append(0, 2);            // slot
  append(0, 2);            // generation
  append(0, 3);            // recovery epoch
  append(attempt, 2);      // attempt
  append(payload, 8);      // payload
  return value;
}

// valid i1, next_epoch i3, checkpoint i2, boundary i2, slot i2, generation i2,
// transaction_epoch i3, attempt i2. `next_epoch` differing from the transaction
// epoch turns this into a killing recovery that invalidates the committed
// window entry.
pyc::cpp::Wire<17> pack_recovery(std::uint64_t next_epoch) {
  pyc::cpp::Wire<17> value{0};
  auto append = [&](std::uint64_t field, unsigned width) {
    value = pyc::cpp::shl(value, width) | pyc::cpp::Wire<17>{field};
  };
  append(1, 1);            // valid
  append(next_epoch, 3);   // next epoch
  append(0, 2);            // checkpoint
  append(0, 2);            // boundary
  append(0, 2);            // slot
  append(0, 2);            // generation
  append(0, 3);            // transaction epoch
  append(0, 2);            // attempt
  return value;
}

// Drive one dispatch and one completion in the same cycle and clock until both
// are accepted, so the stress exercises the real ready/valid handshake against
// the issue and completion queues instead of assuming free queues.
void drive_cycle(Model &model, pyc::cpp::Wire<84> dispatch,
                 pyc::cpp::Wire<17> completion, unsigned limit = 64) {
  model.in0_data = dispatch;
  model.in0_valid = pyc::cpp::Wire<1>{1};
  model.in1_data = completion;
  model.in1_valid = pyc::cpp::Wire<1>{1};
  model.in2_data = pyc::cpp::Wire<17>{0};
  model.in2_valid = pyc::cpp::Wire<1>{0};

  bool dispatched = false;
  bool completed = false;
  for (unsigned cycle = 0; cycle < limit; ++cycle) {
    model.clk = pyc::cpp::Wire<1>{0};
    model.eval();
    const bool dispatch_ready = model.in0_ready.toBool();
    const bool completion_ready = model.in1_ready.toBool();
    clock_one(model);
    if (!dispatched && dispatch_ready) {
      dispatched = true;
      model.in0_valid = pyc::cpp::Wire<1>{0};
    }
    if (!completed && completion_ready) {
      completed = true;
      model.in1_valid = pyc::cpp::Wire<1>{0};
    }
    if (dispatched && completed)
      return;
  }
  throw std::runtime_error("miniOOO dispatch/completion was not accepted");
}

// A killing recovery is driven on its own cycle: the invalidation has to
// observe the window entry the commit rule already wrote, so it cannot share
// the cycle that allocates it.
void drive_recovery(Model &model, pyc::cpp::Wire<84> dispatch,
                    pyc::cpp::Wire<17> recovery, unsigned limit = 64) {
  model.in0_data = dispatch;
  model.in0_valid = pyc::cpp::Wire<1>{1};
  model.in1_data = pyc::cpp::Wire<17>{0};
  model.in1_valid = pyc::cpp::Wire<1>{0};
  model.in2_data = recovery;
  model.in2_valid = pyc::cpp::Wire<1>{1};

  bool dispatched = false;
  bool recovered = false;
  for (unsigned cycle = 0; cycle < limit; ++cycle) {
    model.clk = pyc::cpp::Wire<1>{0};
    model.eval();
    const bool dispatch_ready = model.in0_ready.toBool();
    const bool recovery_ready = model.in2_ready.toBool();
    clock_one(model);
    if (!dispatched && dispatch_ready) {
      dispatched = true;
      model.in0_valid = pyc::cpp::Wire<1>{0};
    }
    if (!recovered && recovery_ready) {
      recovered = true;
      model.in2_valid = pyc::cpp::Wire<1>{0};
    }
    if (dispatched && recovered)
      return;
  }
  throw std::runtime_error("miniOOO dispatch/recovery was not accepted");
}

} // namespace

int main() {
  Model model;
  model.rst = pyc::cpp::Wire<1>{1};
  clock_one(model);
  clock_one(model);
  model.rst = pyc::cpp::Wire<1>{0};

  std::uint64_t state = 0x9e3779b97f4a7c15ULL;
  constexpr unsigned cycles = 48;
  constexpr unsigned stale_period = 8;
  unsigned dispatched = 0;
  unsigned completed = 0;
  unsigned recovered = 0;
  unsigned stale_trials = 0;
  std::uint64_t retire_at_half = 0;

  for (unsigned cycle = 0; cycle < cycles; ++cycle) {
    // Every eighth cycle drops the identity match, so the load disposition has
    // to classify both lanes as stale instead of forwarding them. The other
    // cycles alternate the alias, disjoint, executed and killed qualifications
    // so the forward, bypass, replay and wait dispositions stay reachable.
    const bool stale_trial = (cycle % stale_period) == stale_period - 1;
    const std::uint64_t identity = stale_trial ? 0 : 3;
    const std::uint64_t alias = (cycle % 2) == 0 ? 3 : 0;
    const std::uint64_t disjoint = (cycle % 2) == 0 ? 0 : 3;
    const std::uint64_t executed = (cycle % 4) == 3 ? 3 : 0;
    const std::uint64_t killed = (cycle % 16) == 15 ? 3 : 0;
    const std::uint64_t payload = next_random(state) & 0xff;
    if (stale_trial)
      ++stale_trials;

    const auto dispatch = pack_dispatch(identity, alias, disjoint,
                                        /*data_ready=*/3, executed, killed);
    if (cycle == 0) {
      // The first cycle issues the killing recovery against the window version
      // the very first commit publishes, before any qualified update has moved
      // it forward.
      drive_recovery(model, dispatch, pack_recovery(1));
      ++dispatched;
      ++recovered;
    } else {
      // Each qualified update advances the window version, so the completion
      // carries the version this cycle's commit will publish.
      drive_cycle(model, dispatch,
                  pack_completion(payload, cycle % 4));
      ++dispatched;
      ++completed;
    }
    if (cycle == cycles / 2)
      retire_at_half =
          model.obligation_no_stale_update_window_retire_slot0_coverage;
  }

  const auto retire_checks =
      model.obligation_no_stale_update_window_retire_slot0_checks;
  const auto recover_checks =
      model.obligation_no_stale_update_window_recover_slot0_checks;
  const auto stale_checks =
      model.obligation_no_stale_response_issue_q_disposition0_checks;
  const auto retire =
      model.obligation_no_stale_update_window_retire_slot0_coverage;
  const auto recover =
      model.obligation_no_stale_update_window_recover_slot0_coverage;
  const auto stale =
      model.obligation_no_stale_response_issue_q_disposition0_coverage;
  const auto failures =
      model.obligation_no_stale_update_window_retire_slot0_failures +
      model.obligation_no_stale_update_window_recover_slot0_failures +
      model.obligation_no_stale_response_issue_q_disposition0_failures;

  if (dispatched != cycles || completed != cycles - 1 || recovered != 1) {
    std::cerr << "miniOOO stress: handshake stalled dispatched=" << dispatched
              << " completed=" << completed << " recovered=" << recovered
              << "\n";
    return 1;
  }
  // Reaching the end already proves no obligation aborted the model; the
  // explicit failure sum keeps a silently disabled check from passing.
  if (failures != 0) {
    std::cerr << "miniOOO stress: obligations failed count=" << failures << "\n";
    return 1;
  }
  // The versioned window must keep accepting qualified updates instead of
  // committing once and then wedging, must have committed a killing
  // invalidation, and a stale lane must have been observed as stale rather than
  // forwarded.
  if (retire == 0 || retire <= retire_at_half || recover == 0 || stale == 0) {
    std::cerr << "miniOOO stress: obligation coverage missing retire=" << retire
              << " retire_at_half=" << retire_at_half << " recover=" << recover
              << " stale=" << stale << "\n"
              << "  retire checks=" << retire_checks
              << " recover checks=" << recover_checks
              << " stale checks=" << stale_checks << "\n";
    return 1;
  }
  if (stale_trials != cycles / stale_period) {
    std::cerr << "miniOOO stress: stale schedule drifted=" << stale_trials
              << "\n";
    return 1;
  }
  std::cout << "miniOOO stress PASS cycles=" << cycles
            << " stale_trials=" << stale_trials << " dispatched=" << dispatched
            << " completed=" << completed << " recovered=" << recovered
            << " retire_coverage=" << retire
            << " retire_at_half=" << retire_at_half
            << " recover_coverage=" << recover << " stale_coverage=" << stale
            << " failures=" << failures << "\n";
  return 0;
}
