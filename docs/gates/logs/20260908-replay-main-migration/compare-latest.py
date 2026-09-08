import json
import sys
from pathlib import Path

sys.path.insert(0, "tests/python/agentic-circuit/tools")
from replay_format import integer, read_replay  # noqa: E402

base = Path(".pycircuit_out/replay/verified-rob")
current = Path(".pycircuit_out/replay/main-migration/latest-verified")
results = []
for case, file in [
    ("single", "execution.pyctrace"),
    ("dual", "execution.pyctrace"),
    ("equivalence", "scan.pyctrace"),
    ("equivalence", "activation.pyctrace"),
]:
    a = read_replay(next(base.glob(f"{case}-*/{file}")))
    b = read_replay(next(current.glob(f"{case}-*/{file}")))
    ids = {
        str(integer(o["id"]))
        for o in a.manifest["objects"]
        if o.get("visual") in ("queue", "table")
    }

    def project(state, ids=ids):
        return {k: v for k, v in state.items() if k in ids}

    assert a.complete and b.complete
    assert project(a.initial) == project(b.initial), (case, "initial")
    assert len(a.commits) == len(b.commits)
    for i, (old, new) in enumerate(zip(a.commits, b.commits, strict=True)):
        assert (old["time"], old["delta"]) == (new["time"], new["delta"])
        assert project(old["changes"]) == project(new["changes"]), (case, i)
    assert a.events == b.events, (case, "events")
    assert project(a.final) == project(b.final)

    def topology(objects):
        result = {}
        for o in objects:

            def refs(singular, plural, o=o):
                x = o.get(plural, o.get(singular, []))
                return sorted(
                    integer(r["ref"]) for r in (x if isinstance(x, list) else [x])
                )

            result[integer(o["id"])] = (
                o["name"],
                o["path"],
                o["object_kind"],
                refs("input", "inputs"),
                refs("output", "outputs"),
                refs("table", "tables"),
            )
        return result

    assert topology(a.manifest["objects"]) == topology(b.manifest["objects"]), (
        case,
        "topology",
    )
    results.append(
        {
            "case": case,
            "trace": file,
            "boundaries": len(a.commits),
            "events": len(a.events),
            "equal": True,
        }
    )
sys.stdout.write(json.dumps(results, indent=2) + "\n")
