`include "nested_method_ports.svh"
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0,allow=1,index=0;
  wire [15:0] q,rows,untouched,untouched_rows;
  wire [7:0] observed,observed_row;
  wire [31:0] candidate;
  wire candidate_valid;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.`PYC_CANDIDATE_VALID(candidate_valid),.*);
  string path;
  integer fd,n,count=0;
  logic [7:0] expected_total,expected_selected;
  logic [15:0] expected_rows;
  logic [31:0] expected_candidate;
  logic expected_available,expected_acceptance;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%h %h %h %h %h %h %h %h %h %h\n",pyc_7079635f636c6b,
                pyc_7079635f727374,allow,index,expected_total,expected_rows,
                expected_selected,expected_candidate,expected_available,expected_acceptance);
      if(n!=10)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(q!=={expected_total,8'h5a} || rows!==expected_rows ||
         observed!==expected_total || observed_row!==expected_selected ||
         untouched!==16'h005a || untouched_rows!==0)
        $fatal(1,"committed snapshot/identity mismatch row %0d",count);
      if(candidate_valid!==expected_available ||
         (expected_available && candidate!==expected_candidate))
        $fatal(1,"ordered candidate mismatch row %0d",count);
      accepted=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(accepted!==expected_acceptance)$fatal(1,"acceptance mismatch row %0d",count);
      if(accepted)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;count=count+1;
    end
    if(count!=22)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
