module scf_else_failure;
  logic clk=0,rst=0,permit=0;
  logic [2:0] phase=0;
  wire complete,sampling_error,error;
  pyc_root dut(.pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),
    .pyc_phase(phase),.pyc_root_commit_ok(permit),.pyc_check_complete(complete),
    .pyc_sampling_error(sampling_error),.pyc_local_error(error));
  `define R dut.dut
  integer cases=0;
  function automatic logic four(input integer s);
    case(s) 0:four=0;1:four=1;2:four=1'bx;3:four=1'bz;endcase
  endfunction
  function automatic [31:0] snapshot;
    snapshot={`R.pyc_instance_total.q_current,`R.pyc_instance_total.managed.clock_current,
      `R.pyc_instance_peer.q_current,`R.pyc_instance_peer.managed.clock_current,
      `R.pyc_instance_stop.q_current,`R.pyc_instance_stop.managed.clock_current,
      `R.pyc_instance_allow.q_current,`R.pyc_instance_allow.managed.clock_current};
  endfunction
  initial begin
    logic [31:0] old;
    logic fails;
    #1;
    for(integer stop=0;stop<4;stop=stop+1)
      for(integer allow=0;allow<2;allow=allow+1)begin
        clk=0;permit=1;phase=4;#1;phase=2;#1;phase=0;permit=0;#1;
        `R.pyc_instance_stop.q_current=four(stop);
        `R.pyc_instance_allow.q_current=four(allow);
        clk=1;old=snapshot();#1;phase=1;#1;
        fails=stop>=2||allow==0;
        if(complete!==1||sampling_error!==0||error!==fails)
          $fatal(1,"loop else diagnostic mismatch stop=%0d allow=%0d",stop,allow);
        if(fails)begin
          phase=2;#1;phase=3;#1;phase=0;#1;
          if(snapshot()!==old)$fatal(1,"failed loop else committed state/clock history");
        end else begin
          permit=1;phase=2;#1;phase=0;permit=0;#1;
          if(`R.pyc_instance_total.q_current!==(stop?8'd6:8'd28)||
             `R.pyc_instance_peer.q_current!==8'd8)
            $fatal(1,"loop else success value mismatch");
        end
        cases=cases+1;
      end
    $display("LOOP_ELSE_RTL_OK %0d",cases);$finish;
  end
endmodule
