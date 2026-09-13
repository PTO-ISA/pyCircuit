#ifndef GENERATED_SOURCE
#error "GENERATED_SOURCE must name the generated PYC C++ source"
#endif
#include GENERATED_SOURCE

#include <iostream>

namespace {

using Model = pyc::gen::table_runtime_row;

void settleLow(Model &dut) {
  dut.clk = pyc::cpp::Wire<1>(0);
  dut.step();
}

void tick(Model &dut) {
  settleLow(dut);
  dut.clk = pyc::cpp::Wire<1>(1);
  dut.step();
  settleLow(dut);
}

bool request(Model &dut, uint64_t row, uint64_t tag, uint64_t expected) {
  dut.in_valid = pyc::cpp::Wire<1>(1);
  dut.in_data = pyc::cpp::Wire<10>((row << 8) | tag);
  do {
    tick(dut);
  } while (dut.in_ready.value() == 0);
  dut.in_valid = pyc::cpp::Wire<1>(0);
  do {
    tick(dut);
  } while (dut.out_valid.value() == 0);
  return dut.out_data.value() == expected;
}

} // namespace

int main() {
  Model dut;
  dut.out_ready = pyc::cpp::Wire<1>(1);
  dut.rst = pyc::cpp::Wire<1>(1);
  tick(dut);
  tick(dut);
  dut.rst = pyc::cpp::Wire<1>(0);

  if (!request(dut, 2, 32, (8 << 1) | 1) ||
      !request(dut, 1, 21, (5 << 1) | 1) ||
      !request(dut, 2, 32, (9 << 1) | 1) || !request(dut, 3, 99, 0))
    return 1;
  std::cout << "PASS pyc-cpp table runtime row capture and update\n";
  return 0;
}
