src_common="\
cpp-ctrlline-define.c
cpp-macro-expand.c
cpp-master.c
langlex/langlex-c.c
lex-common/lex.c
lex-common/rope.c
infra/s2bools.c
pathutils/pathutils.c
./../contrib/SafeTypes2/src/s2data.c
./../contrib/SafeTypes2/src/s2dict.c
./../contrib/SafeTypes2/src/s2list.c
./../contrib/SafeTypes2/src/s2obj.c
./../contrib/SafeTypes2/src/s2ref.c
./../contrib/SafeTypes2/src/mem-intercept.c
./../contrib/SafeTypes2/src/siphash.c
./../contrib/librematch/src/librematch.c
./../contrib/librematch/src/regcomp-brackets.c
./../contrib/librematch/src/regcomp-bre.c
./../contrib/librematch/src/regcomp-ere.c
./../contrib/librematch/src/regcomp-interval.c
"

memintercept='-DINTERCEPT_MEM_CALLS'
#memintercept=''

sanitizers='-fsanitize=address'
sanitizers=''
