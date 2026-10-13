module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, exports = {}, format_version = 8 : i64, top = "MixedPorts", type_order = [], types = {}}} {
  func.func @ac_init_top.source_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.source_cap", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.source(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.source", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc1)
    %0 = acir.aggregate(%c7_i32) {acir.source_type = "vector<u32>"} : (i32) -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
  }
  func.func @ac_init_top.accepted_cap(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.accepted_cap", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc2)
    return %c2_i32 : i32
  }
  func.func @ac_init_top.accepted(%arg0: i1) -> !acir.vector<i32> attributes {acir.function = {kind = "helper", name = "ac_init_top.accepted", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "vector<u32>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<u32>"} : () -> !acir.vector<i32>
    return %0 : !acir.vector<i32>
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
  acir.module @top attributes {acir.instance = {definition = "src_mixed_ports_9bff12ad.MixedPorts", inputs = {}, members = {}, name = "top", parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    acir.queue @top.source : i32 {capacity_fn = "ac_init_top.source_cap", initial_fn = "ac_init_top.source", name = "top.source", type = "u32"}
    acir.queue @top.accepted : i32 {capacity_fn = "ac_init_top.accepted_cap", initial_fn = "ac_init_top.accepted", name = "top.accepted", type = "u32"}
    acir.cell @top.last_accepted : i32 {initial_fn = "ac_init_top.last_accepted", name = "top.last_accepted", type = "u32"}
    acir.signal @top.result : i32 {name = "top.result", type = "u32"}
    func.func private @ac_init_top.source_cap(i1) -> i32
    func.func private @ac_init_top.source(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.accepted_cap(i1) -> i32
    func.func private @ac_init_top.accepted(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.last_accepted(i1) -> i32
    func.func private @ac_init_top.ledger.observed(i1) -> i32
    func.func private @ac_init_top.observer.last(i1) -> i32
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %0 = acir.config "enabled" {acir.id = "v2", acir.source_type = "bool"} : i1 loc(#loc6)
      %1 = acir.export %0 {acir.id = "v3", acir.source_type = "bool"} : i1 -> i1
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.ledger attributes {acir.instance = {definition = "src_mixed_ports_9bff12ad.Ledger", inputs = {}, members = {}, name = "top.ledger", parent = "top", rules = ["record"]}} {
    acir.contract {relations = [], rules = ["record"]}
    acir.cell @top.ledger.observed : i32 {initial_fn = "ac_init_top.ledger.observed", name = "top.ledger.observed", type = "u32"}
    func.func private @ac_init_top.source_cap(i1) -> i32
    func.func private @ac_init_top.source(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.accepted_cap(i1) -> i32
    func.func private @ac_init_top.accepted(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.last_accepted(i1) -> i32
    func.func private @ac_init_top.ledger.observed(i1) -> i32
    func.func private @ac_init_top.observer.last(i1) -> i32
    acir.rule @top.router_body.0.call.0.2.0.call.0.5 : (i32) -> () {acir.function = {contract = "Module:top.ledger", definition = "record", instance = "top.ledger", interface = "Module:top.ledger:record", kind = "rule", name = "top.router_body.0.call.0.2.0.call.0.5", output_type = "void", owner = "top.ledger", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "record"}} body {
    ^bb0(%arg0: i32):
      %0 = acir.ref @top.ledger::@top.ledger.observed {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc7)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.router attributes {acir.instance = {definition = "src_mixed_ports_9bff12ad.Router", inputs = {}, members = {}, name = "top.router", parent = "top", rules = ["route"]}} {
    acir.contract {relations = [], rules = ["route"]}
    func.func private @ac_init_top.source_cap(i1) -> i32
    func.func private @ac_init_top.source(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.accepted_cap(i1) -> i32
    func.func private @ac_init_top.accepted(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.last_accepted(i1) -> i32
    func.func private @ac_init_top.ledger.observed(i1) -> i32
    func.func private @ac_init_top.observer.last(i1) -> i32
    acir.rule @top.router_body.0.call.0.2 : (!acir.queue<i32>, i1) -> tuple<i32, i32, i32, i1, i1, i1> {acir.function = {contract = "Module:top.router", definition = "route", instance = "top.router", interface = "Module:top.router:route", kind = "rule", name = "top.router_body.0.call.0.2", output_type = ["queue<u32>", "cell<u32>", "signal<u32>"], owner = "top.router", params = [{consume = false, id = "v1", name = "message", targets = ["top.source"], type = "queue<u32>"}, {consume = false, id = "v2", name = "take", targets = [], type = "bool"}], public = true, result = "tuple<u32,u32,u32,bool,bool,bool>", rule = "route"}} body {
    ^bb0(%arg0: !acir.queue<i32>, %arg1: i1):
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.queue.peek %arg0 {acir.id = "v5", acir.source_type = "u32", role = "consume"} : !acir.queue<i32> -> i32 loc(#loc8)
      acir.rule.call @top.ledger::@top.router_body.0.call.0.2.0.call.0.5(%0) {acir.id = "v6", acir.source_type = "void", contract = "Module:top.ledger", instance = "top.ledger", interface = "Module:top.ledger:record", role = "rule", rule = "record"} : (i32) -> () loc(#loc9)
      cf.cond_br %arg1, ^bb1, ^bb2(%0 : i32)
    ^bb1:  // pred: ^bb0
      %1 = arith.addi %0, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc10)
      %2 = acir.aggregate(%1, %0, %0, %true, %true, %true) {acir.id = "v13", acir.source_type = "tuple<u32,u32,u32,bool,bool,bool>"} : (i32, i32, i32, i1, i1, i1) -> tuple<i32, i32, i32, i1, i1, i1> loc(#loc11)
      acir.return %2 : tuple<i32, i32, i32, i1, i1, i1> {acir.id = "v14", acir.source_type = "void"} loc(#loc11)
    ^bb2(%3: i32):  // pred: ^bb0
      %4 = acir.aggregate() {acir.id = "v19", acir.source_type = "u32"} : () -> i32 loc(#loc12)
      %5 = acir.aggregate() {acir.id = "v21", acir.source_type = "u32"} : () -> i32 loc(#loc12)
      %6 = acir.aggregate(%4, %5, %3, %false, %false, %true) {acir.id = "v24", acir.source_type = "tuple<u32,u32,u32,bool,bool,bool>"} : (i32, i32, i32, i1, i1, i1) -> tuple<i32, i32, i32, i1, i1, i1> loc(#loc12)
      acir.return %6 : tuple<i32, i32, i32, i1, i1, i1> {acir.id = "v25", acir.source_type = "void"} loc(#loc12)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.router_body", owner = "top.router", params = [], result = "void"}} {
      %0 = acir.ref @top::@top.source {acir.source_type = "queue<u32>"} : !acir.queue<i32>
      %1 = acir.external "top_body" "v3" {acir.source_type = "bool"} : i1
      %2 = acir.rule.call @top.router::@top.router_body.0.call.0.2(%0, %1) {acir.id = "v3", acir.source_type = "tuple<u32,u32,u32,bool,bool,bool>", contract = "Module:top.router", instance = "top.router", interface = "Module:top.router:route", output_bindings = [@top::@top.accepted, @top::@top.last_accepted, @top::@top.result], role = "rule", rule = "route"} : (!acir.queue<i32>, i1) -> tuple<i32, i32, i32, i1, i1, i1> loc(#loc13)
      acir.body.end {acir.id = "v13", acir.source_type = "void"} loc(#loc14)
    }
  }
  acir.module @top.observer attributes {acir.instance = {definition = "src_mixed_ports_9bff12ad.Observer", inputs = {}, members = {}, name = "top.observer", parent = "top", rules = ["sample"]}} {
    acir.contract {relations = [], rules = ["sample"]}
    acir.cell @top.observer.last : i32 {initial_fn = "ac_init_top.observer.last", name = "top.observer.last", type = "u32"}
    func.func private @ac_init_top.source_cap(i1) -> i32
    func.func private @ac_init_top.source(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.accepted_cap(i1) -> i32
    func.func private @ac_init_top.accepted(i1) -> !acir.vector<i32>
    func.func private @ac_init_top.last_accepted(i1) -> i32
    func.func private @ac_init_top.ledger.observed(i1) -> i32
    func.func private @ac_init_top.observer.last(i1) -> i32
    acir.rule @top.observer_body.0.call.2.1 : (!acir.signal<i32>) -> () {acir.function = {contract = "Module:top.observer", definition = "sample", instance = "top.observer", interface = "Module:top.observer:sample", kind = "rule", name = "top.observer_body.0.call.2.1", output_type = "void", owner = "top.observer", params = [{consume = false, id = "v1", name = "x", targets = ["top.result"], type = "signal<u32>"}], public = true, result = "void", rule = "sample"}} body {
    ^bb0(%arg0: !acir.signal<i32>):
      %0 = acir.signal.get %arg0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.signal<i32> -> i32 loc(#loc15)
      %1 = acir.ref @top.observer::@top.observer.last {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %1, %0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc16)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.observer_body", owner = "top.observer", params = [], result = "void"}} {
      %0 = acir.ref @top::@top.result {acir.source_type = "signal<u32>"} : !acir.signal<i32>
      acir.rule.call @top.observer::@top.observer_body.0.call.2.1(%0) {acir.id = "v2", acir.source_type = "void", contract = "Module:top.observer", instance = "top.observer", interface = "Module:top.observer:sample", role = "rule", rule = "sample"} : (!acir.signal<i32>) -> () loc(#loc17)
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
#loc7 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":12:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":35:17)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":36:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":38:21)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":38:13)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":39:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":42:16)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":42:9)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":22:16)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":22:9)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":25:9)
