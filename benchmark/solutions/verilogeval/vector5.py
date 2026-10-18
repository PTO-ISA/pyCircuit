import pycircuit as ac


@ac.module
def Top(a: ac.u1, b: ac.u1, c: ac.u1, d: ac.u1, e: ac.u1) -> ac.bits[25]:  # noqa: N802
    out = ac.concat(
        ~(a ^ a),
        ~(a ^ b),
        ~(a ^ c),
        ~(a ^ d),
        ~(a ^ e),
        ~(b ^ a),
        ~(b ^ b),
        ~(b ^ c),
        ~(b ^ d),
        ~(b ^ e),
        ~(c ^ a),
        ~(c ^ b),
        ~(c ^ c),
        ~(c ^ d),
        ~(c ^ e),
        ~(d ^ a),
        ~(d ^ b),
        ~(d ^ c),
        ~(d ^ d),
        ~(d ^ e),
        ~(e ^ a),
        ~(e ^ b),
        ~(e ^ c),
        ~(e ^ d),
        ~(e ^ e),
    )
    return out
