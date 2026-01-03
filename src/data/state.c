#include "state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define ATOMIC_RENAME(src, dst) MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING)
#else
#define ATOMIC_RENAME(src, dst) rename(src, dst)
#endif

int State_Write(FILE* f, const State* state)
{
    if (!f || !state) return 1;

    if (fprintf(f, "installed=%d\n", state->installed ? 1 : 0) < 0)
        return 1;

    if (fprintf(f, "version=%s\n", state->version) < 0)
        return 1;

    return 0;
}

int State_Save(const char* path, const State* state) // TODO: fix on
{
    if (!path || !state) return 1;

    size_t len = strlen(path) + 5; // ".tmp" + null
    char* tmp = (char*)malloc(len);
    if (!tmp) return 1;

    strcpy(tmp, path);
    strcat(tmp, ".tmp");

    FILE* f = fopen(tmp, "w");
    if (!f) {
        free(tmp);
        return 1;
    }

    if (State_Write(f, state) != 0) {
        fclose(f);
        remove(tmp);
        free(tmp);
        return 1;
    }

    fflush(f);
    fclose(f);

    if (!ATOMIC_RENAME(tmp, path)) {
        remove(tmp);
        free(tmp);
        return 1;
    }

    free(tmp);
    return 0;
}

int State_Load(const char* path, State* state)
{
    if (!path || !state) return 1;

    // Default
    state->installed = 0;
    state->version = NULL;

    FILE* f = fopen(path, "r");
    if (!f) {
        return 0;
    }

    char line[64];
    if (fgets(line, sizeof(line), f)) {
        int val = 0;
        const char* str = NULL;
        if (sscanf(line, "installed=%d", &val) == 1) {
            state->installed = val ? 1 : 0;
        } else if (sscanf(line, "version=%s", &str) == 1) {
            state->version = str;
        }
    }

    fclose(f);
    return 0;
}
