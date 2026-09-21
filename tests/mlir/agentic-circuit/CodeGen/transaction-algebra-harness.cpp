#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

using Model = pyc::gen::transaction_algebra;

void clock(Model &model) {
  model.clk = pyc::cpp::Wire<1>{0};
  model.step();
  model.clk = pyc::cpp::Wire<1>{1};
  model.step();
}

struct AllocatorResult {
  std::uint64_t allocation;
  std::uint64_t accepted;
  std::uint64_t next_free;
};

unsigned popcount(std::uint64_t mask) {
  unsigned count = 0;
  for (unsigned lane = 0; lane < 4; ++lane)
    count += (mask >> lane) & 1u;
  return count;
}

std::uint64_t prefix(std::uint64_t valid, std::uint64_t reserved) {
  std::uint64_t accepted = 0;
  bool live = true;
  for (unsigned lane = 0; lane < 4; ++lane) {
    const bool take = live && ((valid >> lane) & 1u) &&
                      ((reserved >> lane) & 1u);
    if (take)
      accepted |= std::uint64_t{1} << lane;
    live = take;
  }
  return accepted;
}

// Reference model of one `ac.multi_allocator`. Acceptance is a contiguous
// request prefix limited by the candidate free capacity, so the allocator can
// never issue more lanes than it has free slots. Allocation then takes the
// lowest candidate free slots first.
AllocatorResult allocate(std::uint64_t request, std::uint64_t free_mask,
                         std::uint64_t release_mask, bool reuse_allowed) {
  const std::uint64_t candidate_free =
      reuse_allowed ? (free_mask | release_mask) : free_mask;
  const unsigned free_count = popcount(candidate_free);
  AllocatorResult result{};
  unsigned accepted_count = 0;
  bool live = true;
  for (unsigned lane = 0; lane < 4; ++lane) {
    const bool take = live && ((request >> lane) & 1u) &&
                      accepted_count < free_count;
    if (take) {
      result.accepted |= std::uint64_t{1} << lane;
      ++accepted_count;
    }
    live = take;
  }
  unsigned allocated_count = 0;
  for (unsigned slot = 0; slot < 4; ++slot) {
    if (((candidate_free >> slot) & 1u) && allocated_count < accepted_count) {
      result.allocation |= std::uint64_t{1} << slot;
      ++allocated_count;
    }
  }
  const std::uint64_t remaining = candidate_free & ~result.allocation;
  result.next_free = reuse_allowed ? remaining : (remaining | release_mask);
  return result;
}

std::uint64_t age_select(std::uint64_t candidates,
                         const std::array<std::uint64_t, 4> &ages) {
  std::uint64_t remaining = candidates;
  std::uint64_t winners = 0;
  for (unsigned pick = 0; pick < 2; ++pick) {
    unsigned winner = 4;
    for (unsigned lane = 0; lane < 4; ++lane) {
      if (!((remaining >> lane) & 1u))
        continue;
      if (winner == 4 || ages[lane] < ages[winner])
        winner = lane;
    }
    if (winner == 4)
      break;
    winners |= std::uint64_t{1} << winner;
    remaining &= ~(std::uint64_t{1} << winner);
  }
  return winners;
}

// The packed request is wider than one machine word, so it is assembled in the
// generated wire type instead of a std::uint64_t, which would silently drop the
// top four bits of the `valid` field.
using PackedInput = pyc::cpp::Wire<68>;

PackedInput pack_input(std::uint64_t valid, std::uint64_t r0,
                       std::uint64_t r1, std::uint64_t r2,
                       std::uint64_t candidates,
                       const std::array<std::uint64_t, 4> &ages,
                       std::uint64_t current, std::uint64_t resolve,
                       std::uint64_t kill, std::uint64_t add,
                       std::uint64_t identity, std::uint64_t effects,
                       std::uint64_t terminal, std::uint64_t free_mask,
                       std::uint64_t release_mask) {
  PackedInput value{0};
  auto append = [&](std::uint64_t field, unsigned width) {
    value = pyc::cpp::shl(value, width) | PackedInput{field};
  };
  append(valid, 4);
  append(r0, 4);
  append(r1, 4);
  append(r2, 4);
  append(candidates, 4);
  for (std::uint64_t age : ages)
    append(age, 3);
  append(current, 4);
  append(resolve, 4);
  append(kill, 4);
  append(add, 4);
  append(identity, 4);
  append(effects, 4);
  append(terminal, 4);
  append(free_mask, 4);
  append(release_mask, 4);
  return value;
}

