`timescale 1ns/1ps
// Canonical PYC runtime wrapper for PULP common_cells cc_stream_arbiter.
// Upstream: https://github.com/pulp-platform/common_cells.git
// Revision: 63b7c50d43e462b59506f69d341ff1e40202866d (Solderpad-0.51)
//
// The upstream type parameter is made explicit here so callers only need
// packed vectors.  ArbMode=0 selects fair round-robin arbitration; ArbMode=1
// selects the documented fixed-priority mode.
module pyc_runtime_pulp_stream_arbiter #(
  parameter integer NUM_INPUTS = 2,
  parameter integer DATA_WIDTH = 8,
  parameter integer ARB_MODE = 0
) (
  input  wire                              clk,
  input  wire                              reset_n,
  input  wire                              clear,
  input  wire [NUM_INPUTS*DATA_WIDTH-1:0]  input_data,
  input  wire [NUM_INPUTS-1:0]             input_valid,
  output wire [NUM_INPUTS-1:0]             input_ready,
  output wire [DATA_WIDTH-1:0]             output_data,
  output wire                              output_valid,
  input  wire                              output_ready
);
  // ``cc_stream_arbiter`` exposes its payload as a packed two-dimensional
  // vector (``data_t [NumInp-1:0]``).  Keep the public PYC ABI flat while
  // using an explicitly packed intermediate; this avoids the array-vs-vector
  // port mismatch reported by Verilator for a typedef-based unpacked array.
  wire [NUM_INPUTS-1:0][DATA_WIDTH-1:0] input_data_vec = input_data;

  cc_stream_arbiter #(
    .data_t(logic [DATA_WIDTH-1:0]),
    .NumInp(NUM_INPUTS),
    .ArbMode((ARB_MODE != 0) ? cc_pkg::ARB_PRIO : cc_pkg::ARB_RR)
  ) impl (
    .clk_i(clk),
    .rst_ni(reset_n),
    .clr_i(clear),
    .inp_data_i(input_data_vec),
    .inp_valid_i(input_valid),
    .inp_ready_o(input_ready),
    .oup_data_o(output_data),
    .oup_valid_o(output_valid),
    .oup_ready_i(output_ready)
  );
endmodule
