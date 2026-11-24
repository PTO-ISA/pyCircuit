`include "child_availability_ports.svh"
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
  logic left_allow=0,left_payload=0,right_allow=0,right_payload=0;
  logic left_enable=0,right_enable=0;
  wire [8:0] packet;
  wire right_gate;
  wire [7:0] left_committed,right_committed,left_seen,right_seen;
  wire packet_valid,right_valid;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.`PYC_PACKET_VALID(packet_valid), .`PYC_RIGHT_VALID(right_valid), .*);
  string path;
  integer fd,n,samples=0,expected_acceptance;
  logic expected_left_valid,expected_right_valid;
  logic [7:0] expected_left_q,expected_right_q,expected_left_seen,expected_right_seen;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%h %h %h %h %h %h %h %h %h %h %h %h %h %h %d\n",
        pyc_7079635f636c6b,pyc_7079635f727374,left_allow,left_payload,
        right_allow,right_payload,left_enable,right_enable,
        expected_left_valid,expected_right_valid,expected_left_q,expected_right_q,
        expected_left_seen,expected_right_seen,expected_acceptance);
      if(n!=15)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(packet_valid!==expected_left_valid || right_valid!==expected_right_valid)
        $fatal(1,"availability mismatch row %0d",samples);
      if(left_committed!==expected_left_q || right_committed!==expected_right_q ||
         left_seen!==expected_left_seen || right_seen!==expected_right_seen ||
         packet[7:0]!==expected_left_q)
        $fatal(1,"committed state mismatch row %0d",samples);
      if(expected_left_valid && packet[8]!==left_payload)
        $fatal(1,"left payload mismatch row %0d",samples);
      if(expected_right_valid && right_gate!==right_payload)
        $fatal(1,"right payload mismatch row %0d",samples);
      accepted=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(accepted!==1'(expected_acceptance))$fatal(1,"acceptance mismatch row %0d",samples);
      if(accepted)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;samples=samples+1;
    end
    if(samples!=28)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
