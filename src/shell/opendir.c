#include "opendir.h"

#include <unistd.h>
#include <limits.h>

#ifndef PATH_MAX
#include <linux/limits.h>
#endif

CommandResult openDir(const char* path, CommandResult (*fn)(void*), void* ctx)
{
    char oldcwd[PATH_MAX];

    if (!getcwd(oldcwd, sizeof(oldcwd)))
        return invalidCommandResult;

    if (chdir(path) != 0)
        return invalidCommandResult;

    CommandResult rc = fn(ctx);

    // TODO
    size_t __attribute__((unused)) _ = chdir(oldcwd);

    return rc;
}
