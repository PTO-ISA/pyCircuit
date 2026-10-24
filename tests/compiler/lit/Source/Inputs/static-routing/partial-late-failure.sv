// Observe the compiler-generated System's real failed sample and all ten lanes.
module routed_zero_commit;
  `define R pycircuit_sim.dut.dut
  reg captured = 0;
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
  final begin
    if (captured !== 1 || pycircuit_sim.pyc_epoch !== 2 ||
        {@STATE_VALUES@} !== values || {@STATE_HISTORIES@} !== histories)
      $display("ROUTED_ZERO_COMMIT_FAIL");
    else
      $display("ROUTED_ZERO_COMMIT_OK");
  end
endmodule
