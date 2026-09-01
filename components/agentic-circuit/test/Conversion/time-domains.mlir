// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Bridge() parameters {} graph {
    ac.return
  }
  ac.module @Top() parameters {} graph {
    ac.instance @cdc of @Bridge() static {} id "cdc" path "cdc" : () -> ()
    ac.time_domain @global period 1 phase 0 scale 1
    ac.time_domain @core period 2 phase 1 scale 2 parent @global
        bridge {kind = "explicit", owner = @cdc}
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}

// CHECK:      acsim.type @core cpp "gfsim::TimeDomainRuntime" kind "time_domain" fingerprint "sha256:{{[0-9a-f]+}}" {bridge = {kind = "explicit", owner = @cdc}, parent = @global, period = 2 : i64, phase = 1 : i64, tick_scale = 2 : i64}
// CHECK-NEXT: acsim.type @global cpp "gfsim::TimeDomainRuntime" kind "time_domain" fingerprint "sha256:{{[0-9a-f]+}}" {period = 1 : i64, phase = 0 : i64, tick_scale = 1 : i64}
