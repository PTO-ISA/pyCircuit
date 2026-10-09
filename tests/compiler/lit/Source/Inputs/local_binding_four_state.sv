module tb;
  logic [0:0] clk=0;
  logic [0:0] rst=1;
  logic [7:0] a;
  logic [7:0] b;
  logic [64:0] wide;
  logic [0:0] flag;
  logic [0:0] one;
  wire [7:0] result;
  wire [7:0] q;
  wire [7:0] signed_mask;
  wire [7:0] signed_q;
  wire [0:0] zero_q;
  wire [8:0] selected;
  wire [7:0] input_alias;
  wire [7:0] child_alias;
  wire [0:0] bool_alias;
  wire [1:0] integer_plus;
  wire [7:0] original_xor;
  wire [7:0] original_or;
  wire [0:0] original_equal;
  wire [64:0] wide_alias;
  wire [0:0] wide_carry;
  wire [63:0] wide_singleton;
  pyc_root dut(.*);
  initial begin
    // Establish native Reset parity through real edges on all three DFFs.
    clk=1'b0;
    rst=1'b1;
    a=8'b0000000z;
    b=8'b00000000;
    wide=65'bz0000000000000000000000000000000000000000000000000000000000000000;
    flag=1'b1;
    one=1'bz;
    rst=1;clk=0;#1;clk=1;#1;clk=0;#1;
    // Independent fixed Work frame0.
    rst=1'b1;
    a=8'b0000000z;
    b=8'b00000000;
    wide=65'bz0000000000000000000000000000000000000000000000000000000000000000;
    flag=1'b1;
    one=1'bz;
    #1;
    if(!(
      result===8'bxxxxxxxx &&
      q===8'b00000000 &&
      signed_mask===8'bxxxxxxxx &&
      signed_q===8'b00000000 &&
      zero_q===1'b0 &&
      selected===9'bxxxxxxxxx &&
      input_alias===8'b0000000z &&
      child_alias===8'b0000000z &&
      bool_alias===1'b1 &&
      integer_plus===2'bxx &&
      original_xor===8'b0000000x &&
      original_or===8'b00000001 &&
      original_equal===1'bx &&
      wide_alias===65'bz0000000000000000000000000000000000000000000000000000000000000000 &&
      wide_carry===1'bx &&
      wide_singleton===64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx))$fatal(1,"local binding value/old-Q/mask oracle failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",result,q,signed_mask,signed_q,zero_q,selected,input_alias,child_alias,bool_alias,integer_plus,original_xor,original_or,original_equal,wide_alias,wide_carry,wide_singleton);
    clk=1'b0;#1;
    // Independent fixed Work frame1.
    rst=1'b1;
    a=8'b0000000z;
    b=8'b00000000;
    wide=65'bz0000000000000000000000000000000000000000000000000000000000000000;
    flag=1'b1;
    one=1'bz;
    #1;
    if(!(
      result===8'bxxxxxxxx &&
      q===8'b00000000 &&
      signed_mask===8'bxxxxxxxx &&
      signed_q===8'b00000000 &&
      zero_q===1'b0 &&
      selected===9'bxxxxxxxxx &&
      input_alias===8'b0000000z &&
      child_alias===8'b0000000z &&
      bool_alias===1'b1 &&
      integer_plus===2'bxx &&
      original_xor===8'b0000000x &&
      original_or===8'b00000001 &&
      original_equal===1'bx &&
      wide_alias===65'bz0000000000000000000000000000000000000000000000000000000000000000 &&
      wide_carry===1'bx &&
      wide_singleton===64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx))$fatal(1,"local binding value/old-Q/mask oracle failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",result,q,signed_mask,signed_q,zero_q,selected,input_alias,child_alias,bool_alias,integer_plus,original_xor,original_or,original_equal,wide_alias,wide_carry,wide_singleton);
    clk=1'b1;#1;
    // Independent fixed Work frame2.
    rst=1'b0;
    a=8'b11111111;
    b=8'b00000001;
    wide=65'b11111111111111111111111111111111111111111111111111111111111111111;
    flag=1'bx;
    one=1'b1;
    #1;
    if(!(
      result===8'b00000000 &&
      q===8'b00000000 &&
      signed_mask===8'b11111110 &&
      signed_q===8'b00000000 &&
      zero_q===1'b0 &&
      selected===9'b100000000 &&
      input_alias===8'b11111111 &&
      child_alias===8'b11111111 &&
      bool_alias===1'bx &&
      integer_plus===2'b10 &&
      original_xor===8'b11111110 &&
      original_or===8'b11111111 &&
      original_equal===1'b0 &&
      wide_alias===65'b11111111111111111111111111111111111111111111111111111111111111111 &&
      wide_carry===1'b1 &&
      wide_singleton===64'b0000000000000000000000000000000000000000000000000000000000000000))$fatal(1,"local binding value/old-Q/mask oracle failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",result,q,signed_mask,signed_q,zero_q,selected,input_alias,child_alias,bool_alias,integer_plus,original_xor,original_or,original_equal,wide_alias,wide_carry,wide_singleton);
    clk=1'b0;#1;
    // Independent fixed Work frame3.
    rst=1'b0;
    a=8'b00000011;
    b=8'b00000100;
    wide=65'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;
    flag=1'b0;
    one=1'b0;
    #1;
    if(!(
      result===8'b00000100 &&
      q===8'b00000000 &&
      signed_mask===8'b11111111 &&
      signed_q===8'b00000000 &&
      zero_q===1'b0 &&
      selected===9'b100000000 &&
      input_alias===8'b00000011 &&
      child_alias===8'b00000011 &&
      bool_alias===1'b0 &&
      integer_plus===2'b01 &&
      original_xor===8'b00000010 &&
      original_or===8'b00000011 &&
      original_equal===1'b0 &&
      wide_alias===65'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      wide_carry===1'bx &&
      wide_singleton===64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx))$fatal(1,"local binding value/old-Q/mask oracle failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",result,q,signed_mask,signed_q,zero_q,selected,input_alias,child_alias,bool_alias,integer_plus,original_xor,original_or,original_equal,wide_alias,wide_carry,wide_singleton);
    clk=1'b1;#1;
    // Independent fixed Work frame4.
    rst=1'b0;
    a=8'bz0000001;
    b=8'b00000011;
    wide=65'bzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz;
    flag=1'bz;
    one=1'bz;
    #1;
    if(!(
      result===8'bxxxxxxxx &&
      q===8'b00000100 &&
      signed_mask===8'bxxxxxxxx &&
      signed_q===8'b11111111 &&
      zero_q===1'bx &&
      selected===9'bxxxxxxxxx &&
      input_alias===8'bz0000001 &&
      child_alias===8'bz0000001 &&
      bool_alias===1'bz &&
      integer_plus===2'bxx &&
      original_xor===8'bx0000000 &&
      original_or===8'bx0000001 &&
      original_equal===1'b0 &&
      wide_alias===65'bzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz &&
      wide_carry===1'bx &&
      wide_singleton===64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx))$fatal(1,"local binding value/old-Q/mask oracle failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",result,q,signed_mask,signed_q,zero_q,selected,input_alias,child_alias,bool_alias,integer_plus,original_xor,original_or,original_equal,wide_alias,wide_carry,wide_singleton);
    clk=1'b1;#1;
    // Independent fixed Work frame5.
    rst=1'b0;
    a=8'bz0000001;
    b=8'b00000011;
    wide=65'b00000000000000000000000000000000000000000000000000000000000000000;
    flag=1'bz;
    one=1'bz;
    #1;
    if(!(
      result===8'bxxxxxxxx &&
      q===8'b00000100 &&
      signed_mask===8'bxxxxxxxx &&
      signed_q===8'b11111111 &&
      zero_q===1'bx &&
      selected===9'bxxxxxxxxx &&
      input_alias===8'bz0000001 &&
      child_alias===8'bz0000001 &&
      bool_alias===1'bz &&
      integer_plus===2'bxx &&
      original_xor===8'bx0000000 &&
      original_or===8'bx0000001 &&
      original_equal===1'b0 &&
      wide_alias===65'b00000000000000000000000000000000000000000000000000000000000000000 &&
      wide_carry===1'b0 &&
      wide_singleton===64'b0000000000000000000000000000000000000000000000000000000000000000))$fatal(1,"local binding value/old-Q/mask oracle failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",result,q,signed_mask,signed_q,zero_q,selected,input_alias,child_alias,bool_alias,integer_plus,original_xor,original_or,original_equal,wide_alias,wide_carry,wide_singleton);
    clk=1'b0;#1;
    // Independent fixed Work frame6.
    rst=1'b0;
    a=8'b00000000;
    b=8'b11111111;
    wide=65'b00000000000000000000000000000000000000000000000000000000000000000;
    flag=1'b0;
    one=1'b0;
    #1;
    if(!(
      result===8'b00000001 &&
      q===8'b00000100 &&
      signed_mask===8'b00000001 &&
      signed_q===8'b11111111 &&
      zero_q===1'bx &&
      selected===9'b100000000 &&
      input_alias===8'b00000000 &&
      child_alias===8'b00000000 &&
      bool_alias===1'b0 &&
      integer_plus===2'b01 &&
      original_xor===8'b00000001 &&
      original_or===8'b00000001 &&
      original_equal===1'b1 &&
      wide_alias===65'b00000000000000000000000000000000000000000000000000000000000000000 &&
      wide_carry===1'b0 &&
      wide_singleton===64'b0000000000000000000000000000000000000000000000000000000000000000))$fatal(1,"local binding value/old-Q/mask oracle failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",result,q,signed_mask,signed_q,zero_q,selected,input_alias,child_alias,bool_alias,integer_plus,original_xor,original_or,original_equal,wide_alias,wide_carry,wide_singleton);
    clk=1'b1;#1;
    // Independent fixed Work frame7.
    rst=1'b0;
    a=8'b00000000;
    b=8'b11111111;
    wide=65'b00000000000000000000000000000000000000000000000000000000000000000;
    flag=1'b0;
    one=1'b0;
    #1;
    if(!(
      result===8'b00000001 &&
      q===8'b00000001 &&
      signed_mask===8'b00000001 &&
      signed_q===8'b00000001 &&
      zero_q===1'b0 &&
      selected===9'b100000000 &&
      input_alias===8'b00000000 &&
      child_alias===8'b00000000 &&
      bool_alias===1'b0 &&
      integer_plus===2'b01 &&
      original_xor===8'b00000001 &&
      original_or===8'b00000001 &&
      original_equal===1'b1 &&
      wide_alias===65'b00000000000000000000000000000000000000000000000000000000000000000 &&
      wide_carry===1'b0 &&
      wide_singleton===64'b0000000000000000000000000000000000000000000000000000000000000000))$fatal(1,"local binding value/old-Q/mask oracle failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",result,q,signed_mask,signed_q,zero_q,selected,input_alias,child_alias,bool_alias,integer_plus,original_xor,original_or,original_equal,wide_alias,wide_carry,wide_singleton);
    clk=1'b0;#1;
    // Independent fixed Work frame8.
    rst=1'b1;
    a=8'bxxxxxxxx;
    b=8'bxxxxxxxx;
    wide=65'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;
    flag=1'bx;
    one=1'bx;
    #1;
    if(!(
      result===8'bxxxxxxxx &&
      q===8'b00000001 &&
      signed_mask===8'bxxxxxxxx &&
      signed_q===8'b00000001 &&
      zero_q===1'b0 &&
      selected===9'bxxxxxxxxx &&
      input_alias===8'bxxxxxxxx &&
      child_alias===8'bxxxxxxxx &&
      bool_alias===1'bx &&
      integer_plus===2'bxx &&
      original_xor===8'bxxxxxxxx &&
      original_or===8'bxxxxxxx1 &&
      original_equal===1'bx &&
      wide_alias===65'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      wide_carry===1'bx &&
      wide_singleton===64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx))$fatal(1,"local binding value/old-Q/mask oracle failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",result,q,signed_mask,signed_q,zero_q,selected,input_alias,child_alias,bool_alias,integer_plus,original_xor,original_or,original_equal,wide_alias,wide_carry,wide_singleton);
    clk=1'b1;#1;
    // Independent fixed Work frame9.
    rst=1'b1;
    a=8'bxxxxxxxx;
    b=8'bxxxxxxxx;
    wide=65'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;
    flag=1'bx;
    one=1'bx;
    #1;
    if(!(
      result===8'bxxxxxxxx &&
      q===8'b00000000 &&
      signed_mask===8'bxxxxxxxx &&
      signed_q===8'b00000000 &&
      zero_q===1'b0 &&
      selected===9'bxxxxxxxxx &&
      input_alias===8'bxxxxxxxx &&
      child_alias===8'bxxxxxxxx &&
      bool_alias===1'bx &&
      integer_plus===2'bxx &&
      original_xor===8'bxxxxxxxx &&
      original_or===8'bxxxxxxx1 &&
      original_equal===1'bx &&
      wide_alias===65'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      wide_carry===1'bx &&
      wide_singleton===64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx))$fatal(1,"local binding value/old-Q/mask oracle failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",result,q,signed_mask,signed_q,zero_q,selected,input_alias,child_alias,bool_alias,integer_plus,original_xor,original_or,original_equal,wide_alias,wide_carry,wide_singleton);
    clk=1'b0;#1;
    // Independent fixed Work frame10.
    rst=1'b0;
    a=8'b00000111;
    b=8'b00000010;
    wide=65'b00000000000000000000000000000000000000000000000000000000000100001;
    flag=1'b1;
    one=1'b1;
    #1;
    if(!(
      result===8'b00001000 &&
      q===8'b00000000 &&
      signed_mask===8'b00000101 &&
      signed_q===8'b00000000 &&
      zero_q===1'b0 &&
      selected===9'b000001000 &&
      input_alias===8'b00000111 &&
      child_alias===8'b00000111 &&
      bool_alias===1'b1 &&
      integer_plus===2'b10 &&
      original_xor===8'b00000110 &&
      original_or===8'b00000111 &&
      original_equal===1'b0 &&
      wide_alias===65'b00000000000000000000000000000000000000000000000000000000000100001 &&
      wide_carry===1'b0 &&
      wide_singleton===64'b0000000000000000000000000000000000000000000000000000000000000000))$fatal(1,"local binding value/old-Q/mask oracle failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",result,q,signed_mask,signed_q,zero_q,selected,input_alias,child_alias,bool_alias,integer_plus,original_xor,original_or,original_equal,wide_alias,wide_carry,wide_singleton);
    clk=1'b0;#1;
    $finish;
  end
endmodule
