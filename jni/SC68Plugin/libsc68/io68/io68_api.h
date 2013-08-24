/**
 * @ingroup   io68_devel
 * @file      io68/io68_api.h
 * @author    Benjamin Gerard
 * @date      2009/02/10
 * @brief     io68 export header.
 */
/* Time-stamp: <2013-08-04 19:14:37 ben> */

/* Copyright (C) 1998-2013 Benjamin Gerard */

#ifndef IO68_IO68_API_H
#define IO68_IO68_API_H

#ifndef IO68_API
# ifdef IO68_EXPORT
#  ifndef SC68_EXPORT
#   define SC68_EXPORT 1
#  endif
#  include "sc68/sc68.h"
#  define IO68_EXTERN SC68_EXTERN
#  define IO68_API    SC68_API
# elif defined (__cplusplus)
#  define IO68_EXTERN extern "C"
#  define IO68_API    IO68_EXTERN
# else
#  define IO68_EXTERN extern
#  define IO68_API    IO68_EXTERN
# endif
#endif

#endif
