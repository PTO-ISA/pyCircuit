// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/queue-zero.mlir 2>&1 | %FileCheck %s --check-prefix=QUEUE-ZERO
// RUN: %not %acir_opt %t/queue-watermarks.mlir 2>&1 | %FileCheck %s --check-prefix=WATERMARKS
// RUN: %not %acir_opt %t/queue-protocol.mlir 2>&1 | %FileCheck %s --check-prefix=PROTOCOL
// RUN: %not %acir_opt %t/event-unstable.mlir 2>&1 | %FileCheck %s --check-prefix=EVENT-ORDER
// RUN: %not %acir_opt %t/event-domain.mlir 2>&1 | %FileCheck %s --check-prefix=DOMAIN
// RUN: %not %acir_opt %t/resource-width.mlir 2>&1 | %FileCheck %s --check-prefix=ISSUE
// RUN: %not %acir_opt %t/resource-ii.mlir 2>&1 | %FileCheck %s --check-prefix=II
// RUN: %not %acir_opt %t/resource-latency.mlir 2>&1 | %FileCheck %s --check-prefix=LATENCY
// RUN: %not %acir_opt %t/resource-lifecycle.mlir 2>&1 | %FileCheck %s --check-prefix=LIFECYCLE
// RUN: %not %acir_opt %t/resource-arbiter.mlir 2>&1 | %FileCheck %s --check-prefix=ARBITER
// RUN: %not %acir_opt %t/resource-class.mlir 2>&1 | %FileCheck %s --check-prefix=CLASS
// RUN: %not %acir_opt %t/duplicate-owner.mlir 2>&1 | %FileCheck %s --check-prefix=OWNER
// RUN: %not %acir_opt %t/orphan.mlir 2>&1 | %FileCheck %s --check-prefix=PLACEMENT
// RUN: %not %acir_opt %t/queue-bytes.mlir 2>&1 | %FileCheck %s --check-prefix=BYTES
// RUN: %not %acir_opt %t/queue-order.mlir 2>&1 | %FileCheck %s --check-prefix=QUEUE-ORDER
// RUN: %not %acir_opt %t/queue-owner.mlir 2>&1 | %FileCheck %s --check-prefix=QUEUE-OWNER
// RUN: %not %acir_opt %t/queue-payload.mlir 2>&1 | %FileCheck %s --check-prefix=QUEUE-PAYLOAD
// RUN: %not %acir_opt %t/owner-segment.mlir 2>&1 | %FileCheck %s --check-prefix=OWNER-SEGMENT
// RUN: %not %acir_opt %t/delay.mlir 2>&1 | %FileCheck %s --check-prefix=DELAY
// RUN: %not %acir_opt %t/event-capacity.mlir 2>&1 | %FileCheck %s --check-prefix=EVENT-CAPACITY
// RUN: %not %acir_opt %t/event-payload.mlir 2>&1 | %FileCheck %s --check-prefix=EVENT-PAYLOAD
// RUN: %not %acir_opt %t/resource-capacity.mlir 2>&1 | %FileCheck %s --check-prefix=RESOURCE-CAPACITY
// RUN: %not %acir_opt %t/resource-kind.mlir 2>&1 | %FileCheck %s --check-prefix=RESOURCE-KIND
// RUN: %not %acir_opt %t/resource-symbol-latency.mlir 2>&1 | %FileCheck %s --check-prefix=SYMBOL-LATENCY
// RUN: %not %acir_opt %t/resource-ownership.mlir 2>&1 | %FileCheck %s --check-prefix=RESOURCE-OWNERSHIP
// RUN: %not %acir_opt %t/exclusive-arbiter.mlir 2>&1 | %FileCheck %s --check-prefix=EXCLUSIVE-ARBITER
// RUN: %not %acir_opt %t/duplicate-class.mlir 2>&1 | %FileCheck %s --check-prefix=DUPLICATE-CLASS
// RUN: %not %acir_opt %t/queue-schema.mlir 2>&1 | %FileCheck %s --check-prefix=QUEUE-SCHEMA
// RUN: %not %acir_opt %t/resource-latency-schema.mlir 2>&1 | %FileCheck %s --check-prefix=LATENCY-SCHEMA
// RUN: %not %acir_opt %t/resource-arbiter-kind.mlir 2>&1 | %FileCheck %s --check-prefix=ARBITER-KIND
// RUN: %not %acir_opt %t/fifo-weakened.mlir 2>&1 | %FileCheck %s --check-prefix=FIFO-WEAKENED
// RUN: %not %acir_opt %t/per-key-no-correlation.mlir 2>&1 | %FileCheck %s --check-prefix=PER-KEY-CORRELATION

//--- queue-zero.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = i32, entry_capacity = 0 : i64, ordering = "fifo", protocol = @p, ownership = "exclusive", delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// QUEUE-ZERO: entry capacity must be positive

//--- queue-watermarks.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = i32, entry_capacity = 8 : i64, ordering = "fifo", protocol = @p, ownership = "exclusive", watermarks = {low = 7 : i64, high = 7 : i64}, delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// WATERMARKS: watermarks require 0 <= low < high <= entry capacity

//--- queue-protocol.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = i32, entry_capacity = 8 : i64, ordering = "fifo", protocol = @missing, ownership = "exclusive", delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// PROTOCOL: endpoint protocol '@missing' is unresolved

//--- event-unstable.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.event_queue"() <{sym_name = "e", stable_id = "e", path = "e", payload = !ac.event<i32>, capacity = 4 : i64, ordering = "time_only", time_domain = @clock, delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// EVENT-ORDER: ordering must be exactly 'time_then_sequence'

