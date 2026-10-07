module tb;
  logic clk_a=0,rst_a=1,clk_b=0,rst_b=0;
  wire [7:0] a_count,b_count;
  pyc_root dut(.*);
  task row(input logic ca,ra,cb,rb,input logic [7:0] a,b);
    rst_a=ra;rst_b=rb;#1;
    if(!(a_count===a && b_count===b))$fatal(1,"multiclock_regs independent edge/reset/hold/wrap oracle failed");
    $display("WORK %0d %0d",a_count,b_count);
    clk_a=ca;clk_b=cb;#1;
  endtask
  initial begin
`ifdef PYC_MULTICLOCK_FOUR_STATE
    #1;
    if(!(a_count===8'bxxxxxxxx && b_count===8'bxxxxxxxx))
      $fatal(1,"multiclock unreset physical startup must be X in full four-state RTL");
`endif
    // Declared/native historical unasserted smoke intent: clocks identical,
    // only A reset for2 cycles,
    // then1 deasserted cycle, rst_b explicitly0 and finish at cycle2.
    // B's pre-drive0 models the historical C++ Wire default. With the macro
    // disabled, this replay has no output assertions; the enabled checks are
    // additional four-state startup oracles, not historical TB expectations.
    // The retired SV TB generator drove only clk_a; this explicit two-clock
    // replay follows the TB declaration rather than that generator omission.
    for(integer cycle=0;cycle<6;cycle=cycle+1)begin
      rst_a=cycle<2;#1;clk_a=1;clk_b=1;#1;clk_a=0;clk_b=0;#1;
`ifdef PYC_MULTICLOCK_FOUR_STATE
      if(b_count!==8'bxxxxxxxx)$fatal(1,"multiclock A reset must never initialize B");
      if(cycle<2 && a_count!==8'd0)$fatal(1,"multiclock A own reset must establish known0");
`endif
    end
    // New parity preamble, explicitly resetting BOTH lanes on their own
    // rising edges. Startup zeros from Verilator are not evidence.
    rst_a=1;rst_b=1;#1;
`ifdef PYC_MULTICLOCK_FOUR_STATE
    if(b_count!==8'bxxxxxxxx)$fatal(1,"multiclock asserting B reset without edge must hold unknown Q");
`endif
    clk_a=1;clk_b=1;#1;clk_a=0;clk_b=0;#1;
`ifdef PYC_MULTICLOCK_FOUR_STATE
    if(!(a_count===8'd0 && b_count===8'd0))$fatal(1,"multiclock B becomes known only after own reset edge");
    $display("FOUR multiclock unresetBxxxxxxxx ownreset0");
`endif
    row(0,1,0,1,8'd0,8'd0);
    row(1,1,1,1,8'd0,8'd0);
    row(0,0,0,0,8'd0,8'd0);
    row(1,0,0,0,8'd0,8'd0);
    row(1,0,0,0,8'd1,8'd0);
    row(0,0,1,0,8'd1,8'd0);
    row(0,0,1,0,8'd1,8'd1);
    row(1,0,0,0,8'd1,8'd1);
    row(0,0,0,0,8'd2,8'd1);
    row(1,0,1,0,8'd2,8'd1);
    row(1,1,1,1,8'd3,8'd2);
    row(0,1,0,1,8'd3,8'd2);
    row(1,1,0,1,8'd3,8'd2);
    row(0,0,0,1,8'd0,8'd2);
    row(1,0,1,1,8'd0,8'd2);
    row(0,1,0,0,8'd1,8'd0);
    row(0,1,1,0,8'd1,8'd0);
    row(0,1,0,0,8'd1,8'd1);
    row(1,1,1,0,8'd1,8'd1);
    row(0,0,0,0,8'd0,8'd2);
    row(0,0,1,1,8'd0,8'd2);
    row(0,0,0,0,8'd0,8'd0);

    for(integer value=0;value<256;value=value+1)begin
      row(1,0,0,0,value[7:0],8'd0);row(0,0,0,0,(value+1)&255,8'd0);
    end
    for(integer value=0;value<256;value=value+1)begin
      row(0,0,1,0,8'd0,value[7:0]);row(0,0,0,0,8'd0,(value+1)&255);
    end
    for(integer value=0;value<256;value=value+1)begin
      row(1,0,1,0,value[7:0],value[7:0]);row(0,0,0,0,(value+1)&255,(value+1)&255);
    end
`ifdef PYC_MULTICLOCK_FOUR_STATE
    // Unknown resets on a lane without its own posedge are irrelevant; the
    // other clock still commits. No unknown-clock parity is asserted here.
    rst_a=1'bx;rst_b=0;#1;clk_b=1;#1;clk_b=0;#1;
    if(!(a_count===8'd0 && b_count===8'd1))$fatal(1,"multiclock irrelevant A reset X contaminated B");
    rst_a=1'bz;#1;clk_b=1;#1;clk_b=0;#1;
    if(!(a_count===8'd0 && b_count===8'd2))$fatal(1,"multiclock irrelevant A reset Z contaminated B");
    rst_a=0;rst_b=1'bx;#1;clk_a=1;#1;clk_a=0;#1;
    if(!(a_count===8'd1 && b_count===8'd2))$fatal(1,"multiclock irrelevant B reset X contaminated A");
    rst_b=1'bz;#1;clk_a=1;#1;clk_a=0;#1;
    if(!(a_count===8'd2 && b_count===8'd2))$fatal(1,"multiclock irrelevant B reset Z contaminated A");
    $display("FOUR multiclock ignoredresetXZ independentedges");
`endif
    $finish;
  end
endmodule
