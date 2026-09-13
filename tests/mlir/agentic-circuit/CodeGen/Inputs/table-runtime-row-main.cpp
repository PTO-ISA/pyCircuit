#ifndef GENERATED_SOURCE
#error "GENERATED_SOURCE must name the generated QueueGraph C++ source"
#endif
#include GENERATED_SOURCE

#include <iostream>

int main() {
  using gfsim::UInt;
  ac_generated::TableRuntimeRow model;
  auto rows = model.dispatch_rows();
  gfsim::SimTable<UInt<8>> *table = nullptr;
  for (auto &row : rows) {
    auto *object = static_cast<gfsim::SimObject *>(row.object);
    if (row.kind == gfsim::ObjectKind::Memory && object->name() == "state")
      table = dynamic_cast<gfsim::SimTable<UInt<8>> *>(object);
  }
  if (table == nullptr)
    return 1;

  uint64_t tickNumber = 1;
  auto tick = [&] {
    const gfsim::Epoch epoch{tickNumber++, 0};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  };
  auto request = [&](uint64_t row, uint64_t tag) {
    if (!model.requests().proposePush(
            ac_generated::Request{UInt<2>{row}, UInt<8>{tag}}))
      return false;
    model.requests().doXfer({tickNumber - 1, 0});
    tick();
    tick();
    return true;
  };

  if (!request(2, 32) || !request(1, 21) || !request(2, 32) || !request(3, 99))
    return 2;
  const auto &values = model.sink_0_values();
  if (values.size() != 4 ||
      values[0] != ac_generated::Result{UInt<4>{8}, UInt<1>{1}} ||
      values[1] != ac_generated::Result{UInt<4>{5}, UInt<1>{1}} ||
      values[2] != ac_generated::Result{UInt<4>{9}, UInt<1>{1}} ||
      values[3] != ac_generated::Result{UInt<4>{0}, UInt<1>{0}})
    return 3;
  if (table->at(8) != UInt<8>{0} || table->at(9) != UInt<8>{0} ||
      table->at(5) != UInt<8>{0} || table->at(10) != UInt<8>{34})
    return 4;
  std::cout << "PASS gfsim table runtime row capture and update\n";
  return 0;
}
