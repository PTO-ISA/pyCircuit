"""Published nominal authority; the consumer never reads this Python source."""

from enum import Enum
import pycircuit as ac


@ac.encoding(width=2)
class Tag(Enum):
    LOW = 0
    MID = 1
    HIGH = 3


@ac.struct
class Item:
    tag: Tag
    data: ac.bits[9]


@ac.struct
class Parcel:
    entries: ac.table[3, Item]
    salt: ac.bits[9]


@ac.module
def Produce(data: ac.bits[9]) -> Parcel:
    return Parcel(
        entries=(
            Item(tag=Tag.LOW, data=data),
            Item(tag=Tag.HIGH, data=13),
            Item(tag=Tag.MID, data=511),
        ),
        salt=data,
    )
