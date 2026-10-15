#pragma once
#include "runtime.hpp"
#include <set>
namespace acir_generated {
  using namespace acir_sim;
  using acir_sim::json;
  // ACPy payload src_array_arbiter_1242b928.Entry
  struct Entry {
    bool valid{};
    bool ready{};
    uint32_t value{};
    bool operator==(const Entry&) const = default;
  };
  inline void json(std::ostream &o,const Entry &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.ready);
    o<<','; json(o,v.value);
    o<<']'; }
  class Model {
  public:
    struct ArrayArbiter {
      struct AgeMatrix {
        Cell<std::array<std::array<bool,3>,3>> older{};
        Cell<std::array<bool,3>> used{};
      } age;
      struct slots {
        struct Slot {
          Cell<Entry> state{};
        } value0;
        struct Slot_2 {
          Cell<Entry> state{};
        } value1;
        struct Slot_3 {
          Cell<Entry> state{};
          // Rule top.slots.2.wake#4
          template<class Context> void try_wake(Context &ctx) {
            const uint32_t cycle_value = ctx.top.cycle.read();
            const bool condition = (cycle_value==5u);
            if (!(condition)) return;
            // Prepare every effect before publishing state operations.
            const Entry state_value = ctx.top.slots.value2.state.read();
            const Entry entry = state_value;
            const bool ready = entry.valid;
            const bool write_fire = ready;
            Entry write_value{};
            if (write_fire) {
              const Entry entry_2 = Entry{ready,true,entry.value};
              write_value = entry_2;
            }
            if (write_fire) { ctx.top.slots.value2.state.write(write_value); }
            ctx.fired_top_slots_2_wake=true;
          }
        } value2;
      } slots;
      Cell<uint32_t> cycle{};
      Cell<uint32_t> issued{};
      Cell<uint32_t> last{};
      Queue<uint32_t,1> output{};
      Cell<uint32_t> retired{};
      // Rule top.allocate#0
      template<class Context> void try_allocate(Context &ctx) {
        const bool condition = std::get<0>(ctx.compute_shared_computation());
        const uint32_t cycle_value = std::get<0>(ctx.compute_cycle_view_2());
        const bool condition_2 = (cycle_value!=1u);
        const uint32_t cycle_value_2 = ctx.top.cycle.read();
        const bool condition_3 = (cycle_value_2==2u);
        const Entry state_value = ctx.top.slots.value1.state.read();
        if (!((condition && (condition_2 && (condition_3 && (!state_value.valid)))))) return;
        // Prepare every effect before publishing state operations.
        const Entry payload = Entry{true,true,30u};
        const Entry write_value = payload;
        const std::array<bool,3> used_value = ctx.top.age.used.read();
        const std::array<bool,3> flags = used_value;
        const std::array<bool,3> flags_2 = [&]{ auto r=flags; r.at(uint64_t(1ULL))=true; return r; }();
        const std::array<bool,3> write_value_2 = flags_2;
        const std::array<std::array<bool,3>,3> older_value = ctx.top.age.older.read();
        const std::array<std::array<bool,3>,3> matrix = older_value;
        const std::array<std::array<bool,3>,3> matrix_2 = [&]{ auto r=matrix; r.at(uint64_t(0ULL)).at(uint64_t(1ULL))=flags.at(uint64_t(0ULL)); return r; }();
        const std::array<std::array<bool,3>,3> matrix_3 = [&]{ auto r=matrix_2; r.at(uint64_t(1ULL)).at(uint64_t(0ULL))=false; return r; }();
        const std::array<std::array<bool,3>,3> matrix_4 = [&]{ auto r=matrix_3; r.at(uint64_t(1ULL)).at(uint64_t(1ULL))=flags.at(uint64_t(1ULL)); return r; }();
        const std::array<std::array<bool,3>,3> matrix_5 = [&]{ auto r=matrix_4; r.at(uint64_t(1ULL)).at(uint64_t(1ULL))=false; return r; }();
        const std::array<std::array<bool,3>,3> matrix_6 = [&]{ auto r=matrix_5; r.at(uint64_t(2ULL)).at(uint64_t(1ULL))=flags.at(uint64_t(2ULL)); return r; }();
        const std::array<std::array<bool,3>,3> matrix_7 = [&]{ auto r=matrix_6; r.at(uint64_t(1ULL)).at(uint64_t(2ULL))=false; return r; }();
        const std::array<std::array<bool,3>,3> write_value_3 = matrix_7;
        ctx.top.slots.value1.state.write(write_value);
        ctx.top.age.used.write(write_value_2);
        ctx.top.age.older.write(write_value_3);
        ctx.fired_top_allocate=true;
      }
      // Rule top.allocate#1
      template<class Context> void try_allocate_2(Context &ctx) {
        const bool condition = std::get<0>(ctx.compute_shared_computation());
        const uint32_t cycle_value = std::get<0>(ctx.compute_cycle_view_2());
        const bool condition_2 = (cycle_value==1u);
        const Entry state_value = ctx.top.slots.value0.state.read();
        if (!((condition && (condition_2 && (!state_value.valid))))) return;
        // Prepare every effect before publishing state operations.
        const Entry payload = Entry{true,true,10u};
        const Entry write_value = payload;
        const std::array<bool,3> used_value = ctx.top.age.used.read();
        const std::array<bool,3> flags = used_value;
        const std::array<bool,3> flags_2 = [&]{ auto r=flags; r.at(uint64_t(0ULL))=true; return r; }();
        const std::array<bool,3> write_value_2 = flags_2;
        const std::array<std::array<bool,3>,3> older_value = ctx.top.age.older.read();
        const std::array<std::array<bool,3>,3> matrix = older_value;
        const std::array<std::array<bool,3>,3> matrix_2 = [&]{ auto r=matrix; r.at(uint64_t(0ULL)).at(uint64_t(0ULL))=flags.at(uint64_t(0ULL)); return r; }();
        const std::array<std::array<bool,3>,3> matrix_3 = [&]{ auto r=matrix_2; r.at(uint64_t(0ULL)).at(uint64_t(0ULL))=false; return r; }();
        const std::array<std::array<bool,3>,3> matrix_4 = [&]{ auto r=matrix_3; r.at(uint64_t(1ULL)).at(uint64_t(0ULL))=flags.at(uint64_t(1ULL)); return r; }();
        const std::array<std::array<bool,3>,3> matrix_5 = [&]{ auto r=matrix_4; r.at(uint64_t(0ULL)).at(uint64_t(1ULL))=false; return r; }();
        const std::array<std::array<bool,3>,3> matrix_6 = [&]{ auto r=matrix_5; r.at(uint64_t(2ULL)).at(uint64_t(0ULL))=flags.at(uint64_t(2ULL)); return r; }();
        const std::array<std::array<bool,3>,3> matrix_7 = [&]{ auto r=matrix_6; r.at(uint64_t(0ULL)).at(uint64_t(2ULL))=false; return r; }();
        const std::array<std::array<bool,3>,3> write_value_3 = matrix_7;
        ctx.top.slots.value0.state.write(write_value);
        ctx.top.age.used.write(write_value_2);
        ctx.top.age.older.write(write_value_3);
        ctx.fired_top_allocate_2=true;
      }
      // Rule top.allocate#2
      template<class Context> void try_allocate_3(Context &ctx) {
        const uint32_t cycle_value = std::get<0>(ctx.compute_cycle_view());
        const bool condition = (cycle_value==0u);
        const Entry state_value = ctx.top.slots.value2.state.read();
        if (!((condition && (!state_value.valid)))) return;
        // Prepare every effect before publishing state operations.
        const Entry payload = Entry{true,false,20u};
        const Entry write_value = payload;
        const std::array<bool,3> used_value = ctx.top.age.used.read();
        const std::array<bool,3> flags = used_value;
        const std::array<bool,3> flags_2 = [&]{ auto r=flags; r.at(uint64_t(2ULL))=true; return r; }();
        const std::array<bool,3> write_value_2 = flags_2;
        const std::array<std::array<bool,3>,3> older_value = ctx.top.age.older.read();
        const std::array<std::array<bool,3>,3> matrix = older_value;
        const std::array<std::array<bool,3>,3> matrix_2 = [&]{ auto r=matrix; r.at(uint64_t(0ULL)).at(uint64_t(2ULL))=flags.at(uint64_t(0ULL)); return r; }();
        const std::array<std::array<bool,3>,3> matrix_3 = [&]{ auto r=matrix_2; r.at(uint64_t(2ULL)).at(uint64_t(0ULL))=false; return r; }();
        const std::array<std::array<bool,3>,3> matrix_4 = [&]{ auto r=matrix_3; r.at(uint64_t(1ULL)).at(uint64_t(2ULL))=flags.at(uint64_t(1ULL)); return r; }();
        const std::array<std::array<bool,3>,3> matrix_5 = [&]{ auto r=matrix_4; r.at(uint64_t(2ULL)).at(uint64_t(1ULL))=false; return r; }();
        const std::array<std::array<bool,3>,3> matrix_6 = [&]{ auto r=matrix_5; r.at(uint64_t(2ULL)).at(uint64_t(2ULL))=flags.at(uint64_t(2ULL)); return r; }();
        const std::array<std::array<bool,3>,3> matrix_7 = [&]{ auto r=matrix_6; r.at(uint64_t(2ULL)).at(uint64_t(2ULL))=false; return r; }();
        const std::array<std::array<bool,3>,3> write_value_3 = matrix_7;
        ctx.top.slots.value2.state.write(write_value);
        ctx.top.age.used.write(write_value_2);
        ctx.top.age.older.write(write_value_3);
        ctx.fired_top_allocate_3=true;
      }
      // Rule top.drain#5
      template<class Context> void try_drain(Context &ctx) {
        const uint32_t cycle_value = ctx.top.cycle.read();
        const bool condition = (cycle_value>=4u);
        const uint32_t cycle_value_2 = ctx.top.cycle.read();
        const bool condition_2 = (acir_sim::divide(cycle_value_2,2u,false,true,false)==0u);
        const bool output_ready = (!ctx.top.output.empty());
        if (!((condition && (condition_2 && output_ready)))) return;
        // Prepare every effect before publishing state operations.
        const uint32_t output_value = ctx.top.output.read();
        const uint32_t write_value = output_value;
        const uint32_t retired_value = ctx.top.retired.read();
        const uint32_t write_value_2 = (retired_value + 1u);
        ctx.top.output.pop();
        ctx.top.last.write(write_value);
        ctx.top.retired.write(write_value_2);
        ctx.fired_top_drain=true;
      }
      // Rule top.issue#3
      template<class Context> void try_issue(Context &ctx) {
        const uint32_t cycle_value = ctx.top.cycle.read();
        const bool condition = (cycle_value>=3u);
        const Entry state_value = ctx.top.slots.value0.state.read();
        const Entry state_value_2 = ctx.top.slots.value1.state.read();
        const Entry state_value_3 = ctx.top.slots.value2.state.read();
        const std::array<Entry,3> entries = std::array<Entry,3>{state_value,state_value_2,state_value_3};
        const std::array<bool,3> computed_value_4 = [&]{ std::array<bool,3> r{}; acir_sim::full_mask(r); return r; }();
        const bool ready_6 = computed_value_4.at(uint64_t(0ULL));
        const Entry computed_value_5 = entries.at(uint64_t(0ULL));
        const bool ready_8 = computed_value_5.valid;
        const bool ready_14 = computed_value_4.at(uint64_t(1ULL));
        const Entry computed_value_6 = entries.at(uint64_t(1ULL));
        const bool ready_16 = computed_value_6.valid;
        const bool ready_23 = computed_value_4.at(uint64_t(2ULL));
        const Entry computed_value_7 = entries.at(uint64_t(2ULL));
        const bool ready_25 = computed_value_7.valid;
        const std::array<bool,3> payload = std::array<bool,3>{(ready_6 && (ready_8 && computed_value_5.ready)),(ready_14 && (ready_16 && computed_value_6.ready)),(ready_23 && (ready_25 && computed_value_7.ready))};
        const std::tuple<std::array<Entry,3>,std::array<bool,3>> candidates = std::tuple<std::array<Entry,3>,std::array<bool,3>>{entries,payload};
        const std::array<bool,3> computed_value_8 = std::get<1>(candidates);
        const std::array<bool,3> mask = std::array<bool,3>{computed_value_8.at(uint64_t(0ULL)),computed_value_8.at(uint64_t(1ULL)),computed_value_8.at(uint64_t(2ULL))};
        const bool ready_47 = mask.at(uint64_t(2ULL));
        const std::array<std::array<bool,3>,3> older_value = ctx.top.age.older.read();
        const std::array<std::array<bool,3>,3> matrix = older_value;
        const std::array<bool,3> computed_value_9 = matrix.at(uint64_t(2ULL));
        const bool ready_48 = computed_value_9.at(uint64_t(2ULL));
        const bool ready_50 = mask.at(uint64_t(1ULL));
        const std::array<bool,3> computed_value_10 = matrix.at(uint64_t(1ULL));
        const bool ready_51 = computed_value_10.at(uint64_t(2ULL));
        const bool ready_53 = mask.at(uint64_t(0ULL));
        const std::array<bool,3> computed_value_11 = matrix.at(uint64_t(0ULL));
        const bool ready_54 = computed_value_11.at(uint64_t(2ULL));
        const bool oldest = (!(ready_53 && ready_54));
        const bool oldest_2 = ((ready_50 && ready_51) ? false : oldest);
        const bool oldest_3 = ((ready_47 && ready_48) ? false : oldest_2);
        const bool selected_value = (ready_47 ? oldest_3 : false);
        const bool ready_56 = computed_value_9.at(uint64_t(1ULL));
        const bool ready_58 = computed_value_10.at(uint64_t(1ULL));
        const bool ready_60 = computed_value_11.at(uint64_t(1ULL));
        const bool oldest_4 = (!(ready_53 && ready_60));
        const bool oldest_5 = ((ready_50 && ready_58) ? false : oldest_4);
        const bool oldest_6 = ((ready_47 && ready_56) ? false : oldest_5);
        const bool selected_value_2 = (ready_50 ? oldest_6 : false);
        const bool ready_62 = computed_value_9.at(uint64_t(0ULL));
        const bool ready_64 = computed_value_10.at(uint64_t(0ULL));
        const bool ready_66 = computed_value_11.at(uint64_t(0ULL));
        const bool oldest_7 = (!(ready_53 && ready_66));
        const bool oldest_8 = ((ready_50 && ready_64) ? false : oldest_7);
        const bool oldest_9 = ((ready_47 && ready_62) ? false : oldest_8);
        const bool found = (ready_53 ? oldest_9 : false);
        const bool found_2 = (selected_value_2 ? true : found);
        const bool found_3 = (selected_value ? true : found_2);
        const bool ready_68 = (condition && found_3);
        const bool ready_69 = (!ready_68);
        const uint32_t position = uint32_t(selected_value_2);
        const uint32_t position_2 = (selected_value ? 2u : position);
        const bool value_enable0 = (position_2==0u);
        const bool value_enable1 = (position_2==1u);
        const bool value_enable2 = (position_2==2u);
        const bool ready_73 = (ready_69 || ((value_enable0 ? true : value_enable1) ? true : value_enable2));
        const bool value_enable0_2 = (position_2!=0u);
        const bool ready_78 = (ready_68 && value_enable0_2);
        const bool ready_79 = (!ready_78);
        const bool value_enable1_2 = (position_2!=1u);
        const bool ready_84 = (ready_78 && value_enable1_2);
        const bool ready_85 = (!ready_84);
        const bool ready_89 = ((((condition && ready_73) && (ready_69 || ready_73)) && (ready_79 || (ready_79 || ready_73))) && (ready_85 || (ready_85 || (ready_79 || ready_73))));
        const bool ready_90 = (ready_84 && value_enable2);
        const bool ready_91 = (!ready_90);
        const bool ready_92 = (ready_91 || (ready_85 || (ready_79 || ready_73)));
        const bool ready_93 = (ready_91 || ready_92);
        const Entry state_value_4 = ctx.top.slots.value2.state.read();
        const bool ready_96 = state_value_4.valid;
        const bool ready_98 = (!(ready_90 && ready_96));
        const Entry state_value_5 = ctx.top.slots.value2.state.read();
        const bool ready_103 = (ready_96 && state_value_5.ready);
        const bool ready_105 = (ready_91 || (ready_92 && ready_103));
        const bool ready_108 = ((((ready_89 && (ready_91 || ready_93)) && (ready_98 || (ready_98 || ready_93))) && ready_105) && (ready_91 || ready_105));
        const bool ready_109 = (ready_78 && value_enable1);
        const bool ready_110 = (!ready_109);
        const bool ready_111 = (ready_110 || (ready_79 || ready_73));
        const bool ready_112 = (ready_110 || ready_111);
        const Entry state_value_6 = ctx.top.slots.value1.state.read();
        const bool ready_115 = state_value_6.valid;
        const bool ready_117 = (!(ready_109 && ready_115));
        const Entry state_value_7 = ctx.top.slots.value1.state.read();
        const bool ready_122 = (ready_115 && state_value_7.ready);
        const bool ready_124 = (ready_110 || (ready_111 && ready_122));
        const bool ready_127 = ((((ready_108 && (ready_110 || ready_112)) && (ready_117 || (ready_117 || ready_112))) && ready_124) && (ready_110 || ready_124));
        const bool ready_128 = (ready_68 && value_enable0);
        const bool ready_129 = (!ready_128);
        const bool ready_130 = (ready_129 || ready_73);
        const bool ready_131 = (ready_129 || ready_130);
        const Entry state_value_8 = ctx.top.slots.value0.state.read();
        const bool ready_134 = state_value_8.valid;
        const bool ready_136 = (!(ready_128 && ready_134));
        const Entry state_value_9 = ctx.top.slots.value0.state.read();
        const bool ready_141 = (ready_134 && state_value_9.ready);
        const bool ready_143 = (ready_129 || (ready_130 && ready_141));
        const bool ready_146 = ((((ready_127 && (ready_129 || ready_131)) && (ready_136 || (ready_136 || ready_131))) && ready_143) && (ready_129 || ready_143));
        const uint64_t computed_value_12 = uint64_t(position_2);
        const bool condition_3 = (computed_value_12<uint64_t(3ULL));
        const bool value_enable2_2 = (position_2!=2u);
        const bool ready_167 = ((value_enable0 ? ready_141 : (value_enable1 ? ready_122 : (value_enable2_2 || ready_103))) && ((value_enable0 ? true : value_enable1) ? true : value_enable2));
        const bool output_full = ctx.top.output.full();
        const bool ready_177 = ((((ready_146 && (ready_69 || ready_73)) && (ready_69 || condition_3)) && (ready_69 || ready_167)) && (ready_69 || (!output_full)));
        if (!(ready_177)) return;
        // Prepare every effect before publishing state operations.
        const bool ready_179 = (!ready_62);
        const bool ready_180 = (!ready_64);
        const bool ready_181 = (!ready_66);
        const bool ready_183 = (value_enable0_2 && (value_enable1_2 && value_enable2));
        const bool ready_184 = (ready_181 && ready_183);
        const bool ready_185 = (ready_180 && ready_184);
        const bool ready_186 = (ready_179 && ready_185);
        const bool selected_value_9 = (ready_56 ? ready_186 : (ready_58 ? ready_186 : (ready_60 ? ready_186 : ready_183)));
        const bool ready_187 = (!ready_56);
        const bool ready_188 = (!ready_58);
        const bool ready_189 = (ready_188 && ready_183);
        const bool selected_value_10 = (ready_53 ? selected_value_9 : (ready_187 && ready_189));
        const bool ready_191 = (ready_179 && ready_184);
        const bool selected_value_17 = (ready_50 ? (ready_51 ? selected_value_10 : (ready_53 ? (ready_54 ? selected_value_9 : ready_183) : ready_183)) : (ready_53 ? (ready_54 ? ready_191 : ready_183) : ready_183));
        const bool selected_value_22 = (ready_50 ? (ready_53 ? (ready_58 ? ready_185 : (ready_60 ? ready_185 : ready_183)) : ready_189) : (ready_53 && ready_184));
        const bool selected_value_23 = (ready_47 ? (ready_48 ? (ready_50 ? selected_value_10 : (ready_53 && ready_191)) : selected_value_17) : selected_value_22);
        const bool write_fire = selected_value_23;
        Entry write_value{};
        if (write_fire) {
          const Entry payload_2 = Entry{false,false,0u};
          write_value = payload_2;
        }
        const bool ready_196 = (value_enable0_2 && value_enable1);
        const bool ready_197 = (ready_181 && ready_196);
        const bool ready_198 = (ready_180 && ready_197);
        const bool ready_199 = (ready_179 && ready_198);
        const bool selected_value_26 = (ready_56 ? ready_199 : (ready_58 ? ready_199 : (ready_60 ? ready_199 : ready_196)));
        const bool ready_200 = (ready_188 && ready_196);
        const bool selected_value_27 = (ready_53 ? selected_value_26 : (ready_187 && ready_200));
        const bool ready_202 = (ready_179 && ready_197);
        const bool selected_value_31 = (ready_51 ? selected_value_27 : (ready_53 ? (ready_54 ? selected_value_26 : ready_196) : ready_196));
        const bool selected_value_35 = (ready_48 ? (ready_50 ? selected_value_27 : (ready_53 && ready_202)) : (ready_50 ? selected_value_31 : (ready_53 ? (ready_54 ? ready_202 : ready_196) : ready_196)));
        const bool selected_value_39 = (ready_50 ? (ready_53 ? (ready_58 ? ready_198 : (ready_60 ? ready_198 : ready_196)) : ready_200) : (ready_53 && ready_197));
        const bool write_fire_2 = (ready_47 ? selected_value_35 : selected_value_39);
        Entry write_value_2{};
        if (write_fire_2) {
          const Entry payload_3 = Entry{false,false,0u};
          write_value_2 = payload_3;
        }
        const bool ready_207 = (ready_181 && value_enable0);
        const bool ready_208 = (ready_180 && ready_207);
        const bool ready_209 = (ready_179 && ready_208);
        const bool selected_value_43 = (ready_56 ? ready_209 : (ready_58 ? ready_209 : (ready_60 ? ready_209 : value_enable0)));
        const bool ready_210 = (ready_188 && value_enable0);
        const bool selected_value_44 = (ready_53 ? selected_value_43 : (ready_187 && ready_210));
        const bool ready_212 = (ready_179 && ready_207);
        const bool selected_value_48 = (ready_51 ? selected_value_44 : (ready_53 ? (ready_54 ? selected_value_43 : value_enable0) : value_enable0));
        const bool selected_value_51 = (ready_50 ? selected_value_48 : (ready_53 ? (ready_54 ? ready_212 : value_enable0) : value_enable0));
        const bool selected_value_56 = (ready_50 ? (ready_53 ? (ready_58 ? ready_208 : (ready_60 ? ready_208 : value_enable0)) : ready_210) : (ready_53 && ready_207));
        const bool selected_value_57 = (ready_47 ? (ready_48 ? (ready_50 ? selected_value_44 : (ready_53 && ready_212)) : selected_value_51) : selected_value_56);
        const bool write_fire_3 = selected_value_57;
        Entry write_value_3{};
        if (write_fire_3) {
          const Entry payload_4 = Entry{false,false,0u};
          write_value_3 = payload_4;
        }
        const bool ready_217 = (ready_180 && ready_181);
        const bool ready_218 = (ready_179 && ready_217);
        const bool ready_219 = (!ready_60);
        const bool selected_value_59 = (ready_56 ? ready_218 : (ready_58 ? ready_218 : (ready_219 || ready_218)));
        const bool selected_value_60 = (ready_53 ? selected_value_59 : (ready_187 && ready_188));
        const bool ready_222 = (ready_179 && ready_181);
        const bool ready_224 = (!ready_53);
        const bool ready_225 = (!ready_54);
        const bool selected_value_63 = (ready_50 ? (ready_51 ? selected_value_60 : (ready_224 || (ready_225 || selected_value_59))) : (ready_224 || (ready_225 || ready_222)));
        const bool selected_value_67 = (ready_50 ? (ready_53 ? (ready_58 ? ready_217 : (ready_219 || ready_217)) : ready_188) : (ready_53 && ready_181));
        const bool selected_value_68 = (ready_47 ? (ready_48 ? (ready_50 ? selected_value_60 : (ready_53 && ready_222)) : selected_value_63) : selected_value_67);
        const bool check_fire = selected_value_68;
        if (check_fire) {
          acir_sim::check((!(value_enable0_2 && (value_enable1_2 && value_enable2_2))));
        }
        const bool write_fire_4 = selected_value_68;
        std::array<bool,3> write_value_4{};
        if (write_fire_4) {
          const std::array<bool,3> used_value = ctx.top.age.used.read();
          const std::array<bool,3> flags = used_value;
          const std::array<bool,3> flags_2 = [&]{ auto r=flags; r.at((condition_3 ? computed_value_12 : uint64_t(0ULL)))=false; return r; }();
          write_value_4 = flags_2;
        }
        const bool push_fire = selected_value_68;
        uint32_t push_value{};
        if (push_fire) {
          const Entry state_value_10 = ctx.top.slots.value0.state.read();
          const uint32_t value = state_value_10.value;
          const Entry state_value_11 = ctx.top.slots.value1.state.read();
          const uint32_t value_2 = state_value_11.value;
          const Entry state_value_12 = ctx.top.slots.value2.state.read();
          const uint32_t value_3 = state_value_12.value;
          const uint32_t value_4 = (value_enable0 ? value : (value_enable1 ? value_2 : (value_enable2 ? value_3 : uint32_t{})));
          push_value = value_4;
        }
        const bool write_fire_5 = selected_value_68;
        uint32_t write_value_5{};
        if (write_fire_5) {
          const uint32_t issued_value = ctx.top.issued.read();
          write_value_5 = (issued_value + 1u);
        }
        if (write_fire) { ctx.top.slots.value2.state.write(write_value); }
        if (write_fire_2) { ctx.top.slots.value1.state.write(write_value_2); }
        if (write_fire_3) { ctx.top.slots.value0.state.write(write_value_3); }
        if (write_fire_4) { ctx.top.age.used.write(write_value_4); }
        if (push_fire) { ctx.top.output.push(push_value); }
        if (write_fire_5) { ctx.top.issued.write(write_value_5); }
        ctx.fired_top_issue=true;
      }
      // Rule top.clock#6
      template<class Context> void try_clock(Context &ctx) {
        if (!(true)) return;
        // Prepare every effect before publishing state operations.
        const uint32_t cycle_value = ctx.top.cycle.read();
        const uint32_t write_value = (cycle_value + 1u);
        ctx.top.cycle.write(write_value);
        ctx.fired_top_clock=true;
      }
    } top;
    std::optional<std::tuple<uint32_t,bool>> compute_cycle_view_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_cache;
    std::optional<std::tuple<uint32_t,bool>> compute_cycle_view_2_cache;
    bool fired_top_allocate=false;
    bool fired_top_allocate_2=false;
    bool fired_top_allocate_3=false;
    bool fired_top_slots_2_wake=false;
    bool fired_top_drain=false;
    bool fired_top_issue=false;
    bool fired_top_clock=false;
    Model() { reset(); }
    void reset() {
      top.age.older={};
      top.age.older.value=std::array<std::array<bool,3>,3>{std::array<bool,3>{false,false,false},std::array<bool,3>{false,false,false},std::array<bool,3>{false,false,false}};
      top.age.used={};
      top.age.used.value=std::array<bool,3>{false,false,false};
      top.cycle={};
      top.cycle.value=0u;
      top.issued={};
      top.issued.value=0u;
      top.last={};
      top.last.value=0u;
      top.output={};
      top.output.data[0]=99u;
      top.output.count=1;
      top.retired={};
      top.retired.value=0u;
      top.slots.value0.state={};
      top.slots.value0.state.value=Entry{false,false,0u};
      top.slots.value1.state={};
      top.slots.value1.state.value=Entry{false,false,0u};
      top.slots.value2.state={};
      top.slots.value2.state.value=Entry{false,false,0u};
      begin_tick();
    }
    void begin_tick() {
      top.age.older.begin();
      top.age.used.begin();
      top.cycle.begin();
      top.issued.begin();
      top.last.begin();
      top.output.begin();
      top.retired.begin();
      top.slots.value0.state.begin();
      top.slots.value1.state.begin();
      top.slots.value2.state.begin();
      compute_cycle_view_cache.reset();
      compute_shared_computation_cache.reset();
      compute_cycle_view_2_cache.reset();
      fired_top_allocate=false;
      fired_top_allocate_2=false;
      fired_top_allocate_3=false;
      fired_top_slots_2_wake=false;
      fired_top_drain=false;
      fired_top_issue=false;
      fired_top_clock=false;
    }
    void commit() {
      top.age.older.commit();
      top.age.used.commit();
      top.cycle.commit();
      top.issued.commit();
      top.last.commit();
      top.output.commit();
      top.retired.commit();
      top.slots.value0.state.commit();
      top.slots.value1.state.commit();
      top.slots.value2.state.commit();
    }
    void tick() {
      begin_tick();
      top.try_allocate(*this);
      top.try_allocate_2(*this);
      top.try_allocate_3(*this);
      top.slots.value2.try_wake(*this);
      top.try_drain(*this);
      top.try_issue(*this);
      top.try_clock(*this);
      commit();
    }
    void trace(std::ostream &o,uint64_t tick) const { o<<"{\"tick\":"<<tick<<",\"fire\":{";
      o<<"\"top.allocate#0\":"; json(o,fired_top_allocate);
      o<<',';
      o<<"\"top.allocate#1\":"; json(o,fired_top_allocate_2);
      o<<',';
      o<<"\"top.allocate#2\":"; json(o,fired_top_allocate_3);
      o<<',';
      o<<"\"top.slots.2.wake#4\":"; json(o,fired_top_slots_2_wake);
      o<<',';
      o<<"\"top.drain#5\":"; json(o,fired_top_drain);
      o<<',';
      o<<"\"top.issue#3\":"; json(o,fired_top_issue);
      o<<',';
      o<<"\"top.clock#6\":"; json(o,fired_top_clock);
      o<<"},\"state\":{";
      o<<"\"top.age.older\":"; json(o,top.age.older.value);
      o<<',';
      o<<"\"top.age.used\":"; json(o,top.age.used.value);
      o<<',';
      o<<"\"top.cycle\":"; json(o,top.cycle.value);
      o<<',';
      o<<"\"top.issued\":"; json(o,top.issued.value);
      o<<',';
      o<<"\"top.last\":"; json(o,top.last.value);
      o<<',';
      o<<"\"top.output\":"; json(o,top.output);
      o<<',';
      o<<"\"top.retired\":"; json(o,top.retired.value);
      o<<',';
      o<<"\"top.slots.0.state\":"; json(o,top.slots.value0.state.value);
      o<<',';
      o<<"\"top.slots.1.state\":"; json(o,top.slots.value1.state.value);
      o<<',';
      o<<"\"top.slots.2.state\":"; json(o,top.slots.value2.state.value);
      o<<"}}"; }
    const std::tuple<uint32_t,bool> &compute_cycle_view() {
      if (!compute_cycle_view_cache) {
        const uint32_t cycle_value = top.cycle.read();
        compute_cycle_view_cache.emplace(std::tuple<uint32_t,bool>{cycle_value,true});
      }
      return *compute_cycle_view_cache;
    }
    const std::tuple<bool> &compute_shared_computation() {
      if (!compute_shared_computation_cache) {
        const uint32_t cycle_value = std::get<0>(compute_cycle_view());
        const bool condition = (cycle_value!=0u);
        compute_shared_computation_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_cache;
    }
    const std::tuple<uint32_t,bool> &compute_cycle_view_2() {
      if (!compute_cycle_view_2_cache) {
        const uint32_t cycle_value = top.cycle.read();
        compute_cycle_view_2_cache.emplace(std::tuple<uint32_t,bool>{cycle_value,true});
      }
      return *compute_cycle_view_2_cache;
    }
  };
} // namespace acir_generated
