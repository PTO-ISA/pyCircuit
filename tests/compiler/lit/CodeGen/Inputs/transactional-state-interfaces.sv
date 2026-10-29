module tb;
  logic pyc_7079635f636c6b = 0, pyc_7079635f727374 = 0;
  logic [2:0] pyc_phase = 0;
  logic pyc_root_commit_ok = 0;
  wire pyc_check_complete, pyc_sampling_error, pyc_local_error;
  wire [7:0] out0, out1, out2, out3;
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
  task automatic check(input integer incremented, input integer first,
                       input integer second);
    sample(0, 0);
    if (out0 !== incremented[7:0] || out1 !== first[7:0] ||
        out2 !== second[7:0] || out3 !== first[7:0])
      $fatal(1, "State binding/reset oracle");
  endtask
  initial begin
    #1; pyc_root_commit_ok = 1; pyc_phase = 4;
    #1; pyc_phase = 2;
    #1; pyc_phase = 0; pyc_root_commit_ok = 0; #1;
    check(0, 20, 30);
    sample(1, 0); check(4, 0, 10);
    sample(1, 0); check(4, 5, 11);
    sample(1, 0); check(4, 5, 11);
    sample(1, 1); check(0, 20, 30);
    sample(1, 0); check(4, 0, 10);
    sample(1, 0); check(4, 5, 11);
    $finish(0);
  end
endmodule
