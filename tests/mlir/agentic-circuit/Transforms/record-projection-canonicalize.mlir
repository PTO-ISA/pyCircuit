// RUN: %acir_opt %s -canonicalize | %FileCheck %s
// RUN: %acir_opt %s -canonicalize --mlir-print-debuginfo | %FileCheck %s --check-prefix=SOURCE

builtin.module  {
  ac.type_scope @types {
    ac.struct @Pair fields [{name = "left", type = i8}, {name = "right", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Pair> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}

  func.func @project() -> (!ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>) {
    %left_a = ac.var.constant 1 : i8 as !ac.var<i8>
    %right_a = ac.var.constant 2 : i8 as !ac.var<i8>
    %left_b = ac.var.constant 3 : i8 as !ac.var<i8>
    %right_b = ac.var.constant 4 : i8 as !ac.var<i8>
    %replacement = ac.var.constant 5 : i8 as !ac.var<i8>
    %condition = ac.var.constant true as !ac.var<i1>
    %pair_a = ac.var.record %left_a, %right_a : !ac.var<i8>, !ac.var<i8> -> !ac.var<!ac.struct<@types::@Pair>>
    %pair_b = ac.var.record %left_b, %right_b : !ac.var<i8>, !ac.var<i8> -> !ac.var<!ac.struct<@types::@Pair>>
    %direct = ac.var.get %pair_a field "left" : !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<i8>
    %updated = ac.var.with %pair_a, %replacement field "right" : !ac.var<!ac.struct<@types::@Pair>>, !ac.var<i8> -> !ac.var<!ac.struct<@types::@Pair>>
    %same = ac.var.get %updated field "right" : !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<i8>
    %other = ac.var.get %updated field "left" : !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<i8>
    %selected = ac.var.select %condition, %pair_a, %pair_b : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %selected_right = ac.var.get %selected field "right" : !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<i8>
    func.return %direct, %same, %other, %selected_right : !ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>
  }

  func.func @shared_diamond(%input: !ac.var<!ac.struct<@types::@Pair>>) -> !ac.var<i8> {
    %condition = ac.var.constant true as !ac.var<i1>
    %s0 = ac.var.select %condition, %input, %input : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %s1 = ac.var.select %condition, %s0, %s0 : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %s2 = ac.var.select %condition, %s1, %s1 : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %s3 = ac.var.select %condition, %s2, %s2 : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %s4 = ac.var.select %condition, %s3, %s3 : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %s5 = ac.var.select %condition, %s4, %s4 : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %s6 = ac.var.select %condition, %s5, %s5 : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %s7 = ac.var.select %condition, %s6, %s6 : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %s8 = ac.var.select %condition, %s7, %s7 : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %s9 = ac.var.select %condition, %s8, %s8 : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %s10 = ac.var.select %condition, %s9, %s9 : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %s11 = ac.var.select %condition, %s10, %s10 : !ac.var<i1>, !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<!ac.struct<@types::@Pair>>
    %left = ac.var.get %s11 field "left" : !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<i8>
    func.return %left : !ac.var<i8>
  }

  func.func @source_locations() -> (!ac.var<i8>, !ac.var<i8>) {
    %left = ac.var.constant 1 : i8 as !ac.var<i8> loc("producer.py":3:1)
    %right = ac.var.constant 2 : i8 as !ac.var<i8> loc("right.py":4:1)
    %replacement = ac.var.constant 3 : i8 as !ac.var<i8> loc("replacement.py":5:1)
    %record = ac.var.record %left, %right : !ac.var<i8>, !ac.var<i8> -> !ac.var<!ac.struct<@types::@Pair>> loc("record.py":10:1)
    %direct = ac.var.get %record field "left" : !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<i8> loc("projection.py":20:1)
    %updated = ac.var.with %record, %replacement field "right" : !ac.var<!ac.struct<@types::@Pair>>, !ac.var<i8> -> !ac.var<!ac.struct<@types::@Pair>> loc("update.py":25:1)
    %through_update = ac.var.get %updated field "right" : !ac.var<!ac.struct<@types::@Pair>> -> !ac.var<i8> loc("updated_projection.py":30:1)
    func.return %direct, %through_update : !ac.var<i8>, !ac.var<i8> loc("consumer.py":40:1)
  }
}

// CHECK-LABEL: func.func @project
// CHECK-NOT: ac.var.get
// CHECK-NOT: ac.var.record
// CHECK-NOT: ac.var.with
// CHECK: %[[SELECTED:.*]] = ac.var.select %{{.*}}, %{{.*}}, %{{.*}} : !ac.var<i1>, !ac.var<i8> -> !ac.var<i8>
// CHECK: return %{{.*}}, %{{.*}}, %{{.*}}, %[[SELECTED]]
// CHECK-LABEL: func.func @shared_diamond
// CHECK-COUNT-2: ac.var.get
// CHECK: ac.var.select %{{.*}}, %{{.*}}, %{{.*}} : !ac.var<i1>, !ac.var<i8> -> !ac.var<i8>
// CHECK: return

// SOURCE: #[[PRODUCER:loc[0-9]+]] = loc("producer.py":3:1)
// SOURCE-NEXT: #[[RECORD:loc[0-9]+]] = loc("record.py":10:1)
// SOURCE-NEXT: #[[PROJECTION:loc[0-9]+]] = loc("projection.py":20:1)
// SOURCE: #[[REPLACEMENT:loc[0-9]+]] = loc("replacement.py":5:1)
// SOURCE-NEXT: #[[UPDATE:loc[0-9]+]] = loc("update.py":25:1)
// SOURCE-NEXT: #[[UPDATED_PROJECTION:loc[0-9]+]] = loc("updated_projection.py":30:1)
// SOURCE: loc(fused[#[[PRODUCER]], #[[RECORD]], #[[PROJECTION]]])
// SOURCE-NEXT: {{.*}}loc(fused[#[[REPLACEMENT]], #[[UPDATE]], #[[UPDATED_PROJECTION]]])
