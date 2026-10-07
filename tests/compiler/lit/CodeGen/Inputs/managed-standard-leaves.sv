`timescale 1ps/1ps

module tb;
    parameter integer WIDTH = 13;
`ifdef TWO_STATE_PAYLOAD
    typedef bit [WIDTH-1:0] payload_t;
`elsif STRUCT_PAYLOAD
    typedef struct packed { logic [WIDTH-2:0] body; logic flag; } payload_t;
`else
    typedef logic [WIDTH-1:0] payload_t;
`endif
    logic clk = 0, rst = 0, en = 0;
    payload_t dff_data, dffe_data, initial_data;
    wire payload_t dff_q, dffe_q;
    logic [2:0] phase = 0;
    logic permit = 0;
    wire dff_error, dffe_error;
    logic auto_clk = 0, auto_rst = 0, auto_en = 0;
    payload_t auto_data, auto_init;
    wire payload_t auto_ff_q, auto_fe_q;
    integer comparisons = 0, four_state_cases = 0;

    dff #(.T(payload_t), .pyc_managed(1)) ff (
        .clk(clk), .rst(rst), .d(dff_data), .init(initial_data), .q(dff_q),
        .pyc_phase(phase), .pyc_root_commit_ok(permit), .pyc_local_error(dff_error));
    dffe #(.T(payload_t), .pyc_managed(1)) fe (
        .clk(clk), .rst(rst), .en(en), .d(dffe_data), .init(initial_data), .q(dffe_q),
        .pyc_phase(phase), .pyc_root_commit_ok(permit), .pyc_local_error(dffe_error));
    // Real old named-pin autonomous users omit every new private binding.
    dff #(.T(payload_t)) auto_ff (
        .clk(auto_clk), .rst(auto_rst), .d(auto_data), .init(auto_init), .q(auto_ff_q));
    dffe #(.T(payload_t)) auto_fe (
        .clk(auto_clk), .rst(auto_rst), .en(auto_en), .d(auto_data),
        .init(auto_init), .q(auto_fe_q));

    function automatic payload_t pattern(input integer seed);
        logic [WIDTH-1:0] value;
        for (integer i = 0; i < WIDTH; i = i + 1)
            value[i] = ((i * 3 + seed) % 7) < 3;
        return payload_t'(value);
    endfunction
    function automatic payload_t unknown_data();
        logic [WIDTH-1:0] value;
        for (integer i = 0; i < WIDTH; i = i + 1)
            value[i] = i % 2 ? 1'bx : 1'bz;
        return payload_t'(value);
    endfunction
    task automatic q_is(input payload_t ff_gold, input payload_t fe_gold,
                        input string label);
        comparisons = comparisons + 1;
        if (dff_q !== ff_gold || dffe_q !== fe_gold)
            $fatal(1, "%s: dff=%b expected=%b dffe=%b expected=%b",
                   label, dff_q, ff_gold, dffe_q, fe_gold);
    endtask
    task automatic prepare(input logic ff_gold, input logic fe_gold);
        #1; phase = 1; #1;
        if (dff_error !== ff_gold || dffe_error !== fe_gold)
            $fatal(1, "PREPARE local error: dff=%b expected=%b dffe=%b expected=%b",
                   dff_error, ff_gold, dffe_error, fe_gold);
    endtask
    task automatic commit(input logic allowed);
        permit = allowed; phase = 2; #1; phase = 0; #1;
    endtask
    task automatic discard();
        permit = 0; phase = 3; #1; phase = 0; #1;
    endtask
    task automatic cannot_replay(input payload_t ff_gold, input payload_t fe_gold,
                                 input string label);
        // No new PREPARE: permission alone cannot revive consumed work.
        if (dff_error !== 1 || dffe_error !== 1)
            $fatal(1, "%s: omitted PREPARE must fail validation", label);
        commit(1);
        q_is(ff_gold, fe_gold, label);
    endtask
    task automatic host_reset(input payload_t initial_value);
        initial_data = initial_value;
        #1; phase = 4; #1;
        if (dff_error !== 0 || dffe_error !== 0)
            $fatal(1, "host Reset must bypass ordinary control validation");
        commit(1);
        q_is(initial_value, initial_value, "host Reset");
    endtask
    task automatic falling(input payload_t ff_gold, input payload_t fe_gold);
        clk = 0; rst = 0; en = 1;
        prepare(0, 0); commit(1);
        q_is(ff_gold, fe_gold, "falling sample holds Q");
    endtask

    initial begin
        payload_t a, b, c, old_ff, old_fe;
        a = pattern(0); b = pattern(2); c = pattern(5);
        dff_data = b; dffe_data = b; initial_data = a;
        auto_data = b; auto_init = a;
        #1;
        if (dff_error !== 1 || dffe_error !== 1)
            $fatal(1, "missing initial PREPARE must report incomplete work");
`ifndef VERILATOR
        if ($test$plusargs("BAD_AUTONOMOUS_RESET")) begin
            auto_rst = 1'bx; #1; auto_clk = 1; #1;
            $fatal(1, "autonomous reset validation failed to reject");
        end
        if ($test$plusargs("BAD_AUTONOMOUS_ENABLE")) begin
            auto_rst = 0; auto_en = 1'bx; #1; auto_clk = 1; #1;
            $fatal(1, "autonomous enable validation failed to reject");
        end
