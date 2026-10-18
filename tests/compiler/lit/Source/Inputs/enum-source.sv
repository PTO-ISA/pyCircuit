module tb;
  `include "enum-source-vectors.svh"
`ifdef ENUM_SOURCE_MANAGED
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
`endif
  pyc_root dut(.*);
  initial begin
`ifdef ENUM_SOURCE_MANAGED
    pyc_7079635f636c6b=0;pyc_7079635f727374=0;
    pyc_root_commit_ok=1;pyc_phase=4;#1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
`elsif ENUM_SOURCE_STATE
    // Establish the same initialized state/clock history as native host Reset.
    pyc_7079635f636c6b=0;pyc_7079635f727374=1;#1;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
`endif
    for(integer row=0;row<row_count;row=row+1)begin
`ifndef ENUM_SOURCE_FOUR_STATE
      if(known_row(row))begin
`endif
        drive(row);#1;
`ifdef ENUM_SOURCE_MANAGED
        pyc_phase=1;#1;
        if(pyc_check_complete !== 1'b1 || pyc_sampling_error !== 1'b0 ||
           pyc_local_error !== 1'b0)$fatal(1,"managed query prepare failed");
`endif
        if(result !== golden(row))$fatal(1,"enum source members/selection/defaults/old-Q row%0d got%b expected%b",row,result,golden(row));
        $display("WORK %0d %b",row,result);
`ifdef ENUM_SOURCE_MANAGED
        pyc_root_commit_ok=1;pyc_phase=2;#1;
        pyc_phase=0;pyc_root_commit_ok=0;#1;
`endif
`ifdef ENUM_SOURCE_STATE
        pyc_7079635f636c6b=next_clock;
`ifdef ENUM_SOURCE_MANAGED
        pyc_root_commit_ok=0;pyc_phase=1;#1;
        pyc_root_commit_ok=1;pyc_phase=2;#1;
        pyc_phase=0;pyc_root_commit_ok=0;#1;
`else
        #1;
`endif
`endif
`ifndef ENUM_SOURCE_FOUR_STATE
      end
`endif
    end
`ifdef ENUM_SOURCE_FAILURE
    drive_failure();#1;pyc_7079635f636c6b=1;#1;
`ifdef ENUM_SOURCE_MANAGED
    pyc_root_commit_ok=0;pyc_phase=1;#1;
    if(pyc_local_error !== 1'b1)
      $fatal(1,"missing effective unknown-enable failure");
    $fatal(1,"enable must be known");
`else
    $fatal(1,"missing effective unknown-enable failure");
`endif
`endif
    $finish;
  end
endmodule
