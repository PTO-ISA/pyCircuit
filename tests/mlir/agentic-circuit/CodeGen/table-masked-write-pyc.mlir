// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen | %FileCheck %s --check-prefix=VERILOG

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "masked_write"} {
  ac.table @state entry i8 entries 4 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 4>, axis_widths = array<i64: 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:5790212daac33caa1d98f7aefb7b538dfad0d08cec26c945bb8f0ec2d7cdf35a",
    init_version = 1 : i64,
    init_image = [1 : i8, 2 : i8, 3 : i8, 4 : i8]
  }
  %mask = ac.table.match @state predicate {
  ^predicate(%entry: !ac.var<i8>):
    %limit = ac.var.constant 4 : i8 as !ac.var<i8>
    %selected = ac.var.cmp "ult" %entry, %limit : !ac.var<i8> -> !ac.var<i1>
    ac.table.match.yield %selected : !ac.var<i1>
  } -> !ac.var<i4>
  ac.table.masked_write @state %mask : !ac.var<i4> mode "field" write_fields ["$entry"] enable {
  ^enable:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%entry: !ac.var<i8>):
    %one = ac.var.constant 1 : i8 as !ac.var<i8>
    %next = ac.var.add %entry, %one : !ac.var<i8>
    ac.table.yield %next : !ac.var<i8>
  } {ac.endpoint_path = "/update", ac.name = "update"}
  %index = ac.source depth 1 latency 1 {ac.name = "index"} : !ac.queue<i2>
  %value = ac.table.read @state, %index : !ac.queue<i2> depth 1 latency 1 address {
  ^address(%item: !ac.var<i2>):
    ac.table.yield %item : !ac.var<i2>
  } when {
  ^when(%item: !ac.var<i2>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/read", ac.name = "read"} -> !ac.queue<i8>
  ac.sink %value {ac.name = "sink"} : !ac.queue<i8>
}

// PYC: func.func @masked_write
// PYC-COUNT-4: pyc.reg
// PYC-DAG: pyc.constant 1 : i8
// PYC-DAG: pyc.constant 4 : i8
// PYC: pyc.add
// PYC-NOT: sync_mem
// PYC-NOT: ac.table

// VERILOG: module masked_write (
// VERILOG: pyc_reg
