"""Independent Rule Queue behavior through the existing closed-System harness.

Expected tokens and epochs are hand-derived from the source schedule. No DUT
names, FIFO metadata representation, lowering facts or compiler logs form the
oracle. These tests deliberately use implicit admission only.
"""

from __future__ import annotations

import hashlib
import json
import shutil

import pytest
from test_source_system_execution import (
    FIXTURE,
    _assert_closed_observation_identities,
    _bundle,
    _cli,
    _compile,
    _ok,
    _records,
    _run,
    _source_occurrence,
)

pytestmark = pytest.mark.system


def _program(tmp_path, top, latency=1):
    source = tmp_path / "rule queue sources"
    source.mkdir()
    for filename in ("rule_queue_provider.py", "rule_queue_bench.py"):
        shutil.copy2(FIXTURE / filename, source)
    provider_source = source / "rule_queue_provider.py"
    provider_source.write_text(
        provider_source.read_text().replace("latency=1", f"latency={latency}")
    )
    provider = tmp_path / "queue provider"
    consumer = tmp_path / "queue consumer"
    _ok(_compile(source, "rule_queue_provider.py", provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "unavailable queue provider body.ac"
    # Publication must carry resource identity and effect requirements. A
    # consumer cannot repair an incomplete interface by reading provider code.
    provider_source.unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "rule_queue_bench.py", consumer, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "rule-queue-system.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            f"checks.rule_queue_bench.{top}",
            "-o",
            str(final),
        )
    )
    return final


