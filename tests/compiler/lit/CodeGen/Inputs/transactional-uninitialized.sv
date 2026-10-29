module tb;
  logic pyc_7079635f636c6b = 0, pyc_7079635f727374 = 0;
  logic [2:0] pyc_phase = 0;
  logic pyc_root_commit_ok = 0;
  wire pyc_check_complete, pyc_sampling_error, pyc_local_error;
  wire [7:0] out0;
  wire [1:0] out1;
  wire [81:0] out2;
  pyc_root dut (.*);
  task automatic sample(input logic clk, input logic rst);
    pyc_7079635f636c6b = clk;
    pyc_7079635f727374 = rst;
    #1; pyc_phase = 1; #1;
    if (pyc_check_complete !== 1 || pyc_sampling_error !== 0 ||
        pyc_local_error !== 0) $fatal(1, "check outcome");
    pyc_root_commit_ok = 1;
    pyc_phase = 2;
    #1; pyc_phase = 0; pyc_root_commit_ok = 0; #1;
  endtask
  task automatic check(input logic initialized);
    sample(0, 0);
    if (initialized ? (out0 !== 8'd37 || out1 !== 2'd2 || out2 !== {2'd2, 80'd37})
                    : (out0 !== 8'hxx || out1 !== 2'bxx || out2 !== {82{1'bx}}))
      $fatal(1, "uninitialized State/write/reset oracle");
  endtask
  initial begin
    #1; pyc_root_commit_ok = 1; pyc_phase = 4;
    #1; pyc_phase = 2;
    #1; pyc_phase = 0; pyc_root_commit_ok = 0; #1;
    check(0);
    sample(1, 0); check(1);
    sample(1, 0); check(1);
    sample(1, 1); check(0);
    sample(1, 0); check(1);
    $finish(0);
  end
endmodule
