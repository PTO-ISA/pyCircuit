"""Behavioral (stateful) source observation acceptance gate.

This fixture drives the two importers that can carry a source `log`/`report`
site and checks that they expose ONE contract:

* the pure (structural) importer republishes an observation from the module
  level, because `ac.observe` requires module placement;
* the behavioral importer carries the site on the rule result suffix, the same
  route a source assertion already takes.

`docs/reference/language.md:841-842` states that source observations "currently
diagnose at emission; they must not be silently dropped", so a site is only
acceptable if it is either published as `ac.observe` or rejected by name. The
two readings of one rule body must not disagree.

Scope note: native emission is not implemented here, so an
accepted observation still fails the `emit` stage with a diagnostic naming
`ac.observe`. That is recorded, not asserted away: the chain asserts that
`emit` either succeeds or names the unimplemented instrumentation op, which is
the fail-closed contract this fixture owns.
"""

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

# --------------------------------------------------------------------------
# Source templates.
# --------------------------------------------------------------------------

PRELUDE = """\
# ruff: noqa: N802
import pycircuit as ac
from pycircuit import log, report, rule


@ac.module
def Leaf(x: ac.u5) -> {"out": ac.u5}:
    return {"out": x}
"""

# Behavioral module: a nominal struct return plus a module-scope @ac.rule. Any
# A stateful design reaches the importer through this branch.
BEHAVIORAL = (
    PRELUDE
    + """\


@ac.struct
class LeafResult:
    out_y: ac.u8


@ac.rule
def leaf_update(state, in_x) -> LeafResult:
    old = state
    state = in_x
{body}
    return LeafResult(out_y=old)


@ac.module
def LeafUnit(in_x: ac.u8) -> LeafResult:
    r: ac.u8 = 0
    return leaf_update(r, in_x)
"""
)

# Pure module: no persistent declaration and a named mapping return. Only a
# nested `@rule` registered by a bare call reaches the structural importer.
PURE = (
    PRELUDE
    + """\


@ac.module
def LeafUnit(value: ac.u5) -> {"out": ac.u5}:
{body}
    return {"out": 0}
"""
)

PURE_BODY = """\
    @rule
    def observe():
        {statement}
    observe()"""

# Pure rule that also binds an instance. This is the shape whose observation
# statement the rule-body walker defers to the module-level instrumentation pass,
# so it is the coverage that keeps both readings of one rule body in step.
PURE_BINDING_BODY = """\
    child = Leaf()
    @rule
    def bind():
        child(x=value)
        {statement}
    bind()"""

PY_CALL = """\
import pycircuit as ac
from pycircuit import report


@ac.struct
class LeafResult:
    out_y: ac.u8


@ac.rule
def leaf_update(state, in_x) -> LeafResult:
    old = state
    report("leaf.r.q", old)
    return LeafResult(out_y=old)


@ac.module
def LeafUnit(in_x: ac.u8) -> LeafResult:
    r: ac.u8 = 0
    return leaf_update(r, in_x)
"""

# A two-instance parent with one stateful leaf instantiated twice.
TWO_INSTANCE = """\
# ruff: noqa: N802
import pycircuit as ac
from pycircuit import report


@ac.struct
class LeafResult:
    out_y: ac.u8


@ac.struct
class PairResult:
    y0: ac.u8
    y1: ac.u8


@ac.rule
def leaf_update(state, in_x) -> LeafResult:
    old = state
    state = in_x
    report("leaf.r.q", old)
    return LeafResult(out_y=old)


@ac.module
def Leaf(in_x: ac.u8) -> LeafResult:
    r: ac.u8 = 0
    return leaf_update(r, in_x)


@ac.module
def Pair(in_x: ac.u8) -> PairResult:
    u0 = Leaf(in_x)
    u1 = Leaf(in_x)
    return PairResult(y0=u0.out_y, y1=u1.out_y)
"""

