module tb;
  logic pyc_7079635f636c6b = 0, pyc_7079635f727374 = 0;
  logic [2:0] pyc_phase = 0;
  logic pyc_root_commit_ok = 0;
  wire pyc_check_complete, pyc_sampling_error, pyc_local_error;
  wire [7:0] out0, out1, out2, out3, out4;
  bit failure, xz;
  integer expected[0:10][0:4];
  pyc_root dut (.*);

  task automatic sample(input bit clock, input bit reset, input bit accepted);
    pyc_7079635f636c6b = clock;
    pyc_7079635f727374 = reset;
    #1; pyc_phase = 1; #1;
    if (pyc_check_complete !== 1 || pyc_sampling_error !== 0 ||
        pyc_local_error !== !accepted) $fatal(1, "check outcome");
    pyc_root_commit_ok = accepted;
    pyc_phase = accepted ? 2 : 3;
    #1; pyc_phase = 0; pyc_root_commit_ok = 0; #1;
  endtask
  task automatic check(input integer phase);
    // Inspect committed FIFO contents after each discarded transaction.
    if (failure && phase == 5 &&
        (dut.dut.`QUEUE_INSTANCE.count !== 1 ||
         dut.dut.`QUEUE_INSTANCE.storage[dut.dut.`QUEUE_INSTANCE.rd] !== 8'd22))
      $fatal(1, "failed transaction changed Queue contents");
    if (xz) begin
      if (out0 !== phase[7:0] || out1 !== (phase > 0 ? 8'd1 : 8'd0) ||
          out3 !== (phase > 1 ? 8'd1 : 8'd0) ||
          out4 !== (phase > 1 ? 8'b10xz01zx : 8'd0))
        $fatal(1, "four-state Queue payload phase %0d", phase);
    end else if (out0 !== expected[phase][0][7:0] || out1 !== expected[phase][1][7:0] ||
        out2 !== expected[phase][2][7:0] || out3 !== expected[phase][3][7:0] ||
        out4 !== expected[phase][4][7:0])
      $fatal(1, "occupancy oracle phase %0d: %0d %0d %0d %0d %0d", phase, out0,out1,out2,out3,out4);
  endtask
  initial begin
    expected[0][0]=0; expected[0][1]=0; expected[0][2]=0; expected[0][3]=0; expected[0][4]=0;
    expected[1][0]=1; expected[1][1]=1; expected[1][2]=0; expected[1][3]=0; expected[1][4]=0;
    expected[2][0]=2; expected[2][1]=1; expected[2][2]=1; expected[2][3]=0; expected[2][4]=0;
    expected[3][0]=3; expected[3][1]=2; expected[3][2]=1; expected[3][3]=0; expected[3][4]=0;
    expected[4][0]=4; expected[4][1]=2; expected[4][2]=1; expected[4][3]=0; expected[4][4]=0;
    expected[5][0]=5; expected[5][1]=2; expected[5][2]=1; expected[5][3]=1; expected[5][4]=12;
    expected[6][0]=6; expected[6][1]=2; expected[6][2]=2; expected[6][3]=1; expected[6][4]=12;
    expected[7][0]=7; expected[7][1]=2; expected[7][2]=2; expected[7][3]=2; expected[7][4]=23;
    expected[8][0]=8; expected[8][1]=2; expected[8][2]=2; expected[8][3]=2; expected[8][4]=23;
    expected[9][0]=9; expected[9][1]=3; expected[9][2]=2; expected[9][3]=2; expected[9][4]=23;
    expected[10][0]=10; expected[10][1]=3; expected[10][2]=2; expected[10][3]=3; expected[10][4]=22;
    failure = $test$plusargs("failure");
    xz = $test$plusargs("xz");
    #1; pyc_root_commit_ok = 1; pyc_phase = 4;
    #1; pyc_phase = 2;
    #1; pyc_phase = 0; pyc_root_commit_ok = 0; #1;
    for (integer pass = 0; pass < 2; pass = pass + 1) begin
      for (integer phase = 0; phase <= (xz ? 3 : failure || pass == 0 ? 5 : 10); phase = phase + 1) begin
        sample(0,0,1); check(phase);
        if (failure && phase == 5) begin
          sample(1,0,0); sample(0,0,1); check(5);
          sample(1,0,0); sample(0,0,1); check(5);
        end else if (phase < (xz ? 3 : pass == 0 ? 5 : 10)) sample(1,0,1);
      end
      sample(1,1,1); sample(0,0,1); check(0);
    end
    $finish(0);
  end
endmodule
