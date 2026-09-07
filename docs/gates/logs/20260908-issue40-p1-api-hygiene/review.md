# Issue #40 P1 receiver-aware API hygiene

PYC415/PYC418 no longer use untyped text regexes. Python hygiene resolves only
canonical pycircuit imports, tracks aliases and lexical shadowing, propagates
CAS/Wire kinds through expressions, and conservatively joins conditional,
loop, exception, and match paths. Wire and unknown receivers fail closed while
CAS/Forward/State method forms remain valid.

JIT checks the evaluated receiver object before method lookup and evaluates an
Attribute receiver exactly once. Bound methods, reflective `getattr`, and
`functools.partial` chains cannot bypass the removed Wire-method diagnostic.

Fixtures cover misleading variable names, external lookalike imports, qualified
and local aliases, function/class/target shadowing, ternaries, break/else,
try/except, match guards and captures, comprehensions, direct Attribute access,
`getattr`, and partial application.
