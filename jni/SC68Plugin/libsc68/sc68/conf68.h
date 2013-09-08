/**
 * @ingroup   sc68_lib
 * @file      conf68.h
 * @brief     configuration file.
 * @author    Benjamin Gerard
 * @date      1999/07/27
 */
/* Time-stamp: <2013-08-25 07:04:32 ben> */

/* Copyright (C) 1998-2013 Benjamin Gerard */

#ifndef SC68_CONF68_H
#define SC68_CONF68_H

#ifndef SC68_EXTERN
# ifdef __cplusplus
#  define SC68_EXTERN extern "C"
# else
#  define SC68_EXTERN
# endif
#endif

/**
 *  @defgroup  sc68_conf  configuration file
 *  @ingroup   sc68_lib
 *
 *  This module prodives functions to access sc68 configuration.
 *
 *  @{
 */

SC68_EXTERN
/**
 *  Load config.
 *
 *  @param  name  name of application
 *
 *  @return error code
 *  @retval 0 on success
 *  @retval 0 on error
 */
int config68_load(const char * name);

SC68_EXTERN
/**
 *  Save config.
 *
 *  @param  name  name of application
 *
 *  @return error code
 *  @retval 0 on success
 *  @retval 0 on error
 */
int config68_save(const char * name);


SC68_EXTERN
/**
 *  Library one time init.
 *
 *  @param  argc  argument count.
 *  @param  argv  argument values.
 *
 *  @return number of argument remaining in argv
 *  @retval 0 on success
 *  @retval 0 on error
 */
int config68_init(int argc, char * argv[]);

SC68_EXTERN
/**
 *  Library one time shutdown shutdown.
 */
void config68_shutdown(void);

/**
 * @}
 */

#endif
