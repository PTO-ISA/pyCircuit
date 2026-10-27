module tb;
  logic pyc_7079635f636c6b = 0, pyc_7079635f727374 = 0;
  logic [2:0] pyc_phase = 0;
  logic pyc_root_commit_ok = 0;
  wire pyc_check_complete, pyc_sampling_error, pyc_local_error;
  wire [2:0] a, b;
`ifdef FAILURE
  pyc_root dut (.*);
`else
  wire [2:0] counter, shared, alias_a;
  wire [3:0] payload, copy;
  pyc_root dut (.*);
`endif
  task automatic sample(input logic clk, input logic rst, input bit accepted);
    pyc_7079635f636c6b = clk;
    pyc_7079635f727374 = rst;
    #1; pyc_phase = 1; #1;
    if (pyc_check_complete !== 1 || pyc_sampling_error !== 0 ||
        pyc_local_error !== !accepted) $fatal(1, "check outcome");
    pyc_root_commit_ok = accepted;
    pyc_phase = accepted ? 2 : 3;
    #1; pyc_phase = 0; pyc_root_commit_ok = 0; #1;
  endtask
  task automatic cycle(input logic rst, input bit accepted);
    sample(0, rst, 1);
    sample(1, rst, accepted);
  endtask
`ifdef FAILURE
  task automatic check(input integer ea, input integer eb);
    if (a !== ea[2:0] || b !== eb[2:0]) $fatal(1, "state oracle");
    $display("%0d %0d", a, b);
  endtask
`else
  task automatic check(input integer ea, input integer eb, input integer ec,
                       input integer es, input bit copied);
    if (a !== ea[2:0] || b !== eb[2:0] || counter !== ec[2:0] ||
        shared !== es[2:0] || alias_a !== ea[2:0] || payload !== 4'b10xz ||
        copy !== (copied ? 4'b10xz : 4'b0000)) $fatal(1, "state oracle");
    $display("%0d %0d %0d %0d %0d", a, b, counter, shared, copied);
  endtask
`endif
  initial begin
    #1; pyc_root_commit_ok = 1; pyc_phase = 4;
    #1; pyc_phase = 2;
    #1; pyc_phase = 0; pyc_root_commit_ok = 0; #1;
`ifdef FAILURE
    sample(0, 0, 1); check(2, 5);
    cycle(0, 1); check(3, 6);
    cycle(0, 0); check(3, 6);
    sample(1, 0, 0); check(3, 6);
    sample(1, 1, 1); check(2, 5);
    cycle(0, 1); check(3, 6);
`else
    sample(0, 0, 1); check(6, 3, 5, 3, 0);
    cycle(0, 1); check(7, 6, 6, 1, 1);
    cycle(0, 1); check(7, 6, 7, 2, 1);
    cycle(0, 1); check(0, 7, 0, 1, 1);
    cycle(0, 1); check(0, 7, 1, 2, 1);
    cycle(1, 1); check(6, 3, 5, 3, 0);
    cycle(0, 1); check(7, 6, 6, 1, 1);
`endif
    $finish(0);
  end
endmodule
