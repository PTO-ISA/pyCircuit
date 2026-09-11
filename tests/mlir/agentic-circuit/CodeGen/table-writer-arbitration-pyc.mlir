// RUN: %acir_opt --pass-pipeline='builtin.module(ac-verify-value-constraints,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen | %FileCheck %s --check-prefix=VERILOG

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "writer_arbitration"} {
  ac.table @state entry i8 entries 1 init 0 owner "/" stable_id "table/state"
  %left = ac.source depth 1 latency 1 {ac.name = "left"} : !ac.queue<i8>
  %right = ac.source depth 1 latency 1 {ac.name = "right"} : !ac.queue<i8>
  ac.table.write @state, %left : !ac.queue<i8> mode "replace" write_fields ["$entry"] address {
  ^address(%item: !ac.var<i8>):
    %zero = ac.var.constant false as !ac.var<i1>
    ac.table.yield %zero : !ac.var<i1>
  } enable {
  ^enable(%item: !ac.var<i8>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<i8>):
    ac.table.yield %item : !ac.var<i8>
  } {ac.endpoint_path = "/left", ac.name = "left_write", ac.endpoint_id = "writer/left", ac.arbitration = #ac.writer_priority<1>}
  ac.table.write @state, %right : !ac.queue<i8> mode "replace" write_fields ["$entry"] address {
  ^address(%item: !ac.var<i8>):
    %zero = ac.var.constant false as !ac.var<i1>
    ac.table.yield %zero : !ac.var<i1>
  } enable {
  ^enable(%item: !ac.var<i8>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<i8>):
    ac.table.yield %item : !ac.var<i8>
  } {ac.endpoint_path = "/right", ac.name = "right_write", ac.endpoint_id = "writer/right", ac.arbitration = #ac.writer_priority<0>}
  %read_index = ac.source depth 1 latency 1 {ac.name = "read_index"} : !ac.queue<i1>
  %value = ac.table.read @state, %read_index : !ac.queue<i1> depth 1 latency 1 address {
  ^address(%item: !ac.var<i1>):
    %zero = ac.var.constant false as !ac.var<i1>
    ac.table.yield %zero : !ac.var<i1>
  } when {
  ^when(%item: !ac.var<i1>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/read", ac.name = "read"} -> !ac.queue<i8>
  ac.sink %value {ac.name = "sink"} : !ac.queue<i8>
}

// PYC: func.func @writer_arbitration
// PYC: pyc.reg
// PYC: pyc.not
// PYC: pyc.and
// PYC-NOT: sync_mem
// PYC-NOT: ac.table

// VERILOG: module writer_arbitration (
// VERILOG: pyc_reg
