dnl =================================================================
dnl configure.in for cbgp
dnl
dnl @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
dnl $Id: configure.in,v 1.88 2009-08-31 15:04:31 bqu Exp $
dnl =================================================================

AC_REVISION($Revision: 1.88 $)

m4_include([version.m4])
AC_INIT([cbgp], [VERSION_NUMBER], [bruno.quoitin@umons.ac.be])
AC_CONFIG_SRCDIR(src/main.c)
AC_PREREQ(2.57)
AC_PREFIX_DEFAULT(/usr/local)


dnl Obsolete ?
dnl but avoid warning about AC_CANONICAL_TARGET being called multiple
dnl times :(
AC_CANONICAL_SYSTEM

dnl Version setting up for Automake
AM_INIT_AUTOMAKE([no-define])
AC_CONFIG_HEADER(src/config.h)
#AM_MAINTAINER_MODE


dnl Checks for programs.
AC_PROG_CC([gcc])
AC_PROG_INSTALL
#AC_PROG_CXX
AC_PROG_LN_S
AM_PROG_CC_C_O

dnl Checks for required header files/functions
AC_HEADER_STDC
AC_CHECK_HEADER(limits.h)
AC_CHECK_FUNCS(getrlimit setrlimit)
AC_CHECK_HEADER(sys/resource.h)
AC_CHECK_FUNCS(syslog)
AC_CHECK_HEADER(getopt.h,,[
  AC_MSG_ERROR([getopt.h is required])])
AC_CHECK_FUNCS(execvp fork waitpid)
AC_CHECK_FUNCS(vasprintf)


dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T


dnl Now arrange the build libtool
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL
AC_SUBST(CBGP_LT_RELEASE, [$PACKAGE_VERSION])

dnl *****************************************************************
dnl LIBGDS CHECK
dnl *****************************************************************
LIBGDS_VERSION="2.2.0"
AC_SUBST(LIBGDS_VERSION, [$LIBGDS_VERSION])
PKG_CHECK_MODULES([LIBGDS], [libgds >= $LIBGDS_VERSION],[],[
  AC_MSG_ERROR([libgds >= $LIBGDS_VERSION is needed to compile cbgp])
])
AC_SUBST(LIBGDS_CFLAGS)
AC_SUBST(LIBGDS_LIBS)
#AC_CHECK_LIB(gds, list_create, [gds_ok=yes; LIBS="$LIBS -lgds"], gds_ok=no)
#if test "x$gds_ok" != "xyes"; then
#   AC_MSG_ERROR([libgds is needed to compile cbgp])
#else
#   dnl Check for headers
#   AC_CHECK_HEADERS(libgds/fifo.h, gds_header_ok=yes, gds_header_ok=no)
#   if test "x$gds_header_ok" != "xyes"; then
#      AC_MSG_ERROR([libgds headers are needed to compile cbgp])
#   fi
#   dnl Check 'mem_flag_set' function
#   AC_CHECK_FUNCS(mem_flag_set)
#fi


dnl *****************************************************************
dnl ZLIB CHECK
dnl *****************************************************************
AC_CHECK_HEADERS(zlib.h)
if test "$ac_cv_header_zlib_h" != "no"; then
    AC_CHECK_LIB(z,gzopen)
fi


dnl *****************************************************************
dnl JAVA CHECK
dnl *****************************************************************
dnl AC_DEFUN([AC_PROG_JAVAH],[
dnl AC_REQUIRE([AC_CANONICAL_SYSTEM])dnl
dnl AC_REQUIRE([AC_PROG_CPP])dnl
dnl AC_PATH_PROG(JAVAH,javah)
dnl    ac_save_CPPFLAGS="$CPPFLAGS"
dnl changequote(, )dnl
dnl    ac_dir=`echo $ac_cv_path_JAVAH | sed 's,\(.*\)/[^/]*/[^/]*$,\1/include,'`
dnl    ac_machdep=`echo $build_os | sed 's,[-0-9].*,,' | sed 's,cygwin,win32,'`
dnl changequote([, ])dnl
dnl    CPPFLAGS="$ac_save_CPPFLAGS -I$ac_dir -I$ac_dir/$ac_machdep"
dnl    AC_TRY_CPP([#include <jni.h>],
dnl               ac_save_CPPFLAGS="$CPPFLAGS",
dnl               AC_MSG_WARN([unable to include <jni.h>]))
dnl    CPPFLAGS="$ac_save_CPPFLAGS"
dnl ])


dnl *****************************************************************
dnl JAVA/JNI CHECK: BEGIN
dnl *****************************************************************
AC_ARG_WITH(jni,dnl
[  --with-jni              build with the Java Native Interface
  --without-jni           do not use the Java Native Interface
  --with-jni=DIR          specify the location of jni.h ],,
[test -z "$with_jni" && with_jni=no])

if test "$with_jni" != "no"; then

   AX_CHECK_HAVA_HOME()
   dnl AC_MSG_CHECKING([for JAVA_HOME])
   dnl if test -z "$JAVA_HOME"; then
   dnl   AC_MSG_ERROR([undefined])
   dnl   echo "Error: environment variable JAVA_HOME is not set."
   dnl   echo "       This variable should point to your Java SDK."
   dnl   exit -1
   dnl fi
   dnl AC_MSG_RESULT([$JAVA_HOME])

#   JAVA=java
#   JAVAC=javac
   AC_CHECK_CLASSPATH
   AC_PROG_JAVAC
   AC_PROG_JAVA
   AC_PROG_JAVADOC
   AC_SUBST(JAVAC)

   AC_PATH_PROG(JAVAH, javah)

   AX_JNI_INCLUDE_DIR
   for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS
   do
     CPPFLAGS="$CPPFLAGS -I$JNI_INCLUDE_DIR"
   done

   AX_PROG_JAVAH


#   jni_paths=""
#   if test "$with_jni" != "yes"; then
#     jni_paths="$with_jni"
#   fi
#   for jni_path in "$JAVA_HOME/include" "$jni_paths"; do
#     AC_MSG_CHECKING([for jni.h in $jni_path])
#     if test -e "$jni_path/jni.h"; then
#       AC_MSG_RESULT([ok])
#       AC_MSG_CHECKING([for jni_md.h in $jni_path])
#       if test -e "$jni_path/jni_md.h"; then
#         AC_MSG_RESULT([ok])
#         jni_cflags="-I$jni_path"
#         break
#       else
#         ac_machdep=`echo $build_os | sed 's,[-0-9].*,,' | sed 's,cygwin,win32,' | sed 's,linux-gnu,linux,'`
#         AC_MSG_CHECKING([for jni_md.h in $jni_path/$ac_machdep])
#         if test -e "$jni_path/$ac_machdep/jni_md.h"; then
#           AC_MSG_RESULT([ok])
#           jni_cflags="-I$jni_path -I$jni_path/$ac_machdep"
#           break
#         fi
#       fi
#     fi
#   done
#
#   if test -z "$jni_cflags"; then
#     AC_MSG_RESULT([no])
#     echo "Error: no jni.h could be found at the given location(s).";
#     echo "       Specified path: $jni_paths";
#     exit -1;
#   else
#     CFLAGS="$CFLAGS $jni_cflags"
#     CPPFLAGS="$CPPFLAGS $jni_cflags"
#   fi
#
#   dnl Check that jni_md.h header can be compiled
#   AC_CHECK_HEADERS(jni_md.h, [jni_md_header_ok=yes], [jni_md_header_ok=no])
#   if test "$jni_md_header_ok" != "yes"; then
#     AC_MSG_WARN([could not find jni_md.h (machine dependent part of jni.h)])
#     dnl TODO: add machine dependent sub-directory
#     dnl (where jni_md.h might be located)
#   fi
#
#   dnl Check that jni.h header can be compiled
#   AC_CHECK_HEADERS(jni.h, [jni_header_ok=yes], [jni_header_ok=no])
#   if test "$jni_header_ok" != yes; then
#      with_jni="no"
#      echo "Error: could not find java native interface header (jni.h)";
#      echo "       You have requested the JNI of C-BGP.";
#      echo "       Use --with-jni=DIR to mention the path to jni.h";
#      exit -1;
#   else
#      AC_DEFINE(HAVE_JNI, 1, [Define to 1 if jni is properly configured])
#   fi
fi
AM_CONDITIONAL([WITH_JNI], [test "$with_jni" != no])


dnl *****************************************************************
dnl PCRE CHECK
dnl *****************************************************************
AC_ARG_WITH(pcre,
        [  --with-pcre=DIR         use pcre in <DIR>],
           PATH=$withval/bin:$PATH)
AC_CHECK_PROG(LIBPCRE_CONFIG, pcre-config, pcre-config, false)
if test "$LIBPCRE_CONFIG" = "false"; then
   echo "ERROR: Could not find pcre-config in your path.";
   echo "       You need PCRE to compile C-BGP.";
   echo "       Source: http://www.pcre.org";
   echo "       Use --with-pcre=DIR to mention another search path";
   exit -1;
fi
AC_MSG_CHECKING([for pcre libs])
PCRE_LIBS=`pcre-config --libs`
AC_MSG_RESULT($pcre_libs)
AC_MSG_CHECKING([for pcre cflags])
PCRE_CFLAGS=`pcre-config --cflags`
AC_MSG_RESULT($pcre_cflags)
AC_SUBST(PCRE_LIBS)
AC_SUBST(PCRE_CFLAGS)
AC_DEFINE([HAVE_LIBPCRE], 1, [Define to 1 if PCRE library was found ])


# Check for libbz2
AC_CHECK_LIB(bz2, BZ2_bzReadOpen, [],
  AC_MSG_ERROR([libbz2 is required to build libbgpdump],1)
)

dnl *****************************************************************
dnl ENABLE/DISABLE LIBBGPDUMP ?
dnl *****************************************************************
AC_ARG_ENABLE(bgpdump,
   AC_HELP_STRING([--enable-bgpdump],
      [enable bgpdump]),
   cbgp_config_bgpdump="$enableval",
)
if test "$cbgp_config_bgpdump" == "yes"; then
    if test "$ac_cv_lib_z_gzopen" = "no"; then
        AC_WARN([libz not found, bgpdump disabled.])
    else
	AM_CONDITIONAL([WITH_BGPDUMP], [test "1" = "1"])
	AC_DEFINE(HAVE_BGPDUMP, 1, [Define to 1 if bgpdump can be used])
    fi
else
    AM_CONDITIONAL([WITH_BGPDUMP], [test "1" = "0"])
fi


dnl *****************************************************************
dnl READLINE CHECK
dnl *****************************************************************
AC_ARG_WITH(readline,dnl
[  --with-readline        use the builtin minimal readline function (default)
  --without-readline      do not use the builtin readline function
  --with-readline=DIR     Specify the location of GNU readline],,
  [test -z "$with_readline" && with_readline=yes])

dnl GNU readline and the required terminal library
if test "$with_readline" != no; then
  dnl check for terminal library
  dnl this is a very cool solution from octave's configure.in
  gp_tcap=""
  for termlib in ncurses curses termcap terminfo termlib; do
    AC_CHECK_LIB(${termlib}, tputs, [gp_tcap="$gp_tcap -l$termlib"])
    case "$gp_tcap" in
      *-l${termlib}*)
        break
      ;;
    esac
  done

  dnl Default readline (should be found in current paths)
  if test "$with_readline" != "yes"; then
    dnl Specific readline (path specified as an option)
    if test -d "$with_readline/include"; then
      READLINE_CFLAGS="-I$with_readline/include"
    fi
    if test -d "$with_readline/lib"; then
      READLINE_LIBS="-L$with_readline/lib"
    fi
  fi

  readline_test_state=ok
  READLINE_LIBS="$gp_tcap $READLINE_LIBS"

  dnl Check that library is found
  AC_CHECK_LIB(readline, readline,
    [READLINE_LIBS="$READLINE_LIBS -lreadline"],
    [readline_test_state=missing],
    [$READLINE_LIBS])

  dnl Check that this is GNU readline
  if test "$readline_test_state" = ok; then
    AC_CHECK_LIB(readline,rl_gnu_readline_p,
      [AC_DEFINE(HAVE_RL_GNU_READLINE_P, 1,
        [ Define if rl_gnu_readline_p is available. ])],
      [readline_test_state=notgnu],[$READLINE_LIBS])
  fi

  dnl Check readline headers
  if test "$readline_test_state" = ok; then
    AC_CHECK_HEADERS(readline/readline.h,[],[readline_test_state=headers])
    AC_CHECK_HEADERS(readline/history.h,[],[readline_test_state=headers])
  fi
        
  dnl Check readline functions
  if test "$readline_test_state" = ok; then
    AC_CHECK_LIB(readline,rl_completion_matches,
      [AC_DEFINE(HAVE_RL_COMPLETION_MATCHES, 1,
                 [ Define if rl_completion_matches is available. ])],
      [readline_test_state=functions],[$READLINE_LIBS])
    AC_CHECK_LIB(readline,rl_filename_completion_function,
      [AC_DEFINE(HAVE_RL_FILENAME_COMPLETION_FUNCTION, 1,
                 [ Define if rl_filename_completion_function is available. ])],
      [readline_test_state=functions],[$READLINE_LIBS])
    AC_CHECK_LIB(readline,rl_on_new_line,
      [AC_DEFINE(HAVE_RL_ON_NEW_LINE, 1,
                 [ Define if rl_on_new_line is available. ])],
      [readline_test_state=functions],[$READLINE_LIBS])
  fi

  dnl Summary
  if test "$readline_test_state" = ok; then
    AC_DEFINE(HAVE_LIBREADLINE,1,
              [ Define if you are using the GNU readline library. ])
    dnl Add libraries (used by Makefile.am)
    AC_SUBST(READLINE_CFLAGS)
    AC_SUBST(READLINE_LIBS)
  else
    case "$readline_test_state" in
    missing)
      AC_MSG_WARN([The readline library was not found !])
      break;;
    notgnu)
      AC_MSG_WARN([A readline library was found, but it is not from GNU !])
      break;;
    headers)
      AC_MSG_WARN([Some readline headers were not found !
                   Please add path to headers to CPPFLAGS in Makefile])
      break;;
    functions)
      AC_MSG_WARN([Some readline functions were not found !
                   Your readline library might be too old.])
      break;;
    esac
    AC_MSG_WARN([Although C-BGP will still compile, some features
                 will not be available.])
  fi
