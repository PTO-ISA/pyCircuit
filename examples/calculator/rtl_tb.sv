// Physical IR pyc_clk/pyc_rst use the existing injective generated identifier
// spelling: pyc_7079635f636c6b / pyc_7079635f727374.
//
// Independent RTL oracle for the u64 keypad calculator. The scoreboard is a
// transcription of the historical u64 state machine and predicts expected
// values only; every compared value is read from the generated DUT.
//
// The epoch loop and the stimulus trace mirror `driver.cpp` exactly, because
// cmake/verify_example.py requires the RTL `WORK` records to be byte-identical
// to the native runner's.
//
// Key codes: 0..9 digit, 10 add, 11 sub, 12 mul, 13 div, 14 equals, 15 clear.
module tb;
  typedef struct packed {
    logic [63:0] display;
    logic [1:0] op_pending;
  } calc_result_t;

  localparam int EDGES = 237;   // one stimulus entry per clock-high epoch
  localparam int FRAMES = 2 * EDGES;

  logic pyc_7079635f636c6b = 0, pyc_7079635f727374 = 1;
  logic [4:0] key = 0;
  logic key_press = 0;
  calc_result_t result;
  pyc_root dut(.*);

  logic [4:0] tkey[EDGES];
  bit tpress[EDGES];

  // Independent scoreboard state.
  logic [63:0] lhs = 0, rhs = 0, shown = 0;
  logic [1:0] pending = 0;
  bit entering_rhs = 0;

  function automatic void press(input int k);
    bit is_digit = (k <= 9);
    bit is_add = (k == 10), is_sub = (k == 11);
    bit is_mul = (k == 12), is_div = (k == 13);
    bit is_eq = (k == 14), is_ac = (k == 15);
    logic [63:0] divisor, computed;
    if (is_digit) begin
      if (entering_rhs) begin
        rhs = rhs * 64'd10 + 64'(k);
        shown = rhs;
      end else begin
        lhs = lhs * 64'd10 + 64'(k);
        shown = lhs;
      end
    end
    if (is_add || is_sub || is_mul || is_div) begin
      entering_rhs = 1;
      rhs = 0;
      if (is_add) pending = 0;
      if (is_sub) pending = 1;
      if (is_mul) pending = 2;
      if (is_div) pending = 3;
    end
    divisor = (rhs == 0) ? 64'd1 : rhs;  // zero divisor is replaced by one
    computed = lhs;
    if (pending == 0) computed = lhs + rhs;
    if (pending == 1) computed = lhs - rhs;
    if (pending == 2) computed = lhs * rhs;
    if (pending == 3) computed = lhs / divisor;
    if (is_eq) begin
      lhs = computed;
      shown = computed;
      rhs = 0;
      entering_rhs = 0;
    end
    if (is_ac) begin
      lhs = 0; rhs = 0; shown = 0; pending = 0; entering_rhs = 0;
    end
  endfunction

  int cursor = 0;
  task automatic emit(input int k, input bit p);
    tkey[cursor] = 5'(k);
    tpress[cursor] = p;
    cursor++;
  endtask
  task automatic pair(input int k);
    emit(k, 1);
    emit(0, 0);
  endtask

  task automatic number(input string digits);
    for (int i = 0; i < digits.len(); i++) pair(int'(digits.getc(i)) - 48);
  endtask

  task automatic buildTrace();
    cursor = 0;
    // Historical idle shape: reset window, then a stronger idle hold.
    for (int i = 0; i < 3; i++) emit(0, 0);
    for (int i = 0; i < 64; i++) emit(0, 0);
    pair(1); pair(2); pair(10); pair(3); pair(4); pair(14);      // 12 + 34
    pair(15); pair(7); pair(12); pair(6); pair(14);              // 7 * 6
    pair(15); pair(9); pair(11); pair(4); pair(14);              // 9 - 4
    pair(15);                                                    // AC
    pair(8); pair(13); pair(14);                                 // 8 / 0 = 8
    pair(15); pair(1); pair(0); pair(0); pair(13); pair(4); pair(14); // 100 / 4
    pair(15); number("18446744073709551615");  // u64 max
    pair(13); pair(3); pair(14); // max / 3
    pair(12); pair(3); pair(14); // restore max
    pair(10); pair(1); pair(14); // addition wraps to zero
    pair(11); pair(1); pair(14); // subtraction wraps to max
    pair(12); pair(2); pair(14); // multiplication wraps to max-1
    pair(15); number("184467440737095516150"); // decimal entry wraps
    if (cursor != EDGES) $fatal(1, "stimulus count disagrees with EDGES");
  endtask

  initial begin
    buildTrace();
    // Physical reset establishes the same starting state as native host Reset.
    #1; pyc_7079635f636c6b = 1; #1;
    pyc_7079635f636c6b = 0; pyc_7079635f727374 = 0; #1;
    lhs = 0; rhs = 0; shown = 0; pending = 0; entering_rhs = 0;

    // Each frame is one native Work/Xfer epoch. Drive data while preserving
    // the previous clock level, observe settled old-Q, then apply this frame's
    // edge and advance the independent committed-state model.
    for (int frame = 0; frame < FRAMES; frame++) begin
      // Two asserted rising edges, then the deasserted idle edge.
      pyc_7079635f727374 = (frame < 4);
      key = 5'(0);
      key_press = 0;
      if (frame % 2 == 1) begin
        key = tkey[frame / 2];
        key_press = tpress[frame / 2];
      end
      #1;
      if (result.display !== shown || result.op_pending !== pending) begin
        $display("MISMATCH frame %0d key %0d expected %0d/%0d got %0d/%0d",
                 frame, key, shown, pending, result.display, result.op_pending);
        $fatal(1, "calculator RTL oracle failed at frame %0d", frame);
      end
      $display("WORK %0d %0d %0d", frame, result.display, result.op_pending);
      pyc_7079635f636c6b = 1'(frame % 2);
      #1;
      if (frame % 2 == 1 && tpress[frame / 2])
        press(int'(tkey[frame / 2]));
    end
    if (result.display !== 64'd18446744073709551606)
      $fatal(1, "decimal entry must wrap modulo 2^64");
    $display("PASS calculator rtl");
    $finish;
  end
endmodule
