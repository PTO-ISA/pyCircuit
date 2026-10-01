"""Generate a small complete pipeline trace with backpressure and cache reuse."""

import argparse
from pathlib import Path

from .model import pipeline
from review import ReviewTrace


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path, nargs='?',
                        default=Path(__file__).parent / 'review-output' / 'pipeline.html')
    args = parser.parse_args()
    circuit = pipeline(list(range(8)), length=3, period=9, control=tuple(range(35)))
    names = {q.qid: f'link[{i}]' for i, q in enumerate(circuit.named['links'])}
    names[circuit.output.qid] = 'results'
    trace = ReviewTrace(circuit.sim, title='弹性流水线 / 背压与计算复用', queue_names=names)
    for _ in range(90):
        circuit.sim.step()
    actual = [entry[1][0] for entry in circuit.output.current]
    if actual != [(i, i + 3) for i in range(8)]:
        raise AssertionError(actual)
    trace.write_html(args.output)
    print(f'8 inputs completed; review: {args.output}')


if __name__ == '__main__':
    main()
