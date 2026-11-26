// SPDX-License-Identifier: BSD-3-Clause
// Packed-payload register. Policy defaults preserve the original contract.
module dffe #(
    parameter type T = logic [0:0],
    parameter bit pyc_managed = 0,
    parameter bit pyc_physical = 0,
    parameter T pyc_initial = 'x,
    parameter bit pyc_separate_reset_value = 0,
    parameter bit pyc_negative_edge = 0,
    parameter int unsigned pyc_reset_kind = 1, // 0 none, 1 sync, 2 async
    parameter bit pyc_reset_low = 0,
    parameter bit pyc_reset_if_one = 0,
    parameter bit pyc_has_initial = 1,
    // Versioned causal-wave protocol; legacy adapters leave this disabled.
    parameter bit pyc_causal = 0,
    // Compiler-authenticated initial level of this register's clock source.
    parameter logic pyc_initial_clock = 1'b0
) (
    // Interface: direction belongs to the wire port, not the payload.
    input  wire clk,
    input  wire rst,
    input  wire en,
    input  wire T d,
    input  wire T init,
    output wire T q,
    // Private generated lifecycle controls; not source hardware ports.
    input  wire [2:0] pyc_phase,
`ifdef SYNTHESIS
    // Generated physical instances bind this control explicitly. A pull net
    // is a simulation default, not a synthesizable standard-cell input type.
    input  wire pyc_root_commit_ok,
`else
    input  tri1 pyc_root_commit_ok,
`endif
    output wire pyc_local_error,
    input wire T reset_value
);
    // q_current is committed state; causal simulation has a private overlay.
    localparam int unsigned PAYLOAD_BITS = $bits(T);
    T q_current;
    // Physical export proves this typed elaboration value from the declared
    // initializer DAG. Default adapters retain their existing startup policy.
    generate if (pyc_physical) begin : physical_initialization
        initial q_current = pyc_has_initial ? pyc_initial : 'x;
    end endgenerate
`ifndef SYNTHESIS
    T payload_x_check;
    logic [PAYLOAD_BITS-1:0] packed_x_check;
`endif
    wire T effective_reset_value;
    assign effective_reset_value = pyc_separate_reset_value ? reset_value : init;
    wire normalized_reset = pyc_reset_low ? ~rst : rst;
    wire effective_reset = (pyc_reset_kind == 0) ? 1'b0 :
        pyc_reset_if_one ? (normalized_reset === 1'b1) : normalized_reset;
`ifdef SYNTHESIS
    assign q = q_current;
`endif

    // Both adapters use this sole state transition calculation.
    function automatic T next_q(input T current_q, input logic rising,
                                input logic reset, input logic enabled,
                                input T data, input T reset_data);
        next_q = current_q;
        if (reset && ((pyc_reset_kind == 2) || rising))
            next_q = reset_data;
        else if (rising && enabled)
            next_q = data;
    endfunction

`ifndef SYNTHESIS
    // Bit zero diagnoses reset; bit one diagnoses enabled-edge control.
    function automatic bit [1:0] edge_control_error(input logic rising,
                                                   input logic reset,
                                                   input logic enabled);
        edge_control_error = 2'b00;
        if ((pyc_reset_kind != 0) && (rising || (pyc_reset_kind == 2))
            && $isunknown(reset))
            edge_control_error[0] = 1'b1;
        if (rising && !reset && $isunknown(enabled))
            edge_control_error[1] = 1'b1;
    endfunction

    // Read-only detail for the generated physical first-failure reporter.
    function automatic bit [1:0] pyc_physical_control_errors();
        return edge_control_error(1'b1, effective_reset, en);
    endfunction

    initial begin
        if (PAYLOAD_BITS == 0)
            $fatal(1, "dffe: payload must have positive packed width");
        // A two-state engine cannot perform an X-preservation type probe.
`ifndef VERILATOR
        // Check whole-payload X preservation through the declared type.
        payload_x_check = T'({PAYLOAD_BITS{1'bx}});
        packed_x_check = payload_x_check;
        if (packed_x_check !== {PAYLOAD_BITS{1'bx}})
            $fatal(1, "dffe: T must be four-state packed bits or a four-state packed struct");
`endif
        if (!pyc_physical) q_current = 'x;
    end
`endif

`ifndef SYNTHESIS
    generate if (pyc_causal) begin : causal
        logic clock_current = pyc_initial_clock;
        T q_wave, q_pending;
        logic clock_wave, clock_pending;
        bit macro_active = 1'b0;
        bit pending_valid = 1'b0;
        bit wave_complete = 1'b0;
        bit preparation_error = 1'b0;
        logic clock_sample, reset_sample, rising_sample;
        logic enable_sample;
        T data_sample, reset_value_sample;

        // Root outputs need separate committed snapshots. This q is the
        // internal combinational plane used to discover descendant edges.
        assign q = macro_active ? q_wave : q_current;
        // Ready after PREPARE for the root check and after ADVANCE for its
        // final publication preflight. BEGIN alone is never complete.
        assign pyc_local_error = preparation_error | !macro_active |
            !(pending_valid || wave_complete);

        always @(pyc_phase) begin
            case (pyc_phase)
                3'd5: begin // BEGIN: clone committed Q and sampling history.
                    q_wave <= q_current;
                    clock_wave <= clock_current;
                    macro_active <= 1'b1;
                    pending_valid = 1'b0;
                    wave_complete = 1'b0;
                    preparation_error = 1'b0;
                end
                3'd1: begin // PREPARE: one frozen proposal from private Q.
                    wave_complete = 1'b0;
                    if (!macro_active || pending_valid)
                        preparation_error = 1'b1;
                    pending_valid = 1'b0;
                    q_pending = q_wave;
                    clock_pending = clock_wave;
                    clock_sample = clk;
                    reset_sample = effective_reset;
                    data_sample = d;
                    reset_value_sample = effective_reset_value;
                    enable_sample = en;
                    if ($isunknown(clock_sample) || $isunknown(clock_wave))
                        preparation_error = 1'b1;
                    if (!preparation_error) begin
                        rising_sample = (pyc_negative_edge
                            ? (clock_wave && !clock_sample)
                            : (!clock_wave && clock_sample));
                        preparation_error = |edge_control_error(
                            rising_sample, reset_sample, enable_sample);
                        if (!preparation_error) begin
                            q_pending = next_q(q_wave, rising_sample,
                                reset_sample, enable_sample, data_sample,
                                reset_value_sample);
                            clock_pending = clock_sample;
                            pending_valid = 1'b1;
                        end
                    end
                end
                3'd6: begin // ADVANCE: root checked every owner this wave.
                    if (macro_active && pending_valid && !preparation_error &&
                        pyc_root_commit_ok === 1'b1) begin
                        q_wave <= q_pending;
                        clock_wave <= clock_pending;
                        wave_complete = 1'b1;
                    end else begin
                        preparation_error <= 1'b1;
                        wave_complete = 1'b0;
                    end
                    pending_valid = 1'b0;
                end
                3'd7: begin // PUBLISH: exactly once, after the last ADVANCE.
                    if (macro_active && wave_complete && !pending_valid &&
                        !preparation_error && pyc_root_commit_ok === 1'b1) begin
                        q_current <= q_wave;
                        clock_current <= clock_wave;
                    end
                    // Denial consumes the whole macrostep without publishing.
                    macro_active <= 1'b0;
                    pending_valid = 1'b0;
                    wave_complete = 1'b0;
                end
                3'd3: begin // DISCARD: cancel every private wave and history.
                    macro_active <= 1'b0;
                    pending_valid = 1'b0;
                    wave_complete = 1'b0;
                    preparation_error = 1'b0;
                end
                3'd4: begin // RESET_PREPARE: existing host reset values.
                    q_wave <= q_current;
                    clock_wave <= clock_current;
                    q_pending = pyc_has_initial ? init : 'x;
                    clock_pending = pyc_initial_clock;
                    macro_active <= 1'b1;
                    preparation_error = 1'b0;
                    pending_valid = 1'b1;
                    wave_complete = 1'b0;
                end
                3'd2: preparation_error = 1'b1; // Legacy COMMIT is invalid.
                default: begin end // IDLE is only a settling barrier.
            endcase
        end
    end else if (pyc_managed) begin : managed
        assign q = q_current;
        logic clock_current = 1'b0;
        T q_pending;
        logic clock_pending;
        bit pending_valid = 1'b0;
        bit preparation_error = 1'b0;
        logic clock_sample, reset_sample, enable_sample, rising_sample;
        T data_sample, reset_value_sample;

        assign pyc_local_error = preparation_error | !pending_valid;

        always @(pyc_phase) begin
            case (pyc_phase)
                3'd1: begin // PREPARE: freeze pins and stage old-Q work.
                    pending_valid = 1'b0;
                    preparation_error = 1'b0;
                    q_pending = q_current;
                    clock_pending = clock_current;
                    clock_sample = clk;
                    reset_sample = effective_reset;
                    enable_sample = en;
                    data_sample = d;
                    reset_value_sample = effective_reset_value;
                    if ($isunknown(clock_sample)) begin
                        preparation_error = 1'b1;
                    end else begin
                        rising_sample = (pyc_negative_edge
                            ? (clock_current && !clock_sample)
                            : (!clock_current && clock_sample));
                        preparation_error = |edge_control_error(
                            rising_sample, reset_sample, enable_sample);
                        if (!preparation_error) begin
                            q_pending = next_q(q_current, rising_sample,
                                reset_sample, enable_sample, data_sample,
                                reset_value_sample);
                            clock_pending = clock_sample;
                            pending_valid = 1'b1;
                        end
                    end
                end
                3'd2: begin // COMMIT consumes frozen work, including denial.
                    if (pending_valid && pyc_root_commit_ok === 1'b1) begin
                        q_current <= q_pending;
                        clock_current <= clock_pending;
                    end
                    pending_valid <= 1'b0;
                end
                3'd3: begin // DISCARD preserves committed Q and clock.
                    pending_valid = 1'b0;
                    preparation_error = 1'b0;
                end
                3'd4: begin // RESET_PREPARE bypasses ordinary controls.
                    q_pending = pyc_has_initial ? init : 'x;
                    clock_pending = 1'b0;
                    preparation_error = 1'b0;
                    pending_valid = 1'b1;
                end
                default: begin end // IDLE has no lifecycle effects.
            endcase
        end
    end else begin : autonomous
        assign q = q_current;
        bit [1:0] control_error;
        assign pyc_local_error = pyc_physical
            ? |edge_control_error(1'b1, effective_reset, en) : 1'b0;
        task automatic update_q;
            control_error = edge_control_error(1'b1, effective_reset, en);
            if (!pyc_physical && ($isunknown(clk) || |control_error))
                $fatal(1, "dffe: unknown control at register event");
            if (!$isunknown(clk) && (pyc_root_commit_ok === 1'b1))
                q_current <= next_q(q_current, 1'b1, effective_reset,
                                    en, d, effective_reset_value);
        endtask
        if (pyc_reset_kind == 2) begin : asynchronous_reset
            if (pyc_negative_edge) begin
                always @(negedge clk or posedge effective_reset) update_q();
            end else begin
                always @(posedge clk or posedge effective_reset) update_q();
            end
        end else if (pyc_negative_edge) begin : negative_edge
            always @(negedge clk) update_q();
        end else begin : positive_edge
            always @(posedge clk) update_q();
        end
    end endgenerate
`else
    // Hardware retains physical-edge operation and complete-update permission.
    assign pyc_local_error = 1'b0;
    task automatic update_q;
        if (pyc_root_commit_ok === 1'b1)
            q_current <= next_q(q_current, 1'b1, effective_reset, en, d,
                                effective_reset_value);
    endtask
    generate if (pyc_reset_kind == 2) begin : asynchronous_reset
        if (pyc_negative_edge) begin
            always @(negedge clk or posedge effective_reset) update_q();
        end else begin
            always @(posedge clk or posedge effective_reset) update_q();
        end
    end else if (pyc_negative_edge) begin : negative_edge
        always @(negedge clk) update_q();
    end else begin : positive_edge
        always @(posedge clk) update_q();
    end endgenerate
`endif
endmodule
