#include "model_top/perfetto_trace.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "frontend/trace.hpp"

namespace davincioo::model_top {

namespace {

struct TrackRef {
  int pid = 0;
  int tid = 0;
  std::string process_name;
  std::string thread_name;
  int sort_index = 0;
};

struct RobDelta {
  std::uint64_t cycle = 0;
  int order = 0;
  int delta = 0;
};

constexpr int kRobPid = 1;
constexpr int kRobTid = 1;
constexpr int kCorePidBase = 1000;
constexpr int kScalarTidBase = 100;
constexpr int kVecTidBase = 200;
constexpr int kCubeTidBase = 300;
constexpr int kTmaTidBase = 400;

std::uint64_t DurationFromCycles(std::uint64_t start_cycle, std::uint64_t end_cycle) {
  if (start_cycle == kUnsetCycleStamp || end_cycle == kUnsetCycleStamp || end_cycle < start_cycle) {
    return 0;
  }
  return end_cycle - start_cycle;
}

TrackRef MakeEngineTrack(PTOEngineKind kind, std::uint64_t engine_id, std::uint64_t block_idx) {
  const int pid = kCorePidBase + static_cast<int>(block_idx);
  switch (kind) {
    case PTOEngineKind::Scalar:
      return TrackRef{
          .pid = pid,
          .tid = static_cast<int>(kScalarTidBase + engine_id),
          .process_name = "CORE " + std::to_string(block_idx),
          .thread_name = "SCALAR" + std::to_string(engine_id),
          .sort_index = static_cast<int>(block_idx * 1000 + kScalarTidBase + engine_id),
      };
    case PTOEngineKind::Vec:
      return TrackRef{
          .pid = pid,
          .tid = static_cast<int>(kVecTidBase + engine_id),
          .process_name = "CORE " + std::to_string(block_idx),
          .thread_name = "VEC" + std::to_string(engine_id),
          .sort_index = static_cast<int>(block_idx * 1000 + kVecTidBase + engine_id),
      };
    case PTOEngineKind::Cube:
      return TrackRef{
          .pid = pid,
          .tid = static_cast<int>(kCubeTidBase + engine_id),
          .process_name = "CORE " + std::to_string(block_idx),
          .thread_name = "CUBE" + std::to_string(engine_id),
          .sort_index = static_cast<int>(block_idx * 1000 + kCubeTidBase + engine_id),
      };
    case PTOEngineKind::Tma:
      return TrackRef{
          .pid = pid,
          .tid = static_cast<int>(kTmaTidBase + engine_id),
          .process_name = "CORE " + std::to_string(block_idx),
          .thread_name = "TMA" + std::to_string(engine_id),
          .sort_index = static_cast<int>(block_idx * 1000 + kTmaTidBase + engine_id),
      };
    default:
      return TrackRef{};
  }
}

std::string MakeEngineSliceName(const PTOInst& inst) {
  std::ostringstream stream;
  stream << OpcodeName(inst)
         << " seq=" << inst.sequence_id
         << " c" << inst.timestamps.engine_pop_cycle
         << "->" << inst.timestamps.engine_complete_cycle;
  return stream.str();
}

std::string MakeProcessNameEvent(int pid, const std::string& name) {
  std::ostringstream stream;
  stream << "{\"ph\":\"M\",\"pid\":" << pid << ",\"tid\":0,"
         << "\"name\":\"process_name\",\"args\":{\"name\":\"" << frontend::JsonEscape(name) << "\"}}";
  return stream.str();
}

std::string MakeProcessSortEvent(int pid, int sort_index) {
  std::ostringstream stream;
  stream << "{\"ph\":\"M\",\"pid\":" << pid << ",\"tid\":0,"
         << "\"name\":\"process_sort_index\",\"args\":{\"sort_index\":" << sort_index << "}}";
  return stream.str();
}

std::string MakeThreadNameEvent(const TrackRef& track) {
  std::ostringstream stream;
  stream << "{\"ph\":\"M\",\"pid\":" << track.pid << ",\"tid\":" << track.tid << ","
         << "\"name\":\"thread_name\",\"args\":{\"name\":\"" << frontend::JsonEscape(track.thread_name) << "\"}}";
  return stream.str();
}

std::string MakeThreadSortEvent(const TrackRef& track) {
  std::ostringstream stream;
  stream << "{\"ph\":\"M\",\"pid\":" << track.pid << ",\"tid\":" << track.tid << ","
         << "\"name\":\"thread_sort_index\",\"args\":{\"sort_index\":" << track.sort_index << "}}";
  return stream.str();
}

std::string MakeCounterEvent(
    std::uint64_t cycle,
    std::size_t occupancy,
    std::size_t capacity) {
  std::ostringstream stream;
  stream << "{\"name\":\"ROB occupancy\",\"cat\":\"rob\",\"ph\":\"C\","
         << "\"pid\":" << kRobPid << ",\"tid\":" << kRobTid << ",\"ts\":" << cycle << ","
         << "\"args\":{\"occupancy\":" << occupancy << ",\"capacity\":" << capacity << "}}";
  return stream.str();
}

std::string MakeInstantEvent(
    std::uint64_t cycle,
    const TrackRef& track,
    const std::string& name,
    const std::string& args_json) {
  std::ostringstream stream;
  stream << "{\"name\":\"" << frontend::JsonEscape(name) << "\",\"ph\":\"i\",\"s\":\"t\","
         << "\"pid\":" << track.pid << ",\"tid\":" << track.tid << ",\"ts\":" << cycle
         << ",\"args\":" << args_json << "}";
  return stream.str();
}

std::string MakeSliceEvent(const PTOInst& inst, const TrackRef& track) {
  std::ostringstream args;
  args << "{\"opcode\":\"" << frontend::JsonEscape(OpcodeName(inst)) << "\","
       << "\"engine_kind\":\"" << frontend::JsonEscape(std::string(ToString(inst.engine_kind))) << "\","
       << "\"engine_id\":" << inst.engine_id << ","
       << "\"block_idx\":" << inst.block_idx << ","
       << "\"sequence_id\":" << inst.sequence_id << ","
       << "\"rob_id\":" << inst.rob_id << ","
       << "\"start_cycle\":" << inst.timestamps.engine_pop_cycle << ","
       << "\"end_cycle\":" << inst.timestamps.engine_complete_cycle << ","
       << "\"duration_cycles\":" << DurationFromCycles(inst.timestamps.engine_pop_cycle, inst.timestamps.engine_complete_cycle)
       << "}";

  std::ostringstream stream;
  stream << "{\"name\":\"" << frontend::JsonEscape(MakeEngineSliceName(inst)) << "\","
         << "\"cat\":\"engine\",\"ph\":\"X\","
         << "\"pid\":" << track.pid << ",\"tid\":" << track.tid << ","
         << "\"ts\":" << inst.timestamps.engine_pop_cycle << ","
         << "\"dur\":" << DurationFromCycles(inst.timestamps.engine_pop_cycle, inst.timestamps.engine_complete_cycle) << ","
         << "\"args\":" << args.str() << "}";
  return stream.str();
}

std::string MakeFlowEvent(
    const char phase,
    std::uint64_t flow_id,
    std::uint64_t cycle,
    const TrackRef& track,
    std::uint64_t producer_sequence,
    std::uint64_t consumer_sequence,
    std::uint64_t tile_tag,
    bool bind_to_enclosing) {
  std::ostringstream args;
  args << "{\"producer_sequence_id\":" << producer_sequence << ","
       << "\"consumer_sequence_id\":" << consumer_sequence << ","
       << "\"tile_tag\":" << tile_tag << "}";

  std::ostringstream stream;
  stream << "{\"name\":\"wake tag=" << tile_tag << "\",\"cat\":\"dependency\",\"ph\":\"" << phase << "\","
         << "\"pid\":" << track.pid << ",\"tid\":" << track.tid << ",\"ts\":" << cycle << ","
         << "\"id\":" << flow_id;
  if (bind_to_enclosing) {
    stream << ",\"bp\":\"e\"";
  }
  stream << ",\"args\":" << args.str() << "}";
  return stream.str();
}

std::uint64_t BindInsideSlice(std::uint64_t start_cycle, std::uint64_t end_cycle) {
  if (start_cycle == kUnsetCycleStamp || end_cycle == kUnsetCycleStamp || end_cycle <= start_cycle) {
    return start_cycle;
  }
  return end_cycle - 1;
}

void AppendTrackMetadata(
    std::vector<std::string>& events,
    const TrackRef& track,
    std::set<int>& emitted_processes,
    std::set<std::pair<int, int>>& emitted_threads) {
  if (emitted_processes.insert(track.pid).second) {
    events.push_back(MakeProcessNameEvent(track.pid, track.process_name));
    events.push_back(MakeProcessSortEvent(track.pid, track.sort_index / 100));
  }
  if (emitted_threads.insert({track.pid, track.tid}).second) {
    events.push_back(MakeThreadNameEvent(track));
    events.push_back(MakeThreadSortEvent(track));
  }
}

void AppendEngineTrackMetadata(
    std::vector<std::string>& events,
    std::uint64_t block_idx,
    PTOEngineKind kind,
    std::size_t engine_count,
    std::set<int>& emitted_processes,
    std::set<std::pair<int, int>>& emitted_threads) {
  for (std::size_t engine_index = 0; engine_index < engine_count; ++engine_index) {
    AppendTrackMetadata(
        events,
        MakeEngineTrack(kind, engine_index, block_idx),
        emitted_processes,
        emitted_threads);
  }
}

}  // namespace

void WritePerfettoTrace(const std::filesystem::path& path, const SimulationResult& result) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream stream(path, std::ios::out | std::ios::trunc);
  if (!stream) {
    throw std::runtime_error("failed to open Perfetto trace output: " + path.string());
  }

