#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

using Model = pyc::gen::memory_ordering;
using PackedInput = pyc::cpp::Wire<36>;

void clock(Model &model) {
  model.clk = pyc::cpp::Wire<1>{0};
  model.step();
  model.clk = pyc::cpp::Wire<1>{1};
  model.step();
}

// Framework-neutral Core-local ordering model. Addresses are compared for
// real, so an aliasing bug in the DUT is observable: alias requires a resolved
// older store at the same address, and a disjoint proof requires every older
// store to be resolved and at a different address.
struct Store {
  bool valid = false;
  bool resolved = false;
  bool ready = false;
  std::uint64_t address = 0;
};

struct Lane {
  std::uint64_t address = 0;
  bool pending = false;
  bool executed = false;
  bool identity = false;
};

struct Derived {
  std::uint64_t alias = 0;
  std::uint64_t disjoint = 0;
  std::uint64_t ready = 0;
};

Derived derive(const std::array<Store, 2> &stores,
               const std::array<Lane, 4> &lanes) {
  Derived derived{};
  for (unsigned lane = 0; lane < 4; ++lane) {
    bool aliasing = false;
    bool dataReady = false;
    bool allResolved = true;
    for (const Store &store : stores) {
      if (!store.valid)
        continue;
      if (!store.resolved) {
        allResolved = false;
        continue;
      }
      if (store.address == lanes[lane].address) {
        aliasing = true;
        dataReady = dataReady || store.ready;
      }
    }
    derived.alias |= aliasing ? (std::uint64_t{1} << lane) : 0;
    derived.ready |= dataReady ? (std::uint64_t{1} << lane) : 0;
    derived.disjoint |=
        (allResolved && !aliasing) ? (std::uint64_t{1} << lane) : 0;
  }
  return derived;
}

PackedInput pack_input(std::uint64_t pending, std::uint64_t alias,
                       std::uint64_t disjoint, std::uint64_t ready,
                       std::uint64_t executed, std::uint64_t identity,
                       std::uint64_t producer, std::uint64_t consumer,
                       std::uint64_t killed) {
  PackedInput value{0};
  auto append = [&](std::uint64_t field) {
    value = pyc::cpp::shl(value, 4) | PackedInput{field};
  };
  append(pending);
  append(alias);
  append(disjoint);
  append(ready);
  append(executed);
  append(identity);
  append(producer);
  append(consumer);
  append(killed);
  return value;
}

std::uint64_t oracle(std::uint64_t pending, std::uint64_t alias,
                     std::uint64_t disjoint, std::uint64_t ready,
                     std::uint64_t executed, std::uint64_t identity,
                     std::uint64_t producer, std::uint64_t consumer,
                     std::uint64_t killed,
                     std::array<unsigned, 5> *counter) {
  // A killed outstanding load is stale regardless of its response identity.
  const std::uint64_t stale = pending & (~identity | killed);
  const std::uint64_t qualified = pending & ~stale;
  const std::uint64_t forward = qualified & alias & ready & ~executed;
  const std::uint64_t replay = qualified & alias & executed;
  const std::uint64_t bypass = qualified & disjoint & ~alias;
  const std::uint64_t wait = qualified & ~(forward | replay | bypass);
  if (counter) {
    (*counter)[0] += __builtin_popcountll(wait);
    (*counter)[1] += __builtin_popcountll(bypass);
    (*counter)[2] += __builtin_popcountll(forward);
    (*counter)[3] += __builtin_popcountll(replay);
    (*counter)[4] += __builtin_popcountll(stale);
  }
  std::uint64_t result = producer & consumer;
  for (std::uint64_t field : {wait, bypass, forward, replay, stale})
    result = (result << 4) | (field & 15);
  return result;
}

