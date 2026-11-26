`include "fifo_ports.svh"
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
  logic offer=0,data=0,early=0,late=0,take=0,consume=0;
  wire viewed,transferred,derived,in_ready,in_valid,head,out_valid,out_data;
  wire [7:0] moves,seen;
  wire view_valid,move_valid,derived_valid;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.`PYC_VIEW_VALID(view_valid), .`PYC_MOVE_VALID(move_valid),
               .`PYC_DERIVED_VALID(derived_valid), .*);
  string path;
  integer fd,n,samples=0,expected_acceptance;
  logic expected_view_valid,expected_viewed,expected_move_valid,expected_transferred;
  logic expected_derived_valid,expected_derived,expected_ready,expected_present;
  logic expected_head,expected_output_valid,expected_output;
  logic [7:0] expected_moves,expected_seen;
  bit accepted;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    #1;pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %d\n",
        pyc_7079635f636c6b,pyc_7079635f727374,offer,data,early,late,take,consume,
        expected_view_valid,expected_viewed,expected_move_valid,expected_transferred,
        expected_derived_valid,expected_derived,expected_ready,expected_present,
        expected_head,expected_output_valid,expected_output,expected_moves,expected_seen,
        expected_acceptance);
      if(n!=22)$fatal(1,"invalid row");
      #1;pyc_phase=1;#1;
      if(view_valid!==expected_view_valid || move_valid!==expected_move_valid ||
         derived_valid!==expected_derived_valid || derived_valid!==move_valid)
        $fatal(1,"availability mismatch row %0d",samples);
      if((expected_view_valid && viewed!==expected_viewed) ||
         (expected_move_valid && transferred!==expected_transferred) ||
         (expected_derived_valid && derived!==expected_derived))
        $fatal(1,"old-head result mismatch row %0d",samples);
      if(in_ready!==expected_ready || in_valid!==expected_present || head!==expected_head ||
         out_valid!==expected_output_valid || out_data!==expected_output ||
         moves!==expected_moves || seen!==expected_seen)
        $fatal(1,"committed state mismatch row %0d",samples);
      accepted=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(accepted!==1'(expected_acceptance))$fatal(1,"acceptance mismatch row %0d",samples);
      if(accepted)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;samples=samples+1;
    end
    if(samples!=34)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
