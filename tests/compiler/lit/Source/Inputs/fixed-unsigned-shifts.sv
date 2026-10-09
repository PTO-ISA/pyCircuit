// Independent per-bit source positions. No RTL shift operator computes goldens.
module tb;
  logic [7:0] a;
`ifdef EXACT_INTEGER
  wire [7:0] identity;
  wire add_before;
  wire [7:0] static_signed;
`else
  logic [0:0] n1;
  logic [4:0] n5;
  logic [12:0] n13;
  logic [64:0] n65;
  logic [129:0] n130;
  wire [2815:0] result;
  logic [2815:0] expected;
`endif
  pyc_root dut(.*);
  function automatic logic symbol(input integer position,width,row,input bit mask);
    if(mask)begin
      if(row==1)return 1'bx;
      if(row==2)return 1'bz;
      if(row==0)begin
        if(position==0 || position==width-1 || position==63 || position==128)return 1'bx;
        if(position==1 || position+2==width || position==64 || position==127)return 1'bz;
      end else begin
        if(position%11==0)return 1'bz;
        if(position%11==1)return 1'bx;
      end
      return position%2!=0;
    end
    case(row)
      0:return 1'b0;
      1:return 1'b1;
      2:return position==width-1;
      3:return position==0;
      4:return position==(width>63 ? 63 : width/2);
      5:return position==(width>64 ? 64 : 0);
      6:return position%2!=0;
      7:return position%2==0;
      default:return (position*29+row*17)%7<3;
    endcase
  endfunction
  function automatic logic [129:0] pattern(input integer width,row,input bit mask);
    logic [129:0] raw;
    raw=0;
    for(integer position=0;position<width;position=position+1)
      raw[position]=symbol(position,width,row,mask);
    return raw;
  endfunction
`ifndef EXACT_INTEGER
  task shifted(input integer low,out_width,in_width,expression_width,count,
               input bit right,input logic [129:0] raw);
    integer source_position;
    for(integer position=0;position<out_width;position=position+1)begin
      source_position=right ? position+count : position-count;
      if(position<expression_width && source_position>=0 &&
         source_position<expression_width && source_position<in_width)
        expected[low+position]=raw[source_position];
      else expected[low+position]=0;
    end
  endtask
  task matrix(input integer width,low,input logic [129:0] raw);
    integer count;
    for(integer direction=0;direction<2;direction=direction+1)
      for(integer slot=0;slot<6;slot=slot+1)begin
        case(slot)
          0:count=0;1:count=1;2:count=width-1;3:count=width;
          4:count=width+1;5:count=width;
        endcase
        shifted(low+(11-direction*6-slot)*width,width,width,width,count,direction==1,raw);
      end
  endtask
`endif
  task row(input integer row_index,input bit mask,emit);
    logic [129:0] av;
    av=pattern(8,row_index,mask);a=av[7:0];
`ifndef EXACT_INTEGER
    n1=1'(pattern(1,row_index,mask));n5=5'(pattern(5,row_index,mask));
    n13=13'(pattern(13,row_index,mask));n65=65'(pattern(65,row_index,mask));
    n130=pattern(130,row_index,mask);
`endif
    #1;
`ifdef EXACT_INTEGER
    if(identity !== a || add_before !== 1'((int'(a)+1)/256) || static_signed !== 8'd252)
      $fatal(1,"exact Integer arithmetic before shift or static signed regression failed");
    if(emit)$display("WORK %0d %0d %0d",add_before,static_signed,identity);
`else
    expected=0;
    matrix(1,2804,n1);matrix(5,2744,n5);matrix(13,2588,n13);
    matrix(65,1808,n65);matrix(130,248,n130);
    shifted(235,13,13,13,3,0,n13);shifted(170,65,65,65,1,1,n65);
    // Wrapped/unknown arithmetic is fully discarded by the width8 overshift.
    shifted(162,8,8,8,8,1,av);
    shifted(146,16,8,8,1,0,av);shifted(130,16,8,16,1,0,av);
    shifted(0,130,130,130,1,0,n130);
    if(result !== expected)$fatal(1,"fixed unsigned shift positions/XZ/zero-fill/width-order/transport failed");
    if(emit)begin
      if(mask)$display("MASK %b",result);else $display("WORK %b",result);
    end
`endif
  endtask
  initial begin
    for(integer row_index=0;row_index<16;row_index=row_index+1)row(row_index,0,1);
`ifdef FIXED_SHIFTS_FOUR_STATE
`ifndef EXACT_INTEGER
    for(integer row_index=0;row_index<4;row_index=row_index+1)begin
      row(row_index,1,1);row(1,0,0);
    end
`endif
`endif
    $finish;
  end
endmodule
