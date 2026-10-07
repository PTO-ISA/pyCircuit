module tb;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,control_valid=0,lane0_valid=0,lane1_valid=0,take=0;
logic control=0;logic[63:0]lane0_data=0,lane1_data=0;wire[67:0]result;
pyc_root dut(.*);
logic[63:0]qin[0:2][0:1],qout[0:1],admitted[0:2][0:4095],joined[0:4095];
integer count[0:2],head[0:2],tail[0:2],accepted[0:2],dropped[0:2],born[0:2][0:4095];
integer joined_lane[0:4095],joined_birth[0:4095],jh=0,jt=0,co=0,retired[0:1],edge_index=0;
integer row_index=0,peak=0,replacements=0,blocked=0,hol=0,fullReset=0;bit last=0;
function automatic logic[63:0]marker(input integer n);return 64'h8123456789abcdef^(64'(n)*64'h0101010101010101);endfunction
function automatic logic[63:0]known_value(input integer i);
integer length;
if(i<64)return 64'(1)<<i;
if(i<128)return ~(64'(1)<<(i-64));
if(i<193)begin length=i-128;return length==64?'1:(64'(1)<<length)-1;end
if(i<201)case(i-193)
0:return 0;1:return '1;2:return 64'hfffffffffffffff0;3:return 64'h7fffffffffffffff;
4:return 64'h8000000000000000;5:return 64'h8000000000000001;6:return 64'h5555555555555555;7:return 64'haaaaaaaaaaaaaaaa;endcase
return marker(i-201);
endfunction
function automatic logic[63:0]raw_value(input integer i);
integer b,pattern;logic[63:0]p;
p=64'ha55aa55aa55aa55a;
if(i<256)begin b=i/4;p[b]=(i%4)/2?1'bz:1'bx;end
else begin pattern=(i-256)/2;for(integer j=0;j<64;j=j+1)case(pattern)
0:p[j]=1'bx;1:p[j]=1'bz;2:p[j]=j%2?1'bz:1'bx;
3:case(j%4)0:p[j]=0;1:p[j]=1;2:p[j]=1'bx;3:p[j]=1'bz;endcase
endcase end
return p;
endfunction
task row(input bit c,r,cv,route,v0,v1,t,input logic[63:0]a=0,b=0,input bit four=0);
bit pop,move,room,selected,ready[0:2];logic[63:0]out,chosen;integer physical,birth;
pop=co>0&&t;room=co<2||pop;selected=count[0]>0?qin[0][0][0]:0;
move=count[0]>0&&room&&count[1+selected]>0;
ready[0]=count[0]<2||move;ready[1]=count[1]<2||(move&&!selected);ready[2]=count[2]<2||(move&&selected);
out=co>0?qout[0]:64'd0;
pyc_7079635f727374=r;control_valid=cv;control=route;lane0_valid=v0;lane1_valid=v1;take=t;lane0_data=a;lane1_data=b;#1;
if(result!=={ready[0],ready[1],ready[2],(co>0),out})$fatal(1,"select old four-slot model row%0d",row_index);
if(four)$display("FOUR %0d %0d %0d %0d %0d %b",row_index,result[67],result[66],result[65],result[64],result[63:0]);
else $display("WORK %0d %0d %0d %0d %0d %b",row_index,result[67],result[66],result[65],result[64],result[63:0]);
if(c&&!last)begin
 if(r)begin
  physical=count[0]+count[1]+count[2]+co;fullReset=fullReset+(physical==8);
  for(integer i=0;i<3;i=i+1)begin dropped[i]=dropped[i]+tail[i]-head[i];head[i]=0;tail[i]=0;count[i]=0;end
  for(integer i=jh;i<jt;i=i+1)begin dropped[0]=dropped[0]+1;dropped[1+joined_lane[i]]=dropped[1+joined_lane[i]]+1;end
  jh=0;jt=0;co=0;edge_index=0;
 end else begin
  blocked=blocked+(co==2&&!t);hol=hol+(count[0]>0&&count[1+selected]==0&&count[2-selected]==2);
  replacements=replacements+(move&&count[0]==2&&count[1+selected]==2&&co==2&&t);
  if(result[64]&&t)begin
   if(jh==jt||result[63:0]!==joined[jh]||edge_index<joined_birth[jh]+2)$fatal(1,"actual selected retirement/order/latency ledger");
   retired[joined_lane[jh]]=retired[joined_lane[jh]]+1;jh=jh+1;
  end
  if(move)begin
   if(head[0]==tail[0]||head[1+selected]==tail[1+selected]||admitted[0][head[0]][0]!==selected||admitted[1+selected][head[1+selected]]!==qin[1+selected][0])$fatal(1,"independent accepted control/lane join ledger");
   joined[jt]=admitted[1+selected][head[1+selected]];joined_lane[jt]=selected;
   birth=born[0][head[0]]>born[1+selected][head[1+selected]]?born[0][head[0]]:born[1+selected][head[1+selected]];joined_birth[jt]=birth;jt=jt+1;
   head[0]=head[0]+1;head[1+selected]=head[1+selected]+1;
  end
  if(pop)begin qout[0]=qout[1];co=co-1;end
  if(move)begin chosen=qin[1+selected][0];qout[co]=chosen;co=co+1;qin[0][0]=qin[0][1];count[0]=count[0]-1;qin[1+selected][0]=qin[1+selected][1];count[1+selected]=count[1+selected]-1;end
  for(integer i=0;i<3;i=i+1)if(result[67-i]&&(i==0?cv:i==1?v0:v1))begin
   chosen=i==0?64'(route):i==1?a:b;
   admitted[i][tail[i]]=chosen;born[i][tail[i]]=edge_index;tail[i]=tail[i]+1;accepted[i]=accepted[i]+1;qin[i][count[i]]=chosen;count[i]=count[i]+1;
  end
  edge_index=edge_index+1;
 end
 physical=count[0]+count[1]+count[2]+co;if(physical>peak)peak=physical;
 for(integer i=0;i<3;i=i+1)if(count[i]>2||count[i]!=tail[i]-head[i])$fatal(1,"D2 input conservation");
 if(co>2||co!=jt-jh||physical>8)$fatal(1,"D2 output/eight-slot conservation");
