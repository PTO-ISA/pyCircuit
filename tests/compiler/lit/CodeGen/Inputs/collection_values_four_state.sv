module tb;
  logic [0:1][0:2][7:0] a;
  logic [7:0] threshold = 4, base = 8'ha5, candidate = 8'ha7;
  logic [69:0] row = 1, col = 2, get_index = 3;
  logic guard = 1;
  wire [0:1][0:2][7:0] mapped; wire [0:2][0:1][7:0] transposed;
  wire [0:1][0:1][7:0] sliced; wire [0:1][0:2][7:0] rotated;
  wire [0:5][7:0] reshaped; wire [0:1][0:2][7:0] broadcast; wire [0:1][7:0] created;
  wire [2:0] ordinal, low0, low1, high0, high1;
  wire [7:0] selected, add, mul, and_value, or_value, xor_value, min_value, max_value, merged;
  wire in_range, lowvalid0, lowvalid1, highvalid0, highvalid1, merge_en;
  wire [5:0] mask;
  ac_top dut(.a(a), .threshold(threshold), .row(row), .col(col), .get_index(get_index),
    .guard(guard), .base(base), .candidate(candidate), .mapped(mapped), .transposed(transposed),
    .sliced(sliced), .rotated(rotated), .reshaped(reshaped), .broadcast(broadcast), .created(created),
    .ordinal(ordinal), .selected(selected), .in_range(in_range), .mask(mask),
    .low0(low0), .low1(low1), .lowvalid0(lowvalid0), .lowvalid1(lowvalid1),
    .high0(high0), .high1(high1), .highvalid0(highvalid0), .highvalid1(highvalid1),
    .fold_add(add), .fold_mul(mul), .fold_and(and_value), .fold_or(or_value), .fold_xor(xor_value),
    .fold_min(min_value), .fold_max(max_value), .merged(merged), .merge_en(merge_en));
  initial begin
    a[0][0]=1; a[0][1]=2; a[0][2]=3; a[1][0]=4; a[1][1]=5; a[1][2]=6;
    #1;
    if (mapped[0][0]!==5 || mapped[1][2]!==10 ||
        transposed[0][0]!==1 || transposed[0][1]!==4 || transposed[1][0]!==2 ||
        transposed[1][1]!==5 || transposed[2][0]!==3 || transposed[2][1]!==6 ||
        sliced[0][0]!==1 || sliced[0][1]!==3 || sliced[1][0]!==4 || sliced[1][1]!==6 ||
        rotated[0][0]!==3 || rotated[0][1]!==1 || rotated[0][2]!==2 ||
        rotated[1][0]!==6 || rotated[1][1]!==4 || rotated[1][2]!==5 ||
        reshaped[0]!==1 || reshaped[5]!==6 || broadcast[0][0]!==4 || broadcast[1][2]!==4 ||
        created[0]!==8'ha5 || created[1]!==8'ha7)
      $fatal(1,"row-major view/map/create oracle failed");
    if (ordinal!==5 || selected!==4 || in_range!==1 || mask!==56 ||
        low0!==3 || low1!==4 || high0!==5 || high1!==4 ||
        {lowvalid0,lowvalid1,highvalid0,highvalid1}!==4'b1111 ||
        add!==21 || mul!==208 || and_value!==0 || or_value!==7 || xor_value!==7 ||
        min_value!==1 || max_value!==6 || merged!==8'ha7 || merge_en!==1)
      $fatal(1,"index/match/choose/fold/merge oracle failed");
    threshold=7; get_index=0; #1;
    if (mask!==0 || low0!==0 || low1!==0 || {lowvalid0,lowvalid1,highvalid0,highvalid1}!==0 ||
        selected!==1 || in_range!==1) $fatal(1,"none is distinct from get in_range");
    row=0; col=3; #1;
    if (ordinal!==6) $fatal(1,"axis bounds must precede row-major flattening");
    row=70'd1 << 65; col=0; #1;
    if (ordinal!==6) $fatal(1,"wide coordinate high bits were discarded");
    row='x; col=3; get_index=6; #1;
    if (ordinal!==6 || in_range!==0 || selected!==8'hxx) $fatal(1,"known OOB dominates unknown coordinate");
    col=2; get_index='z; guard='x; #1;
    if (ordinal!==3'bxxx || in_range!==1'bx || selected!==8'hxx ||
        merged!==8'b101001x1 || merge_en!==1'bx) $fatal(1,"four-state get/index/merge oracle failed");
    a[0][0]='x; threshold=4; #1;
    if (mask!==6'b11100x || low0!==3'b0xx || low1!==3'bxxx ||
        lowvalid0!==1 || lowvalid1!==1 || high0!==5 || high1!==4 ||
        and_value!==0 || or_value!==8'bxxxxx111 || add!==8'hxx || mul!==8'hxx || xor_value!==8'hxx)
      $fatal(1,"four-state match/choose/fold oracle failed");
    $finish;
  end
endmodule
