#ifndef WEB_WEBDRIVER_H
#define WEB_WEBDRIVER_H

#if defined(_WIN32) || defined(_WIN64)
#define NOMINMAX
#include <windows.h>
typedef DWORD wd_pid_t;
#else /* POSIX */
#include <sys/types.h>
#include <unistd.h>
typedef pid_t wd_pid_t;
#endif

#define WD_ERRORSIZE 100

typedef enum {
    WD_DRV_CHROMEDRIVER = 0,
    WD_DRV_GECKODRIVER = 1,
    WD_DRV_SAFARIDRIVER = 2,
    WD_DRV_MSEDGEDRIVER = 3,
    WD_DRV_IEDRIVER = 4,
    WD_DRV_OPERADRIVER = 5,
    WD_DRV_WEBKITDRIVER = 6,
    WD_DRV_WPEDRIVER = 7,
} wd_driver_backend;

typedef enum {
    WD_OK = 0,
    WD_ERR_NETWORK = 1,
    WD_ERR_JSON = 2,
    WD_ERR_SCRIPT = 3,
    WD_ERR_NOTFOUND = 4,
    WD_ERR_TIMEOUT = 5,
} wd_status;

typedef struct {
    char *base;
    char *id;

    wd_pid_t driver_pid;
} wd_session_t;

wd_status wd_session_start(int drv_port, wd_driver_backend backend,
                           wd_session_t *out);

wd_status wd_session_end(wd_session_t *session);

wd_pid_t wd_spawn_driver(wd_driver_backend backend, const char *extra_args,
                         char errbuf[WD_ERRORSIZE]);

const char *wd_status_to_str(wd_status status);

const char *wd_driver_executable(wd_driver_backend backend);

#endif // WEB_WEBDRIVER_H
