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

/* PHP 7.1 compatibility: HT_IS_INITIALIZED was added in 7.2 */
#ifndef HT_IS_INITIALIZED
# define HT_IS_INITIALIZED(ht) ((ht)->nTableMask != 0)
#endif

ZEND_DECLARE_MODULE_GLOBALS(yaconf);

/* true globals */
static HashTable *ini_containers;
static HashTable *parsed_ini_files;
static HashTable *parsed_ini_dirs;
static zval active_ini_file_section;

/* 0 = temporary (emalloc) during MINIT parse, 1 = persistent (pemalloc) for RINIT reload */
static int yaconf_parse_persistent = 1;

/* compacted block storage */
static char    *yaconf_block = (void *)0x0;
static size_t   yaconf_block_size = 0;

static zend_always_inline int yaconf_ptr_in_block(const void *ptr) /* {{{ */ {
	return (const char*)ptr < yaconf_block + yaconf_block_size
		&& (const char*)ptr >= yaconf_block;
} /* }}} */

static zend_always_inline int yaconf_data_in_block(HashTable *ht) /* {{{ */ {
	/* only the data region moves when a table is detached, so this is the
	   test for "the table still has to be handled carefully" */
	return yaconf_ptr_in_block(HT_GET_DATA_ADDR(ht));
} /* }}} */

static zend_always_inline int yaconf_value_in_block(const zval *zv) /* {{{ */ {
	/* scalar values sit inside the bucket array, strings and sub-arrays are
	   separate block allocations.  A container whose data region has been
	   detached to the heap still holds block-resident values, so the
	   decision must be based on the value's pointer, not the slot's */
	ZEND_ASSERT(Z_TYPE_P(zv) == IS_STRING || Z_TYPE_P(zv) == IS_ARRAY);
	return yaconf_ptr_in_block(Z_PTR_P(zv));
} /* }}} */

zend_class_entry *yaconf_ce;

static void php_yaconf_zval_persistent(zval *zv, zval *rv);
static void php_yaconf_zval_dtor(zval *pzval);
static void yaconf_ht_detach(HashTable *ht);

typedef struct _yaconf_filenode {
	zend_string *filename;   /* relative path from yaconf.directory */
	time_t mtime;
} yaconf_filenode;

typedef struct _yaconf_dirnode {
	zend_string *dirname;    /* relative path from yaconf.directory */
	time_t mtime;
	HashTable *container;    /* borrowed pointer into the ini_containers tree */
} yaconf_dirnode;

/* guards against symlink loops recursing the scan until the C stack overflows */
#define YACONF_MAX_DIR_DEPTH 16

#define PALLOC_HASHTABLE(ht) do { \
	(ht) = (HashTable*)pemalloc(sizeof(HashTable), 1); \
} while(0)

/* {{{ two-phase compaction: consolidate all config arrays + strings into one block
 *
 *   parse:  ini files → persistent tree (individual pemalloc pieces)
 *   compact: walk tree → collect strings & HashTables → calc total size →
 *            allocate one block → copy strings (dedup by content) →
 *            copy HashTables (remap pointers) → free old tree
 *
 * After compaction, ini_containers and dirnode containers point into the
 * single block.  MSHUTDOWN just frees the block.  Hot reload (RINIT) rebuilds
 * the whole tree from scratch and re-compacts.
 */
#define YACONF_ALIGNED_SIZE(size) (((size) + 7UL) & ~7UL)
/* }}} */

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

/* forward declarations */
static int php_yaconf_scan_directory(const char *dirpath, const char *relpath, size_t relpath_len, HashTable *container, int is_initial, int depth);

static void php_yaconf_hash_init(zval *zv, size_t size) /* {{{ */ {
	HashTable *ht;
	ht = (HashTable*)pemalloc(sizeof(HashTable), yaconf_parse_persistent);
	/* ZVAL_PTR_DTOR is necessary in case that this array be cloned */
	zend_hash_init(ht, size, NULL, ZVAL_PTR_DTOR, yaconf_parse_persistent);

#if PHP_VERSION_ID >= 70400
	zend_hash_real_init(ht, 0);
#endif
#if PHP_VERSION_ID >= 70200
	HT_ALLOW_COW_VIOLATION(ht);
#endif

	ZVAL_ARR(zv, ht);
	/* make immutable array */
#if PHP_VERSION_ID < 70300
#if PHP_VERSION_ID < 70200
	/* SEPARATE_ARRAY checks the zval's IMMUTABLE bit here, without it the
	   shared array's refcount is decremented on the first write */
	Z_TYPE_FLAGS_P(zv) = IS_TYPE_COPYABLE | IS_TYPE_IMMUTABLE;
#else
	/* 7.2: a COPYABLE-only zval already is Z_IMMUTABLE */
	Z_TYPE_FLAGS_P(zv) = IS_TYPE_COPYABLE;
#endif
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
				pefree(key, yaconf_parse_persistent);
			}
			php_yaconf_zval_dtor(element);
		} ZEND_HASH_FOREACH_END();
		pefree(HT_GET_DATA_ADDR(ht), yaconf_parse_persistent);
	}
	pefree(ht, yaconf_parse_persistent);
} /* }}} */