fi

dnl *****************************************************************
dnl CHECK FOR IPV6 SUPPORT (BGPDUMP)
dnl *****************************************************************
AC_ARG_ENABLE(ipv6,
  AC_HELP_STRING([--disable-ipv6], [disable support for IPv6])
)
if test "$enable_ipv6" != "no"; then
        disable_ipv6=no
fi
AC_MSG_CHECKING(for IPv6 support)
if test "$disable_ipv6" != no; then
  AC_MSG_RESULT(disabled)
else
  AC_TRY_RUN([ /* AF_INET6 available check */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
main()
{
 struct in6_addr src;
 char buf[128];
 if (inet_ntop(AF_INET6,&src,buf,128)==NULL)
   exit(1);
 else
   exit(0);
}
    ],ipv6=yes,ipv6=no,ipv6=yes)
  if test "$ipv6" = "yes"; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(BGPDUMP_HAVE_IPV6, 1, [The library will be compiled with IPv6 support])
  else
    AC_MSG_RESULT(no)
  fi
fi

dnl *****************************************************************
dnl OTHER CHECKS
dnl *****************************************************************

dnl Conditional compilation with memory debugger
AM_CONDITIONAL([WITH_LIBALLOC], [test "yes" = "no"])

dnl Test for XML availability
AC_ARG_ENABLE(xml,
   AC_HELP_STRING([--enable-xml],
      [enable XML support]),
   cbgp_config_xml="$enableval",
)
if test "x$cbgp_config_xml" = "xyes"; then
   AC_DEFINE([HAVE_XML], 1, [Define to 1 if XML support is compiled])
