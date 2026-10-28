// Observe real generated Memory arrays and storage through the failed sample.
module routed_zero_commit;
  `define R pycircuit_sim.dut.dut
  `define M `R.@REMOTE@.@STORE@.@MEMORY@
  `define W `R.@BRANCH@.@WORKER@
  reg captured = 0;
  reg attributed = 0;
  reg [31:0] ram, other;
  reg [7:0] progress, held;
  reg [3:0] histories;
  function automatic [31:0] memory_bytes;
    memory_bytes = {`M.mem[3], `M.mem[2], `M.mem[1], `M.mem[0]};
  endfunction
  function automatic [31:0] other_bytes;
    other_bytes = {`R.@OTHER_MEMORY@.mem[3], `R.@OTHER_MEMORY@.mem[2],
                   `R.@OTHER_MEMORY@.mem[1], `R.@OTHER_MEMORY@.mem[0]};
  endfunction
  function automatic [3:0] history;
    history = {`M.managed.clock_current, `R.@OTHER_MEMORY@.managed.clock_current,
               `W.@PROGRESS@.managed.clock_current, `W.@HELD@.managed.clock_current};
  endfunction
  initial begin
    wait (pycircuit_sim.pyc_epoch == 2);
    ram = memory_bytes();
    other = other_bytes();
    progress = `W.@PROGRESS@.q_current;
    held = `W.@HELD@.q_current;
    histories = history();
    if (ram !== 32'h00000300 || other !== 32'h00000005 || progress !== 8'd1 ||
        held !== 8'b10xz0011 || histories !== 4'b1111)
      $fatal(1, "successful Memory proposal or transported X/Z mismatch");
    captured = 1;
  end
  always @(pycircuit_sim.pyc_check_complete or `R.@FAILED_CHECK@) begin
    #0;
    if (pycircuit_sim.pyc_phase === 3'd1 && pycircuit_sim.pyc_epoch == 2 &&
        pycircuit_sim.pyc_check_complete === 1'b1 &&
        pycircuit_sim.pyc_sampling_error === 1'b0 &&
        pycircuit_sim.pyc_local_error === 1'b1 &&
        `R.@FAILED_CHECK@_complete === 1'b1 &&
        `R.@FAILED_CHECK@_path === 1'b1 &&
        `R.@FAILED_CHECK@_reset === 1'b0 &&
        `R.@FAILED_CHECK@_condition === 1'b0 && `R.@FAILED_CHECK@ === 1'b1)
      attributed = 1;
  end
  final begin
    if (captured !== 1 || attributed !== 1 || pycircuit_sim.pyc_epoch !== 2 ||
        memory_bytes() !== ram || other_bytes() !== other ||
        `W.@PROGRESS@.q_current !== progress || `W.@HELD@.q_current !== held ||
        history() !== histories)
      $display("ROUTED_ZERO_COMMIT_FAIL");
    else begin
      $display("ROUTED_SOURCE_CHECK_OK @FAILED_MESSAGE@");
      $display("ROUTED_ZERO_COMMIT_OK");
    end
  end
endmodule
