"""Hand-derived anchors and protocol mutation checks, independent of DUT code."""

import copy
import unittest
from fractions import Fraction

from . import generate as oracle


class ArithmeticTest(unittest.TestCase):
    def test_signed_zero_ftz_saturation_and_domain(self):
        self.assertEqual(oracle.mul(0x80000000, 0x3F800000), 0x80000000)
        self.assertEqual(oracle.mul(0x00800000, 0x3F000000), 0)
        self.assertEqual(oracle.mul(0x80800000, 0x3F000000), 0x80000000)
        self.assertEqual(oracle.add(0, 0x80000001), 0x80000000)
        self.assertEqual(oracle.add(0x80000000, 0), 0x80000000)
        self.assertEqual(oracle.add(0, 0x80000000), 0)
        self.assertEqual(oracle.add(0xBF800000, 0x3F800000), 0x80000000)
        self.assertEqual(oracle.mul(0x7F7FFFFF, 0x40000000), 0x7F7FFFFF)
        self.assertEqual(oracle.mul(0xFF7FFFFF, 0x40000000), 0xFF7FFFFF)
        for word in (0x7F800000, 0xFF800000, 0x7FC00000):
            with self.assertRaises(ValueError):
                oracle.decode(word)

    def test_tree_rounding_is_not_linear_or_fused(self):
        values = [0x4B800000, 0x3F800000, 0xCB800000, 0xBF800000] + [0] * 12
        self.assertEqual(oracle.tree(values), 0)
        acc = 0
        for value in values:
            acc = oracle.add(acc, value)
        self.assertEqual(acc, 0xBF800000)
        product = oracle.mul(0x3F800001, 0x3F7FFFFE)
        self.assertEqual(product, 0x3F7FFFFF)
        self.assertEqual(oracle.tree([0xBF800000, product] + [0] * 14), 0xB3800000)
        exact = Fraction(-1) + oracle.decode(0x3F800001) * oracle.decode(0x3F7FFFFE)
        self.assertEqual(oracle.encode(exact), 0xA8800000)

    def test_all_cells_asymmetric_k_orientation(self):
        # Sum(j*(18-j), j=1..16)=952, then operand scales contribute1/128.
        cases = oracle.generate()
        result = next(r for r in cases["results"] if r["tag"] == 3)
        for row in range(16):
            for column in range(16):
                expected = Fraction(119, 16) * (row + 1) * (2 * column + 1)
                self.assertEqual(result["rows"][row][column], oracle.encode(expected))
        signed_zero = next(r for r in cases["results"] if r["tag"] == 114)
        third_tail = next(r for r in cases["results"] if r["tag"] == 9)
        self.assertEqual(third_tail["groups"], 3)
        self.assertEqual(third_tail["useful_macs"], 33 * 256)
        for row in range(16):
            for column in range(16):
                expected = Fraction(1921, 128) * (row + 1) * (2 * column + 1)
                self.assertEqual(third_tail["rows"][row][column], oracle.encode(expected))
        self.assertEqual(signed_zero["rows"], [[0x80000000] * 16 for _ in range(16)])
        self.assertEqual(signed_zero["useful_macs"], 512)
        self.assertEqual(signed_zero["take_cycle"], signed_zero["end_cycle"] + 1)


