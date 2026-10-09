module tb;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;logic[7:0]data=0;wire[9:0]result;pyc_root dut(.*);
logic[7:0]source[0:1],output_data=0,images[0:1],independent[0:1],admitted[0:4095],snapshots[0:4095];
integer count=0,ih=0,it=0,oh=0,ot=0,birth[0:4095],snapshot_birth[0:4095],edge_index=0,row_index=0;
integer accepted=0,retired=0,dropped=0,peak=0,blocked=0,replacements=0,fullReset=0;bit ov=0,last=0;
function automatic logic[7:0]entry(input integer index=0,value=0);return{1'(index),7'(value)};endfunction
function automatic logic[7:0]old(input logic[7:0]a,b,p);if(p[7]===0)return a;if(p[7]===1)return b;return 8'bxxxxxxxx;endfunction
task update_rows(input logic[7:0]p,inout logic[7:0]a,b);
if(p[7]===0)a=p;else if(p[7]===1)b=p;else begin a=(p[7]==0)?p:a;b=(p[7]==1)?p:b;end
endtask
function automatic logic[7:0]raw_value(input integer i);
integer index,b,pattern,symbol;logic[7:0]p;p=0;
if(i<56)begin index=i/28;b=(i%28)/4;symbol=(i%4)/2;p=entry(index,85);p[b]=symbol?1'bz:1'bx;end
else if(i<72)begin index=(i-56)/8;pattern=((i-56)%8)/2;p=entry(index);
for(integer j=0;j<7;j=j+1)case(pattern)0:p[j]=1'bx;1:p[j]=1'bz;2:p[j]=j%2?1'bz:1'bx;3:case(j%4)0:p[j]=0;1:p[j]=1;2:p[j]=1'bx;3:p[j]=1'bz;endcase endcase
end else if(i<88)begin symbol=(i-72)/8;case((i-72)%4)0:p=entry(0,0);1:p=entry(0,127);2:p=entry(0,42);3:p=entry(0,85);endcase p[7]=symbol?1'bz:1'bx;
end else if(i<104)begin symbol=(i-88)/8;pattern=(i-88)%4;p[7]=symbol?1'bz:1'bx;
for(integer j=0;j<7;j=j+1)case(pattern)0:p[j]=1'bx;1:p[j]=1'bz;2:p[j]=j%2?1'bz:1'bx;3:case(j%4)0:p[j]=0;1:p[j]=1;2:p[j]=1'bx;3:p[j]=1'bz;endcase endcase
end else begin p[6:0]=7'bzzzzzzz;p[7]=(i-104)/2?1'bz:1'bx;end
return p;
endfunction
task row(input bit c,r,v,t,input logic[7:0]p=0,input bit four=0);
bit pop,room,move,ready;logic[7:0]prior;integer physical;
pop=ov&&t;room=!ov||pop;move=count>0&&room;ready=count<2||move;
pyc_7079635f727374=r;valid=v;take=t;data=p;#1;
if(result!=={ready,ov,ov?output_data:8'd0})$fatal(1,"table_rule old-slot row%0d",row_index);
if(four)$display("FOUR %0d %0d %0d %b",row_index,result[9],result[8],result[7:0]);else $display("WORK %0d %0d %0d %b",row_index,result[9],result[8],result[7:0]);
if(c&&!last)begin
 if(r)begin fullReset=fullReset+(count+ov==3);dropped=dropped+(it-ih)+(ot-oh);count=0;ov=0;output_data=0;ih=0;it=0;oh=0;ot=0;edge_index=0;images[0]=0;images[1]=0;independent[0]=0;independent[1]=0;end
 else begin
  blocked=blocked+(ov&&!t);replacements=replacements+(move&&count==2&&ov&&t&&v);
  if(result[8]&&t)begin if(oh==ot||result[7:0]!==snapshots[oh]||edge_index<snapshot_birth[oh]+2)$fatal(1,"actual old-Q snapshot retirement ledger");oh=oh+1;retired=retired+1;end
  if(move)begin
   if(ih==it||source[0]!==admitted[ih])$fatal(1,"independent input install order");
   snapshots[ot]=old(independent[0],independent[1],admitted[ih]);snapshot_birth[ot]=birth[ih];ot=ot+1;
   update_rows(admitted[ih],independent[0],independent[1]);ih=ih+1;
  end
  if(result[9]&&v)begin admitted[it]=p;birth[it]=edge_index;it=it+1;accepted=accepted+1;end
  if(pop)begin ov=0;output_data=0;end
  if(move)begin prior=old(images[0],images[1],source[0]);update_rows(source[0],images[0],images[1]);output_data=prior;ov=1;source[0]=source[1];count=count-1;end
  if(v&&ready)begin source[count]=p;count=count+1;end
  edge_index=edge_index+1;
 end
 physical=count+ov;if(physical>peak)peak=physical;
 if(count>2||physical>3||count!=it-ih||int'(ov)!=ot-oh||images[0]!==independent[0]||images[1]!==independent[1])$fatal(1,"three-token/table old-state conservation");
end
last=c;pyc_7079635f636c6b=c;#1;row_index=row_index+1;
endtask
task edge_row(input logic[7:0]p=0,input bit v=0,t=1,r=0,four=0);row(0,r,v,t,p,four);row(1,r,v,t,p,four);endtask
task drain(input bit four);for(integer i=0;i<8;i=i+1)edge_row(0,0,1,0,four);if(count+ov!=0)$fatal(1,"finite table_rule drain");endtask
task fill(input bit four);edge_row(entry(0,50),1,0,0,four);edge_row(entry(1,60),1,0,0,four);edge_row(entry(0,70),1,0,0,four);if(count+ov!=3)$fatal(1,"three original queue slots");endtask
task sequence_rows(input bit four);
integer index;logic[7:0]p,a,b;
count=0;ov=0;last=0;ih=0;it=0;oh=0;ot=0;edge_index=0;row_index=0;accepted=0;retired=0;dropped=0;peak=0;blocked=0;replacements=0;fullReset=0;images[0]=0;images[1]=0;independent[0]=0;independent[1]=0;
row(0,1,0,0,0,four);edge_row(0,0,0,1,four);
edge_row(entry(0,10),1,1,0,four);edge_row(entry(0,20),1,1,0,four);edge_row(entry(1,30),1,1,0,four);edge_row(entry(0,40),1,1,0,four);drain(four);
fill(four);edge_row(entry(1,80),1,0,0,four);row(1,1,1,1,entry(1,90),four);row(1,0,1,0,entry(0,100),four);edge_row(entry(1,110),1,0,0,four);
for(integer i=0;i<12;i=i+1)edge_row(entry(i%2,(i*11)%128),1,1,0,four);drain(four);fill(four);edge_row(0,0,1,1,four);drain(four);
for(integer i=0;i<(four?108:256);i=i+1)begin
 p=four?raw_value(i):8'(i);
 if(p[7]!==0&&p[7]!==1)begin a=entry(0,42);b=entry(1,85);if(i>=104)begin a={1'b0,7'bzzzzzzz};b={1'b1,7'bzzzzzzz};end
 edge_row(a,1,1,0,four);edge_row(b,1,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);end
 index=(p[7]===1)?1:0;edge_row(p,1,1,0,four);edge_row(entry(index,3),1,1,0,four);edge_row(entry(index^1,5),1,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);
 if(count+ov!=0)$fatal(1,"known-index post-install row queries");
end
if(four)for(integer symbol=0;symbol<2;symbol=symbol+1)begin
edge_row(entry(0,10),1,1,0,four);edge_row(entry(1,20),1,1,0,four);drain(four);
edge_row(entry(0,30),1,0,0,four);edge_row(symbol?8'bz0110111:8'bx0110111,1,0,0,four);edge_row(entry(1,99),1,0,0,four);
edge_row(entry(0,11),1,0,0,four);edge_row(entry(1,12),1,0,0,four);row(1,0,1,0,entry(0,13),four);
for(integer i=0;i<4;i=i+1)edge_row(0,0,1,0,four);edge_row(entry(0,7),1,1,0,four);edge_row(entry(1,8),1,1,0,four);drain(four);
end
fill(four);edge_row(0,1,1,1,four);drain(four);
if(row_index!=(four?1613:2687)||count+ov!=0||accepted!=retired+dropped||peak!=3||fullReset<2||blocked<2||replacements<8)$fatal(1,"bounded table_rule coverage");
$display("HISTORY_RTL %0d %0d %0d %0d %0d %0d %0d",accepted,retired,dropped,peak,blocked,replacements,fullReset);
endtask
task reset_dut;valid=0;take=0;pyc_7079635f727374=1;pyc_7079635f636c6b=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;endtask
initial begin
`ifdef PYC_TABLE_RULE_NEGATIVE_POP
reset_dut();valid=1;data=entry(1,7);#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;valid=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;take=1'bx;#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`elsif PYC_TABLE_RULE_NEGATIVE_ACCEPT
reset_dut();valid=1'bx;#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`else
reset_dut();sequence_rows(0);
`ifdef PYC_TABLE_RULE_FOUR_STATE
reset_dut();sequence_rows(1);
`endif
$finish;
`endif
end
endmodule
