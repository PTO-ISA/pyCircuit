"""Opt-in full payload oracle for a separately executed generated simulation.

Set WSE_PTO_ALLGATHER_INPUT and WSE_PTO_ALLGATHER_OUTPUT to its binary files.
Optionally set WSE_PTO_ALLGATHER_MANIFEST to the actual PyPTO program.json.
Without a manifest, the retained baseline is 32 node-major blocks of 1536 bytes.
This test checks observed bytes; it does not compile or execute the DUT.
"""

import json
import os
from pathlib import Path

import pytest

pytestmark = pytest.mark.integration


def test_generated_output_contains_every_original_source_on_every_member():
    input_path = os.environ.get("WSE_PTO_ALLGATHER_INPUT")
    output_path = os.environ.get("WSE_PTO_ALLGATHER_OUTPUT")
    if input_path is None and output_path is None:
        pytest.skip("set input/output paths from a real generated simulation")
    assert input_path and output_path, "both input and output paths are required"
    original = Path(input_path).read_bytes()
    actual = Path(output_path).read_bytes()
    manifest_path = os.environ.get("WSE_PTO_ALLGATHER_MANIFEST")
    members, source_bytes = 32, 1536
    if manifest_path:
        manifest = json.loads(Path(manifest_path).read_text())
        assert manifest["schema"] == "wse-pypto-allgather/1"
        members = len(manifest["members"])
        source_bytes = manifest["phases"][0]["words"] * 4
        assert members > 0 and source_bytes > 0
    output_bytes = members * source_bytes
    assert len(original) == output_bytes
    assert len(actual) == members * output_bytes
    # Row ranks ascend by column; column ranks ascend by row. Their composition
    # follows the declared member order, preserving original FP32 bits exactly.
    for destination in range(members):
        observed = actual[destination * output_bytes : (destination + 1) * output_bytes]
        if observed != original:
            offset = next(
                i for i, (a, b) in enumerate(zip(observed, original, strict=True)) if a != b
            )
            source, local_offset = divmod(offset, source_bytes)
            pytest.fail(
                f"destination_rank {destination}, source_rank {source}, byte {local_offset}: "
                f"got {observed[offset]:02x}, expected {original[offset]:02x}"
            )