# Behavioral observation sites that must be accepted, with the observation
# attributes the final design must carry.
ACCEPTED = {
    "report-alias": (
        '    report("leaf.r.q", old)',
        1,
        ['kind = "report"', 'spec = {name = "leaf.r.q"}'],
    ),
    "report-owner-formal": (
        '    report("leaf.r.q", state)',
        1,
        ['kind = "report"', 'spec = {name = "leaf.r.q"}'],
    ),
    "report-input-formal": (
        '    report("leaf.r.q", in_x)',
        1,
        ['kind = "report"', 'spec = {name = "leaf.r.q"}'],
    ),
    "log-one-item": (
        '    log("info", "leaf.r.q", old)',
        1,
        [
            'kind = "log"',
            'level = "info"',
            'event = "leaf.r.q"',
            'items = [{kind = "value", ordinal = 0 : i32}]',
        ],
    ),
    "log-two-items": (
        '    log("info", "leaf.r.q", old, in_x)',
        1,
        [
            'kind = "log"',
            'items = [{kind = "value", ordinal = 0 : i32}, '
            '{kind = "value", ordinal = 1 : i32}]',
        ],
    ),
    "log-literal-only": (
        '    log("info", "leaf.r.q", "seed")',
        1,
        ['kind = "log"', 'items = [{kind = "literal", text = "seed"}]'],
    ),
    "log-no-items": ('    log("info", "leaf.r.q")', 1, ['kind = "log"', "items = []"]),
    "two-reports": (
        '    report("leaf.r.a", old)\n' '    report("leaf.r.b", in_x)',
        2,
        ['spec = {name = "leaf.r.a"}', 'spec = {name = "leaf.r.b"}'],
    ),
    "report-then-log": (
        '    report("leaf.r.a", old)\n' '    log("warning", "leaf.r.b", in_x)',
        2,
        ['kind = "report"', 'kind = "log"'],
    ),
    # An assertion and an observation share the rule result suffix: the assert
    # obligation must stay on the trailing two results, which is where
    # HardwareSourceChecks.cpp locates it from.
    "report-with-assert": (
        '    assert in_x == 0, "seed"\n' '    report("leaf.r.q", old)',
        1,
        ['kind = "report"', 'spec = {name = "leaf.r.q"}'],
    ),
}

# Namespace-qualified observations use the same canonical provider authority.
ACCEPTED["attribute-report"] = (
    '    ac.report("leaf.r.q", old)',
    1,
    ['kind = "report"', 'spec = {name = "leaf.r.q"}'],
)

# Behavioral sites that must be rejected with the named diagnostic listed.
REJECTED = {
    "print": (
        '    print("leaf.r.q", old)',
        "unsupported behavioral rule statement 'Expr'",
    ),
    "unknown-callee": (
        "    other(in_x)",
        "unsupported behavioral rule statement 'Expr'",
    ),
    "log-keywords": (
        '    log("info", "leaf.r.q", old, flush=True)',
        "log observation does not accept keywords",
    ),
    "log-short": ('    log("info")', "log requires level, event and optional items"),
    "log-dynamic-level": (
        '    log("info", in_x, old)',
        "log requires a valid static level and event",
    ),
    "log-invalid-level": (
        '    log("trace", "leaf.r.q", old)',
        "log requires a valid static level and event",
    ),
    "report-dynamic-name": (
        "    report(in_x, old)",
        "report name must be a nonempty static string",
    ),
    "report-empty-name": (
        '    report("", old)',
        "report name must be a nonempty static string",
    ),
    "report-two-values": (
        '    report("leaf.r.q", old, in_x)',
        "report requires a static name and one value",
    ),
    "report-untypeable-value": (
        '    report("leaf.r.q", -1)',
        "cannot infer finite hardware expression type",
    ),
    "if-nested-report": (
        '    if in_x == 0:\n        report("leaf.r.q", old)',
        "report observation must be an unconditional rule statement",
    ),
    "if-nested-log": (
        '    if in_x == 0:\n        log("info", "leaf.r.q", old)',
        "log observation must be an unconditional rule statement",
    ),
    "match-nested-report": (
        "    match in_x:\n"
        "        case 0:\n"
        '            report("leaf.r.q", old)\n'
        "        case _:\n"
        '            report("leaf.r.other", old)',
        "report observation must be an unconditional rule statement",
    ),
    "match-nested-log": (
        "    match in_x:\n"
        "        case 0:\n"
        '            log("info", "leaf.r.q", old)\n'
        "        case _:\n"
        '            log("info", "leaf.r.other", old)',
        "log observation must be an unconditional rule statement",
    ),
}

