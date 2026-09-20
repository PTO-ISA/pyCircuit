// Synchronous 2R1W memory with registered read data.
//
// - `DEPTH` is in entries (not bytes).
// - Both reads are synchronous (registered outputs).
// - One write port with byte enables `wstrb`.
// - Read-during-write to the same address returns pre-write data ("old-data") by default.
module pyc_sync_mem_dp #(
  parameter ADDR_WIDTH = 64,
  parameter DATA_WIDTH = 64,
  parameter DEPTH = 1024,
  parameter LIVE_WINDOW = 1
) (
  input                   clk,
  input                   rst,

  input                   ren0,
  input  [ADDR_WIDTH-1:0] raddr0,
  output reg [DATA_WIDTH-1:0] rdata0,

  input                   ren1,
  input  [ADDR_WIDTH-1:0] raddr1,
  output reg [DATA_WIDTH-1:0] rdata1,

  input                   wvalid,
  input  [ADDR_WIDTH-1:0] waddr,
  input  [DATA_WIDTH-1:0] wdata,
  input  [(DATA_WIDTH+7)/8-1:0] wstrb
);
  `ifndef SYNTHESIS
  initial begin
    if (DEPTH <= 0) begin
      $display("ERROR: pyc_sync_mem_dp DEPTH must be > 0");
      $finish;
    end
    if (LIVE_WINDOW != 1)
      $fatal(1, "pyc_sync_mem_dp aggressive verification requires LIVE_WINDOW=1");
  end
  `endif

  `ifndef SYNTHESIS
  `ifdef PYC_VERIFY_AGGRESSIVE_SRAM
  reg rdata0_live;
  reg rdata1_live;
  initial begin
    rdata0 = {DATA_WIDTH{1'bx}};
    rdata1 = {DATA_WIDTH{1'bx}};
    rdata0_live = 1'b0;
    rdata1_live = 1'b0;
  end
  `endif
  `endif

  localparam STRB_WIDTH = (DATA_WIDTH + 7) / 8;
  localparam LAST_LANE_BITS = DATA_WIDTH - 8 * (STRB_WIDTH - 1);
  localparam ADDR_BITS = (DEPTH <= 1) ? 1 : $clog2(DEPTH);

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
  reg [DATA_WIDTH-1:0] rd0;
  reg [DATA_WIDTH-1:0] rd1;
  wire [ADDR_BITS-1:0] ra0 = raddr0[ADDR_BITS-1:0];
  wire [ADDR_BITS-1:0] ra1 = raddr1[ADDR_BITS-1:0];
  wire [ADDR_BITS-1:0] wa = waddr[ADDR_BITS-1:0];

  always @(posedge clk) begin
    `ifndef SYNTHESIS
    `ifdef PYC_VERIFY_AGGRESSIVE_SRAM
    if ($isunknown(rst))
      $fatal(1, "pyc_sync_mem_dp reset must be known at posedge");
    if (!rst) begin
      if ($isunknown(ren0) || $isunknown(ren1) || $isunknown(wvalid))
        $fatal(1, "pyc_sync_mem_dp enabled controls must be known at posedge");
      if (ren0 && $isunknown(raddr0))
        $fatal(1, "pyc_sync_mem_dp enabled read0 address must be known");
      if (ren1 && $isunknown(raddr1))
        $fatal(1, "pyc_sync_mem_dp enabled read1 address must be known");
      if (wvalid && ($isunknown(waddr) || $isunknown(wdata) ||
                     $isunknown(wstrb)))
        $fatal(1, "pyc_sync_mem_dp enabled write address/data/strobe must be known");
    end
    `endif
    `endif
    if (rst) begin
      `ifndef SYNTHESIS
      `ifdef PYC_VERIFY_AGGRESSIVE_SRAM
      rdata0 <= {DATA_WIDTH{1'bx}};
      rdata1 <= {DATA_WIDTH{1'bx}};
      rdata0_live <= 1'b0;
      rdata1_live <= 1'b0;
      `else
      rdata0 <= {DATA_WIDTH{1'b0}};
      rdata1 <= {DATA_WIDTH{1'b0}};
      `endif
      `else
      rdata0 <= {DATA_WIDTH{1'b0}};
      rdata1 <= {DATA_WIDTH{1'b0}};
      `endif
    end else begin
      `ifndef SYNTHESIS
      `ifdef PYC_VERIFY_AGGRESSIVE_SRAM
      if (rdata0_live && !ren0) begin
        rdata0 <= {DATA_WIDTH{1'bx}};
        rdata0_live <= 1'b0;
      end
      if (rdata1_live && !ren1) begin
        rdata1 <= {DATA_WIDTH{1'bx}};
        rdata1_live <= 1'b0;
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

      // Registered read port 0.
      if (ren0) begin
        `ifndef SYNTHESIS
        `ifdef PYC_VERIFY_AGGRESSIVE_SRAM
        rdata0_live <= 1'b1;
        `endif
        `endif
        `ifndef SYNTHESIS
        if (ra0 < DEPTH) begin
          rd0 = mem[ra0];
          rdata0 <= rd0;
        end else begin
          rdata0 <= {DATA_WIDTH{1'b0}};
        end
        `else
        rd0 = mem[ra0];
        rdata0 <= rd0;
        `endif
      end

      // Registered read port 1.
      if (ren1) begin
        `ifndef SYNTHESIS
        `ifdef PYC_VERIFY_AGGRESSIVE_SRAM
        rdata1_live <= 1'b1;
        `endif
        `endif
        `ifndef SYNTHESIS
        if (ra1 < DEPTH) begin
          rd1 = mem[ra1];
          rdata1 <= rd1;
        end else begin
          rdata1 <= {DATA_WIDTH{1'b0}};
        end
        `else
        rd1 = mem[ra1];
        rdata1 <= rd1;
        `endif
      end
    end
  end
endmodule
