#include "modules/web/webdriver.h"

#include "modules/web/requests.h"

#include <curl/curl.h>
#include <curl/easy.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define DRIVER_STARTUP_TIMEOUT_CHECK 200 // ms

static const char *wd_status_str_map[] = {
    "WD_OK",           //
    "WD_ERR_NETWORK",  //
    "WD_ERR_JSON",     //
    "WD_ERR_SCRIPT",   //
    "WD_ERR_NOTFOUND", //
    "WD_ERR_TIMEOUT",  //
};

#if defined(_WIN32) || defined(_WIN64)
static const char *wd_driver_backend_exe_str_map[] = {
    "chromedriver.exe",   //
    "geckodriver.exe",    //
    NULL,                 //
    "msedgedriver.exe",   //
    "IEDriverServer.exe", //
    "operadriver.exe",    //
    NULL,                 //
    NULL,                 //
};
#endif // WINDOWS

#if defined(__linux__)
static const char *wd_driver_backend_exe_str_map[] = {
    "chromedriver",    //
    "geckodriver",     //
    NULL,              //
    "msedgedriver",    //
    NULL,              //
    "operadriver",     //
    "WebKitWebDriver", //
    "WPEWebDriver",    //
};
#endif // LINUX

#if defined(__APPLE__)
static const char *wd_driver_backend_exe_str_map[] = {
    "chromedriver", //
    "geckodriver",  //
    "safaridriver", //
    "msedgedriver", //
    NULL,           //
    "operadriver",  //
    NULL,           //
    NULL,           //
};
#endif // MACOS

const char *wd_driver_executable(wd_driver_backend backend) {
    if (backend < 0 || backend > WD_DRV_WPEDRIVER)
        return NULL;
    return wd_driver_backend_exe_str_map[backend];
}

#if defined(_WIN32) || defined(_WIN64)
static wd_pid_t wd_spawn_driver_win(const char *exe, const char *extra_args,
                                    char errbuf[WD_ERRORSIZE]) {
    char cmdline[1024] = {0};
    snprintf(cmdline, sizeof(cmdline), "\"%s\"%s%s", exe,
             (extra_args && *extra_args) ? " " : "",
             (extra_args && *extra_args) ? extra_args : "");

    STARTUPINFOA si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi = {0};

    BOOL ok =
        CreateProcessA(NULL,    /* lpApplicationName (search on PATH) */
                       cmdline, /* lpCommandLine                      */
                       NULL, NULL, FALSE, /* default security / no inherit */
                       CREATE_NO_WINDOW,  /* don’t pop a console  */
                       NULL, NULL, /* inherit environment & cwd          */
                       &si, &pi);

    if (!ok) {
        snprintf(errbuf, WD_ERRORSIZE, "CreateProcess failed (err=%lu)\n",
                 GetLastError());
        return (wd_pid_t)-1;
    }

    CloseHandle(pi.hThread);
    DWORD ec = STILL_ACTIVE;
    DWORD wait = WaitForSingleObject(pi.hProcess, DRIVER_STARTUP_TIMEOUT_CHECK);

    if (wait == WAIT_OBJECT_0) {
        GetExitCodeProcess(pi.hProcess, &ec);
        snprintf(errbuf, WD_ERRORSIZE, "driver exited immediately (code %lu)",
                 ec);
        CloseHandle(pi.hProcess);
        return (wd_pid_t)-1;
    }

    CloseHandle(pi.hProcess);
    return (wd_pid_t)pi.dwProcessId;
}
#else
#include <spawn.h>
extern char **environ;
// TODO: capture the output of the spawn process and put it in errbuf
// if possible
static wd_pid_t wd_spawn_driver_posix(const char *exe, const char *extra_args,
                                      char errbuf[WD_ERRORSIZE]) {
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);

    // open /dev/null once and map it to stdout (fd 1) and stderr (fd 2)
    posix_spawn_file_actions_addopen(&fa, 1, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_adddup2(&fa, 1, 2); // 2>&1

    char *const argv[] = {
        (char *)exe, (extra_args && *extra_args) ? (char *)extra_args : NULL,
        NULL};

    pid_t pid;
    int rc = posix_spawnp(&pid, exe, &fa, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&fa);

    if (rc != 0) {
        snprintf(errbuf, WD_ERRORSIZE, "posix_spawnp: %s", strerror(rc));
        return -1;
    }

    int waited = 0;
    while (waited < DRIVER_STARTUP_TIMEOUT_CHECK) {
        int st;
        pid_t r = waitpid(pid, &st, WNOHANG);
        if (r == 0) {
            usleep(10 * 1000);
            waited += 10;
            continue;
        }
        if (r == pid) {
            if (WIFEXITED(st))
                snprintf(errbuf, WD_ERRORSIZE, "driver exited (status %d)",
                         WEXITSTATUS(st));
            else if (WIFSIGNALED(st))
                snprintf(errbuf, WD_ERRORSIZE, "driver killed by signal %d",
                         WTERMSIG(st));
            else
                snprintf(errbuf, WD_ERRORSIZE,
                         "driver quit unexpectedly (raw %d)", st);
            return -1;
        }
        if (r == -1 && errno == EINTR)
            continue;
        snprintf(errbuf, WD_ERRORSIZE, "waitpid: %s", strerror(errno));
        return -1;
    }
    // child still alive after grace period -> success
    return (wd_pid_t)pid;
}
#endif

