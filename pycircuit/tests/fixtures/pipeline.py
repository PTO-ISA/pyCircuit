from pycircuit import ac

@ac.module
def Source(values: ac.vector[ac.u32]):
    cursor = ac.queue[ac.u32](initial=0)

    @ac.rule
    def produce():
        i = cursor.value
        if i < len(values):
            cursor.value = i + 1
            return values[i]
        return None

    out = produce()

    return out

@ac.rule
def double(message):
    return message.value + message.value

@ac.module
def Transform(message):
    out = double(message)
    return out

@ac.module
def Sink(message, total, count):
    @ac.rule
    def accept(value):
        total.value = total.value + value.value
        count.value = count.value + 1

    accept(message)

@ac.module
def Pipeline(values: ac.vector[ac.u32]):
    total = ac.queue[ac.u32](initial=0)
    count = ac.queue[ac.u32](initial=0)
    source = Source(values)
    transformed = Transform(source)
    Sink(transformed, total, count)
