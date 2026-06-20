Danny's Compiler Collection.
====

This project implement compiler for C and an interpreter for CXING. 
The implementation of C is in its beginning phase, a partial pre-processor
is being implemented as of 2026-06-20. The implementation of CXING is
largely complete, and is ready for extensive testing.

(TL;DR) Compile and Install
----

On POSIX platforms:

```
./configure
make
make install # this one requires appropriate priviledge to relevant directory.
```

About CXING
====

CXING is an entirely new language designed from ground up, with high-level
capabilities such as automatic resource management, and safety features
such as nullish value propagation.

A recurring problem with existing languages, is that they'd fail too
catastrophically, unless the developer put effort into verbosely checking
every error, when in reality (at least according to my personal experience)
they are able to put up with a gradually degraded result.

I (@dannyniu) had considered implementing JavaScript as an alternative,
but it's having too much feature (e.g. UTF-16-based String type, PCRE-based
regular expression semantic) that would be too burdonsome to implement 
completely, and it's being constantly developed.

CXING on the other hand, is minimally implementable, it's developed with
common data-processing applications in mind, and can be easily coded to be
robust in face of the occasional failures.
