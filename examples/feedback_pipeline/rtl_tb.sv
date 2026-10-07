module tb;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic[35:0]data=0;wire[37:0]result;pyc_root dut(.*);
logic[35:0]source[0:1],feedback=0,output_data=0,history[0:4095];
integer source_count=0,head=0,tail=0,birth[0:4095],edge_index=0,row_index=0;
integer accepted=0,retired=0,dropped=0,peak=0,blocked=0,blockedUpdates=0,busyExit=0,fullReset=0;
bit fv=0,ov=0,last=0;
function automatic logic[35:0]item(input logic[31:0]v=0,input integer n=0);return{v,4'(n)};endfunction
function automatic logic[35:0]updated(input logic[35:0]p);return{p[35:4]+32'd1,p[3:0]-4'd1};endfunction
function automatic logic[35:0]completed(input logic[35:0]p);return p[3:0]==0?p:{p[35:4]+32'(p[3:0]),4'd0};endfunction
function automatic logic[31:0]constant_value(input integer i);
case(i)0:return 0;1:return 1;2:return 15;3:return 16;4:return 255;5:return 256;
6:return 32'h7fffffff;7:return 32'h80000000;8:return 32'hffffffff;9:return 32'hfffffffe;10:return 32'h55555555;11:return 32'haaaaaaaa;endcase
endfunction
function automatic logic[35:0]known_value(input integer i);
integer b,n;logic[31:0]v;
if(i<192)return item(constant_value(i/16),i%16);
b=(i-192)/4;n=(i%2)==0?0:15;v=32'(1)<<b;if(((i-192)%4)/2)v=~v;return item(v,n);
endfunction
function automatic logic[35:0]raw_value(input integer i);
integer b,n,pattern;logic[31:0]v;
n=(i%3)==0?0:(i%3)==1?1:15;v=32'ha55aa55a;
if(i<384)begin b=i/12;v[b]=((i%12)/6)?1'bz:1'bx;end
else begin pattern=(i-384)/6;for(integer j=0;j<32;j=j+1)case(pattern)
0:v[j]=1'bx;1:v[j]=1'bz;2:v[j]=j%2?1'bz:1'bx;
3:case(j%4)0:v[j]=0;1:v[j]=1;2:v[j]=1'bx;3:v[j]=1'bz;endcase
endcase end
return item(v,n);
endfunction
task row(input bit c,r,v,t,input logic[35:0]p=0,input bit four=0);
bit available,pop,room,continuing,h,source_pop,update,exit,feedback_pop,ready;logic[35:0]chosen,old_output;integer physical;
available=fv||source_count>0;chosen=fv?feedback:source_count>0?source[0]:36'd0;
pop=ov&&t;room=!ov||pop;continuing=available&&chosen[3:0]>0;h=continuing||room;
source_pop=!fv&&h&&source_count>0;update=available&&continuing;exit=available&&!continuing&&room;feedback_pop=fv&&h;ready=source_count<2||source_pop;old_output=ov?output_data:36'd0;
pyc_7079635f727374=r;valid=v;take=t;data=p;#1;
if(result!=={ready,ov,old_output})$fatal(1,"feedback old-source/resident/output model row%0d",row_index);
if(four)$display("FOUR %0d %0d %0d %b",row_index,result[37],result[36],result[35:0]);else $display("WORK %0d %0d %0d %b",row_index,result[37],result[36],result[35:0]);
if(c&&!last)begin
 if(r)begin fullReset=fullReset+(source_count+fv+ov==4);dropped=dropped+tail-head;head=0;tail=0;edge_index=0;source_count=0;fv=0;ov=0;feedback=0;output_data=0;end
 else begin
  blocked=blocked+(ov&&!t);blockedUpdates=blockedUpdates+(update&&ov&&!t);busyExit=busyExit+(exit&&fv&&source_count==2&&v);
  if(result[36]&&t)begin if(head==tail||result[35:0]!==completed(history[head])||edge_index<birth[head]+int'(history[head][3:0])+2)$fatal(1,"actual feedback accepted/retired identity/latency ledger");head=head+1;retired=retired+1;end
  if(result[37]&&v)begin history[tail]=p;birth[tail]=edge_index;tail=tail+1;accepted=accepted+1;end
  if(pop)begin ov=0;output_data=0;end
  if(feedback_pop)begin fv=0;feedback=0;end
  if(update)begin fv=1;feedback=updated(chosen);end
  if(exit)begin ov=1;output_data=chosen;end
  if(source_pop)begin source[0]=source[1];source_count=source_count-1;end
  if(v&&ready)begin source[source_count]=p;source_count=source_count+1;end
  edge_index=edge_index+1;
 end
 physical=source_count+fv+ov;if(physical>peak)peak=physical;
 if(source_count>2||physical>4||physical!=tail-head)$fatal(1,"feedback D2/D1/D1 conservation");
end
last=c;pyc_7079635f636c6b=c;#1;row_index=row_index+1;
endtask
task edge_row(input logic[35:0]p=0,input bit v=0,t=1,r=0,four=0);row(0,r,v,t,p,four);row(1,r,v,t,p,four);endtask
task drain(input bit four);for(integer i=0;i<70;i=i+1)edge_row(0,0,1,0,four);if(source_count+fv+ov!=0)$fatal(1,"finite feedback drain");endtask
task fill(input bit four);
edge_row(item(10),1,0,0,four);edge_row(item(20,3),1,0,0,four);edge_row(item(30,1),1,0,0,four);edge_row(item(40,2),1,0,0,four);
if(source_count+fv+ov!=4)$fatal(1,"all four original payload slots");
endtask
task sequence_rows(input bit four);
logic[35:0]p;integer n;
source_count=0;fv=0;ov=0;last=0;head=0;tail=0;edge_index=0;row_index=0;accepted=0;retired=0;dropped=0;peak=0;blocked=0;blockedUpdates=0;busyExit=0;fullReset=0;
row(0,1,0,0,0,four);edge_row(0,0,0,1,four);
for(integer remaining=0;remaining<16;remaining=remaining+1)begin edge_row(item(32'hffffffff-32'(remaining),remaining),1,1,0,four);for(integer i=0;i<remaining+2;i=i+1)edge_row(0,0,1,0,four);if(source_count+fv+ov!=0)$fatal(1,"E(n+2) retirement");end
fill(four);edge_row(item(50,4),1,0,0,four);edge_row(item(60,4),1,0,0,four);edge_row(item(70),1,0,0,four);
if(feedback!==item(23))$fatal(1,"continuation progresses under output blockage");
row(1,1,1,1,item(80),four);row(1,0,1,0,item(90),four);edge_row(item(100),1,1,0,four);
if(source_count!=2||fv)$fatal(1,"busy exit cannot process source head or replace full source slot");
drain(four);fill(four);edge_row(0,0,1,1,four);drain(four);
for(integer i=0;i<(four?408:320);i=i+1)begin p=four?raw_value(i):known_value(i);n=int'(p[3:0]);edge_row(p,1,1,0,four);for(integer j=0;j<n+2;j=j+1)edge_row(0,0,1,0,four);if(source_count+fv+ov!=0)$fatal(1,"isolated exact per-edge retirement");end
edge_row(item(32'hffffffff,1),1,1,0,four);for(integer i=0;i<4;i=i+1)edge_row(0,0,1,0,four);
fill(four);edge_row(0,1,1,1,four);drain(four);
if(row_index!=(four?7607:7527)||source_count+fv+ov!=0||accepted!=retired+dropped||peak!=4||fullReset<2||blocked<3||blockedUpdates<2||busyExit<1)$fatal(1,"bounded feedback coverage");
$display("HISTORY_RTL %0d %0d %0d %0d %0d %0d %0d %0d",accepted,retired,dropped,peak,blocked,blockedUpdates,busyExit,fullReset);
endtask
task reset_dut;valid=0;take=0;pyc_7079635f727374=1;pyc_7079635f636c6b=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;endtask
initial begin
`ifdef PYC_FEEDBACK_NEGATIVE_REMAINING
reset_dut();valid=1;data=item(7,2);take=1;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;
data=item(100,`PYC_FEEDBACK_BASE);data[`PYC_FEEDBACK_BIT]=`PYC_FEEDBACK_SYMBOL;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;valid=0;data=0;
// The known resident feedback token masks the queued unknown source count.
repeat(2)begin #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;end
#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`elsif PYC_FEEDBACK_NEGATIVE_POP
reset_dut();valid=1;data=item(5);#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;valid=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;take=1'bx;#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`elsif PYC_FEEDBACK_NEGATIVE_ACCEPT
reset_dut();valid=1'bx;#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`else
reset_dut();sequence_rows(0);
`ifdef PYC_FEEDBACK_FOUR_STATE
reset_dut();sequence_rows(1);
`endif
$finish;
`endif
end
endmodule
