module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
  logic [1:0] raw=0;
  logic [7:0] data=0,tag=0;
  wire [39:0] old,snapshot,candidate;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.*);
  string path;
  integer fd,n,count=0;
  logic [39:0] expected_old,expected_snapshot,expected_candidate;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%b %b %b %b %b %b %b %b\n",pyc_7079635f636c6b,pyc_7079635f727374,raw,data,tag,expected_old,expected_snapshot,expected_candidate);
      if(n!=8)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(old!==expected_old || snapshot!==expected_snapshot || candidate!==expected_candidate)
        $fatal(1,"field write mismatch row %0d",count);
      if(pyc_check_complete!==1 || pyc_sampling_error!==0 || pyc_local_error!==0)
        $fatal(1,"unexpected failure row %0d",count);
      pyc_root_commit_ok=1;pyc_phase=2;#1;
      pyc_phase=0;pyc_root_commit_ok=0;#1;count=count+1;
    end
    if(count!=16)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
