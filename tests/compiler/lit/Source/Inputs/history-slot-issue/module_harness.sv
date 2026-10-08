module tb;
  logic clk = 0, rst = 0;
  logic [2:0] phase = 0;
  logic permission = 0;
  wire error;
  pyc_root dut(.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    .pyc_phase(phase), .pyc_root_commit_ok(permission), .pyc_local_error(error));
  @CONFIG@
  logic [@BITS_MINUS_ONE@:0] before_image, work_image;
  string low_controls;
  integer epoch;
  task transfer;
    phase = 1; #1;
    permission = error === 1'b0;
    if (!permission) $fatal(1, "source check rejected historical execution");
    phase = 2; #1;
    phase = 0; #1;
  endtask
  initial begin
    phase = 4; permission = 1; #1;
    phase = 2; #1;
    phase = 0; #1;
    for (epoch = 0; epoch < @EPOCHS@; epoch = epoch + 1) begin
      before_image = snapshot();
      clk = 0; transfer();
      work_image = snapshot();
      low_controls = controls();
      clk = 1;
      phase = 1; #1;
      permission = error === 1'b0;
      if (!permission) $fatal(1, "source check rejected historical execution");
      if (low_controls != controls()) $fatal(1, "old-Q physical pair changed");
      phase = 2; #1;
      phase = 0; #1;
      $display("ROW %0d %b %b %b %s", epoch, before_image, work_image, snapshot(), low_controls);
    end
    $finish;
  end
endmodule
