"""Load-time, per-launch, and runtime behaviour.

Whitepaper §12 and §14: the runtime works at **three frequencies**, and
conflating them is the design's most common performance mistake. This module
models the tiers, the three load-time dispatch chains with their distinct
destinations, the three-version check, kickstart, and the scheduling constraints
the design makes correctness requirements rather than hints:

1. schedule by complete group — a wave must not split a group that is syncing;
2. insert a barrier at phase boundaries, so all Calendar traffic on the die has
   drained (this is the *only* thing separating epochs across launches, ``SW-1``);
3. write ``CalReg`` only inside a stop-the-world window;
4. provide ``blockId``.

In steady state a launch dispatches activations and a descriptor and **no
Calendar data at all** — the payoff of moving the route information from a
per-launch value to a load-time one (whitepaper §4.2, §12.2).
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum

from wse_model.errors import CalendarError, VersionMismatchError, WseModelError
from wse_model.noc.calreg import CalRegImage

__all__ = [
    "DispatchChain",
    "InstallState",
    "LaunchConstraints",
    "Loader",
    "RuntimeTier",
    "Scheduler",
    "VersionSet",
    "dispatch_chains",
]


class RuntimeTier(str, Enum):
    """The three frequency tiers of whitepaper §12."""

    LOAD = "load time (once per model load)"
    PER_LAUNCH = "per launch (once per operator)"
    RUNTIME = "run time (the runtime does not intervene)"


class DispatchChain(str, Enum):
    """The three load-time chains and the per-launch one."""

    KERNEL_BINARY = "kernel binary (.text + .rodata)"
    WEIGHTS = "model weights"
    CALREG = "CalReg timeslot image"
    ACTIVATIONS = "input activations"


#: Whitepaper §12.1 and §14.1: where each chain goes and how often.
_CHAINS: dict[DispatchChain, dict[str, object]] = {
    DispatchChain.KERNEL_BINARY: {
        "tier": RuntimeTier.LOAD,
        "via_batcher": True,
        "destination": "DDR code/data segments, refilled into I$/D$ on a miss",
        "through_local_dram": False,
        "through_l1": False,
        "note": "indistinguishable from an ordinary dispatch; no special handling",
    },
    DispatchChain.WEIGHTS: {
        "tier": RuntimeTier.LOAD,
        "via_batcher": True,
        "destination": "each core's local DRAM",
        "through_local_dram": True,
        "through_l1": False,
        "note": "2 KB page aligned; written once and then read every inference",
    },
    DispatchChain.CALREG: {
        "tier": RuntimeTier.LOAD,
        "via_batcher": False,
        "destination": "NoC node timeslot registers",
        "through_local_dram": False,
        "through_l1": False,
        "note": (
            "the only dispatch that does not go through the Batcher, because its "
            "destination is the NoC rather than an AICORE; atomic write inside a "
            "stop-the-world window, read-only afterwards"
        ),
    },
    DispatchChain.ACTIVATIONS: {
        "tier": RuntimeTier.PER_LAUNCH,
        "via_batcher": True,
        "destination": "each core's UB / L1",
        "through_local_dram": False,
        "through_l1": False,
        "note": "the only recurring dispatch once the model is loaded",
    },
}


def dispatch_chains() -> dict[DispatchChain, dict[str, object]]:
    """The four chains with their destinations, frequencies, and exclusions."""
    return {chain: dict(payload) for chain, payload in _CHAINS.items()}


@dataclass(frozen=True)
class VersionSet:
    """The three version numbers that stand in for the removed atomic selector.

    Whitepaper §10.3 and §14.2: ``topologyVersion``, ``calendarVersion``, and
    ``routeVersion`` are checked together at load time, and any mismatch must
    fault rather than degrade.
    """

    topology_version: int
    calendar_version: int
    route_version: int

    def check(self, other: VersionSet) -> None:
        mismatches = [
            f"{name}: compiled {compiled} vs chip {chip}"
            for name, compiled, chip in (
                ("topologyVersion", self.topology_version, other.topology_version),
                ("calendarVersion", self.calendar_version, other.calendar_version),
                ("routeVersion", self.route_version, other.route_version),
            )
            if compiled != chip
        ]
        if mismatches:
            raise VersionMismatchError(
                "load-time version check failed: "
                + "; ".join(mismatches)
                + ". The three versions are the system-level fuse replacing the "
                "removed atomic route/timeslot selection instruction "
                "(whitepaper §10.3, HW-13)"
            )

    def describe(self) -> dict[str, int]:
        return {
            "topologyVersion": self.topology_version,
            "calendarVersion": self.calendar_version,
            "routeVersion": self.route_version,
        }


@dataclass
class InstallState:
    """What has been placed on the device, and whether the die is quiet."""

    kernel_installed: bool = False
    weights_installed: bool = False
    calreg_installed: bool = False
    versions_checked: bool = False
    kicked_off: bool = False
    in_flight_transfers: int = 0
    activations_dispatched: int = 0

    @property
    def drained(self) -> bool:
        return self.in_flight_transfers == 0

    @property
    def ready(self) -> bool:
        return (
            self.kernel_installed
            and self.weights_installed
            and self.calreg_installed
            and self.versions_checked
            and self.kicked_off
        )

    def describe(self) -> dict[str, object]:
        return {
            "kernel_installed": self.kernel_installed,
            "weights_installed": self.weights_installed,
            "calreg_installed": self.calreg_installed,
            "versions_checked": self.versions_checked,
            "kicked_off": self.kicked_off,
            "drained": self.drained,
            "ready": self.ready,
            "activations_dispatched": self.activations_dispatched,
        }


@dataclass
class Loader:
    """Loads a compiled package onto a device and drives launches."""

    state: InstallState = field(default_factory=InstallState)

    def install_kernel(self, *, rodata_bytes: int = 0) -> RuntimeTier:
        """Place ``.text`` and ``.rodata`` in DDR. No special handling (whitepaper §14.1)."""
        if rodata_bytes < 0:
            raise WseModelError("rodata_bytes must be non-negative")
        self.state.kernel_installed = True
        return RuntimeTier.LOAD

    def install_weights(self, *, shard_bytes: int) -> RuntimeTier:
        """Place weight shards in each core's local DRAM, page aligned."""
        if shard_bytes % 2048 != 0:
            raise WseModelError(
                f"a {shard_bytes} B weight shard is not 2 KB page aligned; "
                "the design makes 2 KB the real write granularity "
                "(whitepaper §3.4, §14.1)"
            )
        self.state.weights_installed = True
        return RuntimeTier.LOAD

    def install_calreg(self, image: CalRegImage) -> RuntimeTier:
        """Install the timeslot image, which requires a quiet die."""
        if not self.state.drained:
            raise CalendarError(
                f"cannot install the CalReg image while "
                f"{self.state.in_flight_transfers} transfer(s) are in flight; the "
                "write must be atomic inside a stop-the-world window "
                "(whitepaper §12.3, Calendar §3.9)"
            )
        if image.calendar_version < 0:
            raise WseModelError("calendarVersion must be non-negative")
        self.state.calreg_installed = True
        return RuntimeTier.LOAD

    def check_versions(self, compiled: VersionSet, chip: VersionSet) -> None:
        compiled.check(chip)
        self.state.versions_checked = True

    def kickstart(self) -> RuntimeTier:
        """Set PC and stack pointer, invalidate I$ and D$. Moves no data."""
        required = (
            self.state.kernel_installed,
            self.state.weights_installed,
            self.state.calreg_installed,
            self.state.versions_checked,
        )
        if not all(required):
            raise WseModelError(
                "kickstart requires the kernel, weights, CalReg image, and a "
                "passing version check (whitepaper §14.3)"
            )
        self.state.kicked_off = True
        return RuntimeTier.LOAD

    def launch(self, *, activation_bytes: int, descriptor_bytes: int = 64) -> RuntimeTier:
        """Dispatch one operator's inputs. No Calendar data is sent."""
        if not self.state.ready:
            raise WseModelError("launch requires a completed load and kickstart")
        if not self.state.drained:
            raise CalendarError(
                "a new launch requires all Calendar traffic from the previous one "
                "to have drained; cross-launch epoch isolation depends entirely on "
                "this barrier (whitepaper §12.3, SW-1)"
            )
        for nbytes, label in (
            (activation_bytes, "activation_bytes"),
            (descriptor_bytes, "descriptor_bytes"),
        ):
            if nbytes < 0:
                raise WseModelError(f"{label} must be non-negative")
        self.state.activations_dispatched += 1
        return RuntimeTier.PER_LAUNCH

    def steady_state_dispatch_bytes(
        self, *, activation_bytes: int, descriptor_bytes: int = 64
    ) -> int:
        """Bytes dispatched per launch in steady state.

        Whitepaper §12.2: activations plus a descriptor, and **nothing else** —
        no Calendar data, no per-core expansion, no run-time descriptor fan-out.
        """
        return activation_bytes + descriptor_bytes

    def describe(self) -> dict[str, object]:
        return {
            "state": self.state.describe(),
            "chains": {chain.value: dict(payload) for chain, payload in dispatch_chains().items()},
        }


