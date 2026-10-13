module attributes {acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, exports = {}, format_version = 7 : i64, top = "SignalLink", type_order = [], types = {}}} {
  func.func @ac_init_producer.offset(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_producer.offset", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_producer.count(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_producer.count", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_producer.result(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_producer.result", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_reader.last(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_reader.last", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  acir.module @producer attributes {acir.instance = {contract = "Module:producer", definition = "src_signal_link_6cc8a7a9.Producer", inputs = {enabled = {resources = [], type = "bool", value}}, interfaces = {configure = "Module:producer:configure", peek = "Module:producer:peek"}, members = {count = {resources = ["producer.count"], type = "cell<u32>", value}, enabled = {resources = [], type = "bool", value}, offset = {resources = ["producer.offset"], type = "cell<u32>", value}, result = {resources = ["producer.result"], type = "signal<u32>", value}}, methods = ["configure", "peek"], name = "producer", outputs = {resources = ["producer.result"], type = "signal<u32>", value}, parent = "top"}} {
    acir.interface @configure {implementations = [@producer::@top.configure.0.call.0.3, @producer::@ac_summary_producer.configure.0.call.1.1], orders = []}
    acir.interface @peek {implementations = [@producer::@ac_summary_producer.peek.0.call.2.0], orders = []}
    acir.cell @producer.offset : i32 {initial_fn = "ac_init_producer.offset", name = "producer.offset", type = "u32"}
    acir.cell @producer.count : i32 {initial_fn = "ac_init_producer.count", name = "producer.count", type = "u32"}
    acir.signal @producer.result : i32 {initial_fn = "ac_init_producer.result", name = "producer.result", type = "u32"}
    func.func private @ac_init_producer.offset(i1) -> i32
    func.func private @ac_init_producer.count(i1) -> i32
    func.func private @ac_init_producer.result(i1) -> i32
    func.func private @ac_init_reader.last(i1) -> i32
    acir.method @top.configure.0.call.0.3 : (i32) -> () {acir.function = {contract = "Module:producer", definition = "configure", instance = "producer", interface = "Module:producer:configure", kind = "method", method = "configure", name = "top.configure.0.call.0.3", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @producer::@configure} body {
    ^bb0(%arg0: i32):
      %0 = acir.ref @producer::@producer.offset {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_producer.configure.0.call.1.1 : (i32) -> () {acir.function = {contract = "Module:producer", definition = "configure", instance = "producer", interface = "Module:producer:configure", kind = "method", method = "configure", name = "ac_summary_producer.configure.0.call.1.1", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @producer::@configure} body {
    ^bb0(%arg0: i32):
      %0 = acir.ref @producer::@producer.offset {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_producer.peek.0.call.2.0 : () -> i32 {acir.function = {contract = "Module:producer", definition = "peek", instance = "producer", interface = "Module:producer:peek", kind = "method", method = "peek", name = "ac_summary_producer.peek.0.call.2.0", params = [], result = "u32", summary = true}, interface_ref = @producer::@peek} body {
      %0 = acir.ref @producer::@producer.result {acir.id = "v3", acir.source_type = "signal<u32>"} : !acir.signal<i32>
      %1 = acir.signal.get %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.signal<i32> -> i32
      acir.return %1 : i32 {acir.id = "v5", acir.source_type = "void"}
    }
    acir.rule @producer.advance : () -> () {acir.function = {kind = "rule", name = "producer.advance", owner = "producer", params = [], result = "void", rule = "advance"}, output_types = []} body {
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.ref @producer::@producer.count {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = acir.ref @producer::@producer.offset {acir.id = "v5", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %3 = acir.cell.read %2 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %4 = arith.addi %1, %3 {acir.id = "v7", acir.source_type = "u32"} : i32
      %5 = acir.ref @producer::@producer.result {acir.id = "v8", acir.source_type = "signal<u32>"} : !acir.signal<i32>
      acir.signal.set %5, %4 {acir.id = "v9", acir.source_type = "void"} : !acir.signal<i32>, i32
      %6 = acir.cell.read %0 {acir.id = "v10", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %7 = arith.addi %6, %c1_i32 {acir.id = "v12", acir.source_type = "u32"} : i32
      acir.cell.write %0, %7 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i32>, i32
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.work attributes {acir.function = {kind = "work", name = "producer_Work", owner = "producer", params = [], result = "void"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %c16_i32 = arith.constant {acir.source_type = "u32"} 16 : i32
      %0 = acir.config "enabled" {acir.id = "v1", acir.source_type = "bool"} : i1
      %1 = acir.ref @producer::@producer.count {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = scf.if %0 -> (i1) {
        %3 = acir.cell.read %1 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
        %4 = arith.cmpi ult, %3, %c16_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
        scf.yield %4 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %2 {
        acir.rule.call @producer::@producer.advance() {acir.id = "v12", acir.source_type = "void", output_bindings = []} : () -> ()
      }
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @reader attributes {acir.instance = {contract = "Module:reader", definition = "src_signal_link_6cc8a7a9.Reader", inputs = {source = {resources = ["producer.result"], type = "signal<u32>", value}}, interfaces = {}, members = {last = {resources = ["reader.last"], type = "cell<u32>", value}, source = {resources = ["producer.result"], type = "signal<u32>", value}}, methods = [], name = "reader", outputs = {resources = ["reader.last"], type = "cell<u32>", value}, parent = "top"}} {
    acir.cell @reader.last : i32 {initial_fn = "ac_init_reader.last", name = "reader.last", type = "u32"}
    func.func private @ac_init_producer.offset(i1) -> i32
    func.func private @ac_init_producer.count(i1) -> i32
    func.func private @ac_init_producer.result(i1) -> i32
    func.func private @ac_init_reader.last(i1) -> i32
    acir.rule @reader.observe : (i32, i1) -> () {acir.function = {guard_info = {name = "ready", source = "variable"}, kind = "rule", name = "reader.observe", owner = "reader", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}, {consume = false, id = "v2", name = "$guard_observe", targets = [], type = "bool"}], result = "void", rule = "observe"}, output_types = []} guard {
    ^bb0(%arg0: i32, %arg1: i1):
      acir.guard.yield %arg1 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i1):
      %0 = acir.ref @reader::@reader.last {acir.id = "v5", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<i32>, i32
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.work attributes {acir.function = {kind = "work", name = "reader_Work", owner = "reader", params = [], result = "void"}} {
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.ref @producer::@producer.result {acir.id = "v1", acir.source_type = "signal<u32>"} : !acir.signal<i32>
      %1 = acir.signal.get %0 {acir.id = "v2", acir.source_type = "u32", role = "observe"} : !acir.signal<i32> -> i32
      %2 = arith.addi %1, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32
      %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
      acir.rule.call @reader::@reader.observe(%2, %3) {acir.id = "v7", acir.source_type = "void", output_bindings = []} : (i32, i1) -> ()
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @top attributes {acir.instance = {contract = "Module:top", definition = "src_signal_link_6cc8a7a9.SignalLink", inputs = {enabled = {resources = [], type = "bool", value}}, interfaces = {}, members = {enabled = {resources = [], type = "bool", value}, producer = {instance = "producer"}, reader = {instance = "reader"}}, methods = [], name = "top", outputs = [{resources = ["producer.result"], type = "signal<u32>", value}, {resources = ["reader.last"], type = "cell<u32>", value}], parent}} {
    acir.instance @producer = @producer
    acir.instance @reader = @reader
    func.func private @ac_init_producer.offset(i1) -> i32
    func.func private @ac_init_producer.count(i1) -> i32
    func.func private @ac_init_producer.result(i1) -> i32
    func.func private @ac_init_reader.last(i1) -> i32
    acir.rule @top.configure : () -> () {acir.function = {kind = "rule", name = "top.configure", owner = "top", params = [], result = "void", rule = "configure"}, output_types = []} body {
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
      acir.method.call @producer::@top.configure.0.call.0.3(%c4_i32) {acir.id = "v4", acir.source_type = "void", contract = "Module:producer", instance = "producer", interface = "Module:producer:configure", method = "configure", role = "method"} : (i32) -> ()
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.work attributes {acir.function = {kind = "work", name = "top_Work", owner = "top", params = [], result = "void"}} {
      acir.rule.call @top::@top.configure() {acir.id = "v1", acir.source_type = "void", output_bindings = []} : () -> ()
      acir.work.end {acir.source_type = "void"}
    }
  }
}

