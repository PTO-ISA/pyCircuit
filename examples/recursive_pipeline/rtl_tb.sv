module tb;
  logic pyc_7079635f636c6b=0, pyc_7079635f727374=1, valid=0, take=0;
  logic[63:0] data=0;
  wire[65:0] result;
  pyc_root dut(.*);
  // Four independent complete-token FIFOs. Transfers use only old heads and
  // occupancies; arrivals cannot flow through an empty stage on the same edge.
  logic[63:0] queue_data[0:3][0:1], history[0:255];
  integer count[0:3], births[0:255];
  bit last_clock=0, first_retirement=0, zero_retirement=0;
  integer sampled=0,edges=0,accepted=0,retired=0,dropped=0,peak=0;
  integer stalled=0,replacements=0,reset_full=0,head=0,tail=0,outstanding=0;

  task row(input bit clock_level,reset_level,valid_level,
           input logic[63:0] data_level,input bit take_level);
    bit pop[0:3],ready,available;
    logic[63:0] expected,actual,old_heads[0:3];
    integer occupancy;
    pop[3]=count[3]>0&&take_level;
    for(integer i=2;i>=0;i=i-1)
      pop[i]=count[i]>0&&(count[i+1]<2||pop[i+1]);
    ready=count[0]<2||pop[0]; available=count[3]>0;
    expected=available?queue_data[3][0]:64'd0;
    pyc_7079635f727374=reset_level; valid=valid_level;
    data=data_level; take=take_level; #1;
    actual=result[63:0];
    if(result!=={ready,available,expected})
      $fatal(1,"recursive row %0d four-deque old-Q oracle",sampled);
    $display("WORK %0d %0d %0d %b",sampled,result[65],result[64],actual);
    if(clock_level&&!last_clock)begin
      if(reset_level)begin
        reset_full=reset_full+(outstanding==8); dropped=dropped+outstanding;
        outstanding=0; head=0; tail=0;
        for(integer i=0;i<4;i=i+1)count[i]=0;
      end else begin
        stalled=stalled+(valid_level&&!ready);
        replacements=replacements+(outstanding==8&&pop[3]&&valid_level&&ready);
        if(pop[3])begin
          if(outstanding==0||actual!==history[head]||edges-births[head]<4)
            $fatal(1,"recursive order/conservation/earliest edge");
          if(!first_retirement)begin
            if(edges-births[head]!=4||actual!==64'd2)$fatal(1,"recursive E4 result");
            first_retirement=1;
          end
          if(actual==0)zero_retirement=1;
          head=head+1; outstanding=outstanding-1; retired=retired+1;
        end
        if(valid_level&&ready)begin
          history[tail]=data_level+64'd3; births[tail]=edges;
          tail=tail+1; outstanding=outstanding+1; accepted=accepted+1;
        end
        for(integer i=0;i<4;i=i+1)old_heads[i]=queue_data[i][0];
        for(integer i=0;i<4;i=i+1)
          if(pop[i])begin queue_data[i][0]=queue_data[i][1]; count[i]=count[i]-1; end
        for(integer i=1;i<4;i=i+1)
          if(pop[i-1])begin queue_data[i][count[i]]=old_heads[i-1]+64'd1; count[i]=count[i]+1; end
        if(valid_level&&ready)begin queue_data[0][count[0]]=data_level; count[0]=count[0]+1; end
      end
      occupancy=0;
      for(integer i=0;i<4;i=i+1)begin
        if(count[i]>2)$fatal(1,"recursive stage capacity");
        occupancy=occupancy+count[i];
      end
      if(occupancy!=outstanding||occupancy>8)$fatal(1,"recursive token conservation");
      if(occupancy>peak)peak=occupancy;
      edges=edges+1;
    end
    last_clock=clock_level; sampled=sampled+1;
    pyc_7079635f636c6b=clock_level; #1;
  endtask
  task edge_row(input logic[63:0] d,input bit v=1,t=1,r=0);
    row(1,r,v,d,t); row(0,r,v,d,t);
  endtask
  initial begin
    logic[63:0] random_value,offered;
    for(integer i=0;i<4;i=i+1)count[i]=0;
    // Match native host Reset before the first traced Work sample.
    #1; pyc_7079635f636c6b=1; #1; pyc_7079635f636c6b=0; #1;
    row(0,1,0,0,0); edge_row(0,0,0,1);
    edge_row(64'hffffffffffffffff);
    for(integer i=0;i<4;i=i+1)edge_row(0,0);
    for(integer i=0;i<12;i=i+1)edge_row(64'hffffffffffffffff-64'(i),1,0);
    row(1,0,1,17,0); row(1,1,1,18,1);
    row(0,0,1,19,1); row(0,0,0,20,0);
    edge_row(0,1,1,1);
    for(integer i=0;i<12;i=i+1)edge_row(64'(i),1,0);
    for(integer i=0;i<6;i=i+1)edge_row(64'hffffffffffffffff-64'(i));
    random_value=64'hd2106421;
    for(integer i=0;i<48;i=i+1)begin
      random_value=random_value*64'd6364136223846793005+64'd1442695040888963407;
      offered=i%8==0?64'hffffffffffffffff:i%8==1?64'hfffffffffffffffe:random_value;
      edge_row(offered,i%5!=1,i%4!=0);
    end
    for(integer i=0;i<12;i=i+1)edge_row(0,0);
    for(integer i=0;i<12;i=i+1)edge_row(64'(96+i),1,0);
    edge_row(0,1,1,1); edge_row(64'hfffffffffffffffd);
    for(integer i=0;i<4;i=i+1)edge_row(0,0);
    if(sampled>250||outstanding!=0||peak!=8||stalled<3||replacements<6||
       reset_full!=2||!first_retirement||!zero_retirement||dropped!=16||
       accepted!=retired+dropped)$fatal(1,"recursive coverage/conservation");
    $display("HISTORY %0d %0d %0d 0 %0d",accepted,retired,dropped,peak);
    $finish;
  end
endmodule
