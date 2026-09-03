// RUN: %split_file %s %t
// RUN: %acir_queue_cxxgen %t/same-type.mlir > %t.same.cpp
// RUN: %FileCheck %s --check-prefix=SAME < %t.same.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -c %t.same.cpp -o %t.same.o
// RUN: %acir_queue_cxxgen %t/different-type.mlir > %t.different.cpp
// RUN: %FileCheck %s --check-prefix=DIFFERENT < %t.different.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -c %t.different.cpp -o %t.different.o

// SAME-LABEL: struct block_0_release_policy {
// SAME-NOT: table_first
// SAME: gfsim::SimTable<std::uint16_t> *table_second{};
// SAME-LABEL: bool operator()(gfsim::Epoch epoch) const {
// SAME: table_second->checkedAt
// SAME: block_0_release_policy{&slot_0_state_, &table_1_}

// DIFFERENT-LABEL: struct block_0_release_policy {
// DIFFERENT: gfsim::SimTable<std::uint8_t> *table_first{};
// DIFFERENT: gfsim::SimTable<std::uint16_t> *table_second{};
// DIFFERENT-LABEL: bool operator()(gfsim::Epoch epoch) const {
// DIFFERENT: table_first->checkedAt
// DIFFERENT: table_second->checkedAt
// DIFFERENT: block_0_release_policy{&slot_0_state_, &table_0_, &table_1_}

//--- same-type.mlir
module attributes {ac.contract_epoch = "0.4", ac.system = "same_type"} {
  ac.table @first entry i16 entries 4 init 0 owner "/" stable_id "table/first"
  ac.table @second entry i16 entries 4 init 0 owner "/" stable_id "table/second"
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  ac.slot @pending, %input owner "/" stable_id "slot/pending" : !ac.queue<i8>
  ac.slot.release @pending when {
    %index = ac.var.constant 0 : i64 as !ac.var<i64>
    %entry = ac.table.get @second [%index] : !ac.var<i64> -> !ac.var<i16>
    %zero = ac.var.constant 0 : i16 as !ac.var<i16>
    %ready = ac.var.cmp "sgt" %entry, %zero : !ac.var<i16> -> !ac.var<i1>
    ac.slot.yield %ready : !ac.var<i1>
  } {ac.endpoint_path = "/pending__release", ac.name = "pending__release"}
  %first_value = ac.table.read @first depth 1 latency 1 address {
    %index = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %index : !ac.var<i64>
  } when {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/first_value", ac.name = "first_value"} -> !ac.queue<i16>
  %second_value = ac.table.read @second depth 1 latency 1 address {
    %index = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %index : !ac.var<i64>
  } when {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/second_value", ac.name = "second_value"} -> !ac.queue<i16>
  ac.sink %first_value {ac.name = "first_sink"} : !ac.queue<i16>
  ac.sink %second_value {ac.name = "second_sink"} : !ac.queue<i16>
}

//--- different-type.mlir
module attributes {ac.contract_epoch = "0.4", ac.system = "different_type"} {
  ac.table @first entry i8 entries 4 init 0 owner "/" stable_id "table/first"
  ac.table @second entry i16 entries 4 init 0 owner "/" stable_id "table/second"
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  ac.slot @pending, %input owner "/" stable_id "slot/pending" : !ac.queue<i8>
  ac.slot.release @pending when {
    %index = ac.var.constant 0 : i64 as !ac.var<i64>
    %first_entry = ac.table.get @first [%index] : !ac.var<i64> -> !ac.var<i8>
    %first_zero = ac.var.constant 0 : i8 as !ac.var<i8>
    %first_ready = ac.var.cmp "sgt" %first_entry, %first_zero : !ac.var<i8> -> !ac.var<i1>
    %second_entry = ac.table.get @second [%index] : !ac.var<i64> -> !ac.var<i16>
    %second_zero = ac.var.constant 0 : i16 as !ac.var<i16>
    %second_ready = ac.var.cmp "sgt" %second_entry, %second_zero : !ac.var<i16> -> !ac.var<i1>
    %ready = ac.var.mul %first_ready, %second_ready : !ac.var<i1>
    ac.slot.yield %ready : !ac.var<i1>
  } {ac.endpoint_path = "/pending__release", ac.name = "pending__release"}
  %first_value = ac.table.read @first depth 1 latency 1 address {
    %index = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %index : !ac.var<i64>
  } when {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/first_value", ac.name = "first_value"} -> !ac.queue<i8>
  %second_value = ac.table.read @second depth 1 latency 1 address {
    %index = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %index : !ac.var<i64>
  } when {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/second_value", ac.name = "second_value"} -> !ac.queue<i16>
  ac.sink %first_value {ac.name = "first_sink"} : !ac.queue<i8>
  ac.sink %second_value {ac.name = "second_sink"} : !ac.queue<i16>
}
