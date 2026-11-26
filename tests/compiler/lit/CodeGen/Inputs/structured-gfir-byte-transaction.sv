`include "byte_ports.svh"
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0,earlier=0,later=0,write=0;
  logic [1:0] address=0,waddr=0,mask=0;
  logic [15:0] data=0;
  wire [15:0] value;
  wire [7:0] count;
  wire value_valid;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.`PYC_VALUE_VALID(value_valid), .*);
  string path,action;
  integer fd,n,samples=0;
  logic expected_valid;
  logic [15:0] expected;
  logic [7:0] expected_count;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%s %h %h %h %h %h %h %h %h %h %h %h %h\n",action,pyc_7079635f636c6b,pyc_7079635f727374,earlier,later,address,write,waddr,data,mask,expected_valid,expected,expected_count);
      if(n!=13)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(action=="F")begin
        if(!(pyc_sampling_error===1 || pyc_local_error===1))
          $fatal(1,"missing physical failure row %0d",samples);
        pyc_phase=3;#1;pyc_phase=2;#1;
      end else begin
        if(value_valid!==expected_valid || count!==expected_count)
          $fatal(1,"availability/counter mismatch row %0d",samples);
        if(expected_valid && value!==expected)$fatal(1,"read mismatch row %0d",samples);
        if(pyc_check_complete!==1 || pyc_sampling_error!==0 || pyc_local_error!==0)
          $fatal(1,"unexpected failure row %0d",samples);
        pyc_root_commit_ok=1;pyc_phase=2;#1;
      end
      pyc_phase=0;pyc_root_commit_ok=0;#1;samples=samples+1;
    end
    if(samples!=24)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
