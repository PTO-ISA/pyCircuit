`timescale 1ps/1ps

module tb_memory;
    parameter integer KIND = 0, WIDTH = 16, ADDR_WIDTH = 65, DEPTH = 7;
    parameter integer MANAGED = 1;
    localparam integer STRB_WIDTH = KIND == 0 ? WIDTH / 8 : (WIDTH + 7) / 8;
`ifdef TWO_STATE_PAYLOAD
    typedef bit [WIDTH-1:0] payload_t;
`elsif STRUCT_PAYLOAD
    typedef struct packed { logic [WIDTH-2:0] body; logic flag; } payload_t;
`else
    typedef logic [WIDTH-1:0] payload_t;
`endif
    logic clk = 0, rst = 0, wvalid = 0, ren0 = 0, ren1 = 0;
    logic [ADDR_WIDTH-1:0] raddr0 = '0, raddr1 = '0, waddr = '0;
    payload_t wdata = '0;
    logic [STRB_WIDTH-1:0] wstrb = '0;
    wire payload_t q0, q1;
    logic [2:0] phase = 0;
    logic permit = 1;
    wire local_error;
    generate if (KIND == 0) begin : byte_leaf
        byte_mem #(.T(payload_t), .ADDR_WIDTH(ADDR_WIDTH), .DEPTH(DEPTH), .pyc_managed(MANAGED)) dut (
            .clk(clk), .rst(rst), .raddr(raddr0), .rdata(q0), .wvalid(wvalid),
            .waddr(waddr), .wdata(wdata), .wstrb(wstrb), .pyc_phase(phase),
            .pyc_root_commit_ok(permit), .pyc_local_error(local_error));
        assign q1 = '0;
    end else if (KIND == 1) begin : single_leaf
        sync_mem #(.T(payload_t), .ADDR_WIDTH(ADDR_WIDTH), .DEPTH(DEPTH), .pyc_managed(MANAGED)) dut (
            .clk(clk), .rst(rst), .ren(ren0), .raddr(raddr0), .rdata(q0), .wvalid(wvalid),
            .waddr(waddr), .wdata(wdata), .wstrb(wstrb), .pyc_phase(phase),
            .pyc_root_commit_ok(permit), .pyc_local_error(local_error));
        assign q1 = '0;
    end else begin : dual_leaf
        sync_mem_dp #(.T(payload_t), .ADDR_WIDTH(ADDR_WIDTH), .DEPTH(DEPTH), .pyc_managed(MANAGED)) dut (
            .clk(clk), .rst(rst), .ren0(ren0), .raddr0(raddr0), .rdata0(q0),
            .ren1(ren1), .raddr1(raddr1), .rdata1(q1), .wvalid(wvalid), .waddr(waddr),
            .wdata(wdata), .wstrb(wstrb), .pyc_phase(phase),
            .pyc_root_commit_ok(permit), .pyc_local_error(local_error));
    end endgenerate

    integer fd, scanned, command, index = 0, known0, known1;
    logic next_clk, next_rst, next_write, next_ren0, next_ren1, next_permit, gold_error;
    logic [ADDR_WIDTH-1:0] next_read0, next_read1, next_address;
    payload_t next_data, gold_q0, gold_q1;
    logic [STRB_WIDTH-1:0] next_strobes;
    string vectors, text0, text1;
    initial begin
        #1;
`ifndef VERILATOR
        if ($test$plusargs("BAD_AUTO_READ")) begin
            raddr0 = 'x; #1;
            $fatal(1, "invalid continuous read address was admitted");
        end
        if ($test$plusargs("BAD_AUTO_WRITE")) begin
            rst = 0; wvalid = 1; wdata = 'x; wstrb = '0; waddr = '1;
            #1; clk = 1; #1;
            $fatal(1, "invalid enabled write was admitted");
        end
        if ($test$plusargs("BAD_AUTO_PORT1")) begin
            ren0 = 1; ren1 = 1; raddr0 = '0; raddr1 = 'x; wvalid = 1;
            wdata = '1; wstrb = '1; #1; clk = 1; #1;
            $fatal(1, "invalid enabled second read was admitted");
        end
`endif
        if (!$value$plusargs("VECTORS=%s", vectors)) $fatal(1, "missing memory vectors");
        fd = $fopen(vectors, "r");
        if (!fd) $fatal(1, "cannot open memory vectors");
        while (!$feof(fd)) begin
            scanned = $fscanf(fd, "%d %b %b %b %b %b %b %b %b %b %b %b %b %b %d %d %b\n",
                command, next_clk, next_rst, next_write, next_ren0, next_ren1, next_read0, next_read1,
                next_address, next_data, next_strobes, next_permit, gold_q0, gold_q1, known0, known1, gold_error);
            if (scanned == 17) begin
                rst = next_rst; wvalid = next_write; ren0 = next_ren0; ren1 = next_ren1;
                raddr0 = next_read0; raddr1 = next_read1; waddr = next_address;
                wdata = next_data; wstrb = next_strobes; permit = next_permit;
                #1; clk = next_clk; #1;
                if (command != 5) phase = command;
                #1;
`ifdef VERILATOR
                if (known0 && q0 !== gold_q0) $fatal(1, "memory row %0d q0 mismatch", index);
                if (known1 && q1 !== gold_q1) $fatal(1, "memory row %0d q1 mismatch", index);
`else
                if (q0 !== gold_q0 || q1 !== gold_q1)
                    $fatal(1, "memory row %0d cmd=%0d q0=%b/%b q1=%b/%b", index, command, q0, gold_q0, q1, gold_q1);
`endif
                if (local_error !== gold_error) $fatal(1, "memory row %0d completion=%b expected=%b", index, local_error, gold_error);
                // Expected unknown output is a four-state witness only in
                // Icarus. Normalize this trace label for the known engine.
                if (known0) text0 = $sformatf("%b", q0);
                else text0 = "unknown";
                if (known1) text1 = $sformatf("%b", q1);
                else text1 = "unknown";
                $display("ROW %0d %s %s %b", index, text0, text1, local_error);
                index = index + 1;
            end else if (!$feof(fd)) $fatal(1, "malformed memory vectors");
        end
        $fclose(fd);
        if (index < 32) $fatal(1, "incomplete memory selector");
        $display("PASS memory kind=%0d width=%0d address=%0d managed=%0d rows=%0d", KIND, WIDTH, ADDR_WIDTH, MANAGED, index);
        $finish;
    end
    initial begin #1000000; $fatal(1, "memory test timeout"); end
endmodule
