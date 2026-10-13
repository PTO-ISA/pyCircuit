module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  pyc_root dut(.*);
  integer fd, status, clock_level, reset_level, discard, row = 0;
  reg [@WIDTH_MINUS_ONE@:0] count_bits, payload_bits;
  string count_text, payload_text, stimuli;
  initial begin
    if (!$value$plusargs("stimuli=%s", stimuli)) $fatal(1, "missing stimuli");
    fd = $fopen(stimuli, "r");
    if (!fd) $fatal(1, "cannot open stimuli");
    // Real reset edge establishes parity with the native Reset/Xfer lifecycle.
    #1; pyc_7079635f636c6b = 1; #1;
    pyc_7079635f636c6b = 0; #1;
    status = $fscanf(fd, "%d %d %d %s %s\n", clock_level, reset_level,
                      discard, count_text, payload_text);
    while (status == 5) begin
      if (row >= 64) $fatal(1, "unbounded stimulus");
      @INJECT@
      pyc_7079635f727374 = reset_level;
      #1;
      $write("WORK %0d", row);
      @PROBES@
      $write("\n");
      // A discarded native epoch preserves its sampled clock history. The
      // autonomous RTL adapter models this by withholding that physical edge.
      if (!discard) pyc_7079635f636c6b = clock_level;
      #1;
      row = row + 1;
      status = $fscanf(fd, "%d %d %d %s %s\n", clock_level, reset_level,
                        discard, count_text, payload_text);
    end
    if (!$feof(fd) || row == 0) $fatal(1, "invalid stimulus");
    $fclose(fd);
    $finish;
  end
endmodule
