`include "scheduling_widths.svh"
module scheduling_host;
  logic valid=0, take=0;
  logic [`INPUT_BITS-1:0] data=0;
  logic pyc_7079635f636c6b=0, pyc_7079635f727374=0;
  wire [`OUTPUT_BITS-1:0] result;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_local_error;
  logic failed=0;
  string before_state;
  `include "scheduling_rows.svh"
  pyc_root dut(.*);
  function automatic string snapshot;
    return $sformatf("%h", {@STATE_SNAPSHOTS@});
  endfunction
  task automatic host_reset;
    pyc_phase=4; #1;
    pyc_root_commit_ok=1; pyc_phase=2; #1;
    pyc_phase=0; pyc_root_commit_ok=0; failed=0; #1;
  endtask
  task automatic row(input integer index, input logic clock, push, pop,
      input logic [`INPUT_BITS-1:0] token, input logic [`OUTPUT_BITS-1:0] expected,
      input logic expected_failure, reset_host);
    valid=push; take=pop; data=token; pyc_7079635f636c6b=clock; #1;
    if (reset_host) host_reset();
    if (failed) begin
      if (!expected_failure) $fatal(1,"unlabeled sticky recovery view %0d",index);
    end else begin
      before_state=snapshot();
      pyc_phase=1; #1;
      pyc_root_commit_ok=(pyc_local_error === 1'b0);
      if (expected_failure) begin
        if (pyc_local_error !== 1'b1 || pyc_root_commit_ok !== 1'b0)
          $fatal(1,"missing scheduler fault view %0d",index);
        if (snapshot() != before_state) $fatal(1,"Work changed scheduler state");
        pyc_phase=2; #1;
        if (snapshot() != before_state) $fatal(1,"denied Xfer changed scheduler state");
        pyc_phase=3; #1; failed=1;
        if (snapshot() != before_state) $fatal(1,"Discard changed scheduler state");
      end else begin
        if (pyc_root_commit_ok !== 1'b1 || result !== expected)
          $fatal(1,"scheduler successful sample view %0d expected%h actual%h",index,expected,result);
        pyc_phase=2; #1;
      end
      pyc_phase=0; pyc_root_commit_ok=0; #1;
    end
  endtask
  initial begin
    #1;
    for (integer history=0; history<history_count; history=history+1) begin
      pyc_7079635f636c6b=0; host_reset();
      for (integer index=history_begin[history]; index<history_end[history]; index=index+1)
        row(index,vectors[index].clock,vectors[index].push,vectors[index].pop,
            vectors[index].token,vectors[index].expected,
            vectors[index].expected_failure,vectors[index].reset_host);
      $display("SCHEDULING_HISTORY_OK %s views=%0d",history_name[history],history_end[history]-history_begin[history]);
    end
    $finish;
  end
endmodule
