// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu %t.frozen | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.1"} {
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload i64 action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
  }
  ac.module @Core() parameters {} graph {
    ac.queue @cursor payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "cursor" path "cursor" watermarks {kind = "register"}
    ac.queue @handles payload i64 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "handles" path "handles"
    ac.process @step kind "workload" {
      %opened = ac.trace.open source "pto"
      %cursor64, %cursor_valid = ac.try_recv @cursor : i64
      %cursor = arith.index_cast %cursor64 : i64 to index
      %next, %handle, %advanced = ac.trace.next %cursor from source "pto" : i64
      %descriptor = ac.trace.decode %handle : i64 to i64
      ac.trace.event %handle lane "Frontend" phase "fetch" : i64
      %zero = arith.constant 0 : i64
      ac.trace.counter %zero lane "ROB" : i64
      %eof = ac.trace.eof %next from source "pto"
      %position = ac.trace.position %next from source "pto"
      scf.if %advanced {
        %sent = ac.try_send @handles %handle : i64
        scf.if %sent {
          %next64 = arith.index_cast %next : index to i64
          %stored = ac.try_send @cursor %next64 : i64
        }
      }
      ac.yield_sim
    }
    ac.return
  }
  ac.system @soc root @Core as "root" tick 0 "cycle"
      workload @Core::@step seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}

// CHECK-DAG: acsim.type @acir_trace_open_pto cpp "acir.trace.open"
// CHECK-DAG: acsim.type @acir_trace_next_pto cpp "acir.trace.next"
// CHECK-DAG: acsim.type @acir_trace_decode cpp "acir.trace.decode"
// CHECK-DAG: acsim.type @acir_trace_event_Frontend_fetch cpp "acir.trace.event"
// CHECK-DAG: acsim.type @acir_trace_counter_ROB cpp "acir.trace.counter"
// CHECK-DAG: acsim.type @acir_trace_eof_pto cpp "acir.trace.eof"
// CHECK-DAG: acsim.type @acir_trace_position_pto cpp "acir.trace.position"
// CHECK: acsim.process @step
// CHECK: acsim.invoke @acir_trace_open_pto
// CHECK: arith.index_cast
// CHECK: acsim.invoke @acir_trace_next_pto
// CHECK: acsim.invoke @acir_trace_decode
// CHECK: acsim.invoke @acir_trace_event_Frontend_fetch
// CHECK: acsim.invoke @acir_trace_counter_ROB
// CHECK: acsim.invoke @acir_trace_eof_pto
// CHECK: acsim.invoke @acir_trace_position_pto