// Check the DUT's own output, not the oracle's intermediate masks: for every
// qualified lane exactly one live disposition must be set, and a lane that is
// not qualified (identity mismatch or flush kill) must reach none of them and
// must be reported as stale.
bool check_disposition_conformance(std::uint64_t actual,
                                   std::uint64_t pending,
                                   std::uint64_t identity,
                                   std::uint64_t killed,
                                   const char *label) {
  const std::uint64_t qualified = pending & identity & ~killed;
  // Output packing is applies, wait, bypass, forward, replay, stale.
  const std::array<std::uint64_t, 4> masks{
      (actual >> 16) & 15, (actual >> 12) & 15, (actual >> 8) & 15,
      (actual >> 4) & 15};
  const std::uint64_t stale_mask = actual & 15;
  for (unsigned lane = 0; lane < 4; ++lane) {
    const std::uint64_t bit = std::uint64_t{1} << lane;
    unsigned live = 0;
    for (unsigned index = 0; index < 4; ++index)
      live += (masks[index] & bit) ? 1u : 0u;
    const bool stale = (stale_mask & bit) != 0;
    if ((qualified & bit) && (live != 1 || stale)) {
      std::cerr << label << ": lane " << lane
                << " must have exactly one live disposition\n";
      return false;
    }
    if (!(qualified & bit) && (live != 0 || stale != ((pending & bit) != 0))) {
      std::cerr << label << ": lane " << lane
                << " must be consumed as stale only\n";
      return false;
    }
  }
  return true;
}

std::uint64_t run_one(Model &model, PackedInput input) {
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
    if (model.out_valid.toBool()) {
      const std::uint64_t value = model.out_data.value();
      model.out_ready = pyc::cpp::Wire<1>{1};
      clock(model);
      model.out_ready = pyc::cpp::Wire<1>{0};
      return value;
    }
    clock(model);
  }
  throw std::runtime_error("memory ordering did not complete");
}

} // namespace