end
last=c;pyc_7079635f636c6b=c;#1;row_index=row_index+1;
endtask
task edge_row(input bit cv=0,route=0,v0=0,v1=0,t=0,input logic[63:0]a=0,b=0,input bit r=0,four=0);
row(0,r,cv,route,v0,v1,t,a,b,four);row(1,r,cv,route,v0,v1,t,a,b,four);
endtask
task drain(input bit four);for(integer i=0;i<8;i=i+1)edge_row(0,0,0,0,1,0,0,0,four);endtask
task fill(input bit four);
edge_row(1,0,1,1,0,marker(10),marker(11),0,four);edge_row(1,1,1,1,0,marker(12),marker(13),0,four);
edge_row(1,0,1,0,0,marker(14),0,0,four);edge_row(1,1,0,1,0,0,marker(15),0,four);
if(count[0]+count[1]+count[2]+co!=8)$fatal(1,"all eight original slots");
endtask
task sequence_rows(input bit four);
integer n;logic[63:0]p,a,b;
row_index=0;co=0;jh=0;jt=0;edge_index=0;peak=0;replacements=0;blocked=0;hol=0;fullReset=0;last=0;
for(integer i=0;i<3;i=i+1)begin count[i]=0;head[i]=0;tail[i]=0;accepted[i]=0;dropped[i]=0;end
retired[0]=0;retired[1]=0;
row(0,1,0,0,0,0,0,0,0,four);edge_row(0,0,0,0,0,0,0,1,four);
edge_row(1,0,1,0,1,marker(0),0,0,four);edge_row(0,0,0,0,1,0,0,0,four);edge_row(0,0,0,0,1,0,0,0,four);drain(four);
edge_row(0,0,0,1,0,0,marker(1),0,four);edge_row(0,0,0,1,0,0,marker(2),0,four);
edge_row(1,0,0,0,0,0,0,0,four);edge_row(1,0,0,0,0,0,0,0,four);edge_row(1,1,0,0,0,0,0,0,four);
edge_row(0,1,1,0,1,marker(3),0,0,four);edge_row(0,1,0,0,1,0,0,0,four);edge_row(0,1,1,0,1,marker(4),0,0,four);edge_row(0,1,0,0,1,0,0,0,four);
edge_row(1,1,0,0,1,0,0,0,four);edge_row(1,1,0,0,1,0,0,0,four);drain(four);
fill(four);edge_row(1,0,1,1,0,marker(16),marker(17),0,four);
row(1,1,1,1,1,1,1,marker(18),marker(19),four);row(1,0,1,0,1,1,0,marker(20),marker(21),four);
for(integer i=0;i<12;i=i+1)edge_row(1,1'(i%2),1,1,1,marker(30+i),marker(50+i),0,four);
drain(four);edge_row(1,0,0,0,1,0,0,0,four);edge_row(1,1,0,0,1,0,0,0,four);drain(four);
fill(four);edge_row(0,0,0,0,0,0,0,1,four);
n=0;
for(integer i=0;i<(four?264:218);i=i+1)for(integer selected=0;selected<2;selected=selected+1)begin
p=four?raw_value(i):known_value(i);a=selected?marker(100+n):p;b=selected?p:marker(100+n);
edge_row(1,1'(selected),1,1,1,a,b,0,four);edge_row(1,!1'(selected),0,0,1,0,0,0,four);edge_row(0,0,0,0,1,0,0,0,four);edge_row(0,0,0,0,1,0,0,0,four);n=n+1;
end
fill(four);edge_row(0,0,0,0,1,0,0,1,four);drain(four);
if(row_index!=(four?4395:3659)||peak!=8||fullReset<2||blocked<2||hol<2||replacements<8||count[0]+count[1]+count[2]+co!=0)$fatal(1,"bounded coverage");
if(accepted[0]!=retired[0]+retired[1]+dropped[0]||accepted[1]!=retired[0]+dropped[1]||accepted[2]!=retired[1]+dropped[2])$fatal(1,"three independent accepted/retired/drop ledgers");
$display("HISTORY_RTL %0d %0d %0d %0d %0d %0d %0d %0d %0d %0d %0d %0d %0d",accepted[0],accepted[1],accepted[2],retired[0],retired[1],dropped[0],dropped[1],dropped[2],peak,replacements,blocked,hol,fullReset);
endtask
`ifdef PYC_SELECT_FOUR_STATE
task reset_dut;
control_valid=0;lane0_valid=0;lane1_valid=0;take=0;pyc_7079635f727374=1;pyc_7079635f636c6b=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
endtask
task safe_rows;
for(integer mode=0;mode<2;mode=mode+1)for(integer z=0;z<2;z=z+1)for(integer latent=0;latent<2;latent=latent+1)begin
reset_dut();
if(mode==0)begin
 control_valid=1;control=0;lane0_valid=1;lane0_data=marker(300);#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;lane0_data=marker(301);#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;control_valid=0;lane0_valid=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
end
control_valid=1;control=z?1'bz:1'bx;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;control_valid=0;control=0;#1;
if(result[67:65]!==3'b111)$fatal(1,"safe unknown selector independent capacity");
lane0_valid=1;lane1_valid=1;lane0_data=marker(302);lane1_data=marker(303);#1;pyc_7079635f636c6b=1;#1;
if(result[64]!==1'(mode==0))$fatal(1,"safe selector push lost/output appeared early");
if(result[63:0]!==((mode==0)?marker(300):64'd0))$fatal(1,"safe selector copied output changed");
$display("SAFE %0d %0d %0d masked selector and independent pushes passed",mode,z,latent);reset_dut();
end
endtask
`endif
initial begin
`ifdef PYC_SELECT_NEGATIVE
pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;
control_valid=1;control=`PYC_SELECT_ROUTE;lane0_valid=0;lane1_valid=0;lane0_data=marker(400);lane1_data=(`PYC_SELECT_LANES==4)?marker(400):marker(401);
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;control_valid=0;control=0;lane0_valid=(`PYC_SELECT_LANES!=2);lane1_valid=(`PYC_SELECT_LANES!=1);
// Both old lanes empty: these otherwise known input pushes must commit.
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;lane0_valid=0;lane1_valid=0;#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`else
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;sequence_rows(0);
`ifdef PYC_SELECT_FOUR_STATE
reset_dut();sequence_rows(1);safe_rows();
`endif
$finish;
`endif
end
endmodule
