"""Save and authenticate the Queue pipeline's actual compilation stages."""

import re


def verify_stages(final, opt, run):
    text = final.read_text()
    encoded = re.search(r'ac.transactional_source = "([^"\n]*)"', text)
    assert encoded, "Final artifact must preserve normalized linked source"
    linked = final.with_name("linked.ac")
    linked.write_text(
        re.sub(r"\\([0-9a-fA-F]{2})", lambda m: chr(int(m[1], 16)), encoded[1])
    )
    top = re.search(r'ac.transactional_top = "([^"]+)"', text)[1]
    routed = final.with_name("routed.ac")
    refined = final.with_name("refined.ac")
    run([opt, linked, f"--ac-route-transactions=top={top}", "-o", routed])
    run([opt, routed, "--ac-lower-transactions", "-o", refined])
    run([opt, refined, "--ac-verify-hardware"])

    # Mutate actual executable edges, leaving source provenance untouched.
    # A well-typed FIFO with different capacity, or a pop driven by push fire,
    # must not be accepted as a refinement of the normalized transaction.
    for artifact in (routed, refined):
        lines = artifact.read_text().splitlines(keepends=True)
        index = next(
            i
            for i, line in enumerate(lines)
            if "ac.instance(" in line
            and "callee = @pycircuit.__builtins__.fifo_local," in line
        )
        original = lines[index]
        mutations = {
            "depth": original.replace("#ac.math_int<2>", "#ac.math_int<4>", 1),
        }
        call = re.search(r"ac.instance\(([^)]*)\)", original)
        operands = call[1].split(", ")
        assert len(operands) == 5 and operands[2] != operands[4]
        operands[4] = operands[2]
        mutations["pop"] = (
            original[: call.start(1)] + ", ".join(operands) + original[call.end(1) :]
        )
        for name, changed in mutations.items():
            assert changed != original
            lines[index] = changed
            bad = artifact.with_name(f"{artifact.stem}-wrong-{name}.ac")
            bad.write_text("".join(lines))
            run([opt, bad, "--ac-verify-hardware"], fails=True)
