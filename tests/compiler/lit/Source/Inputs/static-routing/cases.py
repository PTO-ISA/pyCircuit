"""Source cases derived from the static-reference contract, not compiler output."""

from pathlib import Path
import runpy

FIXTURES = Path(__file__).resolve().parent
IMPORT = "import pycircuit as ac\n"
PEER = '''
@ac.module
def Peer():
    @ac.method
    def read() -> ac.u8:
        return 7
'''
TOP = '''
@ac.system
def Top():
    peer = Peer()
'''


def source_cases():
    ancestor = (FIXTURES / "ancestor.py").read_text()
    parent_start = ancestor.index("@ac.module\ndef Parent")
    child_start = ancestor.index("@ac.module\ndef Child")
    top_start = ancestor.index("@ac.system")
    reverse = (ancestor[:parent_start] + ancestor[child_start:top_start]
               + ancestor[parent_start:child_start] + ancestor[top_start:])
    cases = [
        ("ancestor-parent-first", [("design.py", ancestor)], "design.Top"),
        ("ancestor-child-first", [("design.py", reverse)], "design.Top"),
        ("late-failure", [("design.py", (FIXTURES / "late_failure.py").read_text())], "design.Top"),
    ]
    partial = (FIXTURES / "partial_writes.py").read_text()
    cases.extend([
        ("partial-writes", [("design.py", partial)], "design.Top"),
        ("partial-late-failure", [("design.py", partial.replace("assert phase < 255", "assert phase == 0"))], "design.Top"),
        ("reference-resources", [("design.py", (FIXTURES / "resources.py").read_text())], "design.Top"),
        ("indexed-demand", [("design.py", (FIXTURES / "indexed_demand.py").read_text())], "design.Exercise"),
        ("indexed-demand-conjunction", [("design.py", (FIXTURES / "indexed_demand.py").read_text().replace(
            "        if choose:", "        if choose and True:"))], "design.Exercise"),
    ])
    for crossbranch in (False, True):
        worker = (FIXTURES / "worker.py").read_text()
        composition = (FIXTURES / "composition.py").read_text()
        if crossbranch:
            worker = worker.replace('"..", "peer"', '"..", "..", "remote", "peer"')
            worker = worker.replace('"..", "distinct"', '"..", "..", "remote", "distinct"')
            composition = composition.replace(
                "    caller = Worker()\n", "\n\n@ac.module\ndef Branch():\n    caller = Worker()\n")
            composition = composition.replace("    parent = Parent()", "    remote = Parent()\n    branch = Branch()")
        cases.append(("crossbranch" if crossbranch else "sibling", [
            ("counter.py", (FIXTURES / "counter.py").read_text()),
            ("worker.py", worker), ("composition.py", composition)], "composition.Top"))
    cases.append(("memory-late-failure", [("design.py",
                  (FIXTURES / "memory_late_failure.py").read_text())], "design.Top"))
    memory_source = runpy.run_path(str(FIXTURES / "memory_cases.py"))["source"]
    for shape, profile, repeated, topology in (
        (0, "byte", False, "ancestor"), (1, "byte", False, "sibling"),
        (2, "byte", False, "crossbranch"), (2, "byte", True, "repeated"),
        (2, "sync", False, "crossbranch"), (2, "nominal", False, "crossbranch"),
        (2, "generic", False, "crossbranch"),
    ):
        cases.append(("memory-" + profile + "-" + topology,
                      [("design.py", memory_source(shape, profile, repeated))], "design.Top"))
    declarations = {
        "unused-missing-topology": '    unused = ac.instance_ref(Peer, "..", "absent")\n',
        "unused-alias": '    unused = ac.instance_ref(Peer, "..", "absent")\n    alias = unused\n',
        "used-descendant": '''    ref = ac.instance_ref(Peer, "peer")
    @ac.rule
    def check() -> None:
        assert ref.read() == 7, "descendant"
    check()
''',
    }
    for name, declaration in declarations.items():
        cases.append((name, [("design.py", IMPORT + PEER + TOP + declaration)], "design.Top"))
    # Retained noncontextual definitions use declaration-local proof at link;
    # they must never become fabricated selected-root occurrences.
    unused = '''
@ac.module
def Unused():
    child = Peer()
    ref = ac.instance_ref(Peer, "child")
    @ac.method
    def read() -> ac.u8:
        return ref.read()
'''
    cases.append(("unused-descendant-definition", [("design.py", IMPORT + PEER + unused + TOP)], "design.Top"))
    unused_self = '''
@ac.module
def Unused():
    ref = ac.instance_ref(Unused)
    @ac.method
    def read() -> ac.u8:
        return 11
    @ac.method
    def via_ref() -> ac.u8:
        return ref.read()
'''
    cases.append(("unused-self-definition", [("design.py", IMPORT + PEER + unused_self + TOP)], "design.Top"))
    return cases


