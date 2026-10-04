"""Process timing, peak RSS and fingerprints for current benchmark runners."""
import hashlib
from pathlib import Path
import statistics
import subprocess
import sys
import tempfile
import time


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def measured(command, **kwargs):
    """Process wall time and peak RSS; runner run_ns remains the fixed-loop timer."""
    with tempfile.NamedTemporaryFile() as rss:
        start = time.perf_counter_ns()
        wrapper = ('import resource,subprocess,sys; p=subprocess.run(sys.argv[2:]); '
                   'open(sys.argv[1],"w").write(str(resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss)); '
                   'sys.exit(p.returncode)')
        p = subprocess.run([sys.executable, '-c', wrapper, rss.name, *map(str, command)],
                           capture_output=True, text=True, timeout=300, **kwargs)
        wall = time.perf_counter_ns() - start
        if p.returncode:
            raise RuntimeError(f'{command}: {p.stderr}')
        return p.stdout, dict(process_ns=wall, max_rss_kib=int(Path(rss.name).read_text()))


def summarize(samples):
    return {key: statistics.median(s[key] for s in samples) for key in samples[0]}
