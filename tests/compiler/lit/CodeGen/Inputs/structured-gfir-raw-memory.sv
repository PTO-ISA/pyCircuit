module tb;
  logic pyc_7079635f636c6b=0, pyc_7079635f727374=0;
  logic ren=0, write=0;
  logic [1:0] raddr=0,other_addr=0,byte_addr=0,waddr=0,mask=0;
  logic [15:0] data=0;
  wire [15:0] single,dual0,dual1,bytes;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.*);
  string path,action;
  integer fd,status,count=0;
  logic [15:0] q,qother,expected_bytes;
  bit accepted;
  initial begin
    if (!$value$plusargs("rows=%s",path)) $fatal(1,"missing rows");
    fd=$fopen(path,"r");
    if (!fd) $fatal(1,"cannot open rows");
    // Existing managed host Reset and globally permitted Xfer convention.
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while (!$feof(fd)) begin
      status=$fscanf(fd,"%s %h %h %h %h %h %h %h %h %h %h %h %h %h\n",
        action,pyc_7079635f636c6b,pyc_7079635f727374,ren,raddr,other_addr,
        byte_addr,write,waddr,data,mask,q,qother,expected_bytes);
      if (status!=14) $fatal(1,"bad row %0d fields %0d",count,status);
      #1;pyc_phase=1;#1;
      accepted=(pyc_check_complete===1'b1 && pyc_sampling_error===1'b0 && pyc_local_error===1'b0);
      if (action=="F") begin
        if (!((pyc_sampling_error===1'b1) || (pyc_local_error===1'b1)))
          $fatal(1,"missing memory address failure at row %0d",count);
      end else begin
        if (!accepted) $fatal(1,"unexpected failure at row %0d",count);
        if (single!==q || dual0!==q || dual1!==qother || bytes!==expected_bytes)
          $fatal(1,"memory oracle row %0d got %h %h %h %h expected %h %h %h",count,single,dual0,dual1,bytes,q,qother,expected_bytes);
      end
      if (action=="C") begin
        pyc_root_commit_ok=accepted;pyc_phase=2;#1;
      end else begin
        pyc_root_commit_ok=0;pyc_phase=3;#1;
        // Match native DiscardNext followed by Xfer: no pending work survives.
        pyc_phase=2;#1;
      end
      pyc_phase=0;pyc_root_commit_ok=0;#1;
      count=count+1;
    end
    if (count!=31) $fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
