`timescale 1ns/1ps

module tb_pyc_runtime_div;
  localparam integer WIDTH = 64;

  reg                  clk;
  reg                  reset;
  reg                  request_valid;
  wire                 request_ready;
  reg  [WIDTH-1:0]     dividend;
  reg  [WIDTH-1:0]     divisor;
  reg                  is_signed;
  reg                  word_mode;
  wire                 response_valid;
  reg                  response_ready;
  wire [WIDTH-1:0]     quotient;
  wire [WIDTH-1:0]     remainder;
  wire                 divide_by_zero;

  pyc_runtime_div #(
    .WIDTH(64),
    .WORD_WIDTH(32),
    .BITS_PER_ITER(1)
  ) dut (
    .clk(clk),
    .reset(reset),
    .request_valid(request_valid),
    .request_ready(request_ready),
    .dividend(dividend),
    .divisor(divisor),
    .is_signed(is_signed),
    .word_mode(word_mode),
    .response_valid(response_valid),
    .response_ready(response_ready),
    .quotient(quotient),
    .remainder(remainder),
    .divide_by_zero(divide_by_zero)
  );

  always #5 clk = ~clk;

  task automatic run_case;
    input [255:0] name;
    input [63:0] lhs;
    input [63:0] rhs;
    input        signed_mode;
    input        word;
    input [63:0] expected_q;
    input [63:0] expected_r;
    input        expected_divzero;
    begin
      while (!request_ready) @(posedge clk);
      dividend      <= lhs;
      divisor       <= rhs;
      is_signed     <= signed_mode;
      word_mode     <= word;
      request_valid <= 1'b1;
      @(posedge clk);
      request_valid <= 1'b0;

      while (!response_valid) @(posedge clk);
      if (quotient !== expected_q ||
          remainder !== expected_r ||
          divide_by_zero !== expected_divzero) begin
        $display("FAIL %s", name);
        $display("  got q=%h r=%h div0=%b", quotient, remainder,
                 divide_by_zero);
        $display("  exp q=%h r=%h div0=%b", expected_q, expected_r,
                 expected_divzero);
        $fatal(1);
      end
      $display("PASS %s q=%h r=%h", name, quotient, remainder);
      @(posedge clk);
    end
  endtask

  initial begin
    clk = 1'b0;
    reset = 1'b1;
    request_valid = 1'b0;
    dividend = 64'b0;
    divisor = 64'b0;
    is_signed = 1'b0;
    word_mode = 1'b0;
    response_ready = 1'b1;

    repeat (3) @(posedge clk);
    reset <= 1'b0;
    @(posedge clk);

    // XLEN signed/unsigned basics.
    run_case("DIV 10/3", 64'd10, 64'd3, 1'b1, 1'b0,
             64'd3, 64'd1, 1'b0);
    run_case("DIVU 15/4", 64'd15, 64'd4, 1'b0, 1'b0,
             64'd3, 64'd3, 1'b0);

    // PTO total divide-by-zero semantics: quotient=0, remainder=dividend.
    run_case("DIV by zero", 64'hffff_ffff_ffff_fff6, 64'd0,
             1'b1, 1'b0,
             64'd0, 64'hffff_ffff_ffff_fff6, 1'b1);

    // Signed minimum / -1: minimum quotient, zero remainder.
    run_case("DIV signed overflow", 64'h8000_0000_0000_0000,
             64'hffff_ffff_ffff_ffff,
             1'b1, 1'b0,
             64'h8000_0000_0000_0000, 64'd0, 1'b0);

    // Word signed division: -10 / 3 = -3 remainder -1, both sign-extended.
    run_case("DIVW -10/3", 64'h0000_0000_ffff_fff6, 64'd3,
             1'b1, 1'b1,
             64'hffff_ffff_ffff_fffd,
             64'hffff_ffff_ffff_ffff,
             1'b0);

    // Unsigned W result is still sign-extended from low 32 bits by PTO.
    run_case("DIVUW ffffffff/1", 64'hffff_ffff_ffff_ffff, 64'd1,
             1'b0, 1'b1,
             64'hffff_ffff_ffff_ffff,
             64'd0,
             1'b0);

    // Zero divisor in unsigned W form: low-32 dividend then sign-extend result.
    run_case("REMUW zero divisor", 64'h1234_5678_ffff_ffff, 64'd0,
             1'b0, 1'b1,
             64'd0,
             64'hffff_ffff_ffff_ffff,
             1'b1);

    // Signed W minimum / -1.
    run_case("DIVW signed overflow", 64'h0000_0000_8000_0000,
             64'h0000_0000_ffff_ffff,
             1'b1, 1'b1,
             64'hffff_ffff_8000_0000,
             64'd0,
             1'b0);

    $display("ALL PTO DIV/REM DIRECTED CASES PASSED");
    $finish;
  end
endmodule
