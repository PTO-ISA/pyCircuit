#!/usr/bin/env bash
# Audit 5 helper: run the project canonicalization/freeze pipeline repeatedly
# and compare text/bytecode/digest hashes. One-off audit evidence generator,
# not part of the repo gate.
set -uo pipefail
cd "$(dirname "$0")/../.."
OPT=${OPT:-.pycircuit_out/acir/dev-llvm22/bin/acir-opt-internal}
CANON='builtin.module(ac-canonicalize-model)'
FREEZE='builtin.module(ac-canonicalize-model,ac-freeze-topology)'
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# Extract freezable sections from split-file tests (split on `//--- name`).
extract() { # file section-name outfile
  awk -v want="$2" '
    /^\/\/---/ { cur=$2; next }
    NR==1 && !/^\/\// { cur="" }
    { if (cur==want) print }
  ' "$1" > "$3"
}
extract tests/mlir/agentic-circuit/Transforms/freeze-topology.mlir valid.mlir "$TMP/freeze-valid.mlir"
extract tests/mlir/agentic-circuit/Transforms/deterministic-canonicalization.mlir a.mlir "$TMP/detcanon-a.mlir"
extract tests/mlir/agentic-circuit/Transforms/deterministic-canonicalization.mlir b.mlir "$TMP/detcanon-b.mlir"

# Group A: full canonicalize+freeze (text + bytecode + topology digest)
GROUP_A=(
  tests/mlir/agentic-circuit/ACIR/hierarchy-valid.mlir
  "$TMP/freeze-valid.mlir"
  "$TMP/detcanon-a.mlir"
  "$TMP/detcanon-b.mlir"
)
# Group B: canonicalize only (text + bytecode)
GROUP_B=(
  tests/mlir/agentic-circuit/ACIR/collections-valid.mlir
  tests/mlir/agentic-circuit/ACIR/resources-valid.mlir
  tests/mlir/agentic-circuit/ACIR/address-time-valid.mlir
  tests/mlir/agentic-circuit/ACIR/contracts-valid.mlir
  tests/mlir/agentic-circuit/ACIR/trace-valid.mlir
  tests/mlir/agentic-circuit/ACSim/ops-valid.mlir
  tests/mlir/agentic-circuit/ACSim/reusable-modules.mlir
)

fail=0
run_group() { # pipeline with-digest label files...
  local pipe="$1" want_digest="$2" label="$3"; shift 3
  for f in "$@"; do
    local name; name=$(basename "$f" .mlir)
    local text_hashes=() bc_hashes=() digests=()
    for i in 1 2 3 4 5; do
      local out rc
      out=$("$OPT" --verify-each=false --pass-pipeline="$pipe" "$f" 2>"$TMP/err")
      rc=$?
      if [ $rc -ne 0 ]; then
        echo "$name: pipeline failed (rc=$rc): $(head -1 "$TMP/err")"
        fail=1; continue 2
      fi
      text_hashes+=("$(printf '%s' "$out" | shasum -a 256 | cut -d' ' -f1)")
      if [ "$want_digest" = yes ]; then
        digests+=("$(printf '%s' "$out" | grep -o 'ac.topology_digest = "[^"]*"' | sort -u | tr '\n' ';')")
      fi
      "$OPT" --verify-each=false --pass-pipeline="$pipe" --emit-bytecode \
        -o "$TMP/$label-$name.$i.bc" "$f" 2>/dev/null
      bc_hashes+=("$(shasum -a 256 "$TMP/$label-$name.$i.bc" | cut -d' ' -f1)")
    done
    local u_text u_bc u_dig status=OK
    u_text=$(printf '%s\n' "${text_hashes[@]}" | sort -u | wc -l | tr -d ' ')
    u_bc=$(printf '%s\n' "${bc_hashes[@]}" | sort -u | wc -l | tr -d ' ')
    u_dig=1
    if [ "$want_digest" = yes ]; then
      u_dig=$(printf '%s\n' "${digests[@]}" | sort -u | wc -l | tr -d ' ')
      if [ -z "${digests[0]}" ]; then u_dig=0; fi
    fi
    if [ "$u_text" != 1 ] || [ "$u_bc" != 1 ] || [ "$u_dig" != 1 ]; then
      status=FAIL; fail=1
    fi
    printf '%-22s [%s] text:%s bc:%s digest:%s  %s\n' \
      "$name" "$label" "$u_text" "$u_bc" "$u_dig" "$status"
  done
}

run_group "$FREEZE" yes freeze "${GROUP_A[@]}"
run_group "$CANON" no canon "${GROUP_B[@]}"
exit $fail
