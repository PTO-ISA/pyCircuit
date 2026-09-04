`timescale 1ns/1ps
// Canonical PYC wrapper for PULP common_cells cc_stream_xbar.
// Inputs and outputs use flat packed vectors; selection and index vectors are
// packed arrays at the implementation boundary.
module pyc_runtime_pulp_stream_xbar #(
  parameter integer NUM_INPUTS = 2,
  parameter integer NUM_OUTPUTS = 1,
  parameter integer DATA_WIDTH = 8,
  parameter integer OUT_SPILL_REG = 0,
  parameter integer EXT_PRIO = 0,
  parameter integer AXI_VALID_READY = 1,
  parameter integer LOCK_IN = 1
) (
  input  wire clk,
  input  wire reset_n,
  input  wire clear,
  input  wire clear_arb,
  input  wire [NUM_OUTPUTS*((NUM_INPUTS <= 1) ? 1 : $clog2(NUM_INPUTS))-1:0] rr_priority,
  input  wire [NUM_INPUTS*DATA_WIDTH-1:0] input_data,
  input  wire [NUM_INPUTS*((NUM_OUTPUTS <= 1) ? 1 : $clog2(NUM_OUTPUTS))-1:0] select_output,
  input  wire [NUM_INPUTS-1:0] input_valid,
  output wire [NUM_INPUTS-1:0] input_ready,
  output wire [NUM_OUTPUTS*DATA_WIDTH-1:0] output_data,
  output wire [NUM_OUTPUTS*((NUM_INPUTS <= 1) ? 1 : $clog2(NUM_INPUTS))-1:0] output_index,
  output wire [NUM_OUTPUTS-1:0] output_valid,
  input  wire [NUM_OUTPUTS-1:0] output_ready
);
  localparam integer INPUT_INDEX_WIDTH = (NUM_INPUTS <= 1) ? 1 : $clog2(NUM_INPUTS);
  localparam integer SELECT_WIDTH = (NUM_OUTPUTS <= 1) ? 1 : $clog2(NUM_OUTPUTS);
  localparam integer RR_BITS = NUM_OUTPUTS * INPUT_INDEX_WIDTH;
  localparam integer DATA_BITS = NUM_INPUTS * DATA_WIDTH;
  localparam integer SEL_BITS = NUM_INPUTS * SELECT_WIDTH;
  localparam integer OUT_DATA_BITS = NUM_OUTPUTS * DATA_WIDTH;
  localparam integer IDX_BITS = NUM_OUTPUTS * INPUT_INDEX_WIDTH;

  wire [NUM_OUTPUTS-1:0][INPUT_INDEX_WIDTH-1:0] rr_vec = rr_priority;
  wire [NUM_INPUTS-1:0][DATA_WIDTH-1:0] data_vec = input_data;
  wire [NUM_INPUTS-1:0][SELECT_WIDTH-1:0] sel_vec = select_output;
  wire [NUM_OUTPUTS-1:0][DATA_WIDTH-1:0] data_out_vec;
  wire [NUM_OUTPUTS-1:0][INPUT_INDEX_WIDTH-1:0] idx_out_vec;

  cc_stream_xbar #(
    .NumInp(NUM_INPUTS), .NumOut(NUM_OUTPUTS), .DataWidth(DATA_WIDTH),
    .OutSpillReg(OUT_SPILL_REG != 0), .ExtPrio(EXT_PRIO != 0),
    .AxiVldRdy(AXI_VALID_READY != 0), .LockIn(LOCK_IN != 0),
    .AxiVldMask('1)
  ) impl (
    .clk_i(clk), .rst_ni(reset_n), .clr_i(clear), .clr_arb_i(clear_arb),
    .rr_i(rr_vec), .data_i(data_vec), .sel_i(sel_vec),
    .valid_i(input_valid), .ready_o(input_ready), .data_o(data_out_vec),
    .idx_o(idx_out_vec), .valid_o(output_valid), .ready_i(output_ready)
  );

  assign output_data = data_out_vec;
  assign output_index = idx_out_vec;
endmodule
