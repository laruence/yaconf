--TEST--
Yaconf section inheritance with numeric section names
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/024/
--FILE--
<?php
// inherit.ini:
//   [1] inherited=parent_value, shared=from_parent
//   [2:1] own=child_own, shared=from_child
//   [base] parent_key=base_value
//   [child:base] child_key=derived_value
//
// Control cases: string section names always worked
var_dump(Yaconf::get("inherit.child.parent_key"));
var_dump(Yaconf::get("inherit.base.parent_key"));
var_dump(Yaconf::get("inherit.2.own"));
var_dump(Yaconf::get("inherit.2.shared"));
var_dump(Yaconf::get("inherit.1.shared"));

// Bug case: the parent lookup in php_yaconf_ini_parser_cb() used
// zend_hash_str_find(), which never sees numeric section names,
// because php_yaconf_symtable_update() stores them as index entries.
// [2:1] must inherit from section [1].
var_dump(Yaconf::get("inherit.2.inherited"));
var_dump(Yaconf::get("inherit.1"));
var_dump(Yaconf::get("inherit.2"));
?>
--EXPECTF--
string(10) "base_value"
string(10) "base_value"
string(9) "child_own"
string(10) "from_child"
string(11) "from_parent"
string(12) "parent_value"
array(2) {
  ["inherited"]=>
  string(12) "parent_value"
  ["shared"]=>
  string(11) "from_parent"
}
array(3) {
  ["inherited"]=>
  string(12) "parent_value"
  ["shared"]=>
  string(10) "from_child"
  ["own"]=>
  string(9) "child_own"
}
