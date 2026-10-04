"""Independent AST → typed ACIR → GFSim compiler (no hardware execution)."""
from .frontend import compile_source
from .backend import emit

__all__ = ['compile_source', 'emit']
