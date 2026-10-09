module tb;
localparam integer WIDTH=20,INPUT_WIDTH=10;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic [INPUT_WIDTH-1:0] data='0;
wire [WIDTH+1:0] result;
wire ready=result[WIDTH+1],out_valid=result[WIDTH];wire [WIDTH-1:0] out_data=result[WIDTH-1:0];
pyc_root dut(.*);
bit first_valid=0,second_valid=0,last_clock=0;
logic [INPUT_WIDTH-1:0] first_data='0;logic [WIDTH-1:0] second_data='0,history[0:9999];
integer birth[0:9999],edge_index=0;
integer row_index=0,stalled=0,accepted=0,retired=0,dropped=0,outstanding=0,peak=0,replacements=0,head=0,tail=0;
// Scalar comparisons and balanced ternary equations from the historical
// semantic contract. No DUT helper, compiler lowering or Runtime oracle.
function automatic logic[WIDTH-1:0] transformed(input logic[INPUT_WIDTH-1:0] p);
logic rn,rr,rw,re,valid,b0,b1,present,conflict,sn,sr,sw,se,selected,member;
logic[3:0]low,high,candidate,decoded,onehot;logic[7:0]class_low,class_high,class_candidate,classification;
rn=p[9:6]==4'd1;rr=p[9:6]==4'd3;rw=p[9:6]==4'd9;re=p[9:6]==4'd15;
valid=(rn|rr)|(rw|re);low=rn?4'd1:4'd3;high=rw?4'd9:4'd15;candidate=(rn|rr)?low:high;decoded=valid?candidate:4'd1;
b0=p[4]!=0;b1=p[5]!=0;present=b0|b1;
if(p[5:4]===2'b00||p[5:4]===2'b01||p[5:4]===2'b10||p[5:4]===2'b11)conflict=(p[4]===1'b1)&&(p[5]===1'b1);else conflict=1'bx;
onehot=conflict?4'd15:(present?(b0?4'd3:4'd9):4'd1);
sn=p[3:0]==4'd1;sr=p[3:0]==4'd3;sw=p[3:0]==4'd9;se=p[3:0]==4'd15;selected=sr|sw;member=(sn|sr)|(sw|se);
class_low=sn?8'd10:8'd11;class_high=sw?8'd12:8'd13;class_candidate=(sn|sr)?class_low:class_high;classification=member?class_candidate:8'd255;
return{decoded,valid,onehot,present,conflict,selected,classification};
endfunction
localparam integer KNOWN_COUNT=1024,FOUR_COUNT=1584;
function automatic logic[INPUT_WIDTH-1:0]known_value(input integer index);return INPUT_WIDTH'(index);endfunction
`ifdef PYC_ENUM_HELPERS_FOUR_STATE
function automatic logic[3:0]symbolic(input integer value,width);
logic[3:0]p;integer digit;p=0;
for(integer b=0;b<width;b=b+1)begin digit=(value>>(2*b))&3;case(digit)0:p[b]=0;1:p[b]=1;2:p[b]=1'bx;3:p[b]=1'bz;endcase end
return p;
endfunction
function automatic logic[3:0]code(input integer i);case(i%4)0:return 1;1:return 3;2:return 9;3:return 15;endcase endfunction
function automatic logic[3:0]dense(input integer i,input bit selector);
case(i)0:return 4'bxxxx;1:return 4'bzzzz;2:return selector?4'bzxzx:4'bxzxz;3:return selector?4'bxz01:4'b01xz;endcase
endfunction
function automatic logic[INPUT_WIDTH-1:0]four_value(input integer i);
integer n,raw,sel,mask,literal,symbol;logic[3:0]r,s,temp;logic[1:0]m;
if(i<512)begin n=i/2;return{symbolic(n,4),2'(n%4),code(n%4)};end
if(i<1024)begin n=(i-512)/2;return{code(n%4),2'((n/4)%4),symbolic(n,4)};end
if(i<1056)begin n=(i-1024)/2;temp=symbolic(n,2);return{4'd1,temp[1:0],4'd15};end
if(i<1568)begin n=(i-1056)/2;raw=n/64;sel=(n/16)%4;mask=n%16;temp=symbolic(mask,2);return{dense(raw,0),temp[1:0],dense(sel,1)};end
n=(i-1568)/2;symbol=n/4;literal=n%4;r=4'd1;m=2'b01;s=4'd1;
case(literal)0:r=symbol?4'bz001:4'bx001;1:s=symbol?4'b00z1:4'b00x1;2:m=symbol?2'b0z:2'b0x;3:m=symbol?2'bz1:2'bx1;endcase
return{r,m,s};
endfunction
// Explicit original-tree distinctions: these cannot be replaced by decoded
// raw carrier, a linear classification chain or bit-AND conflict.
task oracle_witnesses;
logic[19:0]a;
a=transformed(10'bx001010001);if(a[19:15]!==5'bxxx1x)$fatal(1,"raw X001 balanced fallback");
a=transformed(10'bz001010001);if(a[19:15]!==5'bxxx1x)$fatal(1,"raw Z001 balanced fallback");
a=transformed(10'b00010100x1);if(a[7:0]!==8'bxxxx1xxx)$fatal(1,"selector00X1 balanced classification");
a=transformed(10'b00010100z1);if(a[7:0]!==8'bxxxx1xxx)$fatal(1,"selector00Z1 balanced classification");
a=transformed(10'b00010x0001);if(a[14:9]!==6'bxxx1xx)$fatal(1,"mask0X popcount conflict");
a=transformed(10'b00010z0001);if(a[14:9]!==6'bxxx1xx)$fatal(1,"mask0Z popcount conflict");
a=transformed(10'b0001x10001);if(a[14:9]!==6'bxx111x)$fatal(1,"maskX1 priority");
a=transformed(10'b0001z10001);if(a[14:9]!==6'bxx111x)$fatal(1,"maskZ1 priority");
endtask
`endif

 task row(input bit clock_level,reset_level,valid_level,input logic[INPUT_WIDTH-1:0] p,input bit take_level,input bit four=0);
 bit pop_second,room,pop_first,input_room,push_first,push_second,was_full;logic[INPUT_WIDTH-1:0] old_first;
 pop_second=second_valid&&take_level;room=!second_valid||pop_second;pop_first=first_valid&&room;input_room=!first_valid||pop_first;push_first=valid_level&&input_room;push_second=first_valid&&room;
 pyc_7079635f727374=reset_level;valid=valid_level;data=p;take=take_level;#1;
 if(result!=={input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}}})$fatal(1,"enum_helpers row %0d two-slot oracle",row_index);
 if(four)$display("FOUR %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 else $display("WORK %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 stalled=stalled+(valid_level&&!input_room);
 if(clock_level&&!last_clock)begin
  if(reset_level)begin dropped=dropped+outstanding;outstanding=0;head=0;tail=0;edge_index=0;first_valid=0;second_valid=0;first_data=0;second_data=0;end
  else begin
   was_full=outstanding==2;
   if(out_valid&&take_level)begin if(outstanding==0||out_data!==history[head]||edge_index<birth[head]+2)$fatal(1,"enum_helpers commit history loss/order");head=head+1;retired=retired+1;outstanding=outstanding-1;end
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
 task edge_row(input logic[INPUT_WIDTH-1:0]p,input bit v=1,t=1,r=0,four=0);row(1,r,v,p,t,four);row(0,r,v,p,t,four);endtask
task sequence_rows(input bit four);
row_index=0;stalled=0;accepted=0;retired=0;dropped=0;outstanding=0;peak=0;replacements=0;head=0;tail=0;edge_index=0;first_valid=0;second_valid=0;first_data=0;second_data=0;last_clock=0;
row(0,1,0,0,0,four);edge_row(0,0,0,1,four);
// E0 capture, E1 transform, E2 retirement; zero flags are still a token.
edge_row(10'd31,1,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);
for(integer i=0;i<5;i=i+1)edge_row(known_value(i),1,0,0,four);
row(1,0,1,known_value(5),0,four);row(1,0,1,known_value(6),1,four);row(0,0,1,known_value(7),1,four);row(0,0,1,known_value(8),1,four);
edge_row(known_value(9),1,1,0,four);edge_row(known_value(10),1,1,0,four);edge_row(0,1,0,1,four);
if(four)begin
`ifdef PYC_ENUM_HELPERS_FOUR_STATE
edge_row({INPUT_WIDTH{1'bx}},1,0,0,1);edge_row({INPUT_WIDTH{1'bz}},1,0,0,1);edge_row(0,1,0,0,1);
row(1,0,1,{INPUT_WIDTH{1'bx}},0,1);row(1,1,1,0,1,1);row(0,0,1,'1,1,1);row(0,0,0,0,0,1);
edge_row(known_value(0),1,1,0,1);edge_row(0,0,1,0,1);edge_row(0,0,1,0,1);
for(integer i=0;i<FOUR_COUNT;i=i+1)edge_row(four_value(i),1,1,0,1);
// Known recovery after uncertain payloads, including zero and allones.
edge_row(10'd31,1,1,0,1);edge_row(10'd1023,1,1,0,1);
`else
$fatal(1,"four-state sequence requires genuine four-state build");
`endif
end
else for(integer i=0;i<KNOWN_COUNT;i=i+1)edge_row(known_value(i));
edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(known_value(0),1,0,0,four);edge_row(known_value(1),1,0,0,four);edge_row(0,1,1,1,four);
if(row_index!=(four?3229:2089))$fatal(1,"wrong finite sequence rows");
if(stalled<3||peak!=2||replacements<2||outstanding!=0||dropped<2||accepted!=retired+dropped)$fatal(1,"enum_helpers finite history coverage");
if(four)$display("HISTORY_FOUR_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
else $display("HISTORY_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
endtask
initial begin
`ifdef PYC_ENUM_HELPERS_NEGATIVE
// Isolated effective-unknown control failure; this process must terminate.
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
valid=1;data=10'h0d3;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
`ifdef PYC_ENUM_HELPERS_NEGATIVE_POP
// Populate both owners before making the retiring control indeterminate.
data=10'h269;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;take=1'bx;
`else
// Old input can move; new input's effective push is unknown.
valid=1'bx;take=0;
`endif
#1;pyc_7079635f636c6b=1;#2;$fatal(1,"EXPECTED_REJECTION_MISSING");
`else
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(0);
`ifdef PYC_ENUM_HELPERS_FOUR_STATE
pyc_7079635f727374=1;valid=0;take=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
oracle_witnesses();sequence_rows(1);
`endif
$finish;
`endif
end
endmodule
