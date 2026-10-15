// Observation adapter only: all pipeline behavior comes from upstream RV5S.
#include "isa/rv32isainfo.h"
#include "processors/RISC-V/rv5s/rv5s.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <chrono>
#include <charconv>
#include <iostream>
#include <stdexcept>

using CPU = vsrtl::core::RV5S<uint32_t>;
static QJsonValue number(uint64_t n) { return double(n); }
static uint32_t as_u32(const QJsonValue &v) { return uint32_t(v.toDouble()); }
static uint64_t argument(const char *text) {
  const std::string token(text);
  uint64_t value{};
  auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
  if (error != std::errc{} || end != token.data() + token.size())
    throw std::invalid_argument("invalid unsigned cycle argument");
  return value;
}
static void emitJson(const QJsonObject &o) {
  std::cout << QJsonDocument(o).toJson(QJsonDocument::Compact).constData() << '\n';
}

static QJsonObject snapshot(CPU &p, uint32_t base, int count,
                            QJsonValue retire = QJsonValue::Null,
                            QJsonValue store = QJsonValue::Null) {
  QJsonArray stages, regs, data, rawPC, rawValid;
  const auto memOp = p.data_mem->op.eValue<Ripes::MemOp>();
  unsigned memSize = memOp == Ripes::MemOp::LB || memOp == Ripes::MemOp::LBU || memOp == Ripes::MemOp::SB ? 1 : memOp == Ripes::MemOp::LH || memOp == Ripes::MemOp::LHU || memOp == Ripes::MemOp::SH ? 2 : 4;
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
                              {"wb_value", number(p.reg_wr_src->out.uValue())},
                              {"ifid_expanded", number(p.ifid_reg->instr_out.uValue())},
                              {"sequential_pcs", QJsonArray{number(p.ifid_reg->pc4_out.uValue()), number(p.idex_reg->pc4_out.uValue()), number(p.exmem_reg->pc4_out.uValue()), number(p.memwb_reg->pc4_out.uValue())}},
                              {"mem_read", bool(p.exmem_reg->mem_do_read_out.uValue())},
                              {"mem_address", number(p.data_mem->addr.uValue())},
                              {"mem_size", int(memSize)},
                              {"mem_value", number(p.data_mem->data_out.uValue())}}}};
}

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  try {
    if (argc == 2 && std::string(argv[1]) == "--identity") {
      emitJson({{"ripes", RIPES_SHA}, {"vsrtl", VSRTL_SHA}});
      return 0;
    }
    bool benchmark = argc == 3 && std::string(argv[2]) == "--benchmark";
    const bool fixed = argc == 5 && std::string(argv[2]) == "--benchmark-fixed";
    const bool observe = argc == 3 && std::string(argv[2]) == "--observe";
    if (argc != 2 && !benchmark && !fixed && !observe)
      throw std::runtime_error("usage: ripes5-reference INPUT.json [--benchmark | --benchmark-fixed K N | --observe]");
    const auto warmup = fixed ? argument(argv[3]) : 0;
    const auto measured = fixed ? argument(argv[4]) : 0;
    QFile file(argv[1]);
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("cannot read input");
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
      throw std::runtime_error("invalid JSON input");
    auto input = doc.object();
    auto words = input["words"].toArray(), data = input["data"].toArray();
    if (input.contains("text")) {
      const auto bytes = input["text"].toArray();
      words = {};
      for (int i = 0; i < bytes.size(); i += 4) {
        uint32_t w = 0;
        for (int j = 0; j < 4 && i + j < bytes.size(); ++j) w |= as_u32(bytes[i+j]) << (j*8);
        words.append(double(w));
      }
    }
    auto regs = input["registers"].toArray();
    uint32_t base = as_u32(input["data_base"]), end = as_u32(input["end_pc"]);
    int limit = input["max_cycles"].toInt();
    if (regs.size() != 32 || words.isEmpty() || limit < 1 || as_u32(regs[0]) != 0)
      throw std::runtime_error("invalid input dimensions");
    if (fixed && (!measured || warmup > uint64_t(limit) || measured > uint64_t(limit) - warmup))
      throw std::invalid_argument("fixed window requires N > 0 and K + N <= max_cycles");
    using Clock = std::chrono::steady_clock;
    const auto construct_start = Clock::now();
    QStringList extensions;
    const auto isa = input["isa"].toString("I");
    if (isa.contains('M')) extensions.append("M");
    if (isa.contains('C')) extensions.append("C");
    CPU p(extensions);
    QJsonValue system = QJsonValue::Null;
    bool exiting = false;
    uint32_t exitStatus = 0;
    QString output;
    uint64_t lastEcallCycle = UINT64_MAX;
    p.isExecutableAddress = [&](Ripes::AInt pc) {
      return pc % (isa.contains('C') ? 2 : 4) == 0 && pc < uint64_t(words.size()) * 4;
    };
    p.trapHandler = [&] {
      if (lastEcallCycle == uint64_t(p.getCycleCount())) return;
      lastEcallCycle = p.getCycleCount();
      uint32_t number = p.getRegister(Ripes::RVISA::GPR, 17);
      uint32_t arg = p.getRegister(Ripes::RVISA::GPR, 10);
      system = QJsonArray{true, double(p.idex_reg->pc_out.uValue()), 1, double(number), double(arg)};
      // The callback is combinational: an older MEM-stage store has not yet
      // committed. Capture the call now, complete output after its clock edge.
      // Exit must still finalize here to preserve upstream enable/clear timing.
      if (number == 10 || number == 93) {
        exiting = true;
        exitStatus = number == 93 ? arg : 0;
        p.finalize(Ripes::RipesProcessor::FinalizeReason::exitSyscall);
      } else if (number != 1 && number != 4 && number != 11) {
        throw std::runtime_error("unsupported syscall");
      }
    };
    auto completeOutput = [&](const QJsonValue &event) {
      if (event.isNull()) return;
      const auto call = event.toArray();
      const auto number = as_u32(call[3]);
      const auto arg = as_u32(call[4]);
      if (number == 1) output += QString::number(int32_t(arg));
      else if (number == 11) output += QChar(arg & 255);
      else if (number == 4) {
        bool terminated = false;
        for (unsigned i = 0; i < 8192; ++i) {
          if (arg + i >= 8192) throw std::runtime_error("string outside memory");
          auto ch = p.getMemory().readMemConst(arg + i, 1);
          if (!ch) { terminated = true; break; }
          output += QChar(ushort(ch));
        }
        if (!terminated) throw std::runtime_error("unterminated string");
      }
    };
    auto clockWithEnvironment = [&] {
      const auto edgeSystem = system;
      system = QJsonValue::Null;
      p.clockUnguarded();
      completeOutput(edgeSystem);
      return edgeSystem;
    };
    p.postConstruct();
    p.setMaxReverseCycles(0);
    // Public observation controls only; circuit propagation and retirement stay intact.
    p.setEnableSignals(observe);
    p.setEnableClockedSignals(observe);
    p.resetProcessor();
    for (int i = 0; i < words.size(); ++i) p.getMemory().writeMem(i * 4, as_u32(words[i]), 4);
    for (int i = 0; i < data.size(); ++i) p.getMemory().writeMem(base + i * 4, as_u32(data[i]), 4);
    for (unsigned i = 1; i < 32; ++i) p.setRegister(Ripes::RVISA::GPR, i, as_u32(regs[i]));
    p.setProgramCounter(as_u32(input["entry"])); // Propagate after all external initialization.
    const auto construct_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - construct_start).count();
    if (fixed) {
      for (uint64_t i = 0; i < warmup; ++i) clockWithEnvironment();
      const auto before = p.getInstructionsRetired();
      const auto start = Clock::now();
      for (uint64_t i = 0; i < measured; ++i) clockWithEnvironment();
      const auto stop = Clock::now();
      const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
      emitJson({{"cycles", number(p.getCycleCount())}, {"warmup_cycles", number(warmup)},
                {"measured_cycles", number(measured)}, {"run_ns", number(ns)},
                {"construct_ns", number(construct_ns)}, {"retired_before", number(before)},
                {"retired", number(p.getInstructionsRetired())},
                {"retired_delta", number(p.getInstructionsRetired() - before)},
                {"final_state", snapshot(p, base, data.size())},
                {"port_notifications", p.signalsEnabled()},
                {"clock_notifications", p.clockedSignalsEnabled()}, {"reverse_history", false}});
      return 0;
    }
    if (!benchmark) emitJson(snapshot(p, base, data.size()));
    auto start = std::chrono::steady_clock::now();
    bool done = false;
    for (int tick = 0; tick < limit; ++tick) {
      if (p.pc_reg->out.uValue() % (isa.contains('C') ? 2 : 4))
        throw std::runtime_error("unaligned instruction address outside scope");
      if (p.exmem_reg->mem_do_read_out.uValue() || p.data_mem->wr_en.uValue()) {
        uint64_t addr = p.data_mem->addr.uValue();
        auto op = p.data_mem->op.eValue<Ripes::MemOp>();
        unsigned size = op == Ripes::MemOp::LB || op == Ripes::MemOp::LBU || op == Ripes::MemOp::SB ? 1 : op == Ripes::MemOp::LH || op == Ripes::MemOp::LHU || op == Ripes::MemOp::SH ? 2 : 4;
        if (!((addr < 4096 && addr + size <= 4096 && !p.data_mem->wr_en.uValue()) || (addr >= base && addr + size <= uint64_t(base) + 4096)))
          throw std::runtime_error("data access outside aligned data region");
      }
      bool valid = p.memwb_reg->valid_out.uValue() && p.isExecutableAddress(p.memwb_reg->pc_out.uValue());
      uint32_t pc = p.memwb_reg->pc_out.uValue();
      QJsonValue retire = QJsonValue::Null, store = QJsonValue::Null;
      if (!benchmark) {
        if (valid) {
          unsigned rd = p.memwb_reg->wr_reg_idx_out.uValue();
          bool writes = p.memwb_reg->reg_do_write_out.uValue() && rd;
          retire = QJsonObject{{"pc", number(pc)}, {"word", number(p.getMemory().readMemConst(pc, (isa.contains('C') && (p.getMemory().readMemConst(pc, 2) & 3) != 3) ? 2 : 4))},
                              {"write", writes ? QJsonValue(QJsonArray{int(rd), number(p.reg_wr_src->out.uValue())}) : QJsonValue::Null}};
        }
        if (p.data_mem->wr_en.uValue())
          store = QJsonArray{number(p.data_mem->addr.uValue()), number(p.data_mem->data_in.uValue())};
      }
      auto edgeExiting = exiting;
      auto edgeStatus = exitStatus;
      auto edgeSystem = clockWithEnvironment();
      if (!benchmark) {
        auto row = snapshot(p, base, data.size(), retire, store);
        row["system"] = edgeSystem;
        row["output"] = output;
        row["exit_status"] = double(edgeStatus);
        row["exiting"] = edgeExiting;
        emitJson(row);
      }

      if (valid && (pc == end || (exiting && (p.getMemory().readMemConst(pc, 4) == 115)))) { done = true; break; }
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
