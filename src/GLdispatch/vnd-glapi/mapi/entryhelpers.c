#include "entryhelpers.h"

#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>

static int entry_patch_mprotect(void *start, void *end, int prot)
{
    size_t size;
    size_t pageSize = (size_t) sysconf(_SC_PAGESIZE);

    if (((uintptr_t) start) % pageSize != 0
            || ((uintptr_t) end) % pageSize != 0) {
        assert(((uintptr_t) start) % pageSize == 0);
        assert(((uintptr_t) end) % pageSize == 0);
        return 0;
    }

    size = ((uintptr_t) end) - ((uintptr_t) start);

    if (mprotect(start, size, prot) != 0) {
        return 0;
    }
    return 1;
}

int entry_patch_start_helper(void *start, void *end)
{
    // Set the memory protections to read/write/exec.
    // Since this only gets called when no thread has a current context, this
    // could also just be read/write, without exec, and then set it back to
    // read/exec afterward. But then, if the first mprotect succeeds and the
    // second fails, we'll be left with un-executable entrypoints.
    return entry_patch_mprotect(start, end, PROT_READ | PROT_WRITE | PROT_EXEC);
}

int entry_patch_finish_helper(void *start, void *end)
{
    return entry_patch_mprotect(start, end, PROT_READ | PROT_EXEC);
}
