"""AST-only ACPy → transactional MLIR → conditional static rule graph."""
from .frontend import compile_source
from .backend import analyze

__all__ = ['compile_source', 'analyze']
