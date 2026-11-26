`include "credit_ports.svh"
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
  logic [15:0] left=0,right=0;
  wire [15:0] result;
  wire result_valid;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.`PYC_RESULT_VALID(result_valid),.*);
  string path;
  integer fd,n,count=0,left_value,expected_result,expected_acceptance;
  logic right_bit,expected_valid;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%d %d %d %b %b %d %d\n",pyc_7079635f636c6b,
                pyc_7079635f727374,left_value,right_bit,expected_valid,
                expected_result,expected_acceptance);
      if(n!=7)$fatal(1,"invalid row");
      left=16'(left_value);right={15'b0,right_bit};
      #1;pyc_phase=1;#1;
      if(result_valid!==expected_valid || (expected_valid && result!==16'(expected_result)))
        $fatal(1,"credit result mismatch row %0d",count);
      accepted=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(accepted!==1'(expected_acceptance))$fatal(1,"acceptance mismatch row %0d",count);
      if(accepted)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;count=count+1;
    end
    if(count!=28)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
