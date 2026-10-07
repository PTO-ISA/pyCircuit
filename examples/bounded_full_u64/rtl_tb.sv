module tb;
  logic pyc_7079635f636c6b=0, pyc_7079635f727374=1, valid=0;
  logic [63:0] raw=0;
  logic take_wrapped=0,take_saturated=0,take_checked_value=0,take_checked_flag=0;
  wire [197:0] result;
  pyc_root dut(.*);
  bit last_clock=0,source_valid=0,sink_valid[0:3];
  logic [63:0] source_data=0,sink_data[0:3],history[0:3][0:511];
  integer head[0:3],tail[0:3],retired[0:3],dropped[0:3],isolated[0:3];
  integer sampled=0,accepted=0,peak=0,stalled=0,replacements=0,transfers=0;
  task row(input bit clock_level,reset_level,valid_level,
           input logic[63:0] data_level,input logic[3:0] takes);
    bit room,ready,pop,old_source_valid;
    logic [63:0] actual[0:3],old_source;
    bit actual_valid[0:3];
    room=1;
    for(integer k=0;k<4;k=k+1)room=room&&(!sink_valid[k]||takes[k]);
    ready=!source_valid||room;
    pyc_7079635f727374=reset_level;valid=valid_level;raw=data_level;
    take_wrapped=takes[0];take_saturated=takes[1];
    take_checked_value=takes[2];take_checked_flag=takes[3];#1;
    actual_valid[0]=result[196];actual[0]=result[195:132];
    actual_valid[1]=result[131];actual[1]=result[130:67];
    actual_valid[2]=result[66];actual[2]=result[65:2];
    actual_valid[3]=result[1];actual[3]={63'b0,result[0]};
    if(result[197]!==ready)$fatal(1,"full-u64 row %0d input ready",sampled);
    for(integer k=0;k<4;k=k+1)begin
      if(actual_valid[k]!==sink_valid[k]||
         actual[k]!==(sink_valid[k]?(k==3?64'd1:sink_data[k]):64'd0))
        $fatal(1,"full-u64 row %0d branch %0d old-Q",sampled,k);
    end
    $display("WORK %0d %0d %0d %0d %0d %0d %0d %0d %0d %0d",sampled,
      result[197],actual_valid[0],actual[0],actual_valid[1],actual[1],
      actual_valid[2],actual[2],actual_valid[3],actual[3]);
    if(clock_level&&!last_clock)begin
      if(reset_level)begin
        source_valid=0;
        for(integer k=0;k<4;k=k+1)begin
          dropped[k]=dropped[k]+tail[k]-head[k];head[k]=0;tail[k]=0;sink_valid[k]=0;
        end
      end else begin
        stalled=stalled+(valid_level&&!ready);
        replacements=replacements+(source_valid&&room&&valid_level);
        old_source=source_data;old_source_valid=source_valid;
        for(integer k=0;k<4;k=k+1)begin
          pop=sink_valid[k]&&takes[k];
          if(pop)begin
            if(head[k]==tail[k]||sink_data[k]!==history[k][head[k]])
              $fatal(1,"full-u64 branch %0d lost/reordered head",k);
            head[k]=head[k]+1;retired[k]=retired[k]+1;
            isolated[k]=isolated[k]+(!room);
          end
          if(old_source_valid&&room)begin sink_valid[k]=1;sink_data[k]=old_source;end
          else if(pop)sink_valid[k]=0;
          if(valid_level&&ready)begin history[k][tail[k]]=data_level;tail[k]=tail[k]+1;end
          if(tail[k]-head[k]>peak)peak=tail[k]-head[k];
          if(tail[k]-head[k]>2)$fatal(1,"full-u64 branch logical capacity");
        end
        transfers=transfers+(old_source_valid&&room);
        if(valid_level&&ready)begin source_valid=1;source_data=data_level;accepted=accepted+1;end
        else if(old_source_valid&&room)source_valid=0;
      end
    end
    last_clock=clock_level;sampled=sampled+1;pyc_7079635f636c6b=clock_level;#1;
  endtask
  task edge_row(input logic[63:0] d,input bit v=1,input logic[3:0] t=15,input bit r=0);
    row(1,r,v,d,t);row(0,r,v,d,t);
  endtask
  initial begin
    for(integer k=0;k<4;k=k+1)begin
      sink_valid[k]=0;sink_data[k]=0;head[k]=0;tail[k]=0;
      retired[k]=0;dropped[k]=0;isolated[k]=0;
    end
    // Establish the same reset Q that the native runner exposes before Work.
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
    row(0,1,0,0,0);edge_row(0,0,0,1);
    edge_row(0,1,0);edge_row(64'hffffffffffffffff,1,0);
    edge_row(64'h8000000000000000,1,0);edge_row(64'h7fffffffffffffff,1,0);
    for(integer branch=0;branch<4;branch=branch+1)begin
      for(integer i=0;i<4;i=i+1)edge_row(64'(32+branch*4+i),1,4'(15^(1<<branch)));
      edge_row(64'(64+branch));edge_row(64'(68+branch));
    end
    row(1,0,1,77,0);row(1,1,1,78,15);
    row(0,0,1,79,0);row(0,0,0,80,15);
    edge_row(81,1,0);edge_row(82,1,0);edge_row(0,1,15,1);
    edge_row(0);edge_row(64'hffffffffffffffff);edge_row(1);
    edge_row(64'h8000000000000000);edge_row(64'h7fffffffffffffff);
    edge_row(64'hfffffffffffffffe);
    for(integer i=0;i<96;i=i+1)
      edge_row((64'(i)*64'h9e3779b97f4a7c15)^64'ha5a55a5af00f0ff0,i%7!=1,
        {1'(i%6!=3),1'(i%4!=2),1'(i%5!=1),1'(i%3!=0)});
    for(integer i=0;i<4;i=i+1)edge_row(0,0,15);
    edge_row(64'hffffffffffffffff,1,0);edge_row(0,1,0);edge_row(0,1,15,1);
    edge_row(64'hffffffffffffffff);edge_row(0);
    for(integer i=0;i<4;i=i+1)edge_row(0,0,15);
    if(sampled>400||peak!=2||stalled<4||replacements<6||transfers<20||source_valid)
      $fatal(1,"full-u64 bounded coverage");
    for(integer k=0;k<4;k=k+1)begin
      if(sink_valid[k]||head[k]!=tail[k]||dropped[k]<2||isolated[k]==0||
         accepted!=retired[k]+dropped[k])$fatal(1,"full-u64 branch conservation");
      $display("HISTORY %0d %0d %0d %0d 0 %0d",k,accepted,retired[k],dropped[k],peak);
    end
    $finish;
  end
endmodule
