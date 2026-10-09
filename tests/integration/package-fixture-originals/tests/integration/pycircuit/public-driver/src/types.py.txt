from typing import Annotated

Word = Annotated[int, range(256)]
UnusedWord = Annotated[int, range(17)]
_PrivateWord = Annotated[int, range(13)]
