"""Small independent corruption checks for native transactional anchors."""

import pathlib
import re
import subprocess
import sys

opt, source, routed, final, scratch = sys.argv[1:]
scratch = pathlib.Path(scratch)
scratch.mkdir(parents=True, exist_ok=True)


def rejected(name, text, passes):
    path = scratch / (name + ".mlir")
    path.write_text(text)
    # Establish syntactic/local validity separately from semantic rejection.
    subprocess.run([opt, str(path), "-o", str(path) + ".parsed"], check=True)
    result = subprocess.run([opt, str(path), *passes], capture_output=True, text=True)
    if result.returncode == 0:
        raise AssertionError(f"accepted {name}")


source_text = pathlib.Path(source).read_text()
# The 'no' registration now shares the 'yes' predicate: there is no priority.
conflict = source_text.replace('"ac.yield"(%stop)', '"ac.yield"(%go)')
assert conflict != source_text
rejected("overlap", conflict, ["--ac-route-transactions=top=StateCycles"])

# This slice diagnoses unresolved four-state control at backend admission.
unknown_control = source_text.replace(
    '%one = "ac.bits.constant"() {value = #n1} : () -> !b1',
    '%one = "ac.bits.constant4"() {pattern = "x"} : () -> !b1',
)
assert unknown_control != source_text
rejected(
    "unknown-control", unknown_control, ["--ac-route-transactions=top=StateCycles"]
)

text = pathlib.Path(routed).read_text()
updates = list(
    re.finditer(r'(?m)^\s*"ac\.state\.update"\((%[\w#]+), (%[\w#]+), (%[\w#]+)\)', text)
)
assert updates, "test requires serialized logical updates"
update = next(item for item in updates if item[1] != item[2])
state = re.search(r'(?m)^\s*%[\w#]+ = "ac\.state"\((%[\w#]+),', text)
assert state, "test requires logical storage endpoints"
for field, replacement in ((2, update[1]), (3, state[1])):
    assert update[field] != replacement
    mutated = text[: update.start(field)] + replacement + text[update.end(field) :]
    rejected(
        "data" if field == 2 else "enable",
        mutated,
        ["--ac-route-transactions"],
    )

# Final reload must authenticate the physical enable against source semantics.
text = pathlib.Path(final).read_text()
physical = re.search(
    r'(?m)^\s*%[\w#]+ = "?ac\.instance"?\((%[\w#]+), (%[\w#]+), (%[\w#]+), (%[\w#]+), (%[\w#]+)\)',
    text,
)
assert physical, "test requires refined enabled storage"
assert physical[1] != physical[3]
mutated = text[: physical.start(3)] + physical[1] + text[physical.end(3) :]
rejected("physical-enable", mutated, ["--ac-verify-hardware"])
