#include "lane_transform.hpp"

#include <cstdint>
#include <iostream>

namespace {

using Model = pyc::gen::lane_transform;

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

} // namespace

int main() {
  Model dut;
  dut.out_ready = pyc::cpp::Wire<1>(1);
  dut.rst = pyc::cpp::Wire<1>(1);
  tick(dut);
  tick(dut);
  dut.rst = pyc::cpp::Wire<1>(0);

  dut.in_valid_0 = pyc::cpp::Wire<1>(1);
  dut.in_data_0 = pyc::cpp::Wire<8>(10);
  dut.in_valid_1 = pyc::cpp::Wire<1>(1);
  dut.in_data_1 = pyc::cpp::Wire<8>(11);
  tick(dut);

  dut.in_valid_1 = pyc::cpp::Wire<1>(0);
  dut.in_data_0 = pyc::cpp::Wire<8>(20);
  tick(dut);
  if (dut.out_valid_0.value() == 0 || dut.out_valid_1.value() == 0 ||
      dut.out_valid_2.value() != 0 || dut.out_data_0.value() != 11 ||
      dut.out_data_1.value() != 12)
    return 1;

  dut.in_valid_0 = pyc::cpp::Wire<1>(0);
  tick(dut);
  if (dut.out_valid_0.value() == 0 || dut.out_valid_1.value() != 0 ||
      dut.out_valid_2.value() != 0 || dut.out_data_0.value() != 21)
    return 2;

  dut.rst = pyc::cpp::Wire<1>(1);
  tick(dut);
  if (dut.out_valid_0.value() != 0 || dut.out_valid_1.value() != 0 ||
      dut.out_valid_2.value() != 0)
    return 3;

  std::cout << "PASS pyc-cpp lane_transform behavior\n";
  return 0;
}
