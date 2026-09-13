// RUN: %acir_queue_plan %s | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %s | %FileCheck %s --check-prefix=CXX

module attributes {ac.contract_epoch = "0.5", ac.freeze_epoch = "0.5", ac.frozen_owners = [{kind = "ac.system_root", owner = @Top, path = "root", stable_id = "root"}, {kind = "ac.instance", owner = @Top::@first__second__third__fourth, path = "root.first__second__third__fourth", stable_id = "root/first__second__third__fourth"}], ac.frozen_system = @atomic_fanout, ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.topology_digest = "579003ce154b0256900c3b114afc8c18680856246a8467d18bcc9bad6cd300e6", ac.topology_frozen = true} {
  ac.system @atomic_fanout root @Top as "root" tick 0 "cycle" seed {kind = "fixed", value = 0 : i64} instrumentation [] results {format = "json", id = "default"} selected true
  ac.module @fanout(%arg0: !ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>) parameters {} attributes {ac.definition_fingerprint = "sha256:0390bffef3fd7ad53dc2b2f993443e2ab3b5ab22e7a5028dc732de71d80d0242", ac.input_display_names = ["value"], ac.output_display_names = ["first", "second", "third", "fourth"]} graph {
    %0:4 = ac.scope @body(%arg0) {
    ^bb0(%arg1: !ac.queue<i8>):
      %1:4 = ac.firing %arg1 depths [1, 1, 1, 1] latencies [1, 1, 1, 1] stable_id "fanout/first" domain "cycle" {
      ^bb0(%arg2: !ac.var<i8>):
        %2 = ac.var.constant 1 : i8 as !ac.var<i8>
        %3 = ac.var.add %arg2, %2 {ac.display_name = "second"} : !ac.var<i8>
        %4 = ac.var.constant 2 : i8 as !ac.var<i8>
        %5 = ac.var.add %arg2, %4 {ac.display_name = "third"} : !ac.var<i8>
        %6 = ac.var.constant 3 : i8 as !ac.var<i8>
        %7 = ac.var.add %arg2, %6 {ac.display_name = "fourth"} : !ac.var<i8>
        %8 = ac.var.constant true as !ac.var<i1>
        ac.firing.condition %8 : !ac.var<i1>
        ac.firing.output %arg2 when %8 ordinal 0 : !ac.var<i8>, !ac.var<i1>
        ac.firing.output %3 when %8 ordinal 1 : !ac.var<i8>, !ac.var<i1>
        ac.firing.output %5 when %8 ordinal 2 : !ac.var<i8>, !ac.var<i1>
        ac.firing.output %7 when %8 ordinal 3 : !ac.var<i8>, !ac.var<i1>
        ac.firing.yield %arg2, %3, %5, %7 : !ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>
      } {ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 1 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 2 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 3 : i64}], ac.arbitration_membership = [], ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 1 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 2 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 3 : i64}], ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 1 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 2 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 3 : i64}], ac.guard_kind = #ac<rule_guard_kind always>, ac.initially_active = false, ac.name = "first", ac.output_names = ["first", "second", "third", "fourth"], ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}, {ordinal = 1 : i64, presence_kind = #ac<rule_output_presence_kind always>}, {ordinal = 2 : i64, presence_kind = #ac<rule_output_presence_kind always>}, {ordinal = 3 : i64, presence_kind = #ac<rule_output_presence_kind always>}], ac.rule_definition = "split", ac.rule_footprints = [], ac.rule_priority = 0 : i64, ac.schedule_kind = #ac<rule_schedule_kind independent>, ac.source_column = 1 : i64, ac.source_file = "<queue-model>", ac.source_line = 5 : i64, ac.state_accesses = [], ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 1 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 2 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 3 : i64}]} : (!ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>)
      ac.scope.yield %1#0, %1#1, %1#2, %1#3 : !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>
    } : (!ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>)
    ac.return %0#0, %0#1, %0#2, %0#3 : !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>
  }
  ac.module @Top() parameters {} attributes {ac.definition_fingerprint = "sha256:e476051eba137b29ed1e4e0296b0fda86697d7b595bf1fd1963942512422cda7", ac.specialization = "sha256:74c63d2df502d04bf88334d5da230dd24cd9ee7ca9b20bd941b3d74033d927b9"} graph {
    %0 = ac.scope @inputs() {
      %2 = ac.source depth 1 latency 1 {ac.name = "value"} : !ac.queue<i8>
      ac.scope.yield %2 : !ac.queue<i8>
    } : () -> !ac.queue<i8>
    %1:4 = ac.instance @first__second__third__fourth of @fanout(%0) static {} id "first__second__third__fourth" path "first__second__third__fourth" {ac.specialization = "sha256:9dad7fa8fbe622fbb623d1bd709b242916eed259f4e9a3a18644360311944a47"} : (!ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>)
    ac.scope @outputs(%1#0, %1#1, %1#2, %1#3) {
    ^bb0(%arg0: !ac.queue<i8>, %arg1: !ac.queue<i8>, %arg2: !ac.queue<i8>, %arg3: !ac.queue<i8>):
      ac.sink %arg0 {ac.name = "sink_0"} : !ac.queue<i8>
      ac.sink %arg1 {ac.name = "sink_1"} : !ac.queue<i8>
      ac.sink %arg2 {ac.name = "sink_2"} : !ac.queue<i8>
      ac.sink %arg3 {ac.name = "sink_3"} : !ac.queue<i8>
      ac.scope.yield
    } : (!ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>) -> ()
    ac.return
  }
}

// PLAN: "display_rule_name":"split"
// PLAN-SAME: "outputs":["first","second","third","fourth"]
// PLAN-SAME: "definition":"fanout"
// PLAN-SAME: "interface_inputs":[{"display_name":"value","lanes":1,"name":"input_0","payload_type":"i8","rate":1}]
// PLAN-SAME: "interface_outputs":[{"display_name":"first","lanes":1,"name":"first","payload_type":"i8","rate":1},{"display_name":"second","lanes":1,"name":"second","payload_type":"i8","rate":1},{"display_name":"third","lanes":1,"name":"third","payload_type":"i8","rate":1},{"display_name":"fourth","lanes":1,"name":"fourth","payload_type":"i8","rate":1}]
// CXX-COUNT-1: class [[FANOUT:Module_Fanout]] final : public gfsim::Module
// CXX: gfsim::QueueStateTransition<[[FANOUT]]_rule_split_policy, std::tuple<>, std::tuple<gfsim::UInt<8>>, std::tuple<gfsim::UInt<8>, gfsim::UInt<8>, gfsim::UInt<8>, gfsim::UInt<8>>, std::tuple<>> rule_split_;
