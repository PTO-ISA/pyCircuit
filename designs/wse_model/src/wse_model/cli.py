"""The ``wse-model`` command line interface.

The CLI is a thin, deterministic front end over the semantic core. Every command
can emit JSON so that results can be diffed, gated, or consumed by another tool.
"""

from __future__ import annotations

import argparse
import json
import sys
from collections.abc import Sequence
from pathlib import Path
from typing import Any

from wse_model import __version__
from wse_model.analysis import (
    BANDWIDTHS,
    PRECISIONS,
    AicoreSpec,
    dcache_budget,
    flit_header_cost,
    local_dram_vs_noc_ratio,
    roofline_table,
    two_phase_allgather_bytes,
)
from wse_model.calendar.table import CalendarRouteTable, TableLayout
from wse_model.collective import run_allgather
from wse_model.compiler import check_compiled_product, load_object_file
from wse_model.core import MatmulShape, matmul_timing
from wse_model.errors import WseModelError
from wse_model.fixtures import (
    FFN_PHASE_B_ROW_BYTES,
    FFN_PHASE_C_ROW_BYTES,
    GOLDEN_ROUTE_BITS,
    ffn_example,
    golden_route_bits,
    group_allgather_bitmap,
)
from wse_model.host import (
    BatcherMem,
    LaunchConstraints,
    RuntimeTier,
    UbBus,
    dispatch_chains,
    dispatch_paths,
)
from wse_model.host.ub_bus import local_dram_over_fabric_ratio
from wse_model.noc import Noc
from wse_model.open_items import OPEN_ITEMS, Resolution
from wse_model.topology import CALENDAR_BASELINE, PROFILES, topology_profile

#: Logical identities the ``calendar encode`` command understands.
CALENDAR_KEYS = {
    "row-allgather": ("FFN keyId 0: AllGather over the ROW group of the source (Calendar §2.2.1)"),
    "col-allgather": ("FFN keyId 1: AllGather over the COL group of the source (Calendar §2.2.1)"),
    "golden": "The Calendar §2.2.1 consistency anchor (source N00)",
}


