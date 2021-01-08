/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: c72f0ce39e6e3bea0a9abce64ba099a5923c45d7 */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_yaconf_get, 0, 1, MAY_BE_STRING|MAY_BE_NULL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, default, IS_MIXED, 0, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_yaconf_has, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()


ZEND_METHOD(yaconf, get);
ZEND_METHOD(yaconf, has);


static const zend_function_entry class_yaconf_methods[] = {
	ZEND_ME(yaconf, get, arginfo_class_yaconf_get, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(yaconf, has, arginfo_class_yaconf_has, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_FE_END
};
