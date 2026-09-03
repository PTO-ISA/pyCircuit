// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

// This file covers all 16 SSA-legal ACIR public types. Channel's 17th
// parser/printer case is covered by ACIRTypesTest.PublicTypeInventoryRoundTrips.
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "test_protocol"}> ({
    "ac.role"() <{sym_name = "producer", dual = @consumer, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "consumer", dual = @producer, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "idle", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "send", from = @producer, to = @consumer, payload = i8, action = "offer"}> : () -> ()
    "ac.transition"() <{source = @idle, target = @idle, event = @send, transfer = true}> ({}) : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "MemoryPort"}> ({
    "ac.role"() <{sym_name = "initiator", dual = @target, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "target", dual = @initiator, cardinality = "exclusive"}> : () -> ()
  }) : () -> ()
  "builtin.unrealized_conversion_cast"() : () -> !ac.struct<@types::@Header>
  "builtin.unrealized_conversion_cast"() : () -> !ac.packet<@types::@Request>
  "builtin.unrealized_conversion_cast"() : () -> !ac.transaction<@types::@Dma>
  "builtin.unrealized_conversion_cast"() : () -> !ac.enum<@types::@Opcode>
  "builtin.unrealized_conversion_cast"() : () -> !ac.union<@types::@Payload>
  "builtin.unrealized_conversion_cast"() : () -> !ac.optional<i32>
  "builtin.unrealized_conversion_cast"() : () -> !ac.list<!ac.struct<@types::@Entry>>
  "builtin.unrealized_conversion_cast"() : () -> !ac.vector<4 x i8>
  "builtin.unrealized_conversion_cast"() : () -> !ac.vector<9223372036854775807 x i8>
  "builtin.unrealized_conversion_cast"() : () -> !ac.flow<i8, @test_protocol>
  "builtin.unrealized_conversion_cast"() : () -> !ac.endpoint<@MemoryPort, @target>
  "builtin.unrealized_conversion_cast"() : () -> !ac.resource_ref<@Memory, @reader>
  "builtin.unrealized_conversion_cast"() : () -> !ac.duration<cycles>
  "builtin.unrealized_conversion_cast"() : () -> !ac.duration<ticks>
  "builtin.unrealized_conversion_cast"() : () -> !ac.duration<seconds>
  "builtin.unrealized_conversion_cast"() : () -> !ac.duration<milliseconds>
  "builtin.unrealized_conversion_cast"() : () -> !ac.duration<microseconds>
  "builtin.unrealized_conversion_cast"() : () -> !ac.duration<nanoseconds>
  "builtin.unrealized_conversion_cast"() : () -> !ac.duration<picoseconds>
  "builtin.unrealized_conversion_cast"() : () -> !ac.rate<bytes, cycles>
  "builtin.unrealized_conversion_cast"() : () -> !ac.rate<bits, ticks>
  "builtin.unrealized_conversion_cast"() : () -> !ac.rate<entries, seconds>
  "builtin.unrealized_conversion_cast"() : () -> !ac.rate<packets, milliseconds>
  "builtin.unrealized_conversion_cast"() : () -> !ac.rate<transactions, microseconds>
  "builtin.unrealized_conversion_cast"() : () -> !ac.rate<bytes, nanoseconds>
  "builtin.unrealized_conversion_cast"() : () -> !ac.rate<bytes, picoseconds>
  "builtin.unrealized_conversion_cast"() : () -> !ac.event<!ac.transaction<@types::@Dma>>
  "builtin.unrealized_conversion_cast"() : () -> !ac.address<@global>
  "builtin.unrealized_conversion_cast"() : () -> !ac.resource_token<@Memory>
}

// CHECK: !ac.struct<@types::@Header>
// CHECK: !ac.packet<@types::@Request>
// CHECK: !ac.transaction<@types::@Dma>
// CHECK: !ac.enum<@types::@Opcode>
// CHECK: !ac.union<@types::@Payload>
// CHECK: !ac.optional<i32>
// CHECK: !ac.list<!ac.struct<@types::@Entry>>
// CHECK: !ac.vector<4 x i8>
// CHECK: !ac.vector<9223372036854775807 x i8>
// CHECK: !ac.flow<i8, @test_protocol>
// CHECK: !ac.endpoint<@MemoryPort, @target>
// CHECK: !ac.resource_ref<@Memory, @reader>
// CHECK: !ac.duration<cycles>
// CHECK: !ac.duration<ticks>
// CHECK: !ac.duration<seconds>
// CHECK: !ac.duration<milliseconds>
// CHECK: !ac.duration<microseconds>
// CHECK: !ac.duration<nanoseconds>
// CHECK: !ac.duration<picoseconds>
// CHECK: !ac.rate<bytes, cycles>
// CHECK: !ac.rate<bits, ticks>
// CHECK: !ac.rate<entries, seconds>
// CHECK: !ac.rate<packets, milliseconds>
// CHECK: !ac.rate<transactions, microseconds>
// CHECK: !ac.rate<bytes, nanoseconds>
// CHECK: !ac.rate<bytes, picoseconds>
// CHECK: !ac.event<!ac.transaction<@types::@Dma>>
// CHECK: !ac.address<@global>
// CHECK: !ac.resource_token<@Memory>