fi

dnl Compile experimental features ?
AC_ARG_ENABLE(experimental,
   AC_HELP_STRING([--enable-experimental],
       [enable compilation of experimental features]),
   cbgp_experimental="$enableval")
AM_CONDITIONAL([WITH_EXPERIMENTAL], [test "$cbgp_experimental" = "yes"])

dnl Compile with external-best ?
AC_ARG_ENABLE(external-best,
    AC_HELP_STRING([--enable-external-best],
      [enable the advertisement of the external best to internal routers (experimental)]),
    cbgp_external_best="$enableval")
AM_CONDITIONAL([WITH_EXTERNAL_BEST], [test "$cbgp_external_best" = "yes"])

dnl Compile with walton ?
AC_ARG_ENABLE(walton,
    AC_HELP_STRING([--enable-walton],
      [enables the advertisement of more than one path to a BGP speaker (experimental)]),
    cbgp_walton="$enableval")
AM_CONDITIONAL([WITH_WALTON], [test "$cbgp_walton" = "yes"])

dnl Compile with OSPF support ?
AC_ARG_ENABLE(ospf,
   AC_HELP_STRING([--enable-ospf],
       [enable compilation of ospf support]),
   cbgp_ospf="$enableval")
AM_CONDITIONAL([WITH_OSPF], [test "$cbgp_ospf" = "yes"])

