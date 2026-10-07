module tb;
  logic [7:0] data=8'h96;
  wire [15:0] pair;
  wire [7:0] a,b;
  ac_top dut(.pyc_696e707574(data),.pair(pair),.a(a),.b(b));
  initial begin
    #1;if(pair!==16'h9696 || a!==8'h96 || b!==8'h96)$fatal(1,"legal field feedback p.b depends only on input");
    data=8'h23;#1;if(pair!==16'h2323 || a!==8'h23 || b!==8'h23)$fatal(1,"shared field graph failed to update");
    data=8'b1010zxxx;#1;
    if(pair!==16'b1010zxxx1010zxxx || a!==8'b1010zxxx || b!==8'b1010zxxx)
      $fatal(1,"field feedback must preserve X/Z planes");
    $finish;
  end
endmodule
