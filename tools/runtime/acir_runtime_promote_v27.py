#!/usr/bin/env python3
"""Promote the packed OpenTitan one-hot mux adapter as runtime v0.27."""
from __future__ import annotations
import hashlib, json
from pathlib import Path

from _paths import normalize_spec

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / "library" / "verilog"
CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests/parameterized-components-v0.27.json"
LOCK = ROOT / "catalog.lock.json"

def main() -> int:
    name = "opentitan-onehot-mux"
    wrapper = "verilog/pyc_runtime_opentitan_onehot_mux.sv"
    source = "verilog/opentitan/prim_onehot_mux.sv"
    and_source = "verilog/opentitan/prim_and2.sv"
    files = [wrapper, source, and_source, "verilog/opentitan/prim_assert.sv"]
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    entry = normalize_spec({
        "name": name, "module": "pyc_runtime_opentitan_onehot_mux", "implementation": "prim_onehot_mux",
        "source": "opentitan-v0.15", "provider": "github", "status": "accepted", "family": "interconnect-selection",
        "wrapper": wrapper, "files": files,
        "provenance": {"repository": "https://github.com/lowRISC/opentitan.git", "commit": "b16f2be75d2f38c62d861208453ed5b81ccf41b0", "source_file": "hw/ip/prim/rtl/prim_onehot_mux.sv", "license": "Apache-2.0"},
        "interface": {"wrapper_module": "pyc_runtime_opentitan_onehot_mux", "parameters": [{"name":"WIDTH","source":"Width","default":8},{"name":"INPUTS","source":"Inputs","default":2}], "ports":[{"name":"clk","direction":"input","width":"1"},{"name":"rst_n","direction":"input","width":"1"},{"name":"data_in","direction":"input","width":"INPUTS*WIDTH"},{"name":"select_onehot","direction":"input","width":"INPUTS"},{"name":"data_out","direction":"output","width":"WIDTH"}]},
        "oracle": {"id":"opentitan-onehot-mux-v1","kind":"combinational","contract":"for a one-hot-or-zero select, data_out equals the selected packed input word, and zero select produces zero"},
        "dependency_closure": {"status":"complete","source_files":[source,and_source,"verilog/opentitan/prim_assert.sv"],"license_files":["licenses/opentitan/LICENSE"],"include_roots":["verilog/opentitan"]},
        "validation": {"status":"passed","mode":"packaged-functional-verilator-yosys","semantic_status":"functional_oracle_v1","manifest":"manifests/parameterized-components-v0.27.json","configs":[{"WIDTH":1,"INPUTS":1},{"WIDTH":4,"INPUTS":2},{"WIDTH":8,"INPUTS":4},{"WIDTH":3,"INPUTS":5}],"functional_report":".pycircuit_out/runtime-functional-validation/v27-opentitan-onehot-mux.json","runtime_gate_report":".pycircuit_out/runtime-catalog-validation/v27-opentitan-onehot-mux.json"}
    })
    entries = [e for e in catalog.get("entries", []) if e.get("name") != name] + [entry]
    entries.sort(key=lambda e: str(e.get("name", "")))
    catalog.update({"entries": entries, "runtime_api_version":"0.27", "generated_by":"acir-runtime-promote-v27"})
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False)+"\n",encoding="utf-8")
    old=json.loads((ROOT/"manifests/parameterized-components-v0.26.json").read_text(encoding="utf-8"))
    comps=[c for c in old.get("components",[]) if c.get("name") != name]
    comps.append({"name":name,"oracle":entry["oracle"],"parameters":{"WIDTH":"Width","INPUTS":"Inputs"},"configs":entry["validation"]["configs"],"source":{"repository":entry["provenance"]["repository"],"commit":entry["provenance"]["commit"],"files":files,"license":"licenses/opentitan/LICENSE"}})
    hashes={}
    for e in entries:
        for item in [*e.get("files",[]),*e.get("dependency_closure",{}).get("license_files",[])]:
            p=ROOT/item
            if p.is_file(): hashes[item]=hashlib.sha256(p.read_bytes()).hexdigest()
    MANIFEST.write_text(json.dumps({"schema":"acir-runtime-parameterized-components-v0.27","release":"runtime-rtl-v0.27","generated_by":"acir-runtime-promote-v27","toolchain":old.get("toolchain",{}),"policy":old.get("policy",{}),"components":sorted(comps,key=lambda c:c["name"]),"sha256":dict(sorted(hashes.items()))},indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    lock=json.loads(LOCK.read_text(encoding="utf-8")); lock.setdefault("sources",{})["opentitan-v0.15"]={"repository":entry["provenance"]["repository"],"commit":entry["provenance"]["commit"],"license":"Apache-2.0"}; LOCK.write_text(json.dumps(lock,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    print(json.dumps({"new_components":1,"catalog_entries":len(entries),"manifest_components":len(comps)})); return 0
if __name__ == "__main__": raise SystemExit(main())
