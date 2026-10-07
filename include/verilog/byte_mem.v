// SPDX-License-Identifier: BSD-3-Clause
// Byte-addressed memory: continuous little-endian read and synchronous write.
module byte_mem #(
    parameter type T = logic [63:0],
    parameter int unsigned ADDR_WIDTH = 64,
    parameter int unsigned DEPTH = 1024,
    parameter bit pyc_managed = 0
) (
    // Interface. DEPTH counts bytes; the packed payload must contain whole bytes.
    input  wire clk,
    input  wire rst,
    input  wire [ADDR_WIDTH-1:0] raddr,
    output wire T rdata,
    input  wire wvalid,
    input  wire [ADDR_WIDTH-1:0] waddr,
    input  wire T wdata,
    input  wire [$bits(T)/8-1:0] wstrb,
    input  wire [2:0] pyc_phase,
    input  tri1 pyc_root_commit_ok,
    output wire pyc_local_error
);
    // Internal declarations. Each lane has one candidate, not a memory copy.
    localparam int unsigned PAYLOAD_BITS = $bits(T);
    localparam int unsigned STRB_WIDTH = PAYLOAD_BITS / 8;
    localparam int unsigned COMPARE_WIDTH = (ADDR_WIDTH > 32) ? ADDR_WIDTH : 32;

    // Keep the invalid zero-depth declaration bounded until its diagnostic;
    // every admitted depth retains its exact storage geometry.
    logic [7:0] mem [0:((DEPTH == 0) ? 1 : DEPTH)-1];
    wire [31:0] read_index;
    wire [COMPARE_WIDTH-1:0] read_address;
    wire [COMPARE_WIDTH-1:0] depth_limit;
    wire read_in_range;
    logic [PAYLOAD_BITS-1:0] read_candidate_bits;
    T rdata_current;
    logic [7:0] write_candidate [0:STRB_WIDTH-1];
    logic [STRB_WIDTH-1:0] write_candidate_valid;
    int unsigned write_candidate_index [0:STRB_WIDTH-1];
    int unsigned read_lane;
`ifndef SYNTHESIS
    int unsigned init_index;
    T payload_x_check;
    logic [PAYLOAD_BITS-1:0] packed_x_check;
`endif

    // Compare the complete unsigned address before using its bounded low bits.
    assign read_index = raddr;
    assign read_address = raddr;
    assign depth_limit = DEPTH;
    assign read_in_range = read_address < depth_limit;
    assign rdata = rdata_current;

