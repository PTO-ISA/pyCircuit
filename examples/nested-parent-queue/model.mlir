// Child processes communicate through a FIFO owned by their parent Core.
builtin.module attributes {ac.contract_epoch = "0.3"} {
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload i32 action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
  }

  ac.module @Producer() parameters {} graph {
    ac.process @step kind "control" {
      %token = arith.constant 7 : i32
      %ok = ac.try_send @Core::@link %token : i32
      ac.yield_sim
    }
    ac.return
  }

  ac.module @Consumer() parameters {} graph {
    ac.process @step kind "control" {
      %token, %got = ac.try_recv @Core::@link : i32
      scf.if %got {
        ac.assert %got, "token"
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @Core() parameters {} graph {
    ac.queue @link payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "link" path "link"
    ac.queue @clock payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "clock" path "clock"
        watermarks {kind = "register"}
    ac.instance @consumer of @Consumer() static {} id "consumer" path "consumer" : () -> ()
    ac.instance @producer of @Producer() static {} id "producer" path "producer" : () -> ()
    ac.process @tick kind "workload" {
      %unused, %valid = ac.try_recv @clock : i32
      ac.yield_sim
    }
    ac.return
  }

  ac.system @soc root @Core as "root" tick 0 "cycle"
      workload @Core::@tick seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}
