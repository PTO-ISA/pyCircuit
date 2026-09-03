#include "memory_banks.generated.cpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

int main() {
  ac_generated::MemoryBanks model;
  const std::array<ac_generated::BankRequest, 5> requests{{
      ac_generated::BankRequest{0, 3, 1, 41, 1},
      ac_generated::BankRequest{1, 3, 1, 91, 2},
      ac_generated::BankRequest{0, 3, 0, 0, 3},
      ac_generated::BankRequest{1, 3, 0, 0, 4},
      ac_generated::BankRequest{2, 3, 0, 0, 5},
  }};

  for (const auto &request : requests)
    if (!model.requests().proposePush(request))
      return 1;

  auto rows = model.dispatch_rows();
  std::size_t cycles = 0;
  for (std::size_t tick = 0; tick < 64; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
    if (model.sink_0_values().size() == requests.size()) {
      cycles = tick + 1;
      break;
    }
  }

  std::array<std::uint16_t, 6> oldDataByTag{};
  const auto &responses = model.sink_0_values();
  if (responses.size() != requests.size())
    return 2;
  for (const auto &response : responses) {
    if (response.tag == 0 || response.tag >= oldDataByTag.size())
      return 3;
    oldDataByTag[response.tag] = response.data;
  }
  if (oldDataByTag[1] != 0 || oldDataByTag[2] != 0 || oldDataByTag[3] != 41 ||
      oldDataByTag[4] != 91 || oldDataByTag[5] != 0)
    return 4;

  std::cout << "cycles=" << cycles << " bank0=" << oldDataByTag[3]
            << " bank1=" << oldDataByTag[4]
            << " bank2_initial=" << oldDataByTag[5] << "\n";
  return 0;
}
