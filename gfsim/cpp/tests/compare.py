"""Drive C++ and the existing independent Python oracle with identical stimuli.

Reuse the original end-to-end scoreboards (not the runtime) and additionally
compare every C++ tick, including cache work counters against the Python engine.
The 1100-stage drain has a native test to avoid quadratic Python trace overhead.
"""
import inspect
import json
from pathlib import Path
import subprocess
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "experiment"))
from examples.testing import CircuitTestCase
from engine import CapacityCycle
from examples.feedback.model import feedback

RUNNER = sys.argv.pop(1)
ORIGINAL = CircuitTestCase.run_circuit


def stimulus(builder, args, kwargs, ticks, cache):
    bound = inspect.signature(builder).bind(*args, **kwargs)
    bound.apply_defaults()
    p = bound.arguments
    tokens = [builder.__name__, ticks, int(cache), int(p.get("reverse", False))]

    def scalar(*values):
        tokens.extend(int(x) if isinstance(x, bool) else x for x in values)

    def words(values):
        scalar(len(values), *values)

    def changes(values):
        scalar(len(values))
        for due, value in values:
            scalar(due)
            if isinstance(value, tuple):
                scalar(*value)
            else:
                scalar(value)

    kind = builder.__name__
    if kind == "pipeline":
        scalar(p["length"], p["period"], p["iterations"], p["prefill"], p["capacity"])
        words(p["values"])
        words(p["control"])
    elif kind == "packets":
        scalar(p["period"])
        for lane in p["lanes"]:
            words(lane)
        changes(p["changes"])
    elif kind == "pairs":
        scalar(p["period"])
        words(p["left"])
        words(p["right"])
    elif kind == "memory":
        scalar(p["banks"], p["depth"], p["latency"], p["period"], len(p["requests"]))
        for request in p["requests"]:
            scalar(*request)
    elif kind == "feedback":
        scalar(p["self_loop"], len(p["tokens"]))
        for token in p["tokens"]:
            scalar(*token)
    elif kind == "lookup":
        scalar(p["period"], p["depth"])
        words(p["values"])
        changes(p["indices"])
        scalar(len(p["updates"]))
        for index, updates in p["updates"]:
            scalar(index)
            changes(updates)
    elif kind == "retry":
        scalar(p["attempts"])
    else:
        raise ValueError(kind)
    return " ".join(map(str, tokens)) + "\n"


def run_cpp(builder, args, kwargs, ticks, cache=True):
    result = subprocess.run([RUNNER], input=stimulus(builder, args, kwargs, ticks, cache),
                            text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(f"C++ runner failed ({result.returncode}):\n{result.stderr}")
    return [json.loads(line) for line in result.stdout.splitlines()]


def normalized(value):
    return json.loads(json.dumps(value))


def compare(self, builder, *args, ticks=200, **kwargs):
    actual, history = ORIGINAL(self, builder, *args, ticks=ticks, **kwargs)
    traces = [run_cpp(builder, args, kwargs, ticks, cache) for cache in (True, False)]
    oracle = builder(*args, **kwargs, reference=True)
    for tick in range(ticks):
        accepted = oracle.sim.step()
        expected = dict(accepted=sorted(accepted), queues=normalized(oracle.sim.snapshot()),
                        versions=[q.state_version for q in oracle.sim.queues],
                        events=normalized(sorted(oracle.sim.events)), modules=oracle.sim.module_calls,
                        reads=[sorted(s) for s in oracle.sim.reads])
        for mode, trace in enumerate(traces):
            with self.subTest(cpp=builder.__name__, tick=tick, cache=not mode):
                row = trace[tick]
                self.assertEqual({k: row[k] for k in expected}, expected)
                if mode == 0:
                    self.assertEqual(row["rules"], list(history[tick][2]))
    self.assertEqual(traces[0][-1]["stats"]["cache_hits"], actual.sim.stats.cache_hits)
    self.assertEqual(traces[0][-1]["stats"]["rule_work"], actual.sim.stats.rule_work)
    return actual, history


CircuitTestCase.run_circuit = compare

from examples.pipeline.test_model import PipelineTests
from examples.packets.test_model import PacketsTests
from examples.pairs.test_model import PairsTests
from examples.memory.test_model import MemoryTests
from examples.feedback.test_model import FeedbackTests
from examples.lookup.test_model import LookupTests
from examples.retry.test_model import RetryTests


def native_cycle(self):
    for reverse in (False, True):
        for cache in (False, True):
            trace = run_cpp(feedback, (((0, 2), (1, 2)),), dict(reverse=reverse), 1, cache)
            self.assertEqual(trace, [{"cycle": True}])
        oracle = feedback(((0, 2), (1, 2)), reference=True, reverse=reverse)
        with self.assertRaises(CapacityCycle):
            oracle.sim.step()


FeedbackTests.test_real_dynamic_capacity_cycle_terminates_feedback_network = native_cycle
# Executed by CTest's native test, with an output scoreboard and >1000 DFS frames.
del PipelineTests.test_long_prefilled_pipeline_uses_bounded_explicit_stack

if __name__ == "__main__":
    unittest.main(verbosity=2)
