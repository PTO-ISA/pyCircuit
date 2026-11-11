// Executable cycle model for the unimplemented Shared-KV target.
// It models scheduling and bounded resources, not pyCircuit-generated hardware.
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr int kHeads = 64;
constexpr int kDim = 64;
constexpr int kCount = 128;
constexpr int kBatch = 64;
constexpr int kIndexLatency = 12;
constexpr int kRowLatency = 16;
constexpr int kBusBytes = 32;
constexpr int kCredits = 8;
constexpr int kArrayDrain = 5;
constexpr int kSoftmaxCycles = 16;
constexpr int kOutputAckLatency = 12;

int ceil_div(int value, int divisor) { return (value + divisor - 1) / divisor; }

struct Event {
  int cycle;
  std::string kind;
  int batch = -1;
  int slot = -1;
  int wave = -1;
  int position = -1;
};

struct Slot {
  enum State { FREE, FILLING, READY, IN_USE } state = FREE;
  int batch = -1;
  int issued = 0;
  int completed = 0;
};

struct Row {
  int batch;
  int slot;
  int position;
  int due;
  int write_done = -1;
  bool responded = false;
  bool completed = false;
};

int array_cycles(int columns, int reduction) {
  const int tiles = ceil_div(columns, 16);
  const int groups = ceil_div(reduction, 16);
  // Each column tile has its own begin, accepted GROUPs, END, drain, take.
  return tiles * (1 + groups + 1 + kArrayDrain + 1);
}

struct Result {
  int cycles;
  int peak_outstanding;
  int row_requests;
  std::vector<Event> events;
};

