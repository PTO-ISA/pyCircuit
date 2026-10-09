module tb;
  `include "case-statements-vectors.svh"
  pyc_root dut(.*);
  initial begin
`ifdef CASE_STATEMENTS_STATE
    // Establish the same initialized state/clock history as native host Reset.
    pyc_7079635f636c6b=0;pyc_7079635f727374=1;#1;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
`endif
    for(integer row=0;row<row_count;row=row+1)begin
`ifndef CASE_STATEMENTS_FOUR_STATE
      if(known_row(row))begin
`endif
        drive(row);#1;
        if(result !== golden(row))$fatal(1,"case statements members/selection/defaults/old-Q row%0d got%b expected%b",row,result,golden(row));
        $display("WORK %0d %b",row,result);
`ifdef CASE_STATEMENTS_STATE
        pyc_7079635f636c6b=next_clock;#1;
`endif
`ifndef CASE_STATEMENTS_FOUR_STATE
      end
`endif
    end
`ifdef CASE_STATEMENTS_FAILURE
    drive_failure();#1;pyc_7079635f636c6b=1;#1;
    $fatal(1,"missing effective unknown-enable failure");
`endif
    $finish;
  end
endmodule
