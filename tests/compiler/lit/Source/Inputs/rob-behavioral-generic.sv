// Physical IR pyc_clk/pyc_rst use the existing injective generated identifier
// spelling: pyc_7079635f636c6b / pyc_7079635f727374.
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,enable=0;
  logic [2:0] index=0;
  logic [6:0] value=0;
  wire [6:0] prior_payload;
  wire active;
  wire [2:0] cursor;
  wire [8:0] audit;
  wire [2:0] snapshot;
  pyc_root dut(.*);
  int golden_values[8],golden_active[8],golden_cursor=3,golden_audit=341;
  task row(input bit clock_level,en,input int idx,val);
    int nc,na;
    enable=en;index=3'(idx);value=7'(val);
    nc=(golden_cursor+int'(en))%8;na=(golden_audit+5*int'(en))%512;
    #1;
    if(prior_payload!==7'(golden_values[idx]) || active!==1'(golden_active[idx]) ||
       cursor!==3'(nc) || audit!==9'(na) || snapshot!==3'((golden_cursor+2)%8))
      $fatal(1,"independent depth8 table golden failed");
    $display("WORK %0d %0d %0d %0d %0d",prior_payload,active,cursor,audit,snapshot);
    if(clock_level && !pyc_7079635f636c6b && en)begin
      golden_values[idx]=val;golden_active[idx]=1;golden_cursor=nc;golden_audit=na;
    end
    pyc_7079635f636c6b=clock_level;#1;
  endtask
  initial begin
    for(int i=0;i<8;i++)begin golden_values[i]=0;golden_active[i]=0;end
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    for(int i=0;i<128;i++)begin row(0,0,i%8,i);row(1,1,i%8,i);end
    row(1,0,0,0);row(1,1,0,67);row(0,0,0,0);
    row(0,1,0,88);row(1,1,0,99);row(0,0,0,0);
    if(golden_values[0]!=99 || golden_cursor!=4 || golden_audit!=474)
      $fatal(1,"independent table tail oracle failed");
    $finish;
  end
endmodule
