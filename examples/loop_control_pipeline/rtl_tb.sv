module tb;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic [5:0] data=0;
wire [7:0] result;
wire ready=result[7],available=result[6];
wire [5:0] payload=result[5:0];
pyc_root dut(.*);
logic [5:0] source_tokens[0:1],feedback_token=0,output_token=0;
bit feedback_valid=0,output_valid=0,last_clock=0;
integer source_count=0,row_index=0,edges=0,head=0,tail=0;
logic [5:0] history[0:8191];
integer births[0:8191],latencies[0:8191];
integer accepted=0,retired=0,dropped=0,outstanding=0,peak=0;
integer blocked=0,full_progress=0,feedback_exit=0,full_exit=0,replacements=0,reset_full=0;
function automatic logic continuing(input logic [5:0] p);
 return (p[5:2]>0)&&!p[1];
endfunction
function automatic logic [5:0] completed(input logic [5:0] p);
 if(continuing(p)===1'b1)return {4'd0,p[1:0]};
 return p;
endfunction
function automatic logic [5:0] symbolic(input integer index);
 logic [5:0] p;
 for(integer b=0;b<6;b=b+1)begin
  case(index%4)0:p[b]=0;1:p[b]=1;2:p[b]=1'bx;3:p[b]=1'bz;endcase
  index=index/4;
 end
 return p;
