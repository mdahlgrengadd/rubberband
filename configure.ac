
AC_INIT(RubberBand, 1.8.2, chris.cannam@breakfastquay.com)

AC_CONFIG_SRCDIR(src/StretcherImpl.h)
AC_PROG_CXX
AC_PROG_CC
AC_HEADER_STDC
AC_C_BIGENDIAN

AC_SUBST(SRC_CFLAGS)
AC_SUBST(SRC_LIBS)

AC_SUBST(SNDFILE_CFLAGS)
AC_SUBST(SNDFILE_LIBS)

AC_SUBST(FFTW_CFLAGS)
AC_SUBST(FFTW_LIBS)

changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
    *[\ \	]-fPIC\ -Wall[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -fPIC -Wall" ;;
  esac
  case " $CXXFLAGS " in
    *[\ \	]-fPIC\ -Wall[\ \	]*) ;;
    *) CXXFLAGS="$CXXFLAGS -fPIC -Wall" ;;
  esac
fi
changequote([,])dnl

AC_OUTPUT([Makefile])

