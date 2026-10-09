from pycircuit import module, rule


@module
def Independent():
    @rule
    def idle():
        return

    idle()
