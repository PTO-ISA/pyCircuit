"""Register values and rename tags; new rename takes priority over tag clearing."""
from .types import ac, RenameEntry


@ac.module
def RegisterFile(rename, registers, allocation, retirement):
    @ac.rule
    def update():
        issue = allocation.value
        commit = retirement.value
        for i in range(1, 32):
            state = rename[i].value
            if issue.rename and issue.rd == i:
                state = RenameEntry(tag=issue.rename_tag, busy=True)
            if commit.write and commit.entry.dest == i:
                registers[i].value = commit.entry.value
                # Commit tests Reorder.current(): dispatch's new tag wins.
                if state.tag == commit.rob:
                    state.busy = False
            if commit.flush:
                state.busy = False
            rename[i].value = state
    update()
