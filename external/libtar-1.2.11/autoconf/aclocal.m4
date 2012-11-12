m4_include([encap.m4])
m4_include([ac_path_generic.m4])


# PSG_LIB_READLINE
# ----------------
# Check for GNU readline library.
AC_DEFUN([PSG_LIB_READLINE], [
  AC_CHECK_HEADERS([readline/readline.h])
  AC_CHECK_HEADERS([readline/history.h])
  if test "$ac_cv_header_readline_readline_h" = "yes"; then
    AC_SEARCH_LIBS([tputs], [termcap curses])
    AC_CHECK_LIB([readline], [rl_callback_handler_install])
  fi
])


# PSG_LIB_TAR
# -----------
# Check for usable version of libtar library.
AC_DEFUN([PSG_LIB_TAR], [
  psg_old_libs="$LIBS"
  LIBS="$LIBS -ltar"
  AC_CACHE_CHECK([for usable version of libtar],
    [psg_cv_lib_tar_usable],
    [AC_TRY_RUN([
#include <stdio.h>
#include <string.h>
#include <libtar.h>

int main(int argc, char *argv[]) {
  return (strcmp(libtar_version, "1.2") >= 0 ? 0 : 1);
}
],
      [psg_cv_lib_tar_usable=yes],
      [psg_cv_lib_tar_usable=no],
      [psg_cv_lib_tar_usable=no]
    )]
  )
  if test "$psg_cv_lib_tar_usable" = "yes"; then
    AC_DEFINE([HAVE_LIBTAR], 1,
              [Define if your system has a current version of libtar])
  else
    LIBS="$psg_old_libs"
  fi
])


# PSG_LIB_FGET
# ------------
# Check for usable version of libfget library.
AC_DEFUN([PSG_LIB_FGET], [
  psg_old_libs="$LIBS"
  AC_CHECK_LIB([socket], [socket])
  AC_CHECK_LIB([nsl], [gethostbyname])
  LIBS="$LIBS -lfget"
  AC_CACHE_CHECK([for usable version of libfget],
    [psg_cv_lib_fget_usable],
    [AC_TRY_COMPILE([
        #include <libfget.h>
      ], [
	FTP *ftp;
	char buf[10240];
	struct ftp_url fu;

	ftp_url_parse("ftp://host.com/dir/file.txt", &fu);

        ftp_connect(&ftp, fu.fu_hostname, buf, sizeof(buf), 0, 0,
		    -1, -1, NULL, NULL);
      ],
      [psg_cv_lib_fget_usable=yes],
      [psg_cv_lib_fget_usable=no]
    )]
  )
  if test "$psg_cv_lib_fget_usable" = "yes"; then
    AC_DEFINE([HAVE_LIBFGET], 1,
	      [Define if your system has a current version of libfget])
  else
    LIBS="$psg_old_libs";
  fi
])


# PSG_LIB_WRAP
# ------------
# Check for TCP Wrapper library.
AC_DEFUN([PSG_LIB_WRAP], [
  AC_CHECK_HEADERS([tcpd.h])
  if test "$ac_cv_header_tcpd_h" = "yes"; then
    psg_old_libs="$LIBS"
    LIBS="$LIBS -lwrap"
    AC_CACHE_CHECK([for libwrap library],
      [psg_cv_lib_wrap_hosts_ctl],
      AC_TRY_LINK([
          #include <stdio.h>
          #include <tcpd.h>
          int allow_severity;
          int deny_severity;
        ], [
          hosts_ctl("test", STRING_UNKNOWN, "10.0.0.1", STRING_UNKNOWN);
        ],
        [psg_cv_lib_wrap_hosts_ctl=yes],
        [psg_cv_lib_wrap_hosts_ctl=no]
      )
    )
    if test "$psg_cv_lib_wrap_hosts_ctl" = "yes"; then
      AC_DEFINE([HAVE_LIBWRAP], 1, [Define if you have libwrap])
    else
      LIBS="$psg_old_libs"
    fi
  fi
])


# PSG_REPLACE_TYPE(type_t, default, [includes])
# ---------------------------------------------
# Check for arbitrary type in arbitrary header file(s).
AC_DEFUN([PSG_REPLACE_TYPE],
  [AC_CHECK_TYPES([$1],
    ,
    [AC_DEFINE($1, $2,
      [Define to `$2' if not defined in system header files.]
    )],
    $3
  )]
)


# PSG_SHLIB(includes, code)
# -------------------------
# Check how to build shared libraries containing the specified code
# (very rudimentary).
AC_DEFUN([PSG_SHLIB], [
  AC_MSG_CHECKING([how to build shared libraries])
  cflag_options="-fpic";
  ldflag_options="-G -shared";
  if test "$CC" != "gcc"; then
    case "`uname`" in
      HP-UX)
        cflag_options="+Z $cflag_options";
        ldflag_options="-Wl,-b $ldflag_options";
        ;;
      SunOS)
        cflag_options="-Kpic $cflag_options";
        ;;
    esac
  fi
  for SHLIB_CFLAGS in $cflag_options ""; do
    for SHLIB_LDFLAGS in $ldflag_options ""; do
      psg_old_cflags="$CFLAGS";
      CFLAGS="$CFLAGS $SHLIB_CFLAGS";
      psg_old_ldflags="$LDFLAGS";
      LDFLAGS="$LDFLAGS $SHLIB_LDFLAGS";
      AC_LINK_IFELSE([AC_LANG_SOURCE([[
$1

int
dummy(void)
{
	$2
	return 0;
}
]])],
	[psg_cv_flags_shlib="CFLAGS=$SHLIB_CFLAGS LDFLAGS=$SHLIB_LDFLAGS"],
	[psg_cv_flags_shlib=no]
      )
      CFLAGS="$psg_old_cflags";
      LDFLAGS="$psg_old_ldflags";
      if test "$psg_cv_flags_shlib" != "no"; then
	break;
      fi
    done
    if test "$psg_cv_flags_shlib" != "no"; then
      break;
    fi
  done
  if test "$psg_cv_flags_shlib" = "no"; then
    SHLIB_CFLAGS="";
    SHLIB_LDFLAGS="";
  fi
  AC_SUBST([SHLIB_CFLAGS])
  AC_SUBST([SHLIB_LDFLAGS])
  AC_MSG_RESULT([$psg_cv_flags_shlib])
])


# PSG_MODULE(subdir, [args, ...])
# -------------------------------
# Process the module.ac file in subdir.  If the module.ac file defines a
# macro called subdir[]_INIT, call it with the arguments passed to
# PSG_MODULE().
AC_DEFUN([PSG_MODULE], [
  m4_define([subdir], [$1])dnl
  m4_include([$1/module.ac])dnl
  m4_ifdef([$1][_INIT], [$1][_INIT($@)])dnl
  m4_undefine([subdir])dnl
])


