/**
 * @ingroup  file68_lib
 * @file     sc68/file68_vfs_null.h
 * @author   Benjamin Gerard
 * @date     2003-10-10
 * @brief    Null stream header.
 */
/* Time-stamp: <2013-08-09 19:39:53 ben> */

/* Copyright (C) 1998-2013 Benjamin Gerard */

#ifndef _FILE68_VFS_NULL_H_
#define _FILE68_VFS_NULL_H_

#include "file68_vfs.h"

/**
 * @name     Null VFS
 * @ingroup  file68_vfs
 *
 *   Implements a null vfs68_t.
 *
 *   Null stream does nothing but checking some trivial errors (like
 *   access without opening) and dealing with a virtual stream
 *   position.  The null stream length is the position the highest
 *   byte that has been either read or write. The length is resetted
 *   at open.  It implies that stream length can be retrieved by the
 *   vfs68_length() function after vfs68_close() call.
 *
 * @note   null vfs scheme is "null:".
 *
 * @{
 */

FILE68_EXTERN
/**
 * Init null vfs (register null: scheme).
 *
 * @retval  0 on success
 * @retval -1 on error
 */
int vfs68_null_init(void);

FILE68_EXTERN
/**
 * Shutdown null vfs (unregister null: scheme).
 */
void vfs68_null_shutdown(void);

/**
 * @}
 */

#endif

