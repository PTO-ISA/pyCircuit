#!/usr/bin/env bash
# Repeat the canonical workspace NPU pipeline under unrelated roots and require
# byte-identical trace, result, statistics, event, replay, and Perfetto output.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd -P)
BUILD_DIR=${BUILD_DIR:-"$ROOT/.pycircuit_out/acir/dev-llvm22"}
RUNS=${RUNS:-3}
if [[ ! "$RUNS" =~ ^[2-9][0-9]*$ ]]; then
  echo "RUNS must be an integer of at least 2" >&2
  exit 2
fi
if [[ -x "$ROOT/.venv/bin/python" ]]; then
  PYTHON_BIN=${PYTHON_BIN:-"$ROOT/.venv/bin/python"}
else
  PYTHON_BIN=${PYTHON_BIN:-python3}
fi
export PYTHONPATH="$ROOT/python/agentic-circuit/src:$BUILD_DIR/python${PYTHONPATH:+:$PYTHONPATH}"

EXAMPLE="$ROOT/examples/agentic-circuit/workspaces/npu"
GFSIM_TESTS="$BUILD_DIR/bin/GfsimTests"
if [[ ! -x "$GFSIM_TESTS" ]]; then
  echo "missing GfsimTests at $GFSIM_TESTS" >&2
  exit 2
fi

TEMP_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/agentic-circuit-workspace.XXXXXX")
trap 'rm -rf "$TEMP_ROOT"' EXIT

copy_workspace() {
  "$PYTHON_BIN" - "$EXAMPLE" "$1" <<'PY'
import shutil
import sys
from pathlib import Path

source, destination = map(Path, sys.argv[1:])
shutil.copytree(
    source,
    destination,
    ignore=shutil.ignore_patterns("build", "__pycache__", "*.pyc"),
)
PY
}

hash_file() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{print $1}'
  else
    shasum -a 256 "$1" | awk '{print $1}'
  fi
}

compare_outputs() {
  local baseline=$1 candidate=$2
  for relative in \
    traces/pto-trace.json \
    artifacts/run/run-result.json \
    artifacts/run/stats.json \
    artifacts/run/events.jsonl \
    artifacts/perfetto.json; do
    cmp "$baseline/$relative" "$candidate/$relative"
  done
}

"$GFSIM_TESTS" \
  --gtest_filter='ShowcaseTest.LegalWorkOrdersHaveByteIdenticalCommittedResults:NpuDependencyTrackerTest.WorkPermutationPreservesQueuesStatisticsAndObservations:NpuTraceSourceTest.RejectsUnsupportedOpcodeWithoutCommit'

baseline=
for ((iteration = 1; iteration <= RUNS; ++iteration)); do
  workspace="$TEMP_ROOT/root-$iteration/unrelated/npu"
  mkdir -p "$(dirname "$workspace")"
  copy_workspace "$workspace"

  generated_trace="$TEMP_ROOT/generated-trace-$iteration.json"
  "$PYTHON_BIN" "$ROOT/compiler/acir/tools/import-davincioo-pto-trace.py" \
    "$workspace/traces/davincioo.jsonl" "$generated_trace" \
    --source-program examples/agentic-circuit/workspaces/npu
  cmp "$workspace/traces/pto-trace.json" "$generated_trace"

  (
    cd "$workspace"
    PYTHONHASHSEED=$((iteration * 37)) "$PYTHON_BIN" \
      -m agentic_circuit._cli run architecture.py \
      --project agentic-circuit.toml --trace traces/pto-trace.json \
      --stats-format json --event-log jsonl --expect-termination \
      --output-dir artifacts/run --json
  ) >"$TEMP_ROOT/run-$iteration.json"
  "$PYTHON_BIN" "$ROOT/compiler/acir/tools/pack-perfetto-trace.py" \
    "$workspace/artifacts/run/events.jsonl" \
    "$workspace/artifacts/perfetto.json"
  (
    cd "$workspace"
    PYTHONHASHSEED=$((iteration * 53)) "$PYTHON_BIN" \
      -m agentic_circuit._cli run \
      --replay-manifest artifacts/run/run-manifest.json \
      --output-dir artifacts/replay --json
  ) >"$TEMP_ROOT/replay-$iteration.json"
  for relative in run-result.json stats.json events.jsonl; do
    cmp "$workspace/artifacts/run/$relative" \
      "$workspace/artifacts/replay/$relative"
  done

  model=$(find "$workspace/build/main/builds" -type f -path '*/bin/model' -print -quit)
  if [[ -z "$model" || ! -x "$model" ]]; then
    echo "generated model executable is missing" >&2
    exit 1
  fi
  dependencies="$TEMP_ROOT/dependencies-$iteration.txt"
  if command -v otool >/dev/null 2>&1; then
    otool -L "$model" >"$dependencies"
  elif command -v ldd >/dev/null 2>&1; then
    ldd "$model" >"$dependencies"
  else
    echo "neither otool nor ldd is available for dependency scanning" >&2
    exit 2
  fi
  if grep -Eiq 'python|pybind|mlir|plugin' "$dependencies"; then
    echo "generated model has a forbidden runtime dependency" >&2
    cat "$dependencies" >&2
    exit 1
  fi
  if grep -R -Eiq \
    'Python(\.h)?|pybind|importlib|mlir/|dl(open|sym)|runtime_factory|plugin_(loader|registry)' \
    "$workspace/build/main/builds"/*/src/generated; then
    echo "generated source contains a forbidden dynamic dependency" >&2
    exit 1
  fi

  if [[ -z "$baseline" ]]; then
    baseline=$workspace
  else
    compare_outputs "$baseline" "$workspace"
  fi
done

for relative in \
  traces/pto-trace.json \
  artifacts/run/run-result.json \
  artifacts/run/stats.json \
  artifacts/run/events.jsonl \
  artifacts/perfetto.json; do
  printf '%s  %s\n' "$(hash_file "$baseline/$relative")" "$relative"
done
echo "workspace determinism audit: PASS ($RUNS runs)"
