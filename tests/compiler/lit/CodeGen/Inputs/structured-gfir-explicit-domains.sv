module tb;
  logic clk_e=0,rst_e=0,clk_w=0,rst_w=0,allow_e=1,allow_w=1;
  wire [7:0] east_value,west_value;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.*);
  string path;
  integer fd,n,samples=0,expected_acceptance;
  logic [7:0] expected_east,expected_west;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%h %h %h %h %h %h %h %h %d\n",clk_e,rst_e,clk_w,rst_w,allow_e,allow_w,expected_east,expected_west,expected_acceptance);
      if(n!=9)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(east_value!==expected_east || west_value!==expected_west)
        $fatal(1,"domain state mismatch row %0d",samples);
      accepted=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(accepted!==1'(expected_acceptance))$fatal(1,"acceptance mismatch row %0d",samples);
      if(accepted)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;samples=samples+1;
    end
    if(samples!=16)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
