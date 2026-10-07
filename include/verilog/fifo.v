// SPDX-License-Identifier: BSD-3-Clause
// Complete-token FIFO; policy, depth and latency are supplied by verified IR.
module fifo #(
    parameter type T = logic [0:0],
    parameter longint signed DEPTH = 0,
    parameter int READY_POLICY = -1,
    parameter [63:0] AVAILABILITY_LATENCY = 64'd0,
    parameter bit pyc_managed = 0
) (
    input wire clk,
    input wire rst,
    input wire in_valid,
    input wire T in_data,
    input wire out_ready,
    output wire in_ready,
    output wire out_valid,
    output wire T out_data,
    input wire [2:0] pyc_phase,
    input tri1 pyc_root_commit_ok,
    output wire pyc_local_error
);
    localparam int unsigned PAYLOAD_BITS = $bits(T);
    localparam int unsigned PTR_BITS = DEPTH <= 1 ? 1 : $clog2(DEPTH);
    localparam int unsigned COUNT_BITS = DEPTH <= 1 ? 1 :
        $clog2(DEPTH) + (((DEPTH & (DEPTH - 64'sd1)) == 0) ? 1 : 0);

    T storage [0:DEPTH-1];
    logic [PTR_BITS-1:0] rd, wr;
    logic [COUNT_BITS-1:0] count;
    logic initialized;
    T data_read;
    wire do_pop = out_valid & out_ready;
    wire do_push = in_valid & in_ready;

    assign in_ready = initialized ?
        ((READY_POLICY == 0) ? (count < DEPTH) :
         (READY_POLICY == 1) ? ((count < DEPTH) | do_pop) : 1'bx) : 1'bx;
    assign out_data = data_read;

    // The transaction contains one token write, never a second storage array.
    typedef struct packed {
        logic initialized;
        logic [PTR_BITS-1:0] rd, wr;
        logic [COUNT_BITS-1:0] count;
        logic write_token;
        logic [PTR_BITS-1:0] write_index;
        T token;
    } base_candidate_t;
    base_candidate_t candidate;

    // Sole pointer/count/token calculation for both latency modes and adapters.
    function automatic base_candidate_t next_base(
        input logic old_initialized,
        input logic [PTR_BITS-1:0] old_rd, old_wr,
        input logic [COUNT_BITS-1:0] old_count,
        input logic rising, reset, push, pop,
        input T token);
        base_candidate_t result;
        result.initialized = old_initialized;
        result.rd = old_rd;
        result.wr = old_wr;
        result.count = old_count;
        result.write_token = 1'b0;
        result.write_index = old_wr;
        result.token = token;
        if (rising) begin
            if (reset) begin
                result.initialized = 1'b1;
                result.rd = '0;
                result.wr = '0;
                result.count = '0;
            end else if (old_initialized) begin
                if (push) begin
                    result.write_token = 1'b1;
                    result.wr = (old_wr == DEPTH - 64'sd1) ?
                        '0 : old_wr + 1'b1;
                end
                if (pop)
                    result.rd = (old_rd == DEPTH - 64'sd1) ?
                        '0 : old_rd + 1'b1;
                if (push ^ pop)
                    result.count = push ? old_count + 1'b1 : old_count - 1'b1;
            end
        end
        return result;
    endfunction

    // All targets transfer exactly the fields prepared by the shared kernel.
    task automatic commit_base();
        initialized <= candidate.initialized;
        rd <= candidate.rd;
        wr <= candidate.wr;
        count <= candidate.count;
        if (candidate.write_token)
            storage[candidate.write_index] <= candidate.token;
    endtask

`ifndef SYNTHESIS
    function automatic bit [1:0] edge_control_error(
        input logic rising, reset, push, pop);
        edge_control_error = 2'b00;
        if (rising) begin
            if ($isunknown(reset))
                edge_control_error[0] = 1'b1;
            else if (!reset && ($isunknown(push) || $isunknown(pop)))
                edge_control_error[1] = 1'b1;
        end
    endfunction

    T payload_x_check;
    logic [PAYLOAD_BITS-1:0] packed_x_check;
    initial begin
        if (DEPTH <= 0)
            $fatal(1, "fifo: depth must be positive");
        if (AVAILABILITY_LATENCY == 64'd0)
            $fatal(1, "fifo: latency must be positive");
        if (READY_POLICY != 0 && READY_POLICY != 1)
            $fatal(1, "fifo: unsupported ready policy");
        if (PAYLOAD_BITS == 0)
            $fatal(1, "fifo: payload must have positive packed width");
`ifndef VERILATOR
        payload_x_check = T'({PAYLOAD_BITS{1'bx}});
        packed_x_check = payload_x_check;
        if (packed_x_check !== {PAYLOAD_BITS{1'bx}})
            $fatal(1, "fifo: T must preserve four-state packed payloads");
`endif
        initialized = 1'b0;
    end
`endif

    generate
        if (AVAILABILITY_LATENCY == 64'd1) begin : latency_one
            assign out_valid = initialized ? (count != '0) : 1'bx;
            always_comb begin
                data_read = 'x;
                if (initialized) begin
                    data_read = '0;
                    if (count != '0)
                        data_read = storage[rd];
                end
            end

            task automatic prepare_transition(
                input logic rising, reset, push, pop, input T token);
                candidate = next_base(initialized, rd, wr, count,
                                      rising, reset, push, pop, token);
            endtask

            task automatic commit_transition();
                commit_base();
            endtask

`ifndef SYNTHESIS
            if (pyc_managed) begin : managed
                logic clock_current = 1'b0;
                logic clock_pending;
                bit pending_valid = 1'b0;
                bit preparation_error = 1'b0;
                logic clock_sample, reset_sample, push_sample, pop_sample;
                logic rising_sample;
                T token_sample;

                assign pyc_local_error = preparation_error | !pending_valid;

                always @(pyc_phase) begin
                    case (pyc_phase)
                        3'd1: begin // PREPARE freezes the whole transaction.
                            pending_valid = 1'b0;
                            preparation_error = 1'b0;
                            clock_pending = clock_current;
                            clock_sample = clk;
                            reset_sample = rst;
                            push_sample = do_push;
                            pop_sample = do_pop;
                            token_sample = in_data;
                            prepare_transition(1'b0, 1'b0, 1'b0, 1'b0, token_sample);
                            if ($isunknown(clock_sample)) begin
                                preparation_error = 1'b1;
                            end else begin
                                rising_sample = !clock_current && clock_sample;
                                preparation_error = |edge_control_error(
                                    rising_sample, reset_sample, push_sample, pop_sample);
                                if (!preparation_error) begin
                                    prepare_transition(rising_sample, reset_sample,
                                        push_sample, pop_sample, token_sample);
                                    clock_pending = clock_sample;
                                    pending_valid = 1'b1;
                                end
                            end
                        end
                        3'd2: begin // COMMIT transfers only frozen candidates.
                            if (pending_valid && pyc_root_commit_ok === 1'b1) begin
                                commit_transition();
                                clock_current <= clock_pending;
                            end
                            pending_valid <= 1'b0;
                        end
                        3'd3: begin // DISCARD leaves every owner unchanged.
                            pending_valid = 1'b0;
                            preparation_error = 1'b0;
                        end
                        3'd4: begin // Host Reset retains token/deadline storage.
                            prepare_transition(1'b1, 1'b1, 1'b0, 1'b0, T'('x));
                            clock_pending = 1'b0;
                            preparation_error = 1'b0;
                            pending_valid = 1'b1;
                        end
                        default: begin end
                    endcase
                end
            end else begin : autonomous
                bit [1:0] control_error;
                assign pyc_local_error = 1'b0;
                always @(posedge clk) begin
                    control_error = edge_control_error(1'b1, rst, do_push, do_pop);
                    if (control_error[0])
                        $fatal(1, "fifo: reset must be known at the rising edge");
                    if (control_error[1])
                        $fatal(1, "fifo: effective transfers must be known");
                    prepare_transition(1'b1, rst, do_push, do_pop, in_data);
                    if (pyc_root_commit_ok === 1'b1)
                        commit_transition();
                end
            end
`else
            // All physical-edge state, including reset and age, shares permit.
            assign pyc_local_error = 1'b0;
            always @(posedge clk) begin
                prepare_transition(1'b1, rst, do_push, do_pop, in_data);
                if (pyc_root_commit_ok === 1'b1)
                    commit_transition();
            end
`endif
        end else if (AVAILABILITY_LATENCY > 64'd1) begin : latency_delayed
            // Fixed-width arithmetic preserves the full-u64 K=64 projection.
            localparam int unsigned TIMESTAMP_BITS = $clog2(AVAILABILITY_LATENCY);
            localparam [TIMESTAMP_BITS-1:0] DELAY = AVAILABILITY_LATENCY - 64'd1;
            logic [TIMESTAMP_BITS-1:0] deadline [0:DEPTH-1];
            logic [TIMESTAMP_BITS-1:0] tick;
            logic [PTR_BITS-1:0] mature_ptr;
            logic [COUNT_BITS-1:0] eligible_count;
            typedef struct packed {
                logic [TIMESTAMP_BITS-1:0] tick;
                logic [PTR_BITS-1:0] mature_ptr;
                logic [COUNT_BITS-1:0] eligible_count;
                logic [TIMESTAMP_BITS-1:0] write_deadline;
            } timing_candidate_t;
            timing_candidate_t timing_candidate;

            assign out_valid = initialized ? (eligible_count != '0) : 1'bx;
            always_comb begin
                data_read = 'x;
                if (initialized) begin
                    data_read = '0;
                    if (eligible_count != '0)
                        data_read = storage[rd];
                end
            end

            // Only an occupied delayed suffix may consult a deadline.
            function automatic timing_candidate_t next_timing(
                input logic old_initialized,
                input logic [TIMESTAMP_BITS-1:0] old_tick, head_deadline,
                input logic [PTR_BITS-1:0] old_mature_ptr,
                input logic [COUNT_BITS-1:0] old_count, old_eligible_count,
                input logic rising, reset, pop);
                timing_candidate_t result;
                logic mature;
                result.tick = old_tick;
                result.mature_ptr = old_mature_ptr;
                result.eligible_count = old_eligible_count;
                result.write_deadline = old_tick + DELAY;
                mature = 1'b0;
                if (rising) begin
                    if (reset) begin
                        result.tick = '0;
                        result.mature_ptr = '0;
                        result.eligible_count = '0;
                    end else if (old_initialized) begin
                        result.tick = old_tick + 1'b1;
                        if (old_count > old_eligible_count)
                            mature = head_deadline == result.tick;
                        if (mature)
                            result.mature_ptr =
                                (old_mature_ptr == DEPTH - 64'sd1) ?
                                '0 : old_mature_ptr + 1'b1;
                        // Pop uses old eligibility; same-edge maturity cannot
                        // create a pop, and opposite count changes cancel.
                        if (mature ^ pop)
                            result.eligible_count = mature ?
                                old_eligible_count + 1'b1 :
                                old_eligible_count - 1'b1;
                        result.write_deadline = result.tick + DELAY;
                    end
                end
                return result;
            endfunction

            task automatic prepare_transition(
                input logic rising, reset, push, pop, input T token);
                logic [TIMESTAMP_BITS-1:0] head_deadline;
                candidate = next_base(initialized, rd, wr, count,
                                      rising, reset, push, pop, token);
                head_deadline = 'x;
                if (rising && !reset && initialized && count > eligible_count)
                    head_deadline = deadline[mature_ptr];
                timing_candidate = next_timing(initialized, tick, head_deadline,
                    mature_ptr, count, eligible_count, rising, reset, pop);
            endtask

            task automatic commit_transition();
                commit_base();
                tick <= timing_candidate.tick;
                mature_ptr <= timing_candidate.mature_ptr;
                eligible_count <= timing_candidate.eligible_count;
                if (candidate.write_token)
                    deadline[candidate.write_index] <= timing_candidate.write_deadline;
            endtask

`ifndef SYNTHESIS
            if (pyc_managed) begin : managed
                logic clock_current = 1'b0;
                logic clock_pending;
                bit pending_valid = 1'b0;
                bit preparation_error = 1'b0;
                logic clock_sample, reset_sample, push_sample, pop_sample;
                logic rising_sample;
                T token_sample;

                assign pyc_local_error = preparation_error | !pending_valid;

                always @(pyc_phase) begin
                    case (pyc_phase)
                        3'd1: begin // PREPARE freezes the whole transaction.
                            pending_valid = 1'b0;
                            preparation_error = 1'b0;
                            clock_pending = clock_current;
                            clock_sample = clk;
                            reset_sample = rst;
                            push_sample = do_push;
                            pop_sample = do_pop;
                            token_sample = in_data;
                            prepare_transition(1'b0, 1'b0, 1'b0, 1'b0, token_sample);
                            if ($isunknown(clock_sample)) begin
                                preparation_error = 1'b1;
                            end else begin
                                rising_sample = !clock_current && clock_sample;
                                preparation_error = |edge_control_error(
                                    rising_sample, reset_sample, push_sample, pop_sample);
                                if (!preparation_error) begin
                                    prepare_transition(rising_sample, reset_sample,
                                        push_sample, pop_sample, token_sample);
                                    clock_pending = clock_sample;
                                    pending_valid = 1'b1;
                                end
                            end
                        end
                        3'd2: begin // COMMIT transfers only frozen candidates.
                            if (pending_valid && pyc_root_commit_ok === 1'b1) begin
                                commit_transition();
                                clock_current <= clock_pending;
                            end
                            pending_valid <= 1'b0;
                        end
                        3'd3: begin // DISCARD leaves every owner unchanged.
                            pending_valid = 1'b0;
                            preparation_error = 1'b0;
                        end
                        3'd4: begin // Host Reset retains token/deadline storage.
                            prepare_transition(1'b1, 1'b1, 1'b0, 1'b0, T'('x));
                            clock_pending = 1'b0;
                            preparation_error = 1'b0;
                            pending_valid = 1'b1;
                        end
                        default: begin end
                    endcase
                end
            end else begin : autonomous
                bit [1:0] control_error;
                assign pyc_local_error = 1'b0;
                always @(posedge clk) begin
                    control_error = edge_control_error(1'b1, rst, do_push, do_pop);
                    if (control_error[0])
                        $fatal(1, "fifo: reset must be known at the rising edge");
                    if (control_error[1])
                        $fatal(1, "fifo: effective transfers must be known");
                    prepare_transition(1'b1, rst, do_push, do_pop, in_data);
                    if (pyc_root_commit_ok === 1'b1)
                        commit_transition();
                end
            end
`else
            // All physical-edge state, including reset and age, shares permit.
            assign pyc_local_error = 1'b0;
            always @(posedge clk) begin
                prepare_transition(1'b1, rst, do_push, do_pop, in_data);
                if (pyc_root_commit_ok === 1'b1)
                    commit_transition();
            end
`endif
        end else begin : invalid_latency
            assign out_valid = 1'bx;
            assign pyc_local_error = 1'b1;
            always_comb data_read = 'x;
        end
    endgenerate
endmodule
