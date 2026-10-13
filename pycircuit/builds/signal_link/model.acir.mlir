module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, exports = {}, format_version = 8 : i64, top = "SignalLink", type_order = [], types = {}}} {
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
  acir.module @top attributes {acir.instance = {definition = "src_signal_link_6cc8a7a9.SignalLink", inputs = {}, members = {}, name = "top", outputs, parent, rules = ["configure"]}} {
    acir.contract {relations = [], rules = ["configure"]}
    func.func private @ac_init_top.producer.offset(i1) -> i32
    func.func private @ac_init_top.producer.count(i1) -> i32
    func.func private @ac_init_top.reader.last(i1) -> i32
    acir.rule @top_body.0.call.1.4 : () -> () {acir.function = {contract = "Module:top", definition = "configure", instance = "top", interface = "Module:top:configure", kind = "rule", name = "top_body.0.call.1.4", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "configure"}} body {
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc4)
      acir.rule.call @top.producer::@top_body.0.call.1.4.0.call.1.3(%c4_i32) {acir.id = "v4", acir.source_type = "void", contract = "Module:top.producer", instance = "top.producer", interface = "Module:top.producer:configure", role = "rule", rule = "configure"} : (i32) -> () loc(#loc5)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %0 = acir.config "enabled" {acir.id = "v1", acir.source_type = "bool"} : i1 loc(#loc6)
      %1 = acir.export %0 {acir.id = "v2", acir.source_type = "bool"} : i1 -> i1
      %2 = acir.ref @top.producer::@top.producer.result {acir.source_type = "signal<u32>"} : !acir.signal<i32>
      %3 = acir.signal.get %2 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.signal<i32> -> i32 loc(#loc7)
      acir.rule.call @top::@top_body.0.call.1.4() {acir.id = "v5", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:configure", role = "rule", rule = "configure"} : () -> () loc(#loc8)
      %4 = acir.ref @top.reader::@top.reader.last {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %5 = acir.cell.read %4 {acir.id = "v8", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc9)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.producer attributes {acir.instance = {definition = "src_signal_link_6cc8a7a9.Producer", inputs = {}, members = {}, name = "top.producer", outputs, parent = "top", rules = ["advance", "configure", "peek"]}} {
    acir.contract {relations = [], rules = ["advance", "configure", "peek"]}
    acir.cell @top.producer.offset : i32 {initial_fn = "ac_init_top.producer.offset", name = "top.producer.offset", type = "u32"}
    acir.cell @top.producer.count : i32 {initial_fn = "ac_init_top.producer.count", name = "top.producer.count", type = "u32"}
    acir.signal @top.producer.result : i32 {name = "top.producer.result", type = "u32"}
    func.func private @ac_init_top.producer.offset(i1) -> i32
    func.func private @ac_init_top.producer.count(i1) -> i32
    func.func private @ac_init_top.reader.last(i1) -> i32
    acir.rule @top.producer_body.0.call.0.11 : () -> () {acir.function = {contract = "Module:top.producer", definition = "advance", instance = "top.producer", interface = "Module:top.producer:advance", kind = "rule", name = "top.producer_body.0.call.0.11", output_type = "void", owner = "top.producer", params = [], public = true, result = "void", rule = "advance"}} body {
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc10)
      %0 = acir.ref @top.producer::@top.producer.count {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc11)
      %2 = acir.ref @top.producer::@top.producer.offset {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %3 = acir.cell.read %2 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc12)
      %4 = arith.addi %1, %3 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc11)
      %5 = acir.ref @top.producer::@top.producer.result {acir.source_type = "signal<u32>"} : !acir.signal<i32>
      acir.signal.set %5, %4 {acir.id = "v9", acir.source_type = "void"} : !acir.signal<i32>, i32 loc(#loc13)
      %6 = acir.cell.read %0 {acir.id = "v11", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc14)
      %7 = arith.addi %6, %c1_i32 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc14)
      acir.cell.write %0, %7 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc15)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.1.4.0.call.1.3 : (i32) -> () {acir.function = {contract = "Module:top.producer", definition = "configure", instance = "top.producer", interface = "Module:top.producer:configure", kind = "rule", name = "top_body.0.call.1.4.0.call.1.3", output_type = "void", owner = "top.producer", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "configure"}} body {
    ^bb0(%arg0: i32):
      %0 = acir.ref @top.producer::@top.producer.offset {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc16)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.producer_body", owner = "top.producer", params = [], result = "void"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %c16_i32 = arith.constant {acir.source_type = "u32"} 16 : i32
      %0 = acir.external "top_body" "v2" {acir.source_type = "bool"} : i1
      %1 = scf.if %0 -> (i1) {
        %2 = acir.ref @top.producer::@top.producer.count {acir.source_type = "cell<u32>"} : !acir.cell<i32>
        %3 = acir.cell.read %2 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc17)
        %4 = arith.cmpi ult, %3, %c16_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc17)
        scf.yield %4 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %1 {
        acir.rule.call @top.producer::@top.producer_body.0.call.0.11() {acir.id = "v12", acir.source_type = "void", contract = "Module:top.producer", instance = "top.producer", interface = "Module:top.producer:advance", role = "rule", rule = "advance"} : () -> () loc(#loc18)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.reader attributes {acir.instance = {definition = "src_signal_link_6cc8a7a9.Reader", inputs = {}, members = {}, name = "top.reader", outputs, parent = "top", rules = ["observe"]}} {
    acir.contract {relations = [], rules = ["observe"]}
    acir.cell @top.reader.last : i32 {initial_fn = "ac_init_top.reader.last", name = "top.reader.last", type = "u32"}
    func.func private @ac_init_top.producer.offset(i1) -> i32
    func.func private @ac_init_top.producer.count(i1) -> i32
    func.func private @ac_init_top.reader.last(i1) -> i32
    acir.rule @top.reader_body.0.call.3.7 : (i32) -> () {acir.function = {contract = "Module:top.reader", definition = "observe", instance = "top.reader", interface = "Module:top.reader:observe", kind = "rule", name = "top.reader_body.0.call.3.7", output_type = "void", owner = "top.reader", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "observe"}} body {
    ^bb0(%arg0: i32):
      %0 = acir.ref @top.reader::@top.reader.last {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc19)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.reader_body", owner = "top.reader", params = [], result = "void"}} {
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc20)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc21)
      %0 = acir.ref @top.producer::@top.producer.result {acir.source_type = "signal<u32>"} : !acir.signal<i32>
      %1 = acir.signal.get %0 {acir.id = "v2", acir.source_type = "u32", role = "observe"} : !acir.signal<i32> -> i32 loc(#loc22)
      %2 = arith.addi %1, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc23)
      %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc24)
      scf.if %3 {
        acir.rule.call @top.reader::@top.reader_body.0.call.3.7(%2) {acir.id = "v8", acir.source_type = "void", contract = "Module:top.reader", instance = "top.reader", interface = "Module:top.reader:observe", role = "rule", rule = "observe"} : (i32) -> () loc(#loc25)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":8:42)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":9:41)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":34:40)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":57:28)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":57:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":51:18)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":60:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":61:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":62:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:25)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:18)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:26)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:9)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:17)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:9)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":14:9)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":26:24)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":27:13)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":38:9)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":43:29)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:28)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":41:17)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:20)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":43:17)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":45:13)