int main() {
  Model model;
  model.rst = pyc::cpp::Wire<1>{1};
  clock(model);
  clock(model);
  model.rst = pyc::cpp::Wire<1>{0};

  std::uint64_t state = 0x243f6a8885a308d3ULL;
  auto next = [&]() {
    state += 0x9e3779b97f4a7c15ULL;
    std::uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
  };

  std::array<unsigned, 5> counter{};
  for (unsigned trial = 0; trial < 200; ++trial) {
    std::array<Store, 2> stores{};
    for (Store &store : stores) {
      store.valid = (next() & 1) != 0;
      store.resolved = (next() & 1) != 0;
      store.ready = (next() & 1) != 0;
      store.address = next() & 7;
    }
    std::array<Lane, 4> lanes{};
    std::uint64_t pending = 0, executed = 0, identity = 0;
    for (unsigned lane = 0; lane < 4; ++lane) {
      lanes[lane].address = next() & 7;
      lanes[lane].pending = (next() & 1) != 0;
      lanes[lane].executed = (next() & 1) != 0;
      lanes[lane].identity = (next() & 1) != 0;
      pending |= lanes[lane].pending ? (std::uint64_t{1} << lane) : 0;
      executed |= lanes[lane].executed ? (std::uint64_t{1} << lane) : 0;
      identity |= lanes[lane].identity ? (std::uint64_t{1} << lane) : 0;
    }
    const Derived derived = derive(stores, lanes);
    const std::uint64_t producer = next() & 15;
    const std::uint64_t consumer = next() & 15;
    // Flush model: an event boundary invalidates outstanding loads whose
    // recovery epoch changed or whose slot is younger than the boundary.
    const std::uint64_t event_valid = next() & 15;
    const std::uint64_t epoch_changed = next() & 15;
    const std::uint64_t younger = next() & 15;
    const std::uint64_t killed =
        event_valid & pending & (epoch_changed | younger);
    const PackedInput input =
        pack_input(pending, derived.alias, derived.disjoint, derived.ready,
                   executed, identity, producer, consumer, killed);
    const std::uint64_t expected =
        oracle(pending, derived.alias, derived.disjoint, derived.ready,
               executed, identity, producer, consumer, killed, &counter);
    const std::uint64_t actual = run_one(model, input);
    if (actual != expected) {
      std::cerr << "trial " << trial << " got=" << std::hex << actual
                << " expected=" << expected << std::dec << "\n";
      return 1;
    }
    if (!check_disposition_conformance(actual, pending, identity, killed,
                                       "trial"))
      return 1;
  }

  // Directed cases. The random stream reaches every disposition, but these pin
  // the boundary conditions explicitly: an unknown store address must wait, a
  // resolved non-alias store discharges, an alias waits until its data is
  // ready, a late violation replays, and a stale response can only be dropped.
  auto makeStore = [](bool valid, bool resolved, bool ready,
                      std::uint64_t address) {
    Store store;
    store.valid = valid;
    store.resolved = resolved;
    store.ready = ready;
    store.address = address;
    return store;
  };
  auto makeLane = [](std::uint64_t address, bool pending, bool executed,
                     bool identity) {
    Lane lane;
    lane.address = address;
    lane.pending = pending;
    lane.executed = executed;
    lane.identity = identity;
    return lane;
  };
  struct Directed {
    std::array<Store, 2> stores;
    std::array<Lane, 4> lanes;
    std::uint64_t killed;
  };
  std::vector<Directed> directed;
  // Unknown store address: alias and disjoint are both unprovable.
  directed.push_back(
      {{makeStore(true, false, true, 3), makeStore(false, false, false, 0)},
       {makeLane(3, true, false, true), makeLane(4, true, false, true),
        makeLane(5, false, false, true), makeLane(6, false, false, true)},
       0});
  // Every older store resolved and disjoint: bypass.
  directed.push_back(
      {{makeStore(true, true, false, 1), makeStore(true, true, false, 2)},
       {makeLane(3, true, false, true), makeLane(4, true, false, true),
        makeLane(5, false, false, true), makeLane(6, false, false, true)},
       0});
  // Alias with data ready and not yet executed: forward.
  directed.push_back(
      {{makeStore(true, true, true, 3), makeStore(false, false, false, 0)},
       {makeLane(3, true, false, true), makeLane(4, false, false, true),
        makeLane(5, false, false, true), makeLane(6, false, false, true)},
       0});
  // Alias on an already executed load: late violation, replay.
  directed.push_back(
      {{makeStore(true, true, true, 3), makeStore(false, false, false, 0)},
       {makeLane(3, true, true, true), makeLane(4, false, false, true),
        makeLane(5, false, false, true), makeLane(6, false, false, true)},
       0});
  // Alias, but the response identity is stale: it can only be dropped.
  directed.push_back(
      {{makeStore(true, true, true, 3), makeStore(false, false, false, 0)},
       {makeLane(3, true, false, false), makeLane(4, false, false, false),
        makeLane(5, false, false, false), makeLane(6, false, false, false)},
       0});
  // Flush with outstanding: the event advances the epoch for lane zero, so the
  // killed outstanding load is consumed as stale even though its response
  // identity still matches.
  directed.push_back(
      {{makeStore(true, true, true, 3), makeStore(false, false, false, 0)},
       {makeLane(3, true, false, true), makeLane(4, false, false, true),
        makeLane(5, false, false, true), makeLane(6, false, false, true)},
       1});

  for (const Directed &test : directed) {
    std::uint64_t pending = 0, executed = 0, identity = 0;
    for (unsigned lane = 0; lane < 4; ++lane) {
      pending |= test.lanes[lane].pending ? (std::uint64_t{1} << lane) : 0;
      executed |= test.lanes[lane].executed ? (std::uint64_t{1} << lane) : 0;
      identity |= test.lanes[lane].identity ? (std::uint64_t{1} << lane) : 0;
    }
    const Derived derived = derive(test.stores, test.lanes);
    const PackedInput input =
        pack_input(pending, derived.alias, derived.disjoint, derived.ready,
                   executed, identity, 15, 15, test.killed);
    const std::uint64_t expected =
        oracle(pending, derived.alias, derived.disjoint, derived.ready,
               executed, identity, 15, 15, test.killed, &counter);
    const std::uint64_t actual = run_one(model, input);
    if (actual != expected) {
      std::cerr << "directed case got=" << std::hex << actual
                << " expected=" << expected << std::dec << "\n";
      return 1;
    }
    if (!check_disposition_conformance(actual, pending, identity, test.killed,
                                       "directed case"))
      return 1;
  }

  // Non-vacuity: every disposition must actually occur, otherwise the fixture
  // proves nothing about that path.
  const char *names[5] = {"wait", "bypass", "forward", "replay", "stale"};
  for (unsigned index = 0; index < counter.size(); ++index) {
    if (counter[index] == 0) {
      std::cerr << "disposition " << names[index] << " never occurred\n";
      return 1;
    }
  }
  std::cout << "memory ordering PASS 200 wait=" << counter[0]
            << " bypass=" << counter[1] << " forward=" << counter[2]
            << " replay=" << counter[3] << " stale=" << counter[4] << "\n";
  return 0;
}