`ifndef SYNTHESIS
    function automatic bit read_address_error(
        input logic [ADDR_WIDTH-1:0] address);
        return $isunknown(address);
    endfunction

    function automatic bit [2:0] edge_control_error(
        input logic rising, reset, enabled,
        input logic [ADDR_WIDTH-1:0] address,
        input T data,
        input logic [STRB_WIDTH-1:0] strobes);
        edge_control_error = 3'b000;
        if (rising) begin
            if ($isunknown(reset))
                edge_control_error[0] = 1'b1;
            else if (!reset) begin
                if ($isunknown(enabled))
                    edge_control_error[1] = 1'b1;
                else if (enabled && ($isunknown(address) ||
                                     $isunknown(data) || $isunknown(strobes)))
                    edge_control_error[2] = 1'b1;
            end
        end
    endfunction

    initial begin
        if (ADDR_WIDTH == 0 || PAYLOAD_BITS == 0 || DEPTH == 0 || PAYLOAD_BITS % 8 != 0)
            $fatal(1, "byte_mem: positive widths/depth and whole-byte data required");
        // A two-state engine cannot perform an X-preservation type probe.
`ifndef VERILATOR
        // Check whole-payload X preservation through the declared type.
        payload_x_check = T'({PAYLOAD_BITS{1'bx}});
        packed_x_check = payload_x_check;
        if (packed_x_check !== {PAYLOAD_BITS{1'bx}})
            $fatal(1, "byte_mem: T must be four-state packed bits or a four-state packed struct");
`endif
        for (init_index = 0; init_index < DEPTH; init_index = init_index + 1)
            mem[init_index] = 8'h00;
    end
`endif

    // Work read rule: continuously enabled, with its original address diagnostic.
    // Read mem directly so updates to any selected byte retrigger this rule.
    always_comb begin
        read_candidate_bits = '0;
`ifndef SYNTHESIS
        if (read_address_error(raddr)) begin
            read_candidate_bits = 'x;
            if (!pyc_managed)
                $fatal(1, "byte_mem: read address must be known");
        end else begin
`endif
        for (read_lane = 0; read_lane < STRB_WIDTH; read_lane = read_lane + 1) begin
            // Subtract before adding: an out-of-range lane never wraps its index.
            if (read_in_range && read_lane < depth_limit - read_address)
                read_candidate_bits[8 * read_lane +: 8] = mem[read_index + read_lane];
        end
`ifndef SYNTHESIS
        end
`endif
        rdata_current = T'(read_candidate_bits);
    end

    // One write preparation calculation serves every adapter. Complete address
    // comparison precedes narrowing; candidates contain only actual byte lanes.
    task automatic prepare_writes(
        input logic rising, reset, enabled,
        input logic [ADDR_WIDTH-1:0] address,
        input T data,
        input logic [STRB_WIDTH-1:0] strobes);
        logic [COMPARE_WIDTH-1:0] full_address;
        logic [PAYLOAD_BITS-1:0] data_bits;
        int unsigned base_index;
        int unsigned lane;
        full_address = address;
        data_bits = data;
        base_index = address;
        write_candidate_valid = '0;
        for (lane = 0; lane < STRB_WIDTH; lane = lane + 1) begin
            write_candidate[lane] = 8'h00;
            write_candidate_index[lane] = 0;
            if (rising && !reset && enabled && full_address < depth_limit &&
                lane < depth_limit - full_address) begin
                write_candidate_index[lane] = base_index + lane;
                if (strobes[lane]) begin
                    write_candidate[lane] = data_bits[8 * lane +: 8];
                    write_candidate_valid[lane] = 1'b1;
                end
            end
        end
    endtask

    task automatic commit_writes();
        int unsigned lane;
        for (lane = 0; lane < STRB_WIDTH; lane = lane + 1)
            if (write_candidate_valid[lane])
                mem[write_candidate_index[lane]] <= write_candidate[lane];
    endtask

`ifndef SYNTHESIS
    generate if (pyc_managed) begin : managed
        logic clock_current = 1'b0;
        logic clock_pending;
        bit pending_valid = 1'b0;
        bit preparation_error = 1'b0;
        logic clock_sample, reset_sample, enabled_sample, rising_sample;
        logic [ADDR_WIDTH-1:0] read_address_sample, write_address_sample;
        T data_sample;
        logic [STRB_WIDTH-1:0] strobes_sample;

        assign pyc_local_error = preparation_error | !pending_valid;

        always @(pyc_phase) begin
            case (pyc_phase)
                3'd1: begin // PREPARE validates reads on every Work sample.
                    pending_valid = 1'b0;
                    preparation_error = 1'b0;
                    clock_pending = clock_current;
                    clock_sample = clk;
                    reset_sample = rst;
                    enabled_sample = wvalid;
                    read_address_sample = raddr;
                    write_address_sample = waddr;
                    data_sample = wdata;
                    strobes_sample = wstrb;
                    prepare_writes(1'b0, 1'b0, 1'b0, write_address_sample,
                                   data_sample, strobes_sample);
                    if (read_address_error(read_address_sample) ||
                        $isunknown(clock_sample)) begin
                        preparation_error = 1'b1;
                    end else begin
                        rising_sample = !clock_current && clock_sample;
                        preparation_error = |edge_control_error(rising_sample,
                            reset_sample, enabled_sample, write_address_sample,
                            data_sample, strobes_sample);
                        if (!preparation_error) begin
                            prepare_writes(rising_sample, reset_sample,
                                enabled_sample, write_address_sample,
                                data_sample, strobes_sample);
                            clock_pending = clock_sample;
                            pending_valid = 1'b1;
                        end
                    end
                end
                3'd2: begin // Only frozen writes and clock transfer at COMMIT.
                    if (pending_valid && pyc_root_commit_ok === 1'b1) begin
                        commit_writes();
                        clock_current <= clock_pending;
                    end
                    pending_valid <= 1'b0;
                end
                3'd3: begin
                    write_candidate_valid = '0;
                    pending_valid = 1'b0;
                    preparation_error = 1'b0;
                end
                3'd4: begin // Host Reset retains contents and bypasses checks.
                    prepare_writes(1'b0, 1'b1, 1'b0, '0, T'('0), '0);
                    clock_pending = 1'b0;
                    preparation_error = 1'b0;
                    pending_valid = 1'b1;
                end
                default: begin end
            endcase
        end
    end else begin : autonomous
        bit [2:0] control_error;
        assign pyc_local_error = 1'b0;
        always @(posedge clk) begin
            control_error = edge_control_error(1'b1, rst, wvalid, waddr, wdata, wstrb);
            if (control_error[0])
                $fatal(1, "byte_mem: reset must be known at the rising edge");
            if (control_error[1])
                $fatal(1, "byte_mem: write control must be known");
            if (control_error[2])
                $fatal(1, "byte_mem: enabled write address, data and strobes must be known");
            prepare_writes(1'b1, rst, wvalid, waddr, wdata, wstrb);
            if (pyc_root_commit_ok === 1'b1)
                commit_writes();
        end
    end endgenerate
`else
    assign pyc_local_error = 1'b0;
    always @(posedge clk) begin
        prepare_writes(1'b1, rst, wvalid, waddr, wdata, wstrb);
        if (pyc_root_commit_ok === 1'b1)
            commit_writes();
    end
`endif
endmodule