std::uint64_t oracle(std::uint64_t valid, std::uint64_t r0,
                     std::uint64_t r1, std::uint64_t r2,
                     std::uint64_t candidates,
                     const std::array<std::uint64_t, 4> &ages,
                     std::uint64_t current, std::uint64_t resolve,
                     std::uint64_t kill, std::uint64_t add,
                     std::uint64_t identity, std::uint64_t effects,
                     std::uint64_t terminal, std::uint64_t free_mask,
                     std::uint64_t release_mask) {
  const std::uint64_t reserved = r0 & r1 & r2;
  const std::uint64_t accepted = prefix(valid, reserved);
  const std::uint64_t accepted_all =
      (valid & reserved) == valid ? valid : 0;
  const std::uint64_t accepted_independent = valid & reserved;
  const AllocatorResult committed =
      allocate(accepted, free_mask, release_mask, false);
  const AllocatorResult reused =
      allocate(accepted, free_mask, release_mask, true);
  const std::uint64_t commit_mask = committed.accepted;
  const std::uint64_t winners = age_select(candidates, ages);
  const std::uint64_t deps_next = (current | add) & ~(resolve & identity) &
                                  ~(kill & identity);
  const std::uint64_t ready = deps_next == 0;
  const std::uint64_t completed = commit_mask & effects & terminal;
  std::uint64_t result = 0;
  auto append = [&](std::uint64_t field, unsigned width) {
    result = (result << width) | (field & ((std::uint64_t{1} << width) - 1));
  };
  append(reserved, 4);
  append(accepted, 4);
  append(accepted_all, 4);
  append(accepted_independent, 4);
  append(committed.allocation, 4);
  append(committed.accepted, 4);
  append(commit_mask, 4);
  append(committed.next_free, 4);
  append(reused.allocation, 4);
  append(reused.accepted, 4);
  append(reused.next_free, 4);
  append(winners, 4);
  append(deps_next, 4);
  append(ready, 1);
  append(completed, 4);
  return result;
}

std::uint64_t run_one(Model &model, PackedInput input,
                      unsigned stall_cycles) {
  model.in_data = input;
  model.in_valid = pyc::cpp::Wire<1>{1};
  for (unsigned cycle = 0; cycle < 64; ++cycle) {
    model.clk = pyc::cpp::Wire<1>{0};
    model.eval();
    const bool accepted = model.in_ready.toBool();
    clock(model);
    if (accepted) {
      model.in_valid = pyc::cpp::Wire<1>{0};
      break;
    }
  }
  model.out_ready = pyc::cpp::Wire<1>{0};
  for (unsigned cycle = 0; cycle < 128; ++cycle) {
    model.eval();
    if (model.out_valid.toBool() && stall_cycles == 0) {
      const std::uint64_t value = model.out_data.value();
      model.out_ready = pyc::cpp::Wire<1>{1};
      clock(model);
      model.out_ready = pyc::cpp::Wire<1>{0};
      return value;
    }
    if (model.out_valid.toBool() && stall_cycles != 0)
      --stall_cycles;
    clock(model);
  }
  throw std::runtime_error("transaction did not complete");
}

} // namespace

int main() {
  Model model;
  model.rst = pyc::cpp::Wire<1>{1};
  clock(model);
  clock(model);
  model.rst = pyc::cpp::Wire<1>{0};

  // splitmix64 keeps every sampled field independent. A plain odd-multiplier
  // LCG alternates bit zero on every draw, which pins an AND of consecutive
  // fields to zero and silently drops most of the algebra out of the fixture.
  std::uint64_t state = 0x9e3779b97f4a7c15ULL;
  auto next = [&]() {
    state += 0x9e3779b97f4a7c15ULL;
    std::uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
  };

  for (unsigned trial = 0; trial < 200; ++trial) {
    const std::uint64_t valid = next() & 15;
    const std::uint64_t r0 = next() & 15;
    const std::uint64_t r1 = next() & 15;
    const std::uint64_t r2 = next() & 15;
    const std::uint64_t candidates = next() & 15;
    std::array<std::uint64_t, 4> ages{};
    for (auto &age : ages)
      age = next() & 7;
    const std::uint64_t current = next() & 15;
    const std::uint64_t resolve = next() & 15;
    const std::uint64_t kill = next() & 15;
    const std::uint64_t add = next() & 15;
    const std::uint64_t identity = next() & 15;
    const std::uint64_t effects = next() & 15;
    const std::uint64_t terminal = next() & 15;
    const std::uint64_t free_mask = next() & 15;
    const std::uint64_t release_mask = next() & 15;
    const PackedInput input =
        pack_input(valid, r0, r1, r2, candidates, ages, current, resolve, kill,
                   add, identity, effects, terminal, free_mask, release_mask);
    const std::uint64_t expected =
        oracle(valid, r0, r1, r2, candidates, ages, current, resolve, kill, add,
               identity, effects, terminal, free_mask, release_mask);
    const std::uint64_t actual = run_one(model, input, next() % 5);
    if (actual != expected) {
      std::cerr << "trial " << trial << " got=" << std::hex << actual
                << " expected=" << expected << std::dec << "\n";
      return 1;
    }
  }
  std::cout << "transaction algebra PASS 200\n";
  return 0;
}
