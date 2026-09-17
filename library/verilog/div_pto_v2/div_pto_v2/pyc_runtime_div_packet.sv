// Ready/valid packet adapter for the shared PTO iterative divider.
//
// PYC queue graphs expose semantic values as pure SSA expressions, while the
// qualified divider is a variable-latency ready/valid unit.  This adapter
// bridges those contracts by retaining the complete input packet until the
// quotient/remainder response is accepted.  The packet itself is deliberately
// opaque here; the generated PYC wiring supplies the operand and mode slices.
module pyc_runtime_div_packet #(
  parameter integer WIDTH = 64,
  parameter integer WORD_WIDTH = 32,
  parameter integer PACKET_WIDTH = 1,
  parameter integer BITS_PER_ITER = 1
) (
  input  wire                     clk,
  input  wire                     reset,

  input  wire                     request_valid,
  output wire                     request_ready,
  input  wire [PACKET_WIDTH-1:0]  packet_in,
  input  wire [WIDTH-1:0]         dividend,
  input  wire [WIDTH-1:0]         divisor,
  input  wire                     is_signed,
  input  wire                     word_mode,

  output wire                     response_valid,
  input  wire                     response_ready,
  output wire [PACKET_WIDTH-1:0]  packet_out,
  output wire [WIDTH-1:0]         quotient,
  output wire [WIDTH-1:0]         remainder
);

  wire divider_request_ready;
  wire divider_response_valid;
  wire divider_response_ready;
  wire divide_by_zero;
  wire request_fire;

  reg packet_valid_r;
  reg [PACKET_WIDTH-1:0] packet_r;

  // Only one packet may be outstanding.  The input FIFO therefore holds its
  // output stable until this adapter accepts the request.
  assign request_ready = divider_request_ready && !packet_valid_r;
  assign request_fire = request_valid && request_ready;

  assign response_valid = packet_valid_r && divider_response_valid;
  assign divider_response_ready = response_ready && packet_valid_r;
  assign packet_out = packet_valid_r ? packet_r : packet_in;

  always @(posedge clk) begin
    if (reset) begin
      packet_valid_r <= 1'b0;
      packet_r <= {PACKET_WIDTH{1'b0}};
    end else begin
      if (response_valid && response_ready)
        packet_valid_r <= 1'b0;
      if (request_fire) begin
        packet_valid_r <= 1'b1;
        packet_r <= packet_in;
      end
    end
  end

  pyc_runtime_div #(
    .WIDTH(WIDTH),
    .WORD_WIDTH(WORD_WIDTH),
    .BITS_PER_ITER(BITS_PER_ITER)
  ) divider (
    .clk(clk),
    .reset(reset),
    .request_valid(request_fire),
    .request_ready(divider_request_ready),
    .dividend(dividend),
    .divisor(divisor),
    .is_signed(is_signed),
    .word_mode(word_mode),
    .response_valid(divider_response_valid),
    .response_ready(divider_response_ready),
    .quotient(quotient),
    .remainder(remainder),
    .divide_by_zero(divide_by_zero)
  );

endmodule
