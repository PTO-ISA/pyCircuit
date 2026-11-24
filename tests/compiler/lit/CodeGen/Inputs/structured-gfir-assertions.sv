module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0,predicate=1,allow=1;
  wire [7:0] count;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.*);
  string path,outcome;
  integer fd,n,samples=0;
  logic [7:0] expected_count;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%h %h %h %h %h %s\n",pyc_7079635f636c6b,pyc_7079635f727374,predicate,allow,expected_count,outcome);
      if(n!=6)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(count!==expected_count)$fatal(1,"counter mismatch row %0d",samples);
      accepted=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(accepted!==(outcome=="C"))$fatal(1,"acceptance mismatch row %0d",samples);
      if(accepted)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;samples=samples+1;
    end
    if(samples!=22)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
