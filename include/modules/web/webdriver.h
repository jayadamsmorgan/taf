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

#include <json.h>

#define WD_ERRORSIZE 1024

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

typedef enum {
    WD_METHOD_POST = 0,
    WD_METHOD_PUT = 1,
    WD_METHOD_DELETE = 2,
    WD_METHOD_GET = 3,
} wd_method;

wd_status wd_session_start(int drv_port, wd_driver_backend backend,
                           char errbuf[WD_ERRORSIZE], wd_session_t *out);

wd_status wd_session_cmd(wd_session_t *session, wd_method method,
                         const char *endpoint, json_object *payload,
                         json_object **out);

wd_status wd_session_end(wd_session_t *session);

wd_pid_t wd_spawn_driver(wd_driver_backend backend, size_t argc, char **args,
                         char errbuf[WD_ERRORSIZE]);

void wd_kill_driver(wd_pid_t pid);

const char *wd_status_to_str(wd_status status);

const char *wd_driver_executable(wd_driver_backend backend);

#endif // WEB_WEBDRIVER_H