def rejection_cases():
    # Every malformed declaration is checked even when it has no call demand.
    declarations = {
        "computed-path": ('    path = "peer"\n    ref = ac.instance_ref(Peer, path)\n', "literal"),
        "interior-parent": ('    ref = ac.instance_ref(Peer, "peer", "..")\n', "parent"),
        "dot-path": ('    ref = ac.instance_ref(Peer, ".")\n', "identifier"),
        "absolute-path": ('    ref = ac.instance_ref(Peer, "/peer")\n', "identifier"),
        "wildcard-path": ('    ref = ac.instance_ref(Peer, "*")\n', "identifier"),
        "non-module-provider": ('    ref = ac.instance_ref(ac.u8, "peer")\n', "Module"),
        "system-provider": ('    ref = ac.instance_ref(Top)\n', "ordinary Module"),
        "primitive-provider": ('    ref = ac.instance_ref(ac.queue, "peer")\n', "Module"),
        "annotated-reference": ('    ref: ac.u8 = ac.instance_ref(Peer, "peer")\n', "unannotated"),
        "rebind-reference": ('    ref = ac.instance_ref(Peer, "peer")\n    ref = 0\n', "immutable"),
        "rebind-alias": ('    ref = ac.instance_ref(Peer, "peer")\n    alias = ref\n    alias = 0\n', "immutable"),
        "reference-payload": ('    ref = ac.instance_ref(Peer, "peer")\n    value: ac.u8 = ref\n', "unannotated"),
        "reference-return": ('    ref = ac.instance_ref(Peer, "peer")\n    @ac.method\n    def read() -> ac.u8:\n        return ref\n', "receiver"),
        "conditional-reference": ('    if True:\n        ref = ac.instance_ref(Peer, "peer")\n', "Module-body"),
        "rule-local-reference": ('    @ac.rule\n    def step() -> None:\n        ref = ac.instance_ref(Peer, "peer")\n    step()\n', "Module-body"),
        "shadow-reference-formal": ('    ref = ac.instance_ref(Peer, "peer")\n    @ac.method\n    def read(ref: ac.u8) -> ac.u8:\n        return ref\n', "shadow"),
        "empty-wrong-provider": ('    ref = ac.instance_ref(Peer)\n', "current Module"),
    }
    cases = [(name, IMPORT + PEER + TOP + text, diagnostic)
             for name, (text, diagnostic) in declarations.items()]
    shadow = '''
def instance_ref(value: ac.u8) -> ac.u8:
    return value
@ac.system
def Top():
    ref = instance_ref(Peer)
'''
    cases.append(("untrusted-intrinsic-spelling", IMPORT + PEER + shadow, ""))
    cases.append(("top-level-reference", IMPORT + PEER
                  + 'ref = ac.instance_ref(Peer, "peer")\n' + TOP, "top level"))
    return cases


def link_rejection_cases():
    used = '''
    ref = ac.instance_ref(Peer, "peer")
    @ac.rule
    def check() -> None:
        assert ref.read() == 7, "topology required"
    check()
'''
    base = IMPORT + PEER + TOP + used
    wrong = '''
@ac.module
def Other():
    @ac.method
    def read() -> ac.u8:
        return 7
'''
    contextual = '''
@ac.module
def Caller():
    ref = ac.instance_ref(Peer, "..", "peer")
    @ac.method
    def read() -> ac.u8:
        return ref.read()
    @ac.rule
    def step() -> None:
        assert ref.read() == 7, "contextual caller"
    step()
'''
    return [
        ("missing-target", base.replace('Peer, "peer"', 'Peer, "missing"')),
        ("ancestor-escape", base.replace('Peer, "peer"', 'Peer, "..", "peer"')),
        ("wrong-actual-module", IMPORT + PEER + wrong + TOP.replace("peer = Peer()", "peer = Other()") + used),
        ("dead-branch-missing-target", base.replace('Peer, "peer"', 'Peer, "missing"').replace(
            '        assert ref.read() == 7, "topology required"',
            '        if False:\n            assert ref.read() == 7, "topology required"')),
        ("unused-contextual-definition", IMPORT + PEER + '''
@ac.module
def Unused():
    ref = ac.instance_ref(Peer, "..", "peer")
    @ac.method
    def read() -> ac.u8:
        return ref.read()
''' + TOP),
        ("unused-missing-descendant", IMPORT + PEER + '''
@ac.module
def Unused():
    ref = ac.instance_ref(Peer, "missing")
    @ac.method
    def read() -> ac.u8:
        return ref.read()
''' + TOP),
        ("unused-parent-instantiation", IMPORT + PEER + contextual + '''
@ac.module
def ParentA():
    peer = Peer()
    caller = Caller()
@ac.module
def ParentB():
    peer = Peer()
    caller = Caller()
@ac.system
def Top():
    selected = ParentA()
'''),
        ("unused-zero-depth-transitive-context", IMPORT + PEER + contextual + '''
@ac.module
def Unused():
    peer = Peer()
    caller = Caller()
    ref = ac.instance_ref(Caller, "caller")
    @ac.method
    def read() -> ac.u8:
        return ref.read()
''' + TOP),
    ]
