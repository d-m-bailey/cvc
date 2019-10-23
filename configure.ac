#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(CVC, [0.17.24], [cvc@shuharisystem.com])
AC_CONFIG_SRCDIR(src)
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LEX
AC_PROG_YACC

# Checks for libraries.
AC_SEARCH_LIBS([libintl_gettext], [libl])

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h libintl.h locale.h malloc.h stddef.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_FUNC_OBSTACK
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset mkdir pow rint rmdir setenv strcasecmp strchr])

AC_CONFIG_FILES([Makefile 
		scripts/Makefile
		doc/Makefile
		src_py/Makefile
		src/Makefile])
AC_OUTPUT
