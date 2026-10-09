module tb;
  logic pyc_7079635f636c6b=0, pyc_7079635f727374=1;
  logic in_valid=0, out0_ready=0, out1_ready=0;
  logic [7:0] in_data=0;
  wire [18:0] result;
  pyc_root dut(.*);
  // Independent compact FIFO scoreboard. Invalid DUT data may retain old bits;
  // only those invalid payloads are normalized in the cross-backend trace.
  logic [7:0] queue_data[0:3];
  bit last_clock=0;
  integer count=0, sampled=0, accepted=0, retired=0, dropped=0, peak=0;
  integer stalled=0, replacements=0, dual=0, gated=0, reset_full=0;

  task row(input bit clock_level,reset_level,valid_level,
           input logic[7:0] data_level,input bit ready0_level,ready1_level);
    bit valid0,valid1,pop0,pop1,ready;
    logic[7:0] actual0,actual1;
    valid0=count>0; valid1=count>1;
    pop0=valid0&&ready0_level; pop1=valid1&&ready1_level&&pop0;
    ready=count<4||pop0;
    pyc_7079635f727374=reset_level; in_valid=valid_level; in_data=data_level;
    out0_ready=ready0_level; out1_ready=ready1_level; #1;
    actual0=result[16:9]; actual1=result[7:0];
    if(result[18]!==ready||result[17]!==valid0||result[8]!==valid1)
      $fatal(1,"picker row %0d ready/valid",sampled);
    if(valid0&&actual0!==queue_data[0])$fatal(1,"picker first old head %0d",sampled);
    if(valid1&&actual1!==queue_data[1])$fatal(1,"picker second old head %0d",sampled);
    $display("WORK %0d %0d %0d %0d %0d %0d",sampled,result[18],result[17],
             valid0?actual0:8'd0,result[8],valid1?actual1:8'd0);
    if(clock_level&&!last_clock)begin
      if(reset_level)begin
        reset_full=reset_full+(count==4); dropped=dropped+count; count=0;
      end else begin
        stalled=stalled+(valid_level&&!ready);
        replacements=replacements+(count==4&&pop0&&valid_level);
        dual=dual+pop1; gated=gated+(valid1&&ready1_level&&!ready0_level);
        if(pop0)begin
          for(integer i=0;i<3;i=i+1)queue_data[i]=queue_data[i+1];
          count=count-1; retired=retired+1;
        end
        if(pop1)begin
          for(integer i=0;i<3;i=i+1)queue_data[i]=queue_data[i+1];
          count=count-1; retired=retired+1;
        end
        if(valid_level&&ready)begin queue_data[count]=data_level; count=count+1; accepted=accepted+1; end
      end
      if(count>peak)peak=count;
      if(count>4)$fatal(1,"picker capacity");
    end
    last_clock=clock_level; sampled=sampled+1;
    pyc_7079635f636c6b=clock_level; #1;
  endtask
  task edge_row(input logic[7:0] d,input bit v=1,a=0,b=0,r=0);
    row(1,r,v,d,a,b); row(0,r,v,d,a,b);
  endtask
  initial begin
    // Establish host Reset's empty Q before comparable Work samples.
    #1; pyc_7079635f636c6b=1; #1; pyc_7079635f636c6b=0; #1;
    row(0,1,0,0,0,0); edge_row(0,0,0,0,1);
    edge_row(0,1,0,1); edge_row(255,1,0,1);
    edge_row(254,1,0,1); edge_row(128,1,0,1);
    edge_row(11); edge_row(12); edge_row(13,1,0,1);
    edge_row(14,1,1,0); edge_row(15,1,1,1);
    row(1,0,1,16,1,1); row(1,1,1,17,1,1);
    row(0,0,1,18,0,1); row(0,0,0,19,1,1);
    edge_row(20); edge_row(21); edge_row(0,1,1,1,1);
    for(integer i=0;i<4;i=i+1)edge_row(8'(250+i));
    for(integer i=0;i<6;i=i+1)edge_row(8'(i),1,1,0);
    for(integer i=0;i<48;i=i+1)
      edge_row(8'((i*73+29)&255),i%5!=1,i%4!=0,i%3!=1);
    for(integer i=0;i<4;i=i+1)edge_row(0,0,1,1);
    for(integer i=0;i<4;i=i+1)edge_row(8'(96+i));
    edge_row(255,1,1,1,1); edge_row(255); edge_row(0);
    for(integer i=0;i<4;i=i+1)edge_row(0,0,1,1);
    if(sampled>250||count!=0||peak!=4||stalled<3||replacements<6||dual==0||
       gated==0||reset_full!=2||dropped!=8||accepted!=retired+dropped)
      $fatal(1,"picker coverage/conservation");
    $display("HISTORY %0d %0d %0d 0 %0d",accepted,retired,dropped,peak);
    $finish;
  end
endmodule
