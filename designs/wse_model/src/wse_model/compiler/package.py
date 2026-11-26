"""The compiled deployment package and the compile-time concurrency check.

Whitepaper §13.3 lists what leaves the compiler:

* ``.text`` machine code,
* the ``routeBits`` static table in ``.rodata``, 64 B aligned,
* the ``CalReg`` timeslot mirror as a separate artifact,
* the three version numbers,
* the model weights, sliced per core.

Calendar §2.7.1 adds two timing guarantees that only the NoC algorithm can
judge, and Calendar §2.7.3 step 4 adds the per-call-site immediates. This module
assembles those into one object so a package can be validated as a whole instead
of in pieces:

* the two artifacts come from **one** algorithm call, so their versions must
  agree;
* every collective sharing an ``opcode`` shares one ``CalReg`` entry, so two of
  them must not be in flight at once and the algorithm must have returned a
  ``conflictProof``;
* the call site gets ``keyId`` / ``opcode`` / ``redOp`` / ``routeVersion`` plus
  ``expVal`` / ``capacity`` and the compiled geometry — and nothing else, in
  particular no DMA or activation pseudo-operation.
"""

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass, field
from typing import Any

from wse_model.calendar.collective import OPCODE_RESERVED, Collective, RedOp
from wse_model.calendar.key import CalendarKeyRef
from wse_model.calendar.table import CalendarRouteTable
from wse_model.calendar.validate import ValidationReport
from wse_model.compiler.selfcheck import ObjectFile, check_compiled_product
from wse_model.errors import CalendarError, WseModelError
from wse_model.host.runtime import VersionSet
from wse_model.noc.calreg import CalRegImage

__all__ = [
    "CallSiteImmediate",
    "DeploymentPackage",
    "build_package",
    "check_opcode_domains",
    "deployment_artifacts",
]

#: The compiler's artifact list, from whitepaper §13.3.
_ARTIFACTS: tuple[tuple[str, str, str], ...] = (
    ("kernel machine code", ".text", "DDR code segment"),
    ("routeBits static table", ".rodata, 64 B aligned", "DDR data segment"),
    ("CalReg mirror", "separate binary section", "NoC node timeslot registers"),
    ("three version numbers", "metadata", "checked at load time"),
    ("model weights", "per-core slices", "each core's local DRAM"),
)


def deployment_artifacts() -> tuple[dict[str, str], ...]:
    """The five artifacts the compiler emits (whitepaper §13.3)."""
    return tuple(
        {"artifact": name, "form": form, "destination": destination}
        for name, form, destination in _ARTIFACTS
    )


@dataclass(frozen=True)
class CallSiteImmediate:
    """The constants one Calendar call site is compiled down to.

    Calendar §2.7.3 step 4: ``CalendarKeyRef{keyId, opcode, redOp,
    routeVersion}`` plus a conservation-checked ``expVal``, ``capacity``, and
    ``selfOff``. The ``keyId``/``opcode`` pair is indivisible (invariant O1).
    """

    key_id: int
    opcode: int
    red_op: int
    route_version: int
    exp_val: int
    capacity: int
    self_off_stride: int
    n_burst: int

    def __post_init__(self) -> None:
        if self.opcode in OPCODE_RESERVED:
            raise CalendarError(
                f"keyId {self.key_id}: opcode {self.opcode} is reserved and cannot "
                "appear in a call-site immediate (Calendar §2.3)"
            )

    @property
    def key_ref(self) -> CalendarKeyRef:
        return CalendarKeyRef(
            key_id=self.key_id,
            opcode=Collective(self.opcode),
            red_op=RedOp(self.red_op),
            route_version=self.route_version,
        )

    def describe(self) -> dict[str, int]:
        return {
            "keyId": self.key_id,
            "opcode": self.opcode,
            "redOp": self.red_op,
            "routeVersion": self.route_version,
            "expVal": self.exp_val,
            "capacity": self.capacity,
            "selfOffStride": self.self_off_stride,
            "nBurst": self.n_burst,
        }


