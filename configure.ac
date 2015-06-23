#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([liblastlog2], [0.1], [rhack@redhat.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/bck-lastlog2.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h utmp.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T

# Checks for library functions.
AC_CHECK_FUNCS([memset mkdir])

AC_ARG_WITH([lastlog2-path],
	[  --with-lastlog2-path    Path to lastlog2 directory (default /var/log/lastlog2/)],
	[
		if test "x$withval" != "xno" ; then
			AC_DEFINE_UNQUOTED([LASTLOG2_PATH], ["$withval"],
			[Path to lastlog2 directory])
		fi
	 ]
)

AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
