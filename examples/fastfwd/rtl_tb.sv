// Independent declaration-order packing and full-port combinational oracle.
module tb;
  logic [133:0] lane0,lane1,lane2,lane3;
  logic [128:0] engine0,engine1,engine2,engine3;
  wire [1556:0] result;
  logic [133:0] lanes[4];
  logic [128:0] engines[4];
  logic [1556:0] expected;
  pyc_root dut(.*);
  function automatic logic [127:0] payload(input integer epoch,path);
    logic [63:0] low_word,high_word;
    low_word=64'h0123456789abcdef ^ (64'(epoch)*64'h0000000100000001) ^
             (64'(path)*64'h0101010101010101);
    high_word=64'hfedcba9876543210 ^ (64'(epoch)*64'ha500000000000001) ^
              (64'(path)*64'h0f0f0f0f0f0f0f0f);
    if(epoch==0)begin low_word=0;high_word=0;end
    if(epoch==1)begin low_word=64'hffffffffffffffff;high_word=0;end
    if(epoch==2)begin low_word=0;high_word=64'hffffffffffffffff;end
    if(epoch==3)begin low_word=0;high_word=64'd1 << ((path*9)%64);end
    if(epoch==4)begin low_word=64'd1 << ((path*9)%64);high_word=0;end
    if(epoch==5)begin low_word=64'haaaaaaaaaaaaaaaa ^ 64'(path);high_word=64'h5555555555555555 ^ 64'(path);end
    return {high_word,low_word};
  endfunction
  function automatic logic validity(input integer epoch,path);
    if(epoch<8)return 1'((epoch+path)%2);
    if(epoch<16)return path==epoch-8;
    return path!=epoch-16;
  endfunction
  task known_frame(input integer epoch);
    for(integer n=0;n<4;n=n+1)begin
      lanes[n]={validity(epoch,n),payload(epoch,n),5'((epoch*7+n*11)%32)};
      engines[n]={validity(epoch,n+4),payload(epoch,n+4)};
    end
  endtask
  task check_frame(input integer mode);
    lane0=lanes[0];lane1=lanes[1];lane2=lanes[2];lane3=lanes[3];
    engine0=engines[0];engine1=engines[1];engine2=engines[2];engine3=engines[3];#1;
    // Control bits alone disappear. Valid/data survive even when valid is zero,
    // and every engine's latency/datapath fields remain known zero.
    expected={1'b0,lanes[0][133:5],lanes[1][133:5],lanes[2][133:5],lanes[3][133:5],
              engines[0],2'b0,1'b0,128'b0,engines[1],2'b0,1'b0,128'b0,
              engines[2],2'b0,1'b0,128'b0,engines[3],2'b0,1'b0,128'b0};
    if(result !== expected)$fatal(1,"Fastfwd full-width/invalid-data/constants/no-delay oracle failed");
    if(mode==1)$display("WORK %b",result);
    if(mode==2)$display("MASK %b",result);
  endtask
`ifdef PYC_FASTFWD_FOUR_STATE
  task four_frame(input integer mode);
    known_frame(10+mode);
    for(integer n=0;n<4;n=n+1)begin
      lanes[n][4:0]=(mode%2)?5'bzzzzz:5'bxxxxx;
      if(mode>=2)begin
        lanes[n][133]=((mode+n)%2)?1'bz:1'bx;
        engines[n][128]=((mode+n+1)%2)?1'bz:1'bx;
        if(mode==2)begin
          lanes[n][5]=1'bx;lanes[n][132]=1'bz;
          engines[n][0]=1'bx;engines[n][127]=1'bz;
        end
        if(mode==3)begin lanes[n][132:5]={128{1'bz}};engines[n][127:0]={128{1'bz}};end
        if(mode==4)begin lanes[n][132:5]={128{1'bx}};engines[n][127:0]={128{1'bx}};end
        if(mode==5)begin
          for(integer bit_index=0;bit_index<128;bit_index=bit_index+1)begin
            if(bit_index<64 && bit_index%2==1)begin
              lanes[n][bit_index+5]=1'bz;engines[n][bit_index]=1'bz;
            end
            if(bit_index>=64 && bit_index%2==0)begin
              lanes[n][bit_index+5]=1'bx;engines[n][bit_index]=1'bx;
            end
          end
        end
      end
    end
    check_frame(2);
    // Recovery uses the next combinational sample, with no reset/clock phase.
    known_frame(23-mode);check_frame(0);
  endtask
`endif
  initial begin
    for(integer epoch=0;epoch<24;epoch=epoch+1)begin known_frame(epoch);check_frame(1);end
`ifdef PYC_FASTFWD_FOUR_STATE
    for(integer mode=0;mode<6;mode=mode+1)four_frame(mode);
    $display("FOUR fastfwd 6 X/Z full-width transport/control-ignore/known-recovery cases");
`endif
    $finish;
  end
endmodule
