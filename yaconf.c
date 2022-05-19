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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/php_scandir.h"
#include "ext/standard/info.h"
#include "php_yaconf.h"

#if PHP_MAJOR_VERSION > 7
#include "yaconf_arginfo.h"
#else
#include "yaconf_legacy_arginfo.h"
#endif

ZEND_DECLARE_MODULE_GLOBALS(yaconf);

static HashTable *ini_containers;
static HashTable *parsed_ini_files;
static zval active_ini_file_section;

zend_class_entry *yaconf_ce;

static void php_yaconf_zval_persistent(zval *zv, zval *rv);
static void php_yaconf_zval_dtor(zval *pzval);

typedef struct _yaconf_filenode {
	zend_string *filename;
	time_t mtime;
} yaconf_filenode;

#define PALLOC_HASHTABLE(ht) do { \
	(ht) = (HashTable*)pemalloc(sizeof(HashTable), 1); \
} while(0)

/* {{{ yaconf_module_entry
 */
zend_module_entry yaconf_module_entry = {
	STANDARD_MODULE_HEADER,
	"yaconf",
	NULL,
	PHP_MINIT(yaconf),
	PHP_MSHUTDOWN(yaconf),
#ifndef ZTS
	PHP_RINIT(yaconf),
#else
	NULL,
#endif
	NULL,
	PHP_MINFO(yaconf),
	PHP_YACONF_VERSION,
	PHP_MODULE_GLOBALS(yaconf),
	PHP_GINIT(yaconf),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_YACONF
ZEND_GET_MODULE(yaconf)
#endif

static void php_yaconf_hash_init(zval *zv, size_t size) /* {{{ */ {
	HashTable *ht;
	PALLOC_HASHTABLE(ht);
	/* ZVAL_PTR_DTOR is necessary in case that this array be cloned */
	zend_hash_init(ht, size, NULL, ZVAL_PTR_DTOR, 1);

#if PHP_VERSION_ID >= 70400
	zend_hash_real_init(ht, 0);
#endif
#if PHP_VERSION_ID >= 70200
	HT_ALLOW_COW_VIOLATION(ht);
#endif

	ZVAL_ARR(zv, ht);
	/* make immutable array */
#if PHP_VERSION_ID < 70300
	Z_TYPE_FLAGS_P(zv) = IS_TYPE_COPYABLE;
	GC_REFCOUNT(ht) = 2;
	GC_FLAGS(ht) |= IS_ARRAY_IMMUTABLE;
	ht->u.flags |= HASH_FLAG_STATIC_KEYS;
	ht->u.flags &= ~HASH_FLAG_APPLY_PROTECTION;
#else
	Z_TYPE_FLAGS_P(zv) = 0;
	GC_SET_REFCOUNT(ht, 2);
	GC_ADD_FLAGS(ht, IS_ARRAY_IMMUTABLE);
#endif
} 
/* }}} */

static void php_yaconf_hash_destroy(HashTable *ht) /* {{{ */ {
	zend_string *key;
	zval *element;

#if PHP_VERSION_ID < 70400
	if (((ht)->u.flags & HASH_FLAG_INITIALIZED)) {
#else
	if (HT_IS_INITIALIZED(ht)) {
#endif
		ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, element) {
			if (key) {
				free(key);
			}
			php_yaconf_zval_dtor(element);
		} ZEND_HASH_FOREACH_END();
		free(HT_GET_DATA_ADDR(ht));
	}
	free(ht);
} /* }}} */

static void php_yaconf_zval_dtor(zval *pzval) /* {{{ */ {
	switch (Z_TYPE_P(pzval)) {
		case IS_ARRAY:
			php_yaconf_hash_destroy(Z_ARRVAL_P(pzval));
			break;
		case IS_PTR:
		case IS_STRING:
			free(Z_PTR_P(pzval));
			break;
		default:
			break;
	}
}
/* }}} */

static zend_string* php_yaconf_str_persistent(char *str, size_t len) /* {{{ */ {
	zend_string *key = zend_string_init(str, len, 1);
	if (key == NULL) {
		zend_error(E_ERROR, "fail to allocate memory for string, no enough memory?");
	}
	key->h = zend_string_hash_val(key);
#if PHP_VERSION_ID < 70300
	GC_FLAGS(key) |= (IS_STR_INTERNED | IS_STR_PERMANENT);
#else
	GC_ADD_FLAGS(key, IS_STR_INTERNED | IS_STR_PERMANENT);
#endif
	return key;
}
/* }}} */

static zval* php_yaconf_symtable_update(HashTable *ht, char *key, size_t len, zval *zv) /* {{{ */ {
	zend_ulong idx;
	zval *element;
	
	if (ZEND_HANDLE_NUMERIC_STR(key, len, idx)) {
		if ((element = zend_hash_index_find(ht, idx))) {
			php_yaconf_zval_dtor(element);
			ZVAL_COPY_VALUE(element, zv);
		} else {
			element = zend_hash_index_add(ht, idx, zv);
		}
	} else {
		if ((element = zend_hash_str_find(ht, key, len))) {
			php_yaconf_zval_dtor(element);
			ZVAL_COPY_VALUE(element, zv);
		} else {
			element = zend_hash_add(ht, php_yaconf_str_persistent(key, len), zv);
		}
	}

	return element;
}
/* }}} */
	
static void php_yaconf_hash_copy(HashTable *target, HashTable *source) /* {{{ */ {
	zend_string *key;
	zend_long idx;
	zval *element, rv;

	ZEND_HASH_FOREACH_KEY_VAL(source, idx, key, element) {
		php_yaconf_zval_persistent(element, &rv);
		if (key) {
			zend_hash_update(target, php_yaconf_str_persistent(ZSTR_VAL(key), ZSTR_LEN(key)), &rv);
		} else {
			zend_hash_index_update(target, idx, &rv);
		}
	} ZEND_HASH_FOREACH_END();
} /* }}} */

static void php_yaconf_zval_persistent(zval *zv, zval *rv) /* {{{ */ {
	switch (Z_TYPE_P(zv)) {
#if PHP_VERSION_ID < 70300
		case IS_CONSTANT:
#endif
		case IS_STRING:
			ZVAL_INTERNED_STR(rv, php_yaconf_str_persistent(Z_STRVAL_P(zv), Z_STRLEN_P(zv)));
			break;
		case IS_ARRAY:
			{
				php_yaconf_hash_init(rv, zend_hash_num_elements(Z_ARRVAL_P(zv)));
				php_yaconf_hash_copy(Z_ARRVAL_P(rv), Z_ARRVAL_P(zv));
			}
			break;
		case IS_RESOURCE:
		case IS_OBJECT:
		case _IS_BOOL:
		case IS_LONG:
		case IS_NULL:
			ZEND_ASSERT(0);
			break;
	}
} /* }}} */

static inline void php_yaconf_trim_key(char **key, size_t *len) /* {{{ */ {
	/* handle foo : bar :: test */
	while (*len && (**key == ' ' || **key == ':')) {
		(*key)++;
		(*len)--;
	}

	while ((*len) && *((*key) + (*len) - 1) == ' ') {
		(*len)--;
	}
}
/* }}} */

static zval* php_yaconf_parse_nesting_key(HashTable *target, char **key, size_t *key_len, char *delim) /* {{{ */ {
	zval rv;
	zval *pzval;
	char *seg = *key;
	size_t len = *key_len;
	int nesting = 0;

	ZVAL_NULL(&rv);
	do {
		if (UNEXPECTED(++nesting > 64)) {
			YACONF_G(parse_err) = 1;
			php_error(E_WARNING, "Nesting too deep? key name contains more than 64 '.'");
			return NULL;
		}
		if (!(pzval = zend_symtable_str_find(target, seg, delim - seg))) {
			pzval = php_yaconf_symtable_update(target, seg, delim - seg, &rv);
		}

		len -= (delim - seg) + 1;
		seg = delim + 1;
		if ((delim = memchr(seg, '.', len))) {
			if (Z_TYPE_P(pzval) != IS_ARRAY) {
				php_yaconf_zval_dtor(pzval);
				php_yaconf_hash_init(pzval, 8);
			}
		} else {
			*key = seg;
			*key_len = len;
			return pzval;
		}
		target = Z_ARRVAL_P(pzval);
	} while (1);
}
/* }}} */

static void php_yaconf_simple_parser_cb(zval *key, zval *value, zval *index, int callback_type, void *arg) /* {{{ */ {
	zval *pzval, rv;
	HashTable *target = Z_ARRVAL_P((zval *)arg);
	
	if (callback_type == ZEND_INI_PARSER_ENTRY) {
		char *delim;

		if ((delim = memchr(Z_STRVAL_P(key), '.', Z_STRLEN_P(key)))) {
			char *seg = Z_STRVAL_P(key);
			size_t len = Z_STRLEN_P(key);

			pzval = php_yaconf_parse_nesting_key(target, &seg, &len, delim);
			if (pzval == NULL) {
				return;
			}

			if (Z_TYPE_P(pzval) != IS_ARRAY) {
				php_yaconf_hash_init(pzval, 8);
			}

			php_yaconf_zval_persistent(value, &rv);
			php_yaconf_symtable_update(Z_ARRVAL_P(pzval), seg, len, &rv);
		} else {
			php_yaconf_zval_persistent(value, &rv);
			if ((pzval = zend_symtable_find(target, Z_STR_P(key)))) {
				php_yaconf_zval_dtor(pzval);
				ZVAL_COPY_VALUE(pzval, &rv);
			} else {
				php_yaconf_symtable_update(target, Z_STRVAL_P(key), Z_STRLEN_P(key), &rv);
			}
		}
	} else if (callback_type == ZEND_INI_PARSER_POP_ENTRY) {
		zend_ulong idx;
		
		if (ZEND_HANDLE_NUMERIC(Z_STR_P(key), idx)) {
			if ((pzval = zend_hash_index_find(target, idx)) == NULL) {
				php_yaconf_hash_init(&rv, 8);
				pzval = zend_hash_index_update(target, idx, &rv);
			} else if (Z_TYPE_P(pzval) != IS_ARRAY) {
				php_yaconf_zval_dtor(pzval);
				php_yaconf_hash_init(pzval, 8);
			}
		} else {
			char *delim;

			if ((delim = memchr(Z_STRVAL_P(key), '.', Z_STRLEN_P(key)))) {
				zval *parent;
				char *seg = Z_STRVAL_P(key);
				size_t len = Z_STRLEN_P(key);

				parent = php_yaconf_parse_nesting_key(target, &seg, &len, delim);
				if (parent == NULL) {
					return;
				}

				if (Z_TYPE_P(parent) != IS_ARRAY) {
					php_yaconf_hash_init(parent, 8);
					php_yaconf_hash_init(&rv, 8);
					pzval = php_yaconf_symtable_update(Z_ARRVAL_P(parent), seg, len, &rv);
				} else {
					if ((pzval = zend_symtable_str_find(Z_ARRVAL_P(parent), seg, len))) {
						if (Z_TYPE_P(pzval) != IS_ARRAY) {
							php_yaconf_hash_init(&rv, 8);
							pzval = php_yaconf_symtable_update(Z_ARRVAL_P(parent), seg, len, &rv);
						}
					} else {
						php_yaconf_hash_init(&rv, 8);
						pzval = php_yaconf_symtable_update(Z_ARRVAL_P(parent), seg, len, &rv);
					}
				}
			} else {
				if ((pzval = zend_symtable_str_find(target, Z_STRVAL_P(key), Z_STRLEN_P(key)))) {
					if (Z_TYPE_P(pzval) != IS_ARRAY) {
						php_yaconf_zval_dtor(pzval);
						php_yaconf_hash_init(pzval, 8);
					}	
				} else {
					php_yaconf_hash_init(&rv, 8);
					pzval = php_yaconf_symtable_update(target, Z_STRVAL_P(key), Z_STRLEN_P(key), &rv);
				}
			}
		}

		ZEND_ASSERT(Z_TYPE_P(pzval) == IS_ARRAY);
		php_yaconf_zval_persistent(value, &rv);
		if (index && Z_STRLEN_P(index)) {
			php_yaconf_symtable_update(Z_ARRVAL_P(pzval), Z_STRVAL_P(index), Z_STRLEN_P(index), &rv);
		} else {
			zend_hash_next_index_insert(Z_ARRVAL_P(pzval), &rv);
		}
	} else if (callback_type == ZEND_INI_PARSER_SECTION) {
	}
}
/* }}} */

static void php_yaconf_ini_parser_cb(zval *key, zval *value, zval *index, int callback_type, void *arg) /* {{{ */ {
	zval *target = (zval *)arg;

	if (YACONF_G(parse_err)) {
		return;
	}

	if (callback_type == ZEND_INI_PARSER_SECTION) {
		zval *parent;
		char *section, *delim;
		size_t sec_len;
		int nesting = 0;

		php_yaconf_hash_init(&active_ini_file_section, 128);

		section = Z_STRVAL_P(key);
		sec_len = Z_STRLEN_P(key);

		while ((delim = (char *)zend_memrchr(section, ':', sec_len))) {
			section = delim + 1;
			sec_len = sec_len - (delim - Z_STRVAL_P(key) + 1);

			if (++nesting > 16) {
				php_error(E_WARNING, "Nesting too deep? Only less than 16 level inheritance is allowed");
				YACONF_G(parse_err) = 1;
				return;
			}

			php_yaconf_trim_key(&section, &sec_len);
			if ((parent = zend_hash_str_find(Z_ARRVAL_P(target), section, sec_len))) {
				if (Z_TYPE_P(parent) == IS_ARRAY) {
					php_yaconf_hash_copy(Z_ARRVAL(active_ini_file_section), Z_ARRVAL_P(parent));
				} else {
					/* May copy the single value into current section? */
				}
			}
			section = Z_STRVAL_P(key);
			sec_len = delim - section;
		} 
		if (sec_len == 0) {
			php_yaconf_hash_destroy(Z_ARRVAL(active_ini_file_section));
			ZVAL_UNDEF(&active_ini_file_section);
			return;
		}
		php_yaconf_trim_key(&section, &sec_len);
		php_yaconf_symtable_update(Z_ARRVAL_P(target), section, sec_len, &active_ini_file_section);
	} else if (value) {
		if (!Z_ISUNDEF(active_ini_file_section)) {
			target = &active_ini_file_section;
		}
		php_yaconf_simple_parser_cb(key, value, index, callback_type, target);
	}
}
/* }}} */

static int php_yaconf_parse_ini_file(const char *filename, zval *result) /* {{{ */ {
	FILE *fp;
	if ((fp = VCWD_FOPEN(filename, "r"))) {
#if PHP_VERSION_ID < 80100
		zend_file_handle fh = {{0}, 0};
		fh.filename = filename;
		fh.handle.fp = fp;
		fh.type = ZEND_HANDLE_FP;
#else
		zend_file_handle fh;
		zend_stream_init_fp(&fh, fp, filename);
#endif
		ZVAL_UNDEF(&active_ini_file_section);
		YACONF_G(parse_err) = 0;
		php_yaconf_hash_init(result, 128);
		if (zend_parse_ini_file(&fh, 1, 0 /* ZEND_INI_SCANNER_NORMAL */,
					php_yaconf_ini_parser_cb, (void *)result) == FAILURE || YACONF_G(parse_err)) {
			YACONF_G(parse_err) = 0;
			php_yaconf_hash_destroy(Z_ARRVAL_P(result));
			ZVAL_NULL(result);
#if PHP_VERSION_ID >=80100
			zend_destroy_file_handle(&fh);
#endif
			return 0;
		}
#if PHP_VERSION_ID >= 80100
		zend_destroy_file_handle(&fh);
#endif
	}
	return 1;
}
/* }}} */

PHP_YACONF_API zval *php_yaconf_get(zend_string *name) /* {{{ */ {
	if (EXPECTED(ini_containers)) {
		zval *pzval;
		char *seg, *delim;
		size_t len;
		HashTable *target = ini_containers;

		if ((delim = memchr(ZSTR_VAL(name), '.', ZSTR_LEN(name)))) {
			seg = ZSTR_VAL(name);
			len = ZSTR_LEN(name);
			do {
				if (!(pzval = zend_symtable_str_find(target, seg, delim - seg)) || Z_TYPE_P(pzval) != IS_ARRAY) {
					return pzval;
				}
				target = Z_ARRVAL_P(pzval);
				len -= (delim - seg) + 1;
				seg = delim + 1;
				if (!(delim = memchr(seg, '.', len))) {
					return zend_symtable_str_find(target, seg, len);
				}
			} while (1);
		} else {
			return zend_symtable_find(target, name);
		}
	}
	return NULL;
}
/* }}} */

PHP_YACONF_API int php_yaconf_has(zend_string *name) /* {{{ */ {
	return php_yaconf_get(name) != NULL;
}
/* }}} */

/** {{{ proto public Yaconf::get(string $name, $default = NULL)
*/
PHP_METHOD(yaconf, get) {
	zend_string *name;
	zval *val, *defv = NULL;

	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_STR(name)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(defv)
	ZEND_PARSE_PARAMETERS_END();

	val = php_yaconf_get(name);
	if (val) {
		RETURN_ZVAL(val, 0, 0);
	} else if (defv) {
		RETURN_ZVAL(defv, 1, 0);
	}

	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Yaconf::has(string $name)
*/
PHP_METHOD(yaconf, has) {
	zend_string *name;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(name)
	ZEND_PARSE_PARAMETERS_END();

	RETURN_BOOL(php_yaconf_has(name));
}
/* }}} */

/** {{{ proto public Yaconf::__debug_info(string $name)
 */
PHP_METHOD(yaconf, __debug_info) {
	zend_string *name;
	zval *val;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) == FAILURE) {
		return;
	} 

	val = php_yaconf_get(name);
	if (val) {
		zval zv;
		char *address;
		size_t len;
		array_init(return_value);
		ZVAL_STR(&zv, name);
		zend_hash_str_add_new(Z_ARRVAL_P(return_value), "key", sizeof("key") - 1, &zv);
		Z_TRY_ADDREF(zv);
		len = spprintf(&address, 0, "%p", val); /* can not use zend_strpprintf as it only exported after PHP-7.2 */
		ZVAL_STR(&zv, zend_string_init(address, len, 0)); 
		efree(address);
		zend_hash_str_add_new(Z_ARRVAL_P(return_value), "address", sizeof("address") - 1, &zv);
		zend_hash_str_add_new(Z_ARRVAL_P(return_value), "val", sizeof("val") - 1, val);
		Z_TRY_ADDREF_P(val);
		return;
	}

	RETURN_NULL();
}
/* }}} */

