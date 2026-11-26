`include "ram_ports.svh"
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0,req0=0,req1=0,write=0;
  logic [1:0] addr0=0,addr1=0,waddr=0,mask=0;
  logic [15:0] data=0;
  logic consume=0;
  wire [15:0] a,b;
  wire response_bit;
  wire [7:0] count;
  wire valid_a,valid_b,valid_bit;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.`PYC_A_VALID(valid_a), .`PYC_B_VALID(valid_b), .`PYC_BIT_VALID(valid_bit), .*);
  string path;
  integer fd,n,samples=0,expected_acceptance;
  logic expected_valid_a,expected_valid_b,expected_valid_bit,expected_bit;
  logic [7:0] expected_count;
  logic [15:0] expected_a,expected_b;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %d\n",pyc_7079635f636c6b,pyc_7079635f727374,req0,req1,write,addr0,addr1,waddr,data,mask,consume,expected_valid_a,expected_a,expected_valid_b,expected_b,expected_valid_bit,expected_bit,expected_count,expected_acceptance);
      if(n!=19)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(valid_a!==expected_valid_a || valid_b!==expected_valid_b || valid_bit!==expected_valid_bit || valid_bit!==valid_a)
        $fatal(1,"response availability mismatch row %0d",samples);
      if((expected_valid_a && a!==expected_a) || (expected_valid_b && b!==expected_b))
        $fatal(1,"response data mismatch row %0d",samples);
      if(count!==expected_count || (expected_valid_bit && response_bit!==expected_bit))
        $fatal(1,"consumer mismatch row %0d",samples);
      accepted=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(accepted!==1'(expected_acceptance))$fatal(1,"acceptance mismatch row %0d",samples);
      if(accepted)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;samples=samples+1;
    end
    if(samples!=46)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
