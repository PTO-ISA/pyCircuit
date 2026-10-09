// Independent fixed positions, with genuine X/Z symbols in Icarus.
module tb;
  logic tiny;
  logic [4:0] n5;
  logic [6:0] middle;
  logic [72:0] wide;
  logic [2:0] state;
  wire [175:0] result;
  logic [4:0] arithmetic;
  logic [175:0] expected;
  pyc_root dut(.*);
  function logic symbol(input integer bit_index,row,input bit masked);
    integer code;
    begin
      code=masked ? (bit_index+row/2)%4 : ((bit_index+row*3)%5<2);
      case(code) 0:symbol=0;1:symbol=1;2:symbol=1'bx;3:symbol=1'bz;endcase
    end
  endfunction
  task frame(input integer row,input bit masked);
    tiny=symbol(0,row,masked);
    for(integer i=0;i<5;i=i+1)n5[i]=symbol(i,row,masked);
    for(integer i=0;i<7;i=i+1)middle[i]=symbol(i,row,masked);
    for(integer i=0;i<73;i=i+1)wide[i]=symbol(i,row,masked);
    for(integer i=0;i<3;i=i+1)state[i]=symbol(i,row,masked);
    #1;arithmetic=n5+5'd1;
    expected={tiny,n5,tiny,
              tiny,n5,wide[10:0],middle,wide[69:61],
              wide,n5,n5[1:0],tiny,wide[68:62],
              7'b0,n5,tiny,wide[71:57],n5[3:1],tiny,
              arithmetic,tiny,state,tiny,n5,tiny};
    if(result !== expected)$fatal(1,"MSB-first concat placement or X/Z transport failed");
    if(masked)$display("MASK %b",result);else $display("WORK %b",result);
  endtask
  initial begin
    for(integer row=0;row<8;row=row+1)frame(row,0);
`ifdef CONCAT_FOUR_STATE
    for(integer row=0;row<8;row=row+1)frame(row,1);
`endif
    $finish;
  end
endmodule
