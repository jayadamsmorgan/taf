#ifndef VERSION_H
#define VERSION_H

#define _STR(x) #x
#define STR(x) _STR(x)

#define TAF_VERSION_MAJOR 1
#define TAF_VERSION_MINOR 0
#define TAF_VERSION_PATCH 4

#define TAF_VERSION                                                            \
    STR(TAF_VERSION_MAJOR)                                                     \
    "." STR(TAF_VERSION_MINOR) "." STR(TAF_VERSION_PATCH)

#define TAF_VERSION_NUM(MAJOR, MINOR, PATCH)                                   \
    ((MAJOR) * 1000 + (MINOR) * 100 + (PATCH))

#define TAF_VERSION_NUM_CURRENT                                                \
    TAF_VERSION_NUM(TAF_VERSION_MAJOR, TAF_VERSION_MINOR, TAF_VERSION_PATCH)

#endif // VERSION_H
