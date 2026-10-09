module tb;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;logic[32:0]data=0;wire[34:0]result;
wire ready=result[34],available=result[33];wire[32:0]out=result[32:0];pyc_root dut(.*);
logic[32:0]src[0:1],fanF,fanT,resF,resT,merged,accepted_data[0:2047];integer srcId[0:1],fId,tId,rfId,rtId,mId;bit alive[0:2047];
integer ns=0,nf=0,nt=0,nrf=0,nrt=0,nm=0,nextId=0,row_index=0,accepted=0,retired=0,dropped=0,live=0,peak=0,priority_grants=0,overtakes=0,masked=0,fullReset=0;bit last=0;
function automatic logic[32:0]item(input logic[31:0]v,input bit route);return{v,route};endfunction
function automatic logic[32:0]marker(input integer n,input bit route);return{32'h10203040+32'(n)*32'h01010101,route};endfunction
function automatic logic[32:0]transformed(input logic[32:0]p,input bit falseBranch);logic[31:0]v;if($isunknown(p[32:1]))v='x;else v=p[32:1]+(falseBranch?32'd20:32'd10);return{v,p[0]};endfunction
function automatic logic[32:0]known_value(input integer n);integer length;logic[31:0]v;
 if(n<66)begin length=n/2;v=length==32?32'hffffffff:(32'd1<<length)-1;end
 else case((n-66)/2)0:v=0;1:v=32'hffffffff;2:v=32'hfffffff0;3:v=32'h7fffffff;4:v=32'h80000000;5:v=32'h80000001;6:v=32'h55555555;7:v=32'haaaaaaaa;endcase
 return{v,1'(n%2)};
endfunction
function automatic logic[32:0]four_value(input integer n);integer b,pattern;logic[32:0]p;
 p={32'ha55aa55a,1'(n%2)};
 if(n<256)begin b=n/8;p[b+1]=(n%8)/4?1'bz:1'bx;end
 else begin pattern=(n-256)/4;for(integer j=0;j<32;j=j+1)case(pattern)
 0:p[j+1]=1'bx;1:p[j+1]=1'bz;2:p[j+1]=j%2?1'bz:1'bx;
 3:case(j%4)0:p[j+1]=0;1:p[j+1]=1;2:p[j+1]=1'bx;3:p[j+1]=1'bz;endcase
 endcase end
 return p;
endfunction
task row(input bit c,r,v,t,input logic[32:0]p,input bit four,input string tag="TRAFFIC");
 bit mr,pf,pt,rfr,rtr,pff,pft,rff,rtf,move,er,ev;integer selected;logic[32:0]ed,chosen,oldsrc,oldF,oldT;integer chosenId,oldsrcId,oldFid,oldTid;
 mr=nm<1||(nm>0&&t);pf=nrf>0&&mr;pt=nrt>0&&nrf==0&&mr;rfr=nrf<1||pf;rtr=nrt<1||pt;pff=nf>0&&rfr;pft=nt>0&&rtr;rff=nf<1||pff;rtf=nt<1||pft;move=0;selected=0;
 if(ns>0)begin
  if(src[0][0]===1'b1)begin selected=0;move=rff;end
  else if(src[0][0]===1'b0)begin selected=1;move=rtf;end
  else if(rff||rtf)$fatal(1,"unmasked unknown route in positive sequence");
 end
 er=ns<2||move;ev=nm>0;ed=ev?merged:33'd0;
 pyc_7079635f727374=r;valid=v;take=t;data=p;#1;
 if(result!=={er,ev,ed})$fatal(1,"conditional six-deque row%0d",row_index);
 if(four)$display("FOUR %0d %s %0d %0d %b",row_index,tag,ready,available,out);
 else $display("WORK %0d %s %0d %0d %b",row_index,tag,ready,available,out);
 if(c&&!last)begin
  if(r)begin fullReset=fullReset+(live==7);dropped=dropped+live;live=0;ns=0;nf=0;nt=0;nrf=0;nrt=0;nm=0;for(integer i=0;i<nextId;i=i+1)alive[i]=0;end
  else begin
   if(nrf>0&&nrt>0&&pf)priority_grants=priority_grants+1;
   if(ns>0&&(src[0][0]!==1'b0&&src[0][0]!==1'b1))masked=masked+1;
   if(available&&t)begin
    if(!alive[mId]||(accepted_data[mId][0]!==1'b0&&accepted_data[mId][0]!==1'b1)||out!==transformed(accepted_data[mId],accepted_data[mId][0]))$fatal(1,"actual DUT identity/selected branch retirement");
    for(integer i=0;i<mId;i=i+1)if(alive[i])overtakes=overtakes+1;
    alive[mId]=0;retired=retired+1;live=live-1;
   end
   if(ready&&v)begin accepted_data[nextId]=p;alive[nextId]=1;accepted=accepted+1;live=live+1;end
   oldsrc=src[0];oldsrcId=srcId[0];oldF=fanF;oldFid=fId;oldT=fanT;oldTid=tId;
   if(pf)begin chosen=resF;chosenId=rfId;end else if(pt)begin chosen=resT;chosenId=rtId;end
   if(ev&&t)nm=0;
   if(pf)nrf=0;if(pt)nrt=0;
   if(pff)begin nf=0;nrf=1;resF=transformed(oldF,1);rfId=oldFid;end
   if(pft)begin nt=0;nrt=1;resT=transformed(oldT,0);rtId=oldTid;end
   if(pf||pt)begin nm=1;merged=chosen;mId=chosenId;end
   if(move)begin src[0]=src[1];srcId[0]=srcId[1];ns=ns-1;if(selected==0)begin nf=1;fanF=oldsrc;fId=oldsrcId;end else begin nt=1;fanT=oldsrc;tId=oldsrcId;end end
   if(v&&er)begin src[ns]=p;srcId[ns]=nextId;ns=ns+1;nextId=nextId+1;end
   if(live>peak)peak=live;
   if(live!=ns+nf+nt+nrf+nrt+nm||live>7||ns>2)$fatal(1,"physical seven-slot identity conservation");
  end
 end
 last=c;pyc_7079635f636c6b=c;#1;row_index=row_index+1;
endtask
task edge_row(input logic[32:0]p=0,input bit v=0,t=1,r=0,four=0,input string tag="TRAFFIC");row(0,r,v,t,p,four,tag);row(1,r,v,t,p,four,tag);endtask
task sequence_rows(input bit four);integer n;logic[32:0]x;
 ns=0;nf=0;nt=0;nrf=0;nrt=0;nm=0;nextId=0;row_index=0;accepted=0;retired=0;dropped=0;live=0;peak=0;priority_grants=0;overtakes=0;masked=0;fullReset=0;last=0;for(integer i=0;i<2048;i=i+1)alive[i]=0;
 row(0,1,0,0,0,four);edge_row(0,0,0,1,four);
 edge_row(item(7,0),1,1,0,four,"E0");edge_row(0,0,1,0,four,"E1");edge_row(0,0,1,0,four,"E2");edge_row(0,0,1,0,four,"E3");edge_row(0,0,1,0,four,"E4");edge_row(0,0,1,0,four);
 edge_row(0,0,0,1,four);
 for(integer i=0;i<5;i=i+1)edge_row(marker(i,i%2==0),1,0,0,four);
 for(integer i=0;i<8;i=i+1)edge_row(0,0,0,0,four);
 if(ns!=0||live!=5)$fatal(1,"five downstream occupied contributors");
 row(1,1,1,0,marker(10,1),four,"HELD");row(0,1,1,0,marker(11,0),four,"FALL");
 for(integer i=0;i<12;i=i+1)edge_row(0,0,1,0,four,"PRIORITY");
 if(live!=0||priority_grants<2||overtakes<2)$fatal(1,"false-first priority and crossbranch overtaking");
 for(integer i=0;i<7;i=i+1)edge_row(marker(20+i,i%2==0),1,0,0,four);
 for(integer i=0;i<6;i=i+1)edge_row(0,0,0,0,four);if(live!=7)$fatal(1,"seven-slot full capacity");edge_row(marker(30,0),1,0,0,four,"REJECT");
 for(integer i=0;i<40;i=i+1)edge_row(marker(40+i,i%4==0||i%4==3),1,1,0,four);
 n=four?272:82;for(integer i=0;i<n;i=i+1)edge_row(four?four_value(i):known_value(i),1,1,0,four);
 for(integer i=0;i<12;i=i+1)edge_row(0,0,1,0,four);if(live!=0)$fatal(1,"finite drain");
 for(integer i=0;i<7;i=i+1)edge_row(marker(90+i,i%2==0),1,0,0,four);for(integer i=0;i<6;i=i+1)edge_row(0,0,0,0,four);if(live!=7)$fatal(1,"full reset occupancy");edge_row(0,1,1,1,four);
 for(integer i=0;i<5;i=i+1)edge_row(0,0,1,0,four);
 if(four)begin
  for(integer i=0;i<5;i=i+1)edge_row(marker(120+i,i%2==0),1,0,0,four);for(integer i=0;i<8;i=i+1)edge_row(0,0,0,0,four);if(ns!=0||live!=5)$fatal(1,"masked route setup");
  x={32'h31415926,1'bx};edge_row(x,1,0,0,four,"MASKED_ROUTE");for(integer i=0;i<3;i=i+1)edge_row(0,0,0,0,four,"MASKED_ROUTE");edge_row(marker(125,1),1,0,0,four,"MASKED_ROUTE");if(live!=7||masked<3)$fatal(1,"unknown route must be masked when both destinations blocked");edge_row(0,0,0,1,four,"MASKED_RESET");for(integer i=0;i<5;i=i+1)edge_row(0,0,1,0,four);
 end
 if(row_index!=(four?831:403)||live!=0||accepted!=retired+dropped||peak!=7||fullReset<1||priority_grants<2||overtakes<2)$fatal(1,"finite conditional coverage/conservation");
 $display("HISTORY_RTL %0d %0d %0d %0d %0d %0d %0d %0d",accepted,retired,dropped,live,peak,priority_grants,overtakes,masked);
endtask
initial begin
`ifdef PYC_CONDITIONAL_NEGATIVE
 #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
 if(`PYC_CONDITIONAL_NEGATIVE==1)begin
  for(integer i=0;i<5;i=i+1)begin valid=1;take=0;data=marker(200+i,i%2==0);#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;end
  valid=0;repeat(8)begin #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;end
 end
 valid=1;take=0;data={32'h31415926,1'bx};#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;valid=0;#1;
 if(`PYC_CONDITIONAL_NEGATIVE==1)repeat(2)begin #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;end
 take=`PYC_CONDITIONAL_NEGATIVE==1;#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`else
`ifdef PYC_CONDITIONAL_FOUR_STATE
 #1;if(result!==35'bx)$fatal(1,"current contract cold X");
`endif
 #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;sequence_rows(0);
`ifdef PYC_CONDITIONAL_FOUR_STATE
 pyc_7079635f727374=1;valid=0;take=0;#1;pyc_7079635f636c6b=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;sequence_rows(1);
`endif
 $finish;
`endif
end
endmodule
