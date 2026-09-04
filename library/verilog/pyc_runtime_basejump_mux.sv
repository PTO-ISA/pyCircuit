// Canonical Agentic Circuit adapter for BaseJump bsg_mux.
module pyc_runtime_basejump_mux #(
  parameter integer WIDTH = 8,
  parameter integer ELS = 2,
  parameter integer HARDEN = 0,
  localparam integer SELECT_WIDTH = (ELS <= 1) ? 1 : $clog2(ELS)
) (
  input  wire [ELS-1:0][WIDTH-1:0] data,
  input  wire [SELECT_WIDTH-1:0] select,
  output wire [WIDTH-1:0] out
);
  initial begin
    if (WIDTH < 1 || ELS < 1) $error("WIDTH and ELS must be positive");
  end

  bsg_mux #(.width_p(WIDTH), .els_p(ELS), .harden_p(HARDEN)) impl (
    .data_i(data), .sel_i(select), .data_o(out)
  );
endmodule