Result run(int slot_count) {
  if (slot_count < 1 || slot_count > 3) throw std::runtime_error("slot count");
  Result result{};
  auto &events = result.events;
  auto mark = [&](int cycle, std::string kind, int batch = -1, int slot = -1,
                  int wave = -1, int position = -1) {
    events.push_back({cycle, std::move(kind), batch, slot, wave, position});
  };

  int cycle = 0;
  mark(cycle, "count_read_start");
  cycle += kIndexLatency + ceil_div(4, kBusBytes);
  mark(cycle, "count_read_end");
  for (int batch = 0; batch < ceil_div(kCount, kBatch); ++batch) {
    mark(cycle, "index_dma_start", batch);
    cycle += kIndexLatency + ceil_div(kBatch * 4, kBusBytes);
    mark(cycle, "index_dma_end", batch);
    mark(cycle, "index_validate_start", batch);
    cycle += kBatch;  // Validation budget for assumed-valid IDs, not data checks.
    mark(cycle, "index_validate_end", batch);
  }
  mark(cycle, "index_valid_barrier");
  const int index_barrier = cycle;
  // This first target keeps all Q heads in HeadState before KV/array execution.
  // Q is a contiguous FP16/BF16 input and shares the modeled external bus.
  mark(cycle, "q_load_start");
  cycle += kRowLatency + ceil_div(kHeads * kDim * 2, kBusBytes);
  mark(cycle, "q_load_end");
  const int q_ready_cycle = cycle;

  std::vector<Slot> slots(slot_count);
  std::vector<Row> rows;
  std::vector<std::vector<int>> bank_free(slot_count, std::vector<int>(16, 0));
  int next_fill = 0;
  int next_compute = 0;
  int active_slot = -1;
  int wave = 0;
  int stage = -1;  // 0 QK, 1 softmax, 2 PV, 3 atomic batch commit.
  int stage_end = -1;
  int response_bus_free = cycle;
  int outstanding = 0;
  int first_kv_request = -1;

  auto start_fill = [&](int slot_id) {
    Slot &slot = slots.at(slot_id);
    assert(slot.state == Slot::FREE);
    assert(next_fill < ceil_div(kCount, kBatch));
    slot = Slot{Slot::FILLING, next_fill, 0, 0};
    mark(cycle, "kv_fill_start", next_fill, slot_id);
    ++next_fill;
  };
  start_fill(0);

  auto begin_stage = [&](int next_stage) {
    stage = next_stage;
    const int batch = slots[active_slot].batch;
    if (stage == 0) {
      mark(cycle, "qk_start", batch, active_slot, wave);
      stage_end = cycle + array_cycles(kBatch, kDim);
    } else if (stage == 1) {
      mark(cycle, "softmax_start", batch, active_slot, wave);
      stage_end = cycle + kSoftmaxCycles;
    } else if (stage == 2) {
      mark(cycle, "pv_start", batch, active_slot, wave);
      stage_end = cycle + array_cycles(kDim, kBatch);
    } else {
      mark(cycle, "state_commit_start", batch, active_slot, wave);
      stage_end = cycle + 1;
    }
  };

  while (next_compute < ceil_div(kCount, kBatch) || active_slot >= 0) {
    if (cycle > 100000) throw std::runtime_error("simulation did not terminate");

    for (Row &row : rows) {
      if (!row.completed && row.write_done == cycle) {
        row.completed = true;
        --outstanding;
        Slot &slot = slots.at(row.slot);
        assert(slot.state == Slot::FILLING && slot.batch == row.batch);
        ++slot.completed;
        mark(cycle, "kv_row_complete", row.batch, row.slot, -1, row.position);
        if (slot.completed == kBatch) {
          assert(slot.issued == kBatch);
          slot.state = Slot::READY;
          mark(cycle, "kv_fill_end", row.batch, row.slot);
        }
      }
    }

    // One nonpreemptive response stream. A D64 row uses four 32-byte beats.
    if (cycle >= response_bus_free) {
      Row *chosen = nullptr;
      for (Row &row : rows) {
        if (!row.responded && row.due <= cycle &&
            (!chosen || row.due < chosen->due))
          chosen = &row;
      }
      if (chosen) {
        const int words = ceil_div(kDim, 16);
        const int bank = chosen->position % 16;
        const int start = std::max(cycle, bank_free[chosen->slot][bank]);
        chosen->write_done = start + words;
        bank_free[chosen->slot][bank] = chosen->write_done;
        response_bus_free = cycle + words;
        chosen->responded = true;
        mark(cycle, "kv_response_start", chosen->batch, chosen->slot, -1,
             chosen->position);
      }
    }

    if (active_slot >= 0 && cycle == stage_end) {
      const int batch = slots[active_slot].batch;
      if (stage == 0) {
        mark(cycle, "qk_end", batch, active_slot, wave);
        begin_stage(1);
      } else if (stage == 1) {
        mark(cycle, "softmax_end", batch, active_slot, wave);
        begin_stage(2);
      } else if (stage == 2) {
        mark(cycle, "pv_end", batch, active_slot, wave);
        begin_stage(3);
      } else {
        mark(cycle, "state_commit_end", batch, active_slot, wave);
        ++wave;
        if (wave < ceil_div(kHeads, 16)) {
          begin_stage(0);
        } else {
          mark(cycle, "batch_compute_end", batch, active_slot);
          slots[active_slot].state = Slot::FREE;
          slots[active_slot].batch = -1;
          active_slot = -1;
          ++next_compute;
        }
      }
    }

    if (active_slot < 0 && next_compute < ceil_div(kCount, kBatch)) {
      for (int slot_id = 0; slot_id < slot_count; ++slot_id) {
        Slot &slot = slots[slot_id];
        if (slot.state == Slot::READY && slot.batch == next_compute) {
          assert(slot.completed == kBatch);
          slot.state = Slot::IN_USE;
          active_slot = slot_id;
          wave = 0;
          mark(cycle, "batch_compute_start", slot.batch, slot_id);
          begin_stage(0);
          break;
        }
      }
    }

    // Prefetch begins only after a preceding batch has entered the array.
    if (next_fill < ceil_div(kCount, kBatch) && next_compute > 0) {
      for (int slot_id = 0; slot_id < slot_count; ++slot_id) {
        if (slots[slot_id].state == Slot::FREE) {
          start_fill(slot_id);
          break;
        }
      }
    } else if (next_fill < ceil_div(kCount, kBatch) && active_slot >= 0) {
      for (int slot_id = 0; slot_id < slot_count; ++slot_id) {
        if (slots[slot_id].state == Slot::FREE) {
          start_fill(slot_id);
          break;
        }
      }
    }

    if (outstanding < kCredits) {
      for (int slot_id = 0; slot_id < slot_count; ++slot_id) {
        Slot &slot = slots[slot_id];
        if (slot.state == Slot::FILLING && slot.issued < kBatch) {
          const int position = slot.issued++;
          rows.push_back(Row{slot.batch, slot_id, position,
                             cycle + kRowLatency + (position * 7) % 5});
          ++outstanding;
          ++result.row_requests;
          result.peak_outstanding = std::max(result.peak_outstanding, outstanding);
          if (first_kv_request < 0) first_kv_request = cycle;
          mark(cycle, "kv_row_request", slot.batch, slot_id, -1, position);
          break;
        }
      }
    }
    ++cycle;
  }

  assert(index_barrier < q_ready_cycle && q_ready_cycle <= first_kv_request);
  assert(outstanding == 0);
  assert(result.peak_outstanding <= kCredits);
  assert(result.row_requests == kCount);
  mark(cycle, "output_write_start");
  cycle += ceil_div(kHeads * kDim * 2, kBusBytes) + kOutputAckLatency;
  mark(cycle, "output_write_ack");
  mark(cycle, "completion");
  result.cycles = cycle;
  return result;
}

void write_csv(const std::string &path, const Result &result) {
  std::ofstream output(path);
  if (!output) throw std::runtime_error("cannot open trace output");
  output << "cycle,kind,batch,slot,wave,position\n";
  for (const Event &event : result.events)
    output << event.cycle << ',' << event.kind << ',' << event.batch << ','
           << event.slot << ',' << event.wave << ',' << event.position << '\n';
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: shared_kv_trace OUTPUT_PREFIX\n";
    return 2;
  }
  const std::string prefix = argv[1];
  const Result one = run(1);
  const Result two = run(2);
  write_csv(prefix + "-1slot.csv", one);
  write_csv(prefix + "-2slot.csv", two);
  std::cout << "H=" << kHeads << " D=" << kDim << " count=" << kCount
            << " index_batch=64 kv_credit=" << kCredits
            << " memory_latency=" << kRowLatency
            << " bus_bytes_per_cycle=" << kBusBytes << '\n';
  std::cout << "one_slot_cycles=" << one.cycles
            << " two_slot_cycles=" << two.cycles
            << " saved_cycles=" << one.cycles - two.cycles << '\n';
  std::cout << "row_requests=" << two.row_requests
            << " peak_outstanding=" << two.peak_outstanding << '\n';
}
