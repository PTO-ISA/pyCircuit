"""Host input-oracle anchors; no product imports or execution."""

import unittest

from .cases import prefixes, validate


class PrefixCasesTest(unittest.TestCase):
    def test_valid_key_sets(self):
        rows = list(prefixes())
        self.assertEqual(len(rows), 16)
        for name, keys, tokens in rows:
            with self.subTest(name=name):
                self.assertEqual(validate(keys, tokens), 0)
                self.assertEqual(len(tokens), len(set(tokens)))
        depth = next(tokens for name, _, tokens in rows if name == "depth-32")
        self.assertEqual(len(depth), 33)
        self.assertEqual(set(depth), {0} | {2**i for i in range(32)})

    def test_first_error_and_unsigned_range(self):
        self.assertEqual(validate(4, [0, 0, 4]), 4)
        self.assertEqual(validate(4, [4, 0, 0]), 3)
        self.assertEqual(validate(0xFFFFFFFF, [0xFFFFFFFE, 0]), 0)
        self.assertEqual(validate(0xFFFFFFFF, [0xFFFFFFFF]), 3)
        self.assertEqual(validate(2, [0, 1, 0]), 2)


if __name__ == "__main__":
    unittest.main()
