int printf(const char *fmt, ...);
int atoi(const char *);
const char *getenv(const char *);
int main(int argc, char *argv[])
{
    void *utf32 = U"Hello World!";
    void *utf16 = u"Hello World!";
    void *utf8 = u8"Hello World!";
    void *ascii = "Hello World! \"";
    void *strtab[] = {
        ascii, utf8, utf16, utf32,
    };
    int env = atoi(getenv("CHARSET_CODE"));

    double
        a = 100,
        b = 125e3,
        c = 10.96,
        d = 0x52,
        e = 0x7fp-6,
        f = .36,
        g = -12.3,
        h = +0x384.p7,
        i = +28,
        j = 37e-4,
        k = 0x.32p0;

    (void)argc;
    (void)argv;

    printf("%s\n", (char *)strtab[env]);
}
