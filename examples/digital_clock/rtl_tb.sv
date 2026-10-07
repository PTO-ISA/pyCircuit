// Independent calendar oracle. Observe old Q before applying the next clock level.
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
  logic btn_set=0,btn_plus=0,btn_minus=0;
  wire [26:0] result;
  pyc_root dut(.*);
  localparam longint unsigned PERIOD=50000000;
  longint unsigned epochs=0,edge_count=0,ticks=0,midnight=0,held_tick=0;
  integer time_of_day=0,mode=0,blink=0;
  bit last_clock=0;
  function automatic integer bcd(input integer value);
    return (value/10)*16+value%10;
  endfunction
  function automatic logic [26:0] expected;
    return {8'(bcd(time_of_day/3600)),8'(bcd((time_of_day/60)%60)),
            8'(bcd(time_of_day%60)),2'(mode),1'(blink)};
  endfunction
  task row(input bit clock_level,reset,set_value,plus_value,minus_value,input bit emit);
    integer unit_value,modulus,old_field,new_field;
    btn_set=set_value;btn_plus=plus_value;btn_minus=minus_value;
    pyc_7079635f727374=reset;#1;
    epochs=epochs+1;
    if(result !== expected())$fatal(1,"DigitalClock old-Q/calendar oracle epoch %0d got %b expected %b",epochs,result,expected());
    if(emit)$display("WORK %0d %0d %0d %0d %0d %0d",epochs,result[26:19],result[18:11],result[10:3],result[2:1],result[0]);
    if(clock_level && !last_clock)begin
      if(reset)begin time_of_day=0;mode=0;blink=0;edge_count=0;ticks=0;midnight=0;held_tick=0;end
      else begin
        edge_count=edge_count+1;
        if(edge_count%PERIOD==0)begin
          ticks=ticks+1;blink=blink^1;
          if(mode==0)begin
            if(time_of_day==86399)midnight=midnight+1;
            time_of_day=(time_of_day+1)%86400;
          end else held_tick=held_tick+1;
        end
        if(mode!=0 && (minus_value || plus_value))begin
          unit_value=mode==1 ? 3600 : mode==2 ? 60 : 1;
          modulus=mode==1 ? 24 : 60;
          old_field=(time_of_day/unit_value)%modulus;
          new_field=(old_field+(minus_value ? modulus-1 : 1))%modulus;
          time_of_day=time_of_day-old_field*unit_value+new_field*unit_value;
        end
        if(set_value)mode=(mode+1)%4;
      end
    end
    last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;
  endtask
  task edges(input integer count,input bit set_value,plus_value,minus_value);
    for(integer n=0;n<count;n=n+1)begin
      row(0,0,set_value,plus_value,minus_value,1);row(1,0,set_value,plus_value,minus_value,1);
    end
  endtask
  task reset_trace;
    row(0,1,1,1,1,1);row(1,1,1,1,1,1);row(0,0,0,0,0,1);
  endtask
  task short_trace;
    reset_trace();edges(1,1,0,0);
    edges(1,0,0,1);edges(1,0,1,0);edges(24,0,1,0);edges(1,0,1,1);
    edges(1,1,1,0);edges(1,0,0,1);edges(1,0,1,0);edges(60,0,1,0);
    edges(1,0,1,1);edges(1,1,0,1);
    edges(1,0,0,1);edges(1,0,1,0);edges(60,0,1,0);edges(1,0,1,1);
    edges(1,1,1,1);edges(2,0,1,1);
    row(1,0,1,1,1,1);row(1,1,1,1,1,1);row(0,1,1,1,1,1);
    row(0,0,1,1,1,1);row(0,1,1,1,1,1);row(1,1,1,1,1,1);
    row(1,0,1,1,1,1);row(0,0,0,0,0,1);reset_trace();
  endtask
`ifdef PYC_DIGITAL_CLOCK_FOUR_STATE
  task four_edge;
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
  endtask
  task four_reset;
    pyc_7079635f636c6b=0;pyc_7079635f727374=1;btn_set=0;btn_plus=0;btn_minus=0;#1;
    four_edge();pyc_7079635f727374=0;#1;
    if(result !== 27'b0)$fatal(1,"DigitalClock X/Z reset recovery failed");
  endtask
  task four_case(input integer scenario,input logic selector);
    logic [26:0] golden;
    four_reset();
    if(scenario==1 || scenario==2)begin btn_set=1;four_edge();btn_set=0;end
    if(scenario==4)begin btn_set=1;repeat(3)four_edge();btn_set=0;end
    #1;golden=27'(scenario==1 || scenario==2 ? 2 : scenario==4 ? 6 : 0);
    if(scenario==0)begin btn_plus=selector;btn_minus=selector;end
    if(scenario==1)begin btn_plus=selector;golden[26:19]='x;end
    if(scenario==2)begin btn_plus=selector;btn_minus=1;golden[26:19]=8'h23;end
    if(scenario==3)begin btn_set=selector;golden[1]=1'bx;end
    if(scenario==4)begin btn_minus=selector;golden[10:3]='x;end
    #1;four_edge();
    if(result !== golden)$fatal(1,"DigitalClock data-MUX four-state scenario %0d selector %b got%b expected%b",scenario,selector,result,golden);
    btn_set=1;btn_plus=1;btn_minus=1;pyc_7079635f727374=1;#1;four_edge();
    pyc_7079635f727374=0;btn_set=0;btn_plus=0;btn_minus=0;#1;
    if(result !== 27'b0)$fatal(1,"DigitalClock known reset did not recover all six owners");
    $display("FOUR DigitalClock scenario %0d selector %b",scenario,selector);
  endtask
`endif
  initial begin
    longint unsigned edge_limit,local_epoch,edge_number,total_epochs;
    bit is_pilot,set_value,minus_value,emit;
    // Prime RTL reset before the counted stream, matching native executor.Reset.
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    short_trace();
`ifdef PYC_DIGITAL_CLOCK_FOUR_STATE
    for(integer scenario=0;scenario<5;scenario=scenario+1)begin
      four_case(scenario,1'bx);four_case(scenario,1'bz);
    end
`else
    edge_limit=2*PERIOD;is_pilot=$value$plusargs("pilot_edges=%d",edge_limit);
    if(is_pilot && (edge_limit<4 || edge_limit>=PERIOD))$fatal(1,"pilot must be a bounded real-edge prefix");
    total_epochs=epochs+2*edge_limit+1;
    for(local_epoch=0;local_epoch<=2*edge_limit;local_epoch=local_epoch+1)begin
      edge_number=local_epoch/2+1;
      set_value=edge_number<=4 || (edge_number>PERIOD && edge_number<=PERIOD+3) || edge_number==2*PERIOD;
      minus_value=edge_number>=2 && edge_number<=4;
      emit=local_epoch<10 || local_epoch==2*edge_limit || edge_number%1000000==0 || edge_number==PERIOD+1 || edge_number==2*PERIOD+1;
      row(1'(local_epoch%2),0,set_value,0,minus_value,emit);
    end
    if(epochs!=total_epochs || edge_count!=edge_limit || ticks!=(is_pilot ? 0 : 2) ||
       midnight!=(is_pilot ? 0 : 1) || held_tick!=(is_pilot ? 0 : 1) ||
       time_of_day!=(is_pilot ? 86399 : 0) || mode!=0 || blink!=0)
      $fatal(1,"DigitalClock incomplete real-edge/two-tick gate");
    $display("WORK summary %0d %0d %0d %0d %0d %0d",epochs,edge_count,ticks,midnight,held_tick,is_pilot);
`endif
    $finish;
  end
endmodule
