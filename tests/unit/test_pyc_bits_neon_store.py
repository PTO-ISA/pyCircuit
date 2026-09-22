"""Pin the GCC-safe NEON store in library/cpp/pyc_bits.hpp.

``vst1q_u64`` takes a ``uint64x2_t``, but ``vmvnq_u8`` returns a
``uint8x16_t``. Clang silently converts between the two NEON vector types;
GCC refuses, so the implicit conversion broke every aarch64 Linux host build
and every emitted C++ simulator. The result has to be reinterpreted back to
``uint64x2_t`` before the store. This is a source-shape guard because the unit
suite does not own a compiler matrix; the focused aarch64 compile and run
checks cover the real behavior.
"""

from __future__ import annotations

from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
BITS_HEADER = ROOT / "library/cpp/pyc_bits.hpp"


def test_pyc_bits_bitwise_not_reinterprets_before_the_u64_store() -> None:
    source = BITS_HEADER.read_text(encoding="utf-8")

    assert "vmvnq_u8(vreinterpretq_u8_u64(va))" in source
    assert "vreinterpretq_u64_u8(vmvnq_u8(vreinterpretq_u8_u64(va)))" in source
