// 2026-10-05: Old file, no longer relevant, to be deleted.

// Sketch currently demonstrating the allocation of stack spaces for
// evaluating expressions (2026-08-13).
//
// This isn't an actual JavaScript file, the name is just so that
// my IDE can indent and highlight the syntax properly.

// 1st pass:
// color the nodes with cookie; collect initial operand usage stats.
function_body.traversePostOrder((node) => {
  node.color();
  if( node.isDeclarator() )
  {
    function_body.growStackPartition(
      node.evalDeclaration(), STACK_PART_LOCAL_DECLS);
  }
  if( ++ function_body.consumer_stats[node.cookie] > 1 )
    node.reuseCandidate = true; // not committing to reuse yet.
  return TRAVERSE_CONTINUE;
});

// 2nd pass:
// eliminate second computations of reusable operands.
function_body.traversePostOrder((node) => {
  if( node.reuseCandidate )
  {
    node.children.forEach((child) => {
      -- function_body.consumer_stats[child.cookie];
    });
  }
  return TRAVERSE_CONTINUE;
});

// 3rd pass:
// collect reused operands usage stats.
function_body.traversePostOrder((node) => {
  if( function_body.consumer_stats[node.cookie] > 1 )
  {
    function_body.growStackPartition(
      node.typeOf(), STACK_PART_REUSED_OPERANDS);
  }
  else node.reuseCandidate = false;
  return TRAVERSE_CONTINUE;
});

// 4th pass:
// emit mini-instructions.
// (mini stands for machine instruction neutral intermediary)
function_body.traversePostOrder((node) => {
  // 2026-08-15:
  // This part is in error - you can't prune in a post-order traversal.
  if( function_body.usage_stats[node.cookie] == 0 )
  {
    emitInstruction(node);
    if( node.reuseCandidate )
    {
      ++ function_body.usage_stats[node.cookie];
      emitSaveSequence(node);
    }
    return TRAVERSE_CONTINUE;
  }
  else
  {
    emitOperandReuseSequence(node);
    return TRAVERSE_PRUNE;
  }
});

// 2026-08-13 TODO:
// Preferablly nodes with few reuses and shallow depth are not reused.

// 2026-08-13 TODO:
// At this pointer, we need to consider the details of the 4th pass.
// Before MINI instructions, there are 'meta' instructions that
// based on the current state of the mental picture of the compiler,
// determine which MINI instructions are actually necessary,
// unnecessary ones will be omitted.
//
// The META instructions are made as function calls, which
// emits MINI instructions
//
// META stands for Mental Engine for Translation & Analysis.

// 2026-08-24 Issues:
// The 3-partitioning of stack space does ensure operand datum popping order
// is consistent with eviction order of most-distantly-future-used value.
// HOWEVER! WE CAN'T KNOW WHEN TO EVICT TO STACK only by looking at the
// register file!
//
// 2026-10-05:
// Answer to the above 2026-08-24 issue: The entirity of the
// 4-phase design is dropped, in favor of a lazy recompute
// approach. See section '2026-10-05' in "notes.md".
