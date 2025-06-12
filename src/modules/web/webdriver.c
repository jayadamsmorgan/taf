#include "modules/web/webdriver.h"

#include "internal_logging.h"
#include "modules/web/requests.h"

#include <curl/curl.h>
#include <curl/easy.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define DRIVER_STARTUP_TIMEOUT_CHECK 1000 // ms

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
#include <stdio.h>
#include <windows.h>

static wd_pid_t wd_spawn_driver_win(const char *exe, const char *extra_args,
                                    char errbuf[WD_ERRORSIZE]) {
    // Build command line
    char cmdline[1024];
    snprintf(cmdline, sizeof(cmdline), "\"%s\"%s%s", exe,
             (extra_args && *extra_args) ? " " : "",
             (extra_args && *extra_args) ? extra_args : "");

    // Create an inheritable pipe for BOTH stdout & stderr
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    HANDLE readPipe = NULL, writePipe = NULL;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) {
        snprintf(errbuf, WD_ERRORSIZE, "CreatePipe failed (err=%lu)",
                 GetLastError());
        return (wd_pid_t)-1;
    }
    // Ensure the read handle isn't inherited
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    // Setup STARTUPINFO to redirect stdout/stderr into our pipe
    STARTUPINFOA si = {.cb = sizeof(si)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = writePipe;
    si.hStdError = writePipe;

    PROCESS_INFORMATION pi = {0};
    BOOL ok = CreateProcessA(
        NULL,                         // lpApplicationName
        cmdline,                      // lpCommandLine
        NULL, NULL,                   // lpProcessAttributes, lpThreadAttributes
        TRUE,                         // bInheritHandles
        CREATE_NO_WINDOW, NULL, NULL, // lpEnvironment, lpCurrentDirectory
        &si, &pi);

    // Parent no longer needs the write end
    CloseHandle(writePipe);
    if (!ok) {
        DWORD err = GetLastError();
        snprintf(errbuf, WD_ERRORSIZE, "CreateProcessA failed (err=%lu)", err);
        CloseHandle(readPipe);
        return (wd_pid_t)-1;
    }
    // We don't need the thread handle
    CloseHandle(pi.hThread);

    // Wait up to DRIVER_STARTUP_TIMEOUT_CHECK ms for child to exit
    DWORD wait = WaitForSingleObject(pi.hProcess, DRIVER_STARTUP_TIMEOUT_CHECK);
    if (wait == WAIT_OBJECT_0) {
        // Child died immediately — grab its exit code
        DWORD ec = STILL_ACTIVE;
        GetExitCodeProcess(pi.hProcess, &ec);

        // Read up to WD_ERRORSIZE-1 bytes from pipe
        char buf[WD_ERRORSIZE];
        DWORD total = 0, n = 0;
        while (total < WD_ERRORSIZE - 1 &&
               ReadFile(readPipe, buf + total, WD_ERRORSIZE - 1 - total, &n,
                        NULL) &&
               n > 0) {
            total += n;
        }
        buf[total] = '\0';

        // Format into errbuf
        snprintf(errbuf, WD_ERRORSIZE,
                 "driver exited immediately (code %lu): %s", ec,
                 buf[0] ? buf : "(no output)");

        CloseHandle(pi.hProcess);
        CloseHandle(readPipe);
        return (wd_pid_t)-1;
    }

    // If wait == WAIT_TIMEOUT, child is still alive — success
    CloseHandle(pi.hProcess);
    CloseHandle(readPipe);
    return (wd_pid_t)pi.dwProcessId;
}
#else

#include <spawn.h>
extern char **environ;

