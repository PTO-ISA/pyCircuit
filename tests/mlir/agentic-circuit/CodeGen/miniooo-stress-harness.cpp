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

// The window slot 0 entry is generated state: slot(17:16), valid(15),
// generation(14:13), recovery_epoch(12:10), attempt(9:8), payload(7:0). Reading
// it directly is what makes the stress able to distinguish a committed update
// from a rejected one; the obligation counters alone cannot, because the
// no_stale_update coverage condition is `requested && !qualified`, so it counts
// rejected stale updates and grows fastest when the window is wedged.
struct WindowEntry {
  bool valid = false;
  unsigned generation = 0;
  unsigned epoch = 0;
  unsigned attempt = 0;
  unsigned payload = 0;
};

WindowEntry slot0(const Model &model) {
  const std::uint64_t raw = model.pyc_reg_23.value();
  return {((raw >> 15) & 1u) != 0, unsigned((raw >> 13) & 3u),
          unsigned((raw >> 10) & 7u), unsigned((raw >> 8) & 3u),
          unsigned(raw & 0xffu)};
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
// attempt is the window version reference: the commit rule allocates with
// attempt 0 and the retire rule commits only a completion whose attempt still
// matches the stored entry, so a non-zero attempt is a stale completion.
pyc::cpp::Wire<17> pack_completion(std::uint64_t attempt, std::uint64_t payload) {
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
// epoch raises kill; `attempt` still has to match the stored entry for the
// invalidation itself to be qualified rather than rejected as stale.
pyc::cpp::Wire<17> pack_recovery(std::uint64_t next_epoch,
                                std::uint64_t attempt) {
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
  append(attempt, 2);      // attempt
  return value;
}

// Drive one dispatch plus one completion in the same cycle and clock until both
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

// Drive one dispatch plus one recovery in the same cycle. A killing recovery is
// only consumed when its own kill condition holds, so the stress never feeds the
// recovery queue a no-op that would back-pressure the source.
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

// Clock idle cycles while watching the window, used to observe a registered
// invalidation land.
// Clock idle cycles so a registered rule effect becomes observable.
void settle(Model &model, unsigned cycles) {
  for (unsigned cycle = 0; cycle < cycles; ++cycle) {
    model.in0_valid = pyc::cpp::Wire<1>{0};
    model.in1_valid = pyc::cpp::Wire<1>{0};
    model.in2_valid = pyc::cpp::Wire<1>{0};
    clock_one(model);
  }
}

bool window_ever_invalid(Model &model, unsigned cycles) {
  for (unsigned cycle = 0; cycle < cycles; ++cycle) {
    model.in0_valid = pyc::cpp::Wire<1>{0};
    model.in1_valid = pyc::cpp::Wire<1>{0};
    model.in2_valid = pyc::cpp::Wire<1>{0};
    clock_one(model);
    if (!slot0(model).valid)
      return true;
  }
  return false;
}

} // namespace

int main() {
  Model model;
  model.rst = pyc::cpp::Wire<1>{1};
  clock_one(model);
  clock_one(model);
  model.rst = pyc::cpp::Wire<1>{0};

  constexpr unsigned commit_cycles = 24;
  constexpr unsigned stale_cycles = 12;
  constexpr unsigned resume_cycles = 8;
  constexpr unsigned cycles =
      commit_cycles + stale_cycles + 2 + resume_cycles;

  unsigned dispatched = 0;
  unsigned completed = 0;
  unsigned recovered = 0;
  unsigned commit_advances = 0;
  unsigned stale_commits = 0;
  unsigned resume_advances = 0;
  unsigned stale_invalidations = 0;
  unsigned invalidations = 0;
  unsigned window_valid_during_commit = 0;
  std::uint64_t retire_after_warmup = 0;
  bool window_became_valid = false;
  std::uint64_t previous_payload = 0;
  std::uint64_t commit_payload = 0x40;

  // Phase 1: every completion carries the window version the commit rule
  // publishes, so the slot 0 payload has to keep advancing. A window that
  // commits once and then wedges freezes it. Every eighth cycle drops the
  // identity match and the other cycles alternate the alias, disjoint, executed
  // and killed qualifications, so the wait, bypass, forward and replay
  // dispositions all stay reachable while the window commits.
  for (unsigned cycle = 0; cycle < commit_cycles; ++cycle) {
    const bool stale_lane = (cycle % 8) == 7;
    const auto dispatch = pack_dispatch(
        stale_lane ? 0 : 3, (cycle % 2) == 0 ? 3 : 0, (cycle % 2) == 0 ? 0 : 3,
        /*data_ready=*/3, (cycle % 4) == 3 ? 3 : 0, (cycle % 16) == 15 ? 3 : 0);
    drive_cycle(model, dispatch, pack_completion(/*attempt=*/0, commit_payload));
    ++dispatched;
    ++completed;
    const WindowEntry entry = slot0(model);
    if (entry.valid) {
      window_became_valid = true;
      ++window_valid_during_commit;
      if (entry.payload > previous_payload) {
        ++commit_advances;
        previous_payload = entry.payload;
      }
    }
    ++commit_payload;
    if (cycle == 3)
      retire_after_warmup =
          model.obligation_no_stale_update_window_retire_slot0_coverage;
  }
  const auto retire_after_commit =
      model.obligation_no_stale_update_window_retire_slot0_coverage;

  // Phase 2: completions whose version no longer matches the committed entry.
  // The payloads are drawn from a disjoint high band, so any value from that
  // band appearing in the window proves a stale completion was applied. The
  // rejected attempts must instead show up in the no_stale_update coverage
  // counter, which is exactly what that counter counts.
  const auto retire_before_stale =
      model.obligation_no_stale_update_window_retire_slot0_coverage;
  for (unsigned cycle = 0; cycle < stale_cycles; ++cycle) {
    const auto dispatch = pack_dispatch(3, 3, 0, 3, 0, 0);
    drive_cycle(model, dispatch,
                pack_completion(/*attempt=*/1, 0x80u + cycle));
    ++dispatched;
    ++completed;
    if (slot0(model).payload >= 0x80u)
      ++stale_commits;
  }
  const auto retire_after_stale =
      model.obligation_no_stale_update_window_retire_slot0_coverage;
  const auto recover_before_stale =
      model.obligation_no_stale_update_window_recover_slot0_coverage;

  // Phase 3: a killing recovery carrying a stale version must be rejected, so
  // the recover obligation reports a stale-mutation attempt and the entry stays
  // valid.
  const auto dispatch = pack_dispatch(3, 3, 0, 3, 0, 0);
  drive_recovery(model, dispatch, pack_recovery(/*next_epoch=*/1, /*attempt=*/1));
  ++dispatched;
  ++recovered;
  settle(model, 6);
  if (model.obligation_no_stale_update_window_recover_slot0_coverage >
      recover_before_stale)
    ++stale_invalidations;
  if (slot0(model).valid)
    ++window_valid_during_commit;

  // Phase 4: the same killing recovery with the live version has to commit the
  // invalidation, which is observable as the slot 0 valid bit dropping.
  drive_recovery(model, dispatch, pack_recovery(/*next_epoch=*/1, /*attempt=*/0));
  ++dispatched;
  ++recovered;
  if (window_ever_invalid(model, 4))
    ++invalidations;

  // Phase 5: after the committed invalidation the window has to accept
  // qualified updates again instead of staying dead, and with every version
  // matching again it must report no stale rejection at all.
  for (unsigned cycle = 0; cycle < resume_cycles; ++cycle) {
    const auto resume_dispatch = pack_dispatch(3, 3, 0, 3, 0, 0);
    drive_cycle(model, resume_dispatch,
                pack_completion(/*attempt=*/0, commit_payload));
    ++dispatched;
    ++completed;
    const WindowEntry entry = slot0(model);
    if (entry.valid && entry.payload > previous_payload) {
      ++resume_advances;
      previous_payload = entry.payload;
    }
    ++commit_payload;
  }

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
  // The first warm-up cycles legitimately report rejections while the commit
  // rule has not allocated the entry yet, so the matching-phase rate is measured
  // after warm-up. The point of the comparison is that the counter tracks stale
  // rejections, not commits: matching versions barely report, stale ones report
  // on every attempt.
  const auto commit_rejections = retire_after_commit - retire_after_warmup;
  const auto stale_rejections = retire_after_stale - retire_before_stale;

  if (dispatched != cycles || completed != cycles - 2 || recovered != 2) {
    std::cerr << "miniOOO stress: handshake stalled dispatched=" << dispatched
              << " completed=" << completed << " recovered=" << recovered
              << "\n";
    return 1;
  }
  if (failures != 0) {
    std::cerr << "miniOOO stress: obligations failed count=" << failures << "\n";
    return 1;
  }
  // A live window keeps applying qualified updates: the slot 0 payload has to
  // advance on nearly every commit cycle and again after the invalidation. A
  // wedged window freezes the payload, which is what the old coverage-based
  // check failed to detect because it counted rejections instead of commits.
  if (commit_advances < commit_cycles - 4 || resume_advances < 3) {
    std::cerr << "miniOOO stress: window stopped committing commit_advances="
              << commit_advances << " resume_advances=" << resume_advances
              << "\n";
    return 1;
  }
  // No stale completion may reach the window payload, and every matching phase
  // after the window is valid must be rejection-free while the stale phase must
  // be reported as rejected.
  if (stale_commits != 0) {
    std::cerr << "miniOOO stress: stale completions reached the window count="
              << stale_commits << "\n";
    return 1;
  }
  if (stale_rejections == 0) {
    std::cerr << "miniOOO stress: stale attempts were not reported\n";
    return 1;
  }
  if (stale_rejections <= commit_rejections) {
    std::cerr << "miniOOO stress: rejection counter does not discriminate"
              << " stale=" << stale_rejections
              << " matching=" << commit_rejections << "\n";
    return 1;
  }
  // The stale killing recovery must be rejected and the live one committed: the
  // two recover obligations differ exactly there.
  if (stale_invalidations != 1 || invalidations != 1) {
    std::cerr << "miniOOO stress: recovery branch mismatch stale="
              << stale_invalidations << " committed=" << invalidations
              << " recover_before_stale=" << recover_before_stale
              << " recover=" << recover << " retire=" << retire
              << " commit_advances=" << commit_advances
              << " resume_advances=" << resume_advances << "\n";
    return 1;
  }
  if (!window_became_valid || window_valid_during_commit == 0) {
    std::cerr << "miniOOO stress: window never became valid\n";
    return 1;
  }
  if (stale == 0) {
    std::cerr << "miniOOO stress: no stale lane was ever classified\n";
    return 1;
  }
  std::cout << "miniOOO stress PASS cycles=" << cycles
            << " commit_advances=" << commit_advances
            << " resume_advances=" << resume_advances
            << " stale_commits=" << stale_commits
            << " stale_rejections=" << stale_rejections
            << " commit_rejections=" << commit_rejections
            << " stale_invalidations=" << stale_invalidations
            << " invalidations=" << invalidations
            << " retire_coverage=" << retire << " recover_coverage=" << recover
            << " stale_coverage=" << stale << " failures=" << failures << "\n";
  return 0;
}
