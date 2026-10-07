module tb;
  typedef struct packed {logic [4:0] tag; logic [69:0] data;} payload_t;
  logic clk = 0, rst = 1, en = 1;
  payload_t d, init;
  wire payload_t q;
  dffe #(.T(payload_t)) leaf(.clk(clk), .rst(rst), .en(en),
                            .d(d), .init(init), .q(q));
  initial begin
    init = {5'd18, 70'd7}; d = 'x;
    #1; clk = 1; #1;
    if (q !== init) $fatal(1, "typed reset failed");
    clk = 0; rst = 0; d.tag = 9; d.data[68] = 1'bz;
    #1; clk = 1; #1;
    if (q.tag !== 9 || q.data[68] !== 1'bz || q.data[69] !== 1'bx)
      $fatal(1, "four-state typed payload not preserved");
    $finish;
  end
endmodule
