module tb;
localparam integer WIDTH=64, KNOWN_COUNT=202, FOUR_COUNT=264;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic [WIDTH-1:0] data='0;
wire [WIDTH+1:0] result;
wire ready=result[WIDTH+1],out_valid=result[WIDTH];wire [WIDTH-1:0] out_data=result[WIDTH-1:0];
pyc_root dut(.*);
logic[WIDTH-1:0] first_data[0:1],second_data[0:1],history[0:5999];
bit last_clock=0;
integer first_count=0,second_count=0,row_index=0,stalled=0,accepted=0,retired=0,dropped=0,outstanding=0,peak=0,replacements=0,reset_full=0,head=0,tail=0;
// Independent ripple, through exactly64bits; no carry or borrow crosses fields.
function automatic logic[63:0] arithmetic(input logic[63:0] value,input bit subtract);
logic[63:0] computed;bit known,carry;
known=1;carry=1;computed='0;
for(integer b=0;b<64;b=b+1)
 if(value[b]!==1'b0&&value[b]!==1'b1)known=0;
for(integer b=0;b<64;b=b+1)begin
 computed[b]=known?(value[b]^carry):1'bx;
 carry=carry&&(subtract?!value[b]:value[b]);
end
return computed;
endfunction
function automatic logic[WIDTH-1:0] transformed(input logic[WIDTH-1:0] p);
logic[WIDTH-1:0] computed;
computed=arithmetic(p[63:0],0);
return computed;
endfunction
function automatic logic[63:0] carry_value(input integer length);
return length==64?64'hffffffffffffffff:(64'd1<<length)-64'd1;
endfunction
function automatic logic[63:0] borrow_value(input integer length);
return length==64?64'd0:64'd1<<length;
endfunction
function automatic logic[63:0] boundary(input integer ordinal);
case(ordinal)
0:return 64'd0;1:return 64'hffffffffffffffff;2:return 64'hfffffffffffffffe;
3:return 64'h7fffffffffffffff;4:return 64'h8000000000000000;5:return 64'h8000000000000001;
default:begin $fatal(1,"invalid boundary");return 64'd0;end
endcase
endfunction
function automatic logic[63:0] random_value(input integer ordinal);
logic[63:0] state;
state=64'hd2106421;
for(integer index=0;index<=ordinal;index=index+1)
 state=state*64'd6364136223846793005+64'd1442695040888963407;
