module tb;
  logic pyc_7079635f636c6b=0, pyc_7079635f727374=1;
  logic left_enable=0, right_enable=0, index=0;
  logic [4:0] value=0;
  wire [106:0] result;
  pyc_root dut(.*);
  int owner[2], payload[2][2], ready[2][2], expected[25], actual[25];
  bit prior_clock=0;

  function automatic int field(input logic [45:0] child,input int ordinal);
    case(ordinal)
      0:return int'(child[45:41]); 1:return int'(child[40:36]);
      2:return int'(child[35]); 3:return int'(child[34]);
      4:return int'(child[33:29]); 5:return int'(child[28:24]);
      6:return int'(child[23:19]); 7:return int'(child[18:14]);
      8:return int'(child[13:9]); 9:return int'(child[8:5]);
      10:return int'(child[4:0]); default:return -1;
    endcase
  endfunction
  task sample_row(input bit clk,rst,left,right,idx,input int data);
    left_enable=left;right_enable=right;index=idx;value=5'(data);pyc_7079635f727374=rst;
    for(int side=0;side<2;side++)begin
      expected[side*11]=owner[side];
      expected[side*11+1]=(side ? right : left) ? data : owner[side];
      expected[side*11+2]=0;expected[side*11+3]=(side ? right : left) ? 0 : ready[side][idx];expected[side*11+4]=0;
      expected[side*11+5]=payload[side][idx];expected[side*11+6]=payload[side][idx ^ 1];
      expected[side*11+7]=0;expected[side*11+8]=data;expected[side*11+9]=9;
      expected[side*11+10]=owner[side];
    end
    expected[22]=(expected[1]+3)%32;expected[23]=(expected[1]+6)%32;
    expected[24]=(expected[12]+6)%32;
    #1;
    for(int f=0;f<11;f++)begin
      actual[f]=field(result[106:61],f);actual[f+11]=field(result[60:15],f);
    end
    actual[22]=int'(result[14:10]);actual[23]=int'(result[9:5]);actual[24]=int'(result[4:0]);
    for(int f=0;f<25;f++)if(actual[f]!==expected[f])
      $fatal(1,"defaults oracle field %0d got %0d expected %0d",f,actual[f],expected[f]);
    $write("WORK");for(int f=0;f<25;f++)$write(" %0d",actual[f]);$write("\n");
    pyc_7079635f636c6b=clk;#1;
    if(clk && !prior_clock)begin
      if(rst)begin for(int side=0;side<2;side++)begin owner[side]=0;
        for(int item=0;item<2;item++)begin payload[side][item]=7;ready[side][item]=1;end end end
      else for(int side=0;side<2;side++)if(side ? right : left)begin
        owner[side]=data;payload[side][idx]=data;ready[side][idx]=0;end
    end
    prior_clock=clk;
  endtask
  initial begin
    for(int side=0;side<2;side++)begin owner[side]=0;
      for(int item=0;item<2;item++)begin payload[side][item]=7;ready[side][item]=1;end end
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    for(int n=0;n<64;n++)sample_row(1'(n%2),n==31,n%3!=0,n%5<2,1'((n/2)%2),(n*7+13)%32);
    sample_row(1,0,1,0,0,17);sample_row(1,0,0,1,1,29);
    sample_row(0,0,1,1,0,11);sample_row(0,0,0,1,1,19);
    sample_row(1,0,1,1,0,31);sample_row(1,0,1,1,1,23);
    sample_row(0,0,0,0,0,0);sample_row(1,0,0,0,1,0);
    $finish;
  end
endmodule
