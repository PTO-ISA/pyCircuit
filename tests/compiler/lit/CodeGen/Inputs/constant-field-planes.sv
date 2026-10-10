module tb;
  logic clk, rst, en;
  logic [7:0] dynamic0, dynamic1;
  logic [69:0] index0, index1;
  wire [1613:0] records0;
  wire [806:0] records1;
  wire [779:0] wide0;
  wire [389:0] wide1;
  wire [23:0] mixed0, mixed1;
  wire [8:0] modes0, modes1, mode_bits0, mode_bits1;
  wire flag0, flag1, in_range0, in_range1;
  wire [129:0] selected_wide0, selected_wide1;
  wire [7:0] selected_dynamic0, selected_dynamic1, q0, q1;
  wire [15:0] pair_q;
  wire [47:0] generic_observed;
  wire [23:0] computed_observed, mapped_observed, forwarded_observed;
  pyc_root dut(.*);
  initial begin
    clk=0; rst=0; en=1; dynamic0=8'h5a; dynamic1=8'bz101x101;
    index0=70'd1; index1=70'd1; #1;
    if (selected_wide0 !== ((130'd1<<65)|130'd9)) $fatal(1,"wide high word");
    if (flag0 !== 1'b1 || selected_dynamic0 !== 8'h5a || !in_range0) $fatal(1,"known index");
    if (selected_dynamic1 !== dynamic1 || !in_range1) $fatal(1,"dynamic X/Z transport");
    if (mixed0[15:8] !== 8'h5a || mixed1[15:8] !== dynamic1) $fatal(1,"mixed plane");
    index0=70'd3; index1='x; #1;
    if (in_range0 !== 1'b0 || selected_wide0 !== 'x) $fatal(1,"OOB");
    if (in_range1 !== 1'bx || selected_dynamic1 !== 8'hxx) $fatal(1,"X index");
    index1='z; #1;
    if (in_range1 !== 1'bx || selected_wide1 !== 'x) $fatal(1,"Z index");
    $display("constant field planes RTL passed");
    $finish;
  end
endmodule
