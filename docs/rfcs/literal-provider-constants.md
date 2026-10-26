# Immutable literal provider constants

Related issue: [#356](https://github.com/PTO-ISA/pyCircuit/issues/356).
This bounded source contract reuses existing static declarations, namespaces,
source-unit authority and hardware literal lowering. It does not execute host
Python or introduce a second constant evaluator.

## Source declarations

A top-level plain assignment may declare one immutable Integer or Boolean
literal. Integer literals may have one unary `+` or `-` sign. The target is one
ordinary name; uppercase spelling is conventional, not semantic authority.
Integer values retain arbitrary precision, and Boolean values remain distinct
from Integer `0` and `1`.

```python
WORD_BITS = 16
WRITE = 1
ENABLED = True
```

Constants are captured before dependent types and signatures. Declaration order
does not impose host execution order. Ordinary helper formals and locals retain
their lexical bindings; a global value cannot capture a formal accidentally.
Existing protected binding rules still apply to Module/Rule formals, static
parameters, imported bindings and reserved names.

Reassignment, duplicate public bindings, annotated globals, chained/destructuring
assignments, aliases, arithmetic expressions, calls and other literal kinds are
outside this profile. In particular, `WIDTH = 2 * 8` does not become an invitation
to execute Python. Broader host staging and parameterized types remain separate
work under #331.

## Publication and use

Each definition uses the existing source-owned `ac.constant` declaration with
its qualified symbol, owner, origin, declaration role, `StaticType` and
`StaticValue`. Integers use mathematical Integer attributes; Booleans use Boolean
attributes. No fixed-bit width is inferred for the declaration itself.

Ordinary direct, aliased and namespace imports resolve constants from explicitly
supplied published interfaces. Reexports retain the original symbol and owner.
Consumer compilation requires no provider source or implementation body. The
owning body is nevertheless mandatory at link, including definitions whose
values were already materialized or were never imported by a consumer.

At a real source use, the compiler reuses the ordinary literal lowering and its
logical-kind, interval and closed-value facts. Supported static uses such as
widths and parameter defaults become existing literal static expressions.
Boolean values do not become Integer widths. The importer neither fabricates a
literal AST node nor emits unsupported export references for later evaluation.

Native constant declarations retain their broader static-value capabilities.
The Python frontend's new binding route accepts only the Integer/Boolean
literal profile; importing an unsupported native list or record constant must
diagnose rather than reaching an unchecked scalar cast.

## Retirement boundary

Published source bodies retain full owning definitions and imported snapshots.
Header/body admission compares complete declarations before any transformation;
missing, stale, mismatched or duplicate providers reject.

After admission, linking uses a detached clone. Alongside existing source-helper
retirement, it preflights remaining semantic references to constant symbols,
then removes resolved constant declarations. Ordinary occurrence/provenance
references are not executable dependencies. Failure preserves the original
source units and any previously published final output.

Routed/final hardware admission stays closed: its executable graph contains no
`ac.constant`. Saved anchors retain constant-bearing source interfaces as
provenance and reload without original sources or source-unit directories.
Both backends consume the same verified hardware artifact; no backend, Runtime
or consumer-specific constant implementation is added.

## Verification

The independent source suite covers literal-only and mixed providers, all
supported import spellings, source removal, lexical shadowing, signed and wide
Integers, Boolean distinctions, static uses, malformed declarations, missing
owning definitions and failed replacement. A native valid static-call reference
in an unused parameter default exercises retirement rejection without inventing
unsupported Python syntax.

Generated Systems execute independent expected traces with C++ workers 1/2 and
RTL, including after source-absent anchor reload. Complete consumer providers
and models remain in their owning repository; their execution is separate
integration evidence, not a compiler admission rule.
