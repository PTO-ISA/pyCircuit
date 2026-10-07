#!/usr/bin/env python3
"""Run one command and sample its POSIX process-group RSS.

The reported peak is the largest sampled sum of process RSS values in the
owned process group. It is a lower bound on the observed group footprint, not
an exact or exclusive-memory measurement: short-lived processes can be missed
and shared pages can be counted in multiple members.
"""

from __future__ import annotations

import math
import os
import shutil
import signal
import subprocess
import time
from pathlib import Path
from typing import Any


def _sample_process_group(
    ps: str, pgid: int
) -> tuple[list[dict[str, int]], str | None]:
    try:
        result = subprocess.run(
            [ps, "-axo", "pid=,pgid=,rss="],
            text=True,
            capture_output=True,
            check=False,
            timeout=5,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        return [], f"ps sample failed: {exc}"
    if result.returncode:
        detail = result.stderr.strip() or f"ps exited {result.returncode}"
        return [], f"ps sample failed: {detail}"

    members: list[dict[str, int]] = []
    for line in result.stdout.splitlines():
        columns = line.split()
        if len(columns) != 3:
            continue
        try:
            pid, observed_pgid, rss_kib = (int(column) for column in columns)
        except ValueError:
            continue
        if observed_pgid == pgid and rss_kib >= 0:
            members.append({"pid": pid, "rss_kib": rss_kib})
    return members, None


def run_sampled(
    argv: list[str],
    *,
    cwd: Path,
    env: dict[str, str],
    log_dir: Path,
    label: str,
    sample_interval_seconds: float = 0.05,
    timeout_seconds: float = 3600,
) -> dict[str, Any]:
    """Run argv to completion, saving stdout/stderr and sampling process-group RSS."""

    if not argv:
        raise ValueError("argv must not be empty")
    if not math.isfinite(sample_interval_seconds) or sample_interval_seconds <= 0:
        raise ValueError("sample interval must be finite and positive")
    if not math.isfinite(timeout_seconds) or timeout_seconds <= 0:
        raise ValueError("timeout must be finite and positive")

    log_dir.mkdir(parents=True, exist_ok=True)
    stdout_path = log_dir / f"{label}.stdout.txt"
    stderr_path = log_dir / f"{label}.stderr.txt"
    ps = shutil.which("ps") if os.name == "posix" else None
    posix_group = os.name == "posix"
    started = time.perf_counter_ns()
    sample_rows: list[dict[str, Any]] = []
    errors: list[str] = []
    intervals: list[float] = []
    timeout_hit = False
    group_term_sent = False
    group_kill_sent = False
    group_cleanup_seconds = 0.0
    group_members_remaining_after_kill: list[int] | None = None

    with stdout_path.open("wb") as stdout, stderr_path.open("wb") as stderr:
        process = subprocess.Popen(
            argv,
            cwd=cwd,
            env=env,
            stdout=stdout,
            stderr=stderr,
            start_new_session=posix_group,
        )
        pgid = process.pid if posix_group else None
        deadline = time.monotonic() + timeout_seconds
        last_sample_at: float | None = None
        next_sample_at = time.monotonic()

        while process.poll() is None:
            now = time.monotonic()
            if now >= deadline:
                timeout_hit = True
                if posix_group and pgid is not None:
                    try:
                        os.killpg(pgid, signal.SIGTERM)
                        group_term_sent = True
                    except ProcessLookupError:
                        pass
                else:
                    process.terminate()
                cleanup_started = time.monotonic()
                cleanup_deadline = cleanup_started + 1.0
                # The session leader may exit promptly while a child ignores
                # SIGTERM. Keep the owned process group under observation for
                # a bounded grace period even after the leader is reaped.
                while time.monotonic() < cleanup_deadline:
                    process.poll()
                    if posix_group and pgid is not None and ps:
                        members, error = _sample_process_group(ps, pgid)
                        if error:
                            if error not in errors:
                                errors.append(error)
                        elif not members:
                            break
                    elif process.poll() is not None:
                        break
                    remaining_grace = cleanup_deadline - time.monotonic()
                    if remaining_grace <= 0:
                        break
                    time.sleep(min(0.05, remaining_grace))
                if posix_group and pgid is not None:
                    try:
                        # Signal the group, not just the leader; an ignoring
                        # child can outlive a leader that honored SIGTERM.
                        os.killpg(pgid, signal.SIGKILL)
                        group_kill_sent = True
                    except ProcessLookupError:
                        pass
                    if ps:
                        post_kill_deadline = time.monotonic() + 1.0
                        while time.monotonic() < post_kill_deadline:
                            members, error = _sample_process_group(ps, pgid)
                            if error:
                                if error not in errors:
                                    errors.append(error)
                                break
                            group_members_remaining_after_kill = [
                                member["pid"] for member in members
                            ]
                            if not members:
                                break
                            remaining = post_kill_deadline - time.monotonic()
                            if remaining <= 0:
                                break
                            time.sleep(min(0.05, remaining))
                elif process.poll() is None:
                    process.kill()
                process.wait()
                group_cleanup_seconds = time.monotonic() - cleanup_started
                break

            if now >= next_sample_at:
                if posix_group and pgid is not None and ps:
                    members, error = _sample_process_group(ps, pgid)
                    if error:
                        if error not in errors:
                            errors.append(error)
                    elif members:
                        sampled_at = time.monotonic()
                        if last_sample_at is not None:
                            intervals.append(sampled_at - last_sample_at)
                        last_sample_at = sampled_at
                        sample_rows.append(
                            {
                                "elapsed_seconds": (time.perf_counter_ns() - started)
                                / 1_000_000_000,
                                "members": members,
                                "member_count": len(members),
                                "sum_rss_kib": sum(
                                    member["rss_kib"] for member in members
                                ),
                            }
                        )
                next_sample_at = now + sample_interval_seconds

            remaining = max(0.0, min(next_sample_at, deadline) - time.monotonic())
            try:
                process.wait(timeout=min(remaining, 0.1) if remaining else 0.001)
            except subprocess.TimeoutExpired:
                pass

        exit_status = process.wait()

    elapsed = (time.perf_counter_ns() - started) / 1_000_000_000
    unavailable_reason = None
    if not posix_group:
        unavailable_reason = "process-group RSS sampling is POSIX-only"
    elif not ps:
        unavailable_reason = "ps executable unavailable"
    elif not sample_rows:
        unavailable_reason = "no process-group members were observed during sampling"
    elif errors:
        unavailable_reason = "; ".join(errors)

    rss: dict[str, Any] = {
        "available": bool(sample_rows) and not errors,
        "method": "ps -axo pid=,pgid=,rss=; sum RSS of matching process-group members at each sample",
        "unit": "KiB",
        "requested_interval_seconds": sample_interval_seconds,
        "sample_count": len(sample_rows),
        "peak_sampled_sum_kib": max(
            (row["sum_rss_kib"] for row in sample_rows), default=None
        ),
        "peak_sampled_member_count": max(
            (row["member_count"] for row in sample_rows), default=None
        ),
        "observed_interval_min_seconds": min(intervals) if intervals else None,
        "observed_interval_max_seconds": max(intervals) if intervals else None,
        "unavailable_reason": unavailable_reason,
        "limitations": [
            "sampled group sum is a lower bound; a short-lived child can be missed",
            "RSS may count shared pages once for each process",
            "only processes remaining in the launched process group are included",
            "a detached/new-session or reparented process outside the group is not included",
            "sampling is not exact peak memory or exclusive physical memory",
        ],
    }
    return {
        "argv": argv,
        "cwd": str(cwd),
        "wall_seconds": elapsed,
        "exit_status": exit_status,
        "timed_out": timeout_hit,
        "timeout_cleanup": {
            "sigterm_sent_to_group": group_term_sent,
            "sigkill_sent_to_group": group_kill_sent,
            "cleanup_wait_seconds": group_cleanup_seconds,
            "remaining_pids_after_kill": group_members_remaining_after_kill,
        },
        "process_group": {
            "isolated_session": posix_group,
            "pgid": process.pid if posix_group else None,
        },
        "rss": rss,
        "stdout_log": str(stdout_path),
        "stderr_log": str(stderr_path),
    }
