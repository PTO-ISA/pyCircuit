// SPDX-License-Identifier: BSD-3-Clause
// Entry-addressed 2R1W memory with packed-payload ports.
// Reads sample pre-write data; simulation Q expires on its second idle edge.
module sync_mem_dp #(
    parameter type T = logic [63:0],
    parameter int unsigned ADDR_WIDTH = 64,
    parameter int unsigned DEPTH = 1024,
    parameter bit pyc_managed = 0
) (
    // DEPTH counts T entries; strobes select low packed-byte lanes first.
    input wire clk,
    input wire rst,
    input wire ren0,
    input wire [ADDR_WIDTH-1:0] raddr0,
    output wire T rdata0,
    input wire ren1,
    input wire [ADDR_WIDTH-1:0] raddr1,
    output wire T rdata1,
    input wire wvalid,
    input wire [ADDR_WIDTH-1:0] waddr,
    input wire T wdata,
    input wire [($bits(T)/8 + (($bits(T)%8) != 0))-1:0] wstrb,
    input wire [2:0] pyc_phase,
    input tri1 pyc_root_commit_ok,
    output wire pyc_local_error
);
    localparam int unsigned PAYLOAD_BITS = $bits(T);
    localparam int unsigned LIVE_WINDOW = 2;
    localparam int unsigned STRB_WIDTH = PAYLOAD_BITS / 8 + ((PAYLOAD_BITS % 8) != 0);
    localparam int unsigned ADDR_BITS = (DEPTH <= 1) ? 1 : $clog2(DEPTH);
    localparam int unsigned COMPARE_WIDTH = (ADDR_WIDTH > 32) ? ADDR_WIDTH : 32;

`ifdef TARGET_FPGA
    (* ram_style = "block" *)
    (* ramstyle = "M20K" *)
`endif
    // Zero remains invalid; avoid unsigned bound wrap before its diagnostic.
    logic [PAYLOAD_BITS-1:0] mem [0:((DEPTH == 0) ? 1 : DEPTH)-1];
    logic [PAYLOAD_BITS-1:0] write_candidate_bits;
    logic [PAYLOAD_BITS-1:0] write_candidate_enable;
    logic [ADDR_BITS-1:0] write_candidate_index;
    logic write_candidate_valid;

    typedef struct packed {
        T q;
`ifndef SYNTHESIS
        int unsigned idle;
        logic live;
`endif
    } read_candidate_t;
    T rdata0_current;
    read_candidate_t rdata0_candidate;
`ifndef SYNTHESIS
    int unsigned idle0_current;
    logic live0_current;
`endif
    assign rdata0 = rdata0_current;
    T rdata1_current;
    read_candidate_t rdata1_candidate;
`ifndef SYNTHESIS
    int unsigned idle1_current;
    logic live1_current;
`endif
    assign rdata1 = rdata1_current;

    // Read complete unsigned addresses before narrowing to a RAM index.
    function automatic T read_entry(input logic [ADDR_WIDTH-1:0] address);
        logic [COMPARE_WIDTH-1:0] full_address;
        logic [ADDR_BITS-1:0] index;
        full_address = address;
        index = address;
        read_entry = '0;
        if (full_address < DEPTH)
            read_entry = T'(mem[index]);
    endfunction

    // Sole read/lifetime transition, shared by every read port and adapter.
    // The synthesis projection contains no live/idle state or expiry action.
    function automatic read_candidate_t next_read(
        input read_candidate_t current,
        input logic rising, reset, enabled,
        input T old_entry);
        read_candidate_t result;
        result = current;
        if (rising) begin
            if (reset) begin
`ifndef SYNTHESIS
                result.q = 'x;
                result.idle = 0;
                result.live = 1'b0;
`else
                result.q = '0;
`endif
            end else if (enabled) begin
                result.q = old_entry;
`ifndef SYNTHESIS
                result.idle = 0;
                result.live = 1'b1;
            end else if (current.live) begin
                result.idle = current.idle + 1;
                if (result.idle >= LIVE_WINDOW) begin
                    result.q = 'x;
                    result.live = 1'b0;
                end
