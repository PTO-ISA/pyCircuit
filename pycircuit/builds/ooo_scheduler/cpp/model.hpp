#pragma once
#include "runtime.hpp"
#include <set>
namespace acir_generated {
  using namespace acir_sim;
  using acir_sim::json;
  // ACPy payload src_payloads_25ebcaa7.Completion
  struct Completion {
    uint32_t tag{};
    uint32_t rd{};
    uint32_t value{};
    bool operator==(const Completion&) const = default;
  };
  inline void json(std::ostream &o,const Completion &v) { o<<'[';
    json(o,v.tag);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.value);
    o<<']'; }
  // ACPy payload src_payloads_25ebcaa7.Operand
  struct Operand {
    bool ready{};
    uint32_t tag{};
    uint32_t value{};
    bool operator==(const Operand&) const = default;
  };
  inline void json(std::ostream &o,const Operand &v) { o<<'[';
    json(o,v.ready);
    o<<','; json(o,v.tag);
    o<<','; json(o,v.value);
    o<<']'; }
  // ACPy payload src_payloads_25ebcaa7.Entry
  struct Entry {
    bool valid{};
    uint32_t kind{};
    uint32_t tag{};
    uint32_t rd{};
    Operand lhs{};
    Operand rhs{};
    bool operator==(const Entry&) const = default;
  };
  inline void json(std::ostream &o,const Entry &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.kind);
    o<<','; json(o,v.tag);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.lhs);
    o<<','; json(o,v.rhs);
    o<<']'; }
  // ACPy payload src_payloads_25ebcaa7.RobEntry
  struct RobEntry {
    bool valid{};
    bool done{};
    uint32_t tag{};
    uint32_t rd{};
    uint32_t value{};
    bool operator==(const RobEntry&) const = default;
  };
  inline void json(std::ostream &o,const RobEntry &v) { o<<'[';
    json(o,v.valid);
    o<<','; json(o,v.done);
    o<<','; json(o,v.tag);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.value);
    o<<']'; }
  // ACPy payload src_payloads_25ebcaa7.MicroOp
  struct MicroOp {
    uint32_t kind{};
    uint32_t rd{};
    uint32_t rs1{};
    uint32_t rs2{};
    bool operator==(const MicroOp&) const = default;
  };
  inline void json(std::ostream &o,const MicroOp &v) { o<<'[';
    json(o,v.kind);
    o<<','; json(o,v.rd);
    o<<','; json(o,v.rs1);
    o<<','; json(o,v.rs2);
    o<<']'; }
  class Model {
  public:
    struct OutOfOrder {
      struct Adder {
        // Rule top.adder.execute#2
        template<class Context> void try_execute(Context &ctx) {
          const bool add_requests_ready = (!ctx.top.add_requests.empty());
          const bool add_done_full = ctx.top.add_done.full();
          if (!((add_requests_ready && (!add_done_full)))) return;
          // Prepare every effect before publishing state operations.
          const Entry add_requests_value = ctx.top.add_requests.read();
          const Entry entry = add_requests_value;
          const Completion push_value = Completion{entry.tag,entry.rd,(entry.lhs.value + entry.rhs.value)};
          ctx.top.add_requests.pop();
          ctx.top.add_done.push(push_value);
          ctx.fired_top_adder_execute=true;
        }
      } adder;
      struct CompletionAudit {
        Cell<uint32_t> last_tag{};
        // Rule top.audit.observe#8
        template<class Context> void try_observe(Context &ctx) {
          const bool event_ready = ctx.top.event.valid;
          if (!(event_ready)) return;
          // Prepare every effect before publishing state operations.
          const Completion event_value = ctx.top.event.read();
          const Completion saved = event_value;
          const uint32_t write_value = saved.tag;
          ctx.top.audit.last_tag.write(write_value);
          ctx.fired_top_audit_observe=true;
        }
      } audit;
      struct Broadcast {
        // Rule top.cdb.broadcast#5
        template<class Context> void try_broadcast(Context &ctx) {
          const bool add_done_empty = ctx.top.add_done.empty();
          const bool mul_done_ready = (!ctx.top.mul_done.empty());
          const Completion mul_done_value = ctx.top.mul_done.read();
          const Completion result = mul_done_value;
          const Completion add_done_value = ctx.top.add_done.read();
          const Completion result_2 = add_done_value;
          const Completion result_3 = (add_done_empty ? result : result_2);
          const uint32_t tag = result_3.tag;
          const uint32_t computed_value_2 = ((tag - 1u) & 3u);
          const bool value_enable0 = (computed_value_2==0u);
          const bool value_enable1 = (computed_value_2==1u);
          const bool value_enable2 = (computed_value_2==2u);
          const bool value_enable3 = (computed_value_2==3u);
          const bool ready_3 = (value_enable0 || (value_enable1 || (value_enable2 || value_enable3)));
          if (!((add_done_empty ? (mul_done_ready && ready_3) : ready_3))) return;
          // Prepare every effect before publishing state operations.
          const bool pop_fire = add_done_empty;
          const bool prefer_add = (!add_done_empty);
          const bool pop_fire_2 = prefer_add;
          const bool value_enable0_2 = (computed_value_2!=0u);
          const bool value_enable1_2 = (computed_value_2!=1u);
          const bool value_enable2_2 = (computed_value_2!=2u);
          const RobEntry state_value = ctx.top.rob.slots.value3.state.read();
          const RobEntry entry = state_value;
          const bool ready_7 = entry.valid;
          const uint32_t tag_2 = entry.tag;
          const bool ready_14 = (value_enable1_2 && (value_enable2_2 && (value_enable3 && (ready_7 && ((!entry.done) && (tag_2==tag))))));
          const bool write_fire = (value_enable0_2 && ready_14);
          RobEntry write_value{};
          if (write_fire) {
            const RobEntry entry_2 = RobEntry{ready_7,true,tag_2,entry.rd,entry.value};
            const uint32_t value_2 = result_3.value;
            const RobEntry entry_3 = RobEntry{entry_2.valid,entry_2.done,entry_2.tag,entry_2.rd,value_2};
            write_value = entry_3;
          }
          const RobEntry state_value_2 = ctx.top.rob.slots.value2.state.read();
          const RobEntry entry_4 = state_value_2;
          const bool ready_19 = entry_4.valid;
          const uint32_t tag_4 = entry_4.tag;
          const bool ready_26 = (value_enable0_2 && (value_enable1_2 && (value_enable2 && (ready_19 && ((!entry_4.done) && (tag_4==tag))))));
          const bool write_fire_2 = ready_26;
          RobEntry write_value_2{};
          if (write_fire_2) {
            const RobEntry entry_5 = RobEntry{ready_19,true,tag_4,entry_4.rd,entry_4.value};
            const uint32_t value_4 = result_3.value;
            const RobEntry entry_6 = RobEntry{entry_5.valid,entry_5.done,entry_5.tag,entry_5.rd,value_4};
            write_value_2 = entry_6;
          }
          const RobEntry state_value_3 = ctx.top.rob.slots.value1.state.read();
          const RobEntry entry_7 = state_value_3;
          const bool ready_30 = entry_7.valid;
          const uint32_t tag_6 = entry_7.tag;
          const bool write_fire_3 = (value_enable0_2 && (value_enable1 && (ready_30 && ((!entry_7.done) && (tag_6==tag)))));
          RobEntry write_value_3{};
          if (write_fire_3) {
            const RobEntry entry_8 = RobEntry{ready_30,true,tag_6,entry_7.rd,entry_7.value};
            const uint32_t value_6 = result_3.value;
            const RobEntry entry_9 = RobEntry{entry_8.valid,entry_8.done,entry_8.tag,entry_8.rd,value_6};
            write_value_3 = entry_9;
          }
          const RobEntry state_value_4 = ctx.top.rob.slots.value0.state.read();
          const RobEntry entry_10 = state_value_4;
          const bool ready_40 = entry_10.valid;
          const uint32_t tag_8 = entry_10.tag;
          const bool write_fire_4 = (value_enable0 && (ready_40 && ((!entry_10.done) && (tag_8==tag))));
          RobEntry write_value_4{};
          if (write_fire_4) {
            const RobEntry entry_11 = RobEntry{ready_40,true,tag_8,entry_10.rd,entry_10.value};
            const uint32_t value_8 = result_3.value;
            const RobEntry entry_12 = RobEntry{entry_11.valid,entry_11.done,entry_11.tag,entry_11.rd,value_8};
            write_value_4 = entry_12;
          }
          const Entry slot_value = ctx.top.left.slot.read();
          const Entry entry_13 = slot_value;
          const bool ready_49 = entry_13.valid;
          const Operand lhs = entry_13.lhs;
          const bool changed = ((!lhs.ready) && (lhs.tag==tag));
          const uint32_t value_9 = result_3.value;
          const Operand payload = Operand{true,tag,value_9};
          const Entry entry_14 = Entry{ready_49,entry_13.kind,entry_13.tag,entry_13.rd,payload,entry_13.rhs};
          const Entry entry_15 = (changed ? entry_14 : entry_13);
          const Operand rhs_2 = entry_15.rhs;
          const bool ready_52 = rhs_2.ready;
          const bool condition_6 = (rhs_2.tag==tag);
          const bool write_fire_5 = (ready_49 && (ready_52 ? changed : (condition_6 || changed)));
          Entry write_value_5{};
          if (write_fire_5) {
            const Entry entry_16 = Entry{entry_15.valid,entry_15.kind,entry_15.tag,entry_15.rd,entry_15.lhs,payload};
            const Entry entry_17 = (((!ready_52) && condition_6) ? entry_16 : entry_15);
            const Entry entry_18 = (ready_49 ? entry_17 : entry_13);
            write_value_5 = entry_18;
          }
          const Entry slot_value_2 = ctx.top.right.slot.read();
          const Entry entry_19 = slot_value_2;
          const bool ready_59 = entry_19.valid;
          const Operand lhs_3 = entry_19.lhs;
          const bool changed_2 = ((!lhs_3.ready) && (lhs_3.tag==tag));
          const Entry entry_20 = Entry{ready_59,entry_19.kind,entry_19.tag,entry_19.rd,payload,entry_19.rhs};
          const Entry entry_21 = (changed_2 ? entry_20 : entry_19);
          const Operand rhs_4 = entry_21.rhs;
          const bool ready_62 = rhs_4.ready;
          const bool condition_8 = (rhs_4.tag==tag);
          const bool write_fire_6 = (ready_59 && (ready_62 ? changed_2 : (condition_8 || changed_2)));
          Entry write_value_6{};
          if (write_fire_6) {
            const Entry entry_22 = Entry{entry_21.valid,entry_21.kind,entry_21.tag,entry_21.rd,entry_21.lhs,payload};
            const Entry entry_23 = (((!ready_62) && condition_8) ? entry_22 : entry_21);
            const Entry entry_24 = (ready_59 ? entry_23 : entry_19);
            write_value_6 = entry_24;
          }
          const Completion set_value = result_3;
          if (pop_fire) { ctx.top.mul_done.pop(); }
          if (pop_fire_2) { ctx.top.add_done.pop(); }
          if (write_fire) { ctx.top.rob.slots.value3.state.write(write_value); }
          if (write_fire_2) { ctx.top.rob.slots.value2.state.write(write_value_2); }
          if (write_fire_3) { ctx.top.rob.slots.value1.state.write(write_value_3); }
          if (write_fire_4) { ctx.top.rob.slots.value0.state.write(write_value_4); }
          if (write_fire_5) { ctx.top.left.slot.write(write_value_5); }
          if (write_fire_6) { ctx.top.right.slot.write(write_value_6); }
          ctx.top.event.write(set_value);
          ctx.fired_top_cdb_broadcast=true;
        }
      } cdb;
      struct Commit {
        Cell<uint32_t> last_tag{};
        Cell<uint32_t> retired{};
        // Rule top.commit.commit#7
        template<class Context> void try_commit(Context &ctx) {
          const uint32_t head_value = ctx.top.rob.head.read();
          const uint32_t tag = head_value;
          const uint32_t computed_value_2 = ((tag - 1u) & 3u);
          const bool value_enable0 = (computed_value_2==0u);
          const RobEntry state_value = ctx.top.rob.slots.value0.state.read();
          const RobEntry entry = state_value;
          const RobEntry state_value_2 = ctx.top.rob.slots.value0.state.read();
          const RobEntry entry_2 = state_value_2;
          const bool value_enable1 = (computed_value_2==1u);
          const RobEntry state_value_3 = ctx.top.rob.slots.value1.state.read();
          const RobEntry entry_3 = state_value_3;
          const bool value_enable2 = (computed_value_2==2u);
          const RobEntry state_value_4 = ctx.top.rob.slots.value2.state.read();
          const RobEntry entry_4 = state_value_4;
          const bool value_enable3 = (computed_value_2==3u);
          const RobEntry state_value_5 = ctx.top.rob.slots.value3.state.read();
          const RobEntry entry_5 = state_value_5;
          const RobEntry payload = std::get<0>(ctx.compute_shared_computation_21());
          const RobEntry entry_6 = (value_enable0 ? entry_2 : (value_enable1 ? entry_3 : (value_enable2 ? entry_4 : (value_enable3 ? entry_5 : payload))));
          const uint32_t rd = entry_6.rd;
          const uint64_t computed_value_3 = uint64_t(rd);
          const bool ready_3 = ((rd==0u) || (computed_value_3<uint64_t(8ULL)));
          const RobEntry state_value_6 = ctx.top.rob.slots.value1.state.read();
          const RobEntry entry_7 = state_value_6;
          const RobEntry state_value_7 = ctx.top.rob.slots.value2.state.read();
          const RobEntry entry_8 = state_value_7;
          const RobEntry state_value_8 = ctx.top.rob.slots.value3.state.read();
          const RobEntry entry_9 = state_value_8;
          const bool selected_value_4 = (value_enable2 ? (entry_8.valid && (entry_8.done && ((entry_8.tag==tag) && ready_3))) : (value_enable3 && (entry_9.valid && (entry_9.done && ((entry_9.tag==tag) && ready_3)))));
          const bool selected_value_5 = (value_enable1 ? (entry_7.valid && (entry_7.done && ((entry_7.tag==tag) && ready_3))) : selected_value_4);
          if (!((value_enable0 ? (entry.valid && (entry.done && ((entry.tag==tag) && ready_3))) : selected_value_5))) return;
          // Prepare every effect before publishing state operations.
          const bool value_enable0_2 = (computed_value_2!=0u);
          const bool value_enable1_2 = (computed_value_2!=1u);
          const bool value_enable2_2 = (computed_value_2!=2u);
          const bool write_fire = (value_enable0_2 && (value_enable1_2 && (value_enable2_2 && value_enable3)));
          RobEntry write_value{};
          if (write_fire) {
            const RobEntry payload_2 = RobEntry{false,false,0u,0u,0u};
            write_value = payload_2;
          }
          const bool write_fire_2 = (value_enable0_2 && (value_enable1_2 && value_enable2));
          RobEntry write_value_2{};
          if (write_fire_2) {
            const RobEntry payload_3 = RobEntry{false,false,0u,0u,0u};
            write_value_2 = payload_3;
          }
          const bool write_fire_3 = (value_enable0_2 && value_enable1);
          RobEntry write_value_3{};
          if (write_fire_3) {
            const RobEntry payload_4 = RobEntry{false,false,0u,0u,0u};
            write_value_3 = payload_4;
          }
          const bool write_fire_4 = value_enable0;
          RobEntry write_value_4{};
          if (write_fire_4) {
            const RobEntry payload_5 = RobEntry{false,false,0u,0u,0u};
            write_value_4 = payload_5;
          }
          const uint32_t write_value_5 = (tag + 1u);
          const bool write_fire_5 = (rd!=0u);
          uint32_t write_value_6{};
          uint64_t resource_index{};
          if (write_fire_5) {
            write_value_6 = entry_6.value;
            resource_index = computed_value_3;
          }
          const bool write_fire_6 = (rd!=0u);
          uint32_t write_value_7{};
          uint64_t resource_index_2{};
          if (write_fire_6) {
            const uint32_t tag_6 = entry_6.tag;
            write_value_7 = tag_6;
            resource_index_2 = computed_value_3;
          }
          const uint32_t retired_value = ctx.top.commit.retired.read();
          const uint32_t write_value_8 = (retired_value + 1u);
          const uint32_t tag_7 = entry_6.tag;
          const uint32_t write_value_9 = tag_7;
          if (write_fire) { ctx.top.rob.slots.value3.state.write(write_value); }
          if (write_fire_2) { ctx.top.rob.slots.value2.state.write(write_value_2); }
          if (write_fire_3) { ctx.top.rob.slots.value1.state.write(write_value_3); }
          if (write_fire_4) { ctx.top.rob.slots.value0.state.write(write_value_4); }
          ctx.top.rob.head.write(write_value_5);
          if (write_fire_5) { ctx.top.rf.registers.at(resource_index).write(write_value_6); }
          if (write_fire_6) { ctx.top.rf.committed.at(resource_index_2).write(write_value_7); }
          ctx.top.commit.retired.write(write_value_8);
          ctx.top.commit.last_tag.write(write_value_9);
          ctx.fired_top_commit_commit=true;
        }
      } commit;
      struct Dispatch {
        // Rule top.dispatch.dispatch#6
        template<class Context> void try_dispatch(Context &ctx) {
          const bool add_done_empty = ctx.top.add_done.empty();
          const bool mul_done_empty = ctx.top.mul_done.empty();
          const bool source_ready = (!ctx.top.source.empty());
          const MicroOp source_value = ctx.top.source.read();
          const MicroOp inst = source_value;
          const uint32_t index = inst.rs1;
          const bool condition = (index!=0u);
          const uint64_t computed_value = uint64_t(index);
          const uint32_t renamed_value = ((computed_value < 8) ? ctx.top.rf.renamed.at(computed_value).read() : uint32_t{});
          const uint32_t tag = renamed_value;
          const uint32_t committed_value = ((computed_value < 8) ? ctx.top.rf.committed.at(computed_value).read() : uint32_t{});
          const bool condition_3 = (tag==committed_value);
          const uint32_t index_2 = inst.rs2;
          const bool condition_4 = (index_2!=0u);
          const uint64_t computed_value_2 = uint64_t(index_2);
          const uint32_t renamed_value_2 = ((computed_value_2 < 8) ? ctx.top.rf.renamed.at(computed_value_2).read() : uint32_t{});
          const uint32_t tag_2 = renamed_value_2;
          const uint32_t committed_value_2 = ((computed_value_2 < 8) ? ctx.top.rf.committed.at(computed_value_2).read() : uint32_t{});
          const bool condition_6 = (tag_2==committed_value_2);
          const uint32_t tail_value = ctx.top.rob.tail.read();
          const uint32_t head_value = ctx.top.rob.head.read();
          const uint32_t tail_value_2 = ctx.top.rob.tail.read();
          const uint32_t tag_3 = tail_value_2;
          const uint32_t computed_value_5 = ((tag_3 - 1u) & 3u);
          const bool value_enable0 = (computed_value_5==0u);
          const RobEntry state_value = ctx.top.rob.slots.value0.state.read();
          const uint32_t rd = inst.rd;
          const bool condition_8 = (rd!=0u);
          const uint64_t computed_value_6 = uint64_t(rd);
          const Entry slot_value = ctx.top.left.slot.read();
          const bool ready_3 = slot_value.valid;
          const Entry slot_value_2 = ctx.top.right.slot.read();
          const Entry slot_value_3 = ctx.top.left.slot.read();
          const bool selected_value = (ready_3 ? (!slot_value_2.valid) : (!slot_value_3.valid));
          const bool selected_value_2 = (condition_8 ? ((computed_value_6<uint64_t(8ULL)) && selected_value) : selected_value);
          const bool value_enable1 = (computed_value_5==1u);
          const RobEntry state_value_2 = ctx.top.rob.slots.value1.state.read();
          const bool value_enable2 = (computed_value_5==2u);
          const RobEntry state_value_3 = ctx.top.rob.slots.value2.state.read();
          const bool value_enable3 = (computed_value_5==3u);
          const RobEntry state_value_4 = ctx.top.rob.slots.value3.state.read();
          const bool selected_value_3 = (value_enable2 ? ((!state_value_3.valid) && selected_value_2) : (value_enable3 && ((!state_value_4.valid) && selected_value_2)));
          const bool selected_value_5 = (value_enable0 ? ((!state_value.valid) && selected_value_2) : (value_enable1 ? ((!state_value_2.valid) && selected_value_2) : selected_value_3));
          const bool ready_20 = (((tail_value - head_value)<4u) && selected_value_5);
          const uint32_t computed_value_8 = ((tag_2 - 1u) & 3u);
          const bool value_enable0_2 = (computed_value_8==0u);
          const bool value_enable1_2 = (computed_value_8==1u);
          const bool value_enable2_2 = (computed_value_8==2u);
          const bool value_enable3_2 = (computed_value_8==3u);
          const bool selected_value_8 = (value_enable0_2 ? ready_20 : (value_enable1_2 ? ready_20 : (value_enable2_2 ? ready_20 : (value_enable3_2 && ready_20))));
          const bool selected_value_10 = (condition_4 ? ((computed_value_2<uint64_t(8ULL)) && (condition_6 ? ready_20 : selected_value_8)) : ready_20);
          const uint32_t computed_value_10 = ((tag - 1u) & 3u);
          const bool value_enable0_3 = (computed_value_10==0u);
          const bool value_enable1_3 = (computed_value_10==1u);
          const bool value_enable2_3 = (computed_value_10==2u);
          const bool value_enable3_3 = (computed_value_10==3u);
          const bool selected_value_12 = (value_enable1_3 ? selected_value_10 : (value_enable2_3 ? selected_value_10 : (value_enable3_3 && selected_value_10)));
          const bool ready_24 = ((computed_value<uint64_t(8ULL)) && (condition_3 ? selected_value_10 : (value_enable0_3 ? selected_value_10 : selected_value_12)));
          const bool ready_27 = (add_done_empty && (mul_done_empty && (source_ready && (condition ? ready_24 : selected_value_10))));
          if (!(ready_27)) return;
          // Prepare every effect before publishing state operations.
          acir_sim::check((index<8u));
          const bool condition_11 = (tag!=committed_value);
          const bool check_fire = (condition && condition_11);
          if (check_fire) {
            const bool value_enable0_4 = (computed_value_10!=0u);
            const bool value_enable1_4 = (computed_value_10!=1u);
            const bool value_enable2_4 = (computed_value_10!=2u);
            const bool value_enable3_4 = (computed_value_10!=3u);
            acir_sim::check((!(value_enable0_4 && (value_enable1_4 && (value_enable2_4 && value_enable3_4)))));
          }
          acir_sim::check((index_2<8u));
          const bool condition_13 = (tag_2!=committed_value_2);
          const bool check_fire_2 = (condition_4 && condition_13);
          if (check_fire_2) {
            const bool value_enable0_5 = (computed_value_8!=0u);
            const bool value_enable1_5 = (computed_value_8!=1u);
            const bool value_enable2_5 = (computed_value_8!=2u);
            const bool value_enable3_5 = (computed_value_8!=3u);
            acir_sim::check((!(value_enable0_5 && (value_enable1_5 && (value_enable2_5 && value_enable3_5)))));
          }
          const bool value_enable0_6 = (computed_value_5!=0u);
          const bool value_enable1_6 = (computed_value_5!=1u);
          const bool value_enable2_6 = (computed_value_5!=2u);
          const bool write_fire = (value_enable0_6 && (value_enable1_6 && (value_enable2_6 && value_enable3)));
          RobEntry write_value{};
          if (write_fire) {
            const RobEntry payload = RobEntry{true,false,tag_3,rd,0u};
            write_value = payload;
          }
          const bool write_fire_2 = (value_enable0_6 && (value_enable1_6 && value_enable2));
          RobEntry write_value_2{};
          if (write_fire_2) {
            const RobEntry payload_2 = RobEntry{true,false,tag_3,rd,0u};
            write_value_2 = payload_2;
          }
          const bool write_fire_3 = (value_enable0_6 && value_enable1);
          RobEntry write_value_3{};
          if (write_fire_3) {
            const RobEntry payload_3 = RobEntry{true,false,tag_3,rd,0u};
            write_value_3 = payload_3;
          }
          const bool write_fire_4 = value_enable0;
          RobEntry write_value_4{};
          if (write_fire_4) {
            const RobEntry payload_4 = RobEntry{true,false,tag_3,rd,0u};
            write_value_4 = payload_4;
          }
          const uint32_t write_value_5 = (tag_3 + 1u);
          acir_sim::check((rd<8u));
          const bool write_fire_5 = condition_8;
          uint32_t write_value_6{};
          uint64_t resource_index{};
          if (write_fire_5) {
            write_value_6 = tag_3;
            resource_index = computed_value_6;
          }
          const bool write_fire_6 = (!ready_3);
          Entry write_value_7{};
          if (write_fire_6) {
            const RobEntry state_value_5 = ctx.top.rob.slots.value0.state.read();
            const RobEntry state_value_6 = ctx.top.rob.slots.value1.state.read();
            const RobEntry state_value_7 = ctx.top.rob.slots.value2.state.read();
            const RobEntry state_value_8 = ctx.top.rob.slots.value3.state.read();
            const RobEntry payload_5 = std::get<0>(ctx.compute_shared_computation_21());
            const RobEntry selected_value_18 = (value_enable1_3 ? state_value_6 : (value_enable2_3 ? state_value_7 : (value_enable3_3 ? state_value_8 : payload_5)));
            const RobEntry entry = (value_enable0_3 ? state_value_5 : selected_value_18);
            const bool ready_72 = ((entry.valid && entry.done) && (entry.tag==tag));
            const uint32_t registers_value = ((computed_value < 8) ? ctx.top.rf.registers.at(computed_value).read() : uint32_t{});
            const Operand payload_9 = Operand{true,0u,0u};
            const Operand payload_10 = std::get<0>(ctx.compute_shared_computation_14());
            const Operand selected_value_20 = ((index==0u) ? payload_9 : payload_10);
            const Operand selected_value_22 = (condition_11 ? (ready_72 ? Operand{true,tag,entry.value} : Operand{false,tag,0u}) : (condition_3 ? Operand{true,tag,registers_value} : selected_value_20));
            const Operand lhs = (condition ? selected_value_22 : selected_value_20);
            const RobEntry state_value_9 = ctx.top.rob.slots.value0.state.read();
            const RobEntry state_value_10 = ctx.top.rob.slots.value1.state.read();
            const RobEntry state_value_11 = ctx.top.rob.slots.value2.state.read();
            const RobEntry state_value_12 = ctx.top.rob.slots.value3.state.read();
            const RobEntry selected_value_25 = (value_enable1_2 ? state_value_10 : (value_enable2_2 ? state_value_11 : (value_enable3_2 ? state_value_12 : payload_5)));
            const RobEntry entry_2 = (value_enable0_2 ? state_value_9 : selected_value_25);
            const bool ready_76 = ((entry_2.valid && entry_2.done) && (entry_2.tag==tag_2));
            const uint32_t registers_value_2 = ((computed_value_2 < 8) ? ctx.top.rf.registers.at(computed_value_2).read() : uint32_t{});
            const Operand selected_value_27 = ((index_2==0u) ? payload_9 : payload_10);
            const Operand selected_value_29 = (condition_13 ? (ready_76 ? Operand{true,tag_2,entry_2.value} : Operand{false,tag_2,0u}) : (condition_6 ? Operand{true,tag_2,registers_value_2} : selected_value_27));
            const Operand rhs = (condition_4 ? selected_value_29 : selected_value_27);
            const Entry entry_3 = Entry{true,inst.kind,tag_3,rd,lhs,rhs};
            write_value_7 = entry_3;
          }
          const bool write_fire_7 = ready_3;
          Entry write_value_8{};
          if (write_fire_7) {
            const RobEntry state_value_13 = ctx.top.rob.slots.value0.state.read();
            const RobEntry state_value_14 = ctx.top.rob.slots.value1.state.read();
            const RobEntry state_value_15 = ctx.top.rob.slots.value2.state.read();
            const RobEntry state_value_16 = ctx.top.rob.slots.value3.state.read();
            const RobEntry payload_14 = std::get<0>(ctx.compute_shared_computation_21());
            const RobEntry selected_value_32 = (value_enable1_3 ? state_value_14 : (value_enable2_3 ? state_value_15 : (value_enable3_3 ? state_value_16 : payload_14)));
            const RobEntry entry_4 = (value_enable0_3 ? state_value_13 : selected_value_32);
            const bool ready_83 = ((entry_4.valid && entry_4.done) && (entry_4.tag==tag));
            const uint32_t registers_value_3 = ((computed_value < 8) ? ctx.top.rf.registers.at(computed_value).read() : uint32_t{});
            const Operand payload_18 = Operand{true,0u,0u};
            const Operand payload_19 = std::get<0>(ctx.compute_shared_computation_14());
            const Operand selected_value_34 = ((index==0u) ? payload_18 : payload_19);
            const Operand selected_value_36 = (condition_11 ? (ready_83 ? Operand{true,tag,entry_4.value} : Operand{false,tag,0u}) : (condition_3 ? Operand{true,tag,registers_value_3} : selected_value_34));
            const Operand lhs_2 = (condition ? selected_value_36 : selected_value_34);
            const RobEntry state_value_17 = ctx.top.rob.slots.value0.state.read();
            const RobEntry state_value_18 = ctx.top.rob.slots.value1.state.read();
            const RobEntry state_value_19 = ctx.top.rob.slots.value2.state.read();
            const RobEntry state_value_20 = ctx.top.rob.slots.value3.state.read();
            const RobEntry selected_value_39 = (value_enable1_2 ? state_value_18 : (value_enable2_2 ? state_value_19 : (value_enable3_2 ? state_value_20 : payload_14)));
            const RobEntry entry_5 = (value_enable0_2 ? state_value_17 : selected_value_39);
            const bool ready_87 = ((entry_5.valid && entry_5.done) && (entry_5.tag==tag_2));
            const uint32_t registers_value_4 = ((computed_value_2 < 8) ? ctx.top.rf.registers.at(computed_value_2).read() : uint32_t{});
            const Operand selected_value_41 = ((index_2==0u) ? payload_18 : payload_19);
            const Operand selected_value_43 = (condition_13 ? (ready_87 ? Operand{true,tag_2,entry_5.value} : Operand{false,tag_2,0u}) : (condition_6 ? Operand{true,tag_2,registers_value_4} : selected_value_41));
            const Operand rhs_2 = (condition_4 ? selected_value_43 : selected_value_41);
            const Entry entry_6 = Entry{true,inst.kind,tag_3,rd,lhs_2,rhs_2};
            write_value_8 = entry_6;
          }
          ctx.top.source.pop();
          if (write_fire) { ctx.top.rob.slots.value3.state.write(write_value); }
          if (write_fire_2) { ctx.top.rob.slots.value2.state.write(write_value_2); }
          if (write_fire_3) { ctx.top.rob.slots.value1.state.write(write_value_3); }
          if (write_fire_4) { ctx.top.rob.slots.value0.state.write(write_value_4); }
          ctx.top.rob.tail.write(write_value_5);
          if (write_fire_5) { ctx.top.rf.renamed.at(resource_index).write(write_value_6); }
          if (write_fire_6) { ctx.top.left.slot.write(write_value_7); }
          if (write_fire_7) { ctx.top.right.slot.write(write_value_8); }
          ctx.fired_top_dispatch_dispatch=true;
        }
      } dispatch;
      struct ReservationStation {
        Cell<Entry> slot{};
      } left;
      struct Multiplier {
        Cell<bool> active{};
        Cell<uint32_t> remaining{};
        Cell<Entry> request{};
        // Rule top.multiplier.accept#3
        template<class Context> void try_accept(Context &ctx) {
          const bool active_value = std::get<0>(ctx.compute_active_view());
          const bool ready = (!active_value);
          const bool mul_requests_ready = (!ctx.top.mul_requests.empty());
          if (!((ready && mul_requests_ready))) return;
          // Prepare every effect before publishing state operations.
          const Entry mul_requests_value = ctx.top.mul_requests.read();
          const Entry write_value = mul_requests_value;
          const uint32_t write_value_2 = 2u;
          const bool write_value_3 = true;
          ctx.top.mul_requests.pop();
          ctx.top.multiplier.request.write(write_value);
          ctx.top.multiplier.remaining.write(write_value_2);
          ctx.top.multiplier.active.write(write_value_3);
          ctx.fired_top_multiplier_accept=true;
        }
        // Rule top.multiplier.advance#4
        template<class Context> void try_advance(Context &ctx) {
          const bool active_value = std::get<0>(ctx.compute_active_view());
          const uint32_t remaining_value = ctx.top.multiplier.remaining.read();
          const bool condition = (remaining_value!=0u);
          const bool mul_done_full = ctx.top.mul_done.full();
          if (!((active_value && (condition || (!mul_done_full))))) return;
          // Prepare every effect before publishing state operations.
          const bool write_fire = condition;
          uint32_t write_value{};
          if (write_fire) {
            const uint32_t remaining_value_2 = ctx.top.multiplier.remaining.read();
            write_value = (remaining_value_2 - 1u);
          }
          const bool condition_2 = (remaining_value==0u);
          const bool write_fire_2 = condition_2;
          bool write_value_2{};
          if (write_fire_2) {
            write_value_2 = false;
          }
          const bool push_fire = condition_2;
          Completion push_value{};
          if (push_fire) {
            const Entry request_value = ctx.top.multiplier.request.read();
            const Entry entry = request_value;
            push_value = (condition_2 ? Completion{entry.tag,entry.rd,(entry.lhs.value * entry.rhs.value)} : Completion{});
          }
          if (write_fire) { ctx.top.multiplier.remaining.write(write_value); }
          if (write_fire_2) { ctx.top.multiplier.active.write(write_value_2); }
          if (push_fire) { ctx.top.mul_done.push(push_value); }
          ctx.fired_top_multiplier_advance=true;
        }
      } multiplier;
      struct RegisterFile {
        std::array<Cell<uint32_t>,8> renamed{};
        std::array<Cell<uint32_t>,8> committed{};
        std::array<Cell<uint32_t>,8> registers{};
      } rf;
      struct ReservationStation_2 {
        Cell<Entry> slot{};
      } right;
      struct ReorderBuffer {
        struct slots {
          struct RobSlot {
            Cell<RobEntry> state{};
          } value0;
          struct RobSlot_2 {
            Cell<RobEntry> state{};
          } value1;
          struct RobSlot_3 {
            Cell<RobEntry> state{};
          } value2;
          struct RobSlot_4 {
            Cell<RobEntry> state{};
          } value3;
        } slots;
        Cell<uint32_t> head{};
        Cell<uint32_t> tail{};
      } rob;
      struct Scheduler {
        // Rule top.scheduler.issue_mul#1
        template<class Context> void try_issue_mul(Context &ctx) {
          const bool ready = std::get<0>(ctx.compute_shared_computation());
          const bool ready_2 = std::get<0>(ctx.compute_shared_computation_2());
          const bool ready_3 = std::get<0>(ctx.compute_shared_computation_5());
          const uint32_t kind = std::get<0>(ctx.compute_shared_computation_11());
          const bool condition = (kind==1u);
          const bool ready_4 = std::get<0>(ctx.compute_shared_computation_6());
          const bool ready_5 = std::get<0>(ctx.compute_shared_computation_7());
          const bool ready_6 = std::get<0>(ctx.compute_shared_computation_10());
          const uint32_t kind_2 = std::get<0>(ctx.compute_shared_computation_12());
          const bool condition_2 = (kind_2==1u);
          const bool condition_3 = std::get<0>(ctx.compute_shared_computation_13());
          const Entry slot_value = ctx.top.left.slot.read();
          const Entry entry = slot_value;
          const bool ready_7 = entry.valid;
          const bool condition_4 = (entry.kind==1u);
          const bool ready_8 = entry.lhs.ready;
          const bool ready_9 = entry.rhs.ready;
          const bool mul_requests_full = ctx.top.mul_requests.full();
          const bool ready_10 = (!mul_requests_full);
          const bool ready_14 = (ready_7 && (condition_4 && (ready_8 && (ready_9 && ready_10))));
          const Entry slot_value_2 = ctx.top.right.slot.read();
          const Entry entry_2 = slot_value_2;
          const bool ready_15 = entry_2.valid;
          const bool condition_5 = (entry_2.kind==1u);
          const bool ready_16 = entry_2.lhs.ready;
          const bool ready_17 = entry_2.rhs.ready;
          const bool ready_21 = (ready_15 && (condition_5 && (ready_16 && (ready_17 && ready_10))));
          const bool selected_value_4 = (ready_5 ? (ready_6 ? (condition_2 ? (condition_3 ? ready_14 : ready_21) : ready_14) : ready_14) : ready_14);
          const bool selected_value_8 = (ready_2 ? (ready_3 ? (condition ? (ready_4 ? selected_value_4 : ready_14) : ready_21) : ready_21) : ready_21);
          if (!((ready ? selected_value_8 : ready_21))) return;
          // Prepare every effect before publishing state operations.
          const bool ready_22 = std::get<0>(ctx.compute_shared_computation_4());
          const bool ready_23 = std::get<0>(ctx.compute_shared_computation_16());
          const bool ready_24 = std::get<0>(ctx.compute_shared_computation_17());
          const bool condition_7 = std::get<0>(ctx.compute_shared_computation_18());
          const bool ready_31 = (ready_23 || (ready_24 || ((kind!=1u) || (ready_4 && (ready_5 && (ready_6 && (condition_2 && condition_7)))))));
          const bool write_fire = (ready_22 || ready_31);
          Entry write_value{};
          if (write_fire) {
            write_value = std::get<0>(ctx.compute_shared_computation_15());
          }
          const bool ready_34 = std::get<0>(ctx.compute_shared_computation_9());
          const bool ready_35 = std::get<0>(ctx.compute_shared_computation_19());
          const bool ready_36 = std::get<0>(ctx.compute_shared_computation_20());
          const bool ready_43 = (ready_2 && (ready_3 && (condition && (ready_34 || (ready_35 || (ready_36 || ((kind_2!=1u) || condition_3)))))));
          const bool write_fire_2 = (ready && ready_43);
          Entry write_value_2{};
          if (write_fire_2) {
            write_value_2 = std::get<0>(ctx.compute_shared_computation_15());
          }
          const bool a_ready = std::get<0>(ctx.compute_a_ready());
          const bool a_mul = (a_ready && condition);
          const bool b_ready = std::get<0>(ctx.compute_b_ready());
          const bool b_mul = (b_ready && condition_2);
          const bool choose_mul_left = (a_mul && ((!b_mul) || condition_3));
          const Entry slot_value_3 = ctx.top.left.slot.read();
          const Entry entry_3 = slot_value_3;
          const Entry slot_value_4 = ctx.top.right.slot.read();
          const Entry entry_4 = slot_value_4;
          const Entry push_value = (choose_mul_left ? entry_3 : entry_4);
          if (write_fire) { ctx.top.right.slot.write(write_value); }
          if (write_fire_2) { ctx.top.left.slot.write(write_value_2); }
          ctx.top.mul_requests.push(push_value);
          ctx.fired_top_scheduler_issue_mul=true;
        }
        // Rule top.scheduler.issue_add#0
        template<class Context> void try_issue_add(Context &ctx) {
          const bool ready = std::get<0>(ctx.compute_shared_computation());
          const bool ready_2 = std::get<0>(ctx.compute_shared_computation_2());
          const bool ready_3 = std::get<0>(ctx.compute_shared_computation_5());
          const uint32_t kind = std::get<0>(ctx.compute_shared_computation_11());
          const bool condition = (kind==0u);
          const bool ready_4 = std::get<0>(ctx.compute_shared_computation_6());
          const bool ready_5 = std::get<0>(ctx.compute_shared_computation_7());
          const bool ready_6 = std::get<0>(ctx.compute_shared_computation_10());
          const uint32_t kind_2 = std::get<0>(ctx.compute_shared_computation_12());
          const bool condition_2 = (kind_2==0u);
          const bool condition_3 = std::get<0>(ctx.compute_shared_computation_13());
          const Entry slot_value = ctx.top.left.slot.read();
          const Entry entry = slot_value;
          const bool ready_7 = entry.valid;
          const bool condition_4 = (entry.kind==0u);
          const bool ready_8 = entry.lhs.ready;
          const bool ready_9 = entry.rhs.ready;
          const bool add_requests_full = ctx.top.add_requests.full();
          const bool ready_10 = (!add_requests_full);
          const bool ready_14 = (ready_7 && (condition_4 && (ready_8 && (ready_9 && ready_10))));
          const Entry slot_value_2 = ctx.top.right.slot.read();
          const Entry entry_2 = slot_value_2;
          const bool ready_15 = entry_2.valid;
          const bool condition_5 = (entry_2.kind==0u);
          const bool ready_16 = entry_2.lhs.ready;
          const bool ready_17 = entry_2.rhs.ready;
          const bool ready_21 = (ready_15 && (condition_5 && (ready_16 && (ready_17 && ready_10))));
          const bool selected_value_4 = (ready_5 ? (ready_6 ? (condition_2 ? (condition_3 ? ready_14 : ready_21) : ready_14) : ready_14) : ready_14);
          const bool selected_value_8 = (ready_2 ? (ready_3 ? (condition ? (ready_4 ? selected_value_4 : ready_14) : ready_21) : ready_21) : ready_21);
          if (!((ready ? selected_value_8 : ready_21))) return;
          // Prepare every effect before publishing state operations.
          const bool ready_22 = std::get<0>(ctx.compute_shared_computation_4());
          const bool ready_23 = std::get<0>(ctx.compute_shared_computation_16());
          const bool ready_24 = std::get<0>(ctx.compute_shared_computation_17());
          const bool condition_7 = std::get<0>(ctx.compute_shared_computation_18());
          const bool ready_31 = (ready_23 || (ready_24 || ((kind!=0u) || (ready_4 && (ready_5 && (ready_6 && (condition_2 && condition_7)))))));
          const bool write_fire = (ready_22 || ready_31);
          Entry write_value{};
          if (write_fire) {
            write_value = std::get<0>(ctx.compute_shared_computation_15());
          }
          const bool ready_34 = std::get<0>(ctx.compute_shared_computation_9());
          const bool ready_35 = std::get<0>(ctx.compute_shared_computation_19());
          const bool ready_36 = std::get<0>(ctx.compute_shared_computation_20());
          const bool ready_43 = (ready_2 && (ready_3 && (condition && (ready_34 || (ready_35 || (ready_36 || ((kind_2!=0u) || condition_3)))))));
          const bool write_fire_2 = (ready && ready_43);
          Entry write_value_2{};
          if (write_fire_2) {
            write_value_2 = std::get<0>(ctx.compute_shared_computation_15());
          }
          const bool a_ready = std::get<0>(ctx.compute_a_ready());
          const bool a_add = (a_ready && condition);
          const bool b_ready = std::get<0>(ctx.compute_b_ready());
          const bool b_add = (b_ready && condition_2);
          const bool choose_add_left = (a_add && ((!b_add) || condition_3));
          const Entry slot_value_3 = ctx.top.left.slot.read();
          const Entry entry_3 = slot_value_3;
          const Entry slot_value_4 = ctx.top.right.slot.read();
          const Entry entry_4 = slot_value_4;
          const Entry push_value = (choose_add_left ? entry_3 : entry_4);
          if (write_fire) { ctx.top.right.slot.write(write_value); }
          if (write_fire_2) { ctx.top.left.slot.write(write_value_2); }
          ctx.top.add_requests.push(push_value);
          ctx.fired_top_scheduler_issue_add=true;
        }
      } scheduler;
      Queue<Completion,1> add_done{};
      Queue<Entry,1> add_requests{};
      Signal<Completion> event{};
      Queue<Completion,1> mul_done{};
      Queue<Entry,1> mul_requests{};
      Queue<MicroOp,4> source{};
    } top;
    std::optional<std::tuple<Entry,bool>> compute_slot_view_cache;
    std::optional<std::tuple<Entry,bool>> compute_slot_view_2_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_2_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_3_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_4_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_5_cache;
    std::optional<std::tuple<bool>> compute_a_ready_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_6_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_7_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_8_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_9_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_10_cache;
    std::optional<std::tuple<bool>> compute_b_ready_cache;
    std::optional<std::tuple<uint32_t>> compute_shared_computation_11_cache;
    std::optional<std::tuple<uint32_t>> compute_shared_computation_12_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_13_cache;
    std::optional<std::tuple<Operand>> compute_shared_computation_14_cache;
    std::optional<std::tuple<Entry>> compute_shared_computation_15_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_16_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_17_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_18_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_19_cache;
    std::optional<std::tuple<bool>> compute_shared_computation_20_cache;
    std::optional<std::tuple<bool,bool>> compute_active_view_cache;
    std::optional<std::tuple<RobEntry>> compute_shared_computation_21_cache;
    bool fired_top_multiplier_accept=false;
    bool fired_top_scheduler_issue_mul=false;
    bool fired_top_cdb_broadcast=false;
    bool fired_top_adder_execute=false;
    bool fired_top_scheduler_issue_add=false;
    bool fired_top_multiplier_advance=false;
    bool fired_top_dispatch_dispatch=false;
    bool fired_top_commit_commit=false;
    bool fired_top_audit_observe=false;
    Model() { reset(); }
    void reset() {
      top.add_done={};
      top.add_done.count=0;
      top.add_requests={};
      top.add_requests.count=0;
      top.audit.last_tag={};
      top.audit.last_tag.value=0u;
      top.commit.last_tag={};
      top.commit.last_tag.value=0u;
      top.commit.retired={};
      top.commit.retired.value=0u;
      top.event={};
      top.event.value=Completion{0u,0u,0u};
      top.left.slot={};
      top.left.slot.value=Entry{false,0u,0u,0u,Operand{false,0u,0u},Operand{false,0u,0u}};
      top.mul_done={};
      top.mul_done.count=0;
      top.mul_requests={};
      top.mul_requests.count=0;
      top.multiplier.active={};
      top.multiplier.active.value=false;
      top.multiplier.remaining={};
      top.multiplier.remaining.value=0u;
      top.multiplier.request={};
      top.multiplier.request.value=Entry{false,0u,0u,0u,Operand{false,0u,0u},Operand{false,0u,0u}};
      top.rf.committed[0]={};
      top.rf.committed[0].value=0u;
      top.rf.committed[1]={};
      top.rf.committed[1].value=0u;
      top.rf.committed[2]={};
      top.rf.committed[2].value=0u;
      top.rf.committed[3]={};
      top.rf.committed[3].value=0u;
      top.rf.committed[4]={};
      top.rf.committed[4].value=0u;
      top.rf.committed[5]={};
      top.rf.committed[5].value=0u;
      top.rf.committed[6]={};
      top.rf.committed[6].value=0u;
      top.rf.committed[7]={};
      top.rf.committed[7].value=0u;
      top.rf.registers[0]={};
      top.rf.registers[0].value=0u;
      top.rf.registers[1]={};
      top.rf.registers[1].value=0u;
      top.rf.registers[2]={};
      top.rf.registers[2].value=6u;
      top.rf.registers[3]={};
      top.rf.registers[3].value=7u;
      top.rf.registers[4]={};
      top.rf.registers[4].value=0u;
      top.rf.registers[5]={};
      top.rf.registers[5].value=0u;
      top.rf.registers[6]={};
      top.rf.registers[6].value=0u;
      top.rf.registers[7]={};
      top.rf.registers[7].value=0u;
      top.rf.renamed[0]={};
      top.rf.renamed[0].value=0u;
      top.rf.renamed[1]={};
      top.rf.renamed[1].value=0u;
      top.rf.renamed[2]={};
      top.rf.renamed[2].value=0u;
      top.rf.renamed[3]={};
      top.rf.renamed[3].value=0u;
      top.rf.renamed[4]={};
      top.rf.renamed[4].value=0u;
      top.rf.renamed[5]={};
      top.rf.renamed[5].value=0u;
      top.rf.renamed[6]={};
      top.rf.renamed[6].value=0u;
      top.rf.renamed[7]={};
      top.rf.renamed[7].value=0u;
      top.right.slot={};
      top.right.slot.value=Entry{false,0u,0u,0u,Operand{false,0u,0u},Operand{false,0u,0u}};
      top.rob.head={};
      top.rob.head.value=1u;
      top.rob.slots.value0.state={};
      top.rob.slots.value0.state.value=RobEntry{false,false,0u,0u,0u};
      top.rob.slots.value1.state={};
      top.rob.slots.value1.state.value=RobEntry{false,false,0u,0u,0u};
      top.rob.slots.value2.state={};
      top.rob.slots.value2.state.value=RobEntry{false,false,0u,0u,0u};
      top.rob.slots.value3.state={};
      top.rob.slots.value3.state.value=RobEntry{false,false,0u,0u,0u};
      top.rob.tail={};
      top.rob.tail.value=1u;
      top.source={};
      top.source.data[0]=MicroOp{1u,1u,2u,3u};
      top.source.data[1]=MicroOp{0u,4u,1u,2u};
      top.source.data[2]=MicroOp{0u,5u,2u,3u};
      top.source.data[3]=MicroOp{0u,6u,4u,5u};
      top.source.count=4;
      begin_tick();
    }
    void begin_tick() {
      top.add_done.begin();
      top.add_requests.begin();
      top.audit.last_tag.begin();
      top.commit.last_tag.begin();
      top.commit.retired.begin();
      top.event.begin();
      top.left.slot.begin();
      top.mul_done.begin();
      top.mul_requests.begin();
      top.multiplier.active.begin();
      top.multiplier.remaining.begin();
      top.multiplier.request.begin();
      top.rf.committed[0].begin();
      top.rf.committed[1].begin();
      top.rf.committed[2].begin();
      top.rf.committed[3].begin();
      top.rf.committed[4].begin();
      top.rf.committed[5].begin();
      top.rf.committed[6].begin();
      top.rf.committed[7].begin();
      top.rf.registers[0].begin();
      top.rf.registers[1].begin();
      top.rf.registers[2].begin();
      top.rf.registers[3].begin();
      top.rf.registers[4].begin();
      top.rf.registers[5].begin();
      top.rf.registers[6].begin();
      top.rf.registers[7].begin();
      top.rf.renamed[0].begin();
      top.rf.renamed[1].begin();
      top.rf.renamed[2].begin();
      top.rf.renamed[3].begin();
      top.rf.renamed[4].begin();
      top.rf.renamed[5].begin();
      top.rf.renamed[6].begin();
      top.rf.renamed[7].begin();
      top.right.slot.begin();
      top.rob.head.begin();
      top.rob.slots.value0.state.begin();
      top.rob.slots.value1.state.begin();
      top.rob.slots.value2.state.begin();
      top.rob.slots.value3.state.begin();
      top.rob.tail.begin();
      top.source.begin();
      compute_slot_view_cache.reset();
      compute_slot_view_2_cache.reset();
      compute_shared_computation_cache.reset();
      compute_shared_computation_2_cache.reset();
      compute_shared_computation_3_cache.reset();
      compute_shared_computation_4_cache.reset();
      compute_shared_computation_5_cache.reset();
      compute_a_ready_cache.reset();
      compute_shared_computation_6_cache.reset();
      compute_shared_computation_7_cache.reset();
      compute_shared_computation_8_cache.reset();
      compute_shared_computation_9_cache.reset();
      compute_shared_computation_10_cache.reset();
      compute_b_ready_cache.reset();
      compute_shared_computation_11_cache.reset();
      compute_shared_computation_12_cache.reset();
      compute_shared_computation_13_cache.reset();
      compute_shared_computation_14_cache.reset();
      compute_shared_computation_15_cache.reset();
      compute_shared_computation_16_cache.reset();
      compute_shared_computation_17_cache.reset();
      compute_shared_computation_18_cache.reset();
      compute_shared_computation_19_cache.reset();
      compute_shared_computation_20_cache.reset();
      compute_active_view_cache.reset();
      compute_shared_computation_21_cache.reset();
      fired_top_multiplier_accept=false;
      fired_top_scheduler_issue_mul=false;
      fired_top_cdb_broadcast=false;
      fired_top_adder_execute=false;
      fired_top_scheduler_issue_add=false;
      fired_top_multiplier_advance=false;
      fired_top_dispatch_dispatch=false;
      fired_top_commit_commit=false;
      fired_top_audit_observe=false;
    }
    void commit() {
      top.add_done.commit();
      top.add_requests.commit();
      top.audit.last_tag.commit();
      top.commit.last_tag.commit();
      top.commit.retired.commit();
      top.event.commit();
      top.left.slot.commit();
      top.mul_done.commit();
      top.mul_requests.commit();
      top.multiplier.active.commit();
      top.multiplier.remaining.commit();
      top.multiplier.request.commit();
      top.rf.committed[0].commit();
      top.rf.committed[1].commit();
      top.rf.committed[2].commit();
      top.rf.committed[3].commit();
      top.rf.committed[4].commit();
      top.rf.committed[5].commit();
      top.rf.committed[6].commit();
      top.rf.committed[7].commit();
      top.rf.registers[0].commit();
      top.rf.registers[1].commit();
      top.rf.registers[2].commit();
      top.rf.registers[3].commit();
      top.rf.registers[4].commit();
      top.rf.registers[5].commit();
      top.rf.registers[6].commit();
      top.rf.registers[7].commit();
      top.rf.renamed[0].commit();
      top.rf.renamed[1].commit();
      top.rf.renamed[2].commit();
      top.rf.renamed[3].commit();
      top.rf.renamed[4].commit();
      top.rf.renamed[5].commit();
      top.rf.renamed[6].commit();
      top.rf.renamed[7].commit();
      top.right.slot.commit();
      top.rob.head.commit();
      top.rob.slots.value0.state.commit();
      top.rob.slots.value1.state.commit();
      top.rob.slots.value2.state.commit();
      top.rob.slots.value3.state.commit();
      top.rob.tail.commit();
      top.source.commit();
    }
    void tick() {
      begin_tick();
      top.multiplier.try_accept(*this);
      top.scheduler.try_issue_mul(*this);
      top.cdb.try_broadcast(*this);
      top.adder.try_execute(*this);
      top.scheduler.try_issue_add(*this);
      top.multiplier.try_advance(*this);
      top.dispatch.try_dispatch(*this);
      top.commit.try_commit(*this);
      top.audit.try_observe(*this);
      commit();
    }
    void trace(std::ostream &o,uint64_t tick) const { o<<"{\"tick\":"<<tick<<",\"fire\":{";
      o<<"\"top.multiplier.accept#3\":"; json(o,fired_top_multiplier_accept);
      o<<',';
      o<<"\"top.scheduler.issue_mul#1\":"; json(o,fired_top_scheduler_issue_mul);
      o<<',';
      o<<"\"top.cdb.broadcast#5\":"; json(o,fired_top_cdb_broadcast);
      o<<',';
      o<<"\"top.adder.execute#2\":"; json(o,fired_top_adder_execute);
      o<<',';
      o<<"\"top.scheduler.issue_add#0\":"; json(o,fired_top_scheduler_issue_add);
      o<<',';
      o<<"\"top.multiplier.advance#4\":"; json(o,fired_top_multiplier_advance);
      o<<',';
      o<<"\"top.dispatch.dispatch#6\":"; json(o,fired_top_dispatch_dispatch);
      o<<',';
      o<<"\"top.commit.commit#7\":"; json(o,fired_top_commit_commit);
      o<<',';
      o<<"\"top.audit.observe#8\":"; json(o,fired_top_audit_observe);
      o<<"},\"state\":{";
      o<<"\"top.add_done\":"; json(o,top.add_done);
      o<<',';
      o<<"\"top.add_requests\":"; json(o,top.add_requests);
      o<<',';
      o<<"\"top.audit.last_tag\":"; json(o,top.audit.last_tag.value);
      o<<',';
      o<<"\"top.commit.last_tag\":"; json(o,top.commit.last_tag.value);
      o<<',';
      o<<"\"top.commit.retired\":"; json(o,top.commit.retired.value);
      o<<',';
      o<<"\"top.left.slot\":"; json(o,top.left.slot.value);
      o<<',';
      o<<"\"top.mul_done\":"; json(o,top.mul_done);
      o<<',';
      o<<"\"top.mul_requests\":"; json(o,top.mul_requests);
      o<<',';
      o<<"\"top.multiplier.active\":"; json(o,top.multiplier.active.value);
      o<<',';
      o<<"\"top.multiplier.remaining\":"; json(o,top.multiplier.remaining.value);
      o<<',';
      o<<"\"top.multiplier.request\":"; json(o,top.multiplier.request.value);
      o<<',';
      o<<"\"top.rf.committed.0\":"; json(o,top.rf.committed[0].value);
      o<<',';
      o<<"\"top.rf.committed.1\":"; json(o,top.rf.committed[1].value);
      o<<',';
      o<<"\"top.rf.committed.2\":"; json(o,top.rf.committed[2].value);
      o<<',';
      o<<"\"top.rf.committed.3\":"; json(o,top.rf.committed[3].value);
      o<<',';
      o<<"\"top.rf.committed.4\":"; json(o,top.rf.committed[4].value);
      o<<',';
      o<<"\"top.rf.committed.5\":"; json(o,top.rf.committed[5].value);
      o<<',';
      o<<"\"top.rf.committed.6\":"; json(o,top.rf.committed[6].value);
      o<<',';
      o<<"\"top.rf.committed.7\":"; json(o,top.rf.committed[7].value);
      o<<',';
      o<<"\"top.rf.registers.0\":"; json(o,top.rf.registers[0].value);
      o<<',';
      o<<"\"top.rf.registers.1\":"; json(o,top.rf.registers[1].value);
      o<<',';
      o<<"\"top.rf.registers.2\":"; json(o,top.rf.registers[2].value);
      o<<',';
      o<<"\"top.rf.registers.3\":"; json(o,top.rf.registers[3].value);
      o<<',';
      o<<"\"top.rf.registers.4\":"; json(o,top.rf.registers[4].value);
      o<<',';
      o<<"\"top.rf.registers.5\":"; json(o,top.rf.registers[5].value);
      o<<',';
      o<<"\"top.rf.registers.6\":"; json(o,top.rf.registers[6].value);
      o<<',';
      o<<"\"top.rf.registers.7\":"; json(o,top.rf.registers[7].value);
      o<<',';
      o<<"\"top.rf.renamed.0\":"; json(o,top.rf.renamed[0].value);
      o<<',';
      o<<"\"top.rf.renamed.1\":"; json(o,top.rf.renamed[1].value);
      o<<',';
      o<<"\"top.rf.renamed.2\":"; json(o,top.rf.renamed[2].value);
      o<<',';
      o<<"\"top.rf.renamed.3\":"; json(o,top.rf.renamed[3].value);
      o<<',';
      o<<"\"top.rf.renamed.4\":"; json(o,top.rf.renamed[4].value);
      o<<',';
      o<<"\"top.rf.renamed.5\":"; json(o,top.rf.renamed[5].value);
      o<<',';
      o<<"\"top.rf.renamed.6\":"; json(o,top.rf.renamed[6].value);
      o<<',';
      o<<"\"top.rf.renamed.7\":"; json(o,top.rf.renamed[7].value);
      o<<',';
      o<<"\"top.right.slot\":"; json(o,top.right.slot.value);
      o<<',';
      o<<"\"top.rob.head\":"; json(o,top.rob.head.value);
      o<<',';
      o<<"\"top.rob.slots.0.state\":"; json(o,top.rob.slots.value0.state.value);
      o<<',';
      o<<"\"top.rob.slots.1.state\":"; json(o,top.rob.slots.value1.state.value);
      o<<',';
      o<<"\"top.rob.slots.2.state\":"; json(o,top.rob.slots.value2.state.value);
      o<<',';
      o<<"\"top.rob.slots.3.state\":"; json(o,top.rob.slots.value3.state.value);
      o<<',';
      o<<"\"top.rob.tail\":"; json(o,top.rob.tail.value);
      o<<',';
      o<<"\"top.source\":"; json(o,top.source);
      o<<"}}"; }
    const std::tuple<Entry,bool> &compute_slot_view() {
      if (!compute_slot_view_cache) {
        const Entry slot_value = top.left.slot.read();
        compute_slot_view_cache.emplace(std::tuple<Entry,bool>{slot_value,true});
      }
      return *compute_slot_view_cache;
    }
    const std::tuple<Entry,bool> &compute_slot_view_2() {
      if (!compute_slot_view_2_cache) {
        const Entry slot_value = top.right.slot.read();
        compute_slot_view_2_cache.emplace(std::tuple<Entry,bool>{slot_value,true});
      }
      return *compute_slot_view_2_cache;
    }
    const std::tuple<bool> &compute_shared_computation() {
      if (!compute_shared_computation_cache) {
        const Entry slot_value = std::get<0>(compute_slot_view());
        const Entry a = slot_value;
        const bool ready = a.valid;
        compute_shared_computation_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_cache;
    }
    const std::tuple<bool> &compute_shared_computation_2() {
      if (!compute_shared_computation_2_cache) {
        const Entry slot_value = std::get<0>(compute_slot_view());
        const Entry a = slot_value;
        const bool ready = a.lhs.ready;
        compute_shared_computation_2_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_2_cache;
    }
    const std::tuple<bool> &compute_shared_computation_3() {
      if (!compute_shared_computation_3_cache) {
        const bool ready = std::get<0>(compute_shared_computation());
        const bool ready_2 = std::get<0>(compute_shared_computation_2());
        compute_shared_computation_3_cache.emplace(std::tuple<bool>{(ready && ready_2)});
      }
      return *compute_shared_computation_3_cache;
    }
    const std::tuple<bool> &compute_shared_computation_4() {
      if (!compute_shared_computation_4_cache) {
        const bool ready = std::get<0>(compute_shared_computation());
        const bool ready_2 = (!ready);
        compute_shared_computation_4_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_4_cache;
    }
    const std::tuple<bool> &compute_shared_computation_5() {
      if (!compute_shared_computation_5_cache) {
        const Entry slot_value = std::get<0>(compute_slot_view());
        const Entry a = slot_value;
        const bool ready = a.rhs.ready;
        compute_shared_computation_5_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_5_cache;
    }
    const std::tuple<bool> &compute_a_ready() {
      if (!compute_a_ready_cache) {
        const bool ready = std::get<0>(compute_shared_computation_3());
        const bool ready_2 = std::get<0>(compute_shared_computation_5());
        const bool a_ready = (ready && ready_2);
        compute_a_ready_cache.emplace(std::tuple<bool>{a_ready});
      }
      return *compute_a_ready_cache;
    }
    const std::tuple<bool> &compute_shared_computation_6() {
      if (!compute_shared_computation_6_cache) {
        const Entry slot_value = std::get<0>(compute_slot_view_2());
        const Entry b = slot_value;
        const bool ready = b.valid;
        compute_shared_computation_6_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_6_cache;
    }
    const std::tuple<bool> &compute_shared_computation_7() {
      if (!compute_shared_computation_7_cache) {
        const Entry slot_value = std::get<0>(compute_slot_view_2());
        const Entry b = slot_value;
        const bool ready = b.lhs.ready;
        compute_shared_computation_7_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_7_cache;
    }
    const std::tuple<bool> &compute_shared_computation_8() {
      if (!compute_shared_computation_8_cache) {
        const bool ready = std::get<0>(compute_shared_computation_6());
        const bool ready_2 = std::get<0>(compute_shared_computation_7());
        compute_shared_computation_8_cache.emplace(std::tuple<bool>{(ready && ready_2)});
      }
      return *compute_shared_computation_8_cache;
    }
    const std::tuple<bool> &compute_shared_computation_9() {
      if (!compute_shared_computation_9_cache) {
        const bool ready = std::get<0>(compute_shared_computation_6());
        const bool ready_2 = (!ready);
        compute_shared_computation_9_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_9_cache;
    }
    const std::tuple<bool> &compute_shared_computation_10() {
      if (!compute_shared_computation_10_cache) {
        const Entry slot_value = std::get<0>(compute_slot_view_2());
        const Entry b = slot_value;
        const bool ready = b.rhs.ready;
        compute_shared_computation_10_cache.emplace(std::tuple<bool>{ready});
      }
      return *compute_shared_computation_10_cache;
    }
    const std::tuple<bool> &compute_b_ready() {
      if (!compute_b_ready_cache) {
        const bool ready = std::get<0>(compute_shared_computation_8());
        const bool ready_2 = std::get<0>(compute_shared_computation_10());
        const bool b_ready = (ready && ready_2);
        compute_b_ready_cache.emplace(std::tuple<bool>{b_ready});
      }
      return *compute_b_ready_cache;
    }
    const std::tuple<uint32_t> &compute_shared_computation_11() {
      if (!compute_shared_computation_11_cache) {
        const Entry slot_value = std::get<0>(compute_slot_view());
        const Entry a = slot_value;
        const uint32_t kind = a.kind;
        compute_shared_computation_11_cache.emplace(std::tuple<uint32_t>{kind});
      }
      return *compute_shared_computation_11_cache;
    }
    const std::tuple<uint32_t> &compute_shared_computation_12() {
      if (!compute_shared_computation_12_cache) {
        const Entry slot_value = std::get<0>(compute_slot_view_2());
        const Entry b = slot_value;
        const uint32_t kind = b.kind;
        compute_shared_computation_12_cache.emplace(std::tuple<uint32_t>{kind});
      }
      return *compute_shared_computation_12_cache;
    }
    const std::tuple<bool> &compute_shared_computation_13() {
      if (!compute_shared_computation_13_cache) {
        const Entry slot_value = std::get<0>(compute_slot_view());
        const Entry a = slot_value;
        const uint32_t tag = a.tag;
        const Entry slot_value_2 = std::get<0>(compute_slot_view_2());
        const Entry b = slot_value_2;
        const uint32_t tag_2 = b.tag;
        const bool condition = (tag<tag_2);
        compute_shared_computation_13_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_13_cache;
    }
    const std::tuple<Operand> &compute_shared_computation_14() {
      if (!compute_shared_computation_14_cache) {
        const Operand payload = Operand{};
        compute_shared_computation_14_cache.emplace(std::tuple<Operand>{payload});
      }
      return *compute_shared_computation_14_cache;
    }
    const std::tuple<Entry> &compute_shared_computation_15() {
      if (!compute_shared_computation_15_cache) {
        const Operand payload = std::get<0>(compute_shared_computation_14());
        const Entry payload_2 = Entry{false,0u,0u,0u,payload,payload};
        compute_shared_computation_15_cache.emplace(std::tuple<Entry>{payload_2});
      }
      return *compute_shared_computation_15_cache;
    }
    const std::tuple<bool> &compute_shared_computation_16() {
      if (!compute_shared_computation_16_cache) {
        const bool ready = std::get<0>(compute_shared_computation_2());
        const bool ready_2 = (!ready);
        compute_shared_computation_16_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_16_cache;
    }
    const std::tuple<bool> &compute_shared_computation_17() {
      if (!compute_shared_computation_17_cache) {
        const bool ready = std::get<0>(compute_shared_computation_5());
        const bool ready_2 = (!ready);
        compute_shared_computation_17_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_17_cache;
    }
    const std::tuple<bool> &compute_shared_computation_18() {
      if (!compute_shared_computation_18_cache) {
        const Entry slot_value = std::get<0>(compute_slot_view());
        const Entry a = slot_value;
        const uint32_t tag = a.tag;
        const Entry slot_value_2 = std::get<0>(compute_slot_view_2());
        const Entry b = slot_value_2;
        const uint32_t tag_2 = b.tag;
        const bool condition = (tag>=tag_2);
        compute_shared_computation_18_cache.emplace(std::tuple<bool>{condition});
      }
      return *compute_shared_computation_18_cache;
    }
    const std::tuple<bool> &compute_shared_computation_19() {
      if (!compute_shared_computation_19_cache) {
        const bool ready = std::get<0>(compute_shared_computation_7());
        const bool ready_2 = (!ready);
        compute_shared_computation_19_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_19_cache;
    }
    const std::tuple<bool> &compute_shared_computation_20() {
      if (!compute_shared_computation_20_cache) {
        const bool ready = std::get<0>(compute_shared_computation_10());
        const bool ready_2 = (!ready);
        compute_shared_computation_20_cache.emplace(std::tuple<bool>{ready_2});
      }
      return *compute_shared_computation_20_cache;
    }
    const std::tuple<bool,bool> &compute_active_view() {
      if (!compute_active_view_cache) {
        const bool active_value = top.multiplier.active.read();
        compute_active_view_cache.emplace(std::tuple<bool,bool>{active_value,true});
      }
      return *compute_active_view_cache;
    }
    const std::tuple<RobEntry> &compute_shared_computation_21() {
      if (!compute_shared_computation_21_cache) {
        const RobEntry payload = RobEntry{};
        compute_shared_computation_21_cache.emplace(std::tuple<RobEntry>{payload});
      }
      return *compute_shared_computation_21_cache;
    }
  };
} // namespace acir_generated
