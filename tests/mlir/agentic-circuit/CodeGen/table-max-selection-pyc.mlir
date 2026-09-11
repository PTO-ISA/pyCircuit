// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen | %FileCheck %s --check-prefix=VERILOG

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "table_max_selection"} {
  ac.table @state entry i8 entries 4 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 4>, axis_widths = array<i64: 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:5790212daac33caa1d98f7aefb7b538dfad0d08cec26c945bb8f0ec2d7cdf35a",
    init_version = 1 : i64,
    init_image = [1 : i8, 2 : i8, 3 : i8, 4 : i8]
  }
  %matches = ac.table.match @state predicate {
  ^predicate(%entry: !ac.var<i8>):
    %zero = ac.var.constant 0 : i8 as !ac.var<i8>
    %valid = ac.var.cmp "ne" %entry, %zero : !ac.var<i8> -> !ac.var<i1>
    ac.table.match.yield %valid : !ac.var<i1>
  } -> !ac.var<i4>
  %index0, %index1, %valid0, %valid1 = ac.table.choose @state %matches : !ac.var<i4>
      count 2 policy #ac<table_selection_policy max>
      key_order #ac<table_key_ordering unsigned> stable_id "selection/max" key {
  ^key(%entry: !ac.var<i8>):
    ac.table.choose.yield %entry : !ac.var<i8>
  } -> !ac.var<i2>, !ac.var<i2>, !ac.var<i1>, !ac.var<i1>
  %value0 = ac.table.read @state depth 1 latency 1 address {
  ^address: ac.table.yield %index0 : !ac.var<i2>
  } when {
  ^when: ac.table.yield %valid0 : !ac.var<i1>
  } {ac.endpoint_path = "/value0", ac.name = "value0"} -> !ac.queue<i8>
  %value1 = ac.table.read @state depth 1 latency 1 address {
  ^address: ac.table.yield %index1 : !ac.var<i2>
  } when {
  ^when: ac.table.yield %valid1 : !ac.var<i1>
  } {ac.endpoint_path = "/value1", ac.name = "value1"} -> !ac.queue<i8>
  ac.sink %value0 {ac.name = "sink0"} : !ac.queue<i8>
  ac.sink %value1 {ac.name = "sink1"} : !ac.queue<i8>
}

// PYC: func.func @table_max_selection
// PYC: {predicate = "ult"}
// PYC-NOT: sync_mem
// PYC-NOT: ac.table

// VERILOG: module table_max_selection (
