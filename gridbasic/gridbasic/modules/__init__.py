"""Built-in capability modules for GridBasic.

Each module exposes a ``namespace(interp)`` function returning a dict of
name -> GridBasic value (mostly :class:`BoundBuiltin` callables and small
Python objects whose methods are reached via the interpreter's attribute
fallback).
"""
