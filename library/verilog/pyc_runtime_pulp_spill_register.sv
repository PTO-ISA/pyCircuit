// Canonical PYC wrapper for PULP common_cells cc_spill_register.
module pyc_runtime_pulp_spill_register #(
  parameter integer DATA_WIDTH = 8,
  parameter integer BYPASS = 0
) (
  input wire clk,
  input wire rst_n,
  input wire clear,
  input wire valid_in,
  output wire ready_in,
  input wire [DATA_WIDTH-1:0] data_in,
  output wire valid_out,
  input wire ready_out,
  output wire [DATA_WIDTH-1:0] data_out
);
  cc_spill_register #(.data_t(logic [DATA_WIDTH-1:0]), .Bypass(BYPASS[0])) impl (
    .clk_i(clk), .rst_ni(rst_n), .clr_i(clear), .valid_i(valid_in), .ready_o(ready_in),
    .data_i(data_in), .valid_o(valid_out), .ready_i(ready_out), .data_o(data_out)
  );
endmodule
