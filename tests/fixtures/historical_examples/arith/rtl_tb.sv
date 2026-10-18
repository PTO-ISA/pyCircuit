module tb;
  logic [18:0] a,b;
  wire [18:0] sum;
  wire [15:0] lane_mask;
  wire [7:0] acc_width;
  wire [42:0] result;
  assign {sum, lane_mask, acc_width} = result;
  pyc_root dut(.*);
  task row(input logic [18:0] av,bv,expected);
    a=av;b=bv;#1;
    if(sum!==expected || lane_mask!==16'd65535 || acc_width!==8'd19)
      $fatal(1,"arith independent modular sum/constants oracle failed");
    $display("WORK %0d %0d %0d",sum,lane_mask,acc_width);
  endtask
  initial begin
    row(1,2,3);row(0,0,0);row(524287,0,524287);row(524287,1,0);
    row(524287,524287,524286);row(262144,262144,0);
    row(262143,1,262144);row(524286,3,1);
    row(273067,174762,447829);row(12345,54321,66666);
    $finish;
  end
endmodule
