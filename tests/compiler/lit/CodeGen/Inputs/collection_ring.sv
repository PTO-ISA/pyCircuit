module tb;
  logic clk=0, rst=1;
  logic [0:2] en_left, en_right;
  logic [0:2][7:0] init_left, init_right, data;
  wire [0:2][7:0] qa, qb;
  ac_top dut(.*);
  initial begin
    for (integer i=0;i<3;i=i+1) begin
      en_left[i]=1; en_right[i]=1; init_left[i]=i+1; init_right[i]=10+i; data[i]=20+i;
    end
    #1; clk=1; #1;
    if (qa[0]!==1 || qa[1]!==2 || qa[2]!==3 || qb[0]!==10 || qb[1]!==11 || qb[2]!==12)
      $fatal(1,"same occurrence/key must retain independent owners");
    clk=0; rst=0; #1; clk=1; #1;
    if (qa[0]!==2 || qa[1]!==3 || qa[2]!==1 || qb[0]!==20 || qb[1]!==21 || qb[2]!==22)
      $fatal(1,"ring must sample all old Q simultaneously");
    clk=0; en_right[0]=0; data[0]=99; #1; clk=1; #1;
    if (qa[0]!==3 || qa[1]!==1 || qa[2]!==2 || qb[0]!==20) $fatal(1,"per-lane enables changed identity");
    clk=0; rst=1; #1; clk=1; #1;
    if (qa[0]!==1 || qa[1]!==2 || qa[2]!==3 || qb[0]!==10 || qb[1]!==11 || qb[2]!==12)
      $fatal(1,"reset must preserve independent init lane values");
    $finish;
  end
endmodule
