/* DannyNiu/NJF, 2026-07-05. Public Domain */

#include <stdarg.h>
#include <stdio.h>

void ccDiagnose(int level, const char *msg, ...)
{
    static const char *levelmsg[] = {
        "Fatal",
        "Error",
        "Warning",
        "Note",
        "Trace",
    };

    va_list ap;
    va_start(ap, msg);

    fprintf(stderr, "[%s]: ", levelmsg[level]);
    vfprintf(stderr, msg, ap);
    va_end(ap);
}
