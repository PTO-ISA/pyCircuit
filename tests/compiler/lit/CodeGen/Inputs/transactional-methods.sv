module tb;
  logic pyc_7079635f636c6b = 0, pyc_7079635f727374 = 0;
  logic [2:0] pyc_phase = 0;
  logic pyc_root_commit_ok = 0;
  wire pyc_check_complete, pyc_sampling_error, pyc_local_error;
  wire [7:0] out0, out1;
  pyc_root dut (.*);

  task automatic sample(input bit clock, input bit accepted);
    pyc_7079635f636c6b = clock;
    #1; pyc_phase = 1; #1;
    if (pyc_check_complete !== 1 || pyc_sampling_error !== 0 ||
        pyc_local_error !== !accepted) $fatal(1, "check outcome");
    pyc_root_commit_ok = accepted;
    pyc_phase = accepted ? 2 : 3;
    #1; pyc_phase = 0; pyc_root_commit_ok = 0; #1;
  endtask

  initial begin
    #1; pyc_root_commit_ok = 1; pyc_phase = 4;
    #1; pyc_phase = 2;
    #1; pyc_phase = 0; pyc_root_commit_ok = 0; #1;
    for (integer expected = 0; expected < 3; expected = expected + 1) begin
      sample(0, 1);
      if (out0 !== expected[7:0] || out1 !== expected[7:0])
        $fatal(1, "state oracle");
      $display("%0d %0d", out0, out1);
      sample(1, expected < 2);
    end
    sample(0, 1);
    if (out0 !== 8'd2 || out1 !== 8'd2)
      $fatal(1, "failed Method or independent Rule committed");
    $finish(0);
  end
endmodule
