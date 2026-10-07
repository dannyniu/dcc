/* DannyNiu/NJF, 2026-10-05. Public Domain. */

#include "c-semantics.errchk.bits.h"

X( aggr_def,
   Dependencies, ( OpTerm(0) OpTerm(1) ),
   NodeColoring, (),

 // 2026-10-05:
 // These additional phases are specific to expression optimizations.
   OperandDiscount, (),
   CollectReuseStats, (),

 // 2026-10-05:
 // Declarations are handled entirely within compilation,
 // they don't embody as machine instructions.
   Exhalation, (),
    )

X( member_decl_list_genrule,

   Dependencies, ( OpTerm(0) OpTerm(1) ),
   NodeColoring, (),

 // 2026-10-05:
 // See notes in `aggr_def` above.
   OperandDiscount, (),
   CollectReuseStats, (),
   Exhalation, (),
    )