/* {{{  yaconf_methods */
zend_function_entry yaconf_methods[] = {
	PHP_ME(yaconf, get, arginfo_class_Yaconf_get, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(yaconf, has, arginfo_class_Yaconf_has, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(yaconf, __debug_info, arginfo_class_Yaconf___debug_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("yaconf.directory", "", PHP_INI_SYSTEM, OnUpdateString, directory, zend_yaconf_globals, yaconf_globals)
#ifndef ZTS
	STD_PHP_INI_ENTRY("yaconf.check_delay", "300", PHP_INI_SYSTEM, OnUpdateLong, check_delay, zend_yaconf_globals, yaconf_globals)
#endif
PHP_INI_END()
/* }}} */

/* {{{ PHP_GINIT_FUNCTION
*/
PHP_GINIT_FUNCTION(yaconf)
{
	yaconf_globals->directory = NULL;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(yaconf)
{
	const char *dirname;
	size_t dirlen;
	zend_class_entry ce;
	zend_stat_t dir_sb = {0};

	REGISTER_INI_ENTRIES();

	INIT_CLASS_ENTRY(ce, "Yaconf", yaconf_methods);

	yaconf_ce = zend_register_internal_class_ex(&ce, NULL);

	if ((dirname = YACONF_G(directory)) && (dirlen = strlen(dirname)) 
#ifndef ZTS
			&& !VCWD_STAT(dirname, &dir_sb) && S_ISDIR(dir_sb.st_mode)
#endif
			) {
		int ndir;
		struct dirent **namelist;
		char *p, ini_file[MAXPATHLEN];

#ifndef ZTS
		YACONF_G(directory_mtime) = dir_sb.st_mtime;
#endif

		if ((ndir = php_scandir(dirname, &namelist, 0, php_alphasort)) > 0) {
			uint32_t i;
			zend_stat_t sb;

			PALLOC_HASHTABLE(ini_containers);
			zend_hash_init(ini_containers, ndir, NULL, NULL, 1);

			PALLOC_HASHTABLE(parsed_ini_files);
			zend_hash_init(parsed_ini_files, ndir, NULL, NULL, 1);

			for (i = 0; i < ndir; i++) {
				if (!(p = strrchr(namelist[i]->d_name, '.')) || strcmp(p, ".ini")) {
					free(namelist[i]);
					continue;
				}

				snprintf(ini_file, MAXPATHLEN, "%s%c%s", dirname, DEFAULT_SLASH, namelist[i]->d_name);

				if (VCWD_STAT(ini_file, &sb) == 0) {
					if (S_ISREG(sb.st_mode)) {
						zval result;
						FILE *fp;
						yaconf_filenode node;
						if (!php_yaconf_parse_ini_file(ini_file, &result)) {
							continue;
						}
						php_yaconf_symtable_update(ini_containers, namelist[i]->d_name, p - namelist[i]->d_name, &result);
						node.filename = zend_string_init(namelist[i]->d_name, strlen(namelist[i]->d_name), 1);
						node.mtime = sb.st_mtime;
						zend_hash_update_mem(parsed_ini_files, node.filename, &node, sizeof(yaconf_filenode));
					}
				} else {
					php_error(E_ERROR, "Could not stat '%s'", ini_file);
				}
				free(namelist[i]);
			}
#ifndef ZTS
			YACONF_G(last_check) = time(NULL);
#endif
			free(namelist);
		} else {
			php_error(E_ERROR, "Couldn't opendir '%s'", dirname);
		}
	}

	return SUCCESS;
}
/* }}} */

#ifndef ZTS
/* {{{ PHP_RINIT_FUNCTION(yaconf)
*/
PHP_RINIT_FUNCTION(yaconf)
{
	if (YACONF_G(check_delay) && (time(NULL) - YACONF_G(last_check) < YACONF_G(check_delay))) {
		YACONF_DEBUG("config check delay doesn't execeed, ignore");
		return SUCCESS;
	} else {
		char *dirname;
		zend_stat_t dir_sb = {0};

		YACONF_G(last_check) = time(NULL);

		if ((dirname = YACONF_G(directory)) && !VCWD_STAT(dirname, &dir_sb) && S_ISDIR(dir_sb.st_mode)) {
			if (dir_sb.st_mtime == YACONF_G(directory_mtime)) {
				YACONF_DEBUG("config directory is not modefied");
				return SUCCESS;
			} else {
				zval result;
				int i, ndir;
				struct dirent **namelist;
				char *p, ini_file[MAXPATHLEN];

				YACONF_G(directory_mtime) = dir_sb.st_mtime;

				if ((ndir = php_scandir(dirname, &namelist, 0, php_alphasort)) > 0) {
					zend_stat_t sb;
					yaconf_filenode *node = NULL;

					for (i = 0; i < ndir; i++) {
						zval *orig_ht = NULL;
						if (!(p = strrchr(namelist[i]->d_name, '.')) || strcmp(p, ".ini")) {
							free(namelist[i]);
							continue;
						}

						snprintf(ini_file, MAXPATHLEN, "%s%c%s", dirname, DEFAULT_SLASH, namelist[i]->d_name);
						if (VCWD_STAT(ini_file, &sb) || !S_ISREG(sb.st_mode)) {
							free(namelist[i]);
							continue;
						}

						if ((node = (yaconf_filenode*)zend_hash_str_find_ptr(parsed_ini_files, namelist[i]->d_name, strlen(namelist[i]->d_name))) == NULL) {
							YACONF_DEBUG("new configure file found");
						} else if (node->mtime == sb.st_mtime) {
							free(namelist[i]);
							continue;
						}

						if (!php_yaconf_parse_ini_file(ini_file, &result)) {
							free(namelist[i]);
							continue;
						}

						if ((orig_ht = zend_symtable_str_find(ini_containers, namelist[i]->d_name, p - namelist[i]->d_name)) != NULL) {
							php_yaconf_hash_destroy(Z_ARRVAL_P(orig_ht));
							ZVAL_COPY_VALUE(orig_ht, &result);
						} else {
							php_yaconf_symtable_update(ini_containers, namelist[i]->d_name, p - namelist[i]->d_name, &result);
						}

						if (node) {
							node->mtime = sb.st_mtime;
						} else {
							yaconf_filenode n = {0};
							n.filename = zend_string_init(namelist[i]->d_name, strlen(namelist[i]->d_name), 1);
							n.mtime = sb.st_mtime;
							zend_hash_update_mem(parsed_ini_files, n.filename, &n, sizeof(yaconf_filenode));
						}
						free(namelist[i]);
					}
					free(namelist);
				}
				return SUCCESS;
			}
		} 
		YACONF_DEBUG("stat config directory failed");
	}

	return SUCCESS;
}
/* }}} */
#endif

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(yaconf)
{
	UNREGISTER_INI_ENTRIES();

	if (parsed_ini_files) {
		php_yaconf_hash_destroy(parsed_ini_files);
	}

	if (ini_containers) {
		php_yaconf_hash_destroy(ini_containers);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(yaconf)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "yaconf support", "enabled");
	php_info_print_table_row(2, "version", PHP_YACONF_VERSION);
#ifndef ZTS
	php_info_print_table_row(2, "yaconf config last check time",  ctime(&(YACONF_G(last_check))));
#else
	php_info_print_table_row(2, "yaconf config last check time",  "-");
#endif
	php_info_print_table_end();

	php_info_print_table_start();
	php_info_print_table_header(2, "parsed filename", "mtime");
	if (parsed_ini_files && zend_hash_num_elements(parsed_ini_files)) {
		yaconf_filenode *node;
		ZEND_HASH_FOREACH_PTR(parsed_ini_files, node) {
			php_info_print_table_row(2, ZSTR_VAL(node->filename),  ctime(&node->mtime));
		} ZEND_HASH_FOREACH_END();
	}
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
