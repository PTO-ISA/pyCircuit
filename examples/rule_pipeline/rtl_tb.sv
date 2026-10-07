module tb;
localparam integer WIDTH=16;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic [WIDTH-1:0] data='0;
wire [WIDTH+1:0] result;
wire ready=result[WIDTH+1],out_valid=result[WIDTH];wire [WIDTH-1:0] out_data=result[WIDTH-1:0];
pyc_root dut(.*);
integer first_count=0,input_pops=0;bit second_valid=0,last_clock=0;
logic [WIDTH-1:0] first_data[0:1],second_data='0,history[0:1023];
integer row_index=0,stalled=0,accepted=0,retired=0,dropped=0,outstanding=0,peak=0,replacements=0,head=0,tail=0;
// Independent packed oracle: only the original increment field changes.
function automatic logic[WIDTH-1:0] transformed(input logic[WIDTH-1:0] p);
return p + 16'd1;
endfunction
localparam integer KNOWN_COUNT=54;
localparam integer FOUR_COUNT=4*WIDTH+8;
function automatic logic[WIDTH-1:0] known_value(input integer index);
logic[WIDTH-1:0] p;
if(index<17)p=16'((1<<index)-1);
else if(index<33)p=16'((((index-17)*4051)^16'ha55a)&16'hffff);
else if(index==33)p=16'd65534;
else if(index==34)p=16'h5555;
else if(index==35)p=16'haaaa;
else if(index==36)p='0;
else if(index==37)p='1;
else p=16'b1<<(index-38);
return p;
endfunction
function automatic logic[WIDTH-1:0] four_value(input integer index);
integer b,symbol,pattern;logic[WIDTH-1:0] p;
if(index<4*WIDTH)begin
 b=index/4;symbol=(index%4)/2;p=WIDTH'(32'h02a5abcd);
 p[b]=symbol==0?1'bx:1'bz;
end else begin
 pattern=(index-4*WIDTH)/2;p='0;
 for(integer j=0;j<WIDTH;j=j+1)begin
  case(pattern)
  0:p[WIDTH-1-j]=1'bx;
  1:p[WIDTH-1-j]=1'bz;
  2:p[WIDTH-1-j]=(j%2==0)?1'bx:1'bz;
  3:case(j%4)0:p[WIDTH-1-j]=0;1:p[WIDTH-1-j]=1;2:p[WIDTH-1-j]=1'bx;3:p[WIDTH-1-j]=1'bz;endcase
  endcase
 end
end
return p;
endfunction
 task row(input bit clock_level,reset_level,valid_level,input logic[WIDTH-1:0] p,input bit take_level,input bit four=0);
 bit pop_second,room,pop_first,input_room,push_first,push_second,was_full;logic[WIDTH-1:0] old_first;
 pop_second=second_valid&&take_level;room=!second_valid||pop_second;pop_first=(first_count>0)&&room;input_room=(first_count<2)||pop_first;push_first=valid_level&&input_room;push_second=(first_count>0)&&room;
 pyc_7079635f727374=reset_level;valid=valid_level;data=p;take=take_level;#1;
 if(result!=={input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}}})$fatal(1,"rule_pipeline row %0d three-slot oracle",row_index);
 if(four)$display("FOUR %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 else $display("WORK %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 stalled=stalled+(valid_level&&!input_room);
 if(clock_level&&!last_clock)begin
  if(reset_level)begin dropped=dropped+outstanding;outstanding=0;head=0;tail=0;first_count=0;second_valid=0;first_data[0]=0;first_data[1]=0;second_data=0;end
  else begin
   was_full=outstanding==3;
   if(out_valid&&take_level)begin if(outstanding==0||out_data!==history[head])$fatal(1,"rule_pipeline commit history loss/order");head=head+1;retired=retired+1;outstanding=outstanding-1;end
   if(ready&&valid_level)begin history[tail]=transformed(p);tail=tail+1;accepted=accepted+1;outstanding=outstanding+1;if(was_full)replacements=replacements+1;end
   old_first=first_data[0];
   if(push_second)begin second_valid=1;second_data=transformed(old_first);end else if(pop_second)begin second_valid=0;second_data=0;end
   if(pop_first)begin first_data[0]=first_data[1];first_count=first_count-1;input_pops=input_pops+1;end
   if(push_first)begin first_data[first_count]=p;first_count=first_count+1;end
   if(first_count<0||first_count>2)$fatal(1,"input queue capacity");
  end
  if(outstanding>peak)peak=outstanding;if(outstanding<0||outstanding>3)$fatal(1,"history capacity");
 end
 last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;row_index=row_index+1;
 endtask
 task edge_row(input logic[WIDTH-1:0]p,input bit v=1,t=1,r=0,four=0);row(1,r,v,p,t,four);row(0,r,v,p,t,four);endtask
task sequence_rows(input bit four);
row_index=0;input_pops=0;stalled=0;accepted=0;retired=0;dropped=0;outstanding=0;peak=0;replacements=0;head=0;tail=0;first_count=0;second_valid=0;first_data[0]=0;first_data[1]=0;second_data=0;last_clock=0;
row(0,1,0,0,0,four);edge_row(0,0,0,1,four);
// E0 capture, E1 transform, E2 retirement; zero output is still a token.
edge_row(16'd65535,1,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);
for(integer round=0;round<2;round=round+1)begin
for(integer item=0;item<3;item=item+1)edge_row(known_value(17+round*3+item),1,0,0,four);
for(integer item=0;item<3;item=item+1)edge_row(0,0,1,0,four);
end
for(integer i=0;i<5;i=i+1)edge_row(known_value(i),1,0,0,four);
row(1,0,1,known_value(5),0,four);row(1,0,1,known_value(6),1,four);row(0,0,1,known_value(7),1,four);row(0,0,1,known_value(8),1,four);
edge_row(known_value(9),1,1,0,four);edge_row(known_value(10),1,1,0,four);edge_row(0,1,0,1,four);
if(four)begin
edge_row({WIDTH{1'bx}},1,0,0,1);edge_row({WIDTH{1'bz}},1,0,0,1);edge_row(0,1,0,0,1);
row(1,0,1,{WIDTH{1'bx}},0,1);row(1,1,1,0,1,1);row(0,0,1,'1,1,1);row(0,0,0,0,0,1);
edge_row(known_value(0),1,1,0,1);edge_row(0,0,1,0,1);edge_row(0,0,1,0,1);
for(integer i=0;i<FOUR_COUNT;i=i+1)edge_row(four_value(i),1,1,0,1);
edge_row(0,0,1,0,1);edge_row(0,0,1,0,1);edge_row(0,0,1,0,1);
edge_row({WIDTH{1'bx}},1,0,0,1);edge_row({WIDTH{1'bz}},1,0,0,1);edge_row({WIDTH{1'bx}},1,0,0,1);edge_row(0,0,0,1,1);
end
else for(integer i=0;i<KNOWN_COUNT;i=i+1)edge_row(known_value(i));
edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(known_value(0),1,0,0,four);edge_row(known_value(1),1,0,0,four);edge_row(0,1,1,1,four);
if(input_pops<8||stalled<3||peak!=3||replacements<2||outstanding!=0||dropped<2||accepted!=retired+dropped)$fatal(1,"rule_pipeline finite history coverage");
if(four)$display("HISTORY_FOUR_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
else $display("HISTORY_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
endtask
initial begin
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(0);
`ifdef PYC_INCREMENT_PIPELINE_FOUR_STATE
pyc_7079635f727374=1;valid=0;take=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(1);
`endif
$finish;end
endmodule
