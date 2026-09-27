#define Unite(a, b, c) a + b + c

int foo(void)
{
    return
        Unite(71, 12*20, 10*25) +
        Unite(Unite(59, 93, 8+15), 7, 14) *
        1024 + 768 + 1920 * 1080 + 4096 * 2160;
}
