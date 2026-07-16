#define genmac() foo(bar()  bar())
#define foo(x) maceval(x)
#define bar() !.!
#define maceval(x) #x#x
#define s(z) #z
#define t(z) s(z)

char *str = t(genmac());
