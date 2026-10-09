module tb;
localparam integer WIDTH=17;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic [WIDTH-1:0] data='0;
wire [WIDTH+1:0] result;
wire ready=result[WIDTH+1],out_valid=result[WIDTH];wire [WIDTH-1:0] out_data=result[WIDTH-1:0];
pyc_root dut(.*);
bit first_valid=0,second_valid=0,last_clock=0;
logic [WIDTH-1:0] first_data='0,second_data='0,history[0:9999];
integer row_index=0,stalled=0,accepted=0,retired=0,dropped=0,outstanding=0,peak=0,replacements=0,head=0,tail=0;
// Independent per-bit count; preserve the entire original value field.
function automatic logic[WIDTH-1:0] transformed(input logic[WIDTH-1:0] p);
integer ones;bit known;logic[WIDTH-1:0] output_token;
ones=0;known=1;output_token=p;
for(integer b=4;b<17;b=b+1)begin
 if(p[b]!==1'b0&&p[b]!==1'b1)known=0;
 if(p[b]===1'b1)ones=ones+1;
end
for(integer b=0;b<4;b=b+1)output_token[b]=known?((ones>>b)&1):1'bx;
return output_token;
endfunction
localparam integer KNOWN_COUNT=8272;
localparam integer FOUR_COUNT=76;
function automatic logic[WIDTH-1:0] known_value(input integer index);
integer value,prior,sweep;logic[WIDTH-1:0] p;
if(index<8192)begin value=index;prior=(value*7+3)%16;end
else begin
 sweep=(index-8192)/16;prior=(index-8192)%16;
 case(sweep)0:value=0;1:value=1;2:value=4096;3:value=5461;4:value=8191;
 default:$fatal(1,"invalid known sweep");endcase
end
p=WIDTH'((value<<4)|prior);return p;
endfunction
`ifdef PYC_POPCOUNT_PIPELINE_FOUR_STATE
function automatic logic[WIDTH-1:0] four_value(input integer index);
integer b,symbol,pattern;logic[WIDTH-1:0] p;
if(index<52)begin
 b=index/4;symbol=(index%4)/2;p=WIDTH'((32'h15ab<<4)|10);
 p[4+b]=symbol==0?1'bx:1'bz;
end else if(index<68)begin
 b=(index-52)/4;symbol=((index-52)%4)/2;
 p=WIDTH'(((b%2?8191:0)<<4)|5);p[b]=symbol==0?1'bx:1'bz;
end else begin
 pattern=(index-68)/2;p='0;
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
`endif
 task row(input bit clock_level,reset_level,valid_level,input logic[WIDTH-1:0] p,input bit take_level,input bit four=0);
 bit pop_second,room,pop_first,input_room,push_first,push_second,was_full;logic[WIDTH-1:0] old_first;
 pop_second=second_valid&&take_level;room=!second_valid||pop_second;pop_first=first_valid&&room;input_room=!first_valid||pop_first;push_first=valid_level&&input_room;push_second=first_valid&&room;
 pyc_7079635f727374=reset_level;valid=valid_level;data=p;take=take_level;#1;
 if(result!=={input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}}})$fatal(1,"popcount_pipeline row %0d two-slot oracle",row_index);
 if(four)$display("FOUR %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 else $display("WORK %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 stalled=stalled+(valid_level&&!input_room);
 if(clock_level&&!last_clock)begin
  if(reset_level)begin dropped=dropped+outstanding;outstanding=0;head=0;tail=0;first_valid=0;second_valid=0;first_data=0;second_data=0;end
  else begin
   was_full=outstanding==2;
   if(out_valid&&take_level)begin if(outstanding==0||out_data!==history[head])$fatal(1,"popcount_pipeline commit history loss/order");head=head+1;retired=retired+1;outstanding=outstanding-1;end
   if(ready&&valid_level)begin history[tail]=transformed(p);tail=tail+1;accepted=accepted+1;outstanding=outstanding+1;if(was_full)replacements=replacements+1;end
   old_first=first_data;
   if(push_second)begin second_valid=1;second_data=transformed(old_first);end else if(pop_second)begin second_valid=0;second_data=0;end
   if(push_first)begin first_valid=1;first_data=p;end else if(pop_first)begin first_valid=0;first_data=0;end
  end
  if(outstanding>peak)peak=outstanding;if(outstanding<0||outstanding>2)$fatal(1,"history capacity");
 end
 last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;row_index=row_index+1;
 endtask
 task edge_row(input logic[WIDTH-1:0]p,input bit v=1,t=1,r=0,four=0);row(1,r,v,p,t,four);row(0,r,v,p,t,four);endtask
task sequence_rows(input bit four);
row_index=0;stalled=0;accepted=0;retired=0;dropped=0;outstanding=0;peak=0;replacements=0;head=0;tail=0;first_valid=0;second_valid=0;first_data=0;second_data=0;last_clock=0;
row(0,1,0,0,0,four);edge_row(0,0,0,1,four);
// E0 capture, E1 transform, E2 retirement; zero output is still a token.
edge_row(17'd15,1,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);
for(integer i=0;i<5;i=i+1)edge_row(known_value(i),1,0,0,four);
row(1,0,1,known_value(5),0,four);row(1,0,1,known_value(6),1,four);row(0,0,1,known_value(7),1,four);row(0,0,1,known_value(8),1,four);
edge_row(known_value(9),1,1,0,four);edge_row(known_value(10),1,1,0,four);edge_row(0,1,0,1,four);
if(four)begin
`ifdef PYC_POPCOUNT_PIPELINE_FOUR_STATE
edge_row({WIDTH{1'bx}},1,0,0,1);edge_row({WIDTH{1'bz}},1,0,0,1);edge_row(0,1,0,0,1);
row(1,0,1,{WIDTH{1'bx}},0,1);row(1,1,1,0,1,1);row(0,0,1,'1,1,1);row(0,0,0,0,0,1);
edge_row(known_value(0),1,1,0,1);edge_row(0,0,1,0,1);edge_row(0,0,1,0,1);
for(integer i=0;i<FOUR_COUNT;i=i+1)edge_row(four_value(i),1,1,0,1);
// Known recovery after uncertain payloads, including zero and count13.
edge_row(17'd15,1,1,0,1);edge_row(17'(8191<<4),1,1,0,1);
`else
$fatal(1,"four-state sequence requires genuine four-state build");
`endif
end
else for(integer i=0;i<KNOWN_COUNT;i=i+1)edge_row(known_value(i));
edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(known_value(0),1,0,0,four);edge_row(known_value(1),1,0,0,four);edge_row(0,1,1,1,four);
if(row_index!=(four?213:16585))$fatal(1,"wrong finite sequence rows");
if(stalled<3||peak!=2||replacements<2||outstanding!=0||dropped<2||accepted!=retired+dropped)$fatal(1,"popcount_pipeline finite history coverage");
if(four)$display("HISTORY_FOUR_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
else $display("HISTORY_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
endtask
initial begin
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(0);
`ifdef PYC_POPCOUNT_PIPELINE_FOUR_STATE
pyc_7079635f727374=1;valid=0;take=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(1);
`endif
$finish;end
endmodule