def _execute(final, tmp_path, cycles, expected, identity_check=None):
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    transcripts = []
    manifests = {}
    for target in ("cpp", "verilog"):
        target_root = tmp_path / ("rule-queue-" + target)
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        manifests[target] = json.loads(
            (target_root / target / "generated.json").read_text()
        )
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", str(cycles), "--workers", str(workers)]
                if target == "cpp"
                else [f"+cycles={cycles}"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            results = [row for row in rows if row["kind"] == "result"]
            assert len(results) == 1 and results[0]["status"] == "TERMINATED"
            assert int(results[0]["epoch_time"]) == 2 * cycles
            logs = [row for row in rows if row["kind"] == "log"]
            assert {row["spec"]["event"] for row in logs} == set(expected)
            for name, samples in expected.items():
                actual = [row for row in logs if row["spec"]["event"] == name]
                assert [
                    (
                        int(row["evaluation_epoch"]),
                        tuple(int(value["value"]) for value in row["values"]),
                    )
                    for row in actual
                ] == sorted(
                    [
                        (2 * cycle + edge, (cycle, token))
                        for cycle, token in samples
                        for edge in (0, 1)
                    ],
                    key=lambda sample: sample[0],
                )
                assert all(
                    int(row["commit_epoch"]) == int(row["evaluation_epoch"]) + 1
                    for row in actual
                )
            # Exact nonempty samples prevent a blocked or missing Rule from
            # passing by simply publishing no observations.
            assert sum(len(samples) for samples in expected.values()) > 0
            (identity_check or _assert_closed_observation_identities)(rows)
            transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert manifests["cpp"]["entry"] == manifests["verilog"]["entry"]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def _assert_feedback_observation_identities(rows):
    # Frozen authored AST paths, independent of emitted rows/source maps.
    bench = "checks.rule_queue_feedback_bench.ExerciseBranchJoinedFeedback"
    provider = "checks.rule_queue_feedback_provider.BranchJoinedFeedback"

    def body(index):
        return (("field", "body"), ("index", index))

    owner = _source_occurrence(bench, *body(2), *body(1))
    expected = {
        "queue feedback visited": (
            "root/owner",
            _source_occurrence(bench, *body(2), *body(7), ("field", "value")),
            {
                "site": _source_occurrence(provider, *body(2), *body(4), *body(2)),
                "ordinal": 0,
                "invocation": [
                    {
                        "call": _source_occurrence(
                            bench, *body(2), *body(3), *body(0), ("field", "value")
                        ),
                        "receiver": {"instance_path": [owner]},
                    }
                ],
            },
        ),
        "queue feedback retired": (
            "root",
            _source_occurrence(bench, *body(2), *body(8), ("field", "value")),
            {
                "site": _source_occurrence(bench, *body(2), *body(4), *body(1)),
                "ordinal": 0,
                "invocation": [],
            },
        ),
    }
    seen = set()
    for row in rows:
        if row["kind"] == "result":
            continue
        event = row["spec"]["event"]
        instance, registration, identity = expected[event]
        assert row["instance"] == instance
        assert json.loads(row["registration"]) == registration
        assert json.loads(row["site"]) == identity
        seen.add(event)
    assert seen == set(expected)


@pytest.mark.parametrize("latency", [1, 3], ids=["available", "delayed"])
def test_rule_queue_full_replacement_uses_old_head_and_actual_latency(
    tmp_path, latency
):
    final = _program(tmp_path, "ExerciseRuleQueue", latency)
    expected = (
        {"queue replaced": [(2, 10), (3, 20)], "queue taken": [(4, 21)]}
        if latency == 1
        else {"queue replaced": [(3, 10)], "queue taken": [(6, 21)]}
    )
    _execute(final, tmp_path, 8, expected)


def test_rule_queue_resources_and_nested_guards_share_one_atomic_path(tmp_path):
    final = _program(tmp_path, "ExerciseAtomicQueuePaths")
    _execute(
        final,
        tmp_path,
        7,
        {
            "queue atomic exchange": [(4, 11)],
            "queue selected path": [(5, 18)],
        },
    )


@pytest.mark.parametrize("direction", ["pop", "push"])
@pytest.mark.parametrize("separate_rules", [False, True], ids=["one-path", "two-rules"])
def test_rule_queue_same_physical_port_conflicts_reject(
    tmp_path, direction, separate_rules
):
    source = tmp_path / "queue conflict sources"
    source.mkdir()
    effect = "tokens.pop()" if direction == "pop" else "tokens.push(7)"
    body = (
        f"    @ac.rule\n    def first() -> None:\n        {effect}\n"
        f"    @ac.rule\n    def second() -> None:\n        {effect}\n"
        "    first()\n    second()\n"
        if separate_rules
        else f"    @ac.rule\n    def repeated() -> None:\n        {effect}\n"
        f"        {effect}\n    repeated()\n"
    )
    (source / "conflict.py").write_text(
        "import pycircuit as ac\n@ac.system\ndef QueueConflict():\n"
        "    tokens = ac.queue[ac.u8](depth=2, latency=1)\n" + body
    )
    unit = tmp_path / "queue conflict unit"
    _ok(_compile(source, "conflict.py", unit))
    # Actual-owner conflicts are decided after the complete unit closure links.
    final = tmp_path / "conflict.ac"
    final.write_text("preserve prior output\n")
    result = _cli(
        "link", str(unit), "--top", "checks.conflict.QueueConflict", "-o", str(final)
    )
    assert result.returncode != 0
    assert final.read_text() == "preserve prior output\n"
    diagnostic = result.stdout + result.stderr
    messages = " ".join(
        line.split("error:", 1)[1]
        for line in diagnostic.lower().splitlines()
        if "error:" in line
    )
    assert "queue" in messages and "conflict" in messages, diagnostic


def _branch_joined_feedback_program(tmp_path, depth, latency):
    source = tmp_path / "branch feedback sources"
    source.mkdir()
    provider_name = "rule_queue_feedback_provider.py"
    bench_name = "rule_queue_feedback_bench.py"
    for filename in (provider_name, bench_name):
        shutil.copy2(FIXTURE / filename, source)
    provider_source = source / provider_name
    text = provider_source.read_text()
    original = "feedback = ac.queue[LoopItem](depth=1, latency=1)"
    assert text.count(original) == 1
    provider_source.write_text(
        text.replace(
            original, f"feedback = ac.queue[LoopItem](depth={depth}, latency={latency})"
        )
    )
    provider = tmp_path / "branch feedback provider"
    consumer = tmp_path / "branch feedback consumer"
    _ok(_compile(source, provider_name, provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "unavailable feedback provider body.ac"
    provider_source.unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, bench_name, consumer, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "branch-feedback-system.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            "checks.rule_queue_feedback_bench.ExerciseBranchJoinedFeedback",
            "-o",
            str(final),
        )
    )
    return final


@pytest.mark.parametrize(
    "depth,latency", [(1, 1), (2, 3)], ids=["full-single-slot", "delayed-two-slots"]
)
def test_rule_queue_branch_pop_then_joined_push_preserves_replacement(
    tmp_path, depth, latency
):
    final = _branch_joined_feedback_program(tmp_path, depth, latency)
    # Independent old-Q schedules: three ingress values10/11/12 each need two
    # increments before retirement. Every push becomes available after the
    # feedback Queue's actual latency; no same-edge empty bypass is permitted.
    # Depth1 replaces its full feedback head at cycles2/5/8. Depth2/latency3
    # fills with the first two arrivals, then replaces full heads at cycles4/5.
    expected = (
        {
            "queue feedback visited": [
                (1, 10),
                (2, 11),
                (3, 12),
                (4, 11),
                (5, 12),
                (6, 13),
                (7, 12),
                (8, 13),
                (9, 14),
            ],
            "queue feedback retired": [(4, 12), (7, 13), (10, 14)],
        }
        if (depth, latency) == (1, 1)
        else {
            "queue feedback visited": [
                (1, 10),
                (2, 11),
                (4, 11),
                (5, 12),
                (7, 12),
                (8, 13),
                (9, 12),
                (12, 13),
                (15, 14),
            ],
            "queue feedback retired": [(8, 12), (9, 13), (16, 14)],
        }
    )
    _execute(final, tmp_path, 18, expected, _assert_feedback_observation_identities)


@pytest.mark.parametrize(
    "depth,latency", [(1, 1), (2, 3)], ids=["raw-full-replacement", "raw-delayed-head"]
)
def test_rule_queue_raw_bound_pop_preserves_full_replacement(tmp_path, depth, latency):
    source = tmp_path / "raw-bound queue source"
    source.mkdir()
    name = "rule_queue_raw_bound_bench.py"
    text = (FIXTURE / name).read_text()
    assert text.count("depth=1, latency=1") == 1
    (source / name).write_text(
        text.replace("depth=1, latency=1", f"depth={depth}, latency={latency}")
    )
    unit = tmp_path / "raw-bound queue unit"
    _ok(_compile(source, name, unit))
    final = tmp_path / "raw-bound-queue-system.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.rule_queue_raw_bound_bench.ExerciseRawBoundQueue",
            "-o",
            str(final),
        )
    )
    # The raw out_ready boundary is independent traffic policy, with no Rule
    # pop claim. Full same-edge replacement is permitted only for an actually
    # eligible old head; a latency3 full waiting head at cycle5 grants nothing.
    expected = (
        {
            "queue raw-bound pushed": [(0, 10), (3, 13), (4, 14), (5, 15)],
            "queue raw-bound retired": [(3, 10), (4, 13), (5, 14), (6, 15)],
        }
        if (depth, latency) == (1, 1)
        else {
            "queue raw-bound pushed": [(0, 10), (1, 11), (3, 13), (4, 14)],
            "queue raw-bound retired": [(3, 10), (4, 11), (6, 13), (7, 14)],
        }
    )
    _execute(final, tmp_path, 9, expected)


