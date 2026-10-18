module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
  logic valid=0,take=1;
  logic [30:0] request='0;
  wire [52:0] result;
  logic[2:0] pyc_phase=0;
logic pyc_root_commit_ok=0;
wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
pyc_root dut(.*);
// Each stimulus row is one prepared Work epoch. Permission is frozen from
// the complete error channel before any physical queue/state Xfer.
task automatic checked_prepare;
 pyc_root_commit_ok=0;pyc_phase=1;#1;
 pyc_root_commit_ok=(pyc_check_complete===1'b1 && pyc_sampling_error===1'b0 && pyc_local_error===1'b0);
 if(pyc_root_commit_ok!==1'b1)begin
  pyc_phase=3;#1;
  $fatal(1,"prepared pipeline row failed completion/sampling/check validation");
 end
endtask
task automatic checked_xfer;
 pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
endtask
task automatic host_reset;
 pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
 pyc_phase=0;pyc_root_commit_ok=0;#1;
endtask
task automatic physical_reset_edges;
 pyc_7079635f636c6b=1;checked_prepare();checked_xfer();
 pyc_7079635f636c6b=0;pyc_7079635f727374=0;checked_prepare();checked_xfer();
endtask

  integer file,status,row=0,clock_level,reset_level,valid_level,take_level;
  logic [30:0] packet;
  logic [52:0] expected,mask,normalized;
  initial begin
    #1;host_reset();physical_reset_edges();
    // The existing verifier runs in its build cwd. These expected rows come
    // from the independent RAM/queue oracle, never from native DUT output.
    file=$fopen("memory-banks-oracle.rows","r");
    if(file==0)$fatal(1,"missing independent memory-banks expected rows");
    while(!$feof(file)) begin
      status=$fscanf(file,"%d %d %d %h %d %b %b\n",clock_level,reset_level,valid_level,packet,take_level,expected,mask);
      if(status!=7)$fatal(1,"memory_banks row %0d malformed expected row (%0d)",row,status);
      pyc_7079635f636c6b=clock_level[0];pyc_7079635f727374=reset_level[0];valid=valid_level[0];request=packet;take=take_level[0];
      // Compare settled Work before committing the physical edge.
      #1;checked_prepare();
      if((result&mask)!==(expected&mask))
        $fatal(1,"memory_banks precommit row %0d expected %b mask %b actual %b",row,expected,mask,result);
      normalized=result&mask;$display("WORK %0d %b",row,normalized);
      checked_xfer();row=row+1;
    end
    $fclose(file);if(row<400)$fatal(1,"memory_banks finite stress rows missing");$finish;
  end
endmodule
