#include <cstdint>
#include <iostream>

#include <cpp/pyc_tb.hpp>

#include "linx_cpu_pyc.hpp"

using pyc::cpp::Testbench;
using pyc::cpp::Wire;

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  pyc::gen::linx_cpu_pyc dut{};
  dut.boot_pc = Wire<64>(0x10000ull);
  dut.boot_sp = Wire<64>(0x0fff00ull);
  dut.irq = Wire<1>(0);
  dut.irq_vector = Wire<64>(0);
  dut.host_wvalid = Wire<1>(0);
  dut.host_waddr = Wire<64>(0);
  dut.host_wdata = Wire<64>(0);
  dut.host_wstrb = Wire<8>(0);

  Testbench<pyc::gen::linx_cpu_pyc> tb(dut);
  tb.addClock(dut.clk, 1);
  tb.reset(dut.rst, 2, 1);

  for (std::uint64_t i = 0; i < 256 && !dut.halted.toBool(); i++) {
    tb.runCyclesAuto(1);
  }

  if (!dut.icall_contract_valid.toBool()) {
    std::cerr << "error: production ICALL contract path did not retire\n";
    return 1;
  }
  if (dut.icall_contract_target.value() != 0x8800ull) {
    std::cerr << "error: production ICALL path lost retiring SETC.TGT\n";
    return 1;
  }
  if (dut.icall_contract_ra.value() != 0x4002ull) {
    std::cerr << "error: production ICALL path computed wrong independent RA\n";
    return 1;
  }
  if (!dut.icall_contract_raw_3000_invalid.toBool()) {
    std::cerr << "error: raw 0x3000 revived retired compressed ICALL\n";
    return 1;
  }

  std::cout << "ok: linx_cpu_pyc smoke cycles=" << dut.cycles.value() << "\n";
  return 0;
}
