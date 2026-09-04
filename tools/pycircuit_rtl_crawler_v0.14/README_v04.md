# pyCircuit RTL crawler v0.4 — local source-pool workflow

This revision separates repository materialization from RTL discovery.  The
search stage first creates a reproducible local source pool, then scans the
checked-out `.v/.sv/.vh/.svh` files and records candidate matches against the
236-target manifest.  Runtime acceptance still requires dependency closure,
Verilator, simulation and Yosys stages; discovery alone never promotes a
module into the runtime library.

## Target manifests

`targets.json` is the original v0.3 input.  Generate the non-destructive v0.4
manifest with:

```bash
python make_targets_v04.py \
  --targets targets.json \
  --sources sources.expanded.v0.3.json \
  --output targets.v0.4.json
```

`targets.v0.4.json` keeps all 236 canonical targets and adds candidate source
projects plus explicit clone, scan, validation and runtime states.  After a
local discovery wave, attach counts and statuses without changing the source
manifest:

```bash
python update_targets_v04_status.py \
  --targets targets.v0.4.json \
  --sources build/source-cache-v0.4/source-manifest-v0.4.json \
  --discovery build/discovery-v0.4/all-local-v0.4/candidates_raw.csv \
  --output targets.v0.4.materialized.json
```

## Materialize repositories

The materializer uses shallow, blob-filtered sparse checkouts where a source
declares `path_hints`.  It records URL, branch, commit SHA, checkout mode,
file/RTL counts and license-file presence.  License is informational only for
this search experiment (`license_gate: false`); it is not removed from the
provenance record.

```bash
python materialize_source_pool.py \
  --sources sources.expanded.v0.3.json \
  --root build/source-cache-v0.4/repos \
  --manifest build/source-cache-v0.4/source-manifest-v0.4.json \
  --no-update
```

Run one source at a time for large repositories:

```bash
python materialize_source_pool.py --source nvdla --no-update
```

## Discover from the local pool

```bash
python crawler.py \
  --sources sources.expanded.v0.3.json \
  --targets targets.v0.4.json \
  --workdir build/source-cache-v0.4 \
  --output build/discovery-v0.4/all-local-v0.4 \
  --no-update
```

The output contains module inventory, candidate CSV/JSONL, structural details,
dependency edges, file metadata and unmatched targets.  A candidate is still
only a search result until its dependency closure and implementation-specific
Verilator/Yosys checks pass.

## Current wave (2026-08-31)

* 19 configured repositories materialized or recorded in
  `build/source-cache-v0.4/source-manifest-v0.4.json`.
* 18 repositories expose direct RTL in the selected checkout paths (3,530 RTL
  files in total).  Gemmini is currently Chisel/Scala-only and therefore has
  no direct RTL files in the local pool.
* Local discovery scanned 3,256 modules and produced 373 target-keyword hits
  across 355 candidate modules; 46 of 236 targets have at least one local
  structural match.
* Dependency resolution for the discovered candidates: 1,175 resolved edges
  and 332 unresolved/ambiguous/external edges.

These numbers are discovery-stage measurements, not runtime-library acceptance
counts.  Validation and PPA are intentionally staged after candidate closure.
