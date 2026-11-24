module tb;
  logic [3:0] index;
  logic [7:0] base,data;
  wire [7:0] row0,row1,row2,selected;
  wire in_range;
  pyc_root dut(.*);
  task automatic known_case(input integer i);
    index=4'(i);#1;
    if(row0!==(i==0?8'ha5:8'ha0) || row1!==(i==1?8'ha5:8'ha0) || row2!==(i==2?8'ha5:8'ha0))$fatal(1,"mapped row");
    if(i<3)begin if(selected!==8'ha5 || in_range!==1'b1)$fatal(1,"in range");end
    else if(selected!==8'hxx || in_range!==1'b0)$fatal(1,"out of range");
  endtask
  task automatic uncertain(input logic low);
    index={3'b000,low};#1;
    if(row0!==8'b10100x0x || row1!==8'b10100x0x || row2!==8'ha0 || selected!==8'hxx || in_range!==1'bx)$fatal(1,"unknown index");
  endtask
  initial begin
    base=8'ha0;data=8'ha5;
    known_case(0);known_case(1);known_case(2);known_case(3);known_case(8);
    uncertain(1'bx);uncertain(1'bz);
    base=8'hzz;data=8'hzz;#1;
    if(row0!==8'hzz || row1!==8'hzz || row2!==8'hzz)$fatal(1,"equal Z merge");
    $finish;
  end
endmodule