@dataclass
class DeploymentPackage:
    """Everything the loader needs, plus enough to check itself."""

    table: CalendarRouteTable
    calreg: CalRegImage
    kernel_object: ObjectFile | None = None
    #: Byte size of each per-core weight shard, in node order where known.
    weight_shard_bytes: tuple[int, ...] = ()
    #: ``keyId`` -> the NoC algorithm's proof for that opcode domain.
    conflict_proofs: dict[int, str] = field(default_factory=dict)

    @property
    def versions(self) -> VersionSet:
        """The package's version triple, taken from the table and the CalReg mirror."""
        return VersionSet(
            topology_version=self.table.topology_version,
            calendar_version=self.calreg.calendar_version,
            route_version=self.table.route_version,
        )

    @property
    def key_count(self) -> int:
        return self.table.key_count

    @property
    def node_count(self) -> int:
        return self.table.topology.node_count

    @property
    def rodata_bytes(self) -> int:
        """Padded ``.rodata`` segment length, which must be 64 B aligned."""
        return len(self.table.to_bytes())

    def call_site_immediates(self) -> tuple[CallSiteImmediate, ...]:
        """One immediate bundle per logical identity."""
        immediates: list[CallSiteImmediate] = []
        for definition in self.table.ordered_keys():
            key_ref = definition.key_ref(route_version=self.table.route_version)
            geometry = definition.geometry
            land_counts = definition.land_counts()
            member = min(definition.member_nodes) if definition.member_nodes else 0
            land_count = land_counts.get(member, 0)
            exp_val = geometry.exp_val(land_count) if geometry else 0
            capacity = geometry.capacity if geometry else 0
            row_bytes = geometry.row_bytes if geometry else 0
            immediates.append(
                CallSiteImmediate(
                    key_id=definition.key_id,
                    opcode=key_ref.opcode_value,
                    red_op=int(key_ref.red_op),
                    route_version=self.table.route_version,
                    exp_val=exp_val,
                    capacity=capacity,
                    self_off_stride=row_bytes,
                    n_burst=geometry.n_burst if geometry else 0,
                )
            )
        return tuple(immediates)

    def validate(self, *, concurrent: Iterable[frozenset[int]] = ()) -> ValidationReport:
        """Validate the package as a whole.

        Runs the route-table checks, the F1/F2/F3 product checks, the
        artifact-version agreement, and the same-``opcode`` concurrency rule.
        """
        report = ValidationReport()
        report.merge(self.table.validate())

        if self.kernel_object is not None:
            report.ran("f1-f2-f3-product-checks")
            report.merge(
                check_compiled_product(
                    self.kernel_object,
                    key_count=self.key_count,
                    node_count=self.node_count,
                )
            )

        report.ran("artifact-version-agreement")
        if self.calreg.calendar_version != self.table.calendar_version:
            report.error(
                "V-CALREG-VERSION",
                "the CalReg mirror reports calendarVersion "
                f"{self.calreg.calendar_version} but the route table reports "
                f"{self.table.calendar_version}; the two come from one algorithm "
                "call and must agree (Calendar §2.7, whitepaper §10.3)",
            )
        for opcode in self.calreg.opcodes:
            if opcode not in {definition.opcode for definition in self.table.ordered_keys()}:
                report.warn(
                    "V-CALREG-ORPHAN",
                    f"the CalReg mirror installs opcode {opcode}, which no logical "
                    "identity in this package uses",
                )

        report.merge(
            check_opcode_domains(self.table, concurrent=concurrent, proofs=self.conflict_proofs)
        )

        report.ran("weight-shard-pages")
        for index, shard in enumerate(self.weight_shard_bytes):
            if shard <= 0 or shard % 2048 != 0:
                report.error(
                    "V-WEIGHT-ALIGN",
                    f"weight shard {index} is {shard} B; weights land in local DRAM "
                    "2 KB-page aligned (whitepaper §3.4, §14.1)",
                )

        report.ran("rodata-alignment")
        if self.rodata_bytes % 64 != 0:
            report.error(
                "V-RODATA-ALIGN",
                f"the emitted segment is {self.rodata_bytes} B, not a multiple of "
                "64; prohibition F3 requires the table's segment to be alignas(64)",
            )
        return report

    def describe(self) -> dict[str, object]:
        return {
            "table": self.table.describe(),
            "calreg": self.calreg.describe(),
            "versions": self.versions.describe(),
            "rodata_bytes": self.rodata_bytes,
            "key_count": self.key_count,
            "node_count": self.node_count,
            "weight_shards": list(self.weight_shard_bytes),
            "conflict_proofs": dict(sorted(self.conflict_proofs.items())),
            "artifacts": list(deployment_artifacts()),
            "call_site_immediates": [
                immediate.describe() for immediate in self.call_site_immediates()
            ],
            "kernel_object": (self.kernel_object.describe() if self.kernel_object else None),
        }

    def to_dict(self) -> dict[str, Any]:
        return {
            "schema": "wse-model/deployment-package/1",
            **self.describe(),
        }


