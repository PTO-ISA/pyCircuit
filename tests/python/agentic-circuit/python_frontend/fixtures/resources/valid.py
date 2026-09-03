from agentic_circuit import ResourceRef, address_map, address_space, queue


memory = ResourceRef("memory", "Memory", "target")
requests = queue(
    "requests",
    payload_type="Transaction",
    protocol="ready_valid",
    depth=8,
    time_domain="core",
)
system = address_space("system", width=32)
mapping = address_map(system, (0x1000, 0x2000, memory, 0))
