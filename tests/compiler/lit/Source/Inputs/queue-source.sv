`include "queue_source_widths.svh"

module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic valid = 0;
  logic take = 0;
  logic [`Q4_INPUT_BITS-1:0] data = 0;
  wire [`Q4_OUTPUT_BITS-1:0] result;
`ifdef Q4_CHECKS
  logic [2:0] pyc_phase = 0;
  logic pyc_root_commit_ok = 0;
  wire pyc_local_error;
  logic failed = 0;

  task automatic host_reset;
    pyc_phase=4; #1;
    pyc_root_commit_ok=1; pyc_phase=2; #1;
    pyc_phase=0; pyc_root_commit_ok=0; failed=0; #1;
  endtask

  task automatic checked_row(input integer index, input logic clk, rst, push, pop,
      input logic [`Q4_INPUT_BITS-1:0] token,
      input logic [`Q4_OUTPUT_BITS-1:0] expected,
      input logic expected_failure, reset_host);
    pyc_7079635f636c6b=clk; pyc_7079635f727374=rst;
    valid=push; take=pop; data=token; #1;
    if (reset_host) begin
      host_reset();
      $display("HOST_RESET %0d",index);
    end
    if (failed) begin
      if (!expected_failure) $fatal(1,"unlabeled recovery row %0d",index);
      $display("FAILED %0d",index);
    end else begin
      pyc_phase=1; #1;
      // Freeze permission from the complete prepared error channel before Xfer.
      pyc_root_commit_ok=(pyc_local_error === 1'b0);
      if (expected_failure) begin
        if (pyc_local_error !== 1'b1 || pyc_root_commit_ok !== 1'b0)
          $fatal(1,"missing queue failure row %0d",index);
        pyc_phase=3; #1; failed=1;
        $display("FAILED %0d",index);
      end else begin
        if (pyc_root_commit_ok !== 1'b1 || result !== expected)
          $fatal(1,"queue successful sample row %0d failed",index);
        pyc_phase=2; #1;
        $display("WORK %0d",index);
      end
      pyc_phase=0; pyc_root_commit_ok=0; #1;
    end
  endtask
`endif

`ifdef Q4_MAPPING
  wire ready;
  wire available;
  wire [`Q4_INPUT_BITS-1:0] head;
  assign result = {ready, available, head};
`endif
  pyc_root dut(.*);

  initial begin
    #1;
`ifdef Q4_CHECKS
    pyc_7079635f727374=0;
    host_reset();
`else
    pyc_7079635f636c6b = 1;
    #1;
`endif
    pyc_7079635f636c6b = 0;
    pyc_7079635f727374 = 0;
    #1;
`ifdef Q6_DEAD_FAILURE
    valid = 1;
    data = 71;
    pyc_7079635f636c6b = 1; #1; // E0 birth in the unobserved queue.
    pyc_7079635f636c6b = 0; #1;
    valid = 1'bx;
    take = 1'bz;
    pyc_7079635f636c6b = 1; #1; // E1 full waiting: masked unknown controls.
    pyc_7079635f636c6b = 0; #1;
    pyc_7079635f636c6b = 1; #1; // E2 maturity must occur despite dead outputs.
    pyc_7079635f636c6b = 0; #1;
    $display("DEAD queue maturity completed");
    pyc_7079635f636c6b = 1; #1; // E3 effective unknown pop must reject.
    $fatal(1, "dead queue failed to age");
`else
    `include "queue_source_rows.svh"
    $finish;
`endif
  end
endmodule
