#pragma once
#include "runtime.hpp"
#include <set>
namespace acir_generated {
  using namespace acir_sim;
  using acir_sim::json;
  // ACPy payload src_isa_f0424bc8.Executed
  struct Executed {
    uint32_t pc{};
    uint32_t word{};
    uint32_t kind{};
    uint32_t rd{};
    uint32_t value{};
    uint32_t store_value{};
    bool writes{};
    bool operator==(const Executed&) const = default;
  };
  inline void json(std::ostream &o,const Executed &v) { o<<'[';
    json(o,v.pc);
    o<<','; json(o,v.word);
    o<<','; json(o,v.kind);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.value);
    o<<','; json(o,v.store_value);
    o<<','; json(o,v.writes);
    o<<']'; }
  // ACPy payload src_isa_f0424bc8.Operands
  struct Operands {
    uint32_t pc{};
    uint32_t word{};
    uint32_t kind{};
    uint32_t rd{};
    uint32_t lhs{};
    uint32_t rhs{};
    uint32_t store_value{};
    bool writes{};
    bool operator==(const Operands&) const = default;
  };
  inline void json(std::ostream &o,const Operands &v) { o<<'[';
    json(o,v.pc);
    o<<','; json(o,v.word);
    o<<','; json(o,v.kind);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.lhs);
    o<<','; json(o,v.rhs);
    o<<','; json(o,v.store_value);
    o<<','; json(o,v.writes);
    o<<']'; }
  // ACPy payload src_isa_f0424bc8.Fetched
  struct Fetched {
    uint32_t pc{};
    uint32_t word{};
    bool operator==(const Fetched&) const = default;
  };
  inline void json(std::ostream &o,const Fetched &v) { o<<'[';
    json(o,v.pc);
    o<<','; json(o,v.word);
    o<<']'; }
  // ACPy payload src_isa_f0424bc8.Retired
  struct Retired {
    uint32_t pc{};
    uint32_t word{};
    uint32_t rd{};
    uint32_t value{};
    bool writes{};
    bool operator==(const Retired&) const = default;
  };
  inline void json(std::ostream &o,const Retired &v) { o<<'[';
    json(o,v.pc);
    o<<','; json(o,v.word);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.value);
    o<<','; json(o,v.writes);
    o<<']'; }
  // ACPy payload src_isa_f0424bc8.Decoded
  struct Decoded {
    uint32_t pc{};
    uint32_t word{};
    uint32_t kind{};
    uint32_t rd{};
    uint32_t rs1{};
    uint32_t rs2{};
    uint32_t immediate{};
    bool immediate_rhs{};
    bool writes{};
    bool operator==(const Decoded&) const = default;
  };
  inline void json(std::ostream &o,const Decoded &v) { o<<'[';
    json(o,v.pc);
    o<<','; json(o,v.word);
    o<<','; json(o,v.kind);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.rs1);
    o<<','; json(o,v.rs2);
    o<<','; json(o,v.immediate);
    o<<','; json(o,v.immediate_rhs);
    o<<','; json(o,v.writes);
    o<<']'; }
  class Model {
  public:
    struct Pipeline {
      struct DecodeStage {
        // Rule top.decode_stage.read_operands#1
        template<class Context> void try_read_operands(Context &ctx) {
          const bool mem_wb_empty = ctx.top.mem_wb.empty();
          const Fetched if_id_value = ctx.top.if_id.read();
          const Fetched packet = if_id_value;
          const uint32_t word = packet.word;
          const uint32_t opcode = (word & 127u);
          const bool condition = (opcode==19u);
          const bool condition_2 = (opcode==51u);
          const uint32_t funct7 = (word >> 25u);
          const bool condition_3 = (funct7==32u);
          const uint32_t kind = uint32_t(condition_3);
          const bool writes = (opcode==3u);
          const uint32_t kind_2 = (writes ? 2u : 3u);
          const uint32_t kind_3 = (condition_2 ? kind : kind_2);
          const uint32_t kind_4 = (condition ? 0u : kind_3);
          const uint32_t rd = ((word >> 7u) & 31u);
          const uint32_t rd_2 = (writes ? rd : 0u);
          const uint32_t rd_3 = (condition_2 ? rd : rd_2);
          const uint32_t rd_4 = (condition ? rd : rd_3);
          const uint32_t rs1 = ((word >> 15u) & 31u);
          const uint32_t computed_value_3 = (word >> 20u);
          const uint32_t rs2 = (computed_value_3 & 31u);
          const uint32_t rs2_2 = (writes ? 0u : rs2);
          const uint32_t rs2_3 = (condition_2 ? rs2 : rs2_2);
          const uint32_t rs2_4 = (condition ? 0u : rs2_3);
          const uint32_t immediate = ((computed_value_3 ^ 2048u) - 2048u);
          const uint32_t value = ((funct7 << 5u) | rd);
          const uint32_t immediate_2 = ((value ^ 2048u) - 2048u);
          const uint32_t immediate_3 = (writes ? immediate : immediate_2);
          const uint32_t immediate_4 = (condition_2 ? 0u : immediate_3);
          const uint32_t immediate_5 = (condition ? immediate : immediate_4);
          const bool condition_4 = (opcode!=51u);
          const bool immediate_rhs = (condition || condition_4);
          const bool writes_2 = (condition_2 || writes);
          const bool writes_3 = (condition || writes_2);
          const Decoded inst = Decoded{packet.pc,word,kind_4,rd_4,rs1,rs2_4,immediate_5,immediate_rhs,writes_3};
          const uint32_t rs1_2 = inst.rs1;
          const uint64_t computed_value_7 = uint64_t(rs1_2);
          const bool pending_value = ((computed_value_7 < 32) ? ctx.top.rf.pending.at(computed_value_7).read() : bool{});
          const bool ready = (!pending_value);
          const uint32_t rs2_5 = inst.rs2;
          const uint64_t computed_value_8 = uint64_t(rs2_5);
          const bool pending_value_2 = ((computed_value_8 < 32) ? ctx.top.rf.pending.at(computed_value_8).read() : bool{});
          const bool ready_2 = (!pending_value_2);
          const bool ready_3 = inst.writes;
          const uint32_t rd_5 = inst.rd;
          const uint64_t computed_value_9 = uint64_t(rd_5);
          const bool pending_value_3 = ((computed_value_9 < 32) ? ctx.top.rf.pending.at(computed_value_9).read() : bool{});
          const bool ready_4 = (!pending_value_3);
          const bool if_id_ready = (!ctx.top.if_id.empty());
          const bool ready_5 = (if_id_ready && condition);
          if ((mem_wb_empty && (ready && (ready_2 && (ready_3 ? (ready_4 && ready_5) : ready_5))))) {
            const uint32_t funct3 = ((word >> 12u) & 7u);
            const bool condition_5 = (funct3==0u);
            acir_sim::check(condition_5);
          }
          const bool condition_6 = (opcode!=19u);
          const bool ready_11 = (if_id_ready && (condition_6 && condition_2));
          if ((mem_wb_empty && (ready && (ready_2 && (ready_3 ? (ready_4 && ready_11) : ready_11))))) {
            const uint32_t funct3_2 = ((word >> 12u) & 7u);
            const bool condition_7 = (funct3_2==0u);
            acir_sim::check((condition_7 && ((funct7==0u) || condition_3)));
          }
          const bool ready_20 = (if_id_ready && (condition_6 && (condition_4 && writes)));
          if ((mem_wb_empty && (ready && (ready_2 && (ready_3 ? (ready_4 && ready_20) : ready_20))))) {
            const uint32_t funct3_3 = ((word >> 12u) & 7u);
            const bool condition_9 = (funct3_3==2u);
            acir_sim::check(condition_9);
          }
          const bool writes_4 = (opcode!=3u);
          const bool ready_27 = (if_id_ready && (condition_6 && (condition_4 && writes_4)));
          if ((mem_wb_empty && (ready && (ready_2 && (ready_3 ? (ready_4 && ready_27) : ready_27))))) {
            const uint32_t funct3_4 = ((word >> 12u) & 7u);
            const bool condition_11 = (funct3_4==2u);
            acir_sim::check(((opcode==35u) && condition_11));
          }
          const bool condition_12 = (computed_value_7<uint64_t(32ULL));
          const bool condition_13 = (computed_value_8<uint64_t(32ULL));
          const bool id_ex_full = ctx.top.id_ex.full();
          const bool ready_33 = (!id_ex_full);
          const bool ready_37 = (if_id_ready && (condition_12 && (condition_13 && ((computed_value_9<uint64_t(32ULL)) && ready_33))));
          const bool ready_42 = (ready_2 && (ready_3 ? (ready_4 && ready_37) : (if_id_ready && (condition_12 && (condition_13 && ready_33)))));
          if (!((mem_wb_empty && (ready && ready_42)))) return;
          // Prepare every effect before publishing state operations.
          const bool write_fire = (ready_3 && (ready_4 && (rd_5!=0u)));
          bool write_value{};
          uint64_t resource_index{};
          if (write_fire) {
            write_value = true;
            resource_index = computed_value_9;
          }
          const bool condition_16 = (rs1_2!=0u);
          const uint32_t registers_value = ((computed_value_7 < 32) ? ctx.top.rf.registers.at(computed_value_7).read() : uint32_t{});
          const uint32_t payload = uint32_t{};
          const uint32_t lhs = (condition_16 ? registers_value : ((rs1_2==0u) ? 0u : payload));
          const uint32_t registers_value_2 = ((computed_value_8 < 32) ? ctx.top.rf.registers.at(computed_value_8).read() : uint32_t{});
          const bool condition_19 = (rs2_5==0u);
          const uint32_t store_value = ((rs2_5!=0u) ? registers_value_2 : (condition_19 ? 0u : payload));
          const uint32_t rhs = (inst.immediate_rhs ? inst.immediate : store_value);
          const Operands push_value = Operands{inst.pc,inst.word,inst.kind,rd_5,lhs,rhs,store_value,ready_3};
          ctx.top.if_id.pop();
          if (write_fire) { ctx.top.rf.pending.at(resource_index).write(write_value); }
          ctx.top.id_ex.push(push_value);
          ctx.fired_top_decode_stage_read_operands=true;
        }
      } decode_stage;
      struct Execute {
        // Rule top.execute.execute#2
        template<class Context> void try_execute(Context &ctx) {
          const bool id_ex_ready = (!ctx.top.id_ex.empty());
          const bool ex_mem_full = ctx.top.ex_mem.full();
          if (!((id_ex_ready && (!ex_mem_full)))) return;
          // Prepare every effect before publishing state operations.
          const Operands id_ex_value = ctx.top.id_ex.read();
          const Operands inst = id_ex_value;
          const uint32_t kind = inst.kind;
          const uint32_t lhs = inst.lhs;
          const uint32_t rhs = inst.rhs;
          const uint32_t result = ((kind==1u) ? (lhs - rhs) : (lhs + rhs));
          const Executed push_value = Executed{inst.pc,inst.word,kind,inst.rd,result,inst.store_value,inst.writes};
          ctx.top.id_ex.pop();
          ctx.top.ex_mem.push(push_value);
          ctx.fired_top_execute_execute=true;
        }
      } execute;
      struct Fetch {
        Cell<uint32_t> pc{};
        Cell<std::array<uint32_t,6>> rom{};
        // Rule top.fetch.fetch#0
        template<class Context> void try_fetch(Context &ctx) {
          const uint32_t pc_value = ctx.top.fetch.pc.read();
          const uint32_t current_pc = pc_value;
          const bool condition = (current_pc<24u);
          const uint64_t computed_value_2 = uint64_t((current_pc >> 2u));
          const bool if_id_full = ctx.top.if_id.full();
          if (!((condition && ((computed_value_2>=uint64_t(6ULL)) || (!if_id_full))))) return;
          // Prepare every effect before publishing state operations.
          const uint32_t write_value = (current_pc + 4u);
          const bool condition_3 = (computed_value_2<uint64_t(6ULL));
          const bool push_fire = condition_3;
          Fetched push_value{};
          if (push_fire) {
            const std::array<uint32_t,6> rom_value = ctx.top.fetch.rom.read();
            const uint32_t word = rom_value.at((condition_3 ? computed_value_2 : uint64_t(0ULL)));
            push_value = Fetched{current_pc,word};
          }
          ctx.top.fetch.pc.write(write_value);
          if (push_fire) { ctx.top.if_id.push(push_value); }
          ctx.fired_top_fetch_fetch=true;
        }
      } fetch;
      struct DataMemory {
        Cell<std::array<uint32_t,16>> words{};
      } memory;
      struct MemoryStage {
        // Rule top.memory_stage.access#3
        template<class Context> void try_access(Context &ctx) {
          const bool ex_mem_ready = (!ctx.top.ex_mem.empty());
          const Executed ex_mem_value = ctx.top.ex_mem.read();
          const Executed inst = ex_mem_value;
          const uint32_t kind = inst.kind;
          const bool condition = (kind==2u);
          const uint32_t result = inst.value;
          const uint64_t computed_value_2 = uint64_t((result >> 2u));
          const bool mem_wb_full = ctx.top.mem_wb.full();
          const bool ready = (!mem_wb_full);
          const bool condition_3 = (kind==3u);
          const bool condition_4 = (computed_value_2<uint64_t(16ULL));
          const bool selected_value_2 = (condition ? ((computed_value_2>=uint64_t(16ULL)) || ready) : (condition_3 ? (condition_4 && ready) : ready));
          if (!((ex_mem_ready && selected_value_2))) return;
          // Prepare every effect before publishing state operations.
          const bool condition_5 = (kind!=2u);
          const bool check_fire = (condition_5 && condition_3);
          if (check_fire) {
            const bool ready_7 = (((result & 3u)==0u) && (result<64u));
            acir_sim::check(ready_7);
          }
          const bool write_fire = (condition_5 && condition_3);
          std::array<uint32_t,16> write_value{};
          if (write_fire) {
            const std::array<uint32_t,16> words_value = ctx.top.memory.words.read();
            const uint32_t value = inst.store_value;
            const uint64_t selected_value_3 = (condition_4 ? computed_value_2 : uint64_t(0ULL));
            write_value = [&]{ auto r=words_value; r.at(selected_value_3)=value; return r; }();
          }
          const bool check_fire_2 = condition;
          if (check_fire_2) {
            const bool ready_9 = (((result & 3u)==0u) && (result<64u));
            acir_sim::check(ready_9);
          }
          const bool push_fire = (condition_5 || condition_4);
          Retired push_value{};
          if (push_fire) {
            const std::array<uint32_t,16> words_value_2 = ctx.top.memory.words.read();
            const uint64_t selected_value_4 = (condition_4 ? computed_value_2 : uint64_t(0ULL));
            const uint32_t result_2 = words_value_2.at(selected_value_4);
            const uint32_t result_3 = (condition ? result_2 : result);
            push_value = Retired{inst.pc,inst.word,inst.rd,result_3,inst.writes};
          }
          ctx.top.ex_mem.pop();
          if (write_fire) { ctx.top.memory.words.write(write_value); }
          if (push_fire) { ctx.top.mem_wb.push(push_value); }
          ctx.fired_top_memory_stage_access=true;
        }
      } memory_stage;
      struct RegisterFile {
        std::array<Cell<bool>,32> pending{};
        std::array<Cell<uint32_t>,32> registers{};
      } rf;
      struct Writeback {
        Cell<uint32_t> last_pc{};
        Cell<uint32_t> retired{};
        // Rule top.writeback.commit#4
        template<class Context> void try_commit(Context &ctx) {
          const bool mem_wb_ready = (!ctx.top.mem_wb.empty());
          const Retired mem_wb_value = ctx.top.mem_wb.read();
          const Retired inst = mem_wb_value;
          const bool ready = inst.writes;
          const uint32_t rd = inst.rd;
          const uint64_t computed_value = uint64_t(rd);
          if (!((mem_wb_ready && ((!ready) || ((rd==0u) || (computed_value<uint64_t(32ULL))))))) return;
          // Prepare every effect before publishing state operations.
          const bool write_fire = (ready && (rd!=0u));
          uint32_t write_value{};
          uint64_t resource_index{};
          if (write_fire) {
            write_value = inst.value;
            resource_index = computed_value;
          }
          const bool write_fire_2 = (ready && (rd!=0u));
          bool write_value_2{};
          uint64_t resource_index_2{};
          if (write_fire_2) {
            write_value_2 = false;
            resource_index_2 = computed_value;
          }
          const uint32_t retired_value = ctx.top.writeback.retired.read();
          const uint32_t write_value_3 = (retired_value + 1u);
          const uint32_t write_value_4 = inst.pc;
          ctx.top.mem_wb.pop();
          if (write_fire) { ctx.top.rf.registers.at(resource_index).write(write_value); }
          if (write_fire_2) { ctx.top.rf.pending.at(resource_index_2).write(write_value_2); }
          ctx.top.writeback.retired.write(write_value_3);
          ctx.top.writeback.last_pc.write(write_value_4);
          ctx.fired_top_writeback_commit=true;
        }
      } writeback;
      Queue<Executed,1> ex_mem{};
      Queue<Operands,1> id_ex{};
      Queue<Fetched,1> if_id{};
      Queue<Retired,1> mem_wb{};
    } top;
    bool fired_top_writeback_commit=false;
    bool fired_top_memory_stage_access=false;
    bool fired_top_execute_execute=false;
    bool fired_top_decode_stage_read_operands=false;
    bool fired_top_fetch_fetch=false;
    Model() { reset(); }
    void reset() {
      top.ex_mem={};
      top.ex_mem.count=0;
      top.fetch.pc={};
      top.fetch.pc.value=0u;
      top.fetch.rom={};
      top.fetch.rom.value=std::array<uint32_t,6>{7340179u,5243155u,2130355u,3153955u,8707u,1075970739u};
      top.id_ex={};
      top.id_ex.count=0;
      top.if_id={};
      top.if_id.count=0;
      top.mem_wb={};
      top.mem_wb.count=0;
      top.memory.words={};
      top.memory.words.value=std::array<uint32_t,16>{0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u};
      top.rf.pending[0]={};
      top.rf.pending[0].value=false;
      top.rf.pending[1]={};
      top.rf.pending[1].value=false;
      top.rf.pending[10]={};
      top.rf.pending[10].value=false;
      top.rf.pending[11]={};
      top.rf.pending[11].value=false;
      top.rf.pending[12]={};
      top.rf.pending[12].value=false;
      top.rf.pending[13]={};
      top.rf.pending[13].value=false;
      top.rf.pending[14]={};
      top.rf.pending[14].value=false;
      top.rf.pending[15]={};
      top.rf.pending[15].value=false;
      top.rf.pending[16]={};
      top.rf.pending[16].value=false;
      top.rf.pending[17]={};
      top.rf.pending[17].value=false;
      top.rf.pending[18]={};
      top.rf.pending[18].value=false;
      top.rf.pending[19]={};
      top.rf.pending[19].value=false;
      top.rf.pending[2]={};
      top.rf.pending[2].value=false;
      top.rf.pending[20]={};
      top.rf.pending[20].value=false;
      top.rf.pending[21]={};
      top.rf.pending[21].value=false;
      top.rf.pending[22]={};
      top.rf.pending[22].value=false;
      top.rf.pending[23]={};
      top.rf.pending[23].value=false;
      top.rf.pending[24]={};
      top.rf.pending[24].value=false;
      top.rf.pending[25]={};
      top.rf.pending[25].value=false;
      top.rf.pending[26]={};
      top.rf.pending[26].value=false;
      top.rf.pending[27]={};
      top.rf.pending[27].value=false;
      top.rf.pending[28]={};
      top.rf.pending[28].value=false;
      top.rf.pending[29]={};
      top.rf.pending[29].value=false;
      top.rf.pending[3]={};
      top.rf.pending[3].value=false;
      top.rf.pending[30]={};
      top.rf.pending[30].value=false;
      top.rf.pending[31]={};
      top.rf.pending[31].value=false;
      top.rf.pending[4]={};
      top.rf.pending[4].value=false;
      top.rf.pending[5]={};
      top.rf.pending[5].value=false;
      top.rf.pending[6]={};
      top.rf.pending[6].value=false;
      top.rf.pending[7]={};
      top.rf.pending[7].value=false;
      top.rf.pending[8]={};
      top.rf.pending[8].value=false;
      top.rf.pending[9]={};
      top.rf.pending[9].value=false;
      top.rf.registers[0]={};
      top.rf.registers[0].value=0u;
      top.rf.registers[1]={};
      top.rf.registers[1].value=0u;
      top.rf.registers[10]={};
      top.rf.registers[10].value=0u;
      top.rf.registers[11]={};
      top.rf.registers[11].value=0u;
      top.rf.registers[12]={};
      top.rf.registers[12].value=0u;
      top.rf.registers[13]={};
      top.rf.registers[13].value=0u;
      top.rf.registers[14]={};
      top.rf.registers[14].value=0u;
      top.rf.registers[15]={};
      top.rf.registers[15].value=0u;
      top.rf.registers[16]={};
      top.rf.registers[16].value=0u;
      top.rf.registers[17]={};
      top.rf.registers[17].value=0u;
      top.rf.registers[18]={};
      top.rf.registers[18].value=0u;
      top.rf.registers[19]={};
      top.rf.registers[19].value=0u;
      top.rf.registers[2]={};
      top.rf.registers[2].value=0u;
      top.rf.registers[20]={};
      top.rf.registers[20].value=0u;
      top.rf.registers[21]={};
      top.rf.registers[21].value=0u;
      top.rf.registers[22]={};
      top.rf.registers[22].value=0u;
      top.rf.registers[23]={};
      top.rf.registers[23].value=0u;
      top.rf.registers[24]={};
      top.rf.registers[24].value=0u;
      top.rf.registers[25]={};
      top.rf.registers[25].value=0u;
      top.rf.registers[26]={};
      top.rf.registers[26].value=0u;
      top.rf.registers[27]={};
      top.rf.registers[27].value=0u;
      top.rf.registers[28]={};
      top.rf.registers[28].value=0u;
      top.rf.registers[29]={};
      top.rf.registers[29].value=0u;
      top.rf.registers[3]={};
      top.rf.registers[3].value=0u;
      top.rf.registers[30]={};
      top.rf.registers[30].value=0u;
      top.rf.registers[31]={};
      top.rf.registers[31].value=0u;
      top.rf.registers[4]={};
      top.rf.registers[4].value=0u;
      top.rf.registers[5]={};
      top.rf.registers[5].value=0u;
      top.rf.registers[6]={};
      top.rf.registers[6].value=0u;
      top.rf.registers[7]={};
      top.rf.registers[7].value=0u;
      top.rf.registers[8]={};
      top.rf.registers[8].value=0u;
      top.rf.registers[9]={};
      top.rf.registers[9].value=0u;
      top.writeback.last_pc={};
      top.writeback.last_pc.value=0u;
      top.writeback.retired={};
      top.writeback.retired.value=0u;
      begin_tick();
    }
    void begin_tick() {
      top.ex_mem.begin();
      top.fetch.pc.begin();
      top.fetch.rom.begin();
      top.id_ex.begin();
      top.if_id.begin();
      top.mem_wb.begin();
      top.memory.words.begin();
      top.rf.pending[0].begin();
      top.rf.pending[1].begin();
      top.rf.pending[10].begin();
      top.rf.pending[11].begin();
      top.rf.pending[12].begin();
      top.rf.pending[13].begin();
      top.rf.pending[14].begin();
      top.rf.pending[15].begin();
      top.rf.pending[16].begin();
      top.rf.pending[17].begin();
      top.rf.pending[18].begin();
      top.rf.pending[19].begin();
      top.rf.pending[2].begin();
      top.rf.pending[20].begin();
      top.rf.pending[21].begin();
      top.rf.pending[22].begin();
      top.rf.pending[23].begin();
      top.rf.pending[24].begin();
      top.rf.pending[25].begin();
      top.rf.pending[26].begin();
      top.rf.pending[27].begin();
      top.rf.pending[28].begin();
      top.rf.pending[29].begin();
      top.rf.pending[3].begin();
      top.rf.pending[30].begin();
      top.rf.pending[31].begin();
      top.rf.pending[4].begin();
      top.rf.pending[5].begin();
      top.rf.pending[6].begin();
      top.rf.pending[7].begin();
      top.rf.pending[8].begin();
      top.rf.pending[9].begin();
      top.rf.registers[0].begin();
      top.rf.registers[1].begin();
      top.rf.registers[10].begin();
      top.rf.registers[11].begin();
      top.rf.registers[12].begin();
      top.rf.registers[13].begin();
      top.rf.registers[14].begin();
      top.rf.registers[15].begin();
      top.rf.registers[16].begin();
      top.rf.registers[17].begin();
      top.rf.registers[18].begin();
      top.rf.registers[19].begin();
      top.rf.registers[2].begin();
      top.rf.registers[20].begin();
      top.rf.registers[21].begin();
      top.rf.registers[22].begin();
      top.rf.registers[23].begin();
      top.rf.registers[24].begin();
      top.rf.registers[25].begin();
      top.rf.registers[26].begin();
      top.rf.registers[27].begin();
      top.rf.registers[28].begin();
      top.rf.registers[29].begin();
      top.rf.registers[3].begin();
      top.rf.registers[30].begin();
      top.rf.registers[31].begin();
      top.rf.registers[4].begin();
      top.rf.registers[5].begin();
      top.rf.registers[6].begin();
      top.rf.registers[7].begin();
      top.rf.registers[8].begin();
      top.rf.registers[9].begin();
      top.writeback.last_pc.begin();
      top.writeback.retired.begin();
      fired_top_writeback_commit=false;
      fired_top_memory_stage_access=false;
      fired_top_execute_execute=false;
      fired_top_decode_stage_read_operands=false;
      fired_top_fetch_fetch=false;
    }
    void commit() {
      top.ex_mem.commit();
      top.fetch.pc.commit();
      top.fetch.rom.commit();
      top.id_ex.commit();
      top.if_id.commit();
      top.mem_wb.commit();
      top.memory.words.commit();
      top.rf.pending[0].commit();
      top.rf.pending[1].commit();
      top.rf.pending[10].commit();
      top.rf.pending[11].commit();
      top.rf.pending[12].commit();
      top.rf.pending[13].commit();
      top.rf.pending[14].commit();
      top.rf.pending[15].commit();
      top.rf.pending[16].commit();
      top.rf.pending[17].commit();
      top.rf.pending[18].commit();
      top.rf.pending[19].commit();
      top.rf.pending[2].commit();
      top.rf.pending[20].commit();
      top.rf.pending[21].commit();
      top.rf.pending[22].commit();
      top.rf.pending[23].commit();
      top.rf.pending[24].commit();
      top.rf.pending[25].commit();
      top.rf.pending[26].commit();
      top.rf.pending[27].commit();
      top.rf.pending[28].commit();
      top.rf.pending[29].commit();
      top.rf.pending[3].commit();
      top.rf.pending[30].commit();
      top.rf.pending[31].commit();
      top.rf.pending[4].commit();
      top.rf.pending[5].commit();
      top.rf.pending[6].commit();
      top.rf.pending[7].commit();
      top.rf.pending[8].commit();
      top.rf.pending[9].commit();
      top.rf.registers[0].commit();
      top.rf.registers[1].commit();
      top.rf.registers[10].commit();
      top.rf.registers[11].commit();
      top.rf.registers[12].commit();
      top.rf.registers[13].commit();
      top.rf.registers[14].commit();
      top.rf.registers[15].commit();
      top.rf.registers[16].commit();
      top.rf.registers[17].commit();
      top.rf.registers[18].commit();
      top.rf.registers[19].commit();
      top.rf.registers[2].commit();
      top.rf.registers[20].commit();
      top.rf.registers[21].commit();
      top.rf.registers[22].commit();
      top.rf.registers[23].commit();
      top.rf.registers[24].commit();
      top.rf.registers[25].commit();
      top.rf.registers[26].commit();
      top.rf.registers[27].commit();
      top.rf.registers[28].commit();
      top.rf.registers[29].commit();
      top.rf.registers[3].commit();
      top.rf.registers[30].commit();
      top.rf.registers[31].commit();
      top.rf.registers[4].commit();
      top.rf.registers[5].commit();
      top.rf.registers[6].commit();
      top.rf.registers[7].commit();
      top.rf.registers[8].commit();
      top.rf.registers[9].commit();
      top.writeback.last_pc.commit();
      top.writeback.retired.commit();
    }
    void tick() {
      begin_tick();
      top.writeback.try_commit(*this);
      top.memory_stage.try_access(*this);
      top.execute.try_execute(*this);
      top.decode_stage.try_read_operands(*this);
      top.fetch.try_fetch(*this);
      commit();
    }
    void trace(std::ostream &o,uint64_t tick) const { o<<"{\"tick\":"<<tick<<",\"fire\":{";
      o<<"\"top.writeback.commit#4\":"; json(o,fired_top_writeback_commit);
      o<<',';
      o<<"\"top.memory_stage.access#3\":"; json(o,fired_top_memory_stage_access);
      o<<',';
      o<<"\"top.execute.execute#2\":"; json(o,fired_top_execute_execute);
      o<<',';
      o<<"\"top.decode_stage.read_operands#1\":"; json(o,fired_top_decode_stage_read_operands);
      o<<',';
      o<<"\"top.fetch.fetch#0\":"; json(o,fired_top_fetch_fetch);
      o<<"},\"state\":{";
      o<<"\"top.ex_mem\":"; json(o,top.ex_mem);
      o<<',';
      o<<"\"top.fetch.pc\":"; json(o,top.fetch.pc.value);
      o<<',';
      o<<"\"top.fetch.rom\":"; json(o,top.fetch.rom.value);
      o<<',';
      o<<"\"top.id_ex\":"; json(o,top.id_ex);
      o<<',';
      o<<"\"top.if_id\":"; json(o,top.if_id);
      o<<',';
      o<<"\"top.mem_wb\":"; json(o,top.mem_wb);
      o<<',';
      o<<"\"top.memory.words\":"; json(o,top.memory.words.value);
      o<<',';
      o<<"\"top.rf.pending.0\":"; json(o,top.rf.pending[0].value);
      o<<',';
      o<<"\"top.rf.pending.1\":"; json(o,top.rf.pending[1].value);
      o<<',';
      o<<"\"top.rf.pending.10\":"; json(o,top.rf.pending[10].value);
      o<<',';
      o<<"\"top.rf.pending.11\":"; json(o,top.rf.pending[11].value);
      o<<',';
      o<<"\"top.rf.pending.12\":"; json(o,top.rf.pending[12].value);
      o<<',';
      o<<"\"top.rf.pending.13\":"; json(o,top.rf.pending[13].value);
      o<<',';
      o<<"\"top.rf.pending.14\":"; json(o,top.rf.pending[14].value);
      o<<',';
      o<<"\"top.rf.pending.15\":"; json(o,top.rf.pending[15].value);
      o<<',';
      o<<"\"top.rf.pending.16\":"; json(o,top.rf.pending[16].value);
      o<<',';
      o<<"\"top.rf.pending.17\":"; json(o,top.rf.pending[17].value);
      o<<',';
      o<<"\"top.rf.pending.18\":"; json(o,top.rf.pending[18].value);
      o<<',';
      o<<"\"top.rf.pending.19\":"; json(o,top.rf.pending[19].value);
      o<<',';
      o<<"\"top.rf.pending.2\":"; json(o,top.rf.pending[2].value);
      o<<',';
      o<<"\"top.rf.pending.20\":"; json(o,top.rf.pending[20].value);
      o<<',';
      o<<"\"top.rf.pending.21\":"; json(o,top.rf.pending[21].value);
      o<<',';
      o<<"\"top.rf.pending.22\":"; json(o,top.rf.pending[22].value);
      o<<',';
      o<<"\"top.rf.pending.23\":"; json(o,top.rf.pending[23].value);
      o<<',';
      o<<"\"top.rf.pending.24\":"; json(o,top.rf.pending[24].value);
      o<<',';
      o<<"\"top.rf.pending.25\":"; json(o,top.rf.pending[25].value);
      o<<',';
      o<<"\"top.rf.pending.26\":"; json(o,top.rf.pending[26].value);
      o<<',';
      o<<"\"top.rf.pending.27\":"; json(o,top.rf.pending[27].value);
      o<<',';
      o<<"\"top.rf.pending.28\":"; json(o,top.rf.pending[28].value);
      o<<',';
      o<<"\"top.rf.pending.29\":"; json(o,top.rf.pending[29].value);
      o<<',';
      o<<"\"top.rf.pending.3\":"; json(o,top.rf.pending[3].value);
      o<<',';
      o<<"\"top.rf.pending.30\":"; json(o,top.rf.pending[30].value);
      o<<',';
      o<<"\"top.rf.pending.31\":"; json(o,top.rf.pending[31].value);
      o<<',';
      o<<"\"top.rf.pending.4\":"; json(o,top.rf.pending[4].value);
      o<<',';
      o<<"\"top.rf.pending.5\":"; json(o,top.rf.pending[5].value);
      o<<',';
      o<<"\"top.rf.pending.6\":"; json(o,top.rf.pending[6].value);
      o<<',';
      o<<"\"top.rf.pending.7\":"; json(o,top.rf.pending[7].value);
      o<<',';
      o<<"\"top.rf.pending.8\":"; json(o,top.rf.pending[8].value);
      o<<',';
      o<<"\"top.rf.pending.9\":"; json(o,top.rf.pending[9].value);
      o<<',';
      o<<"\"top.rf.registers.0\":"; json(o,top.rf.registers[0].value);
      o<<',';
      o<<"\"top.rf.registers.1\":"; json(o,top.rf.registers[1].value);
      o<<',';
      o<<"\"top.rf.registers.10\":"; json(o,top.rf.registers[10].value);
      o<<',';
      o<<"\"top.rf.registers.11\":"; json(o,top.rf.registers[11].value);
      o<<',';
      o<<"\"top.rf.registers.12\":"; json(o,top.rf.registers[12].value);
      o<<',';
      o<<"\"top.rf.registers.13\":"; json(o,top.rf.registers[13].value);
      o<<',';
      o<<"\"top.rf.registers.14\":"; json(o,top.rf.registers[14].value);
      o<<',';
      o<<"\"top.rf.registers.15\":"; json(o,top.rf.registers[15].value);
      o<<',';
      o<<"\"top.rf.registers.16\":"; json(o,top.rf.registers[16].value);
      o<<',';
      o<<"\"top.rf.registers.17\":"; json(o,top.rf.registers[17].value);
      o<<',';
      o<<"\"top.rf.registers.18\":"; json(o,top.rf.registers[18].value);
      o<<',';
      o<<"\"top.rf.registers.19\":"; json(o,top.rf.registers[19].value);
      o<<',';
      o<<"\"top.rf.registers.2\":"; json(o,top.rf.registers[2].value);
      o<<',';
      o<<"\"top.rf.registers.20\":"; json(o,top.rf.registers[20].value);
      o<<',';
      o<<"\"top.rf.registers.21\":"; json(o,top.rf.registers[21].value);
      o<<',';
      o<<"\"top.rf.registers.22\":"; json(o,top.rf.registers[22].value);
      o<<',';
      o<<"\"top.rf.registers.23\":"; json(o,top.rf.registers[23].value);
      o<<',';
      o<<"\"top.rf.registers.24\":"; json(o,top.rf.registers[24].value);
      o<<',';
      o<<"\"top.rf.registers.25\":"; json(o,top.rf.registers[25].value);
      o<<',';
      o<<"\"top.rf.registers.26\":"; json(o,top.rf.registers[26].value);
      o<<',';
      o<<"\"top.rf.registers.27\":"; json(o,top.rf.registers[27].value);
      o<<',';
      o<<"\"top.rf.registers.28\":"; json(o,top.rf.registers[28].value);
      o<<',';
      o<<"\"top.rf.registers.29\":"; json(o,top.rf.registers[29].value);
      o<<',';
      o<<"\"top.rf.registers.3\":"; json(o,top.rf.registers[3].value);
      o<<',';
      o<<"\"top.rf.registers.30\":"; json(o,top.rf.registers[30].value);
      o<<',';
      o<<"\"top.rf.registers.31\":"; json(o,top.rf.registers[31].value);
      o<<',';
      o<<"\"top.rf.registers.4\":"; json(o,top.rf.registers[4].value);
      o<<',';
      o<<"\"top.rf.registers.5\":"; json(o,top.rf.registers[5].value);
      o<<',';
      o<<"\"top.rf.registers.6\":"; json(o,top.rf.registers[6].value);
      o<<',';
      o<<"\"top.rf.registers.7\":"; json(o,top.rf.registers[7].value);
      o<<',';
      o<<"\"top.rf.registers.8\":"; json(o,top.rf.registers[8].value);
      o<<',';
      o<<"\"top.rf.registers.9\":"; json(o,top.rf.registers[9].value);
      o<<',';
      o<<"\"top.writeback.last_pc\":"; json(o,top.writeback.last_pc.value);
      o<<',';
      o<<"\"top.writeback.retired\":"; json(o,top.writeback.retired.value);
      o<<"}}"; }
  };
} // namespace acir_generated
