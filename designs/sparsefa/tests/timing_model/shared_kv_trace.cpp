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
constexpr int kDefaultCount = 128;
constexpr int kBatch = 64;
constexpr int kIndexLatency = 12;
constexpr int kRowLatency = 16;
constexpr int kBusBytes = 32;
constexpr int kCredits = 8;
constexpr int kArrayBubblesAfterEnd = 4;
constexpr int ceil_div(int value, int divisor) {
  return (value + divisor - 1) / divisor;
}
// Per 16-head wave: four 1024-element ordinary-FP32 passes at 64/cycle,
// plus 1024 weight and 16 alpha exponentials at 32/cycle. This is a
// scheduling assumption, not an implementation or a numeric computation.
constexpr int kSoftmaxCycles = 4 * ceil_div(kBatch * 16, 64) +
                               ceil_div(kBatch * 16 + 16, 32);
constexpr int kOutputAckLatency = 12;

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

struct IndexSlot {
  enum State {
    FREE,
    DMA_WAIT,
    DMA_RECEIVE,
    LOADED,
    VALIDATING,
    VALID,
    DISPATCHING
  } state = FREE;
  int batch = -1;
  int due = -1;
  int end = -1;
  int kv_slot = -1;
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
  // NDF cadence: begin, GROUPs, END, four BUBBLEs, take, then next begin.
  return tiles * (1 + groups + 1 + kArrayBubblesAfterEnd + 1);
}

struct Result {
  int cycles;
  int peak_outstanding;
  int row_requests;
  std::vector<Event> events;
};

