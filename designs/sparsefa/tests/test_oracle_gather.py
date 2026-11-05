"""Hand-derived sparse address expectations, separate from attention arithmetic."""

from __future__ import annotations

import unittest

from .oracle_attention import AttentionInputError
from .oracle_gather import ReadView, gather_rows, index_read_addresses


def _cache(base: int, token_stride: int) -> ReadView:
    return ReadView(
        base, (2, 2, 8, 2), (0x2000, 0x1000, token_stride, 2), 2, base, base + 0x5000
    )


class GatherOracleTest(unittest.TestCase):
    def test_noncontiguous_token_addresses_use_independent_k_v_strides(self) -> None:
        rows = gather_rows(
            [7, 2, 5, -1], 3, _cache(0x1000, 0x100), _cache(0x8000, 0x180), tile_rows=2
        )
        self.assertEqual([r.token for r in rows], [7, 2, 5])
        self.assertEqual([r.list_position for r in rows], [0, 1, 2])
        self.assertEqual(
            [(r.tile, r.packed_row) for r in rows], [(0, 0), (0, 1), (1, 0)]
        )
        self.assertEqual(
            [r.k_addresses for r in rows],
            [(0x1700, 0x1702), (0x1200, 0x1202), (0x1500, 0x1502)],
        )
        self.assertEqual(
            [r.v_addresses for r in rows],
            [(0x8A80, 0x8A82), (0x8300, 0x8302), (0x8780, 0x8782)],
        )

    def test_batch_head_offsets_and_query_identity(self) -> None:
        row = gather_rows(
            [2],
            1,
            _cache(0x1000, 0x100),
            _cache(0x8000, 0x180),
            batch=1,
            head=1,
            query=9,
        )[0]
        self.assertEqual((row.batch, row.head, row.query, row.token), (1, 1, 9, 2))
        self.assertEqual(row.k_addresses, (0x4200, 0x4202))
        self.assertEqual(row.v_addresses, (0xB300, 0xB302))

    def test_count_zero_and_padding_do_not_read_kv(self) -> None:
        k, v = _cache(0x1000, 0x100), _cache(0x8000, 0x180)
        self.assertEqual(gather_rows([-1, 0xFFFFFFFF], 0, k, v), [])
        self.assertEqual([r.token for r in gather_rows([2, 2, -1], 1, k, v)], [2])

    def test_full_prefix_validation_rejects_duplicate_and_late_oob(self) -> None:
        for tokens in ([1, 1], [1, 8]):
            with self.subTest(tokens=tokens), self.assertRaises(AttentionInputError):
                gather_rows(tokens, 2, _cache(0x1000, 0x100), _cache(0x8000, 0x180))

    def test_capacity_boundary(self) -> None:
        view = ReadView(0x1000, (1, 1, 2048, 1), (4096, 4096, 2, 2), 2, 0x1000, 0x2000)
        rows = gather_rows(list(range(2047, -1, -1)), 2048, view, view)
        self.assertEqual(
            (len(rows), rows[-1].tile, rows[-1].packed_row), (2048, 127, 15)
        )
        self.assertEqual(rows[0].k_addresses, (0x1FFE,))
        with self.assertRaises(AttentionInputError):
            gather_rows([0] * 2049, 0, view, view)

    def test_index_count_addresses_and_explicit_head_broadcast(self) -> None:
        index = ReadView(0x100, (2, 2, 3, 4), (0x100, 0, 0x20, 4), 4, 0x100, 0x300)
        count = ReadView(0x800, (2, 2, 3), (0x40, 0, 4), 4, 0x800, 0x880)
        expected = (0x848, (0x240, 0x244, 0x248))
        for head in (0, 1):
            self.assertEqual(
                index_read_addresses(
                    index, count, batch=1, head=head, query=2, count=3, key_length=8
                ),
                expected,
            )
        self.assertEqual(
            index_read_addresses(
                index, count, batch=1, head=1, query=2, count=0, key_length=8
            ),
            (0x848, ()),
        )
        with self.assertRaises(AttentionInputError):
            index_read_addresses(
                index, count, batch=0, head=0, query=0, count=5, key_length=8
            )

    def test_readable_region_checks_last_byte_and_64bit_overflow(self) -> None:
        final = ReadView((1 << 64) - 2, (1,), (2,), 2, 0, 1 << 64)
        self.assertEqual(final.address(0), (1 << 64) - 2)
        for arguments in (
            (0x100, (1,), (2,), 2, 0x100, 0x101),
            ((1 << 64) - 2, (2,), (2,), 2, 0, 1 << 64),
            (0x100, (2,), (1 << 64,), 2, 0, 1 << 64),
            (0x101, (1,), (2,), 2, 0x100, 0x200),
        ):
            with (
                self.subTest(arguments=arguments),
                self.assertRaises(AttentionInputError),
            ):
                ReadView(*arguments)

    def test_noncontiguous_index_items_and_bad_coordinates_rejected(self) -> None:
        index = ReadView(0x100, (1, 1, 1, 2), (0, 0, 0, 8), 4, 0x100, 0x200)
        count = ReadView(0x800, (1, 1, 1), (0, 0, 0), 4, 0x800, 0x804)
        with self.assertRaises(AttentionInputError):
            index_read_addresses(
                index, count, batch=0, head=0, query=0, count=1, key_length=8
            )
        for coordinates in ((2, 0, 0, 0), (0, 0, 8, 0), (0, 0, 0, 2), (True, 0, 0, 0)):
            with (
                self.subTest(coordinates=coordinates),
                self.assertRaises(AttentionInputError),
            ):
                _cache(0x1000, 0x100).address(*coordinates)


if __name__ == "__main__":
    unittest.main()
