module queue_external_pop_probe;
  logic clk=0,rst=0,permit=0;
  logic [2:0] phase=0;
  wire complete,sampling_error,error;
  pyc_root dut(.pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),
    .pyc_phase(phase),.pyc_root_commit_ok(permit),.pyc_check_complete(complete),
    .pyc_sampling_error(sampling_error),.pyc_local_error(error));
  `define R dut.dut
  `define Q `R.pyc_instance_tokens
  integer cases=0;
  function automatic logic four(input integer s);
    case(s) 0:four=0;1:four=1;2:four=1'bx;3:four=1'bz;endcase
  endfunction
  function automatic [255:0] snapshot;
    snapshot={`Q.count,`Q.rd,`Q.wr,`Q.storage[0],`Q.latency_one.managed.clock_current,
      `R.pyc_instance_count.q_current,`R.pyc_instance_count.managed.clock_current,
      `R.pyc_instance_allow.q_current,`R.pyc_instance_allow.managed.clock_current,
      `R.pyc_instance_take.q_current,`R.pyc_instance_take.managed.clock_current};
  endfunction
  task automatic prepare;
    #1;phase=1;#1;
  endtask
  task automatic commit;
    permit=1;phase=2;#1;phase=0;permit=0;#1;
  endtask
  task automatic discard;
    permit=0;phase=2;#1;phase=3;#1;phase=0;#1;
  endtask
  task automatic start(input integer timing,allow,take,full);
    clk=0;rst=0;permit=1;phase=4;#1;phase=2;#1;phase=0;permit=0;#1;
    if(timing==2||timing==3)begin clk=1;rst=1;prepare();commit();end
    `Q.count=full;`Q.rd=0;`Q.wr=0;`Q.storage[0]=11;
    `R.pyc_instance_allow.q_current=four(allow);
    `R.pyc_instance_take.q_current=four(take);
    clk=timing==1||timing==2||timing==4;rst=timing==4;
  endtask
  task automatic sample(input logic fails);
    logic [255:0] old;
    old=snapshot();prepare();
    if(complete!==1||sampling_error!==0||error!==fails)
      $fatal(1,"external-pop diagnostic mismatch expected=%b got=%b/%b/%b",fails,error,sampling_error,complete);
    if(fails)begin
      discard();if(snapshot()!==old)$fatal(1,"failed external pop committed queue/state/history");
    end else commit();
    cases=cases+1;
  endtask
  initial begin
    logic fails,pop;
    integer expected_count,expected_value;
    logic [255:0] old;
    #1;
    for(integer timing=0;timing<5;timing=timing+1)
      for(integer allow=0;allow<4;allow=allow+1)
        for(integer take=0;take<4;take=take+1)
          for(integer full=0;full<2;full=full+1)begin
            start(timing,allow,take,full);
            fails=timing==1&&(allow>=2||(allow==1&&take>=2));sample(fails);
            if(!fails)begin
              pop=allow==1&&take==1&&full;
              expected_count=timing==4?0:timing==1?1:full;
              expected_value=timing==1&&(!full||pop)?23:11;
              if(`Q.count!==expected_count||`Q.storage[0]!==8'(expected_value)||
                 `R.pyc_instance_count.q_current!==8'(timing==1)||
                 `Q.latency_one.managed.clock_current!==(timing==1||timing==2||timing==4))
                $fatal(1,"external-pop old-Q replacement oracle t=%0d allow=%0d take=%0d full=%0d",timing,allow,take,full);
            end
          end
    start(1,1,1,1);old=snapshot();prepare();discard();
    if(snapshot()!==old)$fatal(1,"Discard committed external pop replacement");
    sample(0);
    if(`Q.count!==1||`Q.storage[0]!==23||`R.pyc_instance_count.q_current!==1)
      $fatal(1,"Discard retry lost full replacement or independent state update");
    $display("EXTERNAL_POP_RTL_OK %0d",cases);$finish;
  end
endmodule
