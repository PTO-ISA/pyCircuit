from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


WORKSPACE = Path(__file__).resolve().parent / "fixtures" / "resources"


def load_fixture(name: str):
    path = WORKSPACE / name
    spec = importlib.util.spec_from_file_location(f"resources_{path.stem}", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import resource fixture {path}")
    loaded = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(loaded)
    return loaded


class ResourceFrontendTest(unittest.TestCase):
    def test_queue_and_address_map_are_static_records(self) -> None:
        loaded = load_fixture("valid.py")

        self.assertEqual("ready_valid", loaded.requests.protocol)
        self.assertEqual(8, loaded.requests.depth)
        self.assertEqual("core", loaded.requests.time_domain)
        self.assertEqual((0x1000, 0x2000), loaded.mapping.entries[0].range)
        self.assertEqual("memory", loaded.mapping.entries[0].target.stable_name)

    def test_protocol_roles_must_be_complementary(self) -> None:
        from agentic_circuit._resources import (
            FrontendRuleError,
            ProtocolContract,
            verify_protocol_roles,
        )

        contract = ProtocolContract(
            "ready_valid", "producer", "consumer", "Transaction", "core"
        )

        with self.assertRaisesRegex(FrontendRuleError, "ACPY-PROTOCOL-004"):
            verify_protocol_roles(contract, "producer", "producer")

    def test_equal_priority_address_overlap_is_rejected(self) -> None:
        from agentic_circuit import address_map, address_space
        from agentic_circuit._resources import FrontendRuleError

        memory = load_fixture("invalid.py").memory
        space = address_space("system", width=32)

        with self.assertRaisesRegex(FrontendRuleError, "ACPY-ADDRESS-003"):
            address_map(
                space,
                (0x1000, 0x2000, memory, 0),
                (0x1800, 0x2800, memory, 0),
            )

    def test_dynamic_address_and_nonpositive_depth_are_rejected(self) -> None:
        from agentic_circuit import address_map, address_space, queue
        from agentic_circuit._resources import FrontendRuleError
        from agentic_circuit._types import _test_symbolic

        memory = load_fixture("invalid.py").memory
        space = address_space("system", width=32)
        dynamic = _test_symbolic("base", int)

        with self.assertRaisesRegex(FrontendRuleError, "ACPY-STATIC-002"):
            address_map(space, (dynamic, 0x2000, memory, 0))
        with self.assertRaisesRegex(FrontendRuleError, "ACPY-RESOURCE-002"):
            queue(
                "requests",
                payload_type="Transaction",
                protocol="ready_valid",
                depth=0,
            )


if __name__ == "__main__":
    unittest.main()
