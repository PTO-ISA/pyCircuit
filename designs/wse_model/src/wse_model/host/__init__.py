"""The Batcher, the UB bus, and the runtime/load-time model.

Whitepaper §6, §7, §8 and §12 describe the device's outside world: a single
external entry point, a nine-lane bus, three load-time dispatch chains, and three
frequency tiers. This package models what the sources fix and declares what they
leave open:

* :mod:`wse_model.host.ub_bus` — the 8-lane fabric plus 1-lane host bus, and the
  observation that drives the whole three-way partition: the fabric is an order
  of magnitude narrower than a single core's local DRAM, so only activations may
  cross it, never weights;
* :mod:`wse_model.host.batcher` — the four responsibilities, the two disjoint
  paths, and the cold-miss arithmetic that makes ``Batcher.mem`` worth sharing.
  Its capacity and bandwidth are open item ``Q3`` and are therefore declared
  inputs;
* :mod:`wse_model.host.runtime` — the load / per-launch / runtime tiers, the
  three-version check, the ``CalReg`` install window, kickstart, and the
  scheduling constraints the design makes correctness requirements.
"""

from wse_model.host.batcher import (
    BatcherMem,
    BatcherResponsibility,
    BatcherSpec,
    DispatchPath,
    RefillAccount,
    dispatch_paths,
)
from wse_model.host.runtime import (
    DispatchChain,
    InstallState,
    LaunchConstraints,
    Loader,
    RuntimeTier,
    Scheduler,
    VersionSet,
    dispatch_chains,
)
from wse_model.host.ub_bus import FABRIC_LANES, HOST_LANES, TOTAL_LANES, UbBus, UbLane

__all__ = [
    "FABRIC_LANES",
    "HOST_LANES",
    "TOTAL_LANES",
    "BatcherMem",
    "BatcherResponsibility",
    "BatcherSpec",
    "DispatchChain",
    "DispatchPath",
    "InstallState",
    "LaunchConstraints",
    "Loader",
    "RefillAccount",
    "RuntimeTier",
    "Scheduler",
    "UbBus",
    "UbLane",
    "VersionSet",
    "dispatch_chains",
    "dispatch_paths",
]
