"""Component interface contracts. Readiness is independent of SB relations."""
from .ir import CompileError


def defaults():
    return [
        dict(id='Queue', methods=['read', 'revise', 'pop', 'push', 'query.empty', 'query.full', 'query.size'],
             guards={'push': ['query.full']}, views={'full': dict(visibility='ehr', stages=['pop', 'query.full'])},
             relations=[dict(before=a, after=b, relation='SB', source='default',
                             origin='Pipeline FIFO') for a, b in
                        [('read', 'revise'), ('revise', 'pop')]]),
        dict(id='EHR', methods=['read', 'write'], relations=[]),
        dict(id='Signal', methods=['set', 'get'], relations=[dict(before='set', after='get', relation='SB', source='default', origin='Signal current value')]),
        dict(id='Cell', methods=['read', 'write'], relations=[]),
    ]


def closure(contract):
    """Retain a finite witness for each necessary interface order, including cycles.

    Absence from this table means CF. A witness contains declaration locations;
    no readiness, guard or invocation condition belongs in this table.
    """
    methods = set(contract['methods'])
    result = {}
    relations = list(contract['relations'])
    for view, declaration in contract.get('views', {}).items():
        for before, after in zip(declaration['stages'], declaration['stages'][1:]):
            relations.append(dict(before=before, after=after, relation='SB', source='derived', origin='EHR ' + view + ' prior-port visibility'))
    for method, guards in contract.get('guards', {}).items():
        for guard in guards:
            relations.append(dict(before=guard, after=method, relation='SB', source='derived', origin=method + ' guard calls ' + guard))
    for rel in relations:
        if rel.get('relation') != 'SB' or rel['before'] not in methods or rel['after'] not in methods:
            raise CompileError('invalid SB relation in component contract ' + contract['id'])
        result.setdefault((rel['before'], rel['after']), []).append(rel)
    for mid in sorted(methods):
        for before in sorted(methods):
            for after in sorted(methods):
                if (before, after) not in result and (before, mid) in result and (mid, after) in result:
                    result[before, after] = result[before, mid] + result[mid, after]
    return result