dnl Compile with debugging symbols ?
AC_ARG_ENABLE(debug,
   AC_HELP_STRING([--enable-debug],
       [enable debugging symbols]),
   cbgp_debugging="$enableval")
if test "$cbgp_debugging" = "yes"; then
  CFLAGS="$CFLAGS -ggdb -O0"
  AC_SUBST(CFLAGS)
fi

AC_ARG_ENABLE(memory-debug,
    AC_HELP_STRING([--enable-memory-debug], [enable memory leaks tracking. LibGDS must be compiled with this flag too.]),
    cbgp_memory_debug="$enableval")
if test "$cbgp_memory_debug" = "yes"; then
  CFLAGS="$CFLAGS -D__MEMORY_DEBUG__"
  AC_SUBST(CFLAGS)
fi

dnl Compile with Electric Fence ?
AC_ARG_WITH(efence,
   AC_HELP_STRING([--with-efence],
       [link with Electrical Fence]),
   cbgp_efence="$withval")
AM_CONDITIONAL([WITH_EFENCE], [test "$cbgp_efence" = "yes"])

case $host in
  *-*-cygwin*)
    test_cygwin="yes"
    AC_DEFINE([CYGWIN], 1, [Define to 1 if the system is cygwin])
    ;;
  *)
    test_cygwin="no"
    ;;