`endif
            end
        end
        return result;
    endfunction

`ifndef SYNTHESIS
    int unsigned init_index;
    T payload_x_check;
    logic [PAYLOAD_BITS-1:0] packed_x_check;
    function automatic bit [3:0] edge_control_error(
        input logic rising, reset,
        input logic read_enabled0,
        input logic [ADDR_WIDTH-1:0] read_address0,
        input logic read_enabled1,
        input logic [ADDR_WIDTH-1:0] read_address1,
        input logic write_enabled,
        input logic [ADDR_WIDTH-1:0] write_address,
        input T write_data,
        input logic [STRB_WIDTH-1:0] strobes);
        edge_control_error = 4'b0000;
        if (rising) begin
            if ($isunknown(reset))
                edge_control_error[0] = 1'b1;
            else if (!reset) begin
                if ($isunknown(read_enabled0) || $isunknown(read_enabled1) ||
                    $isunknown(write_enabled))
                    edge_control_error[1] = 1'b1;
                if (read_enabled0 && $isunknown(read_address0))
                    edge_control_error[2] = 1'b1;
                if (read_enabled1 && $isunknown(read_address1))
                    edge_control_error[2] = 1'b1;
                if (write_enabled && ($isunknown(write_address) ||
                                      $isunknown(write_data) || $isunknown(strobes)))
                    edge_control_error[3] = 1'b1;
            end
        end
    endfunction

    initial begin
        if (ADDR_WIDTH == 0 || PAYLOAD_BITS == 0 || DEPTH == 0)
            $fatal(1, "sync_mem_dp: widths and depth must be positive");
`ifndef VERILATOR
        payload_x_check = T'({PAYLOAD_BITS{1'bx}});
        packed_x_check = payload_x_check;
        if (packed_x_check !== {PAYLOAD_BITS{1'bx}})
            $fatal(1, "sync_mem_dp: T must be four-state packed bits or a four-state packed struct");
`endif
        for (init_index = 0; init_index < DEPTH; init_index = init_index + 1)
            mem[init_index] = '0;
        rdata0_current = 'x;
        idle0_current = 0;
        live0_current = 1'b0;
        rdata1_current = 'x;
        idle1_current = 0;
        live1_current = 1'b0;
    end
`endif

    // This preparation freezes old-data reads and one bounded masked write.
    task automatic prepare_transition(input logic rising, reset,
        input logic read_enabled0,
        input logic [ADDR_WIDTH-1:0] read_address0,
        input logic read_enabled1,
        input logic [ADDR_WIDTH-1:0] read_address1,
        input logic write_enabled,
        input logic [ADDR_WIDTH-1:0] write_address,
        input T write_data,
        input logic [STRB_WIDTH-1:0] strobes);
        logic [COMPARE_WIDTH-1:0] full_write_address;
        int unsigned bit_index;
        read_candidate_t current_read;
        T old_entry;
        full_write_address = write_address;
        write_candidate_bits = write_data;
        write_candidate_enable = '0;
        write_candidate_index = '0;
        write_candidate_valid = 1'b0;
        if (rising && !reset && write_enabled && full_write_address < DEPTH) begin
            write_candidate_index = write_address;
            write_candidate_valid = 1'b1;
            // Freeze the partial final lane's mask before COMMIT too.
            for (bit_index = 0; bit_index < PAYLOAD_BITS; bit_index = bit_index + 1)
                write_candidate_enable[bit_index] = strobes[bit_index / 8];
        end
        current_read.q = rdata0_current;
`ifndef SYNTHESIS
        current_read.idle = idle0_current;
        current_read.live = live0_current;
`endif
        old_entry = '0;
        if (rising) begin
            if (reset) begin end
            else if (read_enabled0)
                old_entry = read_entry(read_address0);
        end
        rdata0_candidate = next_read(current_read, rising, reset,
                                     read_enabled0, old_entry);
        current_read.q = rdata1_current;
`ifndef SYNTHESIS
        current_read.idle = idle1_current;
        current_read.live = live1_current;
`endif
        old_entry = '0;
        if (rising) begin
            if (reset) begin end
            else if (read_enabled1)
                old_entry = read_entry(read_address1);
        end
        rdata1_candidate = next_read(current_read, rising, reset,
                                     read_enabled1, old_entry);
    endtask

    // Every transfer uses frozen data/masks/indices; no RAM feedback copy.
    task automatic commit_transition();
        int unsigned bit_index;
        if (write_candidate_valid) begin
            for (bit_index = 0; bit_index < PAYLOAD_BITS; bit_index = bit_index + 1)
                if (write_candidate_enable[bit_index])
                    mem[write_candidate_index][bit_index] <= write_candidate_bits[bit_index];
        end
        rdata0_current <= rdata0_candidate.q;
`ifndef SYNTHESIS
        idle0_current <= rdata0_candidate.idle;
        live0_current <= rdata0_candidate.live;
