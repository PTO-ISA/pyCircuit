"""Registry of design values the sources leave open.

Whitepaper Appendix A (`Q1`-`Q12`) and Calendar §6.3 (`S-1`-`S-7`,
`C-1`-`C-12`) list values and choices that the design documents explicitly do
not fix. This module records them so that the model can reference an item by id
instead of smuggling a plausible default into the code.

The rule (see ``AGENTS.md``): a parameter that stands in for an open item must
carry the item id and a :class:`Resolution`, and code that would produce a
result with a materially different answer under the other reading must call
:func:`require_resolved` first.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from wse_model.errors import OpenItemError

__all__ = [
    "OPEN_ITEMS",
    "OpenItem",
    "Resolution",
    "ResolvedItem",
    "open_item",
    "require_resolved",
]


class Resolution(str, Enum):
    """How the model currently treats an open item."""

    OPEN = "open"  #: no reading chosen; consumers must not depend on a value
    ASSUMED = "assumed"  #: a reading is adopted from a stated assumption
    RESOLVED = "resolved"  #: decided by a decision record in docs/decisions/


@dataclass(frozen=True)
class OpenItem:
    """A design value or choice that the design sources leave open."""

    id: str
    title: str
    source: str
    owner: str
    resolution: Resolution = Resolution.OPEN
    #: The adopted reading, or the reason the item still blocks.
    note: str = ""
    #: Decision record that resolves it, when one exists.
    decision: str | None = None


_WHITEPAPER = "whitepaper-appendix-a"
_CALENDAR = "calendar-6.3"

OPEN_ITEMS: dict[str, OpenItem] = {}


def _register(item: OpenItem) -> None:
    if item.id in OPEN_ITEMS:
        raise RuntimeError(f"duplicate open item id: {item.id}")
    OPEN_ITEMS[item.id] = item


for _item in (
    OpenItem(
        "Q1",
        "NoC node count: 40 (Calendar baseline, 5x8) or 48 (whitepaper hardware, 6x8)",
        _WHITEPAPER,
        "hardware",
        Resolution.OPEN,
        "Both readings are modelled: routeBits is 80 bit at 40 nodes and 96 bit "
        "at 48 nodes, and the flit header grows from about 12 B to about 14 B. "
        "No single value is authoritative yet.",
    ),
    OpenItem(
        "Q2",
        "Reticle / die count per WSE-Lite",
        _WHITEPAPER,
        "hardware",
        Resolution.OPEN,
        "The model assumes a single reticle with an intra-reticle synchronous NoC.",
    ),
    OpenItem(
        "Q3",
        "Batcher specification: Batcher.mem capacity and bandwidth, aggregate "
        "AICORE-side bandwidth, cores per Batcher, parallel dispatch channels",
        _WHITEPAPER,
        "hardware",
        Resolution.OPEN,
        "Batcher timing parameters are declared inputs, never constants.",
    ),
    OpenItem(
        "Q4",
        "Position of DDR relative to local DRAM",
        _WHITEPAPER,
        "hardware",
        Resolution.OPEN,
        "The model keeps the instruction/constant path (DDR -> Batcher.mem -> "
        "I$/D$) separate from the payload path (local DRAM -> L1 -> L0).",
    ),
    OpenItem(
        "Q5",
        "Target model dimensions: H, I, expert count, top-k, head count, weight "
        "precision for DeepSeek-V4 Pro",
        _WHITEPAPER,
        "workload",
        Resolution.OPEN,
        "No concrete partition or latency figure is produced without these.",
    ),
    OpenItem(
        "Q6",
        "MoE load-imbalance policy under invariant E1",
        _WHITEPAPER,
        "workload",
        Resolution.OPEN,
        "Member cores must not conditionally skip a round; whether the mechanism "
        "is a fixed capacity factor with zero sends or something else is open.",
    ),
    OpenItem(
        "Q7",
        "Collective arena placement: UB or L1 (Calendar S-2)",
        _WHITEPAPER,
        "hardware",
        Resolution.OPEN,
        "Determines the tile address-space qualifier and the double-buffer budget.",
    ),
    OpenItem(
        "Q8",
        "Weight layout in local DRAM and the prefetch path to L0B",
        _WHITEPAPER,
        "hardware",
        Resolution.OPEN,
        "Determines whether MTE stages local DRAM through L1.",
    ),
    OpenItem(
        "Q9",
        "Davinci <-> WSE-Lite interface form and round-trip latency",
        _WHITEPAPER,
        "system",
        Resolution.OPEN,
        "Round-trip latency is a declared input to the latency model.",
    ),
    OpenItem(
        "Q10",
        "Whether the two WSE-Lite instances are identically configured",
        _WHITEPAPER,
        "system",
        Resolution.OPEN,
    ),
    OpenItem(
        "Q11",
        "Production weight precision and mixed-precision acceptability",
        _WHITEPAPER,
        "workload",
        Resolution.OPEN,
    ),
    OpenItem(
        "Q12",
        "Degradation path for a faulty core or link (Calendar C-11)",
        _WHITEPAPER,
        "system",
        Resolution.OPEN,
        "Drives whether the .rodata table stays an A-class value or degrades to "
        "a load-time buffer (Calendar §3.11).",
    ),
    OpenItem(
        "S-1",
        "Freeze of operand widths and units for expVal / capacity / epoch, and "
        "the reused operand slots",
        _CALENDAR,
        "hardware-isa",
        Resolution.OPEN,
        "The model uses uint32 for all three, following the Calendar baseline.",
    ),
    OpenItem(
        "S-2",
        "Arena address space and naming (ubuf vs cbuf) for the send and receive facades",
        _CALENDAR,
        "hardware+software",
        Resolution.OPEN,
        "Duplicate of whitepaper Q7.",
    ),
    OpenItem(
        "S-3",
        "Machine encoding of the symmetric address",
        _CALENDAR,
        "hardware-isa",
        Resolution.OPEN,
        "Affects lowering only; the PTO interface is unchanged.",
    ),
    OpenItem(
        "S-4",
        "Machine mnemonics for the send and receive instructions",
        _CALENDAR,
        "isa-review",
        Resolution.OPEN,
        "Documentation only.",
    ),
    OpenItem(
        "S-5",
        "Two small semantics: sid handling in the Calendar direction, and the "
        "ordering meaning of the report instruction's pipe operand",
        _CALENDAR,
        "hardware-isa",
        Resolution.OPEN,
        "Determines whether a barrier before Step4 can be elided.",
    ),
    OpenItem(
        "S-6",
        "Packet / segment identity de-duplication, including multicast copies per land point",
        _CALENDAR,
        "hardware",
        Resolution.OPEN,
        "No software-side substitute: without it expVal can be met early.",
    ),
    OpenItem(
        "S-7",
        "Frozen shape of the three-state mock",
        _CALENDAR,
        "software",
        Resolution.OPEN,
    ),
    OpenItem(
        "C-1",
        "Send-side 80 bit operand encoding: scheme A (two GPR slots or one even/odd "
        "pair plus a 6 bit immediate) vs scheme B (latch into an SPR)",
        _CALENDAR,
        "hardware-isa",
        Resolution.OPEN,
        "Scheme A adds 2 data-plane instructions; scheme B adds 3 plus a fence. "
        "The call site is identical either way.",
    ),
    OpenItem(
        "C-2",
        "Flit header carriage: per-flit copy (baseline) vs per-packet header with "
        "per-packet context in the NoC",
        _CALENDAR,
        "noc",
        Resolution.ASSUMED,
        "The model implements the per-flit baseline, which is what the 12 B / 19% "
        "overhead figure describes.",
        decision="0001",
    ),
    OpenItem(
        "C-3",
        "Reduction element type field in the flit header",
        _CALENDAR,
        "noc+isa",
        Resolution.OPEN,
        "Must be fixed before the header format freezes. AllGather-only is not "
        "blocked, because redOp is dormant there.",
    ),
    OpenItem(
        "C-4",
        "Global synchronisation accuracy of the internal NoC / BSP time base",
        _CALENDAR,
        "noc+bsp",
        Resolution.OPEN,
        "Physical realizability of invariant O2; no software interface change.",
    ),
    OpenItem(
        "C-5",
        "epochTag width and wraparound policy, including the cross-launch counter initial value",
        _CALENDAR,
        "hardware+compiler",
        Resolution.ASSUMED,
        "The model follows the Calendar baseline: per-launch counter starting at 1, "
        "with launch-boundary drain. epochTag is a declared 8-16 bit parameter.",
        decision="0001",
    ),
    OpenItem(
        "C-6",
        "epochTag carriage: implicit context established by the receive command vs "
        "an explicit send operand",
        _CALENDAR,
        "hardware-isa",
        Resolution.ASSUMED,
        "The model follows the Calendar baseline (implicit context).",
        decision="0001",
    ),
    OpenItem(
        "C-7",
        "blockId acquisition: block_idx SPR vs kernel argument",
        _CALENDAR,
        "hardware+runtime",
        Resolution.ASSUMED,
        "The model assumes the SPR path and represents the argument fallback only "
        "as a load-count and value-class difference "
        "(LaunchConstraints.block_id_cost); it does not simulate the extra load or "
        "its second-level dependency.",
        decision="0001",
    ),
    OpenItem(
        "C-8",
        "Static table shape for per-row routing in ReduceScatter / Scatter",
        _CALENDAR,
        "compiler",
        Resolution.OPEN,
        "A third table dimension multiplies both DDR footprint and per-core "
        "D-cache residency by shardCount.",
    ),
    OpenItem(
        "C-9",
        "Static table layout: key-major vs node-major",
        _CALENDAR,
        "compiler",
        Resolution.ASSUMED,
        "The model defaults to the key-major baseline and also models the "
        "node-major variant, because the 4x D-cache reduction is free.",
        decision="0005",
    ),
    OpenItem(
        "C-10",
        "Complete definition of reduction semantics under {P, L}",
        _CALENDAR,
        "noc",
        Resolution.OPEN,
        "Gates opening the Reduce / AllReduce / ReduceScatter branches. It also "
        "covers the redOp numeric codes: Calendar §2.4 lists None/Sum/Max/Min/Prod "
        "but fixes neither the codes nor their order, so the model assigns them in "
        "listing order and pins that choice with a golden test. Every use is inside "
        "a reduction path, which C-10 already blocks.",
    ),
    OpenItem(
        "C-11",
        "Degradation path when a faulty core or link makes each instance's topology different",
        _CALENDAR,
        "system+compiler",
        Resolution.OPEN,
        "Duplicate of whitepaper Q12.",
    ),
    OpenItem(
        "C-12",
        "D-cache strategy for the .rodata table, and whether keyCount <= 16 suffices",
        _CALENDAR,
        "compiler",
        Resolution.OPEN,
    ),
):
    _register(_item)


@dataclass(frozen=True)
class ResolvedItem:
    """A reading adopted for an open item, carrying its provenance."""

    item: OpenItem
    value: object
    rationale: str

    def __post_init__(self) -> None:
        if self.item.resolution is Resolution.OPEN:
            raise OpenItemError(
                f"{self.item.id} is still open and has no adopted reading; "
                "a value may not be attached to it"
            )


def open_item(item_id: str) -> OpenItem:
    """Return the registered open item, or raise :class:`KeyError`."""
    try:
        return OPEN_ITEMS[item_id]
    except KeyError:  # pragma: no cover - programming error
        raise KeyError(f"unknown open item id: {item_id}") from None


def require_resolved(item_id: str, *, consumer: str) -> OpenItem:
    """Return ``item_id``, raising unless the model has adopted a reading.

    Call this from any code path whose numerical or structural result changes
    materially between the readings of an open item.
    """
    item = open_item(item_id)
    if item.resolution is Resolution.OPEN:
        raise OpenItemError(
            f"{consumer} depends on open design item {item.id} ({item.title}), "
            f"which the design sources leave unresolved ({item.source}, owner "
            f"{item.owner}). Choose a reading in docs/decisions/ and update "
            f"wse_model.open_items before relying on this result."
        )
    return item
