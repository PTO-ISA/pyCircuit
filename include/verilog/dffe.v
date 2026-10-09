// SPDX-License-Identifier: BSD-3-Clause
// Packed-payload positive-edge register with synchronous active-high reset.
module dffe #(
    parameter type T = logic [0:0],
    parameter bit pyc_managed = 0
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
    input  tri1 pyc_root_commit_ok,
    output wire pyc_local_error
);
    // Internal declarations: only q_current owns register state.
    localparam int unsigned PAYLOAD_BITS = $bits(T);
    T q_current;
`ifndef SYNTHESIS
    T payload_x_check;
    logic [PAYLOAD_BITS-1:0] packed_x_check;
`endif
    assign q = q_current;

    // Both adapters use this sole state transition calculation.
    function automatic T next_q(input T current_q, input logic rising,
                                input logic reset, input logic enabled,
                                input T data, input T initial_data);
        next_q = current_q;
        if (rising) begin
            if (reset)
                next_q = initial_data;
            else if (enabled)
                next_q = data;
        end
    endfunction

`ifndef SYNTHESIS
    // Bit zero diagnoses reset; bit one diagnoses enabled-edge control.
    function automatic bit [1:0] edge_control_error(input logic rising,
                                                   input logic reset,
                                                   input logic enabled);
        edge_control_error = 2'b00;
        if (rising) begin
            if ($isunknown(reset))
                edge_control_error[0] = 1'b1;
            else if (!reset && $isunknown(enabled))
                edge_control_error[1] = 1'b1;
        end
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
        q_current = 'x;
    end
`endif

`ifndef SYNTHESIS
    generate if (pyc_managed) begin : managed
        logic clock_current = 1'b0;
        T q_pending;
        logic clock_pending;
        bit pending_valid = 1'b0;
        bit preparation_error = 1'b0;
        logic clock_sample, reset_sample, enable_sample, rising_sample;
        T data_sample, init_sample;

        assign pyc_local_error = preparation_error | !pending_valid;

        always @(pyc_phase) begin
            case (pyc_phase)
                3'd1: begin // PREPARE: freeze pins and stage old-Q work.
                    pending_valid = 1'b0;
                    preparation_error = 1'b0;
                    q_pending = q_current;
                    clock_pending = clock_current;
                    clock_sample = clk;
                    reset_sample = rst;
                    enable_sample = en;
                    data_sample = d;
                    init_sample = init;
                    if ($isunknown(clock_sample)) begin
                        preparation_error = 1'b1;
                    end else begin
                        rising_sample = !clock_current && clock_sample;
                        preparation_error = |edge_control_error(
                            rising_sample, reset_sample, enable_sample);
                        if (!preparation_error) begin
                            q_pending = next_q(q_current, rising_sample,
                                reset_sample, enable_sample, data_sample,
                                init_sample);
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
                    q_pending = init;
                    clock_pending = 1'b0;
                    preparation_error = 1'b0;
                    pending_valid = 1'b1;
                end
                default: begin end // IDLE has no lifecycle effects.
            endcase
        end
    end else begin : autonomous
        bit [1:0] control_error;
        assign pyc_local_error = 1'b0;
        always @(posedge clk) begin
            control_error = edge_control_error(1'b1, rst, en);
            if (control_error[0])
                $fatal(1, "dffe: reset must be known at the rising edge");
            if (control_error[1])
                $fatal(1, "dffe: enable must be known at the rising edge");
            if (pyc_root_commit_ok === 1'b1)
                q_current <= next_q(q_current, 1'b1, rst, en, d, init);
        end
    end endgenerate
`else
    // Hardware retains physical-edge operation and complete-update permission.
    assign pyc_local_error = 1'b0;
    always @(posedge clk) begin
        if (pyc_root_commit_ok === 1'b1)
            q_current <= next_q(q_current, 1'b1, rst, en, d, init);
    end
`endif
endmodule
