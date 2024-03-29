AC_PREREQ(2.53)
AC_INIT([matchbox-stroke], 0.0.0, [mallum@handhelds.org])
AC_CONFIG_SRCDIR([src/matchbox-stroke.c])

AM_INIT_AUTOMAKE()
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_GNU_SOURCE
AC_PROG_CC
AC_HEADER_DIRENT
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT

AC_ARG_ENABLE(debug,
  [  --enable-debug    enable debug ( verbose ) build],
     enable_debug=$enableval, enable_debug=no )

AC_ARG_WITH(expat-includes,    
  [  --with-expat-includes=DIR     Use Expat includes in DIR], 
	   expat_includes=$withval, expat_includes=yes)

AC_ARG_WITH(expat-lib,         
  [  --with-expat-lib=DIR          Use Expat library in DIR], 
	   expat_lib=$withval, expat_lib=yes)


PKG_CHECK_MODULES(MBSTROKE, xft libfakekey xrender,,
	         AC_MSG_ERROR([*** Required Librarys not found ***]))

dnl ------ Expat ------------------------------------------------------------

  case "$expat_includes" in
    yes|no)
	EXPAT_CFLAGS=""
	;;
    *)
	EXPAT_CFLAGS="-I$expat_includes"
	;;
  esac
	
  case "$expat_lib" in
    yes)
	EXPAT_LIBS="-lexpat"
	;;
    no)
	;;
    *)
	EXPAT_LIBS="-L$expat_lib -lexpat"
	;;
   esac

   expatsaved_CPPFLAGS="$CPPFLAGS"
   CPPFLAGS="$CPPFLAGS $EXPAT_CFLAGS"
   expatsaved_LIBS="$LIBS"
   LIBS="$LIBS $EXPAT_LIBS"

   AC_CHECK_HEADER(expat.h)
   case "$ac_cv_header_expat_h" in
     no)
	 AC_CHECK_HEADER(xmlparse.h)

	 case "$ac_cv_header_xmlparse_h" in
	   no)
		have_expat_header=no;
		;;
	   yes)
		HAVE_XMLPARSE_H=1
		AC_SUBST(HAVE_XMLPARSE_H)
		AC_DEFINE_UNQUOTED(HAVE_XMLPARSE_H,$HAVE_XMLPARSE_H,
				       [Use xmlparse.h instead of expat.h])
		have_expat_header=yes
	        ;;
	 esac
	 ;;
     yes)
	 have_expat_header=yes
	;;
   esac
	
   case "$have_expat_header" in
     no)
	expat=no
	;;
     yes)
	AC_CHECK_FUNCS(XML_ParserCreate)

	case "$ac_cv_func_XML_ParserCreate" in
          no)
	  	expat=no
		;;
	  yes)
		HAVE_EXPAT=1
		AC_SUBST(HAVE_EXPAT)
		AC_DEFINE_UNQUOTED(HAVE_EXPAT,$HAVE_EXPAT,
		[Found a useable expat library])
		;;
	esac
	;;
   esac
	
   CPPFLAGS="$saved_CPPFLAGS"
   LIBS="$saved_LIBS"

   if test x$expat = xno; then
      AC_MSG_ERROR([cannot find expat library])
   fi


dnl ------ Debug Build ------------------------------------------------------

if test x$enable_debug = xyes; then
   AC_DEFINE_UNQUOTED(WANT_DEBUG, 1, [Make a debug (Verbose) Build])
fi


dnl ------ GCC flags --------------------------------------------------------

if test "x$GCC" = "xyes"; then
        GCC_WARNINGS="-g -Wall -fno-strict-aliasing"
        MBSTROKE_CFLAGS="$GCC_WARNINGS $MBSTROKE_CFLAGS"
fi


dnl ------ Substitute in found libs, clags to Makefiles etc -----------------

AC_SUBST(MBSTROKE_CFLAGS)
AC_SUBST(MBSTROKE_LIBS)

AC_SUBST(EXPAT_LIBS)
AC_SUBST(EXPAT_CFLAGS)


AC_OUTPUT([
Makefile
src/Makefile  
data/Makefile
])

dnl ==========================================================================
echo "
                    Matchbox-Stroke $VERSION
                  =========================

        prefix:                 ${prefix}
        source code location:   ${srcdir}
        compiler:               ${CC} 

        Building with Debug:    ${enable_debug}
"
