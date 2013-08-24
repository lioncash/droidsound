dnl# -*- mode:sh; sh-basic-offset:2; indent-tabs-mode:nil -*-
dnl# 
dnl# the "configure" script is made from this by running GNU "autoconf"
dnl#
dnl# Configuration script for libsc68
dnl#
dnl# by Benjamin Gerard <http://sourceforge.net/users/benjihan>
dnl#

# ,----------------------------------------------------------------------.
# | Autotools and Package                                                |
# `----------------------------------------------------------------------'

AC_INIT(
  [libsc68],
  [3.0.0b],
  [benjihan -4t- sourceforge],,
  [http://sc68.atari.org])
AC_CONFIG_SRCDIR([api68.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign 1.10 silent-rules color-tests dist-bzip2])
AC_CONFIG_HEADERS([config.h])

AC_USE_SYSTEM_EXTENSIONS
LT_INIT([win32-dll])

# ,----------------------------------------------------------------------.
# | sc68                                                                 |
# `----------------------------------------------------------------------'


SC68_PACKAGE([sc68 engine library.])
SC68_SYSTEM
SC68_OPTIONS
SC68_TOOLS
SC68_CHECKS

PAC_CFLAGS="-I[$]includedir"
PAC_LFLAGS="-L[$]libdir -lsc68"

dnl # SC68_WITH_HEADER([file68],[yes],[sc68/file68.h])
SC68_WITH([file68],[yes],[],[sc68/file68.h])

AC_SUBST([SC68_URL])
AC_SUBST([SC68_EMAIL])

# ,----------------------------------------------------------------------.
# | Optional features                                                    |
# `----------------------------------------------------------------------'

AC_ARG_ENABLE(
  [emu68-monolitic],
  [AS_HELP_STRING([--enable-emu68-monolitic],
      [compile 68k instructions as a single huge file @<:@default=no@:>@])],
  [],[enable_emu68_monolitic='no'])
AM_CONDITIONAL([emu68_monolitic],[test "X${enable_emu68_monolitic}" = 'Xyes'])

AC_ARG_WITH(
  [ym-engine],
  [AS_HELP_STRING([--with-ym-engine],
      [set default YM-2149 engine (pulse|blep) @<:@default=auto-select@:>@])],
  [],[with_ym_engine='x'])

case X${with_ym_engine} in
  Xblep)
    YM_ENGINE='YM_ENGINE_BLEP'
    with_ym_engine='BLEP synthesis'
    ;;
  Xdump)
    YM_ENGINE='YM_ENGINE_DUMP'
    with_ym_engine='DUMP registers'
    ;;
  Xorig | Xpuls*)
    YM_ENGINE='YM_ENGINE_PULS'
    with_ym_engine='pulse'
    ;;
  Xx | Xyes | Xno)
    YM_ENGINE='x'
    ;;
  *)
    YM_ENGINE="YM_ENGINE_"[]AS_TR_CPP($with_ym_engine)
    with_ym_engine="unknown (3rd party?) $with_ym_engine engine"
    AC_MSG_WARN([Using unknown YM engine may break compilation.])
    ;;
esac

if test "x-$YM_ENGINE" != "x-x"; then
  AC_DEFINE_UNQUOTED([YM_ENGINE],[$YM_ENGINE],[set default YM engine])
else
  with_ym_engine='auto-select (should be blep)'
fi

# ,----------------------------------------------------------------------.
# | Sized size integers                                                  |
# `----------------------------------------------------------------------'

AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INTMAX_T
AC_TYPE_LONG_LONG_INT
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINTMAX_T
AC_TYPE_UNSIGNED_LONG_LONG_INT

# ,----------------------------------------------------------------------.
# | Standard headers                                                     |
# `----------------------------------------------------------------------'

AC_CHECK_HEADERS(
  [stdio.h stdlib.h string.h assert.h libgen.h])

# ,----------------------------------------------------------------------.
# | Standard functions                                                   |
# `----------------------------------------------------------------------'

AC_CHECK_FUNCS(
  [malloc free vsprintf vsnprintf getenv strtol strtoul stpcpy basename])

# ,----------------------------------------------------------------------.
# | Output                                                               |
# `----------------------------------------------------------------------'
AC_CONFIG_FILES([
    Makefile
    emu68/Makefile
    io68/Makefile
    libsc68-${VERSION}.ebuild:libsc68.ebuild.in
    libsc68.spec
    sc68.pc])
AC_OUTPUT

# ,----------------------------------------------------------------------.
# | Summary                                                              |
# `----------------------------------------------------------------------'

SC68_DUMPVARS

AC_MSG_NOTICE([])
AC_MSG_NOTICE([,-----------------------])
AC_MSG_NOTICE([| $PACKAGE_NAME ($PACKAGE_VERSION)])
AC_MSG_NOTICE([|-----------------------])
AC_MSG_NOTICE([|   debug-mode          : $enable_sc68_debug])
AC_MSG_NOTICE([|   release-mode        : $enable_sc68_release])
AC_MSG_NOTICE([|   all-static          : $enable_sc68_static])
AC_MSG_NOTICE([|   file68              : $has_file68 ($file68_ver)])
AC_MSG_NOTICE([|   default YM engine   : $with_ym_engine])
AC_MSG_NOTICE([+-----------------------])

dnl#
dnl# End of libsc68/configure.ac
dnl#
