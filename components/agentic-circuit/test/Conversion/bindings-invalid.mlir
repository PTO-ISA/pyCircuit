// RUN: %split_file %s %t
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/extern.mlir -o %t/extern.frozen
// RUN: %acir_opt --ac-lower-to-acsim --ac-binding-registry=%S/Inputs/pure-fast.json --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t/extern.frozen | %FileCheck %s --check-prefix=PURE
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/sort-order.mlir -o %t/sort-order.frozen
// RUN: %acir_opt --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t/sort-order.frozen | %FileCheck %s --check-prefix=SORT
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/heterogeneous-array.mlir -o %t/heterogeneous-array.frozen
// RUN: %not %acir_opt --ac-lower-to-acsim --ac-binding-registry=%S/Inputs/stateful-fast.json --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t/heterogeneous-array.frozen -o %t/array.out 2>&1 | %FileCheck %s --check-prefix=ARRAY
// RUN: test ! -s %t/array.out
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/time-domain.mlir -o %t/time-domain.frozen
// RUN: %acir_opt --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t/time-domain.frozen | %FileCheck %s --check-prefix=TD
// RUN: %not %acir_opt --ac-lower-to-acsim --ac-binding-registry=%S/Inputs/bad-registry-structure.json --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t/extern.frozen 2>&1 | %FileCheck %s --check-prefix=REGISTRY
// RUN: %not %acir_opt --ac-lower-to-acsim --ac-binding-registry=%S/Inputs/bad-metadata-empty-work.json --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t/extern.frozen 2>&1 | %FileCheck %s --check-prefix=METADATA

// Pure external calls lower without ownership. Array-specialization,
// stage-boundary, and registry contract rejections fail atomically with their
// exact ACLOWER-* codes.

//--- extern.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module.extern @Leaf : () -> i32 parameters {width = 8 : i64}
      implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top() parameters {} graph {
    %leaf = ac.instance @leaf of @Leaf() static {width = 8 : i64}
        id "leaf" path "leaf" : () -> i32
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
}

//--- sort-order.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.instance @zed of @Zebra() static {} id "zed" path "zed" : () -> ()
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
  ac.module @Zebra() parameters {} graph {
    ac.return
  }
}

//--- heterogeneous-array.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module.extern @Leaf : () -> i32 parameters {width = 8 : i64}
      implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top() parameters {} graph {
    %cells:2 = ac.array @cells of @Leaf shape [2]()
        static [{width = 8 : i64}, {width = 16 : i64}]
        id "cells" path "cells" : () -> (i32, i32)
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
}

//--- time-domain.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.time_domain @global period 1 phase 0 scale 1
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
}

// PURE: acsim.inline @Leaf() : () -> !acsim.expr<@cpp_i32>
// PURE-NOT: acsim.dispatch @Top::@leaf
// SORT: acsim.module @Zebra
// SORT: acsim.module @Top
// ARRAY: error: ACLOWER-ARRAY: differently specialized array elements are outside the lowering stage; lower them as ordered named members instead
// TD: acsim.type @global cpp "gfsim::TimeDomainRuntime" kind "time_domain" fingerprint "sha256:{{[0-9a-f]+}}" {period = 1 : i64, phase = 0 : i64, tick_scale = 1 : i64}
// REGISTRY: error: ACLOWER-BINDING-REGISTRY: registry must contain exactly candidates and requests arrays
// METADATA: error: ACLOWER-BINDING-METADATA: binding effect requires exact executable entry points
