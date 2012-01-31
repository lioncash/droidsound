/**
 * @ingroup  file68_lib
 * @file     sc68/chunk68.h
 * @author   Benjamin Gerard
 * @date     2011-10-02
 * @brief    Chunk definition header file.
 *
 */

/* Copyright (C) 1998-2011 Benjamin Gerard */

#ifndef _FILE68_CHUNK68_H_
#define _FILE68_CHUNK68_H_

/**
 * @addtogroup   file68_lib
 * @{
 */

/**
 * SC68 file chunk header.
 */
typedef struct
{
  char id[4];   /**< Must be "SC??".            */
  char size[4]; /**< Size in bytes (MSB first). */
} chunk68_t;

/**
 * @name SC68 file chunk definitions.
 * @{
 */

#define CH68_CHUNK     "SC"    /**< Chunk identifier.            */
#define CH68_BASE      "68"    /**< Start of file.               */
#define CH68_FNAME     "FN"    /**< File name.                   */
#define CH68_DEFAULT   "DF"    /**< Default music.               */
#define CH68_MUSIC     "MU"    /**< Music (track) section start. */
#define CH68_MNAME     "MN"    /**< Track's name.                */
#define CH68_ANAME     "AN"    /**< Track's author.              */
#define CH68_CNAME     "CN"    /**< Track's original composer.   */
#define CH68_D0        "D0"    /**< Tracks's D0 value.           */
#define CH68_AT        "AT"    /**< Tracks's load address.       */
#define CH68_TIME      "TI"    /**< Tracks's length in seconds.  */
#define CH68_FRAME     "FR"    /**< Tracks's Length in frames.   */
#define CH68_FRQ       "FQ"    /**< Tracks's replay rate in Hz.  */
#define CH68_LOOP      "LP"    /**< Tracks's number of loop.     */
#define CH68_TYP       "TY"    /**< Tracks's HW feature flag.    */
#define CH68_YEAR      "YY"    /**< Tracks's publishing year.    */
#define CH68_COPYRIGHT "CR"    /**< Tracks's copyright owner.    */
#define CH68_TAG       "TG"    /**< Meta tag.                    */
#define CH68_REPLAY    "RE"    /**< External replay.             */
#define CH68_UTF8      "U8"    /**< String are UTF-8 encoded.    */
#define CH68_ALIGN     "32"    /**< Chunk are 32bit aligned.     */
#define CH68_MDATA     "DA"    /**< Music data.                  */
#define CH68_EOF       "EF"    /**< End of file.                 */
#define CH68_NULL      "\0\0"  /**< Null.                        */

/**
 * @}
 */

/**
 * @}
 */

#endif /* #ifndef _FILE68_CHUNK68_H_ */
