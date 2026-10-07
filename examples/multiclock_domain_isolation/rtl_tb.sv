module tb;
  logic clk_a=0,rst_a=1,clk_b=0,rst_b=0;
  wire [7:0] a_count,b_count;
  // pyc_root takes the two domain records as packed structs; the harness drives
  // their clk/rst fields with the same two independent physical pins.
  wire pycircuit_types::ac_example_multiclock_domain_isolation_multiclock_domain_isolation_DomainPins_t d_a,d_b;
  assign d_a.clk=clk_a;assign d_a.rst=rst_a;
  assign d_b.clk=clk_b;assign d_b.rst=rst_b;
  pyc_root dut(.*);
  // `row` checks the value Work samples in this epoch, then applies the clock
  // levels. A sampled value is therefore the state committed by the PREVIOUS
  // row's own rising edges: an edge commits only on its own lane's clock rise.
  task row(input logic ca,ra,cb,rb,input logic [7:0] a,b);
    rst_a=ra;rst_b=rb;#1;
    if(!(a_count===a && b_count===b))$fatal(1,"multiclock_domain_isolation independent edge/reset/hold/wrap oracle failed");
    $display("WORK %0d %0d",a_count,b_count);
    clk_a=ca;clk_b=cb;#1;
  endtask
  initial begin
`ifdef PYC_MULTICLOCK_DOMAIN_ISOLATION_FOUR_STATE
    #1;
    if(!(a_count===8'bxxxxxxxx && b_count===8'bxxxxxxxx))
      $fatal(1,"multiclock_domain_isolation unreset physical startup must be X in full four-state RTL");
`endif
    // Preamble: each lane becomes known only through its OWN reset edge, and
    // one lane's reset never initializes the other lane. It prints no WORK
    // sample, matching the native host, which reaches the same known zero
    // state through the public Reset contract before the first driven epoch.
    rst_a=1;rst_b=1;#1;
`ifdef PYC_MULTICLOCK_DOMAIN_ISOLATION_FOUR_STATE
    if(b_count!==8'bxxxxxxxx)$fatal(1,"multiclock_domain_isolation asserted A reset without an A edge must not initialize B");
`endif
    clk_b=1;#1;clk_b=0;#1;
`ifdef PYC_MULTICLOCK_DOMAIN_ISOLATION_FOUR_STATE
    if(b_count!==8'd0)$fatal(1,"multiclock_domain_isolation B must become known zero on its own reset edge");
    if(a_count!==8'bxxxxxxxx)$fatal(1,"multiclock_domain_isolation B own reset edge must not initialize A");
`endif
    clk_a=1;#1;clk_a=0;#1;
`ifdef PYC_MULTICLOCK_DOMAIN_ISOLATION_FOUR_STATE
    if(!(a_count===8'd0 && b_count===8'd0))$fatal(1,"multiclock_domain_isolation both lanes known zero only after their own reset edges");
    $display("FOUR multiclock_domain_isolation unresetX ownreset0 independent");
`endif
    // Known oracle. Rows 0..2 establish zero through each lane's own reset
    // edge; 3..6 prove reset needs its own edge; 7..14 prove one lane counts
    // while the other holds a different value; 15..18 prove each lane's own
    // reset leaves the other undisturbed; 19..30 prove simultaneous edges.
    row(0,1,0,1,8'd0,8'd0);
    row(1,1,1,1,8'd0,8'd0);
    row(0,0,0,0,8'd0,8'd0);
    row(1,0,0,0,8'd0,8'd0);
    row(1,1,0,0,8'd1,8'd0);
    row(0,1,0,0,8'd1,8'd0);
    row(1,1,0,0,8'd1,8'd0);
    row(0,0,0,0,8'd0,8'd0);
    row(1,0,0,0,8'd0,8'd0);
    row(0,0,0,0,8'd1,8'd0);
    row(1,0,0,0,8'd1,8'd0);
    row(0,0,1,0,8'd2,8'd0);
    row(0,0,0,0,8'd2,8'd1);
    row(0,0,1,0,8'd2,8'd1);
    row(0,0,0,0,8'd2,8'd2);
    row(1,1,0,0,8'd2,8'd2);
    row(0,0,0,0,8'd0,8'd2);
    row(0,0,1,1,8'd0,8'd2);
    row(0,0,0,0,8'd0,8'd0);
    row(1,0,1,0,8'd0,8'd0);
    row(0,0,0,0,8'd1,8'd1);
    row(1,0,0,0,8'd1,8'd1);
    row(0,0,0,0,8'd2,8'd1);
    row(0,0,1,0,8'd2,8'd1);
    row(0,0,0,0,8'd2,8'd2);
    row(1,0,1,0,8'd2,8'd2);
    row(0,0,0,0,8'd3,8'd3);
    row(0,0,1,0,8'd3,8'd3);
    row(0,0,0,0,8'd3,8'd4);
    row(0,0,1,0,8'd3,8'd4);
    row(0,0,0,0,8'd3,8'd5);
    // Phase 0: only A edges, 256 of them, while B holds 5. Phase 1: only B
    // edges, 256 of them, while A holds 3. Phase 2: both edge together from
    // the different offsets 3 and 5. Every phase really wraps 255 -> 0 with
    // rst low, and the held lane never moves.
    for(integer slot=0;slot<512;slot=slot+1)
      row(slot%2==0,0,0,0,(3+(slot+1)/2)&255,8'd5);
    for(integer slot=0;slot<512;slot=slot+1)
      row(0,0,slot%2==0,0,8'd3,(5+(slot+1)/2)&255);
    for(integer slot=0;slot<512;slot=slot+1)
      row(slot%2==0,0,slot%2==0,0,(3+(slot+1)/2)&255,(5+(slot+1)/2)&255);
`ifdef PYC_MULTICLOCK_DOMAIN_ISOLATION_FOUR_STATE
    // An X/Z reset is irrelevant to a lane with no rising edge of its own,
    // while the other lane still commits on its own edge. Unknown-clock RTL
    // parity is not claimed here.
    rst_a=1'bx;rst_b=0;#1;clk_b=1;#1;clk_b=0;#1;
    if(!(a_count===8'd3 && b_count===8'd6))$fatal(1,"multiclock_domain_isolation irrelevant A reset X contaminated B");
    rst_a=1'bz;#1;clk_b=1;#1;clk_b=0;#1;
    if(!(a_count===8'd3 && b_count===8'd7))$fatal(1,"multiclock_domain_isolation irrelevant A reset Z contaminated B");
    rst_a=0;rst_b=1'bx;#1;clk_a=1;#1;clk_a=0;#1;
    if(!(a_count===8'd4 && b_count===8'd7))$fatal(1,"multiclock_domain_isolation irrelevant B reset X contaminated A");
    rst_b=1'bz;#1;clk_a=1;#1;clk_a=0;#1;
    if(!(a_count===8'd5 && b_count===8'd7))$fatal(1,"multiclock_domain_isolation irrelevant B reset Z contaminated A");
    $display("FOUR multiclock_domain_isolation ignoredresetXZ independentedges");
`endif
    $finish;
  end
endmodule
