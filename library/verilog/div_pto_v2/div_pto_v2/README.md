# PTO scalar DIV/REM RTL primitive

This directory contains a PTO-aware wrapper around the imported BaseJump
iterative integer divider.  The BaseJump sources under `basejump/` are kept
unchanged; PTO architectural behavior is enforced at the wrapper boundary.

## Supported operation mapping

| PTO form | `is_signed` | `word_mode` | Architectural result selected by caller |
| --- | ---: | ---: | --- |
| DIV | 1 | 0 | quotient |
| DIVU | 0 | 0 | quotient |
| DIVW | 1 | 1 | quotient |
| DIVUW | 0 | 1 | quotient |
| REM | 1 | 0 | remainder |
| REMU | 0 | 0 | remainder |
| REMW | 1 | 1 | remainder |
| REMUW | 0 | 1 | remainder |

The primitive computes quotient and remainder together.  A higher layer selects
which output corresponds to the decoded mnemonic, so DIV and REM do not require
separate divider hardware.

## PTO semantics implemented here

The implementation follows `pto-spec/asl/scalar/model/alu/semantics.asl`:

- DIV/DIVU/REM/REMU use full XLEN operands.
- Signed W forms sign-extend each low 32-bit operand before division.
- Unsigned W forms zero-extend each low 32-bit operand before division.
- **Every W-form result, including DIVUW/REMUW, sign-extends result[31:0] to XLEN.**
- A zero divisor is a total arithmetic case: quotient = 0, remainder = dividend
  after the relevant operand normalization.
- Signed minimum divided by -1 is also total: quotient = signed minimum,
  remainder = 0.

Zero-divisor and signed-overflow cases bypass the iterative divider.  This both
fixes the architectural result at the canonical boundary and avoids spending a
full iterative divide latency on a result already known from the ISA semantics.

## New helper RTL

- `pyc_word_operand_normalize.sv`
  - full-width pass-through, signed low-word extension, or unsigned low-word
    extension selected at runtime.
  - This is **fixed-word normalization**, not wrapping-field normalization.
- `pyc_word_result_normalize.sv`
  - sign-extends low 32 bits for every W-form result as required by PTO.
- `pyc_div_special_cases.sv`
  - detects zero divisor and signed-minimum/-1 and generates raw quotient and
    remainder without invoking the iterative divider.
- `pyc_runtime_div.sv`
  - canonical ready/valid wrapper, special-case bypass, word-mode metadata
    retention, and one BaseJump divider instance.

## Files retained from the original package

The `basejump/` sources and `LICENSE.basejump` are retained from the supplied
package.  `BITS_PER_ITER` remains a structural parameter of the physical
implementation.  Values 1 and 2 are the supported BaseJump configurations.

## Directed testbench

`tests/tb_pyc_runtime_div.sv` contains directed cases for:

- normal signed and unsigned XLEN division/remainder,
- XLEN divide by zero,
- XLEN signed minimum / -1,
- DIVW signed behavior,
- DIVUW result sign extension,
- unsigned W zero-divisor remainder sign extension,
- signed W minimum / -1.

Run the directed suite from this directory with:

```sh
verilator --binary --timing --top-module tb_pyc_runtime_div \
  -Mdir <build-dir> -f tests/files_tb.f -Wno-fatal
<build-dir>/Vtb_pyc_runtime_div
```

The suite passes with Verilator 5.051.  `pyc_runtime_div` and
`pyc_runtime_div_packet` also pass Yosys 0.68 hierarchy, process lowering,
optimization, and structural checks.

## Interface note

Compared with the original wrapper, the canonical interface adds `word_mode`.
The existing `divide_by_zero` output is retained only as an informational /
diagnostic signal; PTO does not treat divide-by-zero as an architectural fault.
