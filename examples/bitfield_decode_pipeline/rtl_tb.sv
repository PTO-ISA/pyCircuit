module tb;
localparam integer WIDTH=131;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid=0,take=0;
logic [WIDTH-1:0] data='0;
wire [WIDTH+1:0] result;
wire ready=result[WIDTH+1],out_valid=result[WIDTH];wire [WIDTH-1:0] out_data=result[WIDTH-1:0];
pyc_root dut(.*);
bit first_valid=0,second_valid=0,last_clock=0;
logic [WIDTH-1:0] first_data='0,second_data='0,history[0:1023];
integer row_index=0,stalled=0,accepted=0,retired=0,dropped=0,outstanding=0,peak=0,replacements=0,head=0,tail=0;
// Independent original field positions; copied low25 is never derived from word.
function automatic logic[WIDTH-1:0] transformed(input logic[WIDTH-1:0] p);return {p[130:99],p[130:125],p[130:120],p[119:103],p[64:62],p[61:57],p[56:32],p[130:125],p[61:57],p[119:103],p[64:62],p[99]};endfunction
 task row(input bit clock_level,reset_level,valid_level,input logic[WIDTH-1:0] p,input bit take_level,input bit four=0);
 bit pop_second,room,pop_first,input_room,push_first,push_second,was_full;logic[WIDTH-1:0] old_first;
 pop_second=second_valid&&take_level;room=!second_valid||pop_second;pop_first=first_valid&&room;input_room=!first_valid||pop_first;push_first=valid_level&&input_room;push_second=first_valid&&room;
 pyc_7079635f727374=reset_level;valid=valid_level;data=p;take=take_level;#1;
 if(result!=={input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}}})$fatal(1,"bitfield_decode_pipeline row %0d two-slot oracle",row_index);
 if(four)$display("FOUR %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 else $display("WORK %0d %0d %0d %b",row_index,input_room,second_valid,second_valid?second_data:{WIDTH{1'b0}});
 stalled=stalled+(valid_level&&!input_room);
 if(clock_level&&!last_clock)begin
  if(reset_level)begin dropped=dropped+outstanding;outstanding=0;head=0;tail=0;first_valid=0;second_valid=0;first_data=0;second_data=0;end
  else begin
   was_full=outstanding==2;
   if(out_valid&&take_level)begin if(outstanding==0||out_data!==history[head])$fatal(1,"bitfield_decode_pipeline commit history loss/order");head=head+1;retired=retired+1;outstanding=outstanding-1;end
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
logic[WIDTH-1:0] values[0:WIDTH+3];logic[WIDTH-1:0] four_values[0:7+2*WIDTH];
task sequence_rows(input bit four);
row_index=0;stalled=0;accepted=0;retired=0;dropped=0;outstanding=0;peak=0;replacements=0;head=0;tail=0;first_valid=0;second_valid=0;first_data=0;second_data=0;last_clock=0;
row(0,1,0,0,0,four);edge_row(0,0,0,1,four);
for(integer i=0;i<5;i=i+1)edge_row(values[i],1,0,0,four);
row(1,0,1,values[5],0,four);row(1,0,1,values[6],1,four);row(0,0,1,values[7],1,four);row(0,0,1,values[8],1,four);
edge_row(values[9],1,1,0,four);edge_row(values[10],1,1,0,four);edge_row(0,1,0,1,four);
if(four)for(integer i=0;i<8+2*WIDTH;i=i+1)edge_row(four_values[i],1,1,0,1);
else for(integer i=0;i<WIDTH+4;i=i+1)edge_row(values[i]);
edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(0,0,1,0,four);edge_row(values[0],1,0,0,four);edge_row(values[1],1,0,0,four);edge_row(0,1,1,1,four);
if(stalled<3||peak!=2||replacements<2||outstanding!=0||dropped!=4||accepted!=retired+dropped||accepted!=(four?14+2*WIDTH:WIDTH+10)||retired!=(four?10+2*WIDTH:WIDTH+6))$fatal(1,"bitfield_decode_pipeline finite history coverage");
if(four)$display("HISTORY_FOUR_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
else $display("HISTORY_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,outstanding,peak);
endtask
initial begin
values[0]='0;values[1]='1;
for(integer parity=0;parity<2;parity=parity+1)
 for(integer b=0;b<WIDTH;b=b+1) values[2+parity][b]=(b+parity)%2;
for(integer b=0;b<WIDTH;b=b+1) begin values[4+b]='0;values[4+b][b]=1'b1;end
`ifdef PYC_BITFIELD_FOUR_STATE
for(integer shift=0;shift<4;shift=shift+1)
 for(integer latent=0;latent<2;latent=latent+1)
  for(integer b=0;b<WIDTH;b=b+1)
   case((b+shift)%4)
    0:four_values[shift*2+latent][b]=1'b0;
    1:four_values[shift*2+latent][b]=1'b1;
    2:four_values[shift*2+latent][b]=1'bx;
    3:four_values[shift*2+latent][b]=1'bz;
   endcase
for(integer b=0;b<WIDTH;b=b+1)begin
 four_values[8+2*b]='0;four_values[9+2*b]='0;
 four_values[8+2*b][b]=1'bx;four_values[9+2*b][b]=1'bz;
end
`endif
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(0);
`ifdef PYC_BITFIELD_FOUR_STATE
pyc_7079635f727374=1;valid=0;take=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(1);
`endif
$finish;end
endmodule
