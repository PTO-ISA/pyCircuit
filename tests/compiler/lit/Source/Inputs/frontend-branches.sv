module tb;
`ifndef STATE_BRANCHES
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
`endif
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  task automatic host_reset;
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
  endtask
  task automatic managed_commit;
    pyc_root_commit_ok=0;pyc_phase=1;#1;
    pyc_root_commit_ok=(pyc_check_complete===1'b1 && pyc_sampling_error===1'b0 && pyc_local_error===1'b0);
    if(pyc_root_commit_ok!==1'b1)begin pyc_phase=3;#1;$fatal(1,"prepared frontend row failed completion/check validation");end
    pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
  endtask
  `include "frontend-branches-vectors.svh"
  pyc_root dut(.*);
  initial begin
    host_reset();
    for(integer row=0;row<row_count;row=row+1)begin
`ifndef FRONTEND_BRANCHES_FOUR_STATE
      if(known_row(row))begin
`endif
        drive(row);#1;
        if(result !== golden(row))$fatal(1,"frontend branch values/proposals/old-Q row%0d got%b expected%b",row,result,golden(row));
        $display("WORK %0d %b",row,result);
`ifdef STATE_BRANCHES
        pyc_7079635f636c6b=next_clock;
        managed_commit();
`endif
`ifndef FRONTEND_BRANCHES_FOUR_STATE
      end
`endif
    end
`ifdef FRONTEND_BRANCHES_FAILURE
    drive_failure();#1;pyc_7079635f636c6b=1;#1;
    pyc_root_commit_ok=0;pyc_phase=1;#1;
    if(pyc_local_error !== 1'b1)
      $fatal(1,"missing effective unknown-enable failure");
    $fatal(1,"enable must be known");
`endif
    $finish;
  end
endmodule
