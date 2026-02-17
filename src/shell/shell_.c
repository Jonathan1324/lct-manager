#include "shell_.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#endif

const CommandResult invalidCommandResult = {
    -1,
    NULL,
    NULL
};

CommandResult invokeSystemCall(const char* cmd)
{
#ifdef _WIN32
    CommandResult result = {0, NULL, NULL};
    HANDLE outRead, outWrite, errRead, errWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&outRead, &outWrite, &sa, 0) ||
        !CreatePipe(&errRead, &errWrite, &sa, 0)) {
        result.exit_code = -1;
        return result;
    }

    SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(outWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(errWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    STARTUPINFO si = { sizeof(STARTUPINFO) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = outWrite;
    si.hStdError  = errWrite;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi;

    size_t cmdLen = strlen(cmd);
    char* cmdLine = (char*)malloc(cmdLen + 8); // "cmd /C " + null
    if (!cmdLine) {
        CloseHandle(outRead); CloseHandle(outWrite);
        CloseHandle(errRead); CloseHandle(errWrite);
        result.exit_code = -1;
        return result;
    }
    strcpy(cmdLine, "cmd /C ");
    strcat(cmdLine, cmd);

    if (!CreateProcess(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        free(cmdLine);
        CloseHandle(outRead); CloseHandle(outWrite);
        CloseHandle(errRead); CloseHandle(errWrite);
        result.exit_code = -1;
        return result;
    }
    free(cmdLine);

    CloseHandle(outWrite);
    CloseHandle(errWrite);

    size_t stdoutCap = 4096, stdoutLen = 0;
    size_t stderrCap = 4096, stderrLen = 0;
    result.stdout_str = (char*)malloc(stdoutCap);
    result.stderr_str = (char*)malloc(stderrCap);
    if (!result.stdout_str || !result.stderr_str) {
        if (result.stdout_str) free(result.stdout_str);
        if (result.stderr_str) free(result.stderr_str);
        result.stdout_str = NULL;
        result.stderr_str = NULL;

        result.exit_code = -1;
        return result;
    }

    char buf[512];
    DWORD n;
    BOOL running = TRUE;

    while (running) {
        running = FALSE;

        // STDOUT
        DWORD avail = 0;
        if (PeekNamedPipe(outRead, NULL, 0, NULL, &avail, NULL) && avail > 0) {
            running = TRUE;
            if (ReadFile(outRead, buf, sizeof(buf), &n, NULL) && n > 0) {
                if (stdoutLen + n + 1 > stdoutCap) {
                    stdoutCap = (stdoutLen + n + 1) * 2;
                    result.stdout_str = (char*)realloc(result.stdout_str, stdoutCap);
                }
                memcpy(result.stdout_str + stdoutLen, buf, n);
                stdoutLen += n;
            }
        }

        // STDERR
        avail = 0;
        if (PeekNamedPipe(errRead, NULL, 0, NULL, &avail, NULL) && avail > 0) {
            running = TRUE;
            if (ReadFile(errRead, buf, sizeof(buf), &n, NULL) && n > 0) {
                if (stderrLen + n + 1 > stderrCap) {
                    stderrCap = (stderrLen + n + 1) * 2;
                    result.stderr_str = (char*)realloc(result.stderr_str, stderrCap);
                }
                memcpy(result.stderr_str + stderrLen, buf, n);
                stderrLen += n;
            }
        }

        DWORD state = WaitForSingleObject(pi.hProcess, 0);
        if (state == WAIT_TIMEOUT) running = TRUE;
    }

    result.stdout_str[stdoutLen] = '\0';
    result.stderr_str[stderrLen] = '\0';

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    result.exit_code = (int)exitCode;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(outRead);
    CloseHandle(errRead);

    return result;
#else
    CommandResult result = {0, NULL, NULL};
    int outPipe[2], errPipe[2];
    if (pipe(outPipe) != 0 || pipe(errPipe) != 0) {
        result.exit_code = -1;
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        result.exit_code = -1;
        return result;
    }

    if (pid == 0) {
        dup2(outPipe[1], STDOUT_FILENO);
        dup2(errPipe[1], STDERR_FILENO);
        close(outPipe[0]); close(outPipe[1]);
        close(errPipe[0]); close(errPipe[1]);
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        _exit(127);
    } else {
        close(outPipe[1]);
        close(errPipe[1]);

        size_t stdoutCap = 4096, stdoutLen = 0;
        size_t stderrCap = 4096, stderrLen = 0;
        result.stdout_str = (char*)malloc(stdoutCap);
        result.stderr_str = (char*)malloc(stderrCap);
        if (!result.stdout_str || !result.stderr_str) { 
            if (result.stdout_str) free(result.stdout_str);
            if (result.stderr_str) free(result.stderr_str);
            result.stdout_str = NULL;
            result.stderr_str = NULL;

            result.exit_code = -1; 
            return result;
        }

        char buf[512];
        int out_done = 0, err_done = 0;

        int flags_out = fcntl(outPipe[0], F_GETFL, 0);
        fcntl(outPipe[0], F_SETFL, flags_out | O_NONBLOCK);
        int flags_err = fcntl(errPipe[0], F_GETFL, 0);
        fcntl(errPipe[0], F_SETFL, flags_err | O_NONBLOCK);

        while (!out_done || !err_done) {
            fd_set readfds;
            FD_ZERO(&readfds);
            if (!out_done) FD_SET(outPipe[0], &readfds);
            if (!err_done) FD_SET(errPipe[0], &readfds);

            int maxfd = (outPipe[0] > errPipe[0] ? outPipe[0] : errPipe[0]) + 1;
            if (select(maxfd, &readfds, NULL, NULL, NULL) < 0) break;

            // STDOUT
            if (FD_ISSET(outPipe[0], &readfds)) {
                ssize_t n = read(outPipe[0], buf, sizeof(buf));
                if (n > 0) {
                    if (stdoutLen + n + 1 > stdoutCap) {
                        stdoutCap = (stdoutLen + n + 1) * 2;
                        result.stdout_str = (char*)realloc(result.stdout_str, stdoutCap);
                    }
                    memcpy(result.stdout_str + stdoutLen, buf, n);
                    stdoutLen += n;
                } else {
                    out_done = 1;
                }
            }

            // STDERR
            if (FD_ISSET(errPipe[0], &readfds)) {
                ssize_t n = read(errPipe[0], buf, sizeof(buf));
                if (n > 0) {
                    if (stderrLen + n + 1 > stderrCap) {
                        stderrCap = (stderrLen + n + 1) * 2;
                        result.stderr_str = (char*)realloc(result.stderr_str, stderrCap);
                    }
                    memcpy(result.stderr_str + stderrLen, buf, n);
                    stderrLen += n;
                } else {
                    err_done = 1;
                }
            }
        }

        result.stdout_str[stdoutLen] = '\0';
        result.stderr_str[stderrLen] = '\0';

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) result.exit_code = WEXITSTATUS(status);
        else result.exit_code = -1;

        close(outPipe[0]);
        close(errPipe[0]);
    }

    return result;
#endif
}

#ifdef _WIN32
CommandResult invokeShellCall(const char* cmd)
{
    const char cmd_before[] = "powershell -Command \"";
    const char cmd_after[] = "\"";
    unsigned int cmd_length = strlen(cmd);

    unsigned int length = sizeof(cmd_before) - 1 + sizeof(cmd_after) - 1 + cmd_length + 1;; // +1 for '\0'
    for (unsigned int i = 0; i < cmd_length; i++) {
        if (cmd[i] == '\"') length++; // Need to include '\' before the '"'
    }

    char* full_cmd = (char*)malloc(length);
    if (!full_cmd) return invalidCommandResult;

    unsigned int pos = 0;
    memcpy(full_cmd + pos, cmd_before, sizeof(cmd_before) - 1);
    pos += sizeof(cmd_before) - 1;

    for (unsigned int i = 0; i < cmd_length; i++) {
        if (cmd[i] == '"') {
            full_cmd[pos++] = '\\';
            full_cmd[pos++] = '"';
        } else {
            full_cmd[pos++] = cmd[i];
        }
    }

    memcpy(full_cmd + pos, cmd_after, sizeof(cmd_after) - 1);
    pos += sizeof(cmd_after) - 1;

    full_cmd[pos] = '\0';

    CommandResult res = invokeSystemCall(full_cmd);
    free(full_cmd);
    return res;
}
#endif

CommandResult shell3Bases(const char* b1, const char* b2, const char* b3, const char* c1, const char* c2)
{
    unsigned int b1_length = strlen(b1);
    unsigned int b2_length = strlen(b2);
    unsigned int b3_length = strlen(b3);
    unsigned int c1_length = strlen(c1);
    unsigned int c2_length = strlen(c2);
    unsigned int length = b1_length + b2_length + b3_length + c1_length + c2_length + 1;

    char* cmd = (char*)malloc(length);
    if (!cmd) return invalidCommandResult;

    char* pos = cmd;

    memcpy(pos, b1, b1_length);
    pos += b1_length;

    memcpy(pos, c1, c1_length);
    pos += c1_length;

    memcpy(pos, b2, b2_length);
    pos += b2_length;

    memcpy(pos, c2, c2_length);
    pos += c2_length;

    memcpy(pos, b3, b3_length);
    pos += b3_length;

    *pos = '\0';

    CommandResult res = invokeShellCall(cmd);
    free(cmd);
    return res;
}

CommandResult shell2Bases(const char* b1, const char* b2, const char* c1)
{
    unsigned int b1_length = strlen(b1);
    unsigned int b2_length = strlen(b2);
    unsigned int c1_length = strlen(c1);
    unsigned int length = b1_length + b2_length + c1_length + 1;

    char* cmd = (char*)malloc(length);
    if (!cmd) return invalidCommandResult;

    char* pos = cmd;

    memcpy(pos, b1, b1_length);
    pos += b1_length;

    memcpy(pos, c1, c1_length);
    pos += c1_length;

    memcpy(pos, b2, b2_length);
    pos += b2_length;

    *pos = '\0';

    CommandResult res = invokeShellCall(cmd);
    free(cmd);
    return res;
}
