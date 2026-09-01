// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen | %FileCheck %s
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen -o %t.out
// RUN: %acir_opt_public %t.out -o %t.roundtrip
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen -o %t.again
// RUN: diff %t.out %t.again

// A three-level hierarchy: Top instantiates Mid and Child, Mid instantiates
// Child twice. ACSim modules are strictly symbol-sorted (Child < Mid < Top),
// construction is DFS preorder from the root, destruction is its exact
// reverse, and every instance records the target module's exact static
// arguments and specialization fingerprint. Instances of structural modules
// are ownership-only: only the workload process produces a dispatch row.

builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Child() parameters {} graph {
    ac.return
  }
  ac.module @Mid() parameters {} graph {
    ac.instance @left of @Child() static {} id "left" path "left" : () -> ()
    ac.instance @right of @Child() static {} id "right" path "right" : () -> ()
    ac.return
  }
  ac.module @Top() parameters {} graph {
    ac.instance @mid of @Mid() static {} id "mid" path "mid" : () -> ()
    ac.instance @solo of @Child() static {} id "solo" path "solo" : () -> ()
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}

// CHECK:      acsim.model @soc epoch "0.4" root @Top
// CHECK-SAME:   construction ["root.mid", "root.mid.left", "root.mid.right", "root.solo", "root.workload"]
// CHECK-SAME:   destruction ["root.workload", "root.solo", "root.mid.right", "root.mid.left", "root.mid"]
// CHECK:        acsim.module @Child interface {ports = [], resources = [], results = []} static [] specialization "[[CHILD_FP:sha256:[0-9a-f]+]]" exports [] {
// CHECK-NEXT:     acsim.return
// CHECK-NEXT:   }
// CHECK-NEXT:   acsim.module @Mid interface {ports = [], resources = [], results = []} static [] specialization "[[MID_FP:sha256:[0-9a-f]+]]" exports [] {
// CHECK-NEXT:     %{{.+}} = acsim.instance @left target @Child args [] specialization "[[CHILD_FP]]" : !acsim.owner<@Child>
// CHECK-NEXT:     %{{.+}} = acsim.instance @right target @Child args [] specialization "[[CHILD_FP]]" : !acsim.owner<@Child>
// CHECK-NEXT:     acsim.return
// CHECK-NEXT:   }
// CHECK-NEXT:   acsim.module @Top interface {ports = [], resources = [], results = []} static [] specialization "[[TOP_FP:sha256:[0-9a-f]+]]" exports [] {
// CHECK-NEXT:     %{{.+}} = acsim.instance @mid target @Mid args [] specialization "[[MID_FP]]" : !acsim.owner<@Mid>
// CHECK-NEXT:     %{{.+}} = acsim.instance @solo target @Child args [] specialization "[[CHILD_FP]]" : !acsim.owner<@Child>
// CHECK:          acsim.process @workload
// CHECK:          acsim.return
// CHECK-NEXT:   }
// CHECK-NEXT:   %{{.+}}, %{{.+}} = acsim.dispatch @Top::@workload path "root.workload" indices [] object 0 activation 0
// CHECK-NOT:    acsim.dispatch
// CHECK:        acsim.activate
// CHECK-NOT:    acsim.activate
// CHECK-NEXT:   }
