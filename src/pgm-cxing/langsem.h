/* DannyNiu/NJF, 2025-10-02. Public Domain. */

#ifndef cxing2c_langsem_h
#define cxing2c_langsem_h 1

#include "value-nativobj.h"

// 2025-11-15:
// ====
//
// There need to be a procedure for determining
// the type of arithmetic context. Ideally, the
// procedure should be a reduction process that
// iteratively merge 2 inputs at the head of the
// list. This means the merge must be both
// associative and commutative.
//
// Because opaque objects and nulls are treated
// specially, they'll first need pre-processing,
// at value level.
int NormalizeTypeForArithContext(struct value_nativeobj);
//
// Then, 2 operands at a time, determine their
// common arithmetic context. To maintain associativity,
// the function has an additional possible return value
// of valtyp_null, which need to be converted
// to valtyp_long by the end of list processing.
int DetermineTypeForArithContext(int type1, int type2);

struct value_nativeobj ConvertToDouble(struct value_nativeobj val);
struct value_nativeobj ConvertToUlong(struct value_nativeobj val);
struct value_nativeobj ConvertToLong(struct value_nativeobj val);

bool IsNull(struct value_nativeobj val);
bool IsNullish(struct value_nativeobj val);

struct lvalue_nativeobj GetValProperty(
    struct value_nativeobj obj,
    struct value_nativeobj key);

struct value_nativeobj SetValProperty(
    struct value_nativeobj obj,
    struct value_nativeobj key,
    struct value_nativeobj val);

struct value_nativeobj ValueCopy(struct value_nativeobj);

void ValueDestroy(struct value_nativeobj val);

typedef struct value_nativeobj (*cxing_call_proto)(
    int, struct value_nativeobj *);

#endif /* cxing2c_langsem_h */