# Statements written at a pure-rule site. The pure importer publishes a
# well-formed observation from the module level and rejects everything else with
# its own long-standing diagnostic.
PURE_SITES = {
    "report-value": 'report("leaf.r.q", value)',
    "report-literal": 'report("leaf.r.q", 1)',
    "report-dynamic-name": "report(value, 1)",
    "report-two-values": 'report("leaf.r.q", value, value)',
    "report-untypeable-value": 'report("leaf.r.q", -1)',
    "log-value": 'log("info", "leaf.r.q", value)',
    "log-two-values": 'log("info", "leaf.r.q", value, value)',
    "log-no-values": 'log("info", "leaf.r.q")',
    "log-literal-only": 'log("info", "leaf.r.q", "seed")',
    "log-keywords": 'log("info", "leaf.r.q", value, flush=True)',
    "log-short": 'log("info")',
    "log-invalid-level": 'log("trace", "leaf.r.q", value)',
    "log-dynamic-event": 'log("info", value, value)',
    "attribute-report": 'ac.report("leaf.r.q", value)',
    "print": 'print("leaf.r.q", value)',
    "unknown-callee": "other(value)",
}

# Statements whose acceptance the two importers must decide identically, written
# once at a pure-rule site and once in a behavioral rule body.
AGREEMENT = [
    "attribute-report",
    "report-value",
    "report-literal",
    "report-dynamic-name",
    "report-two-values",
    "report-untypeable-value",
    "log-value",
    "log-two-values",
    "log-no-values",
    "log-literal-only",
    "log-keywords",
    "log-short",
    "log-invalid-level",
    "log-dynamic-event",
]

# Statements that are not observations. Both importers must still reject them,
# but each keeps its own long-standing wording ("pure rule" versus "behavioral
# rule statement"), so only the decision is compared.
DECISION_ONLY = ["print", "unknown-callee"]

# The same source statement must reach the same op kind and spec from either
# importer.
AGREEMENT_SPELLING = {
    "report-value": (
        'report("leaf.r.q", {operand})',
        ['kind = "report"', 'spec = {name = "leaf.r.q"}'],
    ),
    "log-value": (
        'log("info", "leaf.r.q", {operand})',
        ['kind = "log"', 'level = "info"', 'event = "leaf.r.q"'],
    ),
    "log-literal-only": (
        'log("info", "leaf.r.q", "seed")',
        ['kind = "log"', 'items = [{kind = "literal", text = "seed"}]'],
    ),
}

# Instrumentation diagnostics that fail closed without dropping the site.
INSTRUMENTATION_DIAGNOSTICS = (
    "'ac.observe' op hardware instrumentation emission is not implemented",
    "'ac.expect' op hardware instrumentation emission is not implemented",
)