esac
AM_CONDITIONAL([HOST_IS_CYGWIN], [test "$test_cygwin" = "yes"])

TESTS="src/checks/basic"

dnl Files to be distributed as part of the examples
EXAMPLES_EXTRA_DIST=`OLDPWD=$PWD; cd examples; ls *.topo *.cli | tr '\n' ' '; cd $OLDPWD`
AC_SUBST([EXAMPLES_EXTRA_DIST])
HELP_EXTRA_DIST=`OLDPWD=$PWD; cd doc/txt; ls *.txt | tr '\n' ' '; cd $OLDPWD`
AC_SUBST([HELP_EXTRA_DIST])

#CFLAGS="$CFLAGS -Wall -Werror -O2"
CFLAGS="$CFLAGS -Wall -O2"
AC_SUBST(CFLAGS)

AC_OUTPUT(
Makefile
doc/Makefile
doc/txt/Makefile
examples/Makefile
lib/Makefile
src/Makefile
src/bgp/Makefile
src/bgp/aslevel/Makefile
src/bgp/attr/Makefile
src/bgp/filter/Makefile
src/cli/Makefile
src/external/Makefile
src/jni/Makefile
src/jni/impl/Makefile
src/net/Makefile
src/net/traffic/Makefile
src/sim/Makefile
src/ui/Makefile
src/util/Makefile
valid/Makefile
valid/CBGPValid/Makefile
valid/tests/Makefile
libcsim.pc
cbgp.spec
src/jni/jni-test.sh)
