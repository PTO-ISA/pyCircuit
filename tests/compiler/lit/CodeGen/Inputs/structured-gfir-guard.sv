module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0,a=0,b=0,select=0;
  wire [7:0] value,sibling;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.*);
  string path;
  integer fd,n,count=0,ok;
  logic [7:0] expected_value,expected_sibling;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%h %h %h %h %h %h %h %d\n",pyc_7079635f636c6b,pyc_7079635f727374,a,b,select,expected_value,expected_sibling,ok);
      if(n!=8)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(value!==expected_value || sibling!==expected_sibling)$fatal(1,"state mismatch row %0d",count);
      accepted=(pyc_check_complete===1'b1 && pyc_sampling_error===1'b0 && pyc_local_error===1'b0);
      if(accepted!==1'(ok))$fatal(1,"guard acceptance mismatch row %0d",count);
      if(ok)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;count=count+1;
    end
    if(count!=16)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
