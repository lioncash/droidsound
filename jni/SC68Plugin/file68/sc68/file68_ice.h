/**
 * @ingroup  file68_lib
 * @file     sc68/file68_ice.h
 * @author   Benjamin Gerard
 * @date     2003-09-06
 * @brief    ICE loader header.
 */
/* Time-stamp: <2013-07-22 01:19:11 ben> */

/* Copyright (C) 1998-2013 Benjamin Gerard */

#ifndef _FILE68_ICE_H_
#define _FILE68_ICE_H_

#include "file68_vfs.h"

/**
 * @defgroup  file68_ice  ICE loader support.
 * @ingroup   file68_lib
 *
 *   Provides functions for loading ICE stream.
 *
 * @{
 */

FILE68_EXTERN
/**
 * Get ICE! depacker version.
 *
 * @retval   1  ICE! is supported but unknown version
 * @retval   0  ICE! is not supported
 * @return  ICE! depacker version
 *
 * @see unice68_ice_version()
 */
int file68_ice_version(void);

FILE68_EXTERN
/**
 * Test ice file header magic header.
 *
 * @param  buffer  Buffer containing at least 12 bytes from ice header.
 *
 * @retval  1  buffer seems to be ice packed..
 * @retval  0  buffer is not ice packed.
 */
int file68_ice_is_magic(const void * buffer);

FILE68_EXTERN
/**
 * Load an iced stream.
 *
 *   The file68_ice_load() function loads and depack an ice packed
 *   file from a stream and returns a allocate buffer with unpacked
 *   data.
 *
 * @param  is     Stream to load (must be opened in read mode).
 * @param  ulen   Pointer to save uncompressed size.
 *
 * @return Pointer to the uncompressed data buffer.
 * @retval 0 Error
 */
void * file68_ice_load(vfs68_t * is, int * ulen);

FILE68_EXTERN
/**
 * Load an iced file.
 *
 * @param  fname    File to load.
 * @param  ulen     Pointer to save uncompressed size.
 *
 * @return Pointer to the uncompressed data buffer.
 * @retval 0 Error
 *
 * @see file68_ice_load()
 */
void * file68_ice_load_file(const char * fname, int * ulen);

/**
 * @}
 */

#endif

