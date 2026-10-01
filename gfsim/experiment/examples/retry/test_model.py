"""Complete retry circuits: inputs, independent comparison and output assertions."""

from examples.testing import CircuitTestCase
from .model import retry


class RetryTests(CircuitTestCase):
    def test_retry_buffer_recycles_identical_bits_then_delivers(self):
        c, _ = self.run_circuit(retry, attempts=8, ticks=20)
        self.assertEqual(c.output.current, ((9, ((8, 42),)),))
        self.assertEqual(c.named["token"].state_version, 9)
        self.assertEqual(c.named["budget"].current, (0,))
