// Host-style signed arithmetic oracle, sampled before every physical clock change.
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
  logic [15:0] x_in=0;
  logic x_valid=0;
  wire [34:0] result;
  longint signed delay1=0,delay2=0,delay3=0;
  logic [33:0] expected_data=0;
  bit expected_valid=0,last_clock=0;
  logic [31:0] random_state=32'h9273a61d;
  pyc_root dut(.*);
  function automatic longint signed signed16(input logic [15:0] raw);
    return raw < 16'd32768 ? longint'(raw) : longint'(raw)-64'sd65536;
  endfunction
  task row(input bit clock_level,reset,input logic [15:0] raw,input bit valid);
    longint signed sample_value,total;
    pyc_7079635f727374=reset;x_in=raw;x_valid=valid;#1;
    if(result !== {expected_data,expected_valid})
      $fatal(1,"DigitalFilter signed FIR/old-Q/hold/valid/reset oracle failed");
    $display("WORK %0d %0d",result[34:1],result[0]);
    if(clock_level && !last_clock)begin
      if(reset)begin
        delay1=0;delay2=0;delay3=0;expected_data=0;expected_valid=0;
      end else begin
        if(valid)begin
          sample_value=signed16(raw);
          total=sample_value+2*delay1+3*delay2+4*delay3;
          expected_data=total[33:0];
          delay3=delay2;delay2=delay1;delay1=sample_value;
        end
        expected_valid=valid;
      end
    end
    last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;
  endtask
  task scenario(input integer which);
    integer sample_value;
    row(0,1,999,1);row(1,1,999,1);row(0,0,0,0);
    for(integer n=0;n<8;n=n+1)begin
      case(which)
        0:sample_value=n==0 ? 1 : 0;
        1:sample_value=n==0 ? -1 : 0;
        2:sample_value=1;
        3:sample_value=n;
        4:sample_value=n%2==0 ? 100 : -100;
        5:sample_value=n<4 ? 10000 : 0;
        6:sample_value=n<4 ? 32767 : 0;
        7:sample_value=n<4 ? -32768 : 0;
        default:$fatal(1,"invalid test scenario");
      endcase
      row(0,0,16'(sample_value),1);row(1,0,16'(sample_value),1);
    end
    row(0,0,12345,0);
  endtask
`ifdef PYC_DIGITAL_FILTER_FOUR_STATE
  task four_check(input logic [33:0] data,input logic valid);
    if(result !== {data,valid})
      $fatal(1,"DigitalFilter exact observable X/Z planes or recovery failed: got%b expected%b",result,{data,valid});
  endtask
  task four_reset;
    pyc_7079635f636c6b=0;pyc_7079635f727374=1;x_valid=0;x_in=0;#1;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    four_check(34'd0,1'b0);
  endtask
  task four_edge;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
  endtask
  task four_case(input integer which,input logic selector,input logic [15:0] unknown_data);
    logic [33:0] held;
    four_reset();
    if(which<2)begin
      x_valid=selector;x_in=16'(which);#1;four_check(34'd0,1'b0);
      four_edge();held=which==0 ? 34'd0 : {33'd0,1'bx};
      // Raw valid preserves Z, whereas the data mux produces X only for a
      // differing bit. Equal zero/zero branches remain completely known.
      four_check(held,selector);#1;four_check(held,selector);
    end else begin
      x_in=unknown_data;x_valid=0;#1;four_edge();four_check(34'd0,1'b0);
      x_valid=1;#1;four_edge();held={34{1'bx}};four_check(held,1'b1);
    end
    // Invalid edges neither capture X/Z input nor flush contaminated history.
    x_in={16{1'bz}};x_valid=0;#1;
    four_edge();four_check(held,1'b0);four_edge();four_check(held,1'b0);
    x_in=0;x_valid=1;#1;
    for(integer edge_count=1;edge_count<=4;edge_count=edge_count+1)begin
      four_edge();
      if(which==0 || edge_count==4)four_check(34'd0,1'b1);
      else four_check({34{1'bx}},1'b1);
    end
    // Recontaminate, then reset must dominate even an uncertain valid selector.
    x_in=unknown_data;#1;four_edge();four_check({34{1'bx}},1'b1);
    x_valid=selector;pyc_7079635f727374=1;#1;four_edge();four_check(34'd0,1'b0);
    pyc_7079635f727374=0;x_in=16'hffff;x_valid=1;#1;
    four_edge();four_check({34{1'b1}},1'b1);
    $display("FOUR digital_filter case%0d selector%b recovery-minus1",which,selector);
  endtask
`endif
  initial begin
    // Match native cold Reset before the compared Work epochs.
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    for(integer scenario_index=0;scenario_index<8;scenario_index=scenario_index+1)
      scenario(scenario_index);
    row(0,1,999,1);row(1,1,999,1);row(0,0,0,0);
    row(0,0,3,1);row(1,0,3,1);row(0,0,-7,1);row(1,0,-7,1);
    row(0,0,11,1);row(1,0,11,1);row(0,0,222,0);
    for(integer n=0;n<32;n=n+1)begin
      row(0,0,16'(n*7919),0);row(1,0,16'(65535-n*3571),0);
    end
    row(1,0,32767,1);row(1,1,-32768,1);row(0,1,1234,1);
    row(0,0,-1234,1);row(1,0,-17,1);row(0,0,101,0);
    row(1,1,4567,1);row(1,0,-4567,1);
    row(0,0,-123,1);row(1,0,-123,1);row(1,0,123,1);
    row(0,0,456,1);row(0,0,789,0);row(1,0,999,0);row(0,0,111,0);
    for(integer n=0;n<64;n=n+1)begin
      random_state=random_state*32'd1664525+32'd1013904223;
      row(0,0,random_state[31:16],n%7!=0);row(1,0,random_state[31:16],n%7!=0);
    end
    row(0,0,0,0);
`ifdef PYC_DIGITAL_FILTER_FOUR_STATE
    four_case(0,1'bx,16'b00000000x0000001);
    four_case(0,1'bz,16'b00000000z0000001);
    four_case(1,1'bx,16'b00000000x0000001);
    four_case(1,1'bz,16'b00000000z0000001);
    four_case(2,1'bx,16'b00000000x0000001);
    four_case(2,1'bz,16'b00000000z0000001);
    four_case(3,1'bx,{16{1'bx}});
    four_case(3,1'bz,{16{1'bz}});
`endif
    $finish;
  end
endmodule