wd_pid_t wd_spawn_driver(wd_driver_backend backend, const char *extra_args,
                         char errbuf[WD_ERRORSIZE]) {
    if (errbuf)
        errbuf[0] = '\0';

    const char *exe = wd_driver_executable(backend);
    if (!exe) {
        snprintf(errbuf, WD_ERRORSIZE, "driver is unavailable on your system");
        return -1;
    }

#if defined(_WIN32) || defined(_WIN64)
    return wd_spawn_driver_win(exe, extra_args, errbuf);
#else
    return wd_spawn_driver_posix(exe, extra_args, errbuf);
#endif
}

const char *wd_status_to_str(wd_status status) {
    return wd_status_str_map[status];
}

static wd_status extract_session_id(struct json_object *root, char **sid_out) {
    /* First try W3C style: {"value":{"sessionId": "..."} } */
    struct json_object *value = NULL;
    if (json_object_object_get_ex(root, "value", &value) &&
        json_object_is_type(value, json_type_object)) {

        struct json_object *sid = NULL;
        if (json_object_object_get_ex(value, "sessionId", &sid) &&
            json_object_is_type(sid, json_type_string)) {
            *sid_out = strdup(json_object_get_string(sid));
            return WD_OK;
        }
    }

    /* Fallback to legacy style: {"sessionId":"...", ...} */
    struct json_object *sid = NULL;
    if (json_object_object_get_ex(root, "sessionId", &sid) &&
        json_object_is_type(sid, json_type_string)) {
        *sid_out = strdup(json_object_get_string(sid));
        return WD_OK;
    }

    return WD_ERR_JSON;
}

wd_status wd_session_start(int drv_port, wd_driver_backend, wd_session_t *out) {

    char drv_url[256];
    snprintf(drv_url, sizeof drv_url, "http://localhost:%d", drv_port);

    char url[256];
    snprintf(url, sizeof url, "%s/session", drv_url);

    char errbuf[CURL_ERROR_SIZE] = {0};

    struct json_object *root =
        requests_http_post_json(url,
                                "{ \"capabilities\": { \"firstMatch\": [ {} ] "
                                "}, \"desiredCapabilities\": {} }",
                                errbuf);

    if (!root) {
        /* choose an error code based on curl vs json parse msg */
        if (strcmp(errbuf, "JSON parse fail") == 0)
            return WD_ERR_JSON;
        else
            return WD_ERR_NETWORK;
    }

    char *sid = NULL;
    wd_status rc = extract_session_id(root, &sid);

    if (rc == WD_OK) {
        if (out) {
            out->base = strdup(drv_url);
            out->id = sid;
        }
    } else if (sid) {
        free(sid);
    }

    json_object_put(root); /* free parsed JSON */
    return rc;
}

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

static int wd_kill_driver_windows(wd_pid_t pid) {
    HANDLE h = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, (DWORD)pid);
    if (!h)
        return -1;

    /* ask politely first: the driver *should* have quit already, but... */
    BOOL ok = TerminateProcess(h, 0); /* hard kill */
    WaitForSingleObject(h, 5000);     /* up to 5 s */
    CloseHandle(h);
    return ok ? 0 : -1;
}
#else
#include <signal.h>

static int wd_kill_driver_posix(wd_pid_t pid) {
    /* SIGTERM → give it a chance to flush logs, close sockets */
    if (kill(pid, SIGTERM) == -1 && errno == ESRCH)
        return 0; /* already gone */

    /* Wait up to ~1 s */
    for (int i = 0; i < 10; ++i) {
        if (waitpid(pid, NULL, WNOHANG) == pid)
            return 0;       /* reaped     */
        usleep(100 * 1000); /* 100 ms     */
    }

    /* Last resort */
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
    return 0;
}
#endif

void wd_kill_driver(wd_pid_t pid) {
#if defined(_WIN32) || defined(_WIN64)
    wd_kill_driver_windows(pid);
#else
    wd_kill_driver_posix(pid);
#endif
}

wd_status wd_session_end(wd_session_t *session) {
    char url[256];
    snprintf(url, sizeof url, "%s/session/%s", session->base, session->id);

    char errbuf[CURL_ERROR_SIZE] = {0};
    struct json_object *resp = requests_http_delete_json(url, errbuf);

    if (resp) {
        json_object_put(resp);
    }

    wd_kill_driver(session->driver_pid);

    free(session->base);
    free(session->id);

    return WD_OK; /* or propagate earlier curl/json error */
}

wd_status wd_session_cmd(wd_session_t *session, wd_method method,
                         const char *endpoint, json_object *payload,
                         json_object **out) {
    char url[256];
    snprintf(url, sizeof url, "%s/session/%s/%s", session->base, session->id,
             endpoint);

    char errbuf[CURL_ERROR_SIZE] = {0};

    const char *payload_str = json_object_to_json_string(payload);

    switch (method) {
    case WD_METHOD_POST:
        *out = requests_http_post_json(url, payload_str, errbuf);
        break;
    case WD_METHOD_PUT:
        *out = requests_http_put_json(url, payload_str, errbuf);
        break;
    case WD_METHOD_DELETE:
        *out = requests_http_delete_json(url, errbuf);
        break;
    case WD_METHOD_GET:
        *out = requests_http_get_json(url, errbuf);
        break;
    }

    if (!*out) {
        if (strcmp(errbuf, "JSON parse fail") == 0)
            return WD_ERR_JSON;
        else
            return WD_ERR_NETWORK;
    }

    return WD_OK;
}
