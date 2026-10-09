module tb;
  `include "enum_vectors.svh"
  pyc_root dut(.*);
  initial begin
`ifdef ENUM_STORAGE
    clk=0;rst=1;#1;clk=1;#1;clk=0;#1;
`endif
    for(integer row=0;row<row_count;row=row+1)begin
`ifndef ENUM_FOUR_STATE
      if(known_row(row))begin
`endif
        drive(row);#1;check(row);$display("WORK %0d %s",row,golden_trace(row));
`ifdef ENUM_STORAGE
        clk=next_clock;#1;
`endif
`ifndef ENUM_FOUR_STATE
      end
`endif
    end
`ifdef ENUM_FAILURE
    drive_failure();#1;clk=1;#1;$fatal(1,"missing effective unknown-enable failure");
`endif
    $finish;
  end
endmodule
