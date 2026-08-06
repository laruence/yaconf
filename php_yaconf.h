/*
  +----------------------------------------------------------------------+
  | Yet Another Conf                                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Xinchen Hui  <laruence@php.net>                              |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_YACONF_H
#define PHP_YACONF_H

extern zend_module_entry yaconf_module_entry;
#define phpext_yaconf_ptr &yaconf_module_entry

#ifdef PHP_WIN32
#define PHP_YACONF_API __declspec(dllexport)
#else
#define PHP_YACONF_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef ZTS
#define YACONF_G(v) TSRMG(yaconf_globals_id, zend_yaconf_globals *, v)
#else
#define YACONF_G(v) (yaconf_globals.v)
#endif

#define PHP_YACONF_VERSION  "1.1.4-dev"

#ifdef YACONF_DEBUG
#undef YACONF_DEBUG
#define YACONF_DEBUG(m) fprintf(stderr, "%s\n", m);
#else
#define YACONF_DEBUG(m) 
#endif

ZEND_BEGIN_MODULE_GLOBALS(yaconf)
	char *directory;
	int   parse_err;
#ifndef ZTS
	long   check_delay;
	time_t last_check;
	time_t directory_mtime;
#endif
ZEND_END_MODULE_GLOBALS(yaconf)

PHP_MINIT_FUNCTION(yaconf);
PHP_MSHUTDOWN_FUNCTION(yaconf);
#ifndef ZTS
PHP_RINIT_FUNCTION(yaconf);
#endif
PHP_MINFO_FUNCTION(yaconf);
PHP_GINIT_FUNCTION(yaconf);

extern ZEND_DECLARE_MODULE_GLOBALS(yaconf);

/* C API for use by other PHP extensions.
 *
 * Both functions may be called any time after yaconf's MINIT has run; an
 * extension linking against this API must ensure yaconf is loaded before
 * itself (e.g. via ZEND_MOD_REQUIRED("yaconf") in its module dependencies).
 *
 * php_yaconf_get(name):
 *   Looks up `name` in the parsed configuration containers. Dots separate
 *   nesting levels ("foo.bar.baz"); numeric segments are looked up with
 *   symtable semantics, the same way INI keys are stored.
 *
 *   Returns a BORROWED pointer to the persistent zval held in yaconf's
 *   immutable containers, or NULL if the key does not exist. The caller
 *   must NOT modify, refcount or destroy the returned zval. If the value
 *   must be kept or handed on (e.g. to userland), copy it with ZVAL_COPY/
 *   ZVAL_DUP - the stored values are either immutable (IS_ARRAY_IMMUTABLE)
 *   or permanent interned strings, so copying is cheap and safe.
 *
 *   In NTS builds the containers are swapped and freed by the hot-reload
 *   in RINIT, so a returned pointer is only valid until the next reload:
 *   never cache it across requests, look the value up every time. In ZTS
 *   builds configurations are loaded once at MINIT and never reloaded,
 *   but the same rules apply.
 *
 *   `name` is only read; it does not need to be permanent/interned.
 *
 * php_yaconf_has(name):
 *   Returns non-zero if php_yaconf_get(name) would return a value,
 *   0 otherwise.
 */
BEGIN_EXTERN_C()
PHP_YACONF_API zval *php_yaconf_get(zend_string *name);
PHP_YACONF_API int php_yaconf_has(zend_string *name);
END_EXTERN_C()

#endif	/* PHP_YACONF_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
