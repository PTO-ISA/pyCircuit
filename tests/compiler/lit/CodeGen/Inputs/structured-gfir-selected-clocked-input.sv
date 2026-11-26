`include "selected_input_ports.svh"
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0,offer=0,select=0,run=0;
  logic [7:0] data=0,fallback=0;
  wire [7:0] result,old,producer_old;
  wire result_valid;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.`PYC_RESULT_VALID(result_valid),.*);
  string path;
  integer fd,n,count=0;
  logic expected_valid,expected_acceptance,discard;
  logic [7:0] expected_result,expected_old,expected_produced;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%b %b %b %b %b %b %b %b %b %b %b %b %b\n",
                pyc_7079635f636c6b,pyc_7079635f727374,offer,data,select,fallback,run,
                expected_valid,expected_result,expected_old,expected_produced,
                expected_acceptance,discard);
      if(n!=13)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(result_valid!==expected_valid || (expected_valid && result!==expected_result) ||
         old!==expected_old || producer_old!==expected_produced)
        $fatal(1,"selected availability/state mismatch row %0d",count);
      accepted=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(accepted!==expected_acceptance)$fatal(1,"acceptance mismatch row %0d",count);
      if(accepted && !discard)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;count=count+1;
    end
    if(count!=27)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
