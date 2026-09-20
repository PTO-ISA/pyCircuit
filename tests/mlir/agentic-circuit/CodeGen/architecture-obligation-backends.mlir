// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: %pyc_opt %t/input.mlir -o %t/model.pyc
// RUN: %pycc %t/model.pyc --cpp %t/generated.cpp
// RUN: %FileCheck %s --check-prefix=CPP < %t/generated.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t/generated.cpp
// RUN: %pycc %t/model.pyc --cpp %t/generated.second.cpp
// RUN: cmp %t/generated.cpp %t/generated.second.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -I%t %t/pass.cpp -o %t/cpp-pass
// RUN: %t/cpp-pass
// RUN: %cxx -std=c++20 -I%source_root/library -I%t %t/fail.cpp -o %t/cpp-fail
// RUN: %not --crash %t/cpp-fail 2>&1 | %FileCheck %s --check-prefix=FAIL
// RUN: %pycc %t/model.pyc --verilog %t/model.sv
// RUN: %FileCheck %s --check-prefix=SVA < %t/model.sv
// RUN: %pycc %t/model.pyc --verilog %t/model.second.sv
// RUN: cmp %t/model.sv %t/model.second.sv
// RUN: %python %source_root/flows/tools/check_generated_rtl.py %t/model.sv --compare %t/model.second.sv --json-out %t/audit.json --html-out %t/diff.html
// RUN: %not %python %source_root/flows/tools/check_generated_rtl.py %t/model.sv --require-synthesis-admissible 2>&1 | %FileCheck %s --check-prefix=SYNTHESIS
// RUN: test -s %t/audit.json
// RUN: test -s %t/diff.html
// RUN: verilator --lint-only --assert -Wno-fatal -I%source_root/library/verilog %t/model.sv
// RUN: verilator --binary --assert --top-module tb_pass -Wno-fatal -I%source_root/library/verilog --Mdir %t/pass_obj %t/model.sv %t/tb_pass.sv
// RUN: %t/pass_obj/Vtb_pass
// RUN: verilator --binary --assert --top-module tb_fail -Wno-fatal -I%source_root/library/verilog --Mdir %t/fail_obj %t/model.sv %t/tb_fail.sv
// RUN: %not %t/fail_obj/Vtb_fail 2>&1 | %FileCheck %s --check-prefix=FAIL

//--- input.mlir

