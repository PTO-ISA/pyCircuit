module tb;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic[63:0] data=0;wire[65:0] result;
wire ready=result[65],out_valid=result[64];wire[63:0]out_data=result[63:0];
pyc_root dut(.*);
logic[63:0] qdata[0:5][0:1],source_word[0:1999];
integer uid[0:5][0:1],branch[0:5][0:1],birth[0:5][0:1],count[0:5];
bit owed[0:1999][0:1],last=0;integer copied[0:1999][0:1];
integer absolute_edge=-2,row_index=0,accepted=0,retired[0:1],dropped[0:1],peak=0,full_resets=0,asymmetric=0,replacements=0,first[0:7];
function automatic logic[63:0] arithmetic(input logic[63:0]p,input integer delta);
logic[63:0] out;bit known;integer carry,sum;
known=1;for(integer i=0;i<64;i=i+1)if(p[i]!==1'b0&&p[i]!==1'b1)known=0;
carry=delta;for(integer i=0;i<64;i=i+1)begin sum=(p[i]===1'b1)+(carry&1);out[i]=known?(sum&1):1'bx;carry=(carry>>1)+(sum>>1);end
return out;
endfunction
function automatic logic[63:0] known_value(input integer index);
integer ordinal;logic[63:0] random;
if(index<65)return index==64?64'hffffffffffffffff:(64'd1<<index)-1;
if(index<130)return index==129?64'd0:64'd1<<(index-65);
case(index)130:return 0;131:return 64'hffffffffffffffff;132:return 64'hfffffffffffffffe;133:return 64'h7fffffffffffffff;134:return 64'h8000000000000000;135:return 64'h8000000000000001;136:return 64'h5555555555555555;137:return 64'haaaaaaaaaaaaaaaa;endcase
if(index<202)begin random=64'hd2106421;for(integer i=0;i<=index-138;i=i+1)random=random*64'd6364136223846793005+64'd1442695040888963407;return random;end
ordinal=index-202;return ordinal==64?64'hfffffffffffffffe:ordinal==0?64'd0:(64'd1<<ordinal)-2;
endfunction
`ifdef PYC_BROADCAST_PIPELINE_FOUR_STATE
function automatic logic[63:0] four_value(input integer index);
logic[63:0]p;integer pattern;
if(index<256)begin p=64'ha5963c69f00f5a96;p[index/4]=(index%4)/2?1'bz:1'bx;end
else begin pattern=(index-256)/2;for(integer i=0;i<64;i=i+1)case(pattern)0:p[63-i]=1'bx;1:p[63-i]=1'bz;2:p[63-i]=i%2?1'bz:1'bx;3:case(i%4)0:p[63-i]=0;1:p[63-i]=1;2:p[63-i]=1'bx;3:p[63-i]=1'bz;endcase endcase end
return p;
endfunction
`endif
function automatic logic[63:0] value_at(input integer index,input bit four);
if(four)begin
`ifdef PYC_BROADCAST_PIPELINE_FOUR_STATE
return four_value(index);
`else
$fatal(1,"four-state fixture requires genuine Icarus");return 0;
`endif
end return known_value(index);
endfunction
 task row(input bit c,r,v,input logic[63:0]p,input bit t,four);
bit sink,merge_room,grant_l,grant_r,move_l,move_r,room_l,room_r,fan,input_ready,output_valid,pop[0:5];
integer old_uid[0:5],old_branch[0:5],old_birth[0:5],dl,dr,physical,newid,chosen,owing;
logic[63:0]old_data[0:5],expected;
sink=count[5]>0&&t;merge_room=count[5]<2||sink;
grant_l=count[3]>0&&merge_room;grant_r=count[3]==0&&count[4]>0&&merge_room;
move_l=count[1]>0&&(count[3]==0||grant_l);move_r=count[2]>0&&(count[4]==0||grant_r);
room_l=count[1]==0||move_l;room_r=count[2]==0||move_r;fan=count[0]>0&&room_l&&room_r;
input_ready=count[0]<2||fan;output_valid=count[5]>0;expected=output_valid?qdata[5][0]:64'd0;
pyc_7079635f727374=r;valid=v;data=p;take=t;#1;
if(result!=={input_ready,output_valid,expected})$fatal(1,"broadcast row %0d six-deque old-head oracle",row_index);
if(four)$display("FOUR %0d %0d %0d %b",row_index,input_ready,output_valid,expected);
else $display("WORK %0d %0d %0d %b",row_index,input_ready,output_valid,expected);
if(c&&!last)begin
 absolute_edge=absolute_edge+1;if(absolute_edge>=2000)$fatal(1,"absolute test domain");
 physical=0;for(integer i=0;i<6;i=i+1)begin physical=physical+count[i];old_data[i]=count[i]?qdata[i][0]:64'd0;old_uid[i]=uid[i][0];old_branch[i]=branch[i][0];old_birth[i]=birth[i][0];end
 if(r)begin
  dl=0;dr=0;for(integer i=0;i<accepted;i=i+1)begin dl=dl+owed[i][0];dr=dr+owed[i][1];owed[i][0]=0;owed[i][1]=0;end
  dropped[0]=dropped[0]+dl;dropped[1]=dropped[1]+dr;
  if(dl||dr)begin
   if(physical!=8||dl!=5||dr!=5)$fatal(1,"full reset branch obligations");full_resets=full_resets+1;
   if(four)$display("DROP FOUR %0d %0d %0d",dl,dr,physical);else $display("DROP WORK %0d %0d %0d",dl,dr,physical);
  end
  for(integer i=0;i<6;i=i+1)count[i]=0;
 end else begin
  if(out_valid&&t)begin
   chosen=old_branch[5];
   if(!owed[old_uid[5]][chosen]||out_data!==arithmetic(source_word[old_uid[5]],chosen+1))$fatal(1,"duplicate/missing branch obligation");
   owed[old_uid[5]][chosen]=0;retired[chosen]=retired[chosen]+1;
  end
  newid=accepted;
  if(ready&&v)begin source_word[accepted]=p;owed[accepted][0]=1;owed[accepted][1]=1;copied[accepted][0]=0;copied[accepted][1]=0;accepted=accepted+1;end
  pop[0]=fan;pop[1]=move_l;pop[2]=move_r;pop[3]=grant_l;pop[4]=grant_r;pop[5]=sink;
  for(integer i=0;i<6;i=i+1)if(pop[i])begin
   if(old_birth[i]>=absolute_edge)$fatal(1,"birth-edge flow-through");
   qdata[i][0]=qdata[i][1];uid[i][0]=uid[i][1];branch[i][0]=branch[i][1];birth[i][0]=birth[i][1];count[i]=count[i]-1;
  end
  asymmetric=asymmetric+(count[0]>0&&(room_l!=room_r));
  if(fan)begin
   for(integer b=0;b<2;b=b+1)begin
    if(copied[old_uid[0]][b]!=0)$fatal(1,"duplicate fan birth");copied[old_uid[0]][b]=1;
    qdata[1+b][count[1+b]]=old_data[0];uid[1+b][count[1+b]]=old_uid[0];branch[1+b][count[1+b]]=b;birth[1+b][count[1+b]]=absolute_edge;count[1+b]=count[1+b]+1;
   end
   if(old_uid[0]==0)first[1]=absolute_edge;
  end
  if(move_l)begin qdata[3][count[3]]=arithmetic(old_data[1],1);uid[3][count[3]]=old_uid[1];branch[3][count[3]]=0;birth[3][count[3]]=absolute_edge;count[3]=count[3]+1;if(old_uid[1]==0)first[2]=absolute_edge;end
  if(move_r)begin qdata[4][count[4]]=arithmetic(old_data[2],2);uid[4][count[4]]=old_uid[2];branch[4][count[4]]=1;birth[4][count[4]]=absolute_edge;count[4]=count[4]+1;if(old_uid[2]==0)first[3]=absolute_edge;end
  if(grant_l||grant_r)begin chosen=grant_l?3:4;qdata[5][count[5]]=old_data[chosen];uid[5][count[5]]=old_uid[chosen];branch[5][count[5]]=old_branch[chosen];birth[5][count[5]]=absolute_edge;count[5]=count[5]+1;if(old_uid[chosen]==0)first[4+old_branch[chosen]]=absolute_edge;end
  if(sink&&old_uid[5]==0)first[6+old_branch[5]]=absolute_edge;
  if(input_ready&&v)begin replacements=replacements+(fan&&count[0]==1);qdata[0][count[0]]=p;uid[0][count[0]]=newid;branch[0][count[0]]=0;birth[0][count[0]]=absolute_edge;count[0]=count[0]+1;if(newid==0)first[0]=absolute_edge;end
 end
 physical=0;for(integer i=0;i<6;i=i+1)begin if(count[i]<0||count[i]>(i==0||i==5?2:1))$fatal(1,"queue geometry");physical=physical+count[i];end
 if(physical>peak)peak=physical;
 for(integer b=0;b<2;b=b+1)begin owing=0;for(integer i=0;i<accepted;i=i+1)owing=owing+owed[i][b];if(accepted!=retired[b]+dropped[b]+owing)$fatal(1,"consumer obligation conservation");end
end
last=c;pyc_7079635f636c6b=c;#1;row_index=row_index+1;
endtask
 task edge_row(input logic[63:0]p,input bit v=1,t=1,r=0,four=0);row(1,r,v,p,t,four);row(0,r,v,p,t,four);endtask
function automatic logic[63:0]marker(input integer serial);return 64'h1000000000000000+serial*16;endfunction
 task fill(input bit four);edge_row(marker(0),1,0,0,four);for(integer i=0;i<4;i=i+1)edge_row(0,0,0,0,four);for(integer i=0;i<8;i=i+1)edge_row(marker(i+1),1,0,0,four);endtask
 task sequence_rows(input bit four);
integer n;
row_index=0;accepted=0;peak=0;full_resets=0;asymmetric=0;replacements=0;last=0;absolute_edge=-2;
for(integer i=0;i<6;i=i+1)count[i]=0;for(integer b=0;b<2;b=b+1)begin retired[b]=0;dropped[b]=0;end
for(integer i=0;i<8;i=i+1)first[i]=-99;
n=four?264:267;
row(0,1,0,0,0,four);edge_row(0,0,0,1,four);edge_row(64'hffffffffffffffff,1,1,0,four);for(integer i=0;i<7;i=i+1)edge_row(0,0,1,0,four);
fill(four);row(1,0,1,marker(20),0,four);row(1,1,1,marker(21),1,four);row(0,0,1,marker(22),0,four);row(0,0,0,marker(23),1,four);edge_row(0,1,1,1,four);
fill(four);for(integer i=0;i<16;i=i+1)edge_row(marker(i+30),1,1,0,four);
for(integer i=0;i<32;i=i+1)edge_row(0,0,1,0,four);
for(integer i=0;i<n;i=i+1)begin
 if(count[0]>=2)$fatal(1,"spaced vector source must be ready");
 edge_row(value_at(i,four),1,1,0,four);
 edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);
end
for(integer i=0;i<32;i=i+1)edge_row(0,0,1,0,four);
fill(four);edge_row(0,1,1,1,four);edge_row(64'hffffffffffffffff,1,1,0,four);for(integer i=0;i<8;i=i+1)edge_row(0,0,1,0,four);
if(peak!=8||full_resets!=2||asymmetric==0||replacements<=5)$fatal(1,"broadcast finite coverage");
for(integer b=0;b<2;b=b+1)if(dropped[b]!=10||accepted!=retired[b]+dropped[b])$fatal(1,"final obligations");
if(first[0]!=0||first[1]!=1||first[2]!=2||first[3]!=2||first[4]!=3||first[5]!=4||first[6]!=4||first[7]!=5)$fatal(1,"E0/E1/E2/E3/E4 left-first phase");
if(four)$display("HISTORY_FOUR_RTL %0d %0d %0d %0d %0d %0d",accepted,retired[0],retired[1],dropped[0],dropped[1],peak);
else $display("HISTORY_RTL %0d %0d %0d %0d %0d %0d",accepted,retired[0],retired[1],dropped[0],dropped[1],peak);
endtask
initial begin
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;sequence_rows(0);
`ifdef PYC_BROADCAST_PIPELINE_FOUR_STATE
pyc_7079635f727374=1;valid=0;take=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;sequence_rows(1);
`endif
$finish;end
endmodule
