module tb;
  logic [12:0] a;
  logic [12:0] b;
  logic [64:0] wide_a;
  logic [64:0] wide_b;
  logic [4:0] computed;
  wire [13:0] sum14;
  wire [12:0] masked13;
  wire [12:0] swapped13;
  wire [7:0] nonfull8;
  wire [0:0] zero1;
  wire [25:0] product26;
  wire [12:0] delta13;
  wire [12:0] nested13;
  wire [65:0] wide_sum66;
  wire [7:0] wide_low8;
  wire [129:0] wide_product130;
  wire [0:0] wide_zero1;
  wire [5:0] computed6;
  pyc_root dut(.*);
  initial begin
    // Independent fixed frame 0.
    a=13'b0000000000000;
    b=13'b0000000000000;
    wide_a=65'bx0000000000000000000000000000000000000000000000000000000000000000;
    wide_b=65'b00000000000000000000000000000000000000000000000000000000000000001;
    computed=5'b00000;
    #1;
    if(!(sum14===14'b00000000000000 &&
         masked13===13'b0000000000000 &&
         swapped13===13'b0000000000000 &&
         nonfull8===8'b00000000 &&
         zero1===1'b0 &&
         product26===26'b00000000000000000000000000 &&
         delta13===13'b0000000000000 &&
         nested13===13'b0000000000000 &&
         wide_sum66===66'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
         wide_low8===8'bxxxxxxxx &&
         wide_product130===130'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
         wide_zero1===1'b0 &&
         computed6===6'b000001))$fatal(1,"exact numeric golden mismatch");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b",sum14,masked13,swapped13,nonfull8,zero1,product26,delta13,nested13,wide_sum66,wide_low8,wide_product130,wide_zero1,computed6);
    // Independent fixed frame 1.
    a=13'b0000000000000;
    b=13'b0000000000000;
    wide_a=65'bz0000000000000000000000000000000000000000000000000000000000000000;
    wide_b=65'b00000000000000000000000000000000000000000000000000000000000000001;
    computed=5'b00000;
    #1;
    if(!(sum14===14'b00000000000000 &&
         masked13===13'b0000000000000 &&
         swapped13===13'b0000000000000 &&
         nonfull8===8'b00000000 &&
         zero1===1'b0 &&
         product26===26'b00000000000000000000000000 &&
         delta13===13'b0000000000000 &&
         nested13===13'b0000000000000 &&
         wide_sum66===66'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
         wide_low8===8'bxxxxxxxx &&
         wide_product130===130'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
         wide_zero1===1'b0 &&
         computed6===6'b000001))$fatal(1,"exact numeric golden mismatch");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b",sum14,masked13,swapped13,nonfull8,zero1,product26,delta13,nested13,wide_sum66,wide_low8,wide_product130,wide_zero1,computed6);
    $finish;
  end
endmodule
