module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic [2:0] pyc_phase = 3'd0;
  logic pyc_root_commit_ok = 1'b0;
  wire pyc_check_complete;
  wire pyc_sampling_error;
  wire pyc_local_error;
  pyc_root dut(.*);
  integer fd, status, clock_level, reset_level, discard, row = 0;
  reg [@WIDTH_MINUS_ONE@:0] count_bits, payload_bits;
  string count_text, payload_text, stimuli;
  initial begin
    if (!$value$plusargs("stimuli=%s", stimuli)) $fatal(1, "missing stimuli");
    fd = $fopen(stimuli, "r");
    if (!fd) $fatal(1, "cannot open stimuli");
    // RESET_PREPARE stages the same false sampling history and storage reset as
    // native Reset. Inputs settle first; this delay is not a hardware cycle.
    #1;
    pyc_root_commit_ok = 1'b1;
    pyc_phase = 3'd4;
    #1;
    pyc_phase = 3'd2;
    #1;
    pyc_phase = 3'd0;
    pyc_root_commit_ok = 1'b0;
    status = $fscanf(fd, "%d %d %d %s %s\n", clock_level, reset_level,
                      discard, count_text, payload_text);
    while (status == 5) begin
      if (row >= 64) $fatal(1, "unbounded stimulus");
      @INJECT@
      pyc_7079635f636c6b = clock_level;
      pyc_7079635f727374 = reset_level;
      // PREPARE owns the Work snapshot. Settle authored pins and all
      // combinational Rule/leaf inputs before changing the phase.
      #1;
      pyc_phase = 3'd1;
      #1;
      if (pyc_check_complete !== 1'b1) begin
        pyc_phase = 3'd3;
        #1;
        $fatal(1, "incomplete sampled check snapshot at row %0d", row);
      end
      if (pyc_sampling_error !== 1'b0) begin
        pyc_phase = 3'd3;
        #1;
        $fatal(1, "physical sampling error at row %0d", row);
      end
      if (pyc_local_error !== 1'b0) begin
        pyc_phase = 3'd3;
        #1;
        $fatal(1, "source or primitive check error at row %0d", row);
      end
      $write("WORK %0d", row);
      @PROBES@
      $write("\n");
      if (discard) begin
        pyc_root_commit_ok = 1'b0;
        pyc_phase = 3'd3;
      end else begin
        pyc_root_commit_ok = 1'b1;
        pyc_phase = 3'd2;
      end
      #1;
      pyc_phase = 3'd0;
      pyc_root_commit_ok = 1'b0;
      row = row + 1;
      status = $fscanf(fd, "%d %d %d %s %s\n", clock_level, reset_level,
                        discard, count_text, payload_text);
    end
    if (!$feof(fd) || row == 0) $fatal(1, "invalid stimulus");
    $fclose(fd);
    $finish;
  end
endmodule
