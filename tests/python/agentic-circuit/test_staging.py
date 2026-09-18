"""Directory publication must work on POSIX and on Windows."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from agentic_circuit._staging import ArtifactStage


class CommitDirectoryTest(unittest.TestCase):
    def publish(self, destination: Path, text: str) -> None:
        with ArtifactStage(destination, expected=["a.txt"]) as stage:
            stage.write_text("a.txt", text)
            stage.commit_directory()

    def assert_no_staging_left(self, parent: Path) -> None:
        leftovers = sorted(
            path.name for path in parent.iterdir() if path.name.startswith(".agentic-stage-")
        )
        self.assertEqual([], leftovers)

    def test_publishes_then_replaces_on_posix_and_on_windows(self) -> None:
        """Both replacement strategies must publish the same closed tree.

        POSIX replaces an existing directory with an atomic exchange
        (renamex_np / renameat2). Windows has neither that primitive nor a
        rename that replaces a directory, so it moves the existing tree aside
        and then moves the new one in. The Windows branch only uses portable
        calls, so it is exercised here by forcing sys.platform.
        """

        with tempfile.TemporaryDirectory() as raw:
            parent = Path(raw)
            destination = parent / "out"

            self.publish(destination, "first")
            self.assertEqual("first", (destination / "a.txt").read_text())
            self.assert_no_staging_left(parent)

            self.publish(destination, "second")
            self.assertEqual("second", (destination / "a.txt").read_text())
            self.assert_no_staging_left(parent)

            forced = "linux" if sys.platform == "win32" else "win32"
            with mock.patch.object(sys, "platform", forced):
                self.publish(destination, "third")
            self.assertEqual("third", (destination / "a.txt").read_text())
            self.assert_no_staging_left(parent)


if __name__ == "__main__":
    unittest.main()
