from __future__ import annotations

from ..spec.types import BundleSpec, SignatureSpec, StructSpec


def StreamSig(
    *,
    name: str = "stream",
    payload: StructSpec | BundleSpec | None = None,
    payload_prefix: str = "payload",
    valid_name: str = "valid",
    ready_name: str = "ready",
) -> SignatureSpec:
    """Create a strict ready/valid stream signature (producer perspective).

    Producer perspective:
    - `valid`: out
    - `ready`: in
    - `payload.*`: out

    Use `StreamSig(...).flip()` for the consumer perspective.
    """

    leaves: dict[str, tuple[str, int, bool]] = {
        str(valid_name): ("out", 1, False),
        str(ready_name): ("in", 1, False),
    }

    if payload is not None:
        if isinstance(payload, StructSpec):
            for path, fld in payload.flatten_fields():
                leaves[f"{payload_prefix}.{path}"] = (
                    "out",
                    int(fld.width or 0),
                    bool(fld.signed),
                )
        elif isinstance(payload, BundleSpec):
            for f in payload.fields:
                leaves[f"{payload_prefix}.{f.name}"] = (
                    "out",
                    int(f.width),
                    bool(f.signed),
                )
        else:
            raise TypeError(
                f"StreamSig payload must be StructSpec or BundleSpec, got {type(payload).__name__}"
            )

    return SignatureSpec.from_leaf_map(name=str(name), fields=leaves)
