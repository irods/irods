dnl ### Normal initialization. ######################################
AC_INIT([libtar], [1.2.11])
AC_PREREQ([2.57])
AC_CONFIG_AUX_DIR([autoconf])
AC_CONFIG_HEADERS([config.h])
AC_COPYRIGHT([[
Copyright (c) 1998-2003 University of Illinois Board of Trustees
Copyright (c) 1998-2003 Mark D. Roth
All rights reserved.
]])
AC_CONFIG_SRCDIR([lib/libtar.h])
ENCAP_PKG([], [postinstall-encap])


dnl ### Load subdirectory modules. ##################################
PSG_MODULE([compat])
PSG_MODULE([listhash], [libtar])


dnl ### Set some option defaults. ###################################
if test -z "$CFLAGS"; then
  CFLAGS="-O"
fi
MKDIR="mkdir -p -m 755"
AC_SUBST([MKDIR])


dnl ### Check for compiler et al. ###################################
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET


dnl ### Compiler characteristics. ##################################
AC_AIX
AC_C_CONST


dnl ### Checks for header files. ###################################
AC_HEADER_STDC
AC_CHECK_HEADERS([unistd.h])
AC_HEADER_MAJOR
PSG_REPLACE_TYPE([major_t], [unsigned int], [
  #include <sys/types.h>
  #ifdef MAJOR_IN_MKDEV
  # include <sys/mkdev.h>
  #else
  # ifdef MAJOR_IN_SYSMACROS
  #  include <sys/sysmacros.h>
  # endif
  #endif
])
PSG_REPLACE_TYPE([minor_t], [unsigned int], [
  #include <sys/types.h>
  #ifdef MAJOR_IN_MKDEV
  # include <sys/mkdev.h>
  #else
  # ifdef MAJOR_IN_SYSMACROS
  #  include <sys/sysmacros.h>
  # endif
  #endif
])
PSG_REPLACE_TYPE([dev_t], [unsigned long], [
  #include <sys/types.h>
  #ifdef MAJOR_IN_MKDEV
  # include <sys/mkdev.h>
  #else
  # ifdef MAJOR_IN_SYSMACROS
  #  include <sys/sysmacros.h>
  # endif
  #endif
])
PSG_REPLACE_TYPE([socklen_t], [unsigned long], [
  #include <sys/types.h>
  #include <sys/socket.h>
])
PSG_REPLACE_TYPE([uint64_t], [long long])
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T
PSG_REPLACE_TYPE([nlink_t], [unsigned short])


dnl ### Check for needed functions. ################################
COMPAT_FUNC_BASENAME
COMPAT_FUNC_DIRNAME
COMPAT_FUNC_FNMATCH
AC_CHECK_FUNCS([lchown])
COMPAT_FUNC_MAKEDEV
COMPAT_FUNC_SNPRINTF
COMPAT_FUNC_STRDUP
AC_FUNC_STRFTIME
COMPAT_FUNC_STRLCPY
COMPAT_FUNC_STRMODE
COMPAT_FUNC_STRSEP


dnl ### Check for libraries. #######################################
AC_ARG_WITH([zlib],
  [  --without-zlib          Use external gzip binary instead of zlib],
  [],
  [with_zlib=yes])
if test "$with_zlib" = "yes"; then
  AC_CHECK_LIB([z], [gzread])
fi


dnl ### Create output files. #######################################
AC_CONFIG_FILES([Makefile lib/Makefile libtar/Makefile doc/Makefile])
AC_OUTPUT

