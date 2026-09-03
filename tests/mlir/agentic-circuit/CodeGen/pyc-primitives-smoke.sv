// Small, bounded behavioral smoke test for the in-tree PYC runtime modules.
// It is intentionally separate from the C++/LLVM build so it can be run with
// one Verilator job against a generated self-contained Verilog artifact.
module pyc_primitives_smoke;
  reg clk;
  reg rst;
  reg fifo_in_valid;
  reg fifo_out_ready;
  reg [7:0] fifo_in_data;
  wire fifo_in_ready;
  wire fifo_out_valid;
  wire [7:0] fifo_out_data;
  reg [1:0] req2;
  reg       cursor2;
  wire [1:0] grant2;
  reg [2:0] req3;
  reg [1:0] cursor3;
  wire [2:0] grant3;
  reg [3:0] req4;
  reg [1:0] cursor4;
  wire [3:0] grant4;
  reg [7:0] pop_in;
  wire [3:0] pop_out;

  pyc_fifo #(.WIDTH(8), .DEPTH(2)) fifo (
    .clk(clk), .rst(rst), .in_valid(fifo_in_valid),
    .in_ready(fifo_in_ready), .in_data(fifo_in_data),
    .out_valid(fifo_out_valid), .out_ready(fifo_out_ready),
    .out_data(fifo_out_data));

  pyc_rr_arbiter #(.NUM_INPUTS(2), .POINTER_WIDTH(1)) arb2 (
    .req(req2), .cursor(cursor2), .grant(grant2));
  pyc_rr_arbiter #(.NUM_INPUTS(3), .POINTER_WIDTH(2)) arb3 (
    .req(req3), .cursor(cursor3), .grant(grant3));
  pyc_rr_arbiter #(.NUM_INPUTS(4), .POINTER_WIDTH(2)) arb4 (
    .req(req4), .cursor(cursor4), .grant(grant4));
  pyc_popcount #(.IN_WIDTH(8), .OUT_WIDTH(4)) pop (
    .in(pop_in), .out(pop_out));

  task automatic expect_grant(input [3:0] expected, input [3:0] actual,
                              input integer case_id);
    begin
      if (actual !== expected) begin
        $display("FAIL arbiter case=%0d expected=%b actual=%b",
                 case_id, expected, actual);
        $finish(1);
      end
    end
  endtask

  initial begin
    clk = 1'b0;
    rst = 1'b1;
    fifo_in_valid = 1'b0;
    fifo_out_ready = 1'b0;
    fifo_in_data = 8'h00;

    req2 = 2'b11; cursor2 = 1'b0; #1;
    expect_grant(4'b0001, {2'b00, grant2}, 20);
    cursor2 = 1'b1; #1;
    expect_grant(4'b0010, {2'b00, grant2}, 21);

    req3 = 3'b111; cursor3 = 2'd0; #1;
    expect_grant(4'b0001, {1'b0, grant3}, 30);
    cursor3 = 2'd1; #1;
    expect_grant(4'b0010, {1'b0, grant3}, 31);
    cursor3 = 2'd2; #1;
    expect_grant(4'b0100, {1'b0, grant3}, 32);
    // Cursor 3 is outside the legal range for a 3-way arbiter; the primitive
    // still normalizes it deterministically instead of indexing out of range.
    cursor3 = 2'd3; #1;
    expect_grant(4'b0001, {1'b0, grant3}, 33);

    req4 = 4'b1111; cursor4 = 2'd3; #1;
    expect_grant(4'b1000, grant4, 40);
    req4 = 4'b0101; cursor4 = 2'd1; #1;
    expect_grant(4'b0100, grant4, 41);

    pop_in = 8'b10110100; #1;
    if (pop_out !== 4'd4) begin
      $display("FAIL popcount expected=4 actual=%0d", pop_out);
      $finish(1);
    end

    // One push, observe the head, then pop it.  This also checks the reset and
    // ready/valid contract of the migrated cycle-level FIFO.
    @(negedge clk);
    rst = 1'b0;
    fifo_in_data = 8'hA5;
    fifo_in_valid = 1'b1;
    @(negedge clk);
    fifo_in_valid = 1'b0;
    #1;
    if (!fifo_out_valid || fifo_out_data !== 8'hA5) begin
      $display("FAIL fifo head valid=%b data=%h", fifo_out_valid, fifo_out_data);
      $finish(1);
    end
    fifo_out_ready = 1'b1;
    @(negedge clk);
    fifo_out_ready = 1'b0;
    #1;
    if (fifo_out_valid) begin
      $display("FAIL fifo did not pop head");
      $finish(1);
    end
    $display("PYC_PRIMITIVES_SMOKE PASS");
    $finish;
  end

  always #1 clk = ~clk;
endmodule
