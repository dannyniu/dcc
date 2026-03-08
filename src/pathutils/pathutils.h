/* DannyNiu/NJF, 2026-03-08. Public Domain. */

#ifndef dcc_pathutils_h
#define dcc_pathutils_h

/// @fn
/// @param path a nul-terminated string.
/// @return number of forward slashes in the string.
int PathCountSlashes(const char *path);

/// @fn
/// @param origin a nul-terminated string.
/// @param path another nul-terminated string.
/// @return
/// a new string allocated in the heap containing
/// the concatenation of the directory name of
/// the origin and the entirity of path, as a
/// valid file path.
char *PathReplaceBasename(const char *origin, const char *path);

#endif /* dcc_pathutils_h */
