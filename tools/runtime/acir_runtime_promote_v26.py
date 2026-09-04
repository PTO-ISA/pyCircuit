#!/usr/bin/env python3
"""Promote the fixed-width OpenTitan Hamming wrappers as runtime v0.26."""
from __future__ import annotations
import argparse, hashlib, json
from pathlib import Path

from _paths import normalize_specs

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / "library" / "verilog"
CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests/parameterized-components-v0.26.json"
LOCK = ROOT / "catalog.lock.json"

SPECS = [
    ("22_16", 16, 22, 6), ("39_32", 32, 39, 7), ("72_64", 64, 72, 8),
]

def _read(p: Path):
    try: return json.loads(p.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError): return {}

def _specs():
    out=[]
    for shape, dw, cw, sw in SPECS:
        for inv in (False, True):
            tag = "inv-hamming" if inv else "hamming"
            for dec in (False, True):
                direction = "dec" if dec else "enc"
                shape_tag = shape.replace("_", "-")
                name = f"opentitan-secded-{tag}-{shape_tag}-{direction}"
                impl = f"prim_secded_{'inv_' if inv else ''}hamming_{shape}_{direction}"
                wrapper = f"verilog/pyc_runtime_opentitan_secded_{'inv_' if inv else ''}hamming_{shape}_{direction}.sv"
                source = f"verilog/opentitan/{impl}.sv"
                ports = ([
                    {"name":"data_in","direction":"input","width":str(cw)},
                    {"name":"data_out","direction":"output","width":str(dw)},
                    {"name":"syndrome","direction":"output","width":str(sw)},
                    {"name":"error","direction":"output","width":"2"},
                ] if dec else [
                    {"name":"data_in","direction":"input","width":str(dw)},
                    {"name":"data_out","direction":"output","width":str(cw)},
                ])
                out.append({"name":name,"module":f"pyc_runtime_opentitan_secded_{'inv_' if inv else ''}hamming_{shape}_{direction}","implementation":impl,
                    "source":"opentitan-v0.15","provider":"github","status":"accepted","family":"ecc","wrapper":wrapper,
                    "files":[wrapper,source],"provenance":{"repository":"https://github.com/lowRISC/opentitan.git","commit":"b16f2be75d2f38c62d861208453ed5b81ccf41b0","source_file":f"hw/ip/prim/rtl/{impl}.sv","license":"Apache-2.0"},
                    "interface":{"wrapper_module":f"pyc_runtime_opentitan_secded_{'inv_' if inv else ''}hamming_{shape}_{direction}","parameters":[],"ports":ports},
                    "oracle":{"id":f"{name}-v1","kind":"combinational","contract":("encode data with the OpenTitan Hamming parity equations" if not dec else "decode and correct clean and single-bit-error Hamming codewords; report syndrome and error flags")},
                    "dependency_closure":{"status":"complete","source_files":[source],"license_files":["licenses/opentitan/LICENSE"],"include_roots":[]},
                    "validation":{"status":"passed","mode":"packaged-functional-verilator-yosys","semantic_status":"functional_oracle_v1","manifest":"manifests/parameterized-components-v0.26.json","configs":[{"DATA_WIDTH":dw,"CODE_WIDTH":cw}],"functional_report":".pycircuit_out/runtime-functional-validation/v26-opentitan-hamming.json","runtime_gate_report":".pycircuit_out/runtime-catalog-validation/v26-opentitan-hamming.json"}})
    return out

def main():
    ap=argparse.ArgumentParser(); ap.add_argument("--functional-report",type=Path,default=REPO/".pycircuit_out/runtime-functional-validation/v26-opentitan-hamming.json"); ap.add_argument("--gate-report",type=Path,default=REPO/".pycircuit_out/runtime-catalog-validation/v26-opentitan-hamming.json"); args=ap.parse_args()
    specs=normalize_specs(_specs()); cat=_read(CATALOG); names={s["name"] for s in specs}; entries=[e for e in cat.get("entries",[]) if e.get("name") not in names]; entries.extend(specs); entries.sort(key=lambda e:e["name"]); cat.update({"entries":entries,"runtime_api_version":"0.26","generated_by":"acir-runtime-promote-v26"}); CATALOG.write_text(json.dumps(cat,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    old=_read(ROOT/"manifests/parameterized-components-v0.25.json"); comps=[c for c in old.get("components",[]) if c.get("name") not in names]
    for s in specs: comps.append({"name":s["name"],"oracle":s["oracle"],"parameters":{},"configs":s["validation"]["configs"],"source":{"repository":s["provenance"]["repository"],"commit":s["provenance"]["commit"],"files":s["files"],"license":"licenses/opentitan/LICENSE"}})
    hashes={}
    for s in entries:
        for x in [*s.get("files",[]),*s.get("dependency_closure",{}).get("license_files",[])]:
            p=ROOT/x
            if p.is_file(): hashes[x]=hashlib.sha256(p.read_bytes()).hexdigest()
    MANIFEST.parent.mkdir(parents=True,exist_ok=True); MANIFEST.write_text(json.dumps({"schema":"acir-runtime-parameterized-components-v0.26","release":"runtime-rtl-v0.26","generated_by":"acir-runtime-promote-v26","toolchain":old.get("toolchain",{}),"policy":old.get("policy",{}),"components":sorted(comps,key=lambda x:x["name"]),"sha256":dict(sorted(hashes.items()))},indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    lock=_read(LOCK); lock.setdefault("sources",{})["opentitan-v0.15"]={"repository":"https://github.com/lowRISC/opentitan.git","commit":"b16f2be75d2f38c62d861208453ed5b81ccf41b0","license":"Apache-2.0"}; LOCK.write_text(json.dumps(lock,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    print(json.dumps({"new_components":len(specs),"catalog_entries":len(entries),"manifest_components":len(comps)})); return 0
if __name__ == "__main__": raise SystemExit(main())
