/* DannyNiu/NJF, 2026-06-23. Public Domain. */

// DSFA stands for 'Direct Structure Field Access'.

#if defined(DSFA_GetImpl0) && defined(DSFA_SetImpl0) &&         \
    defined(DSFA_InitSetImpl0) && defined(DSFA_Xmacros_Defs)

#include "runtime.h"

#define Xmacro_Integer(DSFA_Type, XmacroField)                  \
    if( strcmp(#XmacroField, key) == 0 )                        \
        return (struct value_nativeobj){                        \
            .proper.l = ((DSFA_Type *)backing)->XmacroField,    \
            .type = (const void *)&type_nativeobj_long };

#define Xmacro_Unsigned(DSFA_Type, XmacroField)                 \
    if( strcmp(#XmacroField, key) == 0 )                        \
        return (struct value_nativeobj){                        \
            .proper.u = ((DSFA_Type *)backing)->XmacroField,    \
            .type = (const void *)&type_nativeobj_ulong };

#define Xmacro_U32BE(DSFA_Type, XmacroField)                            \
    if( strcmp(#XmacroField, key) == 0 )                                \
        return (struct value_nativeobj){                                \
            .proper.u = ntohl(((DSFA_Type *)backing)->XmacroField),     \
            .type = (const void *)&type_nativeobj_ulong };

#define Xmacro_U16BE(DSFA_Type, XmacroField)                            \
    if( strcmp(#XmacroField, key) == 0 )                                \
        return (struct value_nativeobj){                                \
            .proper.u = ntohs(((DSFA_Type *)backing)->XmacroField),     \
            .type = (const void *)&type_nativeobj_ulong };

#define Xmacro_String(DSFA_Type, XmacroField)                           \
    if( strcmp(#XmacroField, key) == 0 )                                \
    {                                                                   \
        size_t len = sizeof(((DSFA_Type *)backing)->XmacroField);       \
        s2data_t *ret = s2data_create(len);                             \
        if( !ret ) return (struct value_nativeobj){                     \
                .proper.l = errno,                                      \
                .type = (const void *)&type_nativeobj_null };           \
        memcpy(s2data_weakmap(ret),                                     \
               &((DSFA_Type *)backing)->XmacroField, len);              \
        return (struct value_nativeobj){                                \
            .proper.p = ret,                                            \
            .type = (const void *)&type_nativeobj_s2impl_str };         \
    }

struct value_nativeobj DSFA_GetImpl0(
    int argn, struct value_nativeobj args[])
{
    // The underlying backing is an `s2data_t`.
    const void *backing = s2data_weakmap(args[0].proper.p);
    const char *key = s2data_weakmap(args[1].proper.p);

    // 2026-06-23 (tentative): Assertions on arguments by caller.
    (void)argn;

#include DSFA_Xmacros_Defs

    CxingDebug("Unrecognized structure field: %s.\n", key);
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

#undef Xmacro_Integer
#undef Xmacro_Unsigned
#undef Xmacro_U32BE
#undef Xmacro_U16BE
#undef Xmacro_String

#define Xmacro_Integer(DSFA_Type, XmacroField)                          \
    if( strcmp(#XmacroField, key) == 0 )                                \
    {                                                                   \
        if( !IsInteger(args[2]) )                                       \
        {                                                               \
            CxingDebug(#XmacroField " of " #DSFA_Type                   \
                       " should be an integer.\n");                     \
            return (struct value_nativeobj){                            \
                .proper.p = NULL,                                       \
                .type = (const void *)&type_nativeobj_morgoth };        \
        }                                                               \
        ((DSFA_Type *)backing)->XmacroField = args[2].proper.l;         \
        args[2].proper.l = ((DSFA_Type *)backing)->XmacroField;         \
        return args[2];                                                 \
    }

#define Xmacro_Unsigned(DSFA_Type, XmacroField)                         \
    if( strcmp(#XmacroField, key) == 0 )                                \
    {                                                                   \
        if( !IsInteger(args[2]) )                                       \
        {                                                               \
            CxingDebug(#XmacroField " of " #DSFA_Type                   \
                       " should be an integer.\n");                     \
            return (struct value_nativeobj){                            \
                .proper.p = NULL,                                       \
                .type = (const void *)&type_nativeobj_morgoth };        \
        }                                                               \
        ((DSFA_Type *)backing)->XmacroField = args[2].proper.u;         \
        args[2].proper.u = ((DSFA_Type *)backing)->XmacroField;         \
        return args[2];                                                 \
    }

#define Xmacro_U32BE(DSFA_Type, XmacroField)                            \
    if( strcmp(#XmacroField, key) == 0 )                                \
    {                                                                   \
        if( !IsInteger(args[2]) )                                       \
        {                                                               \
            CxingDebug(#XmacroField " of " #DSFA_Type                   \
                       " should be an integer.\n");                     \
            return (struct value_nativeobj){                            \
                .proper.p = NULL,                                       \
                .type = (const void *)&type_nativeobj_morgoth };        \
        }                                                               \
        ((DSFA_Type *)backing)->XmacroField = htonl(args[2].proper.u);  \
        args[2].proper.u = (uint32_t)args[2].proper.u;                  \
        return args[2];                                                 \
    }

#define Xmacro_U16BE(DSFA_Type, XmacroField)                            \
    if( strcmp(#XmacroField, key) == 0 )                                \
    {                                                                   \
        if( !IsInteger(args[2]) )                                       \
        {                                                               \
            CxingDebug(#XmacroField " of " #DSFA_Type                   \
                       " should be an integer.\n");                     \
            return (struct value_nativeobj){                            \
                .proper.p = NULL,                                       \
                .type = (const void *)&type_nativeobj_morgoth };        \
        }                                                               \
        ((DSFA_Type *)backing)->XmacroField = htons(args[2].proper.u);  \
        args[2].proper.u = (uint16_t)args[2].proper.u;                  \
        return args[2];                                                 \
    }

#define Xmacro_String(DSFA_Type, XmacroField)                           \
    if( strcmp(#XmacroField, key) == 0 )                                \
    {                                                                   \
        size_t len = sizeof(((DSFA_Type *)backing)->XmacroField);       \
        s2data_t *str = args[2].proper.p;                               \
        if( args[2].type != (const void *)&type_nativeobj_s2impl_str )  \
        {                                                               \
            CxingDebug(#XmacroField " of " #DSFA_Type                   \
                       " should be a string.\n");                       \
            return (struct value_nativeobj){                            \
                .proper.p = NULL,                                       \
                .type = (const void *)&type_nativeobj_morgoth };        \
        }                                                               \
        if( s2data_len(str) != len )                                    \
        {                                                               \
            CxingDebug(#XmacroField " of " #DSFA_Type                   \
                       " should be of %zd bytes.\n", len);              \
            return (struct value_nativeobj){                            \
                .proper.p = NULL,                                       \
                .type = (const void *)&type_nativeobj_morgoth };        \
        }                                                               \
        memcpy(&((DSFA_Type *)backing)->XmacroField,                    \
               s2data_weakmap(str), len);                               \
        return args[2];                                                 \
    }

struct value_nativeobj DSFA_SetImpl0(
    int argn, struct value_nativeobj args[])
{
    // The underlying backing is an `s2data_t`.
    const void *backing = s2data_weakmap(args[0].proper.p);
    const char *key = s2data_weakmap(args[1].proper.p);

    // 2026-06-23 (tentative): Assertions on arguments by caller.
    (void)argn;

#include DSFA_Xmacros_Defs

    CxingDebug("Unrecognized structure field: %s.\n", key);
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj DSFA_InitSetImpl0(
    int argn, struct value_nativeobj args[])
{
    const char *key = s2data_weakmap(args[1].proper.p);

    if( strcmp(key, "__proto__") == 0 )
    {
        return args[0];
    }
    else
    {
        return DSFA_SetImpl0(argn, args);
    }
}

#undef Xmacro_Integer
#undef Xmacro_Unsigned
#undef Xmacro_U32BE
#undef Xmacro_U16BE
#undef Xmacro_String

#endif // defined(DSFA_*Impl0), defined(DSFA_Xmacros_Defs).
