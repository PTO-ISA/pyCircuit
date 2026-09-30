from typing import Annotated

Word = Annotated[int, range(256)]
Phase = Annotated[int, range(8)]
