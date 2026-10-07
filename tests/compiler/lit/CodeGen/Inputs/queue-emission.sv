`ifdef QUEUE_OWNERS
module tb;
  logic clk=0,rst=1,valid=0,take=0;
  logic [12:0] data=47;
  wire ready,available;
  wire [12:0] head,echo;
  pyc_root dut(.*);
  initial begin
    #1;clk=1;#1;clk=0;rst=0;#1;
    if(head!==13'd0)$fatal(1,"owner reset");
    valid=1;#1;clk=1;#1;clk=0;valid=0;#1;
`ifdef QUEUE_DEAD
    if(ready!==1'b1||available!==1'b0||head!==13'd0||echo!==13'd0)$fatal(1,"dead constant output");
    $display("DEAD constant output checked before failure");
`ifdef QUEUE_DELAYED_DEAD
    valid=1'bx;take=1'bx;
    repeat(2)begin #1;clk=1;#1;clk=0;end
    valid=0;#1;clk=1;#1;
`else
    valid=1'bx;#1;clk=1;#1;
`endif
    $fatal(1,"unused queue failure was removed");
`else
    if(available!==1'b1||head!==13'd47||echo!==13'd47)$fatal(1,"collision queue/ordinary child");
    take=1;#1;clk=1;#1;clk=0;#1;
    if(available!==1'b0||head!==13'd0)$fatal(1,"collision drain");
    $display("COLLISION fifo plus fifo_state passed");
    $finish;
`endif
  end
endmodule
`else
`include "queue_vectors.svh"
module tb;
  `QUEUE_PORTS
  pyc_root dut(.*);
  initial begin
`ifdef QUEUE_FOUR_STATE
    #1;
    `QUEUE_COLD
`endif
    // Match native initialization/host Reset with an actual hardware reset edge.
    #1;rst=1;clk=1;`QUEUE_RESET_CLOCKS_HIGH
    #1;clk=0;`QUEUE_RESET_CLOCKS_LOW
    rst=0;#1;
`ifdef QUEUE_FOUR_STATE
    `include "queue_rows_four.svh"
`else
    `include "queue_rows_known.svh"
`endif
    $finish;
  end
endmodule

`endif
