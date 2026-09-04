// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  "builtin.unrealized_conversion_cast"() : () -> !acsim.value<@cpp_i32>
  "builtin.unrealized_conversion_cast"() : () -> !acsim.expr<@cpp_i32>
  "builtin.unrealized_conversion_cast"() : () -> !acsim.owner<@fifo_binding>
  "builtin.unrealized_conversion_cast"() : () -> !acsim.ref<@fifo_binding>
  "builtin.unrealized_conversion_cast"() : () -> !acsim.port<@stream, @producer, @packet, @ready_valid>
  "builtin.unrealized_conversion_cast"() : () -> !acsim.resource<@memory, @initiator>
  "builtin.unrealized_conversion_cast"() : () -> !acsim.array<[2, 3], !acsim.owner<@fifo_binding>>
  "builtin.unrealized_conversion_cast"() : () -> !acsim.object_id
  "builtin.unrealized_conversion_cast"() : () -> !acsim.activation_id
  "builtin.unrealized_conversion_cast"() : () -> !acsim.pc<@tick>
  "builtin.unrealized_conversion_cast"() : () -> !acsim.wake<@event>
}

// CHECK: !acsim.value<@cpp_i32>
// CHECK: !acsim.expr<@cpp_i32>
// CHECK: !acsim.owner<@fifo_binding>
// CHECK: !acsim.ref<@fifo_binding>
// CHECK: !acsim.port<@stream, @producer, @packet, @ready_valid>
// CHECK: !acsim.resource<@memory, @initiator>
// CHECK: !acsim.array<[2, 3], !acsim.owner<@fifo_binding>>
// CHECK: !acsim.object_id
// CHECK: !acsim.activation_id
// CHECK: !acsim.pc<@tick>
// CHECK: !acsim.wake<@event>
