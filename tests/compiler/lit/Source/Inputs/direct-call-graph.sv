// Independently specified field values and one-register temporal feedback.
module tb;
  function automatic logic [72:0] wide_value(input integer n);
    logic [63:0] low_word;
    low_word=64'h0123456789abcdef ^ (64'(n)*64'h0101010101010101);
    return {9'((n*51+257)%512),low_word};
  endfunction
`ifdef DIRECT_BYTE_STATIC
  logic [7:0] left=0,right=0;
  wire [15:0] result;
  task byte_row(input logic [7:0] lv,rv,input bit unknown_case);
    left=lv;right=rv;#1;
    if(result !== {8'(left+1),8'(right+1)})$fatal(1,"byte increment/lane mismatch");
    if(unknown_case)$display("MASK %b",result);else $display("WORK %b",result);
  endtask
  initial begin
    for(int n=0;n<256;n++)begin
      byte_row(8'(n),8'(197*n+31),0);byte_row(8'(n),8'(n),0);
      byte_row(8'(n),8'd0,0);byte_row(8'd0,8'(n),0);
    end
    for(int n=0;n<4;n++)begin
`ifdef DIRECT_FOUR_STATE
      logic [7:0] unknown_value;
      unknown_value=(n%2)?8'bzzzzzzzz:8'bxxxxxxxx;
      if(n<2)byte_row(unknown_value,n?8'd31:8'd17,1);
      else byte_row(n==2?8'd255:8'd0,unknown_value,1);
`endif
      byte_row(8'(n+3),8'(n+7),0);
    end
    $finish;
  end
`elsif DIRECT_BYTE_STATE
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
  logic [7:0] left=0,right=0,state_left=0,state_right=0;
  wire [15:0] result;
  bit last_clock=0;
  task byte_row(input bit clock_level,reset,input logic [7:0] lv,rv,input int mode);
    logic [7:0] next_left,next_right;
    pyc_7079635f727374=reset;left=lv;right=rv;#1;
    next_left=state_left+left;next_right=state_right+right;
    if(result !== {next_left,next_right})$fatal(1,"new-sum/independent state mismatch");
    if(mode==1)$display("WORK %b",result);if(mode==2)$display("MASK %b",result);
    if(clock_level&&!last_clock)begin
      if(reset)begin state_left=0;state_right=0;end
      else begin state_left=next_left;state_right=next_right;end
    end
    last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;
  endtask
  initial begin
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    byte_row(0,0,0,0,1);byte_row(1,0,1,10,1);byte_row(0,0,0,0,1);
    byte_row(1,0,2,0,1);byte_row(0,0,0,0,1);byte_row(1,0,253,1,1);
    byte_row(0,0,0,0,1);byte_row(1,0,0,245,1);
    byte_row(1,0,7,99,1);byte_row(1,0,8,1,1);byte_row(0,0,0,0,1);
    byte_row(1,0,13,29,1);byte_row(0,0,0,0,1);byte_row(1,0,3,0,1);
    byte_row(0,0,0,0,1);byte_row(1,0,0,200,1);byte_row(0,0,0,0,1);
    byte_row(1,1,9,11,1);byte_row(1,0,4,5,1);byte_row(0,0,0,0,1);
    byte_row(1,0,255,1,1);byte_row(0,0,0,0,1);
    for(int n=0;n<4;n++)begin
`ifdef DIRECT_FOUR_STATE
      logic [7:0] unknown_value;
      unknown_value=(n%2)?8'bzzzzzzzz:8'bxxxxxxxx;
      if(n<2)byte_row(0,0,unknown_value,n?8'd31:8'd17,2);
      else byte_row(0,0,n==2?8'd255:8'd0,unknown_value,2);
`endif
      byte_row(0,0,8'(n+3),8'(n+7),1);
    end
`ifdef DIRECT_FOUR_STATE
    byte_row(1,0,8'bxxxxxxxx,8'd7,0);
    byte_row(0,0,0,0,2);
`endif
    // Both two-/four-state paths commit this reset before printed recovery.
    byte_row(1,1,0,0,0);byte_row(0,0,2,3,1);
    byte_row(1,0,4,5,1);byte_row(0,0,0,0,1);
    $finish;
  end
`elsif DIRECT_FEEDBACK
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
  logic [72:0] delta=0;
  logic enable=0;
  wire [145:0] result;
  logic [72:0] state=0;
  bit last_clock=0;
  task row(input bit clock_level,reset,en,input logic [72:0] value,input integer mode);
    pyc_7079635f727374=reset;delta=value;enable=en;#1;
    if(result !== {state,state^value})$fatal(1,"direct-call old-Q/hold/reset/one-edge feedback mismatch");
    if(mode==1)$display("WORK %b",result);if(mode==2)$display("MASK %b",result);
    if(clock_level&&!last_clock)begin if(reset)state=0;else if(en)state=state^value;end
    last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;
  endtask
  initial begin
    bit [15:0] clocks=16'b0101011001010110;
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    for(int n=0;n<16;n++)row(clocks[n],n==9,n!=4&&n!=6,wide_value(n),1);
`ifdef DIRECT_FOUR_STATE
    for(int n=0;n<4;n++)begin
      logic [72:0] unknown_value;
      unknown_value=(n%2)?{73{1'bz}}:{73{1'bx}};
      row(0,0,1,unknown_value,0);row(1,0,1,unknown_value,0);row(0,0,0,73'd0,2);
      row(1,1,0,73'd0,0);row(0,0,0,73'd0,0);
    end
`endif
    $finish;
  end
`elsif DIRECT_LEXICAL
  logic [12:0] first,second;
  wire [116:0] result;
  initial begin
    for(int n=0;n<12;n++)begin
      first=13'(n*37);second=13'(n*101+17);#1;
      if(result !== {first,second,second,first,first,second,first,first,second})
        $fatal(1,"direct-call lexical early/late/call snapshots or local mutation mismatch");
      $display("WORK %b",result);
    end
    $finish;
  end
`else
  logic [72:0] wide;
`ifdef DIRECT_GRAPH
  logic [12:0] x,y;
  wire [197:0] result;
`else
  wire [437:0] result;
`endif
  task row(input logic [72:0] value,input logic [12:0] xv,yv,input bit unknown_case);
    wide=value;
`ifdef DIRECT_GRAPH
    x=xv;y=yv;#1;
    if(result !== {x,y,y,x,wide,wide})$fatal(1,"direct-call opposing field connections/zero-latency mismatch");
`else
    #1;
    if(result !== {wide,wide,wide,wide,73'd17,73'd17})$fatal(1,"direct-call nested/fanout/zeroarg value mismatch");
`endif
    if(unknown_case)$display("MASK %b",result);else $display("WORK %b",result);
  endtask
  initial begin
    for(int n=0;n<12;n++)row(wide_value(n),13'(n*37),13'(n*101+17),0);
`ifdef DIRECT_FOUR_STATE
    for(int n=0;n<4;n++)begin
      logic [72:0] value;
      value=wide_value(n);
      if(n<2)begin value[63]=(n==0)?1'bx:1'bz;value[64]=(n==0)?1'bx:1'bz;end
      else value=(n==2)?{73{1'bx}}:{73{1'bz}};
      row(value,13'(73+n),13'(171+n),1);
    end
`endif
    $finish;
  end
`endif
  pyc_root dut(.*);
endmodule
