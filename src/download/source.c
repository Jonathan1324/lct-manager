#include "source.h"
#include "../shell/shell.h"
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
static int is_directory(const char* path)
{
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) return 0;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
#else
#include <sys/stat.h>
#include <dirent.h>
static int is_directory(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}
#endif

static char* find_top_level_folder(const char* base_dir)
{
#ifdef _WIN32
    char* search_path = (char*)malloc(strlen(base_dir) + 3);
    if (!search_path) return NULL;
    memcpy(search_path, base_dir, strlen(base_dir));
    search_path[strlen(base_dir)] = '\\';
    search_path[strlen(base_dir)+1] = '*';
    search_path[strlen(base_dir)+2] = '\0';

    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    free(search_path);
    if (hFind == INVALID_HANDLE_VALUE) return NULL;

    char* folder = NULL;
    do {
        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(find_data.cFileName, ".") != 0 &&
            strcmp(find_data.cFileName, "..") != 0)
        {
            size_t len = strlen(find_data.cFileName) + 1;
            folder = (char*)malloc(len);
            if (folder) memcpy(folder, find_data.cFileName, len);
            break;
        }
    } while (FindNextFileA(hFind, &find_data));
    FindClose(hFind);
    return folder;
#else
    DIR* dir = opendir(base_dir);
    if (!dir) return NULL;

    struct dirent* entry;
    char* folder = NULL;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        size_t base_len = strlen(base_dir);
        size_t name_len = strlen(entry->d_name);
        char* path = (char*)malloc(base_len + 1 + name_len + 1);
        if (!path) continue;
        memcpy(path, base_dir, base_len);
        path[base_len] = '/';
        memcpy(path + base_len + 1, entry->d_name, name_len);
        path[base_len + 1 + name_len] = '\0';

        if (is_directory(path)) {
            folder = (char*)malloc(name_len + 1);
            if (folder) memcpy(folder, entry->d_name, name_len + 1);
            free(path);
            break;
        }
        free(path);
    }
    closedir(dir);
    return folder;
#endif
}

char* downloadSource(const char* version, const char* path)
{
    if (!version) return NULL;

    const char base1[] = "https://github.com/Jonathan1324/LCT/archive/refs/tags/";
#ifdef _WIN32
    const char base2[] = ".zip";
#else
    const char base2[] = ".tar.gz";
#endif

    size_t base1_len = sizeof(base1) - 1;
    size_t base2_len = sizeof(base2) - 1;
    size_t version_len = strlen(version);
    size_t path_len = strlen(path);

    size_t url_len = base1_len + version_len + base2_len + 1;
    char* url = (char*)malloc(url_len);
    if (!url) return NULL;

    char* pos = url;
    memcpy(pos, base1, base1_len);
    pos += base1_len;
    memcpy(pos, version, version_len);
    pos += version_len;
    memcpy(pos, base2, base2_len);
    pos += base2_len;
    *pos = '\0';

    size_t file_len = path_len + 1 + version_len + base2_len + 1;
    char* file_path = (char*)malloc(file_len);
    if (!file_path) {
        free(url);
        return NULL;
    }
    pos = file_path;
    memcpy(pos, path, path_len);
    pos += path_len;
    *pos = '/';
    pos++;
    memcpy(pos, version, version_len);
    pos += version_len;
    memcpy(pos, base2, base2_len);
    pos += base2_len;
    *pos = '\0';

    CommandResult res = curl(url, file_path);
    if (res.exit_code != 0) {
        free(url);
        free(file_path);
        return NULL;
    }
    free(url);

    // TODO: currently here
    fputs("==> Unarchiving source of ", stdout);
    fputs(version, stdout);
    fputs("...\n", stdout);
    fflush(stdout);
#ifdef _WIN32
    res = unzip(file_path, path);
#else
    res = tar_gz(file_path, path);
#endif
    if (res.exit_code != 0) {
        free(file_path);
        return NULL;
    }
    sh_remove(file_path);
    free(file_path);

    char* top_level_folder = find_top_level_folder(path);

    return top_level_folder;
}
