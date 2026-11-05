"""Independent hand-derived anchors for exact arithmetic and protocol model."""

import copy
import importlib.util
import json
import tempfile
import unittest
from fractions import Fraction
from pathlib import Path

spec = importlib.util.spec_from_file_location(
    "array_vectors", Path(__file__).with_name("generate.py")
)
vectors = importlib.util.module_from_spec(spec)
spec.loader.exec_module(vectors)
runner_spec = importlib.util.spec_from_file_location(
    "array_runner", Path(__file__).with_name("run.py")
)
runner = importlib.util.module_from_spec(runner_spec)
runner_spec.loader.exec_module(runner)


class ArrayOracleTest(unittest.TestCase):
    def test_integer_outer_product_all_lanes(self):
        for row in range(16):
            for column in range(16):
                a, b = row + 1, 2 * column + 1
                self.assertEqual(
                    vectors.decode(
                        vectors.mac(
                            0, vectors.encode(Fraction(a)), vectors.encode(Fraction(b))
                        )
                    ),
                    a * b,
                )

    def test_rounding_nonfused_ftz_and_zero(self):
        self.assertEqual(vectors.mac(0xBF800000, 0x3F800001, 0x3F7FFFFE), 0xB3800000)
        fused = vectors.encode(
            Fraction(-1) + vectors.decode(0x3F800001) * vectors.decode(0x3F7FFFFE)
        )
        self.assertEqual(fused, 0xA8800000)
        self.assertEqual(vectors.mac(0, 0x00800000, 0x3F000000), 0)
        self.assertEqual(vectors.mac(0x80000000, 0, 0x3F800000), 0x80000000)
        self.assertEqual(vectors.encode(Fraction(1 << 128)), 0x7F7FFFFF)

    def test_rejected_beat_abort_and_restart(self):
        s = vectors.Schedule()
        s.add("begin", tag=3, length=1, cell_mask=1)
        beat = {
            "lhs": [0x40000000] * 16,
            "rhs": [0x40400000] * 16,
            "row_mask": 65535,
            "col_mask": 65535,
        }
        s.add("step", tag=4, index=0, **beat)
        self.assertEqual(s.steps, 0)
        s.add("step", tag=3, index=0, **beat)
        self.assertEqual(s.useful, 1)
        before = copy.deepcopy(s.rows)
        s.add("step", tag=3, index=0, **beat)
        self.assertEqual(s.rows, before)
        s.add("abort", tag=3)
        s.add("take")
        self.assertEqual(s.results, [])
        s.add("begin", tag=5, length=1, cell_mask=0)
        s.add("step", tag=5, index=0, **beat)
        s.add("end", tag=5)
        s.add("take")
        self.assertEqual(s.results[0]["rows"], [[0] * 16 for _ in range(16)])

    def test_trace_checker_rejects_missing_wrong_and_late_results(self):
        cases = vectors.generate()
        records = []

        def log(event, epoch, values):
            return {
                "kind": "log",
                "commit_epoch": str(epoch),
                "spec": {"event": event},
                "values": [{"kind": "integer", "value": str(v)} for v in values],
            }

        for action in cases["accepted_actions"]:
            records.append(log("array_action", 2 * (action[0] + 1), action))
        for result in cases["results"]:
            epoch = 2 * (result["take_cycle"] + 1)
            for row, words in enumerate(result["rows"]):
                records.append(log("array_row", epoch, [result["tag"], row, *words]))
            records.append(
                log(
                    "array_result",
                    epoch,
                    [result["tag"], result["steps"], result["useful_macs"]],
                )
            )
        records.append(log("array_pass", 2 * cases["cycles"], [cases["total_results"]]))
        records.append(
            {
                "kind": "result",
                "status": "TERMINATED",
                "error": None,
                "epoch_time": str(2 * cases["cycles"]),
            }
        )
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.log"

            def check(rows):
                path.write_text("".join(json.dumps(row) + "\n" for row in rows))
                return runner.validate(path, cases)

            _, metrics = check(records)
            self.assertEqual(metrics[3]["begin_to_take_cycles"], 66)
            for mutation in range(4):
                changed = copy.deepcopy(records)
                row_index = next(
                    i
                    for i, r in enumerate(changed)
                    if r.get("spec", {}).get("event") == "array_row"
                )
                if mutation == 0:
                    changed.pop(0)
                elif mutation == 1:
                    changed[row_index]["values"][2]["value"] = "12345"
                elif mutation == 2:
                    changed[row_index]["commit_epoch"] = str(
                        int(changed[row_index]["commit_epoch"]) + 2
                    )
                else:
                    changed.insert(row_index, copy.deepcopy(changed[row_index]))
                with self.assertRaises(ValueError):
                    check(changed)

    def test_dense_cadence_and_sparse_coverage(self):
        cases = vectors.generate()
        for result in cases["results"][:4]:
            self.assertEqual(
                result["take_cycle"] - result["begin_cycle"], result["steps"] + 2
            )
            self.assertEqual(result["useful_macs"], 256 * result["steps"])
        for row, result in enumerate(cases["results"][4:20]):
            self.assertEqual(result["useful_macs"], 32)
            self.assertTrue(
                all(
                    word == 0
                    for r, words in enumerate(result["rows"])
                    if r != row
                    for word in words
                )
            )


if __name__ == "__main__":
    unittest.main()
