dnl# -*- mode:sh; sh-basic-offset:2; indent-tabs-mode:nil -*-
dnl#
dnl# the "configure" script is made from this by running GNU "autoconf"
dnl#
dnl# Configuration script for unice68
dnl#
dnl# by Benjamin Gerard <http://sourceforge.net/users/benjihan>
dnl#

# ,----------------------------------------------------------------------.
# | Autotools and Package                                                |
# `----------------------------------------------------------------------'

AC_INIT(
  [unice68],
  [20130728],
  [benjihan -4t- sourceforge],,
  [http://sc68.atari.org])
AC_CONFIG_SRCDIR([unice68.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign 1.10 silent-rules dist-bzip2])
AC_CONFIG_HEADERS([config.h])

AC_USE_SYSTEM_EXTENSIONS
LT_INIT([win32-dll])

# ,----------------------------------------------------------------------.
# | sc68                                                                 |
# `----------------------------------------------------------------------'

SC68_PACKAGE([ice packer/depacker program and library.])
SC68_SYSTEM
SC68_OPTIONS
SC68_TOOLS
SC68_CHECKS

PAC_CFLAGS="-I[$]includedir"
PAC_LFLAGS="-L[$]libdir -lunice68"

# ,----------------------------------------------------------------------.
# | Optionnal features                                                   |
# `----------------------------------------------------------------------'

# Command line depacker.
AC_ARG_ENABLE(
  [unice68-cli],
  [AS_HELP_STRING([--enable-unice68-cli],
      [enable command line program @<:@default=yes@:>@])],
  [],[enable_unice68_cli="yes"])
AM_CONDITIONAL([UNICE68_CLI],[test "x$enable_unice68_cli" = "xyes"])

# ,----------------------------------------------------------------------.
# | Functions and headers                                                |
# `----------------------------------------------------------------------'
AC_CHECK_HEADERS([stdio.h stdlib.h string.h stdarg.h errno.h libgen.h])
AC_CHECK_FUNCS([basename freopen])

# ,----------------------------------------------------------------------.
# | Output                                                               |
# `----------------------------------------------------------------------'

AC_CONFIG_FILES([
    Makefile
    package.texi
    unice68-${VERSION}.ebuild:unice68.ebuild.in
    unice68.1.tpl
    unice68.pc
    unice68.spec])
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
AC_MSG_NOTICE([|   all-static-mode     : $enable_sc68_static])
AC_MSG_NOTICE([|   build cli           : $enable_unice68_cli])
AC_MSG_NOTICE([+-----------------------])
AC_MSG_NOTICE([])

dnl#
dnl# End of unice68/configure.ac
dnl#