endfunction
task row(input string tag,input bit clock_level,input logic [5:0] p=0,
         input logic v=0,t=0,r=0);
 logic [5:0] selected,expected_data,next_feedback;
 logic c,h,source_take,result_ready,expected_ready,expected_valid;
 logic source_pop,source_push,feedback_pop,feedback_push,output_pop,output_push;
 bit full,resident;
 selected=feedback_valid?feedback_token:(source_count>0?source_tokens[0]:6'd0);
 c=continuing(selected);result_ready=!output_valid|(output_valid&t);
 h=c|result_ready;source_take=!feedback_valid&h;
 expected_ready=(source_count<2)|((source_count>0)&source_take);
 expected_valid=output_valid;expected_data=output_valid?output_token:6'd0;
 pyc_7079635f727374=r;valid=v;data=p;take=t;#1;
 if(result!=={expected_ready,expected_valid,expected_data})
  $fatal(1,"loop-control row %0d %s independent old-Q queues",row_index,tag);
 if(tag=="FOUR")$display("FOUR %0d %s %b %b %b",row_index,tag,ready,available,payload);
 else $display("WORK %0d %s %b %b %b",row_index,tag,ready,available,payload);
 if(clock_level&&!last_clock)begin
  if(r===1'b1)begin
   reset_full=reset_full+(outstanding==4);dropped=dropped+outstanding;
   outstanding=0;head=0;tail=0;source_count=0;feedback_valid=0;output_valid=0;
  end else begin
   if(r!==1'b0)$fatal(1,"unknown reset in successful sequence");
   full=source_count==2;resident=feedback_valid;
   blocked=blocked+(resident&&c===1'b0&&result_ready===1'b0);
   full_progress=full_progress+(resident&&c===1'b1&&output_valid&&t===1'b0);
   if(resident&&c===1'b0&&result_ready===1'b1)begin
    feedback_exit=feedback_exit+1;full_exit=full_exit+full;
    if(source_take!==1'b0||(full&&ready!==1'b0))$fatal(1,"resident exit source-head/full-refill forbidden");
   end
   replacements=replacements+(full&&ready===1'b1&&v===1'b1);
   if(available===1'b1&&t===1'b1)begin
    if(outstanding==0||payload!==history[head])$fatal(1,"actual retirement identity/order/planes");
    if((tag=="KNOWN"||tag=="FOUR")&&edges-births[head]!=latencies[head])$fatal(1,"exact n+2 / leading-break E2 latency");
    head=head+1;retired=retired+1;outstanding=outstanding-1;
   end
   if(ready===1'b1&&v===1'b1)begin
    history[tail]=completed(p);births[tail]=edges;
    latencies[tail]=(continuing(p)=== 1'b1 ? p[5:2] : 0)+2;
    tail=tail+1;accepted=accepted+1;outstanding=outstanding+1;
   end
   source_pop=(source_count>0)&source_take;source_push=v&expected_ready;
   feedback_pop=feedback_valid&h;
   feedback_push=(feedback_valid||(source_count>0))&c&(!feedback_valid|feedback_pop);
   output_pop=output_valid&t;output_push=(feedback_valid||(source_count>0))&!c&result_ready;
   if((source_pop!==1'b0&&source_pop!==1'b1)||
      (source_push!==1'b0&&source_push!==1'b1)||
      (feedback_pop!==1'b0&&feedback_pop!==1'b1)||
      (feedback_push!==1'b0&&feedback_push!==1'b1)||
      (output_pop!==1'b0&&output_pop!==1'b1)||
      (output_push!==1'b0&&output_push!==1'b1))
    $fatal(1,"unknown effective transfer in successful sequence");
   next_feedback={selected[5:2]-4'd1,selected[1:0]};
   if(source_pop)begin source_tokens[0]=source_tokens[1];source_count=source_count-1;end
   if(source_push)begin source_tokens[source_count]=p;source_count=source_count+1;end
   if(feedback_pop)feedback_valid=0;
   if(feedback_push)begin feedback_token=next_feedback;feedback_valid=1;end
   if(output_pop)output_valid=0;
   if(output_push)begin output_token=selected;output_valid=1;end
  end
  edges=edges+1;
 end
 last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;row_index=row_index+1;
 if(outstanding>peak)peak=outstanding;
 if(source_count<0||source_count>2||outstanding>4||outstanding!=source_count+feedback_valid+output_valid||accepted!=retired+dropped+outstanding)
  $fatal(1,"four-slot capacity or actual ledger conservation");
endtask
task edge_row(input string tag,input logic [5:0] p=0,input logic v=0,t=0,r=0);
 row(tag,0,p,v,t,r);row(tag,1,p,v,t,r);
endtask
task isolated(input bit four);
 integer limit,n;logic [5:0] p;string tag;
 tag=four?"FOUR":"KNOWN";limit=four?4096:64;
 edge_row(tag,0,0,0,1);
 for(integer i=0;i<limit;i=i+1)begin
  for(integer latent=0;latent<(four?2:1);latent=latent+1)begin
   p=four?symbolic(i):6'(i);
   if(continuing(p)!==1'bx)begin
    n=continuing(p)=== 1'b1 ? p[5:2] : 0;
    edge_row(tag,p,1,1);
    for(integer j=0;j<n+3;j=j+1)edge_row(tag,0,0,1);
   end
  end
 end
endtask
task extended;
 edge_row("EXT",38,1);edge_row("EXT",61,1);edge_row("EXT",19,1);edge_row("EXT",30,1);
 for(integer i=0;i<20;i=i+1)edge_row("EXT",6'((i%16)*4),1);
 row("EXT",1,63,1,1,1);row("EXT",1,17,1,0);
 row("EXT",0,23,1,0,1);row("EXT",0,27,1,0,1);
 for(integer i=0;i<80;i=i+1)edge_row("EXT",6'((i%5)*4+(i%3==0)*2+i%2),1,1);
 for(integer i=0;i<24;i=i+1)edge_row("EXT",0,0,1);
 edge_row("EXT",38,1);edge_row("EXT",61,1);edge_row("EXT",19,1);edge_row("EXT",30,1);
 edge_row("EXT",0,1,1,1);
 for(integer i=0;i<4;i=i+1)edge_row("EXT",0,0,1);
endtask
task finish_phase(input bit four);
 string phase;
 phase=four?"FOUR":"KNOWN_EXT";
 if(outstanding!=0||accepted!=retired+dropped)$fatal(1,"finite drain/conservation");
 if(four)begin
  if(accepted!=2192||retired!=accepted||dropped!=0)$fatal(1,"1096 symbolic positives twice");
 end else if(accepted<100||peak!=4||dropped!=4||reset_full!=1||blocked<5||full_progress<10||full_exit<1||replacements<5)
  $fatal(1,"known scheduler coverage");
 $display("HISTORY_RTL %s %0d %0d %0d %0d %0d",phase,accepted,retired,dropped,outstanding,peak);
 $display("COVERAGE_RTL %s %0d %0d %0d %0d %0d %0d",phase,blocked,full_progress,feedback_exit,full_exit,replacements,reset_full);
endtask
initial begin
`ifdef PYC_LOOP_CONTROL_NEGATIVE
 // Each genuine RTL fatal case is isolated; no failed system is resumed.
 #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
 if(`PYC_LOOP_CONTROL_NEGATIVE<=4)begin
  case(`PYC_LOOP_CONTROL_NEGATIVE)
   1:data=6'bx00100;2:data=6'bz00100;
   3:data=6'b0001x0;4:data=6'b0001z0;
  endcase
  valid=1;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;valid=0;#1;
 end else if(`PYC_LOOP_CONTROL_NEGATIVE==5)pyc_7079635f727374=1'bx;
 else if(`PYC_LOOP_CONTROL_NEGATIVE==6)valid=1'bz;
 else begin
  // Prepare output, feedback and both source slots before an unknown output pop.
  edge_row("SETUP",38,1);edge_row("SETUP",13,1);edge_row("SETUP",19,1);edge_row("SETUP",30,1);
  pyc_7079635f636c6b=0;valid=0;take=1'bx;#1;
 end
 #1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`else
`ifdef PYC_LOOP_CONTROL_FOUR_STATE
 #1;if(result!==8'bxxxxxxxx)$fatal(1,"current cold output X");
`endif
 // Establish the same known-empty state as the native host Reset.
 #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
 isolated(0);extended();finish_phase(0);
`ifdef PYC_LOOP_CONTROL_FOUR_STATE
 // Host Reset counterpart; positive suite begins with independent empty images.
 pyc_7079635f636c6b=0;pyc_7079635f727374=1;valid=0;take=0;#1;
 pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
 source_count=0;feedback_valid=0;output_valid=0;last_clock=0;
 row_index=0;edges=0;head=0;tail=0;accepted=0;retired=0;dropped=0;outstanding=0;peak=0;
 blocked=0;full_progress=0;feedback_exit=0;full_exit=0;replacements=0;reset_full=0;
 isolated(1);finish_phase(1);
`endif
 $finish;
`endif
end
endmodule
