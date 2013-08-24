/**
 * @ingroup  file68_lib
 * @file     sc68/file68_ord.h
 * @author   Benjamin Gerard
 * @date     2003-08-12
 * @brief    Byte ordering header.
 */
/* Time-stamp: <2013-07-22 00:15:26 ben> */

/* Copyright (C) 1998-2013 Benjamin Gerard */

#ifndef _FILE68_ORD_H_
#define _FILE68_ORD_H_

#ifndef FILE68_API
# include "file68_api.h"
#endif

/**
 * @defgroup  file68_ord  Byte ordering
 * @ingroup   file68_lib
 *
 *   Provides functions for dealing with host byte order.
 *
 * @{
 */

FILE68_API
/**
 * Get integer byte order.
 *
 *   The endian68_byte_order() function returns an integer which gives
 *   the position of each byte in the memory.
 *
 *   Examples:
 *   - Intel little endian will return 0x03020100.
 *   - Motorola big endian will return 0x00010203.
 *
 * @return byte order.
 */
int endian68_byte_order(void);

FILE68_API
/**
 * Check if byte order is little endian.
 *
 *   The endian68_is_little() function checks if the byte order is
 *   little endian.
 *
 * @return little endian test.
 * @retval  1  byte order is little endian.
 * @retval  0  byte order is not little endian.
 *
 * @warning The function only test if the less signifiant byte is
 *          stored at offset 0.
 *
 * @see endian68_is_big();
 */
int endian68_is_little(void);

FILE68_API
/**
 * Check if byte order is big endian.
 *
 *   The endian68_is_big() function checks if the byte order is big
 *   endian. By the way it returns !endian68_is_little().
 *
 * @return big endian test.
 * @retval  1  byte order is big endian.
 * @retval  0  byte order is not big endian.
 *
 * @see endian68_is_little();
 */
int endian68_is_big(void);

/**
 * @}
 */

#endif