Result run(int slot_count, int count) {
  if (slot_count < 1 || slot_count > 2) throw std::runtime_error("slot count");
  if (count <= 0 || count % kBatch != 0 || count > 2048)
    throw std::runtime_error("count must be a multiple of 64 in [64, 2048]");
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
  // Q is resident before streaming Index batches; it shares the external bus.
  mark(cycle, "q_load_start");
  cycle += kRowLatency + ceil_div(kHeads * kDim * 2, kBusBytes);
  mark(cycle, "q_load_end");
  const int q_ready_cycle = cycle;

  const int batches = ceil_div(count, kBatch);
  std::vector<IndexSlot> index_slots(2);
  std::vector<Slot> slots(slot_count);
  std::vector<Row> rows;
  std::vector<std::vector<int>> bank_free(slot_count, std::vector<int>(16, 0));
  int next_fill = 0;
  int next_compute = 0;
  int next_index_load = 0;
  int next_index_validate = 0;
  int active_index_dma = -1;
  int active_validator = -1;
  int active_slot = -1;
  int wave = 0;
  int stage = -1;  // 0 QK, 1 softmax, 2 PV, 3 atomic batch commit.
  int stage_end = -1;
  int response_bus_free = cycle;
  int outstanding = 0;
  int first_kv_request = -1;

  auto start_index_dma = [&](int index_slot_id) {
    IndexSlot &index = index_slots.at(index_slot_id);
    assert(index.state == IndexSlot::FREE && active_index_dma < 0);
    index = IndexSlot{IndexSlot::DMA_WAIT, next_index_load,
                      cycle + kIndexLatency, -1, -1};
    active_index_dma = index_slot_id;
    mark(cycle, "index_slot_acquire", index.batch, index_slot_id);
    mark(cycle, "index_dma_start", index.batch, index_slot_id);
    ++next_index_load;
  };
  start_index_dma(0);

  auto start_fill = [&](int slot_id, int index_slot_id) {
    Slot &slot = slots.at(slot_id);
    assert(slot.state == Slot::FREE);
    assert(next_fill < batches);
    slot = Slot{Slot::FILLING, next_fill, 0, 0};
    mark(cycle, "kv_fill_start", next_fill, slot_id);
    IndexSlot &index = index_slots.at(index_slot_id);
    assert(index.state == IndexSlot::VALID && index.batch == next_fill);
    index.state = IndexSlot::DISPATCHING;
    index.kv_slot = slot_id;
    mark(cycle, "index_dispatch_start", next_fill, index_slot_id);
    ++next_fill;
  };

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

  while (next_compute < batches || active_slot >= 0) {
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

    if (active_index_dma >= 0) {
      IndexSlot &index = index_slots[active_index_dma];
      if (index.state == IndexSlot::DMA_RECEIVE && cycle == index.end) {
        mark(cycle, "index_dma_end", index.batch, active_index_dma);
        index.state = IndexSlot::LOADED;
        active_index_dma = -1;
      }
    }
    if (active_validator >= 0) {
      IndexSlot &index = index_slots[active_validator];
      if (cycle == index.end) {
        mark(cycle, "index_validate_end", index.batch, active_validator);
        mark(cycle, "index_batch_valid", index.batch, active_validator);
        index.state = IndexSlot::VALID;
        ++next_index_validate;
        active_validator = -1;
      }
    }

    // Index and KV responses share one nonpreemptive 32 B/cycle stream.
    if (cycle >= response_bus_free) {
      IndexSlot *index = active_index_dma >= 0
                             ? &index_slots[active_index_dma] : nullptr;
      if (index && index->state == IndexSlot::DMA_WAIT && index->due <= cycle) {
        index->state = IndexSlot::DMA_RECEIVE;
        index->end = cycle + ceil_div(kBatch * 4, kBusBytes);
        response_bus_free = index->end;
        mark(cycle, "index_response_start", index->batch, active_index_dma);
      } else {
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
    }

    if (active_validator < 0 && next_index_validate < batches) {
      for (int index_slot_id = 0; index_slot_id < 2; ++index_slot_id) {
        IndexSlot &index = index_slots[index_slot_id];
        if (index.state == IndexSlot::LOADED &&
            index.batch == next_index_validate) {
          index.state = IndexSlot::VALIDATING;
          index.end = cycle + kBatch;  // Assumed-valid IDs, no content check.
          active_validator = index_slot_id;
          mark(cycle, "index_validate_start", index.batch, index_slot_id);
          break;
        }
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

    if (active_slot < 0 && next_compute < batches) {
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

    // A validated batch can reserve an independent KV slot immediately.
    if (next_fill < batches) {
      for (int index_slot_id = 0; index_slot_id < 2; ++index_slot_id) {
        if (index_slots[index_slot_id].state != IndexSlot::VALID ||
            index_slots[index_slot_id].batch != next_fill) continue;
        for (int slot_id = 0; slot_id < slot_count; ++slot_id) {
          if (slots[slot_id].state == Slot::FREE) {
            start_fill(slot_id, index_slot_id);
            break;
          }
        }
        break;
      }
    }

    // Only two 64-entry Index slots; a new batch waits until a slot is free.
    // Starting after the preceding batch validates exposes Index/KV overlap.
    if (active_index_dma < 0 && next_index_load < batches &&
        next_index_load <= next_index_validate) {
      for (int index_slot_id = 0; index_slot_id < 2; ++index_slot_id) {
        if (index_slots[index_slot_id].state == IndexSlot::FREE) {
          start_index_dma(index_slot_id);
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
          if (slot.issued == kBatch) {
            for (int index_slot_id = 0; index_slot_id < 2; ++index_slot_id) {
              IndexSlot &index = index_slots[index_slot_id];
              if (index.state == IndexSlot::DISPATCHING &&
                  index.batch == slot.batch && index.kv_slot == slot_id) {
                mark(cycle, "index_slot_release", index.batch, index_slot_id);
                index = IndexSlot{};
                break;
              }
            }
          }
          break;
        }
      }
    }
    ++cycle;
  }

  assert(q_ready_cycle <= first_kv_request);
  assert(next_index_load == batches && next_index_validate == batches);
  for (const IndexSlot &index : index_slots) assert(index.state == IndexSlot::FREE);
  assert(outstanding == 0);
  assert(result.peak_outstanding <= kCredits);
  assert(result.row_requests == count);
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
  if (argc != 2 && argc != 3) {
    std::cerr << "usage: shared_kv_trace OUTPUT_PREFIX [INDEX_COUNT]\n";
    return 2;
  }
  const std::string prefix = argv[1];
  int count = kDefaultCount;
  if (argc == 3) {
    const std::string argument = argv[2];
    std::size_t parsed = 0;
    try {
      count = std::stoi(argument, &parsed);
    } catch (const std::exception &) {
      std::cerr << "invalid INDEX_COUNT: expected a multiple of 64 in [64, 2048]\n";
      return 2;
    }
    if (parsed != argument.size() || count <= 0 || count % kBatch != 0 ||
        count > 2048) {
      std::cerr << "invalid INDEX_COUNT: expected a multiple of 64 in [64, 2048]\n";
      return 2;
    }
  }
  const Result one = run(1, count);
  const Result two = run(2, count);
  write_csv(prefix + "-1slot.csv", one);
  write_csv(prefix + "-2slot.csv", two);
  std::cout << "H=" << kHeads << " D=" << kDim << " count=" << count
            << " index_batch=64 kv_credit=" << kCredits
            << " memory_latency=" << kRowLatency
            << " bus_bytes_per_cycle=" << kBusBytes << '\n';
  std::cout << "one_slot_cycles=" << one.cycles
            << " two_slot_cycles=" << two.cycles
            << " saved_cycles=" << one.cycles - two.cycles << '\n';
  std::cout << "row_requests=" << two.row_requests
            << " peak_outstanding=" << two.peak_outstanding << '\n';
}
