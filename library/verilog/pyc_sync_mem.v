// Synchronous 1R1W memory with registered read data.
//
// - `DEPTH` is in entries (not bytes).
// - Read is synchronous: when `ren` is asserted, `rdata` updates on the next
//   rising edge of `clk`.
// - Write is synchronous with byte enables `wstrb`.
//
// Note: Read-during-write to the same address returns the pre-write data
// ("old-data") by default.
module pyc_sync_mem #(
  parameter ADDR_WIDTH = 64,
  parameter DATA_WIDTH = 64,
  parameter DEPTH = 1024,
  parameter LIVE_WINDOW = 1
) (
  input                   clk,
  input                   rst,

  input                   ren,
  input  [ADDR_WIDTH-1:0] raddr,
  output reg [DATA_WIDTH-1:0] rdata,

  input                   wvalid,
  input  [ADDR_WIDTH-1:0] waddr,
  input  [DATA_WIDTH-1:0] wdata,
  input  [(DATA_WIDTH+7)/8-1:0] wstrb
);
  `ifndef SYNTHESIS
  initial begin
    if (DEPTH <= 0) begin
      $display("ERROR: pyc_sync_mem DEPTH must be > 0");
      $finish;
    end
    if (LIVE_WINDOW != 1)
      $fatal(1, "pyc_sync_mem aggressive verification requires LIVE_WINDOW=1");
  end
  `endif

  `ifndef SYNTHESIS
  `ifdef PYC_VERIFY_AGGRESSIVE_SRAM
  reg rdata_live;
  initial begin
    rdata = {DATA_WIDTH{1'bx}};
    rdata_live = 1'b0;
  end
  `endif
  `endif

  localparam STRB_WIDTH = (DATA_WIDTH + 7) / 8;
  localparam LAST_LANE_BITS = DATA_WIDTH - 8 * (STRB_WIDTH - 1);
  // Use a compact address for FPGA-friendly inference. For non-power-of-two
  // depths, some addresses in [DEPTH, 2**ADDR_BITS) are unused.
  localparam ADDR_BITS = (DEPTH <= 1) ? 1 : $clog2(DEPTH);

  // Storage.
  `ifdef PYC_TARGET_FPGA
  (* ram_style = "block" *)
  (* ramstyle = "M20K" *)
  reg [DATA_WIDTH-1:0] mem [0:DEPTH-1];
  `else
  reg [DATA_WIDTH-1:0] mem [0:DEPTH-1];
  `endif

  `ifndef SYNTHESIS
  // Deterministic simulation init: keep C++/Verilog equivalence stable.
  integer init_i;
  initial begin
    for (init_i = 0; init_i < DEPTH; init_i = init_i + 1)
      mem[init_i] = {DATA_WIDTH{1'b0}};
  end
  `endif

  integer i;
  reg [DATA_WIDTH-1:0] rd_word;
  wire [ADDR_BITS-1:0] ra = raddr[ADDR_BITS-1:0];
  wire [ADDR_BITS-1:0] wa = waddr[ADDR_BITS-1:0];

  always @(posedge clk) begin
    `ifndef SYNTHESIS
    `ifdef PYC_VERIFY_AGGRESSIVE_SRAM
    if ($isunknown(rst))
      $fatal(1, "pyc_sync_mem reset must be known at posedge");
    if (!rst) begin
      if ($isunknown(ren) || $isunknown(wvalid))
        $fatal(1, "pyc_sync_mem enabled controls must be known at posedge");
      if (ren && $isunknown(raddr))
        $fatal(1, "pyc_sync_mem enabled read address must be known");
      if (wvalid && ($isunknown(waddr) || $isunknown(wdata) ||
                     $isunknown(wstrb)))
        $fatal(1, "pyc_sync_mem enabled write address/data/strobe must be known");
    end
    `endif
    `endif
    if (rst) begin
      `ifndef SYNTHESIS
      `ifdef PYC_VERIFY_AGGRESSIVE_SRAM
      rdata <= {DATA_WIDTH{1'bx}};
      rdata_live <= 1'b0;
      `else
      rdata <= {DATA_WIDTH{1'b0}};
      `endif
      `else
      rdata <= {DATA_WIDTH{1'b0}};
      `endif
    end else begin
      `ifndef SYNTHESIS
      `ifdef PYC_VERIFY_AGGRESSIVE_SRAM
      if (rdata_live && !ren) begin
        rdata <= {DATA_WIDTH{1'bx}};
        rdata_live <= 1'b0;
      end
      `endif
      `endif
      // Write with per-lane strobes; last lane may be narrower than 8 bits.
      if (wvalid) begin
        `ifndef SYNTHESIS
        if (wa < DEPTH) begin
          for (i = 0; i < STRB_WIDTH - 1; i = i + 1) begin
            if (wstrb[i])
              mem[wa][8 * i +: 8] <= wdata[8 * i +: 8];
          end
          if (wstrb[STRB_WIDTH-1])
            mem[wa][8*(STRB_WIDTH-1) +: LAST_LANE_BITS] <= wdata[8*(STRB_WIDTH-1) +: LAST_LANE_BITS];
        end
        `else
        for (i = 0; i < STRB_WIDTH - 1; i = i + 1) begin
          if (wstrb[i])
            mem[wa][8 * i +: 8] <= wdata[8 * i +: 8];
        end
        if (wstrb[STRB_WIDTH-1])
          mem[wa][8*(STRB_WIDTH-1) +: LAST_LANE_BITS] <= wdata[8*(STRB_WIDTH-1) +: LAST_LANE_BITS];
        `endif
      end

      // Registered read.
      if (ren) begin
        `ifndef SYNTHESIS
        `ifdef PYC_VERIFY_AGGRESSIVE_SRAM
        rdata_live <= 1'b1;
        `endif
        `endif
        `ifndef SYNTHESIS
        if (ra < DEPTH) begin
          rd_word = mem[ra];
          rdata <= rd_word;
        end else begin
          rdata <= {DATA_WIDTH{1'b0}};
        end
        `else
        rd_word = mem[ra];
        rdata <= rd_word;
        `endif
      end
    end
  end
endmodule