  std::vector<std::string> events;
  events.reserve(result.processed.size() * 6 + 32);

  std::set<int> emitted_processes;
  std::set<std::pair<int, int>> emitted_threads;
  const TrackRef rob_track{
      .pid = kRobPid,
      .tid = kRobTid,
      .process_name = "ROB",
      .thread_name = "ROB occupancy / cap=" + std::to_string(result.rob_capacity),
      .sort_index = 0,
  };
  AppendTrackMetadata(events, rob_track, emitted_processes, emitted_threads);
  std::set<std::uint64_t> block_indices;
  for (const PTOInst& inst : result.processed) {
    block_indices.insert(inst.block_idx);
  }
  for (const std::uint64_t block_idx : block_indices) {
    AppendEngineTrackMetadata(events, block_idx, PTOEngineKind::Scalar, result.scalar_engine_count, emitted_processes, emitted_threads);
    AppendEngineTrackMetadata(events, block_idx, PTOEngineKind::Vec, result.vec_engine_count, emitted_processes, emitted_threads);
    AppendEngineTrackMetadata(events, block_idx, PTOEngineKind::Cube, result.cube_engine_count, emitted_processes, emitted_threads);
    AppendEngineTrackMetadata(events, block_idx, PTOEngineKind::Tma, result.tma_engine_count, emitted_processes, emitted_threads);
  }

