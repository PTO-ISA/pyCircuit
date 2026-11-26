"""Analyze verified PTOAS SSA and lower a bounded AllGather to Calendar commands.

Run under PTOAS's Python with its package on PYTHONPATH. This is a compiler
analysis of address/control expressions, not a payload interpreter. Arbitrary
PTO kernels are rejected. Window bases remain symbolic throughout the analysis.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import operator
from collections import Counter
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path

from ptoas.mlir import ir
from ptoas.mlir.dialects import pto

from wse_model.calendar.route_bits import RouteBits
from wse_model.compiler.allgather import lower_calendar
from wse_model.noc.forward import simulate_multicast
from wse_model.topology import CALENDAR_BASELINE


@dataclass(frozen=True)
class Address:
    terms: dict[str, Fraction]
    offset: Fraction = Fraction(0)

    def __add__(self, other):
        if not isinstance(other, Address):
            return Address(self.terms, self.offset + other)
        names = self.terms.keys() | other.terms.keys()
        terms = {name: self.terms.get(name, 0) + other.terms.get(name, 0) for name in names}
        return Address(
            {name: value for name, value in terms.items() if value}, self.offset + other.offset
        )

    __radd__ = __add__

    def __mul__(self, scalar):
        return Address(
            {name: value * scalar for name, value in self.terms.items()}, self.offset * scalar
        )

    __rmul__ = __mul__

    def __sub__(self, other):
        return self + other * -1

    def __truediv__(self, scalar):
        return self * Fraction(1, scalar)


@dataclass(frozen=True)
class Pointer:
    address: Address
    item_bytes: int


@dataclass(frozen=True)
class Tile:
    shape: tuple[int, ...]
    valid: tuple[int, ...]


@dataclass(frozen=True)
class View:
    pointer: Pointer
    shape: tuple[int, ...]
    strides: tuple[int, ...]

    def contiguous_bytes(self):
        stride = 1
        for size, actual in reversed(list(zip(self.shape, self.strides, strict=True))):
            if size != 1 and actual != stride:
                raise ValueError("noncontiguous transfer is outside this profile")
            stride *= size
        return math.prod(self.shape) * self.pointer.item_bytes


def symbol(name):
    return Address({name: Fraction(1)})


def window(pointer, ranks, *, signal=False):
    addr = pointer.address
    for rank in range(ranks):
        prefix = "signal" if signal else "data"
        wanted = {f"{prefix}-window{rank}": Fraction(1)}
        if addr.terms == wanted and addr.offset.denominator == 1:
            return rank, int(addr.offset)
    raise ValueError(f"unproved window address: {addr}")


class KernelAnalysis:
    def __init__(self, rank, ranks):
        self.rank, self.ranks = rank, ranks
        self.values = {}
        self.events = []
        self.flags = set()
        self.op_names = set()

    def block(self, block):
        for op in block.operations:
            name = op.operation.name
            self.op_names.add(name)
            args = [self.values[v] for v in op.operands]
            attrs = {a.name: a.attr for a in op.attributes}
            outputs = []
            if name == "arith.constant":
                outputs = [int(attrs["value"].value)]
            elif name in ("arith.addi", "arith.subi", "arith.maxsi"):
                fn = {
                    "arith.addi": operator.add,
                    "arith.subi": operator.sub,
                    "arith.maxsi": max,
                }[name]
                outputs = [fn(*args)]
            elif name == "arith.divsi":
                if not isinstance(args[1], int) or args[1] <= 0:
                    raise ValueError("address division requires a positive constant divisor")
                if isinstance(args[0], Address):
                    # CommContext windows are byte addresses aligned to 64 B;
                    # this profile converts their deltas to 4-byte elements.
                    if args[1] != 4 or args[0].offset % 4:
                        raise ValueError("unproved window alignment")
                    outputs = [args[0] / 4]
                else:
                    outputs = [abs(args[0]) // args[1] * (-1 if args[0] < 0 else 1)]
            elif name == "arith.shrui":
                width = ir.IntegerType(op.operands[0].type).width
                if not isinstance(args[0], int) or args[0] < 0 or not 0 <= args[1] < width:
                    raise ValueError("logical right shift outside the nonnegative compiler subset")
                outputs = [args[0] >> args[1]]
            elif name == "arith.trunci":
                if str(op.operands[0].type) != "i64" or str(op.results[0].type) != "i32":
                    raise ValueError("only the CommContext i64-to-i32 extraction is admitted")
                width = ir.IntegerType(op.results[0].type).width
                outputs = [args[0] & ((1 << width) - 1)]
            elif name == "arith.index_cast":
                source_type = str(op.operands[0].type)
                if source_type not in ("i32", "i64") or str(op.results[0].type) != "index":
                    raise ValueError("only widening i32/i64-to-index casts are admitted")
                if isinstance(args[0], Address) and source_type != "i64":
                    raise ValueError("symbolic address cast must preserve 64 bits")
                outputs = args
            elif name == "arith.cmpi":
                pred = int(attrs["predicate"].value)
                comparisons = (
                    operator.eq,
                    operator.ne,
                    operator.lt,
                    operator.le,
                    operator.gt,
                    operator.ge,
                )
                if pred >= len(comparisons):
                    raise ValueError("unsupported integer comparison")
                outputs = [comparisons[pred](*args)]
            elif name == "pto.load":
                context, slot = args
                if context not in ("data-context", "signal-context"):
                    raise ValueError("only CommContext scalar loads are admitted")
                if slot == 0:
                    outputs = [(self.ranks << 32) | self.rank]
                elif 2 <= slot < 2 + self.ranks:
                    prefix = "data" if context == "data-context" else "signal"
                    outputs = [symbol(f"{prefix}-window{slot - 2}")]
                else:
                    raise ValueError("unknown CommContext ABI field")
            elif name == "pto.addptr":
                pointer, offset = args
                outputs = [
                    Pointer(pointer.address + offset * pointer.item_bytes, pointer.item_bytes)
                ]
            elif name == "pto.make_tensor_view":
                rank = (len(args) - 1) // 2
                outputs = [View(args[0], tuple(args[1 : 1 + rank]), tuple(args[1 + rank :]))]
            elif name == "pto.partition_view":
                view = args[0]
                rank = len(view.shape)
                offsets, sizes = args[1 : 1 + rank], args[1 + rank :]
                if len(sizes) != rank or any(
                    o < 0 or s <= 0 or o + s > d
                    for o, s, d in zip(offsets, sizes, view.shape, strict=True)
                ):
                    raise ValueError("partition is outside its tensor view")
                delta = sum(o * s for o, s in zip(offsets, view.strides, strict=True))
                pointer = Pointer(
                    view.pointer.address + delta * view.pointer.item_bytes, view.pointer.item_bytes
                )
                outputs = [View(pointer, tuple(sizes), view.strides)]
            elif name == "pto.alloc_tile":
                tile_type = pto.TileBufType(op.results[0].type)
                if str(tile_type.element_type) != "f32" or len(args) != 3:
                    raise ValueError("expected explicitly sized FP32 staging tile")
                outputs = [Tile(tuple(tile_type.shape), tuple(args[1:]))]
            elif name == "scf.for":
                if len(args) != 3 or len(op.results):
                    raise ValueError("loop-carried values are outside this profile")
                lo, hi, step = args
                if step != 1 or not 0 <= lo <= hi <= 64:
                    raise ValueError("unbounded communication loop")
                body = op.regions[0].blocks[0]
                for index in range(lo, hi, step):
                    self.values[body.arguments[0]] = index
                    self.block(body)
            elif name == "scf.if":
                if len(op.results):
                    raise ValueError("result-bearing conditional is outside this profile")
                selected = 0 if args[0] else 1
                if selected < len(op.regions) and len(op.regions[selected].blocks):
                    self.block(op.regions[selected].blocks[0])
            elif name in ("scf.yield", "func.return"):
                if args:
                    raise ValueError("unexpected return values")
            elif name in ("pto.set_flag", "pto.wait_flag"):
                key = tuple(str(attrs[field]) for field in ("src_pipe", "dst_pipe", "event_id"))
                self.events.append((name, *key))
                if name == "pto.set_flag":
                    if key in self.flags:
                        raise ValueError("sync event set twice without a wait")
                    self.flags.add(key)
                else:
                    if key not in self.flags:
                        raise ValueError("sync wait has no producer")
                    self.flags.remove(key)
            elif name == "pto.barrier":
                if str(attrs["pipe"]) not in (
                    "#pto.pipe<PIPE_ALL>",
                    "#pto.pipe<PIPE_MTE2>",
                    "#pto.pipe<PIPE_MTE3>",
                ):
                    raise ValueError("unsupported barrier")
                self.events.append(("barrier", str(attrs["pipe"])))
            elif name == "pto.cmo.cacheinvalid":
                if str(attrs["space"]) != "#pto.address_space<gm>":
                    raise ValueError("unsupported cache domain")
                if args:
                    target, offset = window(args[0].pointer, self.ranks)
                    self.events.append(("invalidate", target, offset, args[0].contiguous_bytes()))
                else:
                    self.events.append(("invalidate-all",))
            elif name == "pto.fence.barrier_all":
                if str(attrs["scope"]) != "#pto.fence_scope<gm>":
                    raise ValueError("unsupported fence domain")
                self.events.append(("fence",))
            elif name == "pto.comm.tput":
                if str(attrs["atomicType"]) != "#pto<atomic_type atomic_none>":
                    raise ValueError("AllGather must copy, not reduce")
                dst, src, *tiles = args
                if (
                    len(tiles) != 1
                    or not isinstance(tiles[0], Tile)
                    or src.pointer.address.terms != {"source": Fraction(1)}
                ):
                    raise ValueError("unsupported TPUT operands")
                if tiles[0].shape != src.shape or tiles[0].valid != src.shape:
                    raise ValueError(
                        "staging tile shape/valid shape does not cover the exact transfer"
                    )
                if src.pointer.address.offset.denominator != 1:
                    raise ValueError("fractional source byte offset")
                nbytes = src.contiguous_bytes()
                if dst.contiguous_bytes() != nbytes:
                    raise ValueError("TPUT size mismatch")
                target, offset = window(dst.pointer, self.ranks)
                self.events.append(("put", target, int(src.pointer.address.offset), offset, nbytes))
            elif name in ("pto.comm.tnotify", "pto.comm.twait"):
                view, value = args
                if view.contiguous_bytes() != 4:
                    raise ValueError("signal must be one i32")
                target, offset = window(view.pointer, self.ranks, signal=True)
                if name == "pto.comm.tnotify":
                    if str(attrs["notifyOp"]) != "#pto<notify_op atomic_add>":
                        raise ValueError("unsupported signal update")
                    self.events.append(("notify", target, offset, value))
                else:
                    if str(attrs["cmp"]) != "#pto<wait_cmp ge>":
                        raise ValueError("unsupported wait comparison")
                    self.events.append(("wait", target, offset, value))
            else:
                raise ValueError(f"operation outside the AllGather compiler subset: {name}")
            if len(outputs) != len(op.results):
                raise ValueError(f"unexpected results for {name}")
            for value, result in zip(outputs, op.results, strict=True):
                if type(value) is int and name.startswith("arith."):
                    width = (
                        64
                        if ir.IndexType.isinstance(result.type)
                        else ir.IntegerType(result.type).width
                    )
                    if not -(1 << (width - 1)) <= value < (1 << (width - 1)):
                        raise ValueError(
                            f"integer range/overflow outside the compiler subset: {name}"
                        )
            self.values.update(zip(op.results, outputs, strict=True))


def analyze(text, phase):
    ranks, words = phase["ranks"], phase["words"]
    transfers, operations = [], set()
    with ir.Context() as context:
        pto.register_dialect(context)
        module = ir.Module.parse(text)
        module.operation.verify()
        if str(module.operation.attributes["pto.target_arch"]) != '"a3"':
            raise ValueError("expected PTOAS a3 output")
        functions = list(module.body.operations)
        if len(functions) != 1 or functions[0].operation.name != "func.func":
            raise ValueError("expected one kernel")
        function = functions[0]
        if str(function.attributes["sym_name"]) != json.dumps(phase["kernel"]):
            raise ValueError("kernel identity mismatch")
        body = function.regions[0].blocks[0]
        types = [str(v.type) for v in body.arguments]
        if types != [f"!pto.ptr<{t}, gm>" for t in ("f32", "f32", "i32", "i64", "i64")]:
            raise ValueError("unsupported kernel ABI")
        for rank in range(ranks):
            analysis = KernelAnalysis(rank, ranks)
            actuals = [
                Pointer(symbol("source"), 4),
                Pointer(symbol(f"data-window{rank}"), 4),
                Pointer(symbol(f"signal-window{rank}"), 4),
                "data-context",
                "signal-context",
            ]
            analysis.values.update(zip(body.arguments, actuals, strict=True))
            analysis.block(body)

            def flag(action, src, dst):
                return (
                    f"pto.{action}_flag",
                    f"#pto.pipe<PIPE_MTE{src}>",
                    f"#pto.pipe<PIPE_MTE{dst}>",
                    "#pto.event<EVENT_ID1>",
                )

            barrier_all = ("barrier", "#pto.pipe<PIPE_ALL>")
            expected = [flag("set", 3, 2), flag("set", 2, 3)]
            for peer in range(ranks):
                expected.extend(
                    [
                        barrier_all,
                        flag("wait", 3, 2),
                        ("barrier", "#pto.pipe<PIPE_MTE2>"),
                        flag("wait", 2, 3),
                        ("put", peer, 0, rank * words * 4, words * 4),
                        flag("set", 2, 3),
                        flag("set", 3, 2),
                        barrier_all,
                        ("invalidate", peer, rank * words * 4, words * 4),
                        ("fence",),
                    ]
                )
            peers = [peer for peer in range(ranks) if peer != rank]
            expected.extend(("notify", peer, rank * 4, 1) for peer in peers)
            expected.extend(("wait", rank, peer * 4, 1) for peer in peers)
            expected.append(("invalidate-all",))
            expected.extend(("notify", rank, peer * 4, -1) for peer in peers)
            expected.extend([flag("wait", 3, 2), flag("wait", 2, 3), barrier_all])
            if analysis.events != expected or analysis.flags:
                raise ValueError(
                    f"rank {rank}: transfer/synchronization trace differs from AllGather: {analysis.events}"
                )
            operations.update(analysis.op_names)
            for event in analysis.events:
                if event[0] == "put":
                    transfers.append(
                        {
                            "source_rank": rank,
                            "target_rank": event[1],
                            "source_offset": event[2],
                            "target_offset": event[3],
                            "nbytes": event[4],
                        }
                    )
    return transfers, sorted(operations)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler-dir", type=Path, required=True)
    args = parser.parse_args()
    directory = args.compiler_dir
    program = json.loads((directory / "program.json").read_text())
    transfers, analysis = [], []
    for phase in program["phases"]:
        raw = (directory / (phase["name"] + ".pto")).read_bytes()
        if hashlib.sha256(raw).hexdigest() != phase["raw_pto_sha256"]:
            raise ValueError("raw PTO digest does not match its program")
        lowered = (directory / (phase["name"] + ".lowered.pto")).read_bytes()
        copies, operations = analyze(lowered.decode(), phase)
        transfers.append(copies)
        analysis.append(
            {
                "phase": phase["name"],
                "transfers": copies,
                "operations": operations,
                "lowered_pto_sha256": hashlib.sha256(lowered).hexdigest(),
            }
        )
    descriptors = lower_calendar(program, transfers)
    binary = b"".join(descriptor.pack() for descriptor in descriptors)
    (directory / "commands.bin").write_bytes(binary)
    # The 48-byte fragments remain algorithm-side timing inputs. Only the
    # separate, algorithm-neutral 64-byte instruction stream drives hardware.
    from wse_model.programs.allgather import compile_allgather

    machine_program = compile_allgather(program, descriptors)
    machine_binary = machine_program.pack()
    (directory / "machine.bin").write_bytes(machine_binary)
    (directory / "machine-program.json").write_text(
        json.dumps(machine_program.manifest(), indent=2) + "\n"
    )
    receipt = {
        "status": "PASS",
        "analysis": analysis,
        "descriptors": len(descriptors),
        "commands_sha256": hashlib.sha256(binary).hexdigest(),
        "machine_program_sha256": hashlib.sha256(machine_binary).hexdigest(),
        "machine_instructions": len(machine_program.commands),
    }
    # Static workload accounting: payload byte-hops, with no invented link latency.
    traffic = []
    for phase_id, phase in enumerate(program["phases"]):
        links = Counter()
        source_bytes = remote_bytes = 0
        for descriptor in descriptors:
            if descriptor.phase != phase_id:
                continue
            trace = simulate_multicast(
                CALENDAR_BASELINE.topology,
                RouteBits.from_int(descriptor.route, node_count=40),
                source=descriptor.source,
            )
            source_bytes += descriptor.nbytes
            remote_bytes += descriptor.nbytes * len(trace.land_nodes)
            for link, traversals in trace.link_load().items():
                links[link] += descriptor.nbytes * traversals
        traffic.append(
            {
                "phase": phase["name"],
                "source_read_bytes": source_bytes,
                "self_copy_bytes": source_bytes,
                "remote_delivery_bytes": remote_bytes,
                "destination_write_bytes": source_bytes + remote_bytes,
                "link_payload_byte_hops": sum(links.values()),
                "links": [
                    {"nodes": list(link), "payload_bytes_both_directions": count}
                    for link, count in sorted(links.items())
                ],
            }
        )
    receipt["static_traffic"] = {
        "basis": "compiled descriptors and Calendar forwarding; excludes protocol overhead",
        "phases": traffic,
    }
    (directory / "lowering.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(f"Validated {len(descriptors)} Calendar fragments from PTOAS SSA")


if __name__ == "__main__":
    main()
