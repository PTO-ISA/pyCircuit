"""The NoC: flit format, resident ``CalReg``, passive forwarding, and delivery."""

from wse_model.noc.calreg import CalRegBank, CalRegImage, CalRegSlot, StopTheWorldWindow
from wse_model.noc.flit import (
    DEFAULT_EPOCH_TAG_BITS,
    DEFAULT_LINK_WIDTH_BYTES,
    Flit,
    FlitHeader,
    flit_count,
    split_payload,
)
from wse_model.noc.forward import (
    Hop,
    Landing,
    MulticastTrace,
    delivery_order,
    link_load,
    simulate_multicast,
)
from wse_model.noc.network import Noc, SendOutcome, SendPlan

__all__ = [
    "CalRegBank",
    "CalRegImage",
    "CalRegSlot",
    "DEFAULT_EPOCH_TAG_BITS",
    "DEFAULT_LINK_WIDTH_BYTES",
    "Flit",
    "FlitHeader",
    "Hop",
    "Landing",
    "MulticastTrace",
    "Noc",
    "SendOutcome",
    "SendPlan",
    "StopTheWorldWindow",
    "delivery_order",
    "flit_count",
    "link_load",
    "simulate_multicast",
    "split_payload",
]
