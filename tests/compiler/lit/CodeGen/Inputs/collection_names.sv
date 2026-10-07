module tb;
  logic [7:0] PYC_COUNT=1,implementation_=2,n0=4;
  wire [7:0] a,b,c,q;
  ac_top dut(.*);
  initial begin
    #1;
    if(a!==1 || b!==2 || c!==4 || q!==7)$fatal(1,"source symbol/pin and family internal name collision");
    $finish;
  end
endmodule
