module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, format_version = 9 : i64, top = "MixedPorts", type_order = [], types = {}}} {
  func.func @ac_init_top.source_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.source_cap", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.source(%arg0: i1) -> tuple<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.source", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "tuple<u32>"}} {
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc1)
    %0 = acir.aggregate(%c7_i32) {acir.source_type = "tuple<u32>"} : (i32) -> tuple<i32>
    return %0 : tuple<i32>
  }
  func.func @ac_init_top.accepted_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.accepted_cap", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc2)
    return %c2_i32 : i32
  }
  func.func @ac_init_top.accepted(%arg0: i1) -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.accepted", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.last_accepted(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.last_accepted", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc3)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.ledger.observed(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.ledger.observed", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc4)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.observer.last(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.observer.last", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc5)
    return %c0_i32 : i32
  }
  acir.module @top attributes {acir.instance = {definition = "src_mixed_ports_9bff12ad.MixedPorts", name = "top", parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    acir.queue @top.source : i32 {capacity_fn = "ac_init_top.source_cap", initial_fn = "ac_init_top.source", name = "top.source", type = "u32"}
    acir.queue @top.accepted : i32 {capacity_fn = "ac_init_top.accepted_cap", initial_fn = "ac_init_top.accepted", name = "top.accepted", type = "u32"}
    acir.cell @top.last_accepted : i32 {initial_fn = "ac_init_top.last_accepted", name = "top.last_accepted", type = "u32"}
    acir.signal @top.result : i32 {name = "top.result", type = "u32"}
    func.func private @ac_init_top.source_cap(i1) -> i32
    func.func private @ac_init_top.source(i1) -> tuple<i32>
    func.func private @ac_init_top.accepted_cap(i1) -> i32
    func.func private @ac_init_top.accepted(i1) -> tuple<>
    func.func private @ac_init_top.last_accepted(i1) -> i32
    func.func private @ac_init_top.ledger.observed(i1) -> i32
    func.func private @ac_init_top.observer.last(i1) -> i32
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.source {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %1 = acir.config "enabled" {acir.id = "v3", acir.source_type = "bool"} : i1 loc(#loc6)
      %2 = acir.export %1 {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1
      acir.module.call @top.router loc(#loc7)
      %3 = acir.ref @top::@top.result {acir.source_type = "signal<u32>"} : !acir.signal<i32>
      acir.module.call @top.observer loc(#loc8)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.ledger attributes {acir.instance = {definition = "src_mixed_ports_9bff12ad.Ledger", name = "top.ledger", parent = "top", rules = ["record"]}} {
    acir.contract {relations = [], rules = ["record"]}
    acir.cell @top.ledger.observed : i32 {initial_fn = "ac_init_top.ledger.observed", name = "top.ledger.observed", type = "u32"}
    func.func private @ac_init_top.source_cap(i1) -> i32
    func.func private @ac_init_top.source(i1) -> tuple<i32>
    func.func private @ac_init_top.accepted_cap(i1) -> i32
    func.func private @ac_init_top.accepted(i1) -> tuple<>
    func.func private @ac_init_top.last_accepted(i1) -> i32
    func.func private @ac_init_top.ledger.observed(i1) -> i32
    func.func private @ac_init_top.observer.last(i1) -> i32
    acir.rule @top.router_body.0.call.0.3.0.call.0.8 : (i32) -> () {acir.function = {contract = "Module:top.ledger", definition = "record", instance = "top.ledger", interface = "Module:top.ledger:record", kind = "rule", name = "top.router_body.0.call.0.3.0.call.0.8", output_type = "void", owner = "top.ledger", params = [{id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "record"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.ledger::@top.ledger.observed {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc9)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.router attributes {acir.instance = {definition = "src_mixed_ports_9bff12ad.Router", name = "top.router", parent = "top", rules = ["route"]}} {
    acir.contract {relations = [], rules = ["route"]}
    func.func private @ac_init_top.source_cap(i1) -> i32
    func.func private @ac_init_top.source(i1) -> tuple<i32>
    func.func private @ac_init_top.accepted_cap(i1) -> i32
    func.func private @ac_init_top.accepted(i1) -> tuple<>
    func.func private @ac_init_top.last_accepted(i1) -> i32
    func.func private @ac_init_top.ledger.observed(i1) -> i32
    func.func private @ac_init_top.observer.last(i1) -> i32
    acir.rule @top.router_body.0.call.0.3 : (!acir.queue<i32>, i1) -> tuple<i32, i32, i32, i1, i1, i1> {acir.function = {contract = "Module:top.router", definition = "route", instance = "top.router", interface = "Module:top.router:route", kind = "rule", name = "top.router_body.0.call.0.3", output_type = ["queue<u32>", "cell<u32>", "signal<u32>"], owner = "top.router", params = [{id = "v1", name = "message", targets = ["top.source"], type = "queue<u32>"}, {id = "v2", name = "take", targets = [], type = "bool"}], public = true, result = "tuple<u32,u32,u32,bool,bool,bool>", rule = "route"}} body {
    ^bb0(%arg0: !acir.queue<i32>, %arg1: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "tuple<u32,u32,u32,bool,bool,bool>"} : () -> tuple<i32, i32, i32, i1, i1, i1>
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.queue.peek %arg0 {acir.id = "v8", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc10)
      acir.rule.call @top.ledger::@top.router_body.0.call.0.3.0.call.0.8(%1) {acir.id = "v9", acir.source_type = "void", contract = "Module:top.ledger", instance = "top.ledger", interface = "Module:top.ledger:record", role = "rule", rule = "record"} : (i32) -> () loc(#loc11)
      %2:3 = scf.if %arg1 -> (i1, i1, tuple<i32, i32, i32, i1, i1, i1>) {
        %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc12)
        %5 = arith.addi %1, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc13)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %true_1 = arith.constant {acir.source_type = "bool"} true
        %true_2 = arith.constant {acir.source_type = "bool"} true
        %6 = acir.aggregate(%5, %1, %1, %true_0, %true_1, %true_2) {acir.id = "v15", acir.source_type = "tuple<u32,u32,u32,bool,bool,bool>"} : (i32, i32, i32, i1, i1, i1) -> tuple<i32, i32, i32, i1, i1, i1> loc(#loc14)
        %true_3 = arith.constant {acir.source_type = "bool"} true
        %false_4 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_4, %true_3, %6 : i1, i1, tuple<i32, i32, i32, i1, i1, i1>
      } else {
        scf.yield %true, %false, %0 : i1, i1, tuple<i32, i32, i32, i1, i1, i1>
      }
      %3:3 = scf.if %2#0 -> (i1, i1, tuple<i32, i32, i32, i1, i1, i1>) {
        %5 = acir.aggregate() {acir.id = "v24", acir.source_type = "u32"} : () -> i32 loc(#loc15)
        %false_0 = arith.constant {acir.source_type = "bool"} false
        %6 = acir.aggregate() {acir.id = "v26", acir.source_type = "u32"} : () -> i32 loc(#loc15)
        %false_1 = arith.constant {acir.source_type = "bool"} false
        %true_2 = arith.constant {acir.source_type = "bool"} true
        %7 = acir.aggregate(%5, %6, %1, %false_0, %false_1, %true_2) {acir.id = "v29", acir.source_type = "tuple<u32,u32,u32,bool,bool,bool>"} : (i32, i32, i32, i1, i1, i1) -> tuple<i32, i32, i32, i1, i1, i1> loc(#loc15)
        %true_3 = arith.constant {acir.source_type = "bool"} true
        %false_4 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_4, %true_3, %7 : i1, i1, tuple<i32, i32, i32, i1, i1, i1>
      } else {
        scf.yield %2#0, %2#1, %2#2 : i1, i1, tuple<i32, i32, i32, i1, i1, i1>
      }
      %4 = acir.unary %3#0 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %4 {acir.source_type = "void"}
      acir.return %3#2 : tuple<i32, i32, i32, i1, i1, i1> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.router_body", owner = "top.router", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.source {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %1 = acir.external "top_body" "v4" {acir.source_type = "bool"} : i1
      %2 = acir.rule.call @top.router::@top.router_body.0.call.0.3(%0, %1) {acir.id = "v4", acir.source_type = "tuple<u32,u32,u32,bool,bool,bool>", contract = "Module:top.router", instance = "top.router", interface = "Module:top.router:route", output_bindings = [@top::@top.accepted, @top::@top.last_accepted, @top::@top.result], role = "rule", rule = "route"} : (!acir.queue<i32>, i1) -> tuple<i32, i32, i32, i1, i1, i1> loc(#loc16)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %3 = acir.extract %2[%c0_i32] {acir.id = "v6", acir.source_type = "u32", indexed = true, output_call = "v4", slot = 0 : i64} : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32 loc(#loc16)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %4 = acir.extract %2[%c1_i32] {acir.id = "v8", acir.source_type = "u32", indexed = true, output_call = "v4", slot = 1 : i64} : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32 loc(#loc16)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %5 = acir.extract %2[%c2_i32] {acir.id = "v10", acir.source_type = "u32", indexed = true, output_call = "v4", slot = 2 : i64} : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32 loc(#loc16)
      %6 = acir.ref @top::@top.accepted {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %7 = acir.ref @top::@top.last_accepted {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %8 = acir.ref @top::@top.result {acir.source_type = "signal<u32>"} : !acir.signal<i32>
      %false = arith.constant {acir.source_type = "bool"} false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.observer attributes {acir.instance = {definition = "src_mixed_ports_9bff12ad.Observer", name = "top.observer", parent = "top", rules = ["sample"]}} {
    acir.contract {relations = [], rules = ["sample"]}
    acir.cell @top.observer.last : i32 {initial_fn = "ac_init_top.observer.last", name = "top.observer.last", type = "u32"}
    func.func private @ac_init_top.source_cap(i1) -> i32
    func.func private @ac_init_top.source(i1) -> tuple<i32>
    func.func private @ac_init_top.accepted_cap(i1) -> i32
    func.func private @ac_init_top.accepted(i1) -> tuple<>
    func.func private @ac_init_top.last_accepted(i1) -> i32
    func.func private @ac_init_top.ledger.observed(i1) -> i32
    func.func private @ac_init_top.observer.last(i1) -> i32
    acir.rule @top.observer_body.0.call.2.2 : (!acir.signal<i32>) -> () {acir.function = {contract = "Module:top.observer", definition = "sample", instance = "top.observer", interface = "Module:top.observer:sample", kind = "rule", name = "top.observer_body.0.call.2.2", output_type = "void", owner = "top.observer", params = [{id = "v1", name = "x", targets = ["top.result"], type = "signal<u32>"}], public = true, result = "void", rule = "sample"}} body {
    ^bb0(%arg0: !acir.signal<i32>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.signal.get %arg0 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.signal<i32> -> i32 loc(#loc17)
      %1 = acir.ref @top.observer::@top.observer.last {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %1, %0 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc18)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.observer_body", owner = "top.observer", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.result {acir.source_type = "signal<u32>"} : !acir.signal<i32>
      acir.rule.call @top.observer::@top.observer_body.0.call.2.2(%0) {acir.id = "v3", acir.source_type = "void", contract = "Module:top.observer", instance = "top.observer", interface = "Module:top.observer:sample", role = "rule", rule = "sample"} : (!acir.signal<i32>) -> () loc(#loc19)
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":48:43)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":49:46)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":50:49)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":8:44)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":18:40)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":47:18)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":57:43)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":58:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":12:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":35:17)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":36:9)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":38:29)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":38:21)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":38:13)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":39:9)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":42:16)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":22:16)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":22:9)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":25:9)