module attributes {pyc.top = @obligation_top, pyc.frontend.contract = "pycircuit"} {
  func.func @obligation_top(%clk: !pyc.clock, %rst: !pyc.reset, %value: i8) attributes {
      arg_names = ["clk", "rst", "value"],
      result_names = [],
      pyc.value_params = [],
      pyc.value_param_types = [],
      pyc.kind = "module",
      pyc.inline = "false",
      pyc.params = "{}",
      pyc.base = "obligation_top",
      pyc.struct.metrics = "{\"ast_node_count\":3,\"collection_count\":0,\"collection_instance_count\":0,\"estimated_inline_cost\":3,\"hardware_call_count\":0,\"instance_count\":0,\"loop_count\":0,\"module_call_count\":0,\"module_family_collection_count\":0,\"repeat_pressure\":0,\"repeated_body_clusters\":[],\"source_loc\":0,\"state_alloc_count\":0,\"state_call_count\":0}",
      pyc.struct.collections = "[]"
    } {
    %maximum = pyc.constant 127 : i8
    %true = pyc.constant 1 : i1
    %greater = pyc.cmp %maximum, %value {predicate = "ult"} : i8, i8 -> i1
    %safe = pyc.not %greater : i1
    pyc.assert %true {msg = "ready valid integrity", obligation_id = "ready_valid:channel", obligation_kind = "ready_valid_integrity", severity = "error", sampling_kind = "pre_publish", sampling_edge = "none", sample_anchor = "bounded", source = "fixture.py:7:3", ndf_ids = ["NDF-RV-001"]}
    pyc.assert %true {msg = "no partial commit", obligation_id = "atomicity:commit", obligation_kind = "no_partial_commit", severity = "fatal", sampling_kind = "pre_publish", sampling_edge = "none", sample_anchor = "bounded", source = "fixture.py:7:3", ndf_ids = ["NDF-ATOMIC-001"]}
    pyc.assert %true {msg = "no stale update", obligation_id = "stale:update", obligation_kind = "no_stale_update", severity = "error", sampling_kind = "pre_publish", sampling_edge = "none", sample_anchor = "bounded", source = "fixture.py:7:3", ndf_ids = ["NDF-STALE-001"]}
    pyc.assert %true {msg = "credit balance", obligation_id = "credit:balance", obligation_kind = "credit_balance", severity = "error", sampling_kind = "pre_publish", sampling_edge = "none", sample_anchor = "bounded", source = "fixture.py:7:3", ndf_ids = ["NDF-CREDIT-001"]}
    pyc.assert %true {msg = "pipeline alignment", obligation_id = "pipeline:alignment", obligation_kind = "pipeline_alignment", severity = "error", sampling_kind = "pre_publish", sampling_edge = "none", sample_anchor = "bounded", source = "fixture.py:7:3", ndf_ids = ["NDF-PIPE-001"]}
    pyc.assert %safe {msg = "value must be at most 127", obligation_id = "range:bounded", obligation_kind = "range", severity = "error", sampling_kind = "pre_publish", sampling_edge = "none", sample_anchor = "bounded", source = "fixture.py:7:3", ndf_ids = ["NDF-RANGE-001"]}
    func.return
  }
}

// CPP: std::uint64_t obligation_range_bounded_checks = 0;
// CPP: std::uint64_t obligation_range_bounded_failures = 0;
// CPP: obligation/range:bounded/condition
// CPP: architecture_obligation id=range:bounded kind=range severity=error sampling=pre_publish/none@bounded source=fixture.py:7:3 ndf=[NDF-RANGE-001]: value must be at most 127

// SVA-DAG: obligation_ready_valid_channel: assert property
// SVA-DAG: obligation_atomicity_commit: assert property
// SVA-DAG: obligation_stale_update: assert property
// SVA-DAG: obligation_credit_balance: assert property
// SVA-DAG: obligation_pipeline_alignment: assert property
// SVA-DAG: obligation_range_bounded: assert property (@(posedge clk) (
// SVA-DAG: else $fatal(1, "architecture_obligation id=range:bounded kind=range severity=error sampling=pre_publish/none@bounded source=fixture.py:7:3 ndf=[NDF-RANGE-001]: value must be at most 127");
// SVA-DAG: obligation_range_bounded_coverage: cover property (@(posedge clk) (

// FAIL: architecture_obligation id=range:bounded kind=range severity=error sampling=pre_publish/none@bounded source=fixture.py:7:3 ndf=[NDF-RANGE-001]: value must be at most 127
// SYNTHESIS: runtime-checked obligations cannot authorize synthesis

//--- pass.cpp
#include "generated.cpp"

int main() {
  pyc::gen::obligation_top model;
  model.value = pyc::cpp::Wire<8>({127});
  model.eval();
  return 0;
}

//--- fail.cpp
#include "generated.cpp"

int main() {
  pyc::gen::obligation_top model;
  model.value = pyc::cpp::Wire<8>({128});
  model.eval();
  return 0;
}

//--- tb_pass.sv
module tb_pass;
  logic clk = 0;
  logic rst = 0;
  logic [7:0] value = 8'd127;
  obligation_top dut (.*);
  initial begin
    #1 clk = 1;
    #1 $finish;
  end
endmodule

//--- tb_fail.sv
module tb_fail;
  logic clk = 0;
  logic rst = 0;
  logic [7:0] value = 8'd128;
  obligation_top dut (.*);
  initial begin
    #1 clk = 1;
    #1 $finish;
  end
endmodule
