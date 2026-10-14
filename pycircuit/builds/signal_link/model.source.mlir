module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, format_version = 9 : i64, top = "SignalLink", type_order = [], types = {}}} {
  func.func @ac_init_top.producer.offset(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.producer.offset", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.producer.count(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.producer.count", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc2)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.reader.last(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.reader.last", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc3)
    return %c0_i32 : i32
  }
  acir.module @top attributes {acir.instance = {definition = "src_signal_link_6cc8a7a9.SignalLink", name = "top", parent, rules = ["configure"]}} {
    acir.contract {relations = [], rules = ["configure"]}
    acir.signal @top.result : i32 {name = "top.result", type = "u32"}
    func.func private @ac_init_top.producer.offset(i1) -> i32
    func.func private @ac_init_top.producer.count(i1) -> i32
    func.func private @ac_init_top.reader.last(i1) -> i32
    acir.rule @top_body.0.call.1.4 : () -> () {acir.function = {contract = "Module:top", definition = "configure", instance = "top", interface = "Module:top:configure", kind = "rule", name = "top_body.0.call.1.4", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "configure"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc4)
      acir.rule.call @top.producer::@top_body.0.call.1.4.0.call.1.4(%c4_i32) {acir.id = "v5", acir.source_type = "void", contract = "Module:top.producer", instance = "top.producer", interface = "Module:top.producer:configure", role = "rule", rule = "configure"} : (i32) -> () loc(#loc5)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.config "enabled" {acir.id = "v2", acir.source_type = "bool"} : i1 loc(#loc6)
      %1 = acir.export %0 {acir.id = "v3", acir.source_type = "bool"} : i1 -> i1
      acir.module.call @top.producer loc(#loc7)
      acir.rule.call @top::@top_body.0.call.1.4() {acir.id = "v5", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:configure", role = "rule", rule = "configure"} : () -> () loc(#loc8)
      %2 = acir.ref @top::@top.result {acir.source_type = "signal<u32>"} : !acir.signal<i32>
      acir.module.call @top.reader loc(#loc9)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.producer attributes {acir.instance = {definition = "src_signal_link_6cc8a7a9.Producer", name = "top.producer", parent = "top", rules = ["advance", "configure", "peek"]}} {
    acir.contract {relations = [], rules = ["advance", "configure", "peek"]}
    acir.cell @top.producer.offset : i32 {initial_fn = "ac_init_top.producer.offset", name = "top.producer.offset", type = "u32"}
    acir.cell @top.producer.count : i32 {initial_fn = "ac_init_top.producer.count", name = "top.producer.count", type = "u32"}
    func.func private @ac_init_top.producer.offset(i1) -> i32
    func.func private @ac_init_top.producer.count(i1) -> i32
    func.func private @ac_init_top.reader.last(i1) -> i32
    acir.rule @top.producer_body.0.call.0.10 : () -> i32 {acir.function = {contract = "Module:top.producer", definition = "advance", instance = "top.producer", interface = "Module:top.producer:advance", kind = "rule", name = "top.producer_body.0.call.0.10", output_type = "signal<u32>", owner = "top.producer", params = [], public = true, result = "u32", rule = "advance"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.producer::@top.producer.count {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc10)
      %3 = acir.ref @top.producer::@top.producer.offset {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %4 = acir.cell.read %3 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc11)
      %5 = arith.addi %2, %4 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc10)
      %6 = acir.ref @top.producer::@top.producer.count {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %7 = acir.cell.read %6 {acir.id = "v12", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc12)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc13)
      %8 = arith.addi %7, %c1_i32 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc12)
      %9 = acir.ref @top.producer::@top.producer.count {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %9, %8 {acir.id = "v16", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc14)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %10 = "acir.optional"(%5, %true_0) {acir.source_type = "u32"} : (i32, i1) -> i32
      acir.return %10 : i32 {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.1.4.0.call.1.4 : (i32) -> () {acir.function = {contract = "Module:top.producer", definition = "configure", instance = "top.producer", interface = "Module:top.producer:configure", kind = "rule", name = "top_body.0.call.1.4.0.call.1.4", output_type = "void", owner = "top.producer", params = [{id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "configure"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.producer::@top.producer.offset {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc15)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.producer_body", owner = "top.producer", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.external "top_body" "v3" {acir.source_type = "bool"} : i1
      %1 = acir.ref @top.producer::@top.producer.count {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = scf.if %0 -> (i1) {
        %5 = acir.cell.read %1 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc16)
        %c16_i32 = arith.constant {acir.source_type = "u32"} 16 : i32 loc(#loc17)
        %6 = arith.cmpi ult, %5, %c16_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc16)
        scf.yield %6 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %3 = acir.ref @top::@top.result {acir.source_type = "signal<u32>"} : !acir.signal<i32>
      %4 = scf.if %2 -> (i1) {
        %5 = acir.rule.call @top.producer::@top.producer_body.0.call.0.10() {acir.id = "v11", acir.source_type = "u32", contract = "Module:top.producer", instance = "top.producer", interface = "Module:top.producer:advance", output_binding = @top::@top.result, role = "rule", rule = "advance"} : () -> i32 loc(#loc18)
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      } else {
        scf.yield %true : i1
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.reader attributes {acir.instance = {definition = "src_signal_link_6cc8a7a9.Reader", name = "top.reader", parent = "top", rules = ["observe"]}} {
    acir.contract {relations = [], rules = ["observe"]}
    acir.cell @top.reader.last : i32 {initial_fn = "ac_init_top.reader.last", name = "top.reader.last", type = "u32"}
    func.func private @ac_init_top.producer.offset(i1) -> i32
    func.func private @ac_init_top.producer.count(i1) -> i32
    func.func private @ac_init_top.reader.last(i1) -> i32
    acir.rule @top.reader_body.0.call.3.7 : (i32) -> () {acir.function = {contract = "Module:top.reader", definition = "observe", instance = "top.reader", interface = "Module:top.reader:observe", kind = "rule", name = "top.reader_body.0.call.3.7", output_type = "void", owner = "top.reader", params = [{id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "observe"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.reader::@top.reader.last {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc19)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.reader_body", owner = "top.reader", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.result {acir.source_type = "signal<u32>"} : !acir.signal<i32>
      %1 = acir.signal.get %0 {acir.id = "v3", acir.source_type = "u32", role = "observe"} : !acir.signal<i32> -> i32 loc(#loc20)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc21)
      %2 = arith.addi %1, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc22)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
      %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc24)
      scf.if %3 {
        acir.rule.call @top.reader::@top.reader_body.0.call.3.7(%2) {acir.id = "v8", acir.source_type = "void", contract = "Module:top.reader", instance = "top.reader", interface = "Module:top.reader:observe", role = "rule", rule = "observe"} : (i32) -> () loc(#loc25)
      } else {
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":8:42)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":9:41)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":33:40)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":56:28)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":56:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":49:18)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":59:18)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":60:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":61:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:17)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:25)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:17)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:25)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:9)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":13:9)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":26:24)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":26:32)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":27:20)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":37:9)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":40:17)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":41:28)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":41:20)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:29)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:17)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":44:13)
