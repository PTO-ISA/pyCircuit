module tb;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,left_valid=0,right_valid=0,left_take=0,right_take=0;
logic[15:0]left_data=0;logic[31:0]right_data=0;wire[51:0]result;
wire lr=result[51],lv=result[50],rr=result[33],rv=result[32];wire[15:0]ld=result[49:34];wire[31:0]rd=result[31:0];
pyc_root dut(.*);
logic[15:0]lin[0:1],lout[0:1],lhistory[0:2047];logic[31:0]rin[0:1],rout[0:1],rhistory[0:2047];
integer li=0,ri=0,lo=0,ro=0,row_index=0,la=0,ra=0,lp=0,rp=0,lDrop=0,rDrop=0,ln=0,rn=0,lh=0,rh=0,ltail=0,rtail=0,lpeak=0,rpeak=0,peak=0,pairs=0,blockedL=0,blockedR=0,indL=0,indR=0,replacement=0,fullReset=0;
bit last=0;
function automatic logic[15:0]known_left(input integer n);return 16'(16'ha55a^(n*257));endfunction
function automatic logic[31:0]known_right(input integer n);return 32'h89abcdef^(32'(n)*32'h01010101);endfunction
function automatic logic[31:0]raw_value(input integer index,input integer width);
 integer b,pattern;logic[31:0]p;
 p=32'ha55aa55a;
 if(index<4*width)begin b=index/4;p[b]=(index%4)/2?1'bz:1'bx;end
 else begin
 pattern=(index-4*width)/2;
 for(integer j=0;j<width;j=j+1)case(pattern)
 0:p[j]=1'bx;1:p[j]=1'bz;2:p[j]=j%2?1'bz:1'bx;
 3:case(j%4)0:p[j]=0;1:p[j]=1;2:p[j]=1'bx;3:p[j]=1'bz;endcase
 endcase
 end
 return p;
endfunction
task row(input bit c,r,a,b,t,u,input logic[15:0]x,input logic[31:0]y,input bit four);
 bit popL,popR,move,readyL,readyR,wasFull;logic[15:0]headL;logic[31:0]headR;
 popL=lo>0&&t;popR=ro>0&&u;move=li>0&&ri>0&&(lo<2||popL)&&(ro<2||popR);readyL=li<2||move;readyR=ri<2||move;
 headL=lo>0?lout[0]:16'd0;headR=ro>0?rout[0]:32'd0;
 pyc_7079635f727374=r;left_valid=a;right_valid=b;left_take=t;right_take=u;left_data=x;right_data=y;#1;
 if(result!=={readyL,(lo>0),headL,readyR,(ro>0),headR})$fatal(1,"barrier old-state four-deque row%0d",row_index);
 if(four)$display("FOUR %0d %0d %0d %b %0d %0d %b",row_index,lr,lv,ld,rr,rv,rd);
 else $display("WORK %0d %0d %0d %b %0d %0d %b",row_index,lr,lv,ld,rr,rv,rd);
 if(c&&!last)begin
  if(r)begin
   fullReset=fullReset+(ln+rn==8);lDrop=lDrop+ln;rDrop=rDrop+rn;ln=0;rn=0;lh=0;rh=0;ltail=0;rtail=0;li=0;ri=0;lo=0;ro=0;
  end else begin
   blockedL=blockedL+(lo==2&&!t);blockedR=blockedR+(ro==2&&!u);
   indL=indL+(lv&&t&&!(rv&&u));indR=indR+(rv&&u&&!(lv&&t));replacement=replacement+(move&&li==2&&ri==2&&a&&b);
   if(lv&&t)begin if(ln==0||ld!==lhistory[lh])$fatal(1,"actual left accepted/retired ledger");lh=lh+1;lp=lp+1;ln=ln-1;end
   if(rv&&u)begin if(rn==0||rd!==rhistory[rh])$fatal(1,"actual right accepted/retired ledger");rh=rh+1;rp=rp+1;rn=rn-1;end
   if(lr&&a)begin lhistory[ltail]=x;ltail=ltail+1;la=la+1;ln=ln+1;end
   if(rr&&b)begin rhistory[rtail]=y;rtail=rtail+1;ra=ra+1;rn=rn+1;end
   if(popL)begin lout[0]=lout[1];lo=lo-1;end
   if(popR)begin rout[0]=rout[1];ro=ro-1;end
   if(move)begin
    lout[lo]=lin[0];rout[ro]=rin[0];lo=lo+1;ro=ro+1;
    lin[0]=lin[1];rin[0]=rin[1];li=li-1;ri=ri-1;pairs=pairs+1;
   end
   if(a&&readyL)begin lin[li]=x;li=li+1;end
   if(b&&readyR)begin rin[ri]=y;ri=ri+1;end
   if(ln>lpeak)lpeak=ln;if(rn>rpeak)rpeak=rn;
   if(li>2||ri>2||lo>2||ro>2||ln>4||rn>4||ln!=li+lo||rn!=ri+ro)$fatal(1,"four D2 queue capacity and stream ledgers");
  end
  if(ln+rn>peak)peak=ln+rn;
 end
 last=c;pyc_7079635f636c6b=c;#1;row_index=row_index+1;
endtask
task edge_row(input logic[15:0]x,input logic[31:0]y,input bit a=1,b=1,t=1,u=1,r=0,four=0);
 row(0,r,a,b,t,u,x,y,four);row(1,r,a,b,t,u,x,y,four);
