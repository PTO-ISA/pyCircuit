`include "ram_ports.svh"
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0,req0=0,req1=0,write=0;
  logic [1:0] addr0=0,addr1=0,waddr=0,mask=0;
  logic [15:0] data=0;
  wire [15:0] a,b;
  wire valid_a,valid_b;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.`PYC_A_VALID(valid_a), .`PYC_B_VALID(valid_b), .*);
  string path;
  integer fd,n,count=0;
  logic expected_valid_a,expected_valid_b;
  logic [15:0] expected_a,expected_b;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%h %h %h %h %h %h %h %h %h %h %h %h %h %h\n",pyc_7079635f636c6b,pyc_7079635f727374,req0,req1,write,addr0,addr1,waddr,data,mask,expected_valid_a,expected_a,expected_valid_b,expected_b);
      if(n!=14)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(valid_a!==expected_valid_a || valid_b!==expected_valid_b)
        $fatal(1,"response availability mismatch row %0d",count);
      if((expected_valid_a && a!==expected_a) || (expected_valid_b && b!==expected_b))
        $fatal(1,"response data mismatch row %0d",count);
      if(pyc_check_complete!==1 || pyc_sampling_error!==0 || pyc_local_error!==0)
        $fatal(1,"unexpected failure row %0d",count);
      pyc_root_commit_ok=1;pyc_phase=2;#1;
      pyc_phase=0;pyc_root_commit_ok=0;#1;count=count+1;
    end
    if(count!=30)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
