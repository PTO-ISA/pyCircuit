#include "pycircuit_system.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
constexpr unsigned kNodes = 40, kBankBytes = 65536, kAlignment = 8;
constexpr unsigned kBeat = 64, kInstruction = 64;
using Beat = std::array<std::uint8_t, kBeat>;

static void require(bool condition, const std::string &message) {
  if (!condition) throw std::runtime_error(message);
}
template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
static std::vector<std::uint8_t> load(const fs::path &path) {
  std::ifstream stream(path, std::ios::binary);
  require(bool(stream), "cannot open " + path.string());
  return {(std::istreambuf_iterator<char>(stream)), {}};
}
static std::uint64_t little(const std::vector<std::uint8_t> &data,
                            std::size_t offset, unsigned bytes) {
  require(offset + bytes <= data.size(), "truncated input record");
  std::uint64_t value = 0;
  for (unsigned i = 0; i < bytes; ++i)
    value |= std::uint64_t(data[offset + i]) << (8 * i);
  return value;
}
struct Region { unsigned node, address, bytes; };
static Region region(const std::vector<std::uint8_t> &data, std::size_t offset) {
  Region result{unsigned(little(data, offset, 4)), unsigned(little(data, offset + 4, 4)),
                unsigned(little(data, offset + 8, 4))};
  require(result.node < kNodes && result.address % kAlignment == 0 &&
          result.bytes > 0 && result.bytes % kAlignment == 0 &&
          std::uint64_t(result.address) + result.bytes <= kBankBytes,
          "invalid host node-local memory range");
  return result;
}
static unsigned transfer_bytes(unsigned address, unsigned remaining) {
  return address % kBeat == 0 && remaining >= kBeat ? kBeat : kAlignment;
}
struct Request {
  unsigned kind = 0, node = 0, address = 0, count = 0;
  bool wide = false;
  Beat data{};
  std::uint64_t permits = (std::uint64_t(1) << kNodes) - 1;
};
struct Snapshot {
  Beat data{};
  std::uint64_t reads = 0, writes = 0;
  unsigned pc = 0, stage = 0, node = 0, offset = 0, counted = 0;
  unsigned pending = 0, epoch = 0, fault_code = 0;
  bool active = false, complete = false, fault = false, done = false;
};
struct Simulation {
  pyc_dut dut{1};
  pyc_dut::Inputs inputs{};
  std::uint64_t cycles = 0, steps = 0;

  static auto pack(const Request &request) {
    gfsim::Bits<610> bits{};
    auto put = [&](unsigned offset, unsigned width, std::uint64_t value) {
      for (unsigned bit = 0; bit < width; ++bit)
        if ((value >> bit) & 1) bits = gfsim::detail::setBit(bits, offset + bit);
    };
    put(0, 40, request.permits); put(40, 16, request.count);
    for (unsigned byte = 0; byte < (request.wide ? kBeat : kAlignment); ++byte)
      put(56 + 8 * byte, 8, request.data[byte]);
    put(568, 1, request.wide); put(569, 32, request.address);
    put(601, 6, request.node); put(607, 3, request.kind);
    return gfsim::FourState<610>::known(bits);
  }
  void drive(const Request &request, unsigned clock) {
    inputs.host = decltype(inputs.host)::fromPacked(pack(request));
    inputs.pyc_7079635f636c6b = known<1>(clock);
    inputs.pyc_7079635f727374 = known<1>(0);
    dut.drive(inputs);
  }
  Simulation() {
    drive({}, 0); dut.system().Build(); dut.system().Reset();
    require(dut.system().state() == gfsim::SimSystemState::Ready,
            "machine initialization failed");
  }
  void step() {
    ++steps;
    if (dut.system().Step() != gfsim::SimStepResult::Running) {
      auto error = dut.system().failureInfo();
      throw std::runtime_error("checked machine step failed: " + std::string(error.code) +
                               " " + std::string(error.message));
    }
  }
  Snapshot sample() {
    auto bits = dut.sample().result.packed();
    auto get = [&](unsigned offset, unsigned width) {
      std::uint64_t value = 0;
      for (unsigned bit = 0; bit < width; ++bit) {
        if (!bits.knownMask().bit(offset + bit) || bits.zMask().bit(offset + bit))
          throw std::runtime_error("unknown machine observation");
        value |= std::uint64_t(bits.value().bit(offset + bit)) << bit;
      }
      return value;
    };
    Snapshot out;
    out.writes = get(0, 64); out.reads = get(64, 64); out.done = get(128, 1);
    out.fault_code = get(129, 8); out.fault = get(137, 1); out.epoch = get(138, 8);
    out.pending = get(146, 8); out.complete = get(154, 1); out.active = get(155, 1);
    out.counted = get(156, 32); out.offset = get(188, 32); out.node = get(220, 6);
    out.stage = get(226, 5); out.pc = get(231, 16);
    for (unsigned byte = 0; byte < kBeat; ++byte)
      out.data[byte] = get(247 + 8 * byte, 8);
    return out;
  }
  Snapshot cycle(const Request &request) {
    drive(request, 0); step();
    drive(request, 1); step();
    ++cycles;
    return sample();
  }
};

