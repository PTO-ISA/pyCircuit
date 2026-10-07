// Independent scalar SystemVerilog ternary-fold oracle; genuine X/Z.
module tb;
  logic [0:0] w1;
  logic [1:0] w2;
  logic [2:0] w3;
  logic [3:0] w4;
  logic [4:0] w5;
  logic [7:0] w8;
  logic [8:0] w9;
  logic [12:0] w13;
  logic [30:0] w31;
  logic [31:0] w32;
  logic [32:0] w33;
  logic [62:0] w63;
  logic [63:0] w64;
  logic [64:0] w65;
  logic [72:0] w73;
  logic [126:0] w127;
  logic [127:0] w128;
  logic [128:0] w129;
  wire [509:0] result;
  pyc_root dut(.*);
  integer frame=0,known_frames=0,masked_frames=0;
  localparam integer SCALAR=0,ZERO=1,ONES=2,ONEHOT=3,MIXED=4,FOUR_SMALL=5,
      ZERO_MASKED=6,ONES_MASKED=7,UPPER_ONE=8,LOWER_ONE=9,DENSE=10,LITERAL=11;
`ifdef PRIORITY_FOUR_STATE
  function automatic logic [3:0] table_input(input integer row);
    case(row)
      0:return 4'b0000; 1:return 4'b0100; 2:return 4'b1010;
      3:return 4'bx100; 4:return 4'b0x01; 5:return 4'b000x; 6:return 4'b1xxx;
    endcase
  endfunction
  function automatic logic [1:0] table_index(input integer row,input bit high_order);
    case(row)
      0:return 2'b00; 1:return 2'b10; 2:return high_order?2'b11:2'b01;
      3:return high_order?2'b1x:2'b10; 4:return high_order?2'bx0:2'b00;
      5:return 2'b00; 6:return high_order?2'b11:2'bxx;
    endcase
  endfunction
  function automatic logic table_valid(input integer row);
    case(row) 0:return 1'b0; 5:return 1'bx; default:return 1'b1; endcase
  endfunction
  function automatic logic table_conflict(input integer row);
    case(row) 0,1:return 1'b0; 2:return 1'b1; default:return 1'bx; endcase
  endfunction
`endif
  function automatic logic [128:0] stimulus(input integer width,input integer pattern,input integer row,input bit high_z);
    logic [128:0] value; logic [3:0] literal_bits; value='0;
`ifdef PRIORITY_FOUR_STATE
    if(pattern==LITERAL) literal_bits=table_input(row);
`endif
    for(integer bit_index=0;bit_index<width;bit_index=bit_index+1) begin
      case(pattern)
        SCALAR:value[bit_index]=bit_index<8 && ((row>>bit_index)&1);
        ZERO:value[bit_index]=1'b0;
        ONES:value[bit_index]=1'b1;
        ONEHOT:value[bit_index]=bit_index==row;
        MIXED:value[bit_index]=((bit_index*7+row*3)%11)<5;
