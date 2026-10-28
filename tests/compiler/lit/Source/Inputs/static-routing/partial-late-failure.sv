// Observe the compiler-generated System's real failed sample and all ten lanes.
module routed_zero_commit;
  `define R pycircuit_sim.dut.dut
  reg captured = 0;
  reg attributed = 0;
  reg [88:0] values;
  reg [9:0] histories;
  initial begin
    wait (pycircuit_sim.pyc_epoch == 2);
    values = {@STATE_VALUES@};
    histories = {@STATE_HISTORIES@};
    if (histories !== 10'b1111111111)
      $fatal(1, "partial writer successful histories");
    captured = 1;
  end
  // Authenticate the settled failed source check before DISCARD clears it.
  // This endpoint is matched to the native failure descriptor and final ACIR.
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
        {@STATE_VALUES@} !== values || {@STATE_HISTORIES@} !== histories)
      $display("ROUTED_ZERO_COMMIT_FAIL");
    else begin
      $display("ROUTED_SOURCE_CHECK_OK @FAILED_MESSAGE@");
      $display("ROUTED_ZERO_COMMIT_OK");
    end
  end
endmodule
