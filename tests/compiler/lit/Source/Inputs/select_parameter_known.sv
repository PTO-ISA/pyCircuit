module tb;
  logic [7:0] a;
  logic [11:0] b;
  logic [0:0] flag;
  wire [7:0] default_right;
  wire [7:0] default_left;
  wire [7:0] override_right;
  wire [7:0] override_left;
  wire [11:0] wide_right;
  wire [11:0] wide_left;
  pyc_root dut(.*);
  initial begin
    a=8'b00000000;
    b=12'b000000000000;
    flag=1'b0;
    #1;
    if(!(default_right===8'b11111111 &&
         default_left===8'b00000000 &&
         override_right===8'b00001111 &&
         override_left===8'b00000000 &&
         wide_right===12'b001111111111 &&
         wide_left===12'b000000000000))$fatal(1,"pure parameter IfExp golden failed");
    $display("WORK %b %b %b %b %b %b",default_right,default_left,override_right,override_left,wide_right,wide_left);
    a=8'b11111111;
    b=12'b111111111111;
    flag=1'b1;
    #1;
    if(!(default_right===8'b11111111 &&
         default_left===8'b11111111 &&
         override_right===8'b11111111 &&
         override_left===8'b00001111 &&
         wide_right===12'b111111111111 &&
         wide_left===12'b001111111111))$fatal(1,"pure parameter IfExp golden failed");
    $display("WORK %b %b %b %b %b %b",default_right,default_left,override_right,override_left,wide_right,wide_left);
    a=8'b00001111;
    b=12'b001111111111;
    flag=1'b0;
    #1;
    if(!(default_right===8'b11111111 &&
         default_left===8'b00001111 &&
         override_right===8'b00001111 &&
         override_left===8'b00001111 &&
         wide_right===12'b001111111111 &&
         wide_left===12'b001111111111))$fatal(1,"pure parameter IfExp golden failed");
    $display("WORK %b %b %b %b %b %b",default_right,default_left,override_right,override_left,wide_right,wide_left);
    a=8'b10101010;
    b=12'b101010101010;
    flag=1'b1;
    #1;
    if(!(default_right===8'b10101010 &&
         default_left===8'b11111111 &&
         override_right===8'b10101010 &&
         override_left===8'b00001111 &&
         wide_right===12'b101010101010 &&
         wide_left===12'b001111111111))$fatal(1,"pure parameter IfExp golden failed");
    $display("WORK %b %b %b %b %b %b",default_right,default_left,override_right,override_left,wide_right,wide_left);
    a=8'b01111111;
    b=12'b100000000000;
    flag=1'b0;
    #1;
    if(!(default_right===8'b11111111 &&
         default_left===8'b01111111 &&
         override_right===8'b00001111 &&
         override_left===8'b01111111 &&
         wide_right===12'b001111111111 &&
         wide_left===12'b100000000000))$fatal(1,"pure parameter IfExp golden failed");
    $display("WORK %b %b %b %b %b %b",default_right,default_left,override_right,override_left,wide_right,wide_left);
    a=8'b00000011;
    b=12'b000000000111;
    flag=1'b1;
    #1;
    if(!(default_right===8'b00000011 &&
         default_left===8'b11111111 &&
         override_right===8'b00000011 &&
         override_left===8'b00001111 &&
         wide_right===12'b000000000111 &&
         wide_left===12'b001111111111))$fatal(1,"pure parameter IfExp golden failed");
    $display("WORK %b %b %b %b %b %b",default_right,default_left,override_right,override_left,wide_right,wide_left);
    $finish;
  end
endmodule