return state;
endfunction
function automatic logic[WIDTH-1:0] known_value(input integer index);
integer ordinal;logic[63:0] value,remaining;
if(index<65)value=carry_value(index);
else if(index<130)value=borrow_value(index-65);
else if(index<136)value=boundary(index-130);
else if(index==136)value=64'h5555555555555555;
else if(index==137)value=64'haaaaaaaaaaaaaaaa;
else value=random_value(index-138);
return value;
endfunction
`ifdef PYC_QUEUE_PIPELINE_FOUR_STATE
function automatic logic[63:0] dense(input integer pattern);
logic[63:0] value;
for(integer bit_index=0;bit_index<64;bit_index=bit_index+1)
 case(pattern)
 0:value[63-bit_index]=1'bx;1:value[63-bit_index]=1'bz;
 2:value[63-bit_index]=bit_index%2?1'bz:1'bx;
 3:case(bit_index%4)0:value[63-bit_index]=0;1:value[63-bit_index]=1;2:value[63-bit_index]=1'bx;3:value[63-bit_index]=1'bz;endcase
 endcase
return value;
endfunction
function automatic logic[WIDTH-1:0] four_value(input integer index);
integer owner,b,symbol,ordinal,pattern,low;
logic[WIDTH-1:0] p;
if(index<256)begin
 owner=index/256;b=(index%256)/4;symbol=(index%4)/2;
 p=64'ha5a5a5a55a5a5a5a;
 low=WIDTH-64-owner*64;p[low+b]=symbol==0?1'bx:1'bz;
end else if(index<264)begin
 ordinal=index-256;owner=ordinal/8;pattern=(ordinal%8)/2;
 p='1;
 low=WIDTH-64-owner*64;p[low+:64]=dense(pattern);
end
return p;
endfunction
`endif
function automatic logic[WIDTH-1:0] zero_result_input();
return 64'hffffffffffffffff;
endfunction
 task row(input bit clock_level,reset_level,valid_level,input logic[WIDTH-1:0] p,input bit take_level,input bit four=0);
 bit pop_second,room,advance,input_room,push_first,was_full;logic[WIDTH-1:0] old_first,expected_data;
 pop_second=second_count>0&&take_level;room=second_count<2||pop_second;
 advance=first_count>0&&room;input_room=first_count<2||advance;push_first=valid_level&&input_room;
 expected_data=second_count>0?second_data[0]:{WIDTH{1'b0}};
 pyc_7079635f727374=reset_level;valid=valid_level;data=p;take=take_level;#1;
 if(result!=={input_room,(second_count>0),expected_data})$fatal(1,"queue_pipeline row %0d two-depth-two deque oracle",row_index);
 if(four)$display("FOUR %0d %0d %0d %b",row_index,input_room,second_count>0,expected_data);
 else $display("WORK %0d %0d %0d %b",row_index,input_room,second_count>0,expected_data);
 stalled=stalled+(valid_level&&!input_room);
 if(clock_level&&!last_clock)begin
  if(reset_level)begin
   reset_full=reset_full+(outstanding==4);dropped=dropped+outstanding;outstanding=0;head=0;tail=0;first_count=0;second_count=0;
  end else begin
   was_full=outstanding==4;
   if(out_valid&&take_level)begin
    if(outstanding==0||out_data!==history[head])$fatal(1,"queue_pipeline commit history loss/order");
    head=head+1;retired=retired+1;outstanding=outstanding-1;
   end
   if(ready&&valid_level)begin
    history[tail]=transformed(p);tail=tail+1;accepted=accepted+1;outstanding=outstanding+1;
    if(was_full)replacements=replacements+1;
   end
   old_first=first_data[0];
   if(pop_second)begin second_data[0]=second_data[1];second_count=second_count-1;end
   if(advance)begin
    second_data[second_count]=transformed(old_first);second_count=second_count+1;
    first_data[0]=first_data[1];first_count=first_count-1;
   end
   if(push_first)begin first_data[first_count]=p;first_count=first_count+1;end
  end
  if(outstanding>peak)peak=outstanding;
  if(outstanding<0||outstanding>4||first_count>2||second_count>2||outstanding!=first_count+second_count)$fatal(1,"history capacity");
 end
 last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;row_index=row_index+1;
 endtask
 task edge_row(input logic[WIDTH-1:0]p,input bit v=1,t=1,r=0,four=0);row(1,r,v,p,t,four);row(0,r,v,p,t,four);endtask
function automatic logic[WIDTH-1:0] vector_value(input integer index,input bit four);
 if(four)begin
`ifdef PYC_QUEUE_PIPELINE_FOUR_STATE
 return four_value(index);
`else
 $fatal(1,"four-state sequence requires genuine four-state build");return '0;
`endif
 end
 return known_value(index);
endfunction
 task sequence_rows(input bit four);
integer vector_count;
row_index=0;stalled=0;accepted=0;retired=0;dropped=0;outstanding=0;peak=0;replacements=0;reset_full=0;head=0;tail=0;first_count=0;second_count=0;last_clock=0;
vector_count=four?FOUR_COUNT:KNOWN_COUNT;
row(0,1,0,0,0,four);edge_row(0,0,0,1,four);
edge_row(zero_result_input(),1,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);
for(integer i=0;i<5;i=i+1)edge_row(vector_value(i,four),1,0,0,four);
row(1,0,1,vector_value(5,four),0,four);row(1,1,1,vector_value(6,four),1,four);
row(0,0,1,vector_value(7,four),1,four);row(0,0,1,vector_value(8,four),0,four);
edge_row(0,1,1,1,four);
for(integer i=0;i<4;i=i+1)edge_row(vector_value(i,four),1,0,0,four);
for(integer i=0;i<6;i=i+1)edge_row(vector_value(i+4,four),1,1,0,four);
for(integer i=0;i<vector_count;i=i+1)edge_row(vector_value(i,four),1,1,0,four);
for(integer i=0;i<5;i=i+1)edge_row(0,0,1,0,four);
for(integer i=0;i<4;i=i+1)edge_row(vector_value(i,four),1,0,0,four);
edge_row(0,1,1,1,four);
edge_row(zero_result_input(),1,1,0,four);edge_row(0,1,1,0,four);
for(integer i=0;i<5;i=i+1)edge_row(0,0,1,0,four);
if(row_index!=2*vector_count+79)$fatal(1,"wrong finite sequence rows");
if(stalled<3||peak!=4||replacements<6||outstanding!=0||dropped!=8||reset_full!=2||accepted!=retired+dropped)$fatal(1,"queue_pipeline finite history coverage");
if(four)$display("HISTORY_FOUR_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
else $display("HISTORY_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
endtask
initial begin
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(0);
`ifdef PYC_QUEUE_PIPELINE_FOUR_STATE
pyc_7079635f727374=1;valid=0;take=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(1);
`endif
$finish;end
endmodule
