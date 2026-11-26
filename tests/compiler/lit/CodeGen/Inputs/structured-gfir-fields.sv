module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
  logic [1:0] raw=0;
  logic [22:0] data=0,tag=0;
  wire [114:0] old,snapshot,candidate;
  wire [22:0] selected;
  wire in_range;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.*);
  string path;
  integer fd,n,count=0;
  logic [114:0] expected_old,expected_snapshot,expected_candidate;
  logic [22:0] expected_selected;
  logic expected_in_range;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%b %b %b %b %b %b %b %b %b %b\n",
                pyc_7079635f636c6b,pyc_7079635f727374,raw,data,tag,
                expected_old,expected_snapshot,expected_candidate,expected_selected,expected_in_range);
      if(n!=10)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(old!==expected_old || snapshot!==expected_snapshot || candidate!==expected_candidate ||
         selected!==expected_selected || in_range!==expected_in_range)
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
