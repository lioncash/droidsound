/**
 * @ingroup   emu68_lib
 * @file      emu68/assert68.h
 * @brief     assert feature header.
 * @author    Benjamin Gerard
 * @date      2009/06/12
 */
/* Time-stamp: <2013-08-04 23:12:56 ben> */

/* Copyright (C) 1998-2013 Benjamin Gerard */

#ifndef EMU68_ASSERT68_H
#define EMU68_ASSERT68_H

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif
#ifndef assert
# error "assert macro must be defined"
#endif
#if defined(NDEBUG_LIBSC68) && !defined(NDEBUG)
# error "Compile libsc68 in release mode with assert enable"
#endif

#ifndef EMU68_BREAK
# ifndef DEBUG
#  define EMU68_BREAK 1                 /* NOT debug mode */
# else
#  define EMU68_BREAK 0                 /*     debug mode */
# endif
#endif

#endif
