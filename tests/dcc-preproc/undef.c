#define Hello(x) "World!" #x
#define Bonjour "G'Day"
#undef Hello
#define Hello(y) y ## y
#define Bonjour "How'dy"

Hello(1+2+3+4)