`endif
        // Physical high at host Reset still stages native clock history false.
        clk = 1; rst = 0; en = 0;
        host_reset(a);
        cannot_replay(a, a, "host Reset candidate consumed once");
        en = 1; prepare(0, 0);
        q_is(a, a, "PREPARE owns no Q");
        commit(1); q_is(b, b, "first high after host Reset rises");
        cannot_replay(b, b, "successful candidate consumed once");

        // A held or falling known clock does not demand reset/enable knownness.
`ifndef VERILATOR
        rst = 1'bx; en = 1'bz; four_state_cases = four_state_cases + 1;
`else
        rst = 0; en = 1;
`endif
        dff_data = c; dffe_data = c;
        prepare(0, 0); commit(1); q_is(b, b, "held high holds Q");
        clk = 0; prepare(0, 0); commit(1);
        q_is(b, b, "falling does not sample reset or enable");

        clk = 1; rst = 0; en = 0; dff_data = c;
`ifndef VERILATOR
        dffe_data = unknown_data(); four_state_cases = four_state_cases + 1;
`else
        dffe_data = a;
`endif
        prepare(0, 0); commit(1); q_is(c, b, "disabled DFFE ignores data");
        // Discard a falling clock, then retry it; the next rise must still work.
        clk = 0; en = 1; prepare(0, 0); discard();
        q_is(c, b, "discard preserves Q and sampled clock");
        cannot_replay(c, b, "discarded falling candidate cannot replay");
        // If a stale falling clock was replayed, this held high would rise.
        clk = 1; dff_data = a; dffe_data = a;
        prepare(0, 0); commit(1);
        q_is(c, b, "discarded falling history remains high");
        clk = 0;
        prepare(0, 0); commit(1); q_is(c, b, "retry falling commits only clock");
        clk = 1; dff_data = a; dffe_data = a; prepare(0, 0);
        // Live mutation after PREPARE must not alter payload/reset/enable/clock.
        clk = 0; rst = 1; en = 0; initial_data = c;
        dff_data = b; dffe_data = b;
        #1; q_is(c, b, "frozen candidate leaves Q old");
        commit(1); q_is(a, a, "commit uses frozen candidate");
        clk = 1; rst = 0; en = 1;
        #1; q_is(a, a, "managed physical edge never commits autonomously");
        prepare(0, 0); commit(1);
        q_is(a, a, "frozen sampled high prevents invented retry edge");

        falling(a, a);
        clk = 1; dff_data = c; dffe_data = c; prepare(0, 0);
        commit(0); q_is(a, a, "permission guards complete update");
        cannot_replay(a, a, "denied candidate cannot replay under new permission");
        dff_data = b; dffe_data = b;
        prepare(0, 0); commit(1);
        q_is(b, b, "denied commit preserves retry rising clock");

`ifndef VERILATOR
        falling(b, b); clk = 1; dff_data = c; dffe_data = c;
        prepare(0, 0); commit(1'bx); q_is(b, b, "unknown permission cannot commit");
        // The frozen leaf interface is tri1: explicit Z, like omission, pulls
        // up to permission one. Missing checked-family context is a distinct
        // generated-family binding guard, not this primitive's permit default.
        prepare(0, 0); commit(1'bz); q_is(c, c, "tri1 Z permission resolves high");
        dff_data = a; dffe_data = a;
        prepare(0, 0); commit(1); q_is(c, c, "Z-permitted commit records rising clock");
        // Restore a distinct known baseline before control-failure tests.
        falling(c, c); clk = 1; dff_data = b; dffe_data = b;
        prepare(0, 0); commit(1); q_is(b, b, "known baseline restored");
        falling(b, b);
        clk = 1; rst = 0; en = 1'bx; dff_data = a; dffe_data = c;
        prepare(0, 1); q_is(b, b, "sibling validation precedes global decision");
        discard(); q_is(b, b, "DFFE failure discards valid DFF progress");
        cannot_replay(b, b, "discarded rising candidates cannot replay");
        en = 1; prepare(0, 0); commit(1); q_is(a, c, "retry after enable failure");
        falling(a, c);
        clk = 1; rst = 1'bx;
        prepare(1, 1); discard(); q_is(a, c, "unknown reset discards");
        rst = 0; dff_data = b; dffe_data = a;
        prepare(0, 0); commit(1); q_is(b, a, "retry after reset failure");
        // Unknown clock is invalid on every sample, irrespective of edge.
        falling(b, a); clk = 1'bx;
        prepare(1, 1); discard(); q_is(b, a, "unknown X clock discards");
        clk = 1'bz; prepare(1, 1); discard();
        q_is(b, a, "unknown Z clock discards");
        clk = 1; dff_data = c; dffe_data = c;
        prepare(0, 0); commit(1); q_is(c, c, "known retry after unknown clock");
        four_state_cases = four_state_cases + 4;
`else
        clk = 1; rst = 0; en = 1;
        q_is(b, b, "known-mode retry state");
`endif
        old_ff = dff_q; old_fe = dffe_q;
        falling(old_ff, old_fe);
        clk = 1; rst = 1; initial_data = a;
`ifndef VERILATOR
        en = 1'bx;
`else
        en = 0;
`endif
        prepare(0, 0); q_is(old_ff, old_fe, "ordinary physical Reset is pending");
        discard(); q_is(old_ff, old_fe, "sibling failure discards physical Reset");
        prepare(0, 0); commit(1); q_is(a, a, "physical Reset retry");
`ifndef VERILATOR
        rst = 1'bx; en = 1'bz; clk = 1'bx;
        host_reset(unknown_data());
        four_state_cases = four_state_cases + 1;
`else
        host_reset(c);
`endif
        clk = 1; rst = 0; en = 1; dff_data = b; dffe_data = c;
        prepare(0, 0); commit(1); q_is(b, c, "host Reset restarts clock history");
`ifndef VERILATOR
        falling(b, c); clk = 1;
        dff_data = unknown_data(); dffe_data = unknown_data();
        prepare(0, 0); commit(1);
        q_is(unknown_data(), unknown_data(), "enabled X/Z payload transport");
        four_state_cases = four_state_cases + 1;
`endif

        // The same owning leaves retain old autonomous behavior and signatures.
        auto_rst = 1; auto_en = 0;
`ifndef VERILATOR
        auto_en = 1'bx;
`endif
        #1; auto_clk = 1; #1;
        if (auto_ff_q !== a || auto_fe_q !== a) $fatal(1, "autonomous reset");
        auto_clk = 0; auto_rst = 0; auto_en = 0;
`ifndef VERILATOR
        auto_data = unknown_data();
`else
        auto_data = b;
`endif
        #1; auto_clk = 1; #1;
        if (auto_ff_q !== auto_data || auto_fe_q !== a)
            $fatal(1, "autonomous disabled DFFE data");
        auto_clk = 0; auto_en = 1; auto_data = c;
        #1; auto_clk = 1; #1;
        if (auto_ff_q !== c || auto_fe_q !== c) $fatal(1, "autonomous data");
        auto_data = b; #1;
        if (auto_ff_q !== c || auto_fe_q !== c) $fatal(1, "autonomous held clock");
        $display("PASS registers width=%0d comparisons=%0d four_state=%0d",
                 WIDTH, comparisons, four_state_cases);
        $finish;
    end
    initial begin #10000; $fatal(1, "register test timeout"); end
endmodule

// SYNTHESIS removes phase/lifecycle machinery but retains complete-update safety.
module tb_synthesis;
    parameter integer WIDTH = 13;
    typedef logic [WIDTH-1:0] payload_t;
    logic clk = 0, rst = 1, en = 1, permit = 1;
    payload_t data = payload_t'(9), initial_data = payload_t'(5);
    wire payload_t ff_q, fe_q;
    wire ff_error, fe_error;
    dff #(.T(payload_t), .pyc_managed(1)) ff (
        .clk(clk), .rst(rst), .d(data), .init(initial_data), .q(ff_q),
        .pyc_phase(3'd0), .pyc_root_commit_ok(permit), .pyc_local_error(ff_error));
    dffe #(.T(payload_t), .pyc_managed(1)) fe (
        .clk(clk), .rst(rst), .en(en), .d(data), .init(initial_data), .q(fe_q),
        .pyc_phase(3'd0), .pyc_root_commit_ok(permit), .pyc_local_error(fe_error));
    initial begin
        #1; clk = 1; #1;
        if (ff_q !== initial_data || fe_q !== initial_data) $fatal(1, "synthesis reset");
        clk = 0; rst = 0; permit = 0;
        #1; clk = 1; #1;
        if (ff_q !== initial_data || fe_q !== initial_data) $fatal(1, "synthesis permit bypass");
        clk = 0; permit = 1;
        #1; clk = 1; #1;
        if (ff_q !== data || fe_q !== data) $fatal(1, "synthesis ordinary update");
        clk = 0; en = 0; data = payload_t'(3);
        #1; clk = 1; #1;
        if (ff_q !== data || fe_q !== payload_t'(9)) $fatal(1, "synthesis enable");
        clk = 0; rst = 1; permit = 0; initial_data = payload_t'(7);
        #1; clk = 1; #1;
        if (ff_q !== payload_t'(3) || fe_q !== payload_t'(9))
            $fatal(1, "synthesis physical reset bypassed permission");
        $display("PASS synthesis registers width=%0d", WIDTH);
        $finish;
    end
    initial begin #1000; $fatal(1, "synthesis test timeout"); end
endmodule
