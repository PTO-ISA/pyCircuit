module tb;
localparam integer WIDTH=13;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic [WIDTH-1:0] data='0;
wire [WIDTH+1:0] result;
wire ready=result[WIDTH+1],out_valid=result[WIDTH];wire [WIDTH-1:0] out_data=result[WIDTH-1:0];
pyc_root dut(.*);
bit first_valid=0,second_valid=0,last_clock=0;
logic [WIDTH-1:0] first_data='0,second_data='0,history[0:1023];
integer row_index=0,stalled=0,accepted=0,retired=0,dropped=0,outstanding=0,peak=0,replacements=0,head=0,tail=0,native_original_retired=0;
// Independent packed model: RUN tag, header increment, original peer and Enum swap.
function automatic logic[WIDTH-1:0] transformed(input logic[WIDTH-1:0] p);
return {1'b1,p[11:9],3'(p[8:6]+3'd1),p[5:3],p[1],p[2],p[2]==1'b0};
endfunction
localparam integer KNOWN_COUNT=271;
localparam integer FOUR_COUNT=4*WIDTH+8;
function automatic logic[WIDTH-1:0] known_value(input integer index);
integer code,peer,modes,tag_bits,nested;logic[WIDTH-1:0] p;
if(index<256)begin
 code=index/32;peer=(index%32)/4;modes=index%4;
 tag_bits=((code&1)<<3)|((code+peer)&7);nested=(code<<3)|peer;
 p=13'((tag_bits<<9)|(nested<<3)|(modes<<1)|(peer&1));
end else if(index==256)p='0;
else if(index==257)p='1;
else p=13'b1<<(index-258);
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
 pop_second=second_valid&&take_level;room=!second_valid||pop_second;pop_first=first_valid&&room;input_room=!first_valid||pop_first;push_first=valid_level&&input_room;push_second=first_valid&&room;
 pyc_7079635f727374=reset_level;valid=valid_level;data=p;take=take_level;#1;
 if(result!=={input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}}})$fatal(1,"recursive_aggregate_payload_pipeline row %0d two-slot oracle",row_index);
 if(four)$display("FOUR %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 else $display("WORK %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 stalled=stalled+(valid_level&&!input_room);
 if(clock_level&&!last_clock)begin
  if(reset_level)begin dropped=dropped+outstanding;outstanding=0;head=0;tail=0;first_valid=0;second_valid=0;first_data=0;second_data=0;end
  else begin
   was_full=outstanding==2;
   if(out_valid&&take_level)begin if(outstanding==0||out_data!==history[head])$fatal(1,"recursive_aggregate_payload_pipeline commit history loss/order");head=head+1;retired=retired+1;outstanding=outstanding-1;end
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
// Original backend cycles: reset0, inject1, sole transformed output2.
for(integer cycle=0;cycle<10;cycle=cycle+1)begin
 edge_row(cycle==1?13'd2962:0,cycle==1,1,cycle==0,four);
 if(!ready||out_valid!==(cycle==2)||out_data!==(cycle==2?13'd7125:13'd0))$fatal(1,"original backend timing/value");
 $display("ORIGINAL_BACKEND %0d %0d %b",cycle,out_valid,out_data);
end
edge_row(0,0,0,1,four);
// Original native vector: five Work/Xfer ticks and exactly one token.
native_original_retired=retired;
for(integer cycle=0;cycle<5;cycle=cycle+1)begin
 edge_row(cycle==0?13'd2962:0,cycle==0,1,0,four);
 if(!ready||out_valid!==(cycle==1)||out_data!==(cycle==1?13'd7125:13'd0))$fatal(1,"original native timing/value");
 $display("ORIGINAL_NATIVE %0d %0d %b",cycle,out_valid,out_data);
end
if(retired!=native_original_retired+1)$fatal(1,"original native exactly one retirement");
// E0 capture, E1 transform, E2 retirement, including header wrap.
edge_row(13'(7<<6),1,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);
for(integer i=0;i<5;i=i+1)edge_row(known_value(i),1,0,0,four);
row(1,0,1,known_value(5),0,four);row(1,0,1,known_value(6),1,four);row(0,0,1,known_value(7),1,four);row(0,0,1,known_value(8),1,four);
edge_row(known_value(9),1,1,0,four);edge_row(known_value(10),1,1,0,four);edge_row(0,1,0,1,four);
if(four)begin
edge_row({WIDTH{1'bx}},1,0,0,1);edge_row({WIDTH{1'bz}},1,0,0,1);edge_row(0,1,0,0,1);
row(1,0,1,{WIDTH{1'bx}},0,1);row(1,1,1,0,1,1);row(0,0,1,'1,1,1);row(0,0,0,0,0,1);
edge_row(known_value(0),1,1,0,1);edge_row(0,0,1,0,1);edge_row(0,0,1,0,1);
for(integer i=0;i<FOUR_COUNT;i=i+1)edge_row(four_value(i),1,1,0,1);
end
else for(integer i=0;i<KNOWN_COUNT;i=i+1)edge_row(known_value(i));
edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(known_value(0),1,0,0,four);edge_row(known_value(1),1,0,0,four);edge_row(0,1,1,1,four);
if(stalled<3||peak!=2||replacements<2||outstanding!=0||dropped<2||accepted!=retired+dropped)$fatal(1,"recursive_aggregate_payload_pipeline finite history coverage");
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
