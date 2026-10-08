# Nominal enum declarations and common hardware values

The common hardware IR and C++/Verilog emitters support nominal Enum payloads.
Python supports declarations, nominal type annotations, imports/reexports,
members, equality, selection, defaults and explicit conversion in both directions
through the same compile/link flow. Conversion from bits returns separate nominal
data and Boolean membership through a fixed two-name local binding.

## Python declarations and independent source units

```python
from enum import Enum
import pycircuit as ac

@ac.encoding(width=3)
class State(Enum):
    IDLE = 0
    WAIT = 3
    DONE = 5

@ac.module
def Identity(value: State) -> {"out": State}:
    return {"out": value}
```

The required width is a positive closed Integer expression. The default kind is
`"explicit"`; `"binary_sequential"`, `"binary_one_hot"` and `"gray_sequential"`
use uniformly bound `enum.auto()` members in declaration order. Import aliases
and namespace aliases are resolved by their actual binding. Conflicting imports,
shadowing, duplicate codes, Boolean codes, methods, mixins and class hooks reject.
Capture never executes the class, decorator or provider Python.

A file can contain only nominal declarations, reexports or no declarations.
Compile it independently and supply its published interface to consumers.
Link requires the complete provider-unit closure, and checks real provider
definitions against their interfaces. Reexports retain provider identity;
equal codes in different Enum declarations do not permit implicit conversion.

Final source ownership includes empty and facade units. C++ groups without
modules are header-only. RTL groups without modules have source maps with empty
generated membership; no placeholder RTL module is created. Both targets derive
this inventory from the same verified final artifact.

## Python values and defaults

`State.WAIT` creates a nominal member value. Equality and inequality require the
same Enum declaration and return Boolean values; other comparisons, arithmetic,
bitwise operations, truthiness and implicit numeric casts reject. Import aliases
retain nominal identity. Type classification follows actual bindings even when
a nominal declaration or alias has the same spelling as a builtin type.

Conditional expressions and rule branches select same-Enum values using existing
four-state selection. Known-invalid and X/Z carriers are ordinary data; selection
does not assert membership. Ports, nested structs, tables and persistent variables
retain nominal identity and use existing storage/transaction rules.

`ac.enum_to_bits(value)` accepts one positional Enum value and produces fixed
unsigned bits of exactly the declared Enum width, preserving value/known/Z planes.
Ordinary behavioral unsigned destination boundaries can subsequently zero-extend
that result. They cannot change the conversion's own width or silently narrow it.
Structural module and rule-formal boundaries keep their existing exact contracts.

Initializers and same-source struct defaults may use canonical members, including
members imported through a published interface. Every declared default is checked,
even when unused or overridden. Explicit `state: State = 0` or `Entry(state=0)`
rejects, including when code zero is declared. An omitted field without a default
uses recursive zero only if its Enum declares a zero-code member. Merely declaring
a field whose Enum lacks code zero remains legal; constructing its omitted value
does not. `table(init=0)` uses recursive zero and ignores field defaults, whereas
`table(init=Entry())` applies the constructor's defaults. General imported struct
constructor-default execution remains unsupported.

## Conversion from bits and two-result binding

Inside a supported module or rule body:

```python
decoded, valid = ac.enum_from_bits[State](raw)
```

The raw input must be authoritative fixed bits of exactly the Enum width. There
is no implicit Integer/Boolean cast, widening or truncation. The expression is
evaluated once; one common operation supplies both results. `decoded` preserves
every raw value/known/Z bit, including invalid encodings. `valid` is a separate
Boolean with the four-state membership function described below. Either result
can be used independently; ignoring membership adds no assertion or data repair.

The target must contain exactly two distinct ordinary names. Lists, nested or
starred targets, field/index targets and arbitrary tuple producers are unsupported.
Targets cannot shadow reserved names, declarations, imports, instances or module
inputs. Both destination binding boundaries are checked before either local is
published. The structural module slice retains immutable locals; existing
sequential locals in behavioral modules and rules retain their binding rules.
Pure rule formals may be rebound within those boundaries. A formal bound directly
to a persistent owner cannot be a tuple target, even if otherwise read-only;
decode into locals first, then use an ordinary assignment to propose an update.

Existing branch definite-assignment rules apply to each name. Branches merge the
two results independently: decoding valid codes 00 and 01 separately and joining
with an unknown condition may produce carrier 0X with membership 1. Membership
is not recomputed from that joined carrier. Using unknown membership as a write
guard retains the ordinary unknown-enable failure behavior; conversion itself
does not assert validity. General Python unpacking remains unsupported.

## Common hardware representation

`!ac.enum<"model.State">` names one `ac.enum` declaration with a positive closed
width and ordered member names/codes. Equal widths or codes do not make different
qualified names interchangeable. Codes have arbitrary precision, are unique,
nonnegative and fit the width. The declaration records explicit codes or verified
binary-sequential, binary-one-hot or Gray-sequential codes in declaration order.

| Operation | Meaning |
| --- | --- |
| `ac.enum.create` | Materialize the known code of a declared member. |
| `ac.enum.to_bits` | Expose the exact-width carrier, preserving all four-state planes. |
| `ac.enum.from_bits` | Preserve raw bits as the specified nominal carrier and return separate one-bit membership. |

From-bits permits known-invalid codes and X/Z. Ignoring membership neither changes
the carrier nor adds a check. Membership is the OR of existing four-state equality
comparisons against every member code. For two-bit members `00` and `01`, inputs
`1X` and `1Z` give known false; `0X`, `0Z` and `X0` give X. Logical member coverage
does not establish physical validity or justify removing an invalid-code fallback.

Enum remains a single nominal leaf in ports, nested structs, tables and generic
type bindings. The generated C++ member named `code` is not a source field path.
Existing layout, dependency and storage analysis applies. Both from-bits results
depend on the raw input. Different nominal types require explicit bit conversion;
bits arithmetic/comparison and table folds do not implicitly accept Enum values.
Selection can use explicit conversions around the existing four-state bits select.

C++ emits a distinct wrapper containing `gfsim::Bits<W>` and the existing hardware
traits. RTL emits a qualified packed typedef. Enum definitions precede struct
users in both targets. Existing DFFE and collection kernels retain old-Q Work,
enabled Xfer, hold, reset, discard and whole-system failure semantics. Invalid or
unknown data does not itself become an enable or membership fault.

Large member lists use balanced existing EQ/OR expressions in generated C++.
This avoids deep compiler expression nesting without changing the membership
truth function or adding a Runtime primitive. Target bit-capacity and generated
name checks cover every emitted Enum/Struct declaration, including unused ones.
See [generated naming](name-mangling.md) for collision rejection.

Native IR, execution and independent test evidence is bound under
`docs/gates/logs/20261004-nominal-enum/`. Declaration/import/provenance evidence is
under `docs/gates/logs/20261004-enum-source/`, together with the source-value
execution evidence. Case lowering has separate acceptance requirements;
common-IR execution alone does not establish source-language support.
