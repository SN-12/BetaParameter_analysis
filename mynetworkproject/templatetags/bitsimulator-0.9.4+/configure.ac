AC_INIT([bitsimulator], [0.9.4+], [dominique.dhoutaut@univ-fcomte.fr])
AC_CONFIG_SRCDIR([src/packet.cpp])
AC_CONFIG_MACRO_DIR([.])
AC_CONFIG_HEADERS([src/config.h])
AM_INIT_AUTOMAKE([subdir-objects foreign])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_LANG([C++])

# used for make check, to xfail if not gcc
AM_CONDITIONAL([USE_GCC], [test "x${GCC}" = "xyes"])

# the simulator requires long type to have at least 8 bytes
AC_CHECK_SIZEOF([long])
AS_IF([test "$ac_cv_sizeof_long" -lt 8],
  AC_MSG_ERROR([The software runs only on machines with at least 8 bytes for long type]),)

# Checks for libraries.
AC_CHECK_PROG([PKG_CONFIG],[pkg-config],[yes],[no])
  test "$PKG_CONFIG" == "no" && AC_MSG_ERROR([You need the pkg-config program to build the software])

AC_CHECK_HEADER([tclap/CmdLine.h],,AC_MSG_ERROR([You need the tclap library to build the software]))
  # -DHAVE_SSTREAM is a workaround for tclap bug https://sourceforge.net/p/tclap/bugs/23/ ("inconsistent usage of config.h and HAVE_CONFIG_H")
  # the bug was fixed in tcplap 1.2.2
  PKG_CHECK_EXISTS([tclap >= 1.2.2], [], CPPFLAGS="$CPPFLAGS -DHAVE_SSTREAM")
AC_CHECK_LIB([tinyxml2],[main],,AC_MSG_ERROR([You need the tinyxml2 library to build the software]))
AC_CHECK_LIB([SDL2], [SDL_Init],,AC_MSG_ERROR([You need the SDL2 library to build the software]))
AC_CHECK_LIB([SDL2_gfx], [aapolygonColor],,AC_MSG_ERROR([You need the SDL2_gfx library to build the software]))
AC_CHECK_LIB([SDL2_ttf], [TTF_Init],,AC_MSG_ERROR([You need the SDL2_ttf library to build the software]))
AC_CHECK_LIB([pthread], [pthread_create],,AC_MSG_ERROR([You need the pthread library to build the software]))

PKG_CHECK_MODULES([freetype2], [freetype2],,)
AX_CHECK_GL([ ],)  # space as 1st parameter so that it does not add GL_CFLAGS to CFLAGS, useless for bitsimulator binary

# Checks for header files.
#AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_CHECK_FUNCS([floor pow sqrt])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
