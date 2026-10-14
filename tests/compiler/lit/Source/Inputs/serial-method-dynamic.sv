module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic [2:0] pyc_phase = 3'd0;
  logic pyc_root_commit_ok = 1'b0;
  wire pyc_check_complete, pyc_sampling_error, pyc_local_error;
  pyc_root dut(.*);

  integer fd, fields, row = 0;
  string stimuli, op, clock, reset, lf, ls, rf, rs, sf, ss;
  string outcome;

  function automatic logic wire_bit(input string token);
    logic parsed;
    begin
      if (token.len() != 1 || $sscanf(token, "%b", parsed) != 1)
        $fatal(1, "invalid one-bit token %s", token);
      wire_bit = parsed;
    end
  endfunction

  task automatic idle_phase;
    begin
      pyc_phase = 3'd0;
      pyc_root_commit_ok = 1'b0;
      #1;
    end
  endtask

  task automatic reset_design;
    begin
      // Reset is independent of the ordinary clock/reset input levels.
      #1;
      pyc_root_commit_ok = 1'b1;
      pyc_phase = 3'd4;
      #1;
      pyc_phase = 3'd2;
      #1;
      idle_phase();
    end
  endtask

  task automatic prepare;
    begin
      // Settle real state injections and all combinational pins before Work.
      #1;
      pyc_root_commit_ok = 1'b0;
      pyc_phase = 3'd1;
      #1;
    end
  endtask

  task automatic finish_sample;
    begin
      if (pyc_check_complete !== 1'b1)
        $fatal(1, "incomplete sampled snapshot at row %0d", row);
      if (pyc_sampling_error !== 1'b0)
        outcome = "physical";
      else if (pyc_local_error !== 1'b0)
        outcome = "guard";
      else
        outcome = "ok";
      if (outcome == "ok") begin
        pyc_root_commit_ok = 1'b1;
        pyc_phase = 3'd2;
      end else begin
        pyc_root_commit_ok = 1'b0;
        pyc_phase = 3'd3;
      end
      #1;
      idle_phase();
    end
  endtask

  initial begin
    if (!$value$plusargs("stimuli=%s", stimuli))
      $fatal(1, "missing stimuli");
    fd = $fopen(stimuli, "r");
    if (!fd)
      $fatal(1, "cannot open stimuli");
    reset_design();
    fields = $fscanf(fd, "%s %s %s %s %s %s %s %s %s\n",
                     op, clock, reset, lf, ls, rf, rs, sf, ss);
    while (fields == 9) begin
      if (row >= 256)
        $fatal(1, "unbounded stimulus");
      if (op != "S" && op != "R" && op != "D" && op != "F" && op != "T")
        $fatal(1, "invalid operation %s", op);
      // T exercises the native sticky failure state; it has no RTL phase.
      if (op != "T") begin
        @INJECT@
        pyc_7079635f636c6b = wire_bit(clock);
        pyc_7079635f727374 = wire_bit(reset);
        #1;
        $write("PRE %0d", row);
        @PROBES@
        $write("\n");
        if (op == "R") begin
          reset_design();
          outcome = "reset";
        end else begin
          prepare();
          if (op == "D") begin
            pyc_phase = 3'd3;
            #1;
            // A later COMMIT cannot resurrect the discarded proposals.
            pyc_root_commit_ok = 1'b1;
            pyc_phase = 3'd2;
            #1;
            idle_phase();
            outcome = "discard";
          end else begin
            if (op == "F") begin
              pyc_7079635f636c6b = 1'bx;
              pyc_7079635f727374 = 1'bx;
              #1;
            end
            finish_sample();
          end
        end
        $write("POST %0d %s", row, outcome);
        @PROBES@
        $write("\n");
      end
      row = row + 1;
      fields = $fscanf(fd, "%s %s %s %s %s %s %s %s %s\n",
                       op, clock, reset, lf, ls, rf, rs, sf, ss);
    end
    if (!$feof(fd) || row == 0)
      $fatal(1, "invalid stimulus");
    $fclose(fd);
    $finish;
  end
endmodule
