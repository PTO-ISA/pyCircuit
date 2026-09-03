// RUN: %acir_opcode_catalog > %t
// RUN: diff %source_root/schemas/agentic-circuit/opcodes.json %t
// RUN: %FileCheck %s < %t
// RUN: %not grep -E '"operation":"ac\.(decode|dispatch|rename|retire)"' %t

// CHECK: "contract_epoch":"0.4"
// CHECK-SAME: "operation":"ac{{\.}}barrier"
// CHECK-SAME: "operation":"ac{{\.}}broadcast"
// CHECK-SAME: "operation":"ac{{\.}}credit"
// CHECK-SAME: "operation":"ac{{\.}}dependency"
// CHECK-SAME: "operation":"ac{{\.}}expect"
// CHECK-SAME: "operation":"ac{{\.}}feedback"
// CHECK-SAME: "operation":"ac{{\.}}fork"
// CHECK-SAME: "operation":"ac{{\.}}memory{{\.}}instance"
// CHECK-SAME: "operation":"ac{{\.}}memory{{\.}}request"
// CHECK-SAME: "operation":"ac{{\.}}merge"
// CHECK-SAME: "operation":"ac{{\.}}observe"
// CHECK-SAME: "operation":"ac{{\.}}reorder"
// CHECK-SAME: "operation":"ac{{\.}}route"
// CHECK-SAME: "operation":"ac{{\.}}scope"
// CHECK-SAME: "operation":"ac{{\.}}select"
// CHECK-SAME: "operation":"ac{{\.}}sink"
// CHECK-SAME: "operation":"ac{{\.}}source"
// CHECK-SAME: "operation":"ac{{\.}}transform"
// CHECK-SAME: "schema":"agentic-circuit-opcode-catalog"
