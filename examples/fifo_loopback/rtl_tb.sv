module tb;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,in_valid=0,out_ready=0;
logic[7:0] in_data=0;
wire[9:0] result;
wire ready=result[9],available=result[8];wire[7:0] data=result[7:0];
pyc_root dut(.*);
logic[7:0] tokens[0:1],history[0:2047];
bit last_clock=0;
integer count=0,row_index=0,head=0,tail=0,initial_count=0,accepted=0,retired=0,dropped=0,outstanding=0,peak=0,full_replace=0,reset_full=0;
string phase="";
task finish_phase;
 if(initial_count+accepted!=retired+dropped+outstanding)$fatal(1,"ledger conservation");
 if(phase=="SMOKE" && (initial_count!=0||accepted!=4||retired!=3||dropped!=0||outstanding!=1||history[head]!==8'h2a))$fatal(1,"historical smoke finish");
 if(phase=="DRAIN" && (initial_count!=1||accepted!=0||retired!=1||dropped!=0||outstanding!=0))$fatal(1,"separate extended drain");
 if(phase=="EXT"||phase=="FOUR")begin
  if(initial_count!=0||accepted!=(phase=="EXT"?301:85)||retired!=accepted-4||dropped!=4||outstanding!=0||peak!=2||full_replace!=41||reset_full!=2)$fatal(1,"extended coverage");
 end
 $display("HISTORY_RTL %s %0d %0d %0d %0d %0d %0d",phase,initial_count,accepted,retired,dropped,outstanding,peak);
endtask
task row(input string tag,input integer tag_index,input bit clock_level,input logic reset_level,valid_level,input logic[7:0] payload,input logic take_level);
 logic expected_ready,expected_valid,push_token,pop_token;logic[7:0] expected_data;bit was_full;string next_phase;
 if(tag=="OBS_SMOKE")next_phase="SMOKE";else next_phase=tag;
 if(phase!=next_phase)begin
  if(phase!="")finish_phase();
  phase=next_phase;initial_count=outstanding;accepted=0;retired=0;dropped=0;full_replace=0;reset_full=0;peak=initial_count;
 end
 expected_valid=count>0;expected_ready=(count<2)|(expected_valid&take_level);expected_data=count>0?tokens[0]:8'd0;
 pyc_7079635f727374=reset_level;in_valid=valid_level;in_data=payload;out_ready=take_level;#1;
 if(result!=={expected_ready,expected_valid,expected_data})$fatal(1,"fifo_loopback row%0d %s independent deque",row_index,tag);
 if(tag=="FOUR")$display("FOUR %0d %s %0d %b %b %b",row_index,tag,tag_index,ready,available,data);
 else $display("WORK %0d %s %0d %b %b %b",row_index,tag,tag_index,ready,available,data);
 if(clock_level&&!last_clock)begin
  if(reset_level===1'b1)begin
   reset_full=reset_full+(outstanding==2);dropped=dropped+outstanding;outstanding=0;head=0;tail=0;count=0;
  end else begin
   if(reset_level!==1'b0)$fatal(1,"unexpected unknown reset in positive sequence");
   was_full=outstanding==2;
   if((available&take_level)===1'b1)begin
    if(outstanding==0||data!==history[head])$fatal(1,"actual DUT retired history loss/order");
    head=head+1;retired=retired+1;outstanding=outstanding-1;
   end
   if((ready&valid_level)===1'b1)begin
    history[tail]=payload;tail=tail+1;accepted=accepted+1;outstanding=outstanding+1;full_replace=full_replace+was_full;
   end
   pop_token=expected_valid&take_level;push_token=expected_ready&valid_level;
   if($isunknown(pop_token)||$isunknown(push_token))$fatal(1,"unexpected effective unknown in positive sequence");
   if(pop_token)begin tokens[0]=tokens[1];count=count-1;end
   if(push_token)begin tokens[count]=payload;count=count+1;end
  end
  if(outstanding>peak)peak=outstanding;
  if(count<0||count>2||outstanding!=count)$fatal(1,"D2 capacity/ledger");
 end
 last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;row_index=row_index+1;
endtask
task edge_row(input string tag,input integer index,input logic[7:0]p=0,input logic v=0,t=0,r=0);
 row(tag,index,0,r,v,p,t);row(tag,index+1,1,r,v,p,t);
endtask
function automatic logic[7:0] four_value(input integer index);
 integer bit_index,pattern;logic[7:0] p;
 if(index<32)begin
  bit_index=index/4;p=8'ha5;p[bit_index]=(index%4)/2?1'bz:1'bx;
 end else begin
  pattern=(index-32)/2;
  for(integer b=0;b<8;b=b+1)begin
   case(pattern)
   0:p[b]=1'bx;1:p[b]=1'bz;2:p[b]=b%2?1'bz:1'bx;
   3:case(b%4)0:p[b]=0;1:p[b]=1;2:p[b]=1'bx;3:p[b]=1'bz;endcase
   endcase
  end
 end
 return p;
endfunction
task extended(input bit four);
 integer n,limit;string tag;logic[7:0] p;
 tag=four?"FOUR":"EXT";n=0;limit=four?40:256;
 row(tag,n++,0,1,0,0,0);edge_row(tag,n,0,0,0,1);n=n+2;
 edge_row(tag,n,8'h17,1,1);n=n+2;edge_row(tag,n,8'he2,1);n=n+2;edge_row(tag,n,8'h33,1);n=n+2;
 row(tag,n++,1,1,1,8'haa,0);row(tag,n++,1,1,1,8'h55,1);
 if(four)begin row(tag,n++,1,1'bx,0,8'hff,1'bx);row(tag,n++,1,1'bz,0,0,1'bz);end
 row(tag,n++,0,1,1,8'h44,0);row(tag,n++,0,1,1,8'h77,0);
 edge_row(tag,n,8'h91,1,1);n=n+2;
 for(integer i=0;i<40;i=i+1)begin edge_row(tag,n,8'((i*37)&255),1,1);n=n+2;end
 edge_row(tag,n,0,four?1'bz:1'b1,four?1'bx:1'b1,1);n=n+2;
 for(integer i=0;i<4;i=i+1)begin edge_row(tag,n,0,0,1);n=n+2;end
 for(integer i=0;i<limit;i=i+1)begin p=four?four_value(i):8'(i);edge_row(tag,n,p,1,1);n=n+2;end
 for(integer i=0;i<3;i=i+1)begin edge_row(tag,n,0,0,1);n=n+2;end
 edge_row(tag,n,8'h11,1);n=n+2;edge_row(tag,n,8'h22,1);n=n+2;
 if(four)begin edge_row(tag,n,0,1'bx);n=n+2;edge_row(tag,n,0,1'bz);n=n+2;end
 edge_row(tag,n,0,four?1'bx:1'b1,four?1'bz:1'b1,1);n=n+2;
 for(integer i=0;i<3;i=i+1)begin edge_row(tag,n,0,0,1);n=n+2;end
 if(four)begin edge_row(tag,n,0,0,1'bx);n=n+2;edge_row(tag,n,0,0,1'bz);n=n+2;end
 if(n!=(four?213:635))$fatal(1,"wrong extended finite sample count");
endtask
initial begin
`ifdef PYC_FIFO_LOOPBACK_NEGATIVE
 #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
 if(`PYC_FIFO_LOOPBACK_NEGATIVE==3)begin in_valid=1;in_data=8'h37;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;in_valid=0;#1;end
 if(`PYC_FIFO_LOOPBACK_NEGATIVE==1)pyc_7079635f727374=1'bx;
 else if(`PYC_FIFO_LOOPBACK_NEGATIVE==2)in_valid=1'bz;
 else out_ready=1'bx;
 #1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`else
`ifdef PYC_FIFO_LOOPBACK_FOUR_STATE
 #1;if(result!==10'bxxxxxxxxxx)$fatal(1,"current-contract cold output must be X");
`endif
 #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
 // Fourteen original low/rising samples plus seven explicit nonedge observations.
 begin
 integer smoke,observation;logic r,v,t;smoke=0;observation=0;
 for(integer cycle=0;cycle<7;cycle=cycle+1)begin
  r=cycle<2;v=cycle>=3;t=v;
  row("SMOKE",smoke++,0,r,v,v?8'h2a:8'd0,t);
  row("SMOKE",smoke++,1,r,v,v?8'h2a:8'd0,t);
  row("OBS_SMOKE",observation++,1,r,v,v?8'h2a:8'd0,t);
 end
 if(smoke!=14||observation!=7)$fatal(1,"original smoke sample accounting");
 end
 edge_row("DRAIN",0,0,0,1);extended(0);finish_phase();
 if(row_index!=658)$fatal(1,"known sample count");
`ifdef PYC_FIFO_LOOPBACK_FOUR_STATE
 // Separate normal host-reset equivalent setup, never continue a failed system.
 pyc_7079635f727374=1;in_valid=0;out_ready=0;#1;pyc_7079635f636c6b=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
 phase="";row_index=0;count=0;last_clock=0;outstanding=0;head=0;tail=0;
 extended(1);finish_phase();if(row_index!=213)$fatal(1,"four-state sample count");
`endif
 $finish;
`endif
end
endmodule
