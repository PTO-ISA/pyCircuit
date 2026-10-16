module tb;
  logic left_first, left_second, right_first, right_second;
  logic left_clk, left_rst, right_clk, right_rst, sample_clk, sample_rst;
  logic pyc_7079635f636c6b, pyc_7079635f727374;
  logic [2:0] pyc_phase;
  logic pyc_root_commit_ok;
  logic selector, then_guard, else_guard, asserted, conditional_clk, conditional_rst;
  wire left_q, right_q, conditional_q;
  wire pyc_check_complete, pyc_sampling_error, pyc_local_error;

  pyc_root dut (.*);

  task automatic drive(input logic lf, input logic ls, input logic rf,
                       input logic rs, input logic rc, input logic rr,
                       input logic lc, input logic lr);
    begin
      left_first = lf;
      left_second = ls;
      right_first = rf;
      right_second = rs;
      left_clk = lc;
      left_rst = lr;
      right_clk = rc;
      right_rst = rr;
      sample_clk = lc;
      sample_rst = lr;
      selector = 0;
      then_guard = 1;
      else_guard = 1;
      asserted = 1;
      conditional_clk = 0;
      conditional_rst = 0;
      // Selected-root domain pins follow the existing hidden-zero C++ ABI.
      pyc_7079635f636c6b = 1'b0;
      pyc_7079635f727374 = 1'b0;
    end
  endtask

  task automatic reset_design;
    begin
      // Inputs settle before RESET_PREPARE just as they do before PREPARE.
      // The phase edge owns freezing; the preceding delay is not a cycle.
      #1;
      pyc_root_commit_ok = 1'b1;
      pyc_phase = 3'd4;
      #1;
      pyc_phase = 3'd2;
      #1;
      pyc_phase = 3'd0;
      pyc_root_commit_ok = 1'b0;
      #1;
    end
  endtask

  function automatic string status_name(input integer code);
    case (code)
      0: status_name = "ok";
      1: status_name = "guard";
      2: status_name = "physical";
      default: status_name = "incomplete";
    endcase
  endfunction

  task automatic show(input string name, input integer expected,
                      input logic expected_left, input logic expected_right);
    integer actual;
    begin
      if (pyc_check_complete !== 1'b1)
        actual = 3;
      else if (pyc_sampling_error !== 1'b0)
        actual = 2;
      else if (pyc_local_error !== 1'b0)
        actual = 1;
      else
        actual = 0;
      if (actual != expected)
        $fatal(1, "%s status %0d expected %0d complete=%b sampling=%b local=%b",
               name, actual, expected, pyc_check_complete,
               pyc_sampling_error, pyc_local_error);
      if (actual == 0) begin
        pyc_root_commit_ok = 1'b1;
        pyc_phase = 3'd2;
      end else begin
        pyc_root_commit_ok = 1'b0;
        pyc_phase = 3'd3;
      end
      #1;
      pyc_phase = 3'd0;
      pyc_root_commit_ok = 1'b0;
      #1;
      if (left_q !== expected_left || right_q !== expected_right)
        $fatal(1, "%s state mismatch left=%b right=%b", name, left_q, right_q);
      $display("ROW %s %s %b %b", name, status_name(actual), left_q, right_q);
    end
  endtask

  task automatic step(input string name, input integer expected,
                      input logic expected_left, input logic expected_right);
    begin
      // Settle combinational Rule and leaf inputs before the PREPARE edge.
      #1;
      pyc_root_commit_ok = 1'b0;
      pyc_phase = 3'd1;
      #1;
      show(name, expected, expected_left, expected_right);
    end
  endtask

  task automatic silent_ok;
    begin
      #1;
      pyc_root_commit_ok = 1'b0;
      pyc_phase = 3'd1;
      #1;
      if (pyc_check_complete !== 1'b1 || pyc_sampling_error !== 1'b0 ||
          pyc_local_error !== 1'b0)
        $fatal(1, "silent setup step failed");
      pyc_root_commit_ok = 1'b1;
      pyc_phase = 3'd2;
      #1;
      pyc_phase = 3'd0;
      pyc_root_commit_ok = 1'b0;
      #1;
    end
  endtask

  task automatic conditional_step(input string name, input integer expected,
                                  input logic expected_q,
                                  input logic expected_independent);
    integer actual;
    begin
      #1;
      pyc_phase = 3'd1;
      #1;
      if (pyc_check_complete !== 1'b1 || pyc_sampling_error !== 1'b0)
        $fatal(1, "conditional check or sampling incomplete");
      actual = pyc_local_error !== 1'b0 ? 1 : 0;
      if (actual != expected)
        $fatal(1, "%s status %0d expected %0d", name, actual, expected);
      pyc_root_commit_ok = actual == 0;
      pyc_phase = actual == 0 ? 3'd2 : 3'd3;
      #1;
      pyc_phase = 3'd0;
      pyc_root_commit_ok = 0;
      #1;
      if (conditional_q !== expected_q || left_q !== expected_independent ||
          right_q !== expected_independent)
        $fatal(1, "%s state cond=%b left=%b right=%b", name,
               conditional_q, left_q, right_q);
      $display("CROW %s %s %b %b %b", name, status_name(actual),
               conditional_q, left_q, right_q);
    end
  endtask

  initial begin
    pyc_phase = 3'd0;
    pyc_root_commit_ok = 1'b0;
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    $display("ROW RESET ok %b %b", left_q, right_q);

    drive(1, 1, 1, 1, 0, 0, 0, 0);
    step("BASE", 0, 0, 0);
    drive(1, 0, 1, 1, 1, 0, 1, 0);
    step("LOCAL_HOLD", 0, 0, 1);
    drive(1'bx, 1, 0, 1'bx, 1, 1'bx, 1, 1'bx);
    step("HELD_MASK", 0, 0, 1);
    drive(1, 1, 1, 1, 0, 1'bz, 0, 1'bz);
    step("FALL", 0, 0, 1);
    drive(1, 1, 1, 1, 0, 0, 1, 0);
    step("INDEPENDENT_LEFT", 0, 1, 1);
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    step("INDEPENDENT_RIGHT", 0, 1, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    silent_ok();
    drive(1, 0, 0, 1'bx, 1, 0, 1, 0);
    step("MASKED_RISING", 0, 1, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    silent_ok();
    drive(1'bx, 0, 1, 1, 1, 0, 1, 0);
    step("GUARD_ROLLBACK", 1, 1, 0);

    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    drive(1'bz, 0, 1, 1, 1, 0, 1, 0);
    step("INTENT_Z_ROLLBACK", 1, 0, 0);

    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    drive(1, 1, 1, 1, 1'bx, 0, 1'bx, 0);
    left_clk = 0;
    right_clk = 0;
    step("CLOCK_X", 2, 0, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    drive(1, 1, 1, 1, 1'bz, 0, 1'bz, 0);
    left_clk = 0;
    right_clk = 0;
    step("CLOCK_Z", 2, 0, 0);

    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 1'bx, 1, 1'bx);
    left_rst = 0;
    right_rst = 0;
    step("RESET_RISING_X", 2, 0, 0);
    drive(0, 1'bx, 0, 1'bx, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(0, 1'bx, 0, 1'bx, 1, 0, 1, 0);
    silent_ok();
    drive(1'bx, 1'bx, 1'bx, 1'bx, 1, 1'bx, 1, 1'bx);
    step("RESET_HELD_X", 0, 0, 0);
    drive(1'bx, 1'bx, 1'bx, 1'bx, 0, 1'bx, 0, 1'bx);
    step("RESET_FALL_X", 0, 0, 0);

    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 1'bz, 1, 1'bz);
    left_rst = 0;
    right_rst = 0;
    step("RESET_RISING_Z", 2, 0, 0);
    drive(0, 1'bx, 0, 1'bx, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(0, 1'bx, 0, 1'bx, 1, 0, 1, 0);
    silent_ok();
    drive(1'bx, 1'bx, 1'bx, 1'bx, 1, 1'bz, 1, 1'bz);
    step("RESET_HELD_Z", 0, 0, 0);
    drive(1'bx, 1'bx, 1'bx, 1'bx, 0, 1'bz, 0, 1'bz);
    step("RESET_FALL_Z", 0, 0, 0);

    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1'bx, 1'bx, 1'bx, 1'bx, 1, 1, 1, 1);
    step("RESET_ACTIVE", 0, 0, 0);
    drive(1'bx, 1'bx, 1'bx, 1'bx, 1'bx, 1'bx, 1'bx, 1'bx);
    reset_design();
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    $display("ROW EXPLICIT_RESET ok %b %b", left_q, right_q);

    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    #1;
    pyc_phase = 3'd1;
    #1;
    drive(1'bx, 1'bx, 1'bx, 1'bx, 1'bx, 1'bx, 1'bx, 1'bx);
    show("FROZEN_AFTER_WORK", 0, 1, 1);

    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    #1;
    pyc_phase = 3'd1;
    #1;
    if (pyc_check_complete !== 1'b1 || pyc_sampling_error !== 1'b0 ||
        pyc_local_error !== 1'b0)
      $fatal(1, "discard preparation failed");
    pyc_phase = 3'd3;
    #1;
    pyc_phase = 3'd0;
    #1;
    if (left_q !== 0 || right_q !== 0)
      $fatal(1, "discard committed state");
    $display("ROW DISCARDED ok %b %b", left_q, right_q);
    step("DISCARD_RETRY", 0, 1, 1);

    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 0; then_guard = 1'bx; else_guard = 1; asserted = 1;
    conditional_clk = 1;
    conditional_step("SELECT_ZERO", 0, 1, 1);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 1; then_guard = 1; else_guard = 1'bz; asserted = 1;
    conditional_clk = 1;
    conditional_step("SELECT_ONE", 0, 1, 1);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 0; then_guard = 0; else_guard = 1; asserted = 1;
    conditional_clk = 1;
    conditional_step("SKIP_THEN_FALSE", 0, 1, 1);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 0; then_guard = 1'bx; else_guard = 1; asserted = 1;
    conditional_clk = 1;
    conditional_step("SKIP_THEN_X", 0, 1, 1);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 0; then_guard = 1'bz; else_guard = 1; asserted = 1;
    conditional_clk = 1;
    conditional_step("SKIP_THEN_Z", 0, 1, 1);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 1; then_guard = 1; else_guard = 0; asserted = 1;
    conditional_clk = 1;
    conditional_step("SKIP_ELSE_FALSE", 0, 1, 1);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 1; then_guard = 1; else_guard = 1'bx; asserted = 1;
    conditional_clk = 1;
    conditional_step("SKIP_ELSE_X", 0, 1, 1);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 1; then_guard = 1; else_guard = 1'bz; asserted = 1;
    conditional_clk = 1;
    conditional_step("SKIP_ELSE_Z", 0, 1, 1);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 1; then_guard = 0; else_guard = 1'bx; asserted = 1'bx;
    conditional_clk = 1;
    conditional_step("THEN_FALSE_HOLD", 0, 0, 1);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 0; then_guard = 1'bx; else_guard = 0; asserted = 1'bx;
    conditional_clk = 1;
    conditional_step("ELSE_FALSE_HOLD", 0, 0, 1);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 1; then_guard = 1'bx; else_guard = 0; asserted = 1;
    conditional_clk = 1;
    conditional_step("THEN_X_ROLLBACK", 1, 0, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 1; then_guard = 1'bz; else_guard = 0; asserted = 1;
    conditional_clk = 1;
    conditional_step("THEN_Z_ROLLBACK", 1, 0, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 0; then_guard = 0; else_guard = 1'bx; asserted = 1;
    conditional_clk = 1;
    conditional_step("ELSE_X_ROLLBACK", 1, 0, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 0; then_guard = 0; else_guard = 1'bz; asserted = 1;
    conditional_clk = 1;
    conditional_step("ELSE_Z_ROLLBACK", 1, 0, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 1'bx; then_guard = 0; else_guard = 0; asserted = 1;
    conditional_clk = 1;
    conditional_step("SELECT_X_LATE_FALSE", 1, 0, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 1'bz; then_guard = 0; else_guard = 0; asserted = 1;
    conditional_clk = 1;
    conditional_step("SELECT_Z_LATE_FALSE", 1, 0, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 1; then_guard = 1; else_guard = 1'bx; asserted = 0;
    conditional_clk = 1;
    conditional_step("THEN_ASSERT_FALSE", 1, 0, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 0; then_guard = 1'bx; else_guard = 1; asserted = 0;
    conditional_clk = 1;
    conditional_step("ELSE_ASSERT_FALSE", 1, 0, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 1; then_guard = 1; else_guard = 1'bx; asserted = 1'bx;
    conditional_clk = 1;
    conditional_step("THEN_ASSERT_X", 1, 0, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    drive(1, 1, 1, 1, 1, 0, 1, 0);
    selector = 0; then_guard = 1'bx; else_guard = 1; asserted = 1'bz;
    conditional_clk = 1;
    conditional_step("ELSE_ASSERT_Z", 1, 0, 0);
    drive(1, 1, 1, 1, 0, 0, 0, 0);
    reset_design();
    silent_ok();
    selector = 1; conditional_clk = 1;
    silent_ok();
    selector = 1'bx; then_guard = 1'bx; else_guard = 1'bz; asserted = 1'bx;
    conditional_step("HELD_UNKNOWN", 0, 1, 0);
    conditional_clk = 0;
    conditional_step("FALLING_UNKNOWN", 0, 1, 0);
    conditional_clk = 1; conditional_rst = 1;
    conditional_step("RESET_UNKNOWN", 0, 0, 0);

    $display("PASS sampled admission lifecycle");
    $finish(0);
  end
endmodule
