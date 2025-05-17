#ifndef VERSION_H
#define VERSION_H

#define _STR(x) #x
#define STR(x) _STR(x)

#define TAF_VERSION_MAJOR 0
#define TAF_VERSION_MINOR 0
#define TAF_VERSION_PATCH 1

#define TAF_VERSION                                                            \
    STR(TAF_VERSION_MAJOR)                                                     \
    "." STR(TAF_VERSION_MINOR) "." STR(TAF_VERSION_PATCH)

#endif // VERSION_H
