`include "ram_composition_ports.svh"
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
  logic active=0,addr=0,mask=0,late=0,response=0;
  logic [7:0] data=0;
  wire [7:0] value,count;
  wire value_valid;
`ifdef MIXED
  wire ready,present;
  wire [7:0] head;
`endif
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.`PYC_VALUE_VALID(value_valid),.*);
  string path;
  integer fd,n,samples=0;
  logic expected_valid,expected_ready,expected_present,expected_acceptance,discard;
  logic [7:0] expected_value,expected_count,expected_head;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b\n",
                pyc_7079635f636c6b,pyc_7079635f727374,active,addr,data,mask,late,response,
                expected_valid,expected_value,expected_count,expected_ready,expected_present,
                expected_head,expected_acceptance,discard);
      if(n!=16)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(value_valid!==expected_valid || (expected_valid && value!==expected_value) || count!==expected_count)
        $fatal(1,"RAM admission/state mismatch row %0d",samples);
`ifdef MIXED
      if(ready!==expected_ready || present!==expected_present || head!==expected_head)
        $fatal(1,"FIFO atomicity mismatch row %0d",samples);
`endif
      accepted=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(accepted!==expected_acceptance)$fatal(1,"acceptance mismatch row %0d",samples);
      if(accepted && !discard)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;samples=samples+1;
    end
`ifdef MIXED
    if(samples!=28)$fatal(1,"incomplete rows");
`else
    if(samples!=27)$fatal(1,"incomplete rows");
`endif
    $fclose(fd);$finish;
  end
endmodule
