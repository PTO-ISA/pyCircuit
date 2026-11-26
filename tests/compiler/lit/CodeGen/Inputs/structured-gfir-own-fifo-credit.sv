module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
  logic take=0,push=1,late=0;
  logic [15:0] data=0;
  wire ready,present;
  wire [15:0] head;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.*);
  string path;
  integer fd,n,samples=0,value,expected_head,expected_acceptance,discard;
  logic expected_ready,expected_present;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%d %d %d %b %b %b %b %b %d %d %d\n",pyc_7079635f636c6b,
                pyc_7079635f727374,value,take,push,late,expected_ready,expected_present,
                expected_head,expected_acceptance,discard);
      if(n!=11)$fatal(1,"invalid row");
      data=16'(value);
      #1;pyc_phase=1;#1;
      if(ready!==expected_ready || present!==expected_present || head!==16'(expected_head))
        $fatal(1,"own-pop state mismatch row %0d",samples);
      accepted=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(accepted!==1'(expected_acceptance))$fatal(1,"acceptance mismatch row %0d",samples);
      if(accepted && !discard)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;samples=samples+1;
    end
`ifdef SELECTED_IN
    if(samples!=41)$fatal(1,"incomplete rows");
`else
    if(samples!=30)$fatal(1,"incomplete rows");
`endif
    $fclose(fd);$finish;
  end
endmodule
