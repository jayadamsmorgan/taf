#include "util/os.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static char *get_os_string(void) {
    typedef LONG(WINAPI * RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll)
        return _strdup("Windows (unknown version)");

    RtlGetVersionPtr pRtlGetVersion =
        (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");
    if (!pRtlGetVersion)
        return _strdup("Windows (unknown version)");

    RTL_OSVERSIONINFOW info = {0};
    info.dwOSVersionInfoSize = sizeof(info);
    if (pRtlGetVersion(&info) != 0)
        return _strdup("Windows (unknown version)");

    char buf[64];
    snprintf(buf, sizeof(buf), "Windows %lu.%lu.%lu",
             (unsigned long)info.dwMajorVersion,
             (unsigned long)info.dwMinorVersion,
             (unsigned long)info.dwBuildNumber);
    return _strdup(buf);
}

#elif defined(__APPLE__)

#include <sys/sysctl.h>
#include <sys/utsname.h>

char *get_os_string(void) {
    char ver[64] = {0};
    size_t len = sizeof(ver);
    if (sysctlbyname("kern.osproductversion", ver, &len, NULL, 0) == 0) {
        struct utsname u;
        if (uname(&u) == 0) {
            char buf[128];
            snprintf(buf, sizeof(buf), "macOS %s (Darwin %s)", ver, u.release);
            return strdup(buf);
        }
        return strdup(ver);
    }

    // fallback
    struct utsname u;
    if (uname(&u) == 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s %s", u.sysname, u.release);
        return strdup(buf);
    }
    return strdup("macOS (unknown version)");
}

#else

#include <sys/utsname.h>

char *get_os_string(void) {
    struct utsname u;
    if (uname(&u) == 0) {
        size_t need = strlen(u.sysname) + strlen(u.release) + 2;
        char *buf = malloc(need);
        if (!buf)
            return NULL;
        snprintf(buf, need, "%s %s", u.sysname, u.release);
        return buf;
    }
    return strdup("Unknown OS");
}

#endif
