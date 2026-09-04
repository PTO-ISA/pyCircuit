#!/usr/bin/env python3
from pathlib import Path
import tempfile
import yaml

from dependency_closure import RepositoryModel, build_dependency_closure
from sv_parser import parse_instances

def main():
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        (root/"src").mkdir()
        (root/"src/pkg.sv").write_text(
            "package cc_pkg; function automatic int cnt_width(int x); return x; endfunction endpackage\n",
            encoding="utf-8")
        (root/"src/fifo.sv").write_text(
            "module cc_fifo #(localparam int W=cc_pkg::cnt_width(4))(); endmodule\n",
            encoding="utf-8")
        source={"path_hints":["src"],"extensions":[".sv"],"exclude_dirs":[".git"]}
        c=build_dependency_closure(RepositoryModel(root,source),"cc_fifo")
        assert "src/pkg.sv" in c["package_files"]

    assert not any(x["module_type"]=="endfunction" for x in parse_instances(
        "function automatic int f(input int x);\\n return x;\\nendfunction\\n"))

    with tempfile.TemporaryDirectory() as td:
        root=Path(td); (root/"src").mkdir()
        (root/"src/top.sv").write_text(
            "module top #(parameter bit Secure=0)();\\n"
            "if (Secure) begin\\n  prim_flop u();\\nend\\nendmodule\\n",
            encoding="utf-8")
        source={"path_hints":["src"],"extensions":[".sv"],"exclude_dirs":[".git"]}
        c=build_dependency_closure(RepositoryModel(root,source),"top",
                                   prune_modules=["prim_flop"])
        assert c["closure_status"]=="COMPLETE"
        assert c["pruned_modules"]==["prim_flop"]

    specs=yaml.safe_load(Path("design_class_specs.yaml").read_text(encoding="utf-8"))
    ot=next(c for c in specs["design_classes"]["FIFO-SYNC"]["candidates"]
            if c["project"]=="opentitan")
    assert set(ot["prune_modules"])=={"prim_count","prim_flop"}
    print("smoke_test_v0.11.1: PASS")

if __name__=="__main__":
    main()
