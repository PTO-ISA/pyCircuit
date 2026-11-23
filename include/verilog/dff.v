// SPDX-License-Identifier: BSD-3-Clause
// Packed-payload register. Policy defaults preserve the original contract.
module dff #(
    parameter type T = logic [0:0],
    parameter bit pyc_managed = 0,
    parameter bit pyc_physical = 0,
    parameter T pyc_initial = 'x,
    parameter bit pyc_separate_reset_value = 0,
    parameter bit pyc_negative_edge = 0,
    parameter int unsigned pyc_reset_kind = 1, // 0 none, 1 sync, 2 async
    parameter bit pyc_reset_low = 0,
    parameter bit pyc_reset_if_one = 0,
    parameter bit pyc_has_initial = 1
) (
    // Interface: direction belongs to the wire port, not the payload.
    input  wire clk,
    input  wire rst,
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
    // Internal declarations: only q_current owns register state.
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
    assign q = q_current;

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
    function automatic bit edge_control_error(input logic rising,
                                               input logic reset);
        return (pyc_reset_kind != 0)
            && (rising || (pyc_reset_kind == 2)) && $isunknown(reset);
    endfunction

    // Read-only detail for the generated physical first-failure reporter.
    function automatic bit [1:0] pyc_physical_control_errors();
        return {1'b0, edge_control_error(1'b1, effective_reset)};
    endfunction

    initial begin
        if (PAYLOAD_BITS == 0)
            $fatal(1, "dff: payload must have positive packed width");
        // A two-state engine cannot perform an X-preservation type probe.
`ifndef VERILATOR
        // Check whole-payload X preservation through the declared type.
        payload_x_check = T'({PAYLOAD_BITS{1'bx}});
        packed_x_check = payload_x_check;
        if (packed_x_check !== {PAYLOAD_BITS{1'bx}})
            $fatal(1, "dff: T must be four-state packed bits or a four-state packed struct");
`endif
        if (!pyc_physical) q_current = 'x;
    end
`endif

`ifndef SYNTHESIS
    generate if (pyc_managed) begin : managed
        logic clock_current = 1'b0;
        T q_pending;
        logic clock_pending;
        bit pending_valid = 1'b0;
        bit preparation_error = 1'b0;
        logic clock_sample, reset_sample, rising_sample;
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
                    data_sample = d;
                    reset_value_sample = effective_reset_value;
                    if ($isunknown(clock_sample)) begin
                        preparation_error = 1'b1;
                    end else begin
                        rising_sample = (pyc_negative_edge
                            ? (clock_current && !clock_sample)
                            : (!clock_current && clock_sample));
                        preparation_error = edge_control_error(
                            rising_sample, reset_sample);
                        if (!preparation_error) begin
                            q_pending = next_q(q_current, rising_sample,
                                reset_sample, 1'b1, data_sample, reset_value_sample);
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
        assign pyc_local_error = pyc_physical
            ? edge_control_error(1'b1, effective_reset) : 1'b0;
        task automatic update_q;
            if (!pyc_physical && ($isunknown(clk) ||
                                 edge_control_error(1'b1, effective_reset)))
                $fatal(1, "dff: unknown control at register event");
            if (!$isunknown(clk) && (pyc_root_commit_ok === 1'b1))
                q_current <= next_q(q_current, 1'b1, effective_reset,
                                    1'b1, d, effective_reset_value);
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
            q_current <= next_q(q_current, 1'b1, effective_reset, 1'b1, d,
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
