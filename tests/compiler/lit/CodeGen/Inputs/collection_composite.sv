module tb;
  localparam integer N=3;
  logic clk=0, rst=1;
  logic [0:N-1] en;
  logic [0:N-1][7:0] d, init;
  wire [0:N-1][7:0] first_q, second_q;
  ac_top dut(.*);
  initial begin
    for (integer i=0;i<N;i=i+1) begin en[i]=1; d[i]=17; init[i]=3; end
    #1; clk=1; #1;
    for (integer i=0;i<N;i=i+1) if (first_q[i]!==3 || second_q[i]!==3) $fatal(1,"composite reset failed");
    clk=0; rst=0; #1; clk=1; #1;
    for (integer i=0;i<N;i=i+1) if (first_q[i]!==17 || second_q[i]!==3) $fatal(1,"leaf descendants must sample old Q");
    clk=0; #1; clk=1; #1;
    for (integer i=0;i<N;i=i+1) if (first_q[i]!==17 || second_q[i]!==17) $fatal(1,"second stage latency changed");
    $finish;
  end
endmodule
