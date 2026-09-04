#!/usr/bin/env python3
from __future__ import annotations

import argparse, csv, json
from collections import defaultdict
from pathlib import Path
from typing import Dict, List

from matcher import match_record
from repo_manager import ensure_repo, git_metadata
from rtl_scanner import discover_files
from sv_parser import parse_sv_file


def load_config(path: Path) -> Dict:
    if path.suffix.lower() == ".json":
        return json.loads(path.read_text(encoding="utf-8"))
    if path.suffix.lower() in {".yaml", ".yml"}:
        try:
            import yaml
        except ImportError as exc:
            raise SystemExit("YAML config requires PyYAML; default JSON needs no extra package.") from exc
        return yaml.safe_load(path.read_text(encoding="utf-8"))
    raise SystemExit("Unsupported config: {}".format(path))


def write_csv(path: Path, rows: List[Dict], fields: List[str]):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
        w.writeheader(); w.writerows(rows)


def write_jsonl(path: Path, rows: List[Dict]):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        for row in rows:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")


def jdump(x):
    return json.dumps(x, ensure_ascii=False, separators=(",", ":"))


def resolve_include(name, by_basename, all_repo_files):
    inc = name.replace("\\", "/").lstrip("./")

    exact = [p for p in all_repo_files if p == inc]
    if len(exact) == 1:
        return "resolved", exact[0]

    suffix = [p for p in all_repo_files if p.endswith("/" + inc)]
    if len(suffix) == 1:
        return "resolved", suffix[0]
    if len(suffix) > 1:
        return "ambiguous", "|".join(sorted(suffix))

    hits = by_basename.get(Path(inc).name, [])
    if len(hits) == 1:
        return "resolved", hits[0]
    if len(hits) > 1:
        return "ambiguous", "|".join(sorted(hits))
    return "unresolved", ""


