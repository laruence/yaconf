dnl $Id$
dnl config.m4 for extension yaconf

PHP_ARG_ENABLE(yaconf, whether to enable yaconf support,
[  --enable-yaconf           Enable yaconf support])

PHP_ARG_WITH(yaml, whether to enable YAML configuration support,
[  --with-yaml[=DIR]         Enable YAML configuration support using libyaml], no, no)

if test "$PHP_YACONF" != "no"; then
  if test "$PHP_YAML" != "no"; then
    if test "$PHP_YAML" = "yes"; then
      dnl Prefer pkg-config when libyaml is installed outside the compiler defaults
      dnl (for example by Homebrew), then fall back to ordinary include/link paths.
      PKG_CHECK_MODULES([YACONF_YAML], [yaml-0.1], [
        yaconf_yaml_save_CPPFLAGS="$CPPFLAGS"
        yaconf_yaml_save_LIBS="$LIBS"
        CPPFLAGS="$CPPFLAGS $YACONF_YAML_CFLAGS"
        LIBS="$YACONF_YAML_LIBS $LIBS"
        AC_CHECK_HEADERS([yaml.h], [],
          [AC_MSG_ERROR([YAML configuration support requested, but yaml.h was not found])])
        AC_LINK_IFELSE([
          AC_LANG_PROGRAM([[#include <yaml.h>]], [[
            yaml_parser_t parser;
            return yaml_parser_initialize(&parser) ? 0 : 1;
          ]])
        ], [], [AC_MSG_ERROR([YAML configuration support requires libyaml])])
        CPPFLAGS="$yaconf_yaml_save_CPPFLAGS"
        LIBS="$yaconf_yaml_save_LIBS"
        PHP_EVAL_INCLINE([$YACONF_YAML_CFLAGS])
        PHP_EVAL_LIBLINE([$YACONF_YAML_LIBS], [YACONF_SHARED_LIBADD])
      ], [
        AC_CHECK_HEADERS([yaml.h], [],
          [AC_MSG_ERROR([YAML configuration support requested, but yaml.h was not found])])
        PHP_CHECK_LIBRARY([yaml], [yaml_parser_initialize],
          [PHP_ADD_LIBRARY([yaml], [1], [YACONF_SHARED_LIBADD])],
          [AC_MSG_ERROR([YAML configuration support requires libyaml])])
      ])
    else
      if test ! -r "$PHP_YAML/include/yaml.h"; then
        AC_MSG_ERROR([YAML configuration support requested, but $PHP_YAML/include/yaml.h was not found])
      fi
      yaconf_yaml_libdir=""
      for yaconf_yaml_candidate in "$PHP_YAML/$PHP_LIBDIR" "$PHP_YAML/lib" "$PHP_YAML/lib64"; do
        if test -r "$yaconf_yaml_candidate/libyaml.a" || test -r "$yaconf_yaml_candidate/libyaml.so" || test -r "$yaconf_yaml_candidate/libyaml.dylib"; then
          yaconf_yaml_libdir="$yaconf_yaml_candidate"
          break
        fi
      done
      if test -z "$yaconf_yaml_libdir"; then
        AC_MSG_ERROR([YAML configuration support requested, but no libyaml library was found under $PHP_YAML])
      fi
      PHP_CHECK_LIBRARY([yaml], [yaml_parser_initialize], [
        PHP_ADD_INCLUDE([$PHP_YAML/include])
        PHP_ADD_LIBRARY_WITH_PATH([yaml], [$yaconf_yaml_libdir], [YACONF_SHARED_LIBADD])
      ], [AC_MSG_ERROR([YAML configuration support requires libyaml in $yaconf_yaml_libdir])],
      [-L$yaconf_yaml_libdir])
    fi
    AC_DEFINE([YACONF_ENABLE_YAML], [1], [Define to 1 to enable YAML configuration support.])
  fi

  PHP_SUBST(YACONF_SHARED_LIBADD)
  PHP_NEW_EXTENSION(yaconf, yaconf.c, $ext_shared)

  PHP_INSTALL_HEADERS([ext/yaconf], [php_yaconf.h])
fi