`endif
        rdata1_current <= rdata1_candidate.q;
`ifndef SYNTHESIS
        idle1_current <= rdata1_candidate.idle;
        live1_current <= rdata1_candidate.live;
`endif
    endtask

`ifndef SYNTHESIS
    generate if (pyc_managed) begin : managed
        logic clock_current = 1'b0;
        logic clock_pending;
        bit pending_valid = 1'b0;
        bit preparation_error = 1'b0;
        logic clock_sample, reset_sample, enabled_sample, rising_sample;
        logic [ADDR_WIDTH-1:0] write_address_sample;
        T data_sample;
        logic [STRB_WIDTH-1:0] strobes_sample;
        logic read_enabled0_sample;
        logic [ADDR_WIDTH-1:0] read_address0_sample;
        logic read_enabled1_sample;
        logic [ADDR_WIDTH-1:0] read_address1_sample;

        assign pyc_local_error = preparation_error | !pending_valid;

        always @(pyc_phase) begin
            case (pyc_phase)
                3'd1: begin
                    pending_valid = 1'b0;
                    preparation_error = 1'b0;
                    clock_pending = clock_current;
                    clock_sample = clk;
                    reset_sample = rst;
                    enabled_sample = wvalid;
                    write_address_sample = waddr;
                    data_sample = wdata;
                    strobes_sample = wstrb;
                    read_enabled0_sample = ren0;
                    read_address0_sample = raddr0;
                    read_enabled1_sample = ren1;
                    read_address1_sample = raddr1;
                    prepare_transition(1'b0, 1'b0, 1'b0, '0, 1'b0, '0,
                                       1'b0, '0, T'('0), '0);
                    if ($isunknown(clock_sample)) begin
                        preparation_error = 1'b1;
                    end else begin
                        rising_sample = !clock_current && clock_sample;
                        preparation_error = |edge_control_error(rising_sample, reset_sample,
                            read_enabled0_sample, read_address0_sample,
                            read_enabled1_sample, read_address1_sample,
                            enabled_sample, write_address_sample, data_sample, strobes_sample);
                        if (!preparation_error) begin
                            prepare_transition(rising_sample, reset_sample,
                                read_enabled0_sample, read_address0_sample,
                                read_enabled1_sample, read_address1_sample,
                                enabled_sample, write_address_sample, data_sample, strobes_sample);
                            clock_pending = clock_sample;
                            pending_valid = 1'b1;
                        end
                    end
                end
                3'd2: begin
                    if (pending_valid && pyc_root_commit_ok === 1'b1) begin
                        commit_transition();
                        clock_current <= clock_pending;
                    end
                    pending_valid <= 1'b0;
                end
                3'd3: begin
                    write_candidate_valid = 1'b0;
                    write_candidate_enable = '0;
                    pending_valid = 1'b0;
                    preparation_error = 1'b0;
                end
                3'd4: begin // Host Reset invalidates Q and retains RAM.
                    prepare_transition(1'b1, 1'b1, 1'b0, '0, 1'b0, '0,
                                       1'b0, '0, T'('0), '0);
                    clock_pending = 1'b0;
                    preparation_error = 1'b0;
                    pending_valid = 1'b1;
                end
                default: begin end
            endcase
        end
    end else begin : autonomous
        bit [3:0] control_error;
        assign pyc_local_error = 1'b0;
        always @(posedge clk) begin
            control_error = edge_control_error(1'b1, rst, ren0, raddr0, ren1, raddr1,
                                                wvalid, waddr, wdata, wstrb);
            if (control_error[0])
                $fatal(1, "sync_mem_dp: reset must be known at the rising edge");
            if (control_error[1])
                $fatal(1, "sync_mem_dp: enabled controls must be known");
            if (control_error[2])
                $fatal(1, "sync_mem_dp: enabled read address must be known");
            if (control_error[3])
                $fatal(1, "sync_mem_dp: enabled write address, data and strobes must be known");
            prepare_transition(1'b1, rst, ren0, raddr0, ren1, raddr1, wvalid, waddr, wdata, wstrb);
            if (pyc_root_commit_ok === 1'b1)
                commit_transition();
        end
    end endgenerate
`else
    assign pyc_local_error = 1'b0;
    always @(posedge clk) begin
        prepare_transition(1'b1, rst, ren0, raddr0, ren1, raddr1, wvalid, waddr, wdata, wstrb);
        if (pyc_root_commit_ok === 1'b1)
            commit_transition();
    end
`endif
endmodule