def _assert_hierarchy_observation_identities(rows):
    bench = "checks.rule_queue_hierarchy_bench.ExerciseQueueHierarchy"
    provider = "checks.rule_queue_hierarchy_provider.HierarchyQueue"

    def body(index):
        return (("field", "body"), ("index", index))

    def signature(event, instance, registration, site):
        return (
            event,
            instance,
            json.dumps(registration, sort_keys=True),
            json.dumps(site, sort_keys=True),
        )

    expected = set()
    for side, callable_index, registration_index in (("left", 4, 8), ("right", 5, 9)):
        expected.add(
            signature(
                "queue hierarchy local",
                f"root/{side}/child",
                _source_occurrence(provider, *body(1), *body(8), ("field", "value")),
                {
                    "site": _source_occurrence(
                        provider, *body(1), *body(6), *body(0), *body(1)
                    ),
                    "invocation": [],
                    "ordinal": 0,
                },
            )
        )
        expected.add(
            signature(
                "queue hierarchy parent",
                "root",
                _source_occurrence(
                    bench, *body(2), *body(registration_index), ("field", "value")
                ),
                {
                    "site": _source_occurrence(
                        bench, *body(2), *body(callable_index), *body(1), *body(1)
                    ),
                    "invocation": [],
                    "ordinal": 0,
                },
            )
        )
    seen = set()
    for row in rows:
        if row["kind"] == "result":
            continue
        actual = signature(
            row["spec"]["event"],
            row["instance"],
            json.loads(row["registration"]),
            json.loads(row["site"]),
        )
        assert actual in expected
        seen.add(actual)
    assert seen == expected


