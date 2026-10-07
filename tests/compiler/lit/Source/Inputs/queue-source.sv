`include "queue_source_widths.svh"

module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic valid = 0;
  logic take = 0;
  logic [`Q4_INPUT_BITS-1:0] data = 0;
  wire [`Q4_OUTPUT_BITS-1:0] result;

`ifdef Q4_MAPPING
  wire ready;
  wire available;
  wire [`Q4_INPUT_BITS-1:0] head;
  assign result = {ready, available, head};
`endif
  pyc_root dut(.*);

  initial begin
    #1;
    pyc_7079635f636c6b = 1;
    #1;
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
