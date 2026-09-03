#include "dma.generated.cpp"

#include <cstddef>
#include <cstdint>
#include <iostream>

namespace {

void step(ac_generated::Dma &model, std::size_t tick) {
  const gfsim::Epoch epoch{tick, 0};
  auto rows = model.dispatch_rows();
  for (auto &row : rows)
    row.work(row.object, epoch);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
}

template <typename Done>
std::size_t runUntil(ac_generated::Dma &model, std::size_t &nextTick,
                     Done done) {
  while (nextTick < 64) {
    const std::size_t current = nextTick++;
    step(model, current);
    if (done())
      return current;
  }
  return 64;
}

} // namespace

int main() {
  ac_generated::Dma model;
  std::size_t nextTick = 0;

  // Host writes 0x1234 to DRAM[5].
  if (!model.dram_seed().proposePush({5, 3, 0x1234, 1}))
    return 1;
  const std::size_t seedTick = runUntil(
      model, nextTick, [&] { return model.sink_0_values().size() == 1; });
  if (seedTick == 64 || model.sink_0_values()[0].data != 0)
    return 2;

  // DMA reads DRAM[5] and writes the returned value into SRAM[3].
  if (!model.requests().proposePush({5, 3, 0, 2}))
    return 3;
  const std::size_t copyTick = runUntil(
      model, nextTick, [&] { return model.sink_1_values().size() == 1; });
  if (copyTick == 64 || model.sink_1_values()[0].tag != 2 ||
      model.sink_1_values()[0].data != 0)
    return 4;

  // Read SRAM[3].  The response proves that the DMA copied the DRAM value.
  if (!model.sram_reads().proposePush({0, 3, 0, 3}))
    return 5;
  const std::size_t verifyTick = runUntil(
      model, nextTick, [&] { return model.sink_2_values().size() == 1; });
  if (verifyTick == 64 || model.sink_2_values()[0].tag != 3 ||
      model.sink_2_values()[0].data != 0x1234)
    return 6;

  std::cout << "seed_tick=" << seedTick << " copy_tick=" << copyTick
            << " verify_tick=" << verifyTick << " dram_value=0x" << std::hex
            << model.sink_2_values()[0].data << std::dec
            << " copy_old_sram=" << model.sink_1_values()[0].data << "\n";
  return 0;
}