static void php_yaconf_zval_dtor(zval *pzval) /* {{{ */ {
	switch (Z_TYPE_P(pzval)) {
		case IS_ARRAY:
			php_yaconf_hash_destroy(Z_ARRVAL_P(pzval));
			break;
		case IS_PTR:
		case IS_STRING:
			pefree(Z_PTR_P(pzval), yaconf_parse_persistent);
			break;
		default:
			break;
	}
}
/* }}} */

static zend_string* php_yaconf_str_persistent(char *str, size_t len) /* {{{ */ {
	zend_string *key = zend_string_init(str, len, yaconf_parse_persistent);
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

static int yaconf_hash_need_detach(HashTable *ht) /* {{{ */ {
	/* new key into a block table: the compacted region holds a full
	   nTableSize bucket area (see yaconf_compact_copy_ht), so inserts
	   stay inside it until the table grows full.  Detach only when
	   this write would spill past that region or trigger a resize
	   whose pefree() would free block memory; empty tables carry no
	   buckets at all, so their first insert must detach too */
	if (UNEXPECTED(yaconf_data_in_block(ht)) &&
			(ht->nNumUsed == 0 || ht->nNumUsed >= ht->nTableSize)) {
		return 1;
	}

	return 0;
}
/* }}} */

static zval* php_yaconf_symtable_update(HashTable *ht, char *key, size_t len, zval *zv) /* {{{ */ {
	zend_ulong idx;
	zval *element;
	
	if (UNEXPECTED(ZEND_HANDLE_NUMERIC_STR(key, len, idx))) {
		if ((element = zend_hash_index_find(ht, idx))) {
			/* a value still living in the compacted block is freed with the
			   whole block at MSHUTDOWN; anything else needs an explicit dtor */
			if (!yaconf_value_in_block(element)) {
				php_yaconf_zval_dtor(element);
			}
			ZVAL_COPY_VALUE(element, zv);
		} else {
			if (UNEXPECTED(yaconf_hash_need_detach(ht))) {
				yaconf_ht_detach(ht);
			}
			element = zend_hash_index_add(ht, idx, zv);
		}
	} else {
		if ((element = zend_hash_str_find(ht, key, len))) {
			if (!yaconf_value_in_block(element)) {
				php_yaconf_zval_dtor(element);
			}
			ZVAL_COPY_VALUE(element, zv);
		} else {
			if (UNEXPECTED(yaconf_hash_need_detach(ht))) {
				yaconf_ht_detach(ht);
			}
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
				php_yaconf_zval_dtor(pzval);
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
					php_yaconf_zval_dtor(parent);
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
			if ((parent = zend_symtable_str_find(Z_ARRVAL_P(target), section, sec_len))) {
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
		return 1;
	}
	return 0;
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
					return NULL;
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

static void php_yaconf_handle_file(const char *fullpath, const char *relpath, size_t relpath_len, const char *name, size_t key_len, HashTable *container, time_t mtime, int is_initial) /* {{{ */ {
	/* load one ".ini" file, keyed by the basename without the ".ini" suffix */
	yaconf_filenode *node;
	zval result;

	/* name clash with a same-named directory, the directory wins and the file is skipped */
	if (relpath_len > 4 && zend_hash_str_find_ptr(parsed_ini_dirs, relpath, relpath_len - 4)) {
		php_error(E_WARNING,
				"yaconf: name conflict between config file '%s' and a directory with the same name", relpath);
		return;
	}

	node = (yaconf_filenode*)zend_hash_str_find_ptr(parsed_ini_files, relpath, relpath_len);
	if (node && node->mtime == mtime) {
		return; /* unchanged */
	}

	if (!php_yaconf_parse_ini_file(fullpath, &result)) {
		return;
	}

	/* symtable_update replaces (and destroys) any previous value for this key */
	php_yaconf_symtable_update(container, (char*)name, key_len, &result);

	if (node) {
		node->mtime = mtime;
	} else {
		yaconf_filenode n;
		n.filename = zend_string_init(relpath, relpath_len, 1);
		n.mtime = mtime;
		zend_hash_update_mem(parsed_ini_files, n.filename, &n, sizeof(yaconf_filenode));
	}
}
/* }}} */

static void php_yaconf_handle_directory(const char *fullpath, const char *relpath, size_t relpath_len, const char *name, size_t name_len, HashTable *container, time_t mtime, int is_initial, int depth) /* {{{ */ {
	/* link an immutable persistent container for a sub-directory into the parent, then recurse */
	yaconf_dirnode *node;
	char file_relpath[MAXPATHLEN + 8];
	size_t file_relpath_len;

	/* a same-named ".ini" file loses to the directory: drop its registry entry,
	   the stale container under this key is replaced by the symtable_update below */
	file_relpath_len = snprintf(file_relpath, sizeof(file_relpath), "%s.ini", relpath);
	zend_hash_str_del(parsed_ini_files, file_relpath, file_relpath_len);

	if ((node = (yaconf_dirnode*)zend_hash_str_find_ptr(parsed_ini_dirs, relpath, relpath_len)) != NULL) {
		/* already tracked */
		return;
	}

	{
		zval dirzv;
		HashTable *dir_ht;
		yaconf_dirnode n;

		php_yaconf_hash_init(&dirzv, 8);
		dir_ht = Z_ARRVAL(dirzv);

		php_yaconf_symtable_update(container, (char*)name, name_len, &dirzv);

		n.dirname = zend_string_init(relpath, relpath_len, 1);
		n.mtime = mtime;
		n.container = dir_ht;
		zend_hash_update_mem(parsed_ini_dirs, n.dirname, &n, sizeof(yaconf_dirnode));

		php_yaconf_scan_directory(fullpath, relpath, relpath_len, dir_ht, is_initial, depth + 1);
	}
}
/* }}} */

static int php_yaconf_scan_directory(const char *dirpath, const char *relpath, size_t relpath_len, HashTable *container, int is_initial, int depth) /* {{{ */ {
	/* recursively load ".ini" files and sub-directories into container, relpath is relative to yaconf.directory ("" for the root) */
	int ndir;
	struct dirent **namelist;
	uint32_t i;

	if (depth > YACONF_MAX_DIR_DEPTH) {
		php_error(is_initial ? E_ERROR : E_WARNING,
				"yaconf: directory nesting deeper than %d levels at '%s', skipping", YACONF_MAX_DIR_DEPTH, dirpath);
		return 0;
	}

	if ((ndir = php_scandir(dirpath, &namelist, 0, php_alphasort)) <= 0) {
		return ndir;
	}

	for (i = 0; i < (uint32_t)ndir; i++) {
		char *name = namelist[i]->d_name;
		size_t name_len = strlen(name);
		char fullpath[MAXPATHLEN];
		char sub_relpath[MAXPATHLEN];
		size_t sub_relpath_len;
		zend_stat_t sb = {0};

		if (name[0] == '.' && (name_len == 1 || (name_len == 2 && name[1] == '.'))) {
			free(namelist[i]);
			continue;
		}

		snprintf(fullpath, sizeof(fullpath), "%s%c%s", dirpath, DEFAULT_SLASH, name);
		if (VCWD_STAT(fullpath, &sb) != 0) {
			/* vanished between scandir() and stat() */
			free(namelist[i]);
			continue;
		}

		if (relpath_len) {
			sub_relpath_len = snprintf(sub_relpath, sizeof(sub_relpath), "%s/%s", relpath, name);
		} else {
			sub_relpath_len = snprintf(sub_relpath, sizeof(sub_relpath), "%s", name);
		}

		if (S_ISDIR(sb.st_mode)) {
			php_yaconf_handle_directory(fullpath, sub_relpath, sub_relpath_len, name, name_len, container, sb.st_mtime, is_initial, depth);
		} else if (S_ISREG(sb.st_mode)) {
			char *dot;
			if ((dot = strrchr(name, '.')) && strcmp(dot, ".ini") == 0) {
				php_yaconf_handle_file(fullpath, sub_relpath, sub_relpath_len, name, (size_t)(dot - name), container, sb.st_mtime, is_initial);
			}
		}
		free(namelist[i]);
	}
	free(namelist);
	return ndir;
}
/* }}} */

static void php_yaconf_check_directories(const char *root) /* {{{ */ {
	/* stat every tracked sub-directory and re-scan the changed ones, changes inside a sub-directory do not bump the root's mtime */
	yaconf_dirnode **snapshot;
	yaconf_dirnode *node;
	uint32_t count, i, n = 0;

	if (!parsed_ini_dirs || (count = zend_hash_num_elements(parsed_ini_dirs)) == 0) {
		return;
	}

	/* re-scanning can add new dirnodes, so iterate over a snapshot */
	snapshot = (yaconf_dirnode**)emalloc(count * sizeof(yaconf_dirnode*));
	ZEND_HASH_FOREACH_PTR(parsed_ini_dirs, node) {
		snapshot[n++] = node;
	} ZEND_HASH_FOREACH_END();

	for (i = 0; i < n; i++) {
		char fullpath[MAXPATHLEN];
		zend_stat_t sb = {0};
		int depth = 1;
		const char *p;

		node = snapshot[i];
		for (p = ZSTR_VAL(node->dirname); *p; p++) {
			if (*p == '/') {
				depth++;
			}
		}

		snprintf(fullpath, sizeof(fullpath), "%s%c%s", root, DEFAULT_SLASH, ZSTR_VAL(node->dirname));
		if (VCWD_STAT(fullpath, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
			/* removed, keep serving the stale config */
			continue;
		}

		if (sb.st_mtime != node->mtime) {
			node->mtime = sb.st_mtime;
			php_yaconf_scan_directory(fullpath, ZSTR_VAL(node->dirname), ZSTR_LEN(node->dirname), node->container, 0, depth);
		}
	}
	efree(snapshot);
}
/* }}} */

static void yaconf_ht_detach(HashTable *ht) /* {{{ */ {

	/*  detach a block-resident HashTable's data region to the persistent heap
	 *
	 * Copy the compacted region into an independent pemalloc() allocation so the
	 * engine may resize it on its own: the growth path in zend_hash_do_resize()
	 * reallocates through the table's persistent flag and frees the old region,
	 * which is only legal once that region is no longer part of the block.
	 *
	 * The region is copied as-is.  yaconf_compact_copy_ht already stores every
	 * table in canonical engine layout (hash slots followed by a full nTableSize
	 * bucket area, an empty one is hash-region-only), and hash chains reference
	 * buckets by relative index, so the copy relocates the table intact.
	 *
	 * The old block region turns into dead bytes, reclaimed with the whole
	 * block at MSHUTDOWN.
	 */
	char *data;

	data = (char*)pemalloc(HT_HASH_SIZE(ht->nTableMask) + HT_DATA_SIZE(ht->nTableSize), 1);
	if (UNEXPECTED(!data)) {
		zend_error(E_ERROR, "yaconf: out of memory detaching config table");
	}

	memcpy(data, HT_GET_DATA_ADDR(ht), HT_HASH_SIZE(ht->nTableMask) + HT_DATA_SIZE(ht->nTableSize));
	HT_SET_DATA_ADDR(ht, data);
} /* }}} */

static void yaconf_compact_collect_zval(zval *zv, HashTable *str_set, HashTable *ht_set) /* {{{ */ {
	/* collect all unique strings and HashTable pointers from the tree */
	if (Z_TYPE_P(zv) == IS_STRING) {
		zend_string *str = Z_STR_P(zv);
		zend_hash_str_update_ptr(str_set, ZSTR_VAL(str), ZSTR_LEN(str), str);
	} else if (Z_TYPE_P(zv) == IS_ARRAY) {
		HashTable *ht = Z_ARRVAL_P(zv);
		zend_string *key;
		zval *val;

		/* the parsed config is strictly a tree, never a DAG: every container
		   comes from php_yaconf_hash_init() and section inheritance deep
		   copies, so each table is reached exactly once — no visited check.
		   ht_set only feeds step 7 (freeing the old structs) */
		zend_hash_index_update_ptr(ht_set, (zend_ulong)(uintptr_t)ht, ht);
		ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, val) {
			if (key) {
				zend_hash_str_update_ptr(str_set, ZSTR_VAL(key), ZSTR_LEN(key), key);
			}
			yaconf_compact_collect_zval(val, str_set, ht_set);
		} ZEND_HASH_FOREACH_END();
	}
} /* }}} */

static void yaconf_compact_calc_ht(HashTable *ht, size_t *total) /* {{{ */ {
	/* calc size of one HashTable: struct + data region */
	*total += YACONF_ALIGNED_SIZE(sizeof(zend_array));

	if (!HT_IS_INITIALIZED(ht) || ht->nNumUsed == 0) {
		/* empty or lazy table: a full-sized hash region is still required —
		   a zeroed slot would make lookups loop forever */
		*total += YACONF_ALIGNED_SIZE(HT_HASH_SIZE(ht->nTableMask));
		return;
	}

	/* ref: zend_persist_calc.c zend_hash_persist_calc; the bucket area is
	   grown to nTableSize (not nNumUsed) so reloads can add new keys in
	   place without touching the block until the table actually fills up */
	*total += YACONF_ALIGNED_SIZE(HT_HASH_SIZE(ht->nTableMask) + (size_t)ht->nTableSize * sizeof(Bucket));
} /* }}} */

static zend_always_inline zend_string *yaconf_compact_str(HashTable *str_xlat, zend_string *s) /* {{{ */ {
	/* helper: remap a string via content lookup */
	return zend_hash_str_find_ptr(str_xlat, ZSTR_VAL(s), ZSTR_LEN(s));
} /* }}} */

static zend_always_inline HashTable *yaconf_compact_ht(HashTable *ht_xlat, HashTable *ht) /* {{{ */ {
	/* helper: remap a HashTable via old-ptr lookup */
	return zend_hash_index_find_ptr(ht_xlat, (zend_ulong)(uintptr_t)ht);
} /* }}} */

static void yaconf_compact_copy_ht(HashTable *src, char **cursor, HashTable *str_xlat, HashTable *ht_xlat) /* {{{ */ {
	/* copy one HashTable (and its children) into the block, remap pointers */
	zend_array *dst;
	uint32_t i;
	Bucket *dst_bucket;

	/* strictly a tree (see yaconf_compact_collect_zval), each table is
	   visited exactly once — no "already copied" check */

	/* copy zend_array struct */
	dst = (zend_array*)*cursor;
	*cursor += YACONF_ALIGNED_SIZE(sizeof(zend_array));
	memcpy(dst, src, sizeof(zend_array));
	zend_hash_index_update_ptr(ht_xlat, (zend_ulong)(uintptr_t)src, dst);

	/* the engine dup()s these tables on first write (SEPARATE_ARRAY): PHP 7.0's
	   zend_array_dup() inherits source->pDestructor, and a dup destroyed with a
	   NULL destructor skips every value dtor, leaking the zend_reference that
	   reference-iteration (foreach &$v) creates in the buckets.  Block tables
	   themselves are never destroyed individually (the whole block is pefree()d
	   at MSHUTDOWN), so setting the dtor here only affects the dup; >= 7.1
	   zend_array_dup() overwrites it with ZVAL_PTR_DTOR anyway (php commit
	   b711a96). */
	dst->pDestructor = ZVAL_PTR_DTOR;
	dst->nInternalPointer = HT_INVALID_IDX;

	if (UNEXPECTED(!HT_IS_INITIALIZED(src) || src->nNumUsed == 0)) {
		/* empty or lazy table: a full-sized region of HT_INVALID_IDX slots,
		   see yaconf_compact_calc_ht */
		size_t hash_bytes = HT_HASH_SIZE(src->nTableMask);
		char *data = *cursor;
		*cursor += YACONF_ALIGNED_SIZE(hash_bytes);
		memset(data, HT_INVALID_IDX, hash_bytes);
		HT_SET_DATA_ADDR(dst, data);
#if PHP_VERSION_ID >= 70400
		/* lazy tables arrive with HASH_FLAG_UNINITIALIZED set; the region is
		   real now, so clear it — otherwise an engine access path that
		   real-inits empty tables would reallocate block memory */
		dst->u.flags &= ~HASH_FLAG_UNINITIALIZED;
#endif
		return;
	} else {
		/* copy the data region as-is; the engine's canonical layout already
		   reserves a full nTableSize bucket area, keep the trailing slack
		   buckets so reloads can add keys in place (see calc_ht) */
		size_t region_size = HT_HASH_SIZE(src->nTableMask) + HT_DATA_SIZE(src->nTableSize);
		char *data = *cursor;
		*cursor += YACONF_ALIGNED_SIZE(region_size);
		memcpy(data, HT_GET_DATA_ADDR(src), region_size);
		HT_SET_DATA_ADDR(dst, data);

		/* remap pointers in the copied buckets */
		dst_bucket = dst->arData;
		for (i = 0; i < src->nNumUsed; i++) {
			if (Z_TYPE(dst_bucket[i].val) == IS_UNDEF) continue;

			if (dst_bucket[i].key) {
				dst_bucket[i].key = yaconf_compact_str(str_xlat, dst_bucket[i].key);
			}
			if (Z_TYPE(dst_bucket[i].val) == IS_STRING) {
				Z_STR(dst_bucket[i].val) = yaconf_compact_str(str_xlat, Z_STR(dst_bucket[i].val));
			} else if (Z_TYPE(dst_bucket[i].val) == IS_ARRAY) {
				yaconf_compact_copy_ht(Z_ARRVAL(dst_bucket[i].val), cursor, str_xlat, ht_xlat);
				Z_ARRVAL(dst_bucket[i].val) = yaconf_compact_ht(ht_xlat, Z_ARRVAL(dst_bucket[i].val));
			}
		}
	}
} /* }}} */

static int yaconf_compact(void) /* {{{ */ {
	/* main entry: compact the parsed tree into one block */
	HashTable str_set, ht_set, str_xlat, ht_xlat;
	zend_string *str;
	HashTable *ht;
	char *cursor;
	size_t total = 0;
	yaconf_dirnode *node;

	if (!ini_containers) {
		return 1;
	}

	/* step 1: collect all strings and HashTables */
	zend_hash_init(&str_set, 64, NULL, NULL, 0);
	zend_hash_init(&ht_set, 16, NULL, NULL, 0);

	{
		zend_string *key;
		zval *val;
		zend_hash_index_update_ptr(&ht_set, (zend_ulong)(uintptr_t)ini_containers, ini_containers);
		ZEND_HASH_FOREACH_STR_KEY_VAL(ini_containers, key, val) {
			if (key) {
				zend_hash_str_update_ptr(&str_set, ZSTR_VAL(key), ZSTR_LEN(key), key);
			}
			yaconf_compact_collect_zval(val, &str_set, &ht_set);
		} ZEND_HASH_FOREACH_END();
	}

	/* step 2: calculate total size */
	ZEND_HASH_FOREACH_PTR(&str_set, str) {
		total += YACONF_ALIGNED_SIZE(ZEND_MM_ALIGNED_SIZE(_ZSTR_STRUCT_SIZE(ZSTR_LEN(str))));
	} ZEND_HASH_FOREACH_END();

	ZEND_HASH_FOREACH_PTR(&ht_set, ht) {
		yaconf_compact_calc_ht(ht, &total);
	} ZEND_HASH_FOREACH_END();

	/* step 3: allocate the block */
	yaconf_block = (char*)pemalloc(total, 1);
	if (!yaconf_block) {
		zend_hash_destroy(&str_set);
		zend_hash_destroy(&ht_set);
		return 0;
	}
	yaconf_block_size = total;
	cursor = yaconf_block;

	/* step 4: copy strings into the block, build str_xlat (content → new ptr) */
	zend_hash_init(&str_xlat, zend_hash_num_elements(&str_set), NULL, NULL, 0);
	ZEND_HASH_FOREACH_PTR(&str_set, str) {
		size_t alloc_size = ZEND_MM_ALIGNED_SIZE(_ZSTR_STRUCT_SIZE(ZSTR_LEN(str)));
		zend_string *new_str = (zend_string*)cursor;

		memcpy(new_str, str, alloc_size);
#if PHP_VERSION_ID >= 70300
		GC_SET_REFCOUNT(new_str, 1);
		GC_TYPE_INFO(new_str) = GC_TYPE_INFO(str);
#else
		GC_REFCOUNT(new_str) = 1;
#endif
		new_str->h = str->h;
		cursor += YACONF_ALIGNED_SIZE(alloc_size);

		zend_hash_str_update_ptr(&str_xlat, ZSTR_VAL(new_str), ZSTR_LEN(new_str), new_str);
	} ZEND_HASH_FOREACH_END();

	/* step 5-6: copy HashTables, update ini_containers and dirnode containers */
	zend_hash_init(&ht_xlat, zend_hash_num_elements(&ht_set), NULL, NULL, 0);
	yaconf_compact_copy_ht(ini_containers, &cursor, &str_xlat, &ht_xlat);
	ini_containers = yaconf_compact_ht(&ht_xlat, ini_containers);

	/* the block lives past any request, so flag every block table persistent:
	   when a detached table is later grown by the engine's own resize path,
	   pemalloc/pefree must go through the persistent allocator */
	{
		HashTable *tmp;
		ZEND_HASH_FOREACH_PTR(&ht_xlat, tmp) {
			if (tmp) {
#if PHP_VERSION_ID >= 70300
				GC_ADD_FLAGS(tmp, IS_ARRAY_PERSISTENT);
#else
				tmp->u.flags |= HASH_FLAG_PERSISTENT;
#endif
			}
		} ZEND_HASH_FOREACH_END();
	}

	if (parsed_ini_dirs) {
		ZEND_HASH_FOREACH_PTR(parsed_ini_dirs, node) {
			HashTable *new_container = yaconf_compact_ht(&ht_xlat, node->container);
			if (new_container) {
				node->container = new_container;
			}
		} ZEND_HASH_FOREACH_END();
	}

	/* step 7: free the old individually-allocated tree */
	{
		ZEND_HASH_FOREACH_PTR(&str_set, str) {
			pefree(str, 0);
		} ZEND_HASH_FOREACH_END();

		ZEND_HASH_FOREACH_PTR(&ht_set, ht) {
			if (ht != ini_containers && HT_IS_INITIALIZED(ht) && ht->nNumUsed > 0) {
				pefree(HT_GET_DATA_ADDR(ht), 0);
			}
			pefree(ht, 0);
		} ZEND_HASH_FOREACH_END();
	}

	zend_hash_destroy(&str_set);
	zend_hash_destroy(&ht_set);
	zend_hash_destroy(&str_xlat);
	zend_hash_destroy(&ht_xlat);

	return 1;
} /* }}} */

static void yaconf_compact_free(void) /* {{{ */ {
	/* free the compacted block */
	if (yaconf_block) {
		pefree(yaconf_block, 1);
		yaconf_block = NULL;
		yaconf_block_size = 0;
	}
} /* }}} */

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
		char address[sizeof(void*) * 2 + 3];
		size_t len;

		array_init(return_value);
		ZVAL_STR(&zv, name);

		zend_hash_str_add_new(Z_ARRVAL_P(return_value), "key", sizeof("key") - 1, &zv);
		Z_TRY_ADDREF(zv);

		/* stored values are interned strings or immutable arrays only, Z_PTR_P gets the value's address */
		len = sprintf(address, "%p", Z_PTR_P(val));
		ZVAL_STR(&zv, zend_string_init(address, len, 0));

		zend_hash_str_add_new(Z_ARRVAL_P(return_value), "address", sizeof("address") - 1, &zv);
		zend_hash_str_add_new(Z_ARRVAL_P(return_value), "val", sizeof("val") - 1, val);
		Z_TRY_ADDREF_P(val);

		/* changed: false when the stored value's data still lives inside the compacted block,
		 *           meaning the OS hasn't been forced to copy the page (COW works).
		 *           true when the value has been reallocated outside the block (e.g. RINIT reload). */
		if (yaconf_block && Z_TYPE_P(val) != IS_NULL) {
			uintptr_t block_start = (uintptr_t)yaconf_block;
			uintptr_t block_end   = block_start + yaconf_block_size;
			uintptr_t val_ptr     = (uintptr_t)Z_PTR_P(val);
			ZVAL_BOOL(&zv, !(val_ptr >= block_start && val_ptr < block_end));
		} else {
			ZVAL_BOOL(&zv, 1);
		}
		zend_hash_str_add_new(Z_ARRVAL_P(return_value), "changed", sizeof("changed") - 1, &zv);

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
	zend_class_entry ce;
	zend_stat_t dir_sb = {0};

	REGISTER_INI_ENTRIES();

	INIT_CLASS_ENTRY(ce, "Yaconf", yaconf_methods);

	yaconf_ce = zend_register_internal_class_ex(&ce, NULL);

	if ((dirname = YACONF_G(directory)) && strlen(dirname)
#ifndef ZTS
			&& !VCWD_STAT(dirname, &dir_sb) && S_ISDIR(dir_sb.st_mode)
#endif
			) {
#ifndef ZTS
		YACONF_G(directory_mtime) = dir_sb.st_mtime;
#endif

		/* Phase 1: parse with temporary memory (emalloc) */
		yaconf_parse_persistent = 0;

		ini_containers = (HashTable*)pemalloc(sizeof(HashTable), 0);
		zend_hash_init(ini_containers, 8, NULL, NULL, 0);

		/* parsed_ini_files and parsed_ini_dirs are always persistent (used across requests) */
		PALLOC_HASHTABLE(parsed_ini_files);
		zend_hash_init(parsed_ini_files, 8, NULL, NULL, 1);

		PALLOC_HASHTABLE(parsed_ini_dirs);
		zend_hash_init(parsed_ini_dirs, 8, NULL, NULL, 1);

		if (php_yaconf_scan_directory(dirname, "", 0, ini_containers, 1, 0) <= 0) {
			php_error(E_ERROR, "Couldn't opendir '%s'", dirname);
		}

		/* Phase 2: compact everything into one persistent block */
		yaconf_parse_persistent = 1;
		yaconf_compact();

#ifndef ZTS
		YACONF_G(last_check) = time(NULL);
#endif
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

		if (ini_containers && (dirname = YACONF_G(directory)) && !VCWD_STAT(dirname, &dir_sb) && S_ISDIR(dir_sb.st_mode)) {
			if (dir_sb.st_mtime != YACONF_G(directory_mtime)) {
				YACONF_DEBUG("config directory modified, re-scan the root level");
				YACONF_G(directory_mtime) = dir_sb.st_mtime;
				php_yaconf_scan_directory(dirname, "", 0, ini_containers, 0, 0);
			}
			/* changes inside a sub-directory do not bump the root's mtime, check them individually */
			php_yaconf_check_directories(dirname);
		} else {
			YACONF_DEBUG("stat config directory failed");
		}
	}

	return SUCCESS;
}
/* }}} */
#endif

static void yaconf_containers_destroy(HashTable *ht) /* {{{ */ {
	/* free the containers tree: block-resident parts go with the block,
	   detached data regions, heap structs and reloaded values are freed here */
	zend_string *key;
	zval *element;
	int detached_struct = !yaconf_ptr_in_block(ht);

	ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, element) {
		/* keys are individually pemalloc'd (flagged interned for the engine's
		   sake), only block copies live in the block and are skipped */
		if (key && !yaconf_ptr_in_block(key)) {
			pefree(key, 1);
		}
		if (!yaconf_value_in_block(element)) {
			php_yaconf_zval_dtor(element);
		}
	} ZEND_HASH_FOREACH_END();

	if (!yaconf_ptr_in_block(HT_GET_DATA_ADDR(ht))) {
		/* the data region was detached or belongs to a reloaded heap table:
		   its slots/buckets are already freed above, free the region itself */
		pefree(HT_GET_DATA_ADDR(ht), 1);
	}
	if (detached_struct) {
		pefree(ht, 1);
	}
} /* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(yaconf)
{
	UNREGISTER_INI_ENTRIES();

	if (parsed_ini_dirs) {
		php_yaconf_hash_destroy(parsed_ini_dirs);
	}

	if (parsed_ini_files) {
		php_yaconf_hash_destroy(parsed_ini_files);
	}

	if (ini_containers) {
		yaconf_containers_destroy(ini_containers);
		ini_containers = NULL;
		yaconf_compact_free();
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

	php_info_print_table_start();
	php_info_print_table_header(2, "config sub-directory", "mtime");
	if (parsed_ini_dirs && zend_hash_num_elements(parsed_ini_dirs)) {
		yaconf_dirnode *node;
		ZEND_HASH_FOREACH_PTR(parsed_ini_dirs, node) {
			php_info_print_table_row(2, ZSTR_VAL(node->dirname),  ctime(&node->mtime));
		} ZEND_HASH_FOREACH_END();
	}
	php_info_print_table_end();

	php_info_print_table_start();
	php_info_print_table_header(2, "compacted block", "value");
	if (yaconf_block) {
		char buf[128];
		uint32_t file_count = parsed_ini_files ? zend_hash_num_elements(parsed_ini_files) : 0;
		snprintf(buf, sizeof(buf), "%u file(s)", file_count);
		php_info_print_table_row(2, "compacted files", buf);
		snprintf(buf, sizeof(buf), "%zu bytes (%.1f KB)", yaconf_block_size, (double)yaconf_block_size / 1024.0);
		php_info_print_table_row(2, "compacted size", buf);
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