endtask
task finish_sequence;
 if(peak!=8||lpeak!=4||rpeak!=4||blockedL<3||blockedR<3||indL<2||indR<2||replacement<10||fullReset<2||ln!=0||rn!=0||la!=lp+lDrop||ra!=rp+rDrop)$fatal(1,"barrier finite coverage/conservation");
 $display("HISTORY_RTL 0 %0d %0d %0d %0d %0d",la,lp,lDrop,ln,lpeak);
 $display("HISTORY_RTL 1 %0d %0d %0d %0d %0d",ra,rp,rDrop,rn,rpeak);
 $display("PAIRS_RTL %0d PEAK %0d INDEPENDENT %0d %0d BLOCKED %0d %0d REPLACEMENT %0d FULL_RESET %0d",pairs,peak,indL,indR,blockedL,blockedR,replacement,fullReset);
endtask
task sequence_rows(input bit four);
 integer n;logic[15:0]x;logic[31:0]y;
 row_index=0;li=0;ri=0;lo=0;ro=0;la=0;ra=0;lp=0;rp=0;lDrop=0;rDrop=0;ln=0;rn=0;lh=0;rh=0;ltail=0;rtail=0;lpeak=0;rpeak=0;peak=0;pairs=0;blockedL=0;blockedR=0;indL=0;indR=0;replacement=0;fullReset=0;last=0;
 row(0,1,0,0,0,0,0,0,four);edge_row(0,0,0,0,0,0,1,four);
 for(integer i=0;i<3;i=i+1)edge_row(known_left(i),known_right(i),1,0,0,0,0,four);
 row(1,1,1,0,1,1,known_left(3),known_right(3),four);row(1,0,1,0,0,0,known_left(4),known_right(4),four);
 row(0,1,0,0,0,0,0,0,four);row(0,1,0,0,0,0,0,0,four);
 edge_row(known_left(0),known_right(0),0,1,0,0,0,four);edge_row(known_left(1),known_right(1),0,1,0,0,0,four);
 for(integer i=0;i<8;i=i+1)edge_row(known_left(5+i),known_right(5+i),1,1,1,1,0,four);
 for(integer i=0;i<6;i=i+1)edge_row(0,0,0,0,1,1,0,four);
 edge_row(0,0,0,0,0,0,1,four);
 for(integer i=0;i<4;i=i+1)edge_row(known_left(20+i),known_right(20+i),1,1,0,0,0,four);
 edge_row(known_left(24),known_right(24),1,1,0,0,0,four);
 for(integer i=0;i<3;i=i+1)edge_row(known_left(25+i),known_right(25+i),1,1,1,0,0,four);
 row(1,1,1,1,0,1,known_left(30),known_right(30),four);row(0,1,1,1,0,0,known_left(31),known_right(31),four);
 edge_row(known_left(32),known_right(32),1,1,0,1,0,four);
 for(integer i=0;i<24;i=i+1)edge_row(known_left(33+i),known_right(33+i),1,1,1,1,0,four);
 edge_row(known_left(58),known_right(58),1,1,0,1,0,four);edge_row(0,0,1,1,1,1,1,four);
 for(integer i=0;i<4;i=i+1)edge_row(known_left(60+i),known_right(60+i),1,1,0,0,0,four);
 for(integer i=0;i<3;i=i+1)edge_row(known_left(64+i),known_right(64+i),1,1,0,1,0,four);
 edge_row(known_left(68),known_right(68),1,1,1,0,0,four);
 for(integer i=0;i<8;i=i+1)edge_row(known_left(69+i),known_right(69+i),1,1,1,1,0,four);
 n=four?216:256;
 for(integer i=0;i<n;i=i+1)begin
  if(!four)begin x=known_left(i);y=known_right(i);end
  else if(i<72)begin x=16'(raw_value(i,16));y=known_right(i);end
  else if(i<208)begin x=known_left(i-72);y=raw_value(i-72,32);end
  else begin x=16'(raw_value(64+i-208,16));y=raw_value(128+7-(i-208),32);end
  edge_row(x,y,1,1,1,1,0,four);
 end
 for(integer i=0;i<8;i=i+1)edge_row(0,0,0,0,1,1,0,four);
 for(integer i=0;i<4;i=i+1)edge_row(known_left(80+i),known_right(80+i),1,1,0,0,0,four);
 edge_row(0,0,1,1,1,1,1,four);for(integer i=0;i<5;i=i+1)edge_row(0,0,0,0,1,1,0,four);
 if(row_index!=(four?619:699))$fatal(1,"bounded row count");finish_sequence();
endtask
initial begin
`ifdef PYC_BARRIER_NEGATIVE
 #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
 left_valid=1;right_valid=1;left_data=known_left(100);right_data=known_right(100);#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;left_valid=0;right_valid=0;#1;
 left_valid=1;right_valid=1'bx;left_data=known_left(101);right_data=known_right(101);#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`else
`ifdef PYC_BARRIER_FOUR_STATE
 #1;if(result!==52'bx)$fatal(1,"current-contract cold result must be X");
`endif
 #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;sequence_rows(0);
`ifdef PYC_BARRIER_FOUR_STATE
 pyc_7079635f727374=1;left_valid=0;right_valid=0;left_take=0;right_take=0;#1;pyc_7079635f636c6b=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;sequence_rows(1);
`endif
 $finish;
`endif
end
endmodule
