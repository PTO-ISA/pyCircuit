module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,valid_in=0;
  logic [15:0] a_in=0,b_in=0;
  logic [31:0] acc_in=0;
  wire [32:0] result;
  pyc_root dut(.*);
  logic [63:0] cases[0:51];
  bit last_clock=0,output_valid=0,stage_valid[0:2];
  logic [31:0] output_data=0,stage_data[0:2];
  integer sampled=0,accepted=0,retired=0,dropped=0,peak=0;
  integer holes=0,holds=0,reset_full=0,repeated=0;

  function automatic logic [31:0] evaluated(input logic[63:0] operands);
    logic [15:0] a,b;
    logic [31:0] acc;
    integer unsigned ae,be,ce,product,exponent,pm,cm,distance,shift;
    integer unsigned magnitude,normalized,packed_exp,scan;
    integer highest,displacement;
    bit ps,cs,sign;
    a=operands[63:48];b=operands[47:32];acc=operands[31:0];
    ae=(a>>7)&255;be=(b>>7)&255;ce=(acc>>23)&255;
    if(ae==0||be==0)return ce!=0?acc:32'b0;
    product=(128+(a&127))*(128+(b&127));
    exponent=(ae+be+1024-127)&1023;
    if((product&32768)!=0)begin product=product>>1;exponent=(exponent+1)&1023;end
    exponent=exponent&255;
    pm=product<<9;cm=ce!=0?(acc&32'h7fffff)|32'h800000:0;
    distance=exponent>ce?exponent-ce:ce-exponent;
    shift=distance>26?26:distance;
    if(exponent>ce)cm=cm>>shift;else pm=pm>>shift;
    ps=((a^b)&16'h8000)!=0;cs=acc[31];sign=ps;
    if(ps==cs)magnitude=(pm+cm)&32'h3ffffff;
    else if(pm>=cm)magnitude=pm-cm;
    else begin magnitude=cm-pm;sign=cs;end
    if(magnitude==0)return 0;
    // Numerical bit length, independent of the source priority-mux LZC.
    scan=magnitude;highest=0;
    while(scan>1)begin scan=scan>>1;highest=highest+1;end
    displacement=highest-23;
    normalized=displacement>0?magnitude>>displacement:magnitude<<(-displacement);
    packed_exp=((exponent>ce?exponent:ce)+1024+displacement)&255;
    return {sign,8'(packed_exp),23'(normalized)};
  endfunction
  task row(input bit c,r,v,input logic[63:0] operands);
    integer occupancy;
    pyc_7079635f727374=r;valid_in=v;
    a_in=operands[63:48];b_in=operands[47:32];acc_in=operands[31:0];#1;
    if(result!=={output_data,output_valid})
      $fatal(1,"fmac row %0d four-boundary oracle %h != %h",sampled,result,{output_data,output_valid});
    $display("WORK %0d %0d %0d",sampled,output_data,output_valid);
    repeated=repeated+(c==last_clock);
    if(c&&!last_clock)begin
      occupancy=0;
      for(integer k=0;k<3;k=k+1)occupancy=occupancy+stage_valid[k];
      if(r)begin
        reset_full=reset_full+(occupancy==3);dropped=dropped+occupancy;
        for(integer k=0;k<3;k=k+1)begin stage_valid[k]=0;stage_data[k]=0;end
        output_data=0;output_valid=0;
      end else begin
        output_valid=stage_valid[2];
        if(output_valid)begin output_data=stage_data[2];retired=retired+1;end
        else begin holes=holes+1;holds=holds+(output_data!=0);end
        stage_valid[2]=stage_valid[1];stage_data[2]=stage_data[1];
        stage_valid[1]=stage_valid[0];stage_data[1]=stage_data[0];
        stage_valid[0]=v;stage_data[0]=evaluated(operands);accepted=accepted+v;
        occupancy=0;
        for(integer k=0;k<3;k=k+1)occupancy=occupancy+stage_valid[k];
        if(occupancy>peak)peak=occupancy;
      end
    end
    last_clock=c;sampled=sampled+1;pyc_7079635f636c6b=c;#1;
  endtask
  task edge_row(input logic[63:0] operands,input bit v=1,r=0,held=0);
    row(1,r,v,operands);
    if(held)begin row(1,1,1,cases[6]);row(1,0,0,cases[7]);end
    row(0,r,v,operands);
    if(held)begin row(0,1,1,cases[8]);row(0,0,0,cases[9]);end
  endtask
  initial begin
    cases[0]={16'h0000,16'h0000,32'h00000000};
    cases[1]={16'h0000,16'h3f80,32'h3f812345};
    cases[2]={16'h8000,16'h3f80,32'hc0000000};
    cases[3]={16'h007f,16'h3fff,32'hbf800000};
    cases[4]={16'h3f80,16'h3f80,32'h00000000};
    cases[5]={16'h3f80,16'h3f80,32'h3f800000};
    cases[6]={16'hbf80,16'h3f80,32'h00000000};
    cases[7]={16'h3f80,16'hbf80,32'h3f800000};
    cases[8]={16'h3f80,16'h3f80,32'hc0000000};
    cases[9]={16'hbf80,16'hbf80,32'hbf800000};
    cases[10]={16'h3fc0,16'h3fc0,32'h00000000};
    cases[11]={16'h3fff,16'h3fff,32'h00000000};
    cases[12]={16'h3f81,16'h3f81,32'h00000000};
    cases[13]={16'h3f80,16'h3f80,32'h4c000000};
    cases[14]={16'h3f80,16'h3f80,32'h4c800000};
    cases[15]={16'h3f80,16'h3f80,32'h4d000000};
    cases[16]={16'h0080,16'h0080,32'h00000000};
    cases[17]={16'h7f00,16'h7f00,32'h00000000};
    cases[18]={16'h3f80,16'h3f80,32'h007fffff};
    cases[19]={16'h3f80,16'h3f80,32'h80000000};
    for(integer i=0;i<32;i=i+1)
      cases[20+i]={16'(((i&1)<<15)|((90+i*7%71)<<7)|(i*19%128)),
                    16'((((i>>1)&1)<<15)|((90+i*11%71)<<7)|(i*37%128)),
                    32'((((i>>2)&1)<<31)|((90+i*13%71)<<23)|((32'(i)*32'h9e3779b1)&32'h7fffff))};
    if(evaluated(cases[4])!==32'h3f800000||evaluated(cases[5])!==32'h40000000||
       evaluated(cases[6])!==32'hbf800000||evaluated(cases[7])!==32'h0||
       evaluated(cases[8])!==32'hbf800000||evaluated(cases[10])!==32'h40100000||
       evaluated(cases[11])!==32'h407e0000||evaluated(cases[12])!==32'h3f820200||
       evaluated(cases[16])!==32'h41800000||evaluated(cases[17])!==32'h3e800000)
      $fatal(1,"fmac literal arithmetic anchors");
    for(integer k=0;k<3;k=k+1)begin stage_valid[k]=0;stage_data[k]=0;end
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
    row(0,1,0,cases[0]);edge_row(cases[0],0,1);edge_row(cases[4]);
    for(integer i=0;i<5;i=i+1)edge_row(cases[6],0);
    for(integer i=0;i<3;i=i+1)edge_row(cases[i]);
    edge_row(cases[5],1,1);
    for(integer i=0;i<52;i=i+1)begin
      edge_row(cases[i],1,0,i==5);
      if(i%4==3)edge_row(cases[(i+7)%52],0);
    end
    for(integer i=0;i<6;i=i+1)edge_row(cases[0],0);
    if(sampled>=220||peak!=3||reset_full!=1||dropped!=3||repeated<4||
       holes<=8||holds<=4||output_valid||accepted!=retired+dropped||accepted!=56)
      $fatal(1,"fmac bounded coverage/conservation");
    for(integer k=0;k<3;k=k+1)if(stage_valid[k])$fatal(1,"fmac undrained stage");
    $display("HISTORY %0d %0d %0d",accepted,retired,dropped);
    $finish;
  end
endmodule
