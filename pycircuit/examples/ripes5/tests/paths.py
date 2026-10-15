"""Filesystem layout for Ripes5 models, fixtures, reports and local runs."""
from pathlib import Path

EXAMPLE = Path(__file__).resolve().parents[1]
ROOT = EXAMPLE.parents[2]
MODEL_DIR = EXAMPLE / 'model'
RESULTS = EXAMPLE / 'results'
FIXTURES = EXAMPLE / 'tests/fixtures'
RUNS = ROOT / 'pycircuit/builds/benchmarks/rv32imc'
PERFORMANCE_RUNS = ROOT / 'pycircuit/builds/benchmarks/rv32imc-performance'
AUDIT_RUNS = ROOT / 'pycircuit/builds/benchmarks/rv32imc-audit'
NATIVE_BUILD = ROOT / 'pycircuit/builds/ripes-native'
REVIEW_BUILD = ROOT / 'pycircuit/builds/ripes5'
