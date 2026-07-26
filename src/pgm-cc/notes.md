2026-07-16
====

- static assert declaration could be confused with static assert expr stmt,
  so it's omitted in the transcribed grammar.
  - when appearing in any declarations expecting one, check it then ignore it.
- bit-precise integers will not be considered at the beginning phase,
  nor will complex and decimal types and atomic-qualified types.
