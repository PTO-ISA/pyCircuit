module resource_demand_probe;
  logic clk=0,rst=0,permit=0;
  logic [2:0] phase=0;
  wire complete,sampling_error,error;
  pyc_root dut(.pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),
    .pyc_phase(phase),.pyc_root_commit_ok(permit),.pyc_check_complete(complete),
    .pyc_sampling_error(sampling_error),.pyc_local_error(error));
  `define R dut.dut
  `define L `R.pyc_instance_left
  `define Q `R.pyc_instance_right
  `define O `R.pyc_instance_pyc_6f7574707574
  integer cases=0;
  function automatic logic four(input integer s);
    case(s) 0:four=0;1:four=1;2:four=1'bx;3:four=1'bz;endcase
  endfunction
  function automatic [255:0] snapshot;
    snapshot={`L.count,`Q.count,`O.count,`L.rd,`L.wr,`Q.rd,`Q.wr,`O.rd,`O.wr,
      `L.storage[0],`Q.storage[0],`O.storage[0],
      `L.latency_one.managed.clock_current,`Q.latency_one.managed.clock_current,`O.latency_one.managed.clock_current,
      `R.pyc_instance_count.q_current,`R.pyc_instance_count.managed.clock_current,
      `R.pyc_instance_selector.q_current,`R.pyc_instance_selector.managed.clock_current,
      `R.pyc_instance_early.q_current,`R.pyc_instance_early.managed.clock_current,
      `R.pyc_instance_later.q_current,`R.pyc_instance_later.managed.clock_current,
      `R.pyc_instance_send_left.q_current,`R.pyc_instance_send_left.managed.clock_current,
      `R.pyc_instance_send_right.q_current,`R.pyc_instance_send_right.managed.clock_current,
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
  task automatic start(input integer timing,s,l,r,o);
    clk=0;rst=0;permit=1;phase=4;#1;phase=2;#1;phase=0;permit=0;#1;
    if(timing==2||timing==3)begin clk=1;rst=1;prepare();commit();end
    `L.count=l;`L.rd=0;`L.wr=0;`L.storage[0]=11;
    `Q.count=r;`Q.rd=0;`Q.wr=0;`Q.storage[0]=11;
    `O.count=o;`O.rd=0;`O.wr=0;`O.storage[0]=11;
    `R.pyc_instance_selector.q_current=four(s);
    `R.pyc_instance_early.q_current=1;`R.pyc_instance_later.q_current=1;
    `R.pyc_instance_send_left.q_current=0;`R.pyc_instance_send_right.q_current=0;
    clk=timing==1||timing==2||timing==4;rst=timing==4;
  endtask
  task automatic sample(input logic fails);
    logic [255:0] old;
    old=snapshot();prepare();
    if(complete!==1||sampling_error!==0||error!==fails)
      $fatal(1,"resource diagnostic mismatch expected=%b got=%b/%b/%b",fails,error,sampling_error,complete);
    if(fails)begin discard();if(snapshot()!==old)$fatal(1,"failure committed queue/state/history");end
    else commit();
    cases=cases+1;
  endtask
  task automatic counts(input integer l,r,o,n);
    if(`L.count!==l||`Q.count!==r||`O.count!==o||`R.pyc_instance_count.q_current!==n)
      $fatal(1,"resource count oracle expected=%0d/%0d/%0d/%0d got=%0d/%0d/%0d/%0d",l,r,o,n,`L.count,`Q.count,`O.count,`R.pyc_instance_count.q_current);
  endtask
  initial begin
    logic fails,active;
    logic [255:0] old;
    #1;
    for(integer t=0;t<5;t=t+1)for(integer s=0;s<4;s=s+1)
      for(integer l=0;l<2;l=l+1)for(integer r=0;r<2;r=r+1)for(integer o=0;o<2;o=o+1)begin
        start(t,s,l,r,o);fails=t==1&&s>=2&&!o&&(l||r);sample(fails);
        if(!fails)begin
          active=t==1&&s<2&&!o&&(s?l:r);
          if(t==4)counts(0,0,0,0);
          else counts(l-(active&&s==1),r-(active&&s==0),o+active,t==1);
        end
      end
    for(integer u=2;u<4;u=u+1)begin
      for(integer blocked=0;blocked<2;blocked=blocked+1)begin
        start(1,u,0,0,blocked);`R.pyc_instance_send_left.q_current=1;`R.pyc_instance_send_right.q_current=1;
        sample(0);counts(1,1,blocked,1);
      end
      start(1,1,0,0,0);`R.pyc_instance_early.q_current=four(u);`R.pyc_instance_later.q_current=0;sample(1);
      start(1,1,1,1,0);`R.pyc_instance_early.q_current=0;`R.pyc_instance_later.q_current=four(u);sample(0);counts(1,1,0,1);
    end
    start(1,1,1,1,0);old=snapshot();prepare();discard();
    if(snapshot()!==old)$fatal(1,"Discard committed queue/state/history");
    sample(0);counts(0,1,1,1);
    $display("RESOURCE_DEMAND_RTL_OK %0d",cases);$finish;
  end
endmodule
