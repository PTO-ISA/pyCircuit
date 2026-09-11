#include "lane_bundle.hpp"

#include <cstdint>
#include <iostream>

namespace {

using Model = pyc::gen::lane_bundle;

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

bool expectPair(const Model &dut, std::uint64_t first, std::uint64_t second) {
  return dut.out_valid_0.value() != 0 && dut.out_valid_1.value() != 0 &&
         dut.out_valid_2.value() == 0 && dut.out_data_0.value() == first &&
         dut.out_data_1.value() == second;
}

} // namespace

int main() {
  Model dut;
  dut.out_ready = pyc::cpp::Wire<1>(0);
  dut.rst = pyc::cpp::Wire<1>(1);
  tick(dut);
  tick(dut);
  dut.rst = pyc::cpp::Wire<1>(0);
  settleLow(dut);
  if (dut.in_ready.value() == 0)
    return 1;

  dut.in_valid_0 = pyc::cpp::Wire<1>(1);
  dut.in_data_0 = pyc::cpp::Wire<8>(10);
  dut.in_valid_1 = pyc::cpp::Wire<1>(1);
  dut.in_data_1 = pyc::cpp::Wire<8>(11);
  tick(dut);
  if (!expectPair(dut, 10, 11))
    return 2;

  dut.in_data_0 = pyc::cpp::Wire<8>(20);
  dut.in_data_1 = pyc::cpp::Wire<8>(21);
  if (dut.in_ready.value() == 0)
    return 3;
  tick(dut);
  if (!expectPair(dut, 10, 11))
    return 4;

  dut.in_data_0 = pyc::cpp::Wire<8>(30);
  dut.in_data_1 = pyc::cpp::Wire<8>(31);
  if (dut.in_ready.value() != 0)
    return 5;
  tick(dut);
  if (!expectPair(dut, 10, 11))
    return 6;

  dut.out_ready = pyc::cpp::Wire<1>(1);
  settleLow(dut);
  if (dut.in_ready.value() == 0)
    return 7;
  tick(dut);
  if (!expectPair(dut, 20, 21))
    return 8;

  dut.in_valid_1 = pyc::cpp::Wire<1>(0);
  dut.in_data_0 = pyc::cpp::Wire<8>(40);
  tick(dut);
  if (!expectPair(dut, 30, 31))
    return 9;

  dut.in_valid_0 = pyc::cpp::Wire<1>(0);
  tick(dut);
  if (dut.out_valid_0.value() == 0 || dut.out_valid_1.value() != 0 ||
      dut.out_valid_2.value() != 0 || dut.out_data_0.value() != 40)
    return 10;

  dut.rst = pyc::cpp::Wire<1>(1);
  tick(dut);
  if (dut.out_valid_0.value() != 0 || dut.out_valid_1.value() != 0 ||
      dut.out_valid_2.value() != 0)
    return 11;

  std::cout << "PASS pyc-cpp lane_bundle behavior\n";
  return 0;
}
