"""Source-closure import policy for captured Agentic Circuit designs.

Issue #158: the closure allowed only ``__future__``, ``agentic_circuit`` and
``enum`` as external imports, so a multi-file model that imported
``collections.abc`` (or ``typing``, ``dataclasses``, ``pathlib``) failed the whole
``workspace=`` capture with an opaque ``ACPY-JIT-006``. The closure only has to
produce deterministic source text, and a standard library import introduces no
un-captured workspace source, so the stdlib is importable apart from an explicit
denylist of modules that can smuggle host state, platform state, nondeterminism,
or dynamic code execution into elaboration.
"""

from __future__ import annotations

import textwrap
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from agentic_circuit._source_closure import (
    SourceClosureError,
    capture_source_closure,
)


class SourceClosureTestCase(unittest.TestCase):
    """Shared workspace builder for one capture per (sub)test."""

    root: Path

    def setUp(self) -> None:
        self.workspace = TemporaryDirectory()
        self.addCleanup(self.workspace.cleanup)
        self.root = Path(self.workspace.name).resolve()

    def write(self, relative: str, source: str) -> Path:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(textwrap.dedent(source), encoding="utf-8")
        return path

    def package(self, statement: str, *, name: str = "pkg") -> Path:
        self.write(f"{name}/__init__.py", "")
        self.write(
            f"{name}/types.py",
            f"""
            from __future__ import annotations

            {statement}

            import agentic_circuit as ac


            @ac.struct
            class S:
                lane: ac.u8
            """,
        )
        return self.write(
            f"{name}/top.py",
            f"""
            from __future__ import annotations

            import agentic_circuit as ac

            from {name}.types import S


            @ac.system
            def top() -> None:
                q = ac.source(S, depth=2, latency=1)
                ac.sink(q)
            """,
        )

    def capture(self, entry: Path, workspace: Path | None = None):
        return capture_source_closure(entry, workspace or self.root)

    def capture_after(self, statement: str) -> SourceClosureError:
        """Return the rejection raised for *statement*, or fail the test."""

        entry = self.package(statement)
        with self.assertRaises(SourceClosureError) as caught:
            self.capture(entry)
        return caught.exception

    def captured_paths(self, statement: str) -> list[str]:
        entry = self.package(statement)
        return [item.path for item in self.capture(entry).entries]


class StandardLibraryImportTest(SourceClosureTestCase):
    def test_standard_library_imports_are_captured(self) -> None:
        for statement in (
            "from collections.abc import Iterable",
            "import typing",
            "import math",
            "from dataclasses import dataclass",
            "from functools import partial",
            "from typing import Any, Final",
        ):
            with self.subTest(statement=statement):
                self.setUp()
                self.assertEqual(
                    ["pkg/__init__.py", "pkg/top.py", "pkg/types.py"],
                    self.captured_paths(statement),
                )

    def test_denied_standard_library_modules_fail_closed(self) -> None:
        for module in (
            "random",
            "time",
            "os",
            "pathlib",
            "subprocess",
            "ctypes",
            "pickle",
            "inspect",
            "sys",
            "uuid",
            "socket",
            "io",
        ):
            for statement in (f"import {module}", f"from {module} import x"):
                with self.subTest(statement=statement):
                    self.setUp()
                    message = str(self.capture_after(statement))
                    self.assertIn("ACPY-JIT-006", message)
                    self.assertIn("standard library import", message)
                    self.assertIn(repr(module), message)

    def test_unknown_external_import_is_still_rejected(self) -> None:
        message = str(self.capture_after("import numpy"))
        self.assertIn("external import 'numpy' is not allowed", message)

    def test_private_external_imports_are_rejected(self) -> None:
        for statement in (
            "from collections import _abc",
            "import collections._abc",
            "from agentic_circuit import _types",
            "from typing import _SpecialForm",
        ):
            with self.subTest(statement=statement):
                self.setUp()
                self.assertIn(
                    "private external import", str(self.capture_after(statement))
                )

    def test_future_import_stays_allowed(self) -> None:
        self.assertEqual(3, len(self.captured_paths("from __future__ import annotations")))

    def test_enum_imported_names_stay_restricted(self) -> None:
        self.assertIn(
            "enum imports are restricted",
            str(self.capture_after("from enum import Flag")),
        )
        self.assertEqual(
            3,
            len(self.captured_paths("from enum import Enum, IntEnum, auto, unique")),
        )


