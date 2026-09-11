// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen | %FileCheck %s --check-prefix=VERILOG

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "table_read"} {
  ac.table @state entry i8 entries 4 init 0 owner "/" stable_id "table/state"
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

// PYC: func.func @table_read
// PYC-COUNT-4: pyc.reg
// PYC: pyc.select
// PYC-NOT: sync_mem
// PYC-NOT: ac.table

// VERILOG: module table_read (
// VERILOG: pyc_reg
