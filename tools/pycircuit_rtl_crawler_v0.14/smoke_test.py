#!/usr/bin/env python3
from pathlib import Path
import tempfile
from matcher import match_record
from sv_parser import parse_sv_file, parse_instances

DEMO = r'''
`include "demo_defs.svh"
module child #(parameter int W = 8) (
    input logic [W-1:0] a_i,
    output logic [W-1:0] y_o
);
assign y_o = a_i;
endmodule

module cc_rr_arb_tree #(
    parameter int unsigned NumIn = 4,
    parameter type DataT = logic [31:0]
) (
    input logic clk_i,
    input logic rst_ni,
    input logic [NumIn-1:0] req_i,
    output logic [NumIn-1:0] gnt_o,
    input logic valid_i,
    output logic ready_o
);
    import cc_pkg::*;
    child #(.W(8)) i_child (.a_i('0), .y_o());
    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) gnt_o <= '0;
    end
endmodule
'''

def test_no_control_statement_false_instances():
    body = """
    if (cond) begin
    end
    for (i = 0; i < 4; i = i + 1) begin
    end
    child #(.W(8)) i_child (.a_i('0), .y_o());
    """
    inst = parse_instances(body)
    assert any(x["module_type"] == "child" for x in inst), inst
    assert not any(x["module_type"] in {"if","i","for","fo"} for x in inst), inst

def main():
    test_no_control_statement_false_instances()
    with tempfile.TemporaryDirectory() as td:
        root=Path(td)
        (root/'demo.sv').write_text(DEMO,encoding='utf-8')
        (root/'demo_defs.svh').write_text('`define X 1\n',encoding='utf-8')
        pf=parse_sv_file(root/'demo.sv',root)
        mods={m['module']:m for m in pf['modules']}; rr=mods['cc_rr_arb_tree']
        assert any(p['name']=='NumIn' for p in rr['parameters'])
        assert any(p['name']=='clk_i' for p in rr['ports'])
        assert rr['clocks'][0]['name']=='clk_i'
        assert rr['resets'][0]['name']=='rst_ni' and rr['resets'][0]['polarity']=='active_low'
        assert rr['resets'][0]['style']=='async'
        assert 'valid_ready' in rr['handshakes'] and 'req_gnt' in rr['handshakes']
        assert any(i['module_type']=='child' for i in rr['instances'])
        assert 'demo_defs.svh' in rr['includes'] and 'cc_pkg' in rr['imports']
        hits=match_record({'module':rr['module'],'file':rr['file']},[{'target_id':'P0-RR-ARBITER','gap_id':'DF-09','family':'Dataflow','operation':'round_robin_arbiter','priority':'P0','keywords':['rr_arb','round_robin']}])
        assert hits and hits[0]['gap_id']=='DF-09'
    print('smoke_test_v0.2.1: PASS')

if __name__=='__main__': main()
