// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "wire"}> ({
    "ac.role"() <{sym_name = "sender", dual = @receiver, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "receiver", dual = @sender, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "idle", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "send", from = @sender, to = @receiver, payload = i8, action = "offer"}> : () -> ()
    "ac.event"() <{sym_name = "accepted", from = @receiver, to = @sender, payload = i8, action = "accept"}> : () -> ()
    "ac.transition"() <{source = @idle, target = @idle, event = @send, transfer = true}> ({}) : () -> ()
    "ac.transition"() <{source = @idle, target = @idle, event = @accepted}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "backpressure", value = "none"}> : () -> ()
    "ac.guarantee"() <{kind = "ordering", value = "fifo"}> : () -> ()
    "ac.guarantee"() <{kind = "delivery", value = "exactly_once"}> : () -> ()
    "ac.guarantee"() <{kind = "completion", value = "on_accept"}> : () -> ()
    "ac.guarantee"() <{kind = "max_inflight", value = 1 : i64}> : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "Wire"}> ({
    "ac.role"() <{sym_name = "source", dual = @sink, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "sink", dual = @source, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "data", type = !ac.channel<i8, @wire>, from = @source, to = @sink, protocol_from = @sender, protocol_to = @receiver}> : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "SharedWire"}> ({
    "ac.role"() <{sym_name = "source", dual = @sink, cardinality = "shared"}> : () -> ()
    "ac.role"() <{sym_name = "sink", dual = @source, cardinality = "shared"}> : () -> ()
  }) : () -> ()
  "ac.protocol"() <{sym_name = "exchange"}> ({
    "ac.role"() <{sym_name = "initiator", dual = @target, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "target", dual = @initiator, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "ready", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "request", from = @initiator, to = @target, payload = i8, action = "offer"}> : () -> ()
    "ac.event"() <{sym_name = "reply", from = @target, to = @initiator, payload = i16, action = "response"}> : () -> ()
    "ac.transition"() <{source = @ready, target = @ready, event = @request, transfer = true}> ({}) : () -> ()
    "ac.transition"() <{source = @ready, target = @ready, event = @reply}> ({}) : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "Exchange"}> ({
    "ac.role"() <{sym_name = "initiator", dual = @target, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "target", dual = @initiator, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "request", type = !ac.channel<i8, @exchange>, from = @initiator, to = @target, protocol_from = @initiator, protocol_to = @target}> : () -> ()
    "ac.port"() <{sym_name = "reply", type = !ac.channel<i16, @exchange>, from = @target, to = @initiator, protocol_from = @target, protocol_to = @initiator}> : () -> ()
  }) : () -> ()
  %endpoint = "builtin.unrealized_conversion_cast"() : () -> !ac.endpoint<@Wire, @source>
  %shared = "builtin.unrealized_conversion_cast"() : () -> !ac.endpoint<@SharedWire, @source>
  %use0 = "builtin.unrealized_conversion_cast"(%shared) : (!ac.endpoint<@SharedWire, @source>) -> i1
  %use1 = "builtin.unrealized_conversion_cast"(%shared) : (!ac.endpoint<@SharedWire, @source>) -> i1
}

// CHECK: ac.interface
// CHECK: ac.port
// CHECK-SAME: !ac.channel<i8, @wire>
// CHECK: !ac.endpoint<@Wire, @source>
