// Performance harness for the public FastFwd feature example.
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

#include <cpp/pyc_tb.hpp>

#include "fastfwd.hpp"

using pyc::cpp::Testbench;
using pyc::cpp::Wire;

namespace {

struct Args {
  std::uint64_t seed = 1;
  std::uint64_t cycles = 20000;
  std::uint64_t packets = 60000;
};

Args parseArgs(int argc, char **argv) {
  Args a;
  for (int i = 1; i < argc; i++) {
    const std::string k = argv[i];
    if (k == "--seed" && i + 1 < argc) {
      a.seed = static_cast<std::uint64_t>(std::strtoull(argv[++i], nullptr, 10));
      continue;
    }
    if (k == "--cycles" && i + 1 < argc) {
      a.cycles = static_cast<std::uint64_t>(std::strtoull(argv[++i], nullptr, 10));
      continue;
    }
    if (k == "--packets" && i + 1 < argc) {
      a.packets = static_cast<std::uint64_t>(std::strtoull(argv[++i], nullptr, 10));
      continue;
    }
  }
  return a;
}

} // namespace

int main(int argc, char **argv) {
  const Args args = parseArgs(argc, argv);

  pyc::gen::fastfwd dut{};
  Testbench<pyc::gen::fastfwd> tb(dut);

  const std::array<Wire<1> *, 4> in_vld = {&dut.lane0_pkt_in_vld, &dut.lane1_pkt_in_vld, &dut.lane2_pkt_in_vld,
                                           &dut.lane3_pkt_in_vld};
  const std::array<Wire<128> *, 4> in_data = {&dut.lane0_pkt_in_data, &dut.lane1_pkt_in_data, &dut.lane2_pkt_in_data,
                                              &dut.lane3_pkt_in_data};
  const std::array<Wire<5> *, 4> in_ctrl = {&dut.lane0_pkt_in_ctrl, &dut.lane1_pkt_in_ctrl, &dut.lane2_pkt_in_ctrl,
                                            &dut.lane3_pkt_in_ctrl};

  const std::array<Wire<1> *, 4> out_vld = {&dut.lane0_pkt_out_vld, &dut.lane1_pkt_out_vld, &dut.lane2_pkt_out_vld,
                                            &dut.lane3_pkt_out_vld};
  const std::array<Wire<128> *, 4> out_data = {&dut.lane0_pkt_out_data, &dut.lane1_pkt_out_data, &dut.lane2_pkt_out_data,
                                               &dut.lane3_pkt_out_data};

  std::mt19937_64 rng(args.seed);

  std::uint64_t sent = 0;
  std::uint64_t got = 0;
  std::uint64_t bkpr_cycles = 0;

  for (std::uint64_t cyc = 0; cyc < args.cycles; cyc++) {
    std::array<bool, 4> expect_vld{};
    std::array<Wire<128>, 4> expect_data{};

    for (int lane = 0; lane < 4; lane++) {
      const bool can_send = sent < args.packets;
      const bool push = can_send && ((rng() & 3u) != 0u);

      const std::uint64_t lo = rng();
      const std::uint64_t hi = rng();
      const Wire<128> pkt({lo, hi});

      *in_vld[lane] = Wire<1>(push ? 1u : 0u);
      *in_data[lane] = pkt;
      *in_ctrl[lane] = Wire<5>(static_cast<std::uint64_t>(rng() & 0x1fu));

      expect_vld[lane] = push;
      expect_data[lane] = pkt;
      if (push)
        sent++;
    }

    tb.runSteps(1);

    if (dut.pkt_in_bkpr.toBool())
      bkpr_cycles++;

    for (int lane = 0; lane < 4; lane++) {
      const bool ov = out_vld[lane]->toBool();
      if (ov != expect_vld[lane]) {
        std::cerr << "status=FAIL reason=lane_vld_mismatch lane=" << lane << " cycle=" << cyc << "\n";
        return 1;
      }
      if (ov && (*out_data[lane] != expect_data[lane])) {
        std::cerr << "status=FAIL reason=lane_data_mismatch lane=" << lane << " cycle=" << cyc << "\n";
        return 1;
      }
      if (ov)
        got++;
    }
  }

  const double throughput = args.cycles ? static_cast<double>(got) / static_cast<double>(args.cycles) : 0.0;
  const double bkpr = args.cycles ? (100.0 * static_cast<double>(bkpr_cycles) / static_cast<double>(args.cycles)) : 0.0;

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "status=PASS cycles=" << args.cycles << " sent=" << sent << " got=" << got << " throughput=" << throughput
            << " bkpr=" << bkpr << "%\n";
  return 0;
}
