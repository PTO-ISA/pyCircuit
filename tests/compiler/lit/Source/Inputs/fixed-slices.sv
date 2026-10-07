// Oracle uses independently written Verilog bit positions, before clock transitions.
module tb;
`ifdef TABLE_SLICES
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
  logic index=0,write=0;
  logic [12:0] value=0;
  wire [19:0] result;
  logic [12:0] cells[2];
  bit last_clock=0;
  pyc_root dut(.*);
  task row(input bit clock_level,reset,idx,input logic [12:0] data,input bit wr,input integer mode);
    pyc_7079635f727374=reset;index=idx;value=data;write=wr;#1;
    if(result !== {cells[idx],cells[idx][8:2]}) $fatal(1,"Table old-Q/full index/field slice failed");
    if(mode==1) $display("WORK %b",result);
    if(mode==2) $display("MASK %b",result);
    if(clock_level && !last_clock) begin
      if(reset)begin cells[0]=0;cells[1]=0;end
      else if(wr)cells[idx]=data;
    end
    last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;
  endtask
  task unknown_row(input logic [12:0] data);
    row(0,0,1,data,1,0);row(1,0,1,data,1,0);row(0,0,1,0,0,2);
    row(1,1,1,0,0,0);row(0,0,1,0,0,0);
  endtask
  initial begin
    cells[0]=0;cells[1]=0;
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    row(0,0,0,1,1,1);row(1,0,0,8191,1,1);row(1,0,0,2,1,1);row(0,0,0,3,0,1);
    row(1,0,1,4096,1,1);row(0,0,1,0,0,1);row(1,0,0,0,0,1);row(0,0,1,2047,1,1);
    row(1,0,1,31,1,1);row(0,0,1,0,0,1);row(0,1,0,7,1,1);row(1,1,0,7,1,1);
    row(1,0,1,9,1,1);row(0,0,1,9,1,1);row(1,0,1,9,1,1);row(0,0,1,0,0,1);
`ifdef SLICES_FOUR_STATE
    unknown_row(13'b1x0z0101zx0z1);unknown_row(13'bxxxxxxxxxxxxx);
    unknown_row(13'bzzzzzzzzzzzzz);unknown_row(13'b0z101x0101z01);
`endif
    $finish;
  end
`else
  logic tiny,choose;
  logic [12:0] n13;
  logic [39:0] address;
  logic [72:0] wide;
  wire [248:0] result;
  logic [12:0] arithmetic;
  logic [39:0] selected;
  logic [248:0] expected;
  pyc_root dut(.*);
  task row(input logic t,input logic [12:0] s,input logic [39:0] a,input logic [72:0] w,input logic c,input bit unknown_case);
    tiny=t;n13=s;address=a;wide=w;choose=c;#1;
    arithmetic=n13+13'd1; selected=choose ? address : wide[39:0];
    expected={tiny,n13[4:0],n13[9:3],n13[12:8],n13,n13[8:5],address[39:12],
              wide[8:0],wide[71:55],wide[72:64],wide,n13[6:1],
              arithmetic,arithmetic[5:0],selected,selected[16:4]};
    if(result !== expected) $fatal(1,"static LSB-first/full/nested/multiword slice failed");
    if(unknown_case)$display("MASK %b",result);else $display("WORK %b",result);
  endtask
  function logic [12:0] n13_value(input integer index);
    case(index)
      0:n13_value=0;1:n13_value=1;2:n13_value=8191;3:n13_value=4096;4:n13_value=2730;
      5:n13_value=5461;6:n13_value=254;7:n13_value=6001;8:n13_value=8190;9:n13_value=17;
    endcase
  endfunction
  function logic [39:0] address_value(input integer index);
    case(index)
      0:address_value=0;1:address_value=1;2:address_value=40'd1099511627775;3:address_value=40'd549755813888;
      4:address_value=40'd366503875925;5:address_value=40'd733007751850;6:address_value=4095;
      7:address_value=4096;8:address_value=40'd1099511627774;9:address_value=17;
    endcase
  endfunction
  initial begin
    for(integer index=0;index<10;index=index+1) begin
      logic [72:0] w;
      logic [63:0] low_word;
      low_word=64'(address_value(9-index))*64'd65537;
      w={9'((index*57)%512),low_word};
      row(index[0],n13_value(index),address_value(index),w,index[0],0);
    end
`ifdef SLICES_FOUR_STATE
    row(1'bz,13'b1x0z0101zx0z1,{10{4'b1x0z}},{{18{4'b1x0z}},1'b1},1'b1,1);
    row(1'bx,{13{1'bz}},{40{1'bz}},{73{1'bz}},1'b0,1);
    row(1'b1,13'd0,40'd0,{73{1'b1}},1'bx,1);
    row(1'b0,13'd0,40'd0,{{33{1'bz}},40'd0},1'bz,1);
`endif
    $finish;
  end
`endif
endmodule
