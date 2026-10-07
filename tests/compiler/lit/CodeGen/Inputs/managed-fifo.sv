`timescale 1ps/1ps

// Stimulus and golden values come from an architectural ordered-token oracle.
// This bench instantiates the owning FIFO file directly; no substitute leaf.
module tb_fifo;
    parameter integer WIDTH = 13, DEPTH = 3, POLICY = 1, LATENCY = 3;
    parameter integer MANAGED = 1, OMIT_PRIVATE = 0;
`ifdef TWO_STATE_PAYLOAD
    typedef bit [WIDTH-1:0] payload_t;
`elsif STRUCT_PAYLOAD
    typedef struct packed { logic [WIDTH-2:0] body; logic flag; } payload_t;
`else
    typedef logic [WIDTH-1:0] payload_t;
`endif
    logic clk = 0, rst = 0, in_valid = 0, out_ready = 0;
    payload_t in_data = '0;
    wire payload_t out_data;
    wire in_ready, out_valid, local_error;
    logic [2:0] phase = 0;
    logic permit = 1;
    generate if (OMIT_PRIVATE) begin : old_named_pins
        fifo #(.T(payload_t), .DEPTH(DEPTH), .READY_POLICY(POLICY),
               .AVAILABILITY_LATENCY(LATENCY)) dut (
            .clk(clk), .rst(rst), .in_valid(in_valid), .in_data(in_data), .out_ready(out_ready),
            .in_ready(in_ready), .out_valid(out_valid), .out_data(out_data));
        assign local_error = 0;
    end else begin : private_bindings
        fifo #(.T(payload_t), .DEPTH(DEPTH), .READY_POLICY(POLICY),
               .AVAILABILITY_LATENCY(LATENCY), .pyc_managed(MANAGED)) dut (
            .clk(clk), .rst(rst), .in_valid(in_valid), .in_data(in_data),
            .out_ready(out_ready), .in_ready(in_ready), .out_valid(out_valid), .out_data(out_data),
            .pyc_phase(phase), .pyc_root_commit_ok(permit), .pyc_local_error(local_error));
    end endgenerate

    integer input_file, scanned, index = 0, command, initialized;
    logic next_clk, next_rst, next_valid, next_take, next_permit;
    payload_t next_data, gold_data;
    logic gold_ready, gold_valid, gold_error;
    string vectors;
    initial begin
        #1;
`ifndef VERILATOR
        if ($test$plusargs("BAD_AUTO_RESET")) begin
            rst = 1'bx; #1; clk = 1; #1;
            $fatal(1, "autonomous invalid reset was admitted");
        end
        if ($test$plusargs("BAD_AUTO_TRANSFERS")) begin
            rst = 1; #1; clk = 1; #1;
            clk = 0; rst = 0; in_valid = 1'bx; #1; clk = 1; #1;
            $fatal(1, "autonomous invalid effective transfer was admitted");
        end
`endif
        if (!$value$plusargs("VECTORS=%s", vectors)) $fatal(1, "missing FIFO vectors");
        input_file = $fopen(vectors, "r");
        if (!input_file) $fatal(1, "cannot open FIFO vectors");
        while (!$feof(input_file)) begin
            scanned = $fscanf(input_file, "%d %b %b %b %b %b %b %d %b %b %b %b\n",
                command, next_clk, next_rst, next_valid, next_take, next_permit,
                next_data, initialized, gold_ready, gold_valid, gold_data, gold_error);
            if (scanned == 12) begin
                rst = next_rst; in_valid = next_valid; out_ready = next_take;
                permit = next_permit; in_data = next_data;
                #1; clk = next_clk; #1;
                if (command != 5) phase = command;
                #1;
                if (initialized) begin
                    if (in_ready !== gold_ready || out_valid !== gold_valid || out_data !== gold_data)
                        $fatal(1, "FIFO row %0d cmd=%0d ready=%b/%b valid=%b/%b head=%b/%b",
                            index, command, in_ready, gold_ready, out_valid, gold_valid, out_data, gold_data);
                    $display("ROW %0d %b %b %b %b", index, in_ready, out_valid, out_data, local_error);
                end else begin
`ifndef VERILATOR
`ifndef SYNTHESIS
                    if (in_ready !== 1'bx || out_valid !== 1'bx || out_data !== payload_t'('x))
                        $fatal(1, "cold FIFO outputs must be unknown");
`endif
`endif
                    $display("ROW %0d cold %b", index, local_error);
                end
                if (local_error !== gold_error)
                    $fatal(1, "FIFO row %0d completion/error=%b expected=%b", index, local_error, gold_error);
                index = index + 1;
            end else if (!$feof(input_file)) $fatal(1, "malformed FIFO vectors");
        end
        $fclose(input_file);
        if (index < 32) $fatal(1, "empty or incomplete FIFO selector");
        $display("PASS FIFO width=%0d depth=%0d policy=%0d latency=%0d managed=%0d rows=%0d",
                 WIDTH, DEPTH, POLICY, LATENCY, MANAGED, index);
        $finish;
    end
    initial begin #1000000; $fatal(1, "FIFO test timeout"); end
endmodule
