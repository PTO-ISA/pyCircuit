module fault_probe;
  logic clk=0, rst=0;
  logic [2:0] phase=0;
  logic permit=0;
  wire complete, sampling_error, error;
  string before_state;
  integer step_index;
  pyc_root dut(.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    .pyc_phase(phase), .pyc_root_commit_ok(permit),
    .pyc_check_complete(complete), .pyc_sampling_error(sampling_error),
    .pyc_local_error(error));
  function automatic string snapshot;
    return @SNAPSHOT@;
  endfunction
  task automatic step(input logic clock);
    clk=clock; phase=1; #1;
    if (error) $fatal(1,"fault inspected before its effective view");
    case (step_index)
      @EXPECTED@
      default: $fatal(1,"unexpected prefix row");
    endcase
    step_index=step_index+1;
    permit=1; phase=2; #1; phase=0; permit=0; #1;
  endtask
  initial begin
    #1;
    for (integer retry=0; retry<2; retry=retry+1) begin
      clk=0; rst=0; permit=1; phase=4; #1;
      phase=2; #1; phase=0; permit=0; #1;
      step_index=0;
      for (integer epoch=0; epoch<@EDGE@; epoch=epoch+1) begin
        step(0); step(1);
      end
      if (@FAIL@) begin
        before_state=snapshot();
        clk=(@EDGE@ == 0); phase=1; #1;
        if (!error) $fatal(1,"effective invalid head did not fail");
        if (@SIBLING_VALUE_RTL@.en !== 1'b1 ||
            @SIBLING_VALUE_RTL@.d !== @EDGE@ + 1 ||
            @SIBLING_VALUE_RTL@.managed.clock_pending !== (@EDGE@ == 0) ||
            !@SIBLING_VALUE_RTL@.managed.pending_valid)
          $fatal(1,"sibling did not prepare its proposal and clock history");
        if (@EDGE@ == 0) begin
          if (@SIBLING_VALUE_RTL@.managed.q_pending !== 1 ||
              !@SIBLING_QUEUE_RTL@.latency_delayed.managed.pending_valid ||
              !@SIBLING_QUEUE_RTL@.candidate.write_token ||
              @SIBLING_QUEUE_RTL@.candidate.token !== 0 ||
              @SIBLING_QUEUE_RTL@.latency_delayed.timing_candidate.tick !== 1 ||
              @SIBLING_QUEUE_RTL@.latency_delayed.managed.clock_pending !== 1'b1)
            $fatal(1,"rising Work did not prepare nontrivial counter/token/tick/clock updates");
        end
        if (snapshot() != before_state) $fatal(1,"Work committed state");
        phase=2; #1;
        if (snapshot() != before_state) $fatal(1,"denied Xfer changed state or clock");
        phase=3; #1; phase=0; #1;
        if (snapshot() != before_state) $fatal(1,"Discard changed state");
        phase=1; #1;
        if (!error) $fatal(1,"retry lost the invalid head");
        phase=3; #1; phase=0; #1;
        if (snapshot() != before_state) $fatal(1,"retry changed state");
      end
    end
    $display("QUEUE_FAULT_ATOMIC_OK"); $finish;
  end
endmodule
