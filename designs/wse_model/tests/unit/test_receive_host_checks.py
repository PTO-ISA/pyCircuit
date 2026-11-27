"""The host checker must reject incomplete, mismatched or unrelated failures."""

from copy import deepcopy

import pytest

from models.calendar.host_checks import check_run

pytestmark = pytest.mark.unit


def _success():
    result = {
        "accepted": True,
        "status": 1,
        "fault": False,
        "active": True,
        "counted": 0,
        "complete": False,
        "pending": 0,
    }
    expected = {"results": [result], "failure_cycle": None}
    run = {
        "status": "EXECUTED",
        "returncode": 0,
        "terminal": {"status": "TERMINATED", "epoch_time": "2"},
        "samples": [
            {"cycle": 0, "evaluation_epoch": i, "commit_epoch": i + 1, "result": deepcopy(result)}
            for i in range(2)
        ],
    }
    return run, expected


def test_accept_complete_matching_observations():
    run, expected = _success()
    check_run(run, expected, "cpp")


@pytest.mark.parametrize("mutation", ["missing", "value", "epoch", "exit", "build"])
def test_reject_incomplete_or_wrong_result(mutation):
    run, expected = _success()
    if mutation == "missing":
        run["samples"].pop()
    elif mutation == "value":
        run["samples"][1]["result"]["counted"] = 4
    elif mutation == "epoch":
        run["samples"][1]["commit_epoch"] = 3
    elif mutation == "exit":
        run["returncode"] = 1
    else:
        run["status"] = "BUILD_FAILED"
    with pytest.raises(AssertionError):
        check_run(run, expected, "cpp")


def test_unrelated_runtime_failure_is_not_capacity_success():
    expected = {
        "results": [],
        "failure_cycle": 0,
        "error": "WSE_MODEL_CAPACITY: dedup identity slots exhausted",
    }
    run = {
        "status": "EXECUTED",
        "returncode": 1,
        "samples": [],
        "terminal": {
            "status": "FAILED",
            "epoch_time": "0",
            "error": {"message": "unrelated assertion"},
        },
    }
    with pytest.raises(AssertionError):
        check_run(run, expected, "cpp")
