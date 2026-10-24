"""Exact binary32 encodings of the complete positive FP4 codebooks."""

import pycircuit as ac


def level(code: ac.u3, kind: ac.u1) -> ac.u32:
    e2: ac.u32 = 0
    e2 = 1056964608 if code == 1 else e2
    e2 = 1065353216 if code == 2 else e2
    e2 = 1069547520 if code == 3 else e2
    e2 = 1073741824 if code == 4 else e2
    e2 = 1077936128 if code == 5 else e2
    e2 = 1082130432 if code == 6 else e2
    e2 = 1086324736 if code == 7 else e2
    e1: ac.u32 = 0
    e1 = 1048576000 if code == 1 else e1
    e1 = 1056964608 if code == 2 else e1
    e1 = 1061158912 if code == 3 else e1
    e1 = 1065353216 if code == 4 else e1
    e1 = 1067450368 if code == 5 else e1
    e1 = 1069547520 if code == 6 else e1
    e1 = 1071644672 if code == 7 else e1
    return e1 if kind else e2


def midpoint(code: ac.u3, kind: ac.u1) -> ac.u32:
    e2: ac.u32 = 1048576000
    e2 = 1061158912 if code == 1 else e2
    e2 = 1067450368 if code == 2 else e2
    e2 = 1071644672 if code == 3 else e2
    e2 = 1075838976 if code == 4 else e2
    e2 = 1080033280 if code == 5 else e2
    e2 = 1084227584 if code == 6 else e2
    e1: ac.u32 = 1040187392
    e1 = 1052770304 if code == 1 else e1
    e1 = 1059061760 if code == 2 else e1
    e1 = 1063256064 if code == 3 else e1
    e1 = 1066401792 if code == 4 else e1
    e1 = 1068498944 if code == 5 else e1
    e1 = 1070596096 if code == 6 else e1
    return e1 if kind else e2
