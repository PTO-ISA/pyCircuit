"""Single rob_system expected-result scenarios and offline replay generation."""

import os
from pathlib import Path

import pytest

from designs.davincioo.spe.ooo.rob import rob_system
from designs.davincioo.tests.spe.ooo.test_rob import ROOT, generate_rob, verify_scenario

SCENARIOS = ("capacity", "backpressure", "invalid", "recovery")


@pytest.fixture(scope="module")
def generated_single_rob():
    out = Path(
        os.environ.get(
            "PYC_DAVINCIOO_SINGLE_ROB_OUT",
            ROOT / ".pycircuit_out/davincioo-rob/20260908-single",
        )
    ).resolve()
    return generate_rob(rob_system, out, SCENARIOS, single=True)


@pytest.mark.parametrize("scenario", SCENARIOS)
def test_single_rob_expected_results_and_replay(generated_single_rob, scenario):
    verify_scenario(generated_single_rob, scenario, flows=1, system="rob_system")
