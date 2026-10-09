module tb;
  logic [0:2][11:0] r3,c3;
  logic [0:2] g3,bits_guard;
  logic [0:4][11:0] r5,c5;
  logic [0:4] g5;
  logic [0:2][7:0] bits,bits_candidate;
  wire [0:2][0:2][11:0] o3;
  wire [0:4][0:4][11:0] o5;
  wire [0:2][0:2][7:0] bo;
  ac_top dut(.*);
  initial begin
    r3={12'h10a,12'h214,12'h31e};c3={12'h31f,12'h000,12'h000};g3=3'b000;
    r5={12'h10a,12'h214,12'h31e,12'h428,12'h532};c5='0;g5='0;
    bits={8'h12,8'h34,8'ha5};bits_candidate={8'ha7,8'd99,8'd88};bits_guard=3'b010;
    #1;
    for(integer i=0;i<3;i=i+1) begin
      if(o3[0][i]!==12'h31e || o3[1][i]!==12'h10a || o3[2][i]!==12'h214 ||
         bo[0][i]!==8'ha5 || bo[1][i]!==99 || bo[2][i]!==8'h34)
        $fatal(1,"generic N3 shape/type/port lifting oracle failed");
    end
    for(integer i=0;i<5;i=i+1)
      if(o5[0][i]!==12'h532 || o5[1][i]!==12'h10a || o5[2][i]!==12'h214 ||
         o5[3][i]!==12'h31e || o5[4][i]!==12'h428)
        $fatal(1,"same generic definition N5 binding failed");
    g3=3'bx00;bits_guard=3'bx10;r5[4]='z;#1;
    for(integer i=0;i<3;i=i+1)
      if(o3[0][i]!==12'b00110001111x || bo[0][i]!==8'b101001x1)
        $fatal(1,"generic whole-T merge must resolve bits and struct with X guard");
    for(integer i=0;i<5;i=i+1)
      if(o5[0][i]!==12'hzzz) $fatal(1,"generic zero-candidate merge must preserve Z");
    $finish;
  end
endmodule
