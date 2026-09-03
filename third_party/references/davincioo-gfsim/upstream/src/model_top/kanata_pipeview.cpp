#include "model_top/kanata_pipeview.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "davincioo/model/rob.hpp"

namespace davincioo::model_top {

namespace {

struct TimedEvent {
  std::uint64_t cycle = 0;
  int order = 0;
  std::uint64_t inst_id = 0;
  std::string line;
};

bool HasCycleStamp(std::uint64_t cycle) {
  return cycle != kUnsetCycleStamp;
}

std::string EscapeKanataText(const std::string& text) {
  std::string escaped;
  escaped.reserve(text.size());
  for (const char ch : text) {
    if (ch == '\t' || ch == '\n' || ch == '\r') {
      escaped.push_back(' ');
    } else {
      escaped.push_back(ch);
    }
  }
  return escaped;
}

std::string MakeTimestampNote(const PTOInst& inst) {
  std::ostringstream stream;
  stream << "alloc=" << inst.timestamps.rob_alloc_cycle
         << " rename=" << inst.timestamps.rename_cycle
         << " dispatch=" << inst.timestamps.dispatch_cycle
         << " issue=" << inst.timestamps.issue_cycle
         << " engine_pop=" << inst.timestamps.engine_pop_cycle
         << " engine_complete=" << inst.timestamps.engine_complete_cycle
         << " retire=" << inst.timestamps.rob_retire_cycle;
  return stream.str();
}

std::string MakeExecStageName(const PTOInst& inst) {
  std::ostringstream stream;
  stream << ToString(inst.engine_kind) << inst.engine_id;
  return stream.str();
}

void AddEvent(
    std::vector<TimedEvent>& events,
    std::uint64_t cycle,
    int order,
    std::uint64_t inst_id,
    std::string line) {
  if (!HasCycleStamp(cycle)) {
    return;
  }
  events.push_back(TimedEvent{
      .cycle = cycle,
      .order = order,
      .inst_id = inst_id,
      .line = std::move(line),
  });
}

}  // namespace

void WriteKanataPipeView(const std::filesystem::path& path, const SimulationResult& result) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream stream(path, std::ios::out | std::ios::trunc);
  if (!stream) {
    throw std::runtime_error("failed to open Kanata pipeview output: " + path.string());
  }

  stream << "Kanata\t0004\n";
  if (result.processed.empty()) {
    stream << "C=\t0\n";
    return;
  }

  std::vector<TimedEvent> events;
  events.reserve(result.processed.size() * 10);

  std::uint64_t retire_id = 0;
  for (std::size_t index = 0; index < result.processed.size(); ++index) {
    const PTOInst& inst = result.processed[index];
    const std::uint64_t inst_id = static_cast<std::uint64_t>(index);
    const std::uint64_t sim_id = inst.sequence_id;
    const std::uint64_t thread_id = inst.block_idx;

    AddEvent(events, inst.timestamps.rob_alloc_cycle, 0, inst_id,
             "I\t" + std::to_string(inst_id) + "\t" + std::to_string(sim_id) + "\t" + std::to_string(thread_id));
    AddEvent(events, inst.timestamps.rob_alloc_cycle, 1, inst_id,
             "L\t" + std::to_string(inst_id) + "\t0\t" + EscapeKanataText(DumpPTOInst(inst)));
    AddEvent(events, inst.timestamps.rob_alloc_cycle, 2, inst_id,
             "L\t" + std::to_string(inst_id) + "\t1\t" + EscapeKanataText(MakeTimestampNote(inst)));
    AddEvent(events, inst.timestamps.rob_alloc_cycle, 3, inst_id,
             "S\t" + std::to_string(inst_id) + "\t0\tROB");
    AddEvent(events, inst.timestamps.rename_cycle, 4, inst_id,
             "S\t" + std::to_string(inst_id) + "\t0\tRENAME");
    AddEvent(events, inst.timestamps.dispatch_cycle, 5, inst_id,
             "S\t" + std::to_string(inst_id) + "\t0\tDISP");
    AddEvent(events, inst.timestamps.issue_cycle, 6, inst_id,
             "S\t" + std::to_string(inst_id) + "\t0\tISSQ");
    AddEvent(events, inst.timestamps.engine_pop_cycle, 7, inst_id,
             "S\t" + std::to_string(inst_id) + "\t0\t" + MakeExecStageName(inst));
    AddEvent(events, inst.timestamps.engine_complete_cycle, 8, inst_id,
             "S\t" + std::to_string(inst_id) + "\t0\tWB");
    AddEvent(events, inst.timestamps.rob_retire_cycle, 9, inst_id,
             "S\t" + std::to_string(inst_id) + "\t0\tRET");
    AddEvent(events, inst.timestamps.rob_retire_cycle, 10, inst_id,
             "R\t" + std::to_string(inst_id) + "\t" + std::to_string(retire_id++) + "\t0");
  }

  std::sort(events.begin(), events.end(), [](const TimedEvent& lhs, const TimedEvent& rhs) {
    if (lhs.cycle != rhs.cycle) {
      return lhs.cycle < rhs.cycle;
    }
    if (lhs.order != rhs.order) {
      return lhs.order < rhs.order;
    }
    return lhs.inst_id < rhs.inst_id;
  });

  std::uint64_t current_cycle = events.front().cycle;
  stream << "C=\t" << current_cycle << "\n";
  for (const TimedEvent& event : events) {
    if (event.cycle > current_cycle) {
      stream << "C\t" << (event.cycle - current_cycle) << "\n";
      current_cycle = event.cycle;
    }
    stream << event.line << "\n";
  }
}

}  // namespace davincioo::model_top
