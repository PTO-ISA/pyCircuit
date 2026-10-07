module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
  logic RST_BTN=0,START=0,left=0,right=0;
  wire [49:0] result;
  pyc_root dut(.*);
  bit last_clock=0;
  integer sampled=0,age=0,state=0,active_ticks=0,player=8;
  integer tick_count=0,left_stops=0,right_stops=0,both_held=0;
  integer resets=0,j20=0,h_wrap=0,blue_samples=0,blank_samples=0;
  function automatic integer object_y(input integer ticks,first);
    integer partial;
    partial=(ticks%32)-first;
    if(partial<0)partial=0;
    if(partial>12)partial=12;
    return (12*(ticks/32)+partial)&15;
  endfunction
  task row(input bit c,r,button,start_level,left_level,right_level);
    integer pixels,h,v,blue;
    bit hs;
    logic [49:0] expected;
    pixels=age>0?(age-1)/4:0;h=pixels%801;v=pixels/801;
    if(v>1||state>1)$fatal(1,"game oracle bounded scenario");
    hs=h<16||h>=112;blue=v==1&&h>160&&h<800?8:0;
    expected={hs,1'b1,4'b0,4'b0,4'(blue),3'(state),5'(active_ticks&31),
              4'(player),4'd1,4'(object_y(active_ticks,1)),
              4'd4,4'(object_y(active_ticks,4)),4'd7,4'(object_y(active_ticks,8))};
    pyc_7079635f727374=r;RST_BTN=button;START=start_level;
    left=left_level;right=right_level;#1;
    if(result!==expected)$fatal(1,"game row %0d age %0d closed-form oracle: %h != %h",sampled,age,result,expected);
    $display("WORK %0d %0d",sampled,result);
    blank_samples=blank_samples+(!hs);blue_samples=blue_samples+(blue!=0);
    h_wrap=h_wrap+(age==3205);
    if(c&&!last_clock)begin
      if(r)begin age=0;state=0;active_ticks=0;player=8;end
      else begin
        if((age&31)==15)begin
          tick_count=tick_count+1;
          if(state==1)begin
            if((player==1&&object_y(active_ticks,1)==10)||
               (player==4&&object_y(active_ticks,4)==10)||
               (player==7&&object_y(active_ticks,8)==10))
              $fatal(1,"game stimulus unexpected collision coordinate");
            j20=j20+((active_ticks&31)==20);active_ticks=active_ticks+1;
            if(left_level&&!right_level)begin
              left_stops=left_stops+(player==0);if(player>0)player=player-1;
            end
            if(right_level&&!left_level)begin
              right_stops=right_stops+(player==15);if(player<15)player=player+1;
            end
            both_held=both_held+(left_level&&right_level);
            if(button)begin state=0;resets=resets+1;end
          end else if(start_level)state=1;
        end
        age=age+1;
      end
    end
    last_clock=c;sampled=sampled+1;pyc_7079635f636c6b=c;#1;
  endtask
  bit s,b,l,a;
  initial begin
    // Synchronous initialization matches SystemRunner's host Reset snapshot.
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
    row(0,1,0,0,0,0);row(1,1,0,0,0,0);row(0,1,0,0,0,0);
    for(integer n=0;n<4000;n=n+1)begin
      s=n<16||(n>=928&&n<960)||(n>=992&&n<1024);
      b=(n>=896&&n<928)||(n>=960&&n<1024);
      l=(n>=16&&n<320)||(n>=832&&n<896);
      a=n>=320&&n<896;
      row(1,0,b,s,l,a);
      if(n==31)row(1,1,1,1,1,1);
      row(0,0,b,s,l,a);
      if(n==32)row(0,1,1,1,1,1);
    end
    if(sampled>=8192||age!=4000||state!=1||player!=15||tick_count!=125||
       left_stops==0||right_stops==0||both_held!=2||resets!=2||j20<3||
       h_wrap==0||blue_samples==0||blank_samples==0)$fatal(1,"game bounded coverage");
    $display("HISTORY %0d %0d %0d %0d",tick_count,active_ticks,player,resets);
    $finish;
  end
endmodule
