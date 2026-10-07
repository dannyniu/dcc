2026-07-16
====

- static assert declaration could be confused with static assert expr stmt,
  so it's omitted in the transcribed grammar.
  - when appearing in any declarations expecting one, check it then ignore it.
- bit-precise integers will not be considered at the beginning phase,
  nor will complex and decimal types and atomic-qualified types.


2026-10-05
====

Save and restores of non-reused operands certainly should follow the
FILO stack order. However, reused operands may also be evicted from
the register file, as such, further considerations are needed.

A new way to to eliminate redundant ''unsequenced'' computations.

One of the `MiniStream_*` functions (likely `MiniStream_InsertSaveRestores` at
the moment) would perform the following procedure to eliminate 2nd computations
of operands already loaded into the register file. There will no longer be
a partition-II of stack space for reused operands, instead, reused operands
are simply evicted passively from the registers, and are recomputed when the
eviction happens.

Before presenting the procedure, here's some properties met by the procedure:

1. The eliminated mini-instructions must be unsequenced,
2. The eliminated mini-instructions must be direct or descendent operands of
   ones that computes values already present in the register file.

The procedure:
1. First finds a continuous sequence of mInstr that're 'reuse candidate',
   starting from the current position, to the first mInstr whose value is
   loaded into the register file. If there is no such sequence, the procedure is
   cancelled, and the invoker receives an indication to resume normal operation.
2. Verify that for all mInstr, their operands have sequence number strictly less
   than that carried on themselves. Otherwise, cancel and indicate as in step 1.
3. Eliminate this continuous sequence, and replace it with `mNOP(dest_actual=${regid})`,
   where `${regid}` is the register ID corresponding to the cookie value of the
   last mInstr in the sequence.
