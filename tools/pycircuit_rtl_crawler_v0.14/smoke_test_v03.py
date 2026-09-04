#!/usr/bin/env python3
from pathlib import Path
import json
import tempfile

from dependency_closure import RepositoryModel, build_dependency_closure


def main():
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        (root / 'src').mkdir()
        (root / 'include' / 'demo').mkdir(parents=True)

        (root / 'include' / 'demo' / 'defs.svh').write_text('`define DEMO 1\n', encoding='utf-8')
        (root / 'src' / 'pkg.sv').write_text('package demo_pkg; typedef logic [7:0] byte_t; endpackage\n', encoding='utf-8')
        (root / 'src' / 'leaf.sv').write_text('''
module leaf(input logic a_i, output logic y_o);
  assign y_o = a_i;
endmodule
''', encoding='utf-8')
        (root / 'src' / 'mid.sv').write_text('''
`include "demo/defs.svh"
module mid(input logic a_i, output logic y_o);
  leaf i_leaf(.a_i(a_i), .y_o(y_o));
endmodule
''', encoding='utf-8')
        (root / 'src' / 'top.sv').write_text('''
module top(input logic clk_i, input logic rst_ni, input logic a_i, output logic y_o);
  import demo_pkg::*;
  mid i_mid(.a_i(a_i), .y_o(y_o));
endmodule
''', encoding='utf-8')

        source = {
            'extensions': ['.v','.sv','.vh','.svh'],
            'exclude_dirs': ['.git','build'],
            'path_hints': [],
        }
        model = RepositoryModel(root, source)
        closure = build_dependency_closure(model, 'top')

        assert closure['closure_status'] == 'COMPLETE', closure['unresolved']
        assert 'src/top.sv' in closure['module_files']
        assert 'src/mid.sv' in closure['module_files']
        assert 'src/leaf.sv' in closure['module_files']
        assert 'src/pkg.sv' in closure['package_files']
        assert 'include/demo/defs.svh' in closure['header_files']
        assert 'include' in closure['include_roots']
        assert len(closure['unresolved']) == 0

    print('smoke_test_v0.3: PASS')


if __name__ == '__main__':
    main()