def _emit(payload: Any, *, as_json: bool, text: str = "") -> None:
    if as_json:
        print(json.dumps(payload, indent=2, ensure_ascii=False, sort_keys=False))
    elif text:
        print(text)
    else:
        print(json.dumps(payload, indent=2, ensure_ascii=False))


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="wse-model",
        description=(
            "WSE architecture model: Calendar route encoding, NoC forwarding, "
            "collective closure, and roofline analysis."
        ),
    )
    parser.add_argument("--version", action="version", version=f"wse-model {__version__}")
    # ``--json`` is accepted both before and after the subcommand. The shared
    # parent uses SUPPRESS so that an absent leaf flag does not overwrite a value
    # already supplied to the root parser.
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument(
        "--json",
        action="store_true",
        default=argparse.SUPPRESS,
        help="emit machine-readable JSON",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        default=False,
        help="emit machine-readable JSON",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    # topology ------------------------------------------------------------
    topology = subparsers.add_parser("topology", help="inspect NoC topology profiles")
    topology_sub = topology.add_subparsers(dest="topology_command", required=True)
    topology_show = topology_sub.add_parser(
        "show", parents=[common], help="show one topology profile"
    )
    topology_show.add_argument(
        "--profile",
        default=CALENDAR_BASELINE.name,
        choices=sorted(PROFILES),
        help="topology profile to describe",
    )
    topology_show.add_argument("--links", action="store_true", help="list physical links")

    # calendar ------------------------------------------------------------
    calendar = subparsers.add_parser("calendar", help="Calendar encoding and validation")
    calendar_sub = calendar.add_subparsers(dest="calendar_command", required=True)

    encode = calendar_sub.add_parser("encode", parents=[common], help="encode one route bitmap")
    encode.add_argument("--key", default="golden", choices=sorted(CALENDAR_KEYS))
    encode.add_argument("--profile", default=CALENDAR_BASELINE.name, choices=sorted(PROFILES))
    encode.add_argument("--source", type=int, default=0, help="source node id")
    encode.add_argument(
        "--self-delivery",
        action="store_true",
        help="fabric lands the source's own copy (sets L_self)",
    )

    validate = calendar_sub.add_parser("validate", parents=[common], help="validate a route table")
    validate.add_argument(
        "--table",
        type=Path,
        help="route table JSON; omit to validate the built-in FFN example",
    )

    emit = calendar_sub.add_parser("emit", parents=[common], help="emit the .rodata table bytes")
    emit.add_argument(
        "--layout",
        default=TableLayout.KEY_MAJOR.value,
        choices=[layout.value for layout in TableLayout],
    )
    emit.add_argument("--out", type=Path, help="write the raw segment to this path")

    # run -----------------------------------------------------------------
    run = subparsers.add_parser("run", help="run an end-to-end scenario")
    run_sub = run.add_subparsers(dest="run_command", required=True)
    ffn = run_sub.add_parser(
        "ffn-allgather", parents=[common], help="run the FFN two-phase AllGather"
    )
    ffn.add_argument(
        "--phase", default="both", choices=["b", "c", "both"], help="which phase to run"
    )
    ffn.add_argument(
        "--layout",
        default=TableLayout.KEY_MAJOR.value,
        choices=[layout.value for layout in TableLayout],
    )

    # report --------------------------------------------------------------
    report = subparsers.add_parser("report", help="closed-form analysis reports")
    report_sub = report.add_subparsers(dest="report_command", required=True)
    report_sub.add_parser("roofline", parents=[common], help="balance points and minimum batch")
    report_sub.add_parser(
        "bandwidth", parents=[common], help="link bandwidths and the NoC/DRAM ratio"
    )
    report_sub.add_parser("overhead", parents=[common], help="flit header overhead per topology")
    report_sub.add_parser("aicore", parents=[common], help="AICORE specifications")
    report_sub.add_parser(
        "dcache", parents=[common], help="route-table D-cache residency and cold-miss cost"
    )
    matmul = report_sub.add_parser(
        "matmul", parents=[common], help="tile-accurate Cube and memory timing"
    )
    matmul.add_argument("--m", type=int, default=1, help="M dimension (batch)")
    matmul.add_argument("--k", type=int, default=4096, help="K dimension (reduction)")
    matmul.add_argument("--n", type=int, default=4096, help="N dimension (output)")
    matmul.add_argument(
        "--precision", default="fp16", choices=sorted(PRECISIONS), help="weight precision"
    )
    matmul.add_argument(
        "--fetched-weight-bytes",
        type=int,
        default=None,
        help="actual fetched weight bytes when the layout is padded",
    )
    report_sub.add_parser(
        "host", parents=[common], help="UB bus, Batcher responsibilities, and cache refills"
    )
    report_sub.add_parser(
        "runtime", parents=[common], help="load/launch tiers, dispatch chains, and constraints"
    )

    # check ---------------------------------------------------------------
    check = subparsers.add_parser("check", help="run the compiler product self-checks (F1/F2/F3)")
    check_sub = check.add_subparsers(dest="check_command", required=True)
    check_object = check_sub.add_parser(
        "object", parents=[common], help="check a declared kernel object manifest"
    )
    check_object.add_argument(
        "--object",
        type=Path,
        help="kernel object manifest JSON; omit with --example for the built-in baseline",
    )
    check_object.add_argument(
        "--example",
        action="store_true",
        help="check the built-in clean manifest instead of a file",
    )
    check_object.add_argument(
        "--key-count",
        type=int,
        default=2,
        help="keyCount the compiled product is expected to contain",
    )
    check_object.add_argument(
        "--node-count",
        type=int,
        default=40,
        help="nodeCount the compiled product is expected to contain",
    )

    check_package = check_sub.add_parser(
        "package",
        parents=[common],
        help="assemble and validate the compiled deployment package",
    )
    check_package.add_argument(
        "--concurrent",
        action="store_true",
        help=(
            "declare the two logical identities concurrent, which the same-opcode rule must refuse"
        ),
    )

    # open items ----------------------------------------------------------
    items = subparsers.add_parser(
        "open-items", parents=[common], help="list unresolved design items"
    )
    items.add_argument(
        "--status",
        default="all",
        choices=["all", "open", "assumed", "resolved"],
        help="filter by resolution status",
    )

    return parser


# -- handlers --------------------------------------------------------------


def _cmd_topology(args: argparse.Namespace) -> int:
    profile = topology_profile(args.profile)
    payload = profile.describe()
    if args.links:
        payload["links"] = [list(link) for link in profile.topology.links()]
    _emit(payload, as_json=args.json)
    return 0


def _cmd_calendar_encode(args: argparse.Namespace) -> int:
    profile = topology_profile(args.profile)
    topology = profile.topology
    if args.key == "golden":
        if topology.node_count != CALENDAR_BASELINE.topology.node_count:
            raise WseModelError(
                "the golden vector is defined for the 40-node Calendar baseline (Calendar §2.2.1)"
            )
        bitmap = golden_route_bits()
        payload = {
            "key": "golden",
            "description": CALENDAR_KEYS["golden"],
            "source": 0,
            **bitmap.describe(),
            "expected_hex": GOLDEN_ROUTE_BITS,
            "matches_published": bitmap.to_hex() == GOLDEN_ROUTE_BITS,
        }
        _emit(payload, as_json=args.json)
        return 0

    if args.profile != CALENDAR_BASELINE.name:
        raise WseModelError(
            "the FFN row/column groups are defined on the 40-node Calendar "
            "baseline topology (Calendar §2.2.1)"
        )
    example = ffn_example(
        layout=TableLayout(args.layout) if hasattr(args, "layout") else TableLayout.KEY_MAJOR
    )
    key_id = 0 if args.key == "row-allgather" else 1
    definition = example.table.key(key_id)
    group = definition.group_of(args.source)
    if group is None:
        raise WseModelError(
            f"node {args.source} is not an FFN cell, so it has no "
            f"{args.key} group (Calendar §2.2.1)"
        )
    bitmap = group_allgather_bitmap(topology, group, args.source, self_delivery=args.self_delivery)
    payload = {
        "key": args.key,
        "description": CALENDAR_KEYS[args.key],
        "source": args.source,
        "group": list(group),
        "self_delivery": args.self_delivery,
        **bitmap.describe(),
    }
    _emit(payload, as_json=args.json)
    return 0


def _cmd_calendar_validate(args: argparse.Namespace) -> int:
    if args.table is None:
        table = ffn_example().table
        source = "built-in FFN example"
    else:
        data = json.loads(Path(args.table).read_text(encoding="utf-8"))
        table = CalendarRouteTable.from_dict(data)
        source = str(args.table)
    report = table.validate()
    payload = {"source": source, **table.describe(), **report.describe()}
    _emit(payload, as_json=args.json, text=f"route table: {source}\n{report.format()}")
    return 0 if report.ok else 1


def _cmd_calendar_emit(args: argparse.Namespace) -> int:
    table = ffn_example(layout=TableLayout(args.layout)).table
    raw = table.to_bytes()
    if args.out:
        Path(args.out).write_bytes(raw)
    payload = {
        **table.describe(),
        "segment_bytes": len(raw),
        "alignment_bytes": 64,
        "hex": table.rodata_hex(),
    }
    if args.out:
        payload["written_to"] = str(args.out)
    _emit(payload, as_json=args.json)
    return 0


def _cmd_run_ffn(args: argparse.Namespace) -> int:
    example = ffn_example(layout=TableLayout(args.layout))
    noc = Noc(example.table.topology, calreg=example.calreg())

    reports = []
    if args.phase in ("b", "both"):
        report = run_allgather(
            noc,
            example.table,
            0,
            geometry=example.phase_b_geometry,
            groups=example.phase_b_groups,
        )
        report.check()
        reports.append(report)
    if args.phase in ("c", "both"):
        # Phase C is a separate phase over the same opcode domain, so its round
        # number is whatever the core counters yield next (invariant E1).
        report = run_allgather(
            noc,
            example.table,
            1,
            geometry=example.phase_c_geometry,
            groups=example.phase_c_groups,
        )
        report.check()
        reports.append(report)

    payload = {
        "scenario": "ffn-allgather",
        "table": example.table.describe(),
        "phases": [report.describe() for report in reports],
        "all_complete": all(report.ok for report in reports),
    }
    _emit(payload, as_json=args.json)
    return 0


def _cmd_report_roofline(args: argparse.Namespace) -> int:
    _emit(
        {
            "clock_hz": AicoreSpec().clock_hz,
            "points": [point.describe() for point in roofline_table()],
        },
        as_json=args.json,
    )
    return 0


def _cmd_report_bandwidth(args: argparse.Namespace) -> int:
    _emit(
        {
            "links": {name: bandwidth.describe() for name, bandwidth in sorted(BANDWIDTHS.items())},
            "local_dram_vs_noc_link_ratio": round(local_dram_vs_noc_ratio(), 4),
            "ffn_two_phase": two_phase_allgather_bytes(
                row_count=8,
                phase_b_row_bytes=FFN_PHASE_B_ROW_BYTES,
                phase_c_row_bytes=FFN_PHASE_C_ROW_BYTES,
                row_member_count=8,
                col_member_count=4,
                node_count=CALENDAR_BASELINE.topology.node_count,
            ),
        },
        as_json=args.json,
    )
    return 0


def _cmd_report_overhead(args: argparse.Namespace) -> int:
    rows = []
    for name in sorted(PROFILES):
        topology = PROFILES[name].topology
        header = flit_header_cost(node_count=topology.node_count)
        rows.append(
            {
                "profile": name,
                "topology": topology.describe(),
                "header": header.describe(),
            }
        )
    _emit({"profiles": rows}, as_json=args.json)
    return 0


def _cmd_report_aicore(args: argparse.Namespace) -> int:
    _emit(AicoreSpec().describe(), as_json=args.json)
    return 0


def _cmd_report_dcache(args: argparse.Namespace) -> int:
    rows = []
    for layout in TableLayout:
        for key_count in (2, 8, 16):
            rows.append(
                dcache_budget(
                    key_count=key_count,
                    node_count=CALENDAR_BASELINE.topology.node_count,
                    layout=layout,
                ).describe()
            )
    _emit({"entries": rows}, as_json=args.json)
    return 0


def _cmd_check_object(args: argparse.Namespace) -> int:
    from wse_model.fixtures import clean_kernel_object

    if args.object is None and not args.example:
        raise WseModelError(
            "provide --object PATH with a kernel object manifest, or --example to "
            "check the built-in clean baseline"
        )
    if args.object is not None:
        object_file = load_object_file(args.object)
    else:
        object_file = clean_kernel_object(key_count=args.key_count, node_count=args.node_count)
    report = check_compiled_product(
        object_file, key_count=args.key_count, node_count=args.node_count
    )
    payload = {
        "object": object_file.describe(),
        "expected": {"key_count": args.key_count, "node_count": args.node_count},
        **report.describe(),
    }
    _emit(payload, as_json=args.json, text=report.format())
    return 0 if report.ok else 1


def _cmd_report_matmul(args: argparse.Namespace) -> int:
    timing = matmul_timing(
        MatmulShape(args.m, args.k, args.n),
        precision_name=args.precision,
        fetched_weight_bytes=args.fetched_weight_bytes,
    )
    _emit(timing.describe(), as_json=args.json)
    return 0


def _cmd_report_host(args: argparse.Namespace) -> int:
    bus = UbBus()
    mem = BatcherMem()
    _emit(
        {
            "ub_bus": bus.describe(),
            "local_dram_over_fabric_ratio": round(local_dram_over_fabric_ratio(), 4),
            "dispatch_paths": [path.describe() for path in dispatch_paths()],
            "batcher_mem": mem.describe(),
            "ffn_data_cache_refill": mem.data_refill(
                distinct_lines=20, requesters_per_line=4
            ).describe(),
        },
        as_json=args.json,
    )
    return 0


def _cmd_report_runtime(args: argparse.Namespace) -> int:
    _emit(
        {
            "tiers": [tier.value for tier in RuntimeTier],
            "chains": {chain.value: payload for chain, payload in dispatch_chains().items()},
            "block_id_paths": {
                "spr": LaunchConstraints.block_id_cost(from_spr=True),
                "argument": LaunchConstraints.block_id_cost(from_spr=False),
            },
            "steady_state_dispatch": (
                "activations plus a descriptor; no Calendar data, no per-core "
                "expansion, no run-time descriptors (whitepaper §12.2)"
            ),
        },
        as_json=args.json,
    )
    return 0


def _cmd_check_package(args: argparse.Namespace) -> int:
    from wse_model.compiler import build_package
    from wse_model.fixtures import clean_kernel_object

    example = ffn_example()
    package = build_package(
        table=example.table,
        calreg=example.calreg(),
        kernel_object=clean_kernel_object(),
        weight_shard_bytes=(2048, 2048),
        conflict_proofs={0: "fixture:no-conflict-key0", 1: "fixture:no-conflict-key1"},
    )
    concurrent = (
        {frozenset({definition.key_id for definition in example.table.ordered_keys()})}
        if args.concurrent
        else set()
    )
    report = package.validate(concurrent=concurrent)
    payload = {
        **package.describe(),
        **report.describe(),
        "concurrency_declared": sorted(sorted(pair) for pair in concurrent),
    }
    _emit(payload, as_json=args.json, text=report.format())
    return 0 if report.ok else 1


def _cmd_open_items(args: argparse.Namespace) -> int:
    wanted = None if args.status == "all" else Resolution(args.status)
    items = [
        {
            "id": item.id,
            "title": item.title,
            "source": item.source,
            "owner": item.owner,
            "resolution": item.resolution.value,
            "note": item.note,
            "decision": item.decision,
        }
        for item in OPEN_ITEMS.values()
        if wanted is None or item.resolution is wanted
    ]
    _emit(
        {
            "total": len(OPEN_ITEMS),
            "returned": len(items),
            "by_resolution": {
                status.value: sum(1 for item in OPEN_ITEMS.values() if item.resolution is status)
                for status in Resolution
            },
            "items": items,
        },
        as_json=args.json,
    )
    return 0


_HANDLERS = {
    "topology.show": _cmd_topology,
    "calendar.encode": _cmd_calendar_encode,
    "calendar.validate": _cmd_calendar_validate,
    "calendar.emit": _cmd_calendar_emit,
    "run.ffn-allgather": _cmd_run_ffn,
    "report.roofline": _cmd_report_roofline,
    "report.bandwidth": _cmd_report_bandwidth,
    "report.overhead": _cmd_report_overhead,
    "report.aicore": _cmd_report_aicore,
    "report.dcache": _cmd_report_dcache,
    "report.matmul": _cmd_report_matmul,
    "report.host": _cmd_report_host,
    "report.runtime": _cmd_report_runtime,
    "check.object": _cmd_check_object,
    "check.package": _cmd_check_package,
    "open-items": _cmd_open_items,
}


def main(argv: Sequence[str] | None = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)

    if args.command == "topology":
        key = f"topology.{args.topology_command}"
    elif args.command == "run":
        key = f"run.{args.run_command}"
    elif args.command == "report":
        key = f"report.{args.report_command}"
    elif args.command == "calendar":
        key = f"calendar.{args.calendar_command}"
    elif args.command == "check":
        key = f"check.{args.check_command}"
    else:
        key = args.command

    handler = _HANDLERS.get(key)
    if handler is None:  # pragma: no cover - argparse prevents this
        parser.error(f"unhandled command {key}")

    try:
        return handler(args)
    except WseModelError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":  # pragma: no cover
    raise SystemExit(main())
