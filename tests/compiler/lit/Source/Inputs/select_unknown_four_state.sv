module tb;
  logic [7:0] a;
  logic flag;
  wire [7:0] right,left;
  pyc_root dut(.*);
  initial begin
    a=8'bz0000011;flag=1'b1;#1;
    if(!(right===8'bz0000011 && left===8'b00000011))$fatal(1,"opaque same-width pureIfExp golden failed");
    $display("WORK %b %b",right,left);
    a=8'bx0000011;flag=1'b0;#1;
    if(!(right===8'b00000011 && left===8'bx0000011))$fatal(1,"opaque same-width pureIfExp golden failed");
    $display("WORK %b %b",right,left);
    a=8'b00000011;flag=1'bx;#1;
    if(!(right===8'b00000011 && left===8'b00000011))$fatal(1,"opaque same-width pureIfExp golden failed");
    $display("WORK %b %b",right,left);
    a=8'b11111111;flag=1'bz;#1;
    if(!(right===8'bxxxxxx11 && left===8'bxxxxxx11))$fatal(1,"opaque same-width pureIfExp golden failed");
    $display("WORK %b %b",right,left);
    $finish;
  end
endmodule