def test_rule_queue_two_actual_instances_route_cross_level_pop_credit(tmp_path):
    source = tmp_path / "hierarchical queue sources"
    source.mkdir()
    provider_name = "rule_queue_hierarchy_provider.py"
    bench_name = "rule_queue_hierarchy_bench.py"
    for name in (provider_name, bench_name):
        shutil.copy2(FIXTURE / name, source)
    provider = tmp_path / "hierarchical queue provider"
    consumer = tmp_path / "hierarchical queue consumer"
    _ok(_compile(source, provider_name, provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "unavailable hierarchy provider body.ac"
    (source / provider_name).unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, bench_name, consumer, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "hierarchical-queue-system.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            "checks.rule_queue_hierarchy_bench.ExerciseQueueHierarchy",
            "-o",
            str(final),
        )
    )
    assert "UnselectedQueueHierarchy" in final.read_text()
    # Each physical Queue receives six values. Its local Rule drains on even
    # cycles and its ancestor Rule drains on odd cycles, using the same old-Q
    # u1 selector. All five pushes after cycle0 must replace a full old head.
    # Both instances share a definition but retain independent token streams.
    _execute(
        final,
        tmp_path,
        8,
        {
            "queue hierarchy local": [
                (2, 11),
                (2, 31),
                (4, 13),
                (4, 33),
                (6, 15),
                (6, 35),
            ],
            "queue hierarchy parent": [
                (1, 10),
                (1, 30),
                (3, 12),
                (3, 32),
                (5, 14),
                (5, 34),
            ],
        },
        _assert_hierarchy_observation_identities,
    )


def _assert_conditional_exchange_identities(rows):
    bench = "checks.rule_queue_exchange_bench.ExerciseConditionalQueueExchange"
    provider = "checks.rule_queue_exchange_provider.ConditionalQueueExchange"

    def body(index):
        return (("field", "body"), ("index", index))

    registration = _source_occurrence(bench, *body(2), *body(5), ("field", "value"))
    invocation = [
        {
            "call": _source_occurrence(
                bench,
                *body(2),
                *body(3),
                *body(0),
                ("field", "orelse"),
                ("index", 0),
                ("field", "value"),
            ),
            "receiver": {
                "instance_path": [_source_occurrence(bench, *body(2), *body(2))]
            },
        }
    ]
    expected = {
        event: {
            "site": _source_occurrence(
                provider, *body(1), *body(3), *body(3 + ordinal)
            ),
            "invocation": invocation,
            "ordinal": ordinal,
        }
        for ordinal, event in enumerate(
            ("queue conditional first", "queue conditional second")
        )
    }
    seen = set()
    for row in rows:
        if row["kind"] == "result":
            continue
        event = row["spec"]["event"]
        assert row["instance"] == "root/owner"
        assert json.loads(row["registration"]) == registration
        assert json.loads(row["site"]) == expected[event]
        seen.add(event)
    assert seen == set(expected)


@pytest.mark.parametrize("latency", [1, 3], ids=["both-full", "both-delayed"])
def test_rule_queue_conditional_two_queue_exchange_is_atomic(tmp_path, latency):
    source = tmp_path / "conditional queue exchange sources"
    source.mkdir()
    provider_name = "rule_queue_exchange_provider.py"
    bench_name = "rule_queue_exchange_bench.py"
    for name in (provider_name, bench_name):
        shutil.copy2(FIXTURE / name, source)
    provider_source = source / provider_name
    text = provider_source.read_text()
    assert text.count("depth=1, latency=1") == 2
    provider_source.write_text(
        text.replace("depth=1, latency=1", f"depth=1, latency={latency}")
    )
    provider = tmp_path / "conditional queue exchange provider"
    consumer = tmp_path / "conditional queue exchange consumer"
    _ok(_compile(source, provider_name, provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "unavailable exchange provider body.ac"
    provider_source.unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, bench_name, consumer, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "conditional-queue-exchange-system.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            "checks.rule_queue_exchange_bench.ExerciseConditionalQueueExchange",
            "-o",
            str(final),
        )
    )
    # Old heads start at10/40. Odd selected cycles increment each side; even
    # cycles swap the old heads before adding1/2. Every replacement restarts
    # both actual eligibility delays, so latency3 admits only every third cycle.
    samples = (
        [
            (1, 10, 40),
            (2, 42, 11),
            (3, 43, 13),
            (4, 15, 44),
            (5, 16, 46),
            (6, 48, 17),
            (7, 49, 19),
        ]
        if latency == 1
        else [(3, 10, 40), (6, 42, 11), (9, 43, 13), (12, 15, 44), (15, 16, 46)]
    )
    _execute(
        final,
        tmp_path,
        8 if latency == 1 else 17,
        {
            "queue conditional first": [(cycle, first) for cycle, first, _ in samples],
            "queue conditional second": [
                (cycle, second) for cycle, _, second in samples
            ],
        },
        _assert_conditional_exchange_identities,
    )