  std::vector<RobDelta> rob_deltas;
  rob_deltas.reserve(result.processed.size() * 2);
  for (const PTOInst& inst : result.processed) {
    if (inst.timestamps.rob_alloc_cycle != kUnsetCycleStamp) {
      rob_deltas.push_back(RobDelta{
          .cycle = inst.timestamps.rob_alloc_cycle,
          .order = 1,
          .delta = 1,
      });
    }
    if (inst.timestamps.rob_retire_cycle != kUnsetCycleStamp) {
      rob_deltas.push_back(RobDelta{
          .cycle = inst.timestamps.rob_retire_cycle,
          .order = 0,
          .delta = -1,
      });
    }
  }
  std::sort(rob_deltas.begin(), rob_deltas.end(), [](const RobDelta& lhs, const RobDelta& rhs) {
    return std::tie(lhs.cycle, lhs.order, lhs.delta) < std::tie(rhs.cycle, rhs.order, rhs.delta);
  });
  std::size_t occupancy = 0;
  events.push_back(MakeCounterEvent(0, occupancy, result.rob_capacity));
  for (const RobDelta& delta : rob_deltas) {
    occupancy = static_cast<std::size_t>(static_cast<std::int64_t>(occupancy) + delta.delta);
    events.push_back(MakeCounterEvent(delta.cycle, occupancy, result.rob_capacity));
  }
  events.push_back(MakeInstantEvent(
      0,
      rob_track,
      "ROB summary",
      "{\"capacity\":" + std::to_string(result.rob_capacity) +
          ",\"retired\":" + std::to_string(result.processed.size()) +
          ",\"simulated_cycles\":" + std::to_string(result.simulated_cycles) + "}"));