class WorkspaceRootContractTest(SourceClosureTestCase):
    def test_workspace_must_contain_the_root_package(self) -> None:
        entry = self.package("import math")
        self.assertEqual(3, len(self.capture(entry, self.root).entries))

        with self.assertRaises(SourceClosureError) as caught:
            self.capture(entry, self.root / "pkg")
        message = str(caught.exception)
        self.assertIn("root package 'pkg'", message)
        self.assertIn(str(self.root), message)
        self.assertIn("workspace= must be the directory that contains", message)

    def test_hint_does_not_fire_for_a_genuinely_external_import(self) -> None:
        self.write("flat/mod.py", "import numpy\n")
        with self.assertRaises(SourceClosureError) as caught:
            self.capture(self.root / "flat" / "mod.py", self.root / "flat")
        message = str(caught.exception)
        self.assertNotIn("root package", message)
        self.assertIn("external import 'numpy' is not allowed", message)

    def test_package_relative_imports_resolve_from_the_parent_workspace(self) -> None:
        entry = self.package("import math")
        self.write(
            "pkg/sibling.py",
            """
            import agentic_circuit as ac


            @ac.struct
            class T:
                lane: ac.u8
            """,
        )
        self.write(
            "pkg/consumer.py",
            """
            from __future__ import annotations

            import agentic_circuit as ac

            from pkg.sibling import T


            @ac.system
            def top() -> None:
                q = ac.source(T, depth=2, latency=1)
                ac.sink(q)
            """,
        )
        closure = self.capture(self.root / "pkg" / "consumer.py", self.root)
        self.assertIn("pkg/sibling.py", [item.path for item in closure.entries])

        # The same entry produced the misleading "external import 'pkg.sibling'"
        # text when the package root itself was passed as the workspace.
        with self.assertRaises(SourceClosureError) as caught:
            self.capture(self.root / "pkg" / "consumer.py", self.root / "pkg")
        self.assertIn("root package 'pkg'", str(caught.exception))

    def test_entry_must_live_inside_the_workspace(self) -> None:
        entry = self.package("import math")
        self.write("workspace/keep.py", "")
        with self.assertRaises(SourceClosureError) as caught:
            self.capture(entry, self.root / "workspace")
        self.assertIn("outside workspace", str(caught.exception))


class LocalModulePrecedenceTest(SourceClosureTestCase):
    """A workspace module shadows a standard-library module of the same name.

    The workspace precedes the standard library on ``sys.path``, so
    ``from queue import Item`` in a workspace that owns ``queue.py`` means the
    local file. Admitting the standard library by name first silently resolved
    the standard-library module instead: the closure captured only the entry
    file, and the payload type was then unknown to the frontend.
    """

    def shadow(self, module: str, entry_statement: str) -> list[str]:
        self.write(
            f"{module}.py",
            """
            import agentic_circuit as ac


            @ac.struct
            class Item:
                lane: ac.u8
            """,
        )
        entry = self.write(
            "top.py",
            f"""
            from __future__ import annotations

            import agentic_circuit as ac

            {entry_statement}


            @ac.system
            def top() -> None:
                q = ac.source(Item, depth=2, latency=1)
                ac.sink(q)
            """,
        )
        return [item.path for item in self.capture(entry).entries]

    def test_local_module_shadows_an_admitted_standard_library_name(self) -> None:
        for module in ("queue", "sched", "typing", "dataclasses"):
            with self.subTest(module=module):
                self.setUp()
                # The closure is deterministic and ordered by workspace path.
                self.assertEqual(
                    sorted([f"{module}.py", "top.py"]),
                    self.shadow(module, f"from {module} import Item"),
                )

    def test_local_module_shadows_a_denied_standard_library_name(self) -> None:
        # The denylist exists to stop an *uncaptured* module from changing
        # elaboration. A workspace-local `os.py` is captured source, so it is
        # used exactly as the interpreter would use it.
        self.assertEqual(["os.py", "top.py"], self.shadow("os", "from os import Item"))

    def test_module_qualified_local_import_still_reports_the_binding_rule(self) -> None:
        self.write("queue.py", "import agentic_circuit as ac\n")
        entry = self.write(
            "top.py",
            """
            import agentic_circuit as ac
            import queue
            """,
        )
        with self.assertRaises(SourceClosureError) as caught:
            self.capture(entry)
        message = str(caught.exception)
        self.assertIn("module-qualified local import 'queue'", message)
        self.assertIn("import explicit symbols", message)

    def test_standard_library_still_resolves_without_a_local_module(self) -> None:
        entry = self.package("from queue import Queue")
        self.assertEqual(
            ["pkg/__init__.py", "pkg/top.py", "pkg/types.py"],
            [item.path for item in self.capture(entry).entries],
        )


if __name__ == "__main__":
    unittest.main()
