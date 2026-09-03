// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt --ac-lower-to-acsim --ac-binding-registry=%S/Inputs/stateful-fast.json --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen | %FileCheck %s
// RUN: %acir_opt --ac-lower-to-acsim --ac-binding-registry=%S/Inputs/stateful-fast.json --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen -o %t.out
// RUN: %acir_opt %t.out -o %t.roundtrip
// RUN: %acir_opt --ac-lower-to-acsim --ac-binding-registry=%S/Inputs/stateful-fast.json --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen -o %t.again
// RUN: diff %t.out %t.again

// Binding-backed lowering: the extern module @Leaf resolves against the
// closed registry to a stateful binding. The lowering publishes the exact
// acsim.binding lock record (sorted with the generated wake types), replaces
// the extern declaration, lowers the instance against the binding target
// with the ordered constructor arguments, and emits a dispatch row whose
// thunks are the locked entry points. The internal driver supplies the test
// structural provider for @Leaf; the public driver correctly rejects the
// unknown extern.

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

// CHECK:      acsim.model @soc epoch "0.4" root @Top
// CHECK-SAME:   construction ["root.leaf", "root.workload"]
// CHECK-SAME:   destruction ["root.workload", "root.leaf"]
// CHECK:        acsim.type @ac_Leaf cpp "ac.Leaf" kind "schema" fingerprint "sha256:1111111111111111111111111111111111111111111111111111111111111111"
// CHECK:        acsim.type @cpp_i32 cpp "cpp_i32" kind "value" fingerprint "sha256:{{[0-9a-f]+}}"
// CHECK:        acsim.type @gfsim cpp "gfsim" kind "provider" fingerprint "sha256:{{[0-9a-f]+}}"
// CHECK:        acsim.type @gfsim_Leaf cpp "gfsim.Leaf" kind "implementation" fingerprint "sha256:2222222222222222222222222222222222222222222222222222222222222222"
// CHECK-NEXT:   acsim.binding @Leaf record {
// CHECK-SAME:     binding = "Leaf"
// CHECK-SAME:     binding_schema = "acsim-binding-0.1"
// CHECK-SAME:     component_schema = @ac_Leaf
// CHECK-SAME:     contract_epoch = "0.4"
// CHECK-SAME:     effect = "stateful"
// CHECK-SAME:     fingerprint = "sha256:cb4c545dd91c68a5e1b7662dba6ed2879aee647988bbdcda269f9baa7a9be31a"
// CHECK-SAME:     implementation = @gfsim_Leaf
// CHECK-SAME:     ownership = {kind = "unique", placement = "member_or_array"}
// CHECK-SAME:     parameters = [{acir_type = "i64", cpp_type = "std::int64_t", mapping = "constructor_constant", name = "width", ordinal = 0 : i64, value = 8 : i64}]
// CHECK-SAME:     provider = @gfsim
// CHECK:        acsim.module @Top
// CHECK-NEXT:     %{{.+}} = acsim.instance @leaf target @Leaf args [8] specialization "sha256:{{[0-9a-f]+}}" : !acsim.owner<@Leaf>
// CHECK:          acsim.process @workload
// CHECK:          acsim.return
// CHECK-NEXT:   }
// CHECK-NEXT:   %{{.+}}, %{{.+}} = acsim.dispatch @Top::@leaf path "root.leaf" indices [] object 0 activation 0
// CHECK-SAME:     work "gfsim::leaf_work" xfer "gfsim::leaf_xfer" reset "gfsim::leaf_reset" validate "gfsim::leaf_validate"
// CHECK:        %{{.+}}, %{{.+}} = acsim.dispatch @Top::@workload path "root.workload" indices [] object 1 activation 1
// CHECK:        acsim.activate
// CHECK-NEXT:   acsim.activate
// CHECK-NEXT:   }
// CHECK-NOT:    ac.module.extern
