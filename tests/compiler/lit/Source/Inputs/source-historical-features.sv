module tb;
  logic pyc_7079635f636c6b=0, pyc_7079635f727374=1;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  task automatic host_reset;
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
  endtask
  task automatic managed_commit;
    pyc_root_commit_ok=0;pyc_phase=1;#1;
    pyc_root_commit_ok=(pyc_check_complete===1'b1 && pyc_sampling_error===1'b0 && pyc_local_error===1'b0);
    if(pyc_root_commit_ok!==1'b1)begin pyc_phase=3;#1;$fatal(1,"prepared historical row failed completion/check validation");end
    pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
  endtask
`ifdef FEATURE_COUNTER
  logic en=0;
  wire [7:0] result;
`elsif FEATURE_TRACE
  logic [7:0] in_x=0;
  wire [15:0] result;
`else
  logic [7:0] in_a=0;
  wire [7:0] result;
`endif
  pyc_root dut(.*);
  integer file,fields,stimulus,pre0,post0,pre1,post1,n=0;
  string rows;
  task check_values(input integer first,second);
`ifdef FEATURE_TRACE
    if(result[15:8] !== 8'(first) || result[7:0] !== 8'(second))
      $fatal(1,"trace cycle%0d expected%0d,%0d actual%0d,%0d",n,first,second,result[15:8],result[7:0]);
`else
    if(result !== 8'(first))$fatal(1,"cycle%0d expected%0d actual%0d",n,first,result);
`endif
  endtask
  task drive_data(input integer value);
`ifdef FEATURE_COUNTER
    en=1'(value);
`elsif FEATURE_TRACE
    in_x=8'(value);
`else
    in_a=8'(value);
`endif
  endtask
  initial begin
    if(!$value$plusargs("rows=%s",rows))$fatal(1,"missing historical rows");
    file=$fopen(rows,"r");if(!file)$fatal(1,"cannot open historical rows");
    #1;host_reset();
    repeat(2)begin pyc_7079635f636c6b=1;#1;managed_commit();pyc_7079635f636c6b=0;#1;managed_commit();check_values(0,0);end
    pyc_7079635f727374=0;
    while(!$feof(file))begin
      fields=$fscanf(file,"%d %d %d %d %d\n",stimulus,pre0,post0,pre1,post1);
      if(fields==5)begin
        drive_data(stimulus);#1;check_values(pre0,pre1);
        pyc_7079635f636c6b=1;#1;managed_commit();check_values(post0,post1);
        pyc_7079635f636c6b=0;#1;managed_commit();
        $display("CYCLE %0d %0d %0d %0d %0d",n,pre0,post0,pre1,post1);n++;
      end else if(fields!=-1)$fatal(1,"malformed historical row");
    end
    if(n!=`FEATURE_CYCLES)$fatal(1,"wrong historical cycle count");
    drive_data(1);pyc_7079635f727374=1;#1;pyc_7079635f636c6b=1;#1;managed_commit();check_values(0,0);
    $display("PASS");$finish;
  end
endmodule