static void write_bytes(const fs::path &path, const std::vector<std::uint8_t> &bytes) {
  require(!fs::exists(path), "refusing to replace output " + path.string());
  fs::path temporary = path; temporary += ".tmp";
  require(!fs::exists(temporary), "temporary output already exists");
  { std::ofstream stream(temporary, std::ios::binary);
    require(bool(stream), "cannot create output");
    stream.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    require(bool(stream), "cannot write output"); }
  fs::rename(temporary, path);
}

int main(int argc, char **argv) {
  const auto driver_started = Clock::now();
  fs::path receipt;
  bool may_write_receipt = false;
  try {
    require(argc >= 6,
            "usage: wse_runner PROGRAM.bin IMAGE.bin READBACK.bin OUTPUT.bin RECEIPT.json "
            "[--stall-pc N --stall-node N --stall-offset N --stall-cycles N]");
    auto program = load(argv[1]), image = load(argv[2]), ranges = load(argv[3]);
    fs::path output = argv[4]; receipt = argv[5];
    require(!fs::exists(output) && !fs::exists(receipt), "output/receipt already exists");
    may_write_receipt = true;
    require(!program.empty() && program.size() % kInstruction == 0 && program.size() <= 65536,
            "program must contain 1..1024 whole v1 instructions");
    require(ranges.size() % 12 == 0, "truncated readback record");
    int stall_pc = -1, stall_node = -1, stall_cycles = 0, stall_offset = 8;
    for (int arg = 6; arg < argc; arg += 2) {
      require(arg + 1 < argc, "missing option value");
      std::string key = argv[arg]; int value = std::stoi(argv[arg + 1]);
      if (key == "--stall-pc") stall_pc = value;
      else if (key == "--stall-node") stall_node = value;
      else if (key == "--stall-offset") stall_offset = value;
      else if (key == "--stall-cycles") stall_cycles = value;
      else throw std::runtime_error("unknown option " + key);
    }
    if (stall_pc >= 0)
      require(unsigned(stall_pc) < program.size() / kInstruction && stall_node >= 0 &&
              stall_node < int(kNodes) && stall_cycles > 0 && stall_offset >= 0,
              "invalid stall configuration");
    struct Phase {
      std::string name;
      double seconds;
      std::uint64_t cycles, steps;
    };
    std::vector<Phase> phases;
    auto phase_started = Clock::now();
    auto next_progress = phase_started + std::chrono::seconds(30);
    std::uint64_t phase_cycles = 0, phase_steps = 0;
    Simulation simulation;
    auto finish_phase = [&](const char *name) {
      const auto finished = Clock::now();
      phases.push_back({name, std::chrono::duration<double>(finished - phase_started).count(),
                        simulation.cycles - phase_cycles, simulation.steps - phase_steps});
      const auto &phase = phases.back();
      std::cerr << "PHASE complete " << phase.name << " seconds=" << phase.seconds
                << " host_cycles=" << phase.cycles << " steps=" << phase.steps << '\n';
      phase_cycles = simulation.cycles;
      phase_steps = simulation.steps;
      phase_started = Clock::now();
    };
    auto progress = [&](const char *name, const Snapshot &state) {
      const auto now = Clock::now();
      if (now < next_progress) return;
      std::cerr << "PROGRESS phase=" << name
                << " seconds=" << std::chrono::duration<double>(now - phase_started).count()
                << " host_cycles=" << simulation.cycles - phase_cycles << " pc=" << state.pc
                << " stage=" << state.stage << " offset=" << state.offset << '\n';
      next_progress = now + std::chrono::seconds(30);
    };
    finish_phase("build_reset");
    for (std::size_t offset = 0; offset < image.size();) {
      Region patch = region(image, offset); offset += 12;
      require(offset + patch.bytes <= image.size(), "truncated image payload");
      for (unsigned beat = 0; beat < patch.bytes;) {
        Request request; request.kind = 1; request.node = patch.node;
        request.address = patch.address + beat;
        const unsigned size = transfer_bytes(request.address, patch.bytes - beat);
        request.wide = size == kBeat;
        std::copy_n(image.begin() + offset + beat, size, request.data.begin());
        const auto observed = simulation.cycle(request);
        require(!observed.fault, "machine rejected image load");
        progress("image_load", observed);
        beat += size;
      }
      offset += patch.bytes;
    }
    finish_phase("image_load");
    for (unsigned offset = 0; offset < program.size(); offset += kBeat) {
      Request request; request.kind = 2; request.address = offset; request.wide = true;
      std::copy_n(program.begin() + offset, kBeat, request.data.begin());
      const auto observed = simulation.cycle(request);
      require(!observed.fault, "machine rejected program load");
      progress("program_load", observed);
    }
    finish_phase("program_load");
    Request start; start.kind = 3; start.count = program.size() / kInstruction;
    auto state = simulation.cycle(start);
    auto start_cycles = simulation.cycles;
    finish_phase("start");
    unsigned stalled = 0; bool stall_started = false;
    std::vector<Snapshot> stall_samples;
    for (unsigned bound = 0; bound < 10000000 && !state.done && !state.fault; ++bound) {
      Request request;
      if (!stall_started && int(state.pc) == stall_pc && state.stage == 5 &&
          int(state.offset) == stall_offset) stall_started = true;
      if (stall_started && stalled < unsigned(stall_cycles)) {
        request.permits &= ~(std::uint64_t(1) << stall_node);
        stall_samples.push_back(state); ++stalled;
      }
      state = simulation.cycle(request);
      progress("execution", state);
    }
    require(state.done || state.fault, "machine exceeded explicit execution bound");
    require(stall_pc < 0 || stalled == unsigned(stall_cycles), "requested stall was not reached");
    auto execution = state;
    auto execution_cycles = simulation.cycles - start_cycles;
    finish_phase("execution");
    std::vector<std::uint8_t> bytes;
    std::vector<Snapshot> accounts;
    std::vector<Region> requested;
    for (std::size_t offset = 0; offset < ranges.size(); offset += 12) {
      Region range = region(ranges, offset); requested.push_back(range);
      for (unsigned beat = 0; beat < range.bytes;) {
        Request request; request.kind = 4; request.node = range.node;
        request.address = range.address + beat;
        const unsigned size = transfer_bytes(request.address, range.bytes - beat);
        request.wide = size == kBeat;
        auto observed = simulation.cycle(request);
        bytes.insert(bytes.end(), observed.data.begin(), observed.data.begin() + size);
        progress("readback_and_accounts", observed);
        beat += size;
      }
      Request observe; observe.kind = 5; observe.node = range.node;
      accounts.push_back(simulation.cycle(observe));
      progress("readback_and_accounts", accounts.back());
    }
    finish_phase("readback_and_accounts");
    fs::path written = output;
    if (execution.fault) written += ".fault.bin";
    write_bytes(written, bytes);
    std::ofstream report(receipt); require(bool(report), "cannot create receipt");
    report << "{\n  \"status\":\"" << (execution.fault ? "FAULT" : "EXECUTED")
           << "\",\n  \"abi_version\":1,\n  \"workers\":1,\n  \"backend\":\"cpp\",\n"
           << "  \"host_beat_bytes\":" << kBeat
           << ",\n  \"data_beat_bytes\":" << kBeat << ",\n"
           << "  \"instructions\":" << program.size() / kInstruction
           << ",\n  \"execution_cycles\":" << execution_cycles
           << ",\n  \"pc\":" << execution.pc << ",\n  \"stage\":" << execution.stage
           << ",\n  \"fault_code\":" << execution.fault_code
           << ",\n  \"source_read_bytes\":" << execution.reads
           << ",\n  \"destination_write_bytes\":" << execution.writes
           << ",\n  \"output_bytes\":" << bytes.size()
           << ",\n  \"host_cycles\":" << simulation.cycles
           << ",\n  \"steps\":" << simulation.steps
           << ",\n  \"timing\":{\"clock\":\"steady_clock\",\"driver_seconds_before_receipt\":"
           << std::chrono::duration<double>(Clock::now() - driver_started).count()
           << ",\"phases\":[";
    for (unsigned i = 0; i < phases.size(); ++i) {
      if (i) report << ',';
      const auto &phase = phases[i];
      report << "{\"name\":\"" << phase.name << "\",\"seconds\":" << phase.seconds
             << ",\"host_cycles\":" << phase.cycles << ",\"steps\":" << phase.steps << '}';
    }
    report << "]}"
           << ",\n  \"stall_cycles\":" << stalled << ",\n  \"accounts\":[";
    for (unsigned i = 0; i < accounts.size(); ++i) {
      if (i) report << ',';
      const auto &a = accounts[i];
      report << "{\"node\":" << requested[i].node << ",\"counted\":" << a.counted
             << ",\"active\":" << (a.active ? "true" : "false")
             << ",\"complete\":" << (a.complete ? "true" : "false")
             << ",\"epoch\":" << a.epoch << ",\"pending\":" << a.pending << '}';
    }
    report << "],\n  \"stall_observations\":[";
    for (unsigned i = 0; i < stall_samples.size(); ++i) {
      if (i) report << ',';
      const auto &a = stall_samples[i];
      report << "{\"pc\":" << a.pc << ",\"offset\":" << a.offset
             << ",\"stage\":" << a.stage << ",\"read_bytes\":" << a.reads
             << ",\"write_bytes\":" << a.writes << '}';
    }
    report << "]\n}\n"; require(bool(report), "cannot write receipt");
    std::cout << (execution.fault ? "FAULT" : "EXECUTED") << " WSE runtime program, "
              << execution_cycles << " functional cycles\n";
    return execution.fault ? 2 : 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    if (may_write_receipt && !receipt.empty() && !fs::exists(receipt)) {
      std::ofstream report(receipt);
      report << "{\"status\":\"FAILED\",\"backend_failure\":true}\n";
    }
    return 1;
  }
}