def main():
    ap = argparse.ArgumentParser(description="pyCircuit RTL crawler v0.3: discovery + structural metadata")
    ap.add_argument("--sources", default="sources.json")
    ap.add_argument("--targets", default="targets.json")
    ap.add_argument("--workdir", default="work")
    ap.add_argument("--output", default="output")
    ap.add_argument("--source", action="append", default=[])
    ap.add_argument("--no-update", action="store_true")
    args = ap.parse_args()

    base = Path.cwd()
    sources_cfg, targets_cfg = load_config(base/args.sources), load_config(base/args.targets)
    targets = targets_cfg.get("targets", [])
    sources = [
        s for s in sources_cfg.get("sources", [])
        if s.get("enabled", True) and s.get("scan", True)
    ]
    if args.source:
        wanted = set(args.source); sources = [s for s in sources if s["project"] in wanted]
    if not sources: raise SystemExit("No enabled sources selected.")

    repos_dir = (base/args.workdir/"repos").resolve()
    outdir = (base/args.output).resolve()
    inventory=[]; raw_hits=[]; details=[]; dep_edges=[]; file_meta=[]

    print("=== pyCircuit RTL Crawler v0.3 ===")
    print("sources:", ", ".join(s["project"] for s in sources))
    print("targets:", len(targets)); print()

    for source in sources:
        project = source["project"]
        repo_dir = ensure_repo(source, repos_dir, update=not args.no_update)
        meta = git_metadata(repo_dir)
        rtl_files = discover_files(repo_dir, source)
        parsed=[]; module_index=defaultdict(list); package_index=defaultdict(list); by_basename=defaultdict(list); all_repo_files=[]

        for path in rtl_files:
            rel = path.relative_to(repo_dir).as_posix(); all_repo_files.append(rel); by_basename[path.name].append(rel)
            try: pf = parse_sv_file(path, repo_dir)
            except Exception as e:
                print("[parse-warning] {} {}: {}".format(project, rel, e)); continue
            parsed.append(pf)
            for pkg in pf.get("packages_declared", []): package_index[pkg].append(rel)
            for m in pf.get("modules", []): module_index[m["module"]].append(m["file"])
            file_meta.append({
                "source_project":project,"file":rel,
                "include_count":len(pf.get("includes",[])),"import_count":len(pf.get("imports",[])),
                "package_decl_count":len(pf.get("packages_declared",[])),"module_count":len(pf.get("modules",[])),
                "includes_json":jdump(pf.get("includes",[])),"imports_json":jdump(pf.get("imports",[])),
                "packages_declared_json":jdump(pf.get("packages_declared",[])),
            })

        modules = [m for pf in parsed for m in pf.get("modules", [])]
        print("[scan] {}: {} RTL files, {} modules".format(project, len(rtl_files), len(modules)))
        candidate_keys=set()

        for m in modules:
            rec={"module":m["module"],"file":m["file"],"file_stem":Path(m["file"]).stem,"suffix":Path(m["file"]).suffix.lower()}
            inventory.append({
                "source_project":project,"source_priority":source.get("priority",""),"repo_url":meta["remote_url"],
                "commit_sha":meta["commit_sha"],"branch":meta.get("branch",""),**rec,
                "parameter_count":len(m["parameters"]),"port_count":len(m["ports"]),"instance_count":len(m["instances"]),
                "clock_names":";".join(x["name"] for x in m["clocks"]),"reset_names":";".join(x["name"] for x in m["resets"]),
                "handshakes":";".join(m["handshakes"]),
            })
            for hit in match_record(rec, targets):
                candidate_keys.add((project,m["module"],m["file"]))
                raw_hits.append({
                    "target_id":hit.get("target_id",""),"gap_id":hit.get("gap_id",""),"family":hit.get("family",""),
                    "operation":hit.get("operation",""),"data_format":hit.get("data_format",""),"input_format":hit.get("input_format",""),
                    "acc_format":hit.get("acc_format",""),"priority":hit.get("priority",""),"source_project":project,
                    "source_priority":source.get("priority",""),"repo_url":meta["remote_url"],"commit_sha":meta["commit_sha"],
                    "branch":meta.get("branch",""),"module":m["module"],"file":m["file"],"matched_keywords":hit["matched_keywords"],
                    "discovery_match_score":hit["match_score"],
                })

        for m in modules:
            if (project,m["module"],m["file"]) not in candidate_keys: continue
            inst_out=[]; inc_out=[]; imp_out=[]
            for inst in m["instances"]:
                hits=module_index.get(inst["module_type"],[])
                status="resolved" if len(hits)==1 else ("ambiguous" if len(hits)>1 else "external_or_unresolved")
                resolved=hits[0] if len(hits)==1 else "|".join(hits)
                inst_out.append({**inst,"resolution_status":status,"resolved_file":resolved})
                dep_edges.append({"source_project":project,"top_module":m["module"],"top_file":m["file"],"dependency_kind":"submodule",
                                  "dependency_name":inst["module_type"],"instance_name":inst["instance_name"],"resolution_status":status,"resolved_file":resolved})
            for inc in m["includes"]:
                status,resolved=resolve_include(inc,by_basename,all_repo_files); inc_out.append({"include":inc,"resolution_status":status,"resolved_file":resolved})
                dep_edges.append({"source_project":project,"top_module":m["module"],"top_file":m["file"],"dependency_kind":"include",
                                  "dependency_name":inc,"instance_name":"","resolution_status":status,"resolved_file":resolved})
            for pkg in m["imports"]:
                hits=package_index.get(pkg,[]); status="resolved" if len(hits)==1 else ("ambiguous" if len(hits)>1 else "external_or_unresolved")
                resolved=hits[0] if len(hits)==1 else "|".join(hits); imp_out.append({"package":pkg,"resolution_status":status,"resolved_file":resolved})
                dep_edges.append({"source_project":project,"top_module":m["module"],"top_file":m["file"],"dependency_kind":"package",
                                  "dependency_name":pkg,"instance_name":"","resolution_status":status,"resolved_file":resolved})
            details.append({
                "source_project":project,"repo_url":meta["remote_url"],"commit_sha":meta["commit_sha"],"branch":meta.get("branch",""),
                "module":m["module"],"file":m["file"],"parameter_count":len(m["parameters"]),"port_count":len(m["ports"]),
                "instance_count":len(m["instances"]),"clock_count":len(m["clocks"]),"reset_count":len(m["resets"]),
                "clock_names":";".join(x["name"] for x in m["clocks"]),"reset_names":";".join(x["name"] for x in m["resets"]),
                "reset_info_json":jdump(m["resets"]),"handshakes":";".join(m["handshakes"]),
                "parameters_json":jdump(m["parameters"]),"ports_json":jdump(m["ports"]),"instances_json":jdump(inst_out),
                "includes_json":jdump(inc_out),"imports_json":jdump(imp_out),
            })

    raw_hits.sort(key=lambda x:(x["target_id"],-int(x["discovery_match_score"]),x["source_project"],x["module"]))
    inventory.sort(key=lambda x:(x["source_project"],x["module"],x["file"])); details.sort(key=lambda x:(x["source_project"],x["module"],x["file"]))
    dep_edges.sort(key=lambda x:(x["source_project"],x["top_module"],x["dependency_kind"],x["dependency_name"])); file_meta.sort(key=lambda x:(x["source_project"],x["file"]))

    write_csv(outdir/"module_inventory.csv", inventory, ["source_project","source_priority","repo_url","commit_sha","branch","module","file","file_stem","suffix","parameter_count","port_count","instance_count","clock_names","reset_names","handshakes"])
    write_csv(outdir/"candidates_raw.csv", raw_hits, ["target_id","gap_id","family","operation","data_format","input_format","acc_format","priority","source_project","source_priority","repo_url","commit_sha","branch","module","file","matched_keywords","discovery_match_score"])
    write_jsonl(outdir/"candidates_raw.jsonl", raw_hits)
    write_csv(outdir/"candidate_details.csv", details, ["source_project","repo_url","commit_sha","branch","module","file","parameter_count","port_count","instance_count","clock_count","reset_count","clock_names","reset_names","reset_info_json","handshakes","parameters_json","ports_json","instances_json","includes_json","imports_json"])
    write_jsonl(outdir/"candidate_details.jsonl", details)
    write_csv(outdir/"dependency_edges.csv", dep_edges, ["source_project","top_module","top_file","dependency_kind","dependency_name","instance_name","resolution_status","resolved_file"])
    write_csv(outdir/"file_metadata.csv", file_meta, ["source_project","file","include_count","import_count","package_decl_count","module_count","includes_json","imports_json","packages_declared_json"])

    matched={x["target_id"] for x in raw_hits}; unmatched=[]
    for t in targets:
        if t.get("target_id") not in matched:
            unmatched.append({"target_id":t.get("target_id",""),"gap_id":t.get("gap_id",""),"family":t.get("family",""),"operation":t.get("operation",""),"priority":t.get("priority",""),"keywords":";".join(t.get("keywords",[]))})
    write_csv(outdir/"unmatched_targets.csv", unmatched, ["target_id","gap_id","family","operation","priority","keywords"])

    resolved=sum(1 for x in dep_edges if x["resolution_status"]=="resolved")
    print(); print("=== Result ===")
    print("modules discovered :",len(inventory)); print("candidate matches  :",len(raw_hits)); print("candidate modules  :",len(details))
    print("dependency edges   :",len(dep_edges)); print("  resolved         :",resolved); print("  unresolved/other :",len(dep_edges)-resolved)
    print("targets matched    :",len(matched)); print("targets unmatched  :",len(unmatched)); print("output             :",outdir)
    print(); print("New v0.2 files:")
    for name in ["candidate_details.csv","candidate_details.jsonl","dependency_edges.csv","file_metadata.csv"]: print("  -",outdir/name)


if __name__ == "__main__": main()
