#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {

using Model = pyc::gen::versioned_recovery;

void clock(Model &model) {
  model.clk = pyc::cpp::Wire<1>{0};
  model.step();
  model.clk = pyc::cpp::Wire<1>{1};
  model.step();
}

template <typename Ready, typename Valid, typename Data>
void send(Model &model, Ready &&ready, Valid &&valid, Data &&data,
          std::uint64_t value) {
  using DataType = std::remove_reference_t<Data>;
  using ValidType = std::remove_reference_t<Valid>;
  data = DataType{value};
  valid = ValidType{1};
  for (unsigned cycle = 0; cycle < 32; ++cycle) {
    model.clk = pyc::cpp::Wire<1>{0};
    model.eval();
    const bool accepted = ready.toBool();
    clock(model);
    if (accepted) {
      valid = ValidType{0};
      clock(model);
      return;
    }
  }
  throw std::runtime_error("input did not become ready");
}

std::uint64_t entry(std::uint64_t slot, std::uint64_t generation,
                    std::uint64_t epoch, std::uint64_t attempt,
                    std::uint64_t payload) {
  return (slot << 16) | (std::uint64_t{1} << 15) | (generation << 13) |
         (epoch << 10) | (attempt << 8) | payload;
}

std::uint64_t completion(std::uint64_t slot, std::uint64_t generation,
                         std::uint64_t epoch, std::uint64_t attempt,
                         std::uint64_t payload) {
  return (slot << 15) | (generation << 13) | (epoch << 10) | (attempt << 8) |
         payload;
}

std::uint64_t recovery(std::uint64_t nextEpoch, std::uint64_t checkpoint,
                       std::uint64_t boundary, std::uint64_t slot,
                       std::uint64_t generation, std::uint64_t transactionEpoch,
                       std::uint64_t attempt) {
  return (std::uint64_t{1} << 14) | (nextEpoch << 11) | (checkpoint << 9) |
         (boundary << 8) | (slot << 7) | (generation << 5) |
         (transactionEpoch << 2) | attempt;
}

std::uint64_t read(Model &model, std::uint64_t retained, std::uint64_t slot,
                   std::uint64_t generation, std::uint64_t epoch,
                   std::uint64_t attempt) {
  model.out_ready = pyc::cpp::Wire<1>{0};
  const std::uint64_t ref = (retained << 8) | (slot << 7) | (generation << 5) |
                            (epoch << 2) | attempt;
  send(model, model.in2_ready, model.in2_valid, model.in2_data, ref);
  for (unsigned cycle = 0; cycle < 32; ++cycle) {
    model.eval();
    if (model.out_valid.toBool()) {
      const std::uint64_t value = model.out_data.value();
      model.out_ready = pyc::cpp::Wire<1>{1};
      clock(model);
      model.out_ready = pyc::cpp::Wire<1>{0};
      return value;
    }
    clock(model);
  }
  throw std::runtime_error("read result did not become valid");
}

} // namespace

int main() {
  Model model;
  model.rst = pyc::cpp::Wire<1>{1};
  clock(model);
  clock(model);
  model.rst = pyc::cpp::Wire<1>{0};

  send(model, model.in0_ready, model.in0_valid, model.in0_data,
       entry(0, 0, 4, 0, 0x11));
  send(model, model.in0_ready, model.in0_valid, model.in0_data,
       entry(0, 1, 5, 0, 0x22));

  send(model, model.in1_ready, model.in1_valid, model.in1_data,
       completion(0, 0, 4, 0, 0xaa));
  if (read(model, 0, 0, 1, 5, 0) != 0x22)
    return 1;
  if (model.obligation_no_stale_update_window_complete_slot0_coverage == 0)
    return 2;

  send(model, model.in1_ready, model.in1_valid, model.in1_data,
       completion(0, 1, 5, 0, 0xbb));
  if (read(model, 0, 0, 1, 5, 0) != 0xbb)
    return 3;

  send(model, model.in3_ready, model.in3_valid, model.in3_data,
       recovery(6, 1, 0, 0, 1, 5, 0));
  send(model, model.in0_ready, model.in0_valid, model.in0_data,
       entry(0, 2, 6, 0, 0xcc));
  send(model, model.in1_ready, model.in1_valid, model.in1_data,
       completion(0, 1, 5, 0, 0xdd));
  if (read(model, 0, 0, 2, 6, 0) != 0xcc)
    return 4;

  send(model, model.in4_ready, model.in4_valid, model.in4_data,
       entry(1, 0, 6, 0, 0xee));
  send(model, model.in4_ready, model.in4_valid, model.in4_data,
       entry(1, 1, 6, 0, 0xff));
  if (read(model, 1, 1, 0, 6, 0) != 0xee)
    return 5;
  if (model.obligation_no_stale_update_retained_retain_slot0_coverage == 0)
    return 6;
  send(model, model.in5_ready, model.in5_valid, model.in5_data,
       completion(1, 0, 6, 0, 0));
  send(model, model.in4_ready, model.in4_valid, model.in4_data,
       entry(1, 1, 6, 0, 0xff));
  if (read(model, 1, 1, 1, 6, 0) != 0xff)
    return 7;

  std::cout << "versioned recovery PASS\n";
  return 0;
}
