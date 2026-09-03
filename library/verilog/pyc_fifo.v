// PYC ready/valid FIFO migrated from pyCircuit's cycle-level primitive set.
module pyc_fifo #(
  parameter integer WIDTH = 1,
  parameter integer DEPTH = 2
) (
  input  wire             clk,
  input  wire             rst,
  input  wire             in_valid,
  output wire             in_ready,
  input  wire [WIDTH-1:0] in_data,
  output wire             out_valid,
  input  wire             out_ready,
  output wire [WIDTH-1:0] out_data
);
  function integer pyc_clog2;
    input integer value;
    integer i;
    begin
      pyc_clog2 = 0;
      for (i = value - 1; i > 0; i = i >> 1)
        pyc_clog2 = pyc_clog2 + 1;
    end
  endfunction

  localparam integer PTR_W = (DEPTH <= 1) ? 1 : pyc_clog2(DEPTH);
  // Keep comparisons in the pointer/count widths. Unsized integer parameters
  // otherwise trigger width expansion warnings in Verilator.
  localparam [PTR_W:0] DEPTH_LIMIT = (PTR_W + 1)'(DEPTH);
  localparam [PTR_W-1:0] LAST_PTR = PTR_W'(DEPTH - 1);
  reg [WIDTH-1:0] storage [0:DEPTH-1];
  reg [PTR_W-1:0] rd_ptr;
  reg [PTR_W-1:0] wr_ptr;
  reg [PTR_W:0] count;

  assign in_ready = (count < DEPTH_LIMIT) || (out_ready && out_valid);
  assign out_valid = (count != 0);
  assign out_data = out_valid ? storage[rd_ptr] : {WIDTH{1'b0}};

  wire do_pop = out_valid && out_ready;
  wire do_push = in_valid && in_ready;

  function [PTR_W-1:0] bump_ptr;
    input [PTR_W-1:0] p;
    begin
      if (DEPTH <= 1)
        bump_ptr = {PTR_W{1'b0}};
      else if (p == LAST_PTR)
        bump_ptr = {PTR_W{1'b0}};
      else
        bump_ptr = p + 1'b1;
    end
  endfunction

  always @(posedge clk) begin
    if (rst) begin
      rd_ptr <= {PTR_W{1'b0}};
      wr_ptr <= {PTR_W{1'b0}};
      count <= {(PTR_W + 1){1'b0}};
    end else begin
      case ({do_push, do_pop})
        2'b01: begin
          rd_ptr <= bump_ptr(rd_ptr);
          count <= count - 1'b1;
        end
        2'b10: begin
          storage[wr_ptr] <= in_data;
          wr_ptr <= bump_ptr(wr_ptr);
          count <= count + 1'b1;
        end
        2'b11: begin
          storage[wr_ptr] <= in_data;
          rd_ptr <= bump_ptr(rd_ptr);
          wr_ptr <= bump_ptr(wr_ptr);
        end
        default: begin
        end
      endcase
    end
  end
endmodule
