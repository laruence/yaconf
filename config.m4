dnl $Id$
dnl config.m4 for extension yaconf

PHP_ARG_ENABLE(yaconf, whether to enable yaconf support,
[  --enable-yaconf           Enable yaconf support])

if test "$PHP_YACONF" != "no"; then
  PHP_SUBST(YACONF_SHARED_LIBADD)
  PHP_NEW_EXTENSION(yaconf, yaconf.c, $ext_shared)
  PHP_INSTALL_HEADERS([ext/yaconf], [php_yaconf.h])
fi
