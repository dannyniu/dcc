#define Hello(x) "World!" #x
#define Bonjour "G'Day"
#undef Hello
#define Hello(y, ...) (#__VA_ARGS__, y ## y __VA_ARGS__)
#define Bonjour "How'dy"

int x = Hello(1+2+3+4);
double atan2(double, double);
#define M_PI 3.1415926
void rt(void){ float y = Hello(314, ,atan2(23 ,45) *180 / M_PI); }

#if defined Bonjour && defined(Hello)
char *sdecl = "strd";
#endif
