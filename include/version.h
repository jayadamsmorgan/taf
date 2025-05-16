#ifndef VERSION_H
#define VERSION_H

#define _STR(x) #x
#define STR(x) _STR(x)

#define HERNESS_VERSION_MAJOR 0
#define HERNESS_VERSION_MINOR 0
#define HERNESS_VERSION_PATCH 1

#define HERNESS_VERSION                                                        \
    STR(HERNESS_VERSION_MAJOR)                                                 \
    "." STR(HERNESS_VERSION_MINOR) "." STR(HERNESS_VERSION_PATCH)

#endif // VERSION_H