  for (const PTOInst& inst : result.processed) {
    if (inst.engine_id == std::numeric_limits<std::uint64_t>::max() ||
        inst.timestamps.engine_pop_cycle == kUnsetCycleStamp ||
        inst.timestamps.engine_complete_cycle == kUnsetCycleStamp) {
      continue;
    }
    const TrackRef track = MakeEngineTrack(inst.engine_kind, inst.engine_id, inst.block_idx);
    events.push_back(MakeSliceEvent(inst, track));
  }

  std::map<std::pair<std::uint64_t, std::uint64_t>, const PTOInst*> tag_producers;
  std::uint64_t next_flow_id = 1;
  for (const PTOInst& inst : result.processed) {
    std::set<std::tuple<std::uint64_t, std::uint64_t, std::uint64_t>> seen_edges;
    for (const auto& input : inst.tile_reg_inputs) {
      if (!input.rename_managed || input.tile_tag == kInvalidTileTag) {
        continue;
      }
      const auto producer_it = tag_producers.find({inst.block_idx, input.tile_tag});
      if (producer_it == tag_producers.end()) {
        continue;
      }
      const PTOInst& producer = *producer_it->second;
      if (producer.engine_id == std::numeric_limits<std::uint64_t>::max() ||
          producer.timestamps.engine_complete_cycle == kUnsetCycleStamp ||
          inst.timestamps.issue_cycle == kUnsetCycleStamp) {
        continue;
      }
      const auto edge_key = std::make_tuple(producer.sequence_id, inst.sequence_id, input.tile_tag);
      if (!seen_edges.insert(edge_key).second) {
        continue;
      }
      const TrackRef producer_track = MakeEngineTrack(producer.engine_kind, producer.engine_id, producer.block_idx);
      const TrackRef consumer_track = MakeEngineTrack(inst.engine_kind, inst.engine_id, inst.block_idx);
      events.push_back(MakeFlowEvent(
          's',
          next_flow_id,
          BindInsideSlice(producer.timestamps.engine_pop_cycle, producer.timestamps.engine_complete_cycle),
          producer_track,
          producer.sequence_id,
          inst.sequence_id,
          input.tile_tag,
          false));
      events.push_back(MakeFlowEvent(
          'f',
          next_flow_id,
          inst.timestamps.issue_cycle,
          consumer_track,
          producer.sequence_id,
          inst.sequence_id,
          input.tile_tag,
          true));
      ++next_flow_id;
    }
    for (const auto& output : inst.tile_reg_outputs) {
      if (!output.rename_managed || output.tile_tag == kInvalidTileTag) {
        continue;
      }
      tag_producers[{inst.block_idx, output.tile_tag}] = &inst;
    }
  }

  stream << "{\n"
         << "  \"displayTimeUnit\": \"ns\",\n"
         << "  \"otherData\": {\n"
         << "    \"tool\": \"gfsim\",\n"
         << "    \"record_count\": " << result.record_count << ",\n"
         << "    \"simulated_cycles\": " << result.simulated_cycles << ",\n"
         << "    \"rob_capacity\": " << result.rob_capacity << "\n"
         << "  },\n"
         << "  \"traceEvents\": [\n";
  for (std::size_t index = 0; index < events.size(); ++index) {
    stream << "    " << events[index];
    if (index + 1 != events.size()) {
      stream << ",";
    }
    stream << "\n";
  }
  stream << "  ]\n"
         << "}\n";
}

}  // namespace davincioo::model_top
