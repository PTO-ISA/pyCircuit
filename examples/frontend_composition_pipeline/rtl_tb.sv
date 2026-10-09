module tb;
localparam integer WIDTH=34;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic [WIDTH-1:0] data='0;
wire [WIDTH+1:0] result;
wire ready=result[WIDTH+1],out_valid=result[WIDTH];wire [WIDTH-1:0] out_data=result[WIDTH-1:0];
pyc_root dut(.*);
bit first_valid=0,second_valid=0,last_clock=0;
logic [WIDTH-1:0] first_data='0,second_data='0,history[0:11999];
integer birth[0:11999],edge_index=0;
integer row_index=0,stalled=0,accepted=0,retired=0,dropped=0,outstanding=0,peak=0,replacements=0,head=0,tail=0;
// Independent scalar ternary, OR and count-status equations, plus explicit
// MSB declaration packing; no DUT encoder or record-update lowering is used.
function automatic logic[WIDTH-1:0] transformed(input logic[WIDTH-1:0] p);
logic[1:0] index;logic payload_valid,conflict;integer asserted;bit uncertain;
index=0;payload_valid=0;asserted=0;uncertain=0;
for(integer position=3;position>=0;position=position-1)begin
 index=p[13+position]?2'(position):index;
 payload_valid=payload_valid|p[13+position];
 if(p[13+position]===1'b1)asserted=asserted+1;
 else if(p[13+position]!==1'b0)uncertain=1;
end
conflict=uncertain?1'bx:(asserted>1);
return{p[33:13],p[25:21],4'd9,index,payload_valid,conflict};
endfunction
function automatic logic[WIDTH-1:0] packed_value(input integer header_opcode,header_tag,patch_tag,patch_valid,opcode,flags,old);
return {4'(header_opcode),4'(header_tag),4'(patch_tag),1'(patch_valid),4'(opcode),4'(flags),13'(old)};
endfunction
localparam integer KNOWN_COUNT=9120,FOUR_COUNT=596;
function automatic logic[WIDTH-1:0]known_value(input integer i);
integer flags,opcode,tag,patch_valid,slot,low,width,old,header;logic[WIDTH-1:0]p,mask;
if(i<8192)begin
 flags=i/512;opcode=(i/32)%16;tag=(i/2)%16;patch_valid=i%2;
 return packed_value((flags*3+opcode)%16,(tag+patch_valid*7)%16,tag,patch_valid,opcode,flags,(i*73+32'h152b)%8192);
end else if(i<8864)begin
 flags=(i-8192)/42;slot=(i-8192)%42;
 if(slot<16)begin low=9;width=4;old=slot;end
 else if(slot<18)begin low=8;width=1;old=slot-16;end
 else if(slot<34)begin low=4;width=4;old=slot-18;end
 else if(slot<38)begin low=2;width=2;old=slot-34;end
 else if(slot<40)begin low=1;width=1;old=slot-38;end
 else begin low=0;width=1;old=slot-40;end
 p=packed_value((flags+3)%16,(flags*5)%16,flags,flags%2,(flags+7)%16,flags,32'h152b);
 mask=((WIDTH'(1)<<width)-1)<<low;
 return (p&~mask)|(WIDTH'(old)<<low);
end else begin
 header=i-8864;
 return packed_value(header/16,header%16,(header*7)%16,header%2,(header*3)%16,header%16,(header*29)%8192);
end
endfunction
`ifdef PYC_FRONTEND_COMPOSITION_FOUR_STATE
function automatic logic[WIDTH-1:0]four_value(input integer i);
integer group,b,symbol,pattern,literal_index,flags;logic[WIDTH-1:0]p;logic[3:0]literal;
p=packed_value(10,5,7,1,9,0,32'h152b);
if(i<252)begin group=i/84;b=13+(i%84)/4;symbol=(i%4)/2;
 case(group)0:flags=0;1:flags=15;2:flags=10;endcase
 p=packed_value(10,5,7,1,9,flags,32'h152b);
 p[b]=symbol==0?1'bx:1'bz;
end else if(i<564)begin group=(i-252)/104;b=((i-252)%104)/8;symbol=((i-252)%8)/2;
 case(group)0:flags=0;1:flags=15;2:flags=10;endcase
 p=packed_value(10,5,7,1,9,flags,32'h152b);
 case(symbol)0:p[b]=1'b0;1:p[b]=1'b1;2:p[b]=1'bx;3:p[b]=1'bz;endcase
end else if(i<588)begin literal_index=(i-564)/2;
 case(literal_index)
 0:literal=4'bx001;1:literal=4'bz001;2:literal=4'b1xxx;
 3:literal=4'b01zz;4:literal=4'b001x;5:literal=4'b001z;
 6:literal=4'bxxxx;7:literal=4'bzzzz;8:literal=4'bxzxz;
 9:literal=4'bx10z;10:literal=4'b0000;11:literal=4'b1111;
 endcase p[16:13]=literal;
end else begin pattern=(i-588)/2;
for(integer j=0;j<WIDTH;j=j+1)case(pattern)
0:p[WIDTH-1-j]=1'bx;1:p[WIDTH-1-j]=1'bz;2:p[WIDTH-1-j]=j%2?1'bz:1'bx;
3:case(j%4)0:p[WIDTH-1-j]=0;1:p[WIDTH-1-j]=1;2:p[WIDTH-1-j]=1'bx;3:p[WIDTH-1-j]=1'bz;endcase
endcase end
return p;
endfunction
`endif

 task row(input bit clock_level,reset_level,valid_level,input logic[WIDTH-1:0] p,input bit take_level,input bit four=0);
 bit pop_second,room,pop_first,input_room,push_first,push_second,was_full;logic[WIDTH-1:0] old_first;
 pop_second=second_valid&&take_level;room=!second_valid||pop_second;pop_first=first_valid&&room;input_room=!first_valid||pop_first;push_first=valid_level&&input_room;push_second=first_valid&&room;
 pyc_7079635f727374=reset_level;valid=valid_level;data=p;take=take_level;#1;
 if(result!=={input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}}})$fatal(1,"frontend_composition_pipeline row %0d two-slot oracle",row_index);
 if(four)$display("FOUR %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 else $display("WORK %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 stalled=stalled+(valid_level&&!input_room);
 if(clock_level&&!last_clock)begin
  if(reset_level)begin dropped=dropped+outstanding;outstanding=0;head=0;tail=0;edge_index=0;first_valid=0;second_valid=0;first_data=0;second_data=0;end
  else begin
   was_full=outstanding==2;
   if(out_valid&&take_level)begin if(outstanding==0||out_data!==history[head]||edge_index<birth[head]+2)$fatal(1,"frontend_composition_pipeline commit history loss/order");head=head+1;retired=retired+1;outstanding=outstanding-1;end
   if(ready&&valid_level)begin history[tail]=transformed(p);birth[tail]=edge_index;tail=tail+1;accepted=accepted+1;outstanding=outstanding+1;if(was_full)replacements=replacements+1;end
   old_first=first_data;
   if(push_second)begin second_valid=1;second_data=transformed(old_first);end else if(pop_second)begin second_valid=0;second_data=0;end
   if(push_first)begin first_valid=1;first_data=p;end else if(pop_first)begin first_valid=0;first_data=0;end
   edge_index=edge_index+1;
  end
  if(outstanding>peak)peak=outstanding;if(outstanding<0||outstanding>2)$fatal(1,"history capacity");
 end
 last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;row_index=row_index+1;
 endtask
 task edge_row(input logic[WIDTH-1:0]p,input bit v=1,t=1,r=0,four=0);row(1,r,v,p,t,four);row(0,r,v,p,t,four);endtask
task sequence_rows(input bit four);
row_index=0;stalled=0;accepted=0;retired=0;dropped=0;outstanding=0;peak=0;replacements=0;head=0;tail=0;edge_index=0;first_valid=0;second_valid=0;first_data=0;second_data=0;last_clock=0;
row(0,1,0,0,0,four);edge_row(0,0,0,1,four);
// E0 capture, E1 transform, E2 retirement; zero flags are still a token.
edge_row(34'd31,1,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);
for(integer i=0;i<5;i=i+1)edge_row(known_value(i),1,0,0,four);
row(1,0,1,known_value(5),0,four);row(1,0,1,known_value(6),1,four);row(0,0,1,known_value(7),1,four);row(0,0,1,known_value(8),1,four);
edge_row(known_value(9),1,1,0,four);edge_row(known_value(10),1,1,0,four);edge_row(0,1,0,1,four);
if(four)begin
`ifdef PYC_FRONTEND_COMPOSITION_FOUR_STATE
edge_row({WIDTH{1'bx}},1,0,0,1);edge_row({WIDTH{1'bz}},1,0,0,1);edge_row(0,1,0,0,1);
row(1,0,1,{WIDTH{1'bx}},0,1);row(1,1,1,0,1,1);row(0,0,1,'1,1,1);row(0,0,0,0,0,1);
edge_row(known_value(0),1,1,0,1);edge_row(0,0,1,0,1);edge_row(0,0,1,0,1);
for(integer i=0;i<FOUR_COUNT;i=i+1)edge_row(four_value(i),1,1,0,1);
// Known recovery after uncertain payloads, including zero and allones.
edge_row(34'd31,1,1,0,1);edge_row(34'(15<<13),1,1,0,1);
`else
$fatal(1,"four-state sequence requires genuine four-state build");
`endif
end
else for(integer i=0;i<KNOWN_COUNT;i=i+1)edge_row(known_value(i));
edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(known_value(0),1,0,0,four);edge_row(known_value(1),1,0,0,four);edge_row(0,1,1,1,four);
if(row_index!=(four?1253:18281))$fatal(1,"wrong finite sequence rows");
if(stalled<3||peak!=2||replacements<2||outstanding!=0||dropped<2||accepted!=retired+dropped)$fatal(1,"frontend_composition_pipeline finite history coverage");
if(four)$display("HISTORY_FOUR_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
else $display("HISTORY_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
endtask

initial begin
`ifdef PYC_FRONTEND_COMPOSITION_NEGATIVE
// Isolated effective-unknown control failure; this process must terminate.
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
valid=1;data=packed_value(10,5,7,1,9,9,32'h152b);#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
`ifdef PYC_FRONTEND_COMPOSITION_NEGATIVE_POP
// Populate both owners before making the retiring control indeterminate.
data=packed_value(3,12,2,0,6,4,32'h1234);#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;take=1'bx;
`else
// Old input can move; new input's effective push is unknown.
valid=1'bx;take=0;
`endif
#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`else
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(0);
`ifdef PYC_FRONTEND_COMPOSITION_FOUR_STATE
pyc_7079635f727374=1;valid=0;take=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(1);
`endif
$finish;
`endif
end
endmodule
