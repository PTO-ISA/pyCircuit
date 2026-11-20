"""Tests for the independent P0 FlashAttention2 reference models."""

from __future__ import annotations

import random
import unittest

from .oracle_attention import (
    AttentionInputError,
    P0Profile,
    SparseSelection,
    dense_attention,
    max_abs_error,
    p0_layout_address,
    tiled_online_attention,
)


def _tensor(batch: int, heads: int, sequence: int, dimension: int, seed: int) -> list:
    rng = random.Random(seed)
    return [
        [
            [
                [rng.uniform(-0.5, 0.5) for _ in range(dimension)]
                for _ in range(sequence)
            ]
            for _ in range(heads)
        ]
        for _ in range(batch)
    ]


class OracleAttentionTest(unittest.TestCase):
    def test_tiled_online_matches_dense_for_non_divisible_shapes(self) -> None:
        q = _tensor(1, 2, 19, 7, 11)
        k = _tensor(1, 2, 23, 7, 12)
        v = _tensor(1, 2, 23, 5, 13)
        dense = dense_attention(q, k, v, scale=0.25)
        tiled = tiled_online_attention(
            q,
            k,
            v,
            scale=0.25,
            profile=P0Profile(q_tile_rows=5, kv_tile_rows=6, d_tile=4),
        )
        self.assertLess(max_abs_error(dense, tiled), 1e-12)

    def test_causal_empty_row_is_zero(self) -> None:
        q = [[[[1.0], [2.0]]]]
        k = [[[[1.0], [3.0]]]]
        v = [[[[4.0], [8.0]]]]
        profile = P0Profile(
            q_tile_rows=2,
            kv_tile_rows=1,
            d_tile=1,
            mask_mode="causal",
            causal_offset=-1,
        )
        output = tiled_online_attention(q, k, v, scale=1.0, profile=profile)
        self.assertEqual(output[0][0][0], [0.0])
        self.assertEqual(output[0][0][1], [4.0])

    def test_layout_matches_lane_striping_contract(self) -> None:
        self.assertEqual(p0_layout_address(0, 0, 64), (0, 0, 0))
        self.assertEqual(p0_layout_address(0, 15, 64), (15, 0, 0))
        self.assertEqual(p0_layout_address(0, 16, 64), (0, 1, 1))
        self.assertEqual(p0_layout_address(1, 16, 64), (0, 1, 5))

    def test_rejects_ragged_or_non_finite_inputs(self) -> None:
        with self.assertRaises(AttentionInputError):
            dense_attention(
                [[[[1.0], [2.0, 3.0]]]], [[[[1.0]]]], [[[[1.0]]]], scale=1.0
            )
        with self.assertRaises(AttentionInputError):
            dense_attention([[[[float("nan")]]]], [[[[1.0]]]], [[[[1.0]]]], scale=1.0)

    def test_sparse_unsorted_selection_has_hand_computed_result(self) -> None:
        # Equal selected logits: mean V[7], V[2], V[5], not the dense mean.
        q = [[[[0.0], [0.0]]]]
        k = [[[[float(j)] for j in range(8)]]]
        v = [[[[float(j * j)] for j in range(8)]]]
        selection = SparseSelection(
            [[[[7, 2, 5, 0xFFFFFFFF], [3, 0, -1, -1]]]], [[[3, 2]]]
        )
        expected = [[[[26.0], [4.5]]]]
        full_row = dense_attention(q, k, v, scale=1.0, selection=selection)
        online = tiled_online_attention(
            q, k, v, scale=1.0, selection=selection, profile=P0Profile(kv_tile_rows=2)
        )
        self.assertLess(max_abs_error(full_row, expected), 1e-12)
        self.assertLess(max_abs_error(online, expected), 1e-12)
        self.assertNotEqual(full_row, dense_attention(q, k, v, scale=1.0))

    def test_sparse_causal_uses_token_not_list_position(self) -> None:
        q = [[[[1.0], [1.0], [1.0]]]]
        k = [[[[float(j)] for j in range(8)]]]
        v = [[[[10.0 * j] for j in range(8)]]]
        selection = SparseSelection(
            [[[[7, 2, 5], [7, 6, -1], [-1, -1, -1]]]], [[[3, 2, 0]]]
        )
        expected = [[[[20.0], [0.0], [0.0]]]]
        self.assertEqual(
            dense_attention(
                q,
                k,
                v,
                scale=1.0,
                mask_mode="causal",
                causal_offset=4,
                selection=selection,
            ),
            expected,
        )
        for tile in (1, 2, 4):
            self.assertEqual(
                tiled_online_attention(
                    q,
                    k,
                    v,
                    scale=1.0,
                    selection=selection,
                    profile=P0Profile(
                        kv_tile_rows=tile, mask_mode="causal", causal_offset=4
                    ),
                ),
                expected,
            )

    def test_full_index_set_matches_dense_and_permutation(self) -> None:
        q, k, v = (
            _tensor(2, 2, 5, 7, 20),
            _tensor(2, 2, 9, 7, 21),
            _tensor(2, 2, 9, 4, 22),
        )
        permutation = [8, 0, 5, 2, 7, 1, 6, 4, 3]
        selection = SparseSelection(
            [[[permutation[:] for _ in range(5)] for _ in range(2)] for _ in range(2)],
            [[[9] * 5 for _ in range(2)] for _ in range(2)],
        )
        for mode in ("none", "causal"):
            expected = dense_attention(
                q, k, v, scale=0.25, mask_mode=mode, causal_offset=-1
            )
            actual = dense_attention(
                q,
                k,
                v,
                scale=0.25,
                mask_mode=mode,
                causal_offset=-1,
                selection=selection,
            )
            self.assertEqual(expected, actual)
            for tile in (1, 4, 16):
                online = tiled_online_attention(
                    q,
                    k,
                    v,
                    scale=0.25,
                    selection=selection,
                    profile=P0Profile(
                        q_tile_rows=3,
                        kv_tile_rows=tile,
                        d_tile=4,
                        mask_mode=mode,
                        causal_offset=-1,
                    ),
                )
                self.assertLess(max_abs_error(expected, online), 1e-12)

    def test_sparse_lists_are_isolated_by_batch_head_and_query(self) -> None:
        q, k, v = (
            _tensor(2, 2, 3, 1, 30),
            _tensor(2, 2, 5, 1, 31),
            _tensor(2, 2, 5, 2, 32),
        )
        ids = [
            [[[((b * 2 + h) * 3 + i) % 5] for i in range(3)] for h in range(2)]
            for b in range(2)
        ]
        counts = [[[1] * 3 for _ in range(2)] for _ in range(2)]
        expected = [
            [[v[b][h][ids[b][h][i][0]] for i in range(3)] for h in range(2)]
            for b in range(2)
        ]
        selection = SparseSelection(ids, counts)
        self.assertEqual(
            dense_attention(q, k, v, scale=0.5, selection=selection), expected
        )
        self.assertEqual(
            tiled_online_attention(q, k, v, scale=0.5, selection=selection), expected
        )

    def test_nonuniform_sparse_rows_across_multiple_tiles(self) -> None:
        rng = random.Random(42)
        q, k, v = (
            _tensor(1, 2, 7, 5, 40),
            _tensor(1, 2, 23, 5, 41),
            _tensor(1, 2, 23, 3, 42),
        )
        counts = [[[0, 1, 17, 9, 4, 16, 11] for _ in range(2)]]
        ids = [
            [
                [rng.sample(range(23), count) + [-1] * (17 - count) for count in head]
                for head in counts[0]
            ]
        ]
        selection = SparseSelection(ids, counts)
        for mode in ("none", "causal"):
            expected = dense_attention(
                q, k, v, scale=0.5, selection=selection, mask_mode=mode, causal_offset=8
            )
            for tile in (1, 3, 16):
                online = tiled_online_attention(
                    q,
                    k,
                    v,
                    scale=0.5,
                    selection=selection,
                    profile=P0Profile(
                        q_tile_rows=4,
                        kv_tile_rows=tile,
                        d_tile=3,
                        mask_mode=mode,
                        causal_offset=8,
                    ),
                )
                self.assertLess(max_abs_error(expected, online), 1e-12)

    def test_bad_active_indices_are_rejected_before_causal_filtering(self) -> None:
        q, k, v = [[[[0.0]]]], [[[[0.0], [0.0], [0.0]]]], [[[[1.0], [2.0], [3.0]]]]
        for ids, count in [
            ([1, 1], 2),
            ([3, 0], 1),
            ([-1, 0], 1),
            ([True, 0], 1),
            ([1.0, 0], 1),
            ([1, 0], 3),
            ([1, 0], -1),
            ([1, 0], True),
        ]:
            selection = SparseSelection([[[ids]]], [[[count]]])
            with self.subTest(ids=ids, count=count):
                with self.assertRaises(AttentionInputError):
                    dense_attention(
                        q,
                        k,
                        v,
                        scale=1.0,
                        selection=selection,
                        mask_mode="causal",
                        causal_offset=-1,
                    )
                with self.assertRaises(AttentionInputError):
                    tiled_online_attention(
                        q,
                        k,
                        v,
                        scale=1.0,
                        selection=selection,
                        profile=P0Profile(mask_mode="causal", causal_offset=-1),
                    )

    def test_index_shape_and_capacity_are_checked(self) -> None:
        tensor = [[[[1.0]]]]
        for selection in (
            SparseSelection([], []),
            SparseSelection([[[[]]]], [[[0]]]),
            SparseSelection([[[[0]]]], [[[]]]),
            SparseSelection([[[[0] * 2049]]], [[[0]]]),
        ):
            with (
                self.subTest(selection=selection),
                self.assertRaises(AttentionInputError),
            ):
                dense_attention(tensor, tensor, tensor, scale=1.0, selection=selection)

    def test_invalid_profile_rank_or_arithmetic_is_explicit(self) -> None:
        for tile in (0, -1, True, 1.5):
            with self.subTest(tile=tile), self.assertRaises(AttentionInputError):
                P0Profile(kv_tile_rows=tile)
        for bad in ([], [1], [[[]]], [[[[1.0]], [2.0]]]):
            with self.subTest(bad=bad), self.assertRaises(AttentionInputError):
                dense_attention(bad, [[[[1.0]]]], [[[[1.0]]]], scale=1.0)
        for model in (dense_attention, tiled_online_attention):
            with self.assertRaises(AttentionInputError):
                model([[[[1e308]]]], [[[[1e308]]]], [[[[1.0]]]], scale=1.0)


if __name__ == "__main__":
    unittest.main()
