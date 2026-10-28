// Extra monitor beside the compiler-generated System runner. It observes the
// real failed generated sample, without supplying a replacement scheduler.
module routed_zero_commit;
  `define R pycircuit_sim.dut.dut
  reg captured = 0;
  reg attributed = 0;
  reg [7:0] count, progress, sibling;
  reg [2:0] histories;
  function automatic [2:0] history;
    history = {`R.@PEER@.@COUNT@.managed.clock_current,
               `R.@WORKER@.@PROGRESS@.managed.clock_current,
               `R.@SIBLING@.managed.clock_current};
  endfunction
  // Epoch two starts after low and rising successful samples. The following
  // low Work fails its assertion and must preserve all three storage owners.
  initial begin
    wait (pycircuit_sim.pyc_epoch == 2);
    count = `R.@PEER@.@COUNT@.q_current;
    progress = `R.@WORKER@.@PROGRESS@.q_current;
    sibling = `R.@SIBLING@.q_current;
    histories = history();
    if (count !== 8'd16 || progress !== 8'd1 || sibling !== 8'd34 || histories !== 3'b111)
      $fatal(1, "successful routed proposal mismatch");
    captured = 1;
  end
  // Check snapshots are cleared by DISCARD. Observe the settled, completed
  // PREPARE instead, at the exact endpoint authenticated from the native source
  // failure descriptor and the corresponding generated RTL assertion formula.
  always @(pycircuit_sim.pyc_check_complete or `R.@FAILED_CHECK@) begin
    #0;
    if (pycircuit_sim.pyc_phase === 3'd1 && pycircuit_sim.pyc_epoch == 2 &&
        pycircuit_sim.pyc_check_complete === 1'b1 &&
        pycircuit_sim.pyc_sampling_error === 1'b0 &&
        pycircuit_sim.pyc_local_error === 1'b1 &&
        `R.@FAILED_CHECK@_complete === 1'b1 &&
        `R.@FAILED_CHECK@_path === 1'b1 &&
        `R.@FAILED_CHECK@_reset === 1'b0 &&
        `R.@FAILED_CHECK@_condition === 1'b0 &&
        `R.@FAILED_CHECK@ === 1'b1)
      attributed = 1;
  end
  final begin
    if (captured !== 1 || attributed !== 1 || pycircuit_sim.pyc_epoch !== 2 ||
        `R.@PEER@.@COUNT@.q_current !== count ||
        `R.@WORKER@.@PROGRESS@.q_current !== progress ||
        `R.@SIBLING@.q_current !== sibling || history() !== histories)
      $display("ROUTED_ZERO_COMMIT_FAIL");
    else begin
      $display("ROUTED_SOURCE_CHECK_OK @FAILED_MESSAGE@");
      $display("ROUTED_ZERO_COMMIT_OK");
    end
  end
endmodule
