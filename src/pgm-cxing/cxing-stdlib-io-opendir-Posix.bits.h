/* DannyNiu/NJF, 2026-05-23. Public Domain. */

#include "cxing-stdlib.h"
#include <dirent.h>

static void closedir_s2finalfunc(DIR *x)
{
    closedir(x);
}

CxingMethodValueWithImpl(DIR, ReadDir);
CxingMethodValueWithImpl(DIR, RewindDir);
CxingMethodValueWithImpl(DIR, Copy);
CxingMethodValueWithImpl(DIR, Final);

struct value_nativeobj CxingImpl_DIR_ReadDir(
    int argn, struct value_nativeobj args[])
{
    struct dirent *ent;
    s2data_t *ret;
    AssertArgN(1);
    AssertArgImpl(0, DIR, "directory handle");

    ret = s2data_create(0);
    if( !ret )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);
    ent = readdir(s2ref_unwrap(args[0].proper.p));

    if( !ent )
    {
        return (struct value_nativeobj){
            .proper.p = ret,
            .type = (const void *)&type_nativeobj_s2impl_str };
    }

    if( s2data_puts(ret, ent->d_name, strlen(ent->d_name)) != 0 ||
        s2data_putfin(ret) != 0 )
    {
        int subret = errno;
        s2obj_release(ret->pobj);
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_s2impl_str };
}

struct value_nativeobj CxingImpl_DIR_RewindDir(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, DIR, "directory handle");

    rewinddir(s2ref_unwrap(args[0].proper.p));

    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_DIR_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, DIR, "directory handle");

    return CxingImpl_s2Obj_Copy(argn, args);
}

struct value_nativeobj CxingImpl_DIR_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, DIR, "directory handle");

    return CxingImpl_s2Obj_Final(argn, args);
}

const type_nativeobj_struct_p4 type_nativeobj_DIR = {
    .typeid = valtyp_obj,
    .n_entries = 4,
    .static_members = {
        { .name = "readdir", .member = &CxingValue_DIR_ReadDir },
        { .name = "rewinddir", .member = &CxingValue_DIR_RewindDir },
        { .name = "__copy__", .member = &CxingValue_DIR_Copy },
        { .name = "__final__", .member = &CxingValue_DIR_Final },
    },
};

struct value_nativeobj CxingImpl_OpenDir(
    int argn, struct value_nativeobj args[])
{
    DIR *dx;
    s2ref_t *ret;

    AssertArgN(1);
    AssertArgImpl(0, s2impl_str, "string");

    dx = opendir(s2data_weakmap(args[0].proper.p));
    if( !dx )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    ret = s2ref_create(dx, (s2ref_final_func_t)closedir_s2finalfunc);
    if( !ret )
    {
        int subret = errno;
        closedir(dx);
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_DIR };
}
