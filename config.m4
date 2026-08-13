dnl $Id$
dnl config.m4 for extension yaconf

PHP_ARG_ENABLE(yaconf, whether to enable yaconf support,
[  --enable-yaconf           Enable yaconf support])

if test "$PHP_YACONF" != "no"; then
  AC_CHECK_HEADERS([sys/mman.h])
  AC_CHECK_FUNCS([mprotect])

  AS_IF([test "$ac_cv_func_mprotect" = "yes" && test "$ac_cv_header_sys_mman_h" = "yes"], [
    AC_DEFINE([YACONF_HAVE_MPROTECT], [1], [Whether to enable mprotect support])
  ])

  PHP_SUBST(YACONF_SHARED_LIBADD)
  PHP_NEW_EXTENSION(yaconf, yaconf.c, $ext_shared)
  PHP_INSTALL_HEADERS([ext/yaconf], [php_yaconf.h])
fi
