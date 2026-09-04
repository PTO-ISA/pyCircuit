#!/usr/bin/env python3
from pathlib import Path
import tempfile
import yaml
from dependency_closure import RepositoryModel, build_dependency_closure

def main():
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        (root/"src").mkdir()
        (root/"src/top.sv").write_text(
            "module top(); logic x; assign x = dead_pkg::foo; endmodule\n",
            encoding="utf-8")
        source={"path_hints":["src"],"extensions":[".sv"],"exclude_dirs":[".git"]}
        model=RepositoryModel(root, source)
        c0=build_dependency_closure(model,"top")
        assert c0["closure_status"]!="COMPLETE"
        c1=build_dependency_closure(model,"top",prune_packages=["dead_pkg"])
        assert c1["closure_status"]=="COMPLETE"
        assert c1["pruned_packages"]==["dead_pkg"]

    specs=yaml.safe_load(Path("design_class_specs.yaml").read_text(encoding="utf-8"))
    fifo=specs["design_classes"]["FIFO-SYNC"]["candidates"]
    pulp=next(c for c in fifo if c["project"]=="pulp_common_cells")
    ot=next(c for c in fifo if c["project"]=="opentitan")
    assert pulp["prune_packages"]==["assert_rpt_pkg"]
    assert ot["prune_packages"]==["uvm_pkg"]
    assert set(ot["prune_modules"])=={"prim_count","prim_flop"}
    print("smoke_test_v0.11.2: PASS")

if __name__=="__main__":
    main()
