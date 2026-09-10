// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen | %FileCheck %s --check-prefix=VERILOG

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "multidimensional_table"} {
  ac.table @state entry i8 entries 6 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0",
    init_version = 1 : i64,
    init_image = [1 : i8, 2 : i8, 3 : i8, 4 : i8, 5 : i8, 6 : i8]
  }
  %row = ac.source depth 1 latency 1 {ac.name = "row"} : !ac.queue<i1>
  %value = ac.table.read @state, %row : !ac.queue<i1> depth 1 latency 1 address {
  ^address(%item: !ac.var<i1>):
    %column = ac.var.constant 2 : i2 as !ac.var<i2>
    %index = ac.table.index @state[%item, %column] : !ac.var<i1>, !ac.var<i2> -> !ac.var<i3>
    ac.table.yield %index : !ac.var<i3>
  } when {
  ^when(%item: !ac.var<i1>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/read", ac.name = "read"} -> !ac.queue<i8>
  ac.sink %value {ac.name = "sink"} : !ac.queue<i8>
}

// PYC: func.func @multidimensional_table
// PYC-COUNT-6: pyc.reg
// PYC: pyc.zext
// PYC: pyc.mul
// PYC: pyc.assert
// PYC-NOT: sync_mem
// PYC-NOT: ac.table

// VERILOG: module multidimensional_table (
// VERILOG: pyc_reg
