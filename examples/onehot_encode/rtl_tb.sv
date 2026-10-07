module tb;
localparam integer WIDTH=13;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic [WIDTH-1:0] data='0;
wire [WIDTH+1:0] result;
wire ready=result[WIDTH+1],out_valid=result[WIDTH];wire [WIDTH-1:0] out_data=result[WIDTH-1:0];
pyc_root dut(.*);
bit first_valid=0,second_valid=0,last_clock=0;
logic [WIDTH-1:0] first_data='0,second_data='0,history[0:9999];
integer birth[0:9999],edge_index=0;
integer row_index=0,stalled=0,accepted=0,retired=0,dropped=0,outstanding=0,peak=0,replacements=0,head=0,tail=0;
// Independent per-position scalar ternary equation; no compact mask/scan.
function automatic logic[WIDTH-1:0] transformed(input logic[WIDTH-1:0] p);
logic[2:0] index;logic payload_valid,conflict;integer asserted;bit uncertain;
index=0;payload_valid=0;asserted=0;uncertain=0;
for(integer position=7;position>=0;position=position-1)begin
 index=p[5+position]?3'(position):index;
 payload_valid=payload_valid|p[5+position];
 if(p[5+position]===1'b1)asserted=asserted+1;
 else if(p[5+position]!==1'b0)uncertain=1;
end
conflict=uncertain?1'bx:(asserted>1);
return{p[12:5],index,payload_valid,conflict};
endfunction
localparam integer KNOWN_COUNT=8192,FOUR_COUNT=248;
function automatic logic[WIDTH-1:0]known_value(input integer index);
// Exhaustive flags × old index/valid/conflict; declaration/MSB-first packing.
return WIDTH'(index);
endfunction
`ifdef PYC_ONEHOT_ENCODE_FOUR_STATE
function automatic logic[WIDTH-1:0]four_value(input integer index);
integer group,b,symbol,pattern,literal_index;logic[WIDTH-1:0]p;logic[7:0]literal;
p=13'h15;
if(index<96)begin group=index/32;b=(index%32)/4;symbol=(index%4)/2;
 case(group)0:p=WIDTH'((0<<5)|5'h15);1:p=WIDTH'((255<<5)|5'h15);2:p=WIDTH'((32'ha5<<5)|5'h15);endcase
 p[5+b]=symbol==0?1'bx:1'bz;
end else if(index<120)begin literal_index=(index-96)/2;
 case(literal_index)
 0:literal=8'bx0000100;1:literal=8'bz0000100;2:literal=8'b1xxxxxxx;
 3:literal=8'b01zzzzzz;4:literal=8'b0000010x;5:literal=8'b0000010z;
 6:literal=8'bxxxxxxxx;7:literal=8'bzzzzzzzz;8:literal=8'bxzxzxzxz;
 9:literal=8'b001x010z;10:literal=8'b00000000;11:literal=8'b11111111;
 endcase p={literal,5'b10101};
end else if(index<240)begin group=(index-120)/40;b=((index-120)%40)/8;symbol=(((index-120)%8)/2);
 case(group)0:p=WIDTH'((0<<5)|5'h15);1:p=WIDTH'((255<<5)|5'h15);2:p=WIDTH'((32'h84<<5)|5'h15);endcase
 case(symbol)0:p[b]=1'b0;1:p[b]=1'b1;2:p[b]=1'bx;3:p[b]=1'bz;endcase
end else begin pattern=(index-240)/2;
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
 if(result!=={input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}}})$fatal(1,"onehot_encode row %0d two-slot oracle",row_index);
 if(four)$display("FOUR %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 else $display("WORK %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 stalled=stalled+(valid_level&&!input_room);
 if(clock_level&&!last_clock)begin
  if(reset_level)begin dropped=dropped+outstanding;outstanding=0;head=0;tail=0;edge_index=0;first_valid=0;second_valid=0;first_data=0;second_data=0;end
  else begin
   was_full=outstanding==2;
   if(out_valid&&take_level)begin if(outstanding==0||out_data!==history[head]||edge_index<birth[head]+2)$fatal(1,"onehot_encode commit history loss/order");head=head+1;retired=retired+1;outstanding=outstanding-1;end
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
edge_row(13'd31,1,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);
for(integer i=0;i<5;i=i+1)edge_row(known_value(i),1,0,0,four);
row(1,0,1,known_value(5),0,four);row(1,0,1,known_value(6),1,four);row(0,0,1,known_value(7),1,four);row(0,0,1,known_value(8),1,four);
edge_row(known_value(9),1,1,0,four);edge_row(known_value(10),1,1,0,four);edge_row(0,1,0,1,four);
if(four)begin
`ifdef PYC_ONEHOT_ENCODE_FOUR_STATE
edge_row({WIDTH{1'bx}},1,0,0,1);edge_row({WIDTH{1'bz}},1,0,0,1);edge_row(0,1,0,0,1);
row(1,0,1,{WIDTH{1'bx}},0,1);row(1,1,1,0,1,1);row(0,0,1,'1,1,1);row(0,0,0,0,0,1);
edge_row(known_value(0),1,1,0,1);edge_row(0,0,1,0,1);edge_row(0,0,1,0,1);
for(integer i=0;i<FOUR_COUNT;i=i+1)edge_row(four_value(i),1,1,0,1);
// Known recovery after uncertain payloads, including zero and allones.
edge_row(13'd31,1,1,0,1);edge_row(13'(255<<5),1,1,0,1);
`else
$fatal(1,"four-state sequence requires genuine four-state build");
`endif
end
else for(integer i=0;i<KNOWN_COUNT;i=i+1)edge_row(known_value(i));
edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(known_value(0),1,0,0,four);edge_row(known_value(1),1,0,0,four);edge_row(0,1,1,1,four);
if(row_index!=(four?557:16425))$fatal(1,"wrong finite sequence rows");
if(stalled<3||peak!=2||replacements<2||outstanding!=0||dropped<2||accepted!=retired+dropped)$fatal(1,"onehot_encode finite history coverage");
if(four)$display("HISTORY_FOUR_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
else $display("HISTORY_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
endtask
initial begin
`ifdef PYC_ONEHOT_ENCODE_NEGATIVE
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
valid=1'bx;#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`else
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(0);
`ifdef PYC_ONEHOT_ENCODE_FOUR_STATE
pyc_7079635f727374=1;valid=0;take=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(1);
`endif
$finish;
`endif
end
endmodule
