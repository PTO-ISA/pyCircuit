module tb;
`ifndef CASE_STATEMENTS_STATE
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
`endif
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
`ifdef CASE_STATEMENTS_AVAILABILITY
  wire pyc_5f5f7079635f76616c69645f6f75745f30;
`endif
  task automatic host_reset;
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
  endtask
  task automatic managed_commit;
    pyc_root_commit_ok=0;pyc_phase=1;#1;
    pyc_root_commit_ok=(pyc_check_complete===1'b1 && pyc_sampling_error===1'b0 && pyc_local_error===1'b0);
    if(pyc_root_commit_ok!==1'b1)begin pyc_phase=3;#1;$fatal(1,"prepared case row failed completion/check validation");end
    pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
  endtask
  `include "case-statements-vectors.svh"
  pyc_root dut(.*);
  initial begin
    host_reset();
    for(integer row=0;row<row_count;row=row+1)begin
`ifndef CASE_STATEMENTS_FOUR_STATE
      if(known_row(row))begin
`endif
        drive(row);#1;
`ifdef CASE_STATEMENTS_AVAILABILITY
        if(pyc_5f5f7079635f76616c69645f6f75745f30 !== 1'b1)
          $fatal(1,"case result unavailable at row%0d",row);
`endif
        if(result !== golden(row))$fatal(1,"case statements members/selection/defaults/old-Q row%0d got%b expected%b",row,result,golden(row));
        $display("WORK %0d %b",row,result);
`ifdef CASE_STATEMENTS_STATE
        pyc_7079635f636c6b=next_clock;
        managed_commit();
`endif

`ifndef CASE_STATEMENTS_FOUR_STATE
      end
`endif
    end
`ifdef CASE_STATEMENTS_FAILURE
    drive_failure();#1;pyc_7079635f636c6b=1;#1;
    pyc_root_commit_ok=0;pyc_phase=1;#1;
    if(pyc_local_error !== 1'b1)
      $fatal(1,"missing effective unknown-enable failure");
    $fatal(1,"enable must be known");
`endif
    $finish;
  end
endmodule
