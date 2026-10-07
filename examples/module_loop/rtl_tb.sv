module tb;
  logic clk=0,rst=1,en_left=1,en_right=1;
  logic data_left=1,data_right=0,init_left=0,init_right=1;
  wire left,right;
  pyc_root dut(.*);
  localparam logic [0:10] CLOCK       =11'b01010101010;
  localparam logic [0:10] RESET       =11'b00000100011;
  localparam logic [0:10] ENABLE_LEFT =11'b11100011100;
  localparam logic [0:10] ENABLE_RIGHT=11'b11111011100;
  localparam logic [0:10] DATA_RIGHT  =11'b00011100000;
  localparam logic [0:10] OLD_LEFT    =11'b00111100110;
  localparam logic [0:10] OLD_RIGHT   =11'b11110011111;
  initial begin
    // Establish the same reset state as host Reset, outside the eleven epochs.
    #1;clk=1;#1;
    if(left!==0 || right!==1)$fatal(1,"independent reset values failed");
    clk=0;rst=0;#1;
    for(integer epoch=0;epoch<11;epoch=epoch+1)begin
      rst=RESET[epoch];en_left=ENABLE_LEFT[epoch];en_right=ENABLE_RIGHT[epoch];
      data_left=1;data_right=DATA_RIGHT[epoch];init_left=0;init_right=1;
      // Settle non-clock pins, capture old Q, then advance the clock sample.
      #1;
      if(left!==OLD_LEFT[epoch] || right!==OLD_RIGHT[epoch])
        $fatal(1,"module_loop epoch %0d old-Q/hold/reset oracle failed",epoch);
      $display("WORK %0d %0d",left,right);
      clk=CLOCK[epoch];#1;
    end
    $finish;
  end
endmodule
