// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/probe-dataflow.mlir 2>&1 | %FileCheck %s --check-prefix=PROBE
// RUN: %not %acir_opt %t/instrumentation-result.mlir 2>&1 | %FileCheck %s --check-prefix=INSTRUMENT
// RUN: %not %acir_opt %t/bad-stat.mlir 2>&1 | %FileCheck %s --check-prefix=STAT
// RUN: %not %acir_opt %t/monitor-effect.mlir 2>&1 | %FileCheck %s --check-prefix=MONITOR
// RUN: %not %acir_opt %t/static-assert.mlir 2>&1 | %FileCheck %s --check-prefix=STATIC-ASSERT
// RUN: %not %acir_opt %t/require-non-boolean.mlir 2>&1 | %FileCheck %s --check-prefix=REQUIRE-TYPE
// RUN: %not %acir_opt %t/ensure-non-boolean.mlir 2>&1 | %FileCheck %s --check-prefix=ENSURE-TYPE

//--- probe-dataflow.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @p kind "control" {
      %one = arith.constant 1 : i64
      %v = ac.probe @queue kind "queue" : i32
      ac.schedule @worker %v after %one : i32
      ac.yield_sim
    }
    ac.return
  }
}
// PROBE: probe result may only feed observation operations

//--- instrumentation-result.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @p kind "control" {
      ac.instrumentation @bad {
        %value = arith.constant 1 : i64
        ac.schedule @worker %value after %value : i64
      }
      ac.yield_sim
    }
    ac.return
  }
}
// INSTRUMENT: instrumentation may contain only removable observation operations

//--- bad-stat.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.stat @bad kind "average"
    ac.return
  }
}
// STAT: kind must be 'counter', 'gauge', 'histogram', or 'event_log'

//--- monitor-effect.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M(i32) parameters {} graph {
  ^bb0(%v : i32):
    ac.process @p kind "monitor" captures(%v : i32) {
    ^bb0(%captured : i32):
      %accepted = ac.try_send @queue %captured : i32
      ac.yield_sim
    }
    ac.return
  }
}
// MONITOR: monitor process cannot perform functional state effects

//--- static-assert.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M(i1) parameters {} graph {
  ^bb0(%condition : i1):
    ac.assert %condition, "runtime only"
    ac.return
  }
}
// STATIC-ASSERT: operation is not legal in an ac.module structural Graph region

//--- require-non-boolean.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    %bad = arith.constant 1 : i32
    ac.require %bad, "non-boolean require"
    ac.return
  }
}
// REQUIRE-TYPE: error: use of value '%bad' expects different type than prior uses: 'i1' vs 'i32'

//--- ensure-non-boolean.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    %bad = arith.constant 1 : i32
    ac.ensure %bad, "non-boolean ensure"
    ac.return
  }
}
// ENSURE-TYPE: error: use of value '%bad' expects different type than prior uses: 'i1' vs 'i32'
