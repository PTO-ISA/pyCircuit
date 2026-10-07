#!/usr/bin/env python3
"""Run source-check tests with a wall timeout and process-tree RSS allowance."""

import argparse
import json
import math
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time

GUARD_ENV = "PYCIRCUIT_SOURCE_CHECK_RSS_GUARD"
GUARD_MARKER = "process-tree-rss-v1"


def tree_rss_kib(root_pid):
    snapshot = subprocess.run(
        ["ps", "-axo", "pid=,ppid=,rss="],
        capture_output=True,
        text=True,
        timeout=3,
        check=True,
    )
    rows = []
    for line in snapshot.stdout.splitlines():
        fields = line.split()
        if len(fields) != 3:
            raise ValueError("invalid process RSS snapshot")
        pid, parent, rss = map(int, fields)
        if pid <= 0 or parent < 0 or rss < 0:
            raise ValueError("invalid process RSS values")
        rows.append((pid, parent, rss))
    descendants = {root_pid}
    while True:
        expanded = descendants | {
            pid for pid, parent, _ in rows if parent in descendants
        }
        if expanded == descendants:
            break
        descendants = expanded
    return sum(rss for pid, _, rss in rows if pid in descendants)


def interrupted(_signum, _frame):
    raise KeyboardInterrupt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=float, default=60)
    parser.add_argument("--rss-mib", type=int, default=1536)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command:
        parser.error("command required")
    if not math.isfinite(args.timeout) or args.timeout <= 0 or args.rss_mib <= 0:
        parser.error("timeout and RSS allowance must be positive and finite")

    signal.signal(signal.SIGTERM, interrupted)
    started = time.monotonic()
    peak = 0
    reason = None
    status = None
    proc = None
    with tempfile.TemporaryFile(mode="w+") as output:
        try:
            environment = os.environ.copy()
            environment[GUARD_ENV] = GUARD_MARKER
            proc = subprocess.Popen(
                command,
                stdout=output,
                stderr=subprocess.STDOUT,
                env=environment,
                start_new_session=True,
            )
            while proc.poll() is None:
                if time.monotonic() - started > args.timeout:
                    reason = "wall timeout"
                    break
                rss = tree_rss_kib(proc.pid)
                peak = max(peak, rss)
                if rss > args.rss_mib * 1024:
                    reason = "process-tree RSS bound"
                    break
                if time.monotonic() - started > args.timeout:
                    reason = "wall timeout"
                    break
                time.sleep(0.05)
        except KeyboardInterrupt:
            reason = "guard interrupted"
        except Exception as error:
            reason = f"guard monitor failure: {type(error).__name__}: {error}"
        finally:
            # Includes normal command exit: no orphaned death-test descendants
            # may survive the monitored command or a failed ps invocation.
            if proc is not None:
                try:
                    os.killpg(proc.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                except Exception as error:
                    reason = f"guard cleanup failure: {type(error).__name__}: {error}"
                try:
                    status = proc.wait(timeout=5)
                except Exception as error:
                    reason = f"guard reap failure: {type(error).__name__}: {error}"
        output.seek(0)
        shutil.copyfileobj(output, sys.stdout)
    print(json.dumps({  # noqa: T201 - machine-readable guard JSON receipt
        "guard_reason": reason,
        "command_exit": status,
        "elapsed_seconds": round(time.monotonic() - started, 3),
        "peak_tree_rss_kib": peak,
        "rss_limit_mib": args.rss_mib,
        "timeout_seconds": args.timeout,
    }))
    if reason:
        return 124 if reason in {"wall timeout", "process-tree RSS bound"} else 125
    if status is None:
        return 125
    return status if status >= 0 else 128 - status


if __name__ == "__main__":
    sys.exit(main())
