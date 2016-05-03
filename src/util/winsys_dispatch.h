#ifndef WINSYS_DISPATCH_H
#define WINSYS_DISPATCH_H

/*!
 * \file
 *
 * Utility functions for keeping track of the dispatch stubs for window-system
 * functions.
 *
 * These functions keep track of an array of functions, with a name and pointer
 * for each.
 */

/*!
 * Initializes the window system list.
 */
void __glvndWinsysDispatchInit(void);

/*!
 * Frees the list and all the items in it.
 */
void __glvndWinsysDispatchCleanup(void);

/*!
 * Looks up a function by name.
 *
 * \param name The name of the function to look up.
 * \return The index of the function, or -1 if the function was not found.
 */
int __glvndWinsysDispatchFindIndex(const char *name);

/*!
 * Adds a function to the list. The function must not already be in the list.
 *
 * \param name The name of the function.
 * \param dispatch A pointer to the dispatch stub for the function.
 * \return The index of the function, or -1 on error.
 */
int __glvndWinsysDispatchAllocIndex(const char *name, void *dispatch);

/*!
 * Returns the name of a function.
 *
 * \param index The index of the function.
 * \return A pointer to the function name, or \c NULL if the index is invalid.
 */
const char *__glvndWinsysDispatchGetName(int index);

/*!
 * Returns the dispatch stub for a function.
 *
 * \param index The index of the function.
 * \return A pointer to the dispatch stub, or \c NULL if the index is invalid.
 */
void *__glvndWinsysDispatchGetDispatch(int index);

/*!
 * Returns the number of functions in the list.
 */
int __glvndWinsysDispatchGetCount(void);


/*!
 * A dispatch table to keep track of the window-system functions from a vendor
 * library.
 */
typedef struct __GLVNDwinsysVendorDispatchRec __GLVNDwinsysVendorDispatch;

/*!
 * Creates an empty dispatch table.
 */
__GLVNDwinsysVendorDispatch *__glvndWinsysVendorDispatchCreate(void);

/*!
 * Frees a dispatch table.
 */
void __glvndWinsysVendorDispatchDestroy(__GLVNDwinsysVendorDispatch *table);

/*!
 * Adds a function to a dispatch table.
 *
 * \param table The dispatch table.
 * \param index The index of the function to add.
 * \param func The pointer to the vendor library's function.
 */
int __glvndWinsysVendorDispatchAddFunc(__GLVNDwinsysVendorDispatch *table, int index, void *func);

/*!
 * Looks up a function from a dispatch table.
 *
 * \param table The dispatch table.
 * \param index The index of the function to look up.
 * \return The function pointer, or \c NULL if the function is not in the
 * table.
 */
void *__glvndWinsysVendorDispatchLookupFunc(__GLVNDwinsysVendorDispatch *table, int index);

#endif // WINSYS_DISPATCH_H
