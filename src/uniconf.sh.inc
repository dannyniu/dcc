cflags_proj="\
-I "$(realpath ./../../contrib/SafeTypes2/src)"
-I "$(realpath ./../../contrib/librematch/src)"
"

src_proj="\
./../contrib/SafeTypes2/src/s2data.c
./../contrib/SafeTypes2/src/s2dict.c
./../contrib/SafeTypes2/src/s2obj.c
./../contrib/SafeTypes2/src/mem-intercept.c
./../contrib/SafeTypes2/src/siphash.c
./../contrib/librematch/src/librematch.c
./../contrib/librematch/src/regcomp-brackets.c
./../contrib/librematch/src/regcomp-bre.c
./../contrib/librematch/src/regcomp-ere.c
./../contrib/librematch/src/regcomp-interval.c
"