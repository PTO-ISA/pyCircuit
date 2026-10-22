module tb;
  logic [7:0] PYC_COUNT=1,pyc_696d706c656d656e746174696f6e5f=2,pyc_6e5f5f30=4;
  wire [7:0] a,b,c,q;
  ac_top dut(.*);
  initial begin
    #1;
    if(a!==1 || b!==2 || c!==4 || q!==7)$fatal(1,"source symbol/pin and family internal name collision");
    $finish;
  end
endmodule