static wd_pid_t wd_spawn_driver_posix(const char *exe, size_t argc, char **args,
                                      char errbuf[WD_ERRORSIZE]) {
    LOG("Spawning driver with POSIX...");
    int pipe_all[2];
    if (pipe(pipe_all) == -1) {
        const char *err = strerror(errno);
        LOG("Unable to create pipe: %s", err);
        snprintf(errbuf, WD_ERRORSIZE, "pipe: %s", err);
        return (wd_pid_t)-1;
    }

    LOG("Spawning file actions...");
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);

    // In child: close read-end, dup write-end onto stdout & stderr, then
    // close write-end
    posix_spawn_file_actions_addclose(&fa, pipe_all[0]);
    posix_spawn_file_actions_adddup2(&fa, pipe_all[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&fa, pipe_all[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&fa, pipe_all[1]);

    // build argv array (argv[0] = exe, then args[0..argc-1], then NULL)
    LOG("Building argv array...");
    char **argv = malloc((argc + 2) * sizeof(char *));
    if (!argv) {
        LOG("Out of memory.");
        snprintf(errbuf, WD_ERRORSIZE, "malloc: out of memory");
        posix_spawn_file_actions_destroy(&fa);
        close(pipe_all[0]);
        close(pipe_all[1]);
        return (wd_pid_t)-1;
    }
    argv[0] = (char *)exe;
    for (size_t i = 0; i < argc; i++) {
        argv[i + 1] = args[i];
    }
    argv[argc + 1] = NULL;

    // spawn the child
    LOG("Spawning driver...");
    pid_t pid;
    int rc = posix_spawnp(&pid, exe, &fa, NULL, argv, environ);
    LOG("rc: %d, pid: %d", rc, pid);

    LOG("Cleaning up...");
    posix_spawn_file_actions_destroy(&fa);
    free(argv);

    if (rc != 0) {
        const char *err = strerror(rc);
        LOG("Unable to spawn: %s", err);
        snprintf(errbuf, WD_ERRORSIZE, "posix_spawnp: %s (%s)", err, exe);
        close(pipe_all[0]);
        close(pipe_all[1]);
        return (wd_pid_t)-1;
    }

    // parent: close write-end, keep read-end for possible error capture
    LOG("Closing write-end...");
    close(pipe_all[1]);

    // give the child a short grace period; if it dies, capture its output
    LOG("Waiting on driver...");
    int waited = 0;
    while (waited < DRIVER_STARTUP_TIMEOUT_CHECK) {
        int st;
        pid_t r = waitpid(pid, &st, WNOHANG);
        LOG("r = %d", r);
        if (r == 0) {
            LOG("Waiting 10 ms...");
            usleep(10 * 1000);
            waited += 10;
            continue;
        }
        if (r == pid) {
            // child died -> read up to WD_ERRORSIZE-1 bytes from the pipe
            LOG("Child died.");
            char buf[WD_ERRORSIZE];
            ssize_t off = 0, n;
            while ((n = read(pipe_all[0], buf + off, WD_ERRORSIZE - 1 - off)) >
                   0) {
                off += n;
                if (off >= WD_ERRORSIZE - 1)
                    break;
            }
            buf[off] = '\0';
            close(pipe_all[0]);

            // format the combined error
            if (WIFEXITED(st)) {
                LOG("Driver exited with status %d: %s", WEXITSTATUS(st), buf);
                snprintf(errbuf, WD_ERRORSIZE, "driver exited (status %d): %s",
                         WEXITSTATUS(st), buf);
            } else if (WIFSIGNALED(st)) {
                LOG("Driver was killed by signal %d: %s", WTERMSIG(st), buf);
                snprintf(errbuf, WD_ERRORSIZE, "driver killed by signal %d: %s",
                         WTERMSIG(st), buf);
            } else {
                LOG("Driver quit unexpectedly (raw %d): %s", st, buf);
                snprintf(errbuf, WD_ERRORSIZE,
                         "driver quit unexpectedly (raw %d): %s", st, buf);
            }
            return (wd_pid_t)-1;
        }
        if (r == -1 && errno == EINTR) {
            continue;
        }
        const char *err = strerror(errno);
        LOG("waitpid: %s", err);
        snprintf(errbuf, WD_ERRORSIZE, "waitpid: %s", err);
        close(pipe_all[0]);
        return (wd_pid_t)-1;
    }

    // child still alive after grace period -> success
    LOG("Closing read-end...");
    close(pipe_all[0]);

    LOG("Successfully spawned driver.");
    return (wd_pid_t)pid;
}
#endif

wd_pid_t wd_spawn_driver(wd_driver_backend backend, size_t argc, char **args,
                         char errbuf[WD_ERRORSIZE]) {
    LOG("Spawning driver...");
    if (errbuf)
        errbuf[0] = '\0';

    const char *exe = wd_driver_executable(backend);
    if (!exe) {
        LOG("Selected web-driver is not available on current OS.");
        snprintf(errbuf, WD_ERRORSIZE, "driver is unavailable on your system");
        return -1;
    }
    LOG("Executable: %s", exe);
    for (size_t i = 0; i < argc; i++) {
        LOG("args[%zu]: %s", i, args[i]);
    }

#if defined(_WIN32) || defined(_WIN64)
    return wd_spawn_driver_win(exe, argc, args, errbuf);
#else
    return wd_spawn_driver_posix(exe, argc, args, errbuf);
#endif
}

const char *wd_status_to_str(wd_status status) {
    return wd_status_str_map[status];
}

static wd_status extract_session_id(struct json_object *root, char **sid_out) {
    struct json_object *value = NULL, *field = NULL;

    // Try W3C success: { "value": { "sessionId": "…" } }
    if (json_object_object_get_ex(root, "value", &value) &&
        json_object_is_type(value, json_type_object) &&
        json_object_object_get_ex(value, "sessionId", &field) &&
        json_object_is_type(field, json_type_string)) {
        *sid_out = strdup(json_object_get_string(field));
        return WD_OK;
    }

    // Fallback legacy success: { "sessionId": "…" }
    if (json_object_object_get_ex(root, "sessionId", &field) &&
        json_object_is_type(field, json_type_string)) {
        *sid_out = strdup(json_object_get_string(field));
        return WD_OK;
    }

    // If we get here, no sessionId — try to extract an error message.
    //    W3C error: { "value": { "error": "...", "message": "..." } }
    if (json_object_object_get_ex(root, "value", &value) &&
        json_object_is_type(value, json_type_object) &&
        json_object_object_get_ex(value, "message", &field) &&
        json_object_is_type(field, json_type_string)) {
        *sid_out = strdup(json_object_get_string(field));
        return WD_ERR_JSON;
    }

    // Some legacy drivers might return top‐level error/message:
    //    { "error": "...", "message": "..." }
    if (json_object_object_get_ex(root, "message", &field) &&
        json_object_is_type(field, json_type_string)) {
        *sid_out = strdup(json_object_get_string(field));
        return WD_ERR_JSON;
    }

    return WD_ERR_JSON;
}

wd_status wd_session_start(int drv_port, wd_driver_backend,
                           char errbuf[WD_ERRORSIZE], wd_session_t *out) {
    LOG("Starting Web-Driver session...");

    char drv_url[256];
    snprintf(drv_url, sizeof drv_url, "http://localhost:%d", drv_port);
    LOG("Driver URL: %s", drv_url);

    char url[256];
    snprintf(url, sizeof url, "%s/session", drv_url);
    LOG("URL: %s", url);

    char curl_errbuf[CURL_ERROR_SIZE] = {0};

    struct json_object *root =
        requests_http_post_json(url,
                                "{ \"capabilities\": { \"firstMatch\": [ {} ] "
                                "}, \"desiredCapabilities\": {} }",
                                curl_errbuf);

    if (!root) {
        LOG("CURL error: %s", curl_errbuf);
        if (strcmp(curl_errbuf, "JSON parse fail") == 0)
            return WD_ERR_JSON;

        memcpy(errbuf, curl_errbuf, json_min(CURL_ERROR_SIZE, WD_ERRORSIZE));
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
        memcpy(errbuf, sid, json_min(strlen(sid) + 1, WD_ERRORSIZE));
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
    if (session->driver_pid == -1)
        return WD_OK; // Already been closed

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
    session->id = NULL;
    session->driver_pid = -1;

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
