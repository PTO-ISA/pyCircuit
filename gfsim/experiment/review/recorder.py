"""Optional, model-independent observer and offline HTML export (debug use only)."""

import json
from dataclasses import asdict
from pathlib import Path


def value_json(value):
    """Keep record field names and avoid rounding wide integers in JavaScript."""
    if isinstance(value, tuple):
        if hasattr(value, '_fields'):
            return {name: value_json(item) for name, item in zip(value._fields, value)}
        return [value_json(item) for item in value]
    if type(value) is int and abs(value) > 2**53 - 1:
        return str(value)
    return value


class ReviewTrace:
    def __init__(self, sim, *, title='GFSim review', queue_names=None):
        if sim.observer is not None:
            raise ValueError('a review observer is already attached')
        self.sim, self.frame = sim, None
        names = queue_names or {}
        self.data = {
            'title': title,
            'modules': [f'M{i} {type(m).__name__}' for i, m in enumerate(sim.module_objects)],
            'rules': [None] + [
                {'name': f'R{i} {e.work.__name__}', 'module': e.module_id,
                 'pops': e.pops, 'pushes': e.pushes, 'revises': e.revises}
                for i, e in enumerate(sim.entries[1:], 1)],
            'queues': [{'name': f'Q{q.qid} {names.get(q.qid, "")}'.rstrip(),
                        'capacity': q.capacity, 'pop': q.pop_rule, 'push': q.push_rule}
                       for q in sim.queues],
            'initial': [self.queue_state(q) for q in sim.queues],
            'frames': [],
        }
        sim.observer = self

    @staticmethod
    def queue_state(queue):
        return {'version': queue.state_version, 'values': value_json(queue.current)}

    def start(self):
        s = self.sim
        self.before = [(q.state_version, q.current) for q in s.queues]
        self.calls = tuple(s.module_calls)
        self.stats = asdict(s.stats)
        self.seen_reads = set()
        self.frame = {'tick': s.tick, 'calls': [], 'edges': [], 'decisions': [],
                      'reads': [], 'invalidations': [],
                      'due': sorted(event for event in s.events if event[0] <= s.tick),
                      'rules': None, 'error': None}

    def read(self, mid, qid, rid):
        key = (mid, qid, rid)
        if key not in self.seen_reads:
            self.seen_reads.add(key)
            self.frame['reads'].append([mid, qid, rid, self.sim.queues[qid].state_version])

    def rule_call(self, rid, reused):
        r = self.sim.rules[rid]
        if reused:
            reason = '候选完整且 dirty 未置位'
        elif not self.sim.cache:
            reason = '缓存已关闭'
        elif not r.complete:
            reason = '无可复用的完整候选'
        else:
            reason = '参数或实际读取的 Queue 变化'
        self.frame['calls'].append({'rid': rid, 'reuse': reused, 'dirty': self.sim.is_dirty(rid), 'reason': reason})

    def invalidate(self, mid, qid):
        s = self.sim
        module = s.modules[mid]
        slot = module.resource_qids.index(qid)
        readers = [rid for rid in module.rule_ids
                   if module.rule_readers[slot * module.word_count + s.rules[rid].word_index]
                   & s.rules[rid].bit]
        self.frame['invalidations'].append({'mid': mid, 'qid': qid, 'rules': readers})

    def prepared(self):
        s = self.sim
        self.frame['rules'] = [None] + [
            {'complete': r.complete, 'selected': r.selected_tick == s.tick,
             'args': value_json(r.candidate_args), 'dirty': s.is_dirty(rid),
             'deps': [s.modules[s.entries[rid].module_id].resource_qids[slot]
                      for slot in r.read_slots],
             'events': list(r.wake_requests),
             'proposals': [
                 {'qid': qid, 'pop': s.queues[qid].proposal(rid).pop,
                  'push': value_json(s.queues[qid].proposal(rid).push),
                  'revise': value_json(tuple(s.queues[qid].proposal(rid).revises))}
                 for qid in r.participants]}
            for rid, r in enumerate(s.rules[1:], 1)]

    def capacity_edge(self, producer, consumer, qid):
        self.frame['edges'].append([producer, consumer, qid])

    def decision(self, rid, accepted):
        self.frame['decisions'].append([rid, accepted])

    def finish(self, error=None):
        if self.frame is None:
            return
        s, f = self.sim, self.frame
        f['error'] = None if error is None else f'{type(error).__name__}: {error}'
        f['worked'] = [i for i, count in enumerate(s.module_calls) if count != self.calls[i]]
        f['generations'] = [m.read_gen for m in s.modules]
        f['readers'] = [[mid for mid, slot in zip(q.readers, q.reader_slots)
                         if s.is_reader(mid, slot)] for q in s.queues]
        f['changes'] = [[i, self.queue_state(q)] for i, q in enumerate(s.queues)
                        if (q.state_version, q.current) != self.before[i]]
        f['future'] = sorted(s.events)
        f['stats'] = {key: value - self.stats[key] for key, value in asdict(s.stats).items()
                      if key != 'max_stack'}
        f['max_stack_so_far'] = s.stats.max_stack
        self.data['frames'].append(f)
        self.frame = None

    def write_html(self, path):
        # Escape script terminators in user-supplied model/queue names.
        payload = json.dumps(self.data, ensure_ascii=False, separators=(',', ':')).replace('<', '\\u003c')
        template = Path(__file__).with_name('viewer.html').read_text()
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(template.replace('/*TRACE_DATA*/null', payload))