def main():
    parser = argparse.ArgumentParser()
    for name in ("repo", "source-compiler", "linker", "emitter", "scratch"):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    repo = Path(args.repo).resolve()
    scratch = Path(args.scratch).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="c-obs-3-", dir=scratch))
    source, units = root / "source", root / "units"
    source.mkdir()
    units.mkdir()
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python"),
        PYTHONDONTWRITEBYTECODE="1",
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
    )
    commands = []

    def run(command, code=0, diagnostic=None):
        command = list(map(str, command))
        result = subprocess.run(
            command, cwd=repo, env=env, capture_output=True, text=True, timeout=120
        )
        row = {
            "command": command,
            "exit_status": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
        commands.append(row)
        (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        if code is not None:
            assert result.returncode == code, row
        assert "Traceback" not in result.stderr, row
        if diagnostic:
            assert diagnostic in result.stderr, row
        return result

    def cli(*arguments, code=0, diagnostic=None):
        # `code=None` records the stage without constraining its exit status,
        # which is what a decision probe needs.
        return run(
            [sys.executable, "-m", "pycircuit.cli", *arguments],
            code=code,
            diagnostic=diagnostic,
        )

    def digests(text):
        return hashlib.sha256(text.encode()).hexdigest()

    def stem_of(name):
        # A published unit qualifies source declarations by the file stem, and a
        # stem cannot carry a dash.
        return name.replace("-", "_")

    def compile_source(name, text, package):
        path = source / (stem_of(name) + ".py")
        path.write_text(text)
        unit = units / (stem_of(name) + "-unit")
        cli(
            "compile",
            "-c",
            path,
            "--source-root",
            source,
            "--package-prefix",
            package,
            "-o",
            unit,
        )
        return path, unit

    def top_of(name, package, module):
        return f"{package}.{stem_of(name)}.{module}"

    def link(unit, name, package, module):
        final = units / (stem_of(name) + ".ac")
        cli("link", unit, "--top", top_of(name, package, module), "-o", final)
        return final

    def body_of(final):
        return final.read_text()

    # ----------------------------------------------------------------------
    # 1. Behavioral observation sites are accepted and publish ac.observe.
    # ----------------------------------------------------------------------
    behavioral_receipts = []
    for name, (statement, count, fragments) in sorted(ACCEPTED.items()):
        text = BEHAVIORAL.replace("{body}", statement)
        path, unit = compile_source("beh-" + name, text, "beh")
        final = link(unit, "beh-" + name, "beh", "LeafUnit")
        body = body_of(final)
        observed = body.count('"ac.observe"')
        assert observed == count, (name, observed, count, body)
        for fragment in fragments:
            assert fragment in body, (name, fragment, body)
        if name == "report-with-assert":
            assert body.count('"ac.expect"') == 1, name
        behavioral_receipts.append(
            {
                "case": name,
                "statement": statement,
                "source_sha256": digests(text),
                "compile_exit_status": 0,
                "link_exit_status": 0,
                "ac_observe": observed,
                "ac_expect": body.count('"ac.expect"'),
            }
        )

    # ----------------------------------------------------------------------
    # 2. The same statement reaches the same op from the pure importer. This is
    #    the property the deferred structural site exists to preserve.
    # ----------------------------------------------------------------------
    agreement_receipts = []
    for name, (spelling, fragments) in sorted(AGREEMENT_SPELLING.items()):
        pure_path, pure_unit = compile_source(
            "pure-" + name,
            PURE.replace(
                "{body}",
                PURE_BODY.replace("{statement}", spelling.format(operand="value")),
            ),
            "pure",
        )
        pure_final = link(pure_unit, "pure-" + name, "pure", "LeafUnit")
        pure_body = body_of(pure_final)
        beh_path, beh_unit = compile_source(
            "behpair-" + name,
            BEHAVIORAL.replace("{body}", "    " + spelling.format(operand="old")),
            "behpair",
        )
        beh_final = link(beh_unit, "behpair-" + name, "behpair", "LeafUnit")
        beh_body = body_of(beh_final)
        pure_observe = pure_body.count('"ac.observe"')
        beh_observe = beh_body.count('"ac.observe"')
        assert pure_observe == beh_observe == 1, (name, pure_observe, beh_observe)
        for fragment in fragments:
            assert fragment in pure_body, ("pure", name, fragment)
            assert fragment in beh_body, ("behavioral", name, fragment)
        agreement_receipts.append(
            {
                "case": name,
                "pure_observe": pure_observe,
                "behavioral_observe": beh_observe,
                "fragments": fragments,
            }
        )

    # ----------------------------------------------------------------------
    # 3. Pure and behavioral importers decide the same statement set identically
    #    and give observation-specific rejections verbatim.
    # ----------------------------------------------------------------------
    decision_receipts = []
    for name in AGREEMENT:
        statement = PURE_SITES[name]
        pure_status, pure_diag, pure_observe = probe(
            cli,
            source,
            units,
            "decision-pure-" + name,
            statement,
            "dpure",
            behavioral=False,
        )
        beh_statement = statement.replace("value", "old")
        beh_status, beh_diag, beh_observe = probe(
            cli,
            source,
            units,
            "decision-beh-" + name,
            beh_statement,
            "dbeh",
            behavioral=True,
        )
        assert pure_status == beh_status, (
            name,
            pure_status,
            beh_status,
            pure_diag,
            beh_diag,
        )
        assert pure_observe == beh_observe, (name, pure_observe, beh_observe)
        if pure_status != 0:
            assert pure_diag == beh_diag, (name, pure_diag, beh_diag)
        decision_receipts.append(
            {
                "case": name,
                "declared": statement,
                "pure_exit_status": pure_status,
                "behavioral_exit_status": beh_status,
                "pure_diagnostic": pure_diag,
                "behavioral_diagnostic": beh_diag,
                "ac_observe": pure_observe,
            }
        )
    for name in DECISION_ONLY:
        statement = PURE_SITES[name]
        pure_status, pure_diag, _ = probe(
            cli,
            source,
            units,
            "decision-pure-" + name,
            statement,
            "dpure",
            behavioral=False,
        )
        beh_status, beh_diag, _ = probe(
            cli,
            source,
            units,
            "decision-beh-" + name,
            statement.replace("value", "old"),
            "dbeh",
            behavioral=True,
        )
        assert pure_status == beh_status == 1, (name, pure_status, beh_status)
        assert pure_diag and beh_diag, (name, pure_diag, beh_diag)
        decision_receipts.append(
            {
                "case": name,
                "declared": statement,
                "pure_exit_status": pure_status,
                "behavioral_exit_status": beh_status,
                "pure_diagnostic": pure_diag,
                "behavioral_diagnostic": beh_diag,
                "diagnostic_wording_differs_by_design": pure_diag != beh_diag,
            }
        )

    # ----------------------------------------------------------------------
    # 4. Behavioral rejections are named and change no publication.
    # ----------------------------------------------------------------------
    rejection_receipts = []
    for name, (statement, diagnostic) in sorted(REJECTED.items()):
        text = BEHAVIORAL.replace("{body}", statement)
        path = source / (stem_of("reject-" + name) + ".py")
        path.write_text(text)
        target = units / (stem_of("reject-" + name) + "-unit")
        cli(
            "compile",
            "-c",
            path,
            "--source-root",
            source,
            "--package-prefix",
            "reject",
            "-o",
            target,
            code=1,
            diagnostic=diagnostic,
        )
        assert not target.exists(), name
        assert commands[-1]["stdout"] == "", name
        rejection_receipts.append(
            {
                "case": name,
                "statement": statement,
                "expected_diagnostic": diagnostic,
                "exit_status": 1,
                "source_sha256": digests(text),
            }
        )

    # ----------------------------------------------------------------------
    # 5. Pure structural sites: the module-level republish must keep covering
    #    every site the rule-body walker deferred, and everything else stays
    #    named-rejected.
    # ----------------------------------------------------------------------
    pure_receipts = []
    for name, statement in sorted(PURE_SITES.items()):
        text = PURE.replace("{body}", PURE_BODY.replace("{statement}", statement))
        path = source / (stem_of("pureonly-" + name) + ".py")
        path.write_text(text)
        target = units / (stem_of("pureonly-" + name) + "-unit")
        result = cli(
            "compile",
            "-c",
            path,
            "--source-root",
            source,
            "--package-prefix",
            "pureonly",
            "-o",
            target,
            code=None,
        )
        if result.returncode == 0:
            final = link(target, "pureonly-" + name, "pureonly", "LeafUnit")
            body = body_of(final)
            observed = body.count('"ac.observe"')
            assert observed == 1, (name, observed)
            pure_receipts.append(
                {
                    "case": name,
                    "declared": statement,
                    "exit_status": 0,
                    "ac_observe": observed,
                }
            )
            continue
        # A rejected site must name a reason; a silently dropped observation is
        # the one outcome this fixture exists to forbid.
        stderr = result.stderr
        assert "error:" in stderr, (name, stderr)
        assert not target.exists(), name
        pure_receipts.append(
            {
                "case": name,
                "declared": statement,
                "exit_status": 1,
                "diagnostic": stderr.strip().splitlines()[-1],
            }
        )

    # A pure rule that binds an instance and observes: this is the site the
    # rule-body walker defers, so it proves the module-level pass still publishes
    # exactly the statements the walker handed over.
    for name in ("report-deferred", "log-deferred"):
        statement = (
            'report("leaf.r.q", value)'
            if name == "report-deferred"
            else 'log("info", "leaf.r.q", value)'
        )
        text = PURE.replace(
            "{body}", PURE_BINDING_BODY.replace("{statement}", statement)
        )
        path = source / (stem_of("purebind-" + name) + ".py")
        path.write_text(text)
        target = units / (stem_of("purebind-" + name) + "-unit")
        cli(
            "compile",
            "-c",
            path,
            "--source-root",
            source,
            "--package-prefix",
            "purebind",
            "-o",
            target,
        )
        final = link(target, "purebind-" + name, "purebind", "LeafUnit")
        body = body_of(final)
        assert body.count('"ac.observe"') == 1, (name, body)
        assert body.count('"ac.instance"') == 1, (name, body)
        pure_receipts.append(
            {
                "case": name,
                "declared": statement,
                "exit_status": 0,
                "ac_observe": 1,
                "ac_instance": 1,
                "deferred_site": True,
            }
        )

    # ----------------------------------------------------------------------
    # 6. The four-stage chain: one stateful leaf
    #    instantiated twice, observed from the rule body.
    # ----------------------------------------------------------------------
    chain_path, chain_unit = compile_source("chain", TWO_INSTANCE, "chain")
    chain_final = link(chain_unit, "chain", "chain", "Pair")
    chain_body = body_of(chain_final)
    assert chain_body.count('"ac.observe"') == 1, chain_body
    # The shape keeps both instances and one state register distinct.
    assert chain_body.count('instance_name = "__pyc_call_0"') == 1, chain_body
    assert chain_body.count('instance_name = "__pyc_call_1"') == 1, chain_body
    assert chain_body.count('dffe, instance_name = "r"') == 1, chain_body
    chain = {
        "ac_observe": 1,
        "leaf_instances": 2,
        "state_registers": 1,
        "compile_exit_status": 0,
        "link_exit_status": 0,
    }
    for target in ("cpp", "verilog"):
        emitted = cli(
            "emit",
            chain_final,
            "--target",
            target,
            "-o",
            units / ("chain-" + target),
            code=None,
        )
        chain[target + "_exit_status"] = emitted.returncode
        if emitted.returncode != 0:
            # Native emission is separate. Fail-closed means the stage must name
            # the instrumentation op it could not lower instead of publishing a
            # design with the observation missing.
            assert any(
                diagnostic in emitted.stderr
                for diagnostic in INSTRUMENTATION_DIAGNOSTICS
            ), emitted.stderr
            chain[target + "_diagnostic"] = emitted.stderr.strip().splitlines()[-1]
            assert not (units / ("chain-" + target)).exists(), target

    (scratch / "candidate.json").write_text(
        json.dumps(
            {
                "scope": "behavioral/structural source observation acceptance",
                "behavioral_observations": behavioral_receipts,
                "pure_behavioral_agreement": agreement_receipts,
                "importer_decision_agreement": decision_receipts,
                "behavioral_rejections": rejection_receipts,
                "pure_sites": pure_receipts,
                "four_stage_chain": chain,
            },
            indent=2,
        )
        + "\n"
    )
    sys.stdout.write("behavioral source observation gate passed\n")


def probe(cli, source, units, name, statement, package, behavioral):
    """Compile one statement at one site and report its decision."""
    stem = name.replace("-", "_")
    if behavioral:
        text = BEHAVIORAL.replace("{body}", "    " + statement)
    else:
        text = PURE.replace("{body}", PURE_BODY.replace("{statement}", statement))
    module = "LeafUnit"
    path = source / (stem + ".py")
    path.write_text(text)
    target = units / (stem + "-unit")
    result = cli(
        "compile",
        "-c",
        path,
        "--source-root",
        source,
        "--package-prefix",
        package,
        "-o",
        target,
        code=None,
    )
    if result.returncode != 0:
        detail = result.stderr.strip().splitlines()
        return 1, (detail[-1].split("error: ")[-1] if detail else ""), None
    final = units / (stem + ".ac")
    cli("link", target, "--top", f"{package}.{stem}.{module}", "-o", final)
    return 0, "", final.read_text().count('"ac.observe"')


if __name__ == "__main__":
    main()
