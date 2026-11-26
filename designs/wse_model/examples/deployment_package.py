#!/usr/bin/env python3
"""Assemble and validate a compiled deployment package.

Whitepaper §13.3 lists what leaves the compiler; Calendar §2.7.1 adds two timing
guarantees and §2.7.3 step 4 the per-call-site immediates. This script builds the
FFN package, prints the artifacts and the immediates, shows that the two phases
are legal because they are separate phases — and then declares them concurrent to
show the same-opcode rule refusing.

Run:

    python examples/deployment_package.py
"""

from __future__ import annotations

import json

from wse_model.compiler import build_package
from wse_model.fixtures import clean_kernel_object, ffn_example


def main() -> int:
    example = ffn_example()
    package = build_package(
        table=example.table,
        calreg=example.calreg(),
        kernel_object=clean_kernel_object(),
        weight_shard_bytes=(2048, 2048),
        conflict_proofs={0: "fixture:no-conflict-key0", 1: "fixture:no-conflict-key1"},
    )

    print("== the five artifacts (whitepaper §13.3) ==")
    for artifact in package.describe()["artifacts"]:
        print(f"  {artifact['artifact']:26s} {artifact['form']:22s} -> {artifact['destination']}")

    print("\n== versions (one algorithm call, so they must agree) ==")
    print(json.dumps(package.versions.describe(), indent=2))

    print("\n== per-call-site immediates (Calendar §2.7.3 step 4) ==")
    for immediate in package.call_site_immediates():
        described = immediate.describe()
        print(
            f"  keyId={described['keyId']} opcode={described['opcode']} "
            f"expVal={described['expVal']:6d} capacity={described['capacity']:6d} "
            f"selfOffStride={described['selfOffStride']:5d} "
            f"nBurst={described['nBurst']}"
        )

    print("\n== validation: the two phases are separate, so they may share opcode 1 ==")
    sequential = package.validate()
    print(sequential.format())

    print("\n== validation: declaring the two phases concurrent must be refused ==")
    concurrent = package.validate(
        concurrent={frozenset(definition.key_id for definition in example.table.ordered_keys())}
    )
    print(concurrent.format())

    print(
        "\nBoth phases use opcode 1, so they share one CalReg entry and one "
        "alignment semaphore domain: they cannot each align, and the compiler must "
        "redo the phase split rather than let them overlap "
        f"(Calendar §2.3.1, §2.7.1). rodata segment: {package.rodata_bytes} B."
    )
    return 0 if sequential.ok and not concurrent.ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
