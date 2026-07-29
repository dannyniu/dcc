typedef int my_integer_type;
typedef long my_word;
int foo(void){
    typedef short my_type_id;
    {
        typedef float scopedf, shadowf;
        typedef int scopedi, shadowi;
    }
    my_type_id u = 0x123, *v = &u;
    my_word x = (my_type_id)*v;
    typedef char byte, octet;
}
typedef void *memloc;
typedef void const *restrict data;
