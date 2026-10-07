module tb;
localparam integer WIDTH=28;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic [WIDTH-1:0] data='0;
wire [WIDTH+1:0] result;
wire ready=result[WIDTH+1],out_valid=result[WIDTH];wire [WIDTH-1:0] out_data=result[WIDTH-1:0];
pyc_root dut(.*);
bit first_valid=0,second_valid=0,last_clock=0;
logic [WIDTH-1:0] first_data='0,second_data='0,history[0:21999];
integer row_index=0,stalled=0,accepted=0,retired=0,dropped=0,outstanding=0,peak=0,replacements=0,head=0,tail=0;
integer history_birth[0:21999],edge_number=0;
// Independent scalar ternary fold, per-bit sum and endpoint scan.
function automatic logic[2:0]priority_index(input logic[7:0] value,input bit high);
logic[2:0] answer,choice;logic predicate;integer position;
answer=0;
for(integer step=0;step<8;step=step+1)begin
 position=high?step:7-step;predicate=value[position];choice=3'(position);
 if(predicate===1'b1)answer=choice;
 else if(predicate!==1'b0)for(integer b=0;b<3;b=b+1)
  if(answer[b]!==choice[b])answer[b]=1'bx;
end
return answer;
endfunction
function automatic logic[3:0]endpoint_count(input logic[7:0]value,input bit leading);
logic[3:0]answer;logic symbol;integer distance,uncertain,poison,index;bit endpoint;
answer=0;distance=8;uncertain=0;endpoint=0;
for(integer position=0;position<8&&distance==8;position=position+1)begin
 index=leading?7-position:position;symbol=value[index];
 if(symbol===1'b1)distance=position;
 else if(symbol!==1'b0)begin uncertain=uncertain+1;if(position==0)endpoint=1;end
end
if(uncertain==0)answer=4'(distance);
else begin poison=4;if(endpoint&&uncertain==1)begin poison=0;for(integer n=distance;n>0;n=n>>1)poison=poison+1;end
 for(integer b=0;b<poison;b=b+1)answer[b]=1'bx;end
return answer;
endfunction
function automatic logic[WIDTH-1:0]transformed(input logic[WIDTH-1:0]p);
logic[7:0]v;logic asserted,conflict;logic[3:0]count;bit uncertain;integer population;
v=p[27:20];population=0;uncertain=0;
for(integer b=0;b<8;b=b+1)begin
 if(v[b]===1'b1)population=population+1;
 else if(v[b]!==1'b0)uncertain=1;
end
asserted=population>0?1'b1:uncertain?1'bx:1'b0;
conflict=uncertain?1'bx:population>1;
count=uncertain?4'bxxxx:4'(population);
return{v,priority_index(v,0),priority_index(v,1),asserted,conflict,count,
       endpoint_count(v,1),endpoint_count(v,0)};
endfunction
localparam integer KNOWN_COUNT=21056,FOUR_COUNT=376;
function automatic logic[WIDTH-1:0]known_value(input integer index);
integer value,low,high,pop,leading,trailing,sweep,offset;
if(index<256)return WIDTH'((index<<20)|((index*7919+20'habcde)&20'hfffff));
if(index<576)begin offset=index-256;sweep=offset/64;low=(offset%64)/8;high=offset%8;end
else begin offset=index-576;sweep=offset/4096;pop=(offset%4096)/256;leading=(offset%256)/16;trailing=offset%16;end
case(sweep)0:value=0;1:value=1;2:value=128;3:value=129;4:value=255;default:$fatal(1,"known vector index");endcase
if(index<576)return WIDTH'((value<<20)|(low<<17)|(high<<14)|((low&1)<<13)|((high&1)<<12)|12'ha5b);
return WIDTH'((value<<20)|(5<<17)|(3<<14)|(((pop+leading)&1)<<13)|(((pop+trailing)&1)<<12)|(pop<<8)|(leading<<4)|trailing);
endfunction
`ifdef PYC_BIT_PRIMITIVE_PIPELINE_FOUR_STATE
function automatic logic[WIDTH-1:0]four_value(input integer index);
integer group,b,symbol,d,pattern,literal_index;bit leading,reverse;logic[WIDTH-1:0]p;logic[7:0]literal;
p=28'habcde;
if(index<96)begin group=index/32;b=(index%32)/4;symbol=(index%4)/2;
 case(group)0:p=WIDTH'((0<<20)|20'habcde);1:p=WIDTH'((255<<20)|20'habcde);2:p=WIDTH'((32'hab<<20)|20'habcde);endcase
 p[20+b]=symbol==0?1'bx:1'bz;
end else if(index<160)begin group=index-96;leading=group<32;d=(group%32)/4+1;symbol=(group%4)/2;
 p[leading?27:20]=symbol==0?1'bx:1'bz;if(d<8)p[leading?27-d:20+d]=1'b1;
end else if(index<208)begin group=index-160;literal_index=group/4;reverse=(group%4)/2;
 case(literal_index)
 0:literal=8'bx0000100;1:literal=8'bz0000100;2:literal=8'b000001x0;3:literal=8'b000001z0;
 4:literal=8'b1xxxxxxx;5:literal=8'b01zzzzzz;6:literal=8'bx1111111;7:literal=8'bx0111111;
 8:literal=8'b00x11111;9:literal=8'bxx111111;10:literal=8'b00000000;11:literal=8'b10101010;
 endcase
 for(integer j=0;j<8;j=j+1)p[20+j]=literal[reverse?7-j:j];
end else if(index<368)begin b=(index-208)/8;symbol=((index-208)%8)/2;
 p=WIDTH'(((b%2?255:0)<<20)|20'h55555);
 case(symbol)0:p[b]=1'b0;1:p[b]=1'b1;2:p[b]=1'bx;3:p[b]=1'bz;endcase
end else begin pattern=(index-368)/2;
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
 if(result!=={input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}}})$fatal(1,"bit_primitive_pipeline row %0d two-slot oracle",row_index);
 if(four)$display("FOUR %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 else $display("WORK %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 stalled=stalled+(valid_level&&!input_room);
 if(clock_level&&!last_clock)begin
  if(reset_level)begin dropped=dropped+outstanding;outstanding=0;head=0;tail=0;edge_number=0;first_valid=0;second_valid=0;first_data=0;second_data=0;end
  else begin
   was_full=outstanding==2;
   if(out_valid&&take_level)begin if(outstanding==0||out_data!==history[head]||edge_number<history_birth[head]+2)$fatal(1,"bit_primitive_pipeline commit history loss/order");head=head+1;retired=retired+1;outstanding=outstanding-1;end
   if(ready&&valid_level)begin history[tail]=transformed(p);history_birth[tail]=edge_number;tail=tail+1;accepted=accepted+1;outstanding=outstanding+1;if(was_full)replacements=replacements+1;end
   old_first=first_data;
   if(push_second)begin second_valid=1;second_data=transformed(old_first);end else if(pop_second)begin second_valid=0;second_data=0;end
   if(push_first)begin first_valid=1;first_data=p;end else if(pop_first)begin first_valid=0;first_data=0;end
  end
  if(!reset_level)edge_number=edge_number+1;
  if(outstanding>peak)peak=outstanding;if(outstanding<0||outstanding>2)$fatal(1,"history capacity");
 end
 last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;row_index=row_index+1;
 endtask
 task edge_row(input logic[WIDTH-1:0]p,input bit v=1,t=1,r=0,four=0);row(1,r,v,p,t,four);row(0,r,v,p,t,four);endtask
task sequence_rows(input bit four);
row_index=0;stalled=0;accepted=0;retired=0;dropped=0;outstanding=0;peak=0;replacements=0;head=0;tail=0;edge_number=0;first_valid=0;second_valid=0;first_data=0;second_data=0;last_clock=0;
row(0,1,0,0,0,four);edge_row(0,0,0,1,four);
// E0 capture, E1 transform, E2 retirement; zero output is still a token.
edge_row(28'hfffff,1,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);
for(integer i=0;i<5;i=i+1)edge_row(known_value(i),1,0,0,four);
row(1,0,1,known_value(5),0,four);row(1,0,1,known_value(6),1,four);row(0,0,1,known_value(7),1,four);row(0,0,1,known_value(8),1,four);
edge_row(known_value(9),1,1,0,four);edge_row(known_value(10),1,1,0,four);edge_row(0,1,0,1,four);
if(four)begin
`ifdef PYC_BIT_PRIMITIVE_PIPELINE_FOUR_STATE
edge_row({WIDTH{1'bx}},1,0,0,1);edge_row({WIDTH{1'bz}},1,0,0,1);edge_row(0,1,0,0,1);
row(1,0,1,{WIDTH{1'bx}},0,1);row(1,1,1,0,1,1);row(0,0,1,'1,1,1);row(0,0,0,0,0,1);
edge_row(known_value(0),1,1,0,1);edge_row(0,0,1,0,1);edge_row(0,0,1,0,1);
for(integer i=0;i<FOUR_COUNT;i=i+1)edge_row(four_value(i),1,1,0,1);
// Known recovery after uncertain payloads, including zero and count8.
edge_row(28'hfffff,1,1,0,1);edge_row(28'(255<<20),1,1,0,1);
`else
$fatal(1,"four-state sequence requires genuine four-state build");
`endif
end
else for(integer i=0;i<KNOWN_COUNT;i=i+1)edge_row(known_value(i));
edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(known_value(0),1,0,0,four);edge_row(known_value(1),1,0,0,four);edge_row(0,1,1,1,four);
if(row_index!=(four?813:42153))$fatal(1,"wrong finite sequence rows");
if(stalled<3||peak!=2||replacements<2||outstanding!=0||dropped<2||accepted!=retired+dropped)$fatal(1,"bit_primitive_pipeline finite history coverage");
if(four)$display("HISTORY_FOUR_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
else $display("HISTORY_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
endtask
initial begin
`ifdef PYC_BIT_PRIMITIVE_NEGATIVE
// Isolated effective-unknown control failure; this process must terminate.
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
valid=1;data=28'h8100000;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
`ifdef PYC_BIT_PRIMITIVE_NEGATIVE_POP
// Populate both owners before making the retiring control indeterminate.
data=28'h0400000;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;take=1'bx;
`else
// Old input can move; new input's effective push is unknown.
valid=1'bx;take=0;
`endif
#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`else
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(0);
`ifdef PYC_BIT_PRIMITIVE_PIPELINE_FOUR_STATE
pyc_7079635f727374=1;valid=0;take=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(1);
`endif
$finish;
`endif
end
endmodule
