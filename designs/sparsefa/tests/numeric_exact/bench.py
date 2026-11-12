# ruff: noqa: N802
"""Generated independent rational expectations; regenerate with generate.py."""
import pycircuit as ac
from fa2.fa2_numeric import fp_add, fp_div, fp_mul


@ac.struct
class Case:
    a: ac.u32
    b: ac.u32
    add: ac.u32
    mul: ac.u32
    div: ac.u32


def vectors() -> ac.table[32, Case]:
    data: ac.table[32, Case] = (
        Case(
            a=1065353216, b=2541748224, add=1065353215, mul=2541748224, div=3883925504
        ),
        Case(a=3212836864, b=394264576, add=3212836863, mul=2541748224, div=3883925504),
        Case(a=1065353216, b=3212836864, add=0, mul=3212836864, div=3212836864),
        Case(
            a=3212836864, b=1065353216, add=2147483648, mul=3212836864, div=3212836864
        ),
        Case(a=2147483648, b=0, add=2147483648, mul=2147483648, div=2147483648),
        Case(a=0, b=2147483648, add=0, mul=2147483648, div=2147483648),
        Case(a=8388608, b=2155872256, add=0, mul=2147483648, div=3212836864),
        Case(a=8388608, b=1056964608, add=1056964608, mul=0, div=16777216),
        Case(a=8388609, b=2155872256, add=0, mul=2147483648, div=3212836865),
        Case(
            a=2139095039, b=1073741824, add=2139095039, mul=2139095039, div=2130706431
        ),
        Case(
            a=4286578687, b=3221225472, add=4286578687, mul=2139095039, div=2130706431
        ),
        Case(a=1065353216, b=1, add=1065353216, mul=0, div=0),
        Case(
            a=1065353216, b=2147483649, add=1065353216, mul=2147483648, div=2147483648
        ),
        Case(
            a=1073741823, b=3212836864, add=1065353214, mul=3221225471, div=3221225471
        ),
        Case(
            a=1065353215, b=3212836864, add=3011510272, mul=3212836863, div=3212836863
        ),
        Case(a=1065353217, b=3212836864, add=872415232, mul=3212836865, div=3212836865),
        Case(a=3224976945, b=2567042215, add=3224976945, mul=431755965, div=1723233419),
        Case(a=1851709488, b=947960562, add=1851709488, mul=1734352295, div=1969067153),
        Case(a=1726086437, b=597595063, add=1726086437, mul=1259076770, div=2139095039),
        Case(
            a=1507028631, b=2672464853, add=1507028630, mul=3114749787, div=4194670660
        ),
        Case(a=169507386, b=1585344703, add=1585344703, mul=689539406, div=0),
        Case(
            a=1687550749, b=2903334827, add=1687550748, mul=3525682870, div=4144483072
        ),
        Case(
            a=1406921590, b=1279668704, add=1406921986, mul=1621770997, div=1192101845
        ),
        Case(
            a=1383704135, b=2256166156, add=1383704134, mul=2574526238, div=4286578687
        ),
        Case(a=565661589, b=1516634604, add=1516634604, mul=1017426656, div=114034363),
        Case(
            a=1452258444, b=1444359821, add=1457212370, mul=1831451456, div=1072912120
        ),
        Case(a=1253677505, b=1530708449, add=1530708449, mul=1720244601, div=788248573),
        Case(
            a=2600712632, b=1486328917, add=1486328916, mul=3021733309, div=2178840590
        ),
        Case(a=681309318, b=3944165799, add=3944165798, mul=3560453877, div=2147483648),
        Case(a=376279792, b=2299210045, add=376279791, mul=2147483648, div=3436872985),
        Case(
            a=3501922296, b=1336406218, add=3499170757, mul=3774183737, div=3230570771
        ),
        Case(a=1410544337, b=887131623, add=1410544337, mul=1233198606, div=1588056004),
    )
    return data


@ac.rule
def step(index):
    index = index + 1


@ac.system
def ExactSystem():
    index: ac.u5 = 0
    data = vectors()
    vector = data[index]
    added = fp_add(vector.a, vector.b)
    multiplied = fp_mul(vector.a, vector.b)
    divided = fp_div(vector.a, vector.b)

    @ac.rule
    def check():
        assert added == vector.add, "exact RTZ add"
        assert multiplied == vector.mul, "exact RTZ multiply"
        assert divided == vector.div, "exact RTZ divide"
        ac.log("info", "exact", index, added, multiplied, divided)

    check()
    step(index)
