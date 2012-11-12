# ENCAP_PKG([mkencap-options], [install target if enabled],
#           [install target if disabled])
# ---------------------------------------------------------
# Check for Encap tools.
AC_DEFUN([ENCAP_PKG], [
  MKENCAP_OPTS=$1;

  # allow user to disable Encap support
  AC_ARG_ENABLE([encap],
    [  --disable-encap         Do not configure as an Encap package],
    [],
    [enable_encap=default])

  if test "$enable_encap" != "no"; then
    # look for epkg and mkencap
    AC_PATH_PROG([EPKG], [epkg])
    AC_PATH_PROG([MKENCAP], [mkencap])

    # enable by default if epkg or mkencap are found
    if test "${EPKG:+set}" = "set" || test "${MKENCAP:+set}" = "set" && test "$enable_encap" = "default"; then
      enable_encap=yes;
    fi
  fi

  if test "$enable_encap" = "yes"; then
    # generate fallback values for ${ENCAP_SOURCE} and ${ENCAP_TARGET}
    # from the environment or the default prefix
    if test -z "${ENCAP_SOURCE}" && test -z "${ENCAP_TARGET}"; then
      ENCAP_SOURCE="${ac_default_prefix}/encap";
      ENCAP_TARGET="${ac_default_prefix}";
    elif test -z "${ENCAP_TARGET}"; then
      ENCAP_TARGET="`dirname ${ENCAP_SOURCE}`";
    elif test -z "${ENCAP_SOURCE}"; then
      ENCAP_SOURCE="${ENCAP_TARGET}/encap";
    fi

    # if --prefix is specified:
    #   1) if its next-to-last component is "encap", assume that it
    #      points to the package directory
    #   2) otherwise, assume it points to the target directory
    if test "${prefix}" != "NONE"; then
      prefixdir="`dirname ${prefix}`";
      prefixbase="`basename ${prefix}`";
      if test "`basename ${prefixdir}`" = "encap"; then
        ENCAP_SOURCE="${prefixdir}";
        ENCAP_TARGET="`dirname ${ENCAP_SOURCE}`";
      elif test "${prefixdir}" != "${ENCAP_SOURCE}"; then
        ENCAP_SOURCE="${prefix}/encap";
        ENCAP_TARGET="${prefix}";
      fi
      if ( test "`basename ${prefixdir}`" = "encap" || \
           test "${prefixdir}" = "${ENCAP_SOURCE}" ) && \
         test "${prefixbase}" != "${PACKAGE_NAME}-${PACKAGE_VERSION}"; then
        ENCAP_PKGSPEC="${prefixbase}";
      fi
    fi

    # display results
    AC_MSG_CHECKING([for Encap source directory])
    AC_MSG_RESULT([${ENCAP_SOURCE}])
    AC_MSG_CHECKING([for Encap target directory])
    AC_MSG_RESULT([${ENCAP_TARGET}])
    AC_MSG_CHECKING([for Encap package directory])
    if test "${ENCAP_PKGSPEC:-unset}" = "unset"; then
      ENCAP_PKGSPEC='${PACKAGE_NAME}-${PACKAGE_VERSION}';
      AC_MSG_RESULT([${ENCAP_SOURCE}/${PACKAGE_NAME}-${PACKAGE_VERSION}])
    else
      AC_MSG_RESULT([${ENCAP_SOURCE}/${ENCAP_PKGSPEC}])
    fi
    prefix='${ENCAP_SOURCE}/${ENCAP_PKGSPEC}';

    # override default sysconfdir and localstatedir
    if test "$sysconfdir" = '${prefix}/etc'; then
      sysconfdir='${ENCAP_TARGET}/etc';
    fi
    if test "$localstatedir" = '${prefix}/var'; then
      localstatedir='/var/lib/${PACKAGE_NAME}';
    fi

    # check for --disable-epkg-install
    AC_ARG_ENABLE([epkg-install],
      [  --disable-epkg-install  Do not run epkg during make install],
      [],
      [enable_epkg_install=yes])
    if test "$enable_epkg_install" = "no"; then
      EPKG=":";
    fi

    # generate Makefile variables
dnl     AC_SUBST([ENCAP_SOURCE])
dnl     AC_SUBST([ENCAP_TARGET])
dnl     AC_SUBST([ENCAP_PKGSPEC])
dnl     AC_SUBST([EPKG])
dnl     AC_SUBST([MKENCAP])
dnl     AC_SUBST([MKENCAP_OPTS])
dnl 
dnl     m4_ifdef([EM_MAKEFILE_END], [
dnl       # generate rules for make install target
dnl       EM_MAKEFILE_END([[
dnl target modify <install>:
dnl 	command \\\${MKENCAP} \\\`test -f \\\${srcdir}/COPYRIGHT && echo -I \\\${srcdir}/COPYRIGHT\\\` \\\${MKENCAP_OPTS} -s \\\${DESTDIR}\\\${ENCAP_SOURCE} -e \\\${ENCAP_PKGSPEC}
dnl 	command if test -z \\\\\"\\\${DESTDIR}\\\\\"; then \
dnl 		\\\${EPKG} -s \\\${ENCAP_SOURCE} -t \\\${ENCAP_TARGET} \\\${ENCAP_PKGSPEC}; \
dnl 	fi
dnl ]])])

    ENCAP_DEFS="ENCAP_SOURCE = ${ENCAP_SOURCE}\\
ENCAP_TARGET = ${ENCAP_TARGET}\\
ENCAP_PKGSPEC = ${ENCAP_PKGSPEC}\\
EPKG = ${EPKG:-:}\\
MKENCAP = ${MKENCAP:-:}\\
MKENCAP_OPTS = ${MKENCAP_OPTS}";
    AC_SUBST([ENCAP_DEFS])

    dnl ### generate rules for make install target
    ENCAP_INSTALL_RULES='if test -f ${top_srcdir}/COPYRIGHT; then \\\
		${INSTALL_DATA} ${top_srcdir}/COPYRIGHT ${ENCAP_SOURCE}/${ENCAP_PKGSPEC}; \\\
	fi\
	${MKENCAP} ${MKENCAP_OPTS} -s ${DESTDIR}${ENCAP_SOURCE} -e ${ENCAP_PKGSPEC};\
	if test -z \"${DESTDIR}\"; then \\\
		${EPKG} -s ${ENCAP_SOURCE} -t ${ENCAP_TARGET} ${ENCAP_PKGSPEC}; \\\
	fi';
    AC_SUBST([ENCAP_INSTALL_RULES])

    ENCAP_INSTALL_TARGET=$2
  else
    ENCAP_INSTALL_TARGET=$3
  fi

  AC_SUBST([ENCAP_INSTALL_TARGET])
])


