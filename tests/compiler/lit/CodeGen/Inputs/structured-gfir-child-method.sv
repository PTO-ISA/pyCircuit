`include "child_method_ports.svh"
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0,allow=1;
  wire [7:0] q,observed,first,second,q_b,observed_b,untouched;
  wire first_valid,second_valid;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.`PYC_FIRST_VALID(first_valid), .`PYC_SECOND_VALID(second_valid), .*);
  string path;
  integer fd,n,samples=0;
  logic [7:0] expected_old,expected_first,expected_second,expected_other;
  logic expected_available,expected_acceptance;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%h %h %h %h %h %h %h %h %h\n",pyc_7079635f636c6b,pyc_7079635f727374,allow,expected_old,expected_available,expected_first,expected_second,expected_acceptance,expected_other);
      if(n!=9)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(q_b!==expected_other || observed_b!==expected_other || untouched!==0)
        $fatal(1,"instance identity mismatch row %0d",samples);
      if(q!==expected_old || observed!==expected_old)$fatal(1,"candidate leaked row %0d",samples);
      if(first_valid!==expected_available || second_valid!==expected_available)
        $fatal(1,"availability mismatch row %0d",samples);
      if(expected_available && (first!==expected_first || second!==expected_second))
        $fatal(1,"saved candidate mismatch row %0d",samples);
      accepted=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(accepted!==expected_acceptance)$fatal(1,"acceptance mismatch row %0d",samples);
      if(accepted)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;samples=samples+1;
    end
    if(samples!=22)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
