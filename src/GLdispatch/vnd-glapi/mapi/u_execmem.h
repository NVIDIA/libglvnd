#ifndef _U_EXECMEM_H_
#define _U_EXECMEM_H_

/**
 * Allocates \p size bytes of executable memory.
 *
 * The returned pointer may or may not be writable. Call
 * \c u_execmem_get_writable to get a pointer to a writable mapping.
 */
void *u_execmem_alloc(unsigned int size);

/**
 * Returns a writable mapping for a pointer returned by \c u_execmem_alloc.
 *
 * If \p execPtr is a pointer returned by \c u_execmem_alloc, then this
 * function will return a writable mapping of the same memory.
 *
 * If \p execPtr was not returned by \c u_execmem_alloc, then it will be
 * returned unmodified. Thus, it's safe to pass a pointer to a static
 * or dynamic entrypoint.
 */
void *u_execmem_get_writable(void *execPtr);

/**
 * Frees the memory allocated from u_execmem_alloc.
 */
void u_execmem_free(void);

#endif /* _U_EXECMEM_H_ */
