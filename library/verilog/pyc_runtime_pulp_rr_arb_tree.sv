`timescale 1ns/1ps
// Canonical PYC wrapper for PULP common_cells cc_rr_arb_tree.
// The payload ABI is flattened at the boundary; the implementation keeps the
// upstream packed-vector ordering used by Verilator/Yosys.
module pyc_runtime_pulp_rr_arb_tree #(
  parameter integer NUM_INPUTS = 2,
  parameter integer DATA_WIDTH = 8,
  parameter integer EXT_PRIO = 0,
  parameter integer AXI_VALID_READY = 1,
  parameter integer LOCK_IN = 1,
  parameter integer FAIR_ARB = 1
) (
  input  wire                         clk,
  input  wire                         reset_n,
  input  wire                         clear,
  input  wire [((NUM_INPUTS <= 1) ? 1 : $clog2(NUM_INPUTS))-1:0] rr_priority,
  input  wire [NUM_INPUTS-1:0]        requests,
  output wire [NUM_INPUTS-1:0]        grants,
  input  wire [NUM_INPUTS*DATA_WIDTH-1:0] input_data,
  output wire                         request_valid,
  input  wire                         grant_ready,
  output wire [DATA_WIDTH-1:0]        output_data,
  output wire [((NUM_INPUTS <= 1) ? 1 : $clog2(NUM_INPUTS))-1:0] grant_index
);
  localparam integer INDEX_WIDTH = (NUM_INPUTS <= 1) ? 1 : $clog2(NUM_INPUTS);
  // Explicitly packed intermediate avoids typedef/unpacked-array port
  // mismatches in Verilator while preserving one payload per input.
  wire [NUM_INPUTS-1:0][DATA_WIDTH-1:0] data_vec = input_data;

  cc_rr_arb_tree #(
    .NumIn(NUM_INPUTS),
    .DataWidth(DATA_WIDTH),
    .ExtPrio(EXT_PRIO != 0),
    .AxiVldRdy(AXI_VALID_READY != 0),
    .LockIn(LOCK_IN != 0),
    .FairArb(FAIR_ARB != 0)
  ) impl (
    .clk_i(clk), .rst_ni(reset_n), .clr_i(clear),
    .rr_i(rr_priority), .req_i(requests), .gnt_o(grants),
    .data_i(data_vec), .req_o(request_valid), .gnt_i(grant_ready),
    .data_o(output_data), .idx_o(grant_index)
  );
endmodule