def check_opcode_domains(
    table: CalendarRouteTable,
    *,
    concurrent: Iterable[frozenset[int]] = (),
    proofs: dict[int, str] | None = None,
) -> ValidationReport:
    """Check the two timing guarantees Calendar §2.7.1 assigns to the NoC algorithm.

    The model does not re-derive them — that would mean rewriting the scheduling
    algorithm in software, which the design forbids. What it can check is the
    structural precondition and the presence of the algorithm's proof:

    * two logical identities that share an ``opcode`` must not be concurrent,
      because they share one ``CalReg[opcode]`` entry *and* one alignment
      semaphore domain; and
    * every shared-``opcode`` group must carry a ``conflictProof``.

    Reserved opcodes need no check here: :class:`~wse_model.calendar.key.RouteKey`
    refuses to carry one, so a table loaded from JSON with an ``INVALID`` or
    ``EXTENSION`` collective fails closed at construction rather than arriving
    here.
    """
    report = ValidationReport()
    report.ran("same-opcode-not-concurrent")

    definition_by_key = {definition.key_id: definition for definition in table.ordered_keys()}
    by_opcode: dict[int, list[int]] = {}
    for key_id, definition in definition_by_key.items():
        by_opcode.setdefault(definition.opcode, []).append(key_id)

    for opcode, key_ids in sorted(by_opcode.items()):
        if len(key_ids) > 1:
            report.ran("shared-opcode-domain")
        for pair in concurrent:
            members = sorted(set(pair) & set(key_ids))
            if len(members) > 1:
                report.error(
                    "V-OPCODE-CONCURRENT",
                    f"logical identities {members} share opcode {opcode} and may be "
                    "in flight together. One opcode domain has a single hardware "
                    "time origin and a single CalReg entry, so the two cannot each "
                    "align; the phase split must be redone "
                    "(Calendar §2.3.1, §2.7.1)",
                )

    report.ran("conflict-proof-present")
    proofs = proofs or {}
    for key_id in sorted(definition_by_key):
        if not proofs.get(key_id):
            definition = definition_by_key[key_id]
            report.warn(
                "V-NO-CONFLICT-PROOF",
                f"no conflictProof recorded for keyId {key_id} "
                f"({definition.route_key.phase_id}); the same-opcode no-conflict "
                "guarantee is the NoC algorithm's and cannot be re-derived by "
                "software (Calendar §2.7.1)",
                key_id=key_id,
            )
    return report


def build_package(
    *,
    table: CalendarRouteTable,
    calreg: CalRegImage,
    kernel_object: ObjectFile | None = None,
    weight_shard_bytes: Iterable[int] = (),
    conflict_proofs: dict[int, str] | None = None,
) -> DeploymentPackage:
    """Assemble a deployment package from the compiler's outputs."""
    if calreg.calendar_version != table.calendar_version:
        raise WseModelError(
            "the CalReg mirror and the route table must come from one algorithm "
            "call and therefore carry the same calendarVersion "
            f"({calreg.calendar_version} vs {table.calendar_version})"
        )
    return DeploymentPackage(
        table=table,
        calreg=calreg,
        kernel_object=kernel_object,
        weight_shard_bytes=tuple(weight_shard_bytes),
        conflict_proofs=dict(conflict_proofs or {}),
    )
