// Observation adapter only: all pipeline behavior comes from upstream RV5S.
#include "isa/rv32isainfo.h"
#include "processors/RISC-V/rv5s/rv5s.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <chrono>
#include <iostream>
#include <stdexcept>

using CPU = vsrtl::core::RV5S<uint32_t>;
static QJsonValue number(uint64_t n) { return double(n); }
static uint32_t as_u32(const QJsonValue &v) { return uint32_t(v.toDouble()); }
static void emitJson(const QJsonObject &o) {
  std::cout << QJsonDocument(o).toJson(QJsonDocument::Compact).constData() << '\n';
}

static QJsonObject snapshot(CPU &p, uint32_t base, int count,
                            QJsonValue retire = QJsonValue::Null,
                            QJsonValue store = QJsonValue::Null) {
  QJsonArray stages, regs, data, rawPC, rawValid;
  for (unsigned i = 0; i < 5; ++i) {
    const auto s = p.stageInfo({0, i});
    stages.append(QJsonObject{{"valid", s.stage_valid},
                             {"pc", s.stage_valid ? number(s.pc) : QJsonValue::Null}});
    rawPC.append(number(p.getPcForStage({0, i})));
  }
  rawValid = {true, bool(p.ifid_reg->valid_out.uValue()),
              bool(p.idex_reg->valid_out.uValue()), bool(p.exmem_reg->valid_out.uValue()),
              bool(p.memwb_reg->valid_out.uValue())};
  for (unsigned i = 0; i < 32; ++i)
    regs.append(number(uint32_t(p.getRegister(Ripes::RVISA::GPR, i))));
  for (int i = 0; i < count; ++i)
    data.append(number(p.getMemory().readMemConst(base + i * 4, 4)));
  bool redirect = p.controlflow_or->out.uValue();
  bool enable = p.pc_reg->enable.uValue();
  QJsonObject control{
      {"stall", bool(p.hzunit->hazardIDEXClear.uValue())},
      {"flush_ifid", bool(p.ifid_reg->clear.uValue())},
      {"flush_idex", bool(p.idex_reg->clear.uValue())},
      {"pc_enable", enable},
      {"idex_enable", bool(p.idex_reg->enable.uValue())},
      {"exmem_clear", bool(p.exmem_reg->clear.uValue())},
      {"target", redirect ? number(p.alu->res.uValue()) : QJsonValue::Null},
      {"forward_a", number(p.funit->alu_reg1_forwarding_ctrl.uValue())},
      {"forward_b", number(p.funit->alu_reg2_forwarding_ctrl.uValue())}};
  QJsonArray stalled{false, false, bool(p.idex_reg->stalled_out.uValue()),
                     bool(p.exmem_reg->stalled_out.uValue()),
                     bool(p.memwb_reg->stalled_out.uValue())};
  return {{"cycle", number(p.getCycleCount())}, {"stages", stages},
          {"fetch_pc", number(p.pc_reg->out.uValue())},
          {"next_fetch", number(p.pc_src->out.uValue())},
          {"next_pc", number(enable ? p.pc_src->out.uValue() : p.pc_reg->out.uValue())},
          {"control", control}, {"stalled", stalled}, {"registers", regs}, {"data", data},
          {"retired", number(p.getInstructionsRetired())}, {"retire", retire}, {"store", store},
          {"raw", QJsonObject{{"pcs", rawPC}, {"valid", rawValid},
                              {"alu", number(p.alu->res.uValue())},
                              {"wb_value", number(p.reg_wr_src->out.uValue())}}}};
}

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  try {
    if (argc < 2 || argc > 3)
      throw std::runtime_error("usage: ripes5-reference INPUT.json [--benchmark]");
    if (std::string(argv[1]) == "--identity") {
      emitJson({{"ripes", RIPES_SHA}, {"vsrtl", VSRTL_SHA}});
      return 0;
    }
    bool benchmark = argc == 3 && std::string(argv[2]) == "--benchmark";
    QFile file(argv[1]);
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("cannot read input");
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
      throw std::runtime_error("invalid JSON input");
    auto input = doc.object();
    auto words = input["words"].toArray(), data = input["data"].toArray();
    auto regs = input["registers"].toArray();
    uint32_t base = as_u32(input["data_base"]), end = as_u32(input["end_pc"]);
    int limit = input["max_cycles"].toInt();
    if (regs.size() != 32 || words.isEmpty() || limit < 1 || as_u32(regs[0]) != 0)
      throw std::runtime_error("invalid input dimensions");
    CPU p({}); // RV32I, no optional extensions.
    p.isExecutableAddress = [&](Ripes::AInt pc) {
      return pc % 4 == 0 && pc < uint64_t(words.size()) * 4;
    };
    p.trapHandler = [] { throw std::runtime_error("syscall outside comparison scope"); };
    p.postConstruct();
    p.setMaxReverseCycles(0);
    p.resetProcessor();
    for (int i = 0; i < words.size(); ++i) p.getMemory().writeMem(i * 4, as_u32(words[i]), 4);
    for (int i = 0; i < data.size(); ++i) p.getMemory().writeMem(base + i * 4, as_u32(data[i]), 4);
    for (unsigned i = 1; i < 32; ++i) p.setRegister(Ripes::RVISA::GPR, i, as_u32(regs[i]));
    p.setProgramCounter(0); // Propagate after all external initialization.
    if (!benchmark) emitJson(snapshot(p, base, data.size()));
    auto start = std::chrono::steady_clock::now();
    bool done = false;
    for (int tick = 0; tick < limit; ++tick) {
      if (p.pc_reg->out.uValue() % 4)
        throw std::runtime_error("unaligned instruction address outside scope");
      if (p.exmem_reg->mem_do_read_out.uValue() || p.data_mem->wr_en.uValue()) {
        uint64_t addr = p.data_mem->addr.uValue();
        if (addr % 4 || addr < base || addr + 4 > uint64_t(base) + data.size() * 4)
          throw std::runtime_error("data access outside aligned data region");
      }
      bool valid = p.memwb_reg->valid_out.uValue() && p.isExecutableAddress(p.memwb_reg->pc_out.uValue());
      uint32_t pc = p.memwb_reg->pc_out.uValue();
      QJsonValue retire = QJsonValue::Null, store = QJsonValue::Null;
      if (!benchmark) {
        if (valid) {
          unsigned rd = p.memwb_reg->wr_reg_idx_out.uValue();
          bool writes = p.memwb_reg->reg_do_write_out.uValue() && rd;
          retire = QJsonObject{{"pc", number(pc)}, {"word", words[pc / 4]},
                              {"write", writes ? QJsonValue(QJsonArray{int(rd), number(p.reg_wr_src->out.uValue())}) : QJsonValue::Null}};
        }
        if (p.data_mem->wr_en.uValue())
          store = QJsonArray{number(p.data_mem->addr.uValue()), number(p.data_mem->data_in.uValue())};
      }
      p.clockUnguarded();
      if (!benchmark) emitJson(snapshot(p, base, data.size(), retire, store));
      if (valid && pc == end) { done = true; break; }
    }
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
    if (!done) throw std::runtime_error("end marker did not retire within max_cycles");
    if (benchmark) emitJson({{"cycles", number(p.getCycleCount())}, {"run_ns", number(ns)}});
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
