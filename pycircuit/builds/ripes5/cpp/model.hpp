#pragma once
#include "runtime.hpp"
#include <set>
namespace acir_generated {
  using namespace acir_sim;
  using acir_sim::json;
  // ACPy payload src_isa_e31afbf3.Instruction
  struct Instruction {
    uint32_t opcode{};
    uint32_t funct3{};
    uint32_t funct7{};
    uint32_t rd{};
    uint32_t rs1{};
    uint32_t rs2{};
    uint32_t immediate{};
    bool writes{};
    bool load{};
    bool store{};
    bool operator==(const Instruction&) const = default;
  };
  inline void json(std::ostream &o,const Instruction &v) { o<<'[';
    json(o,v.opcode);
    o<<','; json(o,v.funct3);
    o<<','; json(o,v.funct7);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.rs1);
    o<<','; json(o,v.rs2);
    o<<','; json(o,v.immediate);
    o<<','; json(o,v.writes);
    o<<','; json(o,v.load);
    o<<','; json(o,v.store);
    o<<']'; }
  // ACPy payload src_isa_e31afbf3.Executed
  struct Executed {
    bool valid{};
    uint32_t pc{};
    uint32_t word{};
    uint32_t rd{};
    bool writes{};
    bool load{};
    bool store{};
    uint32_t alu{};
    uint32_t store_value{};
    uint32_t value{};
    bool stalled{};
    bool operator==(const Executed&) const = default;
  };
  inline void json(std::ostream &o,const Executed &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.pc);
    o<<','; json(o,v.word);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.writes);
    o<<','; json(o,v.load);
    o<<','; json(o,v.store);
    o<<','; json(o,v.alu);
    o<<','; json(o,v.store_value);
    o<<','; json(o,v.value);
    o<<','; json(o,v.stalled);
    o<<']'; }
  // ACPy payload src_isa_e31afbf3.Decoded
  struct Decoded {
    bool valid{};
    uint32_t pc{};
    uint32_t word{};
    Instruction instruction{};
    uint32_t lhs{};
    uint32_t rhs{};
    bool stalled{};
    bool operator==(const Decoded&) const = default;
  };
  inline void json(std::ostream &o,const Decoded &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.pc);
    o<<','; json(o,v.word);
    o<<','; json(o,v.instruction);
    o<<','; json(o,v.lhs);
    o<<','; json(o,v.rhs);
    o<<','; json(o,v.stalled);
    o<<']'; }
  // ACPy payload src_isa_e31afbf3.Fetched
  struct Fetched {
    bool valid{};
    uint32_t pc{};
    uint32_t word{};
    bool operator==(const Fetched&) const = default;
  };
  inline void json(std::ostream &o,const Fetched &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.pc);
    o<<','; json(o,v.word);
    o<<']'; }
  // ACPy payload src_isa_e31afbf3.Control
  struct Control {
    bool stall{};
    bool redirect{};
    uint32_t target{};
    uint32_t forward_a{};
    uint32_t forward_b{};
    bool operator==(const Control&) const = default;
  };
  inline void json(std::ostream &o,const Control &v) { o<<'[';
    json(o,v.stall);
    o<<','; json(o,v.redirect);
    o<<','; json(o,v.target);
    o<<','; json(o,v.forward_a);
    o<<','; json(o,v.forward_b);
    o<<']'; }
  // ACPy payload src_isa_e31afbf3.Completed
  struct Completed {
    bool valid{};
    uint32_t pc{};
    uint32_t word{};
    uint32_t rd{};
    bool writes{};
    uint32_t value{};
    bool stalled{};
    bool operator==(const Completed&) const = default;
  };
  inline void json(std::ostream &o,const Completed &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.pc);
    o<<','; json(o,v.word);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.writes);
    o<<','; json(o,v.value);
    o<<','; json(o,v.stalled);
    o<<']'; }
  // ACPy payload src_isa_e31afbf3.StoreEvent
  struct StoreEvent {
    bool valid{};
    uint32_t address{};
    uint32_t value{};
    bool operator==(const StoreEvent&) const = default;
  };
  inline void json(std::ostream &o,const StoreEvent &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.address);
    o<<','; json(o,v.value);
    o<<']'; }
  // ACPy payload src_isa_e31afbf3.Retirement
  struct Retirement {
    bool valid{};
    uint32_t pc{};
    uint32_t word{};
    uint32_t rd{};
    bool writes{};
    uint32_t value{};
    bool operator==(const Retirement&) const = default;
  };
  inline void json(std::ostream &o,const Retirement &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.pc);
    o<<','; json(o,v.word);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.writes);
    o<<','; json(o,v.value);
    o<<']'; }
  // ACPy payload src_isa_e31afbf3.ExecutionResult
  struct ExecutionResult {
    Executed packet{};
    bool redirect{};
    uint32_t target{};
    bool operator==(const ExecutionResult&) const = default;
  };
  inline void json(std::ostream &o,const ExecutionResult &v) { o<<'[';
    json(o,v.packet);
    o<<','; json(o,v.redirect);
    o<<','; json(o,v.target);
    o<<']'; }
  // ACPy payload src_isa_e31afbf3.ForwardedOperands
  struct ForwardedOperands {
    uint32_t lhs{};
    uint32_t rhs{};
    uint32_t source_a{};
    uint32_t source_b{};
    bool operator==(const ForwardedOperands&) const = default;
  };
  inline void json(std::ostream &o,const ForwardedOperands &v) { o<<'[';
    json(o,v.lhs);
    o<<','; json(o,v.rhs);
    o<<','; json(o,v.source_a);
    o<<','; json(o,v.source_b);
    o<<']'; }
  class Model {
  public:
    struct Demo {
      struct Ripes5 {
        struct Decode {
          Signal<Instruction> instruction{};
          // Rule top.cpu.decode_stage.decode_instruction#1
          template<class Context> void try_decode_instruction(Context &ctx) {
            if (!(true)) return;
            // Prepare every effect before publishing state operations.
            const Fetched if_id_value = ctx.top.cpu.if_id.read();
            const uint32_t word = if_id_value.word;
            const bool condition = (word!=0u);
            const uint32_t opcode = (word & 127u);
            const bool ready_5 = ((opcode!=111u) && ((opcode!=99u) && ((opcode!=35u) && ((opcode!=19u) && ((opcode!=3u) && (opcode!=103u))))));
            const bool check_fire = (condition && ((opcode!=55u) && ((opcode!=23u) && ready_5)));
            if (check_fire) {
              const bool condition_10 = (opcode==51u);
              acir_sim::check(condition_10);
            }
            const bool condition_11 = (opcode==55u);
            const bool condition_12 = (opcode==23u);
            const bool condition_13 = (opcode==51u);
            const bool condition_14 = (opcode==35u);
            const bool condition_15 = (opcode==99u);
            const Instruction instruction = Instruction{0u,0u,0u,0u,0u,0u,0u,false,false,false};
            const Instruction instruction_2 = Instruction{opcode,instruction.funct3,instruction.funct7,instruction.rd,instruction.rs1,instruction.rs2,instruction.immediate,instruction.writes,instruction.load,instruction.store};
            const uint32_t computed_value = (word >> 12u);
            const Instruction instruction_3 = Instruction{instruction_2.opcode,(computed_value & 7u),instruction_2.funct7,instruction_2.rd,instruction_2.rs1,instruction_2.rs2,instruction_2.immediate,instruction_2.writes,instruction_2.load,instruction_2.store};
            const uint32_t computed_value_3 = (word >> 25u);
            const Instruction instruction_4 = Instruction{instruction_3.opcode,instruction_3.funct3,computed_value_3,instruction_3.rd,instruction_3.rs1,instruction_3.rs2,instruction_3.immediate,instruction_3.writes,instruction_3.load,instruction_3.store};
            const uint32_t computed_value_4 = (word >> 7u);
            const uint32_t computed_value_5 = (computed_value_4 & 31u);
            const Instruction instruction_5 = Instruction{instruction_4.opcode,instruction_4.funct3,instruction_4.funct7,computed_value_5,instruction_4.rs1,instruction_4.rs2,instruction_4.immediate,instruction_4.writes,instruction_4.load,instruction_4.store};
            const Instruction instruction_6 = Instruction{instruction_5.opcode,instruction_5.funct3,instruction_5.funct7,instruction_5.rd,((word >> 15u) & 31u),instruction_5.rs2,instruction_5.immediate,instruction_5.writes,instruction_5.load,instruction_5.store};
            const uint32_t computed_value_8 = (word >> 20u);
            const Instruction instruction_7 = Instruction{instruction_6.opcode,instruction_6.funct3,instruction_6.funct7,instruction_6.rd,instruction_6.rs1,(computed_value_8 & 31u),instruction_6.immediate,instruction_6.writes,instruction_6.load,instruction_6.store};
            const Instruction instruction_8 = (((condition_13 || condition_14) || condition_15) ? instruction_7 : instruction_6);
            const bool condition_16 = (opcode==19u);
            const bool condition_17 = (opcode==3u);
            const bool condition_18 = (opcode==111u);
            const bool ready_35 = (((((condition_13 || condition_16) || condition_17) || condition_11) || condition_12) || condition_18);
            const bool condition_19 = (opcode==103u);
            const Instruction instruction_9 = Instruction{instruction_8.opcode,instruction_8.funct3,instruction_8.funct7,instruction_8.rd,instruction_8.rs1,instruction_8.rs2,instruction_8.immediate,(ready_35 || condition_19),instruction_8.load,instruction_8.store};
            const Instruction instruction_10 = Instruction{instruction_9.opcode,instruction_9.funct3,instruction_9.funct7,instruction_9.rd,instruction_9.rs1,instruction_9.rs2,instruction_9.immediate,instruction_9.writes,condition_17,instruction_9.store};
            const Instruction instruction_11 = Instruction{instruction_10.opcode,instruction_10.funct3,instruction_10.funct7,instruction_10.rd,instruction_10.rs1,instruction_10.rs2,instruction_10.immediate,instruction_10.writes,instruction_10.load,condition_14};
            const uint32_t opcode_10 = instruction_11.opcode;
            const uint32_t funct3_9 = instruction_11.funct3;
            const uint32_t funct7_9 = instruction_11.funct7;
            const uint32_t rd_9 = instruction_11.rd;
            const uint32_t rs2_9 = instruction_11.rs2;
            const bool ready_43 = instruction_11.writes;
            const bool ready_44 = instruction_11.load;
            const bool ready_45 = instruction_11.store;
            const Instruction instruction_12 = Instruction{opcode_10,funct3_9,funct7_9,rd_9,0u,rs2_9,instruction_11.immediate,ready_43,ready_44,ready_45};
            const uint32_t opcode_11 = instruction_12.opcode;
            const uint32_t funct3_10 = instruction_12.funct3;
            const uint32_t funct7_10 = instruction_12.funct7;
            const uint32_t rd_10 = instruction_12.rd;
            const uint32_t rs1_9 = instruction_12.rs1;
            const uint32_t rs2_10 = instruction_12.rs2;
            const bool ready_46 = instruction_12.writes;
            const bool ready_47 = instruction_12.load;
            const bool ready_48 = instruction_12.store;
            const Instruction instruction_13 = Instruction{opcode_11,funct3_10,funct7_10,rd_10,rs1_9,rs2_10,(word & 4294963200u),ready_46,ready_47,ready_48};
            const uint32_t computed_value_11 = (word >> 31u);
            const uint32_t computed_value_18 = (((computed_value_11 << 20u) | ((computed_value & 255u) << 12u)) | ((computed_value_8 & 1u) << 11u));
            const uint32_t immediate_11 = (computed_value_18 | (((word >> 21u) & 1023u) << 1u));
            const Instruction instruction_14 = Instruction{opcode_11,funct3_10,funct7_10,rd_10,rs1_9,rs2_10,((immediate_11 ^ 1048576u) - 1048576u),ready_46,ready_47,ready_48};
            const uint32_t rs1_10 = instruction_11.rs1;
            const uint32_t computed_value_30 = (((computed_value_11 << 12u) | ((computed_value_4 & 1u) << 11u)) | ((computed_value_3 & 63u) << 5u));
            const uint32_t immediate_12 = (computed_value_30 | (((word >> 8u) & 15u) << 1u));
            const Instruction instruction_15 = Instruction{opcode_10,funct3_9,funct7_9,rd_9,rs1_10,rs2_9,((immediate_12 ^ 4096u) - 4096u),ready_43,ready_44,ready_45};
            const uint32_t value = ((computed_value_3 << 5u) | computed_value_5);
            const Instruction instruction_16 = Instruction{opcode_10,funct3_9,funct7_9,rd_9,rs1_10,rs2_9,((value ^ 2048u) - 2048u),ready_43,ready_44,ready_45};
            const Instruction instruction_17 = Instruction{opcode_10,funct3_9,funct7_9,rd_9,rs1_10,rs2_9,((computed_value_8 ^ 2048u) - 2048u),ready_43,ready_44,ready_45};
            const Instruction instruction_18 = (((condition_16 || condition_17) || condition_19) ? instruction_17 : instruction_11);
            const Instruction instruction_19 = (condition_14 ? instruction_16 : instruction_18);
            const Instruction instruction_20 = (condition_15 ? instruction_15 : instruction_19);
            const Instruction instruction_21 = (condition_18 ? instruction_14 : instruction_20);
            const Instruction instruction_22 = ((condition_11 || condition_12) ? instruction_13 : instruction_21);
            const bool check_fire_2 = (condition && (instruction_22.load || instruction_22.store));
            if (check_fire_2) {
              const uint32_t funct3_11 = instruction_22.funct3;
              acir_sim::check((funct3_11==2u));
            }
            const bool check_fire_3 = (condition && condition_13);
            if (check_fire_3) {
              const uint32_t funct7_11 = instruction_22.funct7;
              const bool condition_21 = (funct7_11==0u);
              const bool condition_22 = (funct7_11==32u);
              const uint32_t funct3_12 = instruction_22.funct3;
              const bool condition_23 = (funct3_12==0u);
              const bool condition_24 = (funct3_12==5u);
              acir_sim::check((condition_21 || (condition_22 && (condition_23 || condition_24))));
            }
            const bool check_fire_4 = (condition && condition_19);
            if (check_fire_4) {
              const uint32_t funct3_13 = instruction_22.funct3;
              const bool condition_25 = (funct3_13==0u);
              acir_sim::check(condition_25);
            }
            const uint32_t funct3_14 = instruction_22.funct3;
            const bool check_fire_5 = (condition && (condition_16 && (funct3_14==1u)));
            if (check_fire_5) {
              const uint32_t funct7_12 = instruction_22.funct7;
              const bool condition_27 = (funct7_12==0u);
              acir_sim::check(condition_27);
            }
            const bool condition_28 = (funct3_14==5u);
            const bool check_fire_6 = (condition && (condition_16 && condition_28));
            if (check_fire_6) {
              const uint32_t funct7_13 = instruction_22.funct7;
              const bool condition_29 = (funct7_13==0u);
              const bool condition_30 = (funct7_13==32u);
              acir_sim::check((condition_29 || condition_30));
            }
            const Instruction instruction_23 = (condition ? instruction_22 : instruction);
            const Instruction set_value = instruction_23;
            ctx.top.cpu.decode_stage.instruction.write(set_value);
            ctx.fired_top_cpu_decode_stage_decode_instruction=true;
          }
          // Rule top.cpu.decode_stage.advance#2
          template<class Context> void try_advance(Context &ctx) {
            const bool ready = std::get<0>(ctx.compute_shared_computation_16());
            bool ready_2{};
            if (ready) {
              ready_2 = std::get<0>(ctx.compute_shared_computation_13());
            }
            bool ready_3{};
            if ((ready && (!ready_2))) {
              ready_3 = std::get<0>(ctx.compute_shared_computation_15());
            }
            const bool ready_4 = (ready_2 || ready_3);
            const bool ready_5 = (ready_4 || ready);
            const bool instruction_ready = ctx.top.cpu.decode_stage.instruction.valid;
            const bool ready_7 = (ready_5 && instruction_ready);
            const bool ready_8 = (ready_4 || ready_7);
            const bool ready_10 = (!ready_4);
            const Instruction instruction_value = ctx.top.cpu.decode_stage.instruction.read();
            const Instruction decoded = instruction_value;
            const uint32_t index = decoded.rs1;
            const bool condition = (index!=0u);
            const bool ready_11 = (ready_10 && condition);
            const bool ready_12 = (!ready_11);
            const bool ready_13 = (ready_12 || ready_7);
            const Completed mem_wb_value = ctx.top.cpu.mem_wb.read();
            const Completed completed = mem_wb_value;
            const bool ready_16 = completed.writes;
            const bool ready_18 = ((!ready_16) || instruction_ready);
            const bool ready_19 = (ready_13 && ready_18);
            const uint32_t rd = completed.rd;
            const bool ready_22 = (ready_16 && (rd==index));
            const bool ready_23 = (!ready_22);
            const bool ready_25 = (!(ready_11 && ready_23));
            const uint64_t computed_value = uint64_t(index);
            const bool ready_27 = (instruction_ready && (computed_value<uint64_t(32ULL)));
            const bool ready_30 = (((((ready && ready_5) && ready_8) && (ready_12 || ready_13)) && (ready_12 || ready_19)) && (ready_25 || ((ready_25 || ready_19) && ready_27)));
            const bool condition_4 = (index==0u);
            const bool ready_32 = (instruction_ready && (condition_4 || ready_18));
            const uint32_t index_2 = decoded.rs2;
            const bool condition_5 = (index_2!=0u);
            const bool ready_37 = (ready_10 && condition_5);
            const bool ready_38 = (!ready_37);
            const bool ready_39 = (ready_38 || ready_7);
            const bool ready_42 = (ready_39 && ready_18);
            const bool ready_44 = ((((ready_30 && (ready_4 || (ready_5 && ready_32))) && ready_8) && (ready_38 || ready_39)) && (ready_38 || ready_42));
            const bool ready_45 = (ready_16 && (rd==index_2));
            const bool ready_46 = (!ready_45);
            const bool ready_48 = (!(ready_37 && ready_46));
            const uint64_t computed_value_2 = uint64_t(index_2);
            const bool ready_50 = (instruction_ready && (computed_value_2<uint64_t(32ULL)));
            const bool condition_8 = (index_2==0u);
            const bool ready_55 = (instruction_ready && (condition_8 || ready_18));
            const bool ready_58 = ((ready_44 && (ready_48 || ((ready_48 || ready_42) && ready_50))) && (ready_4 || (ready_5 && ready_55)));
            if (!(ready_58)) return;
            // Prepare every effect before publishing state operations.
            const bool ready_59 = std::get<0>(ctx.compute_shared_computation_17());
            const bool check_fire = ready_59;
            if (check_fire) {
            }
            const bool check_fire_2 = ready_59;
            if (check_fire_2) {
            }
            const bool ready_61 = std::get<0>(ctx.compute_shared_computation_13());
            bool ready_62{};
            if ((!ready_61)) {
              ready_62 = std::get<0>(ctx.compute_shared_computation_15());
            }
            const bool ready_63 = (ready_61 || ready_62);
            bool ready_64{};
            if ((ready_61 || ready_62)) {
              ready_64 = std::get<0>(ctx.compute_shared_computation_12());
            }
            const bool ready_68 = ((instruction_ready && (condition ? (ready_18 && (ready_22 || ready_27)) : instruction_ready)) && ready_32);
            const bool ready_72 = ((instruction_ready && (condition_5 ? (ready_18 && (ready_45 || ready_50)) : instruction_ready)) && ready_55);
            const bool write_fire = (ready_63 ? ready_64 : (ready_68 && ready_72));
            Decoded write_value{};
            if (write_fire) {
              const Decoded bubble = Decoded{false,0u,0u,Instruction{},0u,0u,false};
              const Decoded bubble_2 = Decoded{bubble.valid,bubble.pc,bubble.word,bubble.instruction,bubble.lhs,bubble.rhs,ready_61};
              const Fetched if_id_value = ctx.top.cpu.if_id.read();
              const Fetched packet = if_id_value;
              const uint32_t value = completed.value;
              const uint32_t registers_value = ((computed_value < 32) ? ctx.top.cpu.rf.registers.at(computed_value).read() : uint32_t{});
              const uint32_t payload_2 = std::get<0>(ctx.compute_shared_computation());
              const uint32_t lhs_2 = (condition ? (ready_22 ? value : registers_value) : (condition_4 ? 0u : payload_2));
              const uint32_t registers_value_2 = ((computed_value_2 < 32) ? ctx.top.cpu.rf.registers.at(computed_value_2).read() : uint32_t{});
              const uint32_t rhs_2 = (condition_5 ? (ready_45 ? value : registers_value_2) : (condition_8 ? 0u : payload_2));
              write_value = (ready_63 ? bubble_2 : Decoded{packet.valid,packet.pc,packet.word,decoded,lhs_2,rhs_2,false});
            }
            if (write_fire) { ctx.top.cpu.id_ex.write(write_value); }
            ctx.fired_top_cpu_decode_stage_advance=true;
          }
        } decode_stage;
        struct Execute {
          // Rule top.cpu.execute.advance#3
          template<class Context> void try_advance(Context &ctx) {
            (void)(std::get<0>(ctx.compute_shared_computation_10()));
            const uint32_t opcode = std::get<0>(ctx.compute_opcode());
            const bool condition_2 = std::get<0>(ctx.compute_shared_computation_3());
            const bool condition_4 = std::get<0>(ctx.compute_shared_computation_5());
            const bool condition_5 = std::get<0>(ctx.compute_shared_computation_6());
            const bool condition_6 = std::get<0>(ctx.compute_shared_computation_7());
            const bool condition_7 = std::get<0>(ctx.compute_shared_computation_8());
            const bool condition_8 = std::get<0>(ctx.compute_shared_computation_9());
            const bool ready_6 = (condition_2 && ((opcode!=103u) && (condition_4 && (condition_5 && (condition_6 && (condition_7 && condition_8))))));
            if (((opcode!=111u) && ready_6)) {
              const Decoded id_ex_value = std::get<0>(ctx.compute_id_ex_view_3());
              const Decoded packet = id_ex_value;
              const Instruction executing = packet.instruction;
              const uint32_t function = executing.funct3;
              acir_sim::check((function==7u));
            }
            if (!(true)) return;
            // Prepare every effect before publishing state operations.
            const bool condition_10 = std::get<0>(ctx.compute_shared_computation_2());
            const bool condition_11 = std::get<0>(ctx.compute_shared_computation_4());
            const bool condition_path = (condition_11 ? false : (condition_4 && (condition_5 && (condition_6 && (condition_7 && condition_8)))));
            bool ready_8{};
            if ((condition_10 ? false : (condition_2 && condition_path))) {
              ready_8 = std::get<0>(ctx.compute_shared_computation_11());
            }
            const bool selected_value_5 = (condition_4 ? (condition_5 ? (condition_6 ? (condition_7 ? (condition_8 ? ready_8 : true) : true) : true) : true) : true);
            const bool write_fire = (condition_10 ? true : (condition_2 ? (condition_11 ? true : selected_value_5) : true));
            Executed write_value{};
            if (write_fire) {
              const ExecutionResult result = std::get<0>(ctx.compute_result());
              const Executed packet_3 = result.packet;
              write_value = packet_3;
            }
            if (write_fire) { ctx.top.cpu.ex_mem.write(write_value); }
            ctx.fired_top_cpu_execute_advance=true;
          }
        } execute;
        struct Fetch {
          Cell<uint32_t> pc{};
          Cell<std::array<uint32_t,256>> rom{};
          // Rule top.cpu.fetch.advance#0
          template<class Context> void try_advance(Context &ctx) {
            const bool branch_value_5 = std::get<0>(ctx.compute_shared_computation_12());
            bool branch_value_4{};
            if (branch_value_5) {
              const bool stall = std::get<0>(ctx.compute_shared_computation_13());
              bool branch_value_6{};
              if (stall) {
                branch_value_6 = true;
              } else {
                const bool branch_value_7 = std::get<0>(ctx.compute_shared_computation_16());
                branch_value_6 = branch_value_7;
              }
              branch_value_4 = branch_value_6;
            } else {
              branch_value_4 = false;
            }
            bool branch_value_3{};
            if (branch_value_4) {
              const bool branch_value_11 = std::get<0>(ctx.compute_shared_computation_14());
              bool branch_value_10{};
              if (branch_value_11) {
                const bool redirect = std::get<0>(ctx.compute_shared_computation_15());
                branch_value_10 = redirect;
              } else {
                branch_value_10 = false;
              }
              const bool branch_value_9 = (!branch_value_10);
              bool branch_value_8{};
              if (branch_value_9) {
                branch_value_8 = true;
              } else {
                bool branch_value_13{};
                if (branch_value_9) {
                  branch_value_13 = true;
                } else {
                  const bool branch_value_14 = std::get<0>(ctx.compute_shared_computation_16());
                  branch_value_13 = branch_value_14;
                }
                bool branch_value_12{};
                if (branch_value_13) {
                  branch_value_12 = branch_value_5;
                } else {
                  branch_value_12 = false;
                }
                branch_value_8 = branch_value_12;
              }
              branch_value_3 = branch_value_8;
            } else {
              branch_value_3 = false;
            }
            bool branch_value_2{};
            if (branch_value_3) {
              const bool stall_2 = std::get<0>(ctx.compute_shared_computation_13());
              bool branch_value_15{};
              if (stall_2) {
                branch_value_15 = true;
              } else {
                const bool branch_value_16 = std::get<0>(ctx.compute_shared_computation_16());
                branch_value_15 = branch_value_16;
              }
              branch_value_2 = branch_value_15;
            } else {
              branch_value_2 = false;
            }
            bool branch_value{};
            if (branch_value_2) {
              const bool branch_value_20 = std::get<0>(ctx.compute_shared_computation_14());
              bool branch_value_19{};
              if (branch_value_20) {
                const bool redirect_2 = std::get<0>(ctx.compute_shared_computation_15());
                bool branch_value_21{};
                if (redirect_2) {
                  branch_value_21 = false;
                } else {
                  branch_value_21 = branch_value_20;
                }
                branch_value_19 = branch_value_21;
              } else {
                branch_value_19 = false;
              }
              const bool branch_value_18 = (!branch_value_19);
              bool branch_value_17{};
              if (branch_value_18) {
                branch_value_17 = true;
              } else {
                bool branch_value_22{};
                if (branch_value_18) {
                  branch_value_22 = true;
                } else {
                  const bool branch_value_23 = std::get<0>(ctx.compute_shared_computation_16());
                  branch_value_22 = branch_value_23;
                }
                branch_value_17 = branch_value_22;
              }
              branch_value = branch_value_17;
            } else {
              branch_value = false;
            }
            if (!(branch_value)) return;
            // Prepare every effect before publishing state operations.
            const bool ready = std::get<0>(ctx.compute_shared_computation_14());
            bool ready_2{};
            if (ready) {
              ready_2 = std::get<0>(ctx.compute_shared_computation_15());
            }
            const bool ready_3 = (ready && ready_2);
            const bool write_fire = ready_3;
            uint32_t write_value{};
            if (write_fire) {
              const Control control = std::get<0>(ctx.compute_control_2());
              write_value = control.target;
            }
            const bool ready_5 = std::get<0>(ctx.compute_shared_computation_17());
            const bool check_fire = ready_5;
            if (check_fire) {
              const uint32_t pc_value = ctx.top.cpu.fetch.pc.read();
              acir_sim::check(((pc_value & 3u)==0u));
            }
            const bool write_fire_2 = ready_5;
            uint32_t write_value_2{};
            if (write_fire_2) {
              const uint32_t pc_value_2 = ctx.top.cpu.fetch.pc.read();
              write_value_2 = (pc_value_2 + 4u);
            }
            bool branch_value_24{};
            if (true) {
              bool branch_value_28{};
              if (true) {
                const bool stall_3 = std::get<0>(ctx.compute_shared_computation_13());
                bool branch_value_29{};
                if (stall_3) {
                  branch_value_29 = true;
                } else {
                  bool branch_value_30{};
                  if (true) {
                    const bool redirect_3 = std::get<0>(ctx.compute_shared_computation_15());
                    bool branch_value_32{};
                    if (redirect_3) {
                      branch_value_32 = false;
                    } else {
                      branch_value_32 = ready;
                    }
                    bool branch_value_31{};
                    if (branch_value_32) {
                      const uint32_t branch_value_36 = ctx.top.cpu.fetch.pc.read();
                      const uint32_t branch_value_35 = (branch_value_36 >> 2u);
                      const uint64_t branch_value_34 = uint64_t(branch_value_35);
                      const bool branch_value_33 = (branch_value_34<uint64_t(256ULL));
                      branch_value_31 = branch_value_33;
                    } else {
                      branch_value_31 = true;
                    }
                    branch_value_30 = branch_value_31;
                  } else {
                    branch_value_30 = false;
                  }
                  branch_value_29 = branch_value_30;
                }
                branch_value_28 = branch_value_29;
              } else {
                branch_value_28 = false;
              }
              bool branch_value_27{};
              if (branch_value_28) {
                const bool branch_value_37 = std::get<0>(ctx.compute_shared_computation_16());
                branch_value_27 = branch_value_37;
              } else {
                branch_value_27 = false;
              }
              bool branch_value_26{};
              if (branch_value_27) {
                bool branch_value_38{};
                if (ready) {
                  const bool redirect_4 = std::get<0>(ctx.compute_shared_computation_15());
                  bool branch_value_40{};
                  if (redirect_4) {
                    branch_value_40 = false;
                  } else {
                    branch_value_40 = ready;
                  }
                  bool branch_value_39{};
                  if (branch_value_40) {
                    branch_value_39 = true;
                  } else {
                    branch_value_39 = redirect_4;
                  }
                  branch_value_38 = branch_value_39;
                } else {
                  branch_value_38 = false;
                }
                branch_value_26 = branch_value_38;
              } else {
                branch_value_26 = false;
              }
              bool branch_value_25{};
              if (branch_value_26) {
                const bool branch_value_41 = std::get<0>(ctx.compute_shared_computation_16());
                branch_value_25 = branch_value_41;
              } else {
                branch_value_25 = false;
              }
              branch_value_24 = branch_value_25;
            } else {
              branch_value_24 = false;
            }
            const bool write_fire_3 = branch_value_24;
            Fetched write_value_3{};
            if (write_fire_3) {
              const bool ready_7 = std::get<0>(ctx.compute_shared_computation_13());
              const Fetched payload = Fetched{};
              bool ready_8{};
              if ((!ready_7)) {
                ready_8 = std::get<0>(ctx.compute_shared_computation_15());
              }
              const bool selected_value = (ready_8 ? false : ready);
              const uint32_t pc_value_3 = ctx.top.cpu.fetch.pc.read();
              const std::array<uint32_t,256> rom_value = ctx.top.cpu.fetch.rom.read();
              const uint64_t computed_value_4 = uint64_t((pc_value_3 >> 2u));
              const bool condition_2 = (computed_value_4<uint64_t(256ULL));
              const Fetched packet = Fetched{true,pc_value_3,rom_value.at((condition_2 ? computed_value_4 : uint64_t(0ULL)))};
              write_value_3 = (ready_7 ? payload : (selected_value ? packet : (ready_8 ? Fetched{false,0u,0u} : payload)));
            }
            if (write_fire) { ctx.top.cpu.fetch.pc.write(write_value); }
            if (write_fire_2) { ctx.top.cpu.fetch.pc.write(write_value_2); }
            if (write_fire_3) { ctx.top.cpu.if_id.write(write_value_3); }
            ctx.fired_top_cpu_fetch_advance=true;
          }
        } fetch;
        struct ForwardingUnit {
        } forwarding_unit;
        struct HazardUnit {
        } hazard_unit;
        struct Memory {
          std::array<Cell<uint32_t>,64> words{};
          Cell<StoreEvent> store_event{};
          // Rule top.cpu.memory.advance#4
          template<class Context> void try_advance(Context &ctx) {
            const Executed ex_mem_value = ctx.top.cpu.ex_mem.read();
            const Executed packet = ex_mem_value;
            const bool ready = packet.load;
            const uint32_t alu = packet.alu;
            const uint32_t computed_value = (alu - 4096u);
            const uint32_t index = (computed_value >> 2u);
            const uint64_t computed_value_2 = uint64_t(index);
            const bool condition = (computed_value_2<uint64_t(64ULL));
            const bool ready_2 = packet.store;
            if (!((ready ? condition : ((!ready_2) || condition)))) return;
            // Prepare every effect before publishing state operations.
            const bool ready_5 = (ready || ready_2);
            const bool check_fire = ready_5;
            if (check_fire) {
              acir_sim::check(((((alu & 3u)==0u) && (alu>=4096u)) && (computed_value<256u)));
            }
            const bool write_fire = ready_2;
            uint32_t write_value{};
            uint64_t resource_index{};
            if (write_fire) {
              const uint32_t store_value = packet.store_value;
              write_value = store_value;
              resource_index = computed_value_2;
            }
            const uint32_t store_value_2 = packet.store_value;
            const StoreEvent event = StoreEvent{true,alu,store_value_2};
            const StoreEvent event_2 = StoreEvent{false,0u,0u};
            const StoreEvent event_3 = (ready_2 ? event : event_2);
            const StoreEvent event_4 = (ready_5 ? event_3 : event_2);
            const StoreEvent write_value_2 = event_4;
            const uint32_t words_value = ((computed_value_2 < 64) ? ctx.top.cpu.memory.words.at(computed_value_2).read() : uint32_t{});
            const uint32_t value = words_value;
            const uint32_t value_2 = packet.value;
            const uint32_t value_3 = (ready ? value : value_2);
            const uint32_t value_4 = (ready_5 ? value_3 : value_2);
            const Completed write_value_3 = Completed{packet.valid,packet.pc,packet.word,packet.rd,packet.writes,value_4,packet.stalled};
            if (write_fire) { ctx.top.cpu.memory.words.at(resource_index).write(write_value); }
            ctx.top.cpu.memory.store_event.write(write_value_2);
            ctx.top.cpu.mem_wb.write(write_value_3);
            ctx.fired_top_cpu_memory_advance=true;
          }
        } memory;
        struct RegisterFile {
          std::array<Cell<uint32_t>,32> registers{};
        } rf;
        struct Writeback {
          Cell<uint32_t> retired{};
          Cell<Retirement> retirement{};
          // Rule top.cpu.writeback.advance#5
          template<class Context> void try_advance(Context &ctx) {
            const Completed mem_wb_value = ctx.top.cpu.mem_wb.read();
            const Completed packet = mem_wb_value;
            const bool ready = packet.writes;
            const uint32_t index = packet.rd;
            const uint64_t computed_value = uint64_t(index);
            if (!(((!ready) || ((index==0u) || (computed_value<uint64_t(32ULL)))))) return;
            // Prepare every effect before publishing state operations.
            const bool ready_5 = (ready && (index!=0u));
            const bool write_fire = ready_5;
            uint32_t write_value{};
            uint64_t resource_index{};
            if (write_fire) {
              const uint32_t value = packet.value;
              write_value = value;
              resource_index = computed_value;
            }
            const uint32_t pc = packet.pc;
            const uint32_t code_size = 84u;
            const bool ready_8 = (packet.valid && (pc<code_size));
            const bool write_fire_2 = ready_8;
            uint32_t write_value_2{};
            if (write_fire_2) {
              const uint32_t retired_value = ctx.top.cpu.writeback.retired.read();
              write_value_2 = (retired_value + 1u);
            }
            const uint32_t value_2 = packet.value;
            const Retirement event = Retirement{true,pc,packet.word,index,ready_5,value_2};
            const Retirement event_2 = Retirement{false,0u,0u,0u,false,0u};
            const Retirement event_3 = (ready_8 ? event : event_2);
            const Retirement write_value_3 = event_3;
            if (write_fire) { ctx.top.cpu.rf.registers.at(resource_index).write(write_value); }
            if (write_fire_2) { ctx.top.cpu.writeback.retired.write(write_value_2); }
            ctx.top.cpu.writeback.retirement.write(write_value_3);
            ctx.fired_top_cpu_writeback_advance=true;
          }
        } writeback;
        Cell<Executed> ex_mem{};
        Cell<Decoded> id_ex{};
        Cell<Fetched> if_id{};
        Cell<Control> last_control{};
        Cell<Completed> mem_wb{};
        // Rule top.cpu.observe#6
        template<class Context> void try_observe(Context &ctx) {
          if (!(std::get<0>(ctx.compute_shared_computation_12()))) return;
          // Prepare every effect before publishing state operations.
          const Control write_value = std::get<0>(ctx.compute_control());
          ctx.top.cpu.last_control.write(write_value);
          ctx.fired_top_cpu_observe=true;
        }
      } cpu;
    } top;
    std::optional<std::tuple<Instruction,bool>> compute_instruction_view_cache;
    std::optional<std::tuple<Decoded,bool>> compute_id_ex_view_cache;
    std::optional<std::tuple<Decoded,bool>> compute_id_ex_view_2_cache;
    std::optional<std::tuple<Executed,bool>> compute_ex_mem_view_cache;
    std::optional<std::tuple<Completed,bool>> compute_mem_wb_view_cache;
    std::optional<std::tuple<uint32_t>> compute_shared_computation_cache;
    std::optional<std::tuple<Decoded,bool>> compute_id_ex_view_3_cache;
    std::optional<std::tuple<uint32_t>> compute_opcode_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_2_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_3_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_4_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_5_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_6_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_7_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_8_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_9_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_10_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_11_cache;
    std::optional<std::tuple<ExecutionResult>> compute_result_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_12_cache;
    std::optional<std::tuple<Control>> compute_control_cache;
    std::optional<std::tuple<Control>> compute_control_2_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_13_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_14_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_15_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_16_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_17_cache;
    bool fired_top_cpu_decode_stage_decode_instruction=false;
    bool fired_top_cpu_fetch_advance=false;
    bool fired_top_cpu_decode_stage_advance=false;
    bool fired_top_cpu_execute_advance=false;
    bool fired_top_cpu_memory_advance=false;
    bool fired_top_cpu_writeback_advance=false;
    bool fired_top_cpu_observe=false;
    Model() { reset(); }
    void reset() {
      top.cpu.decode_stage.instruction={};
      top.cpu.decode_stage.instruction.value=Instruction{0u,0u,0u,0u,0u,0u,0u,false,false,false};
      top.cpu.ex_mem={};
      top.cpu.ex_mem.value=Executed{false,0u,0u,0u,false,false,false,0u,0u,0u,false};
      top.cpu.fetch.pc={};
      top.cpu.fetch.pc.value=0u;
      top.cpu.fetch.rom={};
      top.cpu.fetch.rom.value=std::array<uint32_t,256>{5431u,17825939u,1384483u,336131u,1115747u,335907u,103811603u,1114515u,2201187u,335907u,103811731u,336387u,1185379u,1180307u,8388719u,335907u,5579299u,2146435091u,111u,19u,19u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u};
      top.cpu.id_ex={};
      top.cpu.id_ex.value=Decoded{false,0u,0u,Instruction{0u,0u,0u,0u,0u,0u,0u,false,false,false},0u,0u,false};
      top.cpu.if_id={};
      top.cpu.if_id.value=Fetched{false,0u,0u};
      top.cpu.last_control={};
      top.cpu.last_control.value=Control{false,false,0u,0u,0u};
      top.cpu.mem_wb={};
      top.cpu.mem_wb.value=Completed{false,0u,0u,0u,false,0u,false};
      top.cpu.memory.store_event={};
      top.cpu.memory.store_event.value=StoreEvent{false,0u,0u};
      top.cpu.memory.words[0]={};
      top.cpu.memory.words[0].value=0u;
      top.cpu.memory.words[1]={};
      top.cpu.memory.words[1].value=0u;
      top.cpu.memory.words[10]={};
      top.cpu.memory.words[10].value=0u;
      top.cpu.memory.words[11]={};
      top.cpu.memory.words[11].value=0u;
      top.cpu.memory.words[12]={};
      top.cpu.memory.words[12].value=0u;
      top.cpu.memory.words[13]={};
      top.cpu.memory.words[13].value=0u;
      top.cpu.memory.words[14]={};
      top.cpu.memory.words[14].value=0u;
      top.cpu.memory.words[15]={};
      top.cpu.memory.words[15].value=0u;
      top.cpu.memory.words[16]={};
      top.cpu.memory.words[16].value=0u;
      top.cpu.memory.words[17]={};
      top.cpu.memory.words[17].value=0u;
      top.cpu.memory.words[18]={};
      top.cpu.memory.words[18].value=0u;
      top.cpu.memory.words[19]={};
      top.cpu.memory.words[19].value=0u;
      top.cpu.memory.words[2]={};
      top.cpu.memory.words[2].value=0u;
      top.cpu.memory.words[20]={};
      top.cpu.memory.words[20].value=0u;
      top.cpu.memory.words[21]={};
      top.cpu.memory.words[21].value=0u;
      top.cpu.memory.words[22]={};
      top.cpu.memory.words[22].value=0u;
      top.cpu.memory.words[23]={};
      top.cpu.memory.words[23].value=0u;
      top.cpu.memory.words[24]={};
      top.cpu.memory.words[24].value=0u;
      top.cpu.memory.words[25]={};
      top.cpu.memory.words[25].value=0u;
      top.cpu.memory.words[26]={};
      top.cpu.memory.words[26].value=0u;
      top.cpu.memory.words[27]={};
      top.cpu.memory.words[27].value=0u;
      top.cpu.memory.words[28]={};
      top.cpu.memory.words[28].value=0u;
      top.cpu.memory.words[29]={};
      top.cpu.memory.words[29].value=0u;
      top.cpu.memory.words[3]={};
      top.cpu.memory.words[3].value=0u;
      top.cpu.memory.words[30]={};
      top.cpu.memory.words[30].value=0u;
      top.cpu.memory.words[31]={};
      top.cpu.memory.words[31].value=0u;
      top.cpu.memory.words[32]={};
      top.cpu.memory.words[32].value=0u;
      top.cpu.memory.words[33]={};
      top.cpu.memory.words[33].value=0u;
      top.cpu.memory.words[34]={};
      top.cpu.memory.words[34].value=0u;
      top.cpu.memory.words[35]={};
      top.cpu.memory.words[35].value=0u;
      top.cpu.memory.words[36]={};
      top.cpu.memory.words[36].value=0u;
      top.cpu.memory.words[37]={};
      top.cpu.memory.words[37].value=0u;
      top.cpu.memory.words[38]={};
      top.cpu.memory.words[38].value=0u;
      top.cpu.memory.words[39]={};
      top.cpu.memory.words[39].value=0u;
      top.cpu.memory.words[4]={};
      top.cpu.memory.words[4].value=0u;
      top.cpu.memory.words[40]={};
      top.cpu.memory.words[40].value=0u;
      top.cpu.memory.words[41]={};
      top.cpu.memory.words[41].value=0u;
      top.cpu.memory.words[42]={};
      top.cpu.memory.words[42].value=0u;
      top.cpu.memory.words[43]={};
      top.cpu.memory.words[43].value=0u;
      top.cpu.memory.words[44]={};
      top.cpu.memory.words[44].value=0u;
      top.cpu.memory.words[45]={};
      top.cpu.memory.words[45].value=0u;
      top.cpu.memory.words[46]={};
      top.cpu.memory.words[46].value=0u;
      top.cpu.memory.words[47]={};
      top.cpu.memory.words[47].value=0u;
      top.cpu.memory.words[48]={};
      top.cpu.memory.words[48].value=0u;
      top.cpu.memory.words[49]={};
      top.cpu.memory.words[49].value=0u;
      top.cpu.memory.words[5]={};
      top.cpu.memory.words[5].value=0u;
      top.cpu.memory.words[50]={};
      top.cpu.memory.words[50].value=0u;
      top.cpu.memory.words[51]={};
      top.cpu.memory.words[51].value=0u;
      top.cpu.memory.words[52]={};
      top.cpu.memory.words[52].value=0u;
      top.cpu.memory.words[53]={};
      top.cpu.memory.words[53].value=0u;
      top.cpu.memory.words[54]={};
      top.cpu.memory.words[54].value=0u;
      top.cpu.memory.words[55]={};
      top.cpu.memory.words[55].value=0u;
      top.cpu.memory.words[56]={};
      top.cpu.memory.words[56].value=0u;
      top.cpu.memory.words[57]={};
      top.cpu.memory.words[57].value=0u;
      top.cpu.memory.words[58]={};
      top.cpu.memory.words[58].value=0u;
      top.cpu.memory.words[59]={};
      top.cpu.memory.words[59].value=0u;
      top.cpu.memory.words[6]={};
      top.cpu.memory.words[6].value=0u;
      top.cpu.memory.words[60]={};
      top.cpu.memory.words[60].value=0u;
      top.cpu.memory.words[61]={};
      top.cpu.memory.words[61].value=0u;
      top.cpu.memory.words[62]={};
      top.cpu.memory.words[62].value=0u;
      top.cpu.memory.words[63]={};
      top.cpu.memory.words[63].value=0u;
      top.cpu.memory.words[7]={};
      top.cpu.memory.words[7].value=0u;
      top.cpu.memory.words[8]={};
      top.cpu.memory.words[8].value=0u;
      top.cpu.memory.words[9]={};
      top.cpu.memory.words[9].value=0u;
      top.cpu.rf.registers[0]={};
      top.cpu.rf.registers[0].value=0u;
      top.cpu.rf.registers[1]={};
      top.cpu.rf.registers[1].value=0u;
      top.cpu.rf.registers[10]={};
      top.cpu.rf.registers[10].value=0u;
      top.cpu.rf.registers[11]={};
      top.cpu.rf.registers[11].value=0u;
      top.cpu.rf.registers[12]={};
      top.cpu.rf.registers[12].value=0u;
      top.cpu.rf.registers[13]={};
      top.cpu.rf.registers[13].value=0u;
      top.cpu.rf.registers[14]={};
      top.cpu.rf.registers[14].value=0u;
      top.cpu.rf.registers[15]={};
      top.cpu.rf.registers[15].value=0u;
      top.cpu.rf.registers[16]={};
      top.cpu.rf.registers[16].value=0u;
      top.cpu.rf.registers[17]={};
      top.cpu.rf.registers[17].value=0u;
      top.cpu.rf.registers[18]={};
      top.cpu.rf.registers[18].value=0u;
      top.cpu.rf.registers[19]={};
      top.cpu.rf.registers[19].value=0u;
      top.cpu.rf.registers[2]={};
      top.cpu.rf.registers[2].value=0u;
      top.cpu.rf.registers[20]={};
      top.cpu.rf.registers[20].value=0u;
      top.cpu.rf.registers[21]={};
      top.cpu.rf.registers[21].value=0u;
      top.cpu.rf.registers[22]={};
      top.cpu.rf.registers[22].value=0u;
      top.cpu.rf.registers[23]={};
      top.cpu.rf.registers[23].value=0u;
      top.cpu.rf.registers[24]={};
      top.cpu.rf.registers[24].value=0u;
      top.cpu.rf.registers[25]={};
      top.cpu.rf.registers[25].value=0u;
      top.cpu.rf.registers[26]={};
      top.cpu.rf.registers[26].value=0u;
      top.cpu.rf.registers[27]={};
      top.cpu.rf.registers[27].value=0u;
      top.cpu.rf.registers[28]={};
      top.cpu.rf.registers[28].value=0u;
      top.cpu.rf.registers[29]={};
      top.cpu.rf.registers[29].value=0u;
      top.cpu.rf.registers[3]={};
      top.cpu.rf.registers[3].value=0u;
      top.cpu.rf.registers[30]={};
      top.cpu.rf.registers[30].value=0u;
      top.cpu.rf.registers[31]={};
      top.cpu.rf.registers[31].value=0u;
      top.cpu.rf.registers[4]={};
      top.cpu.rf.registers[4].value=0u;
      top.cpu.rf.registers[5]={};
      top.cpu.rf.registers[5].value=0u;
      top.cpu.rf.registers[6]={};
      top.cpu.rf.registers[6].value=0u;
      top.cpu.rf.registers[7]={};
      top.cpu.rf.registers[7].value=0u;
      top.cpu.rf.registers[8]={};
      top.cpu.rf.registers[8].value=0u;
      top.cpu.rf.registers[9]={};
      top.cpu.rf.registers[9].value=0u;
      top.cpu.writeback.retired={};
      top.cpu.writeback.retired.value=0u;
      top.cpu.writeback.retirement={};
      top.cpu.writeback.retirement.value=Retirement{false,0u,0u,0u,false,0u};
      begin_tick();
    }
    void begin_tick() {
      top.cpu.decode_stage.instruction.begin();
      top.cpu.ex_mem.begin();
      top.cpu.fetch.pc.begin();
      top.cpu.fetch.rom.begin();
      top.cpu.id_ex.begin();
      top.cpu.if_id.begin();
      top.cpu.last_control.begin();
      top.cpu.mem_wb.begin();
      top.cpu.memory.store_event.begin();
      top.cpu.memory.words[0].begin();
      top.cpu.memory.words[1].begin();
      top.cpu.memory.words[10].begin();
      top.cpu.memory.words[11].begin();
      top.cpu.memory.words[12].begin();
      top.cpu.memory.words[13].begin();
      top.cpu.memory.words[14].begin();
      top.cpu.memory.words[15].begin();
      top.cpu.memory.words[16].begin();
      top.cpu.memory.words[17].begin();
      top.cpu.memory.words[18].begin();
      top.cpu.memory.words[19].begin();
      top.cpu.memory.words[2].begin();
      top.cpu.memory.words[20].begin();
      top.cpu.memory.words[21].begin();
      top.cpu.memory.words[22].begin();
      top.cpu.memory.words[23].begin();
      top.cpu.memory.words[24].begin();
      top.cpu.memory.words[25].begin();
      top.cpu.memory.words[26].begin();
      top.cpu.memory.words[27].begin();
      top.cpu.memory.words[28].begin();
      top.cpu.memory.words[29].begin();
      top.cpu.memory.words[3].begin();
      top.cpu.memory.words[30].begin();
      top.cpu.memory.words[31].begin();
      top.cpu.memory.words[32].begin();
      top.cpu.memory.words[33].begin();
      top.cpu.memory.words[34].begin();
      top.cpu.memory.words[35].begin();
      top.cpu.memory.words[36].begin();
      top.cpu.memory.words[37].begin();
      top.cpu.memory.words[38].begin();
      top.cpu.memory.words[39].begin();
      top.cpu.memory.words[4].begin();
      top.cpu.memory.words[40].begin();
      top.cpu.memory.words[41].begin();
      top.cpu.memory.words[42].begin();
      top.cpu.memory.words[43].begin();
      top.cpu.memory.words[44].begin();
      top.cpu.memory.words[45].begin();
      top.cpu.memory.words[46].begin();
      top.cpu.memory.words[47].begin();
      top.cpu.memory.words[48].begin();
      top.cpu.memory.words[49].begin();
      top.cpu.memory.words[5].begin();
      top.cpu.memory.words[50].begin();
      top.cpu.memory.words[51].begin();
      top.cpu.memory.words[52].begin();
      top.cpu.memory.words[53].begin();
      top.cpu.memory.words[54].begin();
      top.cpu.memory.words[55].begin();
      top.cpu.memory.words[56].begin();
      top.cpu.memory.words[57].begin();
      top.cpu.memory.words[58].begin();
      top.cpu.memory.words[59].begin();
      top.cpu.memory.words[6].begin();
      top.cpu.memory.words[60].begin();
      top.cpu.memory.words[61].begin();
      top.cpu.memory.words[62].begin();
      top.cpu.memory.words[63].begin();
      top.cpu.memory.words[7].begin();
      top.cpu.memory.words[8].begin();
      top.cpu.memory.words[9].begin();
      top.cpu.rf.registers[0].begin();
      top.cpu.rf.registers[1].begin();
      top.cpu.rf.registers[10].begin();
      top.cpu.rf.registers[11].begin();
      top.cpu.rf.registers[12].begin();
      top.cpu.rf.registers[13].begin();
      top.cpu.rf.registers[14].begin();
      top.cpu.rf.registers[15].begin();
      top.cpu.rf.registers[16].begin();
      top.cpu.rf.registers[17].begin();
      top.cpu.rf.registers[18].begin();
      top.cpu.rf.registers[19].begin();
      top.cpu.rf.registers[2].begin();
      top.cpu.rf.registers[20].begin();
      top.cpu.rf.registers[21].begin();
      top.cpu.rf.registers[22].begin();
      top.cpu.rf.registers[23].begin();
      top.cpu.rf.registers[24].begin();
      top.cpu.rf.registers[25].begin();
      top.cpu.rf.registers[26].begin();
      top.cpu.rf.registers[27].begin();
      top.cpu.rf.registers[28].begin();
      top.cpu.rf.registers[29].begin();
      top.cpu.rf.registers[3].begin();
      top.cpu.rf.registers[30].begin();
      top.cpu.rf.registers[31].begin();
      top.cpu.rf.registers[4].begin();
      top.cpu.rf.registers[5].begin();
      top.cpu.rf.registers[6].begin();
      top.cpu.rf.registers[7].begin();
      top.cpu.rf.registers[8].begin();
      top.cpu.rf.registers[9].begin();
      top.cpu.writeback.retired.begin();
      top.cpu.writeback.retirement.begin();
      compute_instruction_view_cache.reset();
      compute_id_ex_view_cache.reset();
      compute_id_ex_view_2_cache.reset();
      compute_ex_mem_view_cache.reset();
      compute_mem_wb_view_cache.reset();
      compute_shared_computation_cache.reset();
      compute_id_ex_view_3_cache.reset();
      compute_opcode_cache.reset();
      compute_shared_computation_2_cache.reset();
      compute_shared_computation_3_cache.reset();
      compute_shared_computation_4_cache.reset();
      compute_shared_computation_5_cache.reset();
      compute_shared_computation_6_cache.reset();
      compute_shared_computation_7_cache.reset();
      compute_shared_computation_8_cache.reset();
      compute_shared_computation_9_cache.reset();
      compute_shared_computation_10_cache.reset();
      compute_shared_computation_11_cache.reset();
      compute_result_cache.reset();
      compute_shared_computation_12_cache.reset();
      compute_control_cache.reset();
      compute_control_2_cache.reset();
      compute_shared_computation_13_cache.reset();
      compute_shared_computation_14_cache.reset();
      compute_shared_computation_15_cache.reset();
      compute_shared_computation_16_cache.reset();
      compute_shared_computation_17_cache.reset();
      fired_top_cpu_decode_stage_decode_instruction=false;
      fired_top_cpu_fetch_advance=false;
      fired_top_cpu_decode_stage_advance=false;
      fired_top_cpu_execute_advance=false;
      fired_top_cpu_memory_advance=false;
      fired_top_cpu_writeback_advance=false;
      fired_top_cpu_observe=false;
    }
    void commit() {
      top.cpu.decode_stage.instruction.commit();
      top.cpu.ex_mem.commit();
      top.cpu.fetch.pc.commit();
      top.cpu.fetch.rom.commit();
      top.cpu.id_ex.commit();
      top.cpu.if_id.commit();
      top.cpu.last_control.commit();
      top.cpu.mem_wb.commit();
      top.cpu.memory.store_event.commit();
      top.cpu.memory.words[0].commit();
      top.cpu.memory.words[1].commit();
      top.cpu.memory.words[10].commit();
      top.cpu.memory.words[11].commit();
      top.cpu.memory.words[12].commit();
      top.cpu.memory.words[13].commit();
      top.cpu.memory.words[14].commit();
      top.cpu.memory.words[15].commit();
      top.cpu.memory.words[16].commit();
      top.cpu.memory.words[17].commit();
      top.cpu.memory.words[18].commit();
      top.cpu.memory.words[19].commit();
      top.cpu.memory.words[2].commit();
      top.cpu.memory.words[20].commit();
      top.cpu.memory.words[21].commit();
      top.cpu.memory.words[22].commit();
      top.cpu.memory.words[23].commit();
      top.cpu.memory.words[24].commit();
      top.cpu.memory.words[25].commit();
      top.cpu.memory.words[26].commit();
      top.cpu.memory.words[27].commit();
      top.cpu.memory.words[28].commit();
      top.cpu.memory.words[29].commit();
      top.cpu.memory.words[3].commit();
      top.cpu.memory.words[30].commit();
      top.cpu.memory.words[31].commit();
      top.cpu.memory.words[32].commit();
      top.cpu.memory.words[33].commit();
      top.cpu.memory.words[34].commit();
      top.cpu.memory.words[35].commit();
      top.cpu.memory.words[36].commit();
      top.cpu.memory.words[37].commit();
      top.cpu.memory.words[38].commit();
      top.cpu.memory.words[39].commit();
      top.cpu.memory.words[4].commit();
      top.cpu.memory.words[40].commit();
      top.cpu.memory.words[41].commit();
      top.cpu.memory.words[42].commit();
      top.cpu.memory.words[43].commit();
      top.cpu.memory.words[44].commit();
      top.cpu.memory.words[45].commit();
      top.cpu.memory.words[46].commit();
      top.cpu.memory.words[47].commit();
      top.cpu.memory.words[48].commit();
      top.cpu.memory.words[49].commit();
      top.cpu.memory.words[5].commit();
      top.cpu.memory.words[50].commit();
      top.cpu.memory.words[51].commit();
      top.cpu.memory.words[52].commit();
      top.cpu.memory.words[53].commit();
      top.cpu.memory.words[54].commit();
      top.cpu.memory.words[55].commit();
      top.cpu.memory.words[56].commit();
      top.cpu.memory.words[57].commit();
      top.cpu.memory.words[58].commit();
      top.cpu.memory.words[59].commit();
      top.cpu.memory.words[6].commit();
      top.cpu.memory.words[60].commit();
      top.cpu.memory.words[61].commit();
      top.cpu.memory.words[62].commit();
      top.cpu.memory.words[63].commit();
      top.cpu.memory.words[7].commit();
      top.cpu.memory.words[8].commit();
      top.cpu.memory.words[9].commit();
      top.cpu.rf.registers[0].commit();
      top.cpu.rf.registers[1].commit();
      top.cpu.rf.registers[10].commit();
      top.cpu.rf.registers[11].commit();
      top.cpu.rf.registers[12].commit();
      top.cpu.rf.registers[13].commit();
      top.cpu.rf.registers[14].commit();
      top.cpu.rf.registers[15].commit();
      top.cpu.rf.registers[16].commit();
      top.cpu.rf.registers[17].commit();
      top.cpu.rf.registers[18].commit();
      top.cpu.rf.registers[19].commit();
      top.cpu.rf.registers[2].commit();
      top.cpu.rf.registers[20].commit();
      top.cpu.rf.registers[21].commit();
      top.cpu.rf.registers[22].commit();
      top.cpu.rf.registers[23].commit();
      top.cpu.rf.registers[24].commit();
      top.cpu.rf.registers[25].commit();
      top.cpu.rf.registers[26].commit();
      top.cpu.rf.registers[27].commit();
      top.cpu.rf.registers[28].commit();
      top.cpu.rf.registers[29].commit();
      top.cpu.rf.registers[3].commit();
      top.cpu.rf.registers[30].commit();
      top.cpu.rf.registers[31].commit();
      top.cpu.rf.registers[4].commit();
      top.cpu.rf.registers[5].commit();
      top.cpu.rf.registers[6].commit();
      top.cpu.rf.registers[7].commit();
      top.cpu.rf.registers[8].commit();
      top.cpu.rf.registers[9].commit();
      top.cpu.writeback.retired.commit();
      top.cpu.writeback.retirement.commit();
    }
    void tick() {
      begin_tick();
      top.cpu.decode_stage.try_decode_instruction(*this);
      top.cpu.fetch.try_advance(*this);
      top.cpu.decode_stage.try_advance(*this);
      top.cpu.execute.try_advance(*this);
      top.cpu.memory.try_advance(*this);
      top.cpu.writeback.try_advance(*this);
      top.cpu.try_observe(*this);
      commit();
    }
    void trace(std::ostream &o,uint64_t tick) const { o<<"{\"tick\":"<<tick<<",\"fire\":{";
      o<<"\"top.cpu.decode_stage.decode_instruction#1\":"; json(o,fired_top_cpu_decode_stage_decode_instruction);
      o<<',';
      o<<"\"top.cpu.fetch.advance#0\":"; json(o,fired_top_cpu_fetch_advance);
      o<<',';
      o<<"\"top.cpu.decode_stage.advance#2\":"; json(o,fired_top_cpu_decode_stage_advance);
      o<<',';
      o<<"\"top.cpu.execute.advance#3\":"; json(o,fired_top_cpu_execute_advance);
      o<<',';
      o<<"\"top.cpu.memory.advance#4\":"; json(o,fired_top_cpu_memory_advance);
      o<<',';
      o<<"\"top.cpu.writeback.advance#5\":"; json(o,fired_top_cpu_writeback_advance);
      o<<',';
      o<<"\"top.cpu.observe#6\":"; json(o,fired_top_cpu_observe);
      o<<"},\"state\":{";
      o<<"\"top.cpu.ex_mem\":"; json(o,top.cpu.ex_mem.value);
      o<<',';
      o<<"\"top.cpu.fetch.pc\":"; json(o,top.cpu.fetch.pc.value);
      o<<',';
      o<<"\"top.cpu.fetch.rom\":"; json(o,top.cpu.fetch.rom.value);
      o<<',';
      o<<"\"top.cpu.id_ex\":"; json(o,top.cpu.id_ex.value);
      o<<',';
      o<<"\"top.cpu.if_id\":"; json(o,top.cpu.if_id.value);
      o<<',';
      o<<"\"top.cpu.last_control\":"; json(o,top.cpu.last_control.value);
      o<<',';
      o<<"\"top.cpu.mem_wb\":"; json(o,top.cpu.mem_wb.value);
      o<<',';
      o<<"\"top.cpu.memory.store_event\":"; json(o,top.cpu.memory.store_event.value);
      o<<',';
      o<<"\"top.cpu.memory.words.0\":"; json(o,top.cpu.memory.words[0].value);
      o<<',';
      o<<"\"top.cpu.memory.words.1\":"; json(o,top.cpu.memory.words[1].value);
      o<<',';
      o<<"\"top.cpu.memory.words.10\":"; json(o,top.cpu.memory.words[10].value);
      o<<',';
      o<<"\"top.cpu.memory.words.11\":"; json(o,top.cpu.memory.words[11].value);
      o<<',';
      o<<"\"top.cpu.memory.words.12\":"; json(o,top.cpu.memory.words[12].value);
      o<<',';
      o<<"\"top.cpu.memory.words.13\":"; json(o,top.cpu.memory.words[13].value);
      o<<',';
      o<<"\"top.cpu.memory.words.14\":"; json(o,top.cpu.memory.words[14].value);
      o<<',';
      o<<"\"top.cpu.memory.words.15\":"; json(o,top.cpu.memory.words[15].value);
      o<<',';
      o<<"\"top.cpu.memory.words.16\":"; json(o,top.cpu.memory.words[16].value);
      o<<',';
      o<<"\"top.cpu.memory.words.17\":"; json(o,top.cpu.memory.words[17].value);
      o<<',';
      o<<"\"top.cpu.memory.words.18\":"; json(o,top.cpu.memory.words[18].value);
      o<<',';
      o<<"\"top.cpu.memory.words.19\":"; json(o,top.cpu.memory.words[19].value);
      o<<',';
      o<<"\"top.cpu.memory.words.2\":"; json(o,top.cpu.memory.words[2].value);
      o<<',';
      o<<"\"top.cpu.memory.words.20\":"; json(o,top.cpu.memory.words[20].value);
      o<<',';
      o<<"\"top.cpu.memory.words.21\":"; json(o,top.cpu.memory.words[21].value);
      o<<',';
      o<<"\"top.cpu.memory.words.22\":"; json(o,top.cpu.memory.words[22].value);
      o<<',';
      o<<"\"top.cpu.memory.words.23\":"; json(o,top.cpu.memory.words[23].value);
      o<<',';
      o<<"\"top.cpu.memory.words.24\":"; json(o,top.cpu.memory.words[24].value);
      o<<',';
      o<<"\"top.cpu.memory.words.25\":"; json(o,top.cpu.memory.words[25].value);
      o<<',';
      o<<"\"top.cpu.memory.words.26\":"; json(o,top.cpu.memory.words[26].value);
      o<<',';
      o<<"\"top.cpu.memory.words.27\":"; json(o,top.cpu.memory.words[27].value);
      o<<',';
      o<<"\"top.cpu.memory.words.28\":"; json(o,top.cpu.memory.words[28].value);
      o<<',';
      o<<"\"top.cpu.memory.words.29\":"; json(o,top.cpu.memory.words[29].value);
      o<<',';
      o<<"\"top.cpu.memory.words.3\":"; json(o,top.cpu.memory.words[3].value);
      o<<',';
      o<<"\"top.cpu.memory.words.30\":"; json(o,top.cpu.memory.words[30].value);
      o<<',';
      o<<"\"top.cpu.memory.words.31\":"; json(o,top.cpu.memory.words[31].value);
      o<<',';
      o<<"\"top.cpu.memory.words.32\":"; json(o,top.cpu.memory.words[32].value);
      o<<',';
      o<<"\"top.cpu.memory.words.33\":"; json(o,top.cpu.memory.words[33].value);
      o<<',';
      o<<"\"top.cpu.memory.words.34\":"; json(o,top.cpu.memory.words[34].value);
      o<<',';
      o<<"\"top.cpu.memory.words.35\":"; json(o,top.cpu.memory.words[35].value);
      o<<',';
      o<<"\"top.cpu.memory.words.36\":"; json(o,top.cpu.memory.words[36].value);
      o<<',';
      o<<"\"top.cpu.memory.words.37\":"; json(o,top.cpu.memory.words[37].value);
      o<<',';
      o<<"\"top.cpu.memory.words.38\":"; json(o,top.cpu.memory.words[38].value);
      o<<',';
      o<<"\"top.cpu.memory.words.39\":"; json(o,top.cpu.memory.words[39].value);
      o<<',';
      o<<"\"top.cpu.memory.words.4\":"; json(o,top.cpu.memory.words[4].value);
      o<<',';
      o<<"\"top.cpu.memory.words.40\":"; json(o,top.cpu.memory.words[40].value);
      o<<',';
      o<<"\"top.cpu.memory.words.41\":"; json(o,top.cpu.memory.words[41].value);
      o<<',';
      o<<"\"top.cpu.memory.words.42\":"; json(o,top.cpu.memory.words[42].value);
      o<<',';
      o<<"\"top.cpu.memory.words.43\":"; json(o,top.cpu.memory.words[43].value);
      o<<',';
      o<<"\"top.cpu.memory.words.44\":"; json(o,top.cpu.memory.words[44].value);
      o<<',';
      o<<"\"top.cpu.memory.words.45\":"; json(o,top.cpu.memory.words[45].value);
      o<<',';
      o<<"\"top.cpu.memory.words.46\":"; json(o,top.cpu.memory.words[46].value);
      o<<',';
      o<<"\"top.cpu.memory.words.47\":"; json(o,top.cpu.memory.words[47].value);
      o<<',';
      o<<"\"top.cpu.memory.words.48\":"; json(o,top.cpu.memory.words[48].value);
      o<<',';
      o<<"\"top.cpu.memory.words.49\":"; json(o,top.cpu.memory.words[49].value);
      o<<',';
      o<<"\"top.cpu.memory.words.5\":"; json(o,top.cpu.memory.words[5].value);
      o<<',';
      o<<"\"top.cpu.memory.words.50\":"; json(o,top.cpu.memory.words[50].value);
      o<<',';
      o<<"\"top.cpu.memory.words.51\":"; json(o,top.cpu.memory.words[51].value);
      o<<',';
      o<<"\"top.cpu.memory.words.52\":"; json(o,top.cpu.memory.words[52].value);
      o<<',';
      o<<"\"top.cpu.memory.words.53\":"; json(o,top.cpu.memory.words[53].value);
      o<<',';
      o<<"\"top.cpu.memory.words.54\":"; json(o,top.cpu.memory.words[54].value);
      o<<',';
      o<<"\"top.cpu.memory.words.55\":"; json(o,top.cpu.memory.words[55].value);
      o<<',';
      o<<"\"top.cpu.memory.words.56\":"; json(o,top.cpu.memory.words[56].value);
      o<<',';
      o<<"\"top.cpu.memory.words.57\":"; json(o,top.cpu.memory.words[57].value);
      o<<',';
      o<<"\"top.cpu.memory.words.58\":"; json(o,top.cpu.memory.words[58].value);
      o<<',';
      o<<"\"top.cpu.memory.words.59\":"; json(o,top.cpu.memory.words[59].value);
      o<<',';
      o<<"\"top.cpu.memory.words.6\":"; json(o,top.cpu.memory.words[6].value);
      o<<',';
      o<<"\"top.cpu.memory.words.60\":"; json(o,top.cpu.memory.words[60].value);
      o<<',';
      o<<"\"top.cpu.memory.words.61\":"; json(o,top.cpu.memory.words[61].value);
      o<<',';
      o<<"\"top.cpu.memory.words.62\":"; json(o,top.cpu.memory.words[62].value);
      o<<',';
      o<<"\"top.cpu.memory.words.63\":"; json(o,top.cpu.memory.words[63].value);
      o<<',';
      o<<"\"top.cpu.memory.words.7\":"; json(o,top.cpu.memory.words[7].value);
      o<<',';
      o<<"\"top.cpu.memory.words.8\":"; json(o,top.cpu.memory.words[8].value);
      o<<',';
      o<<"\"top.cpu.memory.words.9\":"; json(o,top.cpu.memory.words[9].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.0\":"; json(o,top.cpu.rf.registers[0].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.1\":"; json(o,top.cpu.rf.registers[1].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.10\":"; json(o,top.cpu.rf.registers[10].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.11\":"; json(o,top.cpu.rf.registers[11].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.12\":"; json(o,top.cpu.rf.registers[12].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.13\":"; json(o,top.cpu.rf.registers[13].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.14\":"; json(o,top.cpu.rf.registers[14].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.15\":"; json(o,top.cpu.rf.registers[15].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.16\":"; json(o,top.cpu.rf.registers[16].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.17\":"; json(o,top.cpu.rf.registers[17].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.18\":"; json(o,top.cpu.rf.registers[18].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.19\":"; json(o,top.cpu.rf.registers[19].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.2\":"; json(o,top.cpu.rf.registers[2].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.20\":"; json(o,top.cpu.rf.registers[20].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.21\":"; json(o,top.cpu.rf.registers[21].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.22\":"; json(o,top.cpu.rf.registers[22].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.23\":"; json(o,top.cpu.rf.registers[23].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.24\":"; json(o,top.cpu.rf.registers[24].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.25\":"; json(o,top.cpu.rf.registers[25].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.26\":"; json(o,top.cpu.rf.registers[26].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.27\":"; json(o,top.cpu.rf.registers[27].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.28\":"; json(o,top.cpu.rf.registers[28].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.29\":"; json(o,top.cpu.rf.registers[29].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.3\":"; json(o,top.cpu.rf.registers[3].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.30\":"; json(o,top.cpu.rf.registers[30].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.31\":"; json(o,top.cpu.rf.registers[31].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.4\":"; json(o,top.cpu.rf.registers[4].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.5\":"; json(o,top.cpu.rf.registers[5].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.6\":"; json(o,top.cpu.rf.registers[6].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.7\":"; json(o,top.cpu.rf.registers[7].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.8\":"; json(o,top.cpu.rf.registers[8].value);
      o<<',';
      o<<"\"top.cpu.rf.registers.9\":"; json(o,top.cpu.rf.registers[9].value);
      o<<',';
      o<<"\"top.cpu.writeback.retired\":"; json(o,top.cpu.writeback.retired.value);
      o<<',';
      o<<"\"top.cpu.writeback.retirement\":"; json(o,top.cpu.writeback.retirement.value);
      o<<"}}"; }
    const std::tuple<Instruction,bool> &compute_instruction_view() {
      if (!compute_instruction_view_cache) {
        const Instruction instruction_value = top.cpu.decode_stage.instruction.read();
        const bool instruction_ready = top.cpu.decode_stage.instruction.valid;
        compute_instruction_view_cache.emplace(std::tuple<Instruction,bool>{instruction_value,instruction_ready});
      }
      return *compute_instruction_view_cache;
    }
    const std::tuple<Decoded,bool> &compute_id_ex_view() {
      if (!compute_id_ex_view_cache) {
        const Decoded id_ex_value = top.cpu.id_ex.read();
        compute_id_ex_view_cache.emplace(std::tuple<Decoded,bool>{id_ex_value,true});
      }
      return *compute_id_ex_view_cache;
    }
    const std::tuple<Decoded,bool> &compute_id_ex_view_2() {
      if (!compute_id_ex_view_2_cache) {
        const Decoded id_ex_value = top.cpu.id_ex.read();
        compute_id_ex_view_2_cache.emplace(std::tuple<Decoded,bool>{id_ex_value,true});
      }
      return *compute_id_ex_view_2_cache;
    }
    const std::tuple<Executed,bool> &compute_ex_mem_view() {
      if (!compute_ex_mem_view_cache) {
        const Executed ex_mem_value = top.cpu.ex_mem.read();
        compute_ex_mem_view_cache.emplace(std::tuple<Executed,bool>{ex_mem_value,true});
      }
      return *compute_ex_mem_view_cache;
    }
    const std::tuple<Completed,bool> &compute_mem_wb_view() {
      if (!compute_mem_wb_view_cache) {
        const Completed mem_wb_value = top.cpu.mem_wb.read();
        compute_mem_wb_view_cache.emplace(std::tuple<Completed,bool>{mem_wb_value,true});
      }
      return *compute_mem_wb_view_cache;
    }
    const std::tuple<uint32_t> &compute_shared_computation() {
      if (!compute_shared_computation_cache) {
        const uint32_t payload = uint32_t{};
        compute_shared_computation_cache.emplace(std::tuple<uint32_t>{payload});
      }
      return *compute_shared_computation_cache;
    }
    const std::tuple<Decoded,bool> &compute_id_ex_view_3() {
      if (!compute_id_ex_view_3_cache) {
        const Decoded id_ex_value = top.cpu.id_ex.read();
        compute_id_ex_view_3_cache.emplace(std::tuple<Decoded,bool>{id_ex_value,true});
      }
      return *compute_id_ex_view_3_cache;
    }
    const std::tuple<uint32_t> &compute_opcode() {
      if (!compute_opcode_cache) {
        const Decoded id_ex_value = std::get<0>(compute_id_ex_view_3());
        const Decoded packet = id_ex_value;
        const Instruction executing = packet.instruction;
        const uint32_t opcode = executing.opcode;
        compute_opcode_cache.emplace(std::tuple<uint32_t>{opcode});
      }
      return *compute_opcode_cache;
    }
    const std::tuple<bool> &compute_shared_computation_2() {
      if (!compute_shared_computation_2_cache) {
        const uint32_t opcode = std::get<0>(compute_opcode());
        const bool condition = (opcode==111u);
        compute_shared_computation_2_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_2_cache;
    }
    const std::tuple<bool> &compute_shared_computation_3() {
      if (!compute_shared_computation_3_cache) {
        const uint32_t opcode = std::get<0>(compute_opcode());
        const bool condition = (opcode==99u);
        compute_shared_computation_3_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_3_cache;
    }
    const std::tuple<bool> &compute_shared_computation_4() {
      if (!compute_shared_computation_4_cache) {
        const uint32_t opcode = std::get<0>(compute_opcode());
        const bool condition = (opcode==103u);
        compute_shared_computation_4_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_4_cache;
    }
    const std::tuple<bool> &compute_shared_computation_5() {
      if (!compute_shared_computation_5_cache) {
        const Decoded id_ex_value = std::get<0>(compute_id_ex_view_3());
        const Decoded packet = id_ex_value;
        const Instruction executing = packet.instruction;
        const uint32_t function = executing.funct3;
        const bool condition = (function!=0u);
        compute_shared_computation_5_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_5_cache;
    }
    const std::tuple<bool> &compute_shared_computation_6() {
      if (!compute_shared_computation_6_cache) {
        const Decoded id_ex_value = std::get<0>(compute_id_ex_view_3());
        const Decoded packet = id_ex_value;
        const Instruction executing = packet.instruction;
        const uint32_t function = executing.funct3;
        const bool condition = (function!=1u);
        compute_shared_computation_6_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_6_cache;
    }
    const std::tuple<bool> &compute_shared_computation_7() {
      if (!compute_shared_computation_7_cache) {
        const Decoded id_ex_value = std::get<0>(compute_id_ex_view_3());
        const Decoded packet = id_ex_value;
        const Instruction executing = packet.instruction;
        const uint32_t function = executing.funct3;
        const bool condition = (function!=4u);
        compute_shared_computation_7_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_7_cache;
    }
    const std::tuple<bool> &compute_shared_computation_8() {
      if (!compute_shared_computation_8_cache) {
        const Decoded id_ex_value = std::get<0>(compute_id_ex_view_3());
        const Decoded packet = id_ex_value;
        const Instruction executing = packet.instruction;
        const uint32_t function = executing.funct3;
        const bool condition = (function!=5u);
        compute_shared_computation_8_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_8_cache;
    }
    const std::tuple<bool> &compute_shared_computation_9() {
      if (!compute_shared_computation_9_cache) {
        const Decoded id_ex_value = std::get<0>(compute_id_ex_view_3());
        const Decoded packet = id_ex_value;
        const Instruction executing = packet.instruction;
        const uint32_t function = executing.funct3;
        const bool condition = (function!=6u);
        compute_shared_computation_9_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_9_cache;
    }
    const std::tuple<bool> &compute_shared_computation_10() {
      if (!compute_shared_computation_10_cache) {
        const uint32_t opcode = std::get<0>(compute_opcode());
        const bool condition = (opcode!=55u);
        const bool condition_3 = std::get<0>(compute_shared_computation_2());
        const bool condition_4 = std::get<0>(compute_shared_computation_3());
        const bool ready_2 = (((opcode==23u) || condition_3) || condition_4);
        const bool ready_3 = (!ready_2);
        const bool ready_6 = acir_sim::check((!(condition && (ready_2 ? ready_3 : false))));
        compute_shared_computation_10_cache.emplace(std::tuple<bool>{ready_6});
      }
      return *compute_shared_computation_10_cache;
    }
    const std::tuple<bool> &compute_shared_computation_11() {
      if (!compute_shared_computation_11_cache) {
        const Decoded id_ex_value = std::get<0>(compute_id_ex_view_3());
        const Decoded packet = id_ex_value;
        const Instruction executing = packet.instruction;
        const uint32_t function = executing.funct3;
        const bool ready = acir_sim::check((function==7u));
        compute_shared_computation_11_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_11_cache;
    }
    const std::tuple<ExecutionResult> &compute_result() {
      if (!compute_result_cache) {
        const Decoded id_ex_value = std::get<0>(compute_id_ex_view_3());
        const Decoded packet = id_ex_value;
        const uint32_t pc = packet.pc;
        const Instruction executing = packet.instruction;
        const uint32_t opcode = std::get<0>(compute_opcode());
        const bool condition = (opcode!=55u);
        const bool condition_3 = std::get<0>(compute_shared_computation_2());
        const bool condition_4 = std::get<0>(compute_shared_computation_3());
        const bool ready_6 = (((opcode==23u) || condition_3) || condition_4);
        const uint32_t immediate = executing.immediate;
        const bool condition_7 = std::get<0>(compute_shared_computation_4());
        const bool ready_8 = (((opcode==3u) || (opcode==35u)) || condition_7);
        const bool ready_9 = (!ready_6);
        const bool selected_value = (ready_8 ? false : ready_9);
        const bool condition_8 = (opcode==51u);
        const bool condition_9 = (opcode==19u);
        const bool ready_10 = (condition_8 || condition_9);
        const bool condition_10 = std::get<0>(compute_shared_computation_5());
        const bool condition_11 = std::get<0>(compute_shared_computation_6());
        const uint32_t function = executing.funct3;
        const bool condition_12 = (function!=2u);
        const bool condition_13 = (function!=3u);
        const bool condition_14 = std::get<0>(compute_shared_computation_7());
        const bool condition_15 = std::get<0>(compute_shared_computation_8());
        const bool condition_16 = std::get<0>(compute_shared_computation_9());
        const Decoded id_ex_value_2 = std::get<0>(compute_id_ex_view_2());
        const Decoded packet_2 = id_ex_value_2;
        const Instruction instruction = packet_2.instruction;
        const uint32_t index = instruction.rs1;
        const Executed ex_mem_value = std::get<0>(compute_ex_mem_view());
        const Executed memory = ex_mem_value;
        const bool ready_11 = memory.writes;
        const uint32_t rd_2 = memory.rd;
        const bool ready_12 = (ready_11 && (rd_2==index));
        const bool ready_13 = (!ready_12);
        const Completed mem_wb_value = std::get<0>(compute_mem_wb_view());
        const Completed writeback = mem_wb_value;
        const bool ready_14 = writeback.writes;
        const uint32_t rd_3 = writeback.rd;
        const bool ready_15 = (ready_14 && (rd_3==index));
        const uint32_t payload = std::get<0>(compute_shared_computation());
        const uint32_t source_a = (((index==0u) || (ready_12 ? ready_13 : (ready_15 ? false : ready_13))) ? 0u : ((index!=0u) ? (ready_12 ? 1u : (ready_15 ? 2u : (ready_12 ? 1u : payload))) : payload));
        const uint32_t original = packet_2.lhs;
        const uint32_t value = writeback.value;
        const uint32_t alu = memory.alu;
        const uint32_t selected_value_8 = ((source_a==1u) ? alu : payload);
        const uint32_t lhs = ((source_a!=1u) ? ((source_a!=2u) ? original : ((source_a==2u) ? value : selected_value_8)) : selected_value_8);
        const uint32_t index_2 = instruction.rs2;
        const bool ready_17 = (ready_11 && (rd_2==index_2));
        const bool ready_18 = (!ready_17);
        const bool ready_19 = (ready_14 && (rd_3==index_2));
        const uint32_t source_b = (((index_2==0u) || (ready_17 ? ready_18 : (ready_19 ? false : ready_18))) ? 0u : ((index_2!=0u) ? (ready_17 ? 1u : (ready_19 ? 2u : (ready_17 ? 1u : payload))) : payload));
        const uint32_t original_2 = packet_2.rhs;
        const uint32_t selected_value_17 = ((source_b==1u) ? alu : payload);
        const uint32_t rhs = ((source_b!=1u) ? ((source_b!=2u) ? original_2 : ((source_b==2u) ? value : selected_value_17)) : selected_value_17);
        const ForwardedOperands operands = ForwardedOperands{lhs,rhs,source_a,source_b};
        const uint32_t lhs_2 = operands.lhs;
        const uint32_t rhs_2 = operands.rhs;
        const uint32_t operand = (condition_9 ? immediate : rhs_2);
        const bool condition_33 = (function==6u);
        const bool condition_34 = (function==5u);
        const bool condition_35 = (executing.funct7==32u);
        const uint32_t computed_value_4 = (operand & 31u);
        const bool condition_path = ((opcode==35u) ? false : (condition_8 ? (condition_12 && condition_13) : (condition_9 && (condition_12 && condition_13))));
        const bool condition_path_2 = (condition_33 ? false : (condition_34 && (condition && ((opcode==23u) ? false : ((opcode==3u) ? false : condition_path)))));
        const bool condition_path_3 = (condition_15 ? (condition_16 ? false : condition_path_2) : (condition_34 && (condition && ((opcode==23u) ? false : ((opcode==3u) ? false : condition_path)))));
        const bool condition_path_4 = (condition_7 ? false : (condition_4 ? false : (condition_10 && (condition_11 && (condition_14 && condition_path_3)))));
        uint32_t computed_value_6{};
        if ((condition_3 ? false : condition_path_4)) {
          computed_value_6 = acir_sim::shift(lhs_2,computed_value_4,1);
        }
        const bool condition_path_5 = (condition_12 && (condition_13 && (condition_35 && ((computed_value_4!=0u) && ((lhs_2 & 2147483648u)!=0u)))));
        const bool condition_path_6 = ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_8 ? condition_path_5 : (condition_9 && condition_path_5))));
        const bool condition_path_7 = (condition_16 ? false : (condition_33 ? false : (condition_34 && (condition && ((opcode==23u) ? false : condition_path_6)))));
        const bool condition_path_8 = (condition_15 ? condition_path_7 : (condition_34 && (condition && ((opcode==23u) ? false : condition_path_6))));
        const bool condition_path_9 = (condition_7 ? false : (condition_4 ? false : (condition_10 && (condition_11 && (condition_14 && condition_path_8)))));
        uint32_t computed_value_8{};
        if ((condition_3 ? false : condition_path_9)) {
          computed_value_8 = acir_sim::shift(4294967295u,(32u - computed_value_4),0);
        }
        const uint32_t result = (computed_value_6 | computed_value_8);
        const uint32_t result_2 = (((computed_value_4!=0u) && ((lhs_2 & 2147483648u)!=0u)) ? result : computed_value_6);
        const bool condition_38 = (function==4u);
        const bool condition_39 = (function==3u);
        const bool condition_41 = (function==2u);
        const uint32_t computed_value_11 = (lhs_2 ^ 2147483648u);
        const bool condition_43 = (function==1u);
        const bool condition_path_10 = ((opcode==35u) ? false : (condition_8 ? (condition_12 && condition_13) : (condition_9 && (condition_12 && condition_13))));
        const bool condition_path_11 = (condition_43 && (condition && ((opcode==23u) ? false : ((opcode==3u) ? false : condition_path_10))));
        const bool condition_path_12 = (condition_16 ? false : (condition_33 ? false : (condition_34 ? false : (condition_38 ? false : condition_path_11))));
        const bool condition_path_13 = (condition_15 ? condition_path_12 : (condition_34 ? false : (condition_38 ? false : condition_path_11)));
        const bool condition_path_14 = ((opcode==23u) ? false : ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_8 || condition_9))));
        const bool condition_path_15 = (condition_11 ? (condition_14 ? condition_path_13 : (condition_38 ? false : condition_path_11)) : (condition_43 && (condition && condition_path_14)));
        const bool condition_path_16 = (condition_3 ? false : (condition_7 ? false : (condition_4 ? false : (condition_10 && condition_path_15))));
        uint32_t computed_value_14{};
        if (condition_path_16) {
          computed_value_14 = acir_sim::shift(lhs_2,computed_value_4,0);
        }
        const bool condition_44 = (function==0u);
        const bool condition_45 = (opcode==55u);
        const uint32_t selected_value_22 = (condition_45 ? immediate : payload);
        const uint32_t selected_value_23 = (ready_8 ? (lhs_2 + immediate) : selected_value_22);
        const uint32_t selected_value_24 = (condition_44 ? ((condition_8 && condition_35) ? (lhs_2 - operand) : (lhs_2 + operand)) : selected_value_23);
        const uint32_t selected_value_25 = (condition_43 ? computed_value_14 : selected_value_24);
        const uint32_t selected_value_26 = (condition_41 ? uint32_t((computed_value_11<(operand ^ 2147483648u))) : selected_value_25);
        const uint32_t selected_value_27 = (condition_39 ? uint32_t((lhs_2<operand)) : selected_value_26);
        const uint32_t selected_value_28 = (condition_38 ? (lhs_2 ^ operand) : selected_value_27);
        const uint32_t selected_value_29 = (condition_34 ? (condition_35 ? result_2 : computed_value_6) : selected_value_28);
        const uint32_t selected_value_32 = (condition_15 ? (condition_16 ? (lhs_2 & operand) : (condition_33 ? (lhs_2 | operand) : selected_value_29)) : selected_value_29);
        const uint32_t selected_value_35 = (condition_12 ? (condition_13 ? (condition_14 ? selected_value_32 : selected_value_28) : selected_value_27) : selected_value_26);
        const uint32_t selected_value_38 = (ready_10 ? (condition_10 ? (condition_11 ? selected_value_35 : selected_value_25) : selected_value_24) : 0u);
        const uint32_t result_3 = (condition ? (ready_6 ? (pc + immediate) : (selected_value ? selected_value_38 : selected_value_23)) : selected_value_22);
        const bool jump = (condition_3 || condition_7);
        const Executed next_packet = Executed{packet.valid,pc,packet.word,executing.rd,executing.writes,executing.load,executing.store,result_3,rhs_2,(jump ? (pc + 4u) : result_3),packet.stalled};
        const uint32_t computed_value_19 = (rhs_2 ^ 2147483648u);
        const bool condition_52 = (opcode!=99u);
        const bool selected_value_42 = (condition_52 ? false : bool{});
        const bool selected_value_43 = (condition_44 ? (lhs_2==rhs_2) : selected_value_42);
        const bool selected_value_44 = (condition_43 ? (lhs_2!=rhs_2) : selected_value_43);
        const bool selected_value_45 = (condition_38 ? (computed_value_11<computed_value_19) : selected_value_44);
        const bool selected_value_46 = (condition_34 ? (computed_value_11>=computed_value_19) : selected_value_45);
        const bool selected_value_49 = (condition_15 ? (condition_16 ? (lhs_2>=rhs_2) : (condition_33 ? (lhs_2<rhs_2) : selected_value_46)) : selected_value_46);
        const bool selected_value_52 = (condition_10 ? (condition_11 ? (condition_14 ? selected_value_49 : selected_value_45) : selected_value_44) : selected_value_43);
        const bool ready_25 = (jump || (condition_4 ? selected_value_52 : selected_value_42));
        const ExecutionResult result_4 = ExecutionResult{next_packet,ready_25,(ready_25 ? result_3 : 0u)};
        compute_result_cache.emplace(std::tuple<ExecutionResult>{result_4});
      }
      return *compute_result_cache;
    }
    const std::tuple<bool> &compute_shared_computation_12() {
      if (!compute_shared_computation_12_cache) {
        const Decoded id_ex_value = std::get<0>(compute_id_ex_view());
        const Instruction executing = id_ex_value.instruction;
        const uint32_t rd = executing.rd;
        const bool ready_2 = (executing.load && (rd!=0u));
        const bool ready_3 = (!ready_2);
        bool instruction_ready{};
        if ((executing.load && (rd!=0u))) {
          instruction_ready = std::get<1>(compute_instruction_view());
        }
        Instruction instruction_value{};
        if ((executing.load && (rd!=0u))) {
          instruction_value = std::get<0>(compute_instruction_view());
        }
        const Instruction instruction = instruction_value;
        const bool condition_2 = (rd==instruction.rs1);
        const bool ready_9 = ((ready_3 || (instruction_ready && (condition_2 || instruction_ready))) && (ready_3 || instruction_ready));
        const uint32_t opcode = std::get<0>(compute_opcode());
        const bool condition_3 = (opcode==55u);
        const bool condition_5 = std::get<0>(compute_shared_computation_2());
        const bool condition_6 = std::get<0>(compute_shared_computation_3());
        const bool ready_12 = (((opcode==23u) || condition_5) || condition_6);
        const bool condition_9 = std::get<0>(compute_shared_computation_4());
        const bool ready_14 = (((opcode==3u) || (opcode==35u)) || condition_9);
        const bool ready_15 = (!ready_12);
        const bool selected_value = (ready_14 ? false : ready_15);
        const bool condition_10 = (opcode==51u);
        const bool condition_11 = (opcode==19u);
        const bool ready_16 = (condition_10 || condition_11);
        const bool condition_12 = std::get<0>(compute_shared_computation_5());
        const bool condition_13 = std::get<0>(compute_shared_computation_6());
        const Decoded id_ex_value_2 = std::get<0>(compute_id_ex_view_3());
        const Decoded packet = id_ex_value_2;
        const Instruction executing_2 = packet.instruction;
        const uint32_t function = executing_2.funct3;
        const bool condition_14 = (function!=2u);
        const bool condition_15 = (function!=3u);
        const bool condition_16 = std::get<0>(compute_shared_computation_7());
        const bool condition_17 = std::get<0>(compute_shared_computation_8());
        const bool condition_18 = std::get<0>(compute_shared_computation_9());
        const bool condition_19 = (function==6u);
        const bool condition_20 = (function==5u);
        const bool condition_21 = (executing_2.funct7==32u);
        const uint32_t immediate = executing_2.immediate;
        const Decoded id_ex_value_3 = std::get<0>(compute_id_ex_view_2());
        const Decoded packet_2 = id_ex_value_3;
        const Instruction instruction_2 = packet_2.instruction;
        const uint32_t index = instruction_2.rs1;
        const Executed ex_mem_value = std::get<0>(compute_ex_mem_view());
        const Executed memory = ex_mem_value;
        const bool ready_20 = memory.writes;
        const uint32_t rd_2 = memory.rd;
        const bool ready_21 = (ready_20 && (rd_2==index));
        const bool ready_22 = (!ready_21);
        const Completed mem_wb_value = std::get<0>(compute_mem_wb_view());
        const Completed writeback = mem_wb_value;
        const bool ready_23 = writeback.writes;
        const uint32_t rd_3 = writeback.rd;
        const bool ready_24 = (ready_23 && (rd_3==index));
        const uint32_t payload = std::get<0>(compute_shared_computation());
        const uint32_t source_a = (((index==0u) || (ready_21 ? ready_22 : (ready_24 ? false : ready_22))) ? 0u : ((index!=0u) ? (ready_21 ? 1u : (ready_24 ? 2u : (ready_21 ? 1u : payload))) : payload));
        const uint32_t original = packet_2.lhs;
        const uint32_t value = writeback.value;
        const uint32_t alu = memory.alu;
        const uint32_t selected_value_8 = ((source_a==1u) ? alu : payload);
        const uint32_t lhs = ((source_a!=1u) ? ((source_a!=2u) ? original : ((source_a==2u) ? value : selected_value_8)) : selected_value_8);
        const uint32_t index_2 = instruction_2.rs2;
        const bool ready_26 = (ready_20 && (rd_2==index_2));
        const bool ready_27 = (!ready_26);
        const bool ready_28 = (ready_23 && (rd_3==index_2));
        const uint32_t source_b = (((index_2==0u) || (ready_26 ? ready_27 : (ready_28 ? false : ready_27))) ? 0u : ((index_2!=0u) ? (ready_26 ? 1u : (ready_28 ? 2u : (ready_26 ? 1u : payload))) : payload));
        const uint32_t original_2 = packet_2.rhs;
        const uint32_t selected_value_17 = ((source_b==1u) ? alu : payload);
        const uint32_t rhs = ((source_b!=1u) ? ((source_b!=2u) ? original_2 : ((source_b==2u) ? value : selected_value_17)) : selected_value_17);
        const ForwardedOperands operands = ForwardedOperands{lhs,rhs,source_a,source_b};
        const uint32_t rhs_2 = operands.rhs;
        const uint32_t operand = (condition_11 ? immediate : rhs_2);
        const uint32_t computed_value = (operand & 31u);
        const bool condition_39 = (function==4u);
        const bool condition_40 = (function==3u);
        const bool condition_41 = (function==2u);
        const bool condition_42 = (function==1u);
        const bool condition_43 = (function==0u);
        bool ready_37{};
        if ((executing.load ? ((rd!=0u) ? instruction_ready : true) : true)) {
          ready_37 = std::get<0>(compute_shared_computation_10());
        }
        const bool jump = (condition_5 || condition_9);
        const bool condition_44 = (opcode!=99u);
        const bool condition_path = (condition_18 ? (ready_37 && (!condition_44)) : (condition_19 ? false : (ready_37 && (!condition_44))));
        const bool condition_path_2 = (condition_17 ? condition_path : (condition_19 ? false : (condition_20 ? false : (ready_37 && (!condition_44)))));
        const bool condition_path_3 = (condition_19 ? false : (condition_20 ? false : (condition_39 ? false : (ready_37 && (!condition_44)))));
        const bool condition_path_4 = (condition_20 ? false : (condition_39 ? false : (condition_42 ? false : (ready_37 && (!condition_44)))));
        const bool condition_path_5 = (condition_13 ? (condition_16 ? condition_path_2 : condition_path_3) : (condition_19 ? false : condition_path_4));
        const bool condition_path_6 = (condition_39 ? false : (condition_42 ? false : (condition_43 ? false : (ready_37 && (!condition_44)))));
        const bool condition_path_7 = (condition_12 ? condition_path_5 : (condition_19 ? false : (condition_20 ? false : condition_path_6)));
        const bool condition_path_8 = ((rd!=0u) ? (instruction_ready && (condition_5 ? false : (condition_9 ? false : condition_path_7))) : (condition_5 ? false : (condition_9 ? false : condition_path_7)));
        const bool condition_path_9 = (executing.load ? condition_path_8 : (condition_5 ? false : (condition_9 ? false : condition_path_7)));
        bool ready_42{};
        if (condition_path_9) {
          ready_42 = std::get<0>(compute_shared_computation_11());
        }
        const bool ready_45 = (condition_19 || ready_42);
        const bool ready_47 = (condition_20 || ready_45);
        const bool selected_value_43 = (condition_16 ? ((condition_17 ? ((condition_18 ? ready_42 : true) && ready_45) : true) && ready_47) : true);
        const bool ready_49 = (condition_39 || ready_47);
        const bool ready_51 = (condition_42 || ready_49);
        const bool ready_53 = (condition_43 || ready_51);
        const bool ready_54 = ((condition_12 ? ((condition_13 ? (selected_value_43 && ready_49) : true) && ready_51) : true) && ready_53);
        const bool ready_58 = (jump || ((condition_44 || ready_54) && (condition_44 || ready_53)));
        compute_shared_computation_12_cache.emplace(std::tuple<bool>{(ready_9 && (((ready_37 && (jump || ready_37)) && ready_58) && (ready_58 && ready_37)))});
      }
      return *compute_shared_computation_12_cache;
    }
    const std::tuple<Control> &compute_control() {
      if (!compute_control_cache) {
        const Decoded id_ex_value = std::get<0>(compute_id_ex_view());
        const Instruction executing = id_ex_value.instruction;
        const uint32_t rd = executing.rd;
        const bool ready_2 = (executing.load && (rd!=0u));
        Instruction instruction_value{};
        if ((executing.load && (rd!=0u))) {
          instruction_value = std::get<0>(compute_instruction_view());
        }
        const Instruction instruction = instruction_value;
        const bool condition_2 = (rd==instruction.rs1);
        const bool stall = (ready_2 && (condition_2 || (rd==instruction.rs2)));
        const ExecutionResult result = std::get<0>(compute_result());
        const Decoded id_ex_value_2 = std::get<0>(compute_id_ex_view_2());
        const Decoded packet = id_ex_value_2;
        const Instruction instruction_2 = packet.instruction;
        const uint32_t index = instruction_2.rs1;
        const Executed ex_mem_value = std::get<0>(compute_ex_mem_view());
        const Executed memory = ex_mem_value;
        const bool ready_6 = memory.writes;
        const uint32_t rd_2 = memory.rd;
        const bool ready_7 = (ready_6 && (rd_2==index));
        const bool ready_8 = (!ready_7);
        const Completed mem_wb_value = std::get<0>(compute_mem_wb_view());
        const Completed writeback = mem_wb_value;
        const bool ready_9 = writeback.writes;
        const uint32_t rd_3 = writeback.rd;
        const bool ready_10 = (ready_9 && (rd_3==index));
        const uint32_t payload = std::get<0>(compute_shared_computation());
        const uint32_t source_a = (((index==0u) || (ready_7 ? ready_8 : (ready_10 ? false : ready_8))) ? 0u : ((index!=0u) ? (ready_7 ? 1u : (ready_10 ? 2u : (ready_7 ? 1u : payload))) : payload));
        const uint32_t original = packet.lhs;
        const uint32_t value = writeback.value;
        const uint32_t alu = memory.alu;
        const uint32_t selected_value_7 = ((source_a==1u) ? alu : payload);
        const uint32_t lhs = ((source_a!=1u) ? ((source_a!=2u) ? original : ((source_a==2u) ? value : selected_value_7)) : selected_value_7);
        const uint32_t index_2 = instruction_2.rs2;
        const bool ready_12 = (ready_6 && (rd_2==index_2));
        const bool ready_13 = (!ready_12);
        const bool ready_14 = (ready_9 && (rd_3==index_2));
        const uint32_t source_b = (((index_2==0u) || (ready_12 ? ready_13 : (ready_14 ? false : ready_13))) ? 0u : ((index_2!=0u) ? (ready_12 ? 1u : (ready_14 ? 2u : (ready_12 ? 1u : payload))) : payload));
        const uint32_t original_2 = packet.rhs;
        const uint32_t selected_value_16 = ((source_b==1u) ? alu : payload);
        const uint32_t rhs = ((source_b!=1u) ? ((source_b!=2u) ? original_2 : ((source_b==2u) ? value : selected_value_16)) : selected_value_16);
        const ForwardedOperands operands = ForwardedOperands{lhs,rhs,source_a,source_b};
        const Control control = Control{stall,result.redirect,result.target,operands.source_a,operands.source_b};
        compute_control_cache.emplace(std::tuple<Control>{control});
      }
      return *compute_control_cache;
    }
    const std::tuple<Control> &compute_control_2() {
      if (!compute_control_2_cache) {
        const Control control = std::get<0>(compute_control());
        const Control control_2 = control;
        compute_control_2_cache.emplace(std::tuple<Control>{control_2});
      }
      return *compute_control_2_cache;
    }
    const std::tuple<bool> &compute_shared_computation_13() {
      if (!compute_shared_computation_13_cache) {
        const Control control = std::get<0>(compute_control_2());
        const bool ready = control.stall;
        compute_shared_computation_13_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_13_cache;
    }
    const std::tuple<bool> &compute_shared_computation_14() {
      if (!compute_shared_computation_14_cache) {
        const bool ready = std::get<0>(compute_shared_computation_13());
        const bool ready_2 = (!ready);
        compute_shared_computation_14_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_14_cache;
    }
    const std::tuple<bool> &compute_shared_computation_15() {
      if (!compute_shared_computation_15_cache) {
        const Control control = std::get<0>(compute_control_2());
        const bool ready = control.redirect;
        compute_shared_computation_15_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_15_cache;
    }
    const std::tuple<bool> &compute_shared_computation_16() {
      if (!compute_shared_computation_16_cache) {
        const bool ready = std::get<0>(compute_shared_computation_13());
        const bool ready_2 = std::get<0>(compute_shared_computation_12());
        const bool ready_4 = ((ready || ready_2) && ready_2);
        compute_shared_computation_16_cache.emplace(std::tuple<bool>{ready_4});
      }
      return *compute_shared_computation_16_cache;
    }
    const std::tuple<bool> &compute_shared_computation_17() {
      if (!compute_shared_computation_17_cache) {
        const bool ready = std::get<0>(compute_shared_computation_14());
        bool ready_2{};
        if (ready) {
          ready_2 = std::get<0>(compute_shared_computation_15());
        }
        const bool ready_4 = (ready && (!ready_2));
        compute_shared_computation_17_cache.emplace(std::tuple<bool>{ready_4});
      }
      return *compute_shared_computation_17_cache;
    }
  };
} // namespace acir_generated