//--- event-domain.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.event_queue"() <{sym_name = "e", stable_id = "e", path = "e", payload = !ac.event<i32>, capacity = 4 : i64, ordering = "time_then_sequence", time_domain = @clock, delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// DOMAIN: time domain '@clock' is unresolved

//--- resource-width.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 2 : i64, issue_width = 3 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// ISSUE: issue width must be in [1, capacity]

//--- resource-ii.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 2 : i64, issue_width = 1 : i64, initiation_interval = 0 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// II: initiation interval must be at least one global tick

//--- resource-latency.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 2 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 0 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// LATENCY: fixed latency ticks must be positive

//--- resource-lifecycle.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 2 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "eager", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// LIFECYCLE: lifecycle requires exact reservation/release/cancellation schema

//--- resource-arbiter.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 2 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "shared", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// ARBITER: shared or contested resource requires one arbitration owner

//--- resource-class.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 2 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [@missing], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// CLASS: transaction class '@missing' is unresolved

//--- duplicate-owner.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "a", stable_id = "same", path = "a", capacity = 1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.resource"() <{sym_name = "b", stable_id = "same", path = "b", capacity = 1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// OWNER: duplicate local structural stable id 'same'

//--- orphan.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = i32, entry_capacity = 1 : i64, ordering = "fifo", protocol = @p, ownership = "exclusive", delay_ticks = 1 : i64}> : () -> ()
}
// PLACEMENT: must be a direct child of the unique ac.module Graph block

//--- queue-bytes.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = i32, entry_capacity = 1 : i64, byte_capacity = -1 : i64, ordering = "fifo", protocol = @p, ownership = "exclusive", delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// BYTES: byte capacity must be positive when present

//--- queue-order.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = i32, entry_capacity = 1 : i64, ordering = "unordered", protocol = @p, ownership = "exclusive", delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// QUEUE-ORDER: ordering must be 'fifo' or 'per_key'

//--- queue-owner.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = i32, entry_capacity = 1 : i64, ordering = "fifo", protocol = @p, ownership = "shared", delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// QUEUE-OWNER: queue ownership must be exactly 'exclusive'

//--- queue-payload.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = !ac.endpoint<@I, @r>, entry_capacity = 1 : i64, ordering = "fifo", protocol = @p, ownership = "exclusive", delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// QUEUE-PAYLOAD: queue payload must be a normative ACIR value type

//--- owner-segment.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "bad.name", stable_id = "r", path = "r", capacity = 1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// OWNER-SEGMENT: owner name, stable id, and path must be stable local segments

//--- delay.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 0 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// DELAY: stateful declaration delay_ticks must be exactly one positive tick

//--- event-capacity.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.event_queue"() <{sym_name = "e", stable_id = "e", path = "e", payload = !ac.event<i32>, capacity = -1 : i64, ordering = "time_then_sequence", time_domain = @clock, delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// EVENT-CAPACITY: event queue capacity must be positive

//--- event-payload.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.event_queue"() <{sym_name = "e", stable_id = "e", path = "e", payload = i32, capacity = 1 : i64, ordering = "time_then_sequence", time_domain = @clock, delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// EVENT-PAYLOAD: event queue payload must be an exact !ac.event type

//--- resource-capacity.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = -1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// RESOURCE-CAPACITY: resource capacity must be positive

//--- resource-kind.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "dynamic"}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// RESOURCE-KIND: latency model kind must be 'fixed' or 'symbol'

//--- resource-symbol-latency.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "symbol", ref = @missing}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// SYMBOL-LATENCY: symbol latency model reference is unresolved

//--- resource-ownership.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "public", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// RESOURCE-OWNERSHIP: resource ownership must be exclusive, shared, or contested

//--- exclusive-arbiter.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", arbitration_owner = @x, transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// EXCLUSIVE-ARBITER: exclusive resource cannot declare an arbitration owner

//--- duplicate-class.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = []}> : () -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [@types::@T, @types::@T], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// DUPLICATE-CLASS: duplicate transaction class

//--- queue-schema.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "start", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "done", initial = false, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "finish", from = @a, to = @b, payload = i64, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @start, target = @done, event = @finish}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "completion", value = "on_terminal_phase"}> : () -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = i32, entry_capacity = 1 : i64, ordering = "fifo", protocol = @p, ownership = "exclusive", delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// QUEUE-SCHEMA: queue payload does not match endpoint protocol schema

//--- resource-latency-schema.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64, extra = true}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "exclusive", transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// LATENCY-SCHEMA: fixed latency model requires exact kind/ticks schema

//--- resource-arbiter-kind.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.time_domain"() <{sym_name = "clock", period = 1 : i64, phase = 0 : i64, tick_scale = 1 : i64}> : () -> ()
    "ac.resource"() <{sym_name = "r", stable_id = "r", path = "r", capacity = 1 : i64, issue_width = 1 : i64, initiation_interval = 1 : i64, latency_model = {kind = "fixed", ticks = 1 : i64}, lifecycle = {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}, ownership = "shared", arbitration_owner = @clock, transaction_classes = [], delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// ARBITER-KIND: arbitration owner '@clock' is unresolved

//--- fifo-weakened.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i32, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "ordering", value = "fifo"}> : () -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = i32, entry_capacity = 1 : i64, ordering = "per_key", protocol = @p, ownership = "exclusive", delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// FIFO-WEAKENED: queue ordering 'per_key' weakens protocol ordering 'fifo'

//--- per-key-no-correlation.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i32, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "ordering", value = "unordered"}> : () -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = i32, entry_capacity = 1 : i64, ordering = "per_key", protocol = @p, ownership = "exclusive", delay_ticks = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// PER-KEY-CORRELATION: per_key queue storage requires protocol correlation semantics
