// Parameterized round-robin one-hot arbiter.
// Cursor/state ownership remains in the surrounding PYC graph.
module pyc_rr_arbiter #(
  parameter integer NUM_INPUTS = 2,
  parameter integer POINTER_WIDTH = 1
) (
  input  wire [NUM_INPUTS-1:0] req,
  input  wire [POINTER_WIDTH-1:0] cursor,
  output wire [NUM_INPUTS-1:0] grant
);
  integer offset;
  integer index;
  reg found;
  reg [NUM_INPUTS-1:0] grant_r;

  always @* begin
    grant_r = {NUM_INPUTS{1'b0}};
    found = 1'b0;
    for (offset = 0; offset < NUM_INPUTS; offset = offset + 1) begin
      index = integer'(cursor) + offset;
      if (index >= NUM_INPUTS)
        index = index - NUM_INPUTS;
      if (index >= NUM_INPUTS)
        index = index - NUM_INPUTS;
      if (!found && req[index]) begin
        grant_r[index] = 1'b1;
        found = 1'b1;
      end
    end
  end

  assign grant = grant_r;
endmodule
