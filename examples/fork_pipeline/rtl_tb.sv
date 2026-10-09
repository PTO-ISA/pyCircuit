module tb;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,left_take=0,right_take=0;
logic[63:0] data=0;wire[130:0] result;
wire ready=result[130],left_valid=result[129],right_valid=result[64];
wire[63:0] left_data=result[128:65],right_data=result[63:0];
pyc_root dut(.*);
logic[63:0] input_data,left_q[0:1],right_q[0:1],owed[0:1][0:1999];
integer input_id,left_id[0:1],right_id[0:1],owed_id[0:1][0:1999];
bit input_full=0,done_left=0,done_right=0,last=0;
integer count_left=0,count_right=0,row_index=0,accepted=0,retired[0:1],dropped[0:1],head[0:1],tail[0:1],outstanding[0:1];
integer peak=0,resets=0,partial_copies=0,suppressed=0,replacements=0;
function automatic logic[63:0] known_value(input integer index);
if(index<128)return index%2?~(64'd1<<(index/2)):(64'd1<<(index/2));
case(index)128:return 0;129:return 64'hffffffffffffffff;130:return 64'h5555555555555555;131:return 64'haaaaaaaaaaaaaaaa;default:begin $fatal(1,"bad known vector");return 0;end endcase
endfunction
`ifdef PYC_FORK_PIPELINE_FOUR_STATE
function automatic logic[63:0] four_value(input integer index);
logic[63:0] p;integer b,pattern;
if(index<256)begin p=64'ha5963c69f00f5a96;b=index/4;p[b]=(index%4)/2?1'bz:1'bx;end
else begin
 pattern=(index-256)/2;
 for(integer k=0;k<64;k=k+1)case(pattern)
 0:p[63-k]=1'bx;1:p[63-k]=1'bz;2:p[63-k]=k%2?1'bz:1'bx;
 3:case(k%4)0:p[63-k]=0;1:p[63-k]=1;2:p[63-k]=1'bx;3:p[63-k]=1'bz;endcase
 endcase
end
return p;
endfunction
`endif
function automatic logic[63:0] value_at(input integer index,input bit four);
if(four)begin
`ifdef PYC_FORK_PIPELINE_FOUR_STATE
return four_value(index);
`else
$fatal(1,"four-state fixture requires genuine Icarus");return 0;
`endif
end
return known_value(index);
endfunction
 task row(input bit c,r,v,input logic[63:0]p,input bit l,rr,four);
bit vl,vr,room_l,room_r,copy_l,copy_r,complete,input_ready;integer physical,oldid;logic[63:0]dl,dr,olddata;
vl=count_left>0;vr=count_right>0;dl=vl?left_q[0]:64'd0;dr=vr?right_q[0]:64'd0;
room_l=count_left<2||(vl&&l);room_r=count_right<2||(vr&&rr);
copy_l=input_full&&!done_left&&room_l;copy_r=input_full&&!done_right&&room_r;
complete=input_full&&(done_left||copy_l)&&(done_right||copy_r);input_ready=!input_full||complete;
pyc_7079635f727374=r;valid=v;data=p;left_take=l;right_take=rr;#1;
if(result!=={input_ready,vl,dl,vr,dr})$fatal(1,"fork row %0d old-head/obligation oracle",row_index);
if(four)$display("FOUR %0d %0d %0d %b %0d %b",row_index,input_ready,vl,dl,vr,dr);
else $display("WORK %0d %0d %0d %b %0d %b",row_index,input_ready,vl,dl,vr,dr);
if(c&&!last)begin
 physical=(input_full?1:0)+count_left+count_right;
 if(r)begin
  if(outstanding[0]||outstanding[1])begin
   if(four)$display("DROP FOUR %0d %0d %0d",outstanding[0],outstanding[1],physical);
   else $display("DROP WORK %0d %0d %0d",outstanding[0],outstanding[1],physical);
   resets=resets+1;
  end
  for(integer i=0;i<2;i=i+1)begin dropped[i]=dropped[i]+outstanding[i];outstanding[i]=0;head[i]=0;tail[i]=0;end
  input_full=0;count_left=0;count_right=0;done_left=0;done_right=0;
 end else begin
  if(left_valid&&l)begin
   if(outstanding[0]==0||left_data!==owed[0][head[0]]||left_id[0]!=owed_id[0][head[0]])$fatal(1,"left actual obligation history");
   head[0]=head[0]+1;outstanding[0]=outstanding[0]-1;retired[0]=retired[0]+1;
  end
  if(right_valid&&rr)begin
   if(outstanding[1]==0||right_data!==owed[1][head[1]]||right_id[0]!=owed_id[1][head[1]])$fatal(1,"right actual obligation history");
   head[1]=head[1]+1;outstanding[1]=outstanding[1]-1;retired[1]=retired[1]+1;
  end
  if(ready&&v)begin
   for(integer i=0;i<2;i=i+1)begin owed[i][tail[i]]=p;owed_id[i][tail[i]]=accepted;tail[i]=tail[i]+1;outstanding[i]=outstanding[i]+1;end
  end
  olddata=input_data;oldid=input_id;
  partial_copies=partial_copies+(input_full&&(copy_l!=copy_r)&&!complete);
  suppressed=suppressed+(input_full&&done_left&&room_l)+(input_full&&done_right&&room_r);
  if(vl&&l)begin left_q[0]=left_q[1];left_id[0]=left_id[1];count_left=count_left-1;end
  if(vr&&rr)begin right_q[0]=right_q[1];right_id[0]=right_id[1];count_right=count_right-1;end
  if(copy_l)begin left_q[count_left]=olddata;left_id[count_left]=oldid;count_left=count_left+1;end
  if(copy_r)begin right_q[count_right]=olddata;right_id[count_right]=oldid;count_right=count_right+1;end
  if(complete)begin input_full=0;done_left=0;done_right=0;end
  else if(input_full)begin done_left=done_left||copy_l;done_right=done_right||copy_r;end
  if(input_ready&&v)begin
   replacements=replacements+complete;input_data=p;input_id=accepted;accepted=accepted+1;input_full=1;
  end
 end
 physical=(input_full?1:0)+count_left+count_right;if(physical>peak)peak=physical;
 if(physical>5||count_left>2||count_right>2||(!input_full&&(done_left||done_right)))$fatal(1,"fork physical/state invariant");
 for(integer i=0;i<2;i=i+1)if(accepted!=retired[i]+dropped[i]+outstanding[i]||outstanding[i]>3)$fatal(1,"per-consumer conservation");
end
last=c;pyc_7079635f636c6b=c;#1;row_index=row_index+1;
endtask
 task edge_row(input logic[63:0]p,input bit v=1,l=1,r=1,reset=0,four=0);row(1,reset,v,p,l,r,four);row(0,reset,v,p,l,r,four);endtask
 task partial(input bit mirror,four);
for(integer i=0;i<3;i=i+1)edge_row(value_at(i,four),1,0,0,0,four);
for(integer i=0;i<5;i=i+1)edge_row(value_at(i+3,four),1,!mirror,mirror,0,four);
row(1,0,1,value_at(8,four),!mirror,mirror,four);row(1,1,1,value_at(9,four),1,1,four);
row(0,0,1,value_at(10,four),!mirror,mirror,four);row(0,0,0,value_at(11,four),mirror,!mirror,four);
edge_row(value_at(12,four),1,1,1,0,four);edge_row(value_at(13,four),0,0,0,0,four);
edge_row(0,1,1,1,1,four);
endtask
 task sequence_rows(input bit four);
integer n;
row_index=0;accepted=0;peak=0;resets=0;partial_copies=0;suppressed=0;replacements=0;
input_full=0;count_left=0;count_right=0;done_left=0;done_right=0;last=0;
for(integer i=0;i<2;i=i+1)begin retired[i]=0;dropped[i]=0;outstanding[i]=0;head[i]=0;tail[i]=0;end
n=four?264:132;row(0,1,0,0,0,0,four);edge_row(0,0,0,0,1,four);
partial(0,four);partial(1,four);
for(integer i=0;i<n;i=i+1)edge_row(value_at(i,four),1,1,1,0,four);
for(integer i=0;i<12;i=i+1)edge_row(0,0,1,1,0,four);
for(integer i=0;i<3;i=i+1)edge_row(value_at(i,four),1,0,0,0,four);
for(integer i=0;i<8;i=i+1)edge_row(value_at(i+3,four),1,0,0,0,four);
edge_row(0,1,1,1,1,four);edge_row(value_at(n-1,four),1,1,1,0,four);edge_row(0,0,1,1,0,four);
for(integer i=0;i<12;i=i+1)edge_row(0,0,1,1,0,four);
if(peak!=5||resets!=3||partial_copies<4||suppressed<4||replacements<=20||outstanding[0]!=0||outstanding[1]!=0||dropped[0]!=7||dropped[1]!=7)$fatal(1,"fork finite coverage");
if(four)$display("HISTORY_FOUR_RTL %0d %0d %0d %0d %0d %0d",accepted,retired[0],retired[1],dropped[0],dropped[1],peak);
else $display("HISTORY_RTL %0d %0d %0d %0d %0d %0d",accepted,retired[0],retired[1],dropped[0],dropped[1],peak);
endtask
initial begin
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(0);
`ifdef PYC_FORK_PIPELINE_FOUR_STATE
pyc_7079635f727374=1;valid=0;left_take=0;right_take=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(1);
`endif
$finish;end
endmodule
