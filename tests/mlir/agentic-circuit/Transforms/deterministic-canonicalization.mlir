// RUN: %split_file %s %t
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-canonicalize-model,ac-freeze-topology)' --emit-bytecode -o %t/a.mlirbc %t/a.mlir
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-canonicalize-model,ac-freeze-topology)' --emit-bytecode -o %t/b.mlirbc %t/b.mlir
// RUN: cmp %t/a.mlirbc %t/b.mlirbc
// RUN: sha256sum %t/a.mlirbc | cut -d ' ' -f 1 > %t/a.sha256
// RUN: sha256sum %t/b.mlirbc | cut -d ' ' -f 1 > %t/b.sha256
// RUN: cmp %t/a.sha256 %t/b.sha256
// RUN: %acir_opt %t/a.mlirbc | %FileCheck %s --check-prefix=CANONICAL
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-canonicalize-model,ac-freeze-topology)' --emit-bytecode -o %t/nested-a.mlirbc %t/nested-a.mlir
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-canonicalize-model,ac-freeze-topology)' --emit-bytecode -o %t/nested-b.mlirbc %t/nested-b.mlir
// RUN: cmp %t/nested-a.mlirbc %t/nested-b.mlirbc
// RUN: sha256sum %t/nested-a.mlirbc | cut -d ' ' -f 1 > %t/nested-a.sha256
// RUN: sha256sum %t/nested-b.mlirbc | cut -d ' ' -f 1 > %t/nested-b.sha256
// RUN: cmp %t/nested-a.sha256 %t/nested-b.sha256
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' --emit-bytecode %t/nested-a.mlirbc -o %t/nested-refrozen.mlirbc
// RUN: cmp %t/nested-a.mlirbc %t/nested-refrozen.mlirbc

//--- a.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @Z() parameters {} graph { ac.return }
  ac.module @Top() parameters {} graph {
    ac.instance @z of @Z() static {} id "z" path "z" : () -> ()
    ac.instance @a of @A() static {} id "a" path "a" : () -> ()
    ac.stat @requests kind "counter"
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @A() parameters {} graph { ac.return }
}

//--- b.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @A() parameters {} graph { ac.return }
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.stat @requests kind "counter"
    ac.instance @a of @A() static {} id "a" path "a" : () -> ()
    ac.instance @z of @Z() static {} id "z" path "z" : () -> ()
    ac.return
  }
  ac.module @Z() parameters {} graph { ac.return }
}

// CANONICAL: ac.system @soc
// CANONICAL: ac.module @A
// CANONICAL: ac.module @Top
// CANONICAL: ac.instance @a
// CANONICAL-NEXT: ac.instance @z
// CANONICAL: ac.process @workload
// CANONICAL: ac.stat @requests
// CANONICAL: ac.module @Z

//--- nested-a.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.struct"() <{sym_name = "Z", fields = [{name = "z", type = i8}]}> : () -> ()
    "ac.struct"() <{sym_name = "A", fields = [{name = "a", type = i8}]}> : () -> ()
  }) {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@A> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64},
    !ac.struct<@types::@Z> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >} : () -> ()
  ac.protocol @p {
    ac.role @sender dual @receiver cardinality "exclusive"
    ac.role @receiver dual @sender cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.event @z from @sender to @receiver payload i8 action "notify"
    ac.event @a from @sender to @receiver payload i8 action "notify"
    ac.transition from @idle to @idle on @z transfer false retain false guard {}
    ac.transition from @idle to @idle on @a transfer false retain false guard {}
  }
  ac.interface @I {
    ac.role @source dual @sink cardinality "exclusive"
    ac.role @sink dual @source cardinality "exclusive"
    ac.port @z : !ac.channel<i8, @p> from @source to @sink protocol_roles @sender to @receiver
    ac.port @a : !ac.channel<i8, @p> from @source to @sink protocol_roles @sender to @receiver
  }
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  "ac.module"() <{sym_name = "Node", function_type = (i32) -> i32, static_params = {}}> ({
  ^bb0(%arg0 : i32):
    ac.process @state kind "control" { ac.yield_sim }
    "ac.return"(%arg0) : (i32) -> ()
  }) : () -> ()
  ac.module @Top() parameters {} graph {
    %left = "ac.instance"(%right) <{definition = @Node, sym_name = "left", stable_id = "left", path = "left", static_args = {}}> : (i32) -> i32
    %right = "ac.instance"(%left) <{definition = @Node, sym_name = "right", stable_id = "right", path = "right", static_args = {}}> : (i32) -> i32
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
}

//--- nested-b.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.interface @I {
    ac.port @a : !ac.channel<i8, @p> from @source to @sink protocol_roles @sender to @receiver
    ac.role @sink dual @source cardinality "exclusive"
    ac.port @z : !ac.channel<i8, @p> from @source to @sink protocol_roles @sender to @receiver
    ac.role @source dual @sink cardinality "exclusive"
  }
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.protocol @p {
    ac.event @a from @sender to @receiver payload i8 action "notify"
    ac.state @idle initial true terminal false
    ac.role @receiver dual @sender cardinality "exclusive"
    ac.event @z from @sender to @receiver payload i8 action "notify"
    ac.role @sender dual @receiver cardinality "exclusive"
    ac.transition from @idle to @idle on @z transfer false retain false guard {}
    ac.transition from @idle to @idle on @a transfer false retain false guard {}
  }
  "ac.module"() <{sym_name = "Node", function_type = (i32) -> i32, static_params = {}}> ({
  ^bb0(%arg0 : i32):
    ac.process @state kind "control" { ac.yield_sim }
    "ac.return"(%arg0) : (i32) -> ()
  }) : () -> ()
  ac.module @Top() parameters {} graph {
    %right = "ac.instance"(%left) <{definition = @Node, sym_name = "right", stable_id = "right", path = "right", static_args = {}}> : (i32) -> i32
    %left = "ac.instance"(%right) <{definition = @Node, sym_name = "left", stable_id = "left", path = "left", static_args = {}}> : (i32) -> i32
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.struct"() <{sym_name = "A", fields = [{name = "a", type = i8}]}> : () -> ()
    "ac.struct"() <{sym_name = "Z", fields = [{name = "z", type = i8}]}> : () -> ()
  }) {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@A> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64},
    !ac.struct<@types::@Z> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >} : () -> ()
}
