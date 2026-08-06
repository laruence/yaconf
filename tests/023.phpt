--TEST--
Yaconf::get() with path segments beyond an existing scalar value
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/023/
--FILE--
<?php
// prefix.ini provides:
//   scalar="hello", deep.a.b="leaf", list[0]="a", list[1]="b",
//   [section] key="value", num=42
//
// Control cases: behave the same before and after the fix.
var_dump(Yaconf::get("prefix.scalar"));
var_dump(Yaconf::get("prefix.section.key"));
var_dump(Yaconf::get("prefix.section.num"));
var_dump(Yaconf::get("prefix.deep.a.b"));
var_dump(Yaconf::get("prefix.list.0"));
var_dump(Yaconf::get("prefix.section.nope"));
var_dump(Yaconf::get("prefix.nope.deep"));

// Bug cases: the requested path goes DEEPER than an existing scalar
// value. Keys like "prefix.scalar.deeper" do not exist, so get() must
// return the default and has() must return false. Currently the lookup
// stops at the scalar prefix and returns it, swallowing the rest of
// the path (php_yaconf_get(): the "!IS_ARRAY" branch returns the found
// value even though more segments follow).
var_dump(Yaconf::get("prefix.scalar.deeper"));
var_dump(Yaconf::get("prefix.scalar.deeper", "fallback"));
var_dump(Yaconf::has("prefix.scalar.deeper"));
var_dump(Yaconf::get("prefix.section.key.deeper"));
var_dump(Yaconf::get("prefix.section.num.deeper"));
var_dump(Yaconf::get("prefix.deep.a.b.deeper"));
var_dump(Yaconf::get("prefix.list.0.deeper"));
?>
--EXPECTF--
string(5) "hello"
string(5) "value"
string(2) "42"
string(4) "leaf"
string(1) "a"
NULL
NULL
NULL
string(8) "fallback"
bool(false)
NULL
NULL
NULL
NULL
