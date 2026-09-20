module tb_versioned_recovery;
  logic clk = 0;
  logic rst = 1;
  logic in0_valid = 0;
  logic [16:0] in0_data = '0;
  logic in1_valid = 0;
  logic [15:0] in1_data = '0;
  logic in2_valid = 0;
  logic [8:0] in2_data = '0;
  logic in3_valid = 0;
  logic [14:0] in3_data = '0;
  logic in4_valid = 0;
  logic [16:0] in4_data = '0;
  logic in5_valid = 0;
  logic [15:0] in5_data = '0;
  logic out_ready = 0;
  wire out_valid;
  wire [7:0] out_data;
  wire in0_ready;
  wire in1_ready;
  wire in2_ready;
  wire in3_ready;
  wire in4_ready;
  wire in5_ready;

  versioned_recovery dut(.*);
  always #5 clk = ~clk;

  function automatic [16:0] entry(
      input logic slot, input logic [1:0] generation,
      input logic [2:0] epoch, input logic [1:0] attempt,
      input logic [7:0] payload);
    entry = {slot, 1'b1, generation, epoch, attempt, payload};
  endfunction

  function automatic [15:0] completion(
      input logic slot, input logic [1:0] generation,
      input logic [2:0] epoch, input logic [1:0] attempt,
      input logic [7:0] payload);
    completion = {slot, generation, epoch, attempt, payload};
  endfunction

  function automatic [8:0] read_ref(
      input logic retained, input logic slot, input logic [1:0] generation,
      input logic [2:0] epoch, input logic [1:0] attempt);
    read_ref = {retained, slot, generation, epoch, attempt};
  endfunction

  function automatic [14:0] recovery(
      input logic [2:0] next_epoch, input logic [1:0] checkpoint,
      input logic boundary, input logic slot, input logic [1:0] generation,
      input logic [2:0] transaction_epoch, input logic [1:0] attempt);
    recovery = {1'b1, next_epoch, checkpoint, boundary, slot, generation,
                transaction_epoch, attempt};
  endfunction

  task automatic send_alloc(input logic [16:0] value);
    @(negedge clk);
    in0_data = value;
    in0_valid = 1;
    do @(posedge clk); while (!in0_ready);
    @(negedge clk);
    in0_valid = 0;
    repeat (2) @(posedge clk);
  endtask

  task automatic send_completion(input logic [15:0] value);
    @(negedge clk);
    in1_data = value;
    in1_valid = 1;
    do @(posedge clk); while (!in1_ready);
    @(negedge clk);
    in1_valid = 0;
    repeat (2) @(posedge clk);
  endtask

  task automatic send_recovery(input logic [14:0] value);
    @(negedge clk);
    in3_data = value;
    in3_valid = 1;
    do @(posedge clk); while (!in3_ready);
    @(negedge clk);
    in3_valid = 0;
    repeat (2) @(posedge clk);
  endtask

  task automatic send_retain(input logic [16:0] value);
    @(negedge clk);
    in4_data = value;
    in4_valid = 1;
    do @(posedge clk); while (!in4_ready);
    @(negedge clk);
    in4_valid = 0;
    repeat (2) @(posedge clk);
  endtask

  task automatic send_consume(input logic [15:0] value);
    @(negedge clk);
    in5_data = value;
    in5_valid = 1;
    do @(posedge clk); while (!in5_ready);
    @(negedge clk);
    in5_valid = 0;
    repeat (2) @(posedge clk);
  endtask

  task automatic read_payload(input logic [8:0] reference,
                              input logic [7:0] expected);
    out_ready = 0;
    @(negedge clk);
    in2_data = reference;
    in2_valid = 1;
    do @(posedge clk); while (!in2_ready);
    @(negedge clk);
    in2_valid = 0;
    wait (out_valid);
    #1;
    if (out_data !== expected)
      $fatal(1, "versioned read mismatch got=%02x expected=%02x",
             out_data, expected);
    @(negedge clk);
    out_ready = 1;
    @(posedge clk);
    @(negedge clk);
    out_ready = 0;
  endtask

  initial begin
    repeat (3) @(posedge clk);
    @(negedge clk);
    rst = 0;

    send_alloc(entry(0, 0, 4, 0, 8'h11));
    send_alloc(entry(0, 1, 5, 0, 8'h22));
    send_completion(completion(0, 0, 4, 0, 8'haa));
    read_payload(read_ref(0, 0, 1, 5, 0), 8'h22);
    send_completion(completion(0, 1, 5, 0, 8'hbb));
    read_payload(read_ref(0, 0, 1, 5, 0), 8'hbb);

    send_recovery(recovery(6, 1, 0, 0, 1, 5, 0));
    send_alloc(entry(0, 2, 6, 0, 8'hcc));
    send_completion(completion(0, 1, 5, 0, 8'hdd));
    read_payload(read_ref(0, 0, 2, 6, 0), 8'hcc);

    send_retain(entry(1, 0, 6, 0, 8'hee));
    send_retain(entry(1, 1, 6, 0, 8'hff));
    read_payload(read_ref(1, 1, 0, 6, 0), 8'hee);
    send_consume(completion(1, 0, 6, 0, 0));
    send_retain(entry(1, 1, 6, 0, 8'hff));
    read_payload(read_ref(1, 1, 1, 6, 0), 8'hff);

    $display("versioned recovery RTL PASS");
    $finish;
  end
endmodule
