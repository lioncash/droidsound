/**
 * @ingroup  file68_lib
 * @file     sc68/file68_vfs_ao.h
 * @author   Benjamin Gerard
 * @date     2007-03-08
 * @brief    AO stream header.
 */
/* Time-stamp: <2013-08-03 13:52:51 ben> */

/* Copyright (C) 1998-2013 Benjamin Gerard */

#ifndef _FILE68_VFS_AO_H_
#define _FILE68_VFS_AO_H_

#include "file68_vfs.h"

/**
 * @name     Audio VFS
 * @ingroup  file68_vfs
 *
 *   Implements vfs68_t for XIPH libao (audio output).
 *
 * @{
 */

FILE68_EXTERN
/**
 * Initialize ao virtual file system.
 */
int vfs68_ao_init(void);

FILE68_EXTERN
/**
 * Shutdown ao virtual file system.
 */
void vfs68_ao_shutdown(void);

FILE68_EXTERN
/**
 * Get sampling rate.
 */
int vfs68_ao_sampling(vfs68_t * vfs);

FILE68_EXTERN
/**
 * Create audio VFS.
 */
vfs68_t * vfs68_ao_create(const char * uri, int mode, int spr);

/**
 * @}
 */

#endif