class PipelineTest(unittest.TestCase):
    def setUp(self):
        self.s = oracle.Schedule()
        self.one = self.s.matrix_id([[0x3F800000] * 16 for _ in range(16)])

    def group(self, tag, index=0, **extra):
        fields = {
            "action": "group",
            "tag": tag,
            "group_index": index,
            "left_id": self.one,
            "right_id": self.one,
            "row_mask": 65535,
            "col_mask": 65535,
            "k_mask": 65535,
        }
        fields.update(extra)
        return self.s.add("advance", **fields)

    def test_latency_invalid_advances_and_no_call_hold(self):
        self.s.add("begin", tag=1, length=1, cell_mask=oracle.FULL)
        self.assertTrue(self.group(1))
        snapshot = copy.deepcopy(self.s.slots)
        self.s.add("idle")
        self.assertEqual(self.s.slots, snapshot)
        self.assertFalse(self.group(1))  # duplicate, but first pipeline shift
        self.assertFalse(self.s.add("advance", tag=999))
        self.assertTrue(self.s.add("advance", tag=1, action="end"))
        self.s.add("advance", tag=1)
        self.assertEqual(self.s.retired, 0)
        self.s.add("advance", tag=1)
        self.assertEqual(self.s.retired, 1)
        self.assertEqual(self.s.phase, "result")
        self.assertEqual(self.s.rows, [[0x3F800000] * 16 for _ in range(16)])
        before = copy.deepcopy(self.s.rows)
        self.assertFalse(self.group(1, 1))
        self.assertEqual(self.s.rows, before)
        self.s.add("take")
        self.assertEqual(self.s.results[0]["useful_macs"], 256)

    def test_abort_at_retirement_edge_and_restart(self):
        self.s.add("begin", tag=1, length=16, cell_mask=oracle.FULL)
        self.group(1)
        for _ in range(4):
            self.s.add("advance", tag=1)
        self.assertIsNotNone(self.s.slots[4])
        self.s.add("advance", tag=1, action="abort")
        self.assertEqual(self.s.retired, 0)
        self.assertEqual(self.s.rows, [[0] * 16 for _ in range(16)])
        self.assertEqual(self.s.slots, [None] * 5)
        self.assertFalse(self.s.add("take"))
        self.s.add("begin", tag=2, length=17, cell_mask=1)
        self.group(2, 0, k_mask=1)
        self.group(2, 1, k_mask=0x8001)  # tail removes lane15
        self.s.finish(2)
        self.assertEqual(self.s.results[0]["rows"][0][0], 0x40000000)
        self.assertEqual(self.s.results[0]["useful_macs"], 2)

    def test_empty_group_consumes_and_retires_without_accumulation(self):
        self.s.add("begin", tag=1, length=16, cell_mask=oracle.FULL)
        self.group(1, k_mask=0)
        self.s.finish(1)
        self.assertEqual(self.s.results[0]["groups"], 1)
        self.assertEqual(self.s.results[0]["useful_macs"], 0)
        self.assertEqual(self.s.results[0]["rows"], [[0] * 16 for _ in range(16)])
        self.assertEqual(self.s.results[0]["take_cycle"], 7)


class TraceCheckerTest(unittest.TestCase):
    def test_mutations_fail_for_acceptance_payload_epoch_and_terminal(self):
        import json
        import tempfile
        from pathlib import Path

        from . import run as runner

        cases = oracle.generate()

        def log(event, epoch, values):
            return {
                "kind": "log",
                "commit_epoch": str(epoch),
                "spec": {"event": event},
                "values": [{"kind": "integer", "value": str(v)} for v in values],
            }

        records = [
            log("array3d_action", 2 * (a[0] + 1), a) for a in cases["expected_actions"]
        ]
        for result in cases["results"]:
            epoch = 2 * (result["take_cycle"] + 1)
            for row, values in enumerate(result["rows"]):
                records.append(log("array3d_row", epoch, [result["tag"], row, *values]))
            records.append(
                log(
                    "array3d_result",
                    epoch,
                    [result["tag"], result["groups"], result["useful_macs"]],
                )
            )
        records.append(
            log(
                "array3d_pass",
                2 * (len(cases["instructions"]) + 1),
                [cases["total_results"]],
            )
        )
        records.append(
            {
                "kind": "result",
                "status": "TERMINATED",
                "error": None,
                "epoch_time": str(2 * cases["cycles"]),
            }
        )
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.jsonl"

            def check(items):
                path.write_text("\n".join(json.dumps(r) for r in items) + "\n")
                return runner.validate(path, cases)

            check(records)
            row_index = next(
                i
                for i, r in enumerate(records)
                if r.get("spec", {}).get("event") == "array3d_row"
            )
            for mutation in ("acceptance", "missing", "value", "epoch", "terminal"):
                items = copy.deepcopy(records)
                if mutation == "acceptance":
                    items[0]["values"][-1]["value"] = "0"
                elif mutation == "missing":
                    items.pop(row_index)
                elif mutation == "value":
                    items[row_index]["values"][2]["value"] = "123"
                elif mutation == "epoch":
                    items[row_index]["commit_epoch"] = str(
                        int(items[row_index]["commit_epoch"]) + 2
                    )
                else:
                    items[-1]["error"] = "failure"
                with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                    check(items)


if __name__ == "__main__":
    unittest.main()