@dataclass(frozen=True)
class LaunchConstraints:
    """The four scheduling constraints the runtime must honour (§12.3).

    These are correctness requirements: breaking the first splits a syncing
    group, and breaking the second lets the previous launch's stragglers be
    counted into this launch's epoch.
    """

    @staticmethod
    def check_wave(
        *, waves: tuple[frozenset[int], ...], groups: tuple[frozenset[int], ...]
    ) -> None:
        """No wave may split a group that is syncing."""
        for index, wave in enumerate(waves):
            for group in groups:
                inside = wave & group
                if inside and inside != group:
                    missing = sorted(group - wave)
                    raise CalendarError(
                        f"wave {index} splits a syncing group: it holds "
                        f"{sorted(inside)} but leaves {missing} for a later wave. "
                        "A wave must be scheduled by complete group "
                        "(whitepaper §12.3 constraint 1)"
                    )

    @staticmethod
    def require_phase_barrier(*, drained: bool) -> None:
        """A barrier at the phase boundary is what separates epochs across launches."""
        if not drained:
            raise CalendarError(
                "a launch boundary requires all Calendar traffic on the die to "
                "have drained, otherwise a straggling copy from the previous "
                "launch is counted into this launch's {opcode, epoch} "
                "(whitepaper §12.3 constraint 2, SW-1)"
            )

    @staticmethod
    def require_calreg_window(*, in_flight: int) -> None:
        """``CalReg`` may only be rewritten in a stop-the-world window."""
        if in_flight != 0:
            raise CalendarError(
                f"CalReg may not be rewritten with {in_flight} transfer(s) in "
                "flight (whitepaper §12.3 constraint 3)"
            )

    @staticmethod
    def require_block_id(*, block_id: int, node_count: int) -> None:
        """``blockId`` is the scheme's only runtime-varying index."""
        if not 0 <= block_id < node_count:
            raise CalendarError(
                f"blockId {block_id} is outside 0..{node_count - 1}; it indexes the "
                "route table's node dimension (whitepaper §12.3 constraint 4)"
            )

    @staticmethod
    def block_id_cost(*, from_spr: bool) -> dict[str, object]:
        """The two acquisition paths, and why the SPR wins (open item ``C-7``)."""
        return {
            "source": "block_idx SPR" if from_spr else "kernel argument",
            "loads": 0 if from_spr else 1,
            "value_class": "A" if from_spr else "B",
            "note": (
                "the SPR costs no load; passing it as an argument makes it a "
                "per-launch value, adding one load and one more dependency "
                "(whitepaper §12.3, Calendar C-7)"
            ),
        }


@dataclass
class Scheduler:
    """Tracks the scheduling constraints across a sequence of launches."""

    node_count: int = 40
    groups: tuple[frozenset[int], ...] = ()
    launches: int = 0

    def schedule(self, *, waves: tuple[frozenset[int], ...], drained: bool) -> None:
        LaunchConstraints.require_phase_barrier(drained=drained)
        LaunchConstraints.check_wave(waves=waves, groups=self.groups)
        self.launches += 1

    def describe(self) -> dict[str, object]:
        return {
            "node_count": self.node_count,
            "groups": [sorted(group) for group in self.groups],
            "launches": self.launches,
        }
