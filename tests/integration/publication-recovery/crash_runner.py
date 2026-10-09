"""Test-only public CLI launcher that terminates at a real publication fault point."""

from __future__ import annotations

import os
import signal
import sys
from pathlib import Path

from pycircuit._publication_fs import _PublicationFileSystem


def main() -> int:
    arguments = sys.argv[1:]
    action: str | None = None
    fault_at: str | None = None
    trace_lock: Path | None = None
    while arguments and arguments[0].startswith("--"):
        option = arguments.pop(0)
        if option in {"--crash-at", "--pause-at"}:
            if not arguments:
                raise SystemExit(f"{option} requires a publication point")
            action = "crash" if option == "--crash-at" else "pause"
            fault_at = arguments.pop(0)
        elif option == "--trace-lock":
            if not arguments:
                raise SystemExit("--trace-lock requires a lock path")
            trace_lock = Path(arguments.pop(0)).resolve()
        else:
            raise SystemExit(f"unknown test-runner option: {option}")
    command = arguments
    if not command:
        raise SystemExit("a public pycircuit command is required")

    def terminate(point: str) -> None:
        if point != fault_at:
            return
        if action == "pause":
            os.write(sys.stdout.fileno(), f"paused-at:{point}\n".encode())
            os.kill(os.getpid(), signal.SIGSTOP)
        else:
            os.write(sys.stdout.fileno(), f"crashed-at:{point}\n".encode())
            os.kill(os.getpid(), signal.SIGKILL)

    if action is not None:
        _PublicationFileSystem.fault = lambda self, point: terminate(point)

    if trace_lock is not None:
        original_lock = _PublicationFileSystem.lock

        def traced_lock(self, path: Path, *, shared: bool):
            absolute = Path(path).resolve()
            if absolute != trace_lock:
                return original_lock(self, path, shared=shared)
            mode = "shared" if shared else "exclusive"
            os.write(sys.stdout.fileno(), f"attempt:{mode}\n".encode())
            lock = original_lock(self, path, shared=shared)

            class ObservedLock:
                def __enter__(self):
                    result = lock.__enter__()
                    os.write(sys.stdout.fileno(), f"acquired:{mode}\n".encode())
                    return result

                def __exit__(self, *exc_info: object):
                    return lock.__exit__(*exc_info)

            return ObservedLock()

        _PublicationFileSystem.lock = traced_lock

    from pycircuit.cli import main as pycircuit_main

    return pycircuit_main(command)


if __name__ == "__main__":
    raise SystemExit(main())
