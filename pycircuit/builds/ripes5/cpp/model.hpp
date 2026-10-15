#pragma once
#include "runtime.hpp"
#include <set>
namespace acir_generated {
  using namespace acir_sim;
  using acir_sim::json;
  // ACPy payload src_isa_ae2f66cd.Instruction
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
    bool illegal{};
    uint32_t system{};
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
    o<<','; json(o,v.illegal);
    o<<','; json(o,v.system);
    o<<']'; }
  // ACPy payload src_isa_ae2f66cd.Executed
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
    uint32_t funct3{};
    bool illegal{};
    uint32_t system{};
    uint32_t expanded{};
    uint32_t next_pc{};
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
    o<<','; json(o,v.funct3);
    o<<','; json(o,v.illegal);
    o<<','; json(o,v.system);
    o<<','; json(o,v.expanded);
    o<<','; json(o,v.next_pc);
    o<<']'; }
  // ACPy payload src_isa_ae2f66cd.Decoded
  struct Decoded {
    bool valid{};
    uint32_t pc{};
    uint32_t word{};
    Instruction instruction{};
    uint32_t lhs{};
    uint32_t rhs{};
    bool stalled{};
    uint32_t next_pc{};
    uint32_t expanded{};
    bool fetch_fault{};
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
    o<<','; json(o,v.next_pc);
    o<<','; json(o,v.expanded);
    o<<','; json(o,v.fetch_fault);
    o<<']'; }
  // ACPy payload src_isa_ae2f66cd.Fetched
  struct Fetched {
    bool valid{};
    uint32_t pc{};
    uint32_t word{};
    uint32_t expanded{};
    uint32_t next_pc{};
    bool fetch_fault{};
    bool operator==(const Fetched&) const = default;
  };
  inline void json(std::ostream &o,const Fetched &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.pc);
    o<<','; json(o,v.word);
    o<<','; json(o,v.expanded);
    o<<','; json(o,v.next_pc);
    o<<','; json(o,v.fetch_fault);
    o<<']'; }
  // ACPy payload src_isa_ae2f66cd.Control
  struct Control {
    bool stall{};
    bool redirect{};
    uint32_t target{};
    uint32_t forward_a{};
    uint32_t forward_b{};
    bool ecall_stall{};
    bool stop{};
    bool operator==(const Control&) const = default;
  };
  inline void json(std::ostream &o,const Control &v) { o<<'[';
    json(o,v.stall);
    o<<','; json(o,v.redirect);
    o<<','; json(o,v.target);
    o<<','; json(o,v.forward_a);
    o<<','; json(o,v.forward_b);
    o<<','; json(o,v.ecall_stall);
    o<<','; json(o,v.stop);
    o<<']'; }
  // ACPy payload src_isa_ae2f66cd.Completed
  struct Completed {
    bool valid{};
    uint32_t pc{};
    uint32_t word{};
    uint32_t rd{};
    bool writes{};
    uint32_t value{};
    bool stalled{};
    bool illegal{};
    uint32_t system{};
    uint32_t expanded{};
    uint32_t next_pc{};
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
    o<<','; json(o,v.illegal);
    o<<','; json(o,v.system);
    o<<','; json(o,v.expanded);
    o<<','; json(o,v.next_pc);
    o<<']'; }
  // ACPy payload src_isa_ae2f66cd.StoreEvent
  struct StoreEvent {
    bool valid{};
    uint32_t address{};
    uint32_t value{};
    uint32_t size{};
    bool operator==(const StoreEvent&) const = default;
  };
  inline void json(std::ostream &o,const StoreEvent &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.address);
    o<<','; json(o,v.value);
    o<<','; json(o,v.size);
    o<<']'; }
  // ACPy payload src_isa_ae2f66cd.SystemEvent
  struct SystemEvent {
    bool valid{};
    uint32_t pc{};
    uint32_t kind{};
    uint32_t number{};
    uint32_t argument{};
    bool operator==(const SystemEvent&) const = default;
  };
  inline void json(std::ostream &o,const SystemEvent &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.pc);
    o<<','; json(o,v.kind);
    o<<','; json(o,v.number);
    o<<','; json(o,v.argument);
    o<<']'; }
  // ACPy payload src_isa_ae2f66cd.Retirement
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
            const uint32_t word = if_id_value.expanded;
            const uint32_t opcode = (word & 127u);
            const uint32_t computed_value = (word >> 12u);
            const uint32_t function = (computed_value & 7u);
            const uint32_t upper = (word >> 25u);
            const uint32_t computed_value_2 = (word >> 7u);
            const uint32_t computed_value_3 = (computed_value_2 & 31u);
            const bool condition = (opcode==51u);
            const uint32_t computed_value_5 = ((word >> 15u) & 31u);
            const bool condition_2 = (opcode==19u);
            const bool condition_3 = (opcode==3u);
            const bool condition_4 = (opcode==103u);
            const bool ready = (condition_3 || condition_4);
            const bool condition_5 = (opcode==35u);
            const bool condition_6 = (opcode==99u);
            const bool condition_7 = (opcode==55u);
            const bool condition_8 = (opcode==23u);
            const bool ready_2 = (condition_7 || condition_8);
            const bool condition_9 = (opcode==111u);
            const bool condition_10 = (opcode==15u);
            const bool condition_11 = (opcode==115u);
            const uint32_t selected_value_5 = (condition_6 ? computed_value_5 : (ready_2 ? 0u : (condition_9 ? 0u : (condition_10 ? 0u : (condition_11 ? 0u : computed_value_5)))));
            const uint32_t selected_value_8 = (condition_2 ? computed_value_5 : (ready ? computed_value_5 : (condition_5 ? computed_value_5 : selected_value_5)));
            const uint32_t computed_value_6 = (word >> 20u);
            const uint32_t computed_value_9 = ((computed_value_6 ^ 2048u) - 2048u);
            const uint32_t value = ((upper << 5u) | computed_value_3);
            const uint32_t computed_value_13 = (word >> 31u);
            const uint32_t immediate = ((((computed_value_13 << 12u) | ((computed_value_2 & 1u) << 11u)) | ((upper & 63u) << 5u)) | (((word >> 8u) & 15u) << 1u));
            const uint32_t computed_value_33 = (((computed_value_13 << 20u) | ((computed_value & 255u) << 12u)) | ((computed_value_6 & 1u) << 11u));
            const uint32_t immediate_2 = (computed_value_33 | (((word >> 21u) & 1023u) << 1u));
            const uint32_t selected_value_13 = (condition_6 ? ((immediate ^ 4096u) - 4096u) : (ready_2 ? (word & 4294963200u) : (condition_9 ? ((immediate_2 ^ 1048576u) - 1048576u) : 0u)));
            const uint32_t selected_value_16 = (condition_2 ? computed_value_9 : (ready ? computed_value_9 : (condition_5 ? ((value ^ 2048u) - 2048u) : selected_value_13)));
            const bool condition_12 = (upper==0u);
            const bool condition_13 = (upper==32u);
            const bool legal = (function==0u);
            const bool condition_14 = (function==5u);
            const bool legal_2 = ((condition_12 || (condition_13 && (legal || condition_14))) || (upper==1u));
            const bool condition_18 = (function==1u);
            const bool legal_3 = ((((function!=1u) && (function!=5u)) || (condition_18 && condition_12)) || (condition_14 && (condition_12 || condition_13)));
            const bool ready_13 = (legal || condition_18);
            const bool legal_4 = (condition_3 ? (((ready_13 || (function==2u)) || (function==4u)) || condition_14) : legal);
            const bool legal_5 = (function<=2u);
            const bool legal_6 = (ready_13 || (function>=4u));
            const bool condition_22 = (word==115u);
            const bool legal_7 = (condition_22 || (word==1048691u));
            const bool legal_8 = (condition_11 && legal_7);
            const bool legal_9 = (condition_10 ? legal : legal_8);
            const bool legal_10 = (condition_9 || legal_9);
            const bool legal_11 = (ready_2 || legal_10);
            const bool legal_12 = (condition_6 ? legal_6 : legal_11);
            const bool legal_13 = (condition_5 ? legal_5 : legal_12);
            const bool legal_14 = (ready ? legal_4 : legal_13);
            const bool legal_15 = (condition_2 ? legal_3 : legal_14);
            const bool legal_16 = (condition ? legal_2 : legal_15);
            const bool ready_22 = ((((((condition || condition_2) || condition_3) || condition_7) || condition_8) || condition_9) || condition_4);
            const uint32_t selected_value_22 = (ready_2 ? 0u : (condition_9 ? 0u : (condition_10 ? 0u : (condition_11 ? (condition_22 ? 1u : 2u) : 0u))));
            const uint32_t selected_value_27 = (condition ? 0u : (condition_2 ? 0u : (ready ? 0u : (condition_5 ? 0u : (condition_6 ? 0u : selected_value_22)))));
            const Instruction instruction = Instruction{opcode,function,upper,computed_value_3,(condition ? computed_value_5 : selected_value_8),(((condition || condition_5) || condition_6) ? (computed_value_6 & 31u) : 0u),(condition ? 0u : selected_value_16),(legal_16 && ready_22),(legal_16 && condition_3),(legal_16 && condition_5),(!legal_16),selected_value_27};
            const Instruction set_value = instruction;
            ctx.top.cpu.decode_stage.instruction.write(set_value);
            ctx.fired_top_cpu_decode_stage_decode_instruction=true;
          }
          // Rule top.cpu.decode_stage.advance#2
          template<class Context> void try_advance(Context &ctx) {
            auto compute_conditional_value = [&]() -> bool {
              return std::get<0>(ctx.compute_shared_computation_68());
            };
            auto compute_conditional_value_2 = [&]() -> bool {
              return std::get<0>(ctx.compute_shared_computation_69());
            };
            auto compute_conditional_value_3 = [&]() -> bool {
              const auto &operand = compute_conditional_value();
              return (operand && compute_conditional_value_2());
            };
            auto compute_wait = [&]() -> bool {
              return std::get<0>(ctx.compute_wait());
            };
            std::optional<bool> conditional_value_cache_4;
            auto compute_conditional_value_4 = [&]() -> const bool & {
              if (!conditional_value_cache_4) {
                const auto &operand_2 = compute_wait();
                conditional_value_cache_4.emplace((operand_2 || compute_conditional_value_2()));
              }
              return *conditional_value_cache_4;
            };
            auto compute_conditional_value_5 = [&]() -> bool {
              return std::get<0>(ctx.compute_shared_computation_73());
            };
            auto compute_conditional_value_6 = [&]() -> bool {
              const auto &operand_3 = compute_conditional_value_4();
              return (operand_3 && compute_conditional_value_5());
            };
            auto compute_conditional_value_7 = [&]() -> bool {
              const auto &operand_4 = compute_wait();
              return (operand_4 || compute_conditional_value_6());
            };
            auto compute_conditional_value_8 = [&]() -> bool {
              const auto &operand_5 = compute_conditional_value_3();
              return (operand_5 && compute_conditional_value_7());
            };
            auto compute_stall = [&]() -> bool {
              return std::get<0>(ctx.compute_stall());
            };
            auto compute_conditional_value_9 = [&]() -> bool {
              return std::get<0>(ctx.compute_shared_computation_36());
            };
            auto compute_conditional_value_10 = [&]() -> bool {
              const auto &operand_6 = compute_stall();
              return (operand_6 || compute_conditional_value_9());
            };
            std::optional<bool> conditional_value_cache_11;
            auto compute_conditional_value_11 = [&]() -> const bool & {
              if (!conditional_value_cache_11) {
                const auto &operand_7 = compute_conditional_value_5();
                conditional_value_cache_11.emplace((operand_7 && compute_conditional_value_10()));
              }
              return *conditional_value_cache_11;
            };
            auto compute_conditional_value_12 = [&]() -> bool {
              const auto &operand_8 = compute_conditional_value_4();
              return (operand_8 && compute_conditional_value_11());
            };
            auto compute_conditional_value_13 = [&]() -> bool {
              const auto &operand_9 = compute_wait();
              return (operand_9 || compute_conditional_value_12());
            };
            auto compute_conditional_value_14 = [&]() -> bool {
              const auto &operand_10 = compute_conditional_value_8();
              return (operand_10 && compute_conditional_value_13());
            };
            auto compute_redirect = [&]() -> bool {
              return std::get<0>(ctx.compute_redirect());
            };
            std::optional<bool> conditional_value_cache_15;
            auto compute_conditional_value_15 = [&]() -> const bool & {
              if (!conditional_value_cache_15) {
                const auto &operand_11 = compute_stall();
                conditional_value_cache_15.emplace((operand_11 || compute_redirect()));
              }
              return *conditional_value_cache_15;
            };
            auto compute_conditional_value_16 = [&]() -> bool {
              return std::get<0>(ctx.compute_shared_computation_71());
            };
            auto compute_conditional_value_17 = [&]() -> bool {
              const auto &operand_12 = compute_conditional_value_15();
              return (operand_12 || compute_conditional_value_16());
            };
            std::optional<bool> conditional_value_cache_18;
            auto compute_conditional_value_18 = [&]() -> const bool & {
              if (!conditional_value_cache_18) {
                const auto &operand_13 = compute_conditional_value_11();
                conditional_value_cache_18.emplace((operand_13 && compute_conditional_value_17()));
              }
              return *conditional_value_cache_18;
            };
            auto compute_conditional_value_19 = [&]() -> bool {
              const auto &operand_14 = compute_conditional_value_4();
              return (operand_14 && compute_conditional_value_18());
            };
            auto compute_conditional_value_20 = [&]() -> bool {
              const auto &operand_15 = compute_wait();
              return (operand_15 || compute_conditional_value_19());
            };
            auto compute_conditional_value_21 = [&]() -> bool {
              const auto &operand_16 = compute_conditional_value_14();
              return (operand_16 && compute_conditional_value_20());
            };
            auto compute_conditional_value_22 = [&]() -> bool {
              const auto &operand_17 = compute_conditional_value_18();
              return (operand_17 && compute_conditional_value_2());
            };
            auto compute_conditional_value_23 = [&]() -> bool {
              const auto &operand_18 = compute_conditional_value_15();
              const auto &operand_19 = true;
              return (operand_18 != operand_19);
            };
            auto compute_conditional_value_24 = [&]() -> bool {
              return std::get<0>(ctx.compute_shared_computation_70());
            };
            auto compute_conditional_value_25 = [&]() -> bool {
              const auto &operand_20 = compute_conditional_value_23();
              return (operand_20 && compute_conditional_value_24());
            };
            auto compute_conditional_value_26 = [&]() -> bool {
              const auto &operand_21 = compute_conditional_value_15();
              return (operand_21 || compute_conditional_value_25());
            };
            auto compute_conditional_value_27 = [&]() -> bool {
              const auto &operand_22 = compute_conditional_value_26();
              return (operand_22 && compute_conditional_value_11());
            };
            auto compute_conditional_value_28 = [&]() -> bool {
              return std::get<0>(ctx.compute_shared_computation_35());
            };
            auto compute_conditional_value_29 = [&]() -> bool {
              const auto &operand_23 = compute_conditional_value_27();
              return (operand_23 && compute_conditional_value_28());
            };
            auto compute_conditional_value_30 = [&]() -> bool {
              const auto &operand_24 = compute_conditional_value_22();
              return (operand_24 && compute_conditional_value_29());
            };
            std::optional<bool> conditional_value_cache_31;
            auto compute_conditional_value_31 = [&]() -> const bool & {
              if (!conditional_value_cache_31) {
                const auto &operand_25 = compute_conditional_value_4();
                conditional_value_cache_31.emplace((operand_25 && compute_conditional_value_30()));
              }
              return *conditional_value_cache_31;
            };
            auto compute_conditional_value_32 = [&]() -> bool {
              const auto &operand_26 = compute_wait();
              return (operand_26 || compute_conditional_value_31());
            };
            auto compute_conditional_value_33 = [&]() -> bool {
              const auto &operand_27 = compute_conditional_value_21();
              return (operand_27 && compute_conditional_value_32());
            };
            auto compute_conditional_value_34 = [&]() -> bool {
              return std::get<0>(ctx.compute_shared_computation_51());
            };
            auto compute_stopping = [&]() -> bool {
              return std::get<0>(ctx.compute_stopping());
            };
            auto compute_conditional_value_35 = [&]() -> bool {
              const auto &operand_28 = compute_conditional_value_15();
              return (operand_28 || compute_stopping());
            };
            auto compute_conditional_value_36 = [&]() -> bool {
              const auto &operand_29 = compute_conditional_value_35();
              return (operand_29 ? false : compute_conditional_value_34());
            };
            std::optional<bool> conditional_value_cache_37;
            auto compute_conditional_value_37 = [&]() -> const bool & {
              if (!conditional_value_cache_37) {
                const auto &operand_30 = compute_conditional_value_34();
                conditional_value_cache_37.emplace((operand_30 && compute_conditional_value_36()));
              }
              return *conditional_value_cache_37;
            };
            std::optional<bool> conditional_value_cache_38;
            auto compute_conditional_value_38 = [&]() -> const bool & {
              if (!conditional_value_cache_38) {
                const auto &operand_31 = compute_conditional_value_37();
                const auto &operand_32 = true;
                conditional_value_cache_38.emplace((operand_31 != operand_32));
              }
              return *conditional_value_cache_38;
            };
            auto compute_conditional_value_39 = [&]() -> bool {
              const auto &operand_33 = compute_conditional_value_38();
              return (operand_33 || compute_conditional_value_31());
            };
            std::optional<bool> conditional_value_cache_40;
            auto compute_conditional_value_40 = [&]() -> const bool & {
              if (!conditional_value_cache_40) {
                const auto &operand_34 = compute_conditional_value_38();
                conditional_value_cache_40.emplace((operand_34 || compute_conditional_value_39()));
              }
              return *conditional_value_cache_40;
            };
            auto compute_conditional_value_41 = [&]() -> bool {
              const auto &operand_35 = compute_conditional_value_33();
              return (operand_35 && compute_conditional_value_40());
            };
            std::optional<bool> conditional_value_cache_42;
            auto compute_conditional_value_42 = [&]() -> const bool & {
              if (!conditional_value_cache_42) {
                conditional_value_cache_42.emplace(ctx.top.cpu.decode_stage.instruction.valid);
              }
              return *conditional_value_cache_42;
            };
            std::optional<bool> conditional_value_cache_43;
            auto compute_conditional_value_43 = [&]() -> const bool & {
              if (!conditional_value_cache_43) {
                const auto &operand_36 = compute_conditional_value_40();
                conditional_value_cache_43.emplace((operand_36 && compute_conditional_value_42()));
              }
              return *conditional_value_cache_43;
            };
            std::optional<bool> conditional_value_cache_44;
            auto compute_conditional_value_44 = [&]() -> const bool & {
              if (!conditional_value_cache_44) {
                const auto &operand_37 = compute_conditional_value_38();
                conditional_value_cache_44.emplace((operand_37 || compute_conditional_value_43()));
              }
              return *conditional_value_cache_44;
            };
            auto compute_conditional_value_45 = [&]() -> bool {
              const auto &operand_38 = compute_conditional_value_41();
              return (operand_38 && compute_conditional_value_44());
            };
            std::optional<Instruction> conditional_value_cache_46;
            auto compute_conditional_value_46 = [&]() -> const Instruction & {
              if (!conditional_value_cache_46) {
                conditional_value_cache_46.emplace(ctx.top.cpu.decode_stage.instruction.read());
              }
              return *conditional_value_cache_46;
            };
            std::optional<uint32_t> index_cache;
            auto compute_index = [&]() -> const uint32_t & {
              if (!index_cache) {
                const auto &operand_39 = compute_conditional_value_46();
                index_cache.emplace(operand_39.rs1);
              }
              return *index_cache;
            };
            auto compute_conditional_value_47 = [&]() -> bool {
              const auto &operand_40 = compute_index();
              const auto &operand_41 = 0u;
              return (operand_40!=operand_41);
            };
            std::optional<bool> conditional_value_cache_48;
            auto compute_conditional_value_48 = [&]() -> const bool & {
              if (!conditional_value_cache_48) {
                const auto &operand_42 = compute_conditional_value_37();
                conditional_value_cache_48.emplace((operand_42 && compute_conditional_value_47()));
              }
              return *conditional_value_cache_48;
            };
            std::optional<bool> conditional_value_cache_49;
            auto compute_conditional_value_49 = [&]() -> const bool & {
              if (!conditional_value_cache_49) {
                const auto &operand_43 = compute_conditional_value_48();
                const auto &operand_44 = true;
                conditional_value_cache_49.emplace((operand_43 != operand_44));
              }
              return *conditional_value_cache_49;
            };
            std::optional<bool> conditional_value_cache_50;
            auto compute_conditional_value_50 = [&]() -> const bool & {
              if (!conditional_value_cache_50) {
                const auto &operand_45 = compute_conditional_value_49();
                conditional_value_cache_50.emplace((operand_45 || compute_conditional_value_43()));
              }
              return *conditional_value_cache_50;
            };
            auto compute_conditional_value_51 = [&]() -> bool {
              const auto &operand_46 = compute_conditional_value_49();
              return (operand_46 || compute_conditional_value_50());
            };
            auto compute_conditional_value_52 = [&]() -> bool {
              const auto &operand_47 = compute_conditional_value_45();
              return (operand_47 && compute_conditional_value_51());
            };
            std::optional<Completed> conditional_value_cache_53;
            auto compute_conditional_value_53 = [&]() -> const Completed & {
              if (!conditional_value_cache_53) {
                conditional_value_cache_53.emplace(ctx.top.cpu.mem_wb.read());
              }
              return *conditional_value_cache_53;
            };
            std::optional<bool> conditional_value_cache_54;
            auto compute_conditional_value_54 = [&]() -> const bool & {
              if (!conditional_value_cache_54) {
                const auto &operand_48 = compute_conditional_value_53();
                conditional_value_cache_54.emplace(operand_48.writes);
              }
              return *conditional_value_cache_54;
            };
            auto compute_conditional_value_55 = [&]() -> bool {
              const auto &operand_49 = compute_conditional_value_54();
              const auto &operand_50 = true;
              return (operand_49 != operand_50);
            };
            std::optional<bool> conditional_value_cache_56;
            auto compute_conditional_value_56 = [&]() -> const bool & {
              if (!conditional_value_cache_56) {
                const auto &operand_51 = compute_conditional_value_55();
                conditional_value_cache_56.emplace((operand_51 || compute_conditional_value_42()));
              }
              return *conditional_value_cache_56;
            };
            std::optional<bool> conditional_value_cache_57;
            auto compute_conditional_value_57 = [&]() -> const bool & {
              if (!conditional_value_cache_57) {
                const auto &operand_52 = compute_conditional_value_50();
                conditional_value_cache_57.emplace((operand_52 && compute_conditional_value_56()));
              }
              return *conditional_value_cache_57;
            };
            auto compute_conditional_value_58 = [&]() -> bool {
              const auto &operand_53 = compute_conditional_value_49();
              return (operand_53 || compute_conditional_value_57());
            };
            auto compute_conditional_value_59 = [&]() -> bool {
              const auto &operand_54 = compute_conditional_value_52();
              return (operand_54 && compute_conditional_value_58());
            };
            std::optional<uint32_t> conditional_value_cache_60;
            auto compute_conditional_value_60 = [&]() -> const uint32_t & {
              if (!conditional_value_cache_60) {
                const auto &operand_55 = compute_conditional_value_53();
                conditional_value_cache_60.emplace(operand_55.rd);
              }
              return *conditional_value_cache_60;
            };
            auto compute_conditional_value_61 = [&]() -> bool {
              const auto &operand_56 = compute_conditional_value_60();
              const auto &operand_57 = compute_index();
              return (operand_56==operand_57);
            };
            auto compute_conditional_value_62 = [&]() -> bool {
              const auto &operand_58 = compute_conditional_value_54();
              return (operand_58 && compute_conditional_value_61());
            };
            auto compute_conditional_value_63 = [&]() -> bool {
              const auto &operand_59 = compute_conditional_value_62();
              const auto &operand_60 = true;
              return (operand_59 != operand_60);
            };
            auto compute_conditional_value_64 = [&]() -> bool {
              const auto &operand_61 = compute_conditional_value_48();
              return (operand_61 && compute_conditional_value_63());
            };
            std::optional<bool> conditional_value_cache_65;
            auto compute_conditional_value_65 = [&]() -> const bool & {
              if (!conditional_value_cache_65) {
                const auto &operand_62 = compute_conditional_value_64();
                const auto &operand_63 = true;
                conditional_value_cache_65.emplace((operand_62 != operand_63));
              }
              return *conditional_value_cache_65;
            };
            auto compute_conditional_value_66 = [&]() -> bool {
              const auto &operand_64 = compute_conditional_value_65();
              return (operand_64 || compute_conditional_value_57());
            };
            auto compute_conditional_value_67 = [&]() -> uint64_t {
              const auto &operand_65 = compute_index();
              return uint64_t(operand_65);
            };
            auto compute_conditional_value_68 = [&]() -> bool {
              const auto &operand_66 = compute_conditional_value_67();
              const auto &operand_67 = uint64_t(32ULL);
              return (operand_66<operand_67);
            };
            auto compute_conditional_value_69 = [&]() -> bool {
              const auto &operand_68 = compute_conditional_value_42();
              return (operand_68 && compute_conditional_value_68());
            };
            auto compute_conditional_value_70 = [&]() -> bool {
              const auto &operand_69 = compute_conditional_value_66();
              return (operand_69 && compute_conditional_value_69());
            };
            auto compute_conditional_value_71 = [&]() -> bool {
              const auto &operand_70 = compute_conditional_value_65();
              return (operand_70 || compute_conditional_value_70());
            };
            auto compute_conditional_value_72 = [&]() -> bool {
              const auto &operand_71 = compute_conditional_value_59();
              return (operand_71 && compute_conditional_value_71());
            };
            auto compute_conditional_value_73 = [&]() -> bool {
              const auto &operand_72 = compute_index();
              const auto &operand_73 = 0u;
              return (operand_72==operand_73);
            };
            auto compute_conditional_value_74 = [&]() -> bool {
              const auto &operand_74 = compute_conditional_value_73();
              return (operand_74 || compute_conditional_value_56());
            };
            auto compute_conditional_value_75 = [&]() -> bool {
              const auto &operand_75 = compute_conditional_value_42();
              return (operand_75 && compute_conditional_value_74());
            };
            auto compute_conditional_value_76 = [&]() -> bool {
              const auto &operand_76 = compute_conditional_value_40();
              return (operand_76 && compute_conditional_value_75());
            };
            auto compute_conditional_value_77 = [&]() -> bool {
              const auto &operand_77 = compute_conditional_value_38();
              return (operand_77 || compute_conditional_value_76());
            };
            auto compute_conditional_value_78 = [&]() -> bool {
              const auto &operand_78 = compute_conditional_value_72();
              return (operand_78 && compute_conditional_value_77());
            };
            auto compute_conditional_value_79 = [&]() -> bool {
              const auto &operand_79 = compute_conditional_value_78();
              return (operand_79 && compute_conditional_value_44());
            };
            std::optional<uint32_t> index_cache_2;
            auto compute_index_2 = [&]() -> const uint32_t & {
              if (!index_cache_2) {
                const auto &operand_80 = compute_conditional_value_46();
                index_cache_2.emplace(operand_80.rs2);
              }
              return *index_cache_2;
            };
            auto compute_conditional_value_80 = [&]() -> bool {
              const auto &operand_81 = compute_index_2();
              const auto &operand_82 = 0u;
              return (operand_81!=operand_82);
            };
            std::optional<bool> conditional_value_cache_81;
            auto compute_conditional_value_81 = [&]() -> const bool & {
              if (!conditional_value_cache_81) {
                const auto &operand_83 = compute_conditional_value_37();
                conditional_value_cache_81.emplace((operand_83 && compute_conditional_value_80()));
              }
              return *conditional_value_cache_81;
            };
            std::optional<bool> conditional_value_cache_82;
            auto compute_conditional_value_82 = [&]() -> const bool & {
              if (!conditional_value_cache_82) {
                const auto &operand_84 = compute_conditional_value_81();
                const auto &operand_85 = true;
                conditional_value_cache_82.emplace((operand_84 != operand_85));
              }
              return *conditional_value_cache_82;
            };
            std::optional<bool> conditional_value_cache_83;
            auto compute_conditional_value_83 = [&]() -> const bool & {
              if (!conditional_value_cache_83) {
                const auto &operand_86 = compute_conditional_value_82();
                conditional_value_cache_83.emplace((operand_86 || compute_conditional_value_43()));
              }
              return *conditional_value_cache_83;
            };
            auto compute_conditional_value_84 = [&]() -> bool {
              const auto &operand_87 = compute_conditional_value_82();
              return (operand_87 || compute_conditional_value_83());
            };
            auto compute_conditional_value_85 = [&]() -> bool {
              const auto &operand_88 = compute_conditional_value_79();
              return (operand_88 && compute_conditional_value_84());
            };
            std::optional<bool> conditional_value_cache_86;
            auto compute_conditional_value_86 = [&]() -> const bool & {
              if (!conditional_value_cache_86) {
                const auto &operand_89 = compute_conditional_value_83();
                conditional_value_cache_86.emplace((operand_89 && compute_conditional_value_56()));
              }
              return *conditional_value_cache_86;
            };
            auto compute_conditional_value_87 = [&]() -> bool {
              const auto &operand_90 = compute_conditional_value_82();
              return (operand_90 || compute_conditional_value_86());
            };
            auto compute_conditional_value_88 = [&]() -> bool {
              const auto &operand_91 = compute_conditional_value_85();
              return (operand_91 && compute_conditional_value_87());
            };
            auto compute_conditional_value_89 = [&]() -> bool {
              const auto &operand_92 = compute_conditional_value_60();
              const auto &operand_93 = compute_index_2();
              return (operand_92==operand_93);
            };
            auto compute_conditional_value_90 = [&]() -> bool {
              const auto &operand_94 = compute_conditional_value_54();
              return (operand_94 && compute_conditional_value_89());
            };
            auto compute_conditional_value_91 = [&]() -> bool {
              const auto &operand_95 = compute_conditional_value_90();
              const auto &operand_96 = true;
              return (operand_95 != operand_96);
            };
            auto compute_conditional_value_92 = [&]() -> bool {
              const auto &operand_97 = compute_conditional_value_81();
              return (operand_97 && compute_conditional_value_91());
            };
            std::optional<bool> conditional_value_cache_93;
            auto compute_conditional_value_93 = [&]() -> const bool & {
              if (!conditional_value_cache_93) {
                const auto &operand_98 = compute_conditional_value_92();
                const auto &operand_99 = true;
                conditional_value_cache_93.emplace((operand_98 != operand_99));
              }
              return *conditional_value_cache_93;
            };
            auto compute_conditional_value_94 = [&]() -> bool {
              const auto &operand_100 = compute_conditional_value_93();
              return (operand_100 || compute_conditional_value_86());
            };
            auto compute_conditional_value_95 = [&]() -> uint64_t {
              const auto &operand_101 = compute_index_2();
              return uint64_t(operand_101);
            };
            auto compute_conditional_value_96 = [&]() -> bool {
              const auto &operand_102 = compute_conditional_value_95();
              const auto &operand_103 = uint64_t(32ULL);
              return (operand_102<operand_103);
            };
            auto compute_conditional_value_97 = [&]() -> bool {
              const auto &operand_104 = compute_conditional_value_42();
              return (operand_104 && compute_conditional_value_96());
            };
            auto compute_conditional_value_98 = [&]() -> bool {
              const auto &operand_105 = compute_conditional_value_94();
              return (operand_105 && compute_conditional_value_97());
            };
            auto compute_conditional_value_99 = [&]() -> bool {
              const auto &operand_106 = compute_conditional_value_93();
              return (operand_106 || compute_conditional_value_98());
            };
            auto compute_conditional_value_100 = [&]() -> bool {
              const auto &operand_107 = compute_conditional_value_88();
              return (operand_107 && compute_conditional_value_99());
            };
            auto compute_conditional_value_101 = [&]() -> bool {
              const auto &operand_108 = compute_index_2();
              const auto &operand_109 = 0u;
              return (operand_108==operand_109);
            };
            auto compute_conditional_value_102 = [&]() -> bool {
              const auto &operand_110 = compute_conditional_value_101();
              return (operand_110 || compute_conditional_value_56());
            };
            auto compute_conditional_value_103 = [&]() -> bool {
              const auto &operand_111 = compute_conditional_value_42();
              return (operand_111 && compute_conditional_value_102());
            };
            auto compute_conditional_value_104 = [&]() -> bool {
              const auto &operand_112 = compute_conditional_value_40();
              return (operand_112 && compute_conditional_value_103());
            };
            auto compute_conditional_value_105 = [&]() -> bool {
              const auto &operand_113 = compute_conditional_value_38();
              return (operand_113 || compute_conditional_value_104());
            };
            auto compute_conditional_value_106 = [&]() -> bool {
              const auto &operand_114 = compute_conditional_value_100();
              return (operand_114 && compute_conditional_value_105());
            };
            const bool conditional_result = compute_conditional_value_106();
            if (!(conditional_result)) return;
            // Prepare every effect before publishing state operations.
            const bool ready = std::get<0>(ctx.compute_shared_computation_38());
            const bool condition = std::get<0>(ctx.compute_shared_computation_39());
            const bool ready_2 = std::get<0>(ctx.compute_shared_computation_42());
            const bool ready_3 = std::get<0>(ctx.compute_shared_computation_74());
            const bool ready_4 = std::get<0>(ctx.compute_shared_computation());
            const bool ready_5 = std::get<0>(ctx.compute_shared_computation_2());
            const bool condition_2 = std::get<0>(ctx.compute_shared_computation_4());
            const bool condition_path = (ready ? (ready_4 && (ready_5 && (condition_2 && (condition ? (ready_2 && ready_3) : true)))) : (ready_4 && (ready_5 && condition_2)));
            bool condition_3{};
            if (condition_path) {
              condition_3 = std::get<0>(ctx.compute_shared_computation_6());
            }
            const uint32_t rd = std::get<0>(ctx.compute_shared_computation_3());
            const bool condition_path_2 = (ready_4 && (ready_5 && (condition_2 && (condition_3 && (condition ? (ready_2 && ready_3) : true)))));
            uint32_t rs2{};
            if ((ready ? condition_path_2 : (ready_4 && (ready_5 && (condition_2 && condition_3))))) {
              rs2 = std::get<0>(ctx.compute_shared_computation_5());
            }
            const bool ready_6 = std::get<0>(ctx.compute_shared_computation_9());
            const bool ready_7 = std::get<0>(ctx.compute_shared_computation_23());
            const bool ready_8 = std::get<0>(ctx.compute_shared_computation_44());
            const bool ready_9 = std::get<0>(ctx.compute_shared_computation_67());
            const bool ready_10 = std::get<0>(ctx.compute_shared_computation_46());
            const uint32_t system = std::get<0>(ctx.compute_shared_computation_47());
            const bool condition_6 = std::get<0>(ctx.compute_shared_computation_53());
            const uint32_t computed_value = std::get<0>(ctx.compute_shared_computation_55());
            const uint32_t computed_value_2 = std::get<0>(ctx.compute_shared_computation_58());
            const bool condition_9 = std::get<0>(ctx.compute_shared_computation_62());
            const bool condition_10 = std::get<0>(ctx.compute_shared_computation_63());
            const bool condition_11 = std::get<0>(ctx.compute_shared_computation_64());
            const bool condition_12 = std::get<0>(ctx.compute_shared_computation_65());
            const bool condition_13 = std::get<0>(ctx.compute_shared_computation_66());
            const bool ready_16 = ((computed_value_2!=93u) && (condition_9 || (condition_10 || (condition_11 || (condition_12 || condition_13)))));
            const bool ready_21 = (ready_9 || ((!ready_10) && ((system!=2u) && (condition_6 || ((computed_value!=10u) && ready_16)))));
            const bool ready_22 = (ready_8 && ready_21);
            const uint32_t opcode = std::get<0>(ctx.compute_opcode());
            const bool condition_15 = std::get<0>(ctx.compute_shared_computation_16());
            const bool condition_16 = std::get<0>(ctx.compute_shared_computation_14());
            const bool condition_17 = std::get<0>(ctx.compute_shared_computation_17());
            const bool condition_18 = std::get<0>(ctx.compute_shared_computation_18());
            const bool condition_19 = std::get<0>(ctx.compute_shared_computation_20());
            const bool condition_20 = std::get<0>(ctx.compute_shared_computation_19());
            const bool condition_21 = std::get<0>(ctx.compute_shared_computation_21());
            const bool condition_22 = std::get<0>(ctx.compute_shared_computation_28());
            const bool condition_23 = std::get<0>(ctx.compute_shared_computation_30());
            const bool condition_24 = std::get<0>(ctx.compute_shared_computation_26());
            const bool selected_value_2 = (condition_20 ? (condition_21 ? (condition_22 && ready_22) : (condition_23 && ready_22)) : (condition_24 && ready_22));
            const bool condition_25 = std::get<0>(ctx.compute_shared_computation_27());
            const bool condition_26 = std::get<0>(ctx.compute_shared_computation_24());
            const bool selected_value_4 = (condition_18 ? (condition_19 ? selected_value_2 : (condition_25 && ready_22)) : (condition_26 && ready_22));
            const bool condition_27 = std::get<0>(ctx.compute_shared_computation_25());
            const bool ready_29 = (condition_15 && (condition_16 ? (condition_17 ? selected_value_4 : (condition_27 && ready_22)) : ready_22));
            const bool selected_value_8 = (ready_6 ? (ready_7 ? ready_22 : ((opcode!=111u) && ready_29)) : ready_22);
            const bool selected_value_10 = (ready_5 ? (condition_2 ? (condition_3 && ((rd!=rs2) && selected_value_8)) : selected_value_8) : selected_value_8);
            const bool selected_value_11 = (ready_4 ? selected_value_10 : selected_value_8);
            const bool selected_value_13 = (ready ? (condition ? (ready_2 && (ready_3 && selected_value_11)) : selected_value_11) : selected_value_11);
            const bool check_fire = selected_value_13;
            if (check_fire) {
            }
            const bool check_fire_2 = selected_value_13;
            if (check_fire_2) {
            }
            const bool ready_36 = std::get<0>(ctx.compute_shared_computation_69());
            const bool wait = std::get<0>(ctx.compute_wait());
            bool ready_37{};
            if ((ready_36 && (!wait))) {
              ready_37 = std::get<0>(ctx.compute_shared_computation_73());
            }
            const bool ready_38 = std::get<0>(ctx.compute_shared_computation_51());
            bool stall{};
            if ((ready_36 && (wait ? ready_38 : ready_37))) {
              stall = std::get<0>(ctx.compute_stall());
            }
            bool ready_39{};
            if ((ready_36 && (wait ? false : (ready_37 && (!stall))))) {
              ready_39 = std::get<0>(ctx.compute_shared_computation_36());
            }
            const bool ready_41 = (ready_37 && (stall || ready_39));
            const bool redirect = std::get<0>(ctx.compute_redirect());
            const bool ready_42 = (stall || redirect);
            bool ready_43{};
            if ((ready_36 && (wait ? false : (ready_37 && (stall ? false : (ready_39 && (!redirect))))))) {
              ready_43 = std::get<0>(ctx.compute_shared_computation_71());
            }
            const bool ready_45 = (ready_41 && (ready_42 || ready_43));
            const bool condition_path_3 = (ready_36 && (wait ? false : (ready_37 && (stall ? false : (ready_39 && (redirect ? false : ready_43))))));
            bool ready_48{};
            if (condition_path_3) {
              ready_48 = std::get<0>(ctx.compute_shared_computation_70());
            }
            const bool condition_path_4 = (ready_36 && (wait ? false : (ready_37 && (stall || (ready_39 && (redirect || (ready_43 && ready_48)))))));
            bool ready_52{};
            if (condition_path_4) {
              ready_52 = std::get<0>(ctx.compute_shared_computation_35());
            }
            const bool ready_53 = (((ready_42 || ((!ready_42) && ready_48)) && ready_41) && ready_52);
            const bool ready_54 = ((ready_45 && ready_36) && ready_53);
            const bool stopping = std::get<0>(ctx.compute_stopping());
            const bool ready_55 = (ready_42 || stopping);
            const bool selected_value_14 = (ready_55 ? false : ready_38);
            const bool instruction_ready = ctx.top.cpu.decode_stage.instruction.valid;
            const Instruction instruction_value = ctx.top.cpu.decode_stage.instruction.read();
            const uint32_t index = instruction_value.rs1;
            const bool condition_28 = (index!=0u);
            const Completed mem_wb_value = ctx.top.cpu.mem_wb.read();
            const bool ready_56 = mem_wb_value.writes;
            const bool ready_58 = ((!ready_56) || instruction_ready);
            const uint32_t rd_2 = mem_wb_value.rd;
            const bool ready_59 = (ready_56 && (rd_2==index));
            const uint64_t computed_value_3 = uint64_t(index);
            const bool ready_60 = (instruction_ready && (computed_value_3<uint64_t(32ULL)));
            const bool condition_31 = (index==0u);
            const bool ready_65 = (instruction_ready && (condition_31 || ready_58));
            const bool ready_66 = ((instruction_ready && (condition_28 ? (ready_58 && (ready_59 || ready_60)) : instruction_ready)) && ready_65);
            const uint32_t index_2 = instruction_value.rs2;
            const bool condition_32 = (index_2!=0u);
            const bool ready_72 = (ready_56 && (rd_2==index_2));
            const uint64_t computed_value_4 = uint64_t(index_2);
            const bool ready_73 = (instruction_ready && (computed_value_4<uint64_t(32ULL)));
            const bool condition_35 = (index_2==0u);
            const bool ready_78 = (instruction_ready && (condition_35 || ready_58));
            const bool ready_79 = ((instruction_ready && (condition_32 ? (ready_58 && (ready_72 || ready_73)) : instruction_ready)) && ready_78);
            const bool ready_85 = ((ready_66 && ((((condition_28 && ready_58) || condition_31) && instruction_ready) && ready_65)) && (ready_79 && ((((condition_32 && ready_58) || condition_35) && instruction_ready) && ready_78)));
            const bool ready_86 = (!ready_55);
            const bool ready_91 = (ready_36 && (wait || (ready_54 && (selected_value_14 ? ready_85 : (ready_45 && (ready_86 || ready_37))))));
            const bool ready_93 = (ready_36 && (wait || ready_54));
            const bool condition_path_5 = (redirect ? (ready_52 && ready_38) : (ready_43 && (ready_48 && (ready_52 && (stopping && ready_38)))));
            const bool condition_path_6 = (ready_36 && (wait ? false : (ready_37 && (stall ? (ready_52 && ready_38) : (ready_39 && condition_path_5)))));
            bool ready_98{};
            if (condition_path_6) {
              ready_98 = std::get<0>(ctx.compute_shared_computation_72());
            }
            const bool ready_104 = ((selected_value_14 || ((!selected_value_14) && (((ready_55 && ready_98) || ready_86) && ready_45))) && ready_54);
            const bool ready_112 = (((ready_91 && ready_93) && (ready_38 && (selected_value_14 ? true : ready_55))) && (((wait || (ready_38 && ready_104)) && ready_36) && ((wait || (ready_38 && ready_53)) && ready_36)));
            const bool write_fire = (ready_112 && ready_93);
            Decoded write_value{};
            if (write_fire) {
              const Decoded payload = Decoded{};
              bool stall_2{};
              if ((!wait)) {
                stall_2 = std::get<0>(ctx.compute_stall());
              }
              const bool ready_115 = (stall_2 || redirect);
              const bool ready_116 = (ready_115 || stopping);
              const bool selected_value_19 = (ready_116 ? false : ready_38);
              const Fetched if_id_value = ctx.top.cpu.if_id.read();
              const Instruction decoded = instruction_value;
              const uint32_t value = mem_wb_value.value;
              const uint32_t registers_value = ((computed_value_3 < 32) ? ctx.top.cpu.rf.registers.at(computed_value_3).read() : uint32_t{});
              const uint32_t payload_2 = std::get<0>(ctx.compute_shared_computation_7());
              const uint32_t lhs = (condition_28 ? (ready_59 ? value : registers_value) : (condition_31 ? 0u : payload_2));
              const uint32_t registers_value_2 = ((computed_value_4 < 32) ? ctx.top.cpu.rf.registers.at(computed_value_4).read() : uint32_t{});
              const uint32_t rhs = (condition_32 ? (ready_72 ? value : registers_value_2) : (condition_35 ? 0u : payload_2));
              const Decoded payload_3 = Decoded{if_id_value.valid,if_id_value.pc,if_id_value.word,decoded,lhs,rhs,false,if_id_value.next_pc,if_id_value.expanded,if_id_value.fetch_fault};
              const Decoded bubble = Decoded{false,0u,0u,Instruction{},0u,0u,stall_2,0u,0u,false};
              write_value = (wait ? payload : (selected_value_19 ? payload_3 : (ready_116 ? bubble : payload)));
            }
            if (write_fire) { ctx.top.cpu.id_ex.write(write_value); }
            ctx.fired_top_cpu_decode_stage_advance=true;
          }
        } decode_stage;
        struct Execute {
          // Rule top.cpu.execute.advance#3
          template<class Context> void try_advance(Context &ctx) {
            const bool ready = std::get<0>(ctx.compute_shared_computation_32());
            bool ready_2{};
            if (ready) {
              ready_2 = std::get<0>(ctx.compute_shared_computation_31());
            }
            bool ready_4{};
            if ((ready && ready_2)) {
              ready_4 = std::get<0>(ctx.compute_shared_computation_12());
            }
            bool ready_6{};
            if ((ready && (ready_2 && ready_4))) {
              ready_6 = std::get<0>(ctx.compute_shared_computation_34());
            }
            bool ready_8{};
            if ((ready && (ready_2 && (ready_4 && ready_6)))) {
              ready_8 = std::get<0>(ctx.compute_shared_computation_35());
            }
            bool ready_10{};
            if ((ready && (ready_2 && (ready_4 && (ready_6 && ready_8))))) {
              ready_10 = std::get<0>(ctx.compute_shared_computation_68());
            }
            bool ready_12{};
            if ((ready && (ready_2 && (ready_4 && (ready_6 && (ready_8 && ready_10)))))) {
              ready_12 = std::get<0>(ctx.compute_shared_computation_69());
            }
            if (!(((((((ready && ready_2) && ready_4) && ready_6) && ready_8) && ready_10) && ready_12))) return;
            // Prepare every effect before publishing state operations.
            const bool ready_14 = std::get<0>(ctx.compute_shared_computation_9());
            const bool ready_15 = std::get<0>(ctx.compute_shared_computation_23());
            const bool ready_16 = std::get<0>(ctx.compute_shared_computation_10());
            const bool condition = std::get<0>(ctx.compute_shared_computation_13());
            const bool condition_2 = std::get<0>(ctx.compute_shared_computation_15());
            const bool condition_3 = std::get<0>(ctx.compute_shared_computation_14());
            const bool condition_4 = std::get<0>(ctx.compute_shared_computation_17());
            const bool condition_5 = std::get<0>(ctx.compute_shared_computation_18());
            const bool condition_6 = std::get<0>(ctx.compute_shared_computation_20());
            const bool condition_7 = std::get<0>(ctx.compute_shared_computation_19());
            const bool condition_8 = std::get<0>(ctx.compute_shared_computation_21());
            const bool condition_path = (condition_2 ? false : (condition_3 && (condition_4 && (condition_5 && (condition_6 && (condition_7 && condition_8))))));
            bool ready_17{};
            if ((ready_14 && (ready_15 ? false : (condition ? false : condition_path)))) {
              ready_17 = std::get<0>(ctx.compute_shared_computation_29());
            }
            const bool condition_path_2 = (condition_4 ? (condition_5 ? (condition_6 ? (condition_7 ? (condition_8 ? ready_17 : true) : true) : true) : true) : true);
            const bool condition_path_3 = (condition ? ready_16 : (condition_2 ? ready_16 : (condition_3 ? (ready_16 && condition_path_2) : ready_16)));
            bool ready_18{};
            if ((ready_14 && (ready_15 ? ready_16 : condition_path_3))) {
              ready_18 = std::get<0>(ctx.compute_shared_computation_11());
            }
            const bool ready_19 = std::get<0>(ctx.compute_shared_computation_38());
            const bool condition_9 = std::get<0>(ctx.compute_shared_computation_39());
            const bool ready_20 = std::get<0>(ctx.compute_shared_computation_41());
            const bool ready_21 = std::get<0>(ctx.compute_shared_computation_43());
            const bool condition_path_4 = (condition_4 ? (condition_5 ? (condition_6 ? (condition_7 ? (condition_8 ? ready_17 : true) : true) : true) : true) : true);
            const bool condition_path_5 = (condition_3 ? (ready_16 ? (ready_18 && condition_path_4) : condition_path_4) : (ready_16 ? ready_18 : true));
            const bool condition_path_6 = (condition ? (ready_16 ? ready_18 : true) : (condition_2 ? (ready_16 ? ready_18 : true) : condition_path_5));
            const bool condition_path_7 = (ready_20 ? false : (ready_21 ? false : (ready_15 ? (ready_16 ? ready_18 : true) : condition_path_6)));
            const bool condition_path_8 = (ready_19 ? (condition_9 ? condition_path_7 : (ready_15 ? (ready_16 ? ready_18 : true) : condition_path_6)) : (ready_15 ? (ready_16 ? ready_18 : true) : condition_path_6));
            const bool condition_path_9 = (ready_14 ? condition_path_8 : (ready_19 ? (condition_9 ? (ready_20 ? false : (!ready_21)) : true) : true));
            bool ready_22{};
            if (condition_path_9) {
              ready_22 = std::get<0>(ctx.compute_shared_computation_22());
            }
            const bool selected_value_2 = (ready_19 ? (condition_9 ? (ready_20 || (ready_21 || ready_22)) : ready_22) : ready_22);
            const bool selected_value_3 = (ready_16 ? (ready_18 && selected_value_2) : selected_value_2);
            const bool selected_value_5 = (condition_7 ? (condition_8 ? (ready_17 && selected_value_3) : selected_value_3) : selected_value_3);
            const bool selected_value_8 = (condition_4 ? (condition_5 ? (condition_6 ? selected_value_5 : selected_value_3) : selected_value_3) : selected_value_3);
            const bool selected_value_11 = (condition ? selected_value_3 : (condition_2 ? selected_value_3 : (condition_3 ? selected_value_8 : selected_value_3)));
            const bool write_fire = (ready_14 ? (ready_15 ? selected_value_3 : selected_value_11) : selected_value_2);
            Executed write_value{};
            if (write_fire) {
              const bool wait = std::get<0>(ctx.compute_wait());
              const Executed bubble = Executed{false,0u,0u,0u,false,false,false,0u,0u,0u,true,0u,false,0u,0u,0u};
              const uint32_t pc = std::get<0>(ctx.compute_pc());
              const Decoded id_ex_value = std::get<0>(ctx.compute_id_ex_view_3());
              const Instruction executing = std::get<0>(ctx.compute_executing());
              uint32_t result{};
              if ((!wait)) {
                result = std::get<0>(ctx.compute_result());
              }
              const uint32_t rhs = std::get<0>(ctx.compute_rhs());
              const bool jump = std::get<0>(ctx.compute_jump());
              const uint32_t next_pc = id_ex_value.next_pc;
              const uint32_t value = (jump ? next_pc : result);
              const uint32_t function = std::get<0>(ctx.compute_function());
              const Executed next_packet = Executed{ready_14,pc,id_ex_value.word,executing.rd,executing.writes,executing.load,executing.store,result,rhs,value,id_ex_value.stalled,function,ready_15,executing.system,id_ex_value.expanded,next_pc};
              const Executed packet = next_packet;
              write_value = (wait ? bubble : packet);
            }
            if (write_fire) { ctx.top.cpu.ex_mem.write(write_value); }
            ctx.fired_top_cpu_execute_advance=true;
          }
        } execute;
        struct Fetch {
          Cell<uint32_t> pc{};
          Cell<std::array<uint32_t,1024>> rom{};
          // Rule top.cpu.fetch.advance#0
          template<class Context> void try_advance(Context &ctx) {
            const bool ready = std::get<0>(ctx.compute_shared_computation_68());
            bool ready_2{};
            if (ready) {
              ready_2 = std::get<0>(ctx.compute_shared_computation_71());
            }
            const bool stopping = std::get<0>(ctx.compute_stopping());
            const bool ready_4 = (!stopping);
            const bool ready_5 = (ready_4 || ready_2);
            const uint32_t pc_value = ctx.top.cpu.fetch.pc.read();
            const bool condition = (pc_value<4096u);
            const bool ready_8 = (stopping && condition);
            const bool ready_9 = (!ready_8);
            const bool ready_10 = (ready_9 || ready_5);
            const bool ready_11 = (ready_9 || ready_10);
            const uint32_t pc_value_2 = ctx.top.cpu.fetch.pc.read();
            const bool condition_2 = ((pc_value_2 & 2u)!=0u);
            const bool ready_14 = (!(ready_8 && condition_2));
            const bool ready_17 = ((((ready && ready_2) && (ready_4 || ready_5)) && ready_11) && (ready_14 || (ready_14 || ready_10)));
            const uint32_t pc_value_3 = ctx.top.cpu.fetch.pc.read();
            const bool ready_19 = (condition_2 && ((pc_value_3 + 3u)<4096u));
            const bool ready_21 = (!(ready_8 && ready_19));
            const uint64_t computed_value_4 = uint64_t((pc_value_2 >> 2u));
            const bool condition_4 = (computed_value_4<uint64_t(1024ULL));
            const uint32_t pc_value_4 = ctx.top.cpu.fetch.pc.read();
            const uint64_t computed_value_7 = uint64_t(((pc_value_4 >> 2u) + 1u));
            const bool condition_5 = (computed_value_7<uint64_t(1024ULL));
            const bool selected_value = (ready_19 ? (condition_4 && condition_5) : condition_4);
            const bool ready_28 = (((ready_17 && ready_11) && (ready_21 || (ready_21 || ready_10))) && (ready_9 || (ready_10 && selected_value)));
            const std::array<uint32_t,1024> rom_value = ctx.top.cpu.fetch.rom.read();
            const bool condition_path = (condition_2 ? (((pc_value_3 + 3u)<4096u) ? (condition_4 && condition_5) : condition_4) : condition_4);
            uint32_t raw{};
            if ((ready && (ready_2 && (stopping && (condition && condition_path))))) {
              raw = acir_sim::shift(rom_value.at((condition_4 ? computed_value_4 : uint64_t(0ULL))),((pc_value_2 & 3u) * 8u),1);
            }
            const std::array<uint32_t,1024> rom_value_2 = ctx.top.cpu.fetch.rom.read();
            const uint32_t raw_2 = (raw | (rom_value_2.at((condition_5 ? computed_value_7 : uint64_t(0ULL))) << 16u));
            const uint32_t raw_3 = (ready_19 ? raw_2 : raw);
            const bool condition_6 = (raw_3==0u);
            const bool ready_34 = ((ready_28 && ready_11) && (ready_9 || (ready_10 && (selected_value && (condition_6 || selected_value)))));
            const bool ready_35 = (stopping || ready_2);
            bool ready_36{};
            if ((ready && (ready_2 && (!stopping)))) {
              ready_36 = std::get<0>(ctx.compute_shared_computation_73());
            }
            bool stall{};
            if ((ready && (ready_2 && (stopping ? false : ready_36)))) {
              stall = std::get<0>(ctx.compute_stall());
            }
            bool ready_40{};
            if ((ready && (ready_2 && (stopping ? false : (ready_36 && (!stall)))))) {
              ready_40 = std::get<0>(ctx.compute_shared_computation_69());
            }
            const bool ready_42 = (ready_36 && (stall || ready_40));
            const bool ready_43 = (ready_35 && ready_42);
            const bool wait = std::get<0>(ctx.compute_wait());
            const bool ready_46 = (stall || wait);
            const bool ready_47 = (!ready_46);
            const bool ready_48 = (ready_4 && ready_47);
            const bool ready_49 = (!ready_48);
            bool ready_51{};
            if ((ready && (ready_2 && (stopping ? false : (ready_36 && (stall ? false : (ready_40 && (!wait)))))))) {
              ready_51 = std::get<0>(ctx.compute_shared_computation_36());
            }
            const bool ready_52 = ((ready_49 || ready_43) && ready_51);
            const bool ready_54 = (((ready_34 && (stopping || (ready_35 && ready_36))) && (stopping || ready_43)) && (ready_49 || ready_52));
            const bool redirect = std::get<0>(ctx.compute_redirect());
            const bool ready_55 = (!redirect);
            const bool ready_56 = (ready_48 && ready_55);
            const bool ready_57 = (!ready_56);
            const bool ready_58 = (ready_57 || ready_52);
            const bool ready_59 = (ready_57 || ready_58);
            const uint32_t pc_value_5 = ctx.top.cpu.fetch.pc.read();
            const bool ready_62 = (!(ready_56 && (pc_value_5<4096u)));
            const uint32_t pc_value_6 = ctx.top.cpu.fetch.pc.read();
            const bool ready_67 = ((pc_value_5>=4096u) || ((pc_value_6 & 1u)!=0u));
            const bool ready_68 = (!ready_67);
            const bool ready_69 = (ready_56 && ready_68);
            const bool ready_70 = (!ready_69);
            const bool ready_71 = (ready_70 || ready_58);
            const bool ready_73 = ((((ready_54 && ready_59) && (ready_62 || (ready_62 || ready_58))) && ready_59) && (ready_70 || ready_71));
            const uint32_t pc_value_7 = ctx.top.cpu.fetch.pc.read();
            const uint32_t computed_value_14 = (pc_value_7 & 2u);
            const uint64_t computed_value_16 = uint64_t((pc_value_7 >> 2u));
            const bool condition_11 = (computed_value_16<uint64_t(1024ULL));
            const bool ready_74 = ((computed_value_14==0u) || condition_11);
            const bool ready_75 = (ready_71 && ready_74);
            const bool ready_76 = (ready_70 || ready_75);
            const std::array<uint32_t,1024> rom_value_3 = ctx.top.cpu.fetch.rom.read();
            const bool condition_path_2 = (ready_51 && (redirect ? false : ((pc_value_5<4096u) && (((pc_value_6 & 1u)!=0u) ? false : condition_11))));
            const bool condition_path_3 = (ready_2 && (stopping ? false : (ready_36 && (stall ? false : (ready_40 && (wait ? false : condition_path_2))))));
            uint32_t raw_4{};
            if ((ready && condition_path_3)) {
              raw_4 = acir_sim::shift(rom_value_3.at((condition_11 ? computed_value_16 : uint64_t(0ULL))),((pc_value_7 & 3u) * 8u),1);
            }
            const bool ready_78 = ((computed_value_14!=0u) && ((raw_4 & 3u)==3u));
            const bool ready_79 = (ready_69 && ready_78);
            const bool ready_80 = (!ready_79);
            const bool ready_81 = (ready_80 || ready_75);
            const bool ready_82 = (ready_80 || ready_81);
            const uint32_t pc_value_8 = ctx.top.cpu.fetch.pc.read();
            const uint32_t computed_value_21 = (pc_value_8 + 3u);
            const bool condition_14 = (computed_value_21>=4096u);
            const bool ready_84 = (ready_79 && condition_14);
            const bool ready_85 = (!ready_84);
            const bool condition_15 = (computed_value_21<4096u);
            const bool ready_91 = (!(ready_79 && condition_15));
            const bool ready_94 = (((((ready_73 && ready_76) && ready_82) && (ready_85 || (ready_85 || ready_81))) && ready_82) && (ready_91 || (ready_91 || ready_81)));
            const bool ready_96 = (!ready_78);
            const bool ready_97 = (ready_96 || condition_15);
            const bool ready_98 = (ready_69 && ready_97);
            const bool ready_99 = (!ready_98);
            const bool ready_100 = (ready_99 || ready_75);
            const bool ready_101 = (ready_99 || ready_100);
            const uint32_t pc_value_9 = ctx.top.cpu.fetch.pc.read();
            const uint64_t computed_value_24 = uint64_t(((pc_value_9 >> 2u) + 1u));
            const bool condition_16 = (computed_value_24<uint64_t(1024ULL));
            const bool ready_103 = (ready_74 && (ready_78 ? (condition_15 ? (condition_11 && condition_16) : condition_11) : condition_11));
            const bool ready_104 = (ready_101 && ready_103);
            const bool ready_105 = (ready_99 || ready_104);
            const std::array<uint32_t,1024> rom_value_4 = ctx.top.cpu.fetch.rom.read();
            const uint32_t raw_5 = (raw_4 | (rom_value_4.at((condition_16 ? computed_value_24 : uint64_t(0ULL))) << 16u));
            const uint32_t raw_6 = (condition_15 ? raw_5 : raw_4);
            const uint32_t raw_7 = (ready_78 ? raw_6 : raw_4);
            const uint32_t computed_value_27 = (raw_7 & 3u);
            const bool condition_17 = (computed_value_27!=3u);
            const bool ready_107 = (ready_98 && condition_17);
            const bool ready_108 = (!ready_107);
            const bool ready_110 = ((ready_108 || ready_104) && ready_103);
            const uint32_t h = (raw_7 & 65535u);
            const uint32_t quadrant = (h & 3u);
            const bool condition_18 = (quadrant!=0u);
            const bool ready_113 = (ready_107 && condition_18);
            const bool ready_114 = (!ready_113);
            const bool ready_116 = ((ready_114 || ready_110) && ready_103);
            const bool condition_19 = (quadrant!=1u);
            const bool ready_119 = (ready_113 && condition_19);
            const bool ready_120 = (!ready_119);
            const bool ready_122 = ((ready_120 || ready_116) && ready_103);
            const bool ready_124 = (((((ready_94 && ready_76) && ready_105) && (ready_108 || ready_110)) && (ready_114 || ready_116)) && (ready_120 || ready_122));
            const bool condition_20 = (quadrant==2u);
            const bool ready_125 = (ready_119 && condition_20);
            const bool ready_126 = (!ready_125);
            const bool ready_128 = ((ready_126 || ready_122) && ready_103);
            const uint32_t function = (h >> 13u);
            const bool condition_21 = (function!=0u);
            const bool ready_131 = (ready_125 && condition_21);
            const bool ready_132 = (!ready_131);
            const bool ready_134 = ((ready_132 || ready_128) && ready_103);
            const bool condition_22 = (function!=2u);
            const bool ready_137 = (ready_131 && condition_22);
            const bool ready_138 = (!ready_137);
            const bool ready_140 = ((ready_138 || ready_134) && ready_103);
            const bool condition_23 = (function!=4u);
            const bool ready_144 = (!(ready_137 && condition_23));
            const bool ready_148 = ((((ready_124 && (ready_126 || ready_128)) && (ready_132 || ready_134)) && (ready_138 || ready_140)) && (ready_144 || ((ready_144 || ready_140) && ready_103)));
            const bool condition_24 = (function==4u);
            const bool ready_149 = (ready_137 && condition_24);
            const bool ready_150 = (!ready_149);
            const bool ready_152 = ((ready_150 || ready_140) && ready_103);
            const uint32_t computed_value_28 = (h & 4096u);
            const bool condition_25 = (computed_value_28!=0u);
            const bool ready_155 = (ready_149 && condition_25);
            const bool ready_156 = (!ready_155);
            const bool ready_158 = ((ready_156 || ready_152) && ready_103);
            const uint32_t computed_value_29 = (h >> 2u);
            const uint32_t rt = (computed_value_29 & 31u);
            const bool condition_26 = (rt==0u);
            const bool ready_162 = (!(ready_155 && condition_26));
            const bool ready_166 = (((ready_148 && (ready_150 || ready_152)) && (ready_156 || ready_158)) && (ready_162 || ((ready_162 || ready_158) && ready_103)));
            const bool condition_27 = (computed_value_28==0u);
            const bool ready_167 = (ready_149 && condition_27);
            const bool ready_168 = (!ready_167);
            const bool ready_170 = ((ready_168 || ready_152) && ready_103);
            const bool ready_174 = (!(ready_167 && condition_26));
            const bool condition_28 = (function==2u);
            const bool ready_180 = (!(ready_131 && condition_28));
            const bool ready_184 = (((ready_166 && (ready_168 || ready_170)) && (ready_174 || ((ready_174 || ready_170) && ready_103))) && (ready_180 || ((ready_180 || ready_134) && ready_103)));
            const bool condition_29 = (function==0u);
            const bool ready_186 = (!(ready_125 && condition_29));
            const bool condition_30 = (quadrant==1u);
            const bool ready_191 = (ready_113 && condition_30);
            const bool ready_192 = (!ready_191);
            const bool ready_194 = ((ready_192 || ready_116) && ready_103);
            const bool ready_197 = (ready_191 && condition_21);
            const bool ready_198 = (!ready_197);
            const bool ready_199 = (ready_198 || ready_194);
            const bool ready_202 = (((ready_184 && (ready_186 || ((ready_186 || ready_128) && ready_103))) && (ready_192 || ready_194)) && (ready_198 || (ready_199 && ready_103)));
            const bool condition_31 = (function==1u);
            const bool ready_204 = (ready_103 && (condition_31 || ready_103));
            const bool ready_205 = (ready_199 && ready_204);
            const bool ready_208 = (condition_31 || (function==5u));
            const bool ready_209 = (!ready_208);
            const bool ready_210 = (ready_197 && ready_209);
            const bool ready_211 = (!ready_210);
            const bool ready_213 = ((ready_211 || ready_205) && ready_103);
            const bool ready_216 = (ready_210 && condition_22);
            const bool ready_217 = (!ready_216);
            const bool ready_219 = ((ready_217 || ready_213) && ready_103);
            const bool condition_33 = (function!=3u);
            const bool ready_222 = (ready_216 && condition_33);
            const bool ready_223 = (!ready_222);
            const bool ready_225 = ((ready_223 || ready_219) && ready_103);
            const bool ready_227 = ((((ready_202 && (ready_198 || ready_205)) && (ready_211 || ready_213)) && (ready_217 || ready_219)) && (ready_223 || ready_225));
            const bool ready_229 = (!(ready_222 && condition_23));
            const bool ready_230 = (ready_229 || ready_225);
            const bool condition_34 = (function==6u);
            const bool ready_235 = (ready_103 && (condition_34 || ready_103));
            const bool ready_239 = (ready_222 && condition_24);
            const bool ready_240 = (!ready_239);
            const bool ready_241 = (ready_240 || ready_225);
            const bool ready_244 = (((ready_227 && (ready_229 || (ready_230 && ready_103))) && (ready_229 || (ready_230 && ready_235))) && (ready_240 || (ready_241 && ready_103)));
            const uint32_t computed_value_30 = (h >> 10u);
            const uint32_t computed_value_31 = (computed_value_30 & 3u);
            const bool condition_35 = (computed_value_31==0u);
            const bool ready_246 = (ready_103 && (condition_35 || ready_103));
            const bool ready_247 = (ready_241 && ready_246);
            const bool condition_36 = (computed_value_31==1u);
            const bool ready_250 = (condition_35 || condition_36);
            const bool ready_251 = (!ready_250);
            const bool ready_252 = (ready_239 && ready_251);
            const bool ready_253 = (!ready_252);
            const bool ready_255 = ((ready_253 || ready_247) && ready_103);
            const bool condition_37 = (computed_value_31!=2u);
            const bool ready_258 = (ready_252 && condition_37);
            const bool ready_259 = (!ready_258);
            const bool ready_261 = ((ready_259 || ready_255) && ready_103);
            const bool ready_264 = (ready_258 && condition_27);
            const bool ready_265 = (!ready_264);
            const bool ready_267 = ((ready_265 || ready_261) && ready_103);
            const bool ready_269 = ((((ready_244 && (ready_240 || ready_247)) && (ready_253 || ready_255)) && (ready_259 || ready_261)) && (ready_265 || ready_267));
            const uint32_t computed_value_32 = (h >> 5u);
            const uint32_t computed_value_33 = (computed_value_32 & 3u);
            const bool ready_271 = (!(ready_264 && (computed_value_33!=0u)));
            const bool ready_277 = (!(ready_239 && ready_250));
            const bool ready_281 = ((ready_269 && (ready_271 || ((ready_271 || ready_267) && ready_103))) && (ready_277 || ((ready_277 || ready_247) && ready_103)));
            const bool condition_39 = (function==3u);
            const bool ready_282 = (ready_216 && condition_39);
            const bool ready_283 = (!ready_282);
            const bool ready_285 = ((ready_283 || ready_219) && ready_103);
            const uint32_t computed_value_34 = (h >> 7u);
            const uint32_t rd = (computed_value_34 & 31u);
            const bool condition_40 = (rd!=2u);
            const bool ready_289 = (!(ready_282 && condition_40));
            const bool condition_41 = (rd==2u);
            const bool ready_295 = (!(ready_282 && condition_41));
            const bool ready_299 = (((ready_281 && (ready_283 || ready_285)) && (ready_289 || ((ready_289 || ready_285) && ready_103))) && (ready_295 || ((ready_295 || ready_285) && ready_103)));
            const bool condition_42 = (quadrant==0u);
            const bool ready_300 = (ready_107 && condition_42);
            const bool ready_301 = (!ready_300);
            const bool ready_303 = ((ready_301 || ready_110) && ready_103);
            const bool ready_306 = (ready_300 && condition_21);
            const bool ready_307 = (!ready_306);
            const bool ready_308 = (ready_307 || ready_303);
            const bool ready_313 = (ready_103 && (condition_28 || ready_103));
            const bool ready_314 = (ready_308 && ready_313);
            const bool ready_316 = (((ready_299 && (ready_301 || ready_303)) && (ready_307 || (ready_308 && ready_103))) && (ready_307 || ready_314));
            const bool ready_317 = (condition_28 || condition_34);
            const bool ready_319 = (!(ready_306 && ready_317));
            const bool ready_325 = (!(ready_300 && condition_29));
            const bool ready_329 = ((ready_316 && (ready_319 || ((ready_319 || ready_314) && ready_103))) && (ready_325 || ((ready_325 || ready_303) && ready_103)));
            const bool condition_43 = (computed_value_27==3u);
            const bool ready_332 = (condition_43 || condition_17);
            const bool ready_334 = (!ready_317);
            const bool ready_336 = (ready_313 && (ready_334 || ready_103));
            const bool ready_337 = (ready_103 && (condition_29 ? ready_103 : ready_336));
            const bool condition_44 = (computed_value_31==2u);
            const bool condition_45 = (computed_value_33==0u);
            const bool ready_339 = (ready_103 && (condition_45 || ready_103));
            const bool ready_341 = (ready_103 && (condition_25 || ready_339));
            const bool ready_343 = (ready_103 && (condition_44 || ready_341));
            const bool ready_344 = (ready_246 && (ready_250 ? ready_103 : ready_343));
            const bool ready_345 = (ready_103 && (condition_24 ? ready_344 : ready_235));
            const bool ready_346 = (ready_103 && (condition_39 ? ready_103 : ready_345));
            const bool ready_348 = (ready_103 && (condition_28 || ready_346));
            const bool ready_350 = (ready_204 && (ready_208 || ready_348));
            const bool ready_352 = (ready_103 && (condition_29 || ready_350));
            const bool condition_46 = (quadrant!=2u);
            const bool condition_47 = (rt!=0u);
            const bool ready_354 = (ready_103 && (condition_47 || ready_103));
            const bool ready_355 = (ready_103 && (condition_24 ? ready_354 : ready_103));
            const bool ready_356 = (ready_103 && (condition_28 ? ready_103 : ready_355));
            const bool ready_357 = (ready_103 && (condition_29 ? ready_103 : ready_356));
            const bool ready_359 = (ready_103 && (condition_46 || ready_357));
            const bool ready_360 = (ready_103 && (condition_30 ? ready_352 : ready_359));
            const bool ready_363 = (ready_103 && (condition_43 || (ready_103 && (condition_42 ? ready_337 : ready_360))));
            const bool ready_364 = ((ready_332 && ready_103) && ready_363);
            const bool ready_365 = (ready_364 && ready_332);
            const bool ready_367 = (ready_99 || (ready_100 && ready_365));
            const bool ready_371 = (ready_56 && ready_67);
            const bool ready_372 = (!ready_371);
            const bool ready_375 = ((((((ready_329 && ready_105) && ready_101) && ready_367) && ready_101) && ready_367) && (ready_372 || (ready_372 || ready_58)));
            const bool ready_376 = (ready_48 && redirect);
            const bool ready_377 = (!ready_376);
            const bool condition_path_4 = (stopping ? false : (ready_36 && (stall ? false : (ready_40 && (wait ? false : (ready_51 && redirect))))));
            bool ready_379{};
            if ((ready && (ready_2 && condition_path_4))) {
              ready_379 = std::get<0>(ctx.compute_shared_computation_37());
            }
            if (!((ready_375 && (ready_377 || ((ready_377 || ready_52) && ready_379))))) return;
            // Prepare every effect before publishing state operations.
            const bool halted_value = std::get<0>(ctx.compute_halted_view());
            const bool ready_383 = std::get<0>(ctx.compute_shared_computation_45());
            const bool ready_384 = std::get<0>(ctx.compute_shared_computation_46());
            const bool condition_48 = std::get<0>(ctx.compute_shared_computation_48());
            const bool condition_49 = std::get<0>(ctx.compute_shared_computation_50());
            const bool ready_385 = std::get<0>(ctx.compute_shared_computation_38());
            const bool condition_50 = std::get<0>(ctx.compute_shared_computation_39());
            const bool ready_386 = std::get<0>(ctx.compute_shared_computation_42());
            const bool ready_387 = std::get<0>(ctx.compute_shared_computation_74());
            const bool condition_51 = std::get<0>(ctx.compute_shared_computation_56());
            const bool condition_52 = std::get<0>(ctx.compute_shared_computation_59());
            const uint32_t number = std::get<0>(ctx.compute_number());
            const bool ready_392 = ((number!=1u) && ((number!=4u) && ((number!=11u) && ((number!=10u) && ((number!=93u) && condition)))));
            const bool selected_value_18 = (condition_51 ? condition : (condition_52 ? condition : ready_392));
            const bool selected_value_20 = (ready_385 ? (condition_50 ? (ready_386 && (ready_387 && selected_value_18)) : selected_value_18) : selected_value_18);
            const bool ready_396 = (ready_383 && (ready_384 ? condition : (condition_48 ? condition : (condition_49 && selected_value_20))));
            const bool write_fire = (halted_value ? condition : ready_396);
            uint32_t write_value{};
            if (write_fire) {
              const uint32_t pc_value_10 = ctx.top.cpu.fetch.pc.read();
              const uint32_t raw_8 = acir_sim::shift(rom_value.at((condition_4 ? computed_value_4 : uint64_t(0ULL))),((pc_value_2 & 3u) * 8u),1);
              const uint32_t raw_9 = (raw_8 | (rom_value_2.at((condition_5 ? computed_value_7 : uint64_t(0ULL))) << 16u));
              const uint32_t raw_10 = (ready_19 ? raw_9 : raw_8);
              const bool condition_58 = (raw_10==0u);
              const uint32_t length = ((condition_58 || ((raw_10 & 3u)==3u)) ? 4u : 2u);
              write_value = (pc_value_10 + length);
            }
            bool stall_2{};
            if ((!stopping)) {
              stall_2 = std::get<0>(ctx.compute_stall());
            }
            const bool ready_399 = (stall_2 || wait);
            const bool ready_400 = (!ready_399);
            const bool ready_401 = (ready_4 && ready_400);
            const bool ready_402 = (ready_401 && ready_55);
            const bool ready_403 = (ready_402 && ready_68);
            const bool condition_path_5 = (redirect ? false : ((pc_value_5>=4096u) ? false : (((pc_value_6 & 1u)!=0u) ? false : (computed_value_14!=0u))));
            uint32_t raw_11{};
            if ((stopping ? false : (stall_2 ? false : (wait ? false : condition_path_5)))) {
              raw_11 = acir_sim::shift(rom_value_3.at((condition_11 ? computed_value_16 : uint64_t(0ULL))),((pc_value_7 & 3u) * 8u),1);
            }
            const bool ready_404 = ((computed_value_14!=0u) && ((raw_11 & 3u)==3u));
            const bool ready_405 = (ready_403 && ready_404);
            const bool ready_406 = (ready_405 && condition_14);
            const bool write_fire_2 = ready_406;
            uint32_t write_value_2{};
            if (write_fire_2) {
              const uint32_t pc_value_11 = ctx.top.cpu.fetch.pc.read();
              const uint32_t computed_value_38 = (pc_value_11 + 4u);
              write_value_2 = computed_value_38;
            }
            const bool condition_path_6 = (redirect ? false : ((pc_value_5>=4096u) ? false : (((pc_value_6 & 1u)!=0u) ? false : (computed_value_14!=0u))));
            uint32_t raw_12{};
            if ((stopping ? false : (stall_2 ? false : (wait ? false : condition_path_6)))) {
              raw_12 = acir_sim::shift(rom_value_3.at((condition_11 ? computed_value_16 : uint64_t(0ULL))),((pc_value_7 & 3u) * 8u),1);
            }
            const bool ready_408 = ((computed_value_14!=0u) && ((raw_12 & 3u)==3u));
            const bool ready_409 = (!ready_408);
            const bool ready_410 = (ready_409 || condition_15);
            const bool ready_411 = (ready_403 && ready_410);
            const bool check_fire = ready_411;
            if (check_fire) {
            }
            const bool write_fire_3 = ready_411;
            uint32_t write_value_3{};
            if (write_fire_3) {
              const uint32_t pc_value_12 = ctx.top.cpu.fetch.pc.read();
              const uint32_t raw_13 = acir_sim::shift(rom_value_3.at((condition_11 ? computed_value_16 : uint64_t(0ULL))),((pc_value_7 & 3u) * 8u),1);
              const bool ready_413 = ((computed_value_14!=0u) && ((raw_13 & 3u)==3u));
              const uint32_t raw_14 = (raw_13 | (rom_value_4.at((condition_16 ? computed_value_24 : uint64_t(0ULL))) << 16u));
              const uint32_t raw_15 = (condition_15 ? raw_14 : raw_13);
              const uint32_t raw_16 = (ready_413 ? raw_15 : raw_13);
              const uint32_t computed_value_41 = (raw_16 & 3u);
              const bool condition_63 = (computed_value_41==3u);
              const uint32_t selected_value_24 = (condition_63 ? 4u : 2u);
              const uint32_t computed_value_42 = (pc_value_12 + selected_value_24);
              write_value_3 = computed_value_42;
            }
            const bool ready_414 = (ready_402 && ready_67);
            const bool write_fire_4 = ready_414;
            uint32_t write_value_4{};
            if (write_fire_4) {
              const uint32_t pc_value_13 = ctx.top.cpu.fetch.pc.read();
              const uint32_t computed_value_43 = (pc_value_13 + 4u);
              write_value_4 = computed_value_43;
            }
            const bool ready_416 = (ready_401 && redirect);
            const bool write_fire_5 = ready_416;
            uint32_t write_value_5{};
            if (write_fire_5) {
              write_value_5 = std::get<0>(ctx.compute_shared_computation_33());
            }
            bool ready_421{};
            if ((!stopping)) {
              ready_421 = std::get<0>(ctx.compute_shared_computation_73());
            }
            bool stall_3{};
            if ((stopping ? false : ready_421)) {
              stall_3 = std::get<0>(ctx.compute_stall());
            }
            bool ready_422{};
            if ((stopping ? false : (ready_421 && (!stall_3)))) {
              ready_422 = std::get<0>(ctx.compute_shared_computation_69());
            }
            const bool ready_424 = (ready_421 && (stall_3 || ready_422));
            const bool ready_425 = (stall_3 || wait);
            bool ready_426{};
            if ((stopping ? false : (ready_421 && (stall_3 ? false : (ready_422 && (!wait)))))) {
              ready_426 = std::get<0>(ctx.compute_shared_computation_36());
            }
            const bool condition_path_7 = (redirect ? false : ((pc_value_5>=4096u) ? false : (((pc_value_6 & 1u)!=0u) ? false : condition_11)));
            const bool condition_path_8 = (stopping ? false : (ready_421 && (stall_3 ? false : (ready_422 && (wait ? false : (ready_426 && condition_path_7))))));
            uint32_t raw_17{};
            if (condition_path_8) {
              raw_17 = acir_sim::shift(rom_value_3.at((condition_11 ? computed_value_16 : uint64_t(0ULL))),((pc_value_7 & 3u) * 8u),1);
            }
            const bool ready_427 = ((computed_value_14!=0u) && ((raw_17 & 3u)==3u));
            const bool ready_428 = (!ready_427);
            const bool ready_429 = (ready_428 || condition_15);
            const bool ready_430 = (ready_74 && (ready_427 ? (condition_15 ? (condition_11 && condition_16) : condition_11) : condition_11));
            const uint32_t raw_18 = (raw_17 | (rom_value_4.at((condition_16 ? computed_value_24 : uint64_t(0ULL))) << 16u));
            const uint32_t raw_19 = (condition_15 ? raw_18 : raw_17);
            const uint32_t raw_20 = (ready_427 ? raw_19 : raw_17);
            const uint32_t computed_value_45 = (raw_20 & 3u);
            const bool condition_66 = (computed_value_45==3u);
            const uint32_t h_2 = (raw_20 & 65535u);
            const uint32_t quadrant_2 = (h_2 & 3u);
            const bool condition_67 = (quadrant_2==0u);
            const uint32_t function_2 = (h_2 >> 13u);
            const bool condition_68 = (function_2==0u);
            const bool condition_69 = (function_2==2u);
            const bool ready_433 = (ready_430 && (condition_69 || ready_430));
            const bool condition_70 = (function_2==6u);
            const bool ready_434 = (condition_69 || condition_70);
            const bool ready_435 = (!ready_434);
            const bool ready_437 = (ready_433 && (ready_435 || ready_430));
            const bool ready_438 = (ready_430 && (condition_68 ? ready_430 : ready_437));
            const bool condition_71 = (quadrant_2==1u);
            const bool condition_72 = (function_2==1u);
            const bool ready_440 = (ready_430 && (condition_72 || ready_430));
            const bool ready_441 = (condition_72 || (function_2==5u));
            const bool condition_74 = (function_2==3u);
            const bool condition_75 = (function_2==4u);
            const uint32_t computed_value_46 = (h_2 >> 10u);
            const uint32_t computed_value_47 = (computed_value_46 & 3u);
            const bool condition_76 = (computed_value_47==0u);
            const bool ready_443 = (ready_430 && (condition_76 || ready_430));
            const bool condition_77 = (computed_value_47==1u);
            const bool ready_444 = (condition_76 || condition_77);
            const bool condition_78 = (computed_value_47==2u);
            const uint32_t computed_value_48 = (h_2 & 4096u);
            const bool condition_79 = (computed_value_48!=0u);
            const uint32_t computed_value_49 = (h_2 >> 5u);
            const uint32_t computed_value_50 = (computed_value_49 & 3u);
            const bool condition_80 = (computed_value_50==0u);
            const bool ready_446 = (ready_430 && (condition_80 || ready_430));
            const bool ready_448 = (ready_430 && (condition_79 || ready_446));
            const bool ready_450 = (ready_430 && (condition_78 || ready_448));
            const bool ready_451 = (ready_443 && (ready_444 ? ready_430 : ready_450));
            const bool ready_453 = (ready_430 && (condition_70 || ready_430));
            const bool ready_454 = (ready_430 && (condition_75 ? ready_451 : ready_453));
            const bool ready_455 = (ready_430 && (condition_74 ? ready_430 : ready_454));
            const bool ready_457 = (ready_430 && (condition_69 || ready_455));
            const bool ready_459 = (ready_440 && (ready_441 || ready_457));
            const bool ready_461 = (ready_430 && (condition_68 || ready_459));
            const bool condition_81 = (quadrant_2!=2u);
            const uint32_t computed_value_51 = (h_2 >> 2u);
            const uint32_t rt_2 = (computed_value_51 & 31u);
            const bool condition_82 = (rt_2!=0u);
            const bool ready_463 = (ready_430 && (condition_82 || ready_430));
            const bool ready_464 = (ready_430 && (condition_75 ? ready_463 : ready_430));
            const bool ready_465 = (ready_430 && (condition_69 ? ready_430 : ready_464));
            const bool ready_466 = (ready_430 && (condition_68 ? ready_430 : ready_465));
            const bool ready_468 = (ready_430 && (condition_81 || ready_466));
            const bool ready_469 = (ready_430 && (condition_71 ? ready_461 : ready_468));
            const bool ready_472 = (ready_430 && (condition_66 || (ready_430 && (condition_67 ? ready_438 : ready_469))));
            const bool ready_473 = (ready_430 && ready_472);
            const bool condition_83 = (computed_value_45!=3u);
            const uint32_t computed_value_52 = (h_2 >> 7u);
            const uint32_t computed_value_55 = (h_2 >> 11u);
            const uint32_t computed_value_59 = (computed_value_49 & 1u);
            const uint32_t computed_value_61 = ((((computed_value_52 & 15u) << 6u) | ((computed_value_55 & 3u) << 4u)) | (computed_value_59 << 3u));
            const uint32_t computed_value_63 = ((h_2 >> 6u) & 1u);
            const uint32_t computed_value_64 = (computed_value_63 << 2u);
            const uint32_t imm = (computed_value_61 | computed_value_64);
            const bool condition_84 = (imm==0u);
            const bool ready_479 = ((ready_430 && (condition_68 ? (ready_430 && (condition_84 || ready_430)) : ready_437)) && ready_438);
            const uint32_t rd_2 = (computed_value_52 & 31u);
            const bool condition_85 = (rd_2==2u);
            const uint32_t computed_value_66 = ((h_2 >> 12u) & 1u);
            const uint32_t computed_value_70 = (computed_value_59 << 6u);
            const uint32_t computed_value_72 = (h_2 >> 3u);
            const uint32_t computed_value_73 = (computed_value_72 & 3u);
            const uint32_t computed_value_75 = ((((computed_value_66 << 9u) | (computed_value_63 << 4u)) | computed_value_70) | (computed_value_73 << 7u));
            const uint32_t computed_value_77 = ((computed_value_51 & 1u) << 5u);
            const uint32_t imm_2 = (computed_value_75 | computed_value_77);
            const bool condition_86 = (imm_2==0u);
            const uint32_t computed_value_78 = (computed_value_66 << 5u);
            const uint32_t value = (computed_value_78 | rt_2);
            const uint32_t immediate = ((value ^ 32u) - 32u);
            const bool condition_87 = (immediate==0u);
            const bool selected_value_36 = (condition_85 ? (ready_430 && (condition_86 || ready_430)) : (ready_430 && (condition_87 || ready_430)));
            const bool ready_486 = (ready_430 && (condition_79 || ready_430));
            const bool condition_88 = (computed_value_50==1u);
            const bool ready_492 = (condition_79 || ((ready_430 && (condition_80 || (ready_430 && (condition_88 || ready_430)))) && ready_446));
            const bool selected_value_38 = (ready_444 ? ready_486 : ((ready_430 && (condition_78 ? ready_430 : ((ready_430 && ready_492) && ready_448))) && ready_450));
            const bool ready_499 = (condition_70 || (function_2==7u));
            const bool selected_value_39 = (condition_75 ? ((ready_443 && selected_value_38) && ready_451) : (ready_453 && ((!ready_499) || ready_430)));
            const bool ready_505 = (ready_430 && (condition_74 ? (ready_430 && selected_value_36) : ((ready_430 && selected_value_39) && ready_454)));
            const bool selected_value_42 = (ready_441 ? ready_430 : ((ready_430 && (condition_69 ? ready_430 : (ready_505 && ready_455))) && ready_457));
            const bool ready_512 = ((ready_430 && (condition_68 ? ready_430 : ((ready_440 && selected_value_42) && ready_459))) && ready_461);
            const bool condition_90 = (rd_2==0u);
            const bool ready_514 = (ready_430 && (condition_90 || ready_430));
            const bool condition_91 = (rt_2==0u);
            const bool condition_92 = (function_2!=6u);
            const bool selected_value_45 = (condition_75 ? ((ready_430 && ((ready_430 && (condition_91 ? ready_514 : ready_430)) && ready_463)) && ready_463) : (ready_430 && (condition_92 || ready_430)));
            const bool ready_524 = ((ready_430 && (condition_69 ? ready_514 : ((ready_430 && selected_value_45) && ready_464))) && ready_465);
            const bool ready_528 = (ready_430 && (condition_81 || ((ready_430 && (condition_68 ? ready_486 : ready_524)) && ready_466)));
            const bool selected_value_49 = (condition_67 ? ready_479 : ((ready_430 && (condition_71 ? ready_512 : (ready_528 && ready_468))) && ready_469));
            const bool ready_536 = ((((condition_66 && ready_430) || (condition_83 && (ready_430 && selected_value_49))) && ready_430) && ready_472);
            const bool condition_93 = (imm!=0u);
            const bool condition_94 = (function_2!=0u);
            const bool condition_95 = (function_2!=2u);
            const bool ready_546 = (condition_94 && (((ready_434 && ((condition_69 || condition_95) && ready_430)) || ready_435) && ready_433));
            const bool ready_549 = (condition_67 && (((condition_68 && ((condition_93 || condition_84) && ready_430)) || ready_546) && ready_430));
            const bool condition_96 = (quadrant_2!=0u);
            const bool ready_550 = (!ready_441);
            const bool condition_97 = (imm_2!=0u);
            const bool condition_98 = (rd_2!=2u);
            const bool condition_99 = (immediate!=0u);
            const bool ready_557 = ((condition_85 && ((condition_97 || condition_86) && ready_430)) || (condition_98 && ((condition_99 || condition_87) && ready_430)));
            const bool condition_100 = (function_2!=3u);
            const bool condition_101 = (computed_value_48==0u);
            const bool ready_561 = ((condition_101 || condition_79) && ready_430);
            const bool ready_563 = (!ready_444);
            const bool condition_102 = (computed_value_47!=2u);
            const bool ready_568 = ((ready_444 && ready_561) || (ready_563 && ((condition_78 || (condition_102 && ready_561)) && ready_430)));
            const bool condition_103 = (function_2!=4u);
            const bool ready_574 = (condition_100 && (((condition_75 && (ready_568 && ready_443)) || (condition_103 && ready_453)) && ready_430));
            const bool ready_578 = (condition_69 || (condition_95 && (((condition_74 && (ready_557 && ready_430)) || ready_574) && ready_430)));
            const bool ready_584 = (condition_68 || (condition_94 && ((ready_441 || (ready_550 && (ready_578 && ready_430))) && ready_440)));
            const bool condition_104 = (quadrant_2!=1u);
            const bool condition_105 = (quadrant_2==2u);
            const bool condition_106 = (rd_2!=0u);
            const bool ready_589 = ((condition_90 || condition_106) && ready_430);
            const bool ready_593 = (((condition_91 && ready_589) || condition_82) && ready_430);
            const bool ready_602 = ((condition_75 && (((condition_101 && ready_593) || (condition_79 && ready_593)) && ready_430)) || (condition_103 && ((condition_70 || condition_92) && ready_430)));
            const bool ready_607 = (condition_94 && (((condition_69 && ready_589) || (condition_95 && (ready_602 && ready_430))) && ready_430));
            const bool ready_612 = (((condition_105 && (((condition_68 && ready_561) || ready_607) && ready_430)) || condition_81) && ready_430);
            const bool ready_616 = (condition_96 && (((condition_71 && (ready_584 && ready_430)) || (condition_104 && ready_612)) && ready_430));
            const bool ready_621 = ((ready_536 && ready_473) && (condition_66 || (condition_83 && ((ready_549 || ready_616) && ready_430))));
            const bool selected_value_51 = (ready_427 ? (condition_14 ? condition_11 : (ready_4 || ((pc_value>=4096u) || selected_value))) : (ready_4 || ((pc_value>=4096u) || selected_value)));
            const bool ready_625 = (!ready_429);
            const bool ready_627 = (ready_74 && (ready_625 || ready_473));
            const bool ready_628 = ((ready_74 && (ready_429 ? (ready_473 && ready_621) : (ready_74 && selected_value_51))) && ready_627);
            const bool ready_630 = (ready_67 || ready_627);
            const bool ready_635 = (ready_426 && (redirect || ready_630));
            const bool selected_value_53 = (ready_425 ? (ready_4 || ((pc_value>=4096u) || selected_value)) : ((ready_426 && (redirect || ((ready_67 || ready_628) && ready_630))) && ready_635));
            const bool ready_639 = (ready_424 && (ready_425 || ready_635));
            const bool selected_value_54 = (stopping ? (ready_4 || ((pc_value>=4096u) || selected_value)) : ((ready_424 && selected_value_53) && ready_639));
            const bool ready_649 = (!ready_425);
            const bool ready_654 = (stopping || (ready_649 && (redirect || (ready_67 || (ready_429 ? true : (ready_427 && condition_14))))));
            const bool ready_655 = ((selected_value_54 && (stopping || ((ready_424 && (ready_425 || ready_635)) && ready_639))) && ready_654);
            const bool ready_664 = ((ready_429 || (ready_625 && (((ready_427 && (condition_14 || bool{})) || bool{}) && ready_74))) && ready_74);
            const bool ready_671 = (ready_425 || (ready_649 && ((redirect || (ready_55 && (ready_67 || (ready_68 && ready_664)))) && ready_426)));
            const bool ready_680 = ((ready_655 && (stopping || (ready_4 && (ready_671 && ready_424)))) && ((ready_4 || ((pc_value>=4096u) || selected_value)) && (stopping || ready_639)));
            const bool write_fire_6 = ready_680;
            Fetched write_value_6{};
            if (write_fire_6) {
              const Fetched payload = Fetched{false,0u,0u,0u,0u,false};
              bool stall_4{};
              if ((!stopping)) {
                stall_4 = std::get<0>(ctx.compute_stall());
              }
              const bool ready_682 = (stall_4 || wait);
              const Fetched selected_value_56 = (stopping ? payload : Fetched{});
              const uint32_t pc_value_14 = ctx.top.cpu.fetch.pc.read();
              const uint32_t computed_value_80 = (pc_value_14 + 4u);
              const Fetched packet = Fetched{true,pc_value_14,0u,0u,computed_value_80,true};
              const bool condition_path_9 = (stall_4 ? false : (wait ? false : (redirect ? false : ((pc_value_5>=4096u) ? false : (!((pc_value_6 & 1u)!=0u))))));
              uint32_t raw_21{};
              if ((stopping ? false : condition_path_9)) {
                raw_21 = acir_sim::shift(rom_value_3.at((condition_11 ? computed_value_16 : uint64_t(0ULL))),((pc_value_7 & 3u) * 8u),1);
              }
              const bool ready_683 = ((computed_value_14!=0u) && ((raw_21 & 3u)==3u));
              const bool ready_684 = (!ready_683);
              const bool ready_685 = (ready_684 || condition_15);
              const uint32_t pc_value_15 = ctx.top.cpu.fetch.pc.read();
              const uint32_t raw_22 = (raw_21 | (rom_value_4.at((condition_16 ? computed_value_24 : uint64_t(0ULL))) << 16u));
              const uint32_t raw_23 = (condition_15 ? raw_22 : raw_21);
              const uint32_t raw_24 = (ready_683 ? raw_23 : raw_21);
              const uint32_t computed_value_82 = (raw_24 & 3u);
              const bool condition_108 = (computed_value_82==3u);
              const uint32_t selected_value_57 = (condition_108 ? 4u : 2u);
              const uint32_t h_3 = (raw_24 & 65535u);
              const uint32_t quadrant_3 = (h_3 & 3u);
              const bool condition_110 = (quadrant_3==0u);
              const uint32_t function_3 = (h_3 >> 13u);
              const bool condition_111 = (function_3==0u);
              const uint32_t computed_value_83 = (h_3 >> 7u);
              const uint32_t computed_value_86 = (h_3 >> 11u);
              const uint32_t computed_value_90 = (h_3 >> 5u);
              const uint32_t computed_value_91 = (computed_value_90 & 1u);
              const uint32_t computed_value_93 = ((((computed_value_83 & 15u) << 6u) | ((computed_value_86 & 3u) << 4u)) | (computed_value_91 << 3u));
              const uint32_t computed_value_95 = ((h_3 >> 6u) & 1u);
              const uint32_t computed_value_96 = (computed_value_95 << 2u);
              const uint32_t imm_3 = (computed_value_93 | computed_value_96);
              const bool condition_112 = (imm_3!=0u);
              const uint32_t computed_value_100 = (h_3 >> 2u);
              const uint32_t rt_prime = ((computed_value_100 & 7u) + 8u);
              const uint32_t computed_value_102 = (rt_prime << 7u);
              const uint32_t word = (((((imm_3 & 4095u) << 20u) | 65536u) | computed_value_102) | 19u);
              const uint32_t word_2 = (condition_112 ? word : 0u);
              const bool condition_113 = (function_3==2u);
              const bool condition_114 = (function_3==6u);
              const bool ready_686 = (condition_113 || condition_114);
              const uint32_t computed_value_104 = (computed_value_91 << 6u);
              const uint32_t computed_value_105 = (h_3 >> 10u);
              const uint32_t imm_4 = ((computed_value_104 | ((computed_value_105 & 7u) << 3u)) | computed_value_96);
              const uint32_t rs_prime = ((computed_value_83 & 7u) + 8u);
              const uint32_t computed_value_112 = (rs_prime << 15u);
              const uint32_t computed_value_120 = (rt_prime << 20u);
              const uint32_t computed_value_126 = (((((((imm_4 >> 5u) & 127u) << 25u) | computed_value_120) | computed_value_112) | 8192u) | ((imm_4 & 31u) << 7u));
              const uint32_t word_3 = (condition_113 ? ((((((imm_4 & 4095u) << 20u) | computed_value_112) | 8192u) | computed_value_102) | 3u) : (computed_value_126 | 35u));
              const uint32_t word_4 = (ready_686 ? word_3 : 0u);
              const uint32_t word_5 = (condition_111 ? word_2 : word_4);
              const bool condition_115 = (quadrant_3==1u);
              const uint32_t computed_value_129 = ((h_3 >> 12u) & 1u);
              const uint32_t computed_value_130 = (computed_value_129 << 5u);
              const uint32_t rt_3 = (computed_value_100 & 31u);
              const uint32_t value_2 = (computed_value_130 | rt_3);
              const uint32_t immediate_2 = ((value_2 ^ 32u) - 32u);
              const uint32_t computed_value_133 = ((immediate_2 & 4095u) << 20u);
              const uint32_t rd_3 = (computed_value_83 & 31u);
              const uint32_t computed_value_134 = (rd_3 << 15u);
              const uint32_t computed_value_136 = (rd_3 << 7u);
              const uint32_t word_6 = (((computed_value_133 | computed_value_134) | computed_value_136) | 19u);
              const bool condition_116 = (function_3==1u);
              const bool ready_687 = (condition_116 || (function_3==5u));
              const uint32_t computed_value_142 = (h_3 >> 9u);
              const uint32_t computed_value_145 = (((computed_value_129 << 11u) | ((computed_value_86 & 1u) << 4u)) | ((computed_value_142 & 3u) << 8u));
              const uint32_t computed_value_154 = (((computed_value_145 | (((h_3 >> 8u) & 1u) << 10u)) | ((computed_value_83 & 1u) << 6u)) | (computed_value_95 << 7u));
              const uint32_t computed_value_155 = (h_3 >> 3u);
              const uint32_t computed_value_160 = ((computed_value_100 & 1u) << 5u);
              const uint32_t imm_5 = ((computed_value_154 | ((computed_value_155 & 7u) << 1u)) | computed_value_160);
              const uint32_t imm_6 = ((imm_5 ^ 2048u) - 2048u);
              const uint32_t computed_value_172 = (((((imm_6 >> 20u) & 1u) << 31u) | (((imm_6 >> 1u) & 1023u) << 21u)) | (((imm_6 >> 11u) & 1u) << 20u));
              const uint32_t rd_4 = uint32_t(condition_116);
              const uint32_t word_7 = (((computed_value_172 | (imm_6 & 1044480u)) | (rd_4 << 7u)) | 111u);
              const uint32_t word_8 = ((computed_value_133 | computed_value_136) | 19u);
              const bool condition_118 = (function_3==3u);
              const bool condition_119 = (rd_3==2u);
              const uint32_t computed_value_182 = (computed_value_155 & 3u);
              const uint32_t computed_value_184 = ((((computed_value_129 << 9u) | (computed_value_95 << 4u)) | computed_value_104) | (computed_value_182 << 7u));
              const uint32_t imm_7 = (computed_value_184 | computed_value_160);
              const bool condition_120 = (imm_7!=0u);
              const uint32_t imm_8 = ((imm_7 ^ 512u) - 512u);
              const uint32_t word_9 = (((((imm_8 & 4095u) << 20u) | 65536u) | 256u) | 19u);
              const uint32_t word_10 = (condition_120 ? word_9 : 0u);
              const bool condition_121 = (immediate_2!=0u);
              const uint32_t word_11 = (((immediate_2 << 12u) | computed_value_136) | 55u);
              const uint32_t word_12 = (condition_121 ? word_11 : 0u);
              const uint32_t word_13 = (condition_119 ? word_10 : word_12);
              const bool condition_122 = (function_3==4u);
              const uint32_t computed_value_192 = (computed_value_105 & 3u);
              const bool condition_123 = (computed_value_192==0u);
              const bool condition_124 = (computed_value_192==1u);
              const bool ready_688 = (condition_123 || condition_124);
              const uint32_t computed_value_193 = (h_3 & 4096u);
              const bool condition_125 = (computed_value_193==0u);
              const uint32_t imm_9 = (rt_3 | (condition_124 ? 1024u : 0u));
              const uint32_t computed_value_198 = (rs_prime << 7u);
              const uint32_t word_14 = ((((((imm_9 & 4095u) << 20u) | computed_value_112) | 20480u) | computed_value_198) | 19u);
              const uint32_t word_15 = (condition_125 ? word_14 : 0u);
              const bool condition_126 = (computed_value_192==2u);
              const uint32_t word_16 = ((((computed_value_133 | computed_value_112) | 28672u) | computed_value_198) | 19u);
              const uint32_t computed_value_203 = (computed_value_90 & 3u);
              const bool condition_127 = (computed_value_203==0u);
              const uint32_t upper = (condition_127 ? 32u : 0u);
              const bool condition_128 = (computed_value_203==1u);
              const uint32_t f = (condition_127 ? 0u : (condition_128 ? 4u : ((computed_value_203==2u) ? 6u : 7u)));
              const uint32_t word_17 = ((((((upper << 25u) | computed_value_120) | computed_value_112) | (f << 12u)) | computed_value_198) | 51u);
              const uint32_t word_18 = (condition_125 ? word_17 : 0u);
              const uint32_t word_19 = (condition_126 ? word_16 : word_18);
              const uint32_t word_20 = (ready_688 ? word_15 : word_19);
              const bool ready_689 = (condition_114 || (function_3==7u));
              const uint32_t computed_value_216 = ((((computed_value_129 << 8u) | (computed_value_192 << 3u)) | (computed_value_203 << 6u)) | (computed_value_182 << 1u));
              const uint32_t imm_10 = (computed_value_216 | computed_value_160);
              const uint32_t imm_11 = ((imm_10 ^ 256u) - 256u);
              const bool condition_131 = (function_3!=6u);
              const uint32_t f_2 = uint32_t(condition_131);
              const uint32_t computed_value_227 = ((((((imm_11 >> 12u) & 1u) << 31u) | (((imm_11 >> 5u) & 63u) << 25u)) | computed_value_112) | (f_2 << 12u));
              const uint32_t word_21 = (((computed_value_227 | (((imm_11 >> 1u) & 15u) << 8u)) | (((imm_11 >> 11u) & 1u) << 7u)) | 99u);
              const uint32_t word_22 = (ready_689 ? word_21 : 0u);
              const uint32_t word_23 = (condition_122 ? word_20 : word_22);
              const uint32_t word_24 = (condition_118 ? word_13 : word_23);
              const uint32_t word_25 = (condition_113 ? word_8 : word_24);
              const uint32_t word_26 = (ready_687 ? word_7 : word_25);
              const uint32_t word_27 = (condition_111 ? word_6 : word_26);
              const bool condition_132 = (quadrant_3==2u);
              const uint32_t word_28 = ((((((rt_3 & 4095u) << 20u) | computed_value_134) | 4096u) | computed_value_136) | 19u);
              const uint32_t word_29 = (condition_125 ? word_28 : 0u);
              const bool condition_133 = (rd_3!=0u);
              const uint32_t imm_12 = ((computed_value_130 | (((h_3 >> 4u) & 7u) << 2u)) | ((computed_value_100 & 3u) << 6u));
              const uint32_t word_30 = ((((((imm_12 & 4095u) << 20u) | 65536u) | 8192u) | computed_value_136) | 3u);
              const uint32_t word_31 = (condition_133 ? word_30 : 0u);
              const bool condition_134 = (rt_3==0u);
              const uint32_t word_32 = (computed_value_134 | 103u);
              const uint32_t word_33 = (condition_133 ? word_32 : 0u);
              const uint32_t computed_value_252 = (rt_3 << 20u);
              const uint32_t word_34 = ((computed_value_252 | computed_value_136) | 51u);
              const uint32_t word_35 = (condition_134 ? word_33 : word_34);
              const bool condition_135 = (rd_3==0u);
              const uint32_t word_36 = (condition_135 ? 1048691u : ((computed_value_134 | 128u) | 103u));
              const uint32_t word_37 = (((computed_value_252 | computed_value_134) | computed_value_136) | 51u);
              const uint32_t word_38 = (condition_134 ? word_36 : word_37);
              const uint32_t word_39 = (condition_125 ? word_35 : word_38);
              const uint32_t imm_13 = (((computed_value_142 & 15u) << 2u) | ((computed_value_83 & 3u) << 6u));
              const uint32_t computed_value_270 = (((((((imm_13 >> 5u) & 127u) << 25u) | computed_value_252) | 65536u) | 8192u) | ((imm_13 & 31u) << 7u));
              const uint32_t word_40 = (computed_value_270 | 35u);
              const uint32_t word_41 = (condition_114 ? word_40 : 0u);
              const uint32_t word_42 = (condition_122 ? word_39 : word_41);
              const uint32_t word_43 = (condition_113 ? word_31 : word_42);
              const uint32_t word_44 = (condition_111 ? word_29 : word_43);
              const uint32_t word_45 = (condition_132 ? word_44 : 0u);
              const uint32_t word_46 = (condition_115 ? word_27 : word_45);
              const uint32_t word_47 = (condition_110 ? word_5 : word_46);
              const uint32_t computed_value_271 = (pc_value_15 + selected_value_57);
              const Fetched packet_2 = Fetched{true,pc_value_15,((selected_value_57==2u) ? h_3 : raw_24),(condition_108 ? raw_24 : word_47),computed_value_271,false};
              const uint32_t pc_value_16 = ctx.top.cpu.fetch.pc.read();
              const uint32_t computed_value_272 = (raw_21 & 65535u);
              const uint32_t computed_value_273 = (pc_value_16 + 4u);
              const Fetched packet_3 = Fetched{true,pc_value_16,computed_value_272,computed_value_272,computed_value_273,true};
              const Fetched selected_value_65 = (ready_685 ? packet_2 : (ready_683 ? (condition_14 ? packet_3 : selected_value_56) : selected_value_56));
              const Fetched selected_value_69 = (stopping ? payload : (ready_682 ? selected_value_56 : (redirect ? payload : (ready_67 ? packet : selected_value_65))));
              write_value_6 = selected_value_69;
            }
            if (write_fire) { ctx.top.cpu.fetch.pc.write(write_value); }
            if (write_fire_2) { ctx.top.cpu.fetch.pc.write(write_value_2); }
            if (write_fire_3) { ctx.top.cpu.fetch.pc.write(write_value_3); }
            if (write_fire_4) { ctx.top.cpu.fetch.pc.write(write_value_4); }
            if (write_fire_5) { ctx.top.cpu.fetch.pc.write(write_value_5); }
            if (write_fire_6) { ctx.top.cpu.if_id.write(write_value_6); }
            ctx.fired_top_cpu_fetch_advance=true;
          }
        } fetch;
        struct ForwardingUnit {
        } forwarding_unit;
        struct HazardUnit {
        } hazard_unit;
        struct Memory {
          Cell<StoreEvent> load_event{};
          Cell<StoreEvent> store_event{};
          Cell<std::array<uint32_t,1024>> text{};
          Cell<std::array<uint32_t,1024>> words{};
          // Rule top.cpu.memory.advance#4
          template<class Context> void try_advance(Context &ctx) {
            const Executed ex_mem_value = ctx.top.cpu.ex_mem.read();
            const bool ready = ex_mem_value.valid;
            const bool ready_2 = ex_mem_value.load;
            const bool ready_3 = ex_mem_value.store;
            const bool ready_5 = (ready && (ready_2 || ready_3));
            const uint32_t address = ex_mem_value.alu;
            const bool condition = (address>=4096u);
            const uint32_t computed_value = (address - 4096u);
            const bool condition_2 = (computed_value<4096u);
            const bool ready_7 = (condition && condition_2);
            const uint32_t funct3 = ex_mem_value.funct3;
            uint32_t size{};
            if ((ready && (ready_2 ? (condition ? condition_2 : true) : (ready_3 && (condition && condition_2))))) {
              size = acir_sim::shift(1u,(funct3 & 3u),0);
            }
            const uint32_t computed_value_3 = (8192u - address);
            const bool condition_3 = (size<=computed_value_3);
            const bool ready_8 = (ready_7 && condition_3);
            const uint32_t computed_value_4 = (computed_value & 3u);
            const bool condition_4 = ((computed_value_4 + size)>4u);
            const uint32_t computed_value_6 = (computed_value >> 2u);
            const uint64_t computed_value_7 = uint64_t(computed_value_6);
            const bool condition_5 = (computed_value_7<uint64_t(1024ULL));
            const uint64_t computed_value_9 = uint64_t((computed_value_6 + 1u));
            const bool condition_6 = (computed_value_9<uint64_t(1024ULL));
            const bool ready_12 = ((!((ready_5 && ready_3) && ready_8)) || (condition_4 ? (condition_5 && condition_6) : condition_5));
            const bool ready_14 = (!ready_2);
            const bool condition_7 = (address<4096u);
            const uint32_t computed_value_10 = (4096u - address);
            const bool condition_8 = (size<=computed_value_10);
            const bool ready_16 = ((condition_7 && condition_8) || ready_8);
            const bool ready_17 = (!ready_16);
            const uint32_t computed_value_11 = (address & 3u);
            const bool condition_9 = ((computed_value_11 + size)>4u);
            const bool ready_18 = (!ready_7);
            const uint64_t computed_value_14 = uint64_t((address >> 2u));
            const bool condition_10 = (computed_value_14<uint64_t(1024ULL));
            const bool ready_24 = ((condition ? (ready_18 || condition_5) : (condition || condition_10)) && (condition || (condition_7 && (condition_7 || condition))));
            const uint32_t computed_value_15 = (address & 4294967292u);
            const uint32_t address_2 = (computed_value_15 + 4u);
            const bool condition_11 = (address_2>=4096u);
            const uint32_t computed_value_16 = (computed_value_15 + 4294963204u);
            const bool ready_25 = (condition_11 && (computed_value_16<4096u));
            const bool ready_26 = (!ready_25);
            const uint64_t computed_value_18 = uint64_t((computed_value_16 >> 2u));
            const bool condition_13 = (computed_value_18<uint64_t(1024ULL));
            const uint64_t computed_value_20 = uint64_t((address_2 >> 2u));
            const bool condition_14 = (computed_value_20<uint64_t(1024ULL));
            const bool condition_15 = (address_2<4096u);
            const bool ready_32 = ((condition_11 ? (ready_26 || condition_13) : (condition_11 || condition_14)) && (condition_11 || (condition_15 && (condition_15 || condition_11))));
            const bool ready_37 = (ready_12 && ((!ready_5) || (ready_14 || (ready_17 || (condition_9 ? (ready_24 && ready_32) : ready_24)))));
            if (!(ready_37)) return;
            // Prepare every effect before publishing state operations.
            const bool condition_16 = (computed_value>=4096u);
            uint32_t size_2{};
            if ((ready && (ready_2 && (condition_7 || (!condition_16))))) {
              size_2 = acir_sim::shift(1u,(funct3 & 3u),0);
            }
            const bool condition_17 = (size_2>computed_value_10);
            const bool condition_18 = (size_2>computed_value_3);
            const bool ready_38 = (condition_16 || condition_18);
            const bool check_fire = (ready && (ready_2 && (condition_7 ? condition_17 : ready_38)));
            if (check_fire) {
              acir_sim::check(false);
            }
            uint32_t size_3{};
            if ((ready && (ready_2 && (condition_7 || condition_2)))) {
              size_3 = acir_sim::shift(1u,(funct3 & 3u),0);
            }
            const bool condition_19 = (size_3<=computed_value_10);
            const bool condition_20 = (size_3<=computed_value_3);
            const bool ready_42 = (condition_2 && condition_20);
            const bool check_fire_2 = (ready && (ready_2 && (condition_7 ? condition_19 : ready_42)));
            if (check_fire_2) {
            }
            uint32_t size_4{};
            if ((ready && (ready_2 && (condition_7 || condition_2)))) {
              size_4 = acir_sim::shift(1u,(funct3 & 3u),0);
            }
            const bool condition_21 = (size_4<=computed_value_10);
            const bool condition_22 = ((computed_value_11 + size_4)>4u);
            const bool condition_23 = (computed_value_16>=4096u);
            const bool ready_47 = (condition_22 && (condition_11 && condition_23));
            const bool condition_24 = (size_4<=computed_value_3);
            const bool ready_51 = (ready_2 && (condition_7 ? (condition_21 && ready_47) : (condition_2 && (condition_24 && ready_47))));
            const bool check_fire_3 = (ready && ready_51);
            if (check_fire_3) {
              acir_sim::check(false);
            }
            uint32_t size_5{};
            if ((ready && (ready_2 && (condition_7 || condition_2)))) {
              size_5 = acir_sim::shift(1u,(funct3 & 3u),0);
            }
            const bool condition_25 = (size_5<=computed_value_10);
            const bool condition_26 = ((computed_value_11 + size_5)>4u);
            const bool condition_27 = (size_5<=computed_value_3);
            const bool ready_57 = (ready_2 && (condition_7 ? (condition_25 && condition_26) : (condition_2 && (condition_27 && condition_26))));
            const bool check_fire_4 = (ready && ready_57);
            if (check_fire_4) {
            }
            const bool check_fire_5 = (ready && ready_2);
            if (check_fire_5) {
            }
            uint32_t size_6{};
            if ((ready && (ready_3 && (condition_7 ? false : (!condition_16))))) {
              size_6 = acir_sim::shift(1u,(funct3 & 3u),0);
            }
            const bool condition_28 = (size_6>computed_value_3);
            const bool ready_62 = (condition_16 || condition_28);
            const bool check_fire_6 = (ready && (ready_3 && (condition_7 || ready_62)));
            if (check_fire_6) {
              acir_sim::check(false);
            }
            uint32_t size_7{};
            if ((ready && (ready_3 && (condition && condition_2)))) {
              size_7 = acir_sim::shift(1u,(funct3 & 3u),0);
            }
            const bool condition_29 = (size_7<=computed_value_3);
            const bool ready_67 = (condition_2 && condition_29);
            const bool write_fire = (ready && (ready_3 && (condition && ready_67)));
            std::array<uint32_t,1024> write_value{};
            if (write_fire) {
              const uint32_t size_8 = acir_sim::shift(1u,(funct3 & 3u),0);
              const bool condition_30 = ((computed_value_4 + size_8)>4u);
              const std::array<uint32_t,1024> words_value = ctx.top.cpu.memory.words.read();
              const std::array<uint32_t,1024> updated = words_value;
              const uint64_t selected_value_9 = (condition_5 ? computed_value_7 : uint64_t(0ULL));
              const uint32_t mask = ((size_8==1u) ? 255u : ((size_8==2u) ? 65535u : 4294967295u));
              const uint32_t shift = (computed_value_4 * 8u);
              const uint32_t computed_value_28 = (ex_mem_value.store_value & mask);
              const uint32_t computed_value_30 = ((words_value.at(selected_value_9) & (acir_sim::shift(mask,shift,0) ^ 4294967295u)) | acir_sim::shift(computed_value_28,shift,0));
              const std::array<uint32_t,1024> updated_2 = [&]{ auto r=updated; r.at(selected_value_9)=computed_value_30; return r; }();
              const std::array<uint32_t,1024> words_value_2 = ctx.top.cpu.memory.words.read();
              const uint64_t selected_value_11 = (condition_6 ? computed_value_9 : uint64_t(0ULL));
              const uint32_t high_shift = (32u - shift);
              uint32_t computed_value_32{};
              if (condition_30) {
                computed_value_32 = acir_sim::shift(mask,high_shift,1);
              }
              uint32_t computed_value_35{};
              if (condition_30) {
                computed_value_35 = acir_sim::shift(computed_value_28,high_shift,1);
              }
              const std::array<uint32_t,1024> updated_3 = [&]{ auto r=updated_2; r.at(selected_value_11)=((words_value_2.at(selected_value_11) & (computed_value_32 ^ 4294967295u)) | computed_value_35); return r; }();
              const std::array<uint32_t,1024> updated_4 = (condition_30 ? updated_3 : updated_2);
              write_value = updated_4;
            }
            uint32_t size_9{};
            if ((ready && (ready_3 && (condition && condition_2)))) {
              size_9 = acir_sim::shift(1u,(funct3 & 3u),0);
            }
            const bool condition_33 = (size_9<=computed_value_3);
            const bool ready_72 = (ready_7 && condition_33);
            const uint32_t mask_2 = ((size_9==1u) ? 255u : ((size_9==2u) ? 65535u : 4294967295u));
            const uint32_t computed_value_37 = (ex_mem_value.store_value & mask_2);
            const StoreEvent event = StoreEvent{true,address,computed_value_37,size_9};
            const StoreEvent event_2 = StoreEvent{false,0u,0u,0u};
            const StoreEvent event_3 = (ready_72 ? event : event_2);
            const StoreEvent event_4 = (ready_3 ? event_3 : event_2);
            const StoreEvent event_5 = (ready_5 ? event_4 : event_2);
            const StoreEvent write_value_2 = event_5;
            uint32_t size_10{};
            if ((ready && ready_2)) {
              size_10 = acir_sim::shift(1u,(funct3 & 3u),0);
            }
            const bool condition_36 = (size_10<=computed_value_10);
            const bool condition_37 = (size_10<=computed_value_3);
            const bool ready_74 = (ready_7 && condition_37);
            const bool ready_75 = ((condition_7 && condition_36) || ready_74);
            const bool condition_38 = ((computed_value_11 + size_10)>4u);
            const std::array<uint32_t,1024> words_value_3 = ctx.top.cpu.memory.words.read();
            const uint64_t selected_value_13 = (condition_5 ? computed_value_7 : uint64_t(0ULL));
            const std::array<uint32_t,1024> text_value = ctx.top.cpu.memory.text.read();
            const uint32_t payload = std::get<0>(ctx.compute_shared_computation_7());
            const uint32_t selected_value_17 = (condition ? (ready_7 ? words_value_3.at(selected_value_13) : 0u) : (condition_7 ? text_value.at((condition_10 ? computed_value_14 : uint64_t(0ULL))) : payload));
            const uint32_t shift_2 = (computed_value_11 * 8u);
            uint32_t value{};
            if ((ready && (ready_2 && (condition_7 ? condition_36 : (condition_2 && condition_37))))) {
              value = acir_sim::shift(selected_value_17,shift_2,1);
            }
            const std::array<uint32_t,1024> words_value_4 = ctx.top.cpu.memory.words.read();
            const std::array<uint32_t,1024> text_value_2 = ctx.top.cpu.memory.text.read();
            const uint32_t selected_value_22 = (condition_11 ? (ready_25 ? words_value_4.at((condition_13 ? computed_value_18 : uint64_t(0ULL))) : 0u) : (condition_15 ? text_value_2.at((condition_14 ? computed_value_20 : uint64_t(0ULL))) : payload));
            const bool condition_path = (ready_2 && (condition_7 ? (condition_36 && condition_38) : (condition_2 && (condition_37 && condition_38))));
            uint32_t computed_value_44{};
            if ((ready && condition_path)) {
              computed_value_44 = acir_sim::shift(selected_value_22,(32u - shift_2),0);
            }
            const uint32_t value_2 = (value | computed_value_44);
            const uint32_t value_3 = (condition_38 ? value_2 : value);
            const uint32_t mask_3 = ((size_10==1u) ? 255u : ((size_10==2u) ? 65535u : 4294967295u));
            const uint32_t value_4 = (ready_75 ? (value_3 & mask_3) : 0u);
            const StoreEvent loaded = StoreEvent{true,address,value_4,size_10};
            const StoreEvent loaded_2 = (ready_2 ? loaded : event_2);
            const StoreEvent loaded_3 = (ready_5 ? loaded_2 : event_2);
            const StoreEvent write_value_3 = loaded_3;
            uint32_t size_11{};
            if ((ready && (ready_2 && ((funct3==0u) || ((funct3==1u) || (condition_7 || condition_2)))))) {
              size_11 = acir_sim::shift(1u,(funct3 & 3u),0);
            }
            const bool condition_43 = (size_11<=computed_value_10);
            const bool condition_44 = (size_11<=computed_value_3);
            const bool ready_79 = (ready_7 && condition_44);
            const bool ready_80 = ((condition_7 && condition_43) || ready_79);
            const bool condition_45 = ((computed_value_11 + size_11)>4u);
            uint32_t value_5{};
            if ((ready && (ready_2 && (condition_7 ? condition_43 : (condition_2 && condition_44))))) {
              value_5 = acir_sim::shift(selected_value_17,shift_2,1);
            }
            const bool condition_path_2 = (ready_2 && (condition_7 ? (condition_43 && condition_45) : (condition_2 && (condition_44 && condition_45))));
            uint32_t computed_value_47{};
            if ((ready && condition_path_2)) {
              computed_value_47 = acir_sim::shift(selected_value_22,(32u - shift_2),0);
            }
            const uint32_t value_6 = (value_5 | computed_value_47);
            const uint32_t value_7 = (condition_45 ? value_6 : value_5);
            const uint32_t mask_4 = ((size_11==1u) ? 255u : ((size_11==2u) ? 65535u : 4294967295u));
            const uint32_t value_8 = (ready_80 ? (value_7 & mask_4) : 0u);
            const uint32_t bits = (size_11 * 8u);
            uint32_t sign{};
            if ((ready && (ready_2 && ((funct3==0u) || (funct3==1u))))) {
              sign = acir_sim::shift(1u,(bits - 1u),0);
            }
            const uint32_t value_9 = ((value_8 ^ sign) - sign);
            const uint32_t value_10 = (((funct3==0u) || (funct3==1u)) ? value_9 : value_8);
            const uint32_t value_11 = ex_mem_value.value;
            const uint32_t value_12 = (ready_2 ? value_10 : value_11);
            const uint32_t value_13 = (ready_5 ? value_12 : value_11);
            const Completed payload_2 = Completed{ready,ex_mem_value.pc,ex_mem_value.word,ex_mem_value.rd,ex_mem_value.writes,value_13,ex_mem_value.stalled,ex_mem_value.illegal,ex_mem_value.system,ex_mem_value.expanded,ex_mem_value.next_pc};
            const Completed write_value_4 = payload_2;
            if (write_fire) { ctx.top.cpu.memory.words.write(write_value); }
            ctx.top.cpu.memory.store_event.write(write_value_2);
            ctx.top.cpu.memory.load_event.write(write_value_3);
            ctx.top.cpu.mem_wb.write(write_value_4);
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
            const bool ready = mem_wb_value.valid;
            const bool ready_3 = mem_wb_value.writes;
            const uint32_t index = mem_wb_value.rd;
            const uint64_t computed_value = uint64_t(index);
            if (!(((!ready) || ((!ready_3) || ((index==0u) || (computed_value<uint64_t(32ULL))))))) return;
            // Prepare every effect before publishing state operations.
            const bool ready_8 = (ready_3 && (index!=0u));
            const bool write_fire = (ready && ready_8);
            uint32_t write_value{};
            uint64_t resource_index{};
            if (write_fire) {
              const uint32_t value = mem_wb_value.value;
              write_value = value;
              resource_index = computed_value;
            }
            const uint32_t pc = mem_wb_value.pc;
            const uint32_t code_size = 84u;
            const bool ready_11 = (ready && (pc<code_size));
            const bool write_fire_2 = ready_11;
            uint32_t write_value_2{};
            if (write_fire_2) {
              const uint32_t retired_value = ctx.top.cpu.writeback.retired.read();
              write_value_2 = (retired_value + 1u);
            }
            const uint32_t value_2 = mem_wb_value.value;
            const Retirement event = Retirement{true,pc,mem_wb_value.word,index,ready_8,value_2};
            const Retirement event_2 = Retirement{false,0u,0u,0u,false,0u};
            const Retirement event_3 = (ready_11 ? event : event_2);
            const Retirement write_value_3 = event_3;
            if (write_fire) { ctx.top.cpu.rf.registers.at(resource_index).write(write_value); }
            if (write_fire_2) { ctx.top.cpu.writeback.retired.write(write_value_2); }
            ctx.top.cpu.writeback.retirement.write(write_value_3);
            ctx.fired_top_cpu_writeback_advance=true;
          }
        } writeback;
        Cell<Executed> ex_mem{};
        Cell<uint32_t> exit_status{};
        Cell<bool> halted{};
        Cell<Decoded> id_ex{};
        Cell<Fetched> if_id{};
        Cell<Control> last_control{};
        Cell<Completed> mem_wb{};
        Cell<SystemEvent> system_event{};
        // Rule top.cpu.observe#6
        template<class Context> void try_observe(Context &ctx) {
          if (!(std::get<0>(ctx.compute_shared_computation_68()))) return;
          // Prepare every effect before publishing state operations.
          const bool stall = std::get<0>(ctx.compute_stall());
          const bool redirect = std::get<0>(ctx.compute_redirect());
          const uint32_t selected_value = std::get<0>(ctx.compute_shared_computation_33());
          const uint32_t source_a = std::get<0>(ctx.compute_source_a());
          const uint32_t source_b = std::get<0>(ctx.compute_source_b());
          const bool wait = std::get<0>(ctx.compute_wait());
          const bool stopping = std::get<0>(ctx.compute_stopping());
          const Control control = Control{stall,redirect,selected_value,source_a,source_b,wait,stopping};
          const Control write_value = control;
          ctx.top.cpu.last_control.write(write_value);
          ctx.fired_top_cpu_observe=true;
        }
        // Rule top.cpu.environment#7
        template<class Context> void try_environment(Context &ctx) {
          if (!(true)) return;
          // Prepare every effect before publishing state operations.
          const Decoded id_ex_value = ctx.top.cpu.id_ex.read();
          const bool ready = id_ex_value.valid;
          const bool ready_2 = std::get<0>(ctx.compute_shared_computation_38());
          const bool condition = std::get<0>(ctx.compute_shared_computation_39());
          const bool ready_3 = std::get<0>(ctx.compute_shared_computation_42());
          const bool ready_4 = std::get<0>(ctx.compute_shared_computation_74());
          const bool halted_value = ctx.top.cpu.halted.read();
          const bool ready_5 = (!halted_value);
          const Decoded id_ex_value_2 = ctx.top.cpu.id_ex.read();
          const Instruction instruction = id_ex_value_2.instruction;
          const uint32_t system = instruction.system;
          const bool condition_2 = (system!=0u);
          const bool ready_6 = instruction.illegal;
          const bool ready_7 = (condition_2 || ready_6);
          const bool ready_8 = (ready_5 && ready_7);
          const bool check_fire = (ready && (ready_2 ? (condition ? (ready_3 && (ready_4 && ready_8)) : ready_8) : ready_8));
          if (check_fire) {
          }
          const bool check_fire_2 = (ready && (ready_2 ? (condition ? (ready_3 && (ready_4 && ready_8)) : ready_8) : ready_8));
          if (check_fire_2) {
          }
          const Completed mem_wb_value = ctx.top.cpu.mem_wb.read();
          const bool ready_14 = (mem_wb_value.writes && (mem_wb_value.rd==17u));
          const uint32_t registers_value = ((uint64_t(17ULL) < 32) ? ctx.top.cpu.rf.registers.at(uint64_t(17ULL)).read() : uint32_t{});
          const uint32_t selected_value_3 = (ready_14 ? mem_wb_value.value : registers_value);
          const bool condition_5 = (selected_value_3==10u);
          const bool condition_6 = (selected_value_3==93u);
          const bool ready_18 = (condition_5 || (condition_6 || ((selected_value_3!=1u) && ((selected_value_3!=4u) && (selected_value_3!=11u)))));
          const bool ready_21 = (ready_5 && (condition_2 ? (ready_6 || ((system==2u) || ready_18)) : ready_6));
          const bool write_fire = (ready && (ready_2 ? (condition ? (ready_3 && (ready_4 && ready_21)) : ready_21) : ready_21));
          bool write_value{};
          if (write_fire) {
            write_value = true;
          }
          const bool write_fire_2 = (ready && (ready_2 ? (condition ? (ready_3 && (ready_4 && ready_21)) : ready_21) : ready_21));
          uint32_t write_value_2{};
          if (write_fire_2) {
            const bool condition_10 = (system==1u);
            const Completed mem_wb_value_2 = ctx.top.cpu.mem_wb.read();
            const bool ready_28 = (mem_wb_value_2.writes && (mem_wb_value_2.rd==10u));
            const uint32_t registers_value_2 = ((uint64_t(10ULL) < 32) ? ctx.top.cpu.rf.registers.at(uint64_t(10ULL)).read() : uint32_t{});
            const uint32_t selected_value_7 = (ready_28 ? mem_wb_value_2.value : registers_value_2);
            write_value_2 = ((condition_6 && condition_10) ? selected_value_7 : 0u);
          }
          const bool ready_29 = std::get<0>(ctx.compute_shared_computation_51());
          const Decoded id_ex_value_3 = ctx.top.cpu.id_ex.read();
          const bool condition_12 = (system==1u);
          const bool ready_35 = (((((selected_value_3==1u) || (selected_value_3==4u)) || (selected_value_3==11u)) || condition_5) || condition_6);
          const Completed mem_wb_value_3 = ctx.top.cpu.mem_wb.read();
          const bool ready_39 = (mem_wb_value_3.writes && (mem_wb_value_3.rd==10u));
          const uint32_t registers_value_3 = ((uint64_t(10ULL) < 32) ? ctx.top.cpu.rf.registers.at(uint64_t(10ULL)).read() : uint32_t{});
          const uint32_t selected_value_11 = (ready_39 ? mem_wb_value_3.value : registers_value_3);
          const SystemEvent event = SystemEvent{true,id_ex_value_3.pc,(ready_6 ? 3u : ((condition_12 && (!ready_35)) ? 4u : system)),selected_value_3,selected_value_11};
          const SystemEvent event_2 = SystemEvent{false,0u,0u,0u,0u};
          const SystemEvent event_3 = (ready_7 ? event : event_2);
          const SystemEvent event_4 = (((ready && ready_29) && ready_5) ? event_3 : event_2);
          const SystemEvent write_value_3 = event_4;
          if (write_fire) { ctx.top.cpu.halted.write(write_value); }
          if (write_fire_2) { ctx.top.cpu.exit_status.write(write_value_2); }
          ctx.top.cpu.system_event.write(write_value_3);
          ctx.fired_top_cpu_environment=true;
        }
      } cpu;
    } top;
    std::optional<std::tuple<Instruction,bool>> compute_instruction_view_cache;
    std::optional<std::tuple<Decoded,bool>> compute_id_ex_view_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_2_cache;
    std::optional<std::tuple<uint32_t>> compute_shared_computation_3_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_4_cache;
    std::optional<std::tuple<uint32_t>> compute_shared_computation_5_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_6_cache;
    std::optional<std::tuple<bool>> compute_stall_cache;
    std::optional<std::tuple<Decoded,bool>> compute_id_ex_view_2_cache;
    std::optional<std::tuple<Executed,bool>> compute_ex_mem_view_cache;
    std::optional<std::tuple<Completed,bool>> compute_mem_wb_view_cache;
    std::optional<std::tuple<uint32_t>> compute_shared_computation_7_cache;
    std::optional<std::tuple<uint32_t>> compute_source_a_cache;
    std::optional<std::tuple<uint32_t>> compute_source_b_cache;
    std::optional<std::tuple<uint32_t>> compute_lhs_cache;
    std::optional<std::tuple<uint32_t>> compute_rhs_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_8_cache;
    std::optional<std::tuple<Decoded,bool>> compute_id_ex_view_3_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_9_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_10_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_11_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_12_cache;
    std::optional<std::tuple<Instruction>> compute_executing_cache;
    std::optional<std::tuple<uint32_t>> compute_pc_cache;
    std::optional<std::tuple<uint32_t>> compute_opcode_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_13_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_14_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_15_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_16_cache;
    std::optional<std::tuple<uint32_t>> compute_function_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_17_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_18_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_19_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_20_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_21_cache;
    std::optional<std::tuple<uint32_t>> compute_result_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_22_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_23_cache;
    std::optional<std::tuple<bool>> compute_jump_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_24_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_25_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_26_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_27_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_28_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_29_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_30_cache;
    std::optional<std::tuple<bool>> compute_redirect_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_31_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_32_cache;
    std::optional<std::tuple<uint32_t>> compute_shared_computation_33_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_34_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_35_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_36_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_37_cache;
    std::optional<std::tuple<Decoded,bool>> compute_id_ex_view_4_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_38_cache;
    std::optional<std::tuple<Decoded,bool>> compute_id_ex_view_5_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_39_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_40_cache;
    std::optional<std::tuple<Executed,bool>> compute_ex_mem_view_2_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_41_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_42_cache;
    std::optional<std::tuple<Completed,bool>> compute_mem_wb_view_2_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_43_cache;
    std::optional<std::tuple<bool>> compute_wait_cache;
    std::optional<std::tuple<Decoded,bool>> compute_id_ex_view_6_cache;
    std::optional<std::tuple<bool,bool>> compute_halted_view_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_44_cache;
    std::optional<std::tuple<Decoded,bool>> compute_id_ex_view_7_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_45_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_46_cache;
    std::optional<std::tuple<uint32_t>> compute_shared_computation_47_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_48_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_49_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_50_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_51_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_52_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_53_cache;
    std::optional<std::tuple<Completed,bool>> compute_mem_wb_view_3_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_54_cache;
    std::optional<std::tuple<uint32_t,bool>> compute_value0_view_cache;
    std::optional<std::tuple<uint32_t>> compute_shared_computation_55_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_56_cache;
    std::optional<std::tuple<Completed,bool>> compute_mem_wb_view_4_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_57_cache;
    std::optional<std::tuple<uint32_t,bool>> compute_value0_view_2_cache;
    std::optional<std::tuple<uint32_t>> compute_shared_computation_58_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_59_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_60_cache;
    std::optional<std::tuple<Completed,bool>> compute_mem_wb_view_5_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_61_cache;
    std::optional<std::tuple<uint32_t,bool>> compute_value0_view_3_cache;
    std::optional<std::tuple<uint32_t>> compute_number_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_62_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_63_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_64_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_65_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_66_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_67_cache;
    std::optional<std::tuple<bool>> compute_stopping_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_68_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_69_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_70_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_71_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_72_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_73_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_74_cache;
    bool fired_top_cpu_decode_stage_decode_instruction=false;
    bool fired_top_cpu_fetch_advance=false;
    bool fired_top_cpu_decode_stage_advance=false;
    bool fired_top_cpu_execute_advance=false;
    bool fired_top_cpu_memory_advance=false;
    bool fired_top_cpu_writeback_advance=false;
    bool fired_top_cpu_observe=false;
    bool fired_top_cpu_environment=false;
    Model() { reset(); }
    void reset() {
      top.cpu.decode_stage.instruction={};
      top.cpu.decode_stage.instruction.value=Instruction{0u,0u,0u,0u,0u,0u,0u,false,false,false,false,0u};
      top.cpu.ex_mem={};
      top.cpu.ex_mem.value=Executed{false,0u,0u,0u,false,false,false,0u,0u,0u,false,0u,false,0u,0u,0u};
      top.cpu.exit_status={};
      top.cpu.exit_status.value=0u;
      top.cpu.fetch.pc={};
      top.cpu.fetch.pc.value=0u;
      top.cpu.fetch.rom={};
      top.cpu.fetch.rom.value=std::array<uint32_t,1024>{5431u,17825939u,1384483u,336131u,1115747u,335907u,103811603u,1114515u,2201187u,335907u,103811731u,336387u,1185379u,1180307u,8388719u,335907u,5579299u,2146435091u,111u,19u,19u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u};
      top.cpu.halted={};
      top.cpu.halted.value=false;
      top.cpu.id_ex={};
      top.cpu.id_ex.value=Decoded{false,0u,0u,Instruction{0u,0u,0u,0u,0u,0u,0u,false,false,false,false,0u},0u,0u,false,0u,0u,false};
      top.cpu.if_id={};
      top.cpu.if_id.value=Fetched{false,0u,0u,0u,0u,false};
      top.cpu.last_control={};
      top.cpu.last_control.value=Control{false,false,0u,0u,0u,false,false};
      top.cpu.mem_wb={};
      top.cpu.mem_wb.value=Completed{false,0u,0u,0u,false,0u,false,false,0u,0u,0u};
      top.cpu.memory.load_event={};
      top.cpu.memory.load_event.value=StoreEvent{false,0u,0u,0u};
      top.cpu.memory.store_event={};
      top.cpu.memory.store_event.value=StoreEvent{false,0u,0u,0u};
      top.cpu.memory.text={};
      top.cpu.memory.text.value=std::array<uint32_t,1024>{5431u,17825939u,1384483u,336131u,1115747u,335907u,103811603u,1114515u,2201187u,335907u,103811731u,336387u,1185379u,1180307u,8388719u,335907u,5579299u,2146435091u,111u,19u,19u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u};
      top.cpu.memory.words={};
      top.cpu.memory.words.value=std::array<uint32_t,1024>{0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u};
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
      top.cpu.system_event={};
      top.cpu.system_event.value=SystemEvent{false,0u,0u,0u,0u};
      top.cpu.writeback.retired={};
      top.cpu.writeback.retired.value=0u;
      top.cpu.writeback.retirement={};
      top.cpu.writeback.retirement.value=Retirement{false,0u,0u,0u,false,0u};
      begin_tick();
    }
    void begin_tick() {
      top.cpu.decode_stage.instruction.begin();
      top.cpu.ex_mem.begin();
      top.cpu.exit_status.begin();
      top.cpu.fetch.pc.begin();
      top.cpu.fetch.rom.begin();
      top.cpu.halted.begin();
      top.cpu.id_ex.begin();
      top.cpu.if_id.begin();
      top.cpu.last_control.begin();
      top.cpu.mem_wb.begin();
      top.cpu.memory.load_event.begin();
      top.cpu.memory.store_event.begin();
      top.cpu.memory.text.begin();
      top.cpu.memory.words.begin();
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
      top.cpu.system_event.begin();
      top.cpu.writeback.retired.begin();
      top.cpu.writeback.retirement.begin();
      compute_instruction_view_cache.reset();
      compute_id_ex_view_cache.reset();
      compute_shared_computation_cache.reset();
      compute_shared_computation_2_cache.reset();
      compute_shared_computation_3_cache.reset();
      compute_shared_computation_4_cache.reset();
      compute_shared_computation_5_cache.reset();
      compute_shared_computation_6_cache.reset();
      compute_stall_cache.reset();
      compute_id_ex_view_2_cache.reset();
      compute_ex_mem_view_cache.reset();
      compute_mem_wb_view_cache.reset();
      compute_shared_computation_7_cache.reset();
      compute_source_a_cache.reset();
      compute_source_b_cache.reset();
      compute_lhs_cache.reset();
      compute_rhs_cache.reset();
      compute_shared_computation_8_cache.reset();
      compute_id_ex_view_3_cache.reset();
      compute_shared_computation_9_cache.reset();
      compute_shared_computation_10_cache.reset();
      compute_shared_computation_11_cache.reset();
      compute_shared_computation_12_cache.reset();
      compute_executing_cache.reset();
      compute_pc_cache.reset();
      compute_opcode_cache.reset();
      compute_shared_computation_13_cache.reset();
      compute_shared_computation_14_cache.reset();
      compute_shared_computation_15_cache.reset();
      compute_shared_computation_16_cache.reset();
      compute_function_cache.reset();
      compute_shared_computation_17_cache.reset();
      compute_shared_computation_18_cache.reset();
      compute_shared_computation_19_cache.reset();
      compute_shared_computation_20_cache.reset();
      compute_shared_computation_21_cache.reset();
      compute_result_cache.reset();
      compute_shared_computation_22_cache.reset();
      compute_shared_computation_23_cache.reset();
      compute_jump_cache.reset();
      compute_shared_computation_24_cache.reset();
      compute_shared_computation_25_cache.reset();
      compute_shared_computation_26_cache.reset();
      compute_shared_computation_27_cache.reset();
      compute_shared_computation_28_cache.reset();
      compute_shared_computation_29_cache.reset();
      compute_shared_computation_30_cache.reset();
      compute_redirect_cache.reset();
      compute_shared_computation_31_cache.reset();
      compute_shared_computation_32_cache.reset();
      compute_shared_computation_33_cache.reset();
      compute_shared_computation_34_cache.reset();
      compute_shared_computation_35_cache.reset();
      compute_shared_computation_36_cache.reset();
      compute_shared_computation_37_cache.reset();
      compute_id_ex_view_4_cache.reset();
      compute_shared_computation_38_cache.reset();
      compute_id_ex_view_5_cache.reset();
      compute_shared_computation_39_cache.reset();
      compute_shared_computation_40_cache.reset();
      compute_ex_mem_view_2_cache.reset();
      compute_shared_computation_41_cache.reset();
      compute_shared_computation_42_cache.reset();
      compute_mem_wb_view_2_cache.reset();
      compute_shared_computation_43_cache.reset();
      compute_wait_cache.reset();
      compute_id_ex_view_6_cache.reset();
      compute_halted_view_cache.reset();
      compute_shared_computation_44_cache.reset();
      compute_id_ex_view_7_cache.reset();
      compute_shared_computation_45_cache.reset();
      compute_shared_computation_46_cache.reset();
      compute_shared_computation_47_cache.reset();
      compute_shared_computation_48_cache.reset();
      compute_shared_computation_49_cache.reset();
      compute_shared_computation_50_cache.reset();
      compute_shared_computation_51_cache.reset();
      compute_shared_computation_52_cache.reset();
      compute_shared_computation_53_cache.reset();
      compute_mem_wb_view_3_cache.reset();
      compute_shared_computation_54_cache.reset();
      compute_value0_view_cache.reset();
      compute_shared_computation_55_cache.reset();
      compute_shared_computation_56_cache.reset();
      compute_mem_wb_view_4_cache.reset();
      compute_shared_computation_57_cache.reset();
      compute_value0_view_2_cache.reset();
      compute_shared_computation_58_cache.reset();
      compute_shared_computation_59_cache.reset();
      compute_shared_computation_60_cache.reset();
      compute_mem_wb_view_5_cache.reset();
      compute_shared_computation_61_cache.reset();
      compute_value0_view_3_cache.reset();
      compute_number_cache.reset();
      compute_shared_computation_62_cache.reset();
      compute_shared_computation_63_cache.reset();
      compute_shared_computation_64_cache.reset();
      compute_shared_computation_65_cache.reset();
      compute_shared_computation_66_cache.reset();
      compute_shared_computation_67_cache.reset();
      compute_stopping_cache.reset();
      compute_shared_computation_68_cache.reset();
      compute_shared_computation_69_cache.reset();
      compute_shared_computation_70_cache.reset();
      compute_shared_computation_71_cache.reset();
      compute_shared_computation_72_cache.reset();
      compute_shared_computation_73_cache.reset();
      compute_shared_computation_74_cache.reset();
      fired_top_cpu_decode_stage_decode_instruction=false;
      fired_top_cpu_fetch_advance=false;
      fired_top_cpu_decode_stage_advance=false;
      fired_top_cpu_execute_advance=false;
      fired_top_cpu_memory_advance=false;
      fired_top_cpu_writeback_advance=false;
      fired_top_cpu_observe=false;
      fired_top_cpu_environment=false;
    }
    void commit() {
      top.cpu.decode_stage.instruction.commit();
      top.cpu.ex_mem.commit();
      top.cpu.exit_status.commit();
      top.cpu.fetch.pc.commit();
      top.cpu.fetch.rom.commit();
      top.cpu.halted.commit();
      top.cpu.id_ex.commit();
      top.cpu.if_id.commit();
      top.cpu.last_control.commit();
      top.cpu.mem_wb.commit();
      top.cpu.memory.load_event.commit();
      top.cpu.memory.store_event.commit();
      top.cpu.memory.text.commit();
      top.cpu.memory.words.commit();
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
      top.cpu.system_event.commit();
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
      top.cpu.try_environment(*this);
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
      o<<',';
      o<<"\"top.cpu.environment#7\":"; json(o,fired_top_cpu_environment);
      o<<"},\"state\":{";
      o<<"\"top.cpu.ex_mem\":"; json(o,top.cpu.ex_mem.value);
      o<<',';
      o<<"\"top.cpu.exit_status\":"; json(o,top.cpu.exit_status.value);
      o<<',';
      o<<"\"top.cpu.fetch.pc\":"; json(o,top.cpu.fetch.pc.value);
      o<<',';
      o<<"\"top.cpu.fetch.rom\":"; json(o,top.cpu.fetch.rom.value);
      o<<',';
      o<<"\"top.cpu.halted\":"; json(o,top.cpu.halted.value);
      o<<',';
      o<<"\"top.cpu.id_ex\":"; json(o,top.cpu.id_ex.value);
      o<<',';
      o<<"\"top.cpu.if_id\":"; json(o,top.cpu.if_id.value);
      o<<',';
      o<<"\"top.cpu.last_control\":"; json(o,top.cpu.last_control.value);
      o<<',';
      o<<"\"top.cpu.mem_wb\":"; json(o,top.cpu.mem_wb.value);
      o<<',';
      o<<"\"top.cpu.memory.load_event\":"; json(o,top.cpu.memory.load_event.value);
      o<<',';
      o<<"\"top.cpu.memory.store_event\":"; json(o,top.cpu.memory.store_event.value);
      o<<',';
      o<<"\"top.cpu.memory.text\":"; json(o,top.cpu.memory.text.value);
      o<<',';
      o<<"\"top.cpu.memory.words\":"; json(o,top.cpu.memory.words.value);
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
      o<<"\"top.cpu.system_event\":"; json(o,top.cpu.system_event.value);
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
    const std::tuple<bool> &compute_shared_computation() {
      if (!compute_shared_computation_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view());
        const bool ready = id_ex_value.valid;
        compute_shared_computation_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_cache;
    }
    const std::tuple<bool> &compute_shared_computation_2() {
      if (!compute_shared_computation_2_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view());
        const Instruction executing = id_ex_value.instruction;
        const bool ready = executing.load;
        compute_shared_computation_2_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_2_cache;
    }
    const std::tuple<uint32_t> &compute_shared_computation_3() {
      if (!compute_shared_computation_3_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view());
        const Instruction executing = id_ex_value.instruction;
        const uint32_t rd = executing.rd;
        compute_shared_computation_3_cache.emplace(std::tuple<uint32_t>{rd});
      }
      return *compute_shared_computation_3_cache;
    }
    const std::tuple<bool> &compute_shared_computation_4() {
      if (!compute_shared_computation_4_cache) {
        const uint32_t rd = std::get<0>(this->compute_shared_computation_3());
        const bool condition = (rd!=0u);
        compute_shared_computation_4_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_4_cache;
    }
    const std::tuple<uint32_t> &compute_shared_computation_5() {
      if (!compute_shared_computation_5_cache) {
        const Instruction instruction_value = std::get<0>(this->compute_instruction_view());
        const uint32_t rs2 = instruction_value.rs2;
        compute_shared_computation_5_cache.emplace(std::tuple<uint32_t>{rs2});
      }
      return *compute_shared_computation_5_cache;
    }
    const std::tuple<bool> &compute_shared_computation_6() {
      if (!compute_shared_computation_6_cache) {
        const uint32_t rd = std::get<0>(this->compute_shared_computation_3());
        const Instruction instruction_value = std::get<0>(this->compute_instruction_view());
        const uint32_t rs1 = instruction_value.rs1;
        const bool condition = (rd!=rs1);
        compute_shared_computation_6_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_6_cache;
    }
    const std::tuple<bool> &compute_stall() {
      if (!compute_stall_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_2());
        const bool condition = std::get<0>(this->compute_shared_computation_4());
        const bool ready_4 = ((ready && ready_2) && condition);
        const uint32_t rd = std::get<0>(this->compute_shared_computation_3());
        Instruction instruction_value{};
        if ((ready && (ready_2 && condition))) {
          instruction_value = std::get<0>(this->compute_instruction_view());
        }
        const uint32_t rs1 = instruction_value.rs1;
        const bool condition_2 = (rd==rs1);
        uint32_t rs2{};
        if ((ready && (ready_2 && (condition && (!condition_2))))) {
          rs2 = std::get<0>(this->compute_shared_computation_5());
        }
        const bool stall = (ready_4 && (condition_2 || (rd==rs2)));
        compute_stall_cache.emplace(std::tuple<bool>{stall});
      }
      return *compute_stall_cache;
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
    const std::tuple<uint32_t> &compute_shared_computation_7() {
      if (!compute_shared_computation_7_cache) {
        const uint32_t payload = uint32_t{};
        compute_shared_computation_7_cache.emplace(std::tuple<uint32_t>{payload});
      }
      return *compute_shared_computation_7_cache;
    }
    const std::tuple<uint32_t> &compute_source_a() {
      if (!compute_source_a_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_2());
        const Instruction instruction = id_ex_value.instruction;
        const uint32_t index = instruction.rs1;
        const Executed ex_mem_value = std::get<0>(this->compute_ex_mem_view());
        const bool ready_3 = (ex_mem_value.valid && ex_mem_value.writes);
        const uint32_t rd = ex_mem_value.rd;
        const bool ready_4 = (ready_3 && (rd==index));
        const bool ready_5 = (!ready_4);
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view());
        const bool ready_8 = (mem_wb_value.valid && mem_wb_value.writes);
        const uint32_t rd_2 = mem_wb_value.rd;
        const bool ready_9 = (ready_8 && (rd_2==index));
        const uint32_t payload = std::get<0>(this->compute_shared_computation_7());
        const uint32_t source_a = (((index==0u) || (ready_4 ? ready_5 : (ready_9 ? false : ready_5))) ? 0u : ((index!=0u) ? (ready_4 ? 1u : (ready_9 ? 2u : (ready_4 ? 1u : payload))) : payload));
        compute_source_a_cache.emplace(std::tuple<uint32_t>{source_a});
      }
      return *compute_source_a_cache;
    }
    const std::tuple<uint32_t> &compute_source_b() {
      if (!compute_source_b_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_2());
        const Instruction instruction = id_ex_value.instruction;
        const uint32_t index = instruction.rs2;
        const Executed ex_mem_value = std::get<0>(this->compute_ex_mem_view());
        const bool ready_3 = (ex_mem_value.valid && ex_mem_value.writes);
        const uint32_t rd = ex_mem_value.rd;
        const bool ready_4 = (ready_3 && (rd==index));
        const bool ready_5 = (!ready_4);
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view());
        const bool ready_8 = (mem_wb_value.valid && mem_wb_value.writes);
        const uint32_t rd_2 = mem_wb_value.rd;
        const bool ready_9 = (ready_8 && (rd_2==index));
        const uint32_t payload = std::get<0>(this->compute_shared_computation_7());
        const uint32_t source_b = (((index==0u) || (ready_4 ? ready_5 : (ready_9 ? false : ready_5))) ? 0u : ((index!=0u) ? (ready_4 ? 1u : (ready_9 ? 2u : (ready_4 ? 1u : payload))) : payload));
        compute_source_b_cache.emplace(std::tuple<uint32_t>{source_b});
      }
      return *compute_source_b_cache;
    }
    const std::tuple<uint32_t> &compute_lhs() {
      if (!compute_lhs_cache) {
        const uint32_t source_a = std::get<0>(this->compute_source_a());
        const bool condition = (source_a!=1u);
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_2());
        const uint32_t original = id_ex_value.lhs;
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view());
        const uint32_t value = mem_wb_value.value;
        const bool condition_4 = (source_a==1u);
        const Executed ex_mem_value = std::get<0>(this->compute_ex_mem_view());
        const uint32_t alu = ex_mem_value.alu;
        const uint32_t payload = std::get<0>(this->compute_shared_computation_7());
        const uint32_t selected_value = (condition_4 ? alu : payload);
        const uint32_t lhs = (condition ? ((source_a!=2u) ? original : ((source_a==2u) ? value : selected_value)) : selected_value);
        compute_lhs_cache.emplace(std::tuple<uint32_t>{lhs});
      }
      return *compute_lhs_cache;
    }
    const std::tuple<uint32_t> &compute_rhs() {
      if (!compute_rhs_cache) {
        const uint32_t source_b = std::get<0>(this->compute_source_b());
        const bool condition = (source_b!=1u);
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_2());
        const uint32_t original = id_ex_value.rhs;
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view());
        const uint32_t value = mem_wb_value.value;
        const bool condition_4 = (source_b==1u);
        const Executed ex_mem_value = std::get<0>(this->compute_ex_mem_view());
        const uint32_t alu = ex_mem_value.alu;
        const uint32_t payload = std::get<0>(this->compute_shared_computation_7());
        const uint32_t selected_value = (condition_4 ? alu : payload);
        const uint32_t rhs = (condition ? ((source_b!=2u) ? original : ((source_b==2u) ? value : selected_value)) : selected_value);
        compute_rhs_cache.emplace(std::tuple<uint32_t>{rhs});
      }
      return *compute_rhs_cache;
    }
    const std::tuple<bool> &compute_shared_computation_8() {
      if (!compute_shared_computation_8_cache) {
        const uint32_t source_a = std::get<0>(this->compute_source_a());
        const bool condition = (source_a!=1u);
        const bool condition_2 = (source_a==1u);
        const uint32_t source_b = std::get<0>(this->compute_source_b());
        const bool condition_3 = (source_b!=1u);
        const bool condition_4 = (source_b==1u);
        const bool ready_7 = ((condition || (condition_2 && (condition_2 || condition))) && (condition_3 || (condition_4 && (condition_4 || condition_3))));
        compute_shared_computation_8_cache.emplace(std::tuple<bool>{ready_7});
      }
      return *compute_shared_computation_8_cache;
    }
    const std::tuple<Decoded,bool> &compute_id_ex_view_3() {
      if (!compute_id_ex_view_3_cache) {
        const Decoded id_ex_value = top.cpu.id_ex.read();
        compute_id_ex_view_3_cache.emplace(std::tuple<Decoded,bool>{id_ex_value,true});
      }
      return *compute_id_ex_view_3_cache;
    }
    const std::tuple<bool> &compute_shared_computation_9() {
      if (!compute_shared_computation_9_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_3());
        const bool ready = id_ex_value.valid;
        compute_shared_computation_9_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_9_cache;
    }
    const std::tuple<bool> &compute_shared_computation_10() {
      if (!compute_shared_computation_10_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_3());
        const bool ready = id_ex_value.fetch_fault;
        compute_shared_computation_10_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_10_cache;
    }
    const std::tuple<bool> &compute_shared_computation_11() {
      if (!compute_shared_computation_11_cache) {
        const bool ready = acir_sim::check(false);
        compute_shared_computation_11_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_11_cache;
    }
    const std::tuple<bool> &compute_shared_computation_12() {
      if (!compute_shared_computation_12_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_9());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_10());
        bool ready_5{};
        if ((ready && ready_2)) {
          ready_5 = std::get<0>(this->compute_shared_computation_11());
        }
        const bool ready_6 = ((!(ready && ready_2)) || ready_5);
        compute_shared_computation_12_cache.emplace(std::tuple<bool>{ready_6});
      }
      return *compute_shared_computation_12_cache;
    }
    const std::tuple<Instruction> &compute_executing() {
      if (!compute_executing_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_3());
        const Instruction executing = id_ex_value.instruction;
        compute_executing_cache.emplace(std::tuple<Instruction>{executing});
      }
      return *compute_executing_cache;
    }
    const std::tuple<uint32_t> &compute_pc() {
      if (!compute_pc_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_3());
        const uint32_t pc = id_ex_value.pc;
        compute_pc_cache.emplace(std::tuple<uint32_t>{pc});
      }
      return *compute_pc_cache;
    }
    const std::tuple<uint32_t> &compute_opcode() {
      if (!compute_opcode_cache) {
        const Instruction executing = std::get<0>(this->compute_executing());
        const uint32_t opcode = executing.opcode;
        compute_opcode_cache.emplace(std::tuple<uint32_t>{opcode});
      }
      return *compute_opcode_cache;
    }
    const std::tuple<bool> &compute_shared_computation_13() {
      if (!compute_shared_computation_13_cache) {
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition = (opcode==111u);
        compute_shared_computation_13_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_13_cache;
    }
    const std::tuple<bool> &compute_shared_computation_14() {
      if (!compute_shared_computation_14_cache) {
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition = (opcode==99u);
        compute_shared_computation_14_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_14_cache;
    }
    const std::tuple<bool> &compute_shared_computation_15() {
      if (!compute_shared_computation_15_cache) {
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition = (opcode==103u);
        compute_shared_computation_15_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_15_cache;
    }
    const std::tuple<bool> &compute_shared_computation_16() {
      if (!compute_shared_computation_16_cache) {
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition = (opcode!=103u);
        compute_shared_computation_16_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_16_cache;
    }
    const std::tuple<uint32_t> &compute_function() {
      if (!compute_function_cache) {
        const Instruction executing = std::get<0>(this->compute_executing());
        const uint32_t function = executing.funct3;
        compute_function_cache.emplace(std::tuple<uint32_t>{function});
      }
      return *compute_function_cache;
    }
    const std::tuple<bool> &compute_shared_computation_17() {
      if (!compute_shared_computation_17_cache) {
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition = (function!=0u);
        compute_shared_computation_17_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_17_cache;
    }
    const std::tuple<bool> &compute_shared_computation_18() {
      if (!compute_shared_computation_18_cache) {
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition = (function!=1u);
        compute_shared_computation_18_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_18_cache;
    }
    const std::tuple<bool> &compute_shared_computation_19() {
      if (!compute_shared_computation_19_cache) {
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition = (function!=5u);
        compute_shared_computation_19_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_19_cache;
    }
    const std::tuple<bool> &compute_shared_computation_20() {
      if (!compute_shared_computation_20_cache) {
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition = (function!=4u);
        compute_shared_computation_20_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_20_cache;
    }
    const std::tuple<bool> &compute_shared_computation_21() {
      if (!compute_shared_computation_21_cache) {
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition = (function!=6u);
        compute_shared_computation_21_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_21_cache;
    }
    const std::tuple<uint32_t> &compute_result() {
      if (!compute_result_cache) {
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition = (opcode!=55u);
        const bool condition_3 = std::get<0>(this->compute_shared_computation_13());
        const bool condition_4 = std::get<0>(this->compute_shared_computation_14());
        const bool ready_2 = (((opcode==23u) || condition_3) || condition_4);
        const uint32_t pc = std::get<0>(this->compute_pc());
        const Instruction executing = std::get<0>(this->compute_executing());
        const uint32_t immediate = executing.immediate;
        const bool condition_5 = std::get<0>(this->compute_shared_computation_15());
        const bool ready_3 = (!ready_2);
        const bool selected_value = (condition_5 ? false : ready_3);
        const bool ready_4 = ((opcode==3u) || (opcode==35u));
        const uint32_t lhs = std::get<0>(this->compute_lhs());
        const uint32_t computed_value_2 = (lhs + immediate);
        const bool condition_8 = (opcode==51u);
        const uint32_t funct7 = executing.funct7;
        const bool ready_5 = (condition_8 && (funct7==1u));
        const bool ready_6 = (!ready_4);
        const bool selected_value_2 = (ready_5 ? false : ready_6);
        const bool condition_10 = (opcode==19u);
        const bool ready_7 = (condition_8 || condition_10);
        const bool condition_11 = std::get<0>(this->compute_shared_computation_17());
        const bool condition_12 = std::get<0>(this->compute_shared_computation_18());
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition_13 = (function!=2u);
        const bool condition_14 = (function!=3u);
        const bool condition_15 = std::get<0>(this->compute_shared_computation_20());
        const bool condition_16 = std::get<0>(this->compute_shared_computation_19());
        const bool condition_17 = std::get<0>(this->compute_shared_computation_21());
        const uint32_t rhs = std::get<0>(this->compute_rhs());
        const uint32_t operand = (condition_10 ? immediate : rhs);
        const bool condition_18 = (function==6u);
        const bool condition_19 = (function==5u);
        const bool condition_20 = (funct7==32u);
        const uint32_t computed_value_5 = (operand & 31u);
        const uint32_t computed_value_6 = (lhs & 2147483648u);
        const bool condition_22 = (computed_value_6!=0u);
        const bool condition_path = (condition_11 && (condition_12 && (condition_13 && (condition_14 && (condition_17 ? false : (!condition_18))))));
        const bool condition_path_2 = (condition_19 && (condition_15 && (condition_11 && (condition_12 && (condition_13 && condition_14)))));
        const bool condition_path_3 = ((funct7==1u) ? false : (condition_16 ? (condition_19 && (condition_15 && condition_path)) : condition_path_2));
        const bool condition_path_4 = (condition_15 && (condition_10 && (condition_11 && (condition_12 && (condition_13 && condition_14)))));
        const bool condition_path_5 = (condition_16 ? (condition_19 && (condition_15 && (condition_10 && condition_path))) : (condition_19 && condition_path_4));
        const bool condition_path_6 = ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_8 ? condition_path_3 : condition_path_5)));
        const bool condition_path_7 = ((opcode==23u) ? false : (condition_3 ? false : (condition_4 ? false : (condition_5 ? false : condition_path_6))));
        uint32_t computed_value_7{};
        if ((condition && condition_path_7)) {
          computed_value_7 = acir_sim::shift(lhs,computed_value_5,1);
        }
        const bool condition_path_8 = (condition_14 && (condition_20 && ((computed_value_5!=0u) && (condition_17 ? false : (!condition_18)))));
        const bool condition_path_9 = (condition_15 && (condition_22 && (condition_11 && (condition_12 && (condition_13 && condition_path_8)))));
        const bool condition_path_10 = (condition_11 && (condition_12 && (condition_13 && (condition_14 && (condition_20 && (computed_value_5!=0u))))));
        const bool condition_path_11 = (condition_16 ? (condition_19 && condition_path_9) : (condition_19 && (condition_15 && (condition_22 && condition_path_10))));
        const bool condition_path_12 = (condition_22 && (condition_10 && (condition_11 && (condition_12 && (condition_13 && condition_path_8)))));
        const bool condition_path_13 = (condition_16 ? (condition_19 && (condition_15 && condition_path_12)) : (condition_19 && (condition_15 && (condition_22 && (condition_10 && condition_path_10)))));
        const bool condition_path_14 = ((opcode==35u) ? false : (condition_8 ? ((funct7==1u) ? false : condition_path_11) : condition_path_13));
        const bool condition_path_15 = (condition_3 ? false : (condition_4 ? false : (condition_5 ? false : ((opcode==3u) ? false : condition_path_14))));
        uint32_t computed_value_9{};
        if ((condition && ((opcode==23u) ? false : condition_path_15))) {
          computed_value_9 = acir_sim::shift(4294967295u,(32u - computed_value_5),0);
        }
        const uint32_t result = (computed_value_7 | computed_value_9);
        const uint32_t result_2 = (((computed_value_5!=0u) && condition_22) ? result : computed_value_7);
        const bool condition_23 = (function==4u);
        const bool condition_24 = (function==3u);
        const bool condition_26 = (function==2u);
        const uint32_t computed_value_12 = (lhs ^ 2147483648u);
        const bool condition_28 = (function==1u);
        const bool condition_path_16 = (condition_23 ? (condition_11 && (condition_12 ? false : condition_28)) : (condition_11 && (condition_12 ? (condition_28 && (condition_13 && condition_14)) : condition_28)));
        const bool condition_path_17 = (condition_12 ? (condition_28 && (condition_13 && (condition_14 && (condition_17 ? false : (!condition_18))))) : condition_28);
        const bool condition_path_18 = (condition_23 ? (condition_11 && (condition_12 ? false : condition_28)) : (condition_11 && condition_path_17));
        const bool condition_path_19 = (condition_19 ? (condition_15 ? (condition_11 && (condition_12 ? false : condition_28)) : condition_path_16) : (condition_15 ? condition_path_18 : condition_path_16));
        const bool condition_path_20 = (condition_19 ? (condition_15 ? (condition_11 && (condition_12 ? false : condition_28)) : condition_path_16) : condition_path_16);
        const bool condition_path_21 = (condition_10 && (condition_11 && (condition_12 ? (condition_28 && (condition_13 && condition_14)) : condition_28)));
        const bool condition_path_22 = (condition_23 ? (condition_10 && (condition_11 && (condition_12 ? false : condition_28))) : condition_path_21);
        const bool condition_path_23 = (condition_15 ? (condition_10 && (condition_11 && (condition_12 ? false : condition_28))) : condition_path_22);
        const bool condition_path_24 = (condition_23 ? (condition_10 && (condition_11 && (condition_12 ? false : condition_28))) : (condition_10 && (condition_11 && condition_path_17)));
        const bool condition_path_25 = (condition_16 ? (condition_19 ? condition_path_23 : (condition_15 ? condition_path_24 : condition_path_22)) : (condition_19 ? condition_path_23 : condition_path_22));
        const bool condition_path_26 = (condition_8 ? ((funct7==1u) ? false : (condition_16 ? condition_path_19 : condition_path_20)) : condition_path_25);
        const bool condition_path_27 = (condition_4 ? false : (condition_5 ? false : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_26))));
        uint32_t computed_value_15{};
        if ((condition && ((opcode==23u) ? false : (condition_3 ? false : condition_path_27)))) {
          computed_value_15 = acir_sim::shift(lhs,computed_value_5,0);
        }
        const bool condition_29 = (function==0u);
        const bool condition_30 = (function>3u);
        const bool condition_31 = (rhs!=0u);
        const bool condition_32 = (function!=7u);
        const uint32_t a = (condition_22 ? (0u - lhs) : lhs);
        const bool condition_33 = ((rhs & 2147483648u)!=0u);
        const uint32_t b = (condition_33 ? (0u - rhs) : rhs);
        const bool condition_path_28 = ((funct7==1u) && (condition_30 && (condition_31 && (condition_16 && (condition_32 && condition_15)))));
        const bool condition_path_29 = (condition_5 ? false : ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_8 && condition_path_28))));
        const bool condition_path_30 = (condition && ((opcode==23u) ? false : (condition_3 ? false : (condition_4 ? false : condition_path_29))));
        uint32_t remainder{};
        if (condition_path_30) {
          remainder = acir_sim::divide(a,b,false,true,false);
        }
        const bool condition_34 = (condition_22!=condition_33);
        const bool condition_path_31 = (condition_30 && (condition_31 && (condition_16 && (condition_32 && (condition_15 ? false : condition_23)))));
        const bool condition_path_32 = ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_8 && ((funct7==1u) && condition_path_31))));
        const bool condition_path_33 = ((opcode==23u) ? false : (condition_3 ? false : (condition_4 ? false : (condition_5 ? false : condition_path_32))));
        uint32_t quotient{};
        if ((condition && condition_path_33)) {
          quotient = acir_sim::divide(a,b,false,false,false);
        }
        const bool condition_35 = (function==7u);
        const bool condition_path_34 = (condition_8 && ((funct7==1u) && (condition_30 && (condition_31 && (condition_16 && (!condition_32))))));
        const bool condition_path_35 = (condition_4 ? false : (condition_5 ? false : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_34))));
        uint32_t computed_value_23{};
        if ((condition && ((opcode==23u) ? false : (condition_3 ? false : condition_path_35)))) {
          computed_value_23 = acir_sim::divide(lhs,rhs,false,true,false);
        }
        const bool condition_path_36 = (condition_16 ? (condition_19 && (condition_32 && (condition_15 ? false : (!condition_23)))) : condition_19);
        const bool condition_path_37 = ((opcode==35u) ? false : (condition_8 && ((funct7==1u) && (condition_30 && (condition_31 && condition_path_36)))));
        const bool condition_path_38 = (condition_3 ? false : (condition_4 ? false : (condition_5 ? false : ((opcode==3u) ? false : condition_path_37))));
        uint32_t computed_value_24{};
        if ((condition && ((opcode==23u) ? false : condition_path_38))) {
          computed_value_24 = acir_sim::divide(lhs,rhs,false,false,false);
        }
        const bool condition_36 = (rhs==0u);
        const bool condition_37 = (function<=3u);
        const bool ready_11 = (condition_28 && condition_33);
        const bool ready_12 = (condition_28 || condition_26);
        const uint64_t wide = (uint64_t(lhs) * uint64_t(rhs));
        const uint32_t high = uint32_t((wide >> 32u));
        const uint32_t high_2 = (high - rhs);
        const uint32_t high_3 = ((ready_12 && condition_22) ? high_2 : high);
        const uint32_t high_4 = (high_3 - lhs);
        const uint32_t high_5 = (ready_11 ? high_4 : high_3);
        const uint32_t payload = std::get<0>(this->compute_shared_computation_7());
        const uint32_t selected_value_10 = (condition_37 ? (condition_11 ? high_5 : (condition_29 ? uint32_t(wide) : payload)) : payload);
        const uint32_t selected_value_11 = (condition_36 ? ((condition_23 || condition_19) ? 4294967295u : lhs) : selected_value_10);
        const uint32_t selected_value_12 = (condition_19 ? computed_value_24 : selected_value_11);
        const uint32_t selected_value_13 = (condition_35 ? computed_value_23 : selected_value_12);
        const uint32_t selected_value_15 = (condition_15 ? (condition_22 ? (0u - remainder) : remainder) : (condition_23 ? (condition_34 ? (0u - quotient) : quotient) : selected_value_13));
        const uint32_t selected_value_18 = (condition_31 ? (condition_16 ? (condition_32 ? selected_value_15 : selected_value_13) : selected_value_12) : selected_value_11);
        const bool condition_38 = (opcode==55u);
        const uint32_t selected_value_20 = (condition_38 ? immediate : payload);
        const uint32_t selected_value_21 = (condition_5 ? (computed_value_2 & 4294967294u) : selected_value_20);
        const uint32_t selected_value_22 = (ready_5 ? (condition_30 ? selected_value_18 : selected_value_10) : selected_value_21);
        const uint32_t selected_value_23 = (condition_29 ? ((condition_8 && condition_20) ? (lhs - operand) : (lhs + operand)) : selected_value_22);
        const uint32_t selected_value_24 = (condition_28 ? computed_value_15 : selected_value_23);
        const uint32_t selected_value_25 = (condition_26 ? uint32_t((computed_value_12<(operand ^ 2147483648u))) : selected_value_24);
        const uint32_t selected_value_26 = (condition_24 ? uint32_t((lhs<operand)) : selected_value_25);
        const uint32_t selected_value_27 = (condition_23 ? (lhs ^ operand) : selected_value_26);
        const uint32_t selected_value_28 = (condition_19 ? (condition_20 ? result_2 : computed_value_7) : selected_value_27);
        const uint32_t selected_value_31 = (condition_16 ? (condition_17 ? (lhs & operand) : (condition_18 ? (lhs | operand) : selected_value_28)) : selected_value_28);
        const uint32_t selected_value_34 = (condition_13 ? (condition_14 ? (condition_15 ? selected_value_31 : selected_value_27) : selected_value_26) : selected_value_25);
        const uint32_t selected_value_37 = (ready_7 ? (condition_11 ? (condition_12 ? selected_value_34 : selected_value_24) : selected_value_23) : 0u);
        const uint32_t selected_value_40 = (selected_value ? (ready_4 ? computed_value_2 : (selected_value_2 ? selected_value_37 : selected_value_22)) : selected_value_21);
        const uint32_t result_3 = (condition ? (ready_2 ? (pc + immediate) : selected_value_40) : selected_value_20);
        compute_result_cache.emplace(std::tuple<uint32_t>{result_3});
      }
      return *compute_result_cache;
    }
    const std::tuple<bool> &compute_shared_computation_22() {
      if (!compute_shared_computation_22_cache) {
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition = (opcode!=55u);
        const bool condition_3 = std::get<0>(this->compute_shared_computation_13());
        const bool condition_4 = std::get<0>(this->compute_shared_computation_14());
        const bool ready_2 = (((opcode==23u) || condition_3) || condition_4);
        const bool ready_3 = (!ready_2);
        const bool condition_5 = std::get<0>(this->compute_shared_computation_15());
        const bool selected_value = (condition_5 ? false : ready_3);
        const bool ready_4 = ((opcode==3u) || (opcode==35u));
        const bool ready_5 = (!ready_4);
        const bool ready_9 = acir_sim::check((!(condition && (ready_2 ? ready_3 : (selected_value && (ready_4 ? ready_5 : false))))));
        compute_shared_computation_22_cache.emplace(std::tuple<bool>{ready_9});
      }
      return *compute_shared_computation_22_cache;
    }
    const std::tuple<bool> &compute_shared_computation_23() {
      if (!compute_shared_computation_23_cache) {
        const Instruction executing = std::get<0>(this->compute_executing());
        const bool ready = executing.illegal;
        compute_shared_computation_23_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_23_cache;
    }
    const std::tuple<bool> &compute_jump() {
      if (!compute_jump_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_23());
        const bool ready_2 = (!ready);
        const bool condition = std::get<0>(this->compute_shared_computation_13());
        const bool condition_2 = std::get<0>(this->compute_shared_computation_15());
        const bool jump = (ready_2 && (condition || condition_2));
        compute_jump_cache.emplace(std::tuple<bool>{jump});
      }
      return *compute_jump_cache;
    }
    const std::tuple<bool> &compute_shared_computation_24() {
      if (!compute_shared_computation_24_cache) {
        const uint32_t lhs = std::get<0>(this->compute_lhs());
        const uint32_t rhs = std::get<0>(this->compute_rhs());
        const bool condition = (lhs==rhs);
        compute_shared_computation_24_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_24_cache;
    }
    const std::tuple<bool> &compute_shared_computation_25() {
      if (!compute_shared_computation_25_cache) {
        const uint32_t lhs = std::get<0>(this->compute_lhs());
        const uint32_t rhs = std::get<0>(this->compute_rhs());
        const bool condition = (lhs!=rhs);
        compute_shared_computation_25_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_25_cache;
    }
    const std::tuple<bool> &compute_shared_computation_26() {
      if (!compute_shared_computation_26_cache) {
        const uint32_t lhs = std::get<0>(this->compute_lhs());
        const uint32_t computed_value = (lhs ^ 2147483648u);
        const uint32_t rhs = std::get<0>(this->compute_rhs());
        const uint32_t computed_value_2 = (rhs ^ 2147483648u);
        const bool condition = (computed_value<computed_value_2);
        compute_shared_computation_26_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_26_cache;
    }
    const std::tuple<bool> &compute_shared_computation_27() {
      if (!compute_shared_computation_27_cache) {
        const uint32_t lhs = std::get<0>(this->compute_lhs());
        const uint32_t computed_value = (lhs ^ 2147483648u);
        const uint32_t rhs = std::get<0>(this->compute_rhs());
        const uint32_t computed_value_2 = (rhs ^ 2147483648u);
        const bool condition = (computed_value>=computed_value_2);
        compute_shared_computation_27_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_27_cache;
    }
    const std::tuple<bool> &compute_shared_computation_28() {
      if (!compute_shared_computation_28_cache) {
        const uint32_t lhs = std::get<0>(this->compute_lhs());
        const uint32_t rhs = std::get<0>(this->compute_rhs());
        const bool condition = (lhs<rhs);
        compute_shared_computation_28_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_28_cache;
    }
    const std::tuple<bool> &compute_shared_computation_29() {
      if (!compute_shared_computation_29_cache) {
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition = (function==7u);
        const bool ready = acir_sim::check(condition);
        compute_shared_computation_29_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_29_cache;
    }
    const std::tuple<bool> &compute_shared_computation_30() {
      if (!compute_shared_computation_30_cache) {
        const uint32_t lhs = std::get<0>(this->compute_lhs());
        const uint32_t rhs = std::get<0>(this->compute_rhs());
        const bool condition = (lhs>=rhs);
        compute_shared_computation_30_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_30_cache;
    }
    const std::tuple<bool> &compute_redirect() {
      if (!compute_redirect_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_9());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_23());
        const bool ready_3 = (!ready_2);
        const bool ready_4 = (ready && ready_3);
        const bool jump = std::get<0>(this->compute_jump());
        const bool condition = std::get<0>(this->compute_shared_computation_14());
        const bool condition_2 = std::get<0>(this->compute_shared_computation_17());
        const bool condition_3 = std::get<0>(this->compute_shared_computation_18());
        const bool condition_4 = std::get<0>(this->compute_shared_computation_20());
        const bool condition_5 = std::get<0>(this->compute_shared_computation_19());
        const bool condition_6 = std::get<0>(this->compute_shared_computation_21());
        const bool condition_7 = std::get<0>(this->compute_shared_computation_30());
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition_8 = (function==6u);
        const bool condition_9 = std::get<0>(this->compute_shared_computation_28());
        const bool condition_10 = (function==5u);
        const bool condition_11 = std::get<0>(this->compute_shared_computation_27());
        const bool condition_12 = (function==4u);
        const bool condition_13 = std::get<0>(this->compute_shared_computation_26());
        const bool condition_14 = (function==1u);
        const bool condition_15 = std::get<0>(this->compute_shared_computation_25());
        const bool condition_16 = (function==0u);
        const bool condition_17 = std::get<0>(this->compute_shared_computation_24());
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition_18 = (opcode!=99u);
        const bool selected_value = (condition_18 ? false : bool{});
        const bool selected_value_2 = (condition_16 ? condition_17 : selected_value);
        const bool selected_value_3 = (condition_14 ? condition_15 : selected_value_2);
        const bool selected_value_4 = (condition_12 ? condition_13 : selected_value_3);
        const bool selected_value_5 = (condition_10 ? condition_11 : selected_value_4);
        const bool selected_value_8 = (condition_5 ? (condition_6 ? condition_7 : (condition_8 ? condition_9 : selected_value_5)) : selected_value_5);
        const bool selected_value_11 = (condition_2 ? (condition_3 ? (condition_4 ? selected_value_8 : selected_value_4) : selected_value_3) : selected_value_2);
        const bool redirect = (ready_4 && (jump || (condition ? selected_value_11 : selected_value)));
        compute_redirect_cache.emplace(std::tuple<bool>{redirect});
      }
      return *compute_redirect_cache;
    }
    const std::tuple<bool> &compute_shared_computation_31() {
      if (!compute_shared_computation_31_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_9());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_23());
        const bool ready_3 = (!ready_2);
        const bool ready_4 = (ready && ready_3);
        const bool jump = std::get<0>(this->compute_jump());
        const bool condition = std::get<0>(this->compute_shared_computation_14());
        const bool condition_2 = std::get<0>(this->compute_shared_computation_17());
        const bool condition_3 = std::get<0>(this->compute_shared_computation_18());
        const bool condition_4 = std::get<0>(this->compute_shared_computation_20());
        const bool condition_5 = std::get<0>(this->compute_shared_computation_19());
        const bool condition_6 = std::get<0>(this->compute_shared_computation_21());
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition_7 = (function==6u);
        const bool condition_8 = (function==5u);
        const bool condition_9 = (function==4u);
        const bool condition_10 = (function==1u);
        const bool condition_11 = (function==0u);
        const bool ready_6 = (condition_11 || condition_2);
        const bool ready_8 = (condition_10 || (condition_3 && ready_6));
        const bool ready_10 = (condition_9 || (condition_4 && ready_8));
        const bool ready_12 = (condition_8 || (condition_5 && ready_10));
        const bool ready_19 = ((condition_5 && (condition_6 || (condition_7 && (condition_7 || (condition_6 && ready_12))))) || (condition_8 && ready_12));
        const bool ready_25 = ((condition_3 && ((condition_4 && ready_19) || (condition_9 && ready_10))) || (condition_10 && ready_8));
        const bool ready_27 = (condition_11 && ready_6);
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition_12 = (opcode!=99u);
        const bool condition_path = (condition_8 ? false : (condition_9 ? false : (condition_10 ? false : (condition_11 ? false : (!condition_12)))));
        const bool condition_path_2 = (condition_3 && (condition_4 && (condition_5 && (condition_6 && (condition_7 ? false : condition_path)))));
        bool ready_31{};
        if ((ready && (ready_2 ? false : (jump ? false : (condition && (condition_2 && condition_path_2)))))) {
          ready_31 = std::get<0>(this->compute_shared_computation_29());
        }
        const bool ready_32 = (condition_7 || ready_31);
        const bool ready_33 = (condition_8 || ready_32);
        const bool ready_34 = (condition_9 || ready_33);
        const bool ready_35 = (condition_10 || ready_34);
        const bool ready_36 = (condition_11 || ready_35);
        const bool ready_37 = (condition_12 || ready_36);
        const bool ready_38 = (((condition && ((condition_2 && ready_25) || ready_27)) || condition_12) && ready_37);
        const bool ready_42 = (!ready_4);
        const bool ready_43 = ((ready_4 && (jump || ((!jump) && ready_38))) || ready_42);
        compute_shared_computation_31_cache.emplace(std::tuple<bool>{ready_43});
      }
      return *compute_shared_computation_31_cache;
    }
    const std::tuple<bool> &compute_shared_computation_32() {
      if (!compute_shared_computation_32_cache) {
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition = (opcode==55u);
        const bool condition_3 = std::get<0>(this->compute_shared_computation_13());
        const bool condition_4 = std::get<0>(this->compute_shared_computation_14());
        const bool ready_2 = (((opcode==23u) || condition_3) || condition_4);
        const bool condition_5 = std::get<0>(this->compute_shared_computation_15());
        const bool ready_3 = (!ready_2);
        const bool selected_value = (condition_5 ? false : ready_3);
        const bool ready_4 = ((opcode==3u) || (opcode==35u));
        const bool ready_5 = std::get<0>(this->compute_shared_computation_8());
        const bool condition_8 = (opcode==51u);
        const Instruction executing = std::get<0>(this->compute_executing());
        const uint32_t funct7 = executing.funct7;
        const bool ready_6 = (condition_8 && (funct7==1u));
        const bool ready_7 = (!ready_4);
        const bool selected_value_2 = (ready_6 ? false : ready_7);
        const bool condition_10 = (opcode==19u);
        const bool ready_8 = (condition_8 || condition_10);
        const bool condition_11 = std::get<0>(this->compute_shared_computation_17());
        const bool condition_12 = std::get<0>(this->compute_shared_computation_18());
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition_13 = (function!=2u);
        const bool condition_14 = (function!=3u);
        const bool condition_15 = std::get<0>(this->compute_shared_computation_20());
        const bool condition_16 = std::get<0>(this->compute_shared_computation_19());
        const bool condition_17 = std::get<0>(this->compute_shared_computation_21());
        const bool ready_9 = (condition_10 || ready_5);
        const bool ready_10 = (ready_5 && ready_9);
        const bool condition_18 = (function==6u);
        const bool condition_19 = (function==5u);
        const bool condition_20 = (funct7==32u);
        const uint32_t immediate = executing.immediate;
        const uint32_t rhs = std::get<0>(this->compute_rhs());
        const uint32_t operand = (condition_10 ? immediate : rhs);
        const uint32_t computed_value = (operand & 31u);
        const bool condition_22 = (function==4u);
        const bool condition_23 = (function==3u);
        const bool condition_24 = (function==2u);
        const bool condition_25 = (function==1u);
        const bool condition_26 = (function==0u);
        const bool condition_27 = (function>3u);
        const bool condition_28 = (rhs!=0u);
        const bool condition_29 = (function!=7u);
        const bool condition_30 = (function==7u);
        const bool condition_31 = (rhs==0u);
        const bool ready_14 = (condition_12 || ready_5);
        const bool ready_15 = (condition_25 || condition_24);
        const bool ready_17 = ((!ready_15) || ready_5);
        const bool ready_20 = (ready_17 && ready_14);
        const bool ready_22 = (condition_11 || ready_5);
        const bool ready_23 = (condition_26 || ready_20);
        const bool ready_25 = (condition_27 || ((condition_11 ? ((ready_14 && (ready_17 && ready_5)) && ready_20) : ready_22) && ready_23));
        const bool ready_26 = (ready_5 && (condition_31 ? ready_5 : ready_25));
        const bool selected_value_6 = (condition_19 ? ready_5 : ready_26);
        const bool selected_value_7 = (condition_30 ? ready_5 : selected_value_6);
        const bool ready_30 = ((ready_5 && (condition_15 || ready_5)) && (condition_22 || ready_5));
        const bool selected_value_10 = (condition_29 ? ((condition_15 ? ready_5 : (condition_22 ? ready_5 : selected_value_7)) && ready_30) : selected_value_7);
        const bool ready_32 = (condition_30 || ready_30);
        const bool ready_34 = (condition_19 || ready_32);
        const bool selected_value_12 = (condition_28 ? ((condition_16 ? (selected_value_10 && ready_32) : selected_value_6) && ready_34) : ready_26);
        const bool ready_38 = (ready_5 && (condition_31 || ready_34));
        const bool condition_32 = (function<=3u);
        const bool ready_42 = ((condition_27 || ready_23) && (condition_32 || ready_38));
        const uint32_t lhs = std::get<0>(this->compute_lhs());
        const uint32_t computed_value_2 = (lhs & 2147483648u);
        const bool condition_33 = (computed_value_2!=0u);
        const bool negative_a = (computed_value_2==0u);
        const bool condition_34 = ((rhs & 2147483648u)!=0u);
        const bool condition_35 = (condition_33!=condition_34);
        const bool ready_50 = (condition_25 && condition_34);
        const bool ready_56 = (condition_26 || condition_11);
        const bool ready_57 = (condition_26 && ready_56);
        const bool ready_59 = (condition_32 && ((condition_11 && ((ready_50 || ((!ready_50) && ready_17)) && ready_14)) || ready_57));
        const bool ready_60 = (ready_59 || condition_27);
        const bool ready_63 = ((condition_31 || (condition_28 && ready_60)) && ready_5);
        const bool ready_65 = (condition_19 || (condition_16 && ready_63));
        const bool ready_67 = (condition_30 || (condition_29 && ready_65));
        const bool ready_69 = ((condition_22 && ((condition_35 || (condition_33==condition_34)) && ready_5)) || (condition_15 && ready_67));
        const bool ready_72 = (condition_29 && ((condition_15 && ((condition_33 || negative_a) && ready_5)) || (condition_22 && ready_69)));
        const bool ready_78 = (condition_28 && ((condition_16 && (ready_72 || (condition_30 && ready_67))) || (condition_19 && ready_65)));
        const bool ready_84 = ((condition_27 && ((ready_78 || (condition_31 && ready_63)) && ready_5)) || (condition_32 && ready_60));
        const bool ready_85 = (ready_84 && ready_42);
        const bool ready_86 = (((condition_27 ? ((ready_5 && selected_value_12) && ready_38) : ready_25) && ready_42) && ready_85);
        const bool condition_37 = std::get<0>(this->compute_shared_computation_16());
        const bool ready_87 = (condition_37 || ready_5);
        const bool selected_value_14 = (ready_6 ? ready_86 : ready_87);
        const bool selected_value_15 = (condition_26 ? ready_10 : selected_value_14);
        const bool selected_value_16 = (condition_25 ? ready_10 : selected_value_15);
        const bool selected_value_17 = (condition_24 ? ready_10 : selected_value_16);
        const bool selected_value_18 = (condition_23 ? ready_10 : selected_value_17);
        const bool selected_value_19 = (condition_22 ? ready_10 : selected_value_18);
        const bool selected_value_20 = (condition_19 ? (condition_20 ? ((ready_9 && ((computed_value==0u) || ready_5)) && ready_10) : ready_10) : selected_value_19);
        const bool selected_value_23 = (condition_16 ? (condition_17 ? ready_10 : (condition_18 ? ready_10 : selected_value_20)) : selected_value_20);
        const bool selected_value_26 = (condition_13 ? (condition_14 ? (condition_15 ? selected_value_23 : selected_value_19) : selected_value_18) : selected_value_17);
        const bool selected_value_29 = (ready_8 ? (condition_11 ? (condition_12 ? selected_value_26 : selected_value_16) : selected_value_15) : selected_value_14);
        const bool selected_value_32 = (selected_value ? (ready_4 ? ready_5 : (selected_value_2 ? selected_value_29 : selected_value_14)) : ready_87);
        const bool condition_path = (condition_16 ? (condition_19 || (condition_22 || (condition_27 ? (condition_28 ? (!condition_29) : true) : true))) : (condition_19 || (condition_27 ? (!condition_28) : true)));
        const bool condition_path_2 = (condition_15 ? (condition_16 || (condition_19 || (condition_27 ? (!condition_28) : true))) : condition_path);
        const bool condition_path_3 = (condition_19 ? (condition_26 || condition_27) : (condition_26 ? (condition_27 ? (!condition_28) : true) : (condition_27 && (!condition_28))));
        const bool condition_path_4 = (condition_26 ? (condition_27 ? (condition_28 ? (!condition_29) : true) : true) : (condition_27 && (condition_28 ? (!condition_29) : true)));
        const bool condition_path_5 = (condition_19 ? (condition_26 || condition_27) : (condition_22 ? (condition_26 || condition_27) : condition_path_4));
        const bool condition_path_6 = (condition_15 ? (condition_16 ? (condition_26 || condition_27) : condition_path_3) : (condition_16 ? condition_path_5 : condition_path_3));
        const bool condition_path_7 = (condition_19 ? false : (condition_22 ? false : (condition_25 ? false : (condition_26 ? false : condition_37))));
        const bool condition_path_8 = (condition_16 ? (condition_17 ? false : (condition_18 ? false : condition_path_7)) : condition_path_7);
        const bool condition_path_9 = (condition_15 ? condition_path_8 : (condition_22 ? false : (condition_25 ? false : (condition_26 ? false : condition_37))));
        const bool condition_path_10 = (condition_12 ? (condition_13 && (condition_14 && condition_path_9)) : (condition_25 ? false : (condition_26 ? false : condition_37)));
        const bool condition_path_11 = (condition_10 ? (condition_11 ? condition_path_10 : (condition_26 ? false : condition_37)) : condition_37);
        const bool condition_path_12 = (condition_8 ? ((funct7==1u) ? false : (condition_11 ? condition_path_10 : (condition_26 ? false : condition_37))) : condition_path_11);
        const bool condition_path_13 = (ready_5 ? (condition_8 ? ((funct7==1u) ? (condition_11 ? condition_path_2 : condition_path_6) : true) : true) : condition_path_12);
        const bool condition_path_14 = (condition_5 ? (ready_5 || condition_37) : ((opcode==3u) ? ready_5 : ((opcode==35u) ? ready_5 : condition_path_13)));
        bool ready_90{};
        if ((condition || ((opcode==23u) || (condition_3 || (condition_4 || condition_path_14))))) {
          ready_90 = std::get<0>(this->compute_shared_computation_22());
        }
        const bool condition_38 = (opcode!=55u);
        const bool ready_95 = (condition || condition_38);
        const bool ready_97 = (condition_5 || (condition_37 && ready_95));
        const bool ready_102 = (ready_7 && (selected_value_2 || ((!selected_value_2) && ((ready_6 && ready_85) || ((!ready_6) && ready_97)))));
        const bool ready_109 = (ready_2 || (ready_3 && ((selected_value && (ready_4 || ready_102)) || ((!selected_value) && ready_97))));
        const bool ready_114 = (((condition || (ready_2 || selected_value_32)) && ready_90) && (((condition_38 && ready_109) || (condition && ready_95)) && ready_90));
        const bool jump = std::get<0>(this->compute_jump());
        const bool ready_117 = ((ready_114 && ready_5) && (jump || ready_114));
        compute_shared_computation_32_cache.emplace(std::tuple<bool>{ready_117});
      }
      return *compute_shared_computation_32_cache;
    }
    const std::tuple<uint32_t> &compute_shared_computation_33() {
      if (!compute_shared_computation_33_cache) {
        const bool redirect = std::get<0>(this->compute_redirect());
        uint32_t result{};
        if (redirect) {
          result = std::get<0>(this->compute_result());
        }
        const uint32_t selected_value = (redirect ? result : 0u);
        compute_shared_computation_33_cache.emplace(std::tuple<uint32_t>{selected_value});
      }
      return *compute_shared_computation_33_cache;
    }
    const std::tuple<bool> &compute_shared_computation_34() {
      if (!compute_shared_computation_34_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_32());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_9());
        const bool ready_3 = std::get<0>(this->compute_shared_computation_23());
        const bool ready_4 = (!ready_3);
        const bool ready_5 = (ready_2 && ready_4);
        const bool ready_6 = (!ready_5);
        const bool jump = std::get<0>(this->compute_jump());
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition = (opcode!=99u);
        const bool condition_2 = std::get<0>(this->compute_shared_computation_17());
        const bool condition_3 = std::get<0>(this->compute_shared_computation_18());
        const bool condition_4 = std::get<0>(this->compute_shared_computation_20());
        const bool condition_5 = std::get<0>(this->compute_shared_computation_19());
        const bool condition_6 = std::get<0>(this->compute_shared_computation_21());
        const bool ready_7 = std::get<0>(this->compute_shared_computation_8());
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition_7 = (function==6u);
        const bool condition_8 = (function==5u);
        const bool condition_9 = (function==4u);
        const bool condition_10 = (function==1u);
        const bool condition_11 = (function==0u);
        const bool condition_path = (ready_7 ? (!condition_7) : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_2 = (ready_7 ? (condition_7 ? false : (!condition_8)) : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_3 = (ready_7 ? (condition_7 ? false : (condition_8 ? false : (!condition_9))) : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_4 = (condition_4 ? (condition_5 ? (condition_6 ? ready_7 : condition_path) : condition_path_2) : condition_path_3);
        const bool condition_path_5 = (condition_3 ? condition_path_4 : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_6 = (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (condition_10 ? false : (!condition_11)))));
        const bool condition_path_7 = (jump ? false : (condition ? false : (condition_2 ? condition_path_5 : (ready_7 && condition_path_6))));
        bool ready_8{};
        if ((ready && (ready_2 && (ready_3 ? false : condition_path_7)))) {
          ready_8 = std::get<0>(this->compute_shared_computation_29());
        }
        const bool ready_10 = (condition_2 || ready_7);
        const bool selected_value = (condition_10 ? ready_7 : ready_10);
        const bool selected_value_2 = (condition_9 ? ready_7 : selected_value);
        const bool selected_value_3 = (condition_8 ? ready_7 : selected_value_2);
        const bool ready_11 = (condition_7 || ready_8);
        const bool selected_value_6 = (condition_5 ? ((condition_6 ? (ready_7 && ready_8) : (condition_7 ? ready_7 : selected_value_3)) && ready_11) : selected_value_3);
        const bool ready_13 = (condition_8 || ready_11);
        const bool ready_15 = (condition_9 || ready_13);
        const bool selected_value_8 = (condition_3 ? ((condition_4 ? (selected_value_6 && ready_13) : selected_value_2) && ready_15) : selected_value);
        const bool ready_17 = (condition_10 || ready_15);
        const bool ready_19 = (condition_11 || ready_17);
        const bool ready_22 = (condition || ready_19);
        const bool ready_23 = ((condition || ((condition_2 ? (selected_value_8 && ready_17) : ready_10) && ready_19)) && ready_22);
        const bool condition_12 = std::get<0>(this->compute_shared_computation_14());
        const bool ready_24 = (condition_11 || condition_2);
        const bool ready_26 = (condition_10 || (condition_3 && ready_24));
        const bool ready_28 = (condition_9 || (condition_4 && ready_26));
        const bool ready_30 = (condition_8 || (condition_5 && ready_28));
        const bool ready_37 = ((condition_5 && (condition_6 || (condition_7 && (condition_7 || (condition_6 && ready_30))))) || (condition_8 && ready_30));
        const bool ready_43 = ((condition_3 && ((condition_4 && ready_37) || (condition_9 && ready_28))) || (condition_10 && ready_26));
        const bool ready_45 = (condition_11 && ready_24);
        const bool ready_49 = (((condition_12 && ((condition_2 && ready_43) || ready_45)) || condition) && ready_22);
        const bool ready_52 = (ready_6 || (jump || (ready_23 && ready_49)));
        const bool condition_13 = (opcode==55u);
        const bool condition_15 = std::get<0>(this->compute_shared_computation_13());
        const bool ready_55 = (((opcode==23u) || condition_15) || condition_12);
        const bool condition_16 = std::get<0>(this->compute_shared_computation_15());
        const bool ready_56 = (!ready_55);
        const bool selected_value_10 = (condition_16 ? false : ready_56);
        const bool ready_57 = ((opcode==3u) || (opcode==35u));
        const bool condition_19 = (opcode==51u);
        const Instruction executing = std::get<0>(this->compute_executing());
        const uint32_t funct7 = executing.funct7;
        const bool ready_58 = (condition_19 && (funct7==1u));
        const bool ready_59 = (!ready_57);
        const bool selected_value_11 = (ready_58 ? false : ready_59);
        const bool condition_21 = (opcode==19u);
        const bool ready_60 = (condition_19 || condition_21);
        const bool condition_22 = (function!=2u);
        const bool condition_23 = (function!=3u);
        const bool ready_61 = (condition_21 || ready_7);
        const bool ready_62 = (ready_7 && ready_61);
        const bool condition_24 = (funct7==32u);
        const uint32_t immediate = executing.immediate;
        const uint32_t rhs = std::get<0>(this->compute_rhs());
        const uint32_t operand = (condition_21 ? immediate : rhs);
        const uint32_t computed_value = (operand & 31u);
        const bool condition_26 = (function==3u);
        const bool condition_27 = (function==2u);
        const bool condition_28 = (function>3u);
        const bool condition_29 = (rhs!=0u);
        const bool condition_30 = (function!=7u);
        const bool condition_31 = (function==7u);
        const bool condition_32 = (rhs==0u);
        const bool ready_66 = (condition_3 || ready_7);
        const bool ready_67 = (condition_10 || condition_27);
        const bool ready_69 = ((!ready_67) || ready_7);
        const bool ready_72 = (ready_69 && ready_66);
        const bool ready_74 = (condition_11 || ready_72);
        const bool ready_76 = (condition_28 || ((condition_2 ? ((ready_66 && (ready_69 && ready_7)) && ready_72) : ready_10) && ready_74));
        const bool ready_77 = (ready_7 && (condition_32 ? ready_7 : ready_76));
        const bool selected_value_15 = (condition_8 ? ready_7 : ready_77);
        const bool selected_value_16 = (condition_31 ? ready_7 : selected_value_15);
        const bool ready_81 = ((ready_7 && (condition_4 || ready_7)) && (condition_9 || ready_7));
        const bool selected_value_19 = (condition_30 ? ((condition_4 ? ready_7 : (condition_9 ? ready_7 : selected_value_16)) && ready_81) : selected_value_16);
        const bool ready_83 = (condition_31 || ready_81);
        const bool ready_85 = (condition_8 || ready_83);
        const bool selected_value_21 = (condition_29 ? ((condition_5 ? (selected_value_19 && ready_83) : selected_value_15) && ready_85) : ready_77);
        const bool ready_89 = (ready_7 && (condition_32 || ready_85));
        const bool condition_33 = (function<=3u);
        const bool ready_93 = ((condition_28 || ready_74) && (condition_33 || ready_89));
        const uint32_t lhs = std::get<0>(this->compute_lhs());
        const uint32_t computed_value_2 = (lhs & 2147483648u);
        const bool condition_34 = (computed_value_2!=0u);
        const bool negative_a = (computed_value_2==0u);
        const bool condition_35 = ((rhs & 2147483648u)!=0u);
        const bool condition_36 = (condition_34!=condition_35);
        const bool ready_101 = (condition_10 && condition_35);
        const bool ready_108 = (condition_33 && ((condition_2 && ((ready_101 || ((!ready_101) && ready_69)) && ready_66)) || ready_45));
        const bool ready_109 = (ready_108 || condition_28);
        const bool ready_112 = ((condition_32 || (condition_29 && ready_109)) && ready_7);
        const bool ready_114 = (condition_8 || (condition_5 && ready_112));
        const bool ready_116 = (condition_31 || (condition_30 && ready_114));
        const bool ready_118 = ((condition_9 && ((condition_36 || (condition_34==condition_35)) && ready_7)) || (condition_4 && ready_116));
        const bool ready_121 = (condition_30 && ((condition_4 && ((condition_34 || negative_a) && ready_7)) || (condition_9 && ready_118)));
        const bool ready_127 = (condition_29 && ((condition_5 && (ready_121 || (condition_31 && ready_116))) || (condition_8 && ready_114)));
        const bool ready_133 = ((condition_28 && ((ready_127 || (condition_32 && ready_112)) && ready_7)) || (condition_33 && ready_109));
        const bool ready_134 = (ready_133 && ready_93);
        const bool ready_135 = (((condition_28 ? ((ready_7 && selected_value_21) && ready_89) : ready_76) && ready_93) && ready_134);
        const bool condition_38 = std::get<0>(this->compute_shared_computation_16());
        const bool ready_136 = (condition_38 || ready_7);
        const bool selected_value_23 = (ready_58 ? ready_135 : ready_136);
        const bool selected_value_24 = (condition_11 ? ready_62 : selected_value_23);
        const bool selected_value_25 = (condition_10 ? ready_62 : selected_value_24);
        const bool selected_value_26 = (condition_27 ? ready_62 : selected_value_25);
        const bool selected_value_27 = (condition_26 ? ready_62 : selected_value_26);
        const bool selected_value_28 = (condition_9 ? ready_62 : selected_value_27);
        const bool selected_value_29 = (condition_8 ? (condition_24 ? ((ready_61 && ((computed_value==0u) || ready_7)) && ready_62) : ready_62) : selected_value_28);
        const bool selected_value_32 = (condition_5 ? (condition_6 ? ready_62 : (condition_7 ? ready_62 : selected_value_29)) : selected_value_29);
        const bool selected_value_35 = (condition_22 ? (condition_23 ? (condition_4 ? selected_value_32 : selected_value_28) : selected_value_27) : selected_value_26);
        const bool selected_value_38 = (ready_60 ? (condition_2 ? (condition_3 ? selected_value_35 : selected_value_25) : selected_value_24) : selected_value_23);
        const bool selected_value_41 = (selected_value_10 ? (ready_57 ? ready_7 : (selected_value_11 ? selected_value_38 : selected_value_23)) : ready_136);
        const bool condition_path_8 = ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_19 ? false : (condition_21 ? false : condition_38))));
        const bool condition_path_9 = (condition_13 || ((opcode==23u) || (condition_15 || (condition_16 ? condition_38 : condition_path_8))));
        const bool condition_path_10 = (condition_19 ? ((funct7==1u) ? false : (condition_22 && (condition_23 && condition_38))) : (condition_21 ? (condition_22 && (condition_23 && condition_38)) : condition_38));
        const bool condition_path_11 = (condition_15 || (condition_16 ? condition_38 : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_10))));
        const bool condition_path_12 = (condition_11 ? (condition_12 || condition_path_9) : (condition_12 || (condition_13 || ((opcode==23u) || condition_path_11))));
        const bool condition_path_13 = (condition_9 ? (condition_12 || condition_path_9) : (condition_10 ? (condition_12 || condition_path_9) : condition_path_12));
        const bool condition_path_14 = (condition_7 ? (condition_12 || condition_path_9) : (condition_8 ? (condition_12 || condition_path_9) : condition_path_13));
        const bool condition_path_15 = ((opcode==35u) || (condition_19 ? ((funct7==1u) ? (condition_28 ? (!condition_29) : true) : true) : true));
        const bool condition_path_16 = (condition_13 || ((opcode==23u) || (condition_15 || (condition_16 || ((opcode==3u) || condition_path_15)))));
        const bool condition_path_17 = (ready_7 ? (condition_8 || (condition_12 || condition_path_16)) : (condition_8 ? (condition_12 || condition_path_9) : condition_path_13));
        const bool condition_path_18 = (condition_5 ? (condition_6 ? (ready_7 || (condition_12 || condition_path_9)) : (ready_7 || condition_path_14)) : condition_path_17);
        const bool condition_path_19 = (condition_19 ? ((funct7==1u) ? (condition_28 ? (condition_29 ? (!condition_30) : true) : true) : true) : true);
        const bool condition_path_20 = ((opcode==23u) || (condition_15 || (condition_16 || ((opcode==3u) || ((opcode==35u) || condition_path_19)))));
        const bool condition_path_21 = (ready_7 ? (condition_8 || (condition_9 || (condition_12 || (condition_13 || condition_path_20)))) : condition_path_13);
        const bool condition_path_22 = (condition_5 ? condition_path_21 : (ready_7 ? (condition_8 || (condition_12 || condition_path_16)) : condition_path_13));
        const bool condition_path_23 = ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_19 ? ((funct7==1u) ? false : condition_38) : condition_38)));
        const bool condition_path_24 = (condition_13 || ((opcode==23u) || (condition_15 || (condition_16 ? condition_38 : condition_path_23))));
        const bool condition_path_25 = (condition_10 ? (condition_12 || condition_path_9) : (condition_11 ? (condition_12 || condition_path_9) : (condition_12 || condition_path_24)));
        const bool condition_path_26 = (condition_5 ? (ready_7 || condition_path_25) : (ready_7 ? (condition_8 || (condition_12 || condition_path_16)) : condition_path_25));
        const bool condition_path_27 = (ready_7 ? (condition_8 || (condition_9 || (condition_12 || (condition_13 || condition_path_20)))) : condition_path_25);
        const bool condition_path_28 = (condition_5 ? condition_path_27 : (ready_7 ? (condition_8 || (condition_12 || condition_path_16)) : condition_path_25));
        const bool condition_path_29 = (condition_3 ? (condition_4 ? condition_path_18 : condition_path_22) : (condition_4 ? condition_path_26 : condition_path_28));
        const bool condition_path_30 = (condition_16 || ((opcode==3u) || ((opcode==35u) || (condition_19 ? ((funct7==1u) ? condition_28 : true) : true))));
        const bool condition_path_31 = (condition_11 || (condition_12 || (condition_13 || ((opcode==23u) || (condition_15 || condition_path_30)))));
        const bool condition_path_32 = (ready_7 ? condition_path_31 : (condition_11 ? (condition_12 || condition_path_9) : (condition_12 || condition_path_24)));
        const bool condition_path_33 = ((opcode==3u) || ((opcode==35u) || (condition_19 ? ((funct7==1u) ? (condition_28 && (!condition_29)) : true) : true)));
        const bool condition_path_34 = (condition_12 || (condition_13 || ((opcode==23u) || (condition_15 || (condition_16 || condition_path_33)))));
        const bool condition_path_35 = (condition_8 ? condition_path_31 : (condition_11 ? (condition_12 || condition_path_16) : condition_path_34));
        const bool condition_path_36 = (ready_7 ? condition_path_35 : (condition_11 ? (condition_12 || condition_path_9) : (condition_12 || condition_path_24)));
        const bool condition_path_37 = (condition_19 ? ((funct7==1u) ? (condition_28 && (condition_29 ? (!condition_30) : true)) : true) : true);
        const bool condition_path_38 = ((opcode==23u) || (condition_15 || (condition_16 || ((opcode==3u) || ((opcode==35u) || condition_path_37)))));
        const bool condition_path_39 = (condition_11 ? (condition_12 || (condition_13 || condition_path_20)) : (condition_12 || (condition_13 || condition_path_38)));
        const bool condition_path_40 = (ready_7 ? (condition_8 ? condition_path_31 : (condition_9 ? condition_path_31 : condition_path_39)) : (condition_11 ? (condition_12 || condition_path_9) : (condition_12 || condition_path_24)));
        const bool condition_path_41 = (condition_4 ? (condition_5 ? condition_path_32 : condition_path_36) : (condition_5 ? condition_path_40 : condition_path_36));
        const bool condition_path_42 = (condition_8 ? condition_12 : (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))));
        const bool condition_path_43 = (condition_8 ? false : (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12))));
        const bool condition_path_44 = (ready_7 ? (ready_8 ? (condition_7 ? condition_12 : condition_path_42) : (condition_7 && condition_12)) : (ready_8 && (condition_7 ? false : condition_path_43)));
        const bool condition_path_45 = (ready_7 ? (ready_8 ? condition_path_42 : (condition_7 ? condition_path_42 : (condition_8 && condition_12))) : (ready_8 ? condition_path_43 : (condition_7 && condition_path_43)));
        const bool condition_path_46 = (condition_5 ? (condition_6 ? (ready_7 && (ready_8 && condition_12)) : condition_path_44) : condition_path_45);
        const bool condition_path_47 = (condition_8 ? (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))) : (condition_9 && condition_12));
        const bool condition_path_48 = (condition_7 ? (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))) : condition_path_47);
        const bool condition_path_49 = (ready_8 ? (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))) : condition_path_48);
        const bool condition_path_50 = (condition_7 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12))) : (condition_8 && (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12)))));
        const bool condition_path_51 = (ready_8 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12))) : condition_path_50);
        const bool condition_path_52 = (condition_9 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : (condition_10 && condition_12));
        const bool condition_path_53 = (condition_7 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : (condition_8 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : condition_path_52));
        const bool condition_path_54 = (condition_8 ? (condition_10 ? false : (condition_11 && condition_12)) : (condition_9 && (condition_10 ? false : (condition_11 && condition_12))));
        const bool condition_path_55 = (ready_8 ? (condition_10 ? false : (condition_11 && condition_12)) : (condition_7 ? (condition_10 ? false : (condition_11 && condition_12)) : condition_path_54));
        const bool condition_path_56 = (ready_7 ? (ready_8 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : condition_path_53) : condition_path_55);
        const bool condition_path_57 = (condition_3 ? (condition_4 ? condition_path_46 : (ready_7 ? condition_path_49 : condition_path_51)) : condition_path_56);
        const bool condition_path_58 = (condition ? (condition_2 ? condition_path_29 : condition_path_41) : (condition_2 ? condition_path_57 : (ready_7 && (condition_11 && condition_12))));
        const bool condition_path_59 = (ready_3 ? (condition_2 ? condition_path_29 : condition_path_41) : (jump ? (condition_2 ? condition_path_29 : condition_path_41) : condition_path_58));
        bool ready_139{};
        if ((ready && (ready_2 ? condition_path_59 : (condition_2 ? condition_path_29 : condition_path_41)))) {
          ready_139 = std::get<0>(this->compute_shared_computation_22());
        }
        const bool condition_39 = (opcode!=55u);
        const bool ready_144 = (condition_13 || condition_39);
        const bool ready_146 = (condition_16 || (condition_38 && ready_144));
        const bool ready_150 = (selected_value_11 || ((!selected_value_11) && ((ready_58 && ready_134) || ((!ready_58) && ready_146))));
        const bool ready_156 = ((selected_value_10 && (ready_57 || (ready_59 && ready_150))) || ((!selected_value_10) && ready_146));
        const bool ready_162 = (((condition_39 && (ready_55 || (ready_56 && ready_156))) || (condition_13 && ready_144)) && ready_139);
        const bool ready_163 = (((condition_13 || (ready_55 || selected_value_41)) && ready_139) && ready_162);
        const bool condition_path_60 = ((opcode==35u) ? false : (condition_19 ? false : (condition_21 ? false : (condition_38 && ready_139))));
        const bool condition_path_61 = (condition_15 ? ready_139 : (condition_16 ? (condition_38 && ready_139) : ((opcode==3u) ? false : condition_path_60)));
        const bool condition_path_62 = (condition_12 ? ready_139 : (condition_13 ? ready_139 : ((opcode==23u) ? ready_139 : condition_path_61)));
        const bool condition_path_63 = (condition_21 ? (condition_22 && (condition_23 && (condition_38 && ready_139))) : (condition_38 && ready_139));
        const bool condition_path_64 = (condition_19 ? ((funct7==1u) ? false : (condition_22 && (condition_23 && (condition_38 && ready_139)))) : condition_path_63);
        const bool condition_path_65 = (condition_16 ? (condition_38 && ready_139) : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_64)));
        const bool condition_path_66 = (condition_13 ? ready_139 : ((opcode==23u) ? ready_139 : (condition_15 ? ready_139 : condition_path_65)));
        const bool condition_path_67 = (condition_10 ? condition_path_62 : (condition_11 ? condition_path_62 : (condition_12 ? ready_139 : condition_path_66)));
        const bool condition_path_68 = (condition_7 ? condition_path_62 : (condition_8 ? condition_path_62 : (condition_9 ? condition_path_62 : condition_path_67)));
        const bool condition_path_69 = (condition_6 ? (ready_7 ? ready_139 : condition_path_62) : (ready_7 ? ready_139 : condition_path_68));
        const bool condition_path_70 = (condition_19 ? ((funct7==1u) ? (condition_28 ? (condition_29 ? false : ready_139) : ready_139) : ready_139) : ready_139);
        const bool condition_path_71 = (condition_16 ? ready_139 : ((opcode==3u) ? ready_139 : ((opcode==35u) ? ready_139 : condition_path_70)));
        const bool condition_path_72 = (condition_13 ? ready_139 : ((opcode==23u) ? ready_139 : (condition_15 ? ready_139 : condition_path_71)));
        const bool condition_path_73 = (ready_7 ? (condition_8 ? ready_139 : (condition_12 ? ready_139 : condition_path_72)) : (condition_8 ? condition_path_62 : (condition_9 ? condition_path_62 : condition_path_67)));
        const bool condition_path_74 = ((funct7==1u) ? (condition_28 ? (condition_29 ? (condition_30 ? false : ready_139) : ready_139) : ready_139) : ready_139);
        const bool condition_path_75 = ((opcode==3u) ? ready_139 : ((opcode==35u) ? ready_139 : (condition_19 ? condition_path_74 : ready_139)));
        const bool condition_path_76 = ((opcode==23u) ? ready_139 : (condition_15 ? ready_139 : (condition_16 ? ready_139 : condition_path_75)));
        const bool condition_path_77 = (condition_9 ? ready_139 : (condition_12 ? ready_139 : (condition_13 ? ready_139 : condition_path_76)));
        const bool condition_path_78 = (ready_7 ? (condition_8 ? ready_139 : condition_path_77) : (condition_9 ? condition_path_62 : condition_path_67));
        const bool condition_path_79 = (ready_7 ? (condition_8 ? ready_139 : (condition_12 ? ready_139 : condition_path_72)) : (condition_9 ? condition_path_62 : condition_path_67));
        const bool condition_path_80 = (condition_4 ? (condition_5 ? condition_path_69 : condition_path_73) : (condition_5 ? condition_path_78 : condition_path_79));
        const bool condition_path_81 = ((opcode==35u) ? false : (condition_19 ? ((funct7==1u) ? false : (condition_38 && ready_139)) : (condition_38 && ready_139)));
        const bool condition_path_82 = (condition_15 ? ready_139 : (condition_16 ? (condition_38 && ready_139) : ((opcode==3u) ? false : condition_path_81)));
        const bool condition_path_83 = (condition_12 ? ready_139 : (condition_13 ? ready_139 : ((opcode==23u) ? ready_139 : condition_path_82)));
        const bool condition_path_84 = (ready_7 ? ready_139 : (condition_10 ? condition_path_62 : (condition_11 ? condition_path_62 : condition_path_83)));
        const bool condition_path_85 = (ready_7 ? (condition_8 ? ready_139 : (condition_12 ? ready_139 : condition_path_72)) : (condition_10 ? condition_path_62 : (condition_11 ? condition_path_62 : condition_path_83)));
        const bool condition_path_86 = (ready_7 ? (condition_8 ? ready_139 : condition_path_77) : (condition_10 ? condition_path_62 : (condition_11 ? condition_path_62 : condition_path_83)));
        const bool condition_path_87 = (condition_4 ? (condition_5 ? condition_path_84 : condition_path_85) : (condition_5 ? condition_path_86 : condition_path_85));
        const bool condition_path_88 = ((opcode==35u) ? ready_139 : (condition_19 ? ((funct7==1u) ? (condition_28 && ready_139) : ready_139) : ready_139));
        const bool condition_path_89 = (condition_15 ? ready_139 : (condition_16 ? ready_139 : ((opcode==3u) ? ready_139 : condition_path_88)));
        const bool condition_path_90 = (condition_12 ? ready_139 : (condition_13 ? ready_139 : ((opcode==23u) ? ready_139 : condition_path_89)));
        const bool condition_path_91 = (ready_7 ? (condition_11 ? ready_139 : condition_path_90) : (condition_11 ? condition_path_62 : condition_path_83));
        const bool condition_path_92 = (condition_19 ? ((funct7==1u) ? (condition_28 && (condition_29 ? false : ready_139)) : ready_139) : ready_139);
        const bool condition_path_93 = (condition_16 ? ready_139 : ((opcode==3u) ? ready_139 : ((opcode==35u) ? ready_139 : condition_path_92)));
        const bool condition_path_94 = (condition_13 ? ready_139 : ((opcode==23u) ? ready_139 : (condition_15 ? ready_139 : condition_path_93)));
        const bool condition_path_95 = (condition_11 ? (condition_12 ? ready_139 : condition_path_72) : (condition_12 ? ready_139 : condition_path_94));
        const bool condition_path_96 = (ready_7 ? (condition_8 ? (condition_11 ? ready_139 : condition_path_90) : condition_path_95) : (condition_11 ? condition_path_62 : condition_path_83));
        const bool condition_path_97 = ((funct7==1u) ? (condition_28 && (condition_29 ? (condition_30 ? false : ready_139) : ready_139)) : ready_139);
        const bool condition_path_98 = ((opcode==3u) ? ready_139 : ((opcode==35u) ? ready_139 : (condition_19 ? condition_path_97 : ready_139)));
        const bool condition_path_99 = ((opcode==23u) ? ready_139 : (condition_15 ? ready_139 : (condition_16 ? ready_139 : condition_path_98)));
        const bool condition_path_100 = (condition_11 ? (condition_12 ? ready_139 : (condition_13 ? ready_139 : condition_path_76)) : (condition_12 ? ready_139 : (condition_13 ? ready_139 : condition_path_99)));
        const bool condition_path_101 = (condition_8 ? (condition_11 ? ready_139 : condition_path_90) : (condition_9 ? (condition_11 ? ready_139 : condition_path_90) : condition_path_100));
        const bool condition_path_102 = (condition_5 ? (ready_7 ? condition_path_101 : (condition_11 ? condition_path_62 : condition_path_83)) : condition_path_96);
        const bool condition_path_103 = (condition_2 ? (condition_3 ? condition_path_80 : condition_path_87) : (condition_4 ? (condition_5 ? condition_path_91 : condition_path_96) : condition_path_102));
        const bool condition_path_104 = (condition_9 ? (condition_12 && ready_139) : (condition_10 ? (condition_12 && ready_139) : (condition_11 && (condition_12 && ready_139))));
        const bool condition_path_105 = (condition_7 ? (condition_12 && ready_139) : (condition_8 ? (condition_12 && ready_139) : condition_path_104));
        const bool condition_path_106 = (condition_8 ? false : (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_139)))));
        const bool condition_path_107 = (ready_7 ? (ready_8 ? condition_path_105 : (condition_7 && (condition_12 && ready_139))) : (ready_8 && (condition_7 ? false : condition_path_106)));
        const bool condition_path_108 = (condition_7 ? (condition_8 ? (condition_12 && ready_139) : condition_path_104) : (condition_8 && (condition_12 && ready_139)));
        const bool condition_path_109 = (ready_7 ? (ready_8 ? (condition_8 ? (condition_12 && ready_139) : condition_path_104) : condition_path_108) : (ready_8 ? condition_path_106 : (condition_7 && condition_path_106)));
        const bool condition_path_110 = (condition_5 ? (condition_6 ? (ready_7 && (ready_8 && (condition_12 && ready_139))) : condition_path_107) : condition_path_109);
        const bool condition_path_111 = (condition_7 ? condition_path_104 : (condition_8 ? condition_path_104 : (condition_9 && (condition_12 && ready_139))));
        const bool condition_path_112 = (condition_8 && (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_139)))));
        const bool condition_path_113 = (condition_7 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_139)))) : condition_path_112);
        const bool condition_path_114 = (ready_8 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_139)))) : condition_path_113);
        const bool condition_path_115 = (condition_4 ? condition_path_110 : (ready_7 ? (ready_8 ? condition_path_104 : condition_path_111) : condition_path_114));
        const bool condition_path_116 = (condition_9 ? (condition_10 ? (condition_12 && ready_139) : (condition_11 && (condition_12 && ready_139))) : (condition_10 && (condition_12 && ready_139)));
        const bool condition_path_117 = (condition_8 ? (condition_10 ? (condition_12 && ready_139) : (condition_11 && (condition_12 && ready_139))) : condition_path_116);
        const bool condition_path_118 = (condition_7 ? (condition_10 ? (condition_12 && ready_139) : (condition_11 && (condition_12 && ready_139))) : condition_path_117);
        const bool condition_path_119 = (ready_8 ? (condition_10 ? (condition_12 && ready_139) : (condition_11 && (condition_12 && ready_139))) : condition_path_118);
        const bool condition_path_120 = (condition_8 ? (condition_10 ? false : (condition_11 && (condition_12 && ready_139))) : (condition_9 && (condition_10 ? false : (condition_11 && (condition_12 && ready_139)))));
        const bool condition_path_121 = (condition_7 ? (condition_10 ? false : (condition_11 && (condition_12 && ready_139))) : condition_path_120);
        const bool condition_path_122 = (ready_8 ? (condition_10 ? false : (condition_11 && (condition_12 && ready_139))) : condition_path_121);
        const bool condition_path_123 = (condition_2 ? (condition_3 ? condition_path_115 : (ready_7 ? condition_path_119 : condition_path_122)) : (ready_7 && (condition_11 && (condition_12 && ready_139))));
        const bool condition_path_124 = (ready_3 ? condition_path_103 : (jump ? condition_path_103 : (condition ? condition_path_103 : condition_path_123)));
        bool ready_165{};
        if ((ready && (ready_2 ? condition_path_124 : condition_path_103))) {
          ready_165 = std::get<0>(this->compute_shared_computation_31());
        }
        const bool ready_166 = ((ready_52 && ready_163) && ready_165);
        const bool condition_path_125 = ((opcode==35u) ? false : (condition_19 ? false : (condition_21 ? false : (condition_38 && (ready_139 && ready_165)))));
        const bool condition_path_126 = (condition_16 ? (condition_38 && (ready_139 && ready_165)) : ((opcode==3u) ? false : condition_path_125));
        const bool condition_path_127 = ((opcode==23u) ? (ready_139 && ready_165) : (condition_15 ? (ready_139 && ready_165) : condition_path_126));
        const bool condition_path_128 = (condition_12 ? (ready_139 && ready_165) : (condition_13 ? (ready_139 && ready_165) : condition_path_127));
        const bool condition_path_129 = ((funct7==1u) ? false : (condition_22 && (condition_23 && (condition_38 && (ready_139 && ready_165)))));
        const bool condition_path_130 = (condition_21 ? (condition_22 && (condition_23 && (condition_38 && (ready_139 && ready_165)))) : (condition_38 && (ready_139 && ready_165)));
        const bool condition_path_131 = ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_19 ? condition_path_129 : condition_path_130)));
        const bool condition_path_132 = (condition_15 ? (ready_139 && ready_165) : (condition_16 ? (condition_38 && (ready_139 && ready_165)) : condition_path_131));
        const bool condition_path_133 = (condition_13 ? (ready_139 && ready_165) : ((opcode==23u) ? (ready_139 && ready_165) : condition_path_132));
        const bool condition_path_134 = (condition_10 ? condition_path_128 : (condition_11 ? condition_path_128 : (condition_12 ? (ready_139 && ready_165) : condition_path_133)));
        const bool condition_path_135 = (condition_7 ? condition_path_128 : (condition_8 ? condition_path_128 : (condition_9 ? condition_path_128 : condition_path_134)));
        const bool condition_path_136 = (condition_6 ? (ready_7 ? (ready_139 && ready_165) : condition_path_128) : (ready_7 ? (ready_139 && ready_165) : condition_path_135));
        const bool condition_path_137 = ((funct7==1u) ? (condition_28 ? (condition_29 ? false : (ready_139 && ready_165)) : (ready_139 && ready_165)) : (ready_139 && ready_165));
        const bool condition_path_138 = ((opcode==35u) ? (ready_139 && ready_165) : (condition_19 ? condition_path_137 : (ready_139 && ready_165)));
        const bool condition_path_139 = (condition_16 ? (ready_139 && ready_165) : ((opcode==3u) ? (ready_139 && ready_165) : condition_path_138));
        const bool condition_path_140 = ((opcode==23u) ? (ready_139 && ready_165) : (condition_15 ? (ready_139 && ready_165) : condition_path_139));
        const bool condition_path_141 = (condition_12 ? (ready_139 && ready_165) : (condition_13 ? (ready_139 && ready_165) : condition_path_140));
        const bool condition_path_142 = (ready_7 ? (condition_8 ? (ready_139 && ready_165) : condition_path_141) : (condition_8 ? condition_path_128 : (condition_9 ? condition_path_128 : condition_path_134)));
        const bool condition_path_143 = (condition_28 ? (condition_29 ? (condition_30 ? false : (ready_139 && ready_165)) : (ready_139 && ready_165)) : (ready_139 && ready_165));
        const bool condition_path_144 = (condition_19 ? ((funct7==1u) ? condition_path_143 : (ready_139 && ready_165)) : (ready_139 && ready_165));
        const bool condition_path_145 = ((opcode==3u) ? (ready_139 && ready_165) : ((opcode==35u) ? (ready_139 && ready_165) : condition_path_144));
        const bool condition_path_146 = (condition_15 ? (ready_139 && ready_165) : (condition_16 ? (ready_139 && ready_165) : condition_path_145));
        const bool condition_path_147 = (condition_13 ? (ready_139 && ready_165) : ((opcode==23u) ? (ready_139 && ready_165) : condition_path_146));
        const bool condition_path_148 = (condition_9 ? (ready_139 && ready_165) : (condition_12 ? (ready_139 && ready_165) : condition_path_147));
        const bool condition_path_149 = (ready_7 ? (condition_8 ? (ready_139 && ready_165) : condition_path_148) : (condition_9 ? condition_path_128 : condition_path_134));
        const bool condition_path_150 = (ready_7 ? (condition_8 ? (ready_139 && ready_165) : condition_path_141) : (condition_9 ? condition_path_128 : condition_path_134));
        const bool condition_path_151 = (condition_4 ? (condition_5 ? condition_path_136 : condition_path_142) : (condition_5 ? condition_path_149 : condition_path_150));
        const bool condition_path_152 = (condition_19 ? ((funct7==1u) ? false : (condition_38 && (ready_139 && ready_165))) : (condition_38 && (ready_139 && ready_165)));
        const bool condition_path_153 = (condition_16 ? (condition_38 && (ready_139 && ready_165)) : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_152)));
        const bool condition_path_154 = ((opcode==23u) ? (ready_139 && ready_165) : (condition_15 ? (ready_139 && ready_165) : condition_path_153));
        const bool condition_path_155 = (condition_12 ? (ready_139 && ready_165) : (condition_13 ? (ready_139 && ready_165) : condition_path_154));
        const bool condition_path_156 = (ready_7 ? (ready_139 && ready_165) : (condition_10 ? condition_path_128 : (condition_11 ? condition_path_128 : condition_path_155)));
        const bool condition_path_157 = (ready_7 ? (condition_8 ? (ready_139 && ready_165) : condition_path_141) : (condition_10 ? condition_path_128 : (condition_11 ? condition_path_128 : condition_path_155)));
        const bool condition_path_158 = (ready_7 ? (condition_8 ? (ready_139 && ready_165) : condition_path_148) : (condition_10 ? condition_path_128 : (condition_11 ? condition_path_128 : condition_path_155)));
        const bool condition_path_159 = (condition_4 ? (condition_5 ? condition_path_156 : condition_path_157) : (condition_5 ? condition_path_158 : condition_path_157));
        const bool condition_path_160 = (condition_19 ? ((funct7==1u) ? (condition_28 && (ready_139 && ready_165)) : (ready_139 && ready_165)) : (ready_139 && ready_165));
        const bool condition_path_161 = ((opcode==3u) ? (ready_139 && ready_165) : ((opcode==35u) ? (ready_139 && ready_165) : condition_path_160));
        const bool condition_path_162 = (condition_15 ? (ready_139 && ready_165) : (condition_16 ? (ready_139 && ready_165) : condition_path_161));
        const bool condition_path_163 = (condition_13 ? (ready_139 && ready_165) : ((opcode==23u) ? (ready_139 && ready_165) : condition_path_162));
        const bool condition_path_164 = (condition_11 ? (ready_139 && ready_165) : (condition_12 ? (ready_139 && ready_165) : condition_path_163));
        const bool condition_path_165 = ((funct7==1u) ? (condition_28 && (condition_29 ? false : (ready_139 && ready_165))) : (ready_139 && ready_165));
        const bool condition_path_166 = ((opcode==35u) ? (ready_139 && ready_165) : (condition_19 ? condition_path_165 : (ready_139 && ready_165)));
        const bool condition_path_167 = (condition_16 ? (ready_139 && ready_165) : ((opcode==3u) ? (ready_139 && ready_165) : condition_path_166));
        const bool condition_path_168 = ((opcode==23u) ? (ready_139 && ready_165) : (condition_15 ? (ready_139 && ready_165) : condition_path_167));
        const bool condition_path_169 = (condition_12 ? (ready_139 && ready_165) : (condition_13 ? (ready_139 && ready_165) : condition_path_168));
        const bool condition_path_170 = (ready_7 ? (condition_8 ? condition_path_164 : (condition_11 ? condition_path_141 : condition_path_169)) : (condition_11 ? condition_path_128 : condition_path_155));
        const bool condition_path_171 = (condition_5 ? (ready_7 ? condition_path_164 : (condition_11 ? condition_path_128 : condition_path_155)) : condition_path_170);
        const bool condition_path_172 = (condition_28 && (condition_29 ? (condition_30 ? false : (ready_139 && ready_165)) : (ready_139 && ready_165)));
        const bool condition_path_173 = (condition_19 ? ((funct7==1u) ? condition_path_172 : (ready_139 && ready_165)) : (ready_139 && ready_165));
        const bool condition_path_174 = ((opcode==3u) ? (ready_139 && ready_165) : ((opcode==35u) ? (ready_139 && ready_165) : condition_path_173));
        const bool condition_path_175 = (condition_15 ? (ready_139 && ready_165) : (condition_16 ? (ready_139 && ready_165) : condition_path_174));
        const bool condition_path_176 = (condition_13 ? (ready_139 && ready_165) : ((opcode==23u) ? (ready_139 && ready_165) : condition_path_175));
        const bool condition_path_177 = (condition_11 ? (condition_12 ? (ready_139 && ready_165) : condition_path_147) : (condition_12 ? (ready_139 && ready_165) : condition_path_176));
        const bool condition_path_178 = (ready_7 ? (condition_8 ? condition_path_164 : (condition_9 ? condition_path_164 : condition_path_177)) : (condition_11 ? condition_path_128 : condition_path_155));
        const bool condition_path_179 = (condition_2 ? (condition_3 ? condition_path_151 : condition_path_159) : (condition_4 ? condition_path_171 : (condition_5 ? condition_path_178 : condition_path_170)));
        const bool condition_path_180 = (condition_10 ? (condition_12 && (ready_139 && ready_165)) : (condition_11 && (condition_12 && (ready_139 && ready_165))));
        const bool condition_path_181 = (condition_8 ? (condition_12 && (ready_139 && ready_165)) : (condition_9 ? (condition_12 && (ready_139 && ready_165)) : condition_path_180));
        const bool condition_path_182 = (ready_8 ? (condition_7 ? (condition_12 && (ready_139 && ready_165)) : condition_path_181) : (condition_7 && (condition_12 && (ready_139 && ready_165))));
        const bool condition_path_183 = (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && (ready_139 && ready_165)))));
        const bool condition_path_184 = (ready_7 ? condition_path_182 : (ready_8 && (condition_7 ? false : (condition_8 ? false : condition_path_183))));
        const bool condition_path_185 = (condition_6 ? (ready_7 && (ready_8 && (condition_12 && (ready_139 && ready_165)))) : condition_path_184);
        const bool condition_path_186 = (ready_8 ? condition_path_181 : (condition_7 ? condition_path_181 : (condition_8 && (condition_12 && (ready_139 && ready_165)))));
        const bool condition_path_187 = (ready_8 ? (condition_8 ? false : condition_path_183) : (condition_7 && (condition_8 ? false : condition_path_183)));
        const bool condition_path_188 = (condition_8 ? (condition_9 ? (condition_12 && (ready_139 && ready_165)) : condition_path_180) : (condition_9 && (condition_12 && (ready_139 && ready_165))));
        const bool condition_path_189 = (condition_7 ? (condition_9 ? (condition_12 && (ready_139 && ready_165)) : condition_path_180) : condition_path_188);
        const bool condition_path_190 = (ready_8 ? (condition_9 ? (condition_12 && (ready_139 && ready_165)) : condition_path_180) : condition_path_189);
        const bool condition_path_191 = (ready_8 ? condition_path_183 : (condition_7 ? condition_path_183 : (condition_8 && condition_path_183)));
        const bool condition_path_192 = (condition_4 ? (condition_5 ? condition_path_185 : (ready_7 ? condition_path_186 : condition_path_187)) : (ready_7 ? condition_path_190 : condition_path_191));
        const bool condition_path_193 = (condition_8 ? condition_path_180 : (condition_9 ? condition_path_180 : (condition_10 && (condition_12 && (ready_139 && ready_165)))));
        const bool condition_path_194 = (condition_9 && (condition_10 ? false : (condition_11 && (condition_12 && (ready_139 && ready_165)))));
        const bool condition_path_195 = (condition_8 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_139 && ready_165)))) : condition_path_194);
        const bool condition_path_196 = (condition_7 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_139 && ready_165)))) : condition_path_195);
        const bool condition_path_197 = (ready_8 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_139 && ready_165)))) : condition_path_196);
        const bool condition_path_198 = (ready_7 ? (ready_8 ? condition_path_180 : (condition_7 ? condition_path_180 : condition_path_193)) : condition_path_197);
        const bool condition_path_199 = (condition_2 ? (condition_3 ? condition_path_192 : condition_path_198) : (ready_7 && (condition_11 && (condition_12 && (ready_139 && ready_165)))));
        const bool condition_path_200 = (ready_3 ? condition_path_179 : (jump ? condition_path_179 : (condition ? condition_path_179 : condition_path_199)));
        bool ready_168{};
        if ((ready && (ready_2 ? condition_path_200 : condition_path_179))) {
          ready_168 = std::get<0>(this->compute_shared_computation_12());
        }
        const bool ready_169 = (((ready && ready_52) && ready_166) && ready_168);
        compute_shared_computation_34_cache.emplace(std::tuple<bool>{ready_169});
      }
      return *compute_shared_computation_34_cache;
    }
    const std::tuple<bool> &compute_shared_computation_35() {
      if (!compute_shared_computation_35_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_31());
        bool ready_2{};
        if (ready) {
          ready_2 = std::get<0>(this->compute_shared_computation_12());
        }
        const bool ready_3 = (ready && ready_2);
        compute_shared_computation_35_cache.emplace(std::tuple<bool>{ready_3});
      }
      return *compute_shared_computation_35_cache;
    }
    const std::tuple<bool> &compute_shared_computation_36() {
      if (!compute_shared_computation_36_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_9());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_23());
        const bool ready_3 = (!ready_2);
        const bool ready_4 = (ready && ready_3);
        const bool ready_5 = (!ready_4);
        const bool jump = std::get<0>(this->compute_jump());
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition = (opcode!=99u);
        const bool condition_2 = std::get<0>(this->compute_shared_computation_17());
        const bool condition_3 = std::get<0>(this->compute_shared_computation_18());
        const bool condition_4 = std::get<0>(this->compute_shared_computation_20());
        const bool condition_5 = std::get<0>(this->compute_shared_computation_19());
        const bool condition_6 = std::get<0>(this->compute_shared_computation_21());
        const bool ready_6 = std::get<0>(this->compute_shared_computation_8());
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition_7 = (function==6u);
        const bool condition_8 = (function==5u);
        const bool condition_9 = (function==4u);
        const bool condition_10 = (function==1u);
        const bool condition_11 = (function==0u);
        const bool condition_path = (ready_6 ? (!condition_7) : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_2 = (ready_6 ? (condition_7 ? false : (!condition_8)) : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_3 = (ready_6 ? (condition_7 ? false : (condition_8 ? false : (!condition_9))) : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_4 = (condition_4 ? (condition_5 ? (condition_6 ? ready_6 : condition_path) : condition_path_2) : condition_path_3);
        const bool condition_path_5 = (condition_3 ? condition_path_4 : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_6 = (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (condition_10 ? false : (!condition_11)))));
        const bool condition_path_7 = (jump ? false : (condition ? false : (condition_2 ? condition_path_5 : (ready_6 && condition_path_6))));
        bool ready_7{};
        if ((ready && (ready_2 ? false : condition_path_7))) {
          ready_7 = std::get<0>(this->compute_shared_computation_29());
        }
        const bool ready_9 = (condition_2 || ready_6);
        const bool selected_value = (condition_10 ? ready_6 : ready_9);
        const bool selected_value_2 = (condition_9 ? ready_6 : selected_value);
        const bool selected_value_3 = (condition_8 ? ready_6 : selected_value_2);
        const bool ready_10 = (condition_7 || ready_7);
        const bool selected_value_6 = (condition_5 ? ((condition_6 ? (ready_6 && ready_7) : (condition_7 ? ready_6 : selected_value_3)) && ready_10) : selected_value_3);
        const bool ready_12 = (condition_8 || ready_10);
        const bool ready_14 = (condition_9 || ready_12);
        const bool selected_value_8 = (condition_3 ? ((condition_4 ? (selected_value_6 && ready_12) : selected_value_2) && ready_14) : selected_value);
        const bool ready_16 = (condition_10 || ready_14);
        const bool ready_18 = (condition_11 || ready_16);
        const bool ready_21 = (condition || ready_18);
        const bool condition_12 = std::get<0>(this->compute_shared_computation_14());
        const bool ready_23 = (condition_11 || condition_2);
        const bool ready_25 = (condition_10 || (condition_3 && ready_23));
        const bool ready_27 = (condition_9 || (condition_4 && ready_25));
        const bool ready_29 = (condition_8 || (condition_5 && ready_27));
        const bool ready_36 = ((condition_5 && (condition_6 || (condition_7 && (condition_7 || (condition_6 && ready_29))))) || (condition_8 && ready_29));
        const bool ready_42 = ((condition_3 && ((condition_4 && ready_36) || (condition_9 && ready_27))) || (condition_10 && ready_25));
        const bool ready_44 = (condition_11 && ready_23);
        const bool ready_48 = (((condition_12 && ((condition_2 && ready_42) || ready_44)) || condition) && ready_21);
        const bool ready_49 = (((condition || ((condition_2 ? (selected_value_8 && ready_16) : ready_9) && ready_18)) && ready_21) && ready_48);
        const bool ready_51 = (ready_5 || (jump || ready_49));
        const bool condition_path_8 = (condition_8 ? condition_12 : (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))));
        const bool condition_path_9 = (condition_8 ? false : (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12))));
        const bool condition_path_10 = (ready_6 ? (ready_7 ? (condition_7 ? condition_12 : condition_path_8) : (condition_7 && condition_12)) : (ready_7 && (condition_7 ? false : condition_path_9)));
        const bool condition_path_11 = (ready_6 ? (ready_7 ? condition_path_8 : (condition_7 ? condition_path_8 : (condition_8 && condition_12))) : (ready_7 ? condition_path_9 : (condition_7 && condition_path_9)));
        const bool condition_path_12 = (condition_5 ? (condition_6 ? (ready_6 && (ready_7 && condition_12)) : condition_path_10) : condition_path_11);
        const bool condition_path_13 = (condition_8 ? (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))) : (condition_9 && condition_12));
        const bool condition_path_14 = (condition_7 ? (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))) : condition_path_13);
        const bool condition_path_15 = (ready_7 ? (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))) : condition_path_14);
        const bool condition_path_16 = (condition_7 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12))) : (condition_8 && (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12)))));
        const bool condition_path_17 = (ready_7 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12))) : condition_path_16);
        const bool condition_path_18 = (condition_9 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : (condition_10 && condition_12));
        const bool condition_path_19 = (condition_7 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : (condition_8 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : condition_path_18));
        const bool condition_path_20 = (condition_8 ? (condition_10 ? false : (condition_11 && condition_12)) : (condition_9 && (condition_10 ? false : (condition_11 && condition_12))));
        const bool condition_path_21 = (ready_7 ? (condition_10 ? false : (condition_11 && condition_12)) : (condition_7 ? (condition_10 ? false : (condition_11 && condition_12)) : condition_path_20));
        const bool condition_path_22 = (ready_6 ? (ready_7 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : condition_path_19) : condition_path_21);
        const bool condition_path_23 = (condition_3 ? (condition_4 ? condition_path_12 : (ready_6 ? condition_path_15 : condition_path_17)) : condition_path_22);
        const bool condition_path_24 = (jump || (condition || (condition_2 ? condition_path_23 : (ready_6 && (condition_11 && condition_12)))));
        bool ready_52{};
        if ((ready ? (ready_2 || condition_path_24) : true)) {
          ready_52 = std::get<0>(this->compute_shared_computation_31());
        }
        const bool condition_path_25 = (condition_9 ? (condition_12 && ready_52) : (condition_10 ? (condition_12 && ready_52) : (condition_11 && (condition_12 && ready_52))));
        const bool condition_path_26 = (condition_7 ? (condition_12 && ready_52) : (condition_8 ? (condition_12 && ready_52) : condition_path_25));
        const bool condition_path_27 = (condition_8 ? false : (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_52)))));
        const bool condition_path_28 = (ready_6 ? (ready_7 ? condition_path_26 : (condition_7 && (condition_12 && ready_52))) : (ready_7 && (condition_7 ? false : condition_path_27)));
        const bool condition_path_29 = (condition_7 ? (condition_8 ? (condition_12 && ready_52) : condition_path_25) : (condition_8 && (condition_12 && ready_52)));
        const bool condition_path_30 = (ready_6 ? (ready_7 ? (condition_8 ? (condition_12 && ready_52) : condition_path_25) : condition_path_29) : (ready_7 ? condition_path_27 : (condition_7 && condition_path_27)));
        const bool condition_path_31 = (condition_5 ? (condition_6 ? (ready_6 && (ready_7 && (condition_12 && ready_52))) : condition_path_28) : condition_path_30);
        const bool condition_path_32 = (condition_7 ? condition_path_25 : (condition_8 ? condition_path_25 : (condition_9 && (condition_12 && ready_52))));
        const bool condition_path_33 = (condition_8 && (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_52)))));
        const bool condition_path_34 = (condition_7 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_52)))) : condition_path_33);
        const bool condition_path_35 = (ready_7 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_52)))) : condition_path_34);
        const bool condition_path_36 = (condition_4 ? condition_path_31 : (ready_6 ? (ready_7 ? condition_path_25 : condition_path_32) : condition_path_35));
        const bool condition_path_37 = (condition_9 ? (condition_10 ? (condition_12 && ready_52) : (condition_11 && (condition_12 && ready_52))) : (condition_10 && (condition_12 && ready_52)));
        const bool condition_path_38 = (condition_8 ? (condition_10 ? (condition_12 && ready_52) : (condition_11 && (condition_12 && ready_52))) : condition_path_37);
        const bool condition_path_39 = (condition_7 ? (condition_10 ? (condition_12 && ready_52) : (condition_11 && (condition_12 && ready_52))) : condition_path_38);
        const bool condition_path_40 = (ready_7 ? (condition_10 ? (condition_12 && ready_52) : (condition_11 && (condition_12 && ready_52))) : condition_path_39);
        const bool condition_path_41 = (condition_8 ? (condition_10 ? false : (condition_11 && (condition_12 && ready_52))) : (condition_9 && (condition_10 ? false : (condition_11 && (condition_12 && ready_52)))));
        const bool condition_path_42 = (condition_7 ? (condition_10 ? false : (condition_11 && (condition_12 && ready_52))) : condition_path_41);
        const bool condition_path_43 = (ready_7 ? (condition_10 ? false : (condition_11 && (condition_12 && ready_52))) : condition_path_42);
        const bool condition_path_44 = (condition_2 ? (condition_3 ? condition_path_36 : (ready_6 ? condition_path_40 : condition_path_43)) : (ready_6 && (condition_11 && (condition_12 && ready_52))));
        const bool condition_path_45 = (ready ? (ready_2 ? ready_52 : (jump ? ready_52 : (condition ? ready_52 : condition_path_44))) : ready_52);
        bool ready_54{};
        if (condition_path_45) {
          ready_54 = std::get<0>(this->compute_shared_computation_12());
        }
        const bool condition_path_46 = (condition_10 ? (condition_12 && (ready_52 && ready_54)) : (condition_11 && (condition_12 && (ready_52 && ready_54))));
        const bool condition_path_47 = (condition_8 ? (condition_12 && (ready_52 && ready_54)) : (condition_9 ? (condition_12 && (ready_52 && ready_54)) : condition_path_46));
        const bool condition_path_48 = (ready_7 ? (condition_7 ? (condition_12 && (ready_52 && ready_54)) : condition_path_47) : (condition_7 && (condition_12 && (ready_52 && ready_54))));
        const bool condition_path_49 = (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && (ready_52 && ready_54)))));
        const bool condition_path_50 = (ready_6 ? condition_path_48 : (ready_7 && (condition_7 ? false : (condition_8 ? false : condition_path_49))));
        const bool condition_path_51 = (condition_6 ? (ready_6 && (ready_7 && (condition_12 && (ready_52 && ready_54)))) : condition_path_50);
        const bool condition_path_52 = (ready_7 ? condition_path_47 : (condition_7 ? condition_path_47 : (condition_8 && (condition_12 && (ready_52 && ready_54)))));
        const bool condition_path_53 = (ready_7 ? (condition_8 ? false : condition_path_49) : (condition_7 && (condition_8 ? false : condition_path_49)));
        const bool condition_path_54 = (condition_8 ? (condition_9 ? (condition_12 && (ready_52 && ready_54)) : condition_path_46) : (condition_9 && (condition_12 && (ready_52 && ready_54))));
        const bool condition_path_55 = (condition_7 ? (condition_9 ? (condition_12 && (ready_52 && ready_54)) : condition_path_46) : condition_path_54);
        const bool condition_path_56 = (ready_7 ? (condition_9 ? (condition_12 && (ready_52 && ready_54)) : condition_path_46) : condition_path_55);
        const bool condition_path_57 = (ready_7 ? condition_path_49 : (condition_7 ? condition_path_49 : (condition_8 && condition_path_49)));
        const bool condition_path_58 = (condition_4 ? (condition_5 ? condition_path_51 : (ready_6 ? condition_path_52 : condition_path_53)) : (ready_6 ? condition_path_56 : condition_path_57));
        const bool condition_path_59 = (condition_8 ? condition_path_46 : (condition_9 ? condition_path_46 : (condition_10 && (condition_12 && (ready_52 && ready_54)))));
        const bool condition_path_60 = (condition_8 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_52 && ready_54)))) : (condition_9 && (condition_10 ? false : (condition_11 && (condition_12 && (ready_52 && ready_54))))));
        const bool condition_path_61 = (condition_7 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_52 && ready_54)))) : condition_path_60);
        const bool condition_path_62 = (ready_7 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_52 && ready_54)))) : condition_path_61);
        const bool condition_path_63 = (ready_6 ? (ready_7 ? condition_path_46 : (condition_7 ? condition_path_46 : condition_path_59)) : condition_path_62);
        const bool condition_path_64 = (condition_2 ? (condition_3 ? condition_path_58 : condition_path_63) : (ready_6 && (condition_11 && (condition_12 && (ready_52 && ready_54)))));
        const bool condition_path_65 = (ready_2 ? (ready_52 && ready_54) : (jump ? (ready_52 && ready_54) : (condition ? (ready_52 && ready_54) : condition_path_64)));
        bool ready_56{};
        if ((ready ? condition_path_65 : (ready_52 && ready_54))) {
          ready_56 = std::get<0>(this->compute_shared_computation_34());
        }
        const bool condition_path_66 = (condition_10 ? (condition_12 && (ready_52 && (ready_54 && ready_56))) : (condition_11 && (condition_12 && (ready_52 && (ready_54 && ready_56)))));
        const bool condition_path_67 = (condition_8 ? (condition_12 && (ready_52 && (ready_54 && ready_56))) : (condition_9 ? (condition_12 && (ready_52 && (ready_54 && ready_56))) : condition_path_66));
        const bool condition_path_68 = (ready_7 ? (condition_7 ? (condition_12 && (ready_52 && (ready_54 && ready_56))) : condition_path_67) : (condition_7 && (condition_12 && (ready_52 && (ready_54 && ready_56)))));
        const bool condition_path_69 = (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && (ready_52 && (ready_54 && ready_56))))));
        const bool condition_path_70 = (ready_6 ? condition_path_68 : (ready_7 && (condition_7 ? false : (condition_8 ? false : condition_path_69))));
        const bool condition_path_71 = (condition_6 ? (ready_6 && (ready_7 && (condition_12 && (ready_52 && (ready_54 && ready_56))))) : condition_path_70);
        const bool condition_path_72 = (condition_7 ? condition_path_67 : (condition_8 && (condition_12 && (ready_52 && (ready_54 && ready_56)))));
        const bool condition_path_73 = (ready_7 ? (condition_8 ? false : condition_path_69) : (condition_7 && (condition_8 ? false : condition_path_69)));
        const bool condition_path_74 = (condition_5 ? condition_path_71 : (ready_6 ? (ready_7 ? condition_path_67 : condition_path_72) : condition_path_73));
        const bool condition_path_75 = (condition_8 ? (condition_9 ? (condition_12 && (ready_52 && (ready_54 && ready_56))) : condition_path_66) : (condition_9 && (condition_12 && (ready_52 && (ready_54 && ready_56)))));
        const bool condition_path_76 = (condition_7 ? (condition_9 ? (condition_12 && (ready_52 && (ready_54 && ready_56))) : condition_path_66) : condition_path_75);
        const bool condition_path_77 = (ready_7 ? (condition_9 ? (condition_12 && (ready_52 && (ready_54 && ready_56))) : condition_path_66) : condition_path_76);
        const bool condition_path_78 = (ready_7 ? condition_path_69 : (condition_7 ? condition_path_69 : (condition_8 && condition_path_69)));
        const bool condition_path_79 = (condition_9 ? condition_path_66 : (condition_10 && (condition_12 && (ready_52 && (ready_54 && ready_56)))));
        const bool condition_path_80 = (ready_7 ? condition_path_66 : (condition_7 ? condition_path_66 : (condition_8 ? condition_path_66 : condition_path_79)));
        const bool condition_path_81 = (condition_9 && (condition_10 ? false : (condition_11 && (condition_12 && (ready_52 && (ready_54 && ready_56))))));
        const bool condition_path_82 = (condition_8 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_52 && (ready_54 && ready_56))))) : condition_path_81);
        const bool condition_path_83 = (condition_7 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_52 && (ready_54 && ready_56))))) : condition_path_82);
        const bool condition_path_84 = (ready_7 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_52 && (ready_54 && ready_56))))) : condition_path_83);
        const bool condition_path_85 = (condition_3 ? (condition_4 ? condition_path_74 : (ready_6 ? condition_path_77 : condition_path_78)) : (ready_6 ? condition_path_80 : condition_path_84));
        const bool condition_path_86 = (condition_2 ? condition_path_85 : (ready_6 && (condition_11 && (condition_12 && (ready_52 && (ready_54 && ready_56))))));
        const bool condition_path_87 = (jump ? (ready_52 && (ready_54 && ready_56)) : (condition ? (ready_52 && (ready_54 && ready_56)) : condition_path_86));
        const bool condition_path_88 = (ready ? (ready_2 ? (ready_52 && (ready_54 && ready_56)) : condition_path_87) : (ready_52 && (ready_54 && ready_56)));
        bool ready_58{};
        if (condition_path_88) {
          ready_58 = std::get<0>(this->compute_shared_computation_35());
        }
        const bool ready_59 = ((((ready_51 && ready_52) && ready_54) && ready_56) && ready_58);
        compute_shared_computation_36_cache.emplace(std::tuple<bool>{ready_59});
      }
      return *compute_shared_computation_36_cache;
    }
    const std::tuple<bool> &compute_shared_computation_37() {
      if (!compute_shared_computation_37_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_9());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_23());
        const bool ready_3 = (!ready_2);
        const bool ready_4 = (ready && ready_3);
        const bool ready_5 = (!ready_4);
        const bool jump = std::get<0>(this->compute_jump());
        const uint32_t opcode = std::get<0>(this->compute_opcode());
        const bool condition = (opcode!=99u);
        const bool condition_2 = std::get<0>(this->compute_shared_computation_17());
        const bool condition_3 = std::get<0>(this->compute_shared_computation_18());
        const bool condition_4 = std::get<0>(this->compute_shared_computation_20());
        const bool condition_5 = std::get<0>(this->compute_shared_computation_19());
        const bool condition_6 = std::get<0>(this->compute_shared_computation_21());
        const bool ready_6 = std::get<0>(this->compute_shared_computation_8());
        const uint32_t function = std::get<0>(this->compute_function());
        const bool condition_7 = (function==6u);
        const bool condition_8 = (function==5u);
        const bool condition_9 = (function==4u);
        const bool condition_10 = (function==1u);
        const bool condition_11 = (function==0u);
        const bool condition_path = (ready_6 ? (!condition_7) : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_2 = (ready_6 ? (condition_7 ? false : (!condition_8)) : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_3 = (ready_6 ? (condition_7 ? false : (condition_8 ? false : (!condition_9))) : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_4 = (condition_4 ? (condition_5 ? (condition_6 ? ready_6 : condition_path) : condition_path_2) : condition_path_3);
        const bool condition_path_5 = (condition_3 ? condition_path_4 : (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (!condition_10)))));
        const bool condition_path_6 = (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : (condition_10 ? false : (!condition_11)))));
        const bool condition_path_7 = (jump ? false : (condition ? false : (condition_2 ? condition_path_5 : (ready_6 && condition_path_6))));
        bool ready_7{};
        if ((ready && (ready_2 ? false : condition_path_7))) {
          ready_7 = std::get<0>(this->compute_shared_computation_29());
        }
        const bool ready_9 = (condition_2 || ready_6);
        const bool selected_value = (condition_10 ? ready_6 : ready_9);
        const bool selected_value_2 = (condition_9 ? ready_6 : selected_value);
        const bool selected_value_3 = (condition_8 ? ready_6 : selected_value_2);
        const bool ready_10 = (condition_7 || ready_7);
        const bool selected_value_6 = (condition_5 ? ((condition_6 ? (ready_6 && ready_7) : (condition_7 ? ready_6 : selected_value_3)) && ready_10) : selected_value_3);
        const bool ready_12 = (condition_8 || ready_10);
        const bool ready_14 = (condition_9 || ready_12);
        const bool selected_value_8 = (condition_3 ? ((condition_4 ? (selected_value_6 && ready_12) : selected_value_2) && ready_14) : selected_value);
        const bool ready_16 = (condition_10 || ready_14);
        const bool ready_18 = (condition_11 || ready_16);
        const bool ready_21 = (condition || ready_18);
        const bool condition_12 = std::get<0>(this->compute_shared_computation_14());
        const bool ready_23 = (condition_11 || condition_2);
        const bool ready_25 = (condition_10 || (condition_3 && ready_23));
        const bool ready_27 = (condition_9 || (condition_4 && ready_25));
        const bool ready_29 = (condition_8 || (condition_5 && ready_27));
        const bool ready_36 = ((condition_5 && (condition_6 || (condition_7 && (condition_7 || (condition_6 && ready_29))))) || (condition_8 && ready_29));
        const bool ready_42 = ((condition_3 && ((condition_4 && ready_36) || (condition_9 && ready_27))) || (condition_10 && ready_25));
        const bool ready_44 = (condition_11 && ready_23);
        const bool ready_48 = (((condition_12 && ((condition_2 && ready_42) || ready_44)) || condition) && ready_21);
        const bool ready_49 = (((condition || ((condition_2 ? (selected_value_8 && ready_16) : ready_9) && ready_18)) && ready_21) && ready_48);
        const bool ready_51 = (ready_5 || (jump || ready_49));
        const bool condition_13 = (opcode==55u);
        const bool condition_15 = std::get<0>(this->compute_shared_computation_13());
        const bool ready_53 = (((opcode==23u) || condition_15) || condition_12);
        const bool condition_16 = std::get<0>(this->compute_shared_computation_15());
        const bool ready_54 = (!ready_53);
        const bool selected_value_10 = (condition_16 ? false : ready_54);
        const bool ready_55 = ((opcode==3u) || (opcode==35u));
        const bool condition_19 = (opcode==51u);
        const Instruction executing = std::get<0>(this->compute_executing());
        const uint32_t funct7 = executing.funct7;
        const bool ready_56 = (condition_19 && (funct7==1u));
        const bool ready_57 = (!ready_55);
        const bool selected_value_11 = (ready_56 ? false : ready_57);
        const bool condition_21 = (opcode==19u);
        const bool ready_58 = (condition_19 || condition_21);
        const bool condition_22 = (function!=2u);
        const bool condition_23 = (function!=3u);
        const bool ready_59 = (condition_21 || ready_6);
        const bool ready_60 = (ready_6 && ready_59);
        const bool condition_24 = (funct7==32u);
        const uint32_t immediate = executing.immediate;
        const uint32_t rhs = std::get<0>(this->compute_rhs());
        const uint32_t operand = (condition_21 ? immediate : rhs);
        const uint32_t computed_value = (operand & 31u);
        const bool condition_26 = (function==3u);
        const bool condition_27 = (function==2u);
        const bool condition_28 = (function>3u);
        const bool condition_29 = (rhs!=0u);
        const bool condition_30 = (function!=7u);
        const bool condition_31 = (function==7u);
        const bool condition_32 = (rhs==0u);
        const bool ready_64 = (condition_3 || ready_6);
        const bool ready_65 = (condition_10 || condition_27);
        const bool ready_67 = ((!ready_65) || ready_6);
        const bool ready_70 = (ready_67 && ready_64);
        const bool ready_72 = (condition_11 || ready_70);
        const bool ready_74 = (condition_28 || ((condition_2 ? ((ready_64 && (ready_67 && ready_6)) && ready_70) : ready_9) && ready_72));
        const bool ready_75 = (ready_6 && (condition_32 ? ready_6 : ready_74));
        const bool selected_value_15 = (condition_8 ? ready_6 : ready_75);
        const bool selected_value_16 = (condition_31 ? ready_6 : selected_value_15);
        const bool ready_79 = ((ready_6 && (condition_4 || ready_6)) && (condition_9 || ready_6));
        const bool selected_value_19 = (condition_30 ? ((condition_4 ? ready_6 : (condition_9 ? ready_6 : selected_value_16)) && ready_79) : selected_value_16);
        const bool ready_81 = (condition_31 || ready_79);
        const bool ready_83 = (condition_8 || ready_81);
        const bool selected_value_21 = (condition_29 ? ((condition_5 ? (selected_value_19 && ready_81) : selected_value_15) && ready_83) : ready_75);
        const bool ready_87 = (ready_6 && (condition_32 || ready_83));
        const bool condition_33 = (function<=3u);
        const bool ready_91 = ((condition_28 || ready_72) && (condition_33 || ready_87));
        const uint32_t lhs = std::get<0>(this->compute_lhs());
        const uint32_t computed_value_2 = (lhs & 2147483648u);
        const bool condition_34 = (computed_value_2!=0u);
        const bool negative_a = (computed_value_2==0u);
        const bool condition_35 = ((rhs & 2147483648u)!=0u);
        const bool condition_36 = (condition_34!=condition_35);
        const bool ready_99 = (condition_10 && condition_35);
        const bool ready_106 = (condition_33 && ((condition_2 && ((ready_99 || ((!ready_99) && ready_67)) && ready_64)) || ready_44));
        const bool ready_107 = (ready_106 || condition_28);
        const bool ready_110 = ((condition_32 || (condition_29 && ready_107)) && ready_6);
        const bool ready_112 = (condition_8 || (condition_5 && ready_110));
        const bool ready_114 = (condition_31 || (condition_30 && ready_112));
        const bool ready_116 = ((condition_9 && ((condition_36 || (condition_34==condition_35)) && ready_6)) || (condition_4 && ready_114));
        const bool ready_119 = (condition_30 && ((condition_4 && ((condition_34 || negative_a) && ready_6)) || (condition_9 && ready_116)));
        const bool ready_125 = (condition_29 && ((condition_5 && (ready_119 || (condition_31 && ready_114))) || (condition_8 && ready_112)));
        const bool ready_131 = ((condition_28 && ((ready_125 || (condition_32 && ready_110)) && ready_6)) || (condition_33 && ready_107));
        const bool ready_132 = (ready_131 && ready_91);
        const bool ready_133 = (((condition_28 ? ((ready_6 && selected_value_21) && ready_87) : ready_74) && ready_91) && ready_132);
        const bool condition_38 = std::get<0>(this->compute_shared_computation_16());
        const bool ready_134 = (condition_38 || ready_6);
        const bool selected_value_23 = (ready_56 ? ready_133 : ready_134);
        const bool selected_value_24 = (condition_11 ? ready_60 : selected_value_23);
        const bool selected_value_25 = (condition_10 ? ready_60 : selected_value_24);
        const bool selected_value_26 = (condition_27 ? ready_60 : selected_value_25);
        const bool selected_value_27 = (condition_26 ? ready_60 : selected_value_26);
        const bool selected_value_28 = (condition_9 ? ready_60 : selected_value_27);
        const bool selected_value_29 = (condition_8 ? (condition_24 ? ((ready_59 && ((computed_value==0u) || ready_6)) && ready_60) : ready_60) : selected_value_28);
        const bool selected_value_32 = (condition_5 ? (condition_6 ? ready_60 : (condition_7 ? ready_60 : selected_value_29)) : selected_value_29);
        const bool selected_value_35 = (condition_22 ? (condition_23 ? (condition_4 ? selected_value_32 : selected_value_28) : selected_value_27) : selected_value_26);
        const bool selected_value_38 = (ready_58 ? (condition_2 ? (condition_3 ? selected_value_35 : selected_value_25) : selected_value_24) : selected_value_23);
        const bool selected_value_41 = (selected_value_10 ? (ready_55 ? ready_6 : (selected_value_11 ? selected_value_38 : selected_value_23)) : ready_134);
        const bool condition_path_8 = ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_19 ? false : (condition_21 ? false : condition_38))));
        const bool condition_path_9 = (condition_13 || ((opcode==23u) || (condition_15 || (condition_16 ? condition_38 : condition_path_8))));
        const bool condition_path_10 = (condition_19 ? ((funct7==1u) ? false : (condition_22 && (condition_23 && condition_38))) : (condition_21 ? (condition_22 && (condition_23 && condition_38)) : condition_38));
        const bool condition_path_11 = (condition_15 || (condition_16 ? condition_38 : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_10))));
        const bool condition_path_12 = (condition_11 ? (condition_12 || condition_path_9) : (condition_12 || (condition_13 || ((opcode==23u) || condition_path_11))));
        const bool condition_path_13 = (condition_9 ? (condition_12 || condition_path_9) : (condition_10 ? (condition_12 || condition_path_9) : condition_path_12));
        const bool condition_path_14 = (condition_7 ? (condition_12 || condition_path_9) : (condition_8 ? (condition_12 || condition_path_9) : condition_path_13));
        const bool condition_path_15 = ((opcode==35u) || (condition_19 ? ((funct7==1u) ? (condition_28 ? (!condition_29) : true) : true) : true));
        const bool condition_path_16 = (condition_13 || ((opcode==23u) || (condition_15 || (condition_16 || ((opcode==3u) || condition_path_15)))));
        const bool condition_path_17 = (ready_6 ? (condition_8 || (condition_12 || condition_path_16)) : (condition_8 ? (condition_12 || condition_path_9) : condition_path_13));
        const bool condition_path_18 = (condition_5 ? (condition_6 ? (ready_6 || (condition_12 || condition_path_9)) : (ready_6 || condition_path_14)) : condition_path_17);
        const bool condition_path_19 = (condition_19 ? ((funct7==1u) ? (condition_28 ? (condition_29 ? (!condition_30) : true) : true) : true) : true);
        const bool condition_path_20 = ((opcode==23u) || (condition_15 || (condition_16 || ((opcode==3u) || ((opcode==35u) || condition_path_19)))));
        const bool condition_path_21 = (ready_6 ? (condition_8 || (condition_9 || (condition_12 || (condition_13 || condition_path_20)))) : condition_path_13);
        const bool condition_path_22 = (condition_5 ? condition_path_21 : (ready_6 ? (condition_8 || (condition_12 || condition_path_16)) : condition_path_13));
        const bool condition_path_23 = ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_19 ? ((funct7==1u) ? false : condition_38) : condition_38)));
        const bool condition_path_24 = (condition_13 || ((opcode==23u) || (condition_15 || (condition_16 ? condition_38 : condition_path_23))));
        const bool condition_path_25 = (condition_10 ? (condition_12 || condition_path_9) : (condition_11 ? (condition_12 || condition_path_9) : (condition_12 || condition_path_24)));
        const bool condition_path_26 = (condition_5 ? (ready_6 || condition_path_25) : (ready_6 ? (condition_8 || (condition_12 || condition_path_16)) : condition_path_25));
        const bool condition_path_27 = (ready_6 ? (condition_8 || (condition_9 || (condition_12 || (condition_13 || condition_path_20)))) : condition_path_25);
        const bool condition_path_28 = (condition_5 ? condition_path_27 : (ready_6 ? (condition_8 || (condition_12 || condition_path_16)) : condition_path_25));
        const bool condition_path_29 = (condition_3 ? (condition_4 ? condition_path_18 : condition_path_22) : (condition_4 ? condition_path_26 : condition_path_28));
        const bool condition_path_30 = (condition_16 || ((opcode==3u) || ((opcode==35u) || (condition_19 ? ((funct7==1u) ? condition_28 : true) : true))));
        const bool condition_path_31 = (condition_11 || (condition_12 || (condition_13 || ((opcode==23u) || (condition_15 || condition_path_30)))));
        const bool condition_path_32 = (ready_6 ? condition_path_31 : (condition_11 ? (condition_12 || condition_path_9) : (condition_12 || condition_path_24)));
        const bool condition_path_33 = ((opcode==3u) || ((opcode==35u) || (condition_19 ? ((funct7==1u) ? (condition_28 && (!condition_29)) : true) : true)));
        const bool condition_path_34 = (condition_12 || (condition_13 || ((opcode==23u) || (condition_15 || (condition_16 || condition_path_33)))));
        const bool condition_path_35 = (condition_8 ? condition_path_31 : (condition_11 ? (condition_12 || condition_path_16) : condition_path_34));
        const bool condition_path_36 = (ready_6 ? condition_path_35 : (condition_11 ? (condition_12 || condition_path_9) : (condition_12 || condition_path_24)));
        const bool condition_path_37 = (condition_19 ? ((funct7==1u) ? (condition_28 && (condition_29 ? (!condition_30) : true)) : true) : true);
        const bool condition_path_38 = ((opcode==23u) || (condition_15 || (condition_16 || ((opcode==3u) || ((opcode==35u) || condition_path_37)))));
        const bool condition_path_39 = (condition_11 ? (condition_12 || (condition_13 || condition_path_20)) : (condition_12 || (condition_13 || condition_path_38)));
        const bool condition_path_40 = (ready_6 ? (condition_8 ? condition_path_31 : (condition_9 ? condition_path_31 : condition_path_39)) : (condition_11 ? (condition_12 || condition_path_9) : (condition_12 || condition_path_24)));
        const bool condition_path_41 = (condition_4 ? (condition_5 ? condition_path_32 : condition_path_36) : (condition_5 ? condition_path_40 : condition_path_36));
        const bool condition_path_42 = (condition_8 ? condition_12 : (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))));
        const bool condition_path_43 = (condition_8 ? false : (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12))));
        const bool condition_path_44 = (ready_6 ? (ready_7 ? (condition_7 ? condition_12 : condition_path_42) : (condition_7 && condition_12)) : (ready_7 && (condition_7 ? false : condition_path_43)));
        const bool condition_path_45 = (ready_6 ? (ready_7 ? condition_path_42 : (condition_7 ? condition_path_42 : (condition_8 && condition_12))) : (ready_7 ? condition_path_43 : (condition_7 && condition_path_43)));
        const bool condition_path_46 = (condition_5 ? (condition_6 ? (ready_6 && (ready_7 && condition_12)) : condition_path_44) : condition_path_45);
        const bool condition_path_47 = (condition_8 ? (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))) : (condition_9 && condition_12));
        const bool condition_path_48 = (condition_7 ? (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))) : condition_path_47);
        const bool condition_path_49 = (ready_7 ? (condition_9 ? condition_12 : (condition_10 ? condition_12 : (condition_11 && condition_12))) : condition_path_48);
        const bool condition_path_50 = (condition_7 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12))) : (condition_8 && (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12)))));
        const bool condition_path_51 = (ready_7 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && condition_12))) : condition_path_50);
        const bool condition_path_52 = (condition_9 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : (condition_10 && condition_12));
        const bool condition_path_53 = (condition_7 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : (condition_8 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : condition_path_52));
        const bool condition_path_54 = (condition_8 ? (condition_10 ? false : (condition_11 && condition_12)) : (condition_9 && (condition_10 ? false : (condition_11 && condition_12))));
        const bool condition_path_55 = (ready_7 ? (condition_10 ? false : (condition_11 && condition_12)) : (condition_7 ? (condition_10 ? false : (condition_11 && condition_12)) : condition_path_54));
        const bool condition_path_56 = (ready_6 ? (ready_7 ? (condition_10 ? condition_12 : (condition_11 && condition_12)) : condition_path_53) : condition_path_55);
        const bool condition_path_57 = (condition_3 ? (condition_4 ? condition_path_46 : (ready_6 ? condition_path_49 : condition_path_51)) : condition_path_56);
        const bool condition_path_58 = (condition ? (condition_2 ? condition_path_29 : condition_path_41) : (condition_2 ? condition_path_57 : (ready_6 && (condition_11 && condition_12))));
        const bool condition_path_59 = (ready_2 ? (condition_2 ? condition_path_29 : condition_path_41) : (jump ? (condition_2 ? condition_path_29 : condition_path_41) : condition_path_58));
        bool ready_137{};
        if ((ready ? condition_path_59 : (condition_2 ? condition_path_29 : condition_path_41))) {
          ready_137 = std::get<0>(this->compute_shared_computation_22());
        }
        const bool condition_39 = (opcode!=55u);
        const bool ready_142 = (condition_13 || condition_39);
        const bool ready_144 = (condition_16 || (condition_38 && ready_142));
        const bool ready_148 = (selected_value_11 || ((!selected_value_11) && ((ready_56 && ready_132) || ((!ready_56) && ready_144))));
        const bool ready_154 = ((selected_value_10 && (ready_55 || (ready_57 && ready_148))) || ((!selected_value_10) && ready_144));
        const bool ready_160 = (((condition_39 && (ready_53 || (ready_54 && ready_154))) || (condition_13 && ready_142)) && ready_137);
        const bool ready_161 = (((condition_13 || (ready_53 || selected_value_41)) && ready_137) && ready_160);
        const bool condition_path_60 = ((opcode==35u) ? false : (condition_19 ? false : (condition_21 ? false : (condition_38 && ready_137))));
        const bool condition_path_61 = (condition_15 ? ready_137 : (condition_16 ? (condition_38 && ready_137) : ((opcode==3u) ? false : condition_path_60)));
        const bool condition_path_62 = (condition_12 ? ready_137 : (condition_13 ? ready_137 : ((opcode==23u) ? ready_137 : condition_path_61)));
        const bool condition_path_63 = (condition_21 ? (condition_22 && (condition_23 && (condition_38 && ready_137))) : (condition_38 && ready_137));
        const bool condition_path_64 = (condition_19 ? ((funct7==1u) ? false : (condition_22 && (condition_23 && (condition_38 && ready_137)))) : condition_path_63);
        const bool condition_path_65 = (condition_16 ? (condition_38 && ready_137) : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_64)));
        const bool condition_path_66 = (condition_13 ? ready_137 : ((opcode==23u) ? ready_137 : (condition_15 ? ready_137 : condition_path_65)));
        const bool condition_path_67 = (condition_10 ? condition_path_62 : (condition_11 ? condition_path_62 : (condition_12 ? ready_137 : condition_path_66)));
        const bool condition_path_68 = (condition_7 ? condition_path_62 : (condition_8 ? condition_path_62 : (condition_9 ? condition_path_62 : condition_path_67)));
        const bool condition_path_69 = (condition_6 ? (ready_6 ? ready_137 : condition_path_62) : (ready_6 ? ready_137 : condition_path_68));
        const bool condition_path_70 = (condition_19 ? ((funct7==1u) ? (condition_28 ? (condition_29 ? false : ready_137) : ready_137) : ready_137) : ready_137);
        const bool condition_path_71 = (condition_16 ? ready_137 : ((opcode==3u) ? ready_137 : ((opcode==35u) ? ready_137 : condition_path_70)));
        const bool condition_path_72 = (condition_13 ? ready_137 : ((opcode==23u) ? ready_137 : (condition_15 ? ready_137 : condition_path_71)));
        const bool condition_path_73 = (ready_6 ? (condition_8 ? ready_137 : (condition_12 ? ready_137 : condition_path_72)) : (condition_8 ? condition_path_62 : (condition_9 ? condition_path_62 : condition_path_67)));
        const bool condition_path_74 = ((funct7==1u) ? (condition_28 ? (condition_29 ? (condition_30 ? false : ready_137) : ready_137) : ready_137) : ready_137);
        const bool condition_path_75 = ((opcode==3u) ? ready_137 : ((opcode==35u) ? ready_137 : (condition_19 ? condition_path_74 : ready_137)));
        const bool condition_path_76 = ((opcode==23u) ? ready_137 : (condition_15 ? ready_137 : (condition_16 ? ready_137 : condition_path_75)));
        const bool condition_path_77 = (condition_9 ? ready_137 : (condition_12 ? ready_137 : (condition_13 ? ready_137 : condition_path_76)));
        const bool condition_path_78 = (ready_6 ? (condition_8 ? ready_137 : condition_path_77) : (condition_9 ? condition_path_62 : condition_path_67));
        const bool condition_path_79 = (ready_6 ? (condition_8 ? ready_137 : (condition_12 ? ready_137 : condition_path_72)) : (condition_9 ? condition_path_62 : condition_path_67));
        const bool condition_path_80 = (condition_4 ? (condition_5 ? condition_path_69 : condition_path_73) : (condition_5 ? condition_path_78 : condition_path_79));
        const bool condition_path_81 = ((opcode==35u) ? false : (condition_19 ? ((funct7==1u) ? false : (condition_38 && ready_137)) : (condition_38 && ready_137)));
        const bool condition_path_82 = (condition_15 ? ready_137 : (condition_16 ? (condition_38 && ready_137) : ((opcode==3u) ? false : condition_path_81)));
        const bool condition_path_83 = (condition_12 ? ready_137 : (condition_13 ? ready_137 : ((opcode==23u) ? ready_137 : condition_path_82)));
        const bool condition_path_84 = (ready_6 ? ready_137 : (condition_10 ? condition_path_62 : (condition_11 ? condition_path_62 : condition_path_83)));
        const bool condition_path_85 = (ready_6 ? (condition_8 ? ready_137 : (condition_12 ? ready_137 : condition_path_72)) : (condition_10 ? condition_path_62 : (condition_11 ? condition_path_62 : condition_path_83)));
        const bool condition_path_86 = (ready_6 ? (condition_8 ? ready_137 : condition_path_77) : (condition_10 ? condition_path_62 : (condition_11 ? condition_path_62 : condition_path_83)));
        const bool condition_path_87 = (condition_4 ? (condition_5 ? condition_path_84 : condition_path_85) : (condition_5 ? condition_path_86 : condition_path_85));
        const bool condition_path_88 = ((opcode==35u) ? ready_137 : (condition_19 ? ((funct7==1u) ? (condition_28 && ready_137) : ready_137) : ready_137));
        const bool condition_path_89 = (condition_15 ? ready_137 : (condition_16 ? ready_137 : ((opcode==3u) ? ready_137 : condition_path_88)));
        const bool condition_path_90 = (condition_12 ? ready_137 : (condition_13 ? ready_137 : ((opcode==23u) ? ready_137 : condition_path_89)));
        const bool condition_path_91 = (ready_6 ? (condition_11 ? ready_137 : condition_path_90) : (condition_11 ? condition_path_62 : condition_path_83));
        const bool condition_path_92 = (condition_19 ? ((funct7==1u) ? (condition_28 && (condition_29 ? false : ready_137)) : ready_137) : ready_137);
        const bool condition_path_93 = (condition_16 ? ready_137 : ((opcode==3u) ? ready_137 : ((opcode==35u) ? ready_137 : condition_path_92)));
        const bool condition_path_94 = (condition_13 ? ready_137 : ((opcode==23u) ? ready_137 : (condition_15 ? ready_137 : condition_path_93)));
        const bool condition_path_95 = (condition_11 ? (condition_12 ? ready_137 : condition_path_72) : (condition_12 ? ready_137 : condition_path_94));
        const bool condition_path_96 = (ready_6 ? (condition_8 ? (condition_11 ? ready_137 : condition_path_90) : condition_path_95) : (condition_11 ? condition_path_62 : condition_path_83));
        const bool condition_path_97 = ((funct7==1u) ? (condition_28 && (condition_29 ? (condition_30 ? false : ready_137) : ready_137)) : ready_137);
        const bool condition_path_98 = ((opcode==3u) ? ready_137 : ((opcode==35u) ? ready_137 : (condition_19 ? condition_path_97 : ready_137)));
        const bool condition_path_99 = ((opcode==23u) ? ready_137 : (condition_15 ? ready_137 : (condition_16 ? ready_137 : condition_path_98)));
        const bool condition_path_100 = (condition_11 ? (condition_12 ? ready_137 : (condition_13 ? ready_137 : condition_path_76)) : (condition_12 ? ready_137 : (condition_13 ? ready_137 : condition_path_99)));
        const bool condition_path_101 = (condition_8 ? (condition_11 ? ready_137 : condition_path_90) : (condition_9 ? (condition_11 ? ready_137 : condition_path_90) : condition_path_100));
        const bool condition_path_102 = (condition_5 ? (ready_6 ? condition_path_101 : (condition_11 ? condition_path_62 : condition_path_83)) : condition_path_96);
        const bool condition_path_103 = (condition_2 ? (condition_3 ? condition_path_80 : condition_path_87) : (condition_4 ? (condition_5 ? condition_path_91 : condition_path_96) : condition_path_102));
        const bool condition_path_104 = (condition_9 ? (condition_12 && ready_137) : (condition_10 ? (condition_12 && ready_137) : (condition_11 && (condition_12 && ready_137))));
        const bool condition_path_105 = (condition_7 ? (condition_12 && ready_137) : (condition_8 ? (condition_12 && ready_137) : condition_path_104));
        const bool condition_path_106 = (condition_8 ? false : (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_137)))));
        const bool condition_path_107 = (ready_6 ? (ready_7 ? condition_path_105 : (condition_7 && (condition_12 && ready_137))) : (ready_7 && (condition_7 ? false : condition_path_106)));
        const bool condition_path_108 = (condition_7 ? (condition_8 ? (condition_12 && ready_137) : condition_path_104) : (condition_8 && (condition_12 && ready_137)));
        const bool condition_path_109 = (ready_6 ? (ready_7 ? (condition_8 ? (condition_12 && ready_137) : condition_path_104) : condition_path_108) : (ready_7 ? condition_path_106 : (condition_7 && condition_path_106)));
        const bool condition_path_110 = (condition_5 ? (condition_6 ? (ready_6 && (ready_7 && (condition_12 && ready_137))) : condition_path_107) : condition_path_109);
        const bool condition_path_111 = (condition_7 ? condition_path_104 : (condition_8 ? condition_path_104 : (condition_9 && (condition_12 && ready_137))));
        const bool condition_path_112 = (condition_8 && (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_137)))));
        const bool condition_path_113 = (condition_7 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_137)))) : condition_path_112);
        const bool condition_path_114 = (ready_7 ? (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && ready_137)))) : condition_path_113);
        const bool condition_path_115 = (condition_4 ? condition_path_110 : (ready_6 ? (ready_7 ? condition_path_104 : condition_path_111) : condition_path_114));
        const bool condition_path_116 = (condition_9 ? (condition_10 ? (condition_12 && ready_137) : (condition_11 && (condition_12 && ready_137))) : (condition_10 && (condition_12 && ready_137)));
        const bool condition_path_117 = (condition_8 ? (condition_10 ? (condition_12 && ready_137) : (condition_11 && (condition_12 && ready_137))) : condition_path_116);
        const bool condition_path_118 = (condition_7 ? (condition_10 ? (condition_12 && ready_137) : (condition_11 && (condition_12 && ready_137))) : condition_path_117);
        const bool condition_path_119 = (ready_7 ? (condition_10 ? (condition_12 && ready_137) : (condition_11 && (condition_12 && ready_137))) : condition_path_118);
        const bool condition_path_120 = (condition_8 ? (condition_10 ? false : (condition_11 && (condition_12 && ready_137))) : (condition_9 && (condition_10 ? false : (condition_11 && (condition_12 && ready_137)))));
        const bool condition_path_121 = (condition_7 ? (condition_10 ? false : (condition_11 && (condition_12 && ready_137))) : condition_path_120);
        const bool condition_path_122 = (ready_7 ? (condition_10 ? false : (condition_11 && (condition_12 && ready_137))) : condition_path_121);
        const bool condition_path_123 = (condition_2 ? (condition_3 ? condition_path_115 : (ready_6 ? condition_path_119 : condition_path_122)) : (ready_6 && (condition_11 && (condition_12 && ready_137))));
        const bool condition_path_124 = (ready_2 ? condition_path_103 : (jump ? condition_path_103 : (condition ? condition_path_103 : condition_path_123)));
        bool ready_163{};
        if ((ready ? condition_path_124 : condition_path_103)) {
          ready_163 = std::get<0>(this->compute_shared_computation_31());
        }
        const bool ready_164 = ((ready_51 && ready_161) && ready_163);
        const bool condition_path_125 = ((opcode==35u) ? false : (condition_19 ? false : (condition_21 ? false : (condition_38 && (ready_137 && ready_163)))));
        const bool condition_path_126 = (condition_16 ? (condition_38 && (ready_137 && ready_163)) : ((opcode==3u) ? false : condition_path_125));
        const bool condition_path_127 = ((opcode==23u) ? (ready_137 && ready_163) : (condition_15 ? (ready_137 && ready_163) : condition_path_126));
        const bool condition_path_128 = (condition_12 ? (ready_137 && ready_163) : (condition_13 ? (ready_137 && ready_163) : condition_path_127));
        const bool condition_path_129 = ((funct7==1u) ? false : (condition_22 && (condition_23 && (condition_38 && (ready_137 && ready_163)))));
        const bool condition_path_130 = (condition_21 ? (condition_22 && (condition_23 && (condition_38 && (ready_137 && ready_163)))) : (condition_38 && (ready_137 && ready_163)));
        const bool condition_path_131 = ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_19 ? condition_path_129 : condition_path_130)));
        const bool condition_path_132 = (condition_15 ? (ready_137 && ready_163) : (condition_16 ? (condition_38 && (ready_137 && ready_163)) : condition_path_131));
        const bool condition_path_133 = (condition_13 ? (ready_137 && ready_163) : ((opcode==23u) ? (ready_137 && ready_163) : condition_path_132));
        const bool condition_path_134 = (condition_10 ? condition_path_128 : (condition_11 ? condition_path_128 : (condition_12 ? (ready_137 && ready_163) : condition_path_133)));
        const bool condition_path_135 = (condition_7 ? condition_path_128 : (condition_8 ? condition_path_128 : (condition_9 ? condition_path_128 : condition_path_134)));
        const bool condition_path_136 = (condition_6 ? (ready_6 ? (ready_137 && ready_163) : condition_path_128) : (ready_6 ? (ready_137 && ready_163) : condition_path_135));
        const bool condition_path_137 = ((funct7==1u) ? (condition_28 ? (condition_29 ? false : (ready_137 && ready_163)) : (ready_137 && ready_163)) : (ready_137 && ready_163));
        const bool condition_path_138 = ((opcode==35u) ? (ready_137 && ready_163) : (condition_19 ? condition_path_137 : (ready_137 && ready_163)));
        const bool condition_path_139 = (condition_16 ? (ready_137 && ready_163) : ((opcode==3u) ? (ready_137 && ready_163) : condition_path_138));
        const bool condition_path_140 = ((opcode==23u) ? (ready_137 && ready_163) : (condition_15 ? (ready_137 && ready_163) : condition_path_139));
        const bool condition_path_141 = (condition_12 ? (ready_137 && ready_163) : (condition_13 ? (ready_137 && ready_163) : condition_path_140));
        const bool condition_path_142 = (ready_6 ? (condition_8 ? (ready_137 && ready_163) : condition_path_141) : (condition_8 ? condition_path_128 : (condition_9 ? condition_path_128 : condition_path_134)));
        const bool condition_path_143 = (condition_28 ? (condition_29 ? (condition_30 ? false : (ready_137 && ready_163)) : (ready_137 && ready_163)) : (ready_137 && ready_163));
        const bool condition_path_144 = (condition_19 ? ((funct7==1u) ? condition_path_143 : (ready_137 && ready_163)) : (ready_137 && ready_163));
        const bool condition_path_145 = ((opcode==3u) ? (ready_137 && ready_163) : ((opcode==35u) ? (ready_137 && ready_163) : condition_path_144));
        const bool condition_path_146 = (condition_15 ? (ready_137 && ready_163) : (condition_16 ? (ready_137 && ready_163) : condition_path_145));
        const bool condition_path_147 = (condition_13 ? (ready_137 && ready_163) : ((opcode==23u) ? (ready_137 && ready_163) : condition_path_146));
        const bool condition_path_148 = (condition_9 ? (ready_137 && ready_163) : (condition_12 ? (ready_137 && ready_163) : condition_path_147));
        const bool condition_path_149 = (ready_6 ? (condition_8 ? (ready_137 && ready_163) : condition_path_148) : (condition_9 ? condition_path_128 : condition_path_134));
        const bool condition_path_150 = (ready_6 ? (condition_8 ? (ready_137 && ready_163) : condition_path_141) : (condition_9 ? condition_path_128 : condition_path_134));
        const bool condition_path_151 = (condition_4 ? (condition_5 ? condition_path_136 : condition_path_142) : (condition_5 ? condition_path_149 : condition_path_150));
        const bool condition_path_152 = (condition_19 ? ((funct7==1u) ? false : (condition_38 && (ready_137 && ready_163))) : (condition_38 && (ready_137 && ready_163)));
        const bool condition_path_153 = (condition_16 ? (condition_38 && (ready_137 && ready_163)) : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_152)));
        const bool condition_path_154 = ((opcode==23u) ? (ready_137 && ready_163) : (condition_15 ? (ready_137 && ready_163) : condition_path_153));
        const bool condition_path_155 = (condition_12 ? (ready_137 && ready_163) : (condition_13 ? (ready_137 && ready_163) : condition_path_154));
        const bool condition_path_156 = (ready_6 ? (ready_137 && ready_163) : (condition_10 ? condition_path_128 : (condition_11 ? condition_path_128 : condition_path_155)));
        const bool condition_path_157 = (ready_6 ? (condition_8 ? (ready_137 && ready_163) : condition_path_141) : (condition_10 ? condition_path_128 : (condition_11 ? condition_path_128 : condition_path_155)));
        const bool condition_path_158 = (ready_6 ? (condition_8 ? (ready_137 && ready_163) : condition_path_148) : (condition_10 ? condition_path_128 : (condition_11 ? condition_path_128 : condition_path_155)));
        const bool condition_path_159 = (condition_4 ? (condition_5 ? condition_path_156 : condition_path_157) : (condition_5 ? condition_path_158 : condition_path_157));
        const bool condition_path_160 = (condition_19 ? ((funct7==1u) ? (condition_28 && (ready_137 && ready_163)) : (ready_137 && ready_163)) : (ready_137 && ready_163));
        const bool condition_path_161 = ((opcode==3u) ? (ready_137 && ready_163) : ((opcode==35u) ? (ready_137 && ready_163) : condition_path_160));
        const bool condition_path_162 = (condition_15 ? (ready_137 && ready_163) : (condition_16 ? (ready_137 && ready_163) : condition_path_161));
        const bool condition_path_163 = (condition_13 ? (ready_137 && ready_163) : ((opcode==23u) ? (ready_137 && ready_163) : condition_path_162));
        const bool condition_path_164 = (condition_11 ? (ready_137 && ready_163) : (condition_12 ? (ready_137 && ready_163) : condition_path_163));
        const bool condition_path_165 = ((funct7==1u) ? (condition_28 && (condition_29 ? false : (ready_137 && ready_163))) : (ready_137 && ready_163));
        const bool condition_path_166 = ((opcode==35u) ? (ready_137 && ready_163) : (condition_19 ? condition_path_165 : (ready_137 && ready_163)));
        const bool condition_path_167 = (condition_16 ? (ready_137 && ready_163) : ((opcode==3u) ? (ready_137 && ready_163) : condition_path_166));
        const bool condition_path_168 = ((opcode==23u) ? (ready_137 && ready_163) : (condition_15 ? (ready_137 && ready_163) : condition_path_167));
        const bool condition_path_169 = (condition_12 ? (ready_137 && ready_163) : (condition_13 ? (ready_137 && ready_163) : condition_path_168));
        const bool condition_path_170 = (ready_6 ? (condition_8 ? condition_path_164 : (condition_11 ? condition_path_141 : condition_path_169)) : (condition_11 ? condition_path_128 : condition_path_155));
        const bool condition_path_171 = (condition_5 ? (ready_6 ? condition_path_164 : (condition_11 ? condition_path_128 : condition_path_155)) : condition_path_170);
        const bool condition_path_172 = (condition_28 && (condition_29 ? (condition_30 ? false : (ready_137 && ready_163)) : (ready_137 && ready_163)));
        const bool condition_path_173 = (condition_19 ? ((funct7==1u) ? condition_path_172 : (ready_137 && ready_163)) : (ready_137 && ready_163));
        const bool condition_path_174 = ((opcode==3u) ? (ready_137 && ready_163) : ((opcode==35u) ? (ready_137 && ready_163) : condition_path_173));
        const bool condition_path_175 = (condition_15 ? (ready_137 && ready_163) : (condition_16 ? (ready_137 && ready_163) : condition_path_174));
        const bool condition_path_176 = (condition_13 ? (ready_137 && ready_163) : ((opcode==23u) ? (ready_137 && ready_163) : condition_path_175));
        const bool condition_path_177 = (condition_11 ? (condition_12 ? (ready_137 && ready_163) : condition_path_147) : (condition_12 ? (ready_137 && ready_163) : condition_path_176));
        const bool condition_path_178 = (ready_6 ? (condition_8 ? condition_path_164 : (condition_9 ? condition_path_164 : condition_path_177)) : (condition_11 ? condition_path_128 : condition_path_155));
        const bool condition_path_179 = (condition_2 ? (condition_3 ? condition_path_151 : condition_path_159) : (condition_4 ? condition_path_171 : (condition_5 ? condition_path_178 : condition_path_170)));
        const bool condition_path_180 = (condition_10 ? (condition_12 && (ready_137 && ready_163)) : (condition_11 && (condition_12 && (ready_137 && ready_163))));
        const bool condition_path_181 = (condition_8 ? (condition_12 && (ready_137 && ready_163)) : (condition_9 ? (condition_12 && (ready_137 && ready_163)) : condition_path_180));
        const bool condition_path_182 = (ready_7 ? (condition_7 ? (condition_12 && (ready_137 && ready_163)) : condition_path_181) : (condition_7 && (condition_12 && (ready_137 && ready_163))));
        const bool condition_path_183 = (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && (ready_137 && ready_163)))));
        const bool condition_path_184 = (ready_6 ? condition_path_182 : (ready_7 && (condition_7 ? false : (condition_8 ? false : condition_path_183))));
        const bool condition_path_185 = (condition_6 ? (ready_6 && (ready_7 && (condition_12 && (ready_137 && ready_163)))) : condition_path_184);
        const bool condition_path_186 = (ready_7 ? condition_path_181 : (condition_7 ? condition_path_181 : (condition_8 && (condition_12 && (ready_137 && ready_163)))));
        const bool condition_path_187 = (ready_7 ? (condition_8 ? false : condition_path_183) : (condition_7 && (condition_8 ? false : condition_path_183)));
        const bool condition_path_188 = (condition_8 ? (condition_9 ? (condition_12 && (ready_137 && ready_163)) : condition_path_180) : (condition_9 && (condition_12 && (ready_137 && ready_163))));
        const bool condition_path_189 = (condition_7 ? (condition_9 ? (condition_12 && (ready_137 && ready_163)) : condition_path_180) : condition_path_188);
        const bool condition_path_190 = (ready_7 ? (condition_9 ? (condition_12 && (ready_137 && ready_163)) : condition_path_180) : condition_path_189);
        const bool condition_path_191 = (ready_7 ? condition_path_183 : (condition_7 ? condition_path_183 : (condition_8 && condition_path_183)));
        const bool condition_path_192 = (condition_4 ? (condition_5 ? condition_path_185 : (ready_6 ? condition_path_186 : condition_path_187)) : (ready_6 ? condition_path_190 : condition_path_191));
        const bool condition_path_193 = (condition_8 ? condition_path_180 : (condition_9 ? condition_path_180 : (condition_10 && (condition_12 && (ready_137 && ready_163)))));
        const bool condition_path_194 = (condition_9 && (condition_10 ? false : (condition_11 && (condition_12 && (ready_137 && ready_163)))));
        const bool condition_path_195 = (condition_8 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_137 && ready_163)))) : condition_path_194);
        const bool condition_path_196 = (condition_7 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_137 && ready_163)))) : condition_path_195);
        const bool condition_path_197 = (ready_7 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_137 && ready_163)))) : condition_path_196);
        const bool condition_path_198 = (ready_6 ? (ready_7 ? condition_path_180 : (condition_7 ? condition_path_180 : condition_path_193)) : condition_path_197);
        const bool condition_path_199 = (condition_2 ? (condition_3 ? condition_path_192 : condition_path_198) : (ready_6 && (condition_11 && (condition_12 && (ready_137 && ready_163)))));
        const bool condition_path_200 = (ready_2 ? condition_path_179 : (jump ? condition_path_179 : (condition ? condition_path_179 : condition_path_199)));
        bool ready_165{};
        if ((ready ? condition_path_200 : condition_path_179)) {
          ready_165 = std::get<0>(this->compute_shared_computation_12());
        }
        const bool condition_path_201 = (condition_19 ? false : (condition_21 ? false : (condition_38 && (ready_137 && (ready_163 && ready_165)))));
        const bool condition_path_202 = (condition_16 ? (condition_38 && (ready_137 && (ready_163 && ready_165))) : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_201)));
        const bool condition_path_203 = ((opcode==23u) ? (ready_137 && (ready_163 && ready_165)) : (condition_15 ? (ready_137 && (ready_163 && ready_165)) : condition_path_202));
        const bool condition_path_204 = (condition_12 ? (ready_137 && (ready_163 && ready_165)) : (condition_13 ? (ready_137 && (ready_163 && ready_165)) : condition_path_203));
        const bool condition_path_205 = ((funct7==1u) ? false : (condition_22 && (condition_23 && (condition_38 && (ready_137 && (ready_163 && ready_165))))));
        const bool condition_path_206 = (condition_21 ? (condition_22 && (condition_23 && (condition_38 && (ready_137 && (ready_163 && ready_165))))) : (condition_38 && (ready_137 && (ready_163 && ready_165))));
        const bool condition_path_207 = ((opcode==3u) ? false : ((opcode==35u) ? false : (condition_19 ? condition_path_205 : condition_path_206)));
        const bool condition_path_208 = (condition_15 ? (ready_137 && (ready_163 && ready_165)) : (condition_16 ? (condition_38 && (ready_137 && (ready_163 && ready_165))) : condition_path_207));
        const bool condition_path_209 = (condition_13 ? (ready_137 && (ready_163 && ready_165)) : ((opcode==23u) ? (ready_137 && (ready_163 && ready_165)) : condition_path_208));
        const bool condition_path_210 = (condition_11 ? condition_path_204 : (condition_12 ? (ready_137 && (ready_163 && ready_165)) : condition_path_209));
        const bool condition_path_211 = (condition_8 ? condition_path_204 : (condition_9 ? condition_path_204 : (condition_10 ? condition_path_204 : condition_path_210)));
        const bool condition_path_212 = (ready_6 ? (ready_137 && (ready_163 && ready_165)) : (condition_7 ? condition_path_204 : condition_path_211));
        const bool condition_path_213 = (condition_6 ? (ready_6 ? (ready_137 && (ready_163 && ready_165)) : condition_path_204) : condition_path_212);
        const bool condition_path_214 = (condition_28 ? (condition_29 ? false : (ready_137 && (ready_163 && ready_165))) : (ready_137 && (ready_163 && ready_165)));
        const bool condition_path_215 = (condition_19 ? ((funct7==1u) ? condition_path_214 : (ready_137 && (ready_163 && ready_165))) : (ready_137 && (ready_163 && ready_165)));
        const bool condition_path_216 = ((opcode==3u) ? (ready_137 && (ready_163 && ready_165)) : ((opcode==35u) ? (ready_137 && (ready_163 && ready_165)) : condition_path_215));
        const bool condition_path_217 = (condition_15 ? (ready_137 && (ready_163 && ready_165)) : (condition_16 ? (ready_137 && (ready_163 && ready_165)) : condition_path_216));
        const bool condition_path_218 = (condition_13 ? (ready_137 && (ready_163 && ready_165)) : ((opcode==23u) ? (ready_137 && (ready_163 && ready_165)) : condition_path_217));
        const bool condition_path_219 = (condition_8 ? (ready_137 && (ready_163 && ready_165)) : (condition_12 ? (ready_137 && (ready_163 && ready_165)) : condition_path_218));
        const bool condition_path_220 = (condition_29 ? (condition_30 ? false : (ready_137 && (ready_163 && ready_165))) : (ready_137 && (ready_163 && ready_165)));
        const bool condition_path_221 = ((funct7==1u) ? (condition_28 ? condition_path_220 : (ready_137 && (ready_163 && ready_165))) : (ready_137 && (ready_163 && ready_165)));
        const bool condition_path_222 = ((opcode==35u) ? (ready_137 && (ready_163 && ready_165)) : (condition_19 ? condition_path_221 : (ready_137 && (ready_163 && ready_165))));
        const bool condition_path_223 = (condition_16 ? (ready_137 && (ready_163 && ready_165)) : ((opcode==3u) ? (ready_137 && (ready_163 && ready_165)) : condition_path_222));
        const bool condition_path_224 = ((opcode==23u) ? (ready_137 && (ready_163 && ready_165)) : (condition_15 ? (ready_137 && (ready_163 && ready_165)) : condition_path_223));
        const bool condition_path_225 = (condition_12 ? (ready_137 && (ready_163 && ready_165)) : (condition_13 ? (ready_137 && (ready_163 && ready_165)) : condition_path_224));
        const bool condition_path_226 = (condition_8 ? (ready_137 && (ready_163 && ready_165)) : (condition_9 ? (ready_137 && (ready_163 && ready_165)) : condition_path_225));
        const bool condition_path_227 = (ready_6 ? condition_path_226 : (condition_9 ? condition_path_204 : (condition_10 ? condition_path_204 : condition_path_210)));
        const bool condition_path_228 = (ready_6 ? condition_path_219 : (condition_9 ? condition_path_204 : (condition_10 ? condition_path_204 : condition_path_210)));
        const bool condition_path_229 = (condition_4 ? (condition_5 ? condition_path_213 : (ready_6 ? condition_path_219 : condition_path_211)) : (condition_5 ? condition_path_227 : condition_path_228));
        const bool condition_path_230 = (condition_19 ? ((funct7==1u) ? false : (condition_38 && (ready_137 && (ready_163 && ready_165)))) : (condition_38 && (ready_137 && (ready_163 && ready_165))));
        const bool condition_path_231 = (condition_16 ? (condition_38 && (ready_137 && (ready_163 && ready_165))) : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_230)));
        const bool condition_path_232 = ((opcode==23u) ? (ready_137 && (ready_163 && ready_165)) : (condition_15 ? (ready_137 && (ready_163 && ready_165)) : condition_path_231));
        const bool condition_path_233 = (condition_12 ? (ready_137 && (ready_163 && ready_165)) : (condition_13 ? (ready_137 && (ready_163 && ready_165)) : condition_path_232));
        const bool condition_path_234 = (ready_6 ? (ready_137 && (ready_163 && ready_165)) : (condition_10 ? condition_path_204 : (condition_11 ? condition_path_204 : condition_path_233)));
        const bool condition_path_235 = (ready_6 ? condition_path_219 : (condition_10 ? condition_path_204 : (condition_11 ? condition_path_204 : condition_path_233)));
        const bool condition_path_236 = (ready_6 ? condition_path_226 : (condition_10 ? condition_path_204 : (condition_11 ? condition_path_204 : condition_path_233)));
        const bool condition_path_237 = (condition_4 ? (condition_5 ? condition_path_234 : condition_path_235) : (condition_5 ? condition_path_236 : condition_path_235));
        const bool condition_path_238 = ((funct7==1u) ? (condition_28 && (ready_137 && (ready_163 && ready_165))) : (ready_137 && (ready_163 && ready_165)));
        const bool condition_path_239 = ((opcode==35u) ? (ready_137 && (ready_163 && ready_165)) : (condition_19 ? condition_path_238 : (ready_137 && (ready_163 && ready_165))));
        const bool condition_path_240 = (condition_16 ? (ready_137 && (ready_163 && ready_165)) : ((opcode==3u) ? (ready_137 && (ready_163 && ready_165)) : condition_path_239));
        const bool condition_path_241 = ((opcode==23u) ? (ready_137 && (ready_163 && ready_165)) : (condition_15 ? (ready_137 && (ready_163 && ready_165)) : condition_path_240));
        const bool condition_path_242 = (condition_12 ? (ready_137 && (ready_163 && ready_165)) : (condition_13 ? (ready_137 && (ready_163 && ready_165)) : condition_path_241));
        const bool condition_path_243 = (ready_6 ? (condition_11 ? (ready_137 && (ready_163 && ready_165)) : condition_path_242) : (condition_11 ? condition_path_204 : condition_path_233));
        const bool condition_path_244 = ((funct7==1u) ? (condition_28 && (condition_29 ? false : (ready_137 && (ready_163 && ready_165)))) : (ready_137 && (ready_163 && ready_165)));
        const bool condition_path_245 = ((opcode==35u) ? (ready_137 && (ready_163 && ready_165)) : (condition_19 ? condition_path_244 : (ready_137 && (ready_163 && ready_165))));
        const bool condition_path_246 = (condition_16 ? (ready_137 && (ready_163 && ready_165)) : ((opcode==3u) ? (ready_137 && (ready_163 && ready_165)) : condition_path_245));
        const bool condition_path_247 = ((opcode==23u) ? (ready_137 && (ready_163 && ready_165)) : (condition_15 ? (ready_137 && (ready_163 && ready_165)) : condition_path_246));
        const bool condition_path_248 = (condition_12 ? (ready_137 && (ready_163 && ready_165)) : (condition_13 ? (ready_137 && (ready_163 && ready_165)) : condition_path_247));
        const bool condition_path_249 = (condition_11 ? (condition_12 ? (ready_137 && (ready_163 && ready_165)) : condition_path_218) : condition_path_248);
        const bool condition_path_250 = (condition_8 ? (condition_11 ? (ready_137 && (ready_163 && ready_165)) : condition_path_242) : condition_path_249);
        const bool condition_path_251 = (condition_5 ? condition_path_243 : (ready_6 ? condition_path_250 : (condition_11 ? condition_path_204 : condition_path_233)));
        const bool condition_path_252 = (condition_19 ? ((funct7==1u) ? (condition_28 && condition_path_220) : (ready_137 && (ready_163 && ready_165))) : (ready_137 && (ready_163 && ready_165)));
        const bool condition_path_253 = ((opcode==3u) ? (ready_137 && (ready_163 && ready_165)) : ((opcode==35u) ? (ready_137 && (ready_163 && ready_165)) : condition_path_252));
        const bool condition_path_254 = (condition_15 ? (ready_137 && (ready_163 && ready_165)) : (condition_16 ? (ready_137 && (ready_163 && ready_165)) : condition_path_253));
        const bool condition_path_255 = (condition_13 ? (ready_137 && (ready_163 && ready_165)) : ((opcode==23u) ? (ready_137 && (ready_163 && ready_165)) : condition_path_254));
        const bool condition_path_256 = (condition_11 ? condition_path_225 : (condition_12 ? (ready_137 && (ready_163 && ready_165)) : condition_path_255));
        const bool condition_path_257 = (condition_9 ? (condition_11 ? (ready_137 && (ready_163 && ready_165)) : condition_path_242) : condition_path_256);
        const bool condition_path_258 = (condition_8 ? (condition_11 ? (ready_137 && (ready_163 && ready_165)) : condition_path_242) : condition_path_257);
        const bool condition_path_259 = (condition_5 ? (ready_6 ? condition_path_258 : (condition_11 ? condition_path_204 : condition_path_233)) : (ready_6 ? condition_path_250 : (condition_11 ? condition_path_204 : condition_path_233)));
        const bool condition_path_260 = (condition_2 ? (condition_3 ? condition_path_229 : condition_path_237) : (condition_4 ? condition_path_251 : condition_path_259));
        const bool condition_path_261 = (condition_10 ? (condition_12 && (ready_137 && (ready_163 && ready_165))) : (condition_11 && (condition_12 && (ready_137 && (ready_163 && ready_165)))));
        const bool condition_path_262 = (condition_8 ? (condition_12 && (ready_137 && (ready_163 && ready_165))) : (condition_9 ? (condition_12 && (ready_137 && (ready_163 && ready_165))) : condition_path_261));
        const bool condition_path_263 = (ready_7 ? (condition_7 ? (condition_12 && (ready_137 && (ready_163 && ready_165))) : condition_path_262) : (condition_7 && (condition_12 && (ready_137 && (ready_163 && ready_165)))));
        const bool condition_path_264 = (condition_9 ? false : (condition_10 ? false : (condition_11 && (condition_12 && (ready_137 && (ready_163 && ready_165))))));
        const bool condition_path_265 = (ready_6 ? condition_path_263 : (ready_7 && (condition_7 ? false : (condition_8 ? false : condition_path_264))));
        const bool condition_path_266 = (condition_6 ? (ready_6 && (ready_7 && (condition_12 && (ready_137 && (ready_163 && ready_165))))) : condition_path_265);
        const bool condition_path_267 = (condition_7 ? condition_path_262 : (condition_8 && (condition_12 && (ready_137 && (ready_163 && ready_165)))));
        const bool condition_path_268 = (ready_7 ? (condition_8 ? false : condition_path_264) : (condition_7 && (condition_8 ? false : condition_path_264)));
        const bool condition_path_269 = (condition_5 ? condition_path_266 : (ready_6 ? (ready_7 ? condition_path_262 : condition_path_267) : condition_path_268));
        const bool condition_path_270 = (condition_8 ? (condition_9 ? (condition_12 && (ready_137 && (ready_163 && ready_165))) : condition_path_261) : (condition_9 && (condition_12 && (ready_137 && (ready_163 && ready_165)))));
        const bool condition_path_271 = (condition_7 ? (condition_9 ? (condition_12 && (ready_137 && (ready_163 && ready_165))) : condition_path_261) : condition_path_270);
        const bool condition_path_272 = (ready_7 ? (condition_9 ? (condition_12 && (ready_137 && (ready_163 && ready_165))) : condition_path_261) : condition_path_271);
        const bool condition_path_273 = (ready_7 ? condition_path_264 : (condition_7 ? condition_path_264 : (condition_8 && condition_path_264)));
        const bool condition_path_274 = (condition_9 ? condition_path_261 : (condition_10 && (condition_12 && (ready_137 && (ready_163 && ready_165)))));
        const bool condition_path_275 = (ready_7 ? condition_path_261 : (condition_7 ? condition_path_261 : (condition_8 ? condition_path_261 : condition_path_274)));
        const bool condition_path_276 = (condition_9 && (condition_10 ? false : (condition_11 && (condition_12 && (ready_137 && (ready_163 && ready_165))))));
        const bool condition_path_277 = (condition_8 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_137 && (ready_163 && ready_165))))) : condition_path_276);
        const bool condition_path_278 = (condition_7 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_137 && (ready_163 && ready_165))))) : condition_path_277);
        const bool condition_path_279 = (ready_7 ? (condition_10 ? false : (condition_11 && (condition_12 && (ready_137 && (ready_163 && ready_165))))) : condition_path_278);
        const bool condition_path_280 = (condition_3 ? (condition_4 ? condition_path_269 : (ready_6 ? condition_path_272 : condition_path_273)) : (ready_6 ? condition_path_275 : condition_path_279));
        const bool condition_path_281 = (condition_2 ? condition_path_280 : (ready_6 && (condition_11 && (condition_12 && (ready_137 && (ready_163 && ready_165))))));
        const bool condition_path_282 = (ready_2 ? condition_path_260 : (jump ? condition_path_260 : (condition ? condition_path_260 : condition_path_281)));
        bool ready_167{};
        if ((ready ? condition_path_282 : condition_path_260)) {
          ready_167 = std::get<0>(this->compute_shared_computation_34());
        }
        const bool condition_path_283 = (condition_19 ? false : (condition_21 ? false : (condition_38 && (ready_137 && (ready_163 && (ready_165 && ready_167))))));
        const bool condition_path_284 = (condition_16 ? (condition_38 && (ready_137 && (ready_163 && (ready_165 && ready_167)))) : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_283)));
        const bool condition_path_285 = ((opcode==23u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_15 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_284));
        const bool condition_path_286 = (condition_12 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_13 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_285));
        const bool condition_path_287 = (condition_22 && (condition_23 && (condition_38 && (ready_137 && (ready_163 && (ready_165 && ready_167))))));
        const bool condition_path_288 = (condition_21 ? condition_path_287 : (condition_38 && (ready_137 && (ready_163 && (ready_165 && ready_167)))));
        const bool condition_path_289 = ((opcode==35u) ? false : (condition_19 ? ((funct7==1u) ? false : condition_path_287) : condition_path_288));
        const bool condition_path_290 = (condition_16 ? (condition_38 && (ready_137 && (ready_163 && (ready_165 && ready_167)))) : ((opcode==3u) ? false : condition_path_289));
        const bool condition_path_291 = ((opcode==23u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_15 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_290));
        const bool condition_path_292 = (condition_12 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_13 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_291));
        const bool condition_path_293 = (condition_9 ? condition_path_286 : (condition_10 ? condition_path_286 : (condition_11 ? condition_path_286 : condition_path_292)));
        const bool condition_path_294 = (ready_6 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_7 ? condition_path_286 : (condition_8 ? condition_path_286 : condition_path_293)));
        const bool condition_path_295 = (condition_6 ? (ready_6 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_286) : condition_path_294);
        const bool condition_path_296 = (condition_28 ? (condition_29 ? false : (ready_137 && (ready_163 && (ready_165 && ready_167)))) : (ready_137 && (ready_163 && (ready_165 && ready_167))));
        const bool condition_path_297 = (condition_19 ? ((funct7==1u) ? condition_path_296 : (ready_137 && (ready_163 && (ready_165 && ready_167)))) : (ready_137 && (ready_163 && (ready_165 && ready_167))));
        const bool condition_path_298 = ((opcode==3u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : ((opcode==35u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_297));
        const bool condition_path_299 = (condition_15 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_16 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_298));
        const bool condition_path_300 = (condition_13 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : ((opcode==23u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_299));
        const bool condition_path_301 = (condition_8 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_12 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_300));
        const bool condition_path_302 = (condition_5 ? condition_path_295 : (ready_6 ? condition_path_301 : (condition_8 ? condition_path_286 : condition_path_293)));
        const bool condition_path_303 = (condition_29 ? (condition_30 ? false : (ready_137 && (ready_163 && (ready_165 && ready_167)))) : (ready_137 && (ready_163 && (ready_165 && ready_167))));
        const bool condition_path_304 = ((funct7==1u) ? (condition_28 ? condition_path_303 : (ready_137 && (ready_163 && (ready_165 && ready_167)))) : (ready_137 && (ready_163 && (ready_165 && ready_167))));
        const bool condition_path_305 = ((opcode==35u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_19 ? condition_path_304 : (ready_137 && (ready_163 && (ready_165 && ready_167)))));
        const bool condition_path_306 = (condition_16 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : ((opcode==3u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_305));
        const bool condition_path_307 = ((opcode==23u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_15 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_306));
        const bool condition_path_308 = (condition_12 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_13 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_307));
        const bool condition_path_309 = (condition_8 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_9 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_308));
        const bool condition_path_310 = (condition_5 ? (ready_6 ? condition_path_309 : condition_path_293) : (ready_6 ? condition_path_301 : condition_path_293));
        const bool condition_path_311 = (condition_19 ? ((funct7==1u) ? false : (condition_38 && (ready_137 && (ready_163 && (ready_165 && ready_167))))) : (condition_38 && (ready_137 && (ready_163 && (ready_165 && ready_167)))));
        const bool condition_path_312 = (condition_16 ? (condition_38 && (ready_137 && (ready_163 && (ready_165 && ready_167)))) : ((opcode==3u) ? false : ((opcode==35u) ? false : condition_path_311)));
        const bool condition_path_313 = ((opcode==23u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_15 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_312));
        const bool condition_path_314 = (condition_12 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_13 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_313));
        const bool condition_path_315 = (ready_6 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_10 ? condition_path_286 : (condition_11 ? condition_path_286 : condition_path_314)));
        const bool condition_path_316 = (ready_6 ? condition_path_301 : (condition_10 ? condition_path_286 : (condition_11 ? condition_path_286 : condition_path_314)));
        const bool condition_path_317 = (ready_6 ? condition_path_309 : (condition_10 ? condition_path_286 : (condition_11 ? condition_path_286 : condition_path_314)));
        const bool condition_path_318 = (condition_4 ? (condition_5 ? condition_path_315 : condition_path_316) : (condition_5 ? condition_path_317 : condition_path_316));
        const bool condition_path_319 = ((funct7==1u) ? (condition_28 && (ready_137 && (ready_163 && (ready_165 && ready_167)))) : (ready_137 && (ready_163 && (ready_165 && ready_167))));
        const bool condition_path_320 = ((opcode==35u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_19 ? condition_path_319 : (ready_137 && (ready_163 && (ready_165 && ready_167)))));
        const bool condition_path_321 = (condition_16 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : ((opcode==3u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_320));
        const bool condition_path_322 = ((opcode==23u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_15 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_321));
        const bool condition_path_323 = (condition_12 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_13 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_322));
        const bool condition_path_324 = (ready_6 ? (condition_11 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_323) : (condition_11 ? condition_path_286 : condition_path_314));
        const bool condition_path_325 = ((funct7==1u) ? (condition_28 && (condition_29 ? false : (ready_137 && (ready_163 && (ready_165 && ready_167))))) : (ready_137 && (ready_163 && (ready_165 && ready_167))));
        const bool condition_path_326 = ((opcode==35u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_19 ? condition_path_325 : (ready_137 && (ready_163 && (ready_165 && ready_167)))));
        const bool condition_path_327 = (condition_16 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : ((opcode==3u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_326));
        const bool condition_path_328 = ((opcode==23u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_15 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_327));
        const bool condition_path_329 = (condition_12 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_13 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_328));
        const bool condition_path_330 = (condition_11 ? (condition_12 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_300) : condition_path_329);
        const bool condition_path_331 = (condition_8 ? (condition_11 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_323) : condition_path_330);
        const bool condition_path_332 = (condition_5 ? condition_path_324 : (ready_6 ? condition_path_331 : (condition_11 ? condition_path_286 : condition_path_314)));
        const bool condition_path_333 = ((funct7==1u) ? (condition_28 && condition_path_303) : (ready_137 && (ready_163 && (ready_165 && ready_167))));
        const bool condition_path_334 = ((opcode==35u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_19 ? condition_path_333 : (ready_137 && (ready_163 && (ready_165 && ready_167)))));
        const bool condition_path_335 = (condition_16 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : ((opcode==3u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_334));
        const bool condition_path_336 = ((opcode==23u) ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_15 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_335));
        const bool condition_path_337 = (condition_12 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : (condition_13 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_336));
        const bool condition_path_338 = (condition_9 ? (condition_11 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_323) : (condition_11 ? condition_path_308 : condition_path_337));
        const bool condition_path_339 = (condition_8 ? (condition_11 ? (ready_137 && (ready_163 && (ready_165 && ready_167))) : condition_path_323) : condition_path_338);
        const bool condition_path_340 = (condition_5 ? (ready_6 ? condition_path_339 : (condition_11 ? condition_path_286 : condition_path_314)) : (ready_6 ? condition_path_331 : (condition_11 ? condition_path_286 : condition_path_314)));
        const bool condition_path_341 = (condition_2 ? (condition_3 ? (condition_4 ? condition_path_302 : condition_path_310) : condition_path_318) : (condition_4 ? condition_path_332 : condition_path_340));
        const bool condition_path_342 = (condition_10 ? (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167)))) : (condition_11 && (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167))))));
        const bool condition_path_343 = (condition_9 ? (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167)))) : condition_path_342);
        const bool condition_path_344 = (condition_8 ? (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167)))) : condition_path_343);
        const bool condition_path_345 = (condition_7 ? (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167)))) : condition_path_344);
        const bool condition_path_346 = (ready_7 ? condition_path_345 : (condition_7 && (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167))))));
        const bool condition_path_347 = (condition_10 ? false : (condition_11 && (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167))))));
        const bool condition_path_348 = (ready_7 && (condition_7 ? false : (condition_8 ? false : (condition_9 ? false : condition_path_347))));
        const bool condition_path_349 = (condition_6 ? (ready_6 && (ready_7 && (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167)))))) : (ready_6 ? condition_path_346 : condition_path_348));
        const bool condition_path_350 = (condition_7 ? condition_path_344 : (condition_8 && (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167))))));
        const bool condition_path_351 = (ready_7 ? (condition_8 ? false : (condition_9 ? false : condition_path_347)) : (condition_7 && (condition_8 ? false : (condition_9 ? false : condition_path_347))));
        const bool condition_path_352 = (condition_5 ? condition_path_349 : (ready_6 ? (ready_7 ? condition_path_344 : condition_path_350) : condition_path_351));
        const bool condition_path_353 = (condition_8 ? condition_path_343 : (condition_9 && (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167))))));
        const bool condition_path_354 = (condition_7 ? (condition_9 ? false : condition_path_347) : (condition_8 && (condition_9 ? false : condition_path_347)));
        const bool condition_path_355 = (ready_6 ? (ready_7 ? condition_path_343 : (condition_7 ? condition_path_343 : condition_path_353)) : (ready_7 ? (condition_9 ? false : condition_path_347) : condition_path_354));
        const bool condition_path_356 = (condition_9 ? condition_path_342 : (condition_10 && (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167))))));
        const bool condition_path_357 = (ready_7 ? condition_path_342 : (condition_7 ? condition_path_342 : (condition_8 ? condition_path_342 : condition_path_356)));
        const bool condition_path_358 = (condition_7 ? condition_path_347 : (condition_8 ? condition_path_347 : (condition_9 && condition_path_347)));
        const bool condition_path_359 = (condition_3 ? (condition_4 ? condition_path_352 : condition_path_355) : (ready_6 ? condition_path_357 : (ready_7 ? condition_path_347 : condition_path_358)));
        const bool condition_path_360 = (ready_6 && (condition_11 && (condition_12 && (ready_137 && (ready_163 && (ready_165 && ready_167))))));
        const bool condition_path_361 = (jump ? condition_path_341 : (condition ? condition_path_341 : (condition_2 ? condition_path_359 : condition_path_360)));
        bool ready_169{};
        if ((ready ? (ready_2 ? condition_path_341 : condition_path_361) : condition_path_341)) {
          ready_169 = std::get<0>(this->compute_shared_computation_35());
        }
        const bool ready_170 = (((ready_164 && ready_165) && ready_167) && ready_169);
        compute_shared_computation_37_cache.emplace(std::tuple<bool>{ready_170});
      }
      return *compute_shared_computation_37_cache;
    }
    const std::tuple<Decoded,bool> &compute_id_ex_view_4() {
      if (!compute_id_ex_view_4_cache) {
        const Decoded id_ex_value = top.cpu.id_ex.read();
        compute_id_ex_view_4_cache.emplace(std::tuple<Decoded,bool>{id_ex_value,true});
      }
      return *compute_id_ex_view_4_cache;
    }
    const std::tuple<bool> &compute_shared_computation_38() {
      if (!compute_shared_computation_38_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_4());
        const bool ready = id_ex_value.valid;
        compute_shared_computation_38_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_38_cache;
    }
    const std::tuple<Decoded,bool> &compute_id_ex_view_5() {
      if (!compute_id_ex_view_5_cache) {
        const Decoded id_ex_value = top.cpu.id_ex.read();
        compute_id_ex_view_5_cache.emplace(std::tuple<Decoded,bool>{id_ex_value,true});
      }
      return *compute_id_ex_view_5_cache;
    }
    const std::tuple<bool> &compute_shared_computation_39() {
      if (!compute_shared_computation_39_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_5());
        const bool condition = (id_ex_value.instruction.system==1u);
        compute_shared_computation_39_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_39_cache;
    }
    const std::tuple<bool> &compute_shared_computation_40() {
      if (!compute_shared_computation_40_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_38());
        const bool condition = std::get<0>(this->compute_shared_computation_39());
        compute_shared_computation_40_cache.emplace(std::tuple<bool>{(ready && condition)});
      }
      return *compute_shared_computation_40_cache;
    }
    const std::tuple<Executed,bool> &compute_ex_mem_view_2() {
      if (!compute_ex_mem_view_2_cache) {
        const Executed ex_mem_value = top.cpu.ex_mem.read();
        compute_ex_mem_view_2_cache.emplace(std::tuple<Executed,bool>{ex_mem_value,true});
      }
      return *compute_ex_mem_view_2_cache;
    }
    const std::tuple<bool> &compute_shared_computation_41() {
      if (!compute_shared_computation_41_cache) {
        const Executed ex_mem_value = std::get<0>(this->compute_ex_mem_view_2());
        const bool ready = ex_mem_value.writes;
        compute_shared_computation_41_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_41_cache;
    }
    const std::tuple<bool> &compute_shared_computation_42() {
      if (!compute_shared_computation_42_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_41());
        const bool ready_2 = (!ready);
        compute_shared_computation_42_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_42_cache;
    }
    const std::tuple<Completed,bool> &compute_mem_wb_view_2() {
      if (!compute_mem_wb_view_2_cache) {
        const Completed mem_wb_value = top.cpu.mem_wb.read();
        compute_mem_wb_view_2_cache.emplace(std::tuple<Completed,bool>{mem_wb_value,true});
      }
      return *compute_mem_wb_view_2_cache;
    }
    const std::tuple<bool> &compute_shared_computation_43() {
      if (!compute_shared_computation_43_cache) {
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view_2());
        const bool ready = mem_wb_value.writes;
        compute_shared_computation_43_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_43_cache;
    }
    const std::tuple<bool> &compute_wait() {
      if (!compute_wait_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_40());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_41());
        const bool ready_3 = std::get<0>(this->compute_shared_computation_43());
        const bool wait = (ready && (ready_2 || ready_3));
        compute_wait_cache.emplace(std::tuple<bool>{wait});
      }
      return *compute_wait_cache;
    }
    const std::tuple<Decoded,bool> &compute_id_ex_view_6() {
      if (!compute_id_ex_view_6_cache) {
        const Decoded id_ex_value = top.cpu.id_ex.read();
        compute_id_ex_view_6_cache.emplace(std::tuple<Decoded,bool>{id_ex_value,true});
      }
      return *compute_id_ex_view_6_cache;
    }
    const std::tuple<bool,bool> &compute_halted_view() {
      if (!compute_halted_view_cache) {
        const bool halted_value = top.cpu.halted.read();
        compute_halted_view_cache.emplace(std::tuple<bool,bool>{halted_value,true});
      }
      return *compute_halted_view_cache;
    }
    const std::tuple<bool> &compute_shared_computation_44() {
      if (!compute_shared_computation_44_cache) {
        const bool halted_value = std::get<0>(this->compute_halted_view());
        const bool ready = (!halted_value);
        compute_shared_computation_44_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_44_cache;
    }
    const std::tuple<Decoded,bool> &compute_id_ex_view_7() {
      if (!compute_id_ex_view_7_cache) {
        const Decoded id_ex_value = top.cpu.id_ex.read();
        compute_id_ex_view_7_cache.emplace(std::tuple<Decoded,bool>{id_ex_value,true});
      }
      return *compute_id_ex_view_7_cache;
    }
    const std::tuple<bool> &compute_shared_computation_45() {
      if (!compute_shared_computation_45_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_7());
        const bool ready = id_ex_value.valid;
        compute_shared_computation_45_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_45_cache;
    }
    const std::tuple<bool> &compute_shared_computation_46() {
      if (!compute_shared_computation_46_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_6());
        const Instruction instruction_ex = id_ex_value.instruction;
        const bool ready = instruction_ex.illegal;
        compute_shared_computation_46_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_46_cache;
    }
    const std::tuple<uint32_t> &compute_shared_computation_47() {
      if (!compute_shared_computation_47_cache) {
        const Decoded id_ex_value = std::get<0>(this->compute_id_ex_view_6());
        const Instruction instruction_ex = id_ex_value.instruction;
        const uint32_t system = instruction_ex.system;
        compute_shared_computation_47_cache.emplace(std::tuple<uint32_t>{system});
      }
      return *compute_shared_computation_47_cache;
    }
    const std::tuple<bool> &compute_shared_computation_48() {
      if (!compute_shared_computation_48_cache) {
        const uint32_t system = std::get<0>(this->compute_shared_computation_47());
        const bool condition = (system==2u);
        compute_shared_computation_48_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_48_cache;
    }
    const std::tuple<bool> &compute_shared_computation_49() {
      if (!compute_shared_computation_49_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_46());
        const bool condition = std::get<0>(this->compute_shared_computation_48());
        const bool ready_2 = (ready || condition);
        compute_shared_computation_49_cache.emplace(std::tuple<bool>{(!ready_2)});
      }
      return *compute_shared_computation_49_cache;
    }
    const std::tuple<bool> &compute_shared_computation_50() {
      if (!compute_shared_computation_50_cache) {
        const uint32_t system = std::get<0>(this->compute_shared_computation_47());
        const bool condition = (system==1u);
        compute_shared_computation_50_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_50_cache;
    }
    const std::tuple<bool> &compute_shared_computation_51() {
      if (!compute_shared_computation_51_cache) {
        const bool wait = std::get<0>(this->compute_wait());
        const bool ready = (!wait);
        compute_shared_computation_51_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_51_cache;
    }
    const std::tuple<bool> &compute_shared_computation_52() {
      if (!compute_shared_computation_52_cache) {
        const bool condition = std::get<0>(this->compute_shared_computation_50());
        const bool ready = std::get<0>(this->compute_shared_computation_51());
        const bool ready_2 = (condition && ready);
        compute_shared_computation_52_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_52_cache;
    }
    const std::tuple<bool> &compute_shared_computation_53() {
      if (!compute_shared_computation_53_cache) {
        const uint32_t system = std::get<0>(this->compute_shared_computation_47());
        const bool condition = (system!=1u);
        compute_shared_computation_53_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_53_cache;
    }
    const std::tuple<Completed,bool> &compute_mem_wb_view_3() {
      if (!compute_mem_wb_view_3_cache) {
        const Completed mem_wb_value = top.cpu.mem_wb.read();
        compute_mem_wb_view_3_cache.emplace(std::tuple<Completed,bool>{mem_wb_value,true});
      }
      return *compute_mem_wb_view_3_cache;
    }
    const std::tuple<bool> &compute_shared_computation_54() {
      if (!compute_shared_computation_54_cache) {
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view_3());
        const bool ready_2 = (mem_wb_value.writes && (mem_wb_value.rd==17u));
        compute_shared_computation_54_cache.emplace(std::tuple<bool>{(!ready_2)});
      }
      return *compute_shared_computation_54_cache;
    }
    const std::tuple<uint32_t,bool> &compute_value0_view() {
      if (!compute_value0_view_cache) {
        const uint32_t registers_value = ((uint64_t(17ULL) < 32) ? top.cpu.rf.registers.at(uint64_t(17ULL)).read() : uint32_t{});
        const bool registers_ready = ((uint64_t(17ULL) < 32) ? true : bool{});
        compute_value0_view_cache.emplace(std::tuple<uint32_t,bool>{registers_value,registers_ready});
      }
      return *compute_value0_view_cache;
    }
    const std::tuple<uint32_t> &compute_shared_computation_55() {
      if (!compute_shared_computation_55_cache) {
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view_3());
        const bool ready_2 = (mem_wb_value.writes && (mem_wb_value.rd==17u));
        const uint32_t registers_value = std::get<0>(this->compute_value0_view());
        compute_shared_computation_55_cache.emplace(std::tuple<uint32_t>{(ready_2 ? mem_wb_value.value : registers_value)});
      }
      return *compute_shared_computation_55_cache;
    }
    const std::tuple<bool> &compute_shared_computation_56() {
      if (!compute_shared_computation_56_cache) {
        const uint32_t computed_value = std::get<0>(this->compute_shared_computation_55());
        const bool condition = (computed_value==10u);
        compute_shared_computation_56_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_56_cache;
    }
    const std::tuple<Completed,bool> &compute_mem_wb_view_4() {
      if (!compute_mem_wb_view_4_cache) {
        const Completed mem_wb_value = top.cpu.mem_wb.read();
        compute_mem_wb_view_4_cache.emplace(std::tuple<Completed,bool>{mem_wb_value,true});
      }
      return *compute_mem_wb_view_4_cache;
    }
    const std::tuple<bool> &compute_shared_computation_57() {
      if (!compute_shared_computation_57_cache) {
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view_4());
        const bool ready_2 = (mem_wb_value.writes && (mem_wb_value.rd==17u));
        compute_shared_computation_57_cache.emplace(std::tuple<bool>{(!ready_2)});
      }
      return *compute_shared_computation_57_cache;
    }
    const std::tuple<uint32_t,bool> &compute_value0_view_2() {
      if (!compute_value0_view_2_cache) {
        const uint32_t registers_value = ((uint64_t(17ULL) < 32) ? top.cpu.rf.registers.at(uint64_t(17ULL)).read() : uint32_t{});
        const bool registers_ready = ((uint64_t(17ULL) < 32) ? true : bool{});
        compute_value0_view_2_cache.emplace(std::tuple<uint32_t,bool>{registers_value,registers_ready});
      }
      return *compute_value0_view_2_cache;
    }
    const std::tuple<uint32_t> &compute_shared_computation_58() {
      if (!compute_shared_computation_58_cache) {
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view_4());
        const bool ready_2 = (mem_wb_value.writes && (mem_wb_value.rd==17u));
        const uint32_t registers_value = std::get<0>(this->compute_value0_view_2());
        compute_shared_computation_58_cache.emplace(std::tuple<uint32_t>{(ready_2 ? mem_wb_value.value : registers_value)});
      }
      return *compute_shared_computation_58_cache;
    }
    const std::tuple<bool> &compute_shared_computation_59() {
      if (!compute_shared_computation_59_cache) {
        const uint32_t computed_value = std::get<0>(this->compute_shared_computation_58());
        const bool condition = (computed_value==93u);
        compute_shared_computation_59_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_59_cache;
    }
    const std::tuple<bool> &compute_shared_computation_60() {
      if (!compute_shared_computation_60_cache) {
        const bool condition = std::get<0>(this->compute_shared_computation_56());
        const bool condition_2 = std::get<0>(this->compute_shared_computation_59());
        const bool ready = (condition || condition_2);
        compute_shared_computation_60_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_60_cache;
    }
    const std::tuple<Completed,bool> &compute_mem_wb_view_5() {
      if (!compute_mem_wb_view_5_cache) {
        const Completed mem_wb_value = top.cpu.mem_wb.read();
        compute_mem_wb_view_5_cache.emplace(std::tuple<Completed,bool>{mem_wb_value,true});
      }
      return *compute_mem_wb_view_5_cache;
    }
    const std::tuple<bool> &compute_shared_computation_61() {
      if (!compute_shared_computation_61_cache) {
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view_5());
        const bool ready_2 = (mem_wb_value.writes && (mem_wb_value.rd==17u));
        compute_shared_computation_61_cache.emplace(std::tuple<bool>{(!ready_2)});
      }
      return *compute_shared_computation_61_cache;
    }
    const std::tuple<uint32_t,bool> &compute_value0_view_3() {
      if (!compute_value0_view_3_cache) {
        const uint32_t registers_value = ((uint64_t(17ULL) < 32) ? top.cpu.rf.registers.at(uint64_t(17ULL)).read() : uint32_t{});
        const bool registers_ready = ((uint64_t(17ULL) < 32) ? true : bool{});
        compute_value0_view_3_cache.emplace(std::tuple<uint32_t,bool>{registers_value,registers_ready});
      }
      return *compute_value0_view_3_cache;
    }
    const std::tuple<uint32_t> &compute_number() {
      if (!compute_number_cache) {
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view_5());
        const bool ready_2 = (mem_wb_value.writes && (mem_wb_value.rd==17u));
        const uint32_t registers_value = std::get<0>(this->compute_value0_view_3());
        const uint32_t number = (ready_2 ? mem_wb_value.value : registers_value);
        compute_number_cache.emplace(std::tuple<uint32_t>{number});
      }
      return *compute_number_cache;
    }
    const std::tuple<bool> &compute_shared_computation_62() {
      if (!compute_shared_computation_62_cache) {
        const uint32_t number = std::get<0>(this->compute_number());
        const bool condition = (number==1u);
        compute_shared_computation_62_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_62_cache;
    }
    const std::tuple<bool> &compute_shared_computation_63() {
      if (!compute_shared_computation_63_cache) {
        const uint32_t number = std::get<0>(this->compute_number());
        const bool condition = (number==4u);
        compute_shared_computation_63_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_63_cache;
    }
    const std::tuple<bool> &compute_shared_computation_64() {
      if (!compute_shared_computation_64_cache) {
        const uint32_t number = std::get<0>(this->compute_number());
        const bool condition = (number==11u);
        compute_shared_computation_64_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_64_cache;
    }
    const std::tuple<bool> &compute_shared_computation_65() {
      if (!compute_shared_computation_65_cache) {
        const uint32_t number = std::get<0>(this->compute_number());
        const bool condition = (number==10u);
        compute_shared_computation_65_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_65_cache;
    }
    const std::tuple<bool> &compute_shared_computation_66() {
      if (!compute_shared_computation_66_cache) {
        const uint32_t number = std::get<0>(this->compute_number());
        const bool condition = (number==93u);
        compute_shared_computation_66_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_66_cache;
    }
    const std::tuple<bool> &compute_shared_computation_67() {
      if (!compute_shared_computation_67_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_45());
        const bool ready_2 = (!ready);
        compute_shared_computation_67_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_67_cache;
    }
    const std::tuple<bool> &compute_stopping() {
      if (!compute_stopping_cache) {
        const bool halted_value = std::get<0>(this->compute_halted_view());
        const bool ready = std::get<0>(this->compute_shared_computation_45());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_46());
        const bool condition = std::get<0>(this->compute_shared_computation_48());
        const bool ready_3 = (ready_2 || condition);
        const bool ready_4 = std::get<0>(this->compute_shared_computation_52());
        const bool ready_5 = std::get<0>(this->compute_shared_computation_60());
        const bool condition_2 = std::get<0>(this->compute_shared_computation_62());
        const bool condition_3 = std::get<0>(this->compute_shared_computation_63());
        const bool ready_6 = (condition_2 || condition_3);
        const bool condition_4 = std::get<0>(this->compute_shared_computation_64());
        const bool ready_7 = (ready_6 || condition_4);
        const bool condition_5 = std::get<0>(this->compute_shared_computation_65());
        const bool ready_8 = (ready_7 || condition_5);
        const bool condition_6 = std::get<0>(this->compute_shared_computation_66());
        const bool stopping = (halted_value || (ready && (ready_3 || (ready_4 && (ready_5 || (!(ready_8 || condition_6)))))));
        compute_stopping_cache.emplace(std::tuple<bool>{stopping});
      }
      return *compute_stopping_cache;
    }
    const std::tuple<bool> &compute_shared_computation_68() {
      if (!compute_shared_computation_68_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_2());
        const bool condition = std::get<0>(this->compute_shared_computation_4());
        const bool ready_4 = ((ready && ready_2) && condition);
        const bool ready_5 = (!ready_4);
        bool instruction_ready{};
        if ((ready && (ready_2 && condition))) {
          instruction_ready = std::get<1>(this->compute_instruction_view());
        }
        const uint32_t rd = std::get<0>(this->compute_shared_computation_3());
        Instruction instruction_value{};
        if ((ready && (ready_2 && condition))) {
          instruction_value = std::get<0>(this->compute_instruction_view());
        }
        const uint32_t rs1 = instruction_value.rs1;
        const bool condition_2 = (rd==rs1);
        const bool ready_10 = (ready_5 || instruction_ready);
        bool condition_3{};
        if ((ready && (ready_2 && (condition && (instruction_ready && (!condition_2)))))) {
          condition_3 = std::get<0>(this->compute_shared_computation_6());
        }
        const bool ready_17 = (((ready_4 && ((condition_2 || condition_3) && instruction_ready)) || ready_5) && ready_10);
        const bool ready_18 = (((ready_5 || (instruction_ready && (condition_2 || instruction_ready))) && ready_10) && ready_17);
        const bool condition_path = (ready ? (ready_2 ? (condition ? (instruction_ready && (condition_2 || condition_3)) : true) : true) : true);
        bool ready_19{};
        if (condition_path) {
          ready_19 = std::get<0>(this->compute_shared_computation_36());
        }
        const bool condition_path_2 = (ready_2 ? (condition ? (instruction_ready && (condition_2 ? ready_19 : (condition_3 && ready_19))) : ready_19) : ready_19);
        bool ready_21{};
        if ((ready ? condition_path_2 : ready_19)) {
          ready_21 = std::get<0>(this->compute_shared_computation_37());
        }
        const bool halted_value = std::get<0>(this->compute_halted_view());
        const bool ready_25 = std::get<0>(this->compute_shared_computation_67());
        const bool ready_26 = std::get<0>(this->compute_shared_computation_46());
        const bool condition_4 = std::get<0>(this->compute_shared_computation_48());
        const bool ready_27 = (ready_26 || condition_4);
        const bool condition_5 = std::get<0>(this->compute_shared_computation_53());
        const bool ready_29 = std::get<0>(this->compute_shared_computation_52());
        const bool ready_30 = (!ready_29);
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view_3());
        const bool ready_32 = (mem_wb_value.writes && (mem_wb_value.rd==17u));
        const bool ready_33 = std::get<0>(this->compute_shared_computation_54());
        const bool condition_path_3 = (ready_21 && (halted_value ? false : (ready_25 ? false : (ready_26 ? false : (condition_4 ? false : ready_29)))));
        const bool condition_path_4 = (instruction_ready && (condition_2 ? (ready_19 && condition_path_3) : (condition_3 && (ready_19 && condition_path_3))));
        const bool condition_path_5 = (ready_2 ? (condition ? condition_path_4 : (ready_19 && condition_path_3)) : (ready_19 && condition_path_3));
        bool ready_35{};
        if ((ready ? condition_path_5 : (ready_19 && condition_path_3))) {
          ready_35 = acir_sim::check((!(ready_32 ? ready_33 : false)));
        }
        const bool condition_7 = std::get<0>(this->compute_shared_computation_56());
        const Completed mem_wb_value_2 = std::get<0>(this->compute_mem_wb_view_4());
        const bool ready_39 = (mem_wb_value_2.writes && (mem_wb_value_2.rd==17u));
        const bool ready_40 = std::get<0>(this->compute_shared_computation_57());
        const bool condition_path_6 = (ready_25 ? false : (ready_26 ? false : (condition_4 ? false : (ready_29 && (ready_35 && (!condition_7))))));
        const bool condition_path_7 = (condition_2 ? (ready_19 && (ready_21 && (halted_value ? false : condition_path_6))) : (condition_3 && (ready_19 && (ready_21 && (halted_value ? false : condition_path_6)))));
        const bool condition_path_8 = (condition ? (instruction_ready && condition_path_7) : (ready_19 && (ready_21 && (halted_value ? false : condition_path_6))));
        const bool condition_path_9 = (ready ? (ready_2 ? condition_path_8 : (ready_19 && (ready_21 && (halted_value ? false : condition_path_6)))) : (ready_19 && (ready_21 && (halted_value ? false : condition_path_6))));
        bool ready_42{};
        if (condition_path_9) {
          ready_42 = acir_sim::check((!(ready_39 ? ready_40 : false)));
        }
        const bool ready_46 = (ready_35 && (condition_7 || ready_42));
        const bool ready_47 = std::get<0>(this->compute_shared_computation_60());
        const Completed mem_wb_value_3 = std::get<0>(this->compute_mem_wb_view_5());
        const bool ready_49 = (mem_wb_value_3.writes && (mem_wb_value_3.rd==17u));
        const bool ready_50 = std::get<0>(this->compute_shared_computation_61());
        const bool condition_path_10 = (condition_4 ? false : (ready_29 && (ready_35 && (condition_7 ? (!ready_47) : (ready_42 && (!ready_47))))));
        const bool condition_path_11 = (ready_19 && (ready_21 && (halted_value ? false : (ready_25 ? false : (ready_26 ? false : condition_path_10)))));
        const bool condition_path_12 = (condition ? (instruction_ready && (condition_2 ? condition_path_11 : (condition_3 && condition_path_11))) : condition_path_11);
        bool ready_52{};
        if ((ready ? (ready_2 ? condition_path_12 : condition_path_11) : condition_path_11)) {
          ready_52 = acir_sim::check((!(ready_49 ? ready_50 : false)));
        }
        const bool condition_10 = std::get<0>(this->compute_shared_computation_62());
        const bool condition_11 = std::get<0>(this->compute_shared_computation_63());
        const bool ready_57 = (condition_10 || condition_11);
        const bool condition_12 = std::get<0>(this->compute_shared_computation_64());
        const bool ready_60 = (ready_57 || condition_12);
        const bool condition_13 = std::get<0>(this->compute_shared_computation_65());
        const bool ready_63 = (ready_60 || condition_13);
        const bool ready_65 = ((((ready_52 && (condition_10 || ready_52)) && (ready_57 || ready_52)) && (ready_60 || ready_52)) && (ready_63 || ready_52));
        const bool ready_74 = (ready_27 || (ready_30 || ready_46));
        const bool ready_75 = ((ready_27 || ((ready_30 || (ready_46 && (ready_47 || ready_65))) && (ready_30 || ready_46))) && ready_74);
        const bool ready_79 = (halted_value || ((ready_25 || ready_75) && (ready_25 || ready_74)));
        const bool ready_80 = (((ready_18 && ready_19) && ready_21) && ready_79);
        compute_shared_computation_68_cache.emplace(std::tuple<bool>{ready_80});
      }
      return *compute_shared_computation_68_cache;
    }
    const std::tuple<bool> &compute_shared_computation_69() {
      if (!compute_shared_computation_69_cache) {
        const bool ready_2 = std::get<0>(this->compute_shared_computation_35());
        compute_shared_computation_69_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_69_cache;
    }
    const std::tuple<bool> &compute_shared_computation_70() {
      if (!compute_shared_computation_70_cache) {
        const bool halted_value = std::get<0>(this->compute_halted_view());
        const bool ready = std::get<0>(this->compute_shared_computation_44());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_45());
        const bool ready_3 = std::get<0>(this->compute_shared_computation_46());
        const bool condition = std::get<0>(this->compute_shared_computation_48());
        const bool ready_4 = (ready_3 || condition);
        const bool ready_5 = std::get<0>(this->compute_shared_computation_49());
        const bool ready_6 = std::get<0>(this->compute_shared_computation_52());
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view_3());
        const bool ready_8 = (mem_wb_value.writes && (mem_wb_value.rd==17u));
        const bool ready_9 = std::get<0>(this->compute_shared_computation_54());
        const bool condition_path = (halted_value ? false : (ready && (ready_2 && (ready_3 ? false : (condition ? false : (ready_5 && ready_6))))));
        bool ready_11{};
        if (condition_path) {
          ready_11 = acir_sim::check((!(ready_8 ? ready_9 : false)));
        }
        const bool condition_3 = std::get<0>(this->compute_shared_computation_56());
        const Completed mem_wb_value_2 = std::get<0>(this->compute_mem_wb_view_4());
        const bool ready_15 = (mem_wb_value_2.writes && (mem_wb_value_2.rd==17u));
        const bool ready_16 = std::get<0>(this->compute_shared_computation_57());
        const bool condition_path_2 = (ready_2 && (ready_3 ? false : (condition ? false : (ready_5 && (ready_6 && (ready_11 && (!condition_3)))))));
        bool ready_18{};
        if ((halted_value ? false : (ready && condition_path_2))) {
          ready_18 = acir_sim::check((!(ready_15 ? ready_16 : false)));
        }
        const bool ready_22 = (ready_11 && (condition_3 || ready_18));
        const bool ready_24 = (!ready_6);
        const bool condition_5 = std::get<0>(this->compute_shared_computation_53());
        const bool ready_32 = std::get<0>(this->compute_shared_computation_67());
        const bool ready_35 = (halted_value || (ready && ((ready_2 && (ready_4 || (ready_5 && ((ready_6 && ready_22) || ready_24)))) || ready_32)));
        const bool condition_path_3 = (ready_5 ? (ready_6 ? (ready_11 ? (condition_3 || (ready_18 || ready_32)) : ready_32) : true) : ready_32);
        bool ready_36{};
        if ((halted_value || (ready && (ready_2 ? (ready_3 || (condition || condition_path_3)) : ready_32)))) {
          ready_36 = std::get<0>(this->compute_shared_computation_35());
        }
        const bool ready_37 = (ready_35 && ready_36);
        compute_shared_computation_70_cache.emplace(std::tuple<bool>{ready_37});
      }
      return *compute_shared_computation_70_cache;
    }
    const std::tuple<bool> &compute_shared_computation_71() {
      if (!compute_shared_computation_71_cache) {
        const bool halted_value = std::get<0>(this->compute_halted_view());
        const bool ready = std::get<0>(this->compute_shared_computation_67());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_46());
        const bool condition = std::get<0>(this->compute_shared_computation_48());
        const bool ready_3 = (ready_2 || condition);
        const bool condition_2 = std::get<0>(this->compute_shared_computation_53());
        const bool ready_6 = std::get<0>(this->compute_shared_computation_52());
        const bool ready_7 = (!ready_6);
        const Completed mem_wb_value = std::get<0>(this->compute_mem_wb_view_3());
        const bool ready_9 = (mem_wb_value.writes && (mem_wb_value.rd==17u));
        const bool ready_10 = std::get<0>(this->compute_shared_computation_54());
        bool ready_12{};
        if ((halted_value ? false : (ready ? false : (ready_2 ? false : (condition ? false : ready_6))))) {
          ready_12 = acir_sim::check((!(ready_9 ? ready_10 : false)));
        }
        const bool condition_4 = std::get<0>(this->compute_shared_computation_56());
        const Completed mem_wb_value_2 = std::get<0>(this->compute_mem_wb_view_4());
        const bool ready_16 = (mem_wb_value_2.writes && (mem_wb_value_2.rd==17u));
        const bool ready_17 = std::get<0>(this->compute_shared_computation_57());
        const bool condition_path = (ready ? false : (ready_2 ? false : (condition ? false : (ready_6 && (ready_12 && (!condition_4))))));
        bool ready_19{};
        if ((halted_value ? false : condition_path)) {
          ready_19 = acir_sim::check((!(ready_16 ? ready_17 : false)));
        }
        const bool ready_23 = (ready_12 && (condition_4 || ready_19));
        const bool ready_24 = std::get<0>(this->compute_shared_computation_60());
        const Completed mem_wb_value_3 = std::get<0>(this->compute_mem_wb_view_5());
        const bool ready_26 = (mem_wb_value_3.writes && (mem_wb_value_3.rd==17u));
        const bool ready_27 = std::get<0>(this->compute_shared_computation_61());
        const bool condition_path_2 = (condition ? false : (ready_6 && (ready_12 && (condition_4 ? (!ready_24) : (ready_19 && (!ready_24))))));
        bool ready_29{};
        if ((halted_value ? false : (ready ? false : (ready_2 ? false : condition_path_2)))) {
          ready_29 = acir_sim::check((!(ready_26 ? ready_27 : false)));
        }
        const bool condition_7 = std::get<0>(this->compute_shared_computation_62());
        const bool condition_8 = std::get<0>(this->compute_shared_computation_63());
        const bool ready_34 = (condition_7 || condition_8);
        const bool condition_9 = std::get<0>(this->compute_shared_computation_64());
        const bool ready_37 = (ready_34 || condition_9);
        const bool condition_10 = std::get<0>(this->compute_shared_computation_65());
        const bool ready_40 = (ready_37 || condition_10);
        const bool ready_42 = ((((ready_29 && (condition_7 || ready_29)) && (ready_34 || ready_29)) && (ready_37 || ready_29)) && (ready_40 || ready_29));
        const bool ready_51 = (ready_3 || (ready_7 || ready_23));
        const bool ready_52 = ((ready_3 || ((ready_7 || (ready_23 && (ready_24 || ready_42))) && (ready_7 || ready_23))) && ready_51);
        const bool ready_56 = (halted_value || ((ready || ready_52) && (ready || ready_51)));
        const bool condition_path_3 = (ready_6 ? (ready_12 && (condition_4 ? (ready_24 || ready_29) : (ready_19 && (ready_24 || ready_29)))) : true);
        bool ready_57{};
        if ((halted_value || (ready || (ready_2 || (condition || condition_path_3))))) {
          ready_57 = std::get<0>(this->compute_shared_computation_35());
        }
        const bool condition_path_4 = (condition_4 ? (ready_24 ? ready_57 : (ready_29 && ready_57)) : (ready_19 && (ready_24 ? ready_57 : (ready_29 && ready_57))));
        const bool condition_path_5 = (ready_2 ? ready_57 : (condition ? ready_57 : (ready_6 ? (ready_12 && condition_path_4) : ready_57)));
        bool ready_59{};
        if ((halted_value ? ready_57 : (ready ? ready_57 : condition_path_5))) {
          ready_59 = std::get<0>(this->compute_shared_computation_70());
        }
        const bool ready_60 = ((ready_56 && ready_57) && ready_59);
        compute_shared_computation_71_cache.emplace(std::tuple<bool>{ready_60});
      }
      return *compute_shared_computation_71_cache;
    }
    const std::tuple<bool> &compute_shared_computation_72() {
      if (!compute_shared_computation_72_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_2());
        const bool condition = std::get<0>(this->compute_shared_computation_4());
        const bool ready_4 = ((ready && ready_2) && condition);
        const uint32_t rd = std::get<0>(this->compute_shared_computation_3());
        Instruction instruction_value{};
        if ((ready && (ready_2 && condition))) {
          instruction_value = std::get<0>(this->compute_instruction_view());
        }
        const uint32_t rs1 = instruction_value.rs1;
        const bool condition_2 = (rd==rs1);
        bool condition_3{};
        if ((ready && (ready_2 && (condition && (!condition_2))))) {
          condition_3 = std::get<0>(this->compute_shared_computation_6());
        }
        bool instruction_ready{};
        if ((ready && (ready_2 && condition))) {
          instruction_ready = std::get<1>(this->compute_instruction_view());
        }
        const bool ready_9 = (!ready_4);
        const bool ready_11 = (ready_9 || instruction_ready);
        const bool ready_12 = (((ready_4 && ((condition_2 || condition_3) && instruction_ready)) || ready_9) && ready_11);
        const bool condition_path = (ready_2 ? (condition ? (condition_2 ? instruction_ready : (condition_3 && instruction_ready)) : true) : true);
        bool ready_13{};
        if ((ready ? condition_path : true)) {
          ready_13 = std::get<0>(this->compute_shared_computation_35());
        }
        const bool ready_14 = (ready_12 && ready_13);
        compute_shared_computation_72_cache.emplace(std::tuple<bool>{ready_14});
      }
      return *compute_shared_computation_72_cache;
    }
    const std::tuple<bool> &compute_shared_computation_73() {
      if (!compute_shared_computation_73_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation());
        const bool ready_2 = std::get<0>(this->compute_shared_computation_2());
        const bool condition = std::get<0>(this->compute_shared_computation_4());
        const bool ready_4 = ((ready && ready_2) && condition);
        const bool ready_5 = (!ready_4);
        bool instruction_ready{};
        if ((ready && (ready_2 && condition))) {
          instruction_ready = std::get<1>(this->compute_instruction_view());
        }
        const uint32_t rd = std::get<0>(this->compute_shared_computation_3());
        Instruction instruction_value{};
        if ((ready && (ready_2 && condition))) {
          instruction_value = std::get<0>(this->compute_instruction_view());
        }
        const uint32_t rs1 = instruction_value.rs1;
        const bool condition_2 = (rd==rs1);
        const bool ready_10 = (ready_5 || instruction_ready);
        bool condition_3{};
        if ((ready && (ready_2 && (condition && (instruction_ready && (!condition_2)))))) {
          condition_3 = std::get<0>(this->compute_shared_computation_6());
        }
        const bool ready_17 = (((ready_4 && ((condition_2 || condition_3) && instruction_ready)) || ready_5) && ready_10);
        const bool ready_18 = (((ready_5 || (instruction_ready && (condition_2 || instruction_ready))) && ready_10) && ready_17);
        const bool condition_path = (ready ? (ready_2 ? (condition ? (instruction_ready && (condition_2 || condition_3)) : true) : true) : true);
        bool ready_19{};
        if (condition_path) {
          ready_19 = std::get<0>(this->compute_shared_computation_35());
        }
        const bool condition_path_2 = (ready_2 ? (condition ? (instruction_ready && (condition_2 ? ready_19 : (condition_3 && ready_19))) : ready_19) : ready_19);
        bool ready_21{};
        if ((ready ? condition_path_2 : ready_19)) {
          ready_21 = std::get<0>(this->compute_shared_computation_72());
        }
        const bool ready_22 = ((ready_18 && ready_19) && ready_21);
        compute_shared_computation_73_cache.emplace(std::tuple<bool>{ready_22});
      }
      return *compute_shared_computation_73_cache;
    }
    const std::tuple<bool> &compute_shared_computation_74() {
      if (!compute_shared_computation_74_cache) {
        const bool ready = std::get<0>(this->compute_shared_computation_43());
        const bool ready_2 = (!ready);
        compute_shared_computation_74_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_74_cache;
    }
  };
} // namespace acir_generated
