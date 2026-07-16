#if 1 + 1 ? 0 : 315
int foo = 0x12;
#elif true && false
#if 3/2
int bar = 192;
#elif 3/4
int bar = 256;
#else
int bar = 512;
#endif
#else
int baz = 384;
#endif

#if __has_include("rescan.c") && __has_include(<rescan.c>)
int inc_rescan = true;
#endif

#define mf(s) ctrl-s
#define hn <mf(exprs).c>
#if __has_include(hn)
int main(){ return 0; }
#endif
