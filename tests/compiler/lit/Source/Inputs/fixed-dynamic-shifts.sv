// Independent source-bit oracle for fixed/fixed dynamic shifts.
module tb;
  logic v1;
  logic [4:0] v5, count_wide;
  logic [2:0] count_narrow;
  logic [64:0] v65, count65;
  logic [129:0] v130, count130;
  logic choose;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  wire [428:0] result;
  logic [428:0] expected;
  pyc_root dut(.*);

  function automatic logic symbol(input integer position,width,row,input bit mask);
    if(mask)begin
      if(row==1)return 1'bx;
      if(row==2)return 1'bz;
      if(position==0 || position==width-1 || position==64 || position==129)return 1'bx;
      if(position==1 || position==63 || position==65 || position==128)return 1'bz;
    end
    return (position*17+row*13)%5<2;
  endfunction
  function automatic logic [129:0] pattern(input integer width,row,input bit mask);
    logic [129:0] raw; raw=0;
    for(integer position=0;position<width;position=position+1)
      raw[position]=symbol(position,width,row,mask);
    return raw;
  endfunction
  task shifted(input integer low,width,count,input bit right,
               input logic [129:0] raw);
    integer source;
    for(integer position=0;position<width;position=position+1)begin
      source=right ? position+count : position-count;
      expected[low+position]=(source>=0 && source<width) ? raw[source] : 0;
    end
  endtask
  task row(input integer index,input bit mask,emit);
    integer c1,c5,c65,c130;
    integer quotient,remainder;
    logic [129:0] raw1,raw5,raw65,raw130;
    case(index)
      0:begin c1=0;c5=0;c65=0;c130=0;end
      1:begin c1=1;c5=1;c65=1;c130=1;end
      2:begin c1=0;c5=4;c65=64;c130=129;end
      3:begin c1=1;c5=5;c65=65;c130=130;end
      4:begin c1=2;c5=6;c65=66;c130=131;end
      default:begin c1=1;c5=5;c65=65;c130=130;end
    endcase
    raw1=pattern(1,index,mask);raw5=pattern(5,index,mask);
    raw65=pattern(65,index,mask);raw130=pattern(130,index,mask);
    v1=raw1[0];v5=raw5[4:0];v65=raw65[64:0];v130=raw130;
    count_wide=c1;count_narrow=c5;
    count65=index==5 ? (65'b1<<64) : c65;
    count130=index==5 ? (130'b1<<129) : c130;
    choose=index[0];
    #1; expected=0;
    shifted(413,1,c1,0,raw1);shifted(412,1,c1,1,raw1);
    shifted(407,5,c5,0,raw5);shifted(402,5,c5,1,raw5);
    shifted(337,65,c65,0,raw65);shifted(272,65,c65,1,raw65);
    shifted(142,130,c130,0,raw130);shifted(12,130,c130,1,raw130);
    shifted(424,5,3,0,raw5);
    if(mask)begin
      expected[423:419]='x;expected[418:414]='x;
    end else begin
      quotient=v5;remainder=v5;
      for(integer iteration=0;iteration<3;iteration=iteration+1)begin
        quotient=quotient/3;remainder=remainder%5;
      end
      expected[423:419]=quotient;expected[418:414]=remainder;
    end
    expected[11:7]=v5 & (choose ? 5'd1 : 5'd3);
    expected[6:2]=(choose ? 5'd1 : 5'd3) & v5;
    expected[1]=2<v5;expected[0]=v5>2;
    if(result !== expected)$fatal(1,"dynamic fixed shift/peer normalization oracle failed");
    if(emit)begin if(mask)$display("DMASK %b",result);else $display("DYNAMIC %b",result);end
  endtask
  task count_unknown(input bit high,zbit);
    logic unknown;
    unknown=zbit ? 1'bz : 1'bx;
    v1=pattern(1,3,0);v5=pattern(5,3,0);v65=pattern(65,3,0);v130=pattern(130,3,0);
    count_wide=0;count_narrow=0;count65=0;count130=0;choose=0;
    count_wide[high ? 4 : 0]=unknown;
    count_narrow[high ? 2 : 0]=unknown;
    count65[high ? 64 : 0]=unknown;
    count130[high ? 129 : 0]=unknown;
    #1;expected=0;
    expected[413:12]='x;
    expected[424 +: 5]=(v5<<3);
    begin integer quotient,remainder;
      quotient=v5;remainder=v5;
      for(integer iteration=0;iteration<3;iteration=iteration+1)begin
        quotient=quotient/3;remainder=remainder%5;
      end
      expected[423:419]=quotient;expected[418:414]=remainder;
    end
    expected[11:7]=v5&3;expected[6:2]=3&v5;
    expected[1]=2<v5;expected[0]=v5>2;
    if(result !== expected)$fatal(1,"unknown dynamic count must produce X without Z");
    if(high)begin if(zbit)$display("COUNT HZ %b",result);else $display("COUNT HX %b",result);end
    else begin if(zbit)$display("COUNT LZ %b",result);else $display("COUNT LX %b",result);end
    row(3,0,0);
  endtask
  initial begin
    for(integer index=0;index<6;index=index+1)row(index,0,1);
`ifdef DYNAMIC_FIXED_FOUR_STATE
    for(integer index=0;index<4;index=index+1)row(index,1,1);
    count_unknown(0,0);count_unknown(0,1);
    count_unknown(1,0);count_unknown(1,1);
`endif
    $finish;
  end
endmodule
