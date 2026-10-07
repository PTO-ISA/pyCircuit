// Independent eleven-tick/44-enabled-edge table. Observe before clock changes.
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,go=1,emergency=0;
  wire [21:0] result;
  integer position=0;
  bit last_clock=0;
  pyc_root dut(.*);
  function automatic logic [8:0] tick(input integer index);
    case(index)
      0:return {2'd0,3'd3,3'd4,1'b0};1:return {2'd0,3'd2,3'd3,1'b0};
      2:return {2'd0,3'd1,3'd2,1'b0};3:return {2'd0,3'd0,3'd1,1'b0};
      4:return {2'd1,3'd1,3'd0,1'b0};5:return {2'd1,3'd0,3'd0,1'b1};
      6:return {2'd2,3'd3,3'd2,1'b0};7:return {2'd2,3'd2,3'd1,1'b0};
      8:return {2'd2,3'd1,3'd0,1'b0};9:return {2'd3,3'd0,3'd1,1'b0};
      10:return {2'd3,3'd0,3'd0,1'b1};
      default:$fatal(1,"invalid independent tick index");
    endcase
  endfunction
  function automatic logic [7:0] display_value(input logic [2:0] count);
    case(count)
      0:return 8'h48;1:return 8'h49;2:return 8'h58;3:return 8'h59;
      4:return 8'h5a;5:return 8'h5b;6:return 8'h5c;7:return 8'h5d;
    endcase
  endfunction
  function automatic logic [21:0] expected(input integer point,input logic override_value);
    logic [8:0] state_value;
    logic [1:0] phase;
    state_value=tick(point/4);phase=state_value[8:7];
    return override_value ? {8'h88,8'h88,6'b100100} :
        {display_value(state_value[6:4]),display_value(state_value[3:1]),
         phase>=2,(phase==1)&state_value[0],phase==0,phase<2,
         (phase==3)&state_value[0],phase==2};
  endfunction
  task row(input bit clock_level,reset,go_value,emergency_value);
    pyc_7079635f727374=reset;go=go_value;emergency=emergency_value;#1;
    if(result !== expected(position,emergency_value))
      $fatal(1,"TrafficLights independent table/displays/lights/hold/reset oracle failed");
    $display("WORK %0d %0d %0d %0d %0d %0d %0d %0d",result[21:14],result[13:6],
        result[5],result[4],result[3],result[2],result[1],result[0]);
    if(clock_level && !last_clock)begin
      if(reset)position=0;
      else if(go_value && !emergency_value)position=(position+1)%44;
    end
    last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;
  endtask
  task edges(input integer count,input bit go_value,emergency_value);
    for(integer n=0;n<count;n=n+1)begin row(0,0,go_value,emergency_value);row(1,0,go_value,emergency_value);end
  endtask
  task reset_trace;
    row(0,1,1,0);row(1,1,1,0);row(0,0,1,0);
  endtask
`ifdef PYC_TRAFFIC_LIGHTS_FOUR_STATE
  task four_check(input logic [21:0] golden);
    if(result !== golden)$fatal(1,"TrafficLights X/Z observable mask/recovery failed: got%b expected%b",result,golden);
  endtask
  task four_edge;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
  endtask
  task four_reset;
    pyc_7079635f636c6b=0;pyc_7079635f727374=1;go=1;emergency=0;#1;
    four_edge();pyc_7079635f727374=0;#1;four_check(expected(0,0));
  endtask
  task four_case(input integer scenario,input logic selector);
    logic [21:0] merge;
    four_reset();
    for(integer n=0;n<(scenario==2 ? 3 : 22);n=n+1)four_edge();
    if(scenario<2)begin
      if(scenario==0)begin go=selector;emergency=1;end
      else begin go=0;emergency=selector;end
      #1;merge=expected(22,scenario==0 ? 1'b1 : selector);four_check(merge);
      for(integer n=0;n<4;n=n+1)begin four_edge();four_check(merge);end
      go=1;emergency=0;#1;four_check(expected(22,0));
      four_edge();four_check(expected(23,0));four_edge();four_check(expected(24,0));
    end else begin
      go=selector;#1;four_check(expected(3,0));four_edge();
      four_check({8'b0xxxxxxx,8'b0xxxxxxx,6'b001100});
      go=0;#1;
      for(integer n=0;n<3;n=n+1)begin
        four_edge();four_check({8'b0xxxxxxx,8'b0xxxxxxx,6'b001100});
      end
      emergency=1;#1;four_check(expected(0,1));
      emergency=0;go=selector;pyc_7079635f727374=1;#1;
      four_check({8'b0xxxxxxx,8'b0xxxxxxx,6'b001100});four_edge();
      go=1;pyc_7079635f727374=0;#1;four_check(expected(0,0));
      for(integer n=1;n<=4;n=n+1)begin four_edge();four_check(expected(n,0));end
    end
    $display("FOUR traffic_lights scenario%0d selector%b",scenario,selector);
  endtask
`endif
  initial begin
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    edges(88,1,0);row(0,0,1,0);
    reset_trace();edges(2,1,0);edges(5,0,0);row(1,0,1,0);edges(2,1,0);
    reset_trace();edges(18,1,0);edges(5,0,0);edges(4,1,0);edges(5,0,0);
    row(1,0,1,1);row(1,0,1,0);row(0,0,1,1);edges(8,1,1);row(0,0,1,0);
    edges(16,1,0);edges(5,0,0);edges(4,1,0);edges(5,0,0);edges(2,1,0);
    edges(7,1,0);row(0,1,0,1);row(1,1,0,1);row(1,0,1,0);row(0,0,1,0);
    edges(5,1,0);row(1,0,0,0);row(1,0,1,0);row(0,0,0,0);
    row(0,1,1,0);row(1,1,0,0);row(0,0,1,0);
    // Populated yellow/blink reset also proves reset has priority over pause.
    edges(22,1,0);row(0,1,0,0);row(1,1,0,0);row(0,0,1,0);
`ifdef PYC_TRAFFIC_LIGHTS_FOUR_STATE
    four_case(0,1'bx);four_case(0,1'bz);four_case(1,1'bx);
    four_case(1,1'bz);four_case(2,1'bx);four_case(2,1'bz);
`endif
    $finish;
  end
endmodule
