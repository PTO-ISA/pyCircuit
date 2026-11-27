# ruff: noqa: N802 -- pyCircuit module name.
"""Calendar single-hop bitmap forwarding on the explicit 5-by-8 profile.

No link latency, queue capacity or CalReg schedule is assigned here. Complete
routes still require the core's induced-tree validation before injection.
"""

import pycircuit as ac


@ac.struct
class HopInput:
    route: ac.bits[80]
    node: ac.u6
    has_ingress: bool
    ingress: ac.u6


@ac.struct
class HopResult:
    egress: ac.u40
    lands: bool
    fault: bool


def passes(route: ac.bits[80], node: ac.u6) -> bool:
    offset: ac.u7 = node
    shifted = route >> (2 * offset)
    return shifted[1:2] == 1


# ndf: implements=WSEMIG-FORWARD-001
@ac.module
def ForwardHop(request: HopInput) -> HopResult:
    @ac.rule
    def forward() -> HopResult:
        land_mask: ac.bits[80] = 0x55555555555555555555
        illegal = ((request.route & land_mask) & ~((request.route >> 1) & land_mask)) != 0
        fault = request.node >= 40 or illegal or not passes(request.route, request.node)
        out: ac.u40 = 0
        one: ac.u40 = 1
        offset: ac.u7 = request.node
        pair = request.route >> (2 * offset)
        if not fault:
            if request.node >= 8:
                peer = request.node - 8
                if passes(request.route, peer) and (
                    not request.has_ingress or peer != request.ingress
                ):
                    out = out | (one << peer)
            if request.node[0:3] != 0:
                peer = request.node - 1
                if passes(request.route, peer) and (
                    not request.has_ingress or peer != request.ingress
                ):
                    out = out | (one << peer)
            if request.node[0:3] != 7:
                peer = request.node + 1
                if passes(request.route, peer) and (
                    not request.has_ingress or peer != request.ingress
                ):
                    out = out | (one << peer)
            if request.node < 32:
                peer = request.node + 8
                if passes(request.route, peer) and (
                    not request.has_ingress or peer != request.ingress
                ):
                    out = out | (one << peer)
        return HopResult(egress=out, lands=not fault and pair[0:1] == 1, fault=fault)

    return forward
