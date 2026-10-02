"""Run the complete datapath and export a generic review trace beside this example."""
from pathlib import Path
import argparse

from review import ReviewTrace
from .model import build, workload
from .reference import Reference


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path(__file__).parent / 'review-output/signals.html')
    parser.add_argument('--frequent', action='store_true')
    args = parser.parse_args()
    words, script = workload(frequent=args.frequent)
    sim, reference = build(words=words, script=script), Reference(words, script)
    trace = ReviewTrace(sim, title='Banked stream with shared Signals',
                        queue_names=dict(enumerate(('bank0', 'bank1', 'position0', 'position1',
                                                   'configuration', 'output', 'retired'))))
    for _ in range(120):
        sim.step()
        assert sim.snapshot() == reference.step()
        assert tuple(signal.value for signal in sim.signals) == reference.signals()
    trace.write_html(args.output)
    print(f'{sim.tick} ticks; {sim.queues[6].peek()[0]} retired; '
          f'{sim.stats.signal_work} Signal evaluations; review: {args.output}')


if __name__ == '__main__':
    main()
