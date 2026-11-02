from pycircuit import autorule, module


@module
def Independent():
    @autorule
    def idle():
        return

    idle()
