--TEST--
Yaconf::get() default value types
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/015
--FILE--
<?php
// No default arg → null
var_dump(Yaconf::get("defaults.nonexist"));

// String default
var_dump(Yaconf::get("defaults.nonexist", "hello"));

// Int default
var_dump(Yaconf::get("defaults.nonexist", 42));

// Bool default
var_dump(Yaconf::get("defaults.nonexist", true));
var_dump(Yaconf::get("defaults.nonexist", false));

// Array default
var_dump(Yaconf::get("defaults.nonexist", array("a" => 1)));

// Null default (explicit)
var_dump(Yaconf::get("defaults.nonexist", null));

// Existing value: default must NOT be returned
var_dump(Yaconf::get("defaults.real", "should_not_appear"));
?>
--EXPECTF--
NULL
string(5) "hello"
int(42)
bool(true)
bool(false)
array(1) {
  ["a"]=>
  int(1)
}
NULL
string(5) "hello"
