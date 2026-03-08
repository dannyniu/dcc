/* DannyNiu/NJF, 2026-03-08. Public Domain. */

#include <stdlib.h>
#include <string.h>

int PathCountSlashes(const char *path)
{
    int ret = 0;
    while( *path )
    {
        if( *path++ == '/' )
            ret ++;
    }
    return ret;
}

char *PathReplaceBasename(const char *origin, const char *path)
{
    size_t s, t;
    char *cat;

    s = strlen(path);
    t = strlen(origin);

    while( origin[--t] == '/' );
    while( origin[--t] != '/' );

    s += t;
    if( path[0] != '/' ) s++;

    cat = calloc(1, s+1);
    if( !cat ) return NULL; // no error recovery.

    memcpy(cat, origin, t);
    if( path[0] != '/' )
        cat[t] = '/';

    if( path[0] == '.' && path[1] == '/' )
        strcat(cat, path+strspn(path+1, "/")+1);
    else strcat(cat, path);
    return cat;
}
