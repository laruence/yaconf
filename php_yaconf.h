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

#define PHP_YACONF_VERSION  "1.1.0"

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