`ifdef PRIORITY_FOUR_STATE
        FOUR_SMALL: if(width<=4) case((row>>(2*bit_index))&3)
          0:value[bit_index]=1'b0; 1:value[bit_index]=1'b1;
          2:value[bit_index]=1'bx; 3:value[bit_index]=1'bz;
        endcase
        ZERO_MASKED:value[bit_index]=bit_index==row?(high_z?1'bz:1'bx):1'b0;
        ONES_MASKED:value[bit_index]=bit_index==row?(high_z?1'bz:1'bx):1'b1;
        UPPER_ONE:value[bit_index]=bit_index==row?(high_z?1'bz:1'bx):(bit_index==width-1);
        LOWER_ONE:value[bit_index]=bit_index==row?(high_z?1'bz:1'bx):(bit_index==0);
        DENSE:case((bit_index+row)%4)
          0:value[bit_index]=1'b0; 1:value[bit_index]=1'b1;
          2:value[bit_index]=1'bx; 3:value[bit_index]=1'bz;
        endcase
        LITERAL:if(width==4) value[bit_index]=high_z && literal_bits[bit_index]===1'bx ? 1'bz : literal_bits[bit_index];
`endif
        default:$fatal(1,"invalid stimulus pattern");
      endcase
    end
    return value;
  endfunction
  task automatic drive(input integer pattern,input integer row,input bit high_z);
    w1=stimulus(1,pattern,row,high_z);
    w2=stimulus(2,pattern,row,high_z);
    w3=stimulus(3,pattern,row,high_z);
    w4=stimulus(4,pattern,row,high_z);
    w5=stimulus(5,pattern,row,high_z);
    w8=stimulus(8,pattern,row,high_z);
    w9=stimulus(9,pattern,row,high_z);
    w13=stimulus(13,pattern,row,high_z);
    w31=stimulus(31,pattern,row,high_z);
    w32=stimulus(32,pattern,row,high_z);
    w33=stimulus(33,pattern,row,high_z);
    w63=stimulus(63,pattern,row,high_z);
    w64=stimulus(64,pattern,row,high_z);
    w65=stimulus(65,pattern,row,high_z);
    w73=stimulus(73,pattern,row,high_z);
    w127=stimulus(127,pattern,row,high_z);
    w128=stimulus(128,pattern,row,high_z);
    w129=stimulus(129,pattern,row,high_z);
  endtask
  function automatic integer index_width(input integer value);
    integer n;n=0;do begin n=n+1;value=value>>1;end while(value!=0);return n;
  endfunction
  // Internal oracle layout: index at bits [7:0], valid[8], conflict[9].
  // Index is accumulated by natural-width scalar ternaries, with OR valid.
  function automatic logic [9:0] encode(input logic [128:0] value,input integer width,input integer start,input bit high_order);
    logic [9:0] expected;logic predicate;integer natural,position,population;bit uncertain;
    expected='0;natural=index_width(width-1);population=0;uncertain=0;
    for(integer visit=0;visit<width;visit=visit+1) begin
      position=high_order?visit:width-1-visit;predicate=value[start+position];
      for(integer bit_index=0;bit_index<natural;bit_index=bit_index+1)
        expected[bit_index]=predicate?((position>>bit_index)&1):expected[bit_index];
      expected[8]=expected[8]|predicate;
      if(predicate===1'b1) population=population+1;
      if(predicate!==1'b0 && predicate!==1'b1) uncertain=1;
    end
    expected[9]=uncertain?1'bx:population>1;return expected;
  endfunction
  function automatic logic [128:0] increment(input logic [4:0] value);
    logic [128:0] expected;expected='0;expected[4:0]=value+5'b00001;return expected;
  endfunction
  function automatic logic [509:0] golden();
    logic [509:0] expected; logic [9:0] selected;expected='0;
    selected=encode(w1,1,0,0);
    expected[509 +: 1]=selected[0:0]; expected[508]=selected[8];
    selected=encode(w1,1,0,1);
    expected[507 +: 1]=selected[0:0]; expected[506]=selected[8];
    selected=encode(w1,1,0,0);
    expected[505 +: 1]=selected[0:0]; expected[504]=selected[8];
    expected[503]=selected[9];
    selected=encode(w1,1,0,1);
    expected[502 +: 1]=selected[0:0]; expected[501]=selected[8];
    expected[500]=selected[9];
    selected=encode(w2,2,0,0);
    expected[499 +: 1]=selected[0:0]; expected[498]=selected[8];
    selected=encode(w2,2,0,1);
    expected[497 +: 1]=selected[0:0]; expected[496]=selected[8];
    selected=encode(w2,2,0,0);
    expected[495 +: 1]=selected[0:0]; expected[494]=selected[8];
    expected[493]=selected[9];
    selected=encode(w2,2,0,1);
    expected[492 +: 1]=selected[0:0]; expected[491]=selected[8];
    expected[490]=selected[9];
    selected=encode(w3,3,0,0);
    expected[488 +: 2]=selected[1:0]; expected[487]=selected[8];
    selected=encode(w3,3,0,1);
    expected[485 +: 2]=selected[1:0]; expected[484]=selected[8];
    selected=encode(w3,3,0,0);
    expected[482 +: 2]=selected[1:0]; expected[481]=selected[8];
    expected[480]=selected[9];
    selected=encode(w3,3,0,1);
    expected[478 +: 2]=selected[1:0]; expected[477]=selected[8];
    expected[476]=selected[9];
    selected=encode(w4,4,0,0);
    expected[474 +: 2]=selected[1:0]; expected[473]=selected[8];
    selected=encode(w4,4,0,1);
    expected[471 +: 2]=selected[1:0]; expected[470]=selected[8];
    selected=encode(w4,4,0,0);
    expected[468 +: 2]=selected[1:0]; expected[467]=selected[8];
    expected[466]=selected[9];
    selected=encode(w4,4,0,1);
    expected[464 +: 2]=selected[1:0]; expected[463]=selected[8];
    expected[462]=selected[9];
    selected=encode(w5,5,0,0);
    expected[459 +: 3]=selected[2:0]; expected[458]=selected[8];
    selected=encode(w5,5,0,1);
    expected[455 +: 3]=selected[2:0]; expected[454]=selected[8];
    selected=encode(w5,5,0,0);
    expected[451 +: 3]=selected[2:0]; expected[450]=selected[8];
    expected[449]=selected[9];
    selected=encode(w5,5,0,1);
    expected[446 +: 3]=selected[2:0]; expected[445]=selected[8];
    expected[444]=selected[9];
    selected=encode(w8,8,0,0);
    expected[441 +: 3]=selected[2:0]; expected[440]=selected[8];
    selected=encode(w8,8,0,1);
    expected[437 +: 3]=selected[2:0]; expected[436]=selected[8];
    selected=encode(w8,8,0,0);
    expected[433 +: 3]=selected[2:0]; expected[432]=selected[8];
    expected[431]=selected[9];
    selected=encode(w8,8,0,1);
    expected[428 +: 3]=selected[2:0]; expected[427]=selected[8];
    expected[426]=selected[9];
    selected=encode(w9,9,0,0);
    expected[422 +: 4]=selected[3:0]; expected[421]=selected[8];
    selected=encode(w9,9,0,1);
    expected[417 +: 4]=selected[3:0]; expected[416]=selected[8];
    selected=encode(w9,9,0,0);
    expected[412 +: 4]=selected[3:0]; expected[411]=selected[8];
    expected[410]=selected[9];
    selected=encode(w9,9,0,1);
    expected[406 +: 4]=selected[3:0]; expected[405]=selected[8];
    expected[404]=selected[9];
    selected=encode(w13,13,0,0);
    expected[400 +: 4]=selected[3:0]; expected[399]=selected[8];
    selected=encode(w13,13,0,1);
    expected[395 +: 4]=selected[3:0]; expected[394]=selected[8];
    selected=encode(w13,13,0,0);
    expected[390 +: 4]=selected[3:0]; expected[389]=selected[8];
    expected[388]=selected[9];
    selected=encode(w13,13,0,1);
    expected[384 +: 4]=selected[3:0]; expected[383]=selected[8];
    expected[382]=selected[9];
    selected=encode(w31,31,0,0);
    expected[377 +: 5]=selected[4:0]; expected[376]=selected[8];
    selected=encode(w31,31,0,1);
    expected[371 +: 5]=selected[4:0]; expected[370]=selected[8];
    selected=encode(w31,31,0,0);
    expected[365 +: 5]=selected[4:0]; expected[364]=selected[8];
    expected[363]=selected[9];
    selected=encode(w31,31,0,1);
    expected[358 +: 5]=selected[4:0]; expected[357]=selected[8];
    expected[356]=selected[9];
    selected=encode(w32,32,0,0);
    expected[351 +: 5]=selected[4:0]; expected[350]=selected[8];
    selected=encode(w32,32,0,1);
    expected[345 +: 5]=selected[4:0]; expected[344]=selected[8];
    selected=encode(w32,32,0,0);
    expected[339 +: 5]=selected[4:0]; expected[338]=selected[8];
    expected[337]=selected[9];
    selected=encode(w32,32,0,1);
    expected[332 +: 5]=selected[4:0]; expected[331]=selected[8];
    expected[330]=selected[9];
    selected=encode(w33,33,0,0);
    expected[324 +: 6]=selected[5:0]; expected[323]=selected[8];
    selected=encode(w33,33,0,1);
    expected[317 +: 6]=selected[5:0]; expected[316]=selected[8];
    selected=encode(w33,33,0,0);
    expected[310 +: 6]=selected[5:0]; expected[309]=selected[8];
    expected[308]=selected[9];
    selected=encode(w33,33,0,1);
    expected[302 +: 6]=selected[5:0]; expected[301]=selected[8];
    expected[300]=selected[9];
    selected=encode(w63,63,0,0);
    expected[294 +: 6]=selected[5:0]; expected[293]=selected[8];
    selected=encode(w63,63,0,1);
    expected[287 +: 6]=selected[5:0]; expected[286]=selected[8];
    selected=encode(w63,63,0,0);
    expected[280 +: 6]=selected[5:0]; expected[279]=selected[8];
    expected[278]=selected[9];
    selected=encode(w63,63,0,1);
    expected[272 +: 6]=selected[5:0]; expected[271]=selected[8];
    expected[270]=selected[9];
    selected=encode(w64,64,0,0);
    expected[264 +: 6]=selected[5:0]; expected[263]=selected[8];
    selected=encode(w64,64,0,1);
    expected[257 +: 6]=selected[5:0]; expected[256]=selected[8];
    selected=encode(w64,64,0,0);
    expected[250 +: 6]=selected[5:0]; expected[249]=selected[8];
    expected[248]=selected[9];
    selected=encode(w64,64,0,1);
    expected[242 +: 6]=selected[5:0]; expected[241]=selected[8];
    expected[240]=selected[9];
    selected=encode(w65,65,0,0);
    expected[233 +: 7]=selected[6:0]; expected[232]=selected[8];
    selected=encode(w65,65,0,1);
    expected[225 +: 7]=selected[6:0]; expected[224]=selected[8];
    selected=encode(w65,65,0,0);
    expected[217 +: 7]=selected[6:0]; expected[216]=selected[8];
    expected[215]=selected[9];
    selected=encode(w65,65,0,1);
    expected[208 +: 7]=selected[6:0]; expected[207]=selected[8];
    expected[206]=selected[9];
    selected=encode(w73,73,0,0);
    expected[199 +: 7]=selected[6:0]; expected[198]=selected[8];
    selected=encode(w73,73,0,1);
    expected[191 +: 7]=selected[6:0]; expected[190]=selected[8];
    selected=encode(w73,73,0,0);
    expected[183 +: 7]=selected[6:0]; expected[182]=selected[8];
    expected[181]=selected[9];
    selected=encode(w73,73,0,1);
    expected[174 +: 7]=selected[6:0]; expected[173]=selected[8];
    expected[172]=selected[9];
    selected=encode(w127,127,0,0);
    expected[165 +: 7]=selected[6:0]; expected[164]=selected[8];
    selected=encode(w127,127,0,1);
    expected[157 +: 7]=selected[6:0]; expected[156]=selected[8];
    selected=encode(w127,127,0,0);
    expected[149 +: 7]=selected[6:0]; expected[148]=selected[8];
    expected[147]=selected[9];
    selected=encode(w127,127,0,1);
    expected[140 +: 7]=selected[6:0]; expected[139]=selected[8];
    expected[138]=selected[9];
    selected=encode(w128,128,0,0);
    expected[131 +: 7]=selected[6:0]; expected[130]=selected[8];
    selected=encode(w128,128,0,1);
    expected[123 +: 7]=selected[6:0]; expected[122]=selected[8];
    selected=encode(w128,128,0,0);
    expected[115 +: 7]=selected[6:0]; expected[114]=selected[8];
    expected[113]=selected[9];
    selected=encode(w128,128,0,1);
    expected[106 +: 7]=selected[6:0]; expected[105]=selected[8];
    expected[104]=selected[9];
    selected=encode(w129,129,0,0);
    expected[96 +: 8]=selected[7:0]; expected[95]=selected[8];
    selected=encode(w129,129,0,1);
    expected[87 +: 8]=selected[7:0]; expected[86]=selected[8];
    selected=encode(w129,129,0,0);
    expected[78 +: 8]=selected[7:0]; expected[77]=selected[8];
    expected[76]=selected[9];
    selected=encode(w129,129,0,1);
    expected[68 +: 8]=selected[7:0]; expected[67]=selected[8];
    expected[66]=selected[9];
    selected=encode(w1,1,0,1);
    expected[58 +: 8]=selected[7:0];expected[57]=selected[8];expected[56]=selected[9];
    selected=encode(w73,9,61,1);
    expected[52 +: 4]=selected[3:0];expected[51]=selected[8];expected[50]=selected[9];
    selected=encode(increment(w5),5,0,1);
    expected[47 +: 3]=selected[2:0];expected[46]=selected[8];expected[45]=selected[9];
    selected=encode(w3,3,0,1);
    expected[43 +: 2]=selected[1:0];expected[42]=selected[8];expected[41]=selected[9];
    selected=encode(w5,5,0,1);
    expected[38 +: 3]=selected[2:0];expected[37]=selected[8];expected[36]=selected[9];
    selected=encode(w8,6,0,0);
    expected[33 +: 3]=selected[2:0];expected[32]=selected[8];
    selected=encode(w8,6,0,1);
    expected[29 +: 3]=selected[2:0];expected[28]=selected[8];
    selected=encode(w8,6,0,0);
    expected[25 +: 3]=selected[2:0];expected[24]=selected[8];
    expected[23]=selected[9];
    selected=encode(w8,6,0,1);
    expected[20 +: 3]=selected[2:0];expected[19]=selected[8];
    expected[18]=selected[9];
    selected=encode(w8,7,0,0);
    expected[15 +: 3]=selected[2:0];expected[14]=selected[8];
    selected=encode(w8,7,0,1);
    expected[11 +: 3]=selected[2:0];expected[10]=selected[8];
    selected=encode(w8,7,0,0);
    expected[7 +: 3]=selected[2:0];expected[6]=selected[8];
    expected[5]=selected[9];
    selected=encode(w8,7,0,1);
    expected[2 +: 3]=selected[2:0];expected[1]=selected[8];
    expected[0]=selected[9];
    return expected;
  endfunction
  task automatic sample(input bit masked);
    logic [509:0] expected;
    #1;expected=golden();
    if(result!==expected) $fatal(1,"priority frame %0d got %b expected %b",frame,result,expected);
    if(masked) begin $display("MASK %b",result);masked_frames=masked_frames+1;end
    else begin $display("WORK %b",result);known_frames=known_frames+1;end
    frame=frame+1;
  endtask
`ifdef PRIORITY_FOUR_STATE
  task automatic table_check(input integer row);
    if(result[474 +: 2]!==table_index(row,0)) $fatal(1,"pL literal index row %0d",row);
    if(result[473]!==table_valid(row)) $fatal(1,"pL literal valid row %0d",row);
    if(result[471 +: 2]!==table_index(row,1)) $fatal(1,"pH literal index row %0d",row);
    if(result[470]!==table_valid(row)) $fatal(1,"pH literal valid row %0d",row);
    if(result[468 +: 2]!==table_index(row,0)) $fatal(1,"oL literal index row %0d",row);
    if(result[467]!==table_valid(row)) $fatal(1,"oL literal valid row %0d",row);
    if(result[466]!==table_conflict(row)) $fatal(1,"oL literal conflict row %0d",row);
    if(result[464 +: 2]!==table_index(row,1)) $fatal(1,"oH literal index row %0d",row);
    if(result[463]!==table_valid(row)) $fatal(1,"oH literal valid row %0d",row);
    if(result[462]!==table_conflict(row)) $fatal(1,"oH literal conflict row %0d",row);
  endtask
`endif
  initial begin
    for(integer row=0;row<256;row=row+1) begin drive(SCALAR,row,0);sample(0);end
    drive(ZERO,0,0);sample(0);drive(ONES,0,0);sample(0);
    for(integer row=0;row<129;row=row+1) begin drive(ONEHOT,row,0);sample(0);end
    for(integer row=0;row<32;row=row+1) begin drive(MIXED,row,0);sample(0);end
    if(known_frames!=419) $fatal(1,"wrong known prefix count");
`ifdef PRIORITY_FOUR_STATE
    for(integer row=0;row<256;row=row+1)
      for(integer latent=0;latent<2;latent=latent+1) begin drive(FOUR_SMALL,row,0);sample(1);end
    for(integer pattern=ZERO_MASKED;pattern<=LOWER_ONE;pattern=pattern+1)
      for(integer row=0;row<129;row=row+1)
        for(integer high_z=0;high_z<2;high_z=high_z+1)
          for(integer latent=0;latent<2;latent=latent+1) begin drive(pattern,row,high_z);sample(1);end
    for(integer phase=0;phase<4;phase=phase+1)
      for(integer latent=0;latent<2;latent=latent+1) begin drive(DENSE,phase,0);sample(1);end
    for(integer row=0;row<7;row=row+1)
      for(integer high_z=0;high_z<2;high_z=high_z+1)
        for(integer latent=0;latent<2;latent=latent+1) begin drive(LITERAL,row,high_z);sample(1);table_check(row);end
    if(masked_frames!=2612) $fatal(1,"wrong masked count");
`endif
    drive(ONES,0,0);sample(0);
    if(known_frames!=420) $fatal(1,"wrong known count");
`ifdef PRIORITY_FOUR_STATE
    if(frame!=3032) $fatal(1,"wrong total count");
`else
    if(frame!=420) $fatal(1,"wrong total count");
`endif
    $finish;
  end
endmodule
